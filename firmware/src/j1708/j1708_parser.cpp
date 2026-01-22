/**
 * @file j1708_parser.cpp
 * @brief J1708/J1587 message parser implementation
 * 
 * Implements parsing of J1708 serial messages per SAE J1587 application layer.
 */

#include "j1708_parser.h"
#include <string.h>

#ifndef NATIVE_BUILD
#include <Arduino.h>
#else
static uint32_t _test_millis = 0;
uint32_t millis(void);
#endif

// Inter-byte timeout for message framing (2 bit times at 9600 = ~2ms)
#define J1708_INTER_BYTE_TIMEOUT_MS  10  // Use 10ms for safety margin

/*===========================================================================*/
/*                        PID LENGTH TABLE                                  */
/*===========================================================================*/

/**
 * @brief PID length lookup table
 * 
 * Most J1587 PIDs have fixed lengths. PIDs 0-127 typically have variable length
 * with a length byte. PIDs 128-255 and 256-511 have fixed lengths per J1587.
 */
typedef struct {
    uint8_t pid;
    uint8_t length;     // 0 = variable (has length prefix), 0xFF = unknown
} pid_length_entry_t;

static const pid_length_entry_t pid_lengths[] = {
    // Common fixed-length PIDs
    { 84,  1 },     // Road Speed (0.5 mph/bit)
    { 85,  1 },     // Vehicle Speed Sensor
    { 86,  1 },     // Cruise Control Set Speed
    { 91,  1 },     // Throttle Position (0.4%/bit)
    { 92,  1 },     // Percent Load at Current RPM
    { 96,  1 },     // Fuel Level 1 (0.5%/bit)
    { 97,  1 },     // Fuel Level 2
    { 100, 1 },     // Engine Oil Pressure (4 kPa/bit)
    { 102, 1 },     // Boost Pressure (2 kPa/bit)
    { 105, 1 },     // Intake Manifold Temperature
    { 108, 1 },     // Barometric Pressure (0.5 kPa/bit)
    { 110, 1 },     // Engine Coolant Temperature
    { 168, 1 },     // Battery Voltage (0.05V/bit)
    { 171, 1 },     // Ambient Air Temperature
    { 174, 1 },     // Fuel Temperature
    { 175, 1 },     // Engine Oil Temperature
    { 177, 2 },     // Transmission Oil Temperature (16-bit)
    { 178, 1 },     // Transmission Oil Pressure
    { 183, 2 },     // Fuel Rate (16-bit)
    { 184, 2 },     // Instantaneous Fuel Economy
    { 190, 2 },     // Engine Speed (16-bit, 0.25 rpm/bit)
    { 191, 2 },     // Transmission Output Shaft Speed
    { 194, 0 },     // Diagnostic Codes (variable)
    { 195, 0 },     // Previously Active Codes (variable)
    { 233, 0 },     // Software ID (variable)
    { 234, 0 },     // Component ID (variable)
    { 245, 4 },     // Total Vehicle Distance
    { 247, 4 },     // Engine Total Hours
    { 0xFF, 0xFF }  // End marker
};

/*===========================================================================*/
/*                        INITIALIZATION                                    */
/*===========================================================================*/

void j1708_parser_init(j1708_parser_context_t* ctx) {
    if (ctx == NULL) return;
    
    memset(ctx, 0, sizeof(j1708_parser_context_t));
    ctx->state = J1708_RX_IDLE;
}

/*===========================================================================*/
/*                        CHECKSUM FUNCTIONS                                */
/*===========================================================================*/

bool j1708_validate_checksum(const uint8_t* data, uint8_t len) {
    if (data == NULL || len < 2) return false;
    
    // J1708: Sum of all bytes including checksum should equal 0 (mod 256)
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    
    return (sum == 0);
}

uint8_t j1708_calculate_checksum(const uint8_t* data, uint8_t len) {
    if (data == NULL || len == 0) return 0;
    
    // Checksum = two's complement of sum of all bytes
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    
    return (uint8_t)(0x100 - sum);  // Two's complement
}

/*===========================================================================*/
/*                        PID LENGTH LOOKUP                                 */
/*===========================================================================*/

uint8_t j1708_get_pid_length(uint8_t pid) {
    // First, look up in table for known PIDs
    for (int i = 0; pid_lengths[i].pid != 0xFF; i++) {
        if (pid_lengths[i].pid == pid) {
            return pid_lengths[i].length;
        }
    }
    
    // Extended PIDs (192+) that aren't in the table have variable length
    if (pid >= 192 && pid <= 254) {
        return 0;  // Variable length with length prefix
    }
    
    // Unknown PID - assume variable
    return 0;
}

/*===========================================================================*/
/*                        RECEIVE STATE MACHINE                             */
/*===========================================================================*/

