/**
 * @file zoom_shell_log.c
 * @brief Zoom Shell 日志子系统实现
 *
 * 多级别日志 + ANSI 彩色 + 运行时级别调整
 * 内置命令: log level <0-4> / log on / log off
 */

#include "zoom_shell_log.h"

#if ZOOM_USING_LOG

#include <stdarg.h>
#include <string.h>

static zoom_shell_t   *s_log_shell = NULL;
static zoom_log_level_t s_log_level = ZOOM_LOG_INFO;
static uint8_t          s_log_active = 1;

void zoom_log_init(zoom_shell_t *shell)
{
    s_log_shell = shell;
    s_log_level = ZOOM_LOG_INFO;
    s_log_active = 1;
}

void zoom_log_set_level(zoom_log_level_t level)
{
    if (level <= ZOOM_LOG_NONE) s_log_level = level;
}

zoom_log_level_t zoom_log_get_level(void) { return s_log_level; }

static const char *level_tag(zoom_log_level_t level)
{
    switch (level) {
    case ZOOM_LOG_ERROR:   return "E";
    case ZOOM_LOG_WARN:    return "W";
    case ZOOM_LOG_INFO:    return "I";
    case ZOOM_LOG_DEBUG:   return "D";
    case ZOOM_LOG_VERBOSE: return "V";
    default:               return "?";
    }
}

#if ZOOM_USING_ANSI
static const char *level_color(zoom_log_level_t level)
{
    switch (level) {
    case ZOOM_LOG_ERROR:   return "\033[31m";
    case ZOOM_LOG_WARN:    return "\033[33m";
    case ZOOM_LOG_INFO:    return "\033[32m";
    case ZOOM_LOG_DEBUG:   return "\033[34m";
    case ZOOM_LOG_VERBOSE: return "\033[36m";
    default:               return "";
    }
}
#endif

void zoom_log_write(zoom_log_level_t level, const char *tag,
                    const char *fmt, ...)
{
    if (!s_log_shell || !s_log_active) return;
    if (level > s_log_level) return;

    uint32_t tick = ZOOM_GET_TICK();

#if ZOOM_USING_ANSI
    zoom_printf(s_log_shell, "%s[%lu] %s/%s: ",
                level_color(level), (unsigned long)tick,
                level_tag(level), tag ? tag : "?");
#else
    zoom_printf(s_log_shell, "[%lu] %s/%s: ",
                (unsigned long)tick, level_tag(level), tag ? tag : "?");
#endif

    char buf[ZOOM_LOG_BUFFER_SIZE];
    int pos = 0;
    va_list ap;
    va_start(ap, fmt);

    while (*fmt && pos < ZOOM_LOG_BUFFER_SIZE - 1) {
        if (*fmt != '%') { buf[pos++] = *fmt++; continue; }
        fmt++;
        switch (*fmt) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            while (*s && pos < ZOOM_LOG_BUFFER_SIZE - 1) buf[pos++] = *s++;
            break;
        }
        case 'd': {
            int32_t v = va_arg(ap, int);
            char tmp[12]; int ti = 0;
            uint32_t uv;
            if (v < 0) { buf[pos++] = '-'; uv = (uint32_t)(-(v+1))+1; }
            else uv = (uint32_t)v;
            char rev[11]; int ri = 0;
            do { rev[ri++] = '0' + (char)(uv % 10); uv /= 10; } while (uv);
            while (ri > 0 && pos < ZOOM_LOG_BUFFER_SIZE - 1) buf[pos++] = rev[--ri];
            (void)tmp; (void)ti;
            break;
        }
        case 'u': {
            uint32_t uv = va_arg(ap, unsigned int);
            char rev[11]; int ri = 0;
            do { rev[ri++] = '0' + (char)(uv % 10); uv /= 10; } while (uv);
            while (ri > 0 && pos < ZOOM_LOG_BUFFER_SIZE - 1) buf[pos++] = rev[--ri];
            break;
        }
        case 'x': {
            uint32_t uv = va_arg(ap, unsigned int);
            char hex[8]; int hi = 0;
            if (uv == 0) { buf[pos++] = '0'; }
            else {
                while (uv && hi < 8) {
                    uint8_t n = uv & 0xF;
                    hex[hi++] = n < 10 ? '0'+n : 'a'+n-10;
                    uv >>= 4;
                }
                while (hi > 0 && pos < ZOOM_LOG_BUFFER_SIZE - 1) buf[pos++] = hex[--hi];
            }
            break;
        }
        case '%': buf[pos++] = '%'; break;
        default: buf[pos++] = '%'; if (pos < ZOOM_LOG_BUFFER_SIZE -1) buf[pos++] = *fmt; break;
        }
        if (*fmt) fmt++;
    }
    va_end(ap);
    buf[pos] = '\0';

    s_log_shell->write(buf, (uint16_t)pos);

