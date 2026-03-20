/**
 * @file zoom_shell_var.c
 * @brief Zoom Shell 变量观测系统
 *
 * 允许通过 shell 实时查看和修改注册的变量，用于嵌入式调试。
 * 类似 USMART 的函数调参，但面向变量，支持类型安全的读写。
 *
 * 内置命令: var list / var get <name> / var set <name> <value>
 */

#include "zoom_shell.h"

#if ZOOM_USING_VAR

#include <string.h>

/* ================================================================
 *  内部工具
 * ================================================================ */

static int32_t simple_atoi(const char *s)
{
    int32_t r = 0;
    int neg = 0;
    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') { s++; }

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        while (*s) {
            char c = *s++;
            if (c >= '0' && c <= '9') r = r * 16 + (c - '0');
            else if (c >= 'a' && c <= 'f') r = r * 16 + (c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') r = r * 16 + (c - 'A' + 10);
            else break;
        }
    } else {
        while (*s >= '0' && *s <= '9') r = r * 10 + (*s++ - '0');
    }
    return neg ? -r : r;
}

/**
 * 简易字符串转 float (不依赖 strtof/atof)
 * 支持: "3.14", "-0.5", "100", ".25"
 */
static float simple_atof(const char *s)
{
    float result = 0.0f;
    float sign = 1.0f;
    while (*s == ' ') s++;
    if (*s == '-') { sign = -1.0f; s++; }
    else if (*s == '+') { s++; }

    while (*s >= '0' && *s <= '9') {
        result = result * 10.0f + (*s++ - '0');
    }
    if (*s == '.') {
        s++;
        float frac = 0.1f;
        while (*s >= '0' && *s <= '9') {
            result += (*s++ - '0') * frac;
            frac *= 0.1f;
        }
    }
    return sign * result;
}

/**
 * 简易 float 转字符串 (不依赖 snprintf %f)
 * 精度固定 3 位小数
 */
static int float_to_str(float val, char *buf, uint16_t size)
{
    if (size < 16) return -1;

    int pos = 0;
    if (val < 0) {
        buf[pos++] = '-';
        val = -val;
    }

    uint32_t integer = (uint32_t)val;
    uint32_t frac = (uint32_t)((val - (float)integer) * 1000.0f + 0.5f);

    if (frac >= 1000) { integer++; frac = 0; }

    /* 整数部分 */
    char tmp[11];
    int ti = 0;
    if (integer == 0) { tmp[ti++] = '0'; }
    else {
        while (integer) { tmp[ti++] = '0' + (char)(integer % 10); integer /= 10; }
    }
    while (ti > 0) buf[pos++] = tmp[--ti];

    buf[pos++] = '.';
    buf[pos++] = '0' + (char)(frac / 100);
    buf[pos++] = '0' + (char)((frac / 10) % 10);
    buf[pos++] = '0' + (char)(frac % 10);
    buf[pos] = '\0';
    return pos;
}

static const char *var_type_name(uint8_t type)
{
    switch (type) {
    case ZOOM_VAR_INT8:   return "int8";
    case ZOOM_VAR_INT16:  return "int16";
    case ZOOM_VAR_INT32:  return "int32";
    case ZOOM_VAR_UINT8:  return "uint8";
    case ZOOM_VAR_UINT16: return "uint16";
    case ZOOM_VAR_UINT32: return "uint32";
    case ZOOM_VAR_FLOAT:  return "float";
    case ZOOM_VAR_BOOL:   return "bool";
    case ZOOM_VAR_STRING: return "string";
    default:              return "?";
    }
}

/* ================================================================
 *  公共 API
 * ================================================================ */

const zoom_var_t *zoom_shell_find_var(zoom_shell_t *shell, const char *name)
{
    if (!shell || !name) return NULL;

    for (uint16_t i = 0; i < shell->varList.count; i++) {
        const char *a = shell->varList.base[i].name;
        const char *b = name;
        while (*a && *b && *a == *b) { a++; b++; }
        if (*a == '\0' && *b == '\0') {
            return &shell->varList.base[i];
        }
    }
    return NULL;
}

