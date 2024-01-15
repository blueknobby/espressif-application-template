//
// SPDX-License-Identifier: GPL-2.0-only
//

#include <cstdarg>
#include <cstring>
#include <inttypes.h>

#include "Logger.h"

#include "AppEvents.h"

#include "app_priority.h"

////////////////////////////////////////////////////////////////////////////////
//
//
// Logger does not USE ESP_LOGx logging (or else we'd be lost in recursion hell)
// except in begin() where it's a demonstration that it's actually working
//
// If you care about the details of Logger, you have to use the serial console and
// watch for the printf statements
//
//
////////////////////////////////////////////////////////////////////////////////

static const char *TAG = "logger";

// globally accessible logger
Logger logger;

// internal lock to ensure that only one log message is handled at a time
static SemaphoreHandle_t logLock = NULL;


static int
log_formatting_func(const char *fmt, va_list ap)
{
    char *logMessage = NULL;

    if (logLock == NULL) {
        return 0;
    }

    xSemaphoreTake(logLock, portMAX_DELAY);

    // this will allocate space for the log message.  The pointer will be copied
    // around the event handlingsystem, and will finally end up in the event
    // processing function for use.  After the content has been processed,
    // that function will be responsible for freeing the memory allocated here.

    size_t len = vasprintf(&logMessage, fmt, ap);
    if (logMessage != NULL && len > 0) {
        logger.log(logMessage, len);
    }

    xSemaphoreGive(logLock);

    return len;
}


Logger::Logger()
{
}


void
Logger::begin()
{
    logLock = xSemaphoreCreateBinary();
    if (logLock == NULL) {
        ESP_LOGE(TAG, "Unable to create log semaphore");
        return;
    }

    xSemaphoreGive(logLock);

    esp_event_loop_args_t event_loop_args;
    event_loop_args.queue_size = 40;
    event_loop_args.task_name = "logger";
    event_loop_args.task_priority = PRI_LOGGER;
    event_loop_args.task_stack_size = CONFIG_APP_LOGGER_STACK_SIZE;
    event_loop_args.task_core_id = 1;

    esp_err_t rv;
    rv = esp_event_loop_create(&event_loop_args, &event_loop_handle);
    ESP_ERROR_CHECK(rv);

    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "unable to create Logger event loop");
        return;
    }

    rv = esp_event_handler_register_with(event_loop_handle,
                                         LOGGER_EVENT, ESP_EVENT_ANY_ID,
                                         &Logger::static_event_handler,
                                         static_cast<void*>(this));
    if (rv != ESP_OK) {
        ESP_LOGE(TAG, "Error creating Logger event handler: %s", esp_err_to_name(rv));
    }
    else {
        esp_log_set_vprintf(log_formatting_func);
        ESP_LOGI(TAG, "initialized\n");
    }
}


void
Logger::log(char *msg, size_t len)
{
    if (msg && len > 0) {
        // this makes the assumption that there are no embedded NUL characters in a log
        // message (since the size value isn't passed to the event handling function)
        //
        post_event(POST_LOG_MESSAGE, static_cast<void*>(&msg), sizeof(char*), portMAX_DELAY);
    }
}


////////////////////////////////////////////////////////////////////////////////

void
Logger::static_event_handler(void *ctx,
                             esp_event_base_t event_base,
                             int32_t event_id,
                             void *event_data)
{
    Logger *lp = static_cast<Logger*>(ctx);
    if (lp) {
        lp->event_handler(event_base, event_id, event_data);
    }
}


esp_err_t
Logger::event_handler(esp_event_base_t event_base,
                      int32_t event_id,
                      void *data)
{
    if (event_base == LOGGER_EVENT) {
        switch (event_id) {
            case POST_LOG_MESSAGE:
                handlePostedLogMessage(data);
                break;
            default:
                fprintf(stderr, "unknown LOGGER_EVENT received: %" PRIi32, event_id);
                break;
        }
    }
    else {
        fprintf(stderr, "unknown event BASE %s", event_base);
    }
    return ESP_OK;
}


esp_err_t
Logger::post_event(int32_t event_id,
                   void *event_data,
                   size_t event_data_size,
                   TickType_t ticks_to_wait)
{
    return esp_event_post_to(event_loop_handle,
                             LOGGER_EVENT, event_id,
                             event_data, event_data_size,
                             ticks_to_wait);
}


void
Logger::handlePostedLogMessage(void *event_data)
{
    if (event_data == NULL) {
        return;
    }

    char **msg = static_cast<char**>(event_data);
    if (*msg) {
        char *actual = *msg;
        int msgEnd = strlen(actual)-1;

        while (msgEnd >= 0) {
            if ((actual[msgEnd]=='\n') || (actual[msgEnd]=='\r')) {
                actual[msgEnd]='\0';
                msgEnd -= 1;
            }
            else {
                // only remove the newline/CR at the END of the string...
                break;
            }
        }

        // copy to bluetooth if someone is listening
        //

        // save the message to a file...
        //

        // display to the serial console (puts adds the terminating newline)
        //
        puts(*msg);

        // this was allocated via the vasprintf in log_formatting_func, so now
        // we are finally done with the string...
        free(*msg);
    }
}
