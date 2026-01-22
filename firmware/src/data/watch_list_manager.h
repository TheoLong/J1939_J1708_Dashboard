/**
 * @file watch_list_manager.h
 * @brief Watch List Manager for display parameter selection
 * 
 * Manages which decoded parameters are displayed on the dashboard,
 * with support for user customization, thresholds, and display formatting.
 */

#ifndef WATCH_LIST_MANAGER_H
#define WATCH_LIST_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "data_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/*                        CONFIGURATION                                     */
/*===========================================================================*/

#define WATCH_LIST_MAX_ITEMS    16      // Maximum watched parameters
#define WATCH_LIST_MAX_PAGES    4       // Maximum display pages

/*===========================================================================*/
/*                        DATA STRUCTURES                                   */
/*===========================================================================*/

/**
 * @brief Display widget types
 */
typedef enum {
    WIDGET_GAUGE_CIRCULAR,      // Round gauge (RPM, speed)
    WIDGET_GAUGE_LINEAR,        // Bar gauge (temp, fuel)
    WIDGET_GAUGE_SEMICIRCLE,    // Half-circle gauge
    WIDGET_NUMERIC,             // Plain number
    WIDGET_INDICATOR,           // On/off lamp
    WIDGET_TEXT,                // Text status
    WIDGET_GRAPH                // Trend line
} widget_type_t;

/**
 * @brief Alert levels
 */
typedef enum {
    ALERT_NONE = 0,
    ALERT_INFO,                 // Blue - informational
    ALERT_WARNING,              // Amber - warning
    ALERT_CRITICAL              // Red - critical
} alert_level_t;

/**
 * @brief Watch list entry for a single parameter
 */
typedef struct {
    param_id_t param_id;        // Which parameter to watch
    
    // Display settings
    widget_type_t widget_type;  // How to display
    uint8_t page;               // Which page (0-3)
    uint8_t position;           // Position on page (0-7 typical)
    uint8_t decimal_places;     // Decimal places to show
    
    // Thresholds
    float warn_low;             // Low warning threshold
    float warn_high;            // High warning threshold
    float crit_low;             // Critical low threshold
    float crit_high;            // Critical high threshold
    
    // Override labels
    bool use_custom_label;
    char custom_label[12];      // Custom short label
    char custom_unit[8];        // Custom unit string
    
    // Range for gauges
    float gauge_min;            // Gauge minimum
    float gauge_max;            // Gauge maximum
    
    // State
    bool enabled;
    alert_level_t current_alert;
} watch_item_t;

/**
 * @brief Watch list manager context
 */
typedef struct {
    watch_item_t items[WATCH_LIST_MAX_ITEMS];
    uint8_t item_count;
    uint8_t current_page;
    data_manager_t* data_manager;   // Reference to data source
    bool initialized;
} watch_list_manager_t;

/**
 * @brief Display update callback
 */
typedef void (*watch_update_callback_t)(const watch_item_t* item, float value, alert_level_t alert);

/*===========================================================================*/
/*                        FUNCTION DECLARATIONS                             */
/*===========================================================================*/

/**
 * @brief Initialize watch list manager
 * @param wlm Watch list manager context
 * @param dm Data manager to read values from
 */
void watch_list_init(watch_list_manager_t* wlm, data_manager_t* dm);

/**
 * @brief Add a parameter to the watch list
 * @param wlm Watch list manager
 * @param param_id Parameter to watch
 * @param widget_type Display widget type
 * @param page Display page (0-3)
 * @param position Position on page
 * @return Index of added item, or -1 if full
 */
int watch_list_add(watch_list_manager_t* wlm, param_id_t param_id,
                   widget_type_t widget_type, uint8_t page, uint8_t position);

/**
 * @brief Remove a parameter from the watch list
 * @param wlm Watch list manager
 * @param param_id Parameter to remove
 * @return true if found and removed
 */
bool watch_list_remove(watch_list_manager_t* wlm, param_id_t param_id);

/**
 * @brief Set thresholds for a watched parameter
 * @param wlm Watch list manager
 * @param param_id Parameter to configure
 * @param warn_low Low warning threshold (use -FLT_MAX to disable)
 * @param warn_high High warning threshold (use FLT_MAX to disable)
 * @param crit_low Low critical threshold
 * @param crit_high High critical threshold
 * @return true if found and configured
 */
bool watch_list_set_thresholds(watch_list_manager_t* wlm, param_id_t param_id,
                                float warn_low, float warn_high,
                                float crit_low, float crit_high);

