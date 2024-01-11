//
// SPDX-License-Identifier: GPL-2.0-only
//

#pragma once

#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <string>
#include "thread.hpp"

#include "sdkconfig.h"

#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
#include "driver/usb_serial_jtag.h"
#elif CONFIG_ESP_CONSOLE_UART
#include "driver/uart.h"
#endif

using namespace cpp_freertos;

class CommandLineHandler : public Thread
{
  public:
    CommandLineHandler(std::string name, UBaseType_t priority);
    void begin();
    void Run();

  private:

};

extern CommandLineHandler commandLine;

#endif // COMMANDLINE_H
