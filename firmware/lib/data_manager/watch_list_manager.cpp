/**
 * @file watch_list_manager.cpp
 * @brief Watch List Manager implementation
 */

#include "watch_list_manager.h"
#include <string.h>
#include <float.h>

/*===========================================================================*/
/*                        INITIALIZATION                                    */
/*===========================================================================*/

void watch_list_init(watch_list_manager_t* wlm, data_manager_t* dm) {
    if (wlm == NULL) return;
    
    memset(wlm, 0, sizeof(watch_list_manager_t));
    wlm->data_manager = dm;
    wlm->current_page = 0;
    wlm->initialized = true;
}

/*===========================================================================*/
/*                        ITEM MANAGEMENT                                   */
/*===========================================================================*/

int watch_list_add(watch_list_manager_t* wlm, param_id_t param_id,
                   widget_type_t widget_type, uint8_t page, uint8_t position) {
    if (wlm == NULL || !wlm->initialized) return -1;
    if (wlm->item_count >= WATCH_LIST_MAX_ITEMS) return -1;
    if (page >= WATCH_LIST_MAX_PAGES) return -1;
    
    // Check if already in list
    for (uint8_t i = 0; i < wlm->item_count; i++) {
        if (wlm->items[i].param_id == param_id) {
            return -1;  // Already exists
        }
    }
    
    watch_item_t* item = &wlm->items[wlm->item_count];
    memset(item, 0, sizeof(watch_item_t));
    
    item->param_id = param_id;
    item->widget_type = widget_type;
    item->page = page;
    item->position = position;
    item->enabled = true;
    item->decimal_places = 1;
    
    // Set default thresholds (disabled)
    item->warn_low = -FLT_MAX;
    item->warn_high = FLT_MAX;
    item->crit_low = -FLT_MAX;
    item->crit_high = FLT_MAX;
    
    // Set default gauge range
    item->gauge_min = 0;
    item->gauge_max = 100;
    
    return wlm->item_count++;
}

bool watch_list_remove(watch_list_manager_t* wlm, param_id_t param_id) {
    if (wlm == NULL || !wlm->initialized) return false;
    
    for (uint8_t i = 0; i < wlm->item_count; i++) {
        if (wlm->items[i].param_id == param_id) {
            // Shift remaining items
            for (uint8_t j = i; j < wlm->item_count - 1; j++) {
                wlm->items[j] = wlm->items[j + 1];
            }
            wlm->item_count--;
            return true;
        }
    }
    return false;
}

bool watch_list_set_thresholds(watch_list_manager_t* wlm, param_id_t param_id,
                                float warn_low, float warn_high,
                                float crit_low, float crit_high) {
    watch_item_t* item = watch_list_get_item(wlm, param_id);
    if (item == NULL) return false;
    
    item->warn_low = warn_low;
    item->warn_high = warn_high;
    item->crit_low = crit_low;
    item->crit_high = crit_high;
    
    return true;
}

bool watch_list_set_gauge_range(watch_list_manager_t* wlm, param_id_t param_id,
                                 float min, float max) {
    watch_item_t* item = watch_list_get_item(wlm, param_id);
    if (item == NULL) return false;
    
    item->gauge_min = min;
    item->gauge_max = max;
    
    return true;
}

bool watch_list_set_custom_label(watch_list_manager_t* wlm, param_id_t param_id,
                                  const char* label, const char* unit) {
    watch_item_t* item = watch_list_get_item(wlm, param_id);
    if (item == NULL) return false;
    
    item->use_custom_label = true;
    
    if (label != NULL) {
        strncpy(item->custom_label, label, sizeof(item->custom_label) - 1);
        item->custom_label[sizeof(item->custom_label) - 1] = '\0';
    }
    
    if (unit != NULL) {
        strncpy(item->custom_unit, unit, sizeof(item->custom_unit) - 1);
        item->custom_unit[sizeof(item->custom_unit) - 1] = '\0';
    }
    
    return true;
}

/*===========================================================================*/
/*                        ITEM ACCESS                                       */
/*===========================================================================*/

watch_item_t* watch_list_get_item(watch_list_manager_t* wlm, param_id_t param_id) {
    if (wlm == NULL || !wlm->initialized) return NULL;
    
    for (uint8_t i = 0; i < wlm->item_count; i++) {
        if (wlm->items[i].param_id == param_id) {
            return &wlm->items[i];
        }
    }
    return NULL;
}

watch_item_t* watch_list_get_by_index(watch_list_manager_t* wlm, uint8_t index) {
    if (wlm == NULL || !wlm->initialized) return NULL;
    if (index >= wlm->item_count) return NULL;
    
    return &wlm->items[index];
}

