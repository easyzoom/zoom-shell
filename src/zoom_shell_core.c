/**
 * @file zoom_shell_core.c
 * @brief Zoom Shell 核心实现
 *
 * 包含: init / deinit / run / input / exec / parse / history / TAB / ANSI
 */

#include "zoom_shell.h"
#include <string.h>
#include <stdarg.h>

/* ================================================================
 *  内部常量
 * ================================================================ */
#define ZOOM_KEY_BACKSPACE  0x08
#define ZOOM_KEY_TAB        0x09
#define ZOOM_KEY_LF         0x0A
#define ZOOM_KEY_CR         0x0D
#define ZOOM_KEY_ESC        0x1B
#define ZOOM_KEY_DEL        0x7F

#define ESC_STATE_NONE      0
#define ESC_STATE_ESC       1
#define ESC_STATE_BRACKET   2

/* ================================================================
 *  Shell 实例列表（多实例支持）
 * ================================================================ */
static zoom_shell_t *s_shell_list[ZOOM_SHELL_MAX_NUMBER];
static uint16_t s_shell_count = 0;

/* ================================================================
 *  内部工具函数
 * ================================================================ */

static void shell_write_str(zoom_shell_t *shell, const char *str)
{
    if (shell->write && str) {
        uint16_t len = 0;
        while (str[len]) len++;
        shell->write(str, len);
    }
}

static void shell_write_char(zoom_shell_t *shell, char c)
{
    if (shell->write) {
        shell->write(&c, 1);
    }
}

/** 简易整数转字符串 */
static int int_to_str(int32_t val, char *buf, uint16_t size)
{
    char tmp[12];
    int i = 0;
    int neg = 0;
    uint32_t uval;

    if (val < 0) {
        neg = 1;
        uval = (uint32_t)(-(val + 1)) + 1;
    } else {
        uval = (uint32_t)val;
    }

    do {
        tmp[i++] = '0' + (char)(uval % 10);
        uval /= 10;
    } while (uval && i < (int)sizeof(tmp));

    int total = neg + i;
    if (total >= size) return -1;

    int pos = 0;
    if (neg) buf[pos++] = '-';
    while (i > 0) buf[pos++] = tmp[--i];
    buf[pos] = '\0';
    return pos;
}

/** 简易无符号整数转字符串 */
static int uint_to_str(uint32_t val, char *buf, uint16_t size)
{
    char tmp[11];
    int i = 0;

    do {
        tmp[i++] = '0' + (char)(val % 10);
        val /= 10;
    } while (val && i < (int)sizeof(tmp));

    if (i >= size) return -1;

    int pos = 0;
    while (i > 0) buf[pos++] = tmp[--i];
    buf[pos] = '\0';
    return pos;
}

