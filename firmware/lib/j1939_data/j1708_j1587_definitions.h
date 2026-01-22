/**
 * @file j1708_j1587_definitions.h
 * @brief J1708/J1587 protocol definitions for legacy heavy-duty vehicle communication
 * 
 * SAE J1708 defines the physical/data link layer (RS-485, 9600 bps)
 * SAE J1587 defines the application layer (MIDs, PIDs, parameters)
 * 
 * These protocols are used primarily on older vehicles (pre-2007) and for
 * ABS modules that haven't transitioned to J1939.
 * 
 * @note J1587 is being phased out in favor of J1939, but many ABS systems
 *       still use this protocol.
 */

#ifndef J1708_J1587_DEFINITIONS_H
#define J1708_J1587_DEFINITIONS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/*                        J1708 PROTOCOL CONSTANTS                          */
/*===========================================================================*/

#define J1708_BAUD_RATE          9600      // J1708 serial baud rate
#define J1708_DATA_BITS          8         // 8 data bits
#define J1708_PARITY             0         // No parity
#define J1708_STOP_BITS          1         // 1 stop bit
#define J1708_MAX_MSG_LENGTH     21        // Maximum message length in bytes
#define J1708_MIN_MSG_LENGTH     2         // Minimum: MID + checksum
#define J1708_INTER_BYTE_MAX_MS  2         // Maximum inter-byte gap (bit times)
#define J1708_INTER_MSG_MIN_MS   10        // Minimum inter-message gap

/*===========================================================================*/
/*                        MID (MESSAGE IDENTIFIER) DEFINITIONS              */
/*===========================================================================*/

// MID values identify the source ECU
typedef enum {
    // Engine Controllers
    MID_ENGINE_1                    = 128,  // 0x80 - Engine #1
    MID_ENGINE_2                    = 129,  // 0x81 - Engine #2
    
    // Transmission
    MID_TRANSMISSION                = 130,  // 0x82 - Transmission
    
    // Brakes
    MID_BRAKES_POWER_TRAIN          = 131,  // 0x83 - Brakes - Power Train
    MID_BRAKES_TRAILER_1            = 136,  // 0x88 - Brakes - Trailer #1 (ABS)
    MID_BRAKES_TRAILER_2            = 137,  // 0x89 - Brakes - Trailer #2 (ABS)
    MID_BRAKES_ABS_TRACTOR          = 172,  // 0xAC - Antilock Brakes - Tractor (Bendix EC-60, WABCO)
    
    // Instruments and Body
    MID_INSTRUMENT_CLUSTER          = 140,  // 0x8C - Instrument Cluster
    MID_VEHICLE_MANAGEMENT          = 142,  // 0x8E - Vehicle Management System (often includes ABS on tractors)
    MID_BODY_CONTROLLER             = 144,  // 0x90 - Body Controller
    MID_SUSPENSION                  = 145,  // 0x91 - Suspension
    
    // Climate
    MID_CLIMATE_CONTROL             = 146,  // 0x92 - Cab Climate Control
    
    // Electrical
    MID_ELECTRICAL_CHARGING         = 147,  // 0x93 - Electrical Charging System
    MID_ELECTRICAL_SYSTEM           = 162,  // 0xA2 - Electrical System
    
    // Fuel System
    MID_FUEL_SYSTEM                 = 166,  // 0xA6 - Fuel System
    
    // Axles
    MID_DRIVE_AXLE_1                = 168,  // 0xA8 - Drive Axle #1
    MID_DRIVE_AXLE_2                = 169,  // 0xA9 - Drive Axle #2
    
    // Retarder
    MID_RETARDER_ENGINE             = 160,  // 0xA0 - Retarder - Engine
    MID_RETARDER_DRIVELINE          = 167,  // 0xA7 - Retarder - Driveline
    
    // Cruise Control
    MID_CRUISE_CONTROL              = 148,  // 0x94 - Cruise Control
    
    // Trip Recorder
    MID_TRIP_RECORDER               = 156,  // 0x9C - Trip Recorder
    
    // Diagnostic Tools (per SAE J1587)
    MID_DIAG_TOOL_1                 = 249,  // 0xF9 - Off-board Diagnostic Tool #1
    MID_DIAG_TOOL_2                 = 250,  // 0xFA - Off-board Diagnostic Tool #2
    
    // Tire Pressure
    MID_TIRE_PRESSURE               = 175,  // 0xAF - Tire Pressure Monitoring
    
    // Safety
    MID_COLLISION_AVOIDANCE         = 189,  // 0xBD - Collision Avoidance
    MID_LANE_DEPARTURE              = 236,  // 0xEC - Lane Departure Warning
    
    // Special
    MID_ALL_EXCEPT_OBD              = 253,  // 0xFD - All except off-board diagnostics
    MID_NULL                        = 254,  // 0xFE - Null/Reserved
    MID_ALL                         = 255,  // 0xFF - All devices
} j1587_mid_t;

