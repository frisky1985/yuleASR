# yuleASR — MCAL Drivers Specification

## ADC Driver
- Resolution: 10-bit, 12-bit configurable
- Conversion modes: Single, Continuous, Scan
- Max channels: 16 per ADC instance
- Result alignment: Left/Right
- Notification: Interrupt-based, polling

## CAN Driver
- Protocol: Classical CAN (2.0B) + CAN FD
- Bit rates: 125kbps to 1Mbps (CAN), up to 8Mbps (CAN FD)
- Mailboxes: 64
- FIFO support: Yes
- Loopback mode: Yes
- Bus-off recovery: Automatic (AUTOSAR compliant)

## Crypto Driver
- Algorithms: AES-128/256 (ECB, CBC, CTR), SHA-256, ECC P-256
- HSM acceleration: Yes, S32K312 HSM
- Key storage: HSM secure NVM
- TRNG: Integrated hardware TRNG
- MbedTLS fallback: Yes (for SIL simulation)

## DIO Driver
- Ports/Channels: 8 ports × 32 pins
- Direction: Configurable per pin
- Level: HIGH/LOW
- Interrupt support: Edge-triggered (rising, falling, both)

## PORT Driver
- Pin configurations: ~100 pins
- Mux modes: ALT0-ALT7
- Pad properties: Pull-up/down, slew rate, drive strength

## GPT Driver
- Timers: 8 hardware channels
- Resolution: 32-bit
- Prescaler: 1-65536
- Mode: One-shot, Continuous

## ICU Driver
- Input capture: Up to 8 channels
- Signal measurement: Period, duty cycle, pulse width
- Edge detection: Rising, Falling, Both

## MCU Driver
- Clock sources: SOSC, SIRC, FIRC, PLL, SPLL
- RAM sections: 4 banks
- Power modes: RUN, SLEEP, STOP, STANDBY
- Reset sources: POR, WDG, SW, External

## WDG Driver
- Watchdog type: Internal watchdog (WDOG)
- Timeout range: Configurable (ms to seconds)
- Window mode: Yes
- Test mode: Yes (for diagnostic testing)
