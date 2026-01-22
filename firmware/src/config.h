/**
 * @file config.h
 * @brief Configuration settings for J1939/J1708 Dashboard
 * 
 * Central configuration file containing all pin assignments, 
 * protocol settings, and system parameters.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/*===========================================================================*/
/*                        VERSION INFORMATION                               */
/*===========================================================================*/

#define FIRMWARE_VERSION_MAJOR  0
#define FIRMWARE_VERSION_MINOR  1
#define FIRMWARE_VERSION_PATCH  0
#define FIRMWARE_VERSION_STRING "0.1.0"

/*===========================================================================*/
/*                        PIN ASSIGNMENTS                                   */
/*===========================================================================*/

// CAN Bus (J1939) - SN65HVD230 transceiver
#define PIN_CAN_TX          GPIO_NUM_5
#define PIN_CAN_RX          GPIO_NUM_4

// J1708 Serial - RS485 transceiver
#define PIN_J1708_TX        GPIO_NUM_17     // UART2 TX -> RS485 DI
#define PIN_J1708_RX        GPIO_NUM_16     // UART2 RX -> RS485 RO
#define PIN_RS485_DE        GPIO_NUM_25     // RS485 Direction Enable (DE/RE tied together)

// ADC Inputs (ADC1 - WiFi safe)
#define PIN_FUEL_TANK_1     GPIO_NUM_36     // ADC1_CH0 - Primary fuel tank
#define PIN_FUEL_TANK_2     GPIO_NUM_39     // ADC1_CH3 - Secondary fuel tank
#define PIN_DIMMER          GPIO_NUM_35     // ADC1_CH7 - Dashboard dimmer input
#define PIN_BATTERY_VOLTAGE GPIO_NUM_32     // ADC1_CH4 - Battery voltage monitor
#define PIN_SPARE_ADC       GPIO_NUM_34     // ADC1_CH6 - Spare analog input

// SPI Display (Phase 4)
#define PIN_SPI_CLK         GPIO_NUM_18     // VSPI SCK
#define PIN_SPI_MOSI        GPIO_NUM_23     // VSPI MOSI
#define PIN_SPI_MISO        GPIO_NUM_19     // VSPI MISO
#define PIN_TFT_CS          GPIO_NUM_15     // Display chip select
#define PIN_TFT_DC          GPIO_NUM_22     // Display data/command
#define PIN_TFT_RST         GPIO_NUM_21     // Display reset

// 1-Wire Temperature Sensors (Phase 4)
#define PIN_ONEWIRE         GPIO_NUM_27     // DS18B20 data line

// Thermocouple (Phase 4)
#define PIN_TC_CS           GPIO_NUM_14     // MAX31855 chip select

// Status/Control
#define PIN_STATUS_LED      GPIO_NUM_2      // Built-in LED (boot strapping - use carefully)
#define PIN_RELAY_CONTROL   GPIO_NUM_26     // Future remote start relay

/*===========================================================================*/
/*                        J1939 CAN BUS CONFIGURATION                       */
/*===========================================================================*/

#define J1939_BAUD_RATE             250000      // Standard J1939 baud rate (250 kbps)
#define J1939_TX_QUEUE_SIZE         10          // Transmit queue depth
#define J1939_RX_QUEUE_SIZE         50          // Receive queue depth

// Our device address (use diagnostic tool range to avoid conflicts)
#define J1939_OUR_ADDRESS           0xF9        // Off-board Diagnostic Tool #1

// Source addresses we care about
#define J1939_ADDR_ENGINE           0x00
#define J1939_ADDR_TRANSMISSION     0x03
#define J1939_ADDR_BRAKES           0x0B
#define J1939_ADDR_INSTRUMENT       0x17

/*===========================================================================*/
/*                        J1708 SERIAL CONFIGURATION                        */
/*===========================================================================*/

#define J1708_BAUD_RATE             9600        // J1708 standard baud rate
#define J1708_UART_NUM              UART_NUM_2  // ESP32 UART to use
#define J1708_RX_BUFFER_SIZE        256         // UART receive buffer
#define J1708_TX_BUFFER_SIZE        128         // UART transmit buffer
#define J1708_MAX_MESSAGE_LENGTH    21          // Maximum J1708 message bytes
#define J1708_INTER_BYTE_TIMEOUT_MS 2           // Max gap between message bytes

/*===========================================================================*/
/*                        DATA MANAGER CONFIGURATION                        */
/*===========================================================================*/

#define DATA_MAX_PARAMETERS         128         // Maximum tracked parameters
#define DATA_FRESHNESS_TIMEOUT_MS   5000        // Mark data stale after 5 seconds
#define DATA_UPDATE_CALLBACK_MAX    16          // Maximum parameter change callbacks

