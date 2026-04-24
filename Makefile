# ETH-DDS Integration Makefile

# 编译器设置
CC ?= gcc
CXX ?= g++
AR ?= ar
CFLAGS = -Wall -Wextra -O2 -std=c11
CXXFLAGS = -Wall -Wextra -O2 -std=c++11

# 目录设置
SRC_DIR = src
OBJ_DIR = obj
LIB_DIR = lib
BIN_DIR = bin

# 头文件路径
INCLUDES = -I$(SRC_DIR) \
           -I$(SRC_DIR)/common/types \
           -I$(SRC_DIR)/common/utils \
           -I$(SRC_DIR)/common/log \
           -I$(SRC_DIR)/ethernet/driver \
           -I$(SRC_DIR)/dds/core \
           -I$(SRC_DIR)/dds/pubsub \
           -I$(SRC_DIR)/dds/qos \
           -I$(SRC_DIR)/transport

# 源文件
COMMON_SRCS = $(wildcard $(SRC_DIR)/common/utils/*.c) \
              $(wildcard $(SRC_DIR)/common/log/*.c)
ETHERNET_SRCS = $(wildcard $(SRC_DIR)/ethernet/driver/*.c)
DDS_SRCS = $(wildcard $(SRC_DIR)/dds/core/*.c) \
           $(wildcard $(SRC_DIR)/dds/pubsub/*.c) \
           $(wildcard $(SRC_DIR)/dds/qos/*.c)
TRANSPORT_SRCS = $(wildcard $(SRC_DIR)/transport/*.c)

ALL_SRCS = $(COMMON_SRCS) $(ETHERNET_SRCS) $(DDS_SRCS) $(TRANSPORT_SRCS)

# 目标文件
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(ALL_SRCS))

# 库文件
STATIC_LIB = $(LIB_DIR)/libeth_dds.a

# 默认目标
.PHONY: all clean dirs lib

all: dirs lib

# 创建目录
dirs:
	@mkdir -p $(OBJ_DIR)/common/utils
	@mkdir -p $(OBJ_DIR)/common/log
	@mkdir -p $(OBJ_DIR)/ethernet/driver
	@mkdir -p $(OBJ_DIR)/dds/core
	@mkdir -p $(OBJ_DIR)/dds/pubsub
	@mkdir -p $(OBJ_DIR)/dds/qos
	@mkdir -p $(OBJ_DIR)/transport
	@mkdir -p $(LIB_DIR)
	@mkdir -p $(BIN_DIR)

# 编译静态库
lib: $(STATIC_LIB)

$(STATIC_LIB): $(OBJS)
	@echo "AR $@"
	@$(AR) rcs $@ $^

# 编译规则
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "CC $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 清理
clean:
	@echo "Cleaning..."
	@rm -rf $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)

# 安装
install:
	@echo "Installing to /usr/local..."
	@mkdir -p /usr/local/lib
	@mkdir -p /usr/local/include/eth_dds
	@cp $(STATIC_LIB) /usr/local/lib/
	@cp $(SRC_DIR)/*/types/*.h /usr/local/include/eth_dds/ 2>/dev/null || true
	@cp $(SRC_DIR)/*/utils/*.h /usr/local/include/eth_dds/ 2>/dev/null || true
	@cp $(SRC_DIR)/*/driver/*.h /usr/local/include/eth_dds/ 2>/dev/null || true
	@cp $(SRC_DIR)/*/core/*.h /usr/local/include/eth_dds/ 2>/dev/null || true
	@echo "Installation complete"

# 卸载
uninstall:
	@echo "Uninstalling..."
	@rm -f /usr/local/lib/libeth_dds.a
	@rm -rf /usr/local/include/eth_dds
	@echo "Uninstallation complete"

# 调试目标
debug: CFLAGS += -g -DDEBUG
debug: all

# 打印信息
info:
	@echo "Compiler: $(CC)"
	@echo "Sources: $(ALL_SRCS)"
	@echo "Objects: $(OBJS)"
