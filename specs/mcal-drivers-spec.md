# yuleASR — MCAL Drivers Specification

## ADC Driver

- The system SHALL support 10-bit and 12-bit configurable ADC resolution.
- The system SHALL support Single, Continuous, and Scan conversion modes.
- The system SHALL support up to 16 channels per ADC instance.
- The system SHALL support left and right result alignment.
- The system SHALL support interrupt-based and polling notification modes.

## CAN Driver

- The system SHALL support Classical CAN (2.0B) and CAN FD protocols.
- The system SHALL support bit rates from 125kbps to 1Mbps for CAN and up to 8Mbps for CAN FD.
- The system SHALL provide 64 mailboxes for CAN message buffering.
- The system SHALL support FIFO mode for CAN message reception.
- The system SHALL support loopback mode for self-test.
- The system SHALL provide automatic bus-off recovery conforming to AUTOSAR specification.

## Crypto Driver

- The system SHALL support AES-128/256 encryption in ECB, CBC, and CTR modes.
- The system SHALL support SHA-256 hashing.
- The system SHALL support ECC P-256 elliptic curve cryptography.
- The system SHALL accelerate cryptographic operations using S32K312 HSM.
- The system SHALL store cryptographic keys in HSM secure NVM.
- The system SHALL provide integrated hardware TRNG.
- The system SHALL provide MbedTLS fallback for SIL simulation.

## DIO Driver

- The system SHALL support 8 ports with 32 pins each for digital I/O.
- The system SHALL support configurable pin direction per pin.
- The system SHALL support HIGH and LEVEL output levels.
- The system SHALL support edge-triggered interrupt on rising, falling, and both edges.

## PORT Driver

- The system SHALL support pin mux configuration for approximately 100 pins.
- The system SHALL support ALT0 through ALT7 mux modes.
- The system SHALL support configurable pad properties including pull-up, pull-down, slew rate, and drive strength.

## GPT Driver

- The system SHALL provide 8 hardware timer channels.
- The system SHALL provide 32-bit timer resolution.
- The system SHALL support prescaler values from 1 to 65536.
- The system SHALL support one-shot and continuous timer modes.

## ICU Driver

- The system SHALL support up to 8 input capture channels.
- The system SHALL support signal period, duty cycle, and pulse width measurement.
- The system SHALL support rising, falling, and both edge detection.

## MCU Driver

- The system SHALL support clock sources SOSC, SIRC, FIRC, PLL, and SPLL.
- The system SHALL support 4 RAM section banks.
- The system SHALL support RUN, SLEEP, STOP, and STANDBY power modes.
- The system SHALL support POR, WDG, SW, and External reset sources.

## WDG Driver

- The system SHALL provide configurable watchdog timeout from milliseconds to seconds.
- The system SHALL support window mode watchdog operation.
- The system SHALL support test mode for diagnostic testing.
