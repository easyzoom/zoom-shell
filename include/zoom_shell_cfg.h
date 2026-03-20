/**
 * @file zoom_shell_cfg.h
 * @brief Zoom Shell 默认配置
 *
 * 所有配置项均以 #ifndef 包裹，用户可通过定义 ZOOM_SHELL_CFG_USER 宏
 * 指向自己的头文件来覆盖任意默认值。
 *
 * 编译时传入: -DZOOM_SHELL_CFG_USER='"zoom_shell_cfg_user.h"'
 */

#ifndef ZOOM_SHELL_CFG_H
#define ZOOM_SHELL_CFG_H

#ifdef ZOOM_SHELL_CFG_USER
#include ZOOM_SHELL_CFG_USER
#endif

/* ======================== 功能开关 ======================== */

/** 命令注册方式: 1=链接器段导出, 0=静态数组 */
#ifndef ZOOM_USING_CMD_EXPORT
#define ZOOM_USING_CMD_EXPORT           1
#endif

/** 命令历史记录 */
#ifndef ZOOM_USING_HISTORY
#define ZOOM_USING_HISTORY              1
#endif

/** 用户/权限系统 */
#ifndef ZOOM_USING_USER
#define ZOOM_USING_USER                 1
#endif

/** 变量观测系统 */
#ifndef ZOOM_USING_VAR
#define ZOOM_USING_VAR                  1
#endif

/** 静态插件系统 */
#ifndef ZOOM_USING_PLUGIN
#define ZOOM_USING_PLUGIN               0
#endif

/** 互斥锁支持 */
#ifndef ZOOM_USING_LOCK
#define ZOOM_USING_LOCK                 0
#endif

/** 命令执行计时 */
#ifndef ZOOM_USING_PERF
#define ZOOM_USING_PERF                 1
#endif

/** ANSI转义码支持 (Tab补全/方向键/颜色输出) */
#ifndef ZOOM_USING_ANSI
#define ZOOM_USING_ANSI                 1
#endif

/** 审计日志 */
#ifndef ZOOM_USING_AUDIT_LOG
#define ZOOM_USING_AUDIT_LOG            0
#endif

/** 内存查看/写入 (hexdump) */
#ifndef ZOOM_USING_HEXDUMP
#define ZOOM_USING_HEXDUMP              0
#endif

/** 日志子系统 */
#ifndef ZOOM_USING_LOG
#define ZOOM_USING_LOG                  0
#endif

/** 表达式求值器 */
#ifndef ZOOM_USING_CALC
#define ZOOM_USING_CALC                 0
#endif

/** 透传模式 */
#ifndef ZOOM_USING_PASSTHROUGH
#define ZOOM_USING_PASSTHROUGH          0
#endif

/** 周期执行 */
#ifndef ZOOM_USING_REPEAT
#define ZOOM_USING_REPEAT               0
#endif

/** 按键绑定 */
#ifndef ZOOM_USING_KEYBIND
#define ZOOM_USING_KEYBIND              0
#endif

/** 命令别名 */
#ifndef ZOOM_USING_ALIAS
#define ZOOM_USING_ALIAS                0
#endif

/** 批量脚本 */
#ifndef ZOOM_USING_SCRIPT
#define ZOOM_USING_SCRIPT               0
#endif

/** 进度条 API */
#ifndef ZOOM_USING_PROGRESS
#define ZOOM_USING_PROGRESS             0
#endif

/** 命令行游戏 */
#ifndef ZOOM_USING_GAME
#define ZOOM_USING_GAME                 0
#endif

/* ======================== 容量参数 ======================== */

/** 命令行最大长度 */
#ifndef ZOOM_CMD_MAX_LENGTH
#define ZOOM_CMD_MAX_LENGTH             128
#endif

/** 命令最大参数个数 */
#ifndef ZOOM_CMD_MAX_ARGS
#define ZOOM_CMD_MAX_ARGS               8
#endif

/** 历史记录条数 */
#ifndef ZOOM_HISTORY_MAX_NUMBER
#define ZOOM_HISTORY_MAX_NUMBER         5
#endif

/** 最大用户数 */
#ifndef ZOOM_MAX_USERS
#define ZOOM_MAX_USERS                  4
#endif

/** 最大插件数 */
#ifndef ZOOM_MAX_PLUGINS
#define ZOOM_MAX_PLUGINS                4
#endif

/** 最大别名数 */
#ifndef ZOOM_ALIAS_MAX
#define ZOOM_ALIAS_MAX                  8
#endif

/** 脚本最大行数 */
#ifndef ZOOM_SCRIPT_MAX_LINES
#define ZOOM_SCRIPT_MAX_LINES           16
#endif

/** 最大脚本数 */
#ifndef ZOOM_SCRIPT_MAX
#define ZOOM_SCRIPT_MAX                 4
#endif

/** 最大按键绑定数 */
#ifndef ZOOM_KEYBIND_MAX
#define ZOOM_KEYBIND_MAX                8
#endif

/** 日志缓冲区 */
#ifndef ZOOM_LOG_BUFFER_SIZE
#define ZOOM_LOG_BUFFER_SIZE            128
#endif

/** 提示符最大长度 */
#ifndef ZOOM_PROMPT_MAX_LENGTH
#define ZOOM_PROMPT_MAX_LENGTH          16
#endif

/** 格式化输出缓冲区大小 */
#ifndef ZOOM_PRINT_BUFFER
#define ZOOM_PRINT_BUFFER               128
#endif

/** 最大 shell 实例数 */
#ifndef ZOOM_SHELL_MAX_NUMBER
#define ZOOM_SHELL_MAX_NUMBER           1
#endif

/** 用户名最大长度 */
#ifndef ZOOM_USER_NAME_MAX
#define ZOOM_USER_NAME_MAX              16
#endif

/** 密码最大长度 */
#ifndef ZOOM_USER_PASSWORD_MAX
#define ZOOM_USER_PASSWORD_MAX          16
#endif

/* ======================== 平台适配 ======================== */

/** 获取系统毫秒 tick，用户需实现此宏 */
#ifndef ZOOM_GET_TICK
#define ZOOM_GET_TICK()                 0
#endif

/** 链接器段属性 — 自动适配 ARM CC / IAR / GCC */
#ifndef ZOOM_SECTION
    #if defined(__CC_ARM) || defined(__CLANG_ARM)
        #define ZOOM_SECTION(x)         __attribute__((section(x), aligned(4)))
    #elif defined(__IAR_SYSTEMS_ICC__)
        #define ZOOM_SECTION(x)         @ x
    #elif defined(__GNUC__)
        #define ZOOM_SECTION(x)         __attribute__((section(x), aligned(4)))
    #else
        #define ZOOM_SECTION(x)
    #endif
#endif

/** 防止链接器优化掉未引用符号 */
#ifndef ZOOM_USED
    #if defined(__CC_ARM) || defined(__CLANG_ARM)
        #define ZOOM_USED               __attribute__((used))
    #elif defined(__IAR_SYSTEMS_ICC__)
        #define ZOOM_USED               __root
    #elif defined(__GNUC__)
        #define ZOOM_USED               __attribute__((used))
    #else
        #define ZOOM_USED
    #endif
#endif

#endif /* ZOOM_SHELL_CFG_H */
