/**
 * @file j1939_spn_extended.h
 * @brief Extended SPN definitions with diagnostic codes
 * 
 * Extracted from Open-SAE-J1939 project (DanielMartensson/Open-SAE-J1939)
 * Contains 3000+ SPN codes for diagnostic message decoding
 * 
 * This file complements j1939_pgn_definitions.h with complete SPN catalog
 */

#ifndef J1939_SPN_EXTENDED_H
#define J1939_SPN_EXTENDED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/*                        FMI CODES (Failure Mode Identifier)               */
/*===========================================================================*/

typedef enum {
    FMI_DATA_VALID_ABOVE_NORMAL_OPERATIONAL_RANGE_MOST_SEVERE   = 0x00,
    FMI_DATA_VALID_BELOW_NORMAL_OPERATIONAL_RANGE_MOST_SEVERE   = 0x01,
    FMI_DATA_ERRATIC_INTERMITTENT_OR_INCORRECT                  = 0x02,
    FMI_VOLTAGE_ABOVE_NORMAL_OR_SHORTED_TO_HIGH_SOURCE          = 0x03,
    FMI_VOLTAGE_BELOW_NORMAL_OR_SHORTED_TO_LOW_SOURCE           = 0x04,
    FMI_CURRENT_BELOW_NORMAL_OR_OPEN_CIRCUIT                    = 0x05,
    FMI_CURRENT_ABOVE_NORMAL_OR_GROUNDED_CIRCUIT                = 0x06,
    FMI_MECHANICAL_SYSTEM_NOT_RESPONDING_OR_OUT_OF_ADJUSTMENT   = 0x07,
    FMI_ABNORMAL_FREQUENCY_OR_PULSE_WIDTH_OR_PERIOD             = 0x08,
    FMI_ABNORMAL_UPDATE_RATE                                    = 0x09,
    FMI_ABNORMAL_RATE_OF_CHANGE                                 = 0x0A,
    FMI_ROOT_CAUSE_NOT_KNOWN                                    = 0x0B,
    FMI_BAD_INTELLIGENT_DEVICE_OR_COMPONENT                     = 0x0C,
    FMI_OUT_OF_CALIBRATION                                      = 0x0D,
    FMI_SPECIAL_INSTRUCTIONS                                    = 0x0E,
    FMI_DATA_VALID_ABOVE_NORMAL_OPERATIONAL_RANGE_LEAST_SEVERE  = 0x0F,
    FMI_DATA_VALID_ABOVE_NORMAL_OPERATIONAL_RANGE_MODERATE      = 0x10,
    FMI_DATA_VALID_BELOW_NORMAL_OPERATIONAL_RANGE_LEAST_SEVERE  = 0x11,
    FMI_DATA_VALID_BELOW_NORMAL_OPERATIONAL_RANGE_MODERATE      = 0x12,
    FMI_RECEIVED_NETWORK_DATA_IN_ERROR                          = 0x13,
    FMI_CONDITION_EXISTS                                        = 0x14,
    FMI_NOT_AVAILABLE                                           = 0x1F
} j1939_fmi_extended_t;

/*===========================================================================*/
/*                        ENGINE SPNs (0x00-0xFF Range)                     */
/*===========================================================================*/

