# LinSlave Module Makefile
# 支持目标: all, test, clean

# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -g -I./include
LDFLAGS =

# 目录
SRC_DIR = src
INC_DIR = include
TEST_DIR = test
BUILD_DIR = build

# 源文件
SOURCES = $(SRC_DIR)/LinSlave.c \
          $(SRC_DIR)/LinSlave_Pid.c \
          $(SRC_DIR)/LinSlave_Checksum.c \
          $(SRC_DIR)/LinSlave_Hal.c

TEST_SOURCES = $(TEST_DIR)/test_linslave.c

# 目标文件
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)
TEST_OBJECTS = $(TEST_SOURCES:%.c=$(BUILD_DIR)/%.o)

# 执行文件
TEST_TARGET = $(BUILD_DIR)/test_linslave

# 默认目标
all: directories $(TEST_TARGET)

# 创建目录
directories:
	@mkdir -p $(BUILD_DIR)/$(SRC_DIR)
	@mkdir -p $(BUILD_DIR)/$(TEST_DIR)

# 编译规则
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 测试目标
$(TEST_TARGET): $(OBJECTS) $(TEST_OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

# 运行测试
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# 清理
clean:
	rm -rf $(BUILD_DIR)

# 重构编译
rebuild: clean all

# 帮助
help:
	@echo "LinSlave Module Makefile"
	@echo ""
	@echo "目标:"
	@echo "  all      - 编译测试程序"
	@echo "  test     - 运行单元测试"
	@echo "  clean    - 清理编译产物"
	@echo "  rebuild  - 重新编译"
	@echo "  help     - 显示此帮助"

.PHONY: all directories test clean rebuild help
