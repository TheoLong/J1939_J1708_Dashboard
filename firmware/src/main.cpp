/**
 * @file main.cpp
 * @brief Main application for J1939/J1708 Truck Dashboard
 * 
 * This file contains the main application entry point and core task structure.
 * The dashboard reads vehicle data from J1939 CAN and J1708 serial buses,
 * decodes parameters, and displays them on a TFT screen.
 * 
 * Build with -DSIMULATION_MODE=1 to run without hardware using simulated data.
 * 
 * @version 0.1.0
 * @date January 2026
 */

#include <Arduino.h>
#include "config.h"
#include "can/j1939_parser.h"
#include "j1708/j1708_parser.h"
#include "data/data_manager.h"
#include "data/watch_list_manager.h"
#include "storage/nvs_storage.h"

// Simulation mode
#ifdef SIMULATION_MODE
#include "sim/simulator.h"
#endif

// Include ESP32 CAN driver (when hardware available)
#ifndef NATIVE_BUILD
#include <driver/twai.h>
#endif

/*===========================================================================*/
/*                        GLOBAL INSTANCES                                  */
/*===========================================================================*/

// Parser contexts
static j1939_parser_context_t g_j1939_ctx;
static j1708_parser_context_t g_j1708_ctx;

// Data management
static data_manager_t g_data_manager;
static watch_list_manager_t g_watch_list;
static nvs_storage_t g_storage;

// Statistics
static uint32_t g_can_frames_received = 0;
static uint32_t g_j1708_messages_received = 0;
static uint32_t g_last_stats_time = 0;

// Simulation state
#ifdef SIMULATION_MODE
static sim_scenario_t g_sim_scenario = SIM_SCENARIO_HIGHWAY;
static uint32_t g_sim_last_update = 0;
#endif

/*===========================================================================*/
/*                        TASK HANDLES                                      */
/*===========================================================================*/

#ifndef NATIVE_BUILD
static TaskHandle_t g_can_task_handle = NULL;
static TaskHandle_t g_j1708_task_handle = NULL;
static TaskHandle_t g_display_task_handle = NULL;
static TaskHandle_t g_storage_task_handle = NULL;
#endif

/*===========================================================================*/
/*                        SIMULATION MODE                                   */
/*===========================================================================*/

#ifdef SIMULATION_MODE
/**
 * @brief Callback for simulated CAN frames
 * 
 * This function is called by the simulator when it generates a CAN frame.
 * It processes the frame through the normal parsing pipeline.
 */
