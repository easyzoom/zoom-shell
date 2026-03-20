/**
 * @file zoom_shell_cmds.c
 * @brief Zoom Shell 内置命令
 *
 * 所有命令通过 ZOOM_EXPORT_CMD 宏注册，无需调用 register 函数。
 * 命令函数签名: int cmd(zoom_shell_t *shell, int argc, char *argv[])
 */

#include "zoom_shell.h"
#include <string.h>

/* ================================================================
 *  help — 显示可用命令（支持子命令树）
 * ================================================================ */

static void print_cmd_tree(zoom_shell_t *shell, const zoom_cmd_t *cmds,
                           uint16_t count, int depth)
{
    for (uint16_t i = 0; i < count; i++) {
        const zoom_cmd_t *c = &cmds[i];
        if (c->name[0] == '\0') continue;
        if (c->attr & ZOOM_ATTR_HIDDEN) continue;

#if ZOOM_USING_USER
        if (zoom_shell_get_current_level(shell) < c->min_level) continue;
#endif

        /* 缩进 */
        for (int d = 0; d < depth; d++) {
            zoom_printf(shell, "  ");
        }

        zoom_printf(shell, "  %-14s %s", c->name, c->desc ? c->desc : "");

        if (c->attr & ZOOM_ATTR_DANGEROUS)  zoom_printf(shell, " [!]");
        if (c->attr & ZOOM_ATTR_ADMIN)      zoom_printf(shell, " [A]");
        if (c->attr & ZOOM_ATTR_DEPRECATED) zoom_printf(shell, " [D]");

        zoom_printf(shell, "\r\n");

        if (c->subcmds && c->subcmd_count > 0) {
            print_cmd_tree(shell, c->subcmds, c->subcmd_count, depth + 1);
        }
    }
}

