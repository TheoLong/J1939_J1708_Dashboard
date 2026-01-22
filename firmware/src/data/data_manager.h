/**
 * @file data_manager.h
 * @brief Central data storage and management for vehicle parameters
 * 
 * Provides thread-safe storage for all decoded vehicle parameters with
 * timestamping, freshness tracking, and callback notifications.
 */

#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/*                        CONFIGURATION                                     */
/*===========================================================================*/

// Use config.h value if available, otherwise default
#ifndef DATA_MAX_PARAMETERS
#define DATA_MAX_PARAMETERS         64      // Maximum tracked parameters
#endif
#define DATA_MAX_CALLBACKS          8       // Maximum change callbacks
#define DATA_FRESHNESS_TIMEOUT_MS   5000    // Default stale threshold

/*===========================================================================*/
/*                        PARAMETER IDENTIFIERS                             */
/*===========================================================================*/

/**
 * @brief Unique parameter identifiers for all tracked values
 * 
 * These IDs are used to access parameters in the data manager.
 * Grouped by source/category.
 */
typedef enum {
    // Invalid/None
    PARAM_NONE = 0,
    
    // Engine parameters (1-49)
    PARAM_ENGINE_SPEED = 1,             // RPM
    PARAM_ENGINE_LOAD = 2,              // Percent
    PARAM_THROTTLE_POSITION = 3,        // Percent
    PARAM_COOLANT_TEMP = 4,             // °C
    PARAM_OIL_TEMP = 5,                 // °C
    PARAM_OIL_PRESSURE = 6,             // kPa
    PARAM_FUEL_TEMP = 7,                // °C
    PARAM_INTAKE_TEMP = 8,              // °C
    PARAM_EXHAUST_TEMP = 9,             // °C
    PARAM_BOOST_PRESSURE = 10,          // kPa
    PARAM_BAROMETRIC_PRESSURE = 11,     // kPa
    PARAM_ENGINE_HOURS = 12,            // Hours
    PARAM_ENGINE_TORQUE = 13,           // Percent
    
    // Transmission parameters (50-79)
    PARAM_TRANS_OIL_TEMP = 50,          // °C
    PARAM_TRANS_OIL_PRESSURE = 51,      // kPa
    PARAM_CURRENT_GEAR = 52,            // Gear number
    PARAM_SELECTED_GEAR = 53,           // Gear number
    PARAM_OUTPUT_SHAFT_SPEED = 54,      // RPM
    PARAM_GEAR_RATIO = 55,              // Ratio
    PARAM_CLUTCH_SLIP = 56,             // Percent
    
    // Vehicle parameters (80-109)
    PARAM_VEHICLE_SPEED = 80,           // km/h
    PARAM_WHEEL_SPEED_FL = 81,          // km/h
    PARAM_WHEEL_SPEED_FR = 82,          // km/h
    PARAM_WHEEL_SPEED_RL = 83,          // km/h
    PARAM_WHEEL_SPEED_RR = 84,          // km/h
    PARAM_CRUISE_CONTROL_SPEED = 85,    // km/h
    PARAM_CRUISE_ACTIVE = 86,           // Boolean
    PARAM_PARKING_BRAKE = 87,           // Boolean
    PARAM_BRAKE_SWITCH = 88,            // Boolean
    
    // Fuel parameters (110-129)
    PARAM_FUEL_LEVEL_1 = 110,           // Percent
    PARAM_FUEL_LEVEL_2 = 111,           // Percent
    PARAM_FUEL_RATE = 112,              // L/h
    PARAM_FUEL_ECONOMY_INST = 113,      // km/L
    PARAM_FUEL_ECONOMY_AVG = 114,       // km/L
    PARAM_TOTAL_FUEL_USED = 115,        // Liters
    
    // Electrical parameters (130-149)
    PARAM_BATTERY_VOLTAGE = 130,        // V
    PARAM_CHARGING_VOLTAGE = 131,       // V
    PARAM_ALTERNATOR_CURRENT = 132,     // A
    
    // Environmental parameters (150-169)
    PARAM_AMBIENT_TEMP = 150,           // °C
    PARAM_CAB_TEMP = 151,               // °C
    PARAM_EGT_SENSOR = 152,             // °C (external thermocouple)
    
    // Distance/odometer parameters (170-189)
    PARAM_TOTAL_DISTANCE = 170,         // km
    PARAM_TRIP_A_DISTANCE = 171,        // km
    PARAM_TRIP_B_DISTANCE = 172,        // km
    
    // ABS/Brake parameters (190-209)
    PARAM_ABS_ACTIVE = 190,             // Boolean
    PARAM_BRAKE_PRESSURE_PRIMARY = 191, // kPa
    PARAM_BRAKE_PRESSURE_SECONDARY = 192, // kPa
    
    // Diagnostic parameters (210-229)
    PARAM_ACTIVE_DTC_COUNT = 210,       // Count
    PARAM_MIL_STATUS = 211,             // Boolean
    
    // Computed parameters (230-249)
    PARAM_MPG_CURRENT = 230,            // Miles per gallon
    PARAM_MPH = 231,                    // Speed in MPH
    PARAM_COOLANT_TEMP_F = 232,         // Coolant in Fahrenheit
    
    // External sensor parameters (250-255)
    PARAM_EXT_FUEL_LEVEL = 250,         // Analog aux fuel tank
    PARAM_DIMMER_LEVEL = 251,           // Dashboard dimmer
    
    PARAM_MAX = 256
} param_id_t;

/*===========================================================================*/
/*                        DATA STRUCTURES                                   */
/*===========================================================================*/

/**
 * @brief Source of parameter data
 */
typedef enum {
    SOURCE_UNKNOWN = 0,
    SOURCE_J1939,               // J1939 CAN bus
    SOURCE_J1708,               // J1708 serial bus
    SOURCE_ANALOG,              // ADC input
    SOURCE_COMPUTED,            // Calculated from other parameters
    SOURCE_STORED,              // From NVS storage
    SOURCE_SIMULATED            // Test/simulation data
} data_source_t;

/**
 * @brief Stored parameter value with metadata
 */
typedef struct {
    float value;                // Current value
    float prev_value;           // Previous value (for rate of change)
    uint32_t timestamp_ms;      // When value was last updated
    uint32_t update_count;      // Number of times updated
    data_source_t source;       // Where this data came from
    bool is_valid;              // True if value is valid
} data_parameter_t;

/**
 * @brief Callback function type for parameter changes
 */
typedef void (*data_change_callback_t)(param_id_t param_id, float new_value, float old_value);

/**
 * @brief Data manager context
 */
typedef struct {
    data_parameter_t parameters[DATA_MAX_PARAMETERS];
    data_change_callback_t callbacks[DATA_MAX_CALLBACKS];
    uint8_t callback_count;
    uint32_t total_updates;
    bool initialized;
} data_manager_t;

/*===========================================================================*/
/*                        FUNCTION DECLARATIONS                             */
/*===========================================================================*/

/**
 * @brief Initialize the data manager
 * @param dm Data manager instance
 */
void data_manager_init(data_manager_t* dm);

/**
 * @brief Update a parameter value
 * @param dm Data manager instance
 * @param param_id Parameter identifier
 * @param value New value
 * @param source Source of this data
 * @param timestamp_ms Current timestamp
 */
void data_manager_update(data_manager_t* dm, param_id_t param_id, 
                         float value, data_source_t source, uint32_t timestamp_ms);

/**
 * @brief Get a parameter value
 * @param dm Data manager instance
 * @param param_id Parameter identifier
 * @param value Output value pointer
 * @return true if parameter is valid
 */
bool data_manager_get(data_manager_t* dm, param_id_t param_id, float* value);

/**
 * @brief Get parameter value with timestamp
 * @param dm Data manager instance
 * @param param_id Parameter identifier
 * @param value Output value pointer
 * @param timestamp_ms Output timestamp pointer
 * @return true if parameter is valid
 */
bool data_manager_get_with_timestamp(data_manager_t* dm, param_id_t param_id,
                                      float* value, uint32_t* timestamp_ms);

/**
 * @brief Check if a parameter is fresh (recently updated)
 * @param dm Data manager instance
 * @param param_id Parameter identifier
 * @param current_time_ms Current timestamp
 * @param max_age_ms Maximum age to be considered fresh
 * @return true if parameter was updated within max_age_ms
 */
bool data_manager_is_fresh(data_manager_t* dm, param_id_t param_id,
                           uint32_t current_time_ms, uint32_t max_age_ms);

/**
 * @brief Get the age of a parameter in milliseconds
 * @param dm Data manager instance
 * @param param_id Parameter identifier
 * @param current_time_ms Current timestamp
 * @return Age in milliseconds, or UINT32_MAX if invalid
 */
uint32_t data_manager_get_age(data_manager_t* dm, param_id_t param_id,
                              uint32_t current_time_ms);

/**
 * @brief Mark a parameter as invalid/stale
 * @param dm Data manager instance
 * @param param_id Parameter identifier
 */
void data_manager_invalidate(data_manager_t* dm, param_id_t param_id);

/**
 * @brief Register a callback for parameter changes
 * @param dm Data manager instance
 * @param callback Function to call when parameters change
 * @return true if callback registered successfully
 */
bool data_manager_register_callback(data_manager_t* dm, data_change_callback_t callback);

/**
 * @brief Get parameter name string
 * @param param_id Parameter identifier
 * @return Human-readable parameter name
 */
const char* data_manager_get_param_name(param_id_t param_id);

/**
 * @brief Get parameter unit string
 * @param param_id Parameter identifier
 * @return Unit string (e.g., "rpm", "°C", "kPa")
 */
const char* data_manager_get_param_unit(param_id_t param_id);

/**
 * @brief Get statistics about data manager usage
 * @param dm Data manager instance
 * @param valid_count Output: number of valid parameters
 * @param total_updates Output: total update count
 */
void data_manager_get_stats(data_manager_t* dm, uint32_t* valid_count, uint32_t* total_updates);

#ifdef __cplusplus
}
#endif

#endif /* DATA_MANAGER_H */