static void sim_can_callback(uint32_t can_id, const uint8_t* data, uint8_t len) {
    g_can_frames_received++;
    
    // Parse the frame
    j1939_message_t msg;
    if (!j1939_parse_frame(can_id, data, len, millis(), &msg)) {
        return;
    }
    
    // Decode parameters based on PGN
    float value;
    
    switch (msg.pgn) {
        case 61444:  // EEC1 - Engine Speed
            value = j1939_decode_engine_speed(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_ENGINE_SPEED,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 61443:  // EEC2 - Throttle
            value = j1939_decode_throttle_position(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_THROTTLE_POSITION,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65262:  // ET1 - Coolant Temperature
            value = j1939_decode_coolant_temp(msg.data);
            if (value > -9000) {
                data_manager_update(&g_data_manager, PARAM_COOLANT_TEMP,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65263:  // EFLP1 - Oil Pressure
            value = j1939_decode_oil_pressure(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_OIL_PRESSURE,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65265:  // CCVS - Vehicle Speed
            value = j1939_decode_vehicle_speed(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_VEHICLE_SPEED,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65266:  // LFE - Fuel Rate
            value = j1939_decode_fuel_rate(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_FUEL_RATE,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65270:  // IC1 - Boost Pressure
            value = j1939_decode_boost_pressure(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_BOOST_PRESSURE,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65271:  // VEP1 - Battery Voltage
            value = j1939_decode_battery_voltage(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_BATTERY_VOLTAGE,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65272:  // TRF1 - Trans Oil Temp
            value = j1939_decode_trans_oil_temp(msg.data);
            if (value > -9000) {
                data_manager_update(&g_data_manager, PARAM_TRANS_OIL_TEMP,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65276:  // DD - Fuel Level
            value = j1939_decode_fuel_level(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_FUEL_LEVEL_1,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65253:  // HOURS - Engine Hours
            value = j1939_decode_engine_hours(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_ENGINE_HOURS,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 61445:  // ETC2 - Current Gear
            {
                int8_t gear = j1939_decode_current_gear(msg.data);
                if (gear > -126) {
                    data_manager_update(&g_data_manager, PARAM_CURRENT_GEAR,
                                       (float)gear, SOURCE_J1939, millis());
                }
            }
            break;
    }
}

/**
 * @brief Initialize simulation mode
 */
static void init_simulation(sim_scenario_t scenario) {
    Serial.println("  SIMULATION MODE ACTIVE");
    Serial.printf("  Scenario: %d\n", scenario);
    
    sim_init(sim_can_callback, NULL);  // No J1708 simulation for now
    sim_set_scenario(scenario);
    sim_start();
    
    g_sim_scenario = scenario;
    g_sim_last_update = millis();
}

/**
 * @brief Update simulation (call from main loop)
 */
static void update_simulation(void) {
    uint32_t now = millis();
    uint32_t delta = now - g_sim_last_update;
    
    if (delta > 0) {
        sim_update(delta);
        g_sim_last_update = now;
    }
}

/**
 * @brief Print simulation status
 */
static void print_sim_status(void) {
    const sim_vehicle_state_t* state = sim_get_state();
    
    Serial.println("--- Simulation State ---");
    Serial.printf("  RPM: %.0f  Speed: %.1f km/h  Gear: %d\n",
                  state->engine_rpm, state->vehicle_speed_kmh, state->current_gear);
    Serial.printf("  Coolant: %.1f°C  Oil: %.1f°C  Trans: %.1f°C\n",
                  state->coolant_temp_c, state->oil_temp_c, state->trans_oil_temp_c);
    Serial.printf("  Throttle: %.1f%%  Load: %.1f%%  Fuel: %.1f L/h\n",
                  state->throttle_position, state->engine_load, state->fuel_rate_lph);
    Serial.printf("  Battery: %.1fV  Fuel Level: %.1f%%\n",
                  state->battery_voltage, state->fuel_level_pct);
    if (state->has_active_fault) {
        Serial.printf("  FAULT: SPN %lu FMI %u\n", state->fault_spn, state->fault_fmi);
    }
}
#endif // SIMULATION_MODE

/*===========================================================================*/
/*                        CAN/J1939 FUNCTIONS                               */
/*===========================================================================*/

#ifndef NATIVE_BUILD
/**
 * @brief Initialize ESP32 TWAI (CAN) controller
 */
static bool init_can_bus(void) {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)PIN_CAN_TX, 
        (gpio_num_t)PIN_CAN_RX, 
        TWAI_MODE_NORMAL
    );
    g_config.rx_queue_len = J1939_RX_QUEUE_SIZE;
    g_config.tx_queue_len = J1939_TX_QUEUE_SIZE;
    
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    
    // Accept all extended (29-bit) frames
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        Serial.println("ERROR: Failed to install TWAI driver");
        return false;
    }
    
    if (twai_start() != ESP_OK) {
        Serial.println("ERROR: Failed to start TWAI driver");
        return false;
    }
    
    Serial.println("CAN bus initialized at 250 kbps");
    return true;
}

/**
 * @brief Process a received J1939 CAN frame
 */
static void process_j1939_frame(const twai_message_t* frame) {
    if (frame == NULL) return;
    if (!frame->extd) return;  // J1939 requires extended IDs
    
    g_can_frames_received++;
    
    // Parse the frame
    j1939_message_t msg;
    if (!j1939_parse_frame(frame->identifier, frame->data, frame->data_length_code,
                           millis(), &msg)) {
        return;
    }
    
    // Check for Transport Protocol frames
    if (msg.pgn == PGN_TP_CM || msg.pgn == PGN_TP_DT) {
        if (j1939_tp_handle_frame(&g_j1939_ctx, &msg)) {
            // TP message complete - process it
            uint32_t tp_pgn;
            uint8_t tp_buffer[256];
            uint16_t tp_len = j1939_tp_get_data(&g_j1939_ctx, msg.source_address,
                                                 &tp_pgn, tp_buffer, sizeof(tp_buffer));
            
            if (tp_len > 0 && tp_pgn == 65226) {  // DM1
                j1939_lamp_status_t lamps;
                j1939_dtc_t dtcs[8];
                uint8_t dtc_count = j1939_parse_dm1(tp_buffer, tp_len, &lamps, dtcs, 8);
                
                // Store DTC count
                data_manager_update(&g_data_manager, PARAM_ACTIVE_DTC_COUNT,
                                   (float)dtc_count, SOURCE_J1939, millis());
                
                // Store in NVS
                for (uint8_t i = 0; i < dtc_count; i++) {
                    nvs_dtc_store(&g_storage, dtcs[i].spn, dtcs[i].fmi,
                                  dtcs[i].source_address, millis() / 1000, true);
                }
            }
        }
        return;
    }
    
    // Decode parameters based on PGN
    float value;
    
    switch (msg.pgn) {
        case 61444:  // EEC1 - Engine Speed
            value = j1939_decode_engine_speed(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_ENGINE_SPEED,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 61443:  // EEC2 - Throttle
            value = j1939_decode_throttle_position(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_THROTTLE_POSITION,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65262:  // ET1 - Coolant Temperature
            value = j1939_decode_coolant_temp(msg.data);
            if (value > -9000) {
                data_manager_update(&g_data_manager, PARAM_COOLANT_TEMP,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65263:  // EFLP1 - Oil Pressure
            value = j1939_decode_oil_pressure(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_OIL_PRESSURE,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65265:  // CCVS - Vehicle Speed
            value = j1939_decode_vehicle_speed(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_VEHICLE_SPEED,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65266:  // LFE - Fuel Rate
            value = j1939_decode_fuel_rate(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_FUEL_RATE,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65269:  // AMB - Ambient Temperature
            value = j1939_decode_ambient_temp(msg.data);
            if (value > -9000) {
                data_manager_update(&g_data_manager, PARAM_AMBIENT_TEMP,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65270:  // IC1 - Boost Pressure
            value = j1939_decode_boost_pressure(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_BOOST_PRESSURE,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65271:  // VEP1 - Battery Voltage
            value = j1939_decode_battery_voltage(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_BATTERY_VOLTAGE,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65272:  // TRF1 - Trans Oil Temp
            value = j1939_decode_trans_oil_temp(msg.data);
            if (value > -9000) {
                data_manager_update(&g_data_manager, PARAM_TRANS_OIL_TEMP,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65276:  // DD - Fuel Level
            value = j1939_decode_fuel_level(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_FUEL_LEVEL_1,
                                   value, SOURCE_J1939, millis());
            }
            break;
            
        case 65253:  // HOURS - Engine Hours
            value = j1939_decode_engine_hours(msg.data);
            if (value >= 0) {
                data_manager_update(&g_data_manager, PARAM_ENGINE_HOURS,
                                   value, SOURCE_J1939, millis());
                nvs_lifetime_set_engine_hours(&g_storage, value);
            }
            break;
            
        case 61445:  // ETC2 - Current Gear
            {
                int8_t gear = j1939_decode_current_gear(msg.data);
                if (gear > -126) {
                    data_manager_update(&g_data_manager, PARAM_CURRENT_GEAR,
                                       (float)gear, SOURCE_J1939, millis());
                }
            }
            break;
    }
    
    // Debug output
    #if DEBUG_PARSED_VALUES
    if (g_can_frames_received % 100 == 0) {
        Serial.printf("CAN: PGN %u from SA 0x%02X\n", msg.pgn, msg.source_address);
    }
    #endif
}

/**
 * @brief CAN bus receive task
 */
static void can_task(void* param) {
    twai_message_t message;
    
    while (true) {
        // Wait for CAN message with 10ms timeout
        if (twai_receive(&message, pdMS_TO_TICKS(10)) == ESP_OK) {
            process_j1939_frame(&message);
        }
        
        // Feed watchdog
        vTaskDelay(1);
    }
}
#endif // NATIVE_BUILD

/*===========================================================================*/
/*                        J1708 FUNCTIONS                                   */
/*===========================================================================*/

#ifndef NATIVE_BUILD
/**
 * @brief Initialize J1708 UART interface
 */
static bool init_j1708(void) {
    // Configure UART for J1708 (9600 8N1)
    Serial2.begin(J1708_BAUD_RATE, SERIAL_8N1, PIN_J1708_RX, PIN_J1708_TX);
    
    // Configure RS485 direction pin
    pinMode(PIN_RS485_DE, OUTPUT);
    digitalWrite(PIN_RS485_DE, LOW);  // Start in receive mode
    
    Serial.println("J1708 interface initialized at 9600 bps");
    return true;
}

/**
 * @brief J1708 receive task
 */
static void j1708_task(void* param) {
    while (true) {
        while (Serial2.available()) {
            uint8_t byte = Serial2.read();
            
            if (j1708_receive_byte(&g_j1708_ctx, byte, millis())) {
                // Complete message received
                j1708_message_t msg;
                if (j1708_get_message(&g_j1708_ctx, &msg)) {
                    g_j1708_messages_received++;
                    
                    // Process parameters from the message
                    for (uint8_t i = 0; i < msg.param_count; i++) {
                        j1587_parameter_t* param = &msg.params[i];
                        float value;
                        
                        switch (param->pid) {
                            case 84:  // Road Speed
                                value = j1708_decode_road_speed(param->data, param->data_length);
                                if (value >= 0) {
                                    data_manager_update(&g_data_manager, PARAM_VEHICLE_SPEED,
                                                       value, SOURCE_J1708, millis());
                                }
                                break;
                                
                            case 190:  // Engine Speed
                                value = j1708_decode_engine_rpm(param->data, param->data_length);
                                if (value >= 0) {
                                    data_manager_update(&g_data_manager, PARAM_ENGINE_SPEED,
                                                       value, SOURCE_J1708, millis());
                                }
                                break;
                                
                            case 110:  // Coolant Temperature
                                value = j1708_decode_coolant_temp(param->data, param->data_length);
                                if (value > -9000) {
                                    data_manager_update(&g_data_manager, PARAM_COOLANT_TEMP,
                                                       value, SOURCE_J1708, millis());
                                }
                                break;
                        }
                    }
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
#endif // NATIVE_BUILD

/*===========================================================================*/
/*                        COMPUTED PARAMETERS                               */
/*===========================================================================*/

/**
 * @brief Update computed parameters (MPG, unit conversions, etc.)
 */
static void update_computed_params(void) {
    float speed_kmh, fuel_rate_lph;
    
    // Calculate instantaneous MPG
    if (data_manager_get(&g_data_manager, PARAM_VEHICLE_SPEED, &speed_kmh) &&
        data_manager_get(&g_data_manager, PARAM_FUEL_RATE, &fuel_rate_lph)) {
        
        if (fuel_rate_lph > 0.1f && speed_kmh > 1.0f) {
            // km/L = speed (km/h) / fuel_rate (L/h)
            float km_per_liter = speed_kmh / fuel_rate_lph;
            // Convert to MPG
            float mpg = km_per_liter * 2.35215f;
            
            data_manager_update(&g_data_manager, PARAM_MPG_CURRENT,
                               mpg, SOURCE_COMPUTED, millis());
        }
    }
    
    // Convert speed to MPH
    if (data_manager_get(&g_data_manager, PARAM_VEHICLE_SPEED, &speed_kmh)) {
        data_manager_update(&g_data_manager, PARAM_MPH,
                           speed_kmh * 0.621371f, SOURCE_COMPUTED, millis());
    }
    
    // Convert coolant temp to Fahrenheit
    float coolant_c;
    if (data_manager_get(&g_data_manager, PARAM_COOLANT_TEMP, &coolant_c)) {
        data_manager_update(&g_data_manager, PARAM_COOLANT_TEMP_F,
                           (coolant_c * 9.0f / 5.0f) + 32.0f, SOURCE_COMPUTED, millis());
    }
}

/*===========================================================================*/
/*                        DISPLAY FUNCTIONS                                 */
/*===========================================================================*/

#ifndef NATIVE_BUILD
/**
 * @brief Display task - update screen
 */
static void display_task(void* param) {
    while (true) {
        // Update watch list
        watch_list_update(&g_watch_list, millis());
        
        // Update computed parameters
        update_computed_params();
        
        // TODO: Actual display rendering (Phase 4)
        // For now, print to serial
        #if DEBUG_PARSED_VALUES
        float value;
        if (data_manager_get(&g_data_manager, PARAM_ENGINE_SPEED, &value)) {
            Serial.printf("RPM: %.0f  ", value);
        }
        if (data_manager_get(&g_data_manager, PARAM_VEHICLE_SPEED, &value)) {
            Serial.printf("Speed: %.1f km/h  ", value);
        }
        if (data_manager_get(&g_data_manager, PARAM_COOLANT_TEMP, &value)) {
            Serial.printf("Coolant: %.1f°C  ", value);
        }
        Serial.println();
        #endif
        
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_INTERVAL_MS));
    }
}
#endif // NATIVE_BUILD

/*===========================================================================*/
/*                        STORAGE TASK                                      */
/*===========================================================================*/

#ifndef NATIVE_BUILD
/**
 * @brief Storage task - periodic saves
 */
static void storage_task(void* param) {
    uint32_t last_update = millis();
    float prev_distance = 0;
    float prev_fuel = 0;
    
    while (true) {
        uint32_t now = millis();
        float distance, fuel_rate;
        
        // Calculate distance and fuel deltas
        if (data_manager_get(&g_data_manager, PARAM_VEHICLE_SPEED, &distance)) {
            // Approximate distance from speed (very rough)
            float dt_hours = (now - last_update) / 3600000.0f;
            float distance_delta = distance * dt_hours;
            
            if (data_manager_get(&g_data_manager, PARAM_FUEL_RATE, &fuel_rate)) {
                float fuel_delta = fuel_rate * dt_hours;
                
                nvs_storage_periodic_update(&g_storage, now, distance_delta, fuel_delta);
            }
        }
        
        last_update = now;
        vTaskDelay(pdMS_TO_TICKS(10000));  // 10 second update interval
    }
}
#endif // NATIVE_BUILD

/*===========================================================================*/
/*                        SERIAL OUTPUT                                     */
/*===========================================================================*/

/**
 * @brief Print periodic statistics
 */
static void print_stats(void) {
    uint32_t now = millis();
    
    if (now - g_last_stats_time >= 10000) {  // Every 10 seconds
        Serial.println("\n========== Dashboard Statistics ==========");
        Serial.printf("CAN frames received: %lu\n", g_can_frames_received);
        Serial.printf("J1708 messages received: %lu\n", g_j1708_messages_received);
        
        uint32_t valid_params, total_updates;
        data_manager_get_stats(&g_data_manager, &valid_params, &total_updates);
        Serial.printf("Valid parameters: %lu\n", valid_params);
        Serial.printf("Total updates: %lu\n", total_updates);
        
        Serial.printf("Active DTCs: %u\n", nvs_dtc_get_active_count(&g_storage));
        Serial.printf("Boot count: %lu\n", nvs_system_get_boot_count(&g_storage));
        
        if (!nvs_system_was_clean_shutdown(&g_storage)) {
            Serial.println("WARNING: Last shutdown was not clean!");
        }
        
        Serial.println("==========================================\n");
        
        g_last_stats_time = now;
    }
}

/*===========================================================================*/
/*                        SETUP & LOOP                                      */
/*===========================================================================*/

void setup() {
    // Initialize serial for debugging
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);  // Wait for serial
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("  J1939/J1708 Truck Dashboard v" FIRMWARE_VERSION_STRING);
    Serial.println("========================================");
    Serial.println();
    
    // Initialize parsers
    Serial.println("Initializing parsers...");
    j1939_parser_init(&g_j1939_ctx);
    j1708_parser_init(&g_j1708_ctx);
    
    // Initialize data manager
    Serial.println("Initializing data manager...");
    data_manager_init(&g_data_manager);
    
    // Initialize watch list with defaults
    Serial.println("Initializing watch list...");
    watch_list_init(&g_watch_list, &g_data_manager);
    watch_list_setup_defaults(&g_watch_list);
    
    // Initialize NVS storage
    Serial.println("Initializing persistent storage...");
    if (nvs_storage_init(&g_storage)) {
        Serial.println("  Storage loaded successfully");
        
        const lifetime_stats_t* lifetime = nvs_lifetime_get(&g_storage);
        Serial.printf("  Total distance: %.1f km\n", lifetime->total_distance_km);
        Serial.printf("  Engine hours: %.1f\n", lifetime->engine_hours);
    } else {
        Serial.println("  Warning: Storage initialization failed");
    }
    
#ifndef NATIVE_BUILD
    // Initialize CAN bus
    Serial.println("Initializing CAN bus...");
#ifdef SIMULATION_MODE
    Serial.println("  (Skipped - SIMULATION_MODE active)");
#else
    if (!init_can_bus()) {
        Serial.println("  ERROR: CAN initialization failed!");
    }
#endif
    
    // Initialize J1708
    Serial.println("Initializing J1708...");
#ifdef SIMULATION_MODE
    Serial.println("  (Skipped - SIMULATION_MODE active)");
#else
    if (!init_j1708()) {
        Serial.println("  ERROR: J1708 initialization failed!");
    }
#endif
    
#ifdef SIMULATION_MODE
    // Start simulation instead of hardware tasks
    Serial.println("Starting simulation...");
    init_simulation(SIM_SCENARIO_HIGHWAY);
#else
    // Create tasks
    Serial.println("Starting tasks...");
    
    xTaskCreatePinnedToCore(
        can_task, "CAN_Task", TASK_STACK_CAN,
        NULL, TASK_PRIORITY_CAN, &g_can_task_handle, TASK_CORE_CAN
    );
    
    xTaskCreatePinnedToCore(
        j1708_task, "J1708_Task", TASK_STACK_J1708,
        NULL, TASK_PRIORITY_J1708, &g_j1708_task_handle, TASK_CORE_J1708
    );
    
    xTaskCreatePinnedToCore(
        display_task, "Display_Task", TASK_STACK_DISPLAY,
        NULL, TASK_PRIORITY_DISPLAY, &g_display_task_handle, TASK_CORE_DISPLAY
    );
    
    xTaskCreatePinnedToCore(
        storage_task, "Storage_Task", TASK_STACK_STORAGE,
        NULL, TASK_PRIORITY_STORAGE, &g_storage_task_handle, TASK_CORE_DISPLAY
    );
#endif // SIMULATION_MODE
#endif // NATIVE_BUILD
    
    Serial.println();
    Serial.println("Initialization complete!");
#ifdef SIMULATION_MODE
    Serial.println("Running in SIMULATION MODE - no hardware required");
#else
    Serial.println("Waiting for vehicle data...");
#endif
    Serial.println();
    
    g_last_stats_time = millis();
}

void loop() {
    // Main loop
    
#ifdef SIMULATION_MODE
    // Update simulation - this generates CAN frames
    update_simulation();
    
    // Update display values
    update_computed_params();
    
    // Print simulation status periodically
    static uint32_t last_sim_print = 0;
    if (millis() - last_sim_print >= 2000) {
        print_sim_status();
        
        // Also print parsed data manager values for verification
        float rpm, speed, coolant, gear;
        Serial.println("--- Data Manager Values ---");
        if (data_manager_get(&g_data_manager, PARAM_ENGINE_SPEED, &rpm)) {
            Serial.printf("  RPM: %.0f  ", rpm);
        }
        if (data_manager_get(&g_data_manager, PARAM_VEHICLE_SPEED, &speed)) {
            Serial.printf("Speed: %.1f km/h  ", speed);
        }
        if (data_manager_get(&g_data_manager, PARAM_COOLANT_TEMP, &coolant)) {
            Serial.printf("Coolant: %.1f°C  ", coolant);
        }
        if (data_manager_get(&g_data_manager, PARAM_CURRENT_GEAR, &gear)) {
            Serial.printf("Gear: %.0f", gear);
        }
        Serial.println();
        Serial.println();
        
        last_sim_print = millis();
    }
#endif
    
    print_stats();
    
#ifndef NATIVE_BUILD
    delay(10);  // Faster loop in simulation mode
#else
    // For native testing, exit after one iteration
    exit(0);
#endif
}