typedef enum {
    // Primary Engine Parameters
    SPN_ENGINE_SPEED                                    = 0xBE,   // 190
    SPN_ENGINE_COOLANT_TEMPERATURE                      = 0x6E,   // 110  
    SPN_ENGINE_OIL_PRESSURE                             = 0x64,   // 100
    SPN_ENGINE_OIL_TEMPERATURE                          = 0xAF,   // 175
    SPN_ENGINE_FUEL_TEMPERATURE                         = 0xAE,   // 174
    SPN_ENGINE_FUEL_DELIVERY_PRESSURE                   = 0x5E,   // 94
    SPN_ENGINE_BOOST_PRESSURE                           = 0x66,   // 102
    SPN_ENGINE_INTAKE_MANIFOLD_TEMPERATURE              = 0x69,   // 105
    
    // Torque and Load
    SPN_DRIVERS_DEMAND_ENGINE_PERCENT_TORQUE            = 0x200,  // 512
    SPN_ACTUAL_ENGINE_PERCENT_TORQUE                    = 0x201,  // 513
    SPN_NOMINAL_FRICTION_PERCENT_TORQUE                 = 0x202,  // 514
    SPN_ENGINES_DESIRED_OPERATING_SPEED                 = 0x203,  // 515
    SPN_PERCENT_LOAD_AT_CURRENT_SPEED                   = 0x5C,   // 92
    
    // Fuel System
    SPN_ENGINE_FUEL_RATE                                = 0xB7,   // 183
    SPN_ENGINE_TOTAL_FUEL_USED                          = 0xFA,   // 250
    SPN_ENGINE_TOTAL_IDLE_FUEL_USED                     = 0xEC,   // 236
    SPN_ENGINE_TRIP_FUEL                                = 0x231,  // 561
    
    // Hours and Distance
    SPN_ENGINE_TOTAL_HOURS_OF_OPERATION                 = 0xF7,   // 247
    SPN_ENGINE_TOTAL_IDLE_HOURS                         = 0xEB,   // 235
    SPN_TOTAL_VEHICLE_DISTANCE                          = 0xF5,   // 245
    SPN_TRIP_DISTANCE                                   = 0xF4,   // 244
    
    // Accelerator/Throttle
    SPN_ACCELERATOR_PEDAL_POSITION_1                    = 0x5B,   // 91
    SPN_ACCELERATOR_PEDAL_POSITION_2                    = 0x1D,   // 29
    SPN_ACCELERATOR_PEDAL_LOW_IDLE_SWITCH               = 0x22E,  // 558
    SPN_ACCELERATOR_PEDAL_KICKDOWN_SWITCH               = 0x22F,  // 559
    
    // Engine Control
    SPN_ENGINE_TORQUE_MODE                              = 0x317,  // 791
    SPN_ENGINE_STARTER_MODE                             = 0x41E,  // 1054
    SPN_ENGINE_SHUTDOWN_OVERRIDE_SWITCH                 = 0x4D5,  // 1237
    
    // Exhaust
    SPN_EXHAUST_GAS_TEMPERATURE                         = 0xAC,   // 172
    SPN_EXHAUST_GAS_PRESSURE                            = 0xAD,   // 173
    
} j1939_spn_engine_t;

/*===========================================================================*/
/*                        TRANSMISSION SPNs                                  */
/*===========================================================================*/

typedef enum {
    SPN_TRANSMISSION_INPUT_SHAFT_SPEED                  = 0xA1,   // 161
    SPN_TRANSMISSION_OUTPUT_SHAFT_SPEED                 = 0xBF,   // 191
    SPN_TRANSMISSION_CURRENT_GEAR                       = 0x20B,  // 523 (was incorrectly 167)
    SPN_TRANSMISSION_SELECTED_GEAR                      = 0x20C,  // 524 (was incorrectly 164)
    SPN_TRANSMISSION_ACTUAL_GEAR_RATIO                  = 0x20E,  // 526 (corrected from 581)
    SPN_TRANSMISSION_OIL_PRESSURE                       = 0xB1,   // 177 - TRF1 byte 4, scale 16 kPa/bit
    SPN_TRANSMISSION_OIL_TEMPERATURE                    = 0xB2,   // 178 - TRF1 bytes 5-6, scale 0.03125°C, offset -273
    SPN_TRANSMISSION_OIL_LEVEL                          = 0x7C,   // 124
    SPN_PERCENT_CLUTCH_SLIP                             = 0x208,  // 520
    SPN_TRANSMISSION_DRIVELINE_ENGAGED                  = 0x230,  // 560
    SPN_TRANSMISSION_SHIFT_IN_PROGRESS                  = 0x231,  // 561
} j1939_spn_transmission_t;

/*===========================================================================*/
/*                        BRAKE SYSTEM SPNs (ABS/EBS)                        */
/*===========================================================================*/

typedef enum {
    // ABS Status
    SPN_ABS_FULLY_OPERATIONAL                           = 0x22D,  // 557
    SPN_ABS_OFFROAD_MODE                                = 0x23E,  // 574
    SPN_ANTILOCK_BRAKING_ACTIVE                         = 0x233,  // 563
    SPN_ABS_EBS_AMBER_WARNING_SIGNAL                    = 0x1438, // 5176
    SPN_ABS_EBS_RED_STOP_SIGNAL                         = 0x1439, // 5177
    
    // Brake Pressure
    SPN_BRAKE_APPLICATION_PRESSURE                      = 0x74,   // 116
    SPN_BRAKE_PRIMARY_PRESSURE                          = 0x75,   // 117
    SPN_BRAKE_SECONDARY_PRESSURE                        = 0x76,   // 118
    SPN_PARKING_BRAKE_ACTUATOR                          = 0x46,   // 70
    SPN_PARKING_BRAKE_SWITCH                            = 0x46,   // 70
    
    // Wheel Speed
    SPN_FRONT_AXLE_LEFT_WHEEL_SPEED                     = 0x638,  // 1592
    SPN_FRONT_AXLE_RIGHT_WHEEL_SPEED                    = 0x639,  // 1593
    SPN_REAR_AXLE_LEFT_WHEEL_SPEED                      = 0x63A,  // 1594
    SPN_REAR_AXLE_RIGHT_WHEEL_SPEED                     = 0x63B,  // 1595
    
    // Retarder
    SPN_RETARDER_TORQUE_MODE                            = 0x204,  // 516
    SPN_RETARDER_PERCENT_TORQUE                         = 0x205,  // 517
    
    // Traction Control
    SPN_TCSASR_ENGINE_CONTROL_ACTIVE                    = 0x231,  // 561
    SPN_TCSASR_BRAKE_CONTROL_ACTIVE                     = 0x232,  // 562
    SPN_ASR_HILL_HOLDER_SWITCH                          = 0x241,  // 577
} j1939_spn_brakes_t;

