# Zoom Shell v1.0

**嵌入式增强型 Shell** — 零 malloc，零 OS 依赖，可在裸机单片机上运行。

融合 Zephyr Shell、RT-Thread msh、FreeRTOS CLI、USMART、nr_micro_shell、letter-shell 的精华，打造三大独有特色。

## 三大特色

### 1. 层级子命令树

其他嵌入式 shell 只支持扁平命令，Zoom Shell 支持无限层级嵌套子命令，Tab 补全沿树工作：

```c
ZOOM_SUBCMD_SET(sub_gpio,
    ZOOM_SUBCMD(set,    cmd_gpio_set,    "Set pin level"),
    ZOOM_SUBCMD(get,    cmd_gpio_get,    "Get pin level"),
    ZOOM_SUBCMD(toggle, cmd_gpio_toggle, "Toggle pin"),
);
ZOOM_EXPORT_CMD_WITH_SUB(gpio, sub_gpio, "GPIO operations",
                          ZOOM_ATTR_DEFAULT, ZOOM_USER_USER);
```

```
zoom> gpio set 13 1
[OK] GPIO pin 13 set to 1
```

### 2. 变量观测系统

通过串口实时查看和修改任何注册的变量，类似 Simulink 外部模式：

```c
static int32_t pid_kp = 100;
ZOOM_EXPORT_VAR(pid_kp, pid_kp, ZOOM_VAR_INT32, "PID Kp x100", ZOOM_VAR_RW, ZOOM_USER_GUEST);
```

```
zoom> var list
  Name             Type     Value        Attr  Description
  pid_kp           int32    100          [RW]  PID Kp x100
  motor_speed      float    0.000        [RO]  Motor RPM
zoom> var set pid_kp 200
[OK] pid_kp = 200
```

### 3. 命令执行计时

借鉴 USMART，自动统计命令执行耗时：

```
zoom> perf on
zoom> some_command
[OK] some_command (elapsed: 12 ms)
```

## 功能对比

| 特性 | letter-shell | nr_micro | FreeRTOS CLI | Zephyr | msh | USMART | **Zoom Shell** |
|------|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| 子命令树 | - | - | - | Y | - | - | **Y** |
| 变量观测 | - | - | - | - | - | 部分 | **Y** |
| 命令计时 | - | - | - | - | - | Y | **Y** |
| 用户权限 | Y | - | - | - | - | - | **Y** |
| Tab 补全 | Y | Y | - | Y | - | - | **Y** |
| 历史记录 | Y | Y | - | Y | - | - | **Y** |
| 零 malloc | Y | Y | - | Y | Y | Y | **Y** |
| 链接器段注册 | Y | - | - | Y | Y | - | **Y** |
| 条件编译命令 | - | - | - | Y | - | - | **Y** |
| ANSI 彩色输出 | - | - | - | Y | - | - | **Y** |

## 快速开始

### 编译运行 x86 Demo

```bash
cd zoom-shell
make demo
./build/zoom_shell_demo
```

### 移植到你的 MCU

**第 1 步：创建配置文件** `zoom_shell_cfg_user.h`

```c
#define ZOOM_USING_HISTORY        1
#define ZOOM_USING_USER           1
#define ZOOM_USING_VAR            1
#define ZOOM_CMD_MAX_LENGTH       128
#define ZOOM_HISTORY_MAX_NUMBER   5
#define ZOOM_PRINT_BUFFER         128

#define ZOOM_GET_TICK()  HAL_GetTick()  /* 你的毫秒 tick */
```

**第 2 步：实现 I/O 回调**

```c
#include "zoom_shell.h"

zoom_shell_t shell;
char shell_buffer[768];  /* 128 * (5+1) = 768 */

int16_t uart_read(char *data, uint16_t len) {
    /* 从 UART 读取一个字节 */
    (void)len;
    if (HAL_UART_Receive(&huart1, (uint8_t *)data, 1, HAL_MAX_DELAY) == HAL_OK)
        return 1;
    return 0;
}

int16_t uart_write(const char *data, uint16_t len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)data, len, HAL_MAX_DELAY);
    return len;
}
```

**第 3 步：初始化并运行**

```c
/* 阻塞模式（在 RTOS 任务中） */
shell.read  = uart_read;
shell.write = uart_write;
zoom_shell_init(&shell, shell_buffer, sizeof(shell_buffer));
zoom_shell_run(&shell);  /* 不返回 */

/* 或：中断驱动模式（裸机 main loop） */
shell.read  = uart_read;
shell.write = uart_write;
zoom_shell_init(&shell, shell_buffer, sizeof(shell_buffer));
zoom_shell_print_welcome(&shell);
zoom_shell_show_prompt(&shell);
/* 在 UART 中断中: */
void USART1_IRQHandler(void) {
    char c;
    if (HAL_UART_Receive(&huart1, (uint8_t *)&c, 1, 0) == HAL_OK) {
        zoom_shell_input(&shell, c);
    }
}
```