/*===========================================================================*/
/*                        PID (PARAMETER IDENTIFIER) DEFINITIONS            */
/*===========================================================================*/

// PID values identify specific parameters within a message
typedef enum {
    // Basic Vehicle Parameters
    PID_ROAD_SPEED                  = 84,   // Vehicle Road Speed (0.5 mph/bit)
    PID_VEHICLE_SPEED_SENSOR        = 85,   // Vehicle Speed Sensor (1 mph/bit)
    PID_CRUISE_CONTROL_SET_SPEED    = 86,   // Cruise Control Set Speed
    PID_CRUISE_CONTROL_STATUS       = 89,   // Cruise Control Status
    
    // Engine Parameters
    PID_PERCENT_LOAD                = 92,   // Percent Load at Current RPM
    PID_ENGINE_SPEED                = 190,  // Engine Speed (RPM) - 2 bytes
    PID_ENGINE_OIL_TEMP             = 175,  // Engine Oil Temperature
    PID_ENGINE_COOLANT_TEMP         = 110,  // Engine Coolant Temperature
    PID_ENGINE_OIL_PRESSURE         = 100,  // Engine Oil Pressure
    PID_BOOST_PRESSURE              = 102,  // Turbo Boost Pressure
    PID_INTAKE_MANIFOLD_TEMP        = 105,  // Intake Manifold Temperature
    PID_FUEL_TEMP                   = 174,  // Fuel Temperature
    PID_FUEL_LEVEL_1                = 96,   // Fuel Level (Primary Tank)
    PID_FUEL_LEVEL_2                = 97,   // Fuel Level (Secondary Tank)
    PID_FUEL_RATE                   = 183,  // Instantaneous Fuel Rate
    PID_THROTTLE_POSITION           = 91,   // Throttle Position
    PID_ENGINE_HOURS                = 247,  // Engine Total Hours
    
    // Transmission Parameters (Note: J1587 PIDs differ from J1939 SPNs)
    PID_TRANS_OIL_TEMP              = 177,  // Transmission Oil Temperature (J1587 PID 177)
    PID_TRANS_OIL_PRESSURE          = 178,  // Transmission Oil Pressure (J1587 PID 178)
    PID_TRANS_OIL_LEVEL             = 124,  // Transmission Oil Level
    PID_SELECTED_GEAR               = 162,  // Selected Gear
    PID_CURRENT_GEAR                = 163,  // Current/Attained Gear
    PID_TRANS_OUTPUT_SHAFT_SPEED    = 191,  // Transmission Output Shaft Speed
    
    // Electrical
    PID_BATTERY_VOLTAGE             = 168,  // Battery Voltage
    PID_ALTERNATOR_VOLTAGE          = 167,  // Alternator/Charging Voltage
    
    // Brake Parameters (ABS-specific)
    PID_ABS_STATUS                  = 86,   // ABS Active Status
    PID_BRAKE_STROKE                = 115,  // Brake Stroke Status
    PID_BRAKE_APPLICATION_PRESS     = 116,  // Brake Application Pressure
    PID_BRAKE_PRIMARY_PRESS         = 117,  // Brake Primary Pressure
    PID_BRAKE_SECONDARY_PRESS       = 118,  // Brake Secondary Pressure
    PID_PARKING_BRAKE_STATUS        = 70,   // Parking Brake Status
    
    // Wheel Speed (ABS)
    PID_WHEEL_SPEED_FL              = 904,  // Front Left Wheel Speed (extended)
    PID_WHEEL_SPEED_FR              = 905,  // Front Right Wheel Speed
    PID_WHEEL_SPEED_RL              = 906,  // Rear Left Wheel Speed
    PID_WHEEL_SPEED_RR              = 907,  // Rear Right Wheel Speed
    
    // Environmental
    PID_AMBIENT_TEMP                = 171,  // Ambient Air Temperature
    PID_BAROMETRIC_PRESSURE         = 108,  // Barometric Pressure
    
    // Odometer/Distance
    PID_TOTAL_VEHICLE_DISTANCE      = 245,  // Total Vehicle Distance
    PID_TRIP_DISTANCE               = 244,  // Trip Distance
    
    // Diagnostics
    PID_DIAGNOSTIC_CODES            = 194,  // Active Diagnostic Codes
    PID_PREVIOUSLY_ACTIVE_CODES     = 195,  // Previously Active Codes
    PID_DIAGNOSTIC_REQUEST          = 196,  // Diagnostic Data Request
    PID_DIAGNOSTIC_RESPONSE         = 197,  // Diagnostic Data Response
    PID_COMPONENT_ID                = 234,  // Component Identification
    PID_SOFTWARE_ID                 = 233,  // Software Identification
    
    // Request/Response
    PID_REQUEST_PARAMETER           = 0,    // Request Parameter (PID 0)
    
    // Extended PIDs (Page 2, 128-191 + 192-255 prefix)
    PID_PAGE_2_PREFIX               = 192,  // Extended PID prefix for page 2
    PID_PAGE_2_START                = 256,  // Page 2 PIDs start at 256 (0x100)
    
} j1587_pid_t;

