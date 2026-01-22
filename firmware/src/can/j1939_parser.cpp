/**
 * @file j1939_parser.cpp
 * @brief J1939 CAN message parser implementation
 * 
 * Implements decoding of J1939 CAN messages including:
 * - PGN extraction from 29-bit CAN IDs
 * - SPN value decoding with scaling/offset
 * - Transport Protocol (BAM) reassembly
 * - DM1/DM2 diagnostic message parsing
 */

#include "j1939_parser.h"
#include <string.h>

#ifndef NATIVE_BUILD
#include <Arduino.h>
#else
// For native testing, provide millis() stub
static uint32_t _test_millis = 0;
uint32_t millis(void) { return _test_millis; }
void test_set_millis(uint32_t ms) { _test_millis = ms; }
#endif

/*===========================================================================*/
/*                        INITIALIZATION                                    */
/*===========================================================================*/

void j1939_parser_init(j1939_parser_context_t* ctx) {
    if (ctx == NULL) return;
    
    memset(ctx, 0, sizeof(j1939_parser_context_t));
    
    // Initialize all TP sessions to idle
    for (int i = 0; i < J1939_MAX_ACTIVE_TP; i++) {
        ctx->tp_sessions[i].state = TP_STATE_IDLE;
    }
}

/*===========================================================================*/
/*                        CAN ID EXTRACTION FUNCTIONS                       */
/*===========================================================================*/

uint32_t j1939_extract_pgn(uint32_t can_id) {
    // CAN ID structure (29 bits):
    // Priority (3) | Reserved (1) | Data Page (1) | PDU Format (8) | PDU Specific (8) | Source Address (8)
    //   bits 28-26 |    bit 25    |    bit 24     |   bits 23-16   |    bits 15-8     |     bits 7-0
    
    uint8_t pdu_format = (can_id >> 16) & 0xFF;    // PDU Format (PF)
    uint8_t pdu_specific = (can_id >> 8) & 0xFF;   // PDU Specific (PS)
    uint8_t data_page = (can_id >> 24) & 0x03;     // Data Page + Reserved
    
    if (pdu_format < 240) {
        // PDU1: PS is destination address, NOT part of PGN
        // PGN = DP (2 bits) + PF (8 bits) + 0x00
        return ((uint32_t)data_page << 16) | ((uint32_t)pdu_format << 8);
    } else {
        // PDU2: PS is group extension, IS part of PGN
        // PGN = DP (2 bits) + PF (8 bits) + PS (8 bits)
        return ((uint32_t)data_page << 16) | ((uint32_t)pdu_format << 8) | pdu_specific;
    }
}

uint8_t j1939_extract_source_address(uint32_t can_id) {
    return can_id & 0xFF;
}

uint8_t j1939_extract_priority(uint32_t can_id) {
    return (can_id >> 26) & 0x07;
}

uint8_t j1939_extract_destination(uint32_t can_id) {
    uint8_t pdu_format = (can_id >> 16) & 0xFF;
    
    if (pdu_format < 240) {
        // PDU1: PS is destination address
        return (can_id >> 8) & 0xFF;
    }
    // PDU2: Broadcast message
    return 0xFF;
}

uint32_t j1939_build_can_id(uint32_t pgn, uint8_t source_address, uint8_t priority) {
    // Build CAN ID from PGN, source address, and priority
    // Note: This assumes PDU2 format (PGN includes all 18 bits)
    return ((uint32_t)(priority & 0x07) << 26) | (pgn << 8) | source_address;
}

/*===========================================================================*/
/*                        FRAME PARSING                                     */
/*===========================================================================*/

bool j1939_parse_frame(uint32_t can_id, const uint8_t* data, uint8_t data_len,
                       uint32_t timestamp_ms, j1939_message_t* msg) {
    if (data == NULL || msg == NULL || data_len == 0 || data_len > 8) {
        return false;
    }
    
    msg->pgn = j1939_extract_pgn(can_id);
    msg->source_address = j1939_extract_source_address(can_id);
    msg->priority = j1939_extract_priority(can_id);
    msg->destination = j1939_extract_destination(can_id);
    msg->data_length = data_len;
    msg->timestamp_ms = timestamp_ms;
    
    memcpy(msg->data, data, data_len);
    
    return true;
}

