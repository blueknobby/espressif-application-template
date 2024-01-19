#include "Bluetooth.h"

#include <string>

#include <cstdint>
#include <cstring>

#include "esp_log.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

#include "bt_svc_app.h"
#include "bt_general.h"

#include "bkt_uuid.h"

static const char *TAG = "svc_app";

static uint16_t app_svc_handle_table[APP_IDX_NB];

static uint8_t app_svc_service_uuid[16]    = { APP_COMM_SERVICE_UUID };
static uint8_t app_svc_read_uuid[16]       = { APP_COMM_READ_CHARACTERISTIC_UUID };


static uint8_t app_svc_read_data = 0x22;
static const uint8_t app_svc_read_ccc[2] = {0x00, 0x00};

static notification_type notify_svc_app_spp = NONE;

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)
static uint8_t adv_config_done       = 0;


/* The length of adv data must be less than 31 bytes */

// iOS expects the service list in the advertising packet
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = false,
    .include_txpower     = true,
    .min_interval        = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval        = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance          = 0x00,
    .manufacturer_len    = 0,
    .p_manufacturer_data = NULL,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(app_svc_service_uuid),
    .p_service_uuid      = (uint8_t*) &app_svc_service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

/* scan response data, also less than 31 bytes */

// iOS always requests the response message. include the name in this message
// and don't run into length limitations as easily (since the primary service UUID
// is 16 bytes long)
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp        = true,
    .include_name        = true,
    .include_txpower     = false,
    .min_interval        = 0x0006,
    .max_interval        = 0x0010,
    .appearance          = 0x00,
    .manufacturer_len    = 0,
    .p_manufacturer_data = NULL,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = 0,
    .p_service_uuid      = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};


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



/* Full Database Description - Used to add attributes into the database */
const esp_gatts_attr_db_t app_svc_db[APP_IDX_NB] =
{
    // Service Declaration
    [IDX_APP_SVC] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&primary_service_uuid,
                           ESP_GATT_PERM_READ,
                           sizeof(app_svc_service_uuid),
                           sizeof(app_svc_service_uuid),
                           (uint8_t *)&app_svc_service_uuid}},

    /* Characteristic Declaration */
    [IDX_APP_CHAR_SPP] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&character_declaration_uuid,
                           ESP_GATT_PERM_READ,
                           CHAR_DECLARATION_SIZE,
                           CHAR_DECLARATION_SIZE,
                           (uint8_t *)&char_prop_write_notify}},

    /* Characteristic Value */
    [IDX_APP_CHAR_VAL_SPP] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128,
                           (uint8_t *)&app_svc_read_uuid,
                           ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                           1,
                           1,
                           (uint8_t *)&app_svc_read_data}},

    [IDX_APP_CHAR_CFG_SPP] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t*) &character_client_config_uuid,
                           ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                           sizeof(uint16_t),
                           sizeof(uint16_t),
                           (uint8_t*) &app_svc_read_ccc}},
#if 0
    /* Characteristic Declaration */
    [IDX_APP_CHAR_WRITE]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&character_declaration_uuid,
                           ESP_GATT_PERM_READ,
                           CHAR_DECLARATION_SIZE,
                           CHAR_DECLARATION_SIZE,
                           (uint8_t *)&char_prop_write}},

    /* Characteristic Value */
    [IDX_APP_CHAR_VAL_WRITE] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128,
                           (uint8_t *)&app_svc_write_uuid,
                           ESP_GATT_PERM_WRITE,
                           sizeof(app_svc_write_buffer),
                           0,
                           (uint8_t *)&app_svc_write_buffer}},
#endif
};


static void
prepare_advertising()
{
    esp_err_t ret;

    // TODO -- pull this value dynamically
    ble.setDeviceName("devicename");

    // establish the data broadcast as part of advertising...
    ret = esp_ble_gap_config_adv_data(&adv_data);
    if (ret) {
        ESP_LOGE(TAG, "config adv data failed, error code = %x %s", ret, esp_err_to_name(ret));
    }
    adv_config_done |= ADV_CONFIG_FLAG;

    // and then do the same thing again with the scan response data
    ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
    if (ret) {
        ESP_LOGE(TAG, "config scan response data failed, error code = %x %s", ret, esp_err_to_name(ret));
    }
    adv_config_done |= SCAN_RSP_CONFIG_FLAG;
}


