/**
 * @file zoom_shell_cpp.h
 * @brief Zoom Shell C++ 支持
 *
 * C++ 编译器对 section 属性中的字符串常量处理不同，
 * 需要先声明为独立变量再引用。
 *
 * 在 .cpp 文件中 #include 此头文件后，
 * 即可使用 ZOOM_EXPORT_CMD 等宏导出命令/变量/用户。
 */

#ifndef ZOOM_SHELL_CPP_H
#define ZOOM_SHELL_CPP_H

#ifdef __cplusplus

#include "zoom_shell.h"

#if ZOOM_USING_CMD_EXPORT

/* 取消 C 版宏定义，重新定义 C++ 版本 */
#undef ZOOM_EXPORT_CMD
#undef ZOOM_EXPORT_CMD_WITH_SUB
#undef ZOOM_EXPORT_VAR
#undef ZOOM_EXPORT_USER

#define ZOOM_EXPORT_CMD(_name, _func, _desc, _attr, _level) \
    static const char _zoom_cpp_name_##_name[] = #_name;    \
    static const char _zoom_cpp_desc_##_name[] = _desc;     \
    extern "C" ZOOM_USED const zoom_cmd_t                   \
    _zoom_cmd_##_name ZOOM_SECTION("zoomCommand") = {       \
        .name = _zoom_cpp_name_##_name,                     \
        .func = (_func),                                    \
        .desc = _zoom_cpp_desc_##_name,                     \
        .subcmds = NULL,                                    \
        .subcmd_count = 0,                                  \
        .attr = (_attr),                                    \
        .min_level = (_level),                              \
    }

#define ZOOM_EXPORT_CMD_WITH_SUB(_name, _set, _desc, _attr, _level) \
    static const char _zoom_cpp_name_##_name[] = #_name;             \
    static const char _zoom_cpp_desc_##_name[] = _desc;              \
    extern "C" ZOOM_USED const zoom_cmd_t                            \
    _zoom_cmd_##_name ZOOM_SECTION("zoomCommand") = {                \
        .name = _zoom_cpp_name_##_name,                              \
        .func = NULL,                                                \
        .desc = _zoom_cpp_desc_##_name,                              \
        .subcmds = (_set),                                           \
        .subcmd_count = sizeof(_set) / sizeof((_set)[0]),            \
        .attr = (_attr),                                             \
        .min_level = (_level),                                       \
    }

#if ZOOM_USING_VAR
#undef ZOOM_EXPORT_VAR
#define ZOOM_EXPORT_VAR(_name, _var, _type, _desc, _attr, _level) \
    static const char _zoom_cpp_vname_##_name[] = #_name;          \
    static const char _zoom_cpp_vdesc_##_name[] = _desc;           \
    extern "C" ZOOM_USED const zoom_var_t                          \
    _zoom_var_##_name ZOOM_SECTION("zoomVar") = {                  \
        .name = _zoom_cpp_vname_##_name,                           \
        .desc = _zoom_cpp_vdesc_##_name,                           \
        .addr = (void *)&(_var),                                   \
        .type = (_type),                                           \
        .attr = (_attr),                                           \
        .min_level = (_level),                                     \
    }
#endif

#if ZOOM_USING_USER
#undef ZOOM_EXPORT_USER
#define ZOOM_EXPORT_USER(_name, _password, _level)  \
    static const char _zoom_cpp_uname_##_name[] = #_name;    \
    static const char _zoom_cpp_upass_##_name[] = _password;  \
    extern "C" ZOOM_USED const zoom_user_t                    \
    _zoom_user_##_name ZOOM_SECTION("zoomUser") = {           \
        .name = {0},                                          \
        .password = {0},                                      \
        .level = (_level),                                    \
    }
/* Note: C++ designated initializers for arrays are limited.  \
 * The name/password are set in the linker section as zeros;  \
 * users should use zoom_shell_add_user() at runtime instead  \
 * for C++ modules, or use the C version in .c files. */
#endif

#endif /* ZOOM_USING_CMD_EXPORT */

#endif /* __cplusplus */
#endif /* ZOOM_SHELL_CPP_H */
