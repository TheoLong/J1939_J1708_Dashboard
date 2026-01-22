/**
 * @file j1939_pgn_definitions.h
 * @brief J1939 PGN (Parameter Group Number) definitions for heavy-duty vehicle communication
 * 
 * This file contains definitions for common J1939 PGNs used in engine, transmission,
 * and vehicle systems. Based on SAE J1939-71 Application Layer specification.
 * 
 * Source: SAE J1939 Standard, Open-SAE-J1939 project, CSS Electronics documentation
 * License: MIT (for this implementation file)
 * 
 * @note The official J1939 Digital Annex from SAE contains 1,800+ PGNs and 10,000+ SPNs.
 *       This file contains the most commonly used subset for truck dashboard applications.
 */

#ifndef J1939_PGN_DEFINITIONS_H
#define J1939_PGN_DEFINITIONS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/*                        J1939 PROTOCOL CONSTANTS                          */
/*===========================================================================*/

#define J1939_BAUD_RATE          250000    // Standard J1939 baud rate (250 kbps)
#define J1939_EXTENDED_ID        1         // J1939 uses 29-bit extended CAN IDs
#define J1939_MAX_DATA_LENGTH    8         // Standard CAN frame data length
#define J1939_TP_MAX_LENGTH      1785      // Max bytes via Transport Protocol (255 * 7)

// Special data values per J1939-71
#define J1939_NOT_AVAILABLE_8    0xFF      // 8-bit value not available
#define J1939_ERROR_8            0xFE      // 8-bit error indicator
#define J1939_NOT_AVAILABLE_16   0xFFFF    // 16-bit value not available
#define J1939_ERROR_16           0xFE00    // 16-bit error indicator base
#define J1939_NOT_AVAILABLE_32   0xFFFFFFFF // 32-bit value not available

// Address definitions
#define J1939_ADDR_GLOBAL        0xFF      // Broadcast/global destination
#define J1939_ADDR_NULL          0xFE      // Null/error address
#define J1939_ADDR_ENGINE        0x00      // Engine ECU (typical)
#define J1939_ADDR_TRANSMISSION  0x03      // Transmission ECU (typical)
#define J1939_ADDR_BRAKES        0x0B      // Brake System Controller
#define J1939_ADDR_INSTRUMENT    0x17      // Instrument Cluster (alternate)
#define J1939_ADDR_BODY          0x21      // Body Controller
#define J1939_ADDR_RETARDER      0x0F      // Retarder - Driveline
#define J1939_ADDR_DIAG_TOOL1    0xF9      // Off-board Diagnostic Tool #1
#define J1939_ADDR_DIAG_TOOL2    0xFA      // Off-board Diagnostic Tool #2

/*===========================================================================*/
/*                        PGN DEFINITIONS                                   */
/*===========================================================================*/

// Transport Protocol PGNs
#define PGN_TP_CM                60416     // 0xEC00 - Transport Protocol Connection Management
#define PGN_TP_DT                60160     // 0xEB00 - Transport Protocol Data Transfer
#define PGN_REQUEST              59904     // 0xEA00 - Request PGN

// Engine PGNs
#define PGN_EEC1                 61444     // 0xF004 - Electronic Engine Controller 1
#define PGN_EEC2                 61443     // 0xF003 - Electronic Engine Controller 2
#define PGN_EEC3                 65247     // 0xFEDF - Electronic Engine Controller 3
#define PGN_ET1                  65262     // 0xFEEE - Engine Temperature 1
#define PGN_EFLP1                65263     // 0xFEEF - Engine Fluid Level/Pressure 1
#define PGN_IC1                  65270     // 0xFEF6 - Intake/Exhaust Conditions 1
#define PGN_VEP1                 65271     // 0xFEF7 - Vehicle Electrical Power 1
#define PGN_HOURS                65253     // 0xFEE5 - Engine Hours, Revolutions
#define PGN_LFE                  65266     // 0xFEF2 - Fuel Economy (Liquid)
#define PGN_LFC                  65257     // 0xFEE9 - Fuel Consumption (Liquid)
#define PGN_SHUTDOWN             65252     // 0xFEE4 - Shutdown

