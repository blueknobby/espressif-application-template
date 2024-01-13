//
// SPDX-License-Identifier: GPL-2.0-only
//

// ESP-IDF includes
#include "esp_chip_info.h"
#include "esp_event.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

// general component includes
#include "thread.hpp"

// application component includes
#include "CommandLine.h"

#include "cmds.h"

using namespace cpp_freertos;

static const char *TAG = "main";

nvs_handle nvs_data_handle;



void
shutdown_handler()
{
    ESP_LOGI(TAG, "system shutting down");
}


static void
init_nvs()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    err = nvs_open("storage", NVS_READWRITE, &nvs_data_handle);
    ESP_ERROR_CHECK( err );

    ESP_LOGI(TAG, "nvs initialized");
}


void
print_chip_info()
{
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    ESP_LOGI(TAG, "This is %s chip with %d CPU core(s), WiFi%s%s%s",
             CONFIG_IDF_TARGET,
             chip_info.cores,
             (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
             (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
             (chip_info.features & CHIP_FEATURE_IEEE802154) ? "/802.15.4 (Zigbee/Thread)" : "");

    ESP_LOGI(TAG, "silicon revision %" PRIi16 , chip_info.revision);

    uint32_t flash_size;
    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        ESP_LOGE(TAG,"error reading flash size");
    }

    ESP_LOGI(TAG, "%" PRIu32 "MB %s flash", flash_size / (1024 * 1024),
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    if (chip_info.features & CHIP_FEATURE_EMB_PSRAM) {
        ESP_LOGI(TAG, "embedded PSRAM present");
    }

    ESP_LOGI(TAG, "Minimum free heap size: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());
}


void heap_caps_alloc_failed_hook(size_t requested_size, uint32_t caps, const char *function_name)
{
    ESP_LOGE(TAG, "%s was called but failed to allocate %zu bytes with 0x%" PRIx32 " capabilities. \n",
           function_name, requested_size, caps);

    // DO SOMETHING HERE TO INDICATE TO THE USER THAT THE SYSTEM HAS STOPPED
    while (true) {
        // tickle the watchdog...
    }
}


void
setup()
{
    ESP_ERROR_CHECK(heap_caps_register_failed_alloc_callback(heap_caps_alloc_failed_hook));

    ESP_LOGI(TAG, "setup starting");
    esp_register_shutdown_handler(shutdown_handler);

    print_chip_info();
    show_heap_info();

    init_nvs();

    ESP_ERROR_CHECK( esp_event_loop_create_default() );

    // now start up the various application components
    commandLine.begin();

    register_heap_cmd();
    register_task_cmd();
    register_event_cmd();



    Thread::StartScheduler();

    ESP_LOGI(TAG, "setup complete");
}


void
run_tick_events(uint64_t tick_counter)
{
    // send tick_counter to the main controller
}

void
run_one_second_tick_events(uint64_t one_second_tick_counter)
{
    // send one_second_tick_counter to the main controller

#if CONFIG_APP_RUN_HEAP_INTEGRITY_CHECK
    if ((one_second_tick_counter % CONFIG_APP_HEAP_INTEGRITY_CHECK_INTERVAL) == 0) {
        ESP_LOGI(TAG, "checking heap integrity");
        if (!heap_caps_check_integrity_all(true)) {
            ESP_LOGE(TAG, "***** heap integrity failure");
            // DO SOMETHING FUNKY HERE....
        }
    }
#endif

#if CONFIG_APP_PRINT_HEAP_INFO
    if ((one_second_tick_counter % CONFIG_APP_PRINT_HEAP_INFO_INTERVAL) == 0) {
        ESP_LOGI(TAG, "heap info");
        heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
        show_heap_info();
    }
#endif
}


void
loop()
{
    const int         ms_between_ticks = 50;
    constexpr int     ticks_per_second = 1000 / ms_between_ticks;

    static uint64_t   tick_counter = 1;
    static uint64_t   one_second_tick_counter = 1;
    static TickType_t lastWakeTime = xTaskGetTickCount();

    vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(ms_between_ticks));
    tick_counter += 1;

    run_tick_events(tick_counter);

    if ((tick_counter % ticks_per_second) == 0) {
        one_second_tick_counter += 1;

        run_one_second_tick_events(one_second_tick_counter);
    }


}


extern "C"
void
app_main()
{
    ESP_LOGI(TAG, "start of app_main");

    setup();

    while(true) {
        loop();
    }

    ESP_LOGE(TAG, "this should never happen");
    // NEVER EXIT
}


extern "C"
void
esp_task_wdt_isr_user_handler(void)
{
    printf("Task Watchdog Timer Alert!\n");
    esp_restart();
}



extern "C"
void
vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
#if 1
    if (pcTaskName) {
        for (int i=0; i < 70; i++) { putc('=', stdout); }
        puts("\nstack overflow in task: ");
        puts((const char *) pcTaskName);
        putc('\n', stdout);
        for (int i=0; i < 70; i++) { putc('=', stdout); }
        putc('\n', stdout);
    }
#endif

    // TODO
    // 1. turn off all of the LEDs
    // 2. (probably) turn off the watchdog timers
    // 3. go into a tight loop where we flash the pilot light to indicate
    //    that we're in this hook.

}
