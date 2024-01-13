//
// SPDX-License-Identifier: GPL-2.0-only
//

#pragma once

#ifndef PILOTLIGHT_H
#define PILOTLIGHT_H


#include <string>

#include "thread.hpp"

#include "driver/gpio.h"

#include "sdkconfig.h"


using namespace cpp_freertos;

class PilotLight : public Thread
{
public:
    PilotLight(std::string name, UBaseType_t priority, uint32_t on_time, uint32_t off_time);
    void begin();
    void Run();
    void Suspend();

    void setOnOffTime(uint32_t on_time, uint32_t off_time);
    uint32_t getOnTime() { return pdTICKS_TO_MS(on_time); };
    uint32_t getOffTime() { return pdTICKS_TO_MS(off_time); };

private:
    gpio_num_t pin;
    uint32_t on_time;
    uint32_t off_time;
};

extern PilotLight pilotLight;

#endif // PILOTLIGHT_H
