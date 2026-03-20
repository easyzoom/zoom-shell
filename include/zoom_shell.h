/**
 * @file zoom_shell.h
 * @brief Zoom Shell — 嵌入式增强型 Shell
 * @version 1.0.0
 *
 * 三大特色:
 *   1. 层级子命令树（借鉴 Zephyr Shell，API 更简洁）
 *   2. 变量观测系统（借鉴 USMART，大幅增强）
 *   3. 多级权限体系 + 命令属性
 *
 * 设计原则:
 *   - 零 malloc：所有内存静态分配或由用户提供
 *   - 零 OS 依赖：仅依赖 <stdint.h> <stdbool.h> <stddef.h> <string.h>
 *   - I/O 纯回调：read/write 可对接 UART/USB/SPI 等
 *   - 条件编译裁剪：每个功能模块可独立开关
 */

#ifndef ZOOM_SHELL_H
#define ZOOM_SHELL_H

#include "zoom_shell_cfg.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 *  版本信息
 * ================================================================ */
#define ZOOM_SHELL_VERSION_MAJOR    1
#define ZOOM_SHELL_VERSION_MINOR    0
#define ZOOM_SHELL_VERSION_PATCH    0
#define ZOOM_SHELL_VERSION_STRING   "1.0.0"

/* ================================================================
 *  前向声明
 * ================================================================ */
typedef struct zoom_shell zoom_shell_t;
typedef struct zoom_cmd   zoom_cmd_t;

/* ================================================================
 *  枚举
 * ================================================================ */

/** 命令属性位掩码 */
typedef enum {
    ZOOM_ATTR_DEFAULT    = 0,
    ZOOM_ATTR_HIDDEN     = (1 << 0),
    ZOOM_ATTR_DANGEROUS  = (1 << 1),
    ZOOM_ATTR_DEPRECATED = (1 << 2),
    ZOOM_ATTR_ADMIN      = (1 << 3),
    ZOOM_ATTR_NO_HISTORY = (1 << 4),
} zoom_cmd_attr_t;

/** 用户权限等级 */
typedef enum {
    ZOOM_USER_GUEST = 0,
    ZOOM_USER_USER  = 1,
    ZOOM_USER_ADMIN = 2,
    ZOOM_USER_ROOT  = 3,
} zoom_user_level_t;

/** Shell 状态 */
typedef enum {
    ZOOM_STATE_IDLE = 0,
    ZOOM_STATE_RUNNING,
    ZOOM_STATE_ERROR,
    ZOOM_STATE_SUSPENDED,
} zoom_shell_state_t;

#if ZOOM_USING_VAR
/** 变量类型 */
typedef enum {
    ZOOM_VAR_INT8 = 0,
    ZOOM_VAR_INT16,
    ZOOM_VAR_INT32,
    ZOOM_VAR_UINT8,
    ZOOM_VAR_UINT16,
    ZOOM_VAR_UINT32,
    ZOOM_VAR_FLOAT,
    ZOOM_VAR_BOOL,
    ZOOM_VAR_STRING,
} zoom_var_type_t;

/** 变量读写属性 */
#define ZOOM_VAR_RO     0
#define ZOOM_VAR_RW     1
#endif /* ZOOM_USING_VAR */

/* ================================================================
 *  函数指针类型
 * ================================================================ */

/** 命令处理函数 — 统一传入 shell 实例，避免全局变量 */
typedef int (*zoom_cmd_func_t)(zoom_shell_t *shell, int argc, char *argv[]);

/** I/O 回调 */
typedef int16_t (*zoom_read_t)(char *data, uint16_t len);
typedef int16_t (*zoom_write_t)(const char *data, uint16_t len);

/* ================================================================
 *  数据结构
 * ================================================================ */

/** 命令 — 支持子命令树 */
struct zoom_cmd {
    const char         *name;
    zoom_cmd_func_t     func;           /**< NULL 表示有子命令 */
    const char         *desc;
    const zoom_cmd_t   *subcmds;        /**< 子命令数组，NULL=叶节点 */
    uint16_t            subcmd_count;
    uint8_t             attr;           /**< zoom_cmd_attr_t 位掩码 */
    uint8_t             min_level;      /**< zoom_user_level_t */
};

#if ZOOM_USING_USER
/** 用户 */
typedef struct {
    char                name[ZOOM_USER_NAME_MAX];
    char                password[ZOOM_USER_PASSWORD_MAX];
    uint8_t             level;          /**< zoom_user_level_t */
    uint8_t             _reserved[3];   /**< 对齐到 4 字节边界 */
} zoom_user_t;
#endif

