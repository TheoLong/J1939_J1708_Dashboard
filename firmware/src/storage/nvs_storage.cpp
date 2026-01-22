/**
 * @file nvs_storage.cpp
 * @brief Non-Volatile Storage implementation
 * 
 * Implements persistent storage using ESP32 NVS or RAM-based simulation
 * for native testing.
 */

#include "nvs_storage.h"
#include <string.h>

#ifndef NATIVE_BUILD
#include <Preferences.h>
#include <Arduino.h>

// ESP32 Preferences instances for each namespace
static Preferences prefs_trip_a;
static Preferences prefs_trip_b;
static Preferences prefs_lifetime;
static Preferences prefs_dtc;
static Preferences prefs_settings;
static Preferences prefs_system;

#else
// Native build - use RAM storage simulation
static uint32_t _sim_time = 0;
uint32_t millis(void);
#endif

// Save interval configuration
#define SAVE_INTERVAL_MS        (5 * 60 * 1000)     // 5 minutes
#define DISTANCE_THRESHOLD_KM   1.0f                 // Save if >1km accumulated

/*===========================================================================*/
/*                        DEFAULT VALUES                                    */
/*===========================================================================*/

static const user_settings_t default_settings = {
    .units = 1,             // Imperial
    .brightness = 75,       // 75%
    .default_page = 0,      // Main page
    .temp_unit = 1,         // Fahrenheit
    .pressure_unit = 1,     // PSI
    .fuel_unit = 1,         // MPG
    .fuel_tank_1_size = 200,  // 200L (typical semi tank)
    .fuel_tank_2_size = 200
};

static void init_trip_defaults(trip_data_t* trip) {
    memset(trip, 0, sizeof(trip_data_t));
    trip->is_active = false;
}

static void init_lifetime_defaults(lifetime_stats_t* lifetime) {
    memset(lifetime, 0, sizeof(lifetime_stats_t));
    lifetime->best_mpg = 0.0f;
    lifetime->worst_mpg = 999.0f;
}

/*===========================================================================*/
/*                        INITIALIZATION                                    */
/*===========================================================================*/

bool nvs_storage_init(nvs_storage_t* storage) {
    if (storage == NULL) return false;
    
    memset(storage, 0, sizeof(nvs_storage_t));
    
    // Initialize with defaults
    init_trip_defaults(&storage->trip_a);
    init_trip_defaults(&storage->trip_b);
    init_lifetime_defaults(&storage->lifetime);
    storage->settings = default_settings;
    
    // Load from flash
    bool load_success = nvs_storage_load_all(storage);
    
    // Update system state
    storage->system.boot_count++;
    if (!storage->system.clean_shutdown) {
        storage->system.crash_count++;
    }
    storage->system.clean_shutdown = false;
    
    storage->initialized = true;
    
#ifndef NATIVE_BUILD
    // Save updated boot count
    prefs_system.begin("system", false);
    prefs_system.putUInt("boot_count", storage->system.boot_count);
    prefs_system.putUInt("crash_count", storage->system.crash_count);
    prefs_system.putBool("clean_shut", false);
    prefs_system.end();
#endif
    
    return load_success;
}

/*===========================================================================*/
/*                        LOAD/SAVE OPERATIONS                              */
/*===========================================================================*/