// Transmission PGNs
#define PGN_ETC1                 61442     // 0xF002 - Electronic Transmission Controller 1
#define PGN_ETC2                 61445     // 0xF005 - Electronic Transmission Controller 2
#define PGN_TRF1                 65272     // 0xFEF8 - Transmission Fluids 1
#define PGN_TC1                  256       // 0x0100 - Transmission Control 1
#define PGN_TCO1                 65132     // 0xFE6C - Tachograph 1

// Vehicle PGNs
#define PGN_CCVS                 65265     // 0xFEF1 - Cruise Control/Vehicle Speed
#define PGN_DD                   65276     // 0xFEFC - Dash Display
#define PGN_AMB                  65269     // 0xFEF5 - Ambient Conditions
#define PGN_VDHR                 65217     // 0xFEC1 - High Resolution Vehicle Distance

// Brake/ABS PGNs
#define PGN_EBC1                 61441     // 0xF001 - Electronic Brake Controller 1
#define PGN_EBC2                 61442     // 0xF002 - Electronic Brake Controller 2 (shared with ETC1)

// Diagnostic PGNs
#define PGN_DM1                  65226     // 0xFECA - Active Diagnostic Trouble Codes
#define PGN_DM2                  65227     // 0xFECB - Previously Active DTCs
#define PGN_DM3                  65228     // 0xFECC - Diagnostic Data Clear/Reset

// Address Claim PGNs
#define PGN_ADDRESS_CLAIMED      60928     // 0xEE00 - Address Claimed
#define PGN_COMMANDED_ADDRESS    65240     // 0xFED8 - Commanded Address

// Identification PGNs
#define PGN_COMPONENT_ID         65259     // 0xFEEB - Component Identification
#define PGN_ECU_ID               64965     // 0xFDC5 - ECU Identification
#define PGN_SOFTWARE_ID          65242     // 0xFEDA - Software Identification

// Proprietary PGNs (range)
#define PGN_PROPRIETARY_A        61184     // 0xEF00 - Proprietary A
#define PGN_PROPRIETARY_B_START  65280     // 0xFF00 - Proprietary B Start
#define PGN_PROPRIETARY_B_END    65535     // 0xFFFF - Proprietary B End

/*===========================================================================*/
/*                        SPN DEFINITIONS                                   */
/*===========================================================================*/

// Engine SPNs
#define SPN_ENGINE_SPEED                    190   // 0xBE - Engine Speed (rpm)
#define SPN_DRIVER_DEMAND_TORQUE            512   // 0x200 - Driver's Demand Engine Torque (%)
#define SPN_ACTUAL_ENGINE_TORQUE            513   // 0x201 - Actual Engine Torque (%)
#define SPN_ENGINE_TORQUE_MODE              899   // 0x383 - Engine Torque Mode
#define SPN_ENGINE_COOLANT_TEMP             110   // 0x6E - Engine Coolant Temperature
#define SPN_ENGINE_OIL_TEMP                 175   // 0xAF - Engine Oil Temperature 1
#define SPN_ENGINE_OIL_PRESSURE             100   // 0x64 - Engine Oil Pressure
#define SPN_ENGINE_OIL_LEVEL                98    // 0x62 - Engine Oil Level
#define SPN_ENGINE_FUEL_TEMP                174   // 0xAE - Fuel Temperature 1
#define SPN_FUEL_RATE                       183   // 0xB7 - Engine Fuel Rate
#define SPN_INST_FUEL_ECONOMY               184   // 0xB8 - Instantaneous Fuel Economy
#define SPN_AVG_FUEL_ECONOMY                185   // 0xB9 - Average Fuel Economy
#define SPN_THROTTLE_POSITION               51    // 0x33 - Throttle Position
#define SPN_ENGINE_LOAD                     92    // 0x5C - Engine Percent Load at Current Speed
#define SPN_ACCEL_PEDAL_POS1                91    // 0x5B - Accelerator Pedal Position 1
#define SPN_BOOST_PRESSURE                  102   // 0x66 - Boost Pressure
#define SPN_INTAKE_MANIFOLD_TEMP            105   // 0x69 - Intake Manifold 1 Temperature
#define SPN_EXHAUST_GAS_TEMP                173   // 0xAD - Exhaust Gas Temperature
#define SPN_ENGINE_HOURS                    247   // 0xF7 - Engine Total Hours of Operation
#define SPN_ENGINE_REVOLUTIONS              249   // 0xF9 - Engine Total Revolutions
#define SPN_ENGINE_INTERCOOLER_TEMP         52    // 0x34 - Engine Intercooler Temperature
#define SPN_TURBO_OIL_TEMP                  176   // 0xB0 - Turbo Oil Temperature
#define SPN_AIR_FILTER_DIFF_PRESSURE        107   // 0x6B - Air Filter 1 Differential Pressure
#define SPN_COOLANT_PRESSURE                109   // 0x6D - Coolant Pressure
#define SPN_COOLANT_LEVEL                   111   // 0x6F - Coolant Level
#define SPN_BAROMETRIC_PRESSURE             108   // 0x6C - Barometric Pressure