static int cmd_help(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc > 0) {
        /* help <cmd> — 显示特定命令的帮助 */
        const zoom_cmd_t *cmd = zoom_shell_find_cmd(shell, argv[0]);
        if (!cmd) {
            zoom_error(shell, "Unknown command: %s\r\n", argv[0]);
            return -1;
        }
        zoom_printf(shell, "  %s — %s\r\n", cmd->name,
                    cmd->desc ? cmd->desc : "");
        if (cmd->subcmds && cmd->subcmd_count > 0) {
            zoom_printf(shell, "  Subcommands:\r\n");
            print_cmd_tree(shell, cmd->subcmds, cmd->subcmd_count, 1);
        }
        return 0;
    }

    zoom_printf(shell, "\r\nAvailable commands:\r\n");
    print_cmd_tree(shell, shell->cmdList.base, shell->cmdList.count, 0);
    zoom_printf(shell, "\r\nType 'help <cmd>' for details. [!]=dangerous [A]=admin [D]=deprecated\r\n");
    return 0;
}

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(help, cmd_help, "Show available commands",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

/* ================================================================
 *  version
 * ================================================================ */
static int cmd_version(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    zoom_printf(shell, "Zoom Shell v%s\r\n", ZOOM_SHELL_VERSION_STRING);
    return 0;
}

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(version, cmd_version, "Show version info",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

/* ================================================================
 *  clear
 * ================================================================ */
static int cmd_clear(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
#if ZOOM_USING_ANSI
    zoom_printf(shell, "\033[2J\033[H");
#endif
    return 0;
}

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(clear, cmd_clear, "Clear screen",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

/* ================================================================
 *  echo
 * ================================================================ */
static int cmd_echo(zoom_shell_t *shell, int argc, char *argv[])
{
    for (int i = 0; i < argc; i++) {
        if (i > 0) zoom_printf(shell, " ");
        zoom_printf(shell, "%s", argv[i]);
    }
    zoom_printf(shell, "\r\n");
    return 0;
}

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(echo, cmd_echo, "Echo text",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

/* ================================================================
 *  history
 * ================================================================ */
#if ZOOM_USING_HISTORY

static int cmd_history(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;

    if (shell->history.number == 0) {
        zoom_printf(shell, "  (no history)\r\n");
        return 0;
    }

    for (int16_t i = (int16_t)shell->history.number; i > 0; i--) {
        int16_t idx = (int16_t)shell->history.record - i;
        if (idx < 0) idx += ZOOM_HISTORY_MAX_NUMBER;
        if (shell->history.item[idx][0]) {
            zoom_printf(shell, "  %d: %s\r\n",
                       (int)(shell->history.number - i + 1),
                       shell->history.item[idx]);
        }
    }
    return 0;
}

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(history, cmd_history, "Show command history",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

#endif /* ZOOM_USING_HISTORY */

/* ================================================================
 *  login / logout / user
 * ================================================================ */
#if ZOOM_USING_USER

static int cmd_login(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 2) {
        zoom_error(shell, "Usage: login <username> <password>\r\n");
        return -1;
    }
    if (zoom_shell_login(shell, argv[0], argv[1]) == 0) {
        zoom_ok(shell, "Logged in as '%s'\r\n", argv[0]);
    } else {
        zoom_error(shell, "Login failed\r\n");
    }
    return 0;
}

static int cmd_logout(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    zoom_shell_logout(shell);
    zoom_ok(shell, "Logged out\r\n");
    return 0;
}

static int cmd_user_list(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    zoom_printf(shell, "  %-16s %s\r\n", "Username", "Level");
    zoom_printf(shell, "  %-16s %s\r\n", "--------", "-----");
    for (uint16_t i = 0; i < shell->userCount; i++) {
        const char *lvl_str;
        switch (shell->users[i].level) {
        case ZOOM_USER_ROOT:  lvl_str = "root";  break;
        case ZOOM_USER_ADMIN: lvl_str = "admin"; break;
        case ZOOM_USER_USER:  lvl_str = "user";  break;
        default:              lvl_str = "guest"; break;
        }
        zoom_printf(shell, "  %-16s %s%s\r\n",
                    shell->users[i].name, lvl_str,
                    (int16_t)i == shell->auth.currentIdx ? " (current)" : "");
    }
    return 0;
}

static int cmd_user_add(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 3) {
        zoom_error(shell, "Usage: user add <name> <password> <level:0-3>\r\n");
        return -1;
    }
    uint8_t level = (uint8_t)(argv[2][0] - '0');
    if (level > 3) level = 0;
    if (zoom_shell_add_user(shell, argv[0], argv[1], level) == 0) {
        zoom_ok(shell, "User '%s' added\r\n", argv[0]);
    } else {
        zoom_error(shell, "Failed to add user\r\n");
    }
    return 0;
}

ZOOM_SUBCMD_SET(zoom_user_subcmds,
    ZOOM_SUBCMD(list, cmd_user_list, "List all users"),
    ZOOM_SUBCMD_EX(add, cmd_user_add, "Add user", ZOOM_ATTR_ADMIN, ZOOM_USER_ADMIN),
);

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(login, cmd_login, "Login as user",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
ZOOM_EXPORT_CMD(logout, cmd_logout, "Logout current user",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
ZOOM_EXPORT_CMD_WITH_SUB(user, zoom_user_subcmds, "User management",
                          ZOOM_ATTR_ADMIN, ZOOM_USER_ADMIN);
#endif

#endif /* ZOOM_USING_USER */

/* ================================================================
 *  perf — 命令执行计时开关
 * ================================================================ */
#if ZOOM_USING_PERF

static int cmd_perf_on(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    shell->stats.perf_enabled = 1;
    zoom_ok(shell, "Performance timing enabled\r\n");
    return 0;
}

static int cmd_perf_off(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    shell->stats.perf_enabled = 0;
    zoom_ok(shell, "Performance timing disabled\r\n");
    return 0;
}

ZOOM_SUBCMD_SET(zoom_perf_subcmds,
    ZOOM_SUBCMD(on,  cmd_perf_on,  "Enable timing"),
    ZOOM_SUBCMD(off, cmd_perf_off, "Disable timing"),
);

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD_WITH_SUB(perf, zoom_perf_subcmds, "Command execution timing",
                          ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

#endif /* ZOOM_USING_PERF */

/* ================================================================
 *  stats — 统计信息
 * ================================================================ */
static int cmd_stats(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    zoom_printf(shell, "  Commands registered : %u\r\n", (unsigned)shell->cmdList.count);
    zoom_printf(shell, "  Commands executed   : %lu\r\n", (unsigned long)shell->stats.total_cmds_executed);
#if ZOOM_USING_VAR
    zoom_printf(shell, "  Variables registered: %u\r\n", (unsigned)shell->varList.count);
#endif
#if ZOOM_USING_USER
    zoom_printf(shell, "  Users               : %u\r\n", (unsigned)shell->userCount);
#endif
    return 0;
}

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(stats, cmd_stats, "Show shell statistics",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

/* ================================================================
 *  exit
 * ================================================================ */
static int cmd_exit(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    zoom_printf(shell, "Bye!\r\n");
    return 1;  /* 返回 1 表示退出 */
}

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(exit, cmd_exit, "Exit shell", ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif
