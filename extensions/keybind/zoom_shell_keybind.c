/**
 * @file zoom_shell_keybind.c
 * @brief Zoom Shell 按键绑定系统
 *
 * 预置: Ctrl+L=clear, Ctrl+A=行首, Ctrl+E=行尾, Ctrl+K=删至行尾, Ctrl+U=清行
 * 命令: keybind list
 */

#include "zoom_shell_keybind.h"

#if ZOOM_USING_KEYBIND

#include <string.h>

int zoom_shell_keybind_add(zoom_shell_t *shell, uint8_t key,
                           void (*handler)(zoom_shell_t *), const char *desc)
{
    if (!shell || !handler || shell->keybindCount >= ZOOM_KEYBIND_MAX)
        return -1;
    shell->keybinds[shell->keybindCount].key = key;
    shell->keybinds[shell->keybindCount].handler = handler;
    shell->keybinds[shell->keybindCount].desc = desc;
    shell->keybindCount++;
    return 0;
}

/* --- Built-in key handlers --- */

static void key_clear_screen(zoom_shell_t *shell)
{
#if ZOOM_USING_ANSI
    zoom_printf(shell, "\033[2J\033[H");
    zoom_shell_show_prompt(shell);
    if (shell->parser.length > 0) {
        shell->parser.buffer[shell->parser.length] = '\0';
        shell->write(shell->parser.buffer, shell->parser.length);
    }
#endif
}

static void key_goto_home(zoom_shell_t *shell)
{
#if ZOOM_USING_ANSI
    while (shell->parser.cursor > 0) {
        zoom_printf(shell, "\033[D");
        shell->parser.cursor--;
    }
#endif
}

static void key_goto_end(zoom_shell_t *shell)
{
#if ZOOM_USING_ANSI
    while (shell->parser.cursor < shell->parser.length) {
        zoom_printf(shell, "\033[C");
        shell->parser.cursor++;
    }
#endif
}

static void key_kill_to_end(zoom_shell_t *shell)
{
#if ZOOM_USING_ANSI
    if (shell->parser.cursor < shell->parser.length) {
        zoom_printf(shell, "\033[K");
        shell->parser.length = shell->parser.cursor;
        shell->parser.buffer[shell->parser.length] = '\0';
    }
#endif
}

static void key_clear_line(zoom_shell_t *shell)
{
#if ZOOM_USING_ANSI
    while (shell->parser.cursor > 0) {
        zoom_printf(shell, "\b");
        shell->parser.cursor--;
    }
    zoom_printf(shell, "\033[K");
    shell->parser.length = 0;
    shell->parser.buffer[0] = '\0';
#endif
}

int zoom_shell_keybind_init(zoom_shell_t *shell)
{
    if (!shell) return -1;
    shell->keybindCount = 0;
    zoom_shell_keybind_add(shell, 0x0C, key_clear_screen, "Ctrl+L: Clear screen");
    zoom_shell_keybind_add(shell, 0x01, key_goto_home,    "Ctrl+A: Go to line start");
    zoom_shell_keybind_add(shell, 0x05, key_goto_end,     "Ctrl+E: Go to line end");
    zoom_shell_keybind_add(shell, 0x0B, key_kill_to_end,  "Ctrl+K: Delete to end");
    zoom_shell_keybind_add(shell, 0x15, key_clear_line,   "Ctrl+U: Clear line");
    return 0;
}

int zoom_shell_keybind_handle(zoom_shell_t *shell, char key)
{
    uint8_t k = (uint8_t)key;
    for (uint8_t i = 0; i < shell->keybindCount; i++) {
        if (shell->keybinds[i].key == k && shell->keybinds[i].handler) {
            shell->keybinds[i].handler(shell);
            return 1;
        }
    }
    return 0;
}

/* --- Shell command --- */

static int cmd_keybind_list(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    zoom_printf(shell, "  %-8s %s\r\n", "Key", "Description");
    zoom_printf(shell, "  %-8s %s\r\n", "---", "-----------");
    for (uint8_t i = 0; i < shell->keybindCount; i++) {
        zoom_printf(shell, "  0x%02x    %s\r\n",
                    shell->keybinds[i].key,
                    shell->keybinds[i].desc ? shell->keybinds[i].desc : "");
    }
    return 0;
}

ZOOM_SUBCMD_SET(sub_keybind,
    ZOOM_SUBCMD(list, cmd_keybind_list, "List all key bindings"),
);

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD_WITH_SUB(keybind, sub_keybind, "Key binding management",
                          ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

#endif /* ZOOM_USING_KEYBIND */
