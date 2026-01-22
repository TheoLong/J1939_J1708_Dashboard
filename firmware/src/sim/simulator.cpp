/**
 * @file simulator.cpp
 * @brief Simulation mode implementation
 * 
 * Generates realistic J1939/J1708 data without hardware.
 */

#include "simulator.h"
#include "../can/j1939_parser.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifndef ARDUINO
// For native builds, provide millis() stub
static uint32_t _sim_millis = 0;
uint32_t millis(void) { return _sim_millis; }
#else
#include <Arduino.h>
#endif

/*===========================================================================*/
/*                        PRIVATE DATA                                      */
/*===========================================================================*/

static struct {
    bool initialized;
    bool running;
    sim_scenario_t scenario;
    sim_timing_config_t timing;
    sim_vehicle_state_t state;
    sim_can_frame_cb_t can_callback;
    sim_j1708_msg_cb_t j1708_callback;
    
    // Timing trackers
    uint32_t last_eec1_ms;
    uint32_t last_eec2_ms;
    uint32_t last_et1_ms;
    uint32_t last_ccvs_ms;
    uint32_t last_lfe_ms;
    uint32_t last_etc2_ms;
    uint32_t last_vep1_ms;
    uint32_t last_dd_ms;
    uint32_t last_dm1_ms;
    
    // Simulation time
    uint32_t sim_elapsed_ms;
    
    // For state evolution
    float target_rpm;
    float target_speed;
    float target_throttle;
    
} sim_ctx;

/*===========================================================================*/
/*                        RANDOM HELPERS                                    */
/*===========================================================================*/

static float random_float(float min, float max) {
    float r = (float)rand() / (float)RAND_MAX;
    return min + r * (max - min);
}

