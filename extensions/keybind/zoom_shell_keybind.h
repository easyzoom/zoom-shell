/**
 * @file zoom_shell_keybind.h
 * @brief Zoom Shell 按键绑定系统
 */

#ifndef ZOOM_SHELL_KEYBIND_H
#define ZOOM_SHELL_KEYBIND_H

#include "zoom_shell.h"

#if ZOOM_USING_KEYBIND

#ifdef __cplusplus
extern "C" {
#endif

int  zoom_shell_keybind_init(zoom_shell_t *shell);
int  zoom_shell_keybind_handle(zoom_shell_t *shell, char key);

#ifdef __cplusplus
}
#endif

#endif /* ZOOM_USING_KEYBIND */
#endif /* ZOOM_SHELL_KEYBIND_H */
