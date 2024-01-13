//
// SPDX-License-Identifier: GPL-2.0-only
//

#include <PilotLight.h>

#include <inttypes.h>

#include <ticks.hpp>

#include "esp_console.h"
#include "esp_log.h"
#include "led_strip.h"

#include "app_priority.h"

static const char *TAG = "pilot";


////////////////////////////////////////////////////////////////////////

#if CONFIG_APP_PILOT_LIGHT_COMMAND

int
cmd_pilot(int argc, char**argv)
{
    if (argc==3) {
        long on  = static_cast<int>(strtol(argv[1], NULL, 10));
        long off = static_cast<int>(strtol(argv[2], NULL, 10));
        if ((on > 0) && (off > 0)) {
            ESP_LOGI("pilot", "set on=%ld, off=%ld", on, off);
            pilotLight.setOnOffTime(on,off);
        }
        else {
            ESP_LOGE("pilot", "invalid on or off time");
        }
    }
    else {
        uint32_t on_time = pilotLight.getOnTime();
        uint32_t off_time = pilotLight.getOffTime();

        printf("pilot on=%" PRIu32 " off=%" PRIu32 "\n", on_time, off_time);
    }
    return ESP_OK;
}


void
register_pilot_cmd(void)
{
    const esp_console_cmd_t cmd = {
        .command = "pilot",
        .help = "set pilot light on/off times",
        .hint = NULL,
        .func = &cmd_pilot,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

#endif // CONFIG_APP_PILOT_LIGHT_COMMAND


////////////////////////////////////////////////////////////////////////


PilotLight::PilotLight(std::string name, UBaseType_t priority,
                       uint32_t pon_time, uint32_t poff_time)
    : Thread(name, CONFIG_APP_PILOT_LIGHT_STACK_SIZE, priority, /*CPU*/ 1)
    , pin((gpio_num_t) CONFIG_HW_PILOT_LIGHT_GPIO)
    , on_time(Ticks::MsToTicks(pon_time))
    , off_time(Ticks::MsToTicks(poff_time))
{
}


void
PilotLight::begin()
{
#if CONFIG_APP_PILOT_LIGHT_COMMAND
    register_pilot_cmd();
#endif
    Start();
}


void
PilotLight::setOnOffTime(uint32_t on_time, uint32_t off_time)
{
    this->on_time = pdMS_TO_TICKS(on_time);
    this->off_time = pdMS_TO_TICKS(off_time);
}


void
PilotLight::Run()
{
#if CONFIG_HW_PILOT_LIGHT_IS_NEOPIXEL

    led_strip_handle_t led_strip;

    led_strip_config_t strip_config = {
        .strip_gpio_num = (uint32_t) pin,
        .max_leds = 1, // at least one LED on board
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &led_strip));
    /* Set all LED off to clear all pixels */

#else
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
#endif

    ESP_LOGI(TAG, "pilot light configured");

    while (true) {
#if CONFIG_HW_PILOT_LIGHT_IS_NEOPIXEL
        led_strip_clear(led_strip);
#else
        gpio_set_level(pin, 1);
#endif
        Delay(off_time);

#if CONFIG_HW_PILOT_LIGHT_IS_NEOPIXEL
        led_strip_set_pixel(led_strip, 0, 5, 6, 5);
        led_strip_refresh(led_strip);
#else
        gpio_set_level(pin, 0);
#endif
        Delay(on_time);

    }
}


void
PilotLight::Suspend()
{
    gpio_set_level(pin, 0);
    Thread::Suspend();
}



PilotLight pilotLight("pilotLight",
                      PRI_PILOT_LIGHT,
                      CONFIG_APP_PILOT_LIGHT_ON_TIME,
                      CONFIG_APP_PILOT_LIGHT_OFF_TIME);
