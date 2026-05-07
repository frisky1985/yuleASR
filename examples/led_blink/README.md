# LED Blink Example

## Overview
This example demonstrates basic DIO and GPT usage.

## Hardware Requirements
- STM32F4xx Nucleo board (or compatible)
- User LED on PA5

## Building
```bash
cd examples/led_blink
mkdir build && cd build
cmake ..
make
```

## Flashing
```bash
st-flash write led_blink.bin 0x08000000
```

## Expected Behavior
The user LED (LD2) should blink at 1Hz (500ms on, 500ms off).
