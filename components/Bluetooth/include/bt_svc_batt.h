#pragma once


void batt_profile_event_handler(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param);


extern const esp_gatts_attr_db_t batt_db[];

extern void batt_set_battery_level(uint8_t percentage);