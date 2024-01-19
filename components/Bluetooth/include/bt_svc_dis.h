//
// SPDX-License-Identifier: MIT
//

#pragma once

void dis_profile_event_handler(esp_gatts_cb_event_t event,
                               esp_gatt_if_t gatts_if,
                               esp_ble_gatts_cb_param_t *param);


extern esp_gatts_attr_db_t dis_db[];