/*===========================================================================*/
/*                        PARAMETER DECODING                                */
/*===========================================================================*/

float j1939_decode_engine_speed(const uint8_t* data) {
    // PGN 61444 (EEC1) - Engine Speed at bytes 4-5 (indices 3-4)
    // Scale: 0.125 rpm/bit, Offset: 0
    if (data == NULL) return -1.0f;
    
    uint16_t raw = (uint16_t)data[3] | ((uint16_t)data[4] << 8);
    
    if (!j1939_is_valid_16(raw)) return -1.0f;
    
    return (float)raw * 0.125f;
}

float j1939_decode_coolant_temp(const uint8_t* data) {
    // PGN 65262 (ET1) - Engine Coolant Temperature at byte 1 (index 0)
    // Scale: 1 °C/bit, Offset: -40°C
    if (data == NULL) return -9999.0f;
    
    uint8_t raw = data[0];
    
    if (!j1939_is_valid_8(raw)) return -9999.0f;
    
    return (float)raw - 40.0f;
}

float j1939_decode_vehicle_speed(const uint8_t* data) {
    // PGN 65265 (CCVS) - Wheel-Based Vehicle Speed at bytes 2-3 (indices 1-2)
    // Scale: 1/256 km/h per bit = 0.00390625 km/h, Offset: 0
    if (data == NULL) return -1.0f;
    
    uint16_t raw = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
    
    if (!j1939_is_valid_16(raw)) return -1.0f;
    
    return (float)raw / 256.0f;
}

float j1939_decode_oil_pressure(const uint8_t* data) {
    // PGN 65263 (EFLP1) - Engine Oil Pressure at byte 4 (index 3)
    // Scale: 4 kPa/bit, Offset: 0
    if (data == NULL) return -1.0f;
    
    uint8_t raw = data[3];
    
    if (!j1939_is_valid_8(raw)) return -1.0f;
    
    return (float)raw * 4.0f;
}

float j1939_decode_boost_pressure(const uint8_t* data) {
    // PGN 65270 (IC1) - Boost Pressure at byte 2 (index 1)
    // Scale: 2 kPa/bit, Offset: 0
    if (data == NULL) return -1.0f;
    
    uint8_t raw = data[1];
    
    if (!j1939_is_valid_8(raw)) return -1.0f;
    
    return (float)raw * 2.0f;
}

float j1939_decode_fuel_level(const uint8_t* data) {
    // PGN 65276 (DD) - Fuel Level 1 at byte 2 (index 1)
    // Scale: 0.4 %/bit, Offset: 0
    if (data == NULL) return -1.0f;
    
    uint8_t raw = data[1];
    
    if (!j1939_is_valid_8(raw)) return -1.0f;
    
    return (float)raw * 0.4f;
}

float j1939_decode_battery_voltage(const uint8_t* data) {
    // PGN 65271 (VEP1) - Battery Potential at bytes 7-8 (indices 6-7)
    // Scale: 0.05 V/bit, Offset: 0
    if (data == NULL) return -1.0f;
    
    uint16_t raw = (uint16_t)data[6] | ((uint16_t)data[7] << 8);
    
    if (!j1939_is_valid_16(raw)) return -1.0f;
    
    return (float)raw * 0.05f;
}

int8_t j1939_decode_current_gear(const uint8_t* data) {
    // PGN 61445 (ETC2) - Current Gear at byte 4 (index 3)
    // Scale: 1, Offset: -125
    // Values: -125 (reverse) to +125 (forward gears), 251=park, 252=error
    if (data == NULL) return -126;  // Invalid marker
    
    uint8_t raw = data[3];
    
    if (!j1939_is_valid_8(raw)) return -126;
    
    return (int8_t)((int16_t)raw - 125);
}

