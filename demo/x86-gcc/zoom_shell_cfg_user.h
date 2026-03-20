/**
 * @file zoom_shell_cfg_user.h
 * @brief x86-gcc demo 的配置覆盖 — 功能全开
 */

#ifndef ZOOM_SHELL_CFG_USER_H
#define ZOOM_SHELL_CFG_USER_H

/* 核心功能 */
#define ZOOM_USING_CMD_EXPORT       1
#define ZOOM_USING_HISTORY          1
#define ZOOM_USING_USER             1
#define ZOOM_USING_VAR              1
#define ZOOM_USING_PERF             1
#define ZOOM_USING_ANSI             1
#define ZOOM_USING_AUDIT_LOG        0
#define ZOOM_USING_LOCK             0
#define ZOOM_USING_PLUGIN           0

/* 扩展功能 */
#define ZOOM_USING_HEXDUMP          1
#define ZOOM_USING_LOG              1
#define ZOOM_USING_CALC             1
#define ZOOM_USING_PASSTHROUGH      1
#define ZOOM_USING_REPEAT           1
#define ZOOM_USING_KEYBIND          1
#define ZOOM_USING_ALIAS            1
#define ZOOM_USING_SCRIPT           1
#define ZOOM_USING_PROGRESS         1
#define ZOOM_USING_GAME             1

/* 容量 */
#define ZOOM_CMD_MAX_LENGTH         256
#define ZOOM_CMD_MAX_ARGS           16
#define ZOOM_HISTORY_MAX_NUMBER     10
#define ZOOM_MAX_USERS              8
#define ZOOM_PROMPT_MAX_LENGTH      32
#define ZOOM_PRINT_BUFFER           256

/* 平台 tick — 用 clock() 模拟毫秒 */
#include <stdint.h>
#include <time.h>
static inline uint32_t zoom_user_get_tick(void)
{
    return (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC);
}
#define ZOOM_GET_TICK()     zoom_user_get_tick()

#endif /* ZOOM_SHELL_CFG_USER_H */