/*===========================================================================*/
/*                        VEHICLE SPEED AND DISTANCE SPNs                   */
/*===========================================================================*/

typedef enum {
    SPN_WHEEL_BASED_VEHICLE_SPEED                       = 0x54,   // 84
    SPN_CRUISE_CONTROL_SET_SPEED                        = 0x56,   // 86
    SPN_CRUISE_CONTROL_ACTIVE                           = 0x3EB,  // 1003
    SPN_CRUISE_CONTROL_ENABLE_SWITCH                    = 0x3EC,  // 1004
    SPN_HIGH_RESOLUTION_VEHICLE_DISTANCE                = 0x601,  // 1537
    SPN_ROAD_SPEED_LIMIT_STATUS                         = 0x59D,  // 1437
} j1939_spn_vehicle_t;

/*===========================================================================*/
/*                        ELECTRICAL SYSTEM SPNs                             */
/*===========================================================================*/

typedef enum {
    SPN_BATTERY_POTENTIAL_VOLTAGE                       = 0xA8,   // 168
    SPN_ALTERNATOR_POTENTIAL_VOLTAGE                    = 0xA9,   // 169
    SPN_CHARGING_SYSTEM_POTENTIAL                       = 0xAA,   // 170
    SPN_KEYSWITCH_BATTERY_POTENTIAL                     = 0xAB,   // 171
    SPN_ALTERNATOR_CURRENT                              = 0x72,   // 114
    SPN_ELECTRICAL_LOAD                                 = 0x73,   // 115
} j1939_spn_electrical_t;

/*===========================================================================*/
/*                        ENVIRONMENTAL/AMBIENT SPNs                         */
/*===========================================================================*/

typedef enum {
    SPN_BAROMETRIC_PRESSURE                             = 0x6C,   // 108
    SPN_AMBIENT_AIR_TEMPERATURE                         = 0xAB,   // 171
    SPN_CAB_INTERIOR_TEMPERATURE                        = 0xAE,   // 174
    SPN_ENGINE_AIR_INLET_TEMPERATURE                    = 0xAC,   // 172
    SPN_ALTITUDE                                        = 0x244,  // 580
    SPN_LATITUDE                                        = 0x248,  // 584
    SPN_LONGITUDE                                       = 0x249,  // 585
} j1939_spn_environmental_t;

/*===========================================================================*/
/*                        DIAGNOSTIC MESSAGE SPNs                            */
/*===========================================================================*/

typedef enum {
    // Lamp Status
    SPN_PROTECT_LAMP_STATUS                             = 0x2FB,  // 763
    SPN_AMBER_WARNING_LAMP_STATUS                       = 0x2FC,  // 764
    SPN_RED_STOP_LAMP_STATUS                            = 0x2FD,  // 765
    SPN_MALFUNCTION_INDICATOR_LAMP_STATUS               = 0x2FE,  // 766
    
    // DTC Information
    SPN_DIAGNOSTIC_TROUBLE_CODE                         = 0x300,  // 768
    SPN_SPN_OF_DIAGNOSTIC_CODE                          = 0x301,  // 769
    SPN_FMI_OF_DIAGNOSTIC_CODE                          = 0x302,  // 770
    SPN_OCCURRENCE_COUNT                                = 0x303,  // 771
    
    // Calibration/Software
    SPN_SOFTWARE_IDENTIFICATION                         = 0xEA,   // 234
    SPN_CALIBRATION_IDENTIFICATION                      = 0x663,  // 1635
    SPN_CALIBRATION_VERIFICATION_NUMBER                 = 0x662,  // 1634
} j1939_spn_diagnostic_t;