/** 简易 strcmp */
static int zoom_strcmp(const char *a, const char *b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

/** 简易 strncpy */
static void zoom_strncpy(char *dst, const char *src, uint16_t n)
{
    uint16_t i;
    for (i = 0; i < n - 1 && src[i]; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

/** 简易 strlen */
static uint16_t zoom_strlen(const char *s)
{
    uint16_t len = 0;
    while (s[len]) len++;
    return len;
}

/* ================================================================
 *  zoom_shell_printf — 格式化输出（栈上缓冲，无 malloc）
 *
 *  支持: %s %d %u %x %c %% %ld %lu %lx
 * ================================================================ */
/**
 * 将格式化内容写入 buf+pos，返回写入字节数。
 * 支持宽度和左对齐。
 */
static int pad_write(char *buf, int pos, int limit,
                     const char *content, int content_len,
                     int width, int left_align, char pad_char)
{
    int written = 0;
    int padding = (width > content_len) ? width - content_len : 0;

    if (!left_align) {
        for (int i = 0; i < padding && pos + written < limit; i++)
            buf[pos + written++] = pad_char;
    }
    for (int i = 0; i < content_len && pos + written < limit; i++)
        buf[pos + written++] = content[i];
    if (left_align) {
        for (int i = 0; i < padding && pos + written < limit; i++)
            buf[pos + written++] = ' ';
    }
    return written;
}

int zoom_shell_printf(zoom_shell_t *shell, const char *fmt, ...)
{
    if (!shell || !shell->write || !fmt) return -1;

    char buf[ZOOM_PRINT_BUFFER];
    int pos = 0;
    int limit = ZOOM_PRINT_BUFFER - 1;
    va_list ap;
    va_start(ap, fmt);

    while (*fmt && pos < limit) {
        if (*fmt != '%') {
            buf[pos++] = *fmt++;
            continue;
        }
        fmt++;  /* skip '%' */

        /* flags */
        int left_align = 0;
        char pad_char = ' ';
        while (*fmt == '-' || *fmt == '0') {
            if (*fmt == '-') left_align = 1;
            if (*fmt == '0' && !left_align) pad_char = '0';
            fmt++;
        }

        /* width */
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt++ - '0');
        }

        /* length modifier */
        int is_long = 0;
        if (*fmt == 'l') { is_long = 1; fmt++; }

        /* conversion */
        char tmp[16];
        int tmp_len = 0;

        switch (*fmt) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            tmp_len = (int)zoom_strlen(s);
            pos += pad_write(buf, pos, limit, s, tmp_len, width, left_align, ' ');
            break;
        }
        case 'd': {
            int32_t val = is_long ? (int32_t)va_arg(ap, long) : (int32_t)va_arg(ap, int);
            tmp_len = int_to_str(val, tmp, sizeof(tmp));
            if (tmp_len < 0) tmp_len = 0;
            pos += pad_write(buf, pos, limit, tmp, tmp_len, width, left_align, pad_char);
            break;
        }
        case 'u': {
            uint32_t val = is_long ? (uint32_t)va_arg(ap, unsigned long) : (uint32_t)va_arg(ap, unsigned int);
            tmp_len = uint_to_str(val, tmp, sizeof(tmp));
            if (tmp_len < 0) tmp_len = 0;
            pos += pad_write(buf, pos, limit, tmp, tmp_len, width, left_align, pad_char);
            break;
        }
        case 'x': {
            uint32_t val = is_long ? (uint32_t)va_arg(ap, unsigned long) : (uint32_t)va_arg(ap, unsigned int);
            int ti = 0;
            char hex_tmp[8];
            int hi = 0;
            if (val == 0) { tmp[ti++] = '0'; }
            else {
                while (val && hi < 8) {
                    uint8_t nib = val & 0xF;
                    hex_tmp[hi++] = nib < 10 ? ('0' + nib) : ('a' + nib - 10);
                    val >>= 4;
                }
                while (hi > 0) tmp[ti++] = hex_tmp[--hi];
            }
            tmp[ti] = '\0';
            tmp_len = ti;
            pos += pad_write(buf, pos, limit, tmp, tmp_len, width, left_align, pad_char);
            break;
        }
        case 'c': {
            char c = (char)va_arg(ap, int);
            buf[pos++] = c;
            break;
        }
        case '%':
            buf[pos++] = '%';
            break;
        default:
            buf[pos++] = '%';
            if (pos < limit) buf[pos++] = *fmt;
            break;
        }
        if (*fmt) fmt++;
    }

    va_end(ap);
    buf[pos] = '\0';

    shell->write(buf, (uint16_t)pos);
    return pos;
}

/* ================================================================
 *  初始化 / 反初始化
 * ================================================================ */