uint8_t watch_list_get_page_items(watch_list_manager_t* wlm, uint8_t page,
                                   watch_item_t** items, uint8_t max_items) {
    if (wlm == NULL || !wlm->initialized || items == NULL) return 0;
    
    uint8_t count = 0;
    for (uint8_t i = 0; i < wlm->item_count && count < max_items; i++) {
        if (wlm->items[i].page == page && wlm->items[i].enabled) {
            items[count++] = &wlm->items[i];
        }
    }
    return count;
}

/*===========================================================================*/
/*                        VALUE AND ALERT HANDLING                          */
/*===========================================================================*/

static alert_level_t check_alert_level(watch_item_t* item, float value) {
    // Check critical thresholds first
    if (value <= item->crit_low || value >= item->crit_high) {
        return ALERT_CRITICAL;
    }
    
    // Check warning thresholds
    if (value <= item->warn_low || value >= item->warn_high) {
        return ALERT_WARNING;
    }
    
    return ALERT_NONE;
}

void watch_list_update(watch_list_manager_t* wlm, uint32_t current_time_ms) {
    if (wlm == NULL || !wlm->initialized || wlm->data_manager == NULL) return;
    
    for (uint8_t i = 0; i < wlm->item_count; i++) {
        watch_item_t* item = &wlm->items[i];
        if (!item->enabled) continue;
        
        float value;
        if (data_manager_get(wlm->data_manager, item->param_id, &value)) {
            item->current_alert = check_alert_level(item, value);
        } else {
            item->current_alert = ALERT_NONE;  // No data
        }
    }
}

bool watch_list_get_value(watch_list_manager_t* wlm, watch_item_t* item,
                          float* value, alert_level_t* alert) {
    if (wlm == NULL || item == NULL || wlm->data_manager == NULL) return false;
    
    float val;
    if (!data_manager_get(wlm->data_manager, item->param_id, &val)) {
        return false;
    }
    
    if (value != NULL) *value = val;
    if (alert != NULL) *alert = item->current_alert;
    
    return true;
}

alert_level_t watch_list_get_highest_alert(watch_list_manager_t* wlm) {
    if (wlm == NULL || !wlm->initialized) return ALERT_NONE;
    
    alert_level_t highest = ALERT_NONE;
    for (uint8_t i = 0; i < wlm->item_count; i++) {
        if (wlm->items[i].enabled && wlm->items[i].current_alert > highest) {
            highest = wlm->items[i].current_alert;
        }
    }
    return highest;
}

uint8_t watch_list_get_alert_count(watch_list_manager_t* wlm, alert_level_t level) {
    if (wlm == NULL || !wlm->initialized) return 0;
    
    uint8_t count = 0;
    for (uint8_t i = 0; i < wlm->item_count; i++) {
        if (wlm->items[i].enabled && wlm->items[i].current_alert >= level) {
            count++;
        }
    }
    return count;
}

/*===========================================================================*/
/*                        PAGE NAVIGATION                                   */
/*===========================================================================*/

void watch_list_set_page(watch_list_manager_t* wlm, uint8_t page) {
    if (wlm == NULL) return;
    if (page >= WATCH_LIST_MAX_PAGES) page = 0;
    wlm->current_page = page;
}

uint8_t watch_list_get_page(watch_list_manager_t* wlm) {
    if (wlm == NULL) return 0;
    return wlm->current_page;
}

uint8_t watch_list_next_page(watch_list_manager_t* wlm) {
    if (wlm == NULL) return 0;
    wlm->current_page = (wlm->current_page + 1) % WATCH_LIST_MAX_PAGES;
    return wlm->current_page;
}

/*===========================================================================*/
/*                        DEFAULT SETUP                                     */
/*===========================================================================*/

