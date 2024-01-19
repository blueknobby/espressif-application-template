//
// SPDX-License-Identifier: GPL-2.0-only
//

#include "Bluetooth.h"

#include <cstring>

#include "esp_bt.h"
#include "esp_log.h"

#include "esp_console.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "bt_general.h"

#include "bt_svc_app.h"
#include "bt_svc_dis.h"
#include "bt_svc_batt.h"


static const char *TAG = "ble";


static void
gatts_event_handler(esp_gatts_cb_event_t event,
                    esp_gatt_if_t gatts_if,
                    esp_ble_gatts_cb_param_t *param);


static void
gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);


////////////////////////////////////////////////////////////////////////
//
// command line interface
//

int
cmd_bluetooth(int argc, char** argv)
{
    if (argc>2) {
        if (strcmp(argv[1],"send") == 0) {
            const uint8_t* data= (const uint8_t*) argv[2];
            const size_t len = strlen(argv[2]);

            ble.sendData(data, len);
        }
    }

    return ESP_OK;
}


void
register_bluetooth_cmd(void)
{
    const esp_console_cmd_t cmd = {
        .command = "bt",
        .help = "send raw bluetooth data",
        .hint = NULL,
        .func = &cmd_bluetooth,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}



////////////////////////////////////////////////////////////////////////////////
//
// BLE class implementation
//

BLE::BLE()
{
}


void
BLE::begin()
{
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "init controller failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "enable controller failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "init bluedroid failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "enable bluedroid failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(TAG, "gap register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(APP_APP_ID);
    if (ret){
        ESP_LOGE(TAG, "gatts app app register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(DIS_APP_ID);
    if (ret){
        ESP_LOGE(TAG, "gatts dis app register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(BATT_APP_ID);
    if (ret){
        ESP_LOGE(TAG, "gatts batt app register error, error code = %x", ret);
        return;
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(251);
    if (local_mtu_ret){
        ESP_LOGE(TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }

    register_bluetooth_cmd();

    ESP_LOGI(TAG, "initialized");
}


void
BLE::setDeviceName(const std::string deviceName)
{
    esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(deviceName.c_str());
    if (set_dev_name_ret){
        ESP_LOGE(TAG, "set device name failed, error code = %x", set_dev_name_ret);
    }
}


void
BLE::setBatteryLevel(const uint8_t percentage)
{
    batt_set_battery_level(percentage);
}


void
BLE::sendData(const uint8_t* data, const size_t length)
{
    app_svc_send_data(data, length);
}

BLE ble;

////////////////////////////////////////////////////////////////////////////////
//
//
//


static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .peer_addr           = {0},
    .peer_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};


struct gatts_profile_inst service_profile_tab[SERVICE_COUNT] = {
    [APP_SVC_IDX] = {
        .gatts_cb = app_svc_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
        .name = "app",
        .app_id = 0,
        .conn_id = 0,
    },
    [DIS_SVC_IDX] = {
        .gatts_cb = dis_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
        .name = "dis",
        .app_id = 0,
        .conn_id = 0,
    },
    [BATT_SVC_IDX] = {
        .gatts_cb = batt_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
        .name = "batt",
        .app_id = 0,
        .conn_id = 0,
    },
};



static void
register_gatts_intf(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(TAG, "register app ID %d", param->reg.app_id);
    if (param->reg.status == ESP_GATT_OK) {
        // customize this PER SERVICE (add each service ID)

        uint8_t service_idx = 0;
        switch(param->reg.app_id) {
            case APP_APP_ID:
                service_idx = APP_SVC_IDX;
                break;
            case DIS_APP_ID:
                service_idx = DIS_SVC_IDX;
                break;
            case BATT_APP_ID:
                service_idx = BATT_SVC_IDX;
                break;
            default:
                ESP_LOGE(TAG, "Unknown APP ID %d", param->reg.app_id);
                return;
        }

        service_profile_tab[service_idx].gatts_if = gatts_if;
        service_profile_tab[service_idx].app_id = param->reg.app_id;

        ESP_LOGI(TAG, "set gatts_if for %s", service_profile_tab[service_idx].name.c_str());
    }
    else {
        ESP_LOGE(TAG, "reg app failed, app_id %04x, status %d",
                 param->reg.app_id,
                 param->reg.status);
        return;
    }
}



static
void
gatts_event_handler(esp_gatts_cb_event_t event,
                    esp_gatt_if_t gatts_if,
                    esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGD(TAG, "%s", __FUNCTION__);

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        register_gatts_intf(gatts_if, param);
    }
    do {
        int idx;
        for (idx = 0; idx < SERVICE_COUNT; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == service_profile_tab[idx].gatts_if) {
                if (service_profile_tab[idx].gatts_cb) {
                    service_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}


static
void
gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_complete &= (~ADV_CONFIG_FLAG);
            if (adv_config_complete == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_complete &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_complete == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* advertising start complete event to indicate advertising start successfully or failed */
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "advertising start failed");
            }
            else{
                ESP_LOGI(TAG, "advertising start successfully");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising stop failed");
            }
            else {
                ESP_LOGI(TAG, "Stop adv successfully");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                     param->update_conn_params.status,
                     param->update_conn_params.min_int,
                     param->update_conn_params.max_int,
                     param->update_conn_params.conn_int,
                     param->update_conn_params.latency,
                     param->update_conn_params.timeout);
            break;
        default:
            break;
    }
}


static
const char *
str_for_notification_type(notification_type value)
{
    switch (value) {
        case NONE:
            return "NONE";
        case INDICATE:
            return "INDICATE";
        case NOTIFY:
            return "NOTIFY";
    }
    return "UNKNOWN";
}



void
update_notify_value(esp_ble_gatts_cb_param_t *param, notification_type *value, const char *name_str)
{
    if (value != NULL && param->write.len == 2){
        uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];

        switch (descr_value) {
            case 0x0000:
                *value = NONE;
                break;
            case 0x0001:
                *value = NOTIFY;
                break;
            case 0x0002:
                *value = INDICATE;
                break;
            default:
                ESP_LOGE(TAG, "unknown descr value");
                esp_log_buffer_hex(TAG, param->write.value, param->write.len);
                return;
        }

        ESP_LOGI(TAG, "set notify value for %s to %s", name_str,
                 str_for_notification_type(*value));
    }
}




const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
const uint8_t char_prop_read                = ESP_GATT_CHAR_PROP_BIT_READ;
const uint8_t char_prop_write               = ESP_GATT_CHAR_PROP_BIT_WRITE;
const uint8_t char_prop_notify              = ESP_GATT_CHAR_PROP_BIT_NOTIFY;
const uint8_t char_prop_read_notify         = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
const uint8_t char_prop_write_notify        = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
const uint8_t char_prop_read_write_notify   = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
const uint8_t char_prop_read_write          = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;

bool adv_config_complete;
