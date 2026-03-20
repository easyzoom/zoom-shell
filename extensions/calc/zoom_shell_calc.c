/**
 * @file zoom_shell_calc.c
 * @brief Zoom Shell 表达式求值器
 *
 * 递归下降解析器，支持 + - * / % & | ^ ~ << >> ()，十六进制，二进制输出
 * 命令: calc <expression>
 */

#include "zoom_shell.h"

#if ZOOM_USING_CALC

typedef struct {
    const char *pos;
    int error;
} calc_ctx_t;

static void skip_spaces(calc_ctx_t *ctx)
{
    while (*ctx->pos == ' ') ctx->pos++;
}

static int32_t parse_expr(calc_ctx_t *ctx);

static int32_t parse_number(calc_ctx_t *ctx)
{
    skip_spaces(ctx);
    int neg = 0;
    int32_t val = 0;

    if (*ctx->pos == '-') { neg = 1; ctx->pos++; skip_spaces(ctx); }

    if (ctx->pos[0] == '0' && (ctx->pos[1] == 'x' || ctx->pos[1] == 'X')) {
        ctx->pos += 2;
        while (1) {
            char c = *ctx->pos;
            if (c >= '0' && c <= '9')      { val = val * 16 + (c - '0'); ctx->pos++; }
            else if (c >= 'a' && c <= 'f')  { val = val * 16 + (c - 'a' + 10); ctx->pos++; }
            else if (c >= 'A' && c <= 'F')  { val = val * 16 + (c - 'A' + 10); ctx->pos++; }
            else break;
        }
    } else if (ctx->pos[0] == '0' && (ctx->pos[1] == 'b' || ctx->pos[1] == 'B')) {
        ctx->pos += 2;
        while (*ctx->pos == '0' || *ctx->pos == '1') {
            val = val * 2 + (*ctx->pos - '0');
            ctx->pos++;
        }
    } else {
        if (*ctx->pos < '0' || *ctx->pos > '9') { ctx->error = 1; return 0; }
        while (*ctx->pos >= '0' && *ctx->pos <= '9') {
            val = val * 10 + (*ctx->pos - '0');
            ctx->pos++;
        }
    }
    return neg ? -val : val;
}

static int32_t parse_primary(calc_ctx_t *ctx)
{
    skip_spaces(ctx);
    if (*ctx->pos == '(') {
        ctx->pos++;
        int32_t val = parse_expr(ctx);
        skip_spaces(ctx);
        if (*ctx->pos == ')') ctx->pos++;
        else ctx->error = 1;
        return val;
    }
    if (*ctx->pos == '~') {
        ctx->pos++;
        return ~parse_primary(ctx);
    }
    return parse_number(ctx);
}

static int32_t parse_unary(calc_ctx_t *ctx)
{
    return parse_primary(ctx);
}

static int32_t parse_mul(calc_ctx_t *ctx)
{
    int32_t val = parse_unary(ctx);
    while (!ctx->error) {
        skip_spaces(ctx);
        char op = *ctx->pos;
        if (op == '*' || op == '/' || op == '%') {
            ctx->pos++;
            int32_t r = parse_unary(ctx);
            if (op == '*') val *= r;
            else if (op == '/') { if (r == 0) { ctx->error = 1; return 0; } val /= r; }
            else { if (r == 0) { ctx->error = 1; return 0; } val %= r; }
        } else break;
    }
    return val;
}

static int32_t parse_shift(calc_ctx_t *ctx)
{
    int32_t val = parse_mul(ctx);
    while (!ctx->error) {
        skip_spaces(ctx);
        if (ctx->pos[0] == '<' && ctx->pos[1] == '<') {
            ctx->pos += 2; val <<= parse_mul(ctx);
        } else if (ctx->pos[0] == '>' && ctx->pos[1] == '>') {
            ctx->pos += 2; val >>= parse_mul(ctx);
        } else break;
    }
    return val;
}

static int32_t parse_add(calc_ctx_t *ctx)
{
    int32_t val = parse_shift(ctx);
    while (!ctx->error) {
        skip_spaces(ctx);
        char op = *ctx->pos;
        if (op == '+') { ctx->pos++; val += parse_shift(ctx); }
        else if (op == '-') { ctx->pos++; val -= parse_shift(ctx); }
        else break;
    }
    return val;
}

static int32_t parse_bitand(calc_ctx_t *ctx)
{
    int32_t val = parse_add(ctx);
    while (!ctx->error) {
        skip_spaces(ctx);
        if (*ctx->pos == '&') { ctx->pos++; val &= parse_add(ctx); }
        else break;
    }
    return val;
}

static int32_t parse_bitxor(calc_ctx_t *ctx)
{
    int32_t val = parse_bitand(ctx);
    while (!ctx->error) {
        skip_spaces(ctx);
        if (*ctx->pos == '^') { ctx->pos++; val ^= parse_bitand(ctx); }
        else break;
    }
    return val;
}

static int32_t parse_expr(calc_ctx_t *ctx)
{
    int32_t val = parse_bitxor(ctx);
    while (!ctx->error) {
        skip_spaces(ctx);
        if (*ctx->pos == '|') { ctx->pos++; val |= parse_bitxor(ctx); }
        else break;
    }
    return val;
}

static void print_binary(zoom_shell_t *shell, uint32_t val)
{
    zoom_printf(shell, "0b");
    int started = 0;
    for (int i = 31; i >= 0; i--) {
        if (val & ((uint32_t)1 << i)) { started = 1; zoom_printf(shell, "1"); }
        else if (started) zoom_printf(shell, "0");
    }
    if (!started) zoom_printf(shell, "0");
}

static int cmd_calc(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 1) {
        zoom_error(shell, "Usage: calc <expression>\r\n");
        zoom_printf(shell, "  Supports: + - * / %% & | ^ ~ << >> ()\r\n");
        zoom_printf(shell, "  Example: calc 0xFF & (3 + 5) << 2\r\n");
        return -1;
    }

    /* Concatenate all args into one expression string */
    char expr_buf[ZOOM_CMD_MAX_LENGTH];
    int pos = 0;
    for (int i = 0; i < argc && pos < ZOOM_CMD_MAX_LENGTH - 2; i++) {
        if (i > 0) expr_buf[pos++] = ' ';
        const char *s = argv[i];
        while (*s && pos < ZOOM_CMD_MAX_LENGTH - 1) expr_buf[pos++] = *s++;
    }
    expr_buf[pos] = '\0';

    calc_ctx_t ctx = { .pos = expr_buf, .error = 0 };
    int32_t result = parse_expr(&ctx);

    if (ctx.error) {
        zoom_error(shell, "Syntax error in expression\r\n");
        return -1;
    }

    zoom_printf(shell, "  = %ld (0x%lx, ", (long)result, (unsigned long)(uint32_t)result);
    print_binary(shell, (uint32_t)result);
    zoom_printf(shell, ")\r\n");
    return 0;
}

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(calc, cmd_calc, "Expression calculator",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

#endif /* ZOOM_USING_CALC */