bool nvs_storage_load_all(nvs_storage_t* storage) {
    if (storage == NULL) return false;
    
#ifndef NATIVE_BUILD
    // Load Trip A
    if (prefs_trip_a.begin("trip_a", true)) {
        storage->trip_a.distance_km = prefs_trip_a.getFloat("distance", 0);
        storage->trip_a.fuel_used_liters = prefs_trip_a.getFloat("fuel", 0);
        storage->trip_a.start_time = prefs_trip_a.getUInt("start_time", 0);
        storage->trip_a.duration_seconds = prefs_trip_a.getUInt("duration", 0);
        storage->trip_a.avg_speed_kmh = prefs_trip_a.getFloat("avg_speed", 0);
        storage->trip_a.is_active = prefs_trip_a.getBool("active", false);
        prefs_trip_a.end();
    }
    
    // Load Trip B
    if (prefs_trip_b.begin("trip_b", true)) {
        storage->trip_b.distance_km = prefs_trip_b.getFloat("distance", 0);
        storage->trip_b.fuel_used_liters = prefs_trip_b.getFloat("fuel", 0);
        storage->trip_b.start_time = prefs_trip_b.getUInt("start_time", 0);
        storage->trip_b.duration_seconds = prefs_trip_b.getUInt("duration", 0);
        storage->trip_b.avg_speed_kmh = prefs_trip_b.getFloat("avg_speed", 0);
        storage->trip_b.is_active = prefs_trip_b.getBool("active", false);
        prefs_trip_b.end();
    }
    
    // Load Lifetime Stats
    if (prefs_lifetime.begin("lifetime", true)) {
        storage->lifetime.total_distance_km = prefs_lifetime.getFloat("total_dist", 0);
        storage->lifetime.total_fuel_liters = prefs_lifetime.getFloat("total_fuel", 0);
        storage->lifetime.engine_hours = prefs_lifetime.getFloat("eng_hours", 0);
        storage->lifetime.boot_count = prefs_lifetime.getUInt("boot_count", 0);
        storage->lifetime.best_mpg = prefs_lifetime.getFloat("best_mpg", 0);
        storage->lifetime.worst_mpg = prefs_lifetime.getFloat("worst_mpg", 999);
        storage->lifetime.first_boot_time = prefs_lifetime.getUInt("first_boot", 0);
        storage->lifetime.total_runtime_seconds = prefs_lifetime.getUInt("runtime", 0);
        prefs_lifetime.end();
    }
    
    // Load Settings
    if (prefs_settings.begin("settings", true)) {
        storage->settings.units = prefs_settings.getUChar("units", default_settings.units);
        storage->settings.brightness = prefs_settings.getUChar("brightness", default_settings.brightness);
        storage->settings.default_page = prefs_settings.getUChar("def_page", default_settings.default_page);
        storage->settings.temp_unit = prefs_settings.getUChar("temp_unit", default_settings.temp_unit);
        storage->settings.pressure_unit = prefs_settings.getUChar("press_unit", default_settings.pressure_unit);
        storage->settings.fuel_unit = prefs_settings.getUChar("fuel_unit", default_settings.fuel_unit);
        storage->settings.fuel_tank_1_size = prefs_settings.getUShort("tank1_size", default_settings.fuel_tank_1_size);
        storage->settings.fuel_tank_2_size = prefs_settings.getUShort("tank2_size", default_settings.fuel_tank_2_size);
        prefs_settings.end();
    }
    
    // Load System State
    if (prefs_system.begin("system", true)) {
        storage->system.clean_shutdown = prefs_system.getBool("clean_shut", true);
        storage->system.last_timestamp = prefs_system.getUInt("last_time", 0);
        storage->system.boot_count = prefs_system.getUInt("boot_count", 0);
        storage->system.crash_count = prefs_system.getUInt("crash_count", 0);
        storage->system.pending_distance = prefs_system.getFloat("pend_dist", 0);
        storage->system.pending_fuel = prefs_system.getFloat("pend_fuel", 0);
        prefs_system.end();
    }
    
    // Load DTC History (as a blob)
    if (prefs_dtc.begin("fault_log", true)) {
        storage->dtc_count = prefs_dtc.getUChar("count", 0);
        if (storage->dtc_count > 0 && storage->dtc_count <= NVS_MAX_DTC_HISTORY) {
            prefs_dtc.getBytes("dtcs", storage->dtc_history, 
                               storage->dtc_count * sizeof(stored_dtc_t));
        }
        prefs_dtc.end();
    }
#else
    // Native build - data already initialized with defaults
#endif
    
    return true;
}

