/**
 * @file zoom_shell_port.c
 * @brief RT-Thread 移植层模板
 *
 * 使用 rt_device API 对接 UART。
 */

#include "zoom_shell.h"

/* TODO: 取消注释 */
/* #include <rtthread.h> */
/* #include <rtdevice.h> */

/* #define SHELL_UART_NAME  "uart1" */

/* static rt_device_t s_uart_dev = RT_NULL; */
static zoom_shell_t g_shell;
static char s_shell_buffer[768];

static int16_t port_read(char *data, uint16_t len)
{
    (void)len;
    /*
     * TODO:
     * if (s_uart_dev)
     *     return rt_device_read(s_uart_dev, -1, data, 1);
     */
    (void)data;
    return 0;
}

static int16_t port_write(const char *data, uint16_t len)
{
    /*
     * TODO:
     * if (s_uart_dev)
     *     rt_device_write(s_uart_dev, -1, data, len);
     */
    (void)data;
    return (int16_t)len;
}

void zoom_shell_port_init(void)
{
    /*
     * TODO:
     * s_uart_dev = rt_device_find(SHELL_UART_NAME);
     * if (s_uart_dev) {
     *     rt_device_open(s_uart_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
     * }
     */

    memset(&g_shell, 0, sizeof(g_shell));
    g_shell.read  = port_read;
    g_shell.write = port_write;
    zoom_shell_init(&g_shell, s_shell_buffer, sizeof(s_shell_buffer));
}

/*
 * 线程入口:
 * void shell_thread_entry(void *param) {
 *     zoom_shell_port_init();
 *     zoom_shell_run(&g_shell);
 * }
 *
 * 初始化:
 *     rt_thread_t tid = rt_thread_create("zsh", shell_thread_entry, RT_NULL, 2048, 20, 10);
 *     rt_thread_startup(tid);
 */
