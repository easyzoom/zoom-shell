/**
 * @file zoom_shell_progress.h
 * @brief Zoom Shell 进度条/spinner/gauge API
 */

#ifndef ZOOM_SHELL_PROGRESS_H
#define ZOOM_SHELL_PROGRESS_H

#include "zoom_shell.h"

#if ZOOM_USING_PROGRESS

#ifdef __cplusplus
extern "C" {
#endif

void zoom_progress_bar(zoom_shell_t *shell, uint32_t current, uint32_t total, uint8_t width);
void zoom_spinner(zoom_shell_t *shell);
void zoom_gauge(zoom_shell_t *shell, const char *label, int32_t value, int32_t min_val, int32_t max_val, uint8_t width);

#ifdef __cplusplus
}
#endif

#endif /* ZOOM_USING_PROGRESS */
#endif /* ZOOM_SHELL_PROGRESS_H */