/*===========================================================================*/
/*                        ABS-SPECIFIC DEFINITIONS (Bendix/Meritor)         */
/*===========================================================================*/

// Bendix ABS MIDs
#define MID_BENDIX_ABS_TRACTOR      172    // Tractor ABS (EC-30/EC-60)
#define MID_BENDIX_ABS_TRAILER      136    // Trailer ABS
#define MID_BENDIX_ABS_TRAILER2     137    // Trailer ABS #2

// Meritor/WABCO ABS MIDs
#define MID_MERITOR_ABS_TRACTOR     172    // WABCO/Meritor ABS
#define MID_HALDEX_ABS              172    // Haldex ABS

// Common ABS fault codes (SID - Subsystem ID)
typedef enum {
    // Wheel Speed Sensor Faults
    SID_WHEEL_SENSOR_FL             = 1,    // Front Left Wheel Speed Sensor
    SID_WHEEL_SENSOR_FR             = 2,    // Front Right Wheel Speed Sensor
    SID_WHEEL_SENSOR_RL_OUTER       = 3,    // Rear Left Outer Wheel Speed Sensor
    SID_WHEEL_SENSOR_RR_OUTER       = 4,    // Rear Right Outer Wheel Speed Sensor
    SID_WHEEL_SENSOR_RL_INNER       = 5,    // Rear Left Inner Wheel Speed Sensor
    SID_WHEEL_SENSOR_RR_INNER       = 6,    // Rear Right Inner Wheel Speed Sensor
    
    // Modulator Valve Faults
    SID_MODULATOR_FL                = 7,    // Front Left Modulator Valve
    SID_MODULATOR_FR                = 8,    // Front Right Modulator Valve
    SID_MODULATOR_RL                = 9,    // Rear Left Modulator Valve
    SID_MODULATOR_RR                = 10,   // Rear Right Modulator Valve
    
    // System Faults
    SID_ECU_INTERNAL                = 254,  // ECU Internal Fault
    SID_POWER_SUPPLY                = 248,  // Power Supply
    SID_CAN_COMM                    = 249,  // CAN Communication
    SID_J1708_COMM                  = 250,  // J1708 Communication
} j1587_sid_t;