#if ZOOM_USING_ANSI
    zoom_printf(s_log_shell, "\033[0m\r\n");
#else
    zoom_printf(s_log_shell, "\r\n");
#endif
}

void zoom_log_hexdump(const char *tag, const void *data, uint16_t len)
{
    if (!s_log_shell || !s_log_active) return;
    if (ZOOM_LOG_DEBUG > s_log_level) return;

    const uint8_t *p = (const uint8_t *)data;
    zoom_printf(s_log_shell, "[HEX] %s (%u bytes):\r\n", tag ? tag : "data", (unsigned)len);

    for (uint16_t off = 0; off < len; off += 16) {
        zoom_printf(s_log_shell, "  %04x: ", (unsigned)off);
        uint16_t chunk = (len - off > 16) ? 16 : (len - off);
        for (uint16_t i = 0; i < 16; i++) {
            if (i < chunk) zoom_printf(s_log_shell, "%02x ", p[off + i]);
            else zoom_printf(s_log_shell, "   ");
            if (i == 7) zoom_printf(s_log_shell, " ");
        }
        zoom_printf(s_log_shell, " |");
        for (uint16_t i = 0; i < chunk; i++) {
            char c = (char)p[off + i];
            zoom_printf(s_log_shell, "%c", (c >= 0x20 && c < 0x7F) ? c : '.');
        }
        zoom_printf(s_log_shell, "|\r\n");
    }
}

/* ---- Shell 命令 ---- */

static int cmd_log_level(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 1) {
        zoom_printf(shell, "  Current level: %d (%s)\r\n",
                    (int)s_log_level, level_tag(s_log_level));
        zoom_printf(shell, "  0=ERROR, 1=WARN, 2=INFO, 3=DEBUG, 4=VERBOSE\r\n");
        return 0;
    }
    int lv = argv[0][0] - '0';
    if (lv < 0 || lv > 5) lv = 2;
    zoom_log_set_level((zoom_log_level_t)lv);
    zoom_ok(shell, "Log level set to %d\r\n", lv);
    return 0;
}

static int cmd_log_on(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    s_log_active = 1;
    zoom_ok(shell, "Logging enabled\r\n");
    return 0;
}

static int cmd_log_off(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    s_log_active = 0;
    zoom_ok(shell, "Logging disabled\r\n");
    return 0;
}

static int cmd_log_test(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    ZOOM_LOGE("test", "This is an error message");
    ZOOM_LOGW("test", "This is a warning message");
    ZOOM_LOGI("test", "This is an info message");
    ZOOM_LOGD("test", "This is a debug message");
    ZOOM_LOGV("test", "This is a verbose message");
    zoom_ok(shell, "Log test done (visible entries depend on current level)\r\n");
    return 0;
}

ZOOM_SUBCMD_SET(sub_log,
    ZOOM_SUBCMD(level, cmd_log_level, "Get/set log level (0-4)"),
    ZOOM_SUBCMD(on,    cmd_log_on,    "Enable logging"),
    ZOOM_SUBCMD(off,   cmd_log_off,   "Disable logging"),
    ZOOM_SUBCMD(test,  cmd_log_test,  "Output test messages"),
);

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD_WITH_SUB(log, sub_log, "Log system control",
                          ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

#endif /* ZOOM_USING_LOG */
