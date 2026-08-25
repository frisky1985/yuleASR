# ==============================================================================
# yuleASR Unified Build System — Top-level Makefile
# ==============================================================================
#
# This Makefile wraps the CMake build system for convenience.
# It supports both native (host) and ARM cross-compilation targets.
#
# Usage:
#   make                  # Native debug build
#   make TARGET=arm       # ARM cross-compile for S32K312
#   make test             # Build and run unit tests
#   make clean            # Clean all build artifacts
#   make MODULE=mcal      # Build specific module
#   make release          # Release build
#   make coverage         # Build with coverage + run tests
#
# For full options, use: ./build.sh --help
#
# Copyright (c) 2024 YuleTech
# ==============================================================================

# ── Configuration ───────────────────────────────────────────────────────────
PROJECT_DIR := $(shell pwd)
TARGET     ?= native
BUILD_TYPE ?= Debug
MODULE     ?=
JOBS       ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
VERBOSE    ?= 0

# Build directories
BUILD_DIR_NATIVE := $(PROJECT_DIR)/build-native
BUILD_DIR_ARM   := $(PROJECT_DIR)/build-arm

ifeq ($(TARGET),arm)
    BUILD_DIR := $(BUILD_DIR_ARM)
    TOOLCHAIN_ARG := -DCMAKE_TOOLCHAIN_FILE=$(PROJECT_DIR)/cmake/toolchain-arm-none-eabi.cmake
else
    BUILD_DIR := $(BUILD_DIR_NATIVE)
    TOOLCHAIN_ARG :=
endif

# ── ARM cross-compile probe (for yuleOSH L2 gate) ──────────────────────────
ARM_CC      ?= arm-none-eabi-gcc
ARM_FLAGS   := -mcpu=cortex-m33 -mthumb -ffreestanding -nostdlib -Wl,-e,main

# ── Verbosity ───────────────────────────────────────────────────────────────
ifeq ($(VERBOSE),1)
    CMAKE_VERBOSE := --verbose
    Q :=
else
    CMAKE_VERBOSE :=
    Q := @
endif

# ── Common CMake arguments ──────────────────────────────────────────────────
CMAKE_ARGS := \
    -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
    -DBUILD_TESTING=$(BUILD_TESTING) \
    -DYULE_ENABLE_MCAL=$(YULE_ENABLE_MCAL) \
    $(TOOLCHAIN_ARG)

BUILD_TESTING    ?= OFF
YULE_ENABLE_MCAL ?= ON

# ── Default goal ────────────────────────────────────────────────────────────
.DEFAULT_GOAL := all

.PHONY: all configure build test clean install package \
        arm native release coverage docs \
        help probe size analysis

# ── Primary targets ─────────────────────────────────────────────────────────

all: configure build

configure:
	$(Q)echo "=== Configuring ($(TARGET), $(BUILD_TYPE)) ==="
	$(Q)mkdir -p $(BUILD_DIR)
	$(Q)cmake -B $(BUILD_DIR) -S $(PROJECT_DIR) $(CMAKE_ARGS)

build:
	$(Q)echo "=== Building ($(TARGET), $(BUILD_TYPE)) ==="
ifeq ($(MODULE),)
	$(Q)cmake --build $(BUILD_DIR) --parallel $(JOBS) $(CMAKE_VERBOSE)
else
	$(Q)cmake --build $(BUILD_DIR) --target $(MODULE) --parallel $(JOBS) $(CMAKE_VERBOSE)
endif

test:
	$(Q)echo "=== Building and running tests ==="
	$(Q)mkdir -p $(BUILD_DIR)
	$(Q)cmake -B $(BUILD_DIR) -S $(PROJECT_DIR) $(CMAKE_ARGS) -DBUILD_TESTING=ON
	$(Q)cmake --build $(BUILD_DIR) --parallel $(JOBS) $(CMAKE_VERBOSE)
	$(Q)ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	$(Q)echo "=== Cleaning build artifacts ==="
	$(Q)rm -rf $(BUILD_DIR_NATIVE) $(BUILD_DIR_ARM) $(PROJECT_DIR)/build
	$(Q)echo "Clean complete."

install:
	$(Q)cmake --install $(BUILD_DIR)

package:
	$(Q)cmake --build $(BUILD_DIR) --target package

# ── Convenience targets ─────────────────────────────────────────────────────

arm:
	$(Q)$(MAKE) TARGET=arm all

native:
	$(Q)$(MAKE) TARGET=native all

release:
	$(Q)$(MAKE) BUILD_TYPE=Release all

