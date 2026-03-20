/**
 * @file zoom_shell_cfg_test.h
 * @brief 测试专用配置 — 功能全开，容量适中
 */

#ifndef ZOOM_SHELL_CFG_TEST_H
#define ZOOM_SHELL_CFG_TEST_H

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

#define ZOOM_CMD_MAX_LENGTH         128
#define ZOOM_CMD_MAX_ARGS           8
#define ZOOM_HISTORY_MAX_NUMBER     5
#define ZOOM_MAX_USERS              4
#define ZOOM_PROMPT_MAX_LENGTH      16
#define ZOOM_PRINT_BUFFER           256
#define ZOOM_SHELL_MAX_NUMBER       2

#include <stdint.h>
static inline uint32_t test_get_tick(void) { return 0; }
#define ZOOM_GET_TICK()     test_get_tick()

#endif