int zoom_shell_init(zoom_shell_t *shell, char *buffer, uint16_t size)
{
    if (!shell || !buffer || !shell->read || !shell->write) {
        return -1;
    }

    uint16_t min_size = ZOOM_CMD_MAX_LENGTH;
#if ZOOM_USING_HISTORY
    min_size = (uint16_t)(ZOOM_CMD_MAX_LENGTH * (ZOOM_HISTORY_MAX_NUMBER + 1));
#endif
    if (size < min_size) {
        return -1;
    }

    memset(&shell->parser, 0, sizeof(shell->parser));
    memset(&shell->stats, 0, sizeof(shell->stats));
    memset(&shell->status, 0, sizeof(shell->status));

    shell->state = ZOOM_STATE_IDLE;

    /* 分割缓冲区: [parser_buffer | history_0 | history_1 | ...] */
#if ZOOM_USING_HISTORY
    uint16_t seg = size / (ZOOM_HISTORY_MAX_NUMBER + 1);
    shell->parser.buffer = buffer;
    shell->parser.bufferSize = seg;
    shell->parser.length = 0;
    shell->parser.cursor = 0;

    shell->history.number = 0;
    shell->history.record = 0;
    shell->history.offset = 0;
    for (int i = 0; i < ZOOM_HISTORY_MAX_NUMBER; i++) {
        shell->history.item[i] = buffer + seg * (i + 1);
        shell->history.item[i][0] = '\0';
    }
#else
    shell->parser.buffer = buffer;
    shell->parser.bufferSize = size;
    shell->parser.length = 0;
    shell->parser.cursor = 0;
#endif

    /* 命令表 */
#if ZOOM_USING_CMD_EXPORT
    shell->cmdList.base = &_zoom_cmd_start;
    shell->cmdList.count = (uint16_t)(((size_t)&_zoom_cmd_end
                           - (size_t)&_zoom_cmd_start) / sizeof(zoom_cmd_t));
#else
    shell->cmdList.base = zoom_cmd_list;
    shell->cmdList.count = zoom_cmd_count;
#endif

    /* 变量表 */
#if ZOOM_USING_VAR
#if ZOOM_USING_CMD_EXPORT
    shell->varList.base = &_zoom_var_start;
    shell->varList.count = (uint16_t)(((size_t)&_zoom_var_end
                           - (size_t)&_zoom_var_start) / sizeof(zoom_var_t));
#else
    shell->varList.base = zoom_var_list;
    shell->varList.count = zoom_var_count;
#endif
#endif

    /* 插件表 */
#if ZOOM_USING_PLUGIN
#if ZOOM_USING_CMD_EXPORT
    shell->pluginList.base = &_zoom_plugin_start;
    shell->pluginList.count = (uint16_t)(((size_t)&_zoom_plugin_end
                              - (size_t)&_zoom_plugin_start) / sizeof(zoom_plugin_t));
#else
    shell->pluginList.base = NULL;
    shell->pluginList.count = 0;
#endif
#endif

    /* 用户系统 */
#if ZOOM_USING_USER
    memset(shell->users, 0, sizeof(shell->users));
    shell->userCount = 0;
    shell->auth.currentIdx = -1;
    shell->auth.isChecked = 0;

    /* 从链接器段加载预定义用户 */
#if ZOOM_USING_CMD_EXPORT
    {
        const zoom_user_t *u = &_zoom_user_start;
        const zoom_user_t *end = &_zoom_user_end;
        while (u < end && shell->userCount < ZOOM_MAX_USERS) {
            if (u->name[0]) {
                shell->users[shell->userCount] = *u;
                shell->userCount++;
            }
            u++;
        }
    }
#endif

    /* 如果没有预定义用户，添加默认管理员 */
    if (shell->userCount == 0) {
        zoom_shell_add_user(shell, "admin", "admin", ZOOM_USER_ADMIN);
    }

    /* 默认以 guest 身份运行 */
    shell->auth.currentIdx = -1;
    shell->auth.isChecked = 0;
#endif

    /* 提示符 */
    zoom_strncpy(shell->prompt, "zoom", ZOOM_PROMPT_MAX_LENGTH);

    /* ANSI 状态 */
#if ZOOM_USING_ANSI
    shell->status.escState = ESC_STATE_NONE;
#endif

    /* 注册到全局列表 */
    if (s_shell_count < ZOOM_SHELL_MAX_NUMBER) {
        s_shell_list[s_shell_count++] = shell;
    }

    /* 初始化插件 */
#if ZOOM_USING_PLUGIN
    for (uint16_t i = 0; i < shell->pluginList.count; i++) {
        if (shell->pluginList.base[i].init) {
            shell->pluginList.base[i].init(shell);
        }
    }
#endif

    /* 扩展模块初始化 */
#if ZOOM_USING_PASSTHROUGH
    shell->passthrough.active = 0;
    shell->passthrough.handler = NULL;
#endif

#if ZOOM_USING_REPEAT
    shell->repeat.active = 0;
#endif

#if ZOOM_USING_ALIAS
    shell->alias.count = 0;
#endif

#if ZOOM_USING_SCRIPT
    shell->script.count = 0;
    shell->script.recording = -1;
#endif

#if ZOOM_USING_KEYBIND
    shell->keybindCount = 0;
#endif

    shell->state = ZOOM_STATE_RUNNING;
    shell->status.isActive = 1;
    return 0;
}

void zoom_shell_deinit(zoom_shell_t *shell)
{
    if (!shell) return;

#if ZOOM_USING_PLUGIN
    for (uint16_t i = 0; i < shell->pluginList.count; i++) {
        if (shell->pluginList.base[i].deinit) {
            shell->pluginList.base[i].deinit(shell);
        }
    }
#endif

    shell->state = ZOOM_STATE_IDLE;
    shell->status.isActive = 0;

    for (uint16_t i = 0; i < s_shell_count; i++) {
        if (s_shell_list[i] == shell) {
            s_shell_list[i] = s_shell_list[--s_shell_count];
            break;
        }
    }
}

/* ================================================================
 *  用户系统
 * ================================================================ */
#if ZOOM_USING_USER

int zoom_shell_add_user(zoom_shell_t *shell, const char *name,
                        const char *password, uint8_t level)
{
    if (!shell || !name || !password) return -1;
    if (shell->userCount >= ZOOM_MAX_USERS) return -1;

    for (uint16_t i = 0; i < shell->userCount; i++) {
        if (zoom_strcmp(shell->users[i].name, name) == 0) return -1;
    }

    zoom_user_t *u = &shell->users[shell->userCount];
    zoom_strncpy(u->name, name, ZOOM_USER_NAME_MAX);
    zoom_strncpy(u->password, password, ZOOM_USER_PASSWORD_MAX);
    u->level = level;
    shell->userCount++;
    return 0;
}