#if ZOOM_USING_VAR
/** 变量观测条目 */
typedef struct {
    const char *name;
    const char *desc;
    void       *addr;
    uint8_t     type;       /**< zoom_var_type_t */
    uint8_t     attr;       /**< ZOOM_VAR_RO / ZOOM_VAR_RW */
    uint8_t     min_level;  /**< zoom_user_level_t */
} zoom_var_t;
#endif

#if ZOOM_USING_PLUGIN
/** 静态插件 */
typedef struct {
    const char *name;
    const char *version;
    const char *description;
    int  (*init)(zoom_shell_t *shell);
    void (*deinit)(zoom_shell_t *shell);
} zoom_plugin_t;
#endif

/** 统计信息 */
typedef struct {
    uint32_t total_cmds_executed;
    uint32_t uptime_tick;
#if ZOOM_USING_PERF
    uint8_t  perf_enabled;
#endif
} zoom_stats_t;

/** Shell 主结构 — 全静态 */
struct zoom_shell {
    zoom_shell_state_t state;

    /* ---------- 输入解析器 ---------- */
    struct {
        char    *buffer;
        uint16_t bufferSize;
        uint16_t length;
        uint16_t cursor;
        char    *param[ZOOM_CMD_MAX_ARGS];
        uint16_t paramCount;
        int      keyValue;
    } parser;

    /* ---------- 命令表 ---------- */
    struct {
        const zoom_cmd_t *base;
        uint16_t count;
    } cmdList;

#if ZOOM_USING_VAR
    /* ---------- 变量表 ---------- */
    struct {
        const zoom_var_t *base;
        uint16_t count;
    } varList;
#endif

#if ZOOM_USING_HISTORY
    /* ---------- 历史记录 ---------- */
    struct {
        char    *item[ZOOM_HISTORY_MAX_NUMBER];
        uint16_t number;
        uint16_t record;
        int16_t  offset;
    } history;
#endif

#if ZOOM_USING_USER
    /* ---------- 用户系统 ---------- */
    zoom_user_t users[ZOOM_MAX_USERS];
    uint16_t    userCount;
    struct {
        int16_t  currentIdx;    /**< users[] 中的索引, -1=未登录 */
        uint8_t  isChecked : 1;
    } auth;
#endif

#if ZOOM_USING_PLUGIN
    /* ---------- 插件表 ---------- */
    struct {
        const zoom_plugin_t *base;
        uint16_t count;
    } pluginList;
#endif

    /* ---------- 统计 ---------- */
    zoom_stats_t stats;

    /* ---------- 提示符 ---------- */
    char prompt[ZOOM_PROMPT_MAX_LENGTH];

    /* ---------- I/O 回调 ---------- */
    zoom_read_t  read;
    zoom_write_t write;

#if ZOOM_USING_LOCK
    int (*lock)(zoom_shell_t *);
    int (*unlock)(zoom_shell_t *);
#endif

    /* ---------- 状态标志 ---------- */
    struct {
        uint8_t isActive  : 1;
        uint8_t tabFlag   : 1;
#if ZOOM_USING_ANSI
        uint8_t escState  : 2;   /**< ANSI 转义序列解析状态 */
#endif
    } status;

    /* ---------- 透传模式 ---------- */
#if ZOOM_USING_PASSTHROUGH
    struct {
        int (*handler)(zoom_shell_t *shell, const char *data, uint16_t len);
        char prompt[16];
        uint8_t active;
    } passthrough;
#endif

    /* ---------- 周期执行 ---------- */
#if ZOOM_USING_REPEAT
    struct {
        char     cmd[ZOOM_CMD_MAX_LENGTH];
        uint32_t interval_ms;
        uint32_t last_tick;
        uint8_t  active;
    } repeat;
#endif

    /* ---------- 命令别名 ---------- */
#if ZOOM_USING_ALIAS
    struct {
        struct { char name[16]; char value[ZOOM_CMD_MAX_LENGTH]; } entries[ZOOM_ALIAS_MAX];
        uint8_t count;
    } alias;
#endif

    /* ---------- 批量脚本 ---------- */
#if ZOOM_USING_SCRIPT
    struct {
        struct {
            char name[16];
            char lines[ZOOM_SCRIPT_MAX_LINES][ZOOM_CMD_MAX_LENGTH];
            uint8_t line_count;
        } slots[ZOOM_SCRIPT_MAX];
        uint8_t count;
        int8_t  recording;
    } script;
#endif

