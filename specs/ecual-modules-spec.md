# yuleASR — ECUAL Modules Specification

## CanIf (CAN Interface)
- Max controllers: 2 (CAN0, CAN1)
- Max PDU IDs: 512
- PDU mode: Tx/Rx
- Sleep/wakeup support: Yes

## CanTp (CAN Transport Protocol)
- Protocol: ISO 15765-2
- Max segmentation: 4095 bytes per message
- Flow control: CTS (Continuous, Wait)
- Addressing: Physical, Functional

## CanNm (CAN Network Management)
- Protocol: AUTOSAR CanNm
- Node ID: Configurable (8-bit)
- Message cycle time: Configurable (default 100ms)
- Repeat message timer: Configurable (default 1000ms)
- Bus-synchronization: Yes

## SoAd (Socket Adaptor)
- Max sockets: 32
- Protocols: TCP, UDP
- Connection types: Server, Client
- Support for SOME/IP: Yes

## SomeIpSd (SOME/IP Service Discovery)
- Offer cycle: 1000ms
- Request cycle: 2000ms
- TTL multiplier: 3

## DLT
- Log levels: Fatal, Error, Warn, Info, Debug, Verbose
- Transport: TCP, Serial
- Application ID support: Yes

## XCP
- Transport layers: CAN, Ethernet
- Protocol version: XCP v1.5
- Slave support: Yes
- Calibration page switching: Yes
- DAQ lists: Up to 8