float j1939_decode_trans_oil_temp(const uint8_t* data) {
    // PGN 65272 (TRF1) - Transmission Oil Temperature at bytes 5-6 (indices 4-5)
    // Scale: 0.03125 °C/bit, Offset: -273°C
    if (data == NULL) return -9999.0f;
    
    uint16_t raw = (uint16_t)data[4] | ((uint16_t)data[5] << 8);
    
    if (!j1939_is_valid_16(raw)) return -9999.0f;
    
    return ((float)raw * 0.03125f) - 273.0f;
}

float j1939_decode_engine_hours(const uint8_t* data) {
    // PGN 65253 (HOURS) - Engine Total Hours at bytes 1-4 (indices 0-3)
    // Scale: 0.05 hours/bit, Offset: 0
    if (data == NULL) return -1.0f;
    
    uint32_t raw = (uint32_t)data[0] | 
                   ((uint32_t)data[1] << 8) | 
                   ((uint32_t)data[2] << 16) | 
                   ((uint32_t)data[3] << 24);
    
    if (raw == 0xFFFFFFFF) return -1.0f;  // Not available
    
    return (float)raw * 0.05f;
}

float j1939_decode_fuel_rate(const uint8_t* data) {
    // PGN 65266 (LFE) - Fuel Rate at bytes 1-2 (indices 0-1)
    // Scale: 0.05 L/h per bit, Offset: 0
    if (data == NULL) return -1.0f;
    
    uint16_t raw = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    
    if (!j1939_is_valid_16(raw)) return -1.0f;
    
    return (float)raw * 0.05f;
}

float j1939_decode_throttle_position(const uint8_t* data) {
    // PGN 61443 (EEC2) - Accelerator Pedal Position 1 at byte 2 (index 1)
    // Scale: 0.4 %/bit, Offset: 0
    if (data == NULL) return -1.0f;
    
    uint8_t raw = data[1];
    
    if (!j1939_is_valid_8(raw)) return -1.0f;
    
    return (float)raw * 0.4f;
}

float j1939_decode_ambient_temp(const uint8_t* data) {
    // PGN 65269 (AMB) - Ambient Air Temperature at bytes 4-5 (indices 3-4)
    // Scale: 0.03125 °C/bit, Offset: -273°C
    if (data == NULL) return -9999.0f;
    
    uint16_t raw = (uint16_t)data[3] | ((uint16_t)data[4] << 8);
    
    if (!j1939_is_valid_16(raw)) return -9999.0f;
    
    return ((float)raw * 0.03125f) - 273.0f;
}

/*===========================================================================*/
/*                        TRANSPORT PROTOCOL HANDLING                       */
/*===========================================================================*/

static tp_session_t* find_tp_session(j1939_parser_context_t* ctx, uint8_t source_address) {
    for (int i = 0; i < J1939_MAX_ACTIVE_TP; i++) {
        if (ctx->tp_sessions[i].state != TP_STATE_IDLE &&
            ctx->tp_sessions[i].source_address == source_address) {
            return &ctx->tp_sessions[i];
        }
    }
    return NULL;
}

static tp_session_t* allocate_tp_session(j1939_parser_context_t* ctx) {
    for (int i = 0; i < J1939_MAX_ACTIVE_TP; i++) {
        if (ctx->tp_sessions[i].state == TP_STATE_IDLE) {
            return &ctx->tp_sessions[i];
        }
    }
    return NULL;
}

