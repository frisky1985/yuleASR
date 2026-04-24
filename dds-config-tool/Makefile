# DDS Configuration Tool - Makefile
#
# Copyright (c) 2024 DDS Config Tool Authors
# SPDX-License-Identifier: MIT

# Project name
PROJECT := dds-config
VERSION := 1.0.0

# Directories
SRC_DIR := src
INC_DIR := include
OBJ_DIR := build/obj
BIN_DIR := build/bin
EXAMPLES_DIR := examples
TEST_DIR := tests

# Source files
SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

# Additional library flags for new modules
LDLIBS += -lpthread

# Headers
HEADERS := $(wildcard $(INC_DIR)/*.h)

# Compiler settings
CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -pedantic
CFLAGS += -I$(INC_DIR)
CFLAGS += -D_POSIX_C_SOURCE=200809L
CFLAGS += -DDDS_CONFIG_VERSION=\"$(VERSION)\"

# Debug/Release settings
ifeq ($(DEBUG),1)
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O2 -DNDEBUG
endif

# Linker settings
LDFLAGS :=
LDLIBS :=

# Target binary
TARGET := $(BIN_DIR)/$(PROJECT)

# Installation directories
PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin
INCLUDEDIR := $(PREFIX)/include/$(PROJECT)
LIBDIR := $(PREFIX)/lib
DATADIR := $(PREFIX)/share/$(PROJECT)

# Default target
.PHONY: all clean install uninstall test format lint doc

all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

# Build target
$(TARGET): $(OBJECTS)
	@echo "LINK $@"
	@$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	@echo "Built: $@"

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Clean complete."

# Install target
install: all
	@echo "Installing $(PROJECT)..."
	@mkdir -p $(BINDIR)
	@mkdir -p $(INCLUDEDIR)
	@mkdir -p $(DATADIR)
	@cp $(TARGET) $(BINDIR)/
	@cp $(HEADERS) $(INCLUDEDIR)/
	@cp $(EXAMPLES_DIR)/*.yaml $(DATADIR)/
	@echo "Installed to $(PREFIX)"
	@echo "  Binary: $(BINDIR)/$(PROJECT)"
	@echo "  Headers: $(INCLUDEDIR)/"
	@echo "  Examples: $(DATADIR)/"

# Uninstall target
uninstall:
	@echo "Uninstalling $(PROJECT)..."
	@rm -f $(BINDIR)/$(PROJECT)
	@rm -rf $(INCLUDEDIR)
	@rm -rf $(DATADIR)
	@echo "Uninstall complete."

# Run tests
test: all
	@echo "Running tests..."
	@if [ -d "$(TEST_DIR)" ]; then \
		$(MAKE) -C $(TEST_DIR) test; \
	else \
		echo "No tests directory found."; \
	fi
	@echo "Testing configuration files..."
	@$(TARGET) validate $(EXAMPLES_DIR)/minimal_config.yaml
	@$(TARGET) validate $(EXAMPLES_DIR)/sample_dds_config.yaml
	@echo "All tests passed."

# Format source code with clang-format (if available)
format:
	@if command -v clang-format >/dev/null 2>&1; then \
		echo "Formatting source files..."; \
		clang-format -i $(SOURCES) $(HEADERS); \
		echo "Formatting complete."; \
	else \
		echo "clang-format not found. Install it to use formatting."; \
	fi

# Static analysis with cppcheck (if available)
lint:
	@if command -v cppcheck >/dev/null 2>&1; then \
		echo "Running static analysis..."; \
		cppcheck --enable=all --suppress=missingIncludeSystem \
		         -I$(INC_DIR) $(SOURCES); \
	else \
		echo "cppcheck not found. Install it to use linting."; \
	fi

# Generate documentation
doc:
	@echo "Generating documentation..."
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen Doxyfile 2>/dev/null || echo "Doxyfile not found."; \
	else \
		echo "doxygen not found. Install it to generate documentation."; \
	fi

# Create a distribution tarball
dist: clean
	@echo "Creating distribution tarball..."
	@mkdir -p dist
	@tar czf dist/$(PROJECT)-$(VERSION).tar.gz \
		--exclude=build \
		--exclude=dist \
		--exclude=.git \
		--transform "s|^|$(PROJECT)-$(VERSION)/|" \
		.
	@echo "Created: dist/$(PROJECT)-$(VERSION).tar.gz"

# Show help
help:
	@echo "DDS Configuration Tool - Makefile"
	@echo ""
	@echo "Usage: make [target] [options]"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build the project (default)"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install to PREFIX (default: /usr/local)"
	@echo "  uninstall  - Remove installed files"
	@echo "  test       - Run tests"
	@echo "  format     - Format source code with clang-format"
	@echo "  lint       - Run static analysis with cppcheck"
	@echo "  doc        - Generate documentation"
	@echo "  dist       - Create distribution tarball"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Options:"
	@echo "  DEBUG=1    - Build with debug symbols and no optimization"
	@echo "  PREFIX=path - Set installation prefix"
	@echo ""

# Dependencies
$(OBJ_DIR)/dds_config_cli.o: $(SRC_DIR)/dds_config_cli.c $(HEADERS)
$(OBJ_DIR)/dds_config_parser.o: $(SRC_DIR)/dds_config_parser.c $(HEADERS)
$(OBJ_DIR)/dds_config_validator.o: $(SRC_DIR)/dds_config_validator.c $(HEADERS)
$(OBJ_DIR)/security_config.o: $(SRC_DIR)/security_config.c $(HEADERS)
$(OBJ_DIR)/automotive_config.o: $(SRC_DIR)/automotive_config.c $(HEADERS)
$(OBJ_DIR)/code_generator.o: $(SRC_DIR)/code_generator.c $(HEADERS)
$(OBJ_DIR)/config_hot_reload.o: $(SRC_DIR)/config_hot_reload.c $(HEADERS)

# Print variable values (for debugging)
print-%:
	@echo $* = $($*)
