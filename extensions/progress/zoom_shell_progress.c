/**
 * @file zoom_shell_progress.c
 * @brief Zoom Shell 进度条/spinner/gauge API
 *
 * 命令: progress demo
 */

#include "zoom_shell_progress.h"

#if ZOOM_USING_PROGRESS

static uint8_t s_spinner_state = 0;

void zoom_progress_bar(zoom_shell_t *shell, uint32_t current, uint32_t total,
                       uint8_t width)
{
    if (!shell || total == 0) return;
    if (width < 5) width = 20;
    if (width > 60) width = 60;

    uint32_t pct = (current * 100) / total;
    uint32_t filled = (current * width) / total;
    if (filled > width) filled = width;

    zoom_printf(shell, "\r  [");
    for (uint8_t i = 0; i < width; i++) {
        if (i < filled) zoom_printf(shell, "#");
        else zoom_printf(shell, ".");
    }
    zoom_printf(shell, "] %lu%% (%lu/%lu)",
                (unsigned long)pct,
                (unsigned long)current,
                (unsigned long)total);
}

void zoom_spinner(zoom_shell_t *shell)
{
    if (!shell) return;
    const char frames[] = "|/-\\";
    zoom_printf(shell, "\r  %c ", frames[s_spinner_state & 3]);
    s_spinner_state++;
}

void zoom_gauge(zoom_shell_t *shell, const char *label, int32_t value,
                int32_t min_val, int32_t max_val, uint8_t width)
{
    if (!shell || max_val <= min_val) return;
    if (width < 5) width = 20;

    int32_t range = max_val - min_val;
    int32_t clamped = value;
    if (clamped < min_val) clamped = min_val;
    if (clamped > max_val) clamped = max_val;
    uint32_t filled = (uint32_t)((clamped - min_val) * width / range);

    zoom_printf(shell, "  %-10s [", label ? label : "");
    for (uint8_t i = 0; i < width; i++) {
        if (i < filled) zoom_printf(shell, "|");
        else zoom_printf(shell, " ");
    }
    zoom_printf(shell, "] %ld\r\n", (long)value);
}

/* --- Demo command --- */

static int cmd_progress_demo(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;

    zoom_printf(shell, "\r\n  Progress bar demo:\r\n");
    for (uint32_t i = 0; i <= 100; i += 10) {
        zoom_progress_bar(shell, i, 100, 30);
        zoom_printf(shell, "\r\n");
    }

    zoom_printf(shell, "\r\n  Gauge demo:\r\n");
    zoom_gauge(shell, "CPU",   65,  0, 100, 20);
    zoom_gauge(shell, "Memory", 42,  0, 100, 20);
    zoom_gauge(shell, "Temp",   78, -40, 125, 20);

    zoom_printf(shell, "\r\n  Spinner frames: ");
    for (int i = 0; i < 4; i++) {
        zoom_spinner(shell);
    }
    zoom_printf(shell, "\r\n");

    return 0;
}

ZOOM_SUBCMD_SET(sub_progress,
    ZOOM_SUBCMD(demo, cmd_progress_demo, "Show progress/gauge/spinner demo"),
);

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD_WITH_SUB(progress, sub_progress, "Progress bar utilities",
                          ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

#endif /* ZOOM_USING_PROGRESS */
