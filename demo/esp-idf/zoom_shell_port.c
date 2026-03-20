/**
 * @file zoom_shell_port.c
 * @brief ESP-IDF 移植层模板
 *
 * 使用 ESP-IDF 的 UART 驱动 API。
 * 在 app_main 中创建任务运行 shell。
 */

#include "zoom_shell.h"

/* TODO: 取消注释 */
/* #include "driver/uart.h" */
/* #include "esp_log.h" */

/* #define SHELL_UART_NUM  UART_NUM_0 */

static zoom_shell_t g_shell;
static char s_shell_buffer[2816];  /* 256 * (10+1) */

static int16_t port_read(char *data, uint16_t len)
{
    (void)len;
    /*
     * TODO:
     * int ret = uart_read_bytes(SHELL_UART_NUM, (uint8_t *)data, 1, portMAX_DELAY);
     * return (ret > 0) ? 1 : 0;
     */
    (void)data;
    return 0;
}

static int16_t port_write(const char *data, uint16_t len)
{
    /*
     * TODO:
     * uart_write_bytes(SHELL_UART_NUM, data, len);
     */
    (void)data;
    return (int16_t)len;
}

void zoom_shell_port_init(void)
{
    /*
     * TODO: 初始化 UART
     * uart_config_t cfg = {
     *     .baud_rate = 115200,
     *     .data_bits = UART_DATA_8_BITS,
     *     .parity    = UART_PARITY_DISABLE,
     *     .stop_bits = UART_STOP_BITS_1,
     * };
     * uart_param_config(SHELL_UART_NUM, &cfg);
     * uart_driver_install(SHELL_UART_NUM, 256, 0, 0, NULL, 0);
     */

    memset(&g_shell, 0, sizeof(g_shell));
    g_shell.read  = port_read;
    g_shell.write = port_write;
    zoom_shell_init(&g_shell, s_shell_buffer, sizeof(s_shell_buffer));
}

/*
 * 任务入口:
 * void shell_task(void *param) {
 *     zoom_shell_port_init();
 *     zoom_shell_run(&g_shell);
 *     vTaskDelete(NULL);
 * }
 *
 * app_main:
 *     xTaskCreate(shell_task, "shell", 4096, NULL, 5, NULL);
 */