bool j1939_tp_handle_frame(j1939_parser_context_t* ctx, const j1939_message_t* msg) {
    if (ctx == NULL || msg == NULL) return false;
    
    if (msg->pgn == PGN_TP_CM) {
        // Transport Protocol Connection Management
        uint8_t control_byte = msg->data[0];
        
        if (control_byte == TP_CM_BAM) {
            // Broadcast Announce Message - start new session
            tp_session_t* session = find_tp_session(ctx, msg->source_address);
            
            if (session == NULL) {
                session = allocate_tp_session(ctx);
            }
            
            if (session == NULL) {
                return false;  // No free sessions
            }
            
            session->state = TP_STATE_RECEIVING;
            session->source_address = msg->source_address;
            session->total_size = (uint16_t)msg->data[1] | ((uint16_t)msg->data[2] << 8);
            session->total_packets = msg->data[3];
            session->target_pgn = (uint32_t)msg->data[5] | 
                                  ((uint32_t)msg->data[6] << 8) | 
                                  ((uint32_t)msg->data[7] << 16);
            session->received_packets = 0;
            session->last_packet_time_ms = msg->timestamp_ms;
            memset(session->buffer, 0xFF, sizeof(session->buffer));
        }
    }
    else if (msg->pgn == PGN_TP_DT) {
        // Transport Protocol Data Transfer
        tp_session_t* session = find_tp_session(ctx, msg->source_address);
        
        if (session == NULL || session->state != TP_STATE_RECEIVING) {
            return false;
        }
        
        // Check for timeout
        if ((msg->timestamp_ms - session->last_packet_time_ms) > J1939_TP_TIMEOUT_MS) {
            session->state = TP_STATE_ERROR;
            return false;
        }
        
        uint8_t seq_num = msg->data[0];  // 1-based sequence number
        
        // Validate sequence
        if (seq_num != session->received_packets + 1) {
            session->state = TP_STATE_ERROR;
            return false;
        }
        
        // Copy 7 data bytes to buffer
        uint16_t offset = (seq_num - 1) * 7;
        for (int i = 0; i < 7 && (offset + i) < session->total_size; i++) {
            session->buffer[offset + i] = msg->data[1 + i];
        }
        
        session->received_packets++;
        session->last_packet_time_ms = msg->timestamp_ms;
        
        // Check if complete
        if (session->received_packets >= session->total_packets) {
            session->state = TP_STATE_COMPLETE;
            ctx->tp_complete_count++;
            return true;
        }
    }
    
    return false;
}

uint16_t j1939_tp_get_data(j1939_parser_context_t* ctx, uint8_t source_address,
                           uint32_t* pgn, uint8_t* data, uint16_t max_len) {
    if (ctx == NULL || data == NULL) return 0;
    
    tp_session_t* session = find_tp_session(ctx, source_address);
    
    if (session == NULL || session->state != TP_STATE_COMPLETE) {
        return 0;
    }
    
    if (pgn != NULL) {
        *pgn = session->target_pgn;
    }
    
    uint16_t copy_len = (session->total_size < max_len) ? session->total_size : max_len;
    memcpy(data, session->buffer, copy_len);
    
    // Reset session for reuse
    session->state = TP_STATE_IDLE;
    
    return copy_len;
}

/*===========================================================================*/
/*                        DM1 DIAGNOSTIC MESSAGE PARSING                    */
/*===========================================================================*/

uint8_t j1939_parse_dm1(const uint8_t* data, uint16_t len, 
                        j1939_lamp_status_t* lamps,
                        j1939_dtc_t* dtcs, uint8_t max_dtcs) {
    if (data == NULL || len < 2) return 0;
    
    // Parse lamp status from first two bytes
    if (lamps != NULL) {
        // Byte 1: Protect and Amber Warning lamps
        lamps->protect_lamp = (data[0] & 0x04) != 0;
        lamps->amber_warning_lamp = (data[0] & 0x10) != 0;
        // Byte 2: Red Stop and MIL lamps
        lamps->red_stop_lamp = (data[1] & 0x04) != 0;
        lamps->malfunction_lamp = (data[1] & 0x10) != 0;
    }
    
    if (dtcs == NULL || max_dtcs == 0) return 0;
    
    // Parse DTCs starting at byte 3 (index 2)
    // Each DTC is 4 bytes
    uint8_t dtc_count = 0;
    uint16_t offset = 2;
    
    while (offset + 4 <= len && dtc_count < max_dtcs) {
        // DTC structure (4 bytes):
        // Bytes 0-1: SPN bits 0-15 (little-endian)
        // Byte 2: SPN bits 16-18 (bits 5-7) | FMI (bits 0-4)
        // Byte 3: Occurrence Count (bits 0-6) | Conversion Method (bit 7)
        
        uint32_t spn = (uint32_t)data[offset] | 
                       ((uint32_t)data[offset + 1] << 8) |
                       (((uint32_t)(data[offset + 2] & 0xE0)) << 11);
        
        uint8_t fmi = data[offset + 2] & 0x1F;
        uint8_t oc = data[offset + 3] & 0x7F;
        
        // Skip "no fault" indicator (SPN=0, FMI=0)
        if (spn != 0 || fmi != 0) {
            dtcs[dtc_count].spn = spn;
            dtcs[dtc_count].fmi = fmi;
            dtcs[dtc_count].oc = oc;
            dtcs[dtc_count].is_active = true;
            dtc_count++;
        }
        
        offset += 4;
    }
    
    return dtc_count;
}