int zoom_shell_var_get_str(const zoom_var_t *var, char *buf, uint16_t size)
{
    if (!var || !buf || size < 2) return -1;

    switch (var->type) {
    case ZOOM_VAR_INT8: {
        int32_t v = *(int8_t *)var->addr;
        char tmp[12]; int len = 0; int neg = 0;
        if (v < 0) { neg = 1; v = -v; }
        char rev[11]; int ri = 0;
        do { rev[ri++] = '0' + (char)(v % 10); v /= 10; } while (v);
        if (neg) buf[len++] = '-';
        while (ri > 0 && len < size - 1) buf[len++] = rev[--ri];
        buf[len] = '\0';
        (void)tmp;
        return len;
    }
    case ZOOM_VAR_INT16: {
        int32_t v = *(int16_t *)var->addr;
        int len = 0; int neg = 0;
        if (v < 0) { neg = 1; v = -v; }
        char rev[11]; int ri = 0;
        do { rev[ri++] = '0' + (char)(v % 10); v /= 10; } while (v);
        if (neg) buf[len++] = '-';
        while (ri > 0 && len < size - 1) buf[len++] = rev[--ri];
        buf[len] = '\0';
        return len;
    }
    case ZOOM_VAR_INT32: {
        int32_t v = *(int32_t *)var->addr;
        int len = 0; int neg = 0;
        uint32_t uv;
        if (v < 0) { neg = 1; uv = (uint32_t)(-(v+1)) + 1; }
        else { uv = (uint32_t)v; }
        char rev[11]; int ri = 0;
        do { rev[ri++] = '0' + (char)(uv % 10); uv /= 10; } while (uv);
        if (neg) buf[len++] = '-';
        while (ri > 0 && len < size - 1) buf[len++] = rev[--ri];
        buf[len] = '\0';
        return len;
    }
    case ZOOM_VAR_UINT8: {
        uint32_t v = *(uint8_t *)var->addr;
        char rev[11]; int ri = 0; int len = 0;
        do { rev[ri++] = '0' + (char)(v % 10); v /= 10; } while (v);
        while (ri > 0 && len < size - 1) buf[len++] = rev[--ri];
        buf[len] = '\0';
        return len;
    }
    case ZOOM_VAR_UINT16: {
        uint32_t v = *(uint16_t *)var->addr;
        char rev[11]; int ri = 0; int len = 0;
        do { rev[ri++] = '0' + (char)(v % 10); v /= 10; } while (v);
        while (ri > 0 && len < size - 1) buf[len++] = rev[--ri];
        buf[len] = '\0';
        return len;
    }
    case ZOOM_VAR_UINT32: {
        uint32_t v = *(uint32_t *)var->addr;
        char rev[11]; int ri = 0; int len = 0;
        do { rev[ri++] = '0' + (char)(v % 10); v /= 10; } while (v);
        while (ri > 0 && len < size - 1) buf[len++] = rev[--ri];
        buf[len] = '\0';
        return len;
    }
    case ZOOM_VAR_FLOAT:
        return float_to_str(*(float *)var->addr, buf, size);
    case ZOOM_VAR_BOOL:
        if (*(uint8_t *)var->addr) {
            buf[0] = '1'; buf[1] = '\0'; return 1;
        } else {
            buf[0] = '0'; buf[1] = '\0'; return 1;
        }
    case ZOOM_VAR_STRING: {
        const char *s = (const char *)var->addr;
        uint16_t i = 0;
        while (s[i] && i < size - 1) { buf[i] = s[i]; i++; }
        buf[i] = '\0';
        return i;
    }
    default:
        buf[0] = '?'; buf[1] = '\0';
        return 1;
    }
}