static void
set_connection_params(esp_ble_gatts_cb_param_t *param)
{
    esp_log_buffer_hex(TAG, param->connect.remote_bda, 6);
    esp_ble_conn_update_params_t conn_params;
    memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));

    /* For the iOS system, please refer to Apple official documents about
     * the BLE connection parameters restrictions.
     */
    conn_params.latency = 0;
    conn_params.min_int = 0x10;
    conn_params.max_int = 0x20;
    conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
    //start sent the update connection parameters to the peer device.
    esp_ble_gap_update_conn_params(&conn_params);
}


/*
 * This event handler is also declared to be the one responsible for all
 * "global" events.
 *
 */

void
app_svc_profile_event_handler(esp_gatts_cb_event_t event,
                              esp_gatt_if_t gatts_if,
                              esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGV(TAG, "%s", __FUNCTION__);
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            ESP_LOGI(TAG, "ESP_GATTS_REG_EVT");

            // first, handle the advertising setup
            prepare_advertising();

            // now set up the rest of the details of this service

            esp_err_t ret = esp_ble_gatts_create_attr_tab(app_svc_db, gatts_if, APP_IDX_NB, APP_SVC_IDX);
            if (ret){
                ESP_LOGE(TAG, "create app svc attr table failed, error code = %s", esp_err_to_name(ret));
            }
            }
       	    break;
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_READ_EVT");
            #if 0
            if (param->read.handle == wifi_handle_table[IDX_CHAR_VAL_SPP]) {
                // start notify refresh of current mdns server entries
                throttleController.requestServerList();
            }
            else if (param->read.handle == wifi_handle_table[IDX_PROTOCOL_CHAR_VAL_ROSTER_LIST]) {
                // start notify refresh of current roster entries
                ESP_LOGI(TAG, "protocol roster list requested");
                throttleController.requestProtocolRoster();
            }
            #endif
       	    break;
        case ESP_GATTS_WRITE_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT handle %d", param->write.handle);
            if (param->write.handle == app_svc_handle_table[IDX_APP_CHAR_CFG_SPP]) {
                update_notify_value(param, &notify_svc_app_spp, "spp");
                if (notify_svc_app_spp != NONE) {
                    // do something here?
                }
            }
            else if (param->write.handle == app_svc_handle_table[IDX_APP_CHAR_VAL_SPP]) {
                esp_log_buffer_hex(TAG, param->write.value, param->write.len);

                // do something real with the data read....
            }
            #if 0
            else if (param->write.handle == wifi_handle_table[IDX_WIFI_CHAR_CFG_STATUS]) {
                update_notify_value(param, &notify_status, "wifi status");
            }
            else if (param->write.handle == wifi_handle_table[IDX_WIFI_CHAR_CFG_INTF_INFO]) {
                update_notify_value(param, &notify_intf_info_status, "wifi intf info");
            }
            else if (param->write.handle == wifi_handle_table[IDX_WIFI_CHAR_VAL_COMMAND]) {
                ESP_LOGI(TAG, "WIFI COMMAND: len=%d '%s'", param->write.len, param->write.value);
                std::string command((const char *)param->write.value, param->write.len);
                wifiController.processCommand(command);
            }
            else if (param->write.handle == wifi_handle_table[IDX_PROTOCOL_CHAR_VAL_COMMAND]) {
                ESP_LOGI(TAG, "PROTOCOL COMMAND: len=%d '%s'", param->write.len, param->write.value);
                std::string command((const char *)param->write.value, param->write.len);
                throttleController.processProtocolCommand(command);
            }
            else if (param->write.handle == wifi_handle_table[IDX_OTA_CHAR_VAL_UPDATE]) {
                ESP_LOGD(TAG, "UPDATE MESSAGE: len=%d", param->write.len);
                throttleController.processOTAUpdateMessage(param->write.len, param->write.value);
            }
            else if (param->write.handle == wifi_handle_table[IDX_PROTOCOL_CHAR_CFG_SERVER_LIST]) {
                update_notify_value(param, &notify_protocol_entry, "protocol server entry");
            }
            else if (param->write.handle == wifi_handle_table[IDX_PROTOCOL_CHAR_CFG_ROSTER_LIST]) {
                update_notify_value(param, &notify_roster_list, "protocol roster list");
            }
            else if (param->write.handle == wifi_handle_table[IDX_PROTOCOL_CHAR_CFG_STATUS]) {
                update_notify_value(param, &notify_protocol_status, "protocol status");
            }
            else if (param->write.handle == wifi_handle_table[IDX_PROTOCOL_CHAR_CFG_LASHUP]) {
                update_notify_value(param, &notify_protocol_lashup, "protocol lashup");
            }
            else if (param->write.handle == wifi_handle_table[IDX_OTA_CHAR_CFG_UPDATE]) {
                update_notify_value(param, &notify_ota_status, "ota update");
            }
            #endif
            else {
                ESP_LOGE(TAG, "write to unknown handle %d", param->write.handle);
            }
            /* send response when param->write.need_rsp is true*/
            if (param->write.need_rsp){
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
            }

      	    break;
        case ESP_GATTS_EXEC_WRITE_EVT:
            // the length of gattc prepare write data must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
            ESP_LOGI(TAG, "APP_SVC ESP_GATTS_EXEC_WRITE_EVT");
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGV(TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(TAG, "APP_SVC SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d, if = %d", param->connect.conn_id, gatts_if);
            set_connection_params(param);
            ESP_LOGI(TAG, "connected, stop advertising");
            // update BLE state in the application (connected)
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, conn_id = %d, if = %d", param->connect.conn_id, gatts_if);
            // update BLE state in the application (now disconnected)
            ESP_LOGI(TAG, "disconnected, start advertising");
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != APP_IDX_NB){
                ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to APP_IDX_NB(%d)", param->add_attr_tab.num_handle, APP_IDX_NB);
            }
            else {
                ESP_LOGI(TAG, "create app attribute table successfully, the number handle = %d", param->add_attr_tab.num_handle);
                memcpy(app_svc_handle_table, param->add_attr_tab.handles, sizeof(app_svc_handle_table));

                for(size_t i = IDX_APP_SVC; i < APP_IDX_NB; i++) {
                    ESP_LOGI(TAG, "handle for idx=%d = %d", i, app_svc_handle_table[i]);
                }

                esp_err_t rv = esp_ble_gatts_start_service(app_svc_handle_table[IDX_APP_SVC]);
                if (rv != ESP_OK) {
                    ESP_LOGE(TAG, "unable to start APP service: %d", rv);
                }
                else {
                    ESP_LOGI(TAG, "started APP service");
                }
            }
            break;
        }
        case ESP_GATTS_STOP_EVT:
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
        case ESP_GATTS_LISTEN_EVT:
        case ESP_GATTS_CONGEST_EVT:
        case ESP_GATTS_UNREG_EVT:
        case ESP_GATTS_DELETE_EVT:
        default:
            break;
    }
}


void app_svc_send_data(const uint8_t* data, const size_t length)
{
    if (length > 0) {

        esp_gatt_if_t gatts_if = service_profile_tab[APP_SVC_IDX].gatts_if;
        uint16_t conn_id =  service_profile_tab[APP_SVC_IDX].conn_id;
        uint16_t handle = app_svc_handle_table[IDX_APP_CHAR_VAL_SPP];

        if (notify_svc_app_spp != NONE) {
            esp_ble_gatts_send_indicate(gatts_if,
                                        conn_id,
                                        handle,
                                        length,
                                        (uint8_t*) data,
                                        (notify_svc_app_spp == INDICATE));

            esp_log_buffer_hex(TAG, data, length);
        }
    }
}

#if 0
void ble_notify_wifi_interface_info(const std::string info)
{
    esp_gatt_if_t gatts_if = service_profile_tab[WIFI_SVC_IDX].gatts_if;
    uint16_t conn_id =  service_profile_tab[WIFI_SVC_IDX].conn_id;
    uint16_t handle = wifi_handle_table[IDX_WIFI_CHAR_VAL_INTF_INFO];

    esp_ble_gatts_set_attr_value(handle, info.size(), (uint8_t*) info.data());

    if (notify_intf_info_status != NONE) {
        esp_ble_gatts_send_indicate(gatts_if,
                                    conn_id,
                                    handle,
                                    info.size(),
                                    (uint8_t*) info.data(),
                                    (notify_intf_info_status == INDICATE));

        ESP_LOGI(TAG, "notify intf info status: %s", info.c_str());
    }
}


void ble_notify_wifi_status(const std::string status)
{
    esp_gatt_if_t gatts_if = service_profile_tab[WIFI_SVC_IDX].gatts_if;
    uint16_t conn_id =  service_profile_tab[WIFI_SVC_IDX].conn_id;
    uint16_t handle = wifi_handle_table[IDX_WIFI_CHAR_VAL_STATUS];

    esp_ble_gatts_set_attr_value(handle, status.size(), (uint8_t*) status.data());

    if (notify_status != NONE) {
        esp_ble_gatts_send_indicate(gatts_if,
                                    conn_id,
                                    handle,
                                    status.size(),
                                    (uint8_t*) status.data(),
                                    (notify_status == INDICATE));

        ESP_LOGI(TAG, "notify wifi status: %s", status.c_str());
    }
}


