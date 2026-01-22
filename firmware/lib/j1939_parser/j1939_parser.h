/**
 * @file j1939_parser.h
 * @brief J1939 CAN message parser for heavy-duty vehicle communication
 * 
 * This module handles decoding of J1939 CAN messages, extracting SPNs from PGNs,
 * and applying scaling/offset transformations per SAE J1939-71.
 */

#ifndef J1939_PARSER_H
#define J1939_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/*                        CONSTANTS                                         */
/*===========================================================================*/

#define J1939_MAX_DATA_LENGTH       8           // Standard CAN frame
#define J1939_TP_MAX_LENGTH         1785        // Max via Transport Protocol
#define J1939_TP_TIMEOUT_MS         750         // BAM timeout per J1939-21
#define J1939_MAX_ACTIVE_TP         4           // Max concurrent TP sessions

// Special values per J1939-71
#define J1939_NOT_AVAILABLE_8       0xFF
#define J1939_ERROR_8               0xFE
#define J1939_NOT_AVAILABLE_16      0xFFFF
#define J1939_ERROR_16_BASE         0xFE00

// Transport Protocol PGNs
#define PGN_TP_CM                   60416       // Connection Management
#define PGN_TP_DT                   60160       // Data Transfer

// Transport Protocol control bytes
#define TP_CM_BAM                   32          // Broadcast Announce Message
#define TP_CM_RTS                   16          // Request To Send
#define TP_CM_CTS                   17          // Clear To Send
#define TP_CM_EOM                   19          // End Of Message
#define TP_CM_ABORT                 255         // Connection Abort

/*===========================================================================*/
/*                        DATA STRUCTURES                                   */
/*===========================================================================*/

/**
 * @brief Parsed J1939 message structure
 */
typedef struct {
    uint32_t pgn;               // Parameter Group Number (18-bit)
    uint8_t source_address;     // Source ECU address
    uint8_t destination;        // Destination address (0xFF = broadcast)
    uint8_t priority;           // Priority (0-7, lower = higher)
    uint8_t data[J1939_MAX_DATA_LENGTH];
    uint8_t data_length;        // Actual data length (1-8)
    uint32_t timestamp_ms;      // Reception timestamp
} j1939_message_t;

/**
 * @brief Decoded parameter value
 */
typedef struct {
    uint16_t spn;               // Suspect Parameter Number
    float value;                // Decoded physical value
    const char* name;           // Parameter name
    const char* unit;           // Engineering unit
    bool is_valid;              // True if value is valid (not error/NA)
    uint32_t timestamp_ms;      // When this value was decoded
} j1939_parameter_t;

/**
 * @brief Diagnostic Trouble Code (DTC) structure
 */
typedef struct {
    uint32_t spn;               // Suspect Parameter Number
    uint8_t fmi;                // Failure Mode Identifier
    uint8_t oc;                 // Occurrence Count
    uint8_t source_address;     // Source ECU
    bool is_active;             // Active (DM1) or historical (DM2)
} j1939_dtc_t;

/**
 * @brief DM1 lamp status
 */
typedef struct {
    bool protect_lamp;          // Amber warning lamp
    bool amber_warning_lamp;    // Amber warning lamp
    bool red_stop_lamp;         // Red stop lamp
    bool malfunction_lamp;      // Check engine lamp (MIL)
} j1939_lamp_status_t;

/**
 * @brief Transport Protocol session state
 */
typedef enum {
    TP_STATE_IDLE,              // No transfer in progress
    TP_STATE_RECEIVING,         // Receiving TP.DT packets
    TP_STATE_COMPLETE,          // All packets received
    TP_STATE_ERROR              // Timeout or sequence error
} tp_state_t;

/**
 * @brief Transport Protocol session
 */
typedef struct {
    tp_state_t state;
    uint32_t target_pgn;        // PGN being reassembled
    uint8_t source_address;     // Source of multi-packet message
    uint16_t total_size;        // Expected total bytes
    uint8_t total_packets;      // Expected packet count
    uint8_t received_packets;   // Packets received so far
    uint8_t buffer[J1939_TP_MAX_LENGTH];
    uint32_t last_packet_time_ms;
} tp_session_t;

