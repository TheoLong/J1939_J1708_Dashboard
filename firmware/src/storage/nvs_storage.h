/**
 * @file nvs_storage.h
 * @brief Non-Volatile Storage (NVS) manager for persistent data
 * 
 * Manages persistent storage of trip data, fuel economy statistics,
 * fault code history, and user settings using ESP32's NVS flash partition.
 */

#ifndef NVS_STORAGE_H
#define NVS_STORAGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/*                        CONFIGURATION                                     */
/*===========================================================================*/

#define NVS_MAX_DTC_HISTORY         20      // Maximum stored fault codes
#define NVS_KEY_MAX_LENGTH          15      // NVS key name limit

/*===========================================================================*/
/*                        DATA STRUCTURES                                   */
/*===========================================================================*/

/**
 * @brief Trip data structure (stored in NVS)
 */
typedef struct {
    float distance_km;              // Trip distance
    float fuel_used_liters;         // Fuel consumed this trip
    uint32_t start_time;            // Unix timestamp of trip start
    uint32_t duration_seconds;      // Total driving time
    float avg_speed_kmh;            // Average speed
    float avg_fuel_economy;         // Average L/100km
    bool is_active;                 // Trip currently in progress
} trip_data_t;

/**
 * @brief Lifetime statistics structure
 */
typedef struct {
    float total_distance_km;        // Odometer
    float total_fuel_liters;        // Lifetime fuel consumption
    float engine_hours;             // Total engine hours from ECU
    uint32_t boot_count;            // Number of power cycles
    float best_mpg;                 // Best recorded fuel economy (MPG)
    float worst_mpg;                // Worst recorded fuel economy (MPG)
    uint32_t first_boot_time;       // Unix timestamp of first boot
    uint32_t total_runtime_seconds; // Total system runtime
} lifetime_stats_t;

/**
 * @brief Stored fault code with history
 */
typedef struct {
    uint32_t spn;                   // Suspect Parameter Number
    uint8_t fmi;                    // Failure Mode Identifier
    uint8_t source_address;         // ECU source
    uint32_t first_seen;            // Timestamp first detected
    uint32_t last_seen;             // Timestamp last seen
    uint16_t occurrence_count;      // How many times seen
    bool is_active;                 // Currently active?
} stored_dtc_t;

/**
 * @brief User settings structure
 */
typedef struct {
    uint8_t units;                  // 0 = metric, 1 = imperial
    uint8_t brightness;             // Display brightness (0-100)
    uint8_t default_page;           // Default display page
    uint8_t temp_unit;              // 0 = Celsius, 1 = Fahrenheit
    uint8_t pressure_unit;          // 0 = kPa, 1 = PSI, 2 = bar
    uint8_t fuel_unit;              // 0 = L/100km, 1 = MPG
    uint16_t fuel_tank_1_size;      // Tank 1 capacity in liters
    uint16_t fuel_tank_2_size;      // Tank 2 capacity in liters
} user_settings_t;

/**
 * @brief System state for graceful shutdown detection
 */
typedef struct {
    bool clean_shutdown;            // True if last shutdown was clean
    uint32_t last_timestamp;        // Last known timestamp
    uint32_t boot_count;            // Boot counter
    uint32_t crash_count;           // Unexpected reboot counter
    float pending_distance;         // Unsaved distance accumulator
    float pending_fuel;             // Unsaved fuel accumulator
} system_state_t;

/**
 * @brief NVS storage context
 */
typedef struct {
    bool initialized;
    trip_data_t trip_a;
    trip_data_t trip_b;
    lifetime_stats_t lifetime;
    stored_dtc_t dtc_history[NVS_MAX_DTC_HISTORY];
    uint8_t dtc_count;
    user_settings_t settings;
    system_state_t system;
    
    // Dirty flags for write batching
    bool trip_a_dirty;
    bool trip_b_dirty;
    bool lifetime_dirty;
    bool dtc_dirty;
    bool settings_dirty;
    
    // Accumulators for batched updates
    float distance_accumulator;
    float fuel_accumulator;
    uint32_t last_save_time_ms;
} nvs_storage_t;

/*===========================================================================*/
/*                        FUNCTION DECLARATIONS                             */
/*===========================================================================*/

/**
 * @brief Initialize NVS storage system
 * @param storage Storage context
 * @return true if initialization successful
 */
bool nvs_storage_init(nvs_storage_t* storage);

/**
 * @brief Load all data from NVS flash
 * @param storage Storage context
 * @return true if load successful
 */
bool nvs_storage_load_all(nvs_storage_t* storage);

/**
 * @brief Save all dirty data to NVS flash
 * @param storage Storage context
 * @return true if save successful
 */
bool nvs_storage_save_all(nvs_storage_t* storage);

/**
 * @brief Emergency save critical data (on power loss)
 * @param storage Storage context
 * @return true if save successful
 */
bool nvs_storage_emergency_save(nvs_storage_t* storage);

/**
 * @brief Periodic update - call regularly to batch writes
 * @param storage Storage context
 * @param current_time_ms Current timestamp
 * @param distance_delta_km Distance traveled since last call
 * @param fuel_delta_liters Fuel consumed since last call
 */
