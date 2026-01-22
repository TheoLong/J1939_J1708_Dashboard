/**
 * @file can_driver.h
 * @brief ESP32 TWAI/CAN driver wrapper for J1939 communication
 * 
 * Provides a simplified interface to the ESP32's TWAI (CAN) controller
 * for J1939 applications.
 */

#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/*                        CONFIGURATION                                     */
/*===========================================================================*/

#define CAN_BAUD_250K           250000      // J1939 standard
#define CAN_BAUD_500K           500000      // Alternative

/*===========================================================================*/
/*                        DATA STRUCTURES                                   */
/*===========================================================================*/

/**
 * @brief CAN frame structure
 */
typedef struct {
    uint32_t id;            // 29-bit extended ID for J1939
    uint8_t data[8];        // Frame data
    uint8_t length;         // Data length (0-8)
    bool is_extended;       // True for 29-bit ID
    bool is_rtr;            // Remote transmission request
} can_frame_t;

/**
 * @brief CAN statistics
 */
typedef struct {
    uint32_t rx_count;
    uint32_t tx_count;
    uint32_t rx_errors;
    uint32_t tx_errors;
    uint32_t bus_errors;
    uint8_t tx_error_counter;
    uint8_t rx_error_counter;
} can_stats_t;

/**
 * @brief CAN driver state
 */
typedef enum {
    CAN_STATE_STOPPED,
    CAN_STATE_RUNNING,
    CAN_STATE_BUS_OFF,
    CAN_STATE_RECOVERING
} can_state_t;

/*===========================================================================*/
/*                        FUNCTION DECLARATIONS                             */
/*===========================================================================*/

/**
 * @brief Initialize CAN driver
 * @param tx_pin GPIO for CAN TX
 * @param rx_pin GPIO for CAN RX
 * @param baud_rate Baud rate (use CAN_BAUD_250K for J1939)
 * @return true if initialization successful
 */
bool can_driver_init(uint8_t tx_pin, uint8_t rx_pin, uint32_t baud_rate);

/**
 * @brief Start CAN driver
 * @return true if started successfully
 */
bool can_driver_start(void);

/**
 * @brief Stop CAN driver
 * @return true if stopped successfully
 */
bool can_driver_stop(void);

/**
 * @brief Check if CAN driver is running
 * @return Current state
 */
can_state_t can_driver_get_state(void);

/**
 * @brief Receive a CAN frame
 * @param frame Output frame structure
 * @param timeout_ms Maximum wait time (0 for non-blocking)
 * @return true if frame received
 */
bool can_driver_receive(can_frame_t* frame, uint32_t timeout_ms);

/**
 * @brief Transmit a CAN frame
 * @param frame Frame to transmit
 * @param timeout_ms Maximum wait time for queue space
 * @return true if frame queued successfully
 */
bool can_driver_transmit(const can_frame_t* frame, uint32_t timeout_ms);

/**
 * @brief Get CAN statistics
 * @param stats Output statistics structure
 */
void can_driver_get_stats(can_stats_t* stats);

/**
 * @brief Clear CAN statistics
 */
void can_driver_clear_stats(void);

/**
 * @brief Initiate bus-off recovery
 * @return true if recovery initiated
 */
bool can_driver_recover(void);

/**
 * @brief Set acceptance filter (for J1939, accept all extended frames)
 * @param accept_code Filter acceptance code
 * @param accept_mask Filter acceptance mask
 * @return true if filter set successfully
 */
bool can_driver_set_filter(uint32_t accept_code, uint32_t accept_mask);

#ifdef __cplusplus
}
#endif

#endif /* CAN_DRIVER_H */
