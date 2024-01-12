//
// SPDX-License-Identifier: GPL-2.0-only
//

#include <cstring>

#include "esp_console.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"

#ifdef CONFIG_HEAP_TRACING
#include "esp_heap_trace.h"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static const char *tasksTag = "tasks";
static const char *eventTag = "event";



////////////////////////////////////////////////////////////////////////




void
show_heap_info(void)
{
    ESP_LOGI(tasksTag, "free bytes in heap: %8zu", xPortGetFreeHeapSize());
    ESP_LOGI(tasksTag, "ESP heap size:      %8lu", esp_get_free_heap_size());
    ESP_LOGI(tasksTag, "largest free block: %8zu", heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
    ESP_LOGI(tasksTag, "internal heap size: %8u", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
}


int
cmd_heap(int argc, char**argv)
{
    show_heap_info();
    return ESP_OK;
}


void
register_heap_cmd(void)
{
    const esp_console_cmd_t cmd = {
        .command = "heap",
        .help = "show heap/memory information",
        .hint = NULL,
        .func = &cmd_heap,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}



int
cmd_tasks(int argc, char **argv)
{
    /* Take a snapshot of the number of tasks in case it changes while this
       function is executing. */
    UBaseType_t  uxArraySize = uxTaskGetNumberOfTasks();

    /* Allocate a TaskStatus_t structure for each task.  An array could be
       allocated statically at compile time. */
    TaskStatus_t *pxTaskStatusArray = (TaskStatus_t*) pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

    if( pxTaskStatusArray != NULL )
    {
        uint32_t ulTotalRunTime;
        /* Generate raw status information about each task. */
        uxArraySize = uxTaskGetSystemState( pxTaskStatusArray,
                                            uxArraySize,
                                            &ulTotalRunTime );

        if (uxArraySize > 0) {

            /* For percentage calculations. */
            ulTotalRunTime /= 100UL;

            /* For each populated position in the pxTaskStatusArray array,
               format the raw data as human readable ASCII data. */
            printf("%-18s\tCPri\tBPri\t%-10s\tPct\tHigh Water\n", "Name", "Ctr");
            for( size_t x = 0; x < uxArraySize; x++ ) {

                int percentage = (ulTotalRunTime > 0) ? (pxTaskStatusArray[x].ulRunTimeCounter/ulTotalRunTime) : 0;

                printf("%-18s\t%2d\t%2d\t0x%08lx\t%2d%%\t0x%04lx\n",
                       pxTaskStatusArray[x].pcTaskName,
                       pxTaskStatusArray[x].uxCurrentPriority,
                       pxTaskStatusArray[x].uxBasePriority,
                       pxTaskStatusArray[x].ulRunTimeCounter,
                       percentage,
                       pxTaskStatusArray[x].usStackHighWaterMark);

            }
            printf("\n");

        }

        /* The array is no longer needed, free the memory it consumes. */
        vPortFree( pxTaskStatusArray );

        show_heap_info();
    }
    else {
        // unable to allocate memory
        printf("Unable to allocate task status memory\n");
    }

    return ESP_OK;
}


#ifdef CONFIG_HEAP_TRACING

const size_t heap_trace_count = 40;
heap_trace_record_t heap_trace_record[heap_trace_count];


int
cmd_mem(int argc, char ** argv)
{
    static bool initialized = false;

    if (argc == 2) {
        if (strcmp(argv[1],"init")==0) {
            if (!initialized) {
                heap_trace_init_standalone(heap_trace_record, heap_trace_count);
                initialized = true;
            }
        }
        else if (initialized) {
            if (strcmp(argv[1],"dump")==0) {
                heap_trace_dump();
            }
            else if (strcmp(argv[1],"start")==0) {
                heap_trace_start(HEAP_TRACE_LEAKS);
            }
            else if (strcmp(argv[1],"stop")==0) {
                heap_trace_stop();
            }
        }
        else {
            printf("must run 'mem init' first\n");
        }
    }
    else {
        printf("usage:\n  mem init   *run this first*\n"
               "  mem start\n"
               "  mem stop\n"
               "  mem dump\n");
    }
    return ESP_OK;
}

#endif


void
register_task_cmd()
{
    esp_console_cmd_t cmd = {
        .command = "tasks",
        .help = "Show tasks",
        .hint = NULL,
        .func = &cmd_tasks,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );

#ifdef CONFIG_HEAP_TRACING
    cmd = {
        .command = "mem",
        .help = "Memory debug",
        .hint = NULL,
        .func = &cmd_mem,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
#endif

}



////////////////////////////////////////////////////////////////////////////////


static int
cmd_event(int argc, char **argv)
{
#if defined(CONFIG_ESP_EVENT_LOOP_PROFILING)
    if (argc!=1) {
        printf("usage: event\n");
        return ESP_FAIL;
    }

    ESP_LOGI(eventTag, "event loop debug dump");

    ESP_ERROR_CHECK( esp_event_dump(stdout) );
#else
    ESP_LOGE(eventTag, "ESP_EVENT_LOOP_PROFILING not set");
#endif

    return ESP_OK;
}


void
register_event_cmd()
{
    esp_console_cmd_t cmd = {
        .command = "event",
        .help = "Print event loop debug",
        .hint = NULL,
        .func = &cmd_event,
        .argtable = NULL
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}