// Transmission SPNs
#define SPN_TRANS_OUTPUT_SPEED              191   // 0xBF - Transmission Output Shaft Speed
#define SPN_TRANS_INPUT_SPEED               161   // 0xA1 - Transmission Input Shaft Speed
#define SPN_CLUTCH_SLIP                     522   // 0x20A - Percent Clutch Slip
#define SPN_CURRENT_GEAR                    523   // 0x20B - Current Gear
#define SPN_SELECTED_GEAR                   524   // 0x20C - Selected Gear
#define SPN_GEAR_RATIO                      526   // 0x20E - Actual Gear Ratio
#define SPN_TRANS_OIL_TEMP                  177   // 0xB1 - Transmission Oil Temperature (TRF1 byte 4, pressure)
#define SPN_TRANS_OIL_TEMP_EXT              178   // 0xB2 - Transmission Oil Temperature Extended (TRF1 bytes 5-6)
#define SPN_TRANS_OIL_PRESSURE              177   // 0xB1 - Transmission Oil Pressure (TRF1 byte 4)
#define SPN_TRANS_OIL_LEVEL                 126   // 0x7E - Transmission Oil Level
#define SPN_TRANS_FILTER_DIFF_PRESS         127   // 0x7F - Transmission Filter Differential Pressure
#define SPN_TRANS_CLUTCH_PRESSURE           124   // 0x7C - Transmission Clutch Pressure
#define SPN_DRIVELINE_ENGAGED               560   // 0x230 - Transmission Driveline Engaged
#define SPN_TC_LOCKUP_ENGAGED               573   // 0x23D - Torque Converter Lockup Engaged
#define SPN_SHIFT_IN_PROGRESS               574   // 0x23E - Transmission Shift In Process

// Vehicle Speed SPNs
#define SPN_WHEEL_BASED_SPEED               84    // 0x54 - Wheel-Based Vehicle Speed
#define SPN_CRUISE_CONTROL_SET_SPEED        86    // 0x56 - Cruise Control Set Speed
#define SPN_PARKING_BRAKE                   70    // 0x46 - Parking Brake Switch
#define SPN_CRUISE_ACTIVE                   595   // 0x253 - Cruise Control Active
#define SPN_CRUISE_ENABLE                   596   // 0x254 - Cruise Control Enable Switch
#define SPN_BRAKE_SWITCH                    597   // 0x255 - Brake Switch
#define SPN_CLUTCH_SWITCH                   598   // 0x256 - Clutch Switch

// Fuel SPNs
#define SPN_FUEL_LEVEL1                     96    // 0x60 - Fuel Level 1
#define SPN_FUEL_LEVEL2                     38    // 0x26 - Fuel Level 2
#define SPN_TOTAL_FUEL_USED                 250   // 0xFA - Engine Total Fuel Used
#define SPN_TRIP_FUEL                       252   // 0xFC - Trip Fuel

// Electrical SPNs
#define SPN_BATTERY_VOLTAGE                 168   // 0xA8 - Battery Potential (Voltage) - Switched
#define SPN_CHARGING_VOLTAGE                167   // 0xA7 - Charging System Potential (Voltage)
#define SPN_ALTERNATOR_CURRENT              115   // 0x73 - Alternator Current
#define SPN_BATTERY_CURRENT                 114   // 0x72 - Net Battery Current

// Environmental SPNs
#define SPN_AMBIENT_AIR_TEMP                171   // 0xAB - Ambient Air Temperature
#define SPN_CAB_INTERIOR_TEMP               170   // 0xAA - Cab Interior Temperature
#define SPN_ROAD_SURFACE_TEMP               79    // 0x4F - Road Surface Temperature
#define SPN_AIR_INLET_TEMP                  172   // 0xAC - Air Inlet Temperature

