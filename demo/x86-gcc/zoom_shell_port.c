/**
 * @file zoom_shell_port.c
 * @brief x86-gcc 平台移植层
 *
 * 对接 stdin/stdout 作为 shell I/O。
 */

#include "zoom_shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/* Shell 实例与缓冲区 — 静态分配 */
zoom_shell_t g_shell;
static char  s_shell_buffer[2816];  /* 256 * (10+1) = 2816 for history */

/* ================================================================
 *  I/O 回调
 * ================================================================ */

static int16_t port_read(char *data, uint16_t len)
{
    (void)len;
    int c = getchar();
    if (c == EOF) return 0;
    *data = (char)c;
    return 1;
}

static int16_t port_write(const char *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        putchar(data[i]);
    }
    fflush(stdout);
    return (int16_t)len;
}

/* ================================================================
 *  终端 raw 模式
 * ================================================================ */

static struct termios s_orig_termios;

static void terminal_restore(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &s_orig_termios);
}

static void terminal_raw_mode(void)
{
    tcgetattr(STDIN_FILENO, &s_orig_termios);
    atexit(terminal_restore);

    struct termios raw = s_orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

/* ================================================================
 *  示例自定义命令 — 展示导出宏用法
 * ================================================================ */

static int cmd_hello(zoom_shell_t *shell, int argc, char *argv[])
{
    zoom_printf(shell, "Hello from Zoom Shell!\r\n");
    if (argc > 0) {
        zoom_printf(shell, "Args:");
        for (int i = 0; i < argc; i++)
            zoom_printf(shell, " %s", argv[i]);
        zoom_printf(shell, "\r\n");
    }
    return 0;
}
ZOOM_EXPORT_CMD(hello, cmd_hello, "Greeting demo command",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);

/* 子命令演示: gpio set/get/toggle */
static int cmd_gpio_set(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 2) { zoom_error(shell, "Usage: gpio set <pin> <0|1>\r\n"); return -1; }
    zoom_ok(shell, "GPIO pin %s set to %s\r\n", argv[0], argv[1]);
    return 0;
}

static int cmd_gpio_get(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 1) { zoom_error(shell, "Usage: gpio get <pin>\r\n"); return -1; }
    zoom_printf(shell, "  GPIO pin %s = 1 (simulated)\r\n", argv[0]);
    return 0;
}

static int cmd_gpio_toggle(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 1) { zoom_error(shell, "Usage: gpio toggle <pin>\r\n"); return -1; }
    zoom_ok(shell, "GPIO pin %s toggled\r\n", argv[0]);
    return 0;
}

ZOOM_SUBCMD_SET(sub_gpio,
    ZOOM_SUBCMD(set,    cmd_gpio_set,    "Set pin level"),
    ZOOM_SUBCMD(get,    cmd_gpio_get,    "Get pin level"),
    ZOOM_SUBCMD(toggle, cmd_gpio_toggle, "Toggle pin"),
);
ZOOM_EXPORT_CMD_WITH_SUB(gpio, sub_gpio, "GPIO operations",
                          ZOOM_ATTR_DEFAULT, ZOOM_USER_USER);

/* 变量观测演示 */
static int32_t  demo_pid_kp = 100;
static float    demo_motor_speed = 0.0f;
static uint8_t  demo_led_state = 0;

ZOOM_EXPORT_VAR(pid_kp,      demo_pid_kp,      ZOOM_VAR_INT32, "PID Kp x100",   ZOOM_VAR_RW, ZOOM_USER_GUEST);
ZOOM_EXPORT_VAR(motor_speed, demo_motor_speed,  ZOOM_VAR_FLOAT, "Motor RPM",     ZOOM_VAR_RO, ZOOM_USER_GUEST);
ZOOM_EXPORT_VAR(led_state,   demo_led_state,    ZOOM_VAR_BOOL,  "LED on/off",    ZOOM_VAR_RW, ZOOM_USER_GUEST);

/* 预定义用户 */
ZOOM_EXPORT_USER(admin, "admin", ZOOM_USER_ADMIN);
ZOOM_EXPORT_USER(root,  "root",  ZOOM_USER_ROOT);

/* ================================================================
 *  初始化
 * ================================================================ */

void zoom_shell_port_init(void)
{
    terminal_raw_mode();

    memset(&g_shell, 0, sizeof(g_shell));
    g_shell.read  = port_read;
    g_shell.write = port_write;

    if (zoom_shell_init(&g_shell, s_shell_buffer, sizeof(s_shell_buffer)) != 0) {
        port_write("Shell init failed!\r\n", 20);
        return;
    }

#if ZOOM_USING_KEYBIND
    {
        extern int zoom_shell_keybind_init(zoom_shell_t *);
        zoom_shell_keybind_init(&g_shell);
    }
#endif

#if ZOOM_USING_LOG
    {
        extern void zoom_log_init(zoom_shell_t *);
        zoom_log_init(&g_shell);
    }
#endif
}