int zoom_shell_var_set_str(const zoom_var_t *var, const char *value_str)
{
    if (!var || !value_str) return -1;
    if (var->attr == ZOOM_VAR_RO) return -1;

    switch (var->type) {
    case ZOOM_VAR_INT8:
        *(int8_t *)var->addr = (int8_t)simple_atoi(value_str);
        break;
    case ZOOM_VAR_INT16:
        *(int16_t *)var->addr = (int16_t)simple_atoi(value_str);
        break;
    case ZOOM_VAR_INT32:
        *(int32_t *)var->addr = simple_atoi(value_str);
        break;
    case ZOOM_VAR_UINT8:
        *(uint8_t *)var->addr = (uint8_t)simple_atoi(value_str);
        break;
    case ZOOM_VAR_UINT16:
        *(uint16_t *)var->addr = (uint16_t)simple_atoi(value_str);
        break;
    case ZOOM_VAR_UINT32:
        *(uint32_t *)var->addr = (uint32_t)simple_atoi(value_str);
        break;
    case ZOOM_VAR_FLOAT:
        *(float *)var->addr = simple_atof(value_str);
        break;
    case ZOOM_VAR_BOOL:
        *(uint8_t *)var->addr = (value_str[0] == '1' ||
                                  value_str[0] == 't' ||
                                  value_str[0] == 'T') ? 1 : 0;
        break;
    default:
        return -1;
    }
    return 0;
}

/* ================================================================
 *  内置 var 命令 (var list / var get <name> / var set <name> <val>)
 * ================================================================ */

static int cmd_var_list(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;

    zoom_printf(shell, "  %-16s %-8s %-12s %s  %s\r\n",
                "Name", "Type", "Value", "Attr", "Description");
    zoom_printf(shell, "  %-16s %-8s %-12s %s  %s\r\n",
                "----", "----", "-----", "----", "-----------");

    for (uint16_t i = 0; i < shell->varList.count; i++) {
        const zoom_var_t *v = &shell->varList.base[i];

#if ZOOM_USING_USER
        if (zoom_shell_get_current_level(shell) < v->min_level) continue;
#endif

        char val_buf[32];
        zoom_shell_var_get_str(v, val_buf, sizeof(val_buf));

        zoom_printf(shell, "  %-16s %-8s %-12s [%s]  %s\r\n",
                    v->name,
                    var_type_name(v->type),
                    val_buf,
                    v->attr == ZOOM_VAR_RW ? "RW" : "RO",
                    v->desc ? v->desc : "");
    }
    return 0;
}

static int cmd_var_get(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 1) {
        zoom_error(shell, "Usage: var get <name>\r\n");
        return -1;
    }

    const zoom_var_t *var = zoom_shell_find_var(shell, argv[0]);
    if (!var) {
        zoom_error(shell, "Variable not found: %s\r\n", argv[0]);
        return -1;
    }

#if ZOOM_USING_USER
    if (zoom_shell_get_current_level(shell) < var->min_level) {
        zoom_error(shell, "Permission denied\r\n");
        return -1;
    }
#endif

    char val_buf[32];
    zoom_shell_var_get_str(var, val_buf, sizeof(val_buf));
    zoom_printf(shell, "  %s = %s\r\n", var->name, val_buf);
    return 0;
}

static int cmd_var_set(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 2) {
        zoom_error(shell, "Usage: var set <name> <value>\r\n");
        return -1;
    }

    const zoom_var_t *var = zoom_shell_find_var(shell, argv[0]);
    if (!var) {
        zoom_error(shell, "Variable not found: %s\r\n", argv[0]);
        return -1;
    }

#if ZOOM_USING_USER
    if (zoom_shell_get_current_level(shell) < var->min_level) {
        zoom_error(shell, "Permission denied\r\n");
        return -1;
    }
#endif

    if (var->attr == ZOOM_VAR_RO) {
        zoom_error(shell, "Variable '%s' is read-only\r\n", var->name);
        return -1;
    }

    if (zoom_shell_var_set_str(var, argv[1]) != 0) {
        zoom_error(shell, "Failed to set variable\r\n");
        return -1;
    }

    char val_buf[32];
    zoom_shell_var_get_str(var, val_buf, sizeof(val_buf));
    zoom_ok(shell, "%s = %s\r\n", var->name, val_buf);
    return 0;
}

/* 子命令集 */
ZOOM_SUBCMD_SET(zoom_var_subcmds,
    ZOOM_SUBCMD(list, cmd_var_list, "List all variables"),
    ZOOM_SUBCMD(get,  cmd_var_get,  "Get variable value"),
    ZOOM_SUBCMD(set,  cmd_var_set,  "Set variable value"),
);

/* 导出 var 根命令 */
#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD_WITH_SUB(var, zoom_var_subcmds, "Variable watch system",
                          ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

#endif /* ZOOM_USING_VAR */