// mDNS discovered protocol Servers
void ble_notify_protocol_server_entry(const std::string entry)
{
    esp_gatt_if_t gatts_if = service_profile_tab[WIFI_SVC_IDX].gatts_if;
    uint16_t conn_id =  service_profile_tab[WIFI_SVC_IDX].conn_id;
    uint16_t handle = wifi_handle_table[IDX_PROTOCOL_CHAR_VAL_SERVER_LIST];

    if (notify_protocol_entry != NONE) {
        esp_ble_gatts_send_indicate(gatts_if,
                                    conn_id,
                                    handle,
                                    entry.size(),
                                    (uint8_t*) entry.data(),
                                    (notify_protocol_entry == INDICATE));

        ESP_LOGI(TAG, "notify protocol entry: %s", entry.c_str());
    }
}


// entries from the JMRI Roster
void ble_notify_protocol_roster_list(const std::string entry)
{
    esp_gatt_if_t gatts_if = service_profile_tab[WIFI_SVC_IDX].gatts_if;
    uint16_t conn_id =  service_profile_tab[WIFI_SVC_IDX].conn_id;
    uint16_t handle = wifi_handle_table[IDX_PROTOCOL_CHAR_VAL_ROSTER_LIST];

    if (notify_roster_list != NONE) {
        esp_err_t rv = esp_ble_gatts_send_indicate(gatts_if,
                                                   conn_id,
                                                   handle,
                                                   entry.size(),
                                                   (uint8_t*) entry.data(),
                                                   (notify_roster_list == INDICATE));

        ESP_LOGI(TAG, "notify protocol roster list entry: 0x%02x %s", rv, entry.c_str());
    }
}


// connection state of Protocol connection
void ble_notify_protocol_status(const std::string status)
{
    esp_gatt_if_t gatts_if = service_profile_tab[WIFI_SVC_IDX].gatts_if;
    uint16_t conn_id =  service_profile_tab[WIFI_SVC_IDX].conn_id;
    uint16_t handle = wifi_handle_table[IDX_PROTOCOL_CHAR_VAL_STATUS];

    esp_err_t rv = esp_ble_gatts_set_attr_value(handle, status.size(), (uint8_t*) status.data());
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "can't write protocol status: %s", esp_err_to_name(rv));
    }

    if (notify_protocol_status != NONE) {
        esp_ble_gatts_send_indicate(gatts_if,
                                    conn_id,
                                    handle,
                                    status.size(),
                                    (uint8_t*) status.data(),
                                    (notify_protocol_status == INDICATE));

        ESP_LOGI(TAG, "notify protocol status: %s", status.c_str());
    }
}


void ble_notify_protocol_lashup(const std::string json)
{
    esp_gatt_if_t gatts_if = service_profile_tab[WIFI_SVC_IDX].gatts_if;
    uint16_t conn_id =  service_profile_tab[WIFI_SVC_IDX].conn_id;
    uint16_t handle = wifi_handle_table[IDX_PROTOCOL_CHAR_VAL_LASHUP];

    esp_err_t rv = esp_ble_gatts_set_attr_value(handle, json.size(), (uint8_t*) json.data());
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "can't write protocol lashup: %s", esp_err_to_name(rv));
    }

    if (notify_protocol_lashup != NONE) {
        esp_ble_gatts_send_indicate(gatts_if,
                                    conn_id,
                                    handle,
                                    json.size(),
                                    (uint8_t*) json.data(),
                                    (notify_protocol_lashup == INDICATE));

        ESP_LOGI(TAG, "notify protocol lashup: %s", json.c_str());
    }
}


void ble_notify_ota_status(const std::string status)
{
    esp_gatt_if_t gatts_if = service_profile_tab[WIFI_SVC_IDX].gatts_if;
    uint16_t conn_id =  service_profile_tab[WIFI_SVC_IDX].conn_id;
    uint16_t handle = wifi_handle_table[IDX_OTA_CHAR_VAL_UPDATE];

    esp_err_t rv = esp_ble_gatts_set_attr_value(handle, status.size(), (uint8_t*) status.data());
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "can't write OTA update status: %s", esp_err_to_name(rv));
    }

    if (notify_ota_status != NONE) {
        esp_ble_gatts_send_indicate(gatts_if,
                                    conn_id,
                                    handle,
                                    status.size(),
                                    (uint8_t*) status.data(),
                                    (notify_ota_status == INDICATE));

        ESP_LOGI(TAG, "notify OTA status: len=%d", status.size());
    }
}
#endif