int zoom_shell_login(zoom_shell_t *shell, const char *username,
                     const char *password)
{
    if (!shell || !username || !password) return -1;

    for (uint16_t i = 0; i < shell->userCount; i++) {
        if (zoom_strcmp(shell->users[i].name, username) == 0) {
            if (zoom_strcmp(shell->users[i].password, password) == 0) {
                shell->auth.currentIdx = (int16_t)i;
                shell->auth.isChecked = 1;
                return 0;
            }
            return -1;
        }
    }
    return -1;
}

int zoom_shell_logout(zoom_shell_t *shell)
{
    if (!shell) return -1;
    shell->auth.currentIdx = -1;
    shell->auth.isChecked = 0;
    return 0;
}

uint8_t zoom_shell_get_current_level(zoom_shell_t *shell)
{
    if (!shell) return ZOOM_USER_GUEST;
#if ZOOM_USING_USER
    if (shell->auth.currentIdx >= 0 &&
        shell->auth.currentIdx < (int16_t)shell->userCount) {
        return shell->users[shell->auth.currentIdx].level;
    }
#endif
    return ZOOM_USER_GUEST;
}

#endif /* ZOOM_USING_USER */

/* ================================================================
 *  历史记录
 * ================================================================ */
#if ZOOM_USING_HISTORY

static void history_add(zoom_shell_t *shell, const char *cmd)
{
    if (!cmd || cmd[0] == '\0') return;

    /* 避免重复 */
    if (shell->history.number > 0) {
        uint16_t last = (shell->history.record + ZOOM_HISTORY_MAX_NUMBER - 1)
                        % ZOOM_HISTORY_MAX_NUMBER;
        if (zoom_strcmp(shell->history.item[last], cmd) == 0) {
            shell->history.offset = 0;
            return;
        }
    }

    zoom_strncpy(shell->history.item[shell->history.record],
                 cmd, shell->parser.bufferSize);
    shell->history.record = (shell->history.record + 1) % ZOOM_HISTORY_MAX_NUMBER;
    if (shell->history.number < ZOOM_HISTORY_MAX_NUMBER) {
        shell->history.number++;
    }
    shell->history.offset = 0;
}

static const char *history_get(zoom_shell_t *shell, int16_t offset)
{
    if (offset <= 0 || offset > (int16_t)shell->history.number) return NULL;

    int16_t idx = (int16_t)shell->history.record - offset;
    if (idx < 0) idx += ZOOM_HISTORY_MAX_NUMBER;
    return shell->history.item[idx];
}

static void history_navigate(zoom_shell_t *shell, int dir)
{
    int16_t new_offset = shell->history.offset + (int16_t)dir;
    if (new_offset < 0) new_offset = 0;
    if (new_offset > (int16_t)shell->history.number) {
        new_offset = (int16_t)shell->history.number;
    }

    /* 清除当前行 */
#if ZOOM_USING_ANSI
    /* 回到行首并清除到行尾 */
    while (shell->parser.cursor > 0) {
        shell_write_str(shell, "\b");
        shell->parser.cursor--;
    }
    shell_write_str(shell, "\033[K");
#endif

    shell->parser.length = 0;
    shell->parser.cursor = 0;

    if (new_offset == 0) {
        shell->parser.buffer[0] = '\0';
    } else {
        const char *hist = history_get(shell, new_offset);
        if (hist) {
            uint16_t len = zoom_strlen(hist);
            if (len >= shell->parser.bufferSize) len = shell->parser.bufferSize - 1;
            memcpy(shell->parser.buffer, hist, len);
            shell->parser.buffer[len] = '\0';
            shell->parser.length = len;
            shell->parser.cursor = len;
            shell->write(hist, len);
        }
    }
    shell->history.offset = new_offset;
}

#endif /* ZOOM_USING_HISTORY */

/* ================================================================
 *  Tab 补全
 * ================================================================ */
#if ZOOM_USING_ANSI

