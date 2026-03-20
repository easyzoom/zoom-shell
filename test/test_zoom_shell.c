/**
 * @file test_zoom_shell.c
 * @brief Zoom Shell 自动化测试
 *
 * 使用 mock I/O 捕获输出，通过 zoom_shell_exec / zoom_shell_input
 * 驱动 shell 执行命令并验证结果。
 *
 * 编译: make test
 * 运行: ./build/zoom_shell_test
 */

#include "zoom_shell.h"
#if ZOOM_USING_LOG
#include "zoom_shell_log.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 *  简易测试框架
 * ================================================================ */

static int g_tests_run    = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) do {                         \
    g_tests_run++;                                          \
    if (cond) { g_tests_passed++; }                         \
    else {                                                  \
        g_tests_failed++;                                   \
        printf("  FAIL [%s:%d]: %s\n", __FILE__, __LINE__, msg); \
    }                                                       \
} while (0)

#define TEST_ASSERT_EQ(a, b, msg) TEST_ASSERT((a) == (b), msg)
#define TEST_ASSERT_NE(a, b, msg) TEST_ASSERT((a) != (b), msg)
#define TEST_ASSERT_STR_CONTAINS(haystack, needle, msg) \
    TEST_ASSERT(strstr((haystack), (needle)) != NULL, msg)

#define RUN_TEST(fn) do {                       \
    printf("  [TEST] %s\n", #fn);               \
    fn();                                        \
} while (0)

/* ================================================================
 *  Mock I/O 系统
 * ================================================================ */

#define OUTPUT_BUF_SIZE 8192
static char g_output_buf[OUTPUT_BUF_SIZE];
static int  g_output_pos = 0;

static const char *g_input_data = NULL;
static int         g_input_pos  = 0;

static void mock_output_reset(void)
{
    g_output_pos = 0;
    g_output_buf[0] = '\0';
}

static const char *mock_output_get(void)
{
    g_output_buf[g_output_pos] = '\0';
    return g_output_buf;
}

static int16_t mock_write(const char *data, uint16_t len)
{
    for (uint16_t i = 0; i < len && g_output_pos < OUTPUT_BUF_SIZE - 1; i++) {
        g_output_buf[g_output_pos++] = data[i];
    }
    g_output_buf[g_output_pos] = '\0';
    return (int16_t)len;
}

static int16_t mock_read(char *data, uint16_t len)
{
    (void)len;
    if (!g_input_data || g_input_data[g_input_pos] == '\0') return 0;
    *data = g_input_data[g_input_pos++];
    return 1;
}

static void mock_input_set(const char *input)
{
    g_input_data = input;
    g_input_pos = 0;
}

/* ================================================================
 *  测试用命令和变量
 * ================================================================ */

static int g_cmd_called = 0;
static int g_cmd_last_argc = 0;

static int cmd_test_simple(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argv;
    g_cmd_called++;
    g_cmd_last_argc = argc;
    zoom_printf(shell, "test_ok\r\n");
    return 0;
}

static int cmd_test_retval(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)shell; (void)argc; (void)argv;
    return -42;
}

static int cmd_sub_a(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    zoom_printf(shell, "sub_a_ok\r\n");
    return 0;
}

static int cmd_sub_b(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc;
    if (argv && argv[0])
        zoom_printf(shell, "sub_b_arg=%s\r\n", argv[0]);
    else
        zoom_printf(shell, "sub_b_noarg\r\n");
    return 0;
}

static int cmd_deep(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    zoom_printf(shell, "deep_ok\r\n");
    return 0;
}

static int cmd_admin_only(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    zoom_printf(shell, "admin_ok\r\n");
    return 0;
}

static int cmd_hidden(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    zoom_printf(shell, "hidden_ok\r\n");
    return 0;
}

/* 三层子命令: parent -> mid -> deep_leaf */
ZOOM_SUBCMD_SET(sub_deep,
    ZOOM_SUBCMD(leaf, cmd_deep, "Deep leaf command"),
);

ZOOM_SUBCMD_SET(sub_mid,
    ZOOM_SUBCMD(a, cmd_sub_a, "Sub A"),
    ZOOM_SUBCMD(b, cmd_sub_b, "Sub B"),
    ZOOM_SUBCMD_WITH_SUB(mid, sub_deep, "Mid level"),
);

