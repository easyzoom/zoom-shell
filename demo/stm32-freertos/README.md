# STM32 + FreeRTOS 移植指南

## 步骤

1. 将 `zoom-shell/include/` 和 `zoom-shell/src/` 加入工程
2. 将 `zoom_shell_port.c` 和 `zoom_shell_cfg_user.h` 加入工程
3. 编译选项添加: `-DZOOM_SHELL_CFG_USER='"zoom_shell_cfg_user.h"'`
4. 修改 `zoom_shell_port.c` 中的 UART 句柄和 I/O 实现
5. 在链接脚本中添加 zoom 段 (参考 `demo/x86-gcc/link.lds`)
6. 创建 FreeRTOS 任务运行 `zoom_shell_run()`

## 链接脚本

在 `.rodata` 段之后添加:

```
.zoom_command ALIGN(4) : { _zoom_cmd_start = .; KEEP(*(zoomCommand)) _zoom_cmd_end = .; }
.zoom_var     ALIGN(4) : { _zoom_var_start = .; KEEP(*(zoomVar))     _zoom_var_end = .; }
.zoom_user    ALIGN(4) : { _zoom_user_start = .; KEEP(*(zoomUser))   _zoom_user_end = .; }
```

## 推荐 FreeRTOS 任务配置

- 栈大小: 1024 字 (4KB)
- 优先级: osPriorityBelowNormal
