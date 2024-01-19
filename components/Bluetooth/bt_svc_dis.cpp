#include <cstdint>
#include <cstring>

#include "esp_log.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

#include "Bluetooth.h"

#include "bt_svc_dis.h"
#include "bt_general.h"

static const char *TAG = "svc_dis";

static uint16_t dis_handle_table[DIS_IDX_NB];

static uint16_t dis_service_uuid       = 0x180A;
static uint16_t dis_model_number_uuid  = 0x2A24;
static uint16_t dis_serial_number_uuid = 0x2A25;
static uint16_t dis_fw_revision_uuid   = 0x2A26;
static uint16_t dis_hw_revision_uuid   = 0x2A27;
static uint16_t dis_sw_revision_uuid   = 0x2A28;


static const size_t CHAR_VAL_LEN = 32;
static char dis_model_number_value[CHAR_VAL_LEN];
static char dis_serial_number_value[CHAR_VAL_LEN];

static char dis_fw_revision_value[CHAR_VAL_LEN];
static char dis_hw_revision_value[CHAR_VAL_LEN];
static char dis_sw_revision_value[CHAR_VAL_LEN];



esp_gatts_attr_db_t dis_db[DIS_IDX_NB] =
{
    // Service Declaration
    [IDX_DIS_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
                           sizeof(dis_service_uuid), sizeof(dis_service_uuid), (uint8_t *)&dis_service_uuid}},

    /* Characteristic Declaration */
    [IDX_DIS_CHAR_MODEL_NUMBER]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&character_declaration_uuid,
                           ESP_GATT_PERM_READ,
                           CHAR_DECLARATION_SIZE,
                           CHAR_DECLARATION_SIZE,
                           (uint8_t *)&char_prop_read}},

    /* Characteristic Value */
    [IDX_DIS_CHAR_VAL_MODEL_NUMBER] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&dis_model_number_uuid,
                           ESP_GATT_PERM_READ,
                           CHAR_VAL_LEN,
                           0,
                           (uint8_t *)dis_model_number_value}},


    /* Characteristic Declaration */
    [IDX_DIS_CHAR_SERIAL_NUMBER]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&character_declaration_uuid,
                           ESP_GATT_PERM_READ,
                           CHAR_DECLARATION_SIZE,
                           CHAR_DECLARATION_SIZE,
                           (uint8_t *)&char_prop_read}},

    /* Characteristic Value */
    [IDX_DIS_CHAR_VAL_SERIAL_NUMBER] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&dis_serial_number_uuid,
                           ESP_GATT_PERM_READ,
                           CHAR_VAL_LEN,
                           0,
                           (uint8_t *)dis_serial_number_value}},

    /* Characteristic Declaration */
    [IDX_DIS_CHAR_FW_REVISION]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&character_declaration_uuid,
                           ESP_GATT_PERM_READ,
                           CHAR_DECLARATION_SIZE,
                           CHAR_DECLARATION_SIZE,
                           (uint8_t *)&char_prop_read}},

    /* Characteristic Value */
    [IDX_DIS_CHAR_VAL_FW_REVISION] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&dis_fw_revision_uuid,
                           ESP_GATT_PERM_READ,
                           CHAR_VAL_LEN,
                           0,
                           (uint8_t *)dis_fw_revision_value}},

    /* Characteristic Declaration */
    [IDX_DIS_CHAR_HW_REVISION]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&character_declaration_uuid,
                           ESP_GATT_PERM_READ,
                           CHAR_DECLARATION_SIZE,
                           CHAR_DECLARATION_SIZE,
                           (uint8_t *)&char_prop_read}},

    /* Characteristic Value */
    [IDX_DIS_CHAR_VAL_HW_REVISION] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&dis_hw_revision_uuid,
                           ESP_GATT_PERM_READ,
                           CHAR_VAL_LEN,
                           0,
                           (uint8_t *)dis_hw_revision_value}},

    /* Characteristic Declaration */
    [IDX_DIS_CHAR_SW_REVISION]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&character_declaration_uuid,
                           ESP_GATT_PERM_READ,
                           CHAR_DECLARATION_SIZE,
                           CHAR_DECLARATION_SIZE,
                           (uint8_t *)&char_prop_read}},

    /* Characteristic Value */
    [IDX_DIS_CHAR_VAL_SW_REVISION] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&dis_sw_revision_uuid,
                           ESP_GATT_PERM_READ,
                           CHAR_VAL_LEN,
                           0,
                           (uint8_t *)dis_sw_revision_value}},

};


void
dis_profile_event_handler(esp_gatts_cb_event_t event,
                          esp_gatt_if_t gatts_if,
                          esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            ESP_LOGI(TAG, "ESP_GATTS_REG_EVT");
            esp_err_t ret = esp_ble_gatts_create_attr_tab(dis_db,
                                                          gatts_if,
                                                          DIS_IDX_NB,
                                                          DIS_SVC_IDX);
            if (ret != ESP_OK){
                ESP_LOGE(TAG, "create dis attr table failed, error code = %s", esp_err_to_name(ret));
            }
       	    break;
        }
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_READ_EVT");
       	    break;
        case ESP_GATTS_WRITE_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT");
      	    break;
        case ESP_GATTS_EXEC_WRITE_EVT:
            // the length of gattc prepare write data must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
            ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d, if = %d", param->connect.conn_id, gatts_if);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, conn_id = %d, if = %d", param->connect.conn_id, gatts_if);
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != DIS_IDX_NB){
                ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to DIS_IDX_NB(%d)", param->add_attr_tab.num_handle, DIS_IDX_NB);
            }
            else {
                ESP_LOGI(TAG, "create dis attribute table successfully, the number handle = %d",param->add_attr_tab.num_handle);
                memcpy(dis_handle_table, param->add_attr_tab.handles, sizeof(dis_handle_table));

                // now that we have the handles, populate the "static"
                // values from the HW interface

                std::string model = "model number";
                esp_ble_gatts_set_attr_value(dis_handle_table[IDX_DIS_CHAR_VAL_MODEL_NUMBER],
                                             model.size(),
                                             (const uint8_t*) model.data());

                std::string serial = "serial number";
                esp_ble_gatts_set_attr_value(dis_handle_table[IDX_DIS_CHAR_VAL_SERIAL_NUMBER],
                                             serial.size(),
                                             (const uint8_t*) serial.data());

                std::string fw_rev = "fw revision";
                esp_ble_gatts_set_attr_value(dis_handle_table[IDX_DIS_CHAR_VAL_FW_REVISION],
                                             fw_rev.size(),
                                             (const uint8_t*) fw_rev.data());

                std::string hw_rev = "hw_revision";
                esp_ble_gatts_set_attr_value(dis_handle_table[IDX_DIS_CHAR_VAL_HW_REVISION],
                                             hw_rev.size(),
                                             (const uint8_t*) hw_rev.data());

                std::string sw_rev = "sw_revision";
                esp_ble_gatts_set_attr_value(dis_handle_table[IDX_DIS_CHAR_VAL_SW_REVISION],
                                             sw_rev.size(),
                                             (const uint8_t*) sw_rev.data());

                esp_ble_gatts_start_service(dis_handle_table[IDX_DIS_SVC]);
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
