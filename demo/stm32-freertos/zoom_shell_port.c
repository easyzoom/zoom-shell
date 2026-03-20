/**
 * @file zoom_shell_port.c
 * @brief STM32 + FreeRTOS 移植层模板
 *
 * 使用说明:
 *   1. 将此文件和 zoom_shell_cfg_user.h 加入工程
 *   2. 修改 UART 句柄为实际使用的串口
 *   3. 在 FreeRTOS 任务中调用 zoom_shell_port_init() + zoom_shell_run()
 *   4. 编译时传入: -DZOOM_SHELL_CFG_USER='"zoom_shell_cfg_user.h"'
 */

#include "zoom_shell.h"

/* TODO: 替换为实际的 HAL 头文件 */
/* #include "stm32f4xx_hal.h" */
/* #include "FreeRTOS.h" */
/* #include "task.h" */
/* #include "semphr.h" */

/* TODO: 替换为实际串口句柄 */
/* extern UART_HandleTypeDef huart1; */

static zoom_shell_t g_shell;
static char s_shell_buffer[768];  /* 128 * (5+1) */

/* ---- I/O 回调 ---- */

static int16_t port_read(char *data, uint16_t len)
{
    (void)len;
    /*
     * TODO: 实现串口阻塞读取一个字节
     *
     * 方式1: HAL 阻塞
     *   if (HAL_UART_Receive(&huart1, (uint8_t *)data, 1, HAL_MAX_DELAY) == HAL_OK)
     *       return 1;
     *
     * 方式2: FreeRTOS 队列 (推荐)
     *   在 UART 中断中将字节放入队列，这里 xQueueReceive 阻塞等待
     *   extern QueueHandle_t g_uart_rx_queue;
     *   if (xQueueReceive(g_uart_rx_queue, data, portMAX_DELAY) == pdTRUE)
     *       return 1;
     */
    (void)data;
    return 0;
}

static int16_t port_write(const char *data, uint16_t len)
{
    /*
     * TODO: 实现串口发送
     *   HAL_UART_Transmit(&huart1, (uint8_t *)data, len, HAL_MAX_DELAY);
     */
    (void)data;
    return (int16_t)len;
}

#if ZOOM_USING_LOCK
static int port_lock(zoom_shell_t *shell)
{
    (void)shell;
    /* TODO: xSemaphoreTake(g_shell_mutex, portMAX_DELAY); */
    return 0;
}

static int port_unlock(zoom_shell_t *shell)
{
    (void)shell;
    /* TODO: xSemaphoreGive(g_shell_mutex); */
    return 0;
}
#endif

/* ---- 初始化 ---- */

void zoom_shell_port_init(void)
{
    memset(&g_shell, 0, sizeof(g_shell));
    g_shell.read  = port_read;
    g_shell.write = port_write;

#if ZOOM_USING_LOCK
    g_shell.lock   = port_lock;
    g_shell.unlock = port_unlock;
#endif

    zoom_shell_init(&g_shell, s_shell_buffer, sizeof(s_shell_buffer));
}

/*
 * FreeRTOS 任务入口:
 *
 * void shell_task(void *param)
 * {
 *     zoom_shell_port_init();
 *     zoom_shell_run(&g_shell);
 *     vTaskDelete(NULL);
 * }
 */
