/**
 * @file main.c
 * @brief Zoom Shell x86-gcc demo 入口
 */

#include "zoom_shell.h"
#include <signal.h>

extern zoom_shell_t g_shell;
extern void zoom_shell_port_init(void);

static void signal_handler(int sig)
{
    (void)sig;
    g_shell.state = ZOOM_STATE_IDLE;
}

int main(void)
{
    signal(SIGINT, signal_handler);

    zoom_shell_port_init();
    zoom_shell_run(&g_shell);
    zoom_shell_deinit(&g_shell);

    return 0;
}
