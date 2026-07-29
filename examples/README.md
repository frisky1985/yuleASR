# YuleTech BSW Examples

This directory contains example projects demonstrating the usage of YuleTech AutoSAR BSW.

## Examples

### 1. LED Blink (`led_blink/`)

Simple LED blink example demonstrating basic GPIO control using Dio and Gpt drivers.

**Features:**
- Mcu initialization
- Port configuration
- Dio channel control
- Gpt timer with callback

**Hardware Requirements:**
- LED connected to GPIO pin (default: Port A, Pin 0)
- Timer peripheral

**Build:**
```bash
cd examples/led_blink
make
```

**Usage:**
The LED will blink at 500ms intervals (1Hz).

### 2. CAN Communication Demo (`can_demo/`)

CAN communication example demonstrating message transmission and reception.

**Features:**
- CAN controller initialization
- CAN message transmission
- CAN message reception with callback
- Message echo functionality

**Hardware Requirements:**
- CAN transceiver
- CAN bus connection

**Build:**
```bash
cd examples/can_demo
make
```

**Usage:**
The demo will:
1. Transmit periodic messages (ID: 0x100)
2. Receive messages (ID: 0x200)
3. Echo received messages back with incremented data

## Building Examples

### Prerequisites

- ARM GCC toolchain
- CMake 3.20+
- Make

### Build Steps

1. Set environment variables:
```bash
export ARM_GCC_PATH=/path/to/arm-none-eabi-gcc
```

2. Build:
```bash
cd examples/<example_name>
mkdir build && cd build
cmake ..
make
```

3. Flash to target:
```bash
make flash
```

## Creating New Examples

To create a new example:

1. Create directory:
```bash
mkdir examples/my_example
```

2. Create `main.c`:
```c
#include "Mcu.h"
#include "Port.h"

int main(void)
{
    /* Initialize BSW */
    Mcu_Init(NULL);
    Port_Init(NULL);
    
    /* Your application code */
    
    while (1)
    {
        /* Main loop */
    }
    
    return 0;
}
```

3. Create `CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.20)
project(my_example)

# Include BSW
include_directories(${BSW_ROOT}/src/bsw/mcal/mcu/include)

# Source files
set(SOURCES main.c)

# Create executable
add_executable(my_example ${SOURCES})
```

## Debugging

### Using GDB

```bash
arm-none-eabi-gdb build/my_example.elf
(gdb) target remote localhost:3333
(gdb) load
(gdb) break main
(gdb) continue
```

### Using IDE

Import the example into your preferred IDE:
- **Eclipse**: Import as Makefile project
- **VS Code**: Use Cortex-Debug extension
- **Keil**: Import using uVision project wizard

## Troubleshooting

### LED not blinking
- Check GPIO pin configuration
- Verify clock settings
- Check LED polarity

### CAN not working
- Verify CAN transceiver connection
- Check baudrate settings
- Verify termination resistor

## License

Copyright (c) 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
All Rights Reserved.