**第 4 步：注册命令和变量**

```c
static int cmd_led(zoom_shell_t *sh, int argc, char *argv[]) {
    if (argc < 1) { zoom_error(sh, "Usage: led <on|off>\r\n"); return -1; }
    if (argv[0][0] == 'o' && argv[0][1] == 'n') HAL_GPIO_WritePin(LED_GPIO, LED_PIN, 1);
    else HAL_GPIO_WritePin(LED_GPIO, LED_PIN, 0);
    return 0;
}
ZOOM_EXPORT_CMD(led, cmd_led, "Control LED", ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);

static int32_t adc_value = 0;
ZOOM_EXPORT_VAR(adc, adc_value, ZOOM_VAR_INT32, "ADC reading", ZOOM_VAR_RO, ZOOM_USER_GUEST);
```

**第 5 步：链接脚本** — 添加到你的 `.ld` 文件：

```ld
.zoom_command ALIGN(4) : {
    _zoom_cmd_start = .;
    KEEP(*(zoomCommand))
    _zoom_cmd_end = .;
} > FLASH

.zoom_var ALIGN(4) : {
    _zoom_var_start = .;
    KEEP(*(zoomVar))
    _zoom_var_end = .;
} > FLASH

.zoom_user ALIGN(4) : {
    _zoom_user_start = .;
    KEEP(*(zoomUser))
    _zoom_user_end = .;
} > FLASH
```

## 目录结构

```
zoom-shell/
├── include/
│   ├── zoom_shell.h          # 主头文件（数据结构 + API + 导出宏）
│   └── zoom_shell_cfg.h      # 默认配置宏
├── src/
│   ├── zoom_shell_core.c     # 核心实现
│   ├── zoom_shell_cmds.c     # 内置命令
│   └── zoom_shell_var.c      # 变量观测系统
├── demo/
│   └── x86-gcc/              # x86 平台 Demo
├── Makefile
└── README.md
```

## 配置系统

所有配置通过 `#ifndef` 宏提供默认值，编译时传入 `-DZOOM_SHELL_CFG_USER='"your_cfg.h"'` 覆盖：

| 宏 | 默认值 | 说明 |
|---|---|---|
| `ZOOM_USING_CMD_EXPORT` | 1 | 1=链接器段, 0=静态数组 |
| `ZOOM_USING_HISTORY` | 1 | 命令历史 |
| `ZOOM_USING_USER` | 1 | 用户/权限系统 |
| `ZOOM_USING_VAR` | 1 | 变量观测 |
| `ZOOM_USING_PERF` | 1 | 命令计时 |
| `ZOOM_USING_ANSI` | 1 | ANSI 转义码/彩色 |
| `ZOOM_USING_LOCK` | 0 | 互斥锁 |
| `ZOOM_CMD_MAX_LENGTH` | 128 | 命令行最大长度 |
| `ZOOM_CMD_MAX_ARGS` | 8 | 最大参数数 |
| `ZOOM_HISTORY_MAX_NUMBER` | 5 | 历史条数 |
| `ZOOM_MAX_USERS` | 4 | 最大用户数 |
| `ZOOM_PRINT_BUFFER` | 128 | 格式化输出缓冲区 |
| `ZOOM_GET_TICK()` | 0 | 毫秒 tick 获取函数 |

## API 速查

```c
/* 核心 */
int  zoom_shell_init(zoom_shell_t *shell, char *buffer, uint16_t size);
int  zoom_shell_run(zoom_shell_t *shell);
void zoom_shell_input(zoom_shell_t *shell, char data);
int  zoom_shell_exec(zoom_shell_t *shell, const char *cmd_line);

/* 输出 */
zoom_printf(shell, fmt, ...);   /* 普通输出 */
zoom_info(shell, fmt, ...);     /* [INFO] 蓝色 */
zoom_warn(shell, fmt, ...);     /* [WARN] 黄色 */
zoom_error(shell, fmt, ...);    /* [ERR]  红色 */
zoom_ok(shell, fmt, ...);       /* [OK]   绿色 */

/* 导出宏 */
ZOOM_EXPORT_CMD(name, func, desc, attr, level);
ZOOM_EXPORT_CMD_WITH_SUB(name, subcmd_set, desc, attr, level);
ZOOM_SUBCMD_SET(name, ...);
ZOOM_SUBCMD(name, func, desc);
ZOOM_EXPORT_VAR(name, var, type, desc, rw, level);
ZOOM_EXPORT_USER(name, password, level);
```

## 设计原则

- **零 malloc** — 所有内存静态分配或由用户提供
- **零 OS 依赖** — 仅依赖 `<stdint.h>` `<stdbool.h>` `<stddef.h>` `<string.h>` `<stdarg.h>`
- **I/O 纯回调** — read/write 可对接 UART/USB/SPI/BLE 等任何接口
- **条件编译裁剪** — 每个功能模块可独立开关，最小配置约 2KB ROM

## License

MIT
