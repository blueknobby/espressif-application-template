//
// SPDX-License-Identifier: MIT
//

#pragma once

void app_svc_profile_event_handler(esp_gatts_cb_event_t event,
                                   esp_gatt_if_t gatts_if,
                                   esp_ble_gatts_cb_param_t *param);

void app_svc_send_data(const uint8_t* data, const size_t length);

extern const esp_gatts_attr_db_t app_svc_db[];