static void tab_complete(zoom_shell_t *shell)
{
    if (shell->parser.length == 0) return;

    /* 将当前输入作为前缀 */
    shell->parser.buffer[shell->parser.length] = '\0';

    /* 沿空格分词，确定当前正在补全的层级 */
    const zoom_cmd_t *search_base = shell->cmdList.base;
    uint16_t search_count = shell->cmdList.count;
    const char *prefix = shell->parser.buffer;

    /* 解析已有的完整token来定位子命令层级 */
    char tmp[ZOOM_CMD_MAX_LENGTH];
    zoom_strncpy(tmp, shell->parser.buffer, sizeof(tmp));

    char *tokens[ZOOM_CMD_MAX_ARGS];
    int token_count = 0;
    char *p = tmp;
    while (*p == ' ') p++;
    while (*p && token_count < ZOOM_CMD_MAX_ARGS) {
        tokens[token_count++] = p;
        while (*p && *p != ' ') p++;
        if (*p) { *p++ = '\0'; while (*p == ' ') p++; }
    }

    /* 如果输入以空格结尾，最后一个 token 是完整的，需要进入子命令层级 */
    int input_ends_with_space = (shell->parser.length > 0 &&
                                  shell->parser.buffer[shell->parser.length - 1] == ' ');

    int prefix_token_idx = token_count - 1;
    if (input_ends_with_space) {
        prefix_token_idx = token_count;  /* 空前缀 */
    }

    /* 沿已完成的 token 下钻子命令树 */
    int max_depth = input_ends_with_space ? token_count : token_count - 1;
    for (int depth = 0; depth < max_depth; depth++) {
        int found = 0;
        for (uint16_t i = 0; i < search_count; i++) {
            if (search_base[i].name[0] &&
                zoom_strcmp(search_base[i].name, tokens[depth]) == 0) {
                if (search_base[i].subcmds && search_base[i].subcmd_count > 0) {
                    uint16_t cnt = search_base[i].subcmd_count;
                    search_base = search_base[i].subcmds;
                    search_count = cnt;
                    found = 1;
                }
                break;
            }
        }
        if (!found) break;
    }

    /* 确定要补全的前缀 */
    prefix = (prefix_token_idx >= 0 && prefix_token_idx < token_count)
             ? tokens[prefix_token_idx] : "";
    if (input_ends_with_space) prefix = "";

    uint16_t prefix_len = zoom_strlen(prefix);
    const zoom_cmd_t *match = NULL;
    int match_count = 0;

    for (uint16_t i = 0; i < search_count; i++) {
        if (search_base[i].name[0] == '\0') continue;
        if (search_base[i].attr & ZOOM_ATTR_HIDDEN) continue;
        if (prefix_len == 0 || memcmp(search_base[i].name, prefix, prefix_len) == 0) {
            match = &search_base[i];
            match_count++;
        }
    }

    if (match_count == 1 && match) {
        /* 唯一匹配，自动补全 */
        const char *suffix = match->name + prefix_len;
        uint16_t suffix_len = zoom_strlen(suffix);
        for (uint16_t i = 0; i < suffix_len && shell->parser.length < shell->parser.bufferSize - 2; i++) {
            shell->parser.buffer[shell->parser.length++] = suffix[i];
            shell->parser.cursor++;
            shell_write_char(shell, suffix[i]);
        }
        /* 补个空格 */
        if (shell->parser.length < shell->parser.bufferSize - 1) {
            shell->parser.buffer[shell->parser.length++] = ' ';
            shell->parser.cursor++;
            shell_write_char(shell, ' ');
        }
        shell->parser.buffer[shell->parser.length] = '\0';
    } else if (match_count > 1) {
        /* 多个匹配，列出所有候选 */
        shell_write_str(shell, "\r\n");
        for (uint16_t i = 0; i < search_count; i++) {
            if (search_base[i].name[0] == '\0') continue;
            if (search_base[i].attr & ZOOM_ATTR_HIDDEN) continue;
            if (prefix_len == 0 || memcmp(search_base[i].name, prefix, prefix_len) == 0) {
                shell_write_str(shell, "  ");
                shell_write_str(shell, search_base[i].name);
                shell_write_str(shell, "\r\n");
            }
        }
        /* 重新显示提示符和当前输入 */
        zoom_shell_show_prompt(shell);
        shell->parser.buffer[shell->parser.length] = '\0';
        shell->write(shell->parser.buffer, shell->parser.length);
    }
}

#endif /* ZOOM_USING_ANSI */

/* ================================================================
 *  命令查找 — 沿子命令树查找最深匹配
 * ================================================================ */

const zoom_cmd_t *zoom_shell_find_cmd(zoom_shell_t *shell, const char *name)
{
    if (!shell || !name) return NULL;

    for (uint16_t i = 0; i < shell->cmdList.count; i++) {
        if (shell->cmdList.base[i].name[0] &&
            zoom_strcmp(shell->cmdList.base[i].name, name) == 0) {
            return &shell->cmdList.base[i];
        }
    }
    return NULL;
}

/**
 * 沿子命令树递归查找最深可执行的 handler。
 * 返回匹配的命令，并通过 out_argc/out_argv 返回剩余参数。
 */
