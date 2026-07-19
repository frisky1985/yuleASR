# yuleASR — BSW Services Specification

## Diagnostic Services Specification

### DCM (Diagnostic Communication Manager)

- The system SHALL support UDS service IDs 0x10, 0x11, 0x14, 0x19, 0x22, 0x2E, 0x31, 0x34, 0x36, 0x37 as specified in ISO 14229-1.
- The system SHALL support a maximum of 4 concurrent diagnostic sessions.
- The system SHALL enforce P2 timeout of 50ms for diagnostic responses.
- The system SHALL enforce P2\* timeout of 500ms for diagnostic responses.

### DEM (Diagnostic Event Manager)

- The system SHALL support storage of up to 256 diagnostic trouble codes (DTCs).
- The system SHALL support 3 event priority levels: Low, Medium, High.
- The system SHALL store diagnostic events with primary and secondary (freeze frame) data.
- The system SHALL provide a configurable aging counter with default 40 cycles.

## Communication Services Specification

### COM

- The system SHALL support a configurable signal count with default of 1024 signals.
- The system SHALL support signal group communication.
- The system SHALL support I-PDU send and receive directions.
- The system SHALL support deadline monitoring for signal transmission.

### PduR

- The system SHALL maintain a static routing table generated at build time.
- The system SHALL support a maximum of 512 routing paths.
- The system SHALL support gateway routing between CAN ↔ LIN and CAN ↔ Ethernet.

## Memory Services Specification

### NvM

- The system SHALL support native, redundant, and dataset NVM block management.
- The system SHALL use CRC-32 for write verification.
- The system SHALL support block sizes from 1 to 65536 bytes.
- The system SHALL support a maximum of 512 NVM blocks.
- The system SHALL support 4 job priority levels.

## System Services Specification

### EcuM

- The system SHALL support startup phases STARTUP_ONE and STARTUP_TWO.
- The system SHALL support shutdown targets OFF, RESET, and SLEEP.
- The system SHALL support wakeup sources including CAN, LIN, Ethernet, Pin, and Timer.

### OS (AUTOSAR SC4)

- The system SHALL provide fixed cyclic schedule tables.
- The system SHALL support BCC2 and ECC2 task conformance classes.
- The system SHALL support a maximum of 64 tasks.
- The system SHALL support a maximum of 32 alarms.
- The system SHALL implement the priority ceiling protocol for resource management.
