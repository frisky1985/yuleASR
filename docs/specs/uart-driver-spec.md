# yuleASR — UART Driver Specification

> OpenSpec 合规格式（RFC 2119 + GIVEN/WHEN/THEN 场景）
> 用途: yuleOSH pipeline 真实 LLM 端到端验证

## Requirements

- The system SHALL provide a UART driver with configurable baud rate from 9600 to 921600 bps.
- The system SHALL support 8-bit, 9-bit, and 10-bit data frame formats.
- The system SHALL support even, odd, and no parity modes.
- The system SHALL support 1 and 2 stop bits.
- The system SHALL provide interrupt-based transmit and receive notification.
- The system SHALL provide a ring buffer for received data with configurable depth.
- The system SHALL support hardware flow control via RTS and CTS signals.
- The system SHALL report transmission errors including overrun, framing, and parity errors.

## Scenario: UART initialization with valid configuration

- GIVEN a valid UART channel configuration with baud rate 115200 and 8N1 format
- WHEN the UART driver initializes the channel
- THEN the driver SHALL return success status
- AND the channel SHALL be ready for transmit and receive operations

## Scenario: UART receive with ring buffer

- GIVEN a configured UART channel with a 64-byte ring buffer
- WHEN data bytes arrive on the receive line
- THEN the driver SHALL store the bytes in the ring buffer
- AND the driver SHALL notify the application via the registered callback

## Scenario: UART error detection

- GIVEN a UART channel operating in 8N1 mode
- WHEN a framing error occurs on the receive line
- THEN the driver SHALL set the framing error flag
- AND the driver SHALL report the error through the error callback
