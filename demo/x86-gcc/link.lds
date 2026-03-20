/* Zoom Shell 链接脚本 — 定义命令/变量/用户/插件段 */

SECTIONS
{
    .zoom_command ALIGN(4) :
    {
        _zoom_cmd_start = .;
        KEEP(*(zoomCommand))
        _zoom_cmd_end = .;
    }

    .zoom_var ALIGN(4) :
    {
        _zoom_var_start = .;
        KEEP(*(zoomVar))
        _zoom_var_end = .;
    }

    .zoom_user ALIGN(4) :
    {
        _zoom_user_start = .;
        KEEP(*(zoomUser))
        _zoom_user_end = .;
    }

    .zoom_plugin ALIGN(4) :
    {
        _zoom_plugin_start = .;
        KEEP(*(zoomPlugin))
        _zoom_plugin_end = .;
    }
}
INSERT AFTER .rodata;