/**
 * @brief Set gauge range for a watched parameter
 * @param wlm Watch list manager
 * @param param_id Parameter to configure
 * @param min Gauge minimum value
 * @param max Gauge maximum value
 * @return true if found and configured
 */
bool watch_list_set_gauge_range(watch_list_manager_t* wlm, param_id_t param_id,
                                 float min, float max);

/**
 * @brief Set custom label for a watched parameter
 * @param wlm Watch list manager
 * @param param_id Parameter to configure
 * @param label Custom short label (max 11 chars)
 * @param unit Custom unit string (max 7 chars)
 * @return true if found and configured
 */
bool watch_list_set_custom_label(watch_list_manager_t* wlm, param_id_t param_id,
                                  const char* label, const char* unit);

/**
 * @brief Get watch list item by parameter ID
 * @param wlm Watch list manager
 * @param param_id Parameter to find
 * @return Pointer to watch item, or NULL if not found
 */
watch_item_t* watch_list_get_item(watch_list_manager_t* wlm, param_id_t param_id);

/**
 * @brief Get watch list item by index
 * @param wlm Watch list manager
 * @param index Item index
 * @return Pointer to watch item, or NULL if invalid
 */
watch_item_t* watch_list_get_by_index(watch_list_manager_t* wlm, uint8_t index);

/**
 * @brief Get items for a specific page
 * @param wlm Watch list manager
 * @param page Page number (0-3)
 * @param items Output array for items
 * @param max_items Maximum items to return
 * @return Number of items on that page
 */
uint8_t watch_list_get_page_items(watch_list_manager_t* wlm, uint8_t page,
                                   watch_item_t** items, uint8_t max_items);

/**
 * @brief Update all watch items (check values and alerts)
 * @param wlm Watch list manager
 * @param current_time_ms Current timestamp
 */
void watch_list_update(watch_list_manager_t* wlm, uint32_t current_time_ms);

/**
 * @brief Get current value and alert level for an item
 * @param wlm Watch list manager
 * @param item Watch item
 * @param value Output value
 * @param alert Output alert level
 * @return true if value is valid
 */
bool watch_list_get_value(watch_list_manager_t* wlm, watch_item_t* item,
                          float* value, alert_level_t* alert);

/**
 * @brief Check if any item has an active alert
 * @param wlm Watch list manager
 * @return Highest active alert level
 */
alert_level_t watch_list_get_highest_alert(watch_list_manager_t* wlm);

/**
 * @brief Get count of items with alerts
 * @param wlm Watch list manager
 * @param level Minimum alert level to count
 * @return Count of items at or above specified level
 */
uint8_t watch_list_get_alert_count(watch_list_manager_t* wlm, alert_level_t level);

/**
 * @brief Set current display page
 * @param wlm Watch list manager
 * @param page Page number (0-3)
 */
void watch_list_set_page(watch_list_manager_t* wlm, uint8_t page);

/**
 * @brief Get current display page
 * @param wlm Watch list manager
 * @return Current page number
 */
uint8_t watch_list_get_page(watch_list_manager_t* wlm);

/**
 * @brief Advance to next page (wraps around)
 * @param wlm Watch list manager
 * @return New page number
 */
uint8_t watch_list_next_page(watch_list_manager_t* wlm);

/**
 * @brief Configure default watch list for truck dashboard
 * @param wlm Watch list manager
 * 
 * Sets up a sensible default watch list with common truck parameters:
 * - Page 0: Engine (RPM, coolant, oil pressure, boost)
 * - Page 1: Fuel/Economy (speed, fuel level, MPG)
 * - Page 2: Transmission (trans temp, gear)
 * - Page 3: Diagnostics (DTCs, voltage)
 */
void watch_list_setup_defaults(watch_list_manager_t* wlm);

/**
 * @brief Clear all items from watch list
 * @param wlm Watch list manager
 */
void watch_list_clear(watch_list_manager_t* wlm);

/**
 * @brief Get display label for a watch item
 * @param wlm Watch list manager
 * @param item Watch item
 * @return Label string
 */
const char* watch_list_get_label(watch_list_manager_t* wlm, watch_item_t* item);

/**
 * @brief Get unit string for a watch item
 * @param wlm Watch list manager
 * @param item Watch item
 * @return Unit string
 */
const char* watch_list_get_unit(watch_list_manager_t* wlm, watch_item_t* item);

#ifdef __cplusplus
}
#endif

#endif /* WATCH_LIST_MANAGER_H */