ZOOM_EXPORT_CMD(tsimple, cmd_test_simple, "Simple test cmd",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
ZOOM_EXPORT_CMD(tretval, cmd_test_retval, "Returns -42",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
ZOOM_EXPORT_CMD_WITH_SUB(tparent, sub_mid, "Subcmd test",
                          ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
ZOOM_EXPORT_CMD(tadmin, cmd_admin_only, "Admin only",
                ZOOM_ATTR_ADMIN, ZOOM_USER_ADMIN);
ZOOM_EXPORT_CMD(thidden, cmd_hidden, "Hidden cmd",
                ZOOM_ATTR_HIDDEN, ZOOM_USER_GUEST);

/* 测试变量 */
static int8_t   tv_i8  = -42;
static int16_t  tv_i16 = -1000;
static int32_t  tv_i32 = 100000;
static uint8_t  tv_u8  = 255;
static uint16_t tv_u16 = 65535;
static uint32_t tv_u32 = 1000000;
static float    tv_f   = 3.14f;
static uint8_t  tv_bool = 1;
static int32_t  tv_ro  = 999;

ZOOM_EXPORT_VAR(vi8,   tv_i8,   ZOOM_VAR_INT8,   "test i8",   ZOOM_VAR_RW, ZOOM_USER_GUEST);
ZOOM_EXPORT_VAR(vi16,  tv_i16,  ZOOM_VAR_INT16,  "test i16",  ZOOM_VAR_RW, ZOOM_USER_GUEST);
ZOOM_EXPORT_VAR(vi32,  tv_i32,  ZOOM_VAR_INT32,  "test i32",  ZOOM_VAR_RW, ZOOM_USER_GUEST);
ZOOM_EXPORT_VAR(vu8,   tv_u8,   ZOOM_VAR_UINT8,  "test u8",   ZOOM_VAR_RW, ZOOM_USER_GUEST);
ZOOM_EXPORT_VAR(vu16,  tv_u16,  ZOOM_VAR_UINT16, "test u16",  ZOOM_VAR_RW, ZOOM_USER_GUEST);
ZOOM_EXPORT_VAR(vu32,  tv_u32,  ZOOM_VAR_UINT32, "test u32",  ZOOM_VAR_RW, ZOOM_USER_GUEST);
ZOOM_EXPORT_VAR(vf,    tv_f,    ZOOM_VAR_FLOAT,  "test float",ZOOM_VAR_RW, ZOOM_USER_GUEST);
ZOOM_EXPORT_VAR(vbool, tv_bool, ZOOM_VAR_BOOL,   "test bool", ZOOM_VAR_RW, ZOOM_USER_GUEST);
ZOOM_EXPORT_VAR(vro,   tv_ro,   ZOOM_VAR_INT32,  "read only", ZOOM_VAR_RO, ZOOM_USER_GUEST);

/* 测试用户 */
ZOOM_EXPORT_USER(tester, "pass123", ZOOM_USER_USER);
ZOOM_EXPORT_USER(admin,  "admin",   ZOOM_USER_ADMIN);

/* ================================================================
 *  辅助函数
 * ================================================================ */

static zoom_shell_t g_shell;
static char g_buffer[768];  /* 128 * (5+1) */

static void setup_shell(void)
{
    memset(&g_shell, 0, sizeof(g_shell));
    g_shell.read  = mock_read;
    g_shell.write = mock_write;
    mock_output_reset();
    mock_input_set("");
    g_cmd_called = 0;
    g_cmd_last_argc = 0;

    /* 复原测试变量 */
    tv_i8  = -42;
    tv_i16 = -1000;
    tv_i32 = 100000;
    tv_u8  = 255;
    tv_u16 = 65535;
    tv_u32 = 1000000;
    tv_f   = 3.14f;
    tv_bool = 1;
    tv_ro  = 999;

    int ret = zoom_shell_init(&g_shell, g_buffer, sizeof(g_buffer));
    TEST_ASSERT_EQ(ret, 0, "shell init should succeed");
}

static void teardown_shell(void)
{
    zoom_shell_deinit(&g_shell);
}

/** 执行命令并返回输出（剥离 ANSI 转义前缀） */
static const char *exec_and_capture(const char *cmd)
{
    mock_output_reset();
    zoom_shell_exec(&g_shell, cmd);
    return mock_output_get();
}

/** 通过逐字节 input 喂入一行命令（模拟键盘输入） */
static const char *input_line_and_capture(const char *line)
{
    mock_output_reset();
    for (int i = 0; line[i]; i++) {
        zoom_shell_input(&g_shell, line[i]);
    }
    zoom_shell_input(&g_shell, '\r');
    return mock_output_get();
}

/* ================================================================
 *  测试: 初始化
 * ================================================================ */

static void test_init_null_params(void)
{
    TEST_ASSERT_EQ(zoom_shell_init(NULL, g_buffer, sizeof(g_buffer)), -1,
                   "init with NULL shell should fail");

    zoom_shell_t sh;
    memset(&sh, 0, sizeof(sh));
    TEST_ASSERT_EQ(zoom_shell_init(&sh, g_buffer, sizeof(g_buffer)), -1,
                   "init without read/write should fail");
}

static void test_init_buffer_too_small(void)
{
    zoom_shell_t sh;
    memset(&sh, 0, sizeof(sh));
    sh.read  = mock_read;
    sh.write = mock_write;
    char tiny[10];
    TEST_ASSERT_EQ(zoom_shell_init(&sh, tiny, sizeof(tiny)), -1,
                   "init with tiny buffer should fail");
}

static void test_init_success(void)
{
    setup_shell();
    TEST_ASSERT_EQ(g_shell.state, ZOOM_STATE_RUNNING, "state should be RUNNING after init");
    TEST_ASSERT(g_shell.cmdList.count > 0, "should have commands loaded");
    TEST_ASSERT(g_shell.varList.count > 0, "should have vars loaded");
    TEST_ASSERT(g_shell.userCount > 0, "should have users loaded");
    teardown_shell();
}

/* ================================================================
 *  测试: 命令执行
 * ================================================================ */

static void test_exec_simple_command(void)
{
    setup_shell();
    g_cmd_called = 0;
    const char *out = exec_and_capture("tsimple");
    TEST_ASSERT_EQ(g_cmd_called, 1, "command should be called once");
    TEST_ASSERT_EQ(g_cmd_last_argc, 0, "argc should be 0");
    TEST_ASSERT_STR_CONTAINS(out, "test_ok", "output should contain test_ok");
    teardown_shell();
}

static void test_exec_with_args(void)
{
    setup_shell();
    g_cmd_called = 0;
    exec_and_capture("tsimple arg1 arg2 arg3");
    TEST_ASSERT_EQ(g_cmd_called, 1, "command should be called");
    TEST_ASSERT_EQ(g_cmd_last_argc, 3, "argc should be 3");
    teardown_shell();
}

static void test_exec_quoted_args(void)
{
    setup_shell();
    g_cmd_called = 0;
    exec_and_capture("tsimple \"hello world\" arg2");
    TEST_ASSERT_EQ(g_cmd_last_argc, 2, "quoted string should be one arg");
    teardown_shell();
}

static void test_exec_unknown_command(void)
{
    setup_shell();
    const char *out = exec_and_capture("nonexistent_cmd");
    TEST_ASSERT_STR_CONTAINS(out, "Unknown command", "should show error");
    teardown_shell();
}

static void test_exec_empty_line(void)
{
    setup_shell();
    int ret = zoom_shell_exec(&g_shell, "");
    TEST_ASSERT_EQ(ret, 0, "empty line should return 0");
    ret = zoom_shell_exec(&g_shell, "   ");
    TEST_ASSERT_EQ(ret, 0, "whitespace-only line should return 0");
    teardown_shell();
}

static void test_exec_return_value(void)
{
    setup_shell();
    int ret = zoom_shell_exec(&g_shell, "tretval");
    TEST_ASSERT_EQ(ret, -42, "should propagate command return value");
    teardown_shell();
}

/* ================================================================
 *  测试: 子命令树
 * ================================================================ */

static void test_subcmd_basic(void)
{
    setup_shell();
    const char *out = exec_and_capture("tparent a");
    TEST_ASSERT_STR_CONTAINS(out, "sub_a_ok", "subcmd a should work");
    teardown_shell();
}

static void test_subcmd_with_args(void)
{
    setup_shell();
    const char *out = exec_and_capture("tparent b myarg");
    TEST_ASSERT_STR_CONTAINS(out, "sub_b_arg=myarg", "subcmd b should get arg");
    teardown_shell();
}

static void test_subcmd_deep(void)
{
    setup_shell();
    const char *out = exec_and_capture("tparent mid leaf");
    TEST_ASSERT_STR_CONTAINS(out, "deep_ok", "3-level subcmd should work");
    teardown_shell();
}

static void test_subcmd_container_lists_children(void)
{
    setup_shell();
    const char *out = exec_and_capture("tparent");
    TEST_ASSERT_STR_CONTAINS(out, "Subcommands", "container should list subcmds");
    TEST_ASSERT_STR_CONTAINS(out, "Sub A", "should list subcmd a");
    TEST_ASSERT_STR_CONTAINS(out, "Sub B", "should list subcmd b");
    teardown_shell();
}

/* ================================================================
 *  测试: 用户 / 权限系统
 * ================================================================ */

static void test_user_default_is_guest(void)
{
    setup_shell();
    uint8_t level = zoom_shell_get_current_level(&g_shell);
    TEST_ASSERT_EQ(level, ZOOM_USER_GUEST, "default should be guest");
    teardown_shell();
}

static void test_user_login_success(void)
{
    setup_shell();
    int ret = zoom_shell_login(&g_shell, "tester", "pass123");
    TEST_ASSERT_EQ(ret, 0, "login with correct creds should succeed");
    TEST_ASSERT_EQ(zoom_shell_get_current_level(&g_shell), ZOOM_USER_USER,
                   "level should be USER after login");
    teardown_shell();
}

static void test_user_login_wrong_password(void)
{
    setup_shell();
    int ret = zoom_shell_login(&g_shell, "tester", "wrong");
    TEST_ASSERT_NE(ret, 0, "login with wrong password should fail");
    TEST_ASSERT_EQ(zoom_shell_get_current_level(&g_shell), ZOOM_USER_GUEST,
                   "should remain guest");
    teardown_shell();
}

static void test_user_login_nonexistent(void)
{
    setup_shell();
    int ret = zoom_shell_login(&g_shell, "nobody", "pass");
    TEST_ASSERT_NE(ret, 0, "login as nonexistent user should fail");
    teardown_shell();
}

static void test_user_logout(void)
{
    setup_shell();
    zoom_shell_login(&g_shell, "admin", "admin");
    TEST_ASSERT_EQ(zoom_shell_get_current_level(&g_shell), ZOOM_USER_ADMIN,
                   "should be admin");
    zoom_shell_logout(&g_shell);
    TEST_ASSERT_EQ(zoom_shell_get_current_level(&g_shell), ZOOM_USER_GUEST,
                   "should be guest after logout");
    teardown_shell();
}

static void test_user_permission_denied(void)
{
    setup_shell();
    const char *out = exec_and_capture("tadmin");
    TEST_ASSERT_STR_CONTAINS(out, "Permission denied",
                             "guest should not run admin cmd");
    teardown_shell();
}

static void test_user_permission_granted(void)
{
    setup_shell();
    zoom_shell_login(&g_shell, "admin", "admin");
    const char *out = exec_and_capture("tadmin");
    TEST_ASSERT_STR_CONTAINS(out, "admin_ok",
                             "admin should run admin cmd");
    teardown_shell();
}

static void test_user_add_runtime(void)
{
    setup_shell();
    int initial = g_shell.userCount;
    int ret = zoom_shell_add_user(&g_shell, "newuser", "newpass", ZOOM_USER_USER);
    TEST_ASSERT_EQ(ret, 0, "add_user should succeed");
    TEST_ASSERT_EQ(g_shell.userCount, initial + 1, "count should increase");

    ret = zoom_shell_login(&g_shell, "newuser", "newpass");
    TEST_ASSERT_EQ(ret, 0, "should login with new user");
    teardown_shell();
}

static void test_user_add_duplicate(void)
{
    setup_shell();
    int ret = zoom_shell_add_user(&g_shell, "admin", "admin2", ZOOM_USER_USER);
    TEST_ASSERT_NE(ret, 0, "adding duplicate user should fail");
    teardown_shell();
}

/* ================================================================
 *  测试: 变量观测系统
 * ================================================================ */

static void test_var_find(void)
{
    setup_shell();
    const zoom_var_t *v = zoom_shell_find_var(&g_shell, "vi32");
    TEST_ASSERT(v != NULL, "should find vi32");
    TEST_ASSERT(zoom_shell_find_var(&g_shell, "nonexist") == NULL,
                "should not find nonexist");
    teardown_shell();
}

static void test_var_get_int_types(void)
{
    setup_shell();
    char buf[32];

    zoom_shell_var_get_str(zoom_shell_find_var(&g_shell, "vi8"), buf, sizeof(buf));
    TEST_ASSERT(strcmp(buf, "-42") == 0, "i8 should be -42");

    zoom_shell_var_get_str(zoom_shell_find_var(&g_shell, "vi16"), buf, sizeof(buf));
    TEST_ASSERT(strcmp(buf, "-1000") == 0, "i16 should be -1000");

    zoom_shell_var_get_str(zoom_shell_find_var(&g_shell, "vi32"), buf, sizeof(buf));
    TEST_ASSERT(strcmp(buf, "100000") == 0, "i32 should be 100000");

    zoom_shell_var_get_str(zoom_shell_find_var(&g_shell, "vu8"), buf, sizeof(buf));
    TEST_ASSERT(strcmp(buf, "255") == 0, "u8 should be 255");

    zoom_shell_var_get_str(zoom_shell_find_var(&g_shell, "vu16"), buf, sizeof(buf));
    TEST_ASSERT(strcmp(buf, "65535") == 0, "u16 should be 65535");

    zoom_shell_var_get_str(zoom_shell_find_var(&g_shell, "vu32"), buf, sizeof(buf));
    TEST_ASSERT(strcmp(buf, "1000000") == 0, "u32 should be 1000000");

    teardown_shell();
}

static void test_var_get_float(void)
{
    setup_shell();
    char buf[32];
    zoom_shell_var_get_str(zoom_shell_find_var(&g_shell, "vf"), buf, sizeof(buf));
    TEST_ASSERT(strcmp(buf, "3.140") == 0, "float should be 3.140");
    teardown_shell();
}

static void test_var_get_bool(void)
{
    setup_shell();
    char buf[32];
    zoom_shell_var_get_str(zoom_shell_find_var(&g_shell, "vbool"), buf, sizeof(buf));
    TEST_ASSERT(strcmp(buf, "1") == 0, "bool should be 1");
    teardown_shell();
}

static void test_var_set_int(void)
{
    setup_shell();
    const zoom_var_t *v = zoom_shell_find_var(&g_shell, "vi32");
    zoom_shell_var_set_str(v, "42");
    TEST_ASSERT_EQ(tv_i32, 42, "i32 should be set to 42");

    zoom_shell_var_set_str(v, "-100");
    TEST_ASSERT_EQ(tv_i32, -100, "i32 should be set to -100");

    zoom_shell_var_set_str(v, "0xFF");
    TEST_ASSERT_EQ(tv_i32, 255, "i32 should parse hex 0xFF as 255");
    teardown_shell();
}

static void test_var_set_float(void)
{
    setup_shell();
    const zoom_var_t *v = zoom_shell_find_var(&g_shell, "vf");
    zoom_shell_var_set_str(v, "2.718");
    float diff = tv_f - 2.718f;
    if (diff < 0) diff = -diff;
    TEST_ASSERT(diff < 0.01f, "float should be ~2.718");
    teardown_shell();
}

static void test_var_set_bool(void)
{
    setup_shell();
    const zoom_var_t *v = zoom_shell_find_var(&g_shell, "vbool");
    zoom_shell_var_set_str(v, "0");
    TEST_ASSERT_EQ(tv_bool, 0, "bool should be 0");
    zoom_shell_var_set_str(v, "true");
    TEST_ASSERT_EQ(tv_bool, 1, "bool 'true' should be 1");
    zoom_shell_var_set_str(v, "1");
    TEST_ASSERT_EQ(tv_bool, 1, "bool '1' should be 1");
    teardown_shell();
}

static void test_var_readonly_protection(void)
{
    setup_shell();
    const zoom_var_t *v = zoom_shell_find_var(&g_shell, "vro");
    int ret = zoom_shell_var_set_str(v, "123");
    TEST_ASSERT_NE(ret, 0, "setting RO var should fail");
    TEST_ASSERT_EQ(tv_ro, 999, "RO var value should be unchanged");
    teardown_shell();
}

static void test_var_cmd_list(void)
{
    setup_shell();
    const char *out = exec_and_capture("var list");
    TEST_ASSERT_STR_CONTAINS(out, "vi32", "var list should show vi32");
    TEST_ASSERT_STR_CONTAINS(out, "vf", "var list should show vf");
    TEST_ASSERT_STR_CONTAINS(out, "vro", "var list should show vro");
    TEST_ASSERT_STR_CONTAINS(out, "[RO]", "should show RO attribute");
    TEST_ASSERT_STR_CONTAINS(out, "[RW]", "should show RW attribute");
    teardown_shell();
}

static void test_var_cmd_get(void)
{
    setup_shell();
    const char *out = exec_and_capture("var get vi32");
    TEST_ASSERT_STR_CONTAINS(out, "vi32", "should show var name");
    TEST_ASSERT_STR_CONTAINS(out, "100000", "should show value");
    teardown_shell();
}

static void test_var_cmd_set(void)
{
    setup_shell();
    exec_and_capture("var set vi32 42");
    TEST_ASSERT_EQ(tv_i32, 42, "var set via command should work");
    teardown_shell();
}

static void test_var_cmd_set_readonly(void)
{
    setup_shell();
    const char *out = exec_and_capture("var set vro 123");
    TEST_ASSERT_STR_CONTAINS(out, "read-only", "should reject RO write");
    TEST_ASSERT_EQ(tv_ro, 999, "RO value should be unchanged");
    teardown_shell();
}

/* ================================================================
 *  测试: 逐字节输入 (zoom_shell_input)
 * ================================================================ */

static void test_input_basic_line(void)
{
    setup_shell();
    g_cmd_called = 0;
    /* 先显示 prompt */
    mock_output_reset();
    zoom_shell_show_prompt(&g_shell);
    mock_output_reset();

    const char *out = input_line_and_capture("tsimple");
    TEST_ASSERT_EQ(g_cmd_called, 1, "input line should trigger command");
    TEST_ASSERT_STR_CONTAINS(out, "test_ok", "should see output");
    teardown_shell();
}

static void test_input_backspace(void)
{
    setup_shell();
    g_cmd_called = 0;
    mock_output_reset();

    /* 输入 "tsimplx" 然后退格再输入 "e" */
    const char *input = "tsimplx\x7F" "e";
    for (int i = 0; input[i]; i++) {
        zoom_shell_input(&g_shell, input[i]);
    }
    zoom_shell_input(&g_shell, '\r');
    TEST_ASSERT_EQ(g_cmd_called, 1, "backspace-corrected input should work");
    teardown_shell();
}

static void test_input_empty_enter(void)
{
    setup_shell();
    g_cmd_called = 0;
    mock_output_reset();
    zoom_shell_input(&g_shell, '\r');
    TEST_ASSERT_EQ(g_cmd_called, 0, "empty enter should not execute command");
    teardown_shell();
}

/* ================================================================
 *  测试: 历史记录
 * ================================================================ */

static void test_history_records_commands(void)
{
    setup_shell();
    zoom_shell_exec(&g_shell, "tsimple");

    /* 直接检查内部历史记录状态 */
    zoom_shell_input(&g_shell, '\r');  /* 先空回车刷新状态 */

    /* 输入并执行两条命令 */
    input_line_and_capture("tsimple");
    input_line_and_capture("version");

    TEST_ASSERT(g_shell.history.number >= 2, "should have at least 2 history entries");
    teardown_shell();
}

static void test_history_dedup(void)
{
    setup_shell();

    input_line_and_capture("tsimple");
    uint16_t count1 = g_shell.history.number;
    input_line_and_capture("tsimple");
    uint16_t count2 = g_shell.history.number;

    TEST_ASSERT_EQ(count1, count2, "duplicate command should not add history");
    teardown_shell();
}

static void test_history_cmd_output(void)
{
    setup_shell();
    input_line_and_capture("tsimple");
    input_line_and_capture("version");

    const char *out = exec_and_capture("history");
    TEST_ASSERT_STR_CONTAINS(out, "tsimple", "history should show tsimple");
    TEST_ASSERT_STR_CONTAINS(out, "version", "history should show version");
    teardown_shell();
}

/* ================================================================
 *  测试: 内置命令
 * ================================================================ */

static void test_builtin_help(void)
{
    setup_shell();
    const char *out = exec_and_capture("help");
    TEST_ASSERT_STR_CONTAINS(out, "Available commands", "help should show header");
    TEST_ASSERT_STR_CONTAINS(out, "help", "help should list itself");
    TEST_ASSERT_STR_CONTAINS(out, "version", "help should list version");
    TEST_ASSERT_STR_CONTAINS(out, "var", "help should list var");

    /* 隐藏命令不应出现 */
    char *hidden_pos = strstr(out, "thidden");
    TEST_ASSERT(hidden_pos == NULL, "hidden cmd should not appear in help");
    teardown_shell();
}

static void test_builtin_help_specific(void)
{
    setup_shell();
    const char *out = exec_and_capture("help tparent");
    TEST_ASSERT_STR_CONTAINS(out, "tparent", "should show cmd name");
    TEST_ASSERT_STR_CONTAINS(out, "Sub A", "should show subcmds");
    teardown_shell();
}

static void test_builtin_version(void)
{
    setup_shell();
    const char *out = exec_and_capture("version");
    TEST_ASSERT_STR_CONTAINS(out, "1.0.0", "version should show 1.0.0");
    teardown_shell();
}

static void test_builtin_echo(void)
{
    setup_shell();
    const char *out = exec_and_capture("echo hello world 123");
    TEST_ASSERT_STR_CONTAINS(out, "hello world 123", "echo should output args");
    teardown_shell();
}

static void test_builtin_stats(void)
{
    setup_shell();
    exec_and_capture("tsimple");
    exec_and_capture("tsimple");
    const char *out = exec_and_capture("stats");
    TEST_ASSERT_STR_CONTAINS(out, "Commands executed", "should show stats");
    teardown_shell();
}

static void test_builtin_exit(void)
{
    setup_shell();
    int ret = zoom_shell_exec(&g_shell, "exit");
    TEST_ASSERT_EQ(ret, 1, "exit should return 1");
    teardown_shell();
}

/* ================================================================
 *  测试: printf 格式化
 * ================================================================ */

static void test_printf_basic(void)
{
    setup_shell();
    mock_output_reset();
    zoom_printf(&g_shell, "hello %s %d", "world", 42);
    const char *out = mock_output_get();
    TEST_ASSERT_STR_CONTAINS(out, "hello world 42", "basic printf");
    teardown_shell();
}

static void test_printf_width_right_align(void)
{
    setup_shell();
    mock_output_reset();
    zoom_printf(&g_shell, "[%10s]", "hi");
    const char *out = mock_output_get();
    TEST_ASSERT_STR_CONTAINS(out, "[        hi]", "right-aligned %10s");
    teardown_shell();
}

static void test_printf_width_left_align(void)
{
    setup_shell();
    mock_output_reset();
    zoom_printf(&g_shell, "[%-10s]", "hi");
    const char *out = mock_output_get();
    TEST_ASSERT_STR_CONTAINS(out, "[hi        ]", "left-aligned %-10s");
    teardown_shell();
}

static void test_printf_zero_pad(void)
{
    setup_shell();
    mock_output_reset();
    zoom_printf(&g_shell, "%05d", 42);
    const char *out = mock_output_get();
    TEST_ASSERT_STR_CONTAINS(out, "00042", "zero-padded %05d");
    teardown_shell();
}

static void test_printf_hex(void)
{
    setup_shell();
    mock_output_reset();
    zoom_printf(&g_shell, "0x%x", 0xDEAD);
    const char *out = mock_output_get();
    TEST_ASSERT_STR_CONTAINS(out, "0xdead", "hex output");
    teardown_shell();
}

static void test_printf_negative(void)
{
    setup_shell();
    mock_output_reset();
    zoom_printf(&g_shell, "%d", -12345);
    const char *out = mock_output_get();
    TEST_ASSERT_STR_CONTAINS(out, "-12345", "negative int");
    teardown_shell();
}

static void test_printf_unsigned(void)
{
    setup_shell();
    mock_output_reset();
    zoom_printf(&g_shell, "%u", 4000000000U);
    const char *out = mock_output_get();
    TEST_ASSERT_STR_CONTAINS(out, "4000000000", "large unsigned");
    teardown_shell();
}

static void test_printf_percent(void)
{
    setup_shell();
    mock_output_reset();
    zoom_printf(&g_shell, "100%%");
    const char *out = mock_output_get();
    TEST_ASSERT_STR_CONTAINS(out, "100%", "percent literal");
    teardown_shell();
}

static void test_printf_null_string(void)
{
    setup_shell();
    mock_output_reset();
    zoom_printf(&g_shell, "%s", (const char *)NULL);
    const char *out = mock_output_get();
    TEST_ASSERT_STR_CONTAINS(out, "(null)", "NULL string");
    teardown_shell();
}

/* ================================================================
 *  测试: 边界条件
 * ================================================================ */

static void test_exec_null_params(void)
{
    TEST_ASSERT_EQ(zoom_shell_exec(NULL, "help"), -1, "exec with NULL shell");
    setup_shell();
    TEST_ASSERT_EQ(zoom_shell_exec(&g_shell, NULL), -1, "exec with NULL cmd");
    teardown_shell();
}

static void test_input_when_not_running(void)
{
    setup_shell();
    g_shell.state = ZOOM_STATE_IDLE;
    g_cmd_called = 0;
    zoom_shell_input(&g_shell, 'a');
    zoom_shell_input(&g_shell, '\r');
    TEST_ASSERT_EQ(g_cmd_called, 0, "input when idle should be ignored");
    teardown_shell();
}

static void test_hidden_cmd_still_executable(void)
{
    setup_shell();
    const char *out = exec_and_capture("thidden");
    TEST_ASSERT_STR_CONTAINS(out, "hidden_ok",
                             "hidden cmd should still execute");
    teardown_shell();
}

/* ================================================================
 *  测试: 表达式求值器 (calc)
 * ================================================================ */

static void test_calc_basic_add(void)
{
    setup_shell();
    const char *out = exec_and_capture("calc 3 + 5");
    TEST_ASSERT_STR_CONTAINS(out, "8", "3+5 should be 8");
    teardown_shell();
}

static void test_calc_mul_precedence(void)
{
    setup_shell();
    const char *out = exec_and_capture("calc 2 + 3 * 4");
    TEST_ASSERT_STR_CONTAINS(out, "14", "2+3*4 should be 14");
    teardown_shell();
}

static void test_calc_hex(void)
{
    setup_shell();
    const char *out = exec_and_capture("calc 0xFF");
    TEST_ASSERT_STR_CONTAINS(out, "255", "0xFF should be 255");
    teardown_shell();
}

static void test_calc_bitwise(void)
{
    setup_shell();
    const char *out = exec_and_capture("calc 0xFF & 0x0F");
    TEST_ASSERT_STR_CONTAINS(out, "15", "0xFF & 0x0F should be 15");
    teardown_shell();
}

static void test_calc_shift(void)
{
    setup_shell();
    const char *out = exec_and_capture("calc 1 << 8");
    TEST_ASSERT_STR_CONTAINS(out, "256", "1<<8 should be 256");
    teardown_shell();
}

static void test_calc_parens(void)
{
    setup_shell();
    const char *out = exec_and_capture("calc (2 + 3) * 4");
    TEST_ASSERT_STR_CONTAINS(out, "20", "(2+3)*4 should be 20");
    teardown_shell();
}

static void test_calc_negative(void)
{
    setup_shell();
    const char *out = exec_and_capture("calc -10 + 3");
    /* Result should contain -7 */
    TEST_ASSERT_STR_CONTAINS(out, "-7", "-10+3 should be -7");
    teardown_shell();
}

static void test_calc_division(void)
{
    setup_shell();
    const char *out = exec_and_capture("calc 100 / 7");
    TEST_ASSERT_STR_CONTAINS(out, "14", "100/7 should be 14 (integer)");
    teardown_shell();
}

static void test_calc_modulo(void)
{
    setup_shell();
    const char *out = exec_and_capture("calc 17 % 5");
    TEST_ASSERT_STR_CONTAINS(out, "2", "17%5 should be 2");
    teardown_shell();
}

/* ================================================================
 *  测试: 别名系统
 * ================================================================ */

static void test_alias_add_and_lookup(void)
{
    setup_shell();
    int ret = zoom_shell_alias_add(&g_shell, "gs", "gpio set");
    TEST_ASSERT_EQ(ret, 0, "alias add should succeed");
    const char *val = zoom_shell_alias_lookup(&g_shell, "gs");
    TEST_ASSERT(val != NULL, "alias lookup should find gs");
    TEST_ASSERT(strstr(val, "gpio set") != NULL, "alias value should be gpio set");
    teardown_shell();
}

static void test_alias_remove(void)
{
    setup_shell();
    zoom_shell_alias_add(&g_shell, "test_rm", "some command");
    TEST_ASSERT(zoom_shell_alias_lookup(&g_shell, "test_rm") != NULL, "should exist before remove");
    zoom_shell_alias_remove(&g_shell, "test_rm");
    TEST_ASSERT(zoom_shell_alias_lookup(&g_shell, "test_rm") == NULL, "should not exist after remove");
    teardown_shell();
}

static void test_alias_cmd(void)
{
    setup_shell();
    const char *out = exec_and_capture("alias mytest echo hello");
    TEST_ASSERT_STR_CONTAINS(out, "mytest", "alias cmd should show name");
    const char *val = zoom_shell_alias_lookup(&g_shell, "mytest");
    TEST_ASSERT(val != NULL, "alias should be registered");
    teardown_shell();
}

static void test_alias_nonexistent(void)
{
    setup_shell();
    TEST_ASSERT(zoom_shell_alias_lookup(&g_shell, "nope") == NULL,
                "nonexistent alias should return NULL");
    teardown_shell();
}

/* ================================================================
 *  测试: HexDump
 * ================================================================ */

static void test_hexdump_cmd(void)
{
    setup_shell();
    static uint8_t test_data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x00};
    char addr_str[20];
    /* Convert address to hex string */
    uintptr_t addr = (uintptr_t)test_data;
    int pos = 0;
    addr_str[pos++] = '0';
    addr_str[pos++] = 'x';
    char hex_chars[] = "0123456789abcdef";
    for (int i = (int)(sizeof(uintptr_t) * 2 - 1); i >= 0; i--) {
        addr_str[pos++] = hex_chars[(addr >> (i * 4)) & 0xF];
    }
    addr_str[pos] = '\0';

    char cmd[64];
    int ci = 0;
    const char *prefix = "mem read ";
    while (*prefix) cmd[ci++] = *prefix++;
    const char *as = addr_str;
    while (*as) cmd[ci++] = *as++;
    cmd[ci++] = ' ';
    cmd[ci++] = '6';
    cmd[ci] = '\0';

    const char *out = exec_and_capture(cmd);
    TEST_ASSERT_STR_CONTAINS(out, "48 65 6c 6c 6f", "should show hex bytes");
    TEST_ASSERT_STR_CONTAINS(out, "Hello", "should show ASCII");
    teardown_shell();
}

/* ================================================================
 *  测试: 进度条
 * ================================================================ */

static void test_progress_bar(void)
{
    setup_shell();
    const char *out = exec_and_capture("progress demo");
    TEST_ASSERT_STR_CONTAINS(out, "###", "should show filled portion");
    TEST_ASSERT_STR_CONTAINS(out, "100%", "should show 100%");
    TEST_ASSERT_STR_CONTAINS(out, "CPU", "should show CPU gauge");
    teardown_shell();
}

/* ================================================================
 *  测试: 日志系统
 * ================================================================ */

static void test_log_cmd(void)
{
    setup_shell();
    extern void zoom_log_init(zoom_shell_t *);
    zoom_log_init(&g_shell);
    const char *out = exec_and_capture("log test");
    TEST_ASSERT_STR_CONTAINS(out, "test", "log test should produce output");
    teardown_shell();
}

static void test_log_level(void)
{
    setup_shell();
    extern void zoom_log_init(zoom_shell_t *);
    zoom_log_init(&g_shell);
    exec_and_capture("log level 0");
    extern zoom_log_level_t zoom_log_get_level(void);
    TEST_ASSERT_EQ((int)zoom_log_get_level(), 0, "level should be 0 (ERROR)");
    teardown_shell();
}

/* ================================================================
 *  测试: 按键绑定
 * ================================================================ */

static void test_keybind_init_and_list(void)
{
    setup_shell();
    extern int zoom_shell_keybind_init(zoom_shell_t *);
    zoom_shell_keybind_init(&g_shell);
    TEST_ASSERT(g_shell.keybindCount > 0, "should have default keybinds");
    const char *out = exec_and_capture("keybind list");
    TEST_ASSERT_STR_CONTAINS(out, "Ctrl+L", "should list Ctrl+L");
    teardown_shell();
}

/* ================================================================
 *  测试: 脚本系统
 * ================================================================ */

static void test_script_define_and_run(void)
{
    setup_shell();
    exec_and_capture("script define mytest");
    /* Feed lines in recording mode */
    extern int zoom_shell_script_handle_line(zoom_shell_t *, const char *);
    zoom_shell_script_handle_line(&g_shell, "echo line1");
    zoom_shell_script_handle_line(&g_shell, "echo line2");
    zoom_shell_script_handle_line(&g_shell, "end");
    TEST_ASSERT_EQ(g_shell.script.count, 1, "should have 1 script");

    mock_output_reset();
    const char *out = exec_and_capture("script run mytest");
    TEST_ASSERT_STR_CONTAINS(out, "line1", "should output line1");
    TEST_ASSERT_STR_CONTAINS(out, "line2", "should output line2");
    teardown_shell();
}

static void test_script_list(void)
{
    setup_shell();
    exec_and_capture("script define s1");
    extern int zoom_shell_script_handle_line(zoom_shell_t *, const char *);
    zoom_shell_script_handle_line(&g_shell, "echo hi");
    zoom_shell_script_handle_line(&g_shell, "end");

    const char *out = exec_and_capture("script list");
    TEST_ASSERT_STR_CONTAINS(out, "s1", "should list script s1");
    teardown_shell();
}

/* ================================================================
 *  主入口
 * ================================================================ */

int main(void)
{
    printf("\n========================================\n");
    printf("  Zoom Shell v%s — Test Suite\n", ZOOM_SHELL_VERSION_STRING);
    printf("========================================\n\n");

    /* 初始化 */
    printf("[Init]\n");
    RUN_TEST(test_init_null_params);
    RUN_TEST(test_init_buffer_too_small);
    RUN_TEST(test_init_success);

    /* 命令执行 */
    printf("\n[Command Execution]\n");
    RUN_TEST(test_exec_simple_command);
    RUN_TEST(test_exec_with_args);
    RUN_TEST(test_exec_quoted_args);
    RUN_TEST(test_exec_unknown_command);
    RUN_TEST(test_exec_empty_line);
    RUN_TEST(test_exec_return_value);

    /* 子命令树 */
    printf("\n[Subcommand Tree]\n");
    RUN_TEST(test_subcmd_basic);
    RUN_TEST(test_subcmd_with_args);
    RUN_TEST(test_subcmd_deep);
    RUN_TEST(test_subcmd_container_lists_children);

    /* 用户系统 */
    printf("\n[User System]\n");
    RUN_TEST(test_user_default_is_guest);
    RUN_TEST(test_user_login_success);
    RUN_TEST(test_user_login_wrong_password);
    RUN_TEST(test_user_login_nonexistent);
    RUN_TEST(test_user_logout);
    RUN_TEST(test_user_permission_denied);
    RUN_TEST(test_user_permission_granted);
    RUN_TEST(test_user_add_runtime);
    RUN_TEST(test_user_add_duplicate);

    /* 变量系统 */
    printf("\n[Variable System]\n");
    RUN_TEST(test_var_find);
    RUN_TEST(test_var_get_int_types);
    RUN_TEST(test_var_get_float);
    RUN_TEST(test_var_get_bool);
    RUN_TEST(test_var_set_int);
    RUN_TEST(test_var_set_float);
    RUN_TEST(test_var_set_bool);
    RUN_TEST(test_var_readonly_protection);
    RUN_TEST(test_var_cmd_list);
    RUN_TEST(test_var_cmd_get);
    RUN_TEST(test_var_cmd_set);
    RUN_TEST(test_var_cmd_set_readonly);

    /* 输入处理 */
    printf("\n[Input Processing]\n");
    RUN_TEST(test_input_basic_line);
    RUN_TEST(test_input_backspace);
    RUN_TEST(test_input_empty_enter);

    /* 历史记录 */
    printf("\n[History]\n");
    RUN_TEST(test_history_records_commands);
    RUN_TEST(test_history_dedup);
    RUN_TEST(test_history_cmd_output);

    /* 内置命令 */
    printf("\n[Built-in Commands]\n");
    RUN_TEST(test_builtin_help);
    RUN_TEST(test_builtin_help_specific);
    RUN_TEST(test_builtin_version);
    RUN_TEST(test_builtin_echo);
    RUN_TEST(test_builtin_stats);
    RUN_TEST(test_builtin_exit);

    /* printf 格式化 */
    printf("\n[Printf Formatting]\n");
    RUN_TEST(test_printf_basic);
    RUN_TEST(test_printf_width_right_align);
    RUN_TEST(test_printf_width_left_align);
    RUN_TEST(test_printf_zero_pad);
    RUN_TEST(test_printf_hex);
    RUN_TEST(test_printf_negative);
    RUN_TEST(test_printf_unsigned);
    RUN_TEST(test_printf_percent);
    RUN_TEST(test_printf_null_string);

    /* 边界条件 */
    printf("\n[Edge Cases]\n");
    RUN_TEST(test_exec_null_params);
    RUN_TEST(test_input_when_not_running);
    RUN_TEST(test_hidden_cmd_still_executable);

    /* 表达式求值器 */
    printf("\n[Calc Extension]\n");
    RUN_TEST(test_calc_basic_add);
    RUN_TEST(test_calc_mul_precedence);
    RUN_TEST(test_calc_hex);
    RUN_TEST(test_calc_bitwise);
    RUN_TEST(test_calc_shift);
    RUN_TEST(test_calc_parens);
    RUN_TEST(test_calc_negative);
    RUN_TEST(test_calc_division);
    RUN_TEST(test_calc_modulo);

    /* 别名系统 */
    printf("\n[Alias Extension]\n");
    RUN_TEST(test_alias_add_and_lookup);
    RUN_TEST(test_alias_remove);
    RUN_TEST(test_alias_cmd);
    RUN_TEST(test_alias_nonexistent);

    /* HexDump */
    printf("\n[HexDump Extension]\n");
    RUN_TEST(test_hexdump_cmd);

    /* 进度条 */
    printf("\n[Progress Extension]\n");
    RUN_TEST(test_progress_bar);

    /* 日志系统 */
    printf("\n[Log Extension]\n");
    RUN_TEST(test_log_cmd);
    RUN_TEST(test_log_level);

    /* 按键绑定 */
    printf("\n[Keybind Extension]\n");
    RUN_TEST(test_keybind_init_and_list);

    /* 脚本系统 */
    printf("\n[Script Extension]\n");
    RUN_TEST(test_script_define_and_run);
    RUN_TEST(test_script_list);

    /* 汇总 */
    printf("\n========================================\n");
    printf("  Total: %d  Passed: %d  Failed: %d\n",
           g_tests_run, g_tests_passed, g_tests_failed);
    printf("========================================\n\n");

    return g_tests_failed > 0 ? 1 : 0;
}
