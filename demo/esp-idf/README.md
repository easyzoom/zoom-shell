# ESP-IDF 移植指南

## 步骤

1. 将 `zoom-shell/include/` 和 `zoom-shell/src/` 文件加入 ESP-IDF 组件
2. 复制 `zoom_shell_port.c` 和 `zoom_shell_cfg_user.h` 到组件目录
3. 在 `CMakeLists.txt` 中配置编译选项
4. 修改 `zoom_shell_port.c` 中的 UART 编号
5. 在 `app_main` 中创建任务运行 shell

## CMakeLists.txt 示例

```cmake
idf_component_register(
    SRCS "zoom_shell_port.c"
         "../../src/zoom_shell_core.c"
         "../../src/zoom_shell_cmds.c"
         "../../src/zoom_shell_var.c"
    INCLUDE_DIRS "." "../../include"
)
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    ZOOM_SHELL_CFG_USER="zoom_shell_cfg_user.h"
)
```

## 链接脚本

ESP-IDF 使用 `.lf` 文件定义链接段:

```
[sections:zoom_cmd]
entries:
    .zoom_command

[scheme:zoom_cmd_default]
entries:
    zoom_cmd -> flash_rodata
```
