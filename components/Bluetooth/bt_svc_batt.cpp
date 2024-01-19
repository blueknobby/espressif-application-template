#include <cstdint>
#include <cstring>

#include "esp_log.h"
#include "esp_console.h"
#include "esp_gatts_api.h"

#include "bt_svc_batt.h"
#include "bt_general.h"

static const char *TAG = "svc_batt";

static uint16_t batt_handle_table[BATT_IDX_NB];

static uint16_t batt_service_uuid      = 0x180F;
static uint16_t batt_level_uuid        = 0x2A19;

static uint8_t  batt_level             = 0;      // 0-100

static const uint8_t batt_level_ccc[2]      = {0x00, 0x00};


static notification_type notify_battery = NONE;

/* Full Database Description - Used to add attributes into the database */
const esp_gatts_attr_db_t batt_db[BATT_IDX_NB] =
{
    // Service Declaration
    [IDX_BATT_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&primary_service_uuid,
                           ESP_GATT_PERM_READ,
                           sizeof(batt_service_uuid),
                           sizeof(batt_service_uuid),
                           (uint8_t *)&batt_service_uuid}},

    /* Characteristic Declaration */
    [IDX_BATT_CHAR_BATT_LEVEL]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&character_declaration_uuid,
                           ESP_GATT_PERM_READ,
                           CHAR_DECLARATION_SIZE,
                           CHAR_DECLARATION_SIZE,
                           (uint8_t *)&char_prop_read_notify}},

    /* Characteristic Value */
    [IDX_BATT_CHAR_VAL_BATT_LEVEL] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t *)&batt_level_uuid,
                           ESP_GATT_PERM_READ,
                           sizeof(batt_level),
                           sizeof(batt_level),
                           (uint8_t *)&batt_level}},

    [IDX_BATT_CHAR_CFG_BATT_LEVEL] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16,
                           (uint8_t*) &character_client_config_uuid,
                           ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                           sizeof(uint16_t),
                           sizeof(uint16_t),
                           (uint8_t*) batt_level_ccc}}
};



void batt_set_battery_level(uint8_t level)
{
    if (level <= 100) {

        esp_gatt_if_t gatts_if = service_profile_tab[BATT_SVC_IDX].gatts_if;
        uint16_t conn_id =  service_profile_tab[BATT_SVC_IDX].conn_id;
        uint16_t handle = batt_handle_table[IDX_BATT_CHAR_VAL_BATT_LEVEL];

        batt_level = level;
        esp_ble_gatts_set_attr_value(handle, sizeof(level), (uint8_t*) &level);

        if (notify_battery != NONE) {
            esp_ble_gatts_send_indicate(gatts_if,
                                        conn_id,
                                        handle,
                                        sizeof(level),
                                        (uint8_t*) &level,
                                        (notify_battery == INDICATE));

            ESP_LOGD(TAG, "notify battery level: %u", level);
        }
    }

}


static int
cmd_batt(int argc, char **argv)
{
    if (argc == 1) {
        printf("current battery level is %d\n", batt_level);
    }
    else if (argc == 2) {
        int level = strtol(argv[1], NULL, 10);
        if (level >= 0 && level <= 100) {
            batt_set_battery_level(level);
        }
        else {
            ESP_LOGE(TAG, "level must be in range 0-100");
        }
    }
    return 0;
}

static void
register_battery_cmd(void)
{
    const esp_console_cmd_t cmd = {
        .command = "battery",
        .help = "notify battery level",
        .hint = NULL,
        .func = &cmd_batt,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}


void
batt_profile_event_handler(esp_gatts_cb_event_t event,
                           esp_gatt_if_t gatts_if,
                           esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);
    switch (event) {
        case ESP_GATTS_REG_EVT:{
            ESP_LOGI(TAG, "ESP_GATTS_REG_EVT");

            register_battery_cmd();

            esp_err_t ret = esp_ble_gatts_create_attr_tab(batt_db, gatts_if, BATT_IDX_NB, BATT_SVC_IDX);
            if (ret != ESP_OK){
                ESP_LOGE(TAG, "create batt attr table failed, error code = %s", esp_err_to_name(ret));
            }
            }
       	    break;
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_READ_EVT");
       	    break;
        case ESP_GATTS_WRITE_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT");
            if (!param->write.is_prep) {
                // the data length of gattc write must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
                ESP_LOGI(TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);
                esp_log_buffer_hex(TAG, param->write.value, param->write.len);

                if (batt_handle_table[IDX_BATT_CHAR_CFG_BATT_LEVEL] == param->write.handle && param->write.len == 2){
                    uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                    if (descr_value == 0x0001) {
                        ESP_LOGI(TAG, "batt notify enable");
                        notify_battery = NOTIFY;
                    }
                    else if (descr_value == 0x0002) {
                        ESP_LOGI(TAG, "batt indicate enable");
                        notify_battery = INDICATE;
                    }
                    else if (descr_value == 0x0000) {
                        ESP_LOGI(TAG, "notify/indicate disable ");
                        notify_battery = NONE;
                    }
                    else {
                        ESP_LOGE(TAG, "unknown descr value");
                        esp_log_buffer_hex(TAG, param->write.value, param->write.len);
                    }

                }
                /* send response when param->write.need_rsp is true*/
                if (param->write.need_rsp){
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                }
            }
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
            service_profile_tab[BATT_SVC_IDX].conn_id = param->connect.conn_id;
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != BATT_IDX_NB){
                ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to BATT_IDX_NB(%d)", param->add_attr_tab.num_handle, BATT_IDX_NB);
            }
            else {
                ESP_LOGI(TAG, "create batt attribute table successfully, the number handle = %d", param->add_attr_tab.num_handle);
                memcpy(batt_handle_table, param->add_attr_tab.handles, sizeof(batt_handle_table));
                esp_ble_gatts_start_service(batt_handle_table[IDX_BATT_SVC]);
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
