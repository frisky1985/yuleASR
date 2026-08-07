# Makefile for ARM SIL test executable (QEMU lm3s6965evb)
# Toolchain: arm-none-eabi-gcc

CC       = arm-none-eabi-gcc
CFLAGS   = -mcpu=cortex-m3 -mthumb \
           -Os -ffreestanding -nostdlib -nostartfiles \
           -fdata-sections -ffunction-sections -Wall -Wextra
LDFLAGS  = -T link.ld -Wl,-gc-sections -Wl,-Map=hello.map
OBJCOPY  = arm-none-eabi-objcopy

SRCS     = hello.c startup.c
OBJS     = hello.o startup.o
TARGET   = hello.elf
BIN      = hello.bin

.PHONY: all clean

all: $(BIN)

hello.o: hello.c
	$(CC) $(CFLAGS) -c hello.c -o hello.o

startup.o: startup.c
	$(CC) $(CFLAGS) -c startup.c -o startup.o

$(TARGET): $(OBJS) link.ld
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(TARGET)

$(BIN): $(TARGET)
	$(OBJCOPY) -O binary $(TARGET) $(BIN)
	@echo "Built: $(TARGET) ($(BIN))"

clean:
	rm -f $(OBJS) $(TARGET) $(BIN) hello.map