bool nvs_storage_save_all(nvs_storage_t* storage) {
    if (storage == NULL || !storage->initialized) return false;
    
#ifndef NATIVE_BUILD
    // Save Trip A if dirty
    if (storage->trip_a_dirty) {
        prefs_trip_a.begin("trip_a", false);
        prefs_trip_a.putFloat("distance", storage->trip_a.distance_km);
        prefs_trip_a.putFloat("fuel", storage->trip_a.fuel_used_liters);
        prefs_trip_a.putUInt("start_time", storage->trip_a.start_time);
        prefs_trip_a.putUInt("duration", storage->trip_a.duration_seconds);
        prefs_trip_a.putFloat("avg_speed", storage->trip_a.avg_speed_kmh);
        prefs_trip_a.putBool("active", storage->trip_a.is_active);
        prefs_trip_a.end();
        storage->trip_a_dirty = false;
    }
    
    // Save Trip B if dirty
    if (storage->trip_b_dirty) {
        prefs_trip_b.begin("trip_b", false);
        prefs_trip_b.putFloat("distance", storage->trip_b.distance_km);
        prefs_trip_b.putFloat("fuel", storage->trip_b.fuel_used_liters);
        prefs_trip_b.putUInt("start_time", storage->trip_b.start_time);
        prefs_trip_b.putUInt("duration", storage->trip_b.duration_seconds);
        prefs_trip_b.putFloat("avg_speed", storage->trip_b.avg_speed_kmh);
        prefs_trip_b.putBool("active", storage->trip_b.is_active);
        prefs_trip_b.end();
        storage->trip_b_dirty = false;
    }
    
    // Save Lifetime if dirty
    if (storage->lifetime_dirty) {
        prefs_lifetime.begin("lifetime", false);
        prefs_lifetime.putFloat("total_dist", storage->lifetime.total_distance_km);
        prefs_lifetime.putFloat("total_fuel", storage->lifetime.total_fuel_liters);
        prefs_lifetime.putFloat("eng_hours", storage->lifetime.engine_hours);
        prefs_lifetime.putUInt("boot_count", storage->lifetime.boot_count);
        prefs_lifetime.putFloat("best_mpg", storage->lifetime.best_mpg);
        prefs_lifetime.putFloat("worst_mpg", storage->lifetime.worst_mpg);
        prefs_lifetime.putUInt("first_boot", storage->lifetime.first_boot_time);
        prefs_lifetime.putUInt("runtime", storage->lifetime.total_runtime_seconds);
        prefs_lifetime.end();
        storage->lifetime_dirty = false;
    }
    
    // Save Settings if dirty
    if (storage->settings_dirty) {
        prefs_settings.begin("settings", false);
        prefs_settings.putUChar("units", storage->settings.units);
        prefs_settings.putUChar("brightness", storage->settings.brightness);
        prefs_settings.putUChar("def_page", storage->settings.default_page);
        prefs_settings.putUChar("temp_unit", storage->settings.temp_unit);
        prefs_settings.putUChar("press_unit", storage->settings.pressure_unit);
        prefs_settings.putUChar("fuel_unit", storage->settings.fuel_unit);
        prefs_settings.putUShort("tank1_size", storage->settings.fuel_tank_1_size);
        prefs_settings.putUShort("tank2_size", storage->settings.fuel_tank_2_size);
        prefs_settings.end();
        storage->settings_dirty = false;
    }
    
    // Save DTC History if dirty
    if (storage->dtc_dirty) {
        prefs_dtc.begin("fault_log", false);
        prefs_dtc.putUChar("count", storage->dtc_count);
        if (storage->dtc_count > 0) {
            prefs_dtc.putBytes("dtcs", storage->dtc_history,
                               storage->dtc_count * sizeof(stored_dtc_t));
        }
        prefs_dtc.end();
        storage->dtc_dirty = false;
    }
#endif
    
    return true;
}

bool nvs_storage_emergency_save(nvs_storage_t* storage) {
    if (storage == NULL) return false;
    
    // Force all sections to be saved
    storage->trip_a_dirty = true;
    storage->trip_b_dirty = true;
    storage->lifetime_dirty = true;
    
    // Include accumulated but unsaved data
    if (storage->distance_accumulator > 0 || storage->fuel_accumulator > 0) {
        storage->trip_a.distance_km += storage->distance_accumulator;
        storage->trip_a.fuel_used_liters += storage->fuel_accumulator;
        storage->trip_b.distance_km += storage->distance_accumulator;
        storage->trip_b.fuel_used_liters += storage->fuel_accumulator;
        storage->lifetime.total_distance_km += storage->distance_accumulator;
        storage->lifetime.total_fuel_liters += storage->fuel_accumulator;
        storage->distance_accumulator = 0;
        storage->fuel_accumulator = 0;
    }
    
    return nvs_storage_save_all(storage);
}

