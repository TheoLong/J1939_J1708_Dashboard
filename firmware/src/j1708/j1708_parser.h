/**
 * @file j1708_parser.h
 * @brief J1708/J1587 message parser for legacy heavy-duty vehicle communication
 * 
 * SAE J1708 defines the physical layer (RS-485, 9600 bps)
 * SAE J1587 defines the application layer (MIDs, PIDs, data encoding)
 * 
 * These protocols are primarily used on older vehicles and ABS modules.
 */

#ifndef J1708_PARSER_H
#define J1708_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/*                        CONSTANTS                                         */
/*===========================================================================*/

#define J1708_MAX_MESSAGE_LENGTH    21      // Maximum J1708 message bytes
#define J1708_MIN_MESSAGE_LENGTH    2       // Minimum: MID + checksum
#define J1708_BAUD_RATE            9600     // J1708 serial baud rate
#define J1708_MAX_PIDS             10       // Maximum PIDs per message

// Special MID values
#define J1708_MID_ALL              255      // Broadcast to all devices
#define J1708_MID_NULL             254      // Null/reserved

// Common MIDs
#define MID_ENGINE_1               128
#define MID_TRANSMISSION           130
#define MID_BRAKES_TRAILER_1       136
#define MID_BRAKES_TRAILER_2       137
#define MID_INSTRUMENT_CLUSTER     140
#define MID_VEHICLE_MANAGEMENT     142
#define MID_BRAKES_ABS_TRACTOR     172

// Common PIDs
#define PID_ROAD_SPEED             84
#define PID_PERCENT_LOAD           92
#define PID_ENGINE_COOLANT_TEMP    110
#define PID_ENGINE_OIL_PRESSURE    100
#define PID_ENGINE_SPEED           190
#define PID_TRANS_OIL_TEMP         177
#define PID_BATTERY_VOLTAGE        168
#define PID_DIAGNOSTIC_CODES       194
#define PID_AMBIENT_TEMP           171
#define PID_FUEL_LEVEL_1           96

/*===========================================================================*/
/*                        DATA STRUCTURES                                   */
/*===========================================================================*/

/**
 * @brief Single J1587 parameter from a message
 */
typedef struct {
    uint8_t pid;                    // Parameter ID
    uint8_t data[8];                // Parameter data (variable length)
    uint8_t data_length;            // Actual data length
    bool is_valid;                  // True if successfully parsed
} j1587_parameter_t;

/**
 * @brief Complete J1708 message
 */
typedef struct {
    uint8_t mid;                    // Message Identifier (source ECU)
    uint8_t raw_data[J1708_MAX_MESSAGE_LENGTH];
    uint8_t raw_length;             // Total message length including checksum
    j1587_parameter_t params[J1708_MAX_PIDS];
    uint8_t param_count;            // Number of parsed parameters
    bool checksum_valid;            // True if checksum verified
    uint32_t timestamp_ms;          // Reception timestamp
} j1708_message_t;

/**
 * @brief J1587 diagnostic fault code
 */
typedef struct {
    uint8_t mid;                    // Source MID
    uint8_t pid_or_sid;             // PID or SID (subsystem ID)
    uint8_t fmi;                    // Failure Mode Identifier
    bool is_sid;                    // True if SID, false if PID
    uint8_t occurrence_count;       // How many times this fault occurred
    bool is_active;                 // Currently active fault
} j1587_fault_code_t;

/**
 * @brief Message receiver state machine
 */
typedef enum {
    J1708_RX_IDLE,                  // Waiting for first byte
    J1708_RX_RECEIVING,             // Receiving message bytes
    J1708_RX_COMPLETE               // Message complete
} j1708_rx_state_t;

/**
 * @brief Parser context
 */
typedef struct {
    j1708_rx_state_t state;
    uint8_t buffer[J1708_MAX_MESSAGE_LENGTH];
    uint8_t buffer_index;
    uint32_t last_byte_time_ms;
    uint32_t messages_received;
    uint32_t checksum_errors;
    uint32_t parse_errors;
} j1708_parser_context_t;

/*===========================================================================*/
/*                        FUNCTION DECLARATIONS                             */
/*===========================================================================*/

/**
 * @brief Initialize the J1708 parser
 * @param ctx Parser context to initialize
 */
void j1708_parser_init(j1708_parser_context_t* ctx);