/**
 * @brief Parser context holding all state
 */
typedef struct {
    tp_session_t tp_sessions[J1939_MAX_ACTIVE_TP];
    uint32_t messages_received;
    uint32_t messages_parsed;
    uint32_t parse_errors;
    uint32_t tp_complete_count;
} j1939_parser_context_t;

/*===========================================================================*/
/*                        FUNCTION DECLARATIONS                             */
/*===========================================================================*/

/**
 * @brief Initialize the J1939 parser
 * @param ctx Parser context to initialize
 */
void j1939_parser_init(j1939_parser_context_t* ctx);

/**
 * @brief Extract PGN from 29-bit CAN identifier
 * @param can_id 29-bit extended CAN identifier
 * @return 18-bit Parameter Group Number
 * 
 * Handles PDU1 (PF < 240) and PDU2 (PF >= 240) formats correctly.
 * For PDU1, the PDU Specific field is the destination address, not part of PGN.
 * For PDU2, the PDU Specific field is the group extension, part of PGN.
 */
uint32_t j1939_extract_pgn(uint32_t can_id);

/**
 * @brief Extract source address from CAN ID
 * @param can_id 29-bit extended CAN identifier
 * @return 8-bit source address
 */
uint8_t j1939_extract_source_address(uint32_t can_id);

/**
 * @brief Extract priority from CAN ID
 * @param can_id 29-bit extended CAN identifier
 * @return 3-bit priority (0-7)
 */
uint8_t j1939_extract_priority(uint32_t can_id);

/**
 * @brief Extract destination address from CAN ID (PDU1 only)
 * @param can_id 29-bit extended CAN identifier
 * @return Destination address, or 0xFF if broadcast (PDU2)
 */
uint8_t j1939_extract_destination(uint32_t can_id);

/**
 * @brief Build 29-bit CAN ID from components
 * @param pgn Parameter Group Number
 * @param source_address Source address
 * @param priority Priority (default 6)
 * @return 29-bit extended CAN identifier
 */
uint32_t j1939_build_can_id(uint32_t pgn, uint8_t source_address, uint8_t priority);

/**
 * @brief Parse raw CAN frame into J1939 message structure
 * @param can_id 29-bit extended CAN identifier
 * @param data CAN frame data bytes
 * @param data_len Data length (1-8)
 * @param timestamp_ms Reception timestamp
 * @param msg Output message structure
 * @return true if parsing successful
 */
bool j1939_parse_frame(uint32_t can_id, const uint8_t* data, uint8_t data_len,
                       uint32_t timestamp_ms, j1939_message_t* msg);

/**
 * @brief Decode SPN value from message data
 * @param msg J1939 message
 * @param spn SPN to decode
 * @param value Output decoded value
 * @return true if SPN found and valid
 */
bool j1939_decode_spn(const j1939_message_t* msg, uint16_t spn, float* value);

/**
 * @brief Decode engine speed from EEC1 (PGN 61444)
 * @param data 8-byte EEC1 message data
 * @return Engine speed in RPM, or -1 if invalid
 */
float j1939_decode_engine_speed(const uint8_t* data);

/**
 * @brief Decode coolant temperature from ET1 (PGN 65262)
 * @param data 8-byte ET1 message data
 * @return Temperature in °C, or -9999 if invalid
 */
float j1939_decode_coolant_temp(const uint8_t* data);

/**
 * @brief Decode vehicle speed from CCVS (PGN 65265)
 * @param data 8-byte CCVS message data
 * @return Speed in km/h, or -1 if invalid
 */
float j1939_decode_vehicle_speed(const uint8_t* data);

/**
 * @brief Decode oil pressure from EFLP1 (PGN 65263)
 * @param data 8-byte EFLP1 message data
 * @return Pressure in kPa, or -1 if invalid
 */
float j1939_decode_oil_pressure(const uint8_t* data);

/**
 * @brief Decode boost pressure from IC1 (PGN 65270)
 * @param data 8-byte IC1 message data
 * @return Pressure in kPa, or -1 if invalid
 */
float j1939_decode_boost_pressure(const uint8_t* data);

