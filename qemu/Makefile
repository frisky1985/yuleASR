#=============================================================================
# Makefile -- yuleASR QEMU Simulation Build
#
# Targets:
#   make all        — Build yuleasr_qemu.elf
#   make run        — Build + run in QEMU (nographic)
#   make debug      — Build + run with GDB server
#   make clean      — Remove build artifacts
#   make size       — Show section sizes
#=============================================================================

# Toolchain
PREFIX   ?= arm-none-eabi
CC       := $(PREFIX)-gcc
LD       := $(PREFIX)-gcc
AS       := $(PREFIX)-gcc
OBJCOPY  := $(PREFIX)-objcopy
OBJDUMP  := $(PREFIX)-objdump
SIZE     := $(PREFIX)-size
GDB      := $(PREFIX)-gdb
QEMU     := qemu-system-arm

# Target
TARGET   := yuleasr_qemu
BOARD    := mps2-an500
CPU      := cortex-m7

# Directories
BUILD_DIR ?= build

# Source files
SRCS     := main_qemu.c
ASMS     := startup.S

# Object files
OBJS     := $(SRCS:%.c=$(BUILD_DIR)/%.o) $(ASMS:%.S=$(BUILD_DIR)/%.o)
ELF      := $(BUILD_DIR)/$(TARGET).elf
BIN      := $(BUILD_DIR)/$(TARGET).bin

# Linker script
LDSCRIPT := link.ld

# Compiler flags
CFLAGS   := -mcpu=$(CPU) -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
CFLAGS   += -Wall -Wextra -Werror -Wno-unused-parameter
CFLAGS   += -O0 -g3
CFLAGS   += -fdata-sections -ffunction-sections
CFLAGS   += -nostartfiles -nodefaultlibs -nostdlib
CFLAGS   += -D__QEMU__ -D__CORTEX_M7__ -DS32K312
CFLAGS   += -ffreestanding
CFLAGS   += -I.
CFLAGS   += -I../src/platform/s32k312/include

# Assembly flags
ASFLAGS  := -mcpu=$(CPU) -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
ASFLAGS  += -Wall -g3
ASFLAGS  += -x assembler-with-cpp
ASFLAGS  += -D__QEMU__

# Linker flags
LDFLAGS  := -mcpu=$(CPU) -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
LDFLAGS  += -nostartfiles -nodefaultlibs -nostdlib
LDFLAGS  += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS  += -Wl,-Map,$(BUILD_DIR)/$(TARGET).map
LDFLAGS  += -T $(LDSCRIPT)

# QEMU flags
QEMUFLAGS := -M $(BOARD) -cpu $(CPU)
QEMUFLAGS += -nographic -semihosting
QEMUFLAGS += -kernel $(ELF)

# QEMU debug flags (GDB server on port 1234)
QEMUDBG   := -s -S

# Default target
.PHONY: all
all: $(ELF) $(BIN)

# Link
$(ELF): $(OBJS) $(LDSCRIPT)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	$(SIZE) $@

# Binary
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# Compile C
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Assemble
$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) -c -o $@ $<

# Run in QEMU
.PHONY: run
run: $(ELF)
	$(QEMU) $(QEMUFLAGS)

# Debug (QEMU GDB server)
.PHONY: debug
debug: $(ELF)
	@echo "Starting QEMU with GDB server on port 1234..."
	@echo "Connect: $(GDB) $(ELF) -ex 'target remote :1234'"
	$(QEMU) $(QEMUFLAGS) $(QEMUDBG)

# Disassemble
.PHONY: disasm
disasm: $(ELF)
	$(OBJDUMP) -d $(ELF) | head -100

# Show sizes
.PHONY: size
size: $(ELF)
	$(SIZE) $(ELF)

# Clean
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Help
.PHONY: help
help:
	@echo "yuleASR QEMU Simulation Build"
	@echo ""
	@echo "Targets:"
	@echo "  make          -- Build ELF + BIN"
	@echo "  make run      -- Build and run in QEMU"
	@echo "  make debug    -- Build and run QEMU GDB server"
	@echo "  make clean    -- Remove build artifacts"
	@echo "  make size     -- Show section sizes"
	@echo "  make disasm   -- Disassemble first 100 instructions"
	@echo ""
	@echo "Variables:"
	@echo "  PREFIX=arm-none-eabi  (default)"
	@echo "  BUILD_DIR=build       (default)"