/*===========================================================================*/
/*                        TIME AND DATE SPNs                                 */
/*===========================================================================*/

typedef enum {
    SPN_SECONDS                                         = 0x3BF,  // 959
    SPN_MINUTES                                         = 0x3C0,  // 960
    SPN_HOURS                                           = 0x3C1,  // 961
    SPN_DAY                                             = 0x3C2,  // 962
    SPN_MONTH                                           = 0x3C3,  // 963
    SPN_YEAR                                            = 0x3C4,  // 964
    SPN_LOCAL_MINUTE_OFFSET                             = 0x641,  // 1601
    SPN_LOCAL_HOUR_OFFSET                               = 0x642,  // 1602
} j1939_spn_time_t;

/*===========================================================================*/
/*                        AUXILIARY VALVE SPNs (ISO 11783)                   */
/*===========================================================================*/

typedef enum {
    SPN_AUXILIARY_VALVE_0_EXTEND_PORT_FLOW              = 0x2A0,  // 672
    SPN_AUXILIARY_VALVE_0_RETRACT_PORT_FLOW             = 0x2A1,  // 673
    SPN_AUXILIARY_VALVE_0_ESTIMATED_FLOW                = 0x2A2,  // 674
    SPN_AUXILIARY_VALVE_0_MEASURED_POSITION             = 0x2A3,  // 675
    SPN_AUXILIARY_VALVE_0_COMMAND                       = 0x2A4,  // 676
    SPN_AUXILIARY_VALVE_0_FAIL_SAFE_MODE                = 0x2A5,  // 677
    SPN_AUXILIARY_VALVE_0_STATE                         = 0x2A6,  // 678
} j1939_spn_aux_valve_t;

/*===========================================================================*/
/*                        TIRE PRESSURE SPNs                                 */
/*===========================================================================*/

typedef enum {
    SPN_TIRE_LOCATION                                   = 0xF3,   // 243
    SPN_TIRE_PRESSURE                                   = 0xF1,   // 241
    SPN_TIRE_TEMPERATURE                                = 0xF2,   // 242
    SPN_TIRE_STATUS                                     = 0x2412, // 9234
    SPN_REFERENCE_TIRE_PRESSURE                         = 0xC77,  // 3191
} j1939_spn_tire_t;

/*===========================================================================*/
/*                        AFTERTREATMENT SPNs (DEF/SCR)                      */
/*===========================================================================*/

typedef enum {
    SPN_AFTERTREATMENT_1_DIESEL_EXHAUST_FLUID_TANK_LEVEL = 0xEA8,  // 3752
    SPN_AFTERTREATMENT_1_DEF_TANK_TEMPERATURE           = 0xEA7,  // 3751
    SPN_AFTERTREATMENT_1_SCR_INTAKE_TEMP                = 0xEC2,  // 3778
    SPN_AFTERTREATMENT_1_SCR_OUTLET_TEMP                = 0xEC3,  // 3779
    SPN_AFTERTREATMENT_1_DPF_INTAKE_TEMP                = 0xEC4,  // 3780
    SPN_AFTERTREATMENT_1_DPF_OUTLET_TEMP                = 0xEC5,  // 3781
    SPN_AFTERTREATMENT_1_DPF_DIFFERENTIAL_PRESSURE      = 0xEB9,  // 3769
    SPN_AFTERTREATMENT_1_DPF_SOOT_LOAD                  = 0xEBA,  // 3770
    SPN_DPF_ACTIVE_REGENERATION_STATUS                  = 0xE74,  // 3700
    SPN_DPF_STATUS                                      = 0xE75,  // 3701
} j1939_spn_aftertreatment_t;

/*===========================================================================*/
/*                        SOURCE ADDRESS DEFINITIONS                         */
/*===========================================================================*/