/**
 * @brief Feed a received byte to the parser
 * @param ctx Parser context
 * @param byte Received byte
 * @param timestamp_ms Reception timestamp
 * @return true if a complete message is now available
 */
bool j1708_receive_byte(j1708_parser_context_t* ctx, uint8_t byte, uint32_t timestamp_ms);

/**
 * @brief Get the completed message from the parser
 * @param ctx Parser context
 * @param msg Output message structure
 * @return true if a valid message was retrieved
 */
bool j1708_get_message(j1708_parser_context_t* ctx, j1708_message_t* msg);

/**
 * @brief Parse a complete J1708 message buffer
 * @param data Raw message data (including MID and checksum)
 * @param len Data length
 * @param msg Output message structure
 * @return true if parsing successful
 */
bool j1708_parse_message(const uint8_t* data, uint8_t len, j1708_message_t* msg);

/**
 * @brief Validate J1708 message checksum
 * @param data Message data including checksum
 * @param len Total message length
 * @return true if checksum is valid
 * 
 * J1708 checksum: sum of all bytes (including checksum) = 0x00 (mod 256)
 */
bool j1708_validate_checksum(const uint8_t* data, uint8_t len);

/**
 * @brief Calculate J1708 checksum byte
 * @param data Message data (without checksum)
 * @param len Data length
 * @return Checksum byte to append
 */
uint8_t j1708_calculate_checksum(const uint8_t* data, uint8_t len);

/**
 * @brief Get expected data length for a PID
 * @param pid Parameter ID
 * @return Expected data bytes, or 0 if variable/unknown
 */
uint8_t j1708_get_pid_length(uint8_t pid);

/**
 * @brief Decode road speed from PID 84 data
 * @param data PID data bytes
 * @param len Data length
 * @return Speed in km/h, or -1 if invalid
 */
float j1708_decode_road_speed(const uint8_t* data, uint8_t len);

/**
 * @brief Decode engine RPM from PID 190 data
 * @param data PID data bytes
 * @param len Data length
 * @return RPM, or -1 if invalid
 */
float j1708_decode_engine_rpm(const uint8_t* data, uint8_t len);

/**
 * @brief Decode coolant temperature from PID 110 data
 * @param data PID data bytes
 * @param len Data length
 * @return Temperature in °C, or -9999 if invalid
 */
float j1708_decode_coolant_temp(const uint8_t* data, uint8_t len);

/**
 * @brief Decode oil pressure from PID 100 data
 * @param data PID data bytes
 * @param len Data length
 * @return Pressure in kPa, or -1 if invalid
 */
float j1708_decode_oil_pressure(const uint8_t* data, uint8_t len);

/**
 * @brief Decode transmission oil temperature from PID 177 data
 * @param data PID data bytes
 * @param len Data length
 * @return Temperature in °C, or -9999 if invalid
 */
float j1708_decode_trans_oil_temp(const uint8_t* data, uint8_t len);

/**
 * @brief Decode battery voltage from PID 168 data
 * @param data PID data bytes
 * @param len Data length
 * @return Voltage in V, or -1 if invalid
 */
float j1708_decode_battery_voltage(const uint8_t* data, uint8_t len);

/**
 * @brief Decode fuel level from PID 96 data
 * @param data PID data bytes
 * @param len Data length
 * @return Fuel level %, or -1 if invalid
 */
float j1708_decode_fuel_level(const uint8_t* data, uint8_t len);

/**
 * @brief Parse diagnostic fault codes from PID 194 data
 * @param mid Source MID
 * @param data PID 194 data
 * @param len Data length
 * @param faults Output fault code array
 * @param max_faults Maximum faults to parse
 * @return Number of faults parsed
 */
uint8_t j1708_parse_fault_codes(uint8_t mid, const uint8_t* data, uint8_t len,
                                 j1587_fault_code_t* faults, uint8_t max_faults);

/**
 * @brief Get MID name string
 * @param mid Message Identifier
 * @return Human-readable MID name
 */
const char* j1708_get_mid_name(uint8_t mid);

/**
 * @brief Get PID name string
 * @param pid Parameter Identifier
 * @return Human-readable PID name
 */
const char* j1708_get_pid_name(uint8_t pid);

#ifdef __cplusplus
}
#endif

#endif /* J1708_PARSER_H */
