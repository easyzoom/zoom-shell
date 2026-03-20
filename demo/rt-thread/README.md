# RT-Thread 移植指南

## 步骤

1. 将 `zoom-shell/include/` 和 `zoom-shell/src/` 加入 RT-Thread 工程
2. 复制 `zoom_shell_port.c` 和 `zoom_shell_cfg_user.h` 到 BSP 目录
3. 修改串口设备名称 (默认 `"uart1"`)
4. 编译选项添加: `-DZOOM_SHELL_CFG_USER='"zoom_shell_cfg_user.h"'`
5. 创建线程运行 `zoom_shell_run()`

## SConscript 示例

```python
from building import *
src = Glob('*.c') + Glob('../../src/*.c')
inc = ['.', '../../include']
group = DefineGroup('zoom_shell', src, depend=[''], CPPPATH=inc,
    CPPDEFINES=['ZOOM_SHELL_CFG_USER=\'"zoom_shell_cfg_user.h"\''])
```

## 链接脚本

在 link.lds 的 `.rodata` 段之后添加 zoom 段 (参考 `demo/x86-gcc/link.lds`)。