coverage:
	$(Q)echo "=== Coverage build ==="
	$(Q)mkdir -p $(BUILD_DIR_NATIVE)
	$(Q)cmake -B $(BUILD_DIR_NATIVE) -S $(PROJECT_DIR) \
	    -DCMAKE_BUILD_TYPE=Debug \
	    -DBUILD_TESTING=ON \
	    -DENABLE_COVERAGE=ON \
	    -DYULE_ENABLE_MCAL=$(YULE_ENABLE_MCAL)
	$(Q)cmake --build $(BUILD_DIR_NATIVE) --parallel $(JOBS)
	$(Q)ctest --test-dir $(BUILD_DIR_NATIVE) --output-on-failure
	$(Q)echo "=== Coverage data generated in $(BUILD_DIR_NATIVE) ==="

docs:
	$(Q)echo "=== Building documentation ==="
	$(Q)mkdir -p $(BUILD_DIR_NATIVE)
	$(Q)cmake -B $(BUILD_DIR_NATIVE) -S $(PROJECT_DIR) -DBUILD_DOCUMENTATION=ON
	$(Q)cmake --build $(BUILD_DIR_NATIVE) --target doc

# ── ARM cross-compile probe (yuleOSH L2 gate) ──────────────────────────────
# This target produces build/*.elf for the yuleOSH L2 verification stage.
# It compiles a minimal probe to verify the ARM toolchain is functional.
probe:
	$(Q)echo "=== ARM cross-compile probe (Cortex-M33) ==="
	$(Q)mkdir -p $(PROJECT_DIR)/build
	$(Q)$(ARM_CC) $(ARM_FLAGS) $(PROJECT_DIR)/src/cross/hello.c \
	    -o $(PROJECT_DIR)/build/yuleasr-cross-probe.elf
	$(Q)echo "Probe compiled: build/yuleasr-cross-probe.elf"
	$(Q)arm-none-eabi-size $(PROJECT_DIR)/build/yuleasr-cross-probe.elf 2>/dev/null || true

# ── Size report for ARM builds ──────────────────────────────────────────────
size:
	$(Q)if [ -d "$(BUILD_DIR)/bin" ]; then \
	    echo "=== Binary sizes ==="; \
	    for elf in $(BUILD_DIR)/bin/*.elf; do \
	        [ -f "$$elf" ] && arm-none-eabi-size "$$elf" 2>/dev/null || size "$$elf" 2>/dev/null || true; \
	    done; \
	else \
	    echo "No binaries found in $(BUILD_DIR)/bin"; \
	fi

# ── Static analysis ─────────────────────────────────────────────────────────
analysis:
	$(Q)echo "=== Running static analysis ==="
	$(Q)mkdir -p $(BUILD_DIR_NATIVE)
	$(Q)cmake -B $(BUILD_DIR_NATIVE) -S $(PROJECT_DIR) -DBUILD_TESTING=OFF
	$(Q)cmake --build $(BUILD_DIR_NATIVE) --target analysis

# ── Help ────────────────────────────────────────────────────────────────────
help:
	@echo "yuleASR Build System"
	@echo ""
	@echo "Usage: make [TARGET] [VARIABLES]"
	@echo ""
	@echo "Targets:"
	@echo "  all (default)  Configure and build"
	@echo "  configure      Run CMake configure only"
	@echo "  build          Build all targets"
	@echo "  test           Build and run unit tests"
	@echo "  clean          Clean all build artifacts"
	@echo "  install        Install headers and libraries"
	@echo "  package        Create distributable package"
	@echo "  arm            Cross-compile for ARM (S32K312)"
	@echo "  native         Native host build"
	@echo "  release        Release build"
	@echo "  coverage       Build with coverage + run tests"
	@echo "  docs           Build API documentation"
	@echo "  probe          ARM cross-compile probe (yuleOSH L2)"
	@echo "  size           Show binary sizes"
	@echo "  analysis       Run static analysis (cppcheck, clang-tidy)"
	@echo "  help           Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  TARGET=native|arm   Target architecture (default: native)"
	@echo "  BUILD_TYPE=...      Debug|Release|MinSizeRel|RelWithDebInfo"
	@echo "  MODULE=name         Build specific module only"
	@echo "  JOBS=N              Parallel build jobs (default: auto)"
	@echo "  VERBOSE=1           Verbose build output"
	@echo "  BUILD_TESTING=ON    Enable test build"
	@echo "  YULE_ENABLE_MCAL=ON Build MCAL drivers"
	@echo ""
	@echo "Examples:"
	@echo "  make                         # Native debug build"
	@echo "  make TARGET=arm              # ARM cross-compile"
	@echo "  make test                    # Build + run tests"
	@echo "  make MODULE=mcal             # Build MCAL only"
	@echo "  make release                 # Release build"
	@echo "  make coverage                # Coverage build"
	@echo "  make TARGET=arm MODULE=os    # Cross-compile OS module"