    /* ---------- 按键绑定 ---------- */
#if ZOOM_USING_KEYBIND
    struct {
        uint8_t key;
        void (*handler)(zoom_shell_t *shell);
        const char *desc;
    } keybinds[ZOOM_KEYBIND_MAX];
    uint8_t keybindCount;
#endif

    /* ---------- 用户扩展 ---------- */
    void *userData;
};

/* ================================================================
 *  链接器段符号（ZOOM_USING_CMD_EXPORT == 1 时由链接脚本提供）
 * ================================================================ */
#if ZOOM_USING_CMD_EXPORT
extern const zoom_cmd_t _zoom_cmd_start;
extern const zoom_cmd_t _zoom_cmd_end;
#if ZOOM_USING_VAR
extern const zoom_var_t _zoom_var_start;
extern const zoom_var_t _zoom_var_end;
#endif
#if ZOOM_USING_PLUGIN
extern const zoom_plugin_t _zoom_plugin_start;
extern const zoom_plugin_t _zoom_plugin_end;
#endif
#if ZOOM_USING_USER
extern const zoom_user_t _zoom_user_start;
extern const zoom_user_t _zoom_user_end;
#endif
#endif /* ZOOM_USING_CMD_EXPORT */

/* ================================================================
 *  静态数组模式的外部符号（ZOOM_USING_CMD_EXPORT == 0）
 * ================================================================ */
#if !ZOOM_USING_CMD_EXPORT
extern const zoom_cmd_t   zoom_cmd_list[];
extern const uint16_t     zoom_cmd_count;
#if ZOOM_USING_VAR
extern const zoom_var_t   zoom_var_list[];
extern const uint16_t     zoom_var_count;
#endif
#endif

/* ================================================================
 *  导出宏 — 命令
 * ================================================================ */

#if ZOOM_USING_CMD_EXPORT

/**
 * 导出根命令（链接器段模式）
 * @param _name  命令名（C 标识符）
 * @param _func  处理函数，NULL 表示此命令仅作为子命令容器
 * @param _desc  描述字符串
 * @param _attr  属性位掩码 (ZOOM_ATTR_xxx)
 * @param _level 最低权限 (ZOOM_USER_xxx)
 */
#define ZOOM_EXPORT_CMD(_name, _func, _desc, _attr, _level) \
    ZOOM_USED const zoom_cmd_t _zoom_cmd_##_name             \
    ZOOM_SECTION("zoomCommand") = {                           \
        .name = #_name,                                       \
        .func = (_func),                                      \
        .desc = (_desc),                                      \
        .subcmds = NULL,                                      \
        .subcmd_count = 0,                                    \
        .attr = (_attr),                                      \
        .min_level = (_level),                                \
    }

/** 导出带子命令的根命令 */
#define ZOOM_EXPORT_CMD_WITH_SUB(_name, _set, _desc, _attr, _level) \
    ZOOM_USED const zoom_cmd_t _zoom_cmd_##_name                     \
    ZOOM_SECTION("zoomCommand") = {                                   \
        .name = #_name,                                               \
        .func = NULL,                                                 \
        .desc = (_desc),                                              \
        .subcmds = (_set),                                            \
        .subcmd_count = sizeof(_set) / sizeof((_set)[0]),             \
        .attr = (_attr),                                              \
        .min_level = (_level),                                        \
    }

/** 条件编译导出命令 */
#define ZOOM_COND_CMD(_flag, _name, _func, _desc, _attr, _level)     \
    ZOOM_USED const zoom_cmd_t _zoom_cmd_##_name                      \
    ZOOM_SECTION("zoomCommand") = {                                    \
        .name = (_flag) ? #_name : "",                                 \
        .func = (_flag) ? (_func) : NULL,                              \
        .desc = (_flag) ? (_desc) : "",                                \
        .subcmds = NULL,                                               \
        .subcmd_count = 0,                                             \
        .attr = (_attr),                                               \
        .min_level = (_level),                                         \
    }

#else /* 静态数组模式 */

#define ZOOM_CMD_ITEM(_name, _func, _desc, _attr, _level) \
    { .name = #_name, .func = (_func), .desc = (_desc),   \
      .subcmds = NULL, .subcmd_count = 0,                  \
      .attr = (_attr), .min_level = (_level) }

#define ZOOM_CMD_ITEM_WITH_SUB(_name, _set, _desc, _attr, _level) \
    { .name = #_name, .func = NULL, .desc = (_desc),               \
      .subcmds = (_set), .subcmd_count = sizeof(_set)/sizeof((_set)[0]), \
      .attr = (_attr), .min_level = (_level) }

#endif /* ZOOM_USING_CMD_EXPORT */

