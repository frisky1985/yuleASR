# yuleASR — BSW Services Specification

## Diagnostic Services Specification

### DCM (Diagnostic Communication Manager)
- UDS service support: 0x10, 0x11, 0x14, 0x19, 0x22, 0x2E, 0x31, 0x34, 0x36, 0x37
- Max concurrent sessions: 4
- P2 timeout: 50ms
- P2* timeout: 500ms

### DEM (Diagnostic Event Manager)
- Max DTC storage: 256 events
- Event priority levels: 3 (Low/Medium/High)
- Storage format: Primary + secondary (freeze frame) data
- Aging counter: Configurable (default 40 cycles)

## Communication Services Specification

### COM
- Signal count: Configurable (default 1024)
- Signal group support: Yes
- I-PDU direction: Send/Receive
- Deadline monitoring: Yes

### PduR
- Routing table: Static (generated at build time)
- Max routing paths: 512
- Gateway support: Yes (CAN ↔ LIN, CAN ↔ Eth)

## Memory Services Specification

### NvM
- Block management: Native, Redundant, Dataset
- Write verification: CRC-32
- Block size: 1-65536 bytes
- Max blocks: 512
- Job priority levels: 4

## System Services Specification

### EcuM
- Startup phases: STARTUP_ONE, STARTUP_TWO
- Shutdown targets: OFF, RESET, SLEEP
- Wakeup sources: CAN, LIN, Ethernet, Pin, Timer

### OS (AUTOSAR SC4)
- Schedule tables: Fixed cyclic
- Tasks: BCC2 + ECC2
- Max tasks: 64
- Max alarms: 32
- Resource management: YES, priority ceiling protocol