void nvs_storage_periodic_update(nvs_storage_t* storage, uint32_t current_time_ms,
                                  float distance_delta_km, float fuel_delta_liters);

/*===========================================================================*/
/*                        TRIP MANAGEMENT                                   */
/*===========================================================================*/

/**
 * @brief Reset a trip counter
 * @param storage Storage context
 * @param trip_id 0 = Trip A, 1 = Trip B
 * @param current_time Unix timestamp
 */
void nvs_trip_reset(nvs_storage_t* storage, uint8_t trip_id, uint32_t current_time);

/**
 * @brief Update trip data
 * @param storage Storage context
 * @param trip_id 0 = Trip A, 1 = Trip B
 * @param distance_delta_km Distance to add
 * @param fuel_delta_liters Fuel to add
 * @param duration_delta_sec Driving time to add
 */
void nvs_trip_update(nvs_storage_t* storage, uint8_t trip_id,
                     float distance_delta_km, float fuel_delta_liters,
                     uint32_t duration_delta_sec);

/**
 * @brief Get trip data
 * @param storage Storage context
 * @param trip_id 0 = Trip A, 1 = Trip B
 * @return Pointer to trip data (do not modify directly)
 */
const trip_data_t* nvs_trip_get(nvs_storage_t* storage, uint8_t trip_id);

/**
 * @brief Calculate trip fuel economy
 * @param trip Trip data
 * @return Fuel economy in L/100km, or 0 if insufficient data
 */
float nvs_trip_get_fuel_economy(const trip_data_t* trip);

/*===========================================================================*/
/*                        LIFETIME STATISTICS                               */
/*===========================================================================*/

/**
 * @brief Update lifetime statistics
 * @param storage Storage context
 * @param distance_delta_km Distance to add
 * @param fuel_delta_liters Fuel to add
 */
void nvs_lifetime_update(nvs_storage_t* storage, float distance_delta_km,
                         float fuel_delta_liters);

/**
 * @brief Get lifetime statistics
 * @param storage Storage context
 * @return Pointer to lifetime stats
 */
const lifetime_stats_t* nvs_lifetime_get(nvs_storage_t* storage);

/**
 * @brief Update engine hours from ECU
 * @param storage Storage context
 * @param hours Current engine hours from ECU
 */
void nvs_lifetime_set_engine_hours(nvs_storage_t* storage, float hours);

/*===========================================================================*/
/*                        FAULT CODE HISTORY                                */
/*===========================================================================*/

/**
 * @brief Store a fault code
 * @param storage Storage context
 * @param spn Suspect Parameter Number
 * @param fmi Failure Mode Identifier
 * @param source_address Source ECU address
 * @param timestamp Current timestamp
 * @param is_active Currently active?
 */
void nvs_dtc_store(nvs_storage_t* storage, uint32_t spn, uint8_t fmi,
                   uint8_t source_address, uint32_t timestamp, bool is_active);

/**
 * @brief Clear active fault codes (mark as historical)
 * @param storage Storage context
 */
void nvs_dtc_clear_active(nvs_storage_t* storage);

/**
 * @brief Clear all fault code history
 * @param storage Storage context
 */
void nvs_dtc_clear_all(nvs_storage_t* storage);

/**
 * @brief Get fault code history
 * @param storage Storage context
 * @param count Output: number of stored DTCs
 * @return Pointer to DTC array
 */
const stored_dtc_t* nvs_dtc_get_history(nvs_storage_t* storage, uint8_t* count);

/**
 * @brief Get count of currently active DTCs
 * @param storage Storage context
 * @return Number of active DTCs
 */
uint8_t nvs_dtc_get_active_count(nvs_storage_t* storage);

/*===========================================================================*/
/*                        USER SETTINGS                                     */
/*===========================================================================*/

/**
 * @brief Get user settings
 * @param storage Storage context
 * @return Pointer to settings
 */
const user_settings_t* nvs_settings_get(nvs_storage_t* storage);

/**
 * @brief Update user settings
 * @param storage Storage context
 * @param settings New settings to save
 */
void nvs_settings_set(nvs_storage_t* storage, const user_settings_t* settings);

/**
 * @brief Reset settings to defaults
 * @param storage Storage context
 */
void nvs_settings_reset_defaults(nvs_storage_t* storage);

/*===========================================================================*/
/*                        SYSTEM STATE                                      */
/*===========================================================================*/

/**
 * @brief Mark system as cleanly shutting down
 * @param storage Storage context
 */
void nvs_system_shutdown(nvs_storage_t* storage);

/**
 * @brief Check if last shutdown was clean
 * @param storage Storage context
 * @return true if last shutdown was clean
 */
bool nvs_system_was_clean_shutdown(nvs_storage_t* storage);

/**
 * @brief Get boot count
 * @param storage Storage context
 * @return Number of boots
 */
uint32_t nvs_system_get_boot_count(nvs_storage_t* storage);

#ifdef __cplusplus
}
#endif

#endif /* NVS_STORAGE_H */