// Distance SPNs
#define SPN_TOTAL_VEHICLE_DISTANCE          245   // 0xF5 - Total Vehicle Distance
#define SPN_TRIP_DISTANCE                   244   // 0xF4 - Trip Distance
#define SPN_HI_RES_TOTAL_DISTANCE           917   // 0x395 - High Resolution Total Vehicle Distance
#define SPN_HI_RES_TRIP_DISTANCE            918   // 0x396 - High Resolution Trip Distance

// DTC-related SPNs
#define SPN_MALFUNCTION_LAMP                1213  // DM1 lamp status
#define SPN_DTC_SPN                         1214  // SPN from DTC
#define SPN_DTC_FMI                         1215  // FMI from DTC
#define SPN_DTC_OC                          1216  // Occurrence Count from DTC

/*===========================================================================*/
/*                        PGN STRUCTURE DEFINITIONS                         */
/*===========================================================================*/

/**
 * @brief SPN definition structure
 */
typedef struct {
    uint16_t spn;              // SPN number
    const char* name;          // Human-readable name
    const char* unit;          // Engineering unit
    uint8_t start_byte;        // Start byte position (0-7)
    uint8_t start_bit;         // Start bit position within byte (0-7)
    uint8_t length_bits;       // Length in bits
    float scale;               // Scaling factor (multiply raw by this)
    float offset;              // Offset (add after scaling)
    float min_value;           // Minimum valid value
    float max_value;           // Maximum valid value
} j1939_spn_def_t;

/**
 * @brief PGN definition structure
 */
typedef struct {
    uint32_t pgn;              // PGN number
    const char* name;          // Human-readable name
    const char* acronym;       // Short acronym (e.g., "EEC1")
    uint8_t data_length;       // Expected data length (8 for standard, >8 for TP)
    uint16_t default_rate_ms;  // Default transmission rate in ms (0 = on-request)
    uint8_t spn_count;         // Number of SPNs in this PGN
    const j1939_spn_def_t* spns; // Pointer to SPN definitions array
} j1939_pgn_def_t;

/*===========================================================================*/
/*                        SPN DEFINITIONS FOR KEY PGNs                      */
/*===========================================================================*/

// PGN 61444 (EEC1) - Electronic Engine Controller 1
static const j1939_spn_def_t spns_eec1[] = {
    { 899,  "Engine Torque Mode",                    "",    0, 0, 4,  1.0f,    0.0f,    0.0f,    15.0f   },
    { 512,  "Driver's Demand Engine Torque",         "%",   1, 0, 8,  1.0f,    -125.0f, -125.0f, 125.0f  },
    { 513,  "Actual Engine Torque",                  "%",   2, 0, 8,  1.0f,    -125.0f, -125.0f, 125.0f  },
    { 190,  "Engine Speed",                          "rpm", 3, 0, 16, 0.125f,  0.0f,    0.0f,    8031.875f },
    { 1483, "Source Address of Controlling Device",  "",    5, 0, 8,  1.0f,    0.0f,    0.0f,    255.0f  },
    { 1675, "Engine Starter Mode",                   "",    6, 0, 4,  1.0f,    0.0f,    0.0f,    15.0f   },
    { 2432, "Engine Demand Torque",                  "%",   7, 0, 8,  1.0f,    -125.0f, -125.0f, 125.0f  },
};

// PGN 61443 (EEC2) - Electronic Engine Controller 2
static const j1939_spn_def_t spns_eec2[] = {
    { 558,  "Accelerator Pedal 1 Low Idle Switch",   "",    0, 0, 2,  1.0f,    0.0f,    0.0f,    3.0f    },
    { 559,  "Accelerator Pedal Kickdown Switch",     "",    0, 2, 2,  1.0f,    0.0f,    0.0f,    3.0f    },
    { 1437, "Road Speed Limit Status",               "",    0, 4, 2,  1.0f,    0.0f,    0.0f,    3.0f    },
    { 91,   "Accelerator Pedal Position 1",          "%",   1, 0, 8,  0.4f,    0.0f,    0.0f,    100.0f  },
    { 92,   "Engine Percent Load at Current Speed",  "%",   2, 0, 8,  1.0f,    0.0f,    0.0f,    125.0f  },
    { 974,  "Remote Accelerator Pedal Position",     "%",   3, 0, 8,  0.4f,    0.0f,    0.0f,    100.0f  },
    { 29,   "Accelerator Pedal Position 2",          "%",   4, 0, 8,  0.4f,    0.0f,    0.0f,    100.0f  },
};

