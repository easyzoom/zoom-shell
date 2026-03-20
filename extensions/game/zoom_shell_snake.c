/**
 * @file zoom_shell_snake.c
 * @brief Zoom Shell 贪吃蛇游戏 (原创)
 *
 * 命令: snake
 * 控制: wasd, q=退出
 * 20x15 游戏区域，ANSI 终端渲染
 */

#include "zoom_shell.h"

#if ZOOM_USING_GAME

#define SNAKE_W 20
#define SNAKE_H 15
#define SNAKE_MAX_LEN (SNAKE_W * SNAKE_H)

static struct {
    int16_t x[SNAKE_MAX_LEN];
    int16_t y[SNAKE_MAX_LEN];
    int16_t head;
    int16_t len;
    int16_t dx, dy;
    int16_t food_x, food_y;
    uint32_t score;
    uint8_t  board[SNAKE_W][SNAKE_H];
} snake;

static uint32_t snake_rand_state = 1;
static uint32_t snake_rand(void)
{
    snake_rand_state ^= snake_rand_state << 13;
    snake_rand_state ^= snake_rand_state >> 17;
    snake_rand_state ^= snake_rand_state << 5;
    return snake_rand_state;
}

static void snake_place_food(void)
{
    int attempts = 0;
    do {
        snake.food_x = (int16_t)(snake_rand() % SNAKE_W);
        snake.food_y = (int16_t)(snake_rand() % SNAKE_H);
        attempts++;
    } while (snake.board[snake.food_x][snake.food_y] != 0 && attempts < 300);
}

static void snake_init(void)
{
    for (int x = 0; x < SNAKE_W; x++)
        for (int y = 0; y < SNAKE_H; y++)
            snake.board[x][y] = 0;

    snake.head = 0;
    snake.len = 3;
    snake.dx = 1; snake.dy = 0;
    snake.score = 0;

    int sx = SNAKE_W / 2, sy = SNAKE_H / 2;
    for (int i = 0; i < snake.len; i++) {
        snake.x[i] = (int16_t)(sx - snake.len + 1 + i);
        snake.y[i] = (int16_t)sy;
        snake.board[snake.x[i]][snake.y[i]] = 1;
    }
    snake.head = snake.len - 1;

    snake_place_food();
}

static void snake_draw(zoom_shell_t *shell)
{
    zoom_printf(shell, "\033[H");
    zoom_printf(shell, "  Snake  Score: %lu\r\n", (unsigned long)snake.score);

    /* Top border */
    zoom_printf(shell, "  +");
    for (int x = 0; x < SNAKE_W; x++) zoom_printf(shell, "--");
    zoom_printf(shell, "+\r\n");

    for (int y = 0; y < SNAKE_H; y++) {
        zoom_printf(shell, "  |");
        for (int x = 0; x < SNAKE_W; x++) {
            if (x == snake.food_x && y == snake.food_y)
                zoom_printf(shell, "@@");
            else if (snake.board[x][y])
                zoom_printf(shell, "##");
            else
                zoom_printf(shell, "  ");
        }
        zoom_printf(shell, "|\r\n");
    }

    /* Bottom border */
    zoom_printf(shell, "  +");
    for (int x = 0; x < SNAKE_W; x++) zoom_printf(shell, "--");
    zoom_printf(shell, "+\r\n");

    zoom_printf(shell, "  wasd=move q=quit\r\n");
}

static int snake_step(zoom_shell_t *shell)
{
    int16_t hx = snake.x[snake.head];
    int16_t hy = snake.y[snake.head];
    int16_t nx = hx + snake.dx;
    int16_t ny = hy + snake.dy;

    /* Wall collision */
    if (nx < 0 || nx >= SNAKE_W || ny < 0 || ny >= SNAKE_H) return 0;
    /* Self collision */
    if (snake.board[nx][ny]) return 0;

    int ate = (nx == snake.food_x && ny == snake.food_y);

    /* Move head */
    snake.head = (snake.head + 1) % SNAKE_MAX_LEN;
    snake.x[snake.head] = nx;
    snake.y[snake.head] = ny;
    snake.board[nx][ny] = 1;

    if (ate) {
        snake.len++;
        snake.score += 10;
        snake_place_food();
    } else {
        /* Remove tail */
        int tail = (snake.head - snake.len + SNAKE_MAX_LEN) % SNAKE_MAX_LEN;
        snake.board[snake.x[tail]][snake.y[tail]] = 0;
    }

    snake_draw(shell);
    return 1;
}

static int cmd_snake(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;

    snake_rand_state = ZOOM_GET_TICK() ^ 0xBEEF;
    if (snake_rand_state == 0) snake_rand_state = 99;

    zoom_printf(shell, "\033[?25l\033[2J");
    snake_init();
    snake_draw(shell);

    uint32_t last_tick = ZOOM_GET_TICK();
    uint32_t speed = 200;

    while (1) {
        char c = 0;
        int16_t got = shell->read ? shell->read(&c, 1) : 0;

        if (got > 0) {
            switch (c) {
            case 'w': case 'A': if (snake.dy == 0) { snake.dx = 0; snake.dy = -1; } break;
            case 's': case 'B': if (snake.dy == 0) { snake.dx = 0; snake.dy = 1;  } break;
            case 'a': case 'D': if (snake.dx == 0) { snake.dx = -1; snake.dy = 0; } break;
            case 'd': case 'C': if (snake.dx == 0) { snake.dx = 1;  snake.dy = 0; } break;
            case 'q': goto done;
            }
        }

        uint32_t now = ZOOM_GET_TICK();
        if (now - last_tick >= speed) {
            last_tick = now;
            if (!snake_step(shell)) {
                zoom_printf(shell, "\r\n  GAME OVER! Score: %lu\r\n", (unsigned long)snake.score);
                /* Wait for key to exit */
                if (shell->read) shell->read(&c, 1);
                break;
            }
        }
    }

done:
    zoom_printf(shell, "\033[?25h\033[m");
    return 0;
}

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(snake, cmd_snake, "Play Snake game",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

#endif /* ZOOM_USING_GAME */