static float clamp(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

static float lerp(float current, float target, float rate) {
    float diff = target - current;
    if (fabs(diff) < rate) return target;
    return current + (diff > 0 ? rate : -rate);
}

/*===========================================================================*/
/*                        CAN FRAME BUILDERS                                */
/*===========================================================================*/

static void build_pgn_61444_eec1(uint8_t* data) {
    // EEC1 - Electronic Engine Controller 1
    // Byte 0-3: Engine torque mode, driver demand, actual torque
    // Byte 4-5: Engine speed (0.125 RPM/bit)
    // Byte 6-7: Source address, starter mode
    
    memset(data, 0xFF, 8);
    
    // Torque mode (byte 0, bits 0-3)
    data[0] = 0x01;  // Accelerator pedal
    
    // Driver demand torque (byte 1, offset -125)
    int8_t demand = (int8_t)(sim_ctx.state.throttle_position * 0.5f);
    data[1] = (uint8_t)(demand + 125);
    
    // Actual torque (byte 2, offset -125)
    int8_t actual = (int8_t)(sim_ctx.state.engine_load * 1.25f - 25);
    data[2] = (uint8_t)(actual + 125);
    
    // Engine speed (bytes 3-4, 0.125 RPM/bit, little-endian)
    uint16_t rpm_raw = (uint16_t)(sim_ctx.state.engine_rpm / 0.125f);
    data[3] = rpm_raw & 0xFF;
    data[4] = (rpm_raw >> 8) & 0xFF;
    
    // Source address (byte 5)
    data[5] = 0x00;  // Engine ECU
    
    // Starter mode (byte 6, bits 0-3)
    data[6] = sim_ctx.state.engine_rpm > 0 ? 0x03 : 0x00;  // Start finished or not requested
}

static void build_pgn_61443_eec2(uint8_t* data) {
    // EEC2 - Electronic Engine Controller 2
    memset(data, 0xFF, 8);
    
    // Accelerator pedal position 1 (byte 1, 0.4%/bit)
    data[1] = (uint8_t)(sim_ctx.state.throttle_position / 0.4f);
    
    // Engine load (byte 2, 1%/bit)
    data[2] = (uint8_t)sim_ctx.state.engine_load;
    
    // Remote accel position (byte 3)
    data[3] = 0x00;
    
    // Max available torque (byte 5, 0.4%/bit)
    data[5] = 250;  // 100%
}

static void build_pgn_65262_et1(uint8_t* data) {
    // ET1 - Engine Temperature 1
    memset(data, 0xFF, 8);
    
    // Coolant temp (byte 0, offset -40)
    data[0] = (uint8_t)(sim_ctx.state.coolant_temp_c + 40);
    
    // Fuel temp (byte 1, offset -40)
    data[1] = (uint8_t)(sim_ctx.state.coolant_temp_c - 10 + 40);
    
    // Oil temp (bytes 2-3, 0.03125°C/bit, offset -273, little-endian)
    uint16_t oil_raw = (uint16_t)((sim_ctx.state.oil_temp_c + 273) / 0.03125f);
    data[2] = oil_raw & 0xFF;
    data[3] = (oil_raw >> 8) & 0xFF;
}

static void build_pgn_65263_eflp1(uint8_t* data) {
    // EFLP1 - Engine Fluid Level/Pressure 1
    memset(data, 0xFF, 8);
    
    // Fuel delivery pressure (byte 1, 4 kPa/bit)
    data[1] = (uint8_t)(300 / 4);  // 300 kPa typical
    
    // Oil pressure (byte 3, 4 kPa/bit)
    data[3] = (uint8_t)(sim_ctx.state.oil_pressure_kpa / 4);
    
    // Coolant pressure (byte 4, 2 kPa/bit)
    data[4] = (uint8_t)(100 / 2);  // 100 kPa typical
}

static void build_pgn_65265_ccvs(uint8_t* data) {
    // CCVS - Cruise Control/Vehicle Speed
    memset(data, 0xFF, 8);
    
    // Switches (byte 0)
    data[0] = 0x00;
    if (sim_ctx.state.parking_brake) data[0] |= 0x04;
    
    // Vehicle speed (bytes 1-2, 1/256 km/h/bit, little-endian)
    uint16_t speed_raw = (uint16_t)(sim_ctx.state.vehicle_speed_kmh * 256);
    data[1] = speed_raw & 0xFF;
    data[2] = (speed_raw >> 8) & 0xFF;
    
    // Cruise/brake/clutch switches (byte 3)
    data[3] = 0x00;
    if (sim_ctx.state.cruise_active) data[3] |= 0x01;
    if (sim_ctx.state.brake_switch) data[3] |= 0x10;
    if (sim_ctx.state.clutch_switch) data[3] |= 0x40;
    
    // Cruise set speed (byte 5)
    data[5] = sim_ctx.state.cruise_set_speed;
}

static void build_pgn_65266_lfe(uint8_t* data) {
    // LFE - Liquid Fuel Economy
    memset(data, 0xFF, 8);
    
    // Fuel rate (bytes 0-1, 0.05 L/h/bit, little-endian)
    uint16_t fuel_raw = (uint16_t)(sim_ctx.state.fuel_rate_lph / 0.05f);
    data[0] = fuel_raw & 0xFF;
    data[1] = (fuel_raw >> 8) & 0xFF;
    
    // Instantaneous fuel economy (bytes 2-3, skip)
    
    // Throttle position (byte 6, 0.4%/bit)
    data[6] = (uint8_t)(sim_ctx.state.throttle_position / 0.4f);
}

static void build_pgn_65270_ic1(uint8_t* data) {
    // IC1 - Inlet/Exhaust Conditions 1
    memset(data, 0xFF, 8);
    
    // Boost pressure (byte 1, 2 kPa/bit)
    data[1] = (uint8_t)(sim_ctx.state.boost_pressure_kpa / 2);
    
    // Intake manifold temp (byte 2, offset -40)
    data[2] = (uint8_t)(sim_ctx.state.ambient_temp_c + 20 + 40);  // Intake is warmer
}

static void build_pgn_65271_vep1(uint8_t* data) {
    // VEP1 - Vehicle Electrical Power 1
    memset(data, 0xFF, 8);
    
    // Charging system voltage (bytes 4-5, 0.05V/bit, little-endian)
    uint16_t charging_raw = (uint16_t)(sim_ctx.state.battery_voltage / 0.05f);
    data[4] = charging_raw & 0xFF;
    data[5] = (charging_raw >> 8) & 0xFF;
    
    // Battery voltage (bytes 6-7, 0.05V/bit, little-endian)
    uint16_t batt_raw = (uint16_t)((sim_ctx.state.battery_voltage - 0.3f) / 0.05f);
    data[6] = batt_raw & 0xFF;
    data[7] = (batt_raw >> 8) & 0xFF;
}

static void build_pgn_65272_trf1(uint8_t* data) {
    // TRF1 - Transmission Fluids 1
    memset(data, 0xFF, 8);
    
    // Trans oil level (byte 1, 0.4%/bit)
    data[1] = (uint8_t)(75 / 0.4f);  // 75% level
    
    // Trans oil pressure (byte 3, 16 kPa/bit)
    data[3] = (uint8_t)(1600 / 16);  // 1600 kPa
    
    // Trans oil temp (bytes 4-5, 0.03125°C/bit, offset -273, little-endian)
    uint16_t temp_raw = (uint16_t)((sim_ctx.state.trans_oil_temp_c + 273) / 0.03125f);
    data[4] = temp_raw & 0xFF;
    data[5] = (temp_raw >> 8) & 0xFF;
}

static void build_pgn_61445_etc2(uint8_t* data) {
    // ETC2 - Electronic Transmission Controller 2
    memset(data, 0xFF, 8);
    
    // Selected gear (byte 0, offset -125)
    data[0] = (uint8_t)(sim_ctx.state.selected_gear + 125);
    
    // Gear ratio (bytes 1-2, skip)
    
    // Current gear (byte 3, offset -125)
    data[3] = (uint8_t)(sim_ctx.state.current_gear + 125);
}

static void build_pgn_65276_dd(uint8_t* data) {
    // DD - Dash Display
    memset(data, 0xFF, 8);
    
    // Washer fluid (byte 0, 0.4%/bit)
    data[0] = (uint8_t)(80 / 0.4f);  // 80%
    
    // Fuel level (byte 1, 0.4%/bit)
    data[1] = (uint8_t)(sim_ctx.state.fuel_level_pct / 0.4f);
}

static void build_pgn_65253_hours(uint8_t* data) {
    // HOURS - Engine Hours, Revolutions
    memset(data, 0xFF, 8);
    
    // Engine hours (bytes 0-3, 0.05 hr/bit, little-endian)
    uint32_t hours_raw = (uint32_t)(sim_ctx.state.engine_hours / 0.05f);
    data[0] = hours_raw & 0xFF;
    data[1] = (hours_raw >> 8) & 0xFF;
    data[2] = (hours_raw >> 16) & 0xFF;
    data[3] = (hours_raw >> 24) & 0xFF;
}

static void build_pgn_65226_dm1(uint8_t* data) {
    // DM1 - Active Diagnostic Trouble Codes
    memset(data, 0x00, 8);
    
    if (sim_ctx.state.has_active_fault) {
        // Lamp status (byte 0)
        data[0] = 0x14;  // Amber warning on, MIL on
        data[1] = 0x00;
        
        // SPN (19 bits starting at byte 2)
        uint32_t spn = sim_ctx.state.fault_spn;
        data[2] = spn & 0xFF;
        data[3] = (spn >> 8) & 0xFF;
        data[4] = ((spn >> 16) & 0x07);
        
        // FMI (5 bits)
        data[4] |= (sim_ctx.state.fault_fmi & 0x1F) << 3;
        
        // Occurrence count
        data[5] = sim_ctx.state.fault_occurrence;
    } else {
        // No faults - all lamps off
        data[0] = 0x00;
        data[1] = 0x00;
        data[2] = 0xFF;  // No DTC
        data[3] = 0xFF;
        data[4] = 0xFF;
        data[5] = 0xFF;
    }
}

/*===========================================================================*/
/*                        FRAME TRANSMISSION                                */
/*===========================================================================*/

static void send_can_frame(uint32_t pgn, uint8_t source_addr, const uint8_t* data) {
    if (sim_ctx.can_callback == NULL) return;
    
    // Build 29-bit CAN ID: Priority (3) + Reserved (1) + DP (1) + PGN (16) + SA (8)
    // Using priority 6 (default for most J1939 messages)
    uint32_t can_id = j1939_build_can_id(6, pgn, source_addr);
    
    sim_ctx.can_callback(can_id, data, 8);
}

/*===========================================================================*/
/*                        STATE EVOLUTION                                   */
/*===========================================================================*/

static void update_state_idle(float delta_s) {
    sim_ctx.target_rpm = 700 + random_float(-20, 20);
    sim_ctx.target_speed = 0;
    sim_ctx.target_throttle = 0;
    
    sim_ctx.state.engine_rpm = lerp(sim_ctx.state.engine_rpm, sim_ctx.target_rpm, 50 * delta_s);
    sim_ctx.state.vehicle_speed_kmh = 0;
    sim_ctx.state.throttle_position = lerp(sim_ctx.state.throttle_position, 0, 20 * delta_s);
    sim_ctx.state.engine_load = 15 + random_float(-2, 2);
    sim_ctx.state.fuel_rate_lph = 3.0f + random_float(-0.2f, 0.2f);
    sim_ctx.state.boost_pressure_kpa = 100 + random_float(-5, 5);
    sim_ctx.state.current_gear = 0;  // Neutral
    sim_ctx.state.parking_brake = true;
    
    // Temperatures stabilize
    sim_ctx.state.coolant_temp_c = lerp(sim_ctx.state.coolant_temp_c, 85, 0.5f * delta_s);
    sim_ctx.state.oil_temp_c = lerp(sim_ctx.state.oil_temp_c, 95, 0.3f * delta_s);
    sim_ctx.state.trans_oil_temp_c = lerp(sim_ctx.state.trans_oil_temp_c, 75, 0.3f * delta_s);
}

static void update_state_highway(float delta_s) {
    sim_ctx.target_rpm = 1400 + random_float(-30, 30);
    sim_ctx.target_speed = 105 + random_float(-2, 2);
    sim_ctx.target_throttle = 45 + random_float(-5, 5);
    
    sim_ctx.state.engine_rpm = lerp(sim_ctx.state.engine_rpm, sim_ctx.target_rpm, 100 * delta_s);
    sim_ctx.state.vehicle_speed_kmh = lerp(sim_ctx.state.vehicle_speed_kmh, sim_ctx.target_speed, 5 * delta_s);
    sim_ctx.state.throttle_position = lerp(sim_ctx.state.throttle_position, sim_ctx.target_throttle, 30 * delta_s);
    sim_ctx.state.engine_load = 55 + random_float(-5, 5);
    sim_ctx.state.fuel_rate_lph = 28.0f + random_float(-2.0f, 2.0f);
    sim_ctx.state.boost_pressure_kpa = 180 + random_float(-10, 10);
    sim_ctx.state.current_gear = 10;  // Top gear
    sim_ctx.state.selected_gear = 10;
    sim_ctx.state.parking_brake = false;
    sim_ctx.state.cruise_active = true;
    sim_ctx.state.cruise_set_speed = 105;
    
    // Temperatures at operating temp
    sim_ctx.state.coolant_temp_c = lerp(sim_ctx.state.coolant_temp_c, 88, 0.3f * delta_s);
    sim_ctx.state.oil_temp_c = lerp(sim_ctx.state.oil_temp_c, 105, 0.2f * delta_s);
    sim_ctx.state.trans_oil_temp_c = lerp(sim_ctx.state.trans_oil_temp_c, 85, 0.2f * delta_s);
    
    // Consume fuel
    sim_ctx.state.fuel_level_pct -= 0.001f * delta_s;
    if (sim_ctx.state.fuel_level_pct < 0) sim_ctx.state.fuel_level_pct = 100;
    
    // Accumulate distance and hours
    sim_ctx.state.odometer_km += sim_ctx.state.vehicle_speed_kmh * delta_s / 3600;
    sim_ctx.state.trip_km += sim_ctx.state.vehicle_speed_kmh * delta_s / 3600;
    sim_ctx.state.engine_hours += delta_s / 3600;
}

static void update_state_city(float delta_s) {
    // Cycle through phases: accelerate, cruise, brake, stop
    float cycle = fmod(sim_ctx.sim_elapsed_ms / 1000.0f, 60.0f);  // 60 second cycle
    
    if (cycle < 10) {
        // Accelerating from stop
        sim_ctx.target_rpm = 1800 + random_float(-50, 50);
        sim_ctx.target_speed = cycle * 5;  // 0-50 km/h
        sim_ctx.target_throttle = 60;
        sim_ctx.state.current_gear = (int8_t)(cycle / 2) + 1;
    } else if (cycle < 30) {
        // Cruising
        sim_ctx.target_rpm = 1200;
        sim_ctx.target_speed = 50;
        sim_ctx.target_throttle = 30;
        sim_ctx.state.current_gear = 5;
    } else if (cycle < 40) {
        // Braking
        sim_ctx.target_rpm = 800;
        sim_ctx.target_speed = 50 - (cycle - 30) * 5;
        sim_ctx.target_throttle = 0;
        sim_ctx.state.brake_switch = true;
    } else {
        // Stopped at light
        sim_ctx.target_rpm = 700;
        sim_ctx.target_speed = 0;
        sim_ctx.target_throttle = 0;
        sim_ctx.state.current_gear = 0;
        sim_ctx.state.brake_switch = true;
    }
    
    sim_ctx.state.engine_rpm = lerp(sim_ctx.state.engine_rpm, sim_ctx.target_rpm, 200 * delta_s);
    sim_ctx.state.vehicle_speed_kmh = lerp(sim_ctx.state.vehicle_speed_kmh, sim_ctx.target_speed, 10 * delta_s);
    sim_ctx.state.throttle_position = lerp(sim_ctx.state.throttle_position, sim_ctx.target_throttle, 50 * delta_s);
    
    if (cycle >= 40) {
        sim_ctx.state.brake_switch = false;
    }
}

static void update_state_cold_start(float delta_s) {
    float elapsed = sim_ctx.sim_elapsed_ms / 1000.0f;
    
    if (elapsed < 2) {
        // Cranking
        sim_ctx.state.engine_rpm = 200 + random_float(-30, 30);
        sim_ctx.state.battery_voltage = 10.5f + random_float(-0.5f, 0.5f);
    } else if (elapsed < 5) {
        // Starting
        sim_ctx.state.engine_rpm = lerp(sim_ctx.state.engine_rpm, 900, 200 * delta_s);
        sim_ctx.state.battery_voltage = lerp(sim_ctx.state.battery_voltage, 14.2f, 2 * delta_s);
    } else {
        // Fast idle warmup
        float warmup = clamp((elapsed - 5) / 180, 0, 1);  // 3 min warmup
        sim_ctx.target_rpm = 900 - 200 * warmup;  // Fast idle drops as warming
        sim_ctx.state.engine_rpm = lerp(sim_ctx.state.engine_rpm, sim_ctx.target_rpm, 50 * delta_s);
    }
    
    // Cold temps warming up
    float warmup_factor = clamp(elapsed / 300, 0, 1);  // 5 min to full temp
    sim_ctx.state.coolant_temp_c = lerp(sim_ctx.state.coolant_temp_c, -10 + 95 * warmup_factor, 0.5f * delta_s);
    sim_ctx.state.oil_temp_c = sim_ctx.state.coolant_temp_c - 10;
    sim_ctx.state.oil_pressure_kpa = 150 + 150 * (1 - warmup_factor) + random_float(-10, 10);
    
    sim_ctx.state.vehicle_speed_kmh = 0;
    sim_ctx.state.parking_brake = true;
}

static void update_state_acceleration(float delta_s) {
    float elapsed = sim_ctx.sim_elapsed_ms / 1000.0f;
    
    if (elapsed < 15) {
        // Full throttle acceleration
        sim_ctx.state.throttle_position = 100;
        sim_ctx.state.engine_load = 95 + random_float(-3, 3);
        
        // Shift through gears
        if (sim_ctx.state.engine_rpm > 2000) {
            if (sim_ctx.state.current_gear < 10) {
                sim_ctx.state.current_gear++;
                sim_ctx.state.engine_rpm = 1200;
            }
        }
        
        sim_ctx.target_rpm = 2200;
        sim_ctx.state.engine_rpm = lerp(sim_ctx.state.engine_rpm, sim_ctx.target_rpm, 400 * delta_s);
        
        // Speed increases with each gear
        sim_ctx.target_speed = sim_ctx.state.current_gear * 12.0f;
        sim_ctx.state.vehicle_speed_kmh = lerp(sim_ctx.state.vehicle_speed_kmh, sim_ctx.target_speed, 5 * delta_s);
        
        sim_ctx.state.boost_pressure_kpa = 250 + random_float(-10, 10);
        sim_ctx.state.fuel_rate_lph = 80 + random_float(-5, 5);
    } else {
        // Coast down
        sim_ctx.state.throttle_position = lerp(sim_ctx.state.throttle_position, 0, 30 * delta_s);
        sim_ctx.state.engine_rpm = lerp(sim_ctx.state.engine_rpm, 1200, 100 * delta_s);
    }
    
    sim_ctx.state.parking_brake = false;
    sim_ctx.state.selected_gear = sim_ctx.state.current_gear;
}

static void update_state_fault(float delta_s) {
    // Run highway scenario but with active fault
    update_state_highway(delta_s);
    
    if (!sim_ctx.state.has_active_fault) {
        // Trigger a fault
        sim_ctx.state.has_active_fault = true;
        sim_ctx.state.fault_spn = 110;   // Engine Coolant Temperature
        sim_ctx.state.fault_fmi = 0;     // Data Valid Above Normal Range
        sim_ctx.state.fault_occurrence = 1;
        
        // Simulate overheating
        sim_ctx.state.coolant_temp_c = 105;
    }
}

/*===========================================================================*/
/*                        PUBLIC IMPLEMENTATION                             */
/*===========================================================================*/

void sim_init(sim_can_frame_cb_t can_cb, sim_j1708_msg_cb_t j1708_cb) {
    memset(&sim_ctx, 0, sizeof(sim_ctx));
    
    sim_ctx.can_callback = can_cb;
    sim_ctx.j1708_callback = j1708_cb;
    sim_ctx.scenario = SIM_SCENARIO_IDLE;
    
    // Set default timing
    sim_get_default_timing(&sim_ctx.timing);
    
    // Initialize state
    sim_ctx.state.coolant_temp_c = 85;
    sim_ctx.state.oil_temp_c = 95;
    sim_ctx.state.trans_oil_temp_c = 75;
    sim_ctx.state.oil_pressure_kpa = 350;
    sim_ctx.state.battery_voltage = 13.8f;
    sim_ctx.state.fuel_level_pct = 75;
    sim_ctx.state.ambient_temp_c = 25;
    sim_ctx.state.engine_hours = 12500;
    sim_ctx.state.odometer_km = 450000;
    
    sim_ctx.initialized = true;
}

void sim_set_scenario(sim_scenario_t scenario) {
    sim_ctx.scenario = scenario;
    sim_ctx.sim_elapsed_ms = 0;
    
    // Reset some state based on scenario
    if (scenario == SIM_SCENARIO_COLD_START) {
        sim_ctx.state.coolant_temp_c = -10;
        sim_ctx.state.oil_temp_c = -5;
        sim_ctx.state.engine_rpm = 0;
        sim_ctx.state.battery_voltage = 12.4f;
    }
}

void sim_set_timing(const sim_timing_config_t* timing) {
    if (timing) {
        sim_ctx.timing = *timing;
    }
}

void sim_start(void) {
    sim_ctx.running = true;
    sim_ctx.sim_elapsed_ms = 0;
}

void sim_stop(void) {
    sim_ctx.running = false;
}

bool sim_is_running(void) {
    return sim_ctx.running;
}

void sim_update(uint32_t delta_ms) {
    if (!sim_ctx.initialized || !sim_ctx.running) return;
    
    float delta_s = delta_ms / 1000.0f;
    sim_ctx.sim_elapsed_ms += delta_ms;
    
#ifndef ARDUINO
    _sim_millis += delta_ms;
#endif
    
    // Update vehicle state based on scenario
    switch (sim_ctx.scenario) {
        case SIM_SCENARIO_IDLE:
            update_state_idle(delta_s);
            break;
        case SIM_SCENARIO_HIGHWAY:
            update_state_highway(delta_s);
            break;
        case SIM_SCENARIO_CITY:
            update_state_city(delta_s);
            break;
        case SIM_SCENARIO_COLD_START:
            update_state_cold_start(delta_s);
            break;
        case SIM_SCENARIO_ACCELERATION:
            update_state_acceleration(delta_s);
            break;
        case SIM_SCENARIO_FAULT:
            update_state_fault(delta_s);
            break;
        case SIM_SCENARIO_CUSTOM:
            // User controls state directly
            break;
    }
    
    // Oil pressure varies with RPM
    if (sim_ctx.state.engine_rpm > 0) {
        float rpm_factor = sim_ctx.state.engine_rpm / 2000.0f;
        sim_ctx.state.oil_pressure_kpa = 200 + 200 * rpm_factor + random_float(-10, 10);
    } else {
        sim_ctx.state.oil_pressure_kpa = 0;
    }
    
    // Clamp values
    sim_ctx.state.engine_rpm = clamp(sim_ctx.state.engine_rpm, 0, 2800);
    sim_ctx.state.vehicle_speed_kmh = clamp(sim_ctx.state.vehicle_speed_kmh, 0, 150);
    sim_ctx.state.coolant_temp_c = clamp(sim_ctx.state.coolant_temp_c, -40, 120);
    sim_ctx.state.fuel_level_pct = clamp(sim_ctx.state.fuel_level_pct, 0, 100);
    
    // Generate CAN frames based on timing
    uint32_t now = sim_ctx.sim_elapsed_ms;
    uint8_t data[8];
    
    // EEC1 - Engine speed, torque
    if (now - sim_ctx.last_eec1_ms >= sim_ctx.timing.eec1_interval_ms) {
        build_pgn_61444_eec1(data);
        send_can_frame(61444, 0x00, data);
        sim_ctx.last_eec1_ms = now;
    }
    
    // EEC2 - Throttle, load
    if (now - sim_ctx.last_eec2_ms >= sim_ctx.timing.eec2_interval_ms) {
        build_pgn_61443_eec2(data);
        send_can_frame(61443, 0x00, data);
        sim_ctx.last_eec2_ms = now;
    }
    
    // ET1 - Temperatures
    if (now - sim_ctx.last_et1_ms >= sim_ctx.timing.et1_interval_ms) {
        build_pgn_65262_et1(data);
        send_can_frame(65262, 0x00, data);
        
        build_pgn_65263_eflp1(data);
        send_can_frame(65263, 0x00, data);
        
        build_pgn_65270_ic1(data);
        send_can_frame(65270, 0x00, data);
        
        build_pgn_65272_trf1(data);
        send_can_frame(65272, 0x03, data);  // Transmission SA
        
        build_pgn_65253_hours(data);
        send_can_frame(65253, 0x00, data);
        
        sim_ctx.last_et1_ms = now;
    }
    
    // CCVS - Vehicle speed
    if (now - sim_ctx.last_ccvs_ms >= sim_ctx.timing.ccvs_interval_ms) {
        build_pgn_65265_ccvs(data);
        send_can_frame(65265, 0x00, data);
        sim_ctx.last_ccvs_ms = now;
    }
    
    // LFE - Fuel rate
    if (now - sim_ctx.last_lfe_ms >= sim_ctx.timing.lfe_interval_ms) {
        build_pgn_65266_lfe(data);
        send_can_frame(65266, 0x00, data);
        sim_ctx.last_lfe_ms = now;
    }
    
    // ETC2 - Gear
    if (now - sim_ctx.last_etc2_ms >= sim_ctx.timing.etc2_interval_ms) {
        build_pgn_61445_etc2(data);
        send_can_frame(61445, 0x03, data);  // Transmission SA
        sim_ctx.last_etc2_ms = now;
    }
    
    // VEP1 - Battery voltage
    if (now - sim_ctx.last_vep1_ms >= sim_ctx.timing.vep1_interval_ms) {
        build_pgn_65271_vep1(data);
        send_can_frame(65271, 0x00, data);
        sim_ctx.last_vep1_ms = now;
    }
    
    // DD - Fuel level
    if (now - sim_ctx.last_dd_ms >= sim_ctx.timing.dd_interval_ms) {
        build_pgn_65276_dd(data);
        send_can_frame(65276, 0x00, data);
        sim_ctx.last_dd_ms = now;
    }
    
    // DM1 - Active faults (every 1s if faults present, every 5s if clear)
    uint32_t dm1_interval = sim_ctx.state.has_active_fault ? 1000 : 5000;
    if (now - sim_ctx.last_dm1_ms >= dm1_interval) {
        build_pgn_65226_dm1(data);
        send_can_frame(65226, 0x00, data);
        sim_ctx.last_dm1_ms = now;
    }
}

const sim_vehicle_state_t* sim_get_state(void) {
    return &sim_ctx.state;
}

void sim_set_state(const sim_vehicle_state_t* state) {
    if (state) {
        sim_ctx.state = *state;
    }
}

void sim_trigger_fault(uint32_t spn, uint8_t fmi) {
    sim_ctx.state.has_active_fault = true;
    sim_ctx.state.fault_spn = spn;
    sim_ctx.state.fault_fmi = fmi;
    sim_ctx.state.fault_occurrence++;
}

void sim_clear_fault(void) {
    sim_ctx.state.has_active_fault = false;
}

void sim_get_default_timing(sim_timing_config_t* timing) {
    if (timing == NULL) return;
    
    timing->eec1_interval_ms = 10;      // Engine speed - fast
    timing->eec2_interval_ms = 50;      // Throttle
    timing->et1_interval_ms = 1000;     // Temperatures - slow
    timing->ccvs_interval_ms = 100;     // Vehicle speed
    timing->lfe_interval_ms = 100;      // Fuel rate
    timing->etc2_interval_ms = 100;     // Gear
    timing->vep1_interval_ms = 1000;    // Battery
    timing->dd_interval_ms = 1000;      // Fuel level
}