// PGN 65262 (ET1) - Engine Temperature 1
static const j1939_spn_def_t spns_et1[] = {
    { 110,  "Engine Coolant Temperature",            "°C",  0, 0, 8,  1.0f,    -40.0f,  -40.0f,  210.0f  },
    { 174,  "Fuel Temperature 1",                    "°C",  1, 0, 8,  1.0f,    -40.0f,  -40.0f,  210.0f  },
    { 175,  "Engine Oil Temperature 1",              "°C",  2, 0, 16, 0.03125f,-273.0f, -273.0f, 1735.0f },
    { 176,  "Turbo Oil Temperature",                 "°C",  4, 0, 16, 0.03125f,-273.0f, -273.0f, 1735.0f },
    { 52,   "Engine Intercooler Temperature",        "°C",  6, 0, 8,  1.0f,    -40.0f,  -40.0f,  210.0f  },
    { 1134, "Engine Intercooler Thermostat Opening", "%",   7, 0, 8,  0.4f,    0.0f,    0.0f,    100.0f  },
};

// PGN 65263 (EFLP1) - Engine Fluid Level/Pressure 1
static const j1939_spn_def_t spns_eflp1[] = {
    { 94,   "Fuel Delivery Pressure",                "kPa", 0, 0, 8,  4.0f,    0.0f,    0.0f,    1000.0f },
    { 22,   "Extended Crankcase Blow-by Pressure",   "kPa", 1, 0, 8,  0.05f,   -250.0f, -250.0f, -237.5f },
    { 98,   "Engine Oil Level",                      "%",   2, 0, 8,  0.4f,    0.0f,    0.0f,    100.0f  },
    { 100,  "Engine Oil Pressure",                   "kPa", 3, 0, 8,  4.0f,    0.0f,    0.0f,    1000.0f },
    { 101,  "Crankcase Pressure",                    "kPa", 4, 0, 16, 0.0078125f,-250.0f,-250.0f, 251.99f },
    { 109,  "Coolant Pressure",                      "kPa", 6, 0, 8,  2.0f,    0.0f,    0.0f,    500.0f  },
    { 111,  "Coolant Level",                         "%",   7, 0, 8,  0.4f,    0.0f,    0.0f,    100.0f  },
};

// PGN 65265 (CCVS) - Cruise Control/Vehicle Speed
static const j1939_spn_def_t spns_ccvs[] = {
    { 69,   "Two Speed Axle Switch",                 "",    0, 0, 2,  1.0f,    0.0f,    0.0f,    3.0f    },
    { 70,   "Parking Brake Switch",                  "",    0, 2, 2,  1.0f,    0.0f,    0.0f,    3.0f    },
    { 1633, "Cruise Control Pause Switch",           "",    0, 4, 2,  1.0f,    0.0f,    0.0f,    3.0f    },
    { 84,   "Wheel-Based Vehicle Speed",             "km/h",1, 0, 16, 0.00390625f,0.0f, 0.0f,    250.996f},
    { 595,  "Cruise Control Active",                 "",    3, 0, 2,  1.0f,    0.0f,    0.0f,    3.0f    },
    { 596,  "Cruise Control Enable Switch",          "",    3, 2, 2,  1.0f,    0.0f,    0.0f,    3.0f    },
    { 597,  "Brake Switch",                          "",    3, 4, 2,  1.0f,    0.0f,    0.0f,    3.0f    },
    { 598,  "Clutch Switch",                         "",    3, 6, 2,  1.0f,    0.0f,    0.0f,    3.0f    },
    { 86,   "Cruise Control Set Speed",              "km/h",5, 0, 8,  1.0f,    0.0f,    0.0f,    250.0f  },
};