/* ================================================================
 *  导出宏 — 子命令集
 * ================================================================ */

/** 定义一组子命令 */
#define ZOOM_SUBCMD_SET(_name, ...)                         \
    static const zoom_cmd_t _name[] = { __VA_ARGS__ }

/** 子命令条目（叶节点） */
#define ZOOM_SUBCMD(_name, _func, _desc)                    \
    { .name = #_name, .func = (_func), .desc = (_desc),    \
      .subcmds = NULL, .subcmd_count = 0,                   \
      .attr = ZOOM_ATTR_DEFAULT, .min_level = ZOOM_USER_GUEST }

/** 子命令条目（叶节点，带权限） */
#define ZOOM_SUBCMD_EX(_name, _func, _desc, _attr, _level) \
    { .name = #_name, .func = (_func), .desc = (_desc),    \
      .subcmds = NULL, .subcmd_count = 0,                   \
      .attr = (_attr), .min_level = (_level) }

/** 子命令条目（含下级子命令） */
#define ZOOM_SUBCMD_WITH_SUB(_name, _set, _desc)            \
    { .name = #_name, .func = NULL, .desc = (_desc),       \
      .subcmds = (_set),                                    \
      .subcmd_count = sizeof(_set) / sizeof((_set)[0]),     \
      .attr = ZOOM_ATTR_DEFAULT, .min_level = ZOOM_USER_GUEST }

/* ================================================================
 *  导出宏 — 变量
 * ================================================================ */
#if ZOOM_USING_VAR

#if ZOOM_USING_CMD_EXPORT
#define ZOOM_EXPORT_VAR(_name, _var, _type, _desc, _attr, _level) \
    ZOOM_USED const zoom_var_t _zoom_var_##_name                   \
    ZOOM_SECTION("zoomVar") = {                                     \
        .name = #_name,                                             \
        .desc = (_desc),                                            \
        .addr = (void *)&(_var),                                    \
        .type = (_type),                                            \
        .attr = (_attr),                                            \
        .min_level = (_level),                                      \
    }
#else
#define ZOOM_VAR_ITEM(_name, _var, _type, _desc, _attr, _level) \
    { .name = #_name, .desc = (_desc), .addr = (void *)&(_var), \
      .type = (_type), .attr = (_attr), .min_level = (_level) }
#endif

#endif /* ZOOM_USING_VAR */

/* ================================================================
 *  导出宏 — 用户
 * ================================================================ */
#if ZOOM_USING_USER && ZOOM_USING_CMD_EXPORT
#define ZOOM_EXPORT_USER(_name, _password, _level) \
    ZOOM_USED const zoom_user_t _zoom_user_##_name  \
    ZOOM_SECTION("zoomUser") = {                     \
        .name = #_name,                              \
        .password = _password,                       \
        .level = (_level),                           \
    }
#endif

/* ================================================================
 *  导出宏 — 插件
 * ================================================================ */
#if ZOOM_USING_PLUGIN && ZOOM_USING_CMD_EXPORT
#define ZOOM_EXPORT_PLUGIN(_name, _ver, _desc, _init, _deinit) \
    ZOOM_USED const zoom_plugin_t _zoom_plugin_##_name          \
    ZOOM_SECTION("zoomPlugin") = {                               \
        .name = #_name,                                          \
        .version = (_ver),                                       \
        .description = (_desc),                                  \
        .init = (_init),                                         \
        .deinit = (_deinit),                                     \
    }
#endif

/* ================================================================
 *  输出辅助宏 — 借鉴 Zephyr Shell
 * ================================================================ */

int zoom_shell_printf(zoom_shell_t *shell, const char *fmt, ...);

#if ZOOM_USING_ANSI
#define ZOOM_ANSI_RED       "\033[31m"
#define ZOOM_ANSI_GREEN     "\033[32m"
#define ZOOM_ANSI_YELLOW    "\033[33m"
#define ZOOM_ANSI_BLUE      "\033[34m"
#define ZOOM_ANSI_RESET     "\033[0m"

