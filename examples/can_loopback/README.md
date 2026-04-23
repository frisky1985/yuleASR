# CAN Loopback Example

## Overview
This example demonstrates CAN communication using loopback mode.

## Hardware Requirements
- STM32F4xx Nucleo board with CAN transceiver
- CAN bus termination resistor (120 Ohm)

## Building
```bash
cd examples/can_loopback
mkdir build && cd build
cmake ..
make
```

## Running
```bash
st-flash write can_loopback.bin 0x08000000
```

## Expected Behavior
The board continuously transmits and receives CAN frames.