// PGN 65270 (IC1) - Intake/Exhaust Conditions 1
static const j1939_spn_def_t spns_ic1[] = {
    { 81,   "Particulate Trap Inlet Pressure",       "kPa", 0, 0, 8,  0.5f,    0.0f,    0.0f,    125.0f  },
    { 102,  "Boost Pressure",                        "kPa", 1, 0, 8,  2.0f,    0.0f,    0.0f,    500.0f  },
    { 105,  "Intake Manifold 1 Temperature",         "°C",  2, 0, 8,  1.0f,    -40.0f,  -40.0f,  210.0f  },
    { 106,  "Air Inlet Pressure",                    "kPa", 3, 0, 8,  2.0f,    0.0f,    0.0f,    500.0f  },
    { 107,  "Air Filter 1 Differential Pressure",    "kPa", 4, 0, 8,  0.05f,   0.0f,    0.0f,    12.5f   },
    { 173,  "Exhaust Gas Temperature",               "°C",  5, 0, 16, 0.03125f,-273.0f, -273.0f, 1735.0f },
    { 112,  "Coolant Filter Differential Pressure",  "kPa", 7, 0, 8,  0.5f,    0.0f,    0.0f,    125.0f  },
};

// PGN 65271 (VEP1) - Vehicle Electrical Power 1
static const j1939_spn_def_t spns_vep1[] = {
    { 114,  "Net Battery Current",                   "A",   0, 0, 16, 1.0f,    -125.0f, -125.0f, 125.0f  },
    { 115,  "Alternator Current",                    "A",   2, 0, 16, 1.0f,    0.0f,    0.0f,    250.0f  },
    { 167,  "Charging System Potential (Voltage)",   "V",   4, 0, 16, 0.05f,   0.0f,    0.0f,    3212.75f},
    { 168,  "Battery Potential (Voltage) - Switched","V",   6, 0, 16, 0.05f,   0.0f,    0.0f,    3212.75f},
};

// PGN 65272 (TRF1) - Transmission Fluids 1
static const j1939_spn_def_t spns_trf1[] = {
    { 124,  "Transmission Clutch Pressure",          "kPa", 0, 0, 8,  16.0f,   0.0f,    0.0f,    4000.0f },
    { 126,  "Transmission Oil Level",                "%",   1, 0, 8,  0.4f,    0.0f,    0.0f,    100.0f  },
    { 127,  "Transmission Filter Differential Press","kPa", 2, 0, 8,  2.0f,    0.0f,    0.0f,    500.0f  },
    { 177,  "Transmission Oil Pressure",             "kPa", 3, 0, 8,  16.0f,   0.0f,    0.0f,    4000.0f },
    { 178,  "Transmission Oil Temperature",          "°C",  4, 0, 16, 0.03125f,-273.0f, -273.0f, 1735.0f },
    { 3027, "Transmission Oil Level High/Low",       "%",   6, 0, 8,  0.4f,    -50.0f,  -50.0f,  51.2f   },
    { 3028, "Transmission Oil Level Countdown Timer","s",   7, 0, 8,  1.0f,    0.0f,    0.0f,    250.0f  },
};

// PGN 61445 (ETC2) - Electronic Transmission Controller 2
static const j1939_spn_def_t spns_etc2[] = {
    { 524,  "Selected Gear",                         "",    0, 0, 8,  1.0f,    -125.0f, -125.0f, 125.0f  },
    { 526,  "Actual Gear Ratio",                     "",    1, 0, 16, 0.001f,  0.0f,    0.0f,    64.255f },
    { 523,  "Current Gear",                          "",    3, 0, 8,  1.0f,    -125.0f, -125.0f, 125.0f  },
};

// PGN 65266 (LFE) - Fuel Economy (Liquid)
static const j1939_spn_def_t spns_lfe[] = {
    { 183,  "Fuel Rate",                             "L/h", 0, 0, 16, 0.05f,   0.0f,    0.0f,    3212.75f},
    { 184,  "Instantaneous Fuel Economy",            "km/L",2, 0, 16, 0.001953125f,0.0f,0.0f,   125.5f  },
    { 185,  "Average Fuel Economy",                  "km/L",4, 0, 16, 0.001953125f,0.0f,0.0f,   125.5f  },
    { 51,   "Throttle Position",                     "%",   6, 0, 8,  0.4f,    0.0f,    0.0f,    100.0f  },
};