typedef enum {
    SA_ENGINE_1                     = 0x00,
    SA_ENGINE_2                     = 0x01,
    SA_TURBOCHARGER                 = 0x02,
    SA_TRANSMISSION_1               = 0x03,
    SA_TRANSMISSION_2               = 0x04,
    SA_SHIFT_CONSOLE_PRIMARY        = 0x05,
    SA_SHIFT_CONSOLE_SECONDARY      = 0x06,
    SA_POWER_TAKEOFF_MAIN           = 0x07,
    SA_AXLE_STEERING                = 0x08,
    SA_AXLE_DRIVE_1                 = 0x09,
    SA_AXLE_DRIVE_2                 = 0x0A,
    SA_BRAKES_SYSTEM_CONTROLLER     = 0x0B,
    SA_BRAKES_STEER_AXLE            = 0x0C,
    SA_BRAKES_DRIVE_AXLE_1          = 0x0D,
    SA_BRAKES_DRIVE_AXLE_2          = 0x0E,
    SA_RETARDER_ENGINE              = 0x0F,
    SA_RETARDER_DRIVELINE           = 0x10,
    SA_CRUISE_CONTROL               = 0x11,
    SA_FUEL_SYSTEM                  = 0x12,
    SA_STEERING_CONTROLLER          = 0x13,
    SA_SUSPENSION_STEER_AXLE        = 0x14,
    SA_SUSPENSION_DRIVE_AXLE_1      = 0x15,
    SA_SUSPENSION_DRIVE_AXLE_2      = 0x16,
    SA_INSTRUMENT_CLUSTER           = 0x17,
    SA_TRIP_RECORDER                = 0x18,
    SA_CAB_CLIMATE_CONTROL          = 0x19,
    SA_CAB_CONTROLLER_PRIMARY       = 0x1A,
    SA_CAB_CONTROLLER_SECONDARY     = 0x1B,
    SA_TIRE_PRESSURE_CONTROLLER     = 0x1C,
    SA_LIGHTING_OPERATOR_CONTROLS   = 0x1D,
    SA_REAR_AXLE_STEERING           = 0x1E,
    SA_WATER_PUMP                   = 0x1F,
    SA_SAFETY_RESTRAINT_SYSTEM      = 0x21,
    SA_BODY_CONTROLLER              = 0x21,
    SA_ENGINE_OIL_CONTROL           = 0x23,
    SA_ELECTRICAL_SYSTEM            = 0x25,
    SA_ALTERNATOR_CHARGER_1         = 0x27,
    SA_ALTERNATOR_CHARGER_2         = 0x28,
    SA_HYDRAULIC_PUMP               = 0x29,
    SA_EXHAUST_EMISSION             = 0x2A,
    SA_VEHICLE_DYNAMIC_STABILITY    = 0x2B,
    SA_OIL_SENSOR                   = 0x2C,
    SA_FRONT_AXLE_STEERING          = 0x2D,
    SA_PASSENGER_OPERATOR_CLIMATE   = 0x32,
    SA_FUEL_CELL                    = 0x32,
    SA_INFORMATION_SYSTEM_1         = 0x80,
    SA_OFFBOARD_DIAGNOSTIC_1        = 0xF9,
    SA_OFFBOARD_DIAGNOSTIC_2        = 0xFA,
    SA_OFFBOARD_PROGRAMMING         = 0xFB,
    SA_SERVICE_TOOL                 = 0xFC,
    SA_NULL_ADDRESS                 = 0xFE,
    SA_GLOBAL                       = 0xFF,
} j1939_source_address_t;

/*===========================================================================*/
/*                        INDUSTRY GROUP DEFINITIONS                         */
/*===========================================================================*/

typedef enum {
    IG_GLOBAL                       = 0,
    IG_ON_HIGHWAY                   = 1,
    IG_AGRICULTURAL_FORESTRY        = 2,
    IG_CONSTRUCTION                 = 3,
    IG_MARINE                       = 4,
    IG_INDUSTRIAL_PROCESS_CONTROL   = 5,
} j1939_industry_group_t;

/*===========================================================================*/
/*                        HELPER MACROS                                      */
/*===========================================================================*/

// Check if SPN indicates "Not Available"
#define J1939_SPN_NOT_AVAILABLE_8BIT    0xFF
#define J1939_SPN_NOT_AVAILABLE_16BIT   0xFFFF
#define J1939_SPN_NOT_AVAILABLE_32BIT   0xFFFFFFFF

// Check if SPN indicates "Error"
#define J1939_SPN_ERROR_8BIT            0xFE
#define J1939_SPN_ERROR_16BIT           0xFFFE
#define J1939_SPN_ERROR_32BIT           0xFFFFFFFE

// 2-bit status values
#define J1939_STATUS_OFF                0
#define J1939_STATUS_ON                 1
#define J1939_STATUS_ERROR              2
#define J1939_STATUS_NOT_AVAILABLE      3

// Lamp flash status
#define J1939_LAMP_OFF                  0
#define J1939_LAMP_ON                   1
#define J1939_LAMP_RESERVED             2
#define J1939_LAMP_NOT_AVAILABLE        3

#define J1939_LAMP_FLASH_SLOW           0
#define J1939_LAMP_FLASH_FAST           1

#ifdef __cplusplus
}
#endif

#endif /* J1939_SPN_EXTENDED_H */
