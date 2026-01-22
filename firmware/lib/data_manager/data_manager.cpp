/**
 * @file data_manager.cpp
 * @brief Central data storage and management implementation
 */

#include "data_manager.h"
#include <string.h>
#include <math.h>

/*===========================================================================*/
/*                        INITIALIZATION                                    */
/*===========================================================================*/

void data_manager_init(data_manager_t* dm) {
    if (dm == NULL) return;
    
    memset(dm, 0, sizeof(data_manager_t));
    
    // Initialize all parameters as invalid
    for (int i = 0; i < DATA_MAX_PARAMETERS; i++) {
        dm->parameters[i].is_valid = false;
        dm->parameters[i].source = SOURCE_UNKNOWN;
    }
    
    dm->initialized = true;
}

/*===========================================================================*/
/*                        PARAMETER UPDATES                                 */
/*===========================================================================*/

void data_manager_update(data_manager_t* dm, param_id_t param_id,
                         float value, data_source_t source, uint32_t timestamp_ms) {
    if (dm == NULL || !dm->initialized) return;
    if (param_id == PARAM_NONE || param_id >= PARAM_MAX) return;
    
    data_parameter_t* param = &dm->parameters[param_id];
    
    // Store previous value for callbacks
    float old_value = param->value;
    bool was_valid = param->is_valid;
    
    // Update parameter
    param->prev_value = param->value;
    param->value = value;
    param->timestamp_ms = timestamp_ms;
    param->source = source;
    param->is_valid = true;
    param->update_count++;
    
    dm->total_updates++;
    
    // Notify callbacks if value changed significantly
    if (!was_valid || fabsf(value - old_value) > 0.001f) {
        for (uint8_t i = 0; i < dm->callback_count; i++) {
            if (dm->callbacks[i] != NULL) {
                dm->callbacks[i](param_id, value, old_value);
            }
        }
    }
}

/*===========================================================================*/
/*                        PARAMETER ACCESS                                  */
/*===========================================================================*/

bool data_manager_get(data_manager_t* dm, param_id_t param_id, float* value) {
    if (dm == NULL || !dm->initialized) return false;
    if (param_id == PARAM_NONE || param_id >= PARAM_MAX) return false;
    if (value == NULL) return false;
    
    data_parameter_t* param = &dm->parameters[param_id];
    
    if (!param->is_valid) return false;
    
    *value = param->value;
    return true;
}

bool data_manager_get_with_timestamp(data_manager_t* dm, param_id_t param_id,
                                      float* value, uint32_t* timestamp_ms) {
    if (dm == NULL || !dm->initialized) return false;
    if (param_id == PARAM_NONE || param_id >= PARAM_MAX) return false;
    
    data_parameter_t* param = &dm->parameters[param_id];
    
    if (!param->is_valid) return false;
    
    if (value != NULL) {
        *value = param->value;
    }
    if (timestamp_ms != NULL) {
        *timestamp_ms = param->timestamp_ms;
    }
    
    return true;
}

bool data_manager_is_fresh(data_manager_t* dm, param_id_t param_id,
                           uint32_t current_time_ms, uint32_t max_age_ms) {
    if (dm == NULL || !dm->initialized) return false;
    if (param_id == PARAM_NONE || param_id >= PARAM_MAX) return false;
    
    data_parameter_t* param = &dm->parameters[param_id];
    
    if (!param->is_valid) return false;
    
    uint32_t age = current_time_ms - param->timestamp_ms;
    return age <= max_age_ms;
}

uint32_t data_manager_get_age(data_manager_t* dm, param_id_t param_id,
                              uint32_t current_time_ms) {
    if (dm == NULL || !dm->initialized) return UINT32_MAX;
    if (param_id == PARAM_NONE || param_id >= PARAM_MAX) return UINT32_MAX;
    
    data_parameter_t* param = &dm->parameters[param_id];
    
    if (!param->is_valid) return UINT32_MAX;
    
    return current_time_ms - param->timestamp_ms;
}

void data_manager_invalidate(data_manager_t* dm, param_id_t param_id) {
    if (dm == NULL || !dm->initialized) return;
    if (param_id == PARAM_NONE || param_id >= PARAM_MAX) return;
    
    dm->parameters[param_id].is_valid = false;
}

/*===========================================================================*/
/*                        CALLBACKS                                         */
/*===========================================================================*/

bool data_manager_register_callback(data_manager_t* dm, data_change_callback_t callback) {
    if (dm == NULL || !dm->initialized) return false;
    if (callback == NULL) return false;
    if (dm->callback_count >= DATA_MAX_CALLBACKS) return false;
    
    dm->callbacks[dm->callback_count++] = callback;
    return true;
}

/*===========================================================================*/
/*                        STRING LOOKUPS                                    */
/*===========================================================================*/

typedef struct {
    param_id_t id;
    const char* name;
    const char* unit;
} param_info_t;

