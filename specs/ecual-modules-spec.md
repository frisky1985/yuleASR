# yuleASR — ECUAL Modules Specification

## CanIf (CAN Interface)

- The system SHALL support a maximum of 2 CAN controllers (CAN0, CAN1).
- The system SHALL support up to 512 PDU IDs.
- The system SHALL support transmit and receive PDU modes.
- The system SHALL support sleep and wakeup functionality.

## CanTp (CAN Transport Protocol)

- The system SHALL implement the ISO 15765-2 CAN transport protocol.
- The system SHALL support message segmentation up to 4095 bytes per message.
- The system SHALL support Continuous and Wait flow control modes.
- The system SHALL support Physical and Functional addressing.

## CanNm (CAN Network Management)

- The system SHALL implement AUTOSAR CAN Network Management protocol.
- The system SHALL support a configurable 8-bit node ID.
- The system SHALL support configurable message cycle time with default of 100ms.
- The system SHALL support configurable repeat message timer with default of 1000ms.
- The system SHALL support bus synchronization.

## SoAd (Socket Adaptor)

- The system SHALL support a maximum of 32 sockets.
- The system SHALL support TCP and UDP protocols.
- The system SHALL support Server and Client connection types.
- The system SHALL support SOME/IP protocol communication.

## SomeIpSd (SOME/IP Service Discovery)

- The system SHALL support an offer cycle of 1000ms for service discovery.
- The system SHALL support a request cycle of 2000ms for service discovery.
- The system SHALL support TTL multiplier of 3 for service entries.

## DLT

- The system SHALL support log levels including Fatal, Error, Warn, Info, Debug, and Verbose.
- The system SHALL support TCP and Serial transport for DLT messages.
- The system SHALL support Application ID filtering.

## XCP

- The system SHALL support CAN and Ethernet transport layers for XCP.
- The system SHALL implement XCP protocol version 1.5.
- The system SHALL support XCP slave functionality.
- The system SHALL support calibration page switching.
- The system SHALL support up to 8 DAQ lists.
