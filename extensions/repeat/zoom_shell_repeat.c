/**
 * @file zoom_shell_repeat.c
 * @brief Zoom Shell 周期执行扩展
 *
 * 命令: repeat <interval_ms> <command...> / repeat stop
 */

#include "zoom_shell.h"

#if ZOOM_USING_REPEAT

#include <string.h>

static int cmd_repeat(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 1) {
        zoom_error(shell, "Usage: repeat <ms> <command...>  or  repeat stop\r\n");
        return -1;
    }

    /* stop */
    if (argv[0][0] == 's' && argv[0][1] == 't') {
        shell->repeat.active = 0;
        zoom_ok(shell, "Repeat stopped\r\n");
        return 0;
    }

    if (argc < 2) {
        zoom_error(shell, "Usage: repeat <ms> <command...>\r\n");
        return -1;
    }

    /* Parse interval */
    uint32_t ms = 0;
    const char *p = argv[0];
    while (*p >= '0' && *p <= '9') { ms = ms * 10 + (*p - '0'); p++; }
    if (ms < 10) ms = 10;

    /* Build command string from remaining args */
    int pos = 0;
    for (int i = 1; i < argc && pos < ZOOM_CMD_MAX_LENGTH - 2; i++) {
        if (i > 1) shell->repeat.cmd[pos++] = ' ';
        const char *s = argv[i];
        while (*s && pos < ZOOM_CMD_MAX_LENGTH - 1)
            shell->repeat.cmd[pos++] = *s++;
    }
    shell->repeat.cmd[pos] = '\0';

    shell->repeat.interval_ms = ms;
    shell->repeat.last_tick = ZOOM_GET_TICK();
    shell->repeat.active = 1;

    zoom_ok(shell, "Repeating '%s' every %lu ms (any key to stop)\r\n",
            shell->repeat.cmd, (unsigned long)ms);
    return 0;
}

/**
 * 在 zoom_shell_run 主循环中调用。
 * 返回 1 表示执行了一次命令。
 */
int zoom_shell_repeat_tick(zoom_shell_t *shell)
{
    if (!shell->repeat.active) return 0;

    uint32_t now = ZOOM_GET_TICK();
    if (now - shell->repeat.last_tick >= shell->repeat.interval_ms) {
        shell->repeat.last_tick = now;
        zoom_shell_exec(shell, shell->repeat.cmd);
        return 1;
    }
    return 0;
}

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(repeat, cmd_repeat, "Periodically execute a command",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

#endif /* ZOOM_USING_REPEAT */