static const param_info_t param_info[] = {
    // Engine
    { PARAM_ENGINE_SPEED,       "Engine Speed",         "rpm" },
    { PARAM_ENGINE_LOAD,        "Engine Load",          "%" },
    { PARAM_THROTTLE_POSITION,  "Throttle Position",    "%" },
    { PARAM_COOLANT_TEMP,       "Coolant Temperature",  "°C" },
    { PARAM_OIL_TEMP,           "Oil Temperature",      "°C" },
    { PARAM_OIL_PRESSURE,       "Oil Pressure",         "kPa" },
    { PARAM_FUEL_TEMP,          "Fuel Temperature",     "°C" },
    { PARAM_INTAKE_TEMP,        "Intake Temperature",   "°C" },
    { PARAM_EXHAUST_TEMP,       "Exhaust Temperature",  "°C" },
    { PARAM_BOOST_PRESSURE,     "Boost Pressure",       "kPa" },
    { PARAM_BAROMETRIC_PRESSURE,"Barometric Pressure",  "kPa" },
    { PARAM_ENGINE_HOURS,       "Engine Hours",         "hr" },
    { PARAM_ENGINE_TORQUE,      "Engine Torque",        "%" },
    
    // Transmission
    { PARAM_TRANS_OIL_TEMP,     "Trans Oil Temp",       "°C" },
    { PARAM_TRANS_OIL_PRESSURE, "Trans Oil Pressure",   "kPa" },
    { PARAM_CURRENT_GEAR,       "Current Gear",         "" },
    { PARAM_SELECTED_GEAR,      "Selected Gear",        "" },
    { PARAM_OUTPUT_SHAFT_SPEED, "Output Shaft Speed",   "rpm" },
    { PARAM_GEAR_RATIO,         "Gear Ratio",           "" },
    
    // Vehicle
    { PARAM_VEHICLE_SPEED,      "Vehicle Speed",        "km/h" },
    { PARAM_CRUISE_CONTROL_SPEED,"Cruise Set Speed",    "km/h" },
    { PARAM_CRUISE_ACTIVE,      "Cruise Active",        "" },
    { PARAM_PARKING_BRAKE,      "Parking Brake",        "" },
    { PARAM_BRAKE_SWITCH,       "Brake Switch",         "" },
    
    // Fuel
    { PARAM_FUEL_LEVEL_1,       "Fuel Level 1",         "%" },
    { PARAM_FUEL_LEVEL_2,       "Fuel Level 2",         "%" },
    { PARAM_FUEL_RATE,          "Fuel Rate",            "L/h" },
    { PARAM_FUEL_ECONOMY_INST,  "Inst Fuel Economy",    "km/L" },
    { PARAM_FUEL_ECONOMY_AVG,   "Avg Fuel Economy",     "km/L" },
    { PARAM_TOTAL_FUEL_USED,    "Total Fuel Used",      "L" },
    
    // Electrical
    { PARAM_BATTERY_VOLTAGE,    "Battery Voltage",      "V" },
    { PARAM_CHARGING_VOLTAGE,   "Charging Voltage",     "V" },
    { PARAM_ALTERNATOR_CURRENT, "Alternator Current",   "A" },
    
    // Environmental
    { PARAM_AMBIENT_TEMP,       "Ambient Temperature",  "°C" },
    { PARAM_CAB_TEMP,           "Cab Temperature",      "°C" },
    { PARAM_EGT_SENSOR,         "EGT",                  "°C" },
    
    // Distance
    { PARAM_TOTAL_DISTANCE,     "Total Distance",       "km" },
    { PARAM_TRIP_A_DISTANCE,    "Trip A Distance",      "km" },
    { PARAM_TRIP_B_DISTANCE,    "Trip B Distance",      "km" },
    
    // ABS
    { PARAM_ABS_ACTIVE,         "ABS Active",           "" },
    
    // Diagnostics
    { PARAM_ACTIVE_DTC_COUNT,   "Active DTC Count",     "" },
    { PARAM_MIL_STATUS,         "MIL Status",           "" },
    
    // Computed
    { PARAM_MPG_CURRENT,        "Current MPG",          "mpg" },
    { PARAM_MPH,                "Speed",                "mph" },
    { PARAM_COOLANT_TEMP_F,     "Coolant Temp",         "°F" },
    
    // External
    { PARAM_EXT_FUEL_LEVEL,     "Aux Fuel Level",       "%" },
    { PARAM_DIMMER_LEVEL,       "Dimmer Level",         "%" },
    
    { PARAM_NONE, NULL, NULL }  // End marker
};

const char* data_manager_get_param_name(param_id_t param_id) {
    for (int i = 0; param_info[i].name != NULL; i++) {
        if (param_info[i].id == param_id) {
            return param_info[i].name;
        }
    }
    return "Unknown";
}

const char* data_manager_get_param_unit(param_id_t param_id) {
    for (int i = 0; param_info[i].name != NULL; i++) {
        if (param_info[i].id == param_id) {
            return param_info[i].unit;
        }
    }
    return "";
}

/*===========================================================================*/
/*                        STATISTICS                                        */
/*===========================================================================*/

void data_manager_get_stats(data_manager_t* dm, uint32_t* valid_count, uint32_t* total_updates) {
    if (dm == NULL || !dm->initialized) return;
    
    if (valid_count != NULL) {
        *valid_count = 0;
        for (int i = 0; i < DATA_MAX_PARAMETERS; i++) {
            if (dm->parameters[i].is_valid) {
                (*valid_count)++;
            }
        }
    }
    
    if (total_updates != NULL) {
        *total_updates = dm->total_updates;
    }
}