void nvs_storage_periodic_update(nvs_storage_t* storage, uint32_t current_time_ms,
                                  float distance_delta_km, float fuel_delta_liters) {
    if (storage == NULL || !storage->initialized) return;
    
    // Accumulate data
    storage->distance_accumulator += distance_delta_km;
    storage->fuel_accumulator += fuel_delta_liters;
    
    // Check if we should save
    bool should_save = false;
    
    // Time-based save
    if ((current_time_ms - storage->last_save_time_ms) >= SAVE_INTERVAL_MS) {
        should_save = true;
    }
    
    // Threshold-based save
    if (storage->distance_accumulator >= DISTANCE_THRESHOLD_KM) {
        should_save = true;
    }
    
    if (should_save) {
        // Apply accumulated values to trips and lifetime
        storage->trip_a.distance_km += storage->distance_accumulator;
        storage->trip_a.fuel_used_liters += storage->fuel_accumulator;
        storage->trip_a_dirty = true;
        
        storage->trip_b.distance_km += storage->distance_accumulator;
        storage->trip_b.fuel_used_liters += storage->fuel_accumulator;
        storage->trip_b_dirty = true;
        
        storage->lifetime.total_distance_km += storage->distance_accumulator;
        storage->lifetime.total_fuel_liters += storage->fuel_accumulator;
        storage->lifetime_dirty = true;
        
        // Reset accumulators
        storage->distance_accumulator = 0;
        storage->fuel_accumulator = 0;
        
        // Save to flash
        nvs_storage_save_all(storage);
        storage->last_save_time_ms = current_time_ms;
    }
}

/*===========================================================================*/
/*                        TRIP MANAGEMENT                                   */
/*===========================================================================*/

void nvs_trip_reset(nvs_storage_t* storage, uint8_t trip_id, uint32_t current_time) {
    if (storage == NULL || trip_id > 1) return;
    
    trip_data_t* trip = (trip_id == 0) ? &storage->trip_a : &storage->trip_b;
    
    init_trip_defaults(trip);
    trip->start_time = current_time;
    trip->is_active = true;
    
    if (trip_id == 0) {
        storage->trip_a_dirty = true;
    } else {
        storage->trip_b_dirty = true;
    }
}

void nvs_trip_update(nvs_storage_t* storage, uint8_t trip_id,
                     float distance_delta_km, float fuel_delta_liters,
                     uint32_t duration_delta_sec) {
    if (storage == NULL || trip_id > 1) return;
    
    trip_data_t* trip = (trip_id == 0) ? &storage->trip_a : &storage->trip_b;
    
    trip->distance_km += distance_delta_km;
    trip->fuel_used_liters += fuel_delta_liters;
    trip->duration_seconds += duration_delta_sec;
    
    // Recalculate average speed
    if (trip->duration_seconds > 0) {
        trip->avg_speed_kmh = (trip->distance_km * 3600.0f) / trip->duration_seconds;
    }
    
    // Recalculate fuel economy
    trip->avg_fuel_economy = nvs_trip_get_fuel_economy(trip);
    
    if (trip_id == 0) {
        storage->trip_a_dirty = true;
    } else {
        storage->trip_b_dirty = true;
    }
}

const trip_data_t* nvs_trip_get(nvs_storage_t* storage, uint8_t trip_id) {
    if (storage == NULL || trip_id > 1) return NULL;
    return (trip_id == 0) ? &storage->trip_a : &storage->trip_b;
}

float nvs_trip_get_fuel_economy(const trip_data_t* trip) {
    if (trip == NULL || trip->distance_km < 1.0f) return 0.0f;
    
    // Return L/100km
    return (trip->fuel_used_liters * 100.0f) / trip->distance_km;
}

/*===========================================================================*/
/*                        LIFETIME STATISTICS                               */
/*===========================================================================*/

void nvs_lifetime_update(nvs_storage_t* storage, float distance_delta_km,
                         float fuel_delta_liters) {
    if (storage == NULL) return;
    
    storage->lifetime.total_distance_km += distance_delta_km;
    storage->lifetime.total_fuel_liters += fuel_delta_liters;
    storage->lifetime_dirty = true;
}

const lifetime_stats_t* nvs_lifetime_get(nvs_storage_t* storage) {
    if (storage == NULL) return NULL;
    return &storage->lifetime;
}

void nvs_lifetime_set_engine_hours(nvs_storage_t* storage, float hours) {
    if (storage == NULL) return;
    storage->lifetime.engine_hours = hours;
    storage->lifetime_dirty = true;
}

/*===========================================================================*/
/*                        FAULT CODE HISTORY                                */
/*===========================================================================*/