/*===========================================================================*/
/*                        STRING LOOKUP FUNCTIONS                           */
/*===========================================================================*/

typedef struct {
    uint32_t pgn;
    const char* name;
} pgn_name_t;

static const pgn_name_t pgn_names[] = {
    { 61444, "EEC1 - Electronic Engine Controller 1" },
    { 61443, "EEC2 - Electronic Engine Controller 2" },
    { 61442, "ETC1 - Electronic Transmission Controller 1" },
    { 61445, "ETC2 - Electronic Transmission Controller 2" },
    { 65262, "ET1 - Engine Temperature 1" },
    { 65263, "EFLP1 - Engine Fluid Level/Pressure 1" },
    { 65265, "CCVS - Cruise Control/Vehicle Speed" },
    { 65266, "LFE - Fuel Economy" },
    { 65269, "AMB - Ambient Conditions" },
    { 65270, "IC1 - Intake/Exhaust Conditions 1" },
    { 65271, "VEP1 - Vehicle Electrical Power 1" },
    { 65272, "TRF1 - Transmission Fluids 1" },
    { 65276, "DD - Dash Display" },
    { 65253, "HOURS - Engine Hours, Revolutions" },
    { 65226, "DM1 - Active Diagnostic Trouble Codes" },
    { 65227, "DM2 - Previously Active DTCs" },
    { 60416, "TP.CM - Transport Protocol Connection Management" },
    { 60160, "TP.DT - Transport Protocol Data Transfer" },
    { 0, NULL }
};

const char* j1939_get_pgn_name(uint32_t pgn) {
    for (int i = 0; pgn_names[i].name != NULL; i++) {
        if (pgn_names[i].pgn == pgn) {
            return pgn_names[i].name;
        }
    }
    return "Unknown PGN";
}

static const char* fmi_descriptions[] = {
    "Data Valid But Above Normal Operational Range - Most Severe",
    "Data Valid But Below Normal Operational Range - Most Severe",
    "Data Erratic, Intermittent Or Incorrect",
    "Voltage Above Normal, Or Shorted To High Source",
    "Voltage Below Normal, Or Shorted To Low Source",
    "Current Below Normal Or Open Circuit",
    "Current Above Normal Or Grounded Circuit",
    "Mechanical System Not Responding Or Out Of Adjustment",
    "Abnormal Frequency Or Pulse Width Or Period",
    "Abnormal Update Rate",
    "Abnormal Rate Of Change",
    "Root Cause Not Known",
    "Bad Intelligent Device Or Component",
    "Out Of Calibration",
    "Special Instructions",
    "Data Valid But Above Normal Operating Range - Least Severe",
    "Data Valid But Above Normal Operating Range - Moderately Severe",
    "Data Valid But Below Normal Operating Range - Least Severe",
    "Data Valid But Below Normal Operating Range - Moderately Severe",
    "Received Network Data In Error"
};

const char* j1939_get_fmi_description(uint8_t fmi) {
    if (fmi < sizeof(fmi_descriptions) / sizeof(fmi_descriptions[0])) {
        return fmi_descriptions[fmi];
    }
    if (fmi == 31) {
        return "Condition Exists";
    }
    return "Reserved";
}