// PGN 65276 (DD) - Dash Display
static const j1939_spn_def_t spns_dd[] = {
    { 80,   "Washer Fluid Level",                    "%",   0, 0, 8,  0.4f,    0.0f,    0.0f,    100.0f  },
    { 96,   "Fuel Level 1",                          "%",   1, 0, 8,  0.4f,    0.0f,    0.0f,    100.0f  },
    { 95,   "Engine Fuel Filter Differential Press", "kPa", 2, 0, 8,  2.0f,    0.0f,    0.0f,    500.0f  },
    { 99,   "Engine Oil Filter Differential Press",  "kPa", 3, 0, 8,  0.5f,    0.0f,    0.0f,    125.0f  },
    { 169,  "Cargo Ambient Temperature",             "°C",  4, 0, 16, 0.03125f,-273.0f, -273.0f, 1735.0f },
    { 38,   "Fuel Level 2",                          "%",   6, 0, 8,  0.4f,    0.0f,    0.0f,    100.0f  },
};

// PGN 65269 (AMB) - Ambient Conditions
static const j1939_spn_def_t spns_amb[] = {
    { 108,  "Barometric Pressure",                   "kPa", 0, 0, 8,  0.5f,    0.0f,    0.0f,    125.0f  },
    { 170,  "Cab Interior Temperature",              "°C",  1, 0, 16, 0.03125f,-273.0f, -273.0f, 1735.0f },
    { 171,  "Ambient Air Temperature",               "°C",  3, 0, 16, 0.03125f,-273.0f, -273.0f, 1735.0f },
    { 172,  "Air Inlet Temperature",                 "°C",  5, 0, 8,  1.0f,    -40.0f,  -40.0f,  210.0f  },
    { 79,   "Road Surface Temperature",              "°C",  6, 0, 16, 0.03125f,-273.0f, -273.0f, 1735.0f },
};

// PGN 65253 (HOURS) - Engine Hours, Revolutions
static const j1939_spn_def_t spns_hours[] = {
    { 247,  "Engine Total Hours of Operation",       "hr",  0, 0, 32, 0.05f,   0.0f,    0.0f,    210554060.75f },
    { 249,  "Engine Total Revolutions",              "r",   4, 0, 32, 1000.0f, 0.0f,    0.0f,    4211081215000.0f },
};

/*===========================================================================*/
/*                        PGN CATALOG                                       */
/*===========================================================================*/

/**
 * @brief Main PGN catalog with definitions for all supported PGNs
 */
static const j1939_pgn_def_t j1939_pgn_catalog[] = {
    { PGN_EEC1,  "Electronic Engine Controller 1",      "EEC1",  8, 10,   7, spns_eec1  },
    { PGN_EEC2,  "Electronic Engine Controller 2",      "EEC2",  8, 50,   7, spns_eec2  },
    { PGN_ET1,   "Engine Temperature 1",                "ET1",   8, 1000, 6, spns_et1   },
    { PGN_EFLP1, "Engine Fluid Level/Pressure 1",       "EFLP1", 8, 500,  7, spns_eflp1 },
    { PGN_CCVS,  "Cruise Control/Vehicle Speed",        "CCVS",  8, 100,  9, spns_ccvs  },
    { PGN_IC1,   "Intake/Exhaust Conditions 1",         "IC1",   8, 500,  7, spns_ic1   },
    { PGN_VEP1,  "Vehicle Electrical Power 1",          "VEP1",  8, 1000, 4, spns_vep1  },
    { PGN_TRF1,  "Transmission Fluids 1",               "TRF1",  8, 1000, 7, spns_trf1  },
    { PGN_ETC2,  "Electronic Transmission Controller 2","ETC2",  8, 100,  3, spns_etc2  },
    { PGN_LFE,   "Fuel Economy (Liquid)",               "LFE",   8, 100,  4, spns_lfe   },
    { PGN_DD,    "Dash Display",                        "DD",    8, 1000, 6, spns_dd    },
    { PGN_AMB,   "Ambient Conditions",                  "AMB",   8, 1000, 5, spns_amb   },
    { PGN_HOURS, "Engine Hours, Revolutions",           "HOURS", 8, 1000, 2, spns_hours },
};

#define J1939_PGN_CATALOG_SIZE (sizeof(j1939_pgn_catalog) / sizeof(j1939_pgn_def_t))

/*===========================================================================*/
/*                        FMI (FAILURE MODE IDENTIFIER) CODES               */
/*===========================================================================*/

