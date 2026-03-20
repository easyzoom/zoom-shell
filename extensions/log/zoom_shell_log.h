/**
 * @file zoom_shell_log.h
 * @brief Zoom Shell 日志子系统
 */

#ifndef ZOOM_SHELL_LOG_H
#define ZOOM_SHELL_LOG_H

#include "zoom_shell.h"

#if ZOOM_USING_LOG

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ZOOM_LOG_ERROR = 0,
    ZOOM_LOG_WARN,
    ZOOM_LOG_INFO,
    ZOOM_LOG_DEBUG,
    ZOOM_LOG_VERBOSE,
    ZOOM_LOG_NONE,
} zoom_log_level_t;

void zoom_log_init(zoom_shell_t *shell);
void zoom_log_set_level(zoom_log_level_t level);
zoom_log_level_t zoom_log_get_level(void);
void zoom_log_write(zoom_log_level_t level, const char *tag,
                    const char *fmt, ...);
void zoom_log_hexdump(const char *tag, const void *data, uint16_t len);

#define ZOOM_LOGE(tag, fmt, ...) zoom_log_write(ZOOM_LOG_ERROR,   tag, fmt, ##__VA_ARGS__)
#define ZOOM_LOGW(tag, fmt, ...) zoom_log_write(ZOOM_LOG_WARN,    tag, fmt, ##__VA_ARGS__)
#define ZOOM_LOGI(tag, fmt, ...) zoom_log_write(ZOOM_LOG_INFO,    tag, fmt, ##__VA_ARGS__)
#define ZOOM_LOGD(tag, fmt, ...) zoom_log_write(ZOOM_LOG_DEBUG,   tag, fmt, ##__VA_ARGS__)
#define ZOOM_LOGV(tag, fmt, ...) zoom_log_write(ZOOM_LOG_VERBOSE, tag, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* ZOOM_USING_LOG */
#endif /* ZOOM_SHELL_LOG_H */