#define zoom_printf(sh, fmt, ...)  zoom_shell_printf((sh), fmt, ##__VA_ARGS__)
#define zoom_info(sh, fmt, ...)    zoom_shell_printf((sh), ZOOM_ANSI_BLUE   "[INFO] " fmt ZOOM_ANSI_RESET, ##__VA_ARGS__)
#define zoom_warn(sh, fmt, ...)    zoom_shell_printf((sh), ZOOM_ANSI_YELLOW "[WARN] " fmt ZOOM_ANSI_RESET, ##__VA_ARGS__)
#define zoom_error(sh, fmt, ...)   zoom_shell_printf((sh), ZOOM_ANSI_RED    "[ERR]  " fmt ZOOM_ANSI_RESET, ##__VA_ARGS__)
#define zoom_ok(sh, fmt, ...)      zoom_shell_printf((sh), ZOOM_ANSI_GREEN  "[OK]   " fmt ZOOM_ANSI_RESET, ##__VA_ARGS__)
#else
#define zoom_printf(sh, fmt, ...)  zoom_shell_printf((sh), fmt, ##__VA_ARGS__)
#define zoom_info(sh, fmt, ...)    zoom_shell_printf((sh), "[INFO] " fmt, ##__VA_ARGS__)
#define zoom_warn(sh, fmt, ...)    zoom_shell_printf((sh), "[WARN] " fmt, ##__VA_ARGS__)
#define zoom_error(sh, fmt, ...)   zoom_shell_printf((sh), "[ERR]  " fmt, ##__VA_ARGS__)
#define zoom_ok(sh, fmt, ...)      zoom_shell_printf((sh), "[OK]   " fmt, ##__VA_ARGS__)
#endif

/* ================================================================
 *  API
 * ================================================================ */

/**
 * @brief 初始化 shell
 * @param shell  shell 实例（由用户提供静态变量）
 * @param buffer 缓冲区（由用户提供，内部将分段给 parser 和 history）
 * @param size   缓冲区总大小
 * @return 0 成功, -1 失败
 *
 * 调用前需设置 shell->read 和 shell->write 回调。
 */
int zoom_shell_init(zoom_shell_t *shell, char *buffer, uint16_t size);

/**
 * @brief 阻塞运行主循环（适用于 RTOS 任务或 main 中 while）
 * @return 0 正常退出
 */
int zoom_shell_run(zoom_shell_t *shell);

/**
 * @brief 逐字节输入（适用于 UART 中断驱动模式）
 *
 * 每收到一个字节就调用一次。内部处理行编辑、ANSI 转义、Tab 补全等。
 * 当收到回车时自动执行命令。
 */
void zoom_shell_input(zoom_shell_t *shell, char data);

/**
 * @brief 程序化执行一条命令（不走输入解析器）
 * @param cmd_line 完整命令行字符串
 * @return 命令返回值, -1 表示错误
 */
int zoom_shell_exec(zoom_shell_t *shell, const char *cmd_line);

/** @brief 清理 shell */
void zoom_shell_deinit(zoom_shell_t *shell);

/* ---------- 用户系统 ---------- */
#if ZOOM_USING_USER
int zoom_shell_login(zoom_shell_t *shell, const char *username, const char *password);
int zoom_shell_logout(zoom_shell_t *shell);
int zoom_shell_add_user(zoom_shell_t *shell, const char *name, const char *password, uint8_t level);
uint8_t zoom_shell_get_current_level(zoom_shell_t *shell);
#endif

/* ---------- 变量系统 ---------- */
#if ZOOM_USING_VAR
const zoom_var_t *zoom_shell_find_var(zoom_shell_t *shell, const char *name);
int zoom_shell_var_get_str(const zoom_var_t *var, char *buf, uint16_t size);
int zoom_shell_var_set_str(const zoom_var_t *var, const char *value_str);
#endif

/* ---------- 内部辅助（供内置命令调用） ---------- */
void zoom_shell_print_welcome(zoom_shell_t *shell);
const zoom_cmd_t *zoom_shell_find_cmd(zoom_shell_t *shell, const char *name);
void zoom_shell_show_prompt(zoom_shell_t *shell);

/* ---------- 别名系统 ---------- */
#if ZOOM_USING_ALIAS
int zoom_shell_alias_add(zoom_shell_t *shell, const char *name, const char *value);
int zoom_shell_alias_remove(zoom_shell_t *shell, const char *name);
const char *zoom_shell_alias_lookup(zoom_shell_t *shell, const char *name);
#endif

/* ---------- 透传模式 ---------- */
#if ZOOM_USING_PASSTHROUGH
void zoom_shell_passthrough_set(zoom_shell_t *shell, const char *prompt,
    int (*handler)(zoom_shell_t *, const char *, uint16_t));
void zoom_shell_passthrough_exit(zoom_shell_t *shell);
#endif

/* ---------- 按键绑定 ---------- */
#if ZOOM_USING_KEYBIND
int zoom_shell_keybind_add(zoom_shell_t *shell, uint8_t key,
    void (*handler)(zoom_shell_t *), const char *desc);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZOOM_SHELL_H */
