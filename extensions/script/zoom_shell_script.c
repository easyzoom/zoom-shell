/**
 * @file zoom_shell_script.c
 * @brief Zoom Shell 批量脚本系统
 *
 * 命令: script define <name>, script run <name>, script list, script delete <name>
 */

#include "zoom_shell.h"

#if ZOOM_USING_SCRIPT

#include <string.h>

static int zoom_strcmp_s(const char *a, const char *b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static int find_script(zoom_shell_t *shell, const char *name)
{
    for (uint8_t i = 0; i < shell->script.count; i++) {
        if (zoom_strcmp_s(shell->script.slots[i].name, name) == 0)
            return i;
    }
    return -1;
}

static int cmd_script_define(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 1) {
        zoom_error(shell, "Usage: script define <name>\r\n");
        return -1;
    }
    if (shell->script.count >= ZOOM_SCRIPT_MAX) {
        zoom_error(shell, "Script slots full (max %d)\r\n", ZOOM_SCRIPT_MAX);
        return -1;
    }

    int idx = find_script(shell, argv[0]);
    if (idx < 0) {
        idx = shell->script.count++;
        uint16_t i = 0;
        while (argv[0][i] && i < 15) {
            shell->script.slots[idx].name[i] = argv[0][i];
            i++;
        }
        shell->script.slots[idx].name[i] = '\0';
    }
    shell->script.slots[idx].line_count = 0;
    shell->script.recording = (int8_t)idx;

    zoom_printf(shell, "Recording script '%s'. Enter commands, 'end' to finish:\r\n", argv[0]);
    zoom_printf(shell, "script> ");
    return 0;
}

/**
 * 在录入模式下处理一行输入。返回 1 表示录入模式活跃并已消费。
 */
int zoom_shell_script_handle_line(zoom_shell_t *shell, const char *line)
{
    if (shell->script.recording < 0) return 0;

    int idx = shell->script.recording;

    /* 'end' or empty line -> finish recording */
    if (line[0] == '\0' ||
        (line[0] == 'e' && line[1] == 'n' && line[2] == 'd' &&
         (line[3] == '\0' || line[3] == ' '))) {
        shell->script.recording = -1;
        zoom_ok(shell, "Script '%s' saved (%d lines)\r\n",
                shell->script.slots[idx].name,
                shell->script.slots[idx].line_count);
        return 1;
    }

    if (shell->script.slots[idx].line_count >= ZOOM_SCRIPT_MAX_LINES) {
        shell->script.recording = -1;
        zoom_warn(shell, "Script full (%d lines max), saving.\r\n", ZOOM_SCRIPT_MAX_LINES);
        return 1;
    }

    /* Store line */
    uint16_t i = 0;
    while (line[i] && i < ZOOM_CMD_MAX_LENGTH - 1) {
        shell->script.slots[idx].lines[shell->script.slots[idx].line_count][i] = line[i];
        i++;
    }
    shell->script.slots[idx].lines[shell->script.slots[idx].line_count][i] = '\0';
    shell->script.slots[idx].line_count++;

    zoom_printf(shell, "script> ");
    return 1;
}

static int cmd_script_run(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 1) {
        zoom_error(shell, "Usage: script run <name>\r\n");
        return -1;
    }
    int idx = find_script(shell, argv[0]);
    if (idx < 0) {
        zoom_error(shell, "Script '%s' not found\r\n", argv[0]);
        return -1;
    }

    zoom_printf(shell, "--- Running script '%s' (%d lines) ---\r\n",
                shell->script.slots[idx].name,
                shell->script.slots[idx].line_count);

    for (uint8_t i = 0; i < shell->script.slots[idx].line_count; i++) {
        zoom_printf(shell, "> %s\r\n", shell->script.slots[idx].lines[i]);
        zoom_shell_exec(shell, shell->script.slots[idx].lines[i]);
    }

    zoom_printf(shell, "--- Script done ---\r\n");
    return 0;
}

static int cmd_script_list(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    if (shell->script.count == 0) {
        zoom_printf(shell, "  (no scripts)\r\n");
        return 0;
    }
    zoom_printf(shell, "  %-12s %s\r\n", "Name", "Lines");
    zoom_printf(shell, "  %-12s %s\r\n", "----", "-----");
    for (uint8_t i = 0; i < shell->script.count; i++) {
        zoom_printf(shell, "  %-12s %d\r\n",
                    shell->script.slots[i].name,
                    shell->script.slots[i].line_count);
    }
    return 0;
}

static int cmd_script_delete(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 1) {
        zoom_error(shell, "Usage: script delete <name>\r\n");
        return -1;
    }
    int idx = find_script(shell, argv[0]);
    if (idx < 0) {
        zoom_error(shell, "Script '%s' not found\r\n", argv[0]);
        return -1;
    }
    shell->script.slots[idx] = shell->script.slots[--shell->script.count];
    zoom_ok(shell, "Script '%s' deleted\r\n", argv[0]);
    return 0;
}

ZOOM_SUBCMD_SET(sub_script,
    ZOOM_SUBCMD(define, cmd_script_define, "Define a new script"),
    ZOOM_SUBCMD(run,    cmd_script_run,    "Execute a script"),
    ZOOM_SUBCMD(list,   cmd_script_list,   "List all scripts"),
    ZOOM_SUBCMD(delete, cmd_script_delete, "Delete a script"),
);

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD_WITH_SUB(script, sub_script, "Batch script system",
                          ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

#endif /* ZOOM_USING_SCRIPT */