/**
 * @brief Decode fuel level from DD (PGN 65276)
 * @param data 8-byte DD message data
 * @return Fuel level as %, or -1 if invalid
 */
float j1939_decode_fuel_level(const uint8_t* data);

/**
 * @brief Decode battery voltage from VEP1 (PGN 65271)
 * @param data 8-byte VEP1 message data
 * @return Voltage in V, or -1 if invalid
 */
float j1939_decode_battery_voltage(const uint8_t* data);

/**
 * @brief Decode current gear from ETC2 (PGN 61445)
 * @param data 8-byte ETC2 message data
 * @return Gear number (negative = reverse), or -999 if invalid
 */
int8_t j1939_decode_current_gear(const uint8_t* data);

/**
 * @brief Decode transmission oil temperature from TRF1 (PGN 65272)
 * @param data 8-byte TRF1 message data
 * @return Temperature in °C, or -9999 if invalid
 */
float j1939_decode_trans_oil_temp(const uint8_t* data);

/**
 * @brief Decode engine hours from HOURS (PGN 65253)
 * @param data 8-byte HOURS message data
 * @return Total engine hours, or -1 if invalid
 */
float j1939_decode_engine_hours(const uint8_t* data);

/**
 * @brief Decode fuel rate from LFE (PGN 65266)
 * @param data 8-byte LFE message data
 * @return Fuel rate in L/h, or -1 if invalid
 */
float j1939_decode_fuel_rate(const uint8_t* data);

/**
 * @brief Decode throttle position from EEC2 (PGN 61443)
 * @param data 8-byte EEC2 message data
 * @return Throttle position as %, or -1 if invalid
 */
float j1939_decode_throttle_position(const uint8_t* data);

/**
 * @brief Decode ambient temperature from AMB (PGN 65269)
 * @param data 8-byte AMB message data
 * @return Temperature in °C, or -9999 if invalid
 */
float j1939_decode_ambient_temp(const uint8_t* data);

/**
 * @brief Handle Transport Protocol frame (BAM/TP.DT)
 * @param ctx Parser context
 * @param msg Received J1939 message
 * @return true if a complete TP message is now available
 */
bool j1939_tp_handle_frame(j1939_parser_context_t* ctx, const j1939_message_t* msg);

/**
 * @brief Get completed TP message data
 * @param ctx Parser context
 * @param source_address Source to get TP data for
 * @param pgn Output: PGN of completed message
 * @param data Output buffer for data
 * @param max_len Maximum bytes to copy
 * @return Actual data length, or 0 if no complete message
 */
uint16_t j1939_tp_get_data(j1939_parser_context_t* ctx, uint8_t source_address,
                           uint32_t* pgn, uint8_t* data, uint16_t max_len);

/**
 * @brief Parse DM1 diagnostic trouble codes
 * @param data DM1 message data (may be from TP)
 * @param len Data length
 * @param lamps Output lamp status
 * @param dtcs Output DTC array
 * @param max_dtcs Maximum DTCs to parse
 * @return Number of DTCs parsed
 */
uint8_t j1939_parse_dm1(const uint8_t* data, uint16_t len, 
                        j1939_lamp_status_t* lamps,
                        j1939_dtc_t* dtcs, uint8_t max_dtcs);

/**
 * @brief Check if 8-bit value is valid (not error/NA)
 */
static inline bool j1939_is_valid_8(uint8_t value) {
    return value != J1939_NOT_AVAILABLE_8 && value != J1939_ERROR_8;
}

/**
 * @brief Check if 16-bit value is valid (not error/NA)
 */
static inline bool j1939_is_valid_16(uint16_t value) {
    return value < J1939_ERROR_16_BASE;
}

/**
 * @brief Get PGN name string
 * @param pgn Parameter Group Number
 * @return Human-readable PGN name or "Unknown"
 */
const char* j1939_get_pgn_name(uint32_t pgn);

/**
 * @brief Get FMI description string
 * @param fmi Failure Mode Identifier
 * @return Human-readable FMI description
 */
const char* j1939_get_fmi_description(uint8_t fmi);

#ifdef __cplusplus
}
#endif

#endif /* J1939_PARSER_H */
