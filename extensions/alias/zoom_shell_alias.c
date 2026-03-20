/**
 * @file zoom_shell_alias.c
 * @brief Zoom Shell 命令别名系统
 *
 * 命令: alias <name> <value>, alias list, alias rm <name>
 */

#include "zoom_shell.h"

#if ZOOM_USING_ALIAS

#include <string.h>

static int zoom_strcmp(const char *a, const char *b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static void zoom_strncpy_alias(char *dst, const char *src, uint16_t n)
{
    uint16_t i;
    for (i = 0; i < n - 1 && src[i]; i++) dst[i] = src[i];
    dst[i] = '\0';
}

int zoom_shell_alias_add(zoom_shell_t *shell, const char *name, const char *value)
{
    if (!shell || !name || !value) return -1;

    /* Update existing */
    for (uint8_t i = 0; i < shell->alias.count; i++) {
        if (zoom_strcmp(shell->alias.entries[i].name, name) == 0) {
            zoom_strncpy_alias(shell->alias.entries[i].value, value, ZOOM_CMD_MAX_LENGTH);
            return 0;
        }
    }

    if (shell->alias.count >= ZOOM_ALIAS_MAX) return -1;

    zoom_strncpy_alias(shell->alias.entries[shell->alias.count].name, name, 16);
    zoom_strncpy_alias(shell->alias.entries[shell->alias.count].value, value, ZOOM_CMD_MAX_LENGTH);
    shell->alias.count++;
    return 0;
}

int zoom_shell_alias_remove(zoom_shell_t *shell, const char *name)
{
    if (!shell || !name) return -1;
    for (uint8_t i = 0; i < shell->alias.count; i++) {
        if (zoom_strcmp(shell->alias.entries[i].name, name) == 0) {
            shell->alias.entries[i] = shell->alias.entries[--shell->alias.count];
            return 0;
        }
    }
    return -1;
}

const char *zoom_shell_alias_lookup(zoom_shell_t *shell, const char *name)
{
    if (!shell || !name) return NULL;
    for (uint8_t i = 0; i < shell->alias.count; i++) {
        if (zoom_strcmp(shell->alias.entries[i].name, name) == 0) {
            return shell->alias.entries[i].value;
        }
    }
    return NULL;
}

/* --- Shell commands --- */

static int cmd_alias(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc == 0) {
        /* alias list */
        if (shell->alias.count == 0) {
            zoom_printf(shell, "  (no aliases)\r\n");
            return 0;
        }
        zoom_printf(shell, "  %-12s %s\r\n", "Alias", "Value");
        zoom_printf(shell, "  %-12s %s\r\n", "-----", "-----");
        for (uint8_t i = 0; i < shell->alias.count; i++) {
            zoom_printf(shell, "  %-12s %s\r\n",
                        shell->alias.entries[i].name,
                        shell->alias.entries[i].value);
        }
        return 0;
    }

    if (argc == 1) {
        /* alias rm <name> check or alias <name> to display */
        const char *val = zoom_shell_alias_lookup(shell, argv[0]);
        if (val) {
            zoom_printf(shell, "  %s = '%s'\r\n", argv[0], val);
        } else {
            zoom_error(shell, "Alias '%s' not found\r\n", argv[0]);
        }
        return 0;
    }

    /* alias rm <name> */
    if (argv[0][0] == 'r' && argv[0][1] == 'm' && argv[0][2] == '\0') {
        if (argc < 2) { zoom_error(shell, "Usage: alias rm <name>\r\n"); return -1; }
        if (zoom_shell_alias_remove(shell, argv[1]) == 0)
            zoom_ok(shell, "Alias '%s' removed\r\n", argv[1]);
        else
            zoom_error(shell, "Alias '%s' not found\r\n", argv[1]);
        return 0;
    }

    /* alias <name> <value...> */
    char value_buf[ZOOM_CMD_MAX_LENGTH];
    int pos = 0;
    for (int i = 1; i < argc && pos < ZOOM_CMD_MAX_LENGTH - 2; i++) {
        if (i > 1) value_buf[pos++] = ' ';
        const char *s = argv[i];
        while (*s && pos < ZOOM_CMD_MAX_LENGTH - 1) value_buf[pos++] = *s++;
    }
    value_buf[pos] = '\0';

    if (zoom_shell_alias_add(shell, argv[0], value_buf) == 0)
        zoom_ok(shell, "%s -> '%s'\r\n", argv[0], value_buf);
    else
        zoom_error(shell, "Alias table full\r\n");
    return 0;
}

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(alias, cmd_alias, "Command alias (alias <name> <cmd>, alias rm <name>)",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

#endif /* ZOOM_USING_ALIAS */