static const zoom_cmd_t *resolve_subcmd(const zoom_cmd_t *cmd_table,
                                        uint16_t cmd_count,
                                        int argc, char *argv[],
                                        int *out_argc, char **out_argv[])
{
    if (argc <= 0 || !cmd_table) return NULL;

    for (uint16_t i = 0; i < cmd_count; i++) {
        if (cmd_table[i].name[0] == '\0') continue;
        if (zoom_strcmp(cmd_table[i].name, argv[0]) != 0) continue;

        /* 匹配到了 */
        const zoom_cmd_t *found = &cmd_table[i];

        /* 如果有子命令且后续还有参数，尝试深入 */
        if (found->subcmds && found->subcmd_count > 0 && argc > 1) {
            const zoom_cmd_t *deeper = resolve_subcmd(
                found->subcmds, found->subcmd_count,
                argc - 1, argv + 1,
                out_argc, out_argv);
            if (deeper) return deeper;
        }

        /* 返回当前匹配（叶节点或子命令未匹配） */
        *out_argc = argc - 1;
        *out_argv = argv + 1;
        return found;
    }

    return NULL;
}

/* ================================================================
 *  命令解析与执行
 * ================================================================ */

static int parse_args(char *line, char *argv[], int max_args)
{
    int argc = 0;
    char *p = line;

    while (*p && argc < max_args) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        /* 支持引号包裹的参数 */
        if (*p == '"') {
            p++;
            argv[argc++] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') *p++ = '\0';
        } else {
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = '\0';
        }
    }

    return argc;
}

int zoom_shell_exec(zoom_shell_t *shell, const char *cmd_line)
{
    if (!shell || !cmd_line) return -1;

    /* 复制命令行到 parser buffer（保护原始输入） */
    char cmd_copy[ZOOM_CMD_MAX_LENGTH];
    zoom_strncpy(cmd_copy, cmd_line, sizeof(cmd_copy));

    /* 解析参数 */
    char *argv[ZOOM_CMD_MAX_ARGS];
    int argc = parse_args(cmd_copy, argv, ZOOM_CMD_MAX_ARGS);
    if (argc == 0) return 0;

    /* 沿子命令树查找 */
    int exec_argc;
    char **exec_argv;
    const zoom_cmd_t *cmd = resolve_subcmd(
        shell->cmdList.base, shell->cmdList.count,
        argc, argv, &exec_argc, &exec_argv);

    if (!cmd) {
        zoom_error(shell, "Unknown command: %s\r\n", argv[0]);
        return -1;
    }

    /* 跳过空命令（条件编译禁用的） */
    if (cmd->name[0] == '\0') {
        zoom_error(shell, "Unknown command: %s\r\n", argv[0]);
        return -1;
    }

    /* 权限检查 */
#if ZOOM_USING_USER
    uint8_t level = zoom_shell_get_current_level(shell);
    if (level < cmd->min_level) {
        zoom_error(shell, "Permission denied (need level %d)\r\n", cmd->min_level);
        return -1;
    }
#endif

    /* 没有 handler（纯子命令容器） */
    if (!cmd->func) {
        /* 显示该命令的子命令列表 */
        if (cmd->subcmds && cmd->subcmd_count > 0) {
            zoom_printf(shell, "Subcommands of '%s':\r\n", cmd->name);
            for (uint16_t i = 0; i < cmd->subcmd_count; i++) {
                if (cmd->subcmds[i].name[0] == '\0') continue;
                if (cmd->subcmds[i].attr & ZOOM_ATTR_HIDDEN) continue;
                zoom_printf(shell, "  %-12s %s\r\n",
                           cmd->subcmds[i].name,
                           cmd->subcmds[i].desc ? cmd->subcmds[i].desc : "");
            }
        }
        return 0;
    }

    /* 命令计时 */
#if ZOOM_USING_PERF
    uint32_t tick_start = 0;
    if (shell->stats.perf_enabled) {
        tick_start = ZOOM_GET_TICK();
    }
#endif

    /* 执行 */
    int ret = cmd->func(shell, exec_argc, exec_argv);

    shell->stats.total_cmds_executed++;

#if ZOOM_USING_PERF
    if (shell->stats.perf_enabled && ret != 1) {
        uint32_t elapsed = ZOOM_GET_TICK() - tick_start;
        if (ret == 0) {
            zoom_ok(shell, "%s (elapsed: %lu ms)\r\n",
                    cmd->name, (unsigned long)elapsed);
        } else {
            zoom_error(shell, "%s failed, ret=%d (elapsed: %lu ms)\r\n",
                      cmd->name, ret, (unsigned long)elapsed);
        }
    }
#endif

    return ret;
}

/* ================================================================
 *  提示符
 * ================================================================ */
void zoom_shell_show_prompt(zoom_shell_t *shell)
{
    if (!shell) return;

#if ZOOM_USING_USER
    if (shell->auth.currentIdx >= 0) {
        zoom_printf(shell, "%s@%s> ", shell->users[shell->auth.currentIdx].name, shell->prompt);
    } else {
        zoom_printf(shell, "guest@%s> ", shell->prompt);
    }
#else
    zoom_printf(shell, "%s> ", shell->prompt);
#endif
}

/* ================================================================
 *  欢迎信息
 * ================================================================ */
void zoom_shell_print_welcome(zoom_shell_t *shell)
{
    if (!shell) return;

    shell_write_str(shell, "\r\n");
    shell_write_str(shell, " ______                      ____  _          _ _ \r\n");
    shell_write_str(shell, "|__  / |__   ___  _ __ ___  / ___|| |__   ___| | |\r\n");
    shell_write_str(shell, "  / /| '_ \\ / _ \\| '_ ` _ \\ \\___ \\| '_ \\ / _ \\ | |\r\n");
    shell_write_str(shell, " / /_| | | | (_) | | | | | |___) | | | |  __/ | |\r\n");
    shell_write_str(shell, "/___|_| |_|\\___/|_| |_| |_|____/|_| |_|\\___|_|_|\r\n");
    shell_write_str(shell, "\r\n");
    zoom_printf(shell, " Zoom Shell v%s — Embedded Enhanced Shell\r\n", ZOOM_SHELL_VERSION_STRING);
    shell_write_str(shell, " Type 'help' for available commands.\r\n\r\n");
}

/* ================================================================
 *  逐字节输入处理（UART 中断模式）
 * ================================================================ */