bool j1708_receive_byte(j1708_parser_context_t* ctx, uint8_t byte, uint32_t timestamp_ms) {
    if (ctx == NULL) return false;
    
    // Check for inter-message gap (new message start)
    if (ctx->state == J1708_RX_RECEIVING) {
        if ((timestamp_ms - ctx->last_byte_time_ms) > J1708_INTER_BYTE_TIMEOUT_MS) {
            // Gap detected - previous message might be complete or aborted
            if (ctx->buffer_index >= J1708_MIN_MESSAGE_LENGTH) {
                // Validate checksum
                if (j1708_validate_checksum(ctx->buffer, ctx->buffer_index)) {
                    ctx->state = J1708_RX_COMPLETE;
                    ctx->messages_received++;
                    // Don't start new byte yet, let caller get message first
                    return true;
                } else {
                    ctx->checksum_errors++;
                }
            }
            // Start fresh
            ctx->state = J1708_RX_IDLE;
            ctx->buffer_index = 0;
        }
    }
    
    // If we completed a message, don't overwrite until retrieved
    if (ctx->state == J1708_RX_COMPLETE) {
        return true;
    }
    
    // Add byte to buffer
    if (ctx->buffer_index < J1708_MAX_MESSAGE_LENGTH) {
        ctx->buffer[ctx->buffer_index++] = byte;
        ctx->last_byte_time_ms = timestamp_ms;
        ctx->state = J1708_RX_RECEIVING;
    } else {
        // Buffer overflow - reset
        ctx->state = J1708_RX_IDLE;
        ctx->buffer_index = 0;
        ctx->parse_errors++;
    }
    
    return false;
}

bool j1708_get_message(j1708_parser_context_t* ctx, j1708_message_t* msg) {
    if (ctx == NULL || msg == NULL) return false;
    
    if (ctx->state != J1708_RX_COMPLETE) return false;
    
    // Parse the buffered message
    bool result = j1708_parse_message(ctx->buffer, ctx->buffer_index, msg);
    
    // Reset for next message
    ctx->state = J1708_RX_IDLE;
    ctx->buffer_index = 0;
    
    return result;
}

/*===========================================================================*/
/*                        MESSAGE PARSING                                   */
/*===========================================================================*/

bool j1708_parse_message(const uint8_t* data, uint8_t len, j1708_message_t* msg) {
    if (data == NULL || msg == NULL || len < J1708_MIN_MESSAGE_LENGTH) {
        return false;
    }
    
    memset(msg, 0, sizeof(j1708_message_t));
    
    // Copy raw data
    memcpy(msg->raw_data, data, len);
    msg->raw_length = len;
    
    // Validate checksum
    msg->checksum_valid = j1708_validate_checksum(data, len);
    if (!msg->checksum_valid) {
        return false;
    }
    
    // First byte is MID
    msg->mid = data[0];
    
    // Parse PIDs from remaining bytes (excluding last byte = checksum)
    uint8_t offset = 1;
    uint8_t data_end = len - 1;  // Exclude checksum
    
    while (offset < data_end && msg->param_count < J1708_MAX_PIDS) {
        j1587_parameter_t* param = &msg->params[msg->param_count];
        
        param->pid = data[offset++];
        
        // Determine data length
        uint8_t pid_len = j1708_get_pid_length(param->pid);
        
        if (pid_len == 0) {
            // Variable length - next byte is length
            if (offset >= data_end) break;
            pid_len = data[offset++];
        }
        
        // Validate we have enough data
        if (offset + pid_len > data_end) {
            break;  // Not enough data
        }
        
        // Copy parameter data
        param->data_length = pid_len;
        if (pid_len > 0 && pid_len <= 8) {
            memcpy(param->data, &data[offset], pid_len);
            param->is_valid = true;
        }
        
        offset += pid_len;
        msg->param_count++;
    }
    
    return true;
}

/*===========================================================================*/
/*                        PARAMETER DECODING                                */
/*===========================================================================*/

float j1708_decode_road_speed(const uint8_t* data, uint8_t len) {
    // PID 84: Road Speed
    // 1 byte, 0.5 mph/bit, convert to km/h
    if (data == NULL || len < 1) return -1.0f;
    
    float mph = (float)data[0] * 0.5f;
    return mph * 1.60934f;  // Convert to km/h
}

float j1708_decode_engine_rpm(const uint8_t* data, uint8_t len) {
    // PID 190: Engine Speed
    // 2 bytes, little-endian, 0.25 rpm/bit
    if (data == NULL || len < 2) return -1.0f;
    
    uint16_t raw = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    return (float)raw * 0.25f;
}

float j1708_decode_coolant_temp(const uint8_t* data, uint8_t len) {
    // PID 110: Engine Coolant Temperature
    // 1 byte, 1°F/bit, offset 0, convert to °C
    if (data == NULL || len < 1) return -9999.0f;
    
    float fahrenheit = (float)data[0];
    return (fahrenheit - 32.0f) * 5.0f / 9.0f;  // Convert to °C
}

