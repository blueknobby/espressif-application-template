#include <CommandLine.h>

#include "esp_log.h"
#include "esp_console.h"

#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"

#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
#include "esp_vfs_usb_serial_jtag.h"
#endif

#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"

#include "bkt_priority.h"


CommandLineHandler commandLine("cmdline", /* priority= */ PRI_CMDLINE );


CommandLineHandler::CommandLineHandler(std::string name, UBaseType_t priority)
    : Thread(name, CONFIG_BKT_COMMAND_STACK_SIZE, priority, /*CPU*/ 1)
{
}


void
CommandLineHandler::begin()
{
#if 1
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin and stdout */
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_vfs_dev_usb_serial_jtag_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_usb_serial_jtag_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
    usb_serial_jtag_driver_config_t cfg=USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    usb_serial_jtag_driver_install(&cfg);

    esp_vfs_usb_serial_jtag_use_driver();
#elif CONFIG_ESP_CONSOLE_UART
    //const uart_port_t uart = CONFIG_ESP_CONSOLE_UART_NUM;




    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
            .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0,
#if SOC_UART_SUPPORT_REF_TICK
            .source_clk = UART_SCLK_REF_TICK,
#elif SOC_UART_SUPPORT_XTAL_CLK
            .source_clk = UART_SCLK_XTAL,
#endif
    };
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM,
            256, 0, 0, NULL, 0) );
    ESP_ERROR_CHECK( uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
#else
    #error no supported console type is chosen
#endif

    /* Initialize the console */
    esp_console_config_t console_config;
    memset(&console_config, 0, sizeof(console_config));
    console_config.max_cmdline_args = 8;
    console_config.max_cmdline_length = 256;
#if CONFIG_LOG_COLORS
    console_config.hint_color = atoi(LOG_COLOR_CYAN);
#endif
    ESP_ERROR_CHECK( esp_console_init(&console_config) );


    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

    esp_console_register_help_command();
#endif // 0

    Start(); // get the thread working....
}


void
CommandLineHandler::Run()
{
#if 1
    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    char* prompt = static_cast<char*>(calloc(40,1));

    std::string name = "bkt1";     // hw.getNetworkName();

    snprintf(prompt, 40, "%s%s%s>", LOG_COLOR_I, name.c_str(), LOG_RESET_COLOR);

    printf("\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n");

    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status) { /* zero indicates success */
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
    }

    /* Main loop */
    while(true) {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char* line = linenoise(prompt);
        if (line == NULL) { /* Ignore empty lines */
            continue;
        }
        /* Add the command to the history */
        linenoiseHistoryAdd(line);

        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }
#endif
    while(true) {
        vTaskDelay(1000);
    }
    printf("End of command line loop\n");
}