void zoom_shell_input(zoom_shell_t *shell, char data)
{
    if (!shell || shell->state != ZOOM_STATE_RUNNING) return;

    /* 透传模式优先处理 */
#if ZOOM_USING_PASSTHROUGH
    {
        extern int zoom_shell_passthrough_input(zoom_shell_t *, char);
        if (zoom_shell_passthrough_input(shell, data)) return;
    }
#endif

    /* 周期执行: 任意按键终止 */
#if ZOOM_USING_REPEAT
    if (shell->repeat.active) {
        shell->repeat.active = 0;
        zoom_printf(shell, "\r\n");
        zoom_ok(shell, "Repeat stopped\r\n");
        zoom_shell_show_prompt(shell);
        return;
    }
#endif

#if ZOOM_USING_ANSI
    /* ANSI 转义序列处理 */
    if (shell->status.escState == ESC_STATE_ESC) {
        if (data == '[') {
            shell->status.escState = ESC_STATE_BRACKET;
            return;
        }
        shell->status.escState = ESC_STATE_NONE;
        return;
    }

    if (shell->status.escState == ESC_STATE_BRACKET) {
        shell->status.escState = ESC_STATE_NONE;
        switch (data) {
        case 'A': /* 上 */
#if ZOOM_USING_HISTORY
            history_navigate(shell, 1);
#endif
            return;
        case 'B': /* 下 */
#if ZOOM_USING_HISTORY
            history_navigate(shell, -1);
#endif
            return;
        case 'C': /* 右 */
            if (shell->parser.cursor < shell->parser.length) {
                shell->parser.cursor++;
                shell_write_str(shell, "\033[C");
            }
            return;
        case 'D': /* 左 */
            if (shell->parser.cursor > 0) {
                shell->parser.cursor--;
                shell_write_str(shell, "\033[D");
            }
            return;
        case '1': /* Home (ESC[1~) */
        case '4': /* End  (ESC[4~) */
        case '3': /* Delete (ESC[3~) — 需要再吃一个 '~' */
            shell->parser.keyValue = data;
            return;
        default:
            return;
        }
    }
#endif /* ZOOM_USING_ANSI */

    switch (data) {
#if ZOOM_USING_ANSI
    case ZOOM_KEY_ESC:
        shell->status.escState = ESC_STATE_ESC;
        break;

    case ZOOM_KEY_TAB:
        tab_complete(shell);
        break;
#endif

    case ZOOM_KEY_CR:
    case ZOOM_KEY_LF:
        shell_write_str(shell, "\r\n");
        shell->parser.buffer[shell->parser.length] = '\0';

        if (shell->parser.length > 0) {
#if ZOOM_USING_SCRIPT
            {
                extern int zoom_shell_script_handle_line(zoom_shell_t *, const char *);
                if (shell->script.recording >= 0) {
                    zoom_shell_script_handle_line(shell, shell->parser.buffer);
                    shell->parser.length = 0;
                    shell->parser.cursor = 0;
                    break;
                }
            }
#endif

#if ZOOM_USING_HISTORY
            history_add(shell, shell->parser.buffer);
#endif

            /* 别名展开 */
#if ZOOM_USING_ALIAS
            {
                /* 提取第一个 token */
                char first[16];
                int fi = 0;
                const char *p = shell->parser.buffer;
                while (*p == ' ') p++;
                while (*p && *p != ' ' && fi < 15) first[fi++] = *p++;
                first[fi] = '\0';
                const char *expanded = zoom_shell_alias_lookup(shell, first);
                if (expanded) {
                    char alias_buf[ZOOM_CMD_MAX_LENGTH];
                    int ai = 0;
                    const char *v = expanded;
                    while (*v && ai < ZOOM_CMD_MAX_LENGTH - 1) alias_buf[ai++] = *v++;
                    while (*p && ai < ZOOM_CMD_MAX_LENGTH - 1) alias_buf[ai++] = *p++;
                    alias_buf[ai] = '\0';
                    int ret = zoom_shell_exec(shell, alias_buf);
                    if (ret == 1) { shell->state = ZOOM_STATE_IDLE; return; }
                    shell->parser.length = 0;
                    shell->parser.cursor = 0;
                    zoom_shell_show_prompt(shell);
                    break;
                }
            }
#endif

            int ret = zoom_shell_exec(shell, shell->parser.buffer);
            if (ret == 1) {
                shell->state = ZOOM_STATE_IDLE;
                return;
            }
        }

        shell->parser.length = 0;
        shell->parser.cursor = 0;
        zoom_shell_show_prompt(shell);
        break;

    case ZOOM_KEY_BACKSPACE:
    case ZOOM_KEY_DEL:
        if (shell->parser.cursor > 0) {
            /* 删除光标前一个字符 */
            for (uint16_t i = shell->parser.cursor - 1; i < shell->parser.length - 1; i++) {
                shell->parser.buffer[i] = shell->parser.buffer[i + 1];
            }
            shell->parser.length--;
            shell->parser.cursor--;
            shell->parser.buffer[shell->parser.length] = '\0';

#if ZOOM_USING_ANSI
            shell_write_str(shell, "\b");
            /* 重绘光标后的内容 */
            shell->write(shell->parser.buffer + shell->parser.cursor,
                        shell->parser.length - shell->parser.cursor);
            shell_write_str(shell, " \b");
            /* 移回光标 */
            for (uint16_t i = shell->parser.cursor; i < shell->parser.length; i++) {
                shell_write_str(shell, "\b");
            }
#else
            shell_write_str(shell, "\b \b");
#endif
        }
        break;

    default:
        /* 按键绑定: Ctrl+A ~ Ctrl+Z (0x01 ~ 0x1A) */
#if ZOOM_USING_KEYBIND
        if ((uint8_t)data >= 0x01 && (uint8_t)data <= 0x1A) {
            extern int zoom_shell_keybind_handle(zoom_shell_t *, char);
            if (zoom_shell_keybind_handle(shell, data)) break;
        }
#endif
        /* 可打印字符 */
        if (data >= 0x20 && data < 0x7F) {
            if (shell->parser.length < shell->parser.bufferSize - 1) {
                /* 在光标位置插入 */
                if (shell->parser.cursor < shell->parser.length) {
                    for (uint16_t i = shell->parser.length; i > shell->parser.cursor; i--) {
                        shell->parser.buffer[i] = shell->parser.buffer[i - 1];
                    }
                }
                shell->parser.buffer[shell->parser.cursor] = data;
                shell->parser.length++;
                shell->parser.cursor++;
                shell->parser.buffer[shell->parser.length] = '\0';

#if ZOOM_USING_ANSI
                /* 输出光标后的所有内容并移回光标 */
                shell->write(shell->parser.buffer + shell->parser.cursor - 1,
                            shell->parser.length - shell->parser.cursor + 1);
                for (uint16_t i = shell->parser.cursor; i < shell->parser.length; i++) {
                    shell_write_str(shell, "\b");
                }
#else
                shell_write_char(shell, data);
#endif
            }
        }
        break;
    }
}

/* ================================================================
 *  阻塞运行主循环
 * ================================================================ */
int zoom_shell_run(zoom_shell_t *shell)
{
    if (!shell) return -1;

    shell->state = ZOOM_STATE_RUNNING;
    zoom_shell_print_welcome(shell);
    zoom_shell_show_prompt(shell);

    while (shell->state == ZOOM_STATE_RUNNING) {
        char data;
        int16_t ret = shell->read(&data, 1);
        if (ret > 0) {
            zoom_shell_input(shell, data);
        }
#if ZOOM_USING_REPEAT
        {
            extern int zoom_shell_repeat_tick(zoom_shell_t *);
            zoom_shell_repeat_tick(shell);
        }
#endif
    }

    return 0;
}
