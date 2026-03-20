/**
 * @file zoom_shell_passthrough.c
 * @brief Zoom Shell 透传模式
 *
 * 进入透传后所有输入直接转发到外设回调，Ctrl+D 退出
 * 命令: passthrough <prompt>
 */

#include "zoom_shell.h"

#if ZOOM_USING_PASSTHROUGH

#include <string.h>

void zoom_shell_passthrough_set(zoom_shell_t *shell, const char *prompt,
    int (*handler)(zoom_shell_t *, const char *, uint16_t))
{
    if (!shell || !handler) return;
    shell->passthrough.handler = handler;
    shell->passthrough.active = 1;

    uint16_t i = 0;
    if (prompt) {
        while (prompt[i] && i < sizeof(shell->passthrough.prompt) - 1) {
            shell->passthrough.prompt[i] = prompt[i];
            i++;
        }
    }
    shell->passthrough.prompt[i] = '\0';

    zoom_printf(shell, "\r\n--- Passthrough mode [Ctrl+D to exit] ---\r\n");
    zoom_printf(shell, "%s", shell->passthrough.prompt);
}

void zoom_shell_passthrough_exit(zoom_shell_t *shell)
{
    if (!shell) return;
    shell->passthrough.active = 0;
    shell->passthrough.handler = NULL;
    zoom_printf(shell, "\r\n--- Passthrough mode exited ---\r\n");
    zoom_shell_show_prompt(shell);
}

/**
 * 在 zoom_shell_input 开头调用此函数处理透传数据。
 * 返回 1 表示已消费该字节（不走正常输入流程），0 表示未处理。
 */
int zoom_shell_passthrough_input(zoom_shell_t *shell, char data)
{
    if (!shell->passthrough.active) return 0;

    if (data == 0x04) {
        zoom_shell_passthrough_exit(shell);
        return 1;
    }

    if (shell->passthrough.handler) {
        shell->passthrough.handler(shell, &data, 1);
    }

    if (data == '\r' || data == '\n') {
        zoom_printf(shell, "\r\n%s", shell->passthrough.prompt);
    }

    return 1;
}

/* Demo: echo passthrough handler that just echoes back */
static int echo_passthrough_handler(zoom_shell_t *shell, const char *data, uint16_t len)
{
    if (shell->write) shell->write(data, len);
    return 0;
}

static int cmd_passthrough(zoom_shell_t *shell, int argc, char *argv[])
{
    const char *prompt = (argc > 0) ? argv[0] : "PT> ";
    zoom_shell_passthrough_set(shell, prompt, echo_passthrough_handler);
    return 0;
}

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(passthrough, cmd_passthrough, "Enter passthrough mode",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_USER);
#endif

#endif /* ZOOM_USING_PASSTHROUGH */