// FMI (Failure Mode Identifier) - shared with J1939
typedef enum {
    FMI_J1587_DATA_HIGH             = 0,    // Data valid but above normal range
    FMI_J1587_DATA_LOW              = 1,    // Data valid but below normal range
    FMI_J1587_DATA_ERRATIC          = 2,    // Data erratic, intermittent, incorrect
    FMI_J1587_VOLTAGE_HIGH          = 3,    // Voltage above normal
    FMI_J1587_VOLTAGE_LOW           = 4,    // Voltage below normal
    FMI_J1587_CURRENT_LOW           = 5,    // Current below normal or open circuit
    FMI_J1587_CURRENT_HIGH          = 6,    // Current above normal or grounded
    FMI_J1587_MECHANICAL            = 7,    // Mechanical system not responding
    FMI_J1587_ABNORMAL_FREQ         = 8,    // Abnormal frequency/pulse width
    FMI_J1587_ABNORMAL_UPDATE       = 9,    // Abnormal update rate
    FMI_J1587_ABNORMAL_CHANGE       = 10,   // Abnormal rate of change
    FMI_J1587_UNKNOWN               = 11,   // Root cause not known
    FMI_J1587_BAD_DEVICE            = 12,   // Bad intelligent device
    FMI_J1587_OUT_OF_CAL            = 13,   // Out of calibration
    FMI_J1587_SPECIAL               = 14,   // Special instructions
} j1587_fmi_t;

/*===========================================================================*/
/*                        MESSAGE STRUCTURES                                */
/*===========================================================================*/

/**
 * @brief J1708 message structure
 */
typedef struct {
    uint8_t mid;                   // Message Identifier (source ECU)
    uint8_t data[J1708_MAX_MSG_LENGTH - 2]; // Data bytes (PIDs and values)
    uint8_t data_length;           // Number of data bytes (excluding MID and checksum)
    uint8_t checksum;              // Checksum byte
    uint32_t timestamp_ms;         // Reception timestamp
    bool valid;                    // Checksum validation result
} j1708_message_t;

/**
 * @brief J1587 parameter structure (decoded)
 */
typedef struct {
    uint8_t mid;                   // Source MID
    uint16_t pid;                  // Parameter ID (may be extended)
    float value;                   // Decoded physical value
    const char* unit;              // Engineering unit string
    uint8_t raw_length;            // Number of raw bytes
    uint8_t raw_data[8];           // Raw data bytes
    bool valid;                    // Data validity flag
} j1587_parameter_t;

/**
 * @brief J1587 diagnostic trouble code structure
 */
typedef struct {
    uint8_t mid;                   // Source MID
    uint8_t pid;                   // Should be 194 (active) or 195 (inactive)
    uint8_t sid;                   // Subsystem Identifier
    uint8_t fmi;                   // Failure Mode Identifier
    uint8_t occurrence_count;      // Number of occurrences
    bool active;                   // Currently active?
} j1587_dtc_t;

/*===========================================================================*/
/*                        PARAMETER DEFINITIONS                             */
/*===========================================================================*/

/**
 * @brief J1587 parameter definition for decoding
 */
typedef struct {
    uint16_t pid;                  // Parameter ID
    const char* name;              // Human-readable name
    const char* unit;              // Engineering unit
    uint8_t data_length;           // Expected data length in bytes
    float scale;                   // Scaling factor
    float offset;                  // Offset (added after scaling)
    float min_value;               // Minimum valid value
    float max_value;               // Maximum valid value
} j1587_pid_def_t;

