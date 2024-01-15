//
// SPDX-License-Identifier: GPL-2.0-only
//

#pragma once

#include "esp_event.h"
#include "esp_log.h"

class Logger
{
  public:
    Logger();

    void begin();
    void log(char *msg, size_t len);


  protected:
    static void static_event_handler(void *ctx,
                                     esp_event_base_t event_base,
                                     int32_t event_id,
                                     void *event_data);

    esp_err_t event_handler(esp_event_base_t event_base,
                            int32_t event_id,
                            void *event_data);

    esp_err_t post_event(int32_t event_id,
                         void *event_data,
                         size_t event_data_size,
                         TickType_t ticks_to_wait);

    void handlePostedLogMessage(void *event_data);

    esp_event_loop_handle_t event_loop_handle;
};


extern Logger logger;