void nvs_dtc_store(nvs_storage_t* storage, uint32_t spn, uint8_t fmi,
                   uint8_t source_address, uint32_t timestamp, bool is_active) {
    if (storage == NULL) return;
    
    // Check if this DTC already exists
    for (uint8_t i = 0; i < storage->dtc_count; i++) {
        stored_dtc_t* dtc = &storage->dtc_history[i];
        if (dtc->spn == spn && dtc->fmi == fmi && dtc->source_address == source_address) {
            // Update existing entry
            dtc->last_seen = timestamp;
            dtc->occurrence_count++;
            dtc->is_active = is_active;
            storage->dtc_dirty = true;
            return;
        }
    }
    
    // Add new entry if space available
    if (storage->dtc_count < NVS_MAX_DTC_HISTORY) {
        stored_dtc_t* dtc = &storage->dtc_history[storage->dtc_count];
        dtc->spn = spn;
        dtc->fmi = fmi;
        dtc->source_address = source_address;
        dtc->first_seen = timestamp;
        dtc->last_seen = timestamp;
        dtc->occurrence_count = 1;
        dtc->is_active = is_active;
        storage->dtc_count++;
        storage->dtc_dirty = true;
    } else {
        // Replace oldest entry
        uint32_t oldest_time = UINT32_MAX;
        uint8_t oldest_idx = 0;
        for (uint8_t i = 0; i < NVS_MAX_DTC_HISTORY; i++) {
            if (storage->dtc_history[i].last_seen < oldest_time) {
                oldest_time = storage->dtc_history[i].last_seen;
                oldest_idx = i;
            }
        }
        stored_dtc_t* dtc = &storage->dtc_history[oldest_idx];
        dtc->spn = spn;
        dtc->fmi = fmi;
        dtc->source_address = source_address;
        dtc->first_seen = timestamp;
        dtc->last_seen = timestamp;
        dtc->occurrence_count = 1;
        dtc->is_active = is_active;
        storage->dtc_dirty = true;
    }
}

void nvs_dtc_clear_active(nvs_storage_t* storage) {
    if (storage == NULL) return;
    
    for (uint8_t i = 0; i < storage->dtc_count; i++) {
        storage->dtc_history[i].is_active = false;
    }
    storage->dtc_dirty = true;
}

void nvs_dtc_clear_all(nvs_storage_t* storage) {
    if (storage == NULL) return;
    
    storage->dtc_count = 0;
    memset(storage->dtc_history, 0, sizeof(storage->dtc_history));
    storage->dtc_dirty = true;
}

const stored_dtc_t* nvs_dtc_get_history(nvs_storage_t* storage, uint8_t* count) {
    if (storage == NULL) {
        if (count) *count = 0;
        return NULL;
    }
    
    if (count) *count = storage->dtc_count;
    return storage->dtc_history;
}

uint8_t nvs_dtc_get_active_count(nvs_storage_t* storage) {
    if (storage == NULL) return 0;
    
    uint8_t count = 0;
    for (uint8_t i = 0; i < storage->dtc_count; i++) {
        if (storage->dtc_history[i].is_active) {
            count++;
        }
    }
    return count;
}

/*===========================================================================*/
/*                        USER SETTINGS                                     */
/*===========================================================================*/

const user_settings_t* nvs_settings_get(nvs_storage_t* storage) {
    if (storage == NULL) return &default_settings;
    return &storage->settings;
}

void nvs_settings_set(nvs_storage_t* storage, const user_settings_t* settings) {
    if (storage == NULL || settings == NULL) return;
    
    memcpy(&storage->settings, settings, sizeof(user_settings_t));
    storage->settings_dirty = true;
}

void nvs_settings_reset_defaults(nvs_storage_t* storage) {
    if (storage == NULL) return;
    
    storage->settings = default_settings;
    storage->settings_dirty = true;
}

/*===========================================================================*/
/*                        SYSTEM STATE                                      */
/*===========================================================================*/

void nvs_system_shutdown(nvs_storage_t* storage) {
    if (storage == NULL) return;
    
    storage->system.clean_shutdown = true;
    
#ifndef NATIVE_BUILD
    // Immediately save shutdown flag
    prefs_system.begin("system", false);
    prefs_system.putBool("clean_shut", true);
    prefs_system.end();
#endif
    
    // Save all pending data
    nvs_storage_emergency_save(storage);
}

bool nvs_system_was_clean_shutdown(nvs_storage_t* storage) {
    if (storage == NULL) return true;
    return storage->system.clean_shutdown;
}

uint32_t nvs_system_get_boot_count(nvs_storage_t* storage) {
    if (storage == NULL) return 0;
    return storage->system.boot_count;
}
