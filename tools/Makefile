# DDS Monitoring Tools Makefile
# 车载DDS监控诊断工具构建文件

# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC -pthread
CFLAGS_DEBUG = -Wall -Wextra -g -DDEBUG -pthread
INCLUDES = -I../src/common/log -I./dds_analyzer -I./dds_monitor

# 链接库
LDFLAGS = -lpthread -lpcap -lz -lm

# 目录
BUILD_DIR = build
BIN_DIR = bin

# 源文件
LOG_SOURCES = ../src/common/log/dds_log.c
ANALYZER_SOURCES = dds_analyzer/dds_analyzer.c
MONITOR_SOURCES = dds_monitor/qos_monitor.c dds_monitor/health_check.c dds_monitor/dds_monitor_cli.c

# 头文件
HEADERS = ../src/common/log/dds_log.h \
          dds_analyzer/dds_analyzer.h \
          dds_monitor/qos_monitor.h \
          dds_monitor/health_check.h

# 目标
TARGETS = $(BIN_DIR)/dds_monitor_cli $(BIN_DIR)/libdds_analyzer.so $(BIN_DIR)/libdds_monitor.so

# 默认目标
.PHONY: all clean install debug

all: dirs $(TARGETS)

dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

# DDS日志库
$(BUILD_DIR)/dds_log.o: ../src/common/log/dds_log.c ../src/common/log/dds_log.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# DDS分析器库
$(BUILD_DIR)/dds_analyzer.o: dds_analyzer/dds_analyzer.c dds_analyzer/dds_analyzer.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/libdds_analyzer.so: $(BUILD_DIR)/dds_analyzer.o
	$(CC) -shared -o $@ $^ $(LDFLAGS)

# DDS监控库
$(BUILD_DIR)/qos_monitor.o: dds_monitor/qos_monitor.c dds_monitor/qos_monitor.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/health_check.o: dds_monitor/health_check.c dds_monitor/health_check.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/libdds_monitor.so: $(BUILD_DIR)/qos_monitor.o $(BUILD_DIR)/health_check.o $(BUILD_DIR)/dds_log.o
	$(CC) -shared -o $@ $^ $(LDFLAGS)

# DDS监控CLI
$(BUILD_DIR)/dds_monitor_cli.o: dds_monitor/dds_monitor_cli.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/dds_monitor_cli: $(BUILD_DIR)/dds_monitor_cli.o $(BUILD_DIR)/dds_analyzer.o \
                            $(BUILD_DIR)/qos_monitor.o $(BUILD_DIR)/health_check.o $(BUILD_DIR)/dds_log.o
	$(CC) -o $@ $^ $(LDFLAGS)

# 调试版本
debug: CFLAGS = $(CFLAGS_DEBUG)
debug: all

# 安装
PREFIX ?= /usr/local

install: all
	install -d $(PREFIX)/bin
	install -d $(PREFIX)/lib
	install -d $(PREFIX)/include/dds
	install -m 755 $(BIN_DIR)/dds_monitor_cli $(PREFIX)/bin/
	install -m 755 $(BIN_DIR)/libdds_analyzer.so $(PREFIX)/lib/
	install -m 755 $(BIN_DIR)/libdds_monitor.so $(PREFIX)/lib/
	install -m 644 ../src/common/log/dds_log.h $(PREFIX)/include/dds/
	install -m 644 dds_analyzer/dds_analyzer.h $(PREFIX)/include/dds/
	install -m 644 dds_monitor/qos_monitor.h $(PREFIX)/include/dds/
	install -m 644 dds_monitor/health_check.h $(PREFIX)/include/dds/
	ldconfig

# 卸载
uninstall:
	rm -f $(PREFIX)/bin/dds_monitor_cli
	rm -f $(PREFIX)/lib/libdds_analyzer.so
	rm -f $(PREFIX)/lib/libdds_monitor.so
	rm -rf $(PREFIX)/include/dds

# 清理
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# 帮助
help:
	@echo "DDS Monitoring Tools Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build all targets (default)"
	@echo "  debug    - Build with debug symbols"
	@echo "  install  - Install binaries and libraries"
	@echo "  uninstall- Remove installed files"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  CC       - C compiler (default: gcc)"
	@echo "  CFLAGS   - Compiler flags"
	@echo "  PREFIX   - Installation prefix (default: /usr/local)"
