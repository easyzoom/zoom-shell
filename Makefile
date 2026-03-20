# Zoom Shell Makefile

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99
AR      = ar
ARFLAGS = rcs

# 目录
INCLUDE_DIR = include
SRC_DIR     = src
EXT_DIR     = extensions
DEMO_DIR    = demo/x86-gcc
BUILD_DIR   = build

# 扩展目录包含路径
EXT_INCLUDES = -I$(EXT_DIR)/log -I$(EXT_DIR)/keybind -I$(EXT_DIR)/progress \
               -I$(EXT_DIR)/telnet

# 用户配置头文件
CFLAGS += -I$(INCLUDE_DIR) -I$(DEMO_DIR) $(EXT_INCLUDES)
CFLAGS += -DZOOM_SHELL_CFG_USER='"zoom_shell_cfg_user.h"'

# 核心库源文件
LIB_SRCS = $(SRC_DIR)/zoom_shell_core.c \
           $(SRC_DIR)/zoom_shell_cmds.c \
           $(SRC_DIR)/zoom_shell_var.c

# 扩展源文件
EXT_SRCS = $(EXT_DIR)/hexdump/zoom_shell_hexdump.c \
           $(EXT_DIR)/log/zoom_shell_log.c \
           $(EXT_DIR)/calc/zoom_shell_calc.c \
           $(EXT_DIR)/passthrough/zoom_shell_passthrough.c \
           $(EXT_DIR)/repeat/zoom_shell_repeat.c \
           $(EXT_DIR)/keybind/zoom_shell_keybind.c \
           $(EXT_DIR)/alias/zoom_shell_alias.c \
           $(EXT_DIR)/script/zoom_shell_script.c \
           $(EXT_DIR)/progress/zoom_shell_progress.c \
           $(EXT_DIR)/game/zoom_shell_2048.c \
           $(EXT_DIR)/game/zoom_shell_pushbox.c \
           $(EXT_DIR)/game/zoom_shell_snake.c

# Demo 源文件
DEMO_SRCS = $(DEMO_DIR)/main.c \
            $(DEMO_DIR)/zoom_shell_port.c

# 目标文件
LIB_OBJS  = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(LIB_SRCS)))
EXT_OBJS  = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(EXT_SRCS)))
DEMO_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(DEMO_SRCS)))

# 链接脚本
LDSCRIPT = $(DEMO_DIR)/link.lds
LDFLAGS  = -T$(LDSCRIPT)

# 输出
LIB_STATIC = $(BUILD_DIR)/libzoomshell.a
DEMO_BIN   = $(BUILD_DIR)/zoom_shell_demo

.PHONY: all clean lib demo test help

all: demo

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# 编译库源文件
$(BUILD_DIR)/zoom_shell_core.o: $(SRC_DIR)/zoom_shell_core.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/zoom_shell_cmds.o: $(SRC_DIR)/zoom_shell_cmds.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/zoom_shell_var.o: $(SRC_DIR)/zoom_shell_var.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 编译扩展源文件 (通配规则)
$(BUILD_DIR)/zoom_shell_hexdump.o: $(EXT_DIR)/hexdump/zoom_shell_hexdump.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/zoom_shell_log.o: $(EXT_DIR)/log/zoom_shell_log.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/zoom_shell_calc.o: $(EXT_DIR)/calc/zoom_shell_calc.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/zoom_shell_passthrough.o: $(EXT_DIR)/passthrough/zoom_shell_passthrough.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/zoom_shell_repeat.o: $(EXT_DIR)/repeat/zoom_shell_repeat.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/zoom_shell_keybind.o: $(EXT_DIR)/keybind/zoom_shell_keybind.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/zoom_shell_alias.o: $(EXT_DIR)/alias/zoom_shell_alias.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/zoom_shell_script.o: $(EXT_DIR)/script/zoom_shell_script.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/zoom_shell_progress.o: $(EXT_DIR)/progress/zoom_shell_progress.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/zoom_shell_2048.o: $(EXT_DIR)/game/zoom_shell_2048.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/zoom_shell_pushbox.o: $(EXT_DIR)/game/zoom_shell_pushbox.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/zoom_shell_snake.o: $(EXT_DIR)/game/zoom_shell_snake.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 编译 Demo 源文件
$(BUILD_DIR)/main.o: $(DEMO_DIR)/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/zoom_shell_port.o: $(DEMO_DIR)/zoom_shell_port.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 构建静态库 (核心+扩展)
lib: $(LIB_OBJS) $(EXT_OBJS)
	@echo "Building static library..."
	$(AR) $(ARFLAGS) $(LIB_STATIC) $(LIB_OBJS) $(EXT_OBJS)
	@echo "Built: $(LIB_STATIC)"

# 构建 demo 可执行文件
demo: $(LIB_OBJS) $(EXT_OBJS) $(DEMO_OBJS)
	@echo "Linking demo..."
	$(CC) $(CFLAGS) $(LIB_OBJS) $(EXT_OBJS) $(DEMO_OBJS) $(LDFLAGS) -o $(DEMO_BIN)
	@echo "Built: $(DEMO_BIN)"

# 测试
TEST_DIR    = test
TEST_BIN    = $(BUILD_DIR)/zoom_shell_test
TEST_CFLAGS = -Wall -Wextra -O0 -g -std=c99 -I$(INCLUDE_DIR) -I$(TEST_DIR) $(EXT_INCLUDES) \
              -DZOOM_SHELL_CFG_USER='"zoom_shell_cfg_test.h"'

ALL_TEST_SRCS = $(SRC_DIR)/zoom_shell_core.c \
                $(SRC_DIR)/zoom_shell_cmds.c \
                $(SRC_DIR)/zoom_shell_var.c \
                $(EXT_DIR)/hexdump/zoom_shell_hexdump.c \
                $(EXT_DIR)/log/zoom_shell_log.c \
                $(EXT_DIR)/calc/zoom_shell_calc.c \
                $(EXT_DIR)/passthrough/zoom_shell_passthrough.c \
                $(EXT_DIR)/repeat/zoom_shell_repeat.c \
                $(EXT_DIR)/keybind/zoom_shell_keybind.c \
                $(EXT_DIR)/alias/zoom_shell_alias.c \
                $(EXT_DIR)/script/zoom_shell_script.c \
                $(EXT_DIR)/progress/zoom_shell_progress.c \
                $(EXT_DIR)/game/zoom_shell_2048.c \
                $(EXT_DIR)/game/zoom_shell_pushbox.c \
                $(EXT_DIR)/game/zoom_shell_snake.c \
                $(TEST_DIR)/test_zoom_shell.c

test: | $(BUILD_DIR)
	@echo "Building tests..."
	$(CC) $(TEST_CFLAGS) $(ALL_TEST_SRCS) $(LDFLAGS) -o $(TEST_BIN)
	@echo "Running tests..."
	@$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Zoom Shell v1.0 Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all   - Build demo (default)"
	@echo "  lib   - Build static library (core + extensions)"
	@echo "  demo  - Build x86-gcc demo"
	@echo "  test  - Build and run test suite"
	@echo "  clean - Remove build artifacts"
	@echo "  help  - Show this message"