// Common J1587 parameter definitions
static const j1587_pid_def_t j1587_pid_catalog[] = {
    // Engine Parameters
    { 84,   "Road Speed",                    "mph",  1, 0.5f,    0.0f,    0.0f,    127.5f   },
    { 92,   "Percent Load at Current RPM",   "%",    1, 1.0f,    0.0f,    0.0f,    100.0f   },
    { 190,  "Engine Speed",                  "rpm",  2, 0.25f,   0.0f,    0.0f,    16383.75f},
    { 175,  "Engine Oil Temperature",        "°F",   1, 1.0f,    -40.0f,  -40.0f,  215.0f   },
    { 110,  "Engine Coolant Temperature",    "°F",   1, 1.0f,    -40.0f,  -40.0f,  215.0f   },
    { 100,  "Engine Oil Pressure",           "psi",  1, 0.5f,    0.0f,    0.0f,    127.5f   },
    { 102,  "Turbo Boost Pressure",          "psi",  1, 0.5f,    0.0f,    0.0f,    127.5f   },
    { 105,  "Intake Manifold Temperature",   "°F",   1, 1.0f,    -40.0f,  -40.0f,  215.0f   },
    { 96,   "Fuel Level 1",                  "%",    1, 0.5f,    0.0f,    0.0f,    127.5f   },
    { 183,  "Fuel Rate",                     "gal/h",2, 0.125f,  0.0f,    0.0f,    8191.875f},
    { 91,   "Throttle Position",             "%",    1, 0.4f,    0.0f,    0.0f,    102.0f   },
    { 247,  "Engine Total Hours",            "hrs",  4, 0.05f,   0.0f,    0.0f,    214748364.75f },
    
    // Transmission Parameters (J1587 PIDs)
    { 177,  "Transmission Oil Temperature",  "°F",   1, 1.0f,    -40.0f,  -40.0f,  302.0f   },  // J1587 PID 177 (distinct from J1939 SPN 177)
    { 178,  "Transmission Oil Pressure",     "psi",  1, 4.0f,    0.0f,    0.0f,    1020.0f  },  // J1587 PID 178 (distinct from J1939 SPN 178)
    { 124,  "Transmission Oil Level",        "%",    1, 0.5f,    0.0f,    0.0f,    127.5f   },  // J1587 PID 124
    { 162,  "Selected Gear",                 "",     1, 1.0f,    -125.0f, -125.0f, 125.0f   },
    { 163,  "Current Gear",                  "",     1, 1.0f,    -125.0f, -125.0f, 125.0f   },
    { 191,  "Trans Output Shaft Speed",      "rpm",  2, 0.25f,   0.0f,    0.0f,    16383.75f},
    
    // Electrical
    { 168,  "Battery Voltage",               "V",    2, 0.05f,   0.0f,    0.0f,    3276.75f },  // 2 bytes, 0.05V/bit (J1587 uses 2-byte value)
    { 167,  "Alternator Voltage",            "V",    2, 0.05f,   0.0f,    0.0f,    3276.75f },  // 2 bytes, 0.05V/bit
    
    // Brakes
    { 70,   "Parking Brake Status",          "",     1, 1.0f,    0.0f,    0.0f,    3.0f     },
    { 116,  "Brake Application Pressure",    "psi",  1, 0.5f,    0.0f,    0.0f,    127.5f   },
    { 117,  "Brake Primary Pressure",        "psi",  1, 0.5f,    0.0f,    0.0f,    127.5f   },
    { 118,  "Brake Secondary Pressure",      "psi",  1, 0.5f,    0.0f,    0.0f,    127.5f   },
    
    // Environmental
    { 171,  "Ambient Air Temperature",       "°F",   1, 1.0f,    -40.0f,  -40.0f,  215.0f   },
    { 108,  "Barometric Pressure",           "psi",  1, 0.05f,   0.0f,    0.0f,    12.75f   },
    
    // Distance
    { 245,  "Total Vehicle Distance",        "mi",   4, 0.1f,    0.0f,    0.0f,    429496729.5f },
    { 244,  "Trip Distance",                 "mi",   4, 0.1f,    0.0f,    0.0f,    429496729.5f },
};

#define J1587_PID_CATALOG_SIZE (sizeof(j1587_pid_catalog) / sizeof(j1587_pid_def_t))

/*===========================================================================*/
/*                        MID LOOKUP TABLE                                  */
/*===========================================================================*/

