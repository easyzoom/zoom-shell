/**
 * @file zoom_shell_pushbox.c
 * @brief Zoom Shell 推箱子游戏
 *
 * 命令: pushbox
 * 控制: wasd, r=重置, q=退出
 */

#include "zoom_shell.h"

#if ZOOM_USING_GAME

#define PB_ROWS 7
#define PB_COLS 8

/* 0=路, 1=墙, 2=箱子, 3=终点, 4=玩家, 5=箱子在终点, 7=玩家在终点 */
static const int g_initial_map[PB_ROWS][PB_COLS] = {
    { 0, 1, 1, 1, 1, 1, 1, 0 },
    { 0, 1, 0, 0, 0, 0, 1, 1 },
    { 1, 3, 0, 1, 1, 2, 0, 1 },
    { 1, 0, 3, 3, 2, 0, 0, 1 },
    { 1, 0, 0, 1, 2, 0, 0, 1 },
    { 1, 0, 0, 4, 0, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 0, 0, 0 },
};

static int pb_board[PB_ROWS][PB_COLS];
static int pb_row, pb_col;

static void pb_init(void)
{
    for (int i = 0; i < PB_ROWS; i++)
        for (int j = 0; j < PB_COLS; j++) {
            pb_board[i][j] = g_initial_map[i][j];
            if (pb_board[i][j] == 4) { pb_row = i; pb_col = j; }
        }
}

static void pb_print(zoom_shell_t *shell)
{
    zoom_printf(shell, "\r\n  Zoom Shell PushBox!\r\n");
    for (int i = 0; i < PB_ROWS; i++) {
        zoom_printf(shell, "  ");
        for (int j = 0; j < PB_COLS; j++) {
            switch (pb_board[i][j]) {
            case 0: zoom_printf(shell, "  "); break;
            case 1: zoom_printf(shell, "[]"); break;
            case 2:
            case 5: zoom_printf(shell, "()"); break;
            case 3: zoom_printf(shell, "**"); break;
            case 4:
            case 7: zoom_printf(shell, "<>"); break;
            }
        }
        zoom_printf(shell, "\r\n");
    }
    zoom_printf(shell, "  wasd=move r=reset q=quit\r\n");
}

static int pb_move(zoom_shell_t *shell, int dr, int dc)
{
    int nr = pb_row + dr, nc = pb_col + dc;
    int nnr = pb_row + 2*dr, nnc = pb_col + 2*dc;

    if (nr < 0 || nr >= PB_ROWS || nc < 0 || nc >= PB_COLS) return 0;

    int front = pb_board[nr][nc];

    if (front == 0 || front == 3) {
        pb_board[nr][nc] += 4;
        pb_board[pb_row][pb_col] -= 4;
        pb_row = nr; pb_col = nc;
    } else if (front == 2 || front == 5) {
        if (nnr < 0 || nnr >= PB_ROWS || nnc < 0 || nnc >= PB_COLS) return 0;
        int beyond = pb_board[nnr][nnc];
        if (beyond == 0 || beyond == 3) {
            pb_board[nnr][nnc] += 2;
            pb_board[nr][nc] = pb_board[nr][nc] - 2 + 4;
            pb_board[pb_row][pb_col] -= 4;
            pb_row = nr; pb_col = nc;
        }
    }

    /* Check win */
    int goals = 0;
    for (int i = 0; i < PB_ROWS; i++)
        for (int j = 0; j < PB_COLS; j++)
            if (pb_board[i][j] == 5) goals++;

    zoom_printf(shell, "\033[H");
    pb_print(shell);

    if (goals == 3) {
        zoom_printf(shell, "\r\n  Congratulations! You win!\r\n");
        return 1;
    }
    return 0;
}

static int cmd_pushbox(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;

    zoom_printf(shell, "\033[?25l\033[2J\033[H");
    pb_init();
    pb_print(shell);

    while (1) {
        char c;
        if (!shell->read || shell->read(&c, 1) != 1) break;

        int done = 0;
        switch (c) {
        case 'w': done = pb_move(shell, -1, 0); break;
        case 's': done = pb_move(shell, 1, 0); break;
        case 'a': done = pb_move(shell, 0, -1); break;
        case 'd': done = pb_move(shell, 0, 1); break;
        case 'r':
            pb_init();
            zoom_printf(shell, "\033[H");
            pb_print(shell);
            break;
        case 'q': done = 1; break;
        }
        if (done) break;
    }

    zoom_printf(shell, "\033[?25h\033[m");
    return 0;
}

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(pushbox, cmd_pushbox, "Play Pushbox game",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

#endif /* ZOOM_USING_GAME */
