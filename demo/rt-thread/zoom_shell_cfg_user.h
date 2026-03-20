/**
 * @file zoom_shell_cfg_user.h
 * @brief RT-Thread 推荐配置
 */

#ifndef ZOOM_SHELL_CFG_USER_H
#define ZOOM_SHELL_CFG_USER_H

#define ZOOM_USING_CMD_EXPORT       1
#define ZOOM_USING_HISTORY          1
#define ZOOM_USING_USER             1
#define ZOOM_USING_VAR              1
#define ZOOM_USING_PERF             1
#define ZOOM_USING_ANSI             1
#define ZOOM_USING_HEXDUMP          1
#define ZOOM_USING_CALC             1
#define ZOOM_USING_AUDIT_LOG        0
#define ZOOM_USING_LOCK             0
#define ZOOM_USING_PLUGIN           0

#define ZOOM_CMD_MAX_LENGTH         128
#define ZOOM_CMD_MAX_ARGS           8
#define ZOOM_HISTORY_MAX_NUMBER     5
#define ZOOM_MAX_USERS              4
#define ZOOM_PROMPT_MAX_LENGTH      16
#define ZOOM_PRINT_BUFFER           128

#include <stdint.h>

extern uint32_t rt_tick_get(void);
#define ZOOM_GET_TICK()     rt_tick_get()

#endif