/**
 * @brief MID name lookup structure
 */
typedef struct {
    uint8_t mid;
    const char* name;
    const char* abbreviation;
} j1587_mid_info_t;

static const j1587_mid_info_t j1587_mid_table[] = {
    { 128, "Engine #1",                  "ENG1"  },
    { 129, "Engine #2",                  "ENG2"  },
    { 130, "Transmission",               "TRANS" },
    { 131, "Brakes - Power Train",       "BRK_PT"},
    { 136, "Brakes - Trailer #1",        "BRK_T1"},
    { 137, "Brakes - Trailer #2",        "BRK_T2"},
    { 140, "Instrument Cluster",         "INST"  },
    { 142, "Vehicle Management System",  "VMS"   },
    { 144, "Body Controller",            "BODY"  },
    { 145, "Suspension",                 "SUSP"  },
    { 146, "Cab Climate Control",        "HVAC"  },
    { 147, "Electrical Charging System", "CHRG"  },
    { 148, "Cruise Control",             "CRUISE"},
    { 156, "Trip Recorder",              "TRIP"  },
    { 160, "Retarder - Engine",          "RET_E" },
    { 162, "Electrical System",          "ELEC"  },
    { 166, "Fuel System",                "FUEL"  },
    { 167, "Retarder - Driveline",       "RET_D" },
    { 168, "Drive Axle #1",              "AXLE1" },
    { 169, "Drive Axle #2",              "AXLE2" },
    { 172, "Brakes - Tractor ABS",       "ABS"   },
    { 175, "Tire Pressure Monitoring",   "TPMS"  },
};

#define J1587_MID_TABLE_SIZE (sizeof(j1587_mid_table) / sizeof(j1587_mid_info_t))

/*===========================================================================*/
/*                        HELPER FUNCTIONS                                  */
/*===========================================================================*/

/**
 * @brief Calculate J1708 checksum
 * @param data Message data including MID
 * @param length Length of data (MID + data bytes, excluding checksum)
 * @return Checksum byte (two's complement of sum)
 */
static inline uint8_t j1708_calculate_checksum(const uint8_t* data, uint8_t length) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return (~sum) + 1;  // Two's complement
}

/**
 * @brief Validate J1708 message checksum
 * @param data Complete message including checksum
 * @param length Total message length
 * @return true if checksum is valid
 */
static inline bool j1708_validate_checksum(const uint8_t* data, uint8_t length) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return (sum == 0);  // Sum of all bytes including checksum should be 0
}

/**
 * @brief Check if PID is a multi-byte parameter
 * @param pid Parameter ID
 * @return Expected data length for this PID (0 if unknown)
 */
static inline uint8_t j1587_get_pid_length(uint16_t pid) {
    // Most common PIDs have 1-byte data
    // Some have 2 or 4 bytes (e.g., RPM, distance, hours)
    switch (pid) {
        case 190:  // Engine Speed
        case 191:  // Trans Output Speed
        case 183:  // Fuel Rate
            return 2;
        case 245:  // Total Vehicle Distance
        case 244:  // Trip Distance
        case 247:  // Engine Hours
            return 4;
        case 194:  // Diagnostic Codes (variable)
        case 195:
            return 0;  // Variable length
        default:
            return 1;
    }
}

/**
 * @brief Check if MID is valid J1587 device
 */
static inline bool j1587_is_valid_mid(uint8_t mid) {
    return (mid >= 128 && mid <= 247) || mid == 253 || mid == 255;
}

/**
 * @brief Get MID name from table
 * @param mid Message Identifier
 * @return Pointer to name string, or "Unknown" if not found
 */
static inline const char* j1587_get_mid_name(uint8_t mid) {
    for (size_t i = 0; i < J1587_MID_TABLE_SIZE; i++) {
        if (j1587_mid_table[i].mid == mid) {
            return j1587_mid_table[i].name;
        }
    }
    return "Unknown";
}

#ifdef __cplusplus
}
#endif

#endif /* J1708_J1587_DEFINITIONS_H */
