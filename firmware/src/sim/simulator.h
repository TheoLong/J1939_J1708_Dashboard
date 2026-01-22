/**
 * @file simulator.h
 * @brief Simulation mode for testing without hardware
 * 
 * Generates realistic synthetic J1939/J1708 data for testing the
 * complete firmware data flow without physical CAN/J1708 buses.
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/*                        SIMULATION SCENARIOS                              */
/*===========================================================================*/

/**
 * @brief Predefined simulation scenarios
 */
typedef enum {
    SIM_SCENARIO_IDLE,          // Engine idling at parking lot
    SIM_SCENARIO_HIGHWAY,       // Highway cruising at 105 km/h
    SIM_SCENARIO_CITY,          // City driving with stops
    SIM_SCENARIO_COLD_START,    // Cold engine startup
    SIM_SCENARIO_ACCELERATION,  // Full throttle acceleration
    SIM_SCENARIO_FAULT,         // Engine with active DTCs
    SIM_SCENARIO_CUSTOM         // User-defined values
} sim_scenario_t;

/*===========================================================================*/
/*                        SIMULATED VEHICLE STATE                           */
/*===========================================================================*/

/**
 * @brief Complete simulated vehicle state
 */
typedef struct {
    // Engine
    float engine_rpm;               // 0-3000 RPM
    float engine_load;              // 0-100%
    float throttle_position;        // 0-100%
    float coolant_temp_c;           // -40 to 120°C
    float oil_temp_c;               // -40 to 150°C
    float oil_pressure_kpa;         // 0-1000 kPa
    float fuel_rate_lph;            // 0-100 L/h
    float boost_pressure_kpa;       // 0-400 kPa
    float engine_hours;             // Total hours
    
    // Transmission
    float trans_oil_temp_c;         // -40 to 150°C
    int8_t current_gear;            // -1=R, 0=N, 1-18
    int8_t selected_gear;           // Driver selected gear
    float output_shaft_rpm;         // 0-3000 RPM
    
    // Vehicle
    float vehicle_speed_kmh;        // 0-150 km/h
    float fuel_level_pct;           // 0-100%
    float battery_voltage;          // 0-32V
    float ambient_temp_c;           // -40 to 60°C
    float odometer_km;              // Total distance
    float trip_km;                  // Trip distance
    
    // Status flags
    bool parking_brake;
    bool brake_switch;
    bool clutch_switch;
    bool cruise_active;
    uint8_t cruise_set_speed;       // km/h
    
    // Fault simulation
    bool has_active_fault;
    uint32_t fault_spn;
    uint8_t fault_fmi;
    uint8_t fault_occurrence;
    
} sim_vehicle_state_t;

/*===========================================================================*/
/*                        SIMULATION CONFIGURATION                          */
/*===========================================================================*/

/**
 * @brief Simulation timing configuration
 */
typedef struct {
    uint32_t eec1_interval_ms;      // Engine speed, torque (default: 10ms)
    uint32_t eec2_interval_ms;      // Throttle, load (default: 50ms)
    uint32_t et1_interval_ms;       // Temperatures (default: 1000ms)
    uint32_t ccvs_interval_ms;      // Vehicle speed (default: 100ms)
    uint32_t lfe_interval_ms;       // Fuel rate (default: 100ms)
    uint32_t etc2_interval_ms;      // Gear info (default: 100ms)
    uint32_t vep1_interval_ms;      // Battery (default: 1000ms)
    uint32_t dd_interval_ms;        // Fuel level (default: 1000ms)
} sim_timing_config_t;

/**
 * @brief Complete simulation configuration
 */
typedef struct {
    sim_scenario_t scenario;
    sim_timing_config_t timing;
    bool enable_j1708;              // Also generate J1708 messages
    bool enable_faults;             // Generate random faults
    float time_scale;               // 1.0 = real-time, 2.0 = 2x speed
} sim_config_t;

/*===========================================================================*/
/*                        CALLBACK TYPES                                    */
/*===========================================================================*/

/**
 * @brief Callback for generated CAN frames
 */
typedef void (*sim_can_frame_cb_t)(uint32_t can_id, const uint8_t* data, uint8_t len);

/**
 * @brief Callback for generated J1708 messages
 */
typedef void (*sim_j1708_msg_cb_t)(uint8_t mid, const uint8_t* data, uint8_t len);

/*===========================================================================*/
/*                        PUBLIC API                                        */
/*===========================================================================*/

/**
 * @brief Initialize simulator with default configuration
 * @param can_cb Callback for CAN frames (required)
 * @param j1708_cb Callback for J1708 messages (optional, can be NULL)
 */
void sim_init(sim_can_frame_cb_t can_cb, sim_j1708_msg_cb_t j1708_cb);

/**
 * @brief Set simulation scenario
 * @param scenario Predefined scenario to use
 */
void sim_set_scenario(sim_scenario_t scenario);

/**
 * @brief Set custom timing configuration
 * @param timing Pointer to timing config (NULL for defaults)
 */
void sim_set_timing(const sim_timing_config_t* timing);

/**
 * @brief Start simulation
 */
void sim_start(void);

/**
 * @brief Stop simulation
 */
void sim_stop(void);

/**
 * @brief Check if simulation is running
 * @return true if running
 */
bool sim_is_running(void);

/**
 * @brief Update simulation state (call from main loop or timer)
 * @param delta_ms Milliseconds since last update
 * 
 * This function updates the vehicle state based on the current scenario
 * and generates CAN/J1708 messages at appropriate intervals.
 */
void sim_update(uint32_t delta_ms);

/**
 * @brief Get current simulated vehicle state (read-only)
 * @return Pointer to current state
 */
const sim_vehicle_state_t* sim_get_state(void);

/**
 * @brief Manually set vehicle state (for CUSTOM scenario)
 * @param state New state values
 */
void sim_set_state(const sim_vehicle_state_t* state);

/**
 * @brief Trigger a simulated fault
 * @param spn Suspect Parameter Number
 * @param fmi Failure Mode Indicator
 */
void sim_trigger_fault(uint32_t spn, uint8_t fmi);

/**
 * @brief Clear active fault
 */
void sim_clear_fault(void);

/**
 * @brief Get default timing configuration
 * @param timing Output timing config
 */
void sim_get_default_timing(sim_timing_config_t* timing);

#ifdef __cplusplus
}
#endif

#endif // SIMULATOR_H