void watch_list_setup_defaults(watch_list_manager_t* wlm) {
    if (wlm == NULL) return;
    
    watch_list_clear(wlm);
    
    // ===== Page 0: Engine =====
    int idx;
    
    // Engine RPM
    idx = watch_list_add(wlm, PARAM_ENGINE_SPEED, WIDGET_GAUGE_CIRCULAR, 0, 0);
    if (idx >= 0) {
        watch_list_set_thresholds(wlm, PARAM_ENGINE_SPEED, 400, 2200, 300, 2500);
        watch_list_set_gauge_range(wlm, PARAM_ENGINE_SPEED, 0, 3000);
    }
    
    // Coolant Temperature
    idx = watch_list_add(wlm, PARAM_COOLANT_TEMP, WIDGET_GAUGE_LINEAR, 0, 1);
    if (idx >= 0) {
        watch_list_set_thresholds(wlm, PARAM_COOLANT_TEMP, 70, 100, 50, 110);
        watch_list_set_gauge_range(wlm, PARAM_COOLANT_TEMP, 40, 120);
    }
    
    // Oil Pressure
    idx = watch_list_add(wlm, PARAM_OIL_PRESSURE, WIDGET_GAUGE_LINEAR, 0, 2);
    if (idx >= 0) {
        watch_list_set_thresholds(wlm, PARAM_OIL_PRESSURE, 150, FLT_MAX, 100, FLT_MAX);
        watch_list_set_gauge_range(wlm, PARAM_OIL_PRESSURE, 0, 700);
    }
    
    // Boost Pressure
    idx = watch_list_add(wlm, PARAM_BOOST_PRESSURE, WIDGET_GAUGE_SEMICIRCLE, 0, 3);
    if (idx >= 0) {
        watch_list_set_gauge_range(wlm, PARAM_BOOST_PRESSURE, 0, 300);
    }
    
    // ===== Page 1: Speed/Fuel =====
    
    // Vehicle Speed
    idx = watch_list_add(wlm, PARAM_VEHICLE_SPEED, WIDGET_GAUGE_CIRCULAR, 1, 0);
    if (idx >= 0) {
        watch_list_set_gauge_range(wlm, PARAM_VEHICLE_SPEED, 0, 140);
    }
    
    // Fuel Level
    idx = watch_list_add(wlm, PARAM_FUEL_LEVEL_1, WIDGET_GAUGE_LINEAR, 1, 1);
    if (idx >= 0) {
        watch_list_set_thresholds(wlm, PARAM_FUEL_LEVEL_1, 15, FLT_MAX, 10, FLT_MAX);
        watch_list_set_gauge_range(wlm, PARAM_FUEL_LEVEL_1, 0, 100);
    }
    
    // Fuel Rate
    idx = watch_list_add(wlm, PARAM_FUEL_RATE, WIDGET_NUMERIC, 1, 2);
    
    // Current MPG
    idx = watch_list_add(wlm, PARAM_MPG_CURRENT, WIDGET_NUMERIC, 1, 3);
    
    // ===== Page 2: Transmission =====
    
    // Trans Oil Temp
    idx = watch_list_add(wlm, PARAM_TRANS_OIL_TEMP, WIDGET_GAUGE_LINEAR, 2, 0);
    if (idx >= 0) {
        watch_list_set_thresholds(wlm, PARAM_TRANS_OIL_TEMP, -FLT_MAX, 100, -FLT_MAX, 120);
        watch_list_set_gauge_range(wlm, PARAM_TRANS_OIL_TEMP, 0, 150);
    }
    
    // Current Gear
    idx = watch_list_add(wlm, PARAM_CURRENT_GEAR, WIDGET_NUMERIC, 2, 1);
    if (idx >= 0) {
        wlm->items[idx].decimal_places = 0;
    }
    
    // Engine Hours
    idx = watch_list_add(wlm, PARAM_ENGINE_HOURS, WIDGET_NUMERIC, 2, 2);
    
    // ===== Page 3: Diagnostics =====
    
    // Battery Voltage
    idx = watch_list_add(wlm, PARAM_BATTERY_VOLTAGE, WIDGET_NUMERIC, 3, 0);
    if (idx >= 0) {
        watch_list_set_thresholds(wlm, PARAM_BATTERY_VOLTAGE, 12.0, 15.0, 11.5, 15.5);
    }
    
    // Active DTC Count
    idx = watch_list_add(wlm, PARAM_ACTIVE_DTC_COUNT, WIDGET_INDICATOR, 3, 1);
    if (idx >= 0) {
        watch_list_set_thresholds(wlm, PARAM_ACTIVE_DTC_COUNT, -FLT_MAX, 0.5, -FLT_MAX, 0.5);
        wlm->items[idx].decimal_places = 0;
    }
    
    // Ambient Temp
    idx = watch_list_add(wlm, PARAM_AMBIENT_TEMP, WIDGET_NUMERIC, 3, 2);
}

void watch_list_clear(watch_list_manager_t* wlm) {
    if (wlm == NULL) return;
    
    memset(wlm->items, 0, sizeof(wlm->items));
    wlm->item_count = 0;
    wlm->current_page = 0;
}

/*===========================================================================*/
/*                        LABEL HELPERS                                     */
/*===========================================================================*/

const char* watch_list_get_label(watch_list_manager_t* wlm, watch_item_t* item) {
    if (item == NULL) return "???";
    
    if (item->use_custom_label && item->custom_label[0] != '\0') {
        return item->custom_label;
    }
    
    return data_manager_get_param_name(item->param_id);
}

const char* watch_list_get_unit(watch_list_manager_t* wlm, watch_item_t* item) {
    if (item == NULL) return "";
    
    if (item->use_custom_label && item->custom_unit[0] != '\0') {
        return item->custom_unit;
    }
    
    return data_manager_get_param_unit(item->param_id);
}