/*===========================================================================*/
/*                        STORAGE (NVS) CONFIGURATION                       */
/*===========================================================================*/

// NVS Namespace names
#define NVS_NAMESPACE_TRIP_A        "trip_a"
#define NVS_NAMESPACE_TRIP_B        "trip_b"
#define NVS_NAMESPACE_LIFETIME      "lifetime"
#define NVS_NAMESPACE_FUEL_ECON     "fuel_econ"
#define NVS_NAMESPACE_FAULT_LOG     "fault_log"
#define NVS_NAMESPACE_SETTINGS      "settings"
#define NVS_NAMESPACE_SYSTEM        "system"

// Storage intervals
#define STORAGE_PERIODIC_SAVE_MS    (5 * 60 * 1000)     // Save every 5 minutes
#define STORAGE_DISTANCE_THRESHOLD  1.0                  // Save if >1km traveled

// Power loss detection
#define POWER_LOSS_THRESHOLD_MV     11500       // 11.5V triggers emergency save
#define POWER_LOSS_DEBOUNCE_MS      100         // Debounce power dips

/*===========================================================================*/
/*                        WATCHDOG CONFIGURATION                            */
/*===========================================================================*/

#define WATCHDOG_TIMEOUT_SEC        10          // Hardware watchdog timeout
#define TASK_HEARTBEAT_TIMEOUT_MS   5000        // Soft watchdog per task

/*===========================================================================*/
/*                        DISPLAY CONFIGURATION (Phase 4)                   */
/*===========================================================================*/

#define DISPLAY_WIDTH               320
#define DISPLAY_HEIGHT              240
#define DISPLAY_ROTATION            3           // Landscape
#define DISPLAY_UPDATE_INTERVAL_MS  100         // 10 Hz refresh target

/*===========================================================================*/
/*                        FREERTOS TASK CONFIGURATION                       */
/*===========================================================================*/

// Task stack sizes (words, not bytes - multiply by 4 for bytes)
#define TASK_STACK_CAN              4096
#define TASK_STACK_J1708            4096
#define TASK_STACK_SENSOR           2048
#define TASK_STACK_DISPLAY          4096
#define TASK_STACK_STORAGE          2048

// Task priorities (higher number = higher priority)
#define TASK_PRIORITY_CAN           5           // Highest - time critical
#define TASK_PRIORITY_J1708         4
#define TASK_PRIORITY_DISPLAY       3
#define TASK_PRIORITY_SENSOR        2
#define TASK_PRIORITY_STORAGE       1           // Lowest - background

// Task core assignments (ESP32 has cores 0 and 1)
#define TASK_CORE_CAN               0           // Protocol tasks on core 0
#define TASK_CORE_J1708             0
#define TASK_CORE_SENSOR            1           // Processing tasks on core 1
#define TASK_CORE_DISPLAY           1

/*===========================================================================*/
/*                        DEBUG CONFIGURATION                               */
/*===========================================================================*/

// Enable/disable debug output
#define DEBUG_CAN_FRAMES            0           // Print raw CAN frames
#define DEBUG_J1708_MESSAGES        0           // Print raw J1708 messages
#define DEBUG_PARSED_VALUES         1           // Print decoded values
#define DEBUG_STORAGE_OPS           0           // Print NVS operations

// Serial output
#define SERIAL_BAUD_RATE            115200

/*===========================================================================*/
/*                        UNIT CONVERSION HELPERS                           */
/*===========================================================================*/

// Temperature conversions
#define CELSIUS_TO_FAHRENHEIT(c)    (((c) * 9.0f / 5.0f) + 32.0f)
#define FAHRENHEIT_TO_CELSIUS(f)    (((f) - 32.0f) * 5.0f / 9.0f)

// Speed conversions
#define KMH_TO_MPH(k)               ((k) * 0.621371f)
#define MPH_TO_KMH(m)               ((m) * 1.60934f)

// Pressure conversions
#define KPA_TO_PSI(k)               ((k) * 0.145038f)
#define PSI_TO_KPA(p)               ((p) * 6.89476f)
#define KPA_TO_BAR(k)               ((k) * 0.01f)

// Volume conversions
#define LITERS_TO_GALLONS(l)        ((l) * 0.264172f)
#define GALLONS_TO_LITERS(g)        ((g) * 3.78541f)

// Fuel economy conversions
#define KM_PER_L_TO_MPG(kpl)        ((kpl) * 2.35215f)
#define L_PER_100KM_TO_MPG(l100)    (235.215f / (l100))
#define MPG_TO_L_PER_100KM(mpg)     (235.215f / (mpg))

#endif /* CONFIG_H */