float j1708_decode_oil_pressure(const uint8_t* data, uint8_t len) {
    // PID 100: Engine Oil Pressure
    // 1 byte, 4 kPa/bit (note: some older docs say 0.5 psi/bit)
    if (data == NULL || len < 1) return -1.0f;
    
    return (float)data[0] * 4.0f;  // kPa
}

float j1708_decode_trans_oil_temp(const uint8_t* data, uint8_t len) {
    // PID 177: Transmission Oil Temperature (J1587 definition)
    // 2 bytes, 0.25°C/bit, offset -273.2°C (Kelvin offset)
    // Note: This differs from J1939 SPN 177!
    if (data == NULL || len < 2) return -9999.0f;
    
    uint16_t raw = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    return ((float)raw * 0.25f) - 273.0f;
}

float j1708_decode_battery_voltage(const uint8_t* data, uint8_t len) {
    // PID 168: Battery Voltage
    // 1 byte, 0.05V/bit (some older docs say different scale)
    if (data == NULL || len < 1) return -1.0f;
    
    return (float)data[0] * 0.05f;
}

float j1708_decode_fuel_level(const uint8_t* data, uint8_t len) {
    // PID 96: Fuel Level 1
    // 1 byte, 0.5%/bit
    if (data == NULL || len < 1) return -1.0f;
    
    return (float)data[0] * 0.5f;
}

/*===========================================================================*/
/*                        FAULT CODE PARSING                                */
/*===========================================================================*/

uint8_t j1708_parse_fault_codes(uint8_t mid, const uint8_t* data, uint8_t len,
                                 j1587_fault_code_t* faults, uint8_t max_faults) {
    // PID 194/195 diagnostic code format:
    // Format varies, but typically:
    //   Byte 0: PID (or SID if high bit set)
    //   Byte 1: FMI (failure mode) in lower 4 bits, flags in upper bits
    //   May repeat for multiple codes
    
    if (data == NULL || len < 2 || faults == NULL || max_faults == 0) {
        return 0;
    }
    
    uint8_t fault_count = 0;
    uint8_t offset = 0;
    
    while (offset + 2 <= len && fault_count < max_faults) {
        j1587_fault_code_t* fault = &faults[fault_count];
        
        fault->mid = mid;
        
        // Check if SID (subsystem ID) or PID
        uint8_t id_byte = data[offset];
        if (id_byte & 0x80) {
            // SID format
            fault->is_sid = true;
            fault->pid_or_sid = id_byte & 0x7F;
        } else {
            fault->is_sid = false;
            fault->pid_or_sid = id_byte;
        }
        
        // FMI is in lower 4 bits of next byte (per J1587)
        fault->fmi = data[offset + 1] & 0x0F;
        fault->is_active = true;  // From PID 194
        fault->occurrence_count = 1;  // Not always provided
        
        offset += 2;
        fault_count++;
    }
    
    return fault_count;
}

/*===========================================================================*/
/*                        STRING LOOKUP FUNCTIONS                           */
/*===========================================================================*/

typedef struct {
    uint8_t mid;
    const char* name;
} mid_name_t;

static const mid_name_t mid_names[] = {
    { 128, "Engine #1" },
    { 129, "Engine #2" },
    { 130, "Transmission" },
    { 136, "Trailer #1 ABS" },
    { 137, "Trailer #2 ABS" },
    { 140, "Instrument Cluster" },
    { 142, "Vehicle Management" },
    { 172, "Tractor ABS" },
    { 175, "Tire Pressure Monitor" },
    { 0, NULL }
};

const char* j1708_get_mid_name(uint8_t mid) {
    for (int i = 0; mid_names[i].name != NULL; i++) {
        if (mid_names[i].mid == mid) {
            return mid_names[i].name;
        }
    }
    return "Unknown";
}

typedef struct {
    uint8_t pid;
    const char* name;
} pid_name_t;

static const pid_name_t pid_names[] = {
    { 84,  "Road Speed" },
    { 91,  "Throttle Position" },
    { 92,  "Percent Load" },
    { 96,  "Fuel Level 1" },
    { 100, "Engine Oil Pressure" },
    { 102, "Boost Pressure" },
    { 105, "Intake Manifold Temp" },
    { 110, "Coolant Temperature" },
    { 168, "Battery Voltage" },
    { 171, "Ambient Temperature" },
    { 174, "Fuel Temperature" },
    { 175, "Engine Oil Temperature" },
    { 177, "Trans Oil Temperature" },
    { 183, "Fuel Rate" },
    { 190, "Engine Speed" },
    { 194, "Active Fault Codes" },
    { 195, "Historical Fault Codes" },
    { 0, NULL }
};

const char* j1708_get_pid_name(uint8_t pid) {
    for (int i = 0; pid_names[i].name != NULL; i++) {
        if (pid_names[i].pid == pid) {
            return pid_names[i].name;
        }
    }
    return "Unknown";
}
