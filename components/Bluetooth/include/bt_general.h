//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <string>

#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))



extern const uint16_t primary_service_uuid;
extern const uint16_t character_declaration_uuid;
extern const uint16_t character_client_config_uuid;
extern const uint8_t char_prop_read;
extern const uint8_t char_prop_write;
extern const uint8_t char_prop_notify;
extern const uint8_t char_prop_read_notify;
extern const uint8_t char_prop_write_notify;
extern const uint8_t char_prop_read_write;
extern const uint8_t char_prop_read_write_notify;


typedef enum {
    NONE,
    NOTIFY,
    INDICATE
} notification_type;

extern void
update_notify_value(esp_ble_gatts_cb_param_t *param, notification_type *value, const char *name_str);


const int ADV_CONFIG_FLAG = (1 << 0);
const int SCAN_RSP_CONFIG_FLAG = (1 << 1);

extern bool adv_config_complete;

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    std::string name;
    uint16_t app_id;
    uint16_t conn_id;
#if 0
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
#endif
};



enum
{
    APP_APP_ID = 0x10,  // this number is completely arbitrary....
    DIS_APP_ID,
    BATT_APP_ID,
};


enum
{
    APP_SVC_IDX,
    DIS_SVC_IDX,
    BATT_SVC_IDX,
    SERVICE_COUNT
};



/* Attributes State Machine */

enum
{
    IDX_APP_SVC,

    IDX_APP_CHAR_SPP,
    IDX_APP_CHAR_VAL_SPP,
    IDX_APP_CHAR_CFG_SPP,

    APP_IDX_NB
};


enum
{
    IDX_DIS_SVC,
    IDX_DIS_CHAR_MODEL_NUMBER,
    IDX_DIS_CHAR_VAL_MODEL_NUMBER,

    IDX_DIS_CHAR_SERIAL_NUMBER,
    IDX_DIS_CHAR_VAL_SERIAL_NUMBER,

    IDX_DIS_CHAR_FW_REVISION,
    IDX_DIS_CHAR_VAL_FW_REVISION,

    IDX_DIS_CHAR_HW_REVISION,
    IDX_DIS_CHAR_VAL_HW_REVISION,

    IDX_DIS_CHAR_SW_REVISION,
    IDX_DIS_CHAR_VAL_SW_REVISION,

    DIS_IDX_NB
};


enum
{
    IDX_BATT_SVC,
    IDX_BATT_CHAR_BATT_LEVEL,
    IDX_BATT_CHAR_VAL_BATT_LEVEL,
    IDX_BATT_CHAR_CFG_BATT_LEVEL,
    BATT_IDX_NB
};

// defined at the end of this include to get the value of SERVICE_COUNT
//
extern struct gatts_profile_inst service_profile_tab[SERVICE_COUNT];