typedef enum {
    FMI_DATA_HIGH_MOST_SEVERE     = 0,   // Data Valid But Above Normal Operational Range - Most Severe Level
    FMI_DATA_LOW_MOST_SEVERE      = 1,   // Data Valid But Below Normal Operational Range - Most Severe Level
    FMI_DATA_ERRATIC              = 2,   // Data Erratic, Intermittent Or Incorrect
    FMI_VOLTAGE_HIGH              = 3,   // Voltage Above Normal, Or Shorted To High Source
    FMI_VOLTAGE_LOW               = 4,   // Voltage Below Normal, Or Shorted To Low Source
    FMI_CURRENT_LOW               = 5,   // Current Below Normal Or Open Circuit
    FMI_CURRENT_HIGH              = 6,   // Current Above Normal Or Grounded Circuit
    FMI_MECHANICAL_FAULT          = 7,   // Mechanical System Not Responding Or Out Of Adjustment
    FMI_ABNORMAL_FREQUENCY        = 8,   // Abnormal Frequency Or Pulse Width Or Period
    FMI_ABNORMAL_UPDATE_RATE      = 9,   // Abnormal Update Rate
    FMI_ABNORMAL_RATE_OF_CHANGE   = 10,  // Abnormal Rate Of Change
    FMI_ROOT_CAUSE_UNKNOWN        = 11,  // Root Cause Not Known
    FMI_BAD_DEVICE                = 12,  // Bad Intelligent Device Or Component
    FMI_OUT_OF_CALIBRATION        = 13,  // Out Of Calibration
    FMI_SPECIAL_INSTRUCTIONS      = 14,  // Special Instructions
    FMI_DATA_HIGH_LEAST_SEVERE    = 15,  // Data Valid But Above Normal Operating Range - Least Severe Level
    FMI_DATA_HIGH_MODERATELY      = 16,  // Data Valid But Above Normal Operating Range - Moderately Severe
    FMI_DATA_LOW_LEAST_SEVERE     = 17,  // Data Valid But Below Normal Operating Range - Least Severe Level
    FMI_DATA_LOW_MODERATELY       = 18,  // Data Valid But Below Normal Operating Range - Moderately Severe
    FMI_RECEIVED_NETWORK_ERROR    = 19,  // Received Network Data In Error
    // 20-30 Reserved
    FMI_CONDITION_EXISTS          = 31,  // Condition Exists
} j1939_fmi_t;

/*===========================================================================*/
/*                        HELPER FUNCTIONS                                  */
/*===========================================================================*/

/**
 * @brief Extract PGN from 29-bit CAN ID
 * @param can_id 29-bit extended CAN identifier
 * @return 18-bit PGN value
 */
static inline uint32_t j1939_get_pgn(uint32_t can_id) {
    uint8_t pdu_format = (can_id >> 16) & 0xFF;
    if (pdu_format < 240) {
        // PDU1: PGN does not include destination address
        return (can_id >> 8) & 0x3FF00;
    } else {
        // PDU2: PGN includes group extension
        return (can_id >> 8) & 0x3FFFF;
    }
}

/**
 * @brief Extract source address from 29-bit CAN ID
 * @param can_id 29-bit extended CAN identifier
 * @return 8-bit source address
 */
static inline uint8_t j1939_get_source_address(uint32_t can_id) {
    return can_id & 0xFF;
}

/**
 * @brief Extract priority from 29-bit CAN ID
 * @param can_id 29-bit extended CAN identifier
 * @return 3-bit priority (0-7, lower = higher priority)
 */
static inline uint8_t j1939_get_priority(uint32_t can_id) {
    return (can_id >> 26) & 0x07;
}

/**
 * @brief Build 29-bit CAN ID from components
 * @param pgn 18-bit Parameter Group Number
 * @param sa 8-bit source address
 * @param priority 3-bit priority (default 6)
 * @return 29-bit extended CAN identifier
 */
static inline uint32_t j1939_build_can_id(uint32_t pgn, uint8_t sa, uint8_t priority) {
    return ((uint32_t)priority << 26) | (pgn << 8) | sa;
}

/**
 * @brief Check if 8-bit value is valid (not error or N/A)
 */
static inline bool j1939_is_valid_8(uint8_t value) {
    return value != J1939_NOT_AVAILABLE_8 && value != J1939_ERROR_8;
}

/**
 * @brief Check if 16-bit value is valid (not error or N/A)
 */
static inline bool j1939_is_valid_16(uint16_t value) {
    return value < 0xFE00;
}

#ifdef __cplusplus
}
#endif

#endif /* J1939_PGN_DEFINITIONS_H */
