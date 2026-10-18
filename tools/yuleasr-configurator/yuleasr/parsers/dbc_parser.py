"""
DBC (CAN Database) file parser for AUTOSAR configuration generation.

Parses DBC files and generates configuration for:
- CanIf (CAN Interface)
- CanTp (CAN Transport Protocol)
- PduR (PDU Router)
- Com (Communication)
"""

import re
from dataclasses import dataclass, field
from typing import List, Dict, Optional
from enum import Enum


class SignalByteOrder(Enum):
    """Signal byte order (Intel = Little Endian, Motorola = Big Endian)"""
    INTEL = "Intel"
    MOTOROLA = "Motorola"


class SignalValueType(Enum):
    """Signal value type"""
    UNSIGNED = "Unsigned"
    SIGNED = "Signed"


@dataclass
class Signal:
    """CAN signal definition"""
    name: str
    start_bit: int
    length: int
    byte_order: SignalByteOrder
    value_type: SignalValueType
    factor: float = 1.0
    offset: float = 0.0
    min_value: float = 0.0
    max_value: float = 0.0
    unit: str = ""
    receivers: List[str] = field(default_factory=list)
    comment: str = ""


@dataclass
class Message:
    """CAN message definition"""
    id: int
    name: str
    length: int  # DLC in bytes
    transmitter: str
    signals: List[Signal] = field(default_factory=list)
    comment: str = ""


@dataclass
class Node:
    """ECU Node definition"""
    name: str
    comment: str = ""


@dataclass
class DbcFile:
    """Parsed DBC file structure"""
    version: str = ""
    nodes: List[Node] = field(default_factory=list)
    messages: List[Message] = field(default_factory=list)
    value_tables: Dict[str, Dict[int, str]] = field(default_factory=dict)


class DbcParser:
    """DBC file parser"""

    def __init__(self):
        self.dbc = DbcFile()
        self._current_message: Optional[Message] = None

    def parse(self, content: str) -> DbcFile:
        """Parse DBC file content"""
        lines = content.split('\n')
        i = 0

        while i < len(lines):
            line = lines[i].strip()

            # VERSION
            if line.startswith('VERSION '):
                self.dbc.version = line[8:].strip().strip('"')

            # NS_ (namespace)
            elif line.startswith('NS_ :'):
                pass  # Skip namespace definitions

            # BS_ (bus speed)
            elif line.startswith('BS_'):
                pass  # Skip bus speed

            # BU_ (nodes/ECUs)
            elif line.startswith('BU_'):
                nodes_str = line[4:].strip()
                if nodes_str:
                    for node_name in nodes_str.split():
                        self.dbc.nodes.append(Node(name=node_name))

            # BO_ (message)
            elif line.startswith('BO_ '):
                self._current_message = self._parse_message(line)
                if self._current_message:
                    self.dbc.messages.append(self._current_message)

            # SG_ (signal) - may have leading spaces in original file
            elif (line.startswith('SG_ ') or lines[i].startswith(' SG_ ')) and self._current_message:
                signal = self._parse_signal(line)
                if signal:
                    self._current_message.signals.append(signal)

            # CM_ (comment)
            elif line.startswith('CM_ '):
                self._parse_comment(line)

            # BA_DEF_ (attribute definition)
            elif line.startswith('BA_DEF_'):
                pass  # Skip attribute definitions

            # BA_ (attribute value)
            elif line.startswith('BA_ '):
                pass  # Skip attribute values

            # VAL_ (value description)
            elif line.startswith('VAL_ '):
                pass  # Skip value descriptions

            i += 1

        return self.dbc

    def _parse_message(self, line: str) -> Optional[Message]:
        """Parse message definition: BO_ <id> <name>: <length> <transmitter>"""
        match = re.match(r'BO_\s+(\d+)\s+(\w+)\s*:\s+(\d+)\s+(\w+)', line)
        if match:
            return Message(
                id=int(match.group(1)),
                name=match.group(2),
                length=int(match.group(3)),
                transmitter=match.group(4),
            )
        return None

    def _parse_signal(self, line: str) -> Optional[Signal]:
        """Parse signal definition: SG_ <name> : <start_bit>|<length>@<byte_order><value_type> (<factor>,<offset>) [<min>|<max>] "<unit>" <receivers>"""
        match = re.match(
            r'SG_\s+(\w+)\s*:\s*(\d+)\|(\d+)@([01])([+-])\s*\(([^,]+),([^)]+)\)\s*\[([^|]+)\|([^\]]+)\]\s*"([^"]*)"\s*(.*)',
            line
        )
        if match:
            byte_order = SignalByteOrder.INTEL if match.group(4) == '1' else SignalByteOrder.MOTOROLA
            value_type = SignalValueType.SIGNED if match.group(5) == '-' else SignalValueType.UNSIGNED

            receivers = [r.strip() for r in match.group(11).split(',') if r.strip()]

            return Signal(
                name=match.group(1),
                start_bit=int(match.group(2)),
                length=int(match.group(3)),
                byte_order=byte_order,
                value_type=value_type,
                factor=float(match.group(6)),
                offset=float(match.group(7)),
                min_value=float(match.group(8)),
                max_value=float(match.group(9)),
                unit=match.group(10),
                receivers=receivers,
            )
        return None

    def _parse_comment(self, line: str):
        """Parse comment: CM_ "<text>"; or CM_ BO_ <id> "<text>"; or CM_ SG_ <id> <name> "<text>";"""
        # Remove trailing semicolon
        line = line.rstrip(';').strip()

        # Message comment: CM_ BO_ <id> "<text>"
        match = re.match(r'CM_\s+BO_\s+(\d+)\s+"(.*)"', line)
        if match:
            msg_id = int(match.group(1))
            comment = match.group(2)
            for msg in self.dbc.messages:
                if msg.id == msg_id:
                    msg.comment = comment
                    break
            return

        # Signal comment: CM_ SG_ <id> <name> "<text>"
        match = re.match(r'CM_\s+SG_\s+(\d+)\s+(\w+)\s+"(.*)"', line)
        if match:
            msg_id = int(match.group(1))
            sig_name = match.group(2)
            comment = match.group(3)
            for msg in self.dbc.messages:
                if msg.id == msg_id:
                    for sig in msg.signals:
                        if sig.name == sig_name:
                            sig.comment = comment
                            break
                    break
            return

        # Node comment: CM_ BU_ <name> "<text>"
        match = re.match(r'CM_\s+BU_\s+(\w+)\s+"(.*)"', line)
        if match:
            node_name = match.group(1)
            comment = match.group(2)
            for node in self.dbc.nodes:
                if node.name == node_name:
                    node.comment = comment
                    break
            return

        # General comment: CM_ "<text>"
        match = re.match(r'CM_\s+"(.*)"', line)
        if match:
            pass  # Could store as file-level comment


class AutosarConfigGenerator:
    """Generate AUTOSAR configuration from parsed DBC file"""

    def __init__(self, dbc: DbcFile):
        self.dbc = dbc

    def generate_canif_config(self) -> Dict:
        """Generate CanIf module configuration"""
        tx_l_pdus = []
        rx_l_pdus = []

        for msg in self.dbc.messages:
            # TX L-PDU
            tx_l_pdus.append({
                'CanIfTxPduId': f'CanIfTxPdu_{msg.name}',
                'CanIfTxPduCanId': msg.id,
                'CanIfTxPduDlc': msg.length,
                'CanIfTxPduType': 'CAN_BASIC',
                'CanIfTxPduCanIdType': 'EXTENDED' if msg.id > 0x7FF else 'STANDARD',
            })

            # RX L-PDU
            rx_l_pdus.append({
                'CanIfRxPduId': f'CanIfRxPdu_{msg.name}',
                'CanIfRxPduCanId': msg.id,
                'CanIfRxPduDlc': msg.length,
                'CanIfRxPduUserRxIndicationUL': 'COM',
            })

        return {
            'CanIfInitCfg': {
                'CanIfNumberOfCanRxPduIds': len(rx_l_pdus),
                'CanIfNumberOfCanTxPduIds': len(tx_l_pdus),
                'CanIfNumberOfDynamicCanRxPduIds': 0,
            },
            'CanIfTxL_pduCfg': tx_l_pdus,
            'CanIfRxL_pduCfg': rx_l_pdus,
        }

    def generate_com_config(self) -> Dict:
        """Generate Com module configuration"""
        signal_groups = []
        ipdu_groups = []

        for msg in self.dbc.messages:
            # I-PDU
            ipdu = {
                'ComIPduId': f'ComIPdu_{msg.name}',
                'ComIPduDirection': 'SEND' if msg.transmitter == 'Vector__XXX' else 'RECEIVE',
                'ComIPduLength': msg.length * 8,  # bits
                'ComIPduSignalProcessing': 'DEFERRED',
                'ComIPduCallout': 'NULL',
            }
            ipdu_groups.append(ipdu)

            # Signals
            for sig in msg.signals:
                signal = {
                    'ComSignalId': f'ComSignal_{msg.name}_{sig.name}',
                    'ComBitPosition': sig.start_bit,
                    'ComBitSize': sig.length,
                    'ComSignalType': self._get_com_signal_type(sig),
                    'ComSignalEndianness': 'LITTLE_ENDIAN' if sig.byte_order == SignalByteOrder.INTEL else 'BIG_ENDIAN',
                    'ComSignalInitValue': sig.offset,
                    'ComSignalApplDataFormat': 'NATIVE',
                    'ComFirstTimeoutFactor': 0,
                    'ComTimeoutFactor': 0,
                }
                signal_groups.append(signal)

        return {
            'ComConfig': {
                'ComIPdu': ipdu_groups,
                'ComSignal': signal_groups,
            }
        }

    def generate_pdur_config(self) -> Dict:
        """Generate PduR module configuration"""
        routes = []

        for msg in self.dbc.messages:
            route = {
                'PduRRoutingPathId': f'PduRRoute_{msg.name}',
                'PduRSrcPduId': f'CanIfRxPdu_{msg.name}',
                'PduRDestPduId': f'ComIPdu_{msg.name}',
                'PduRRoutingPathActivation': 'ALWAYS',
            }
            routes.append(route)

        return {
            'PduRConfig': {
                'PduRRoutingPaths': routes,
            }
        }

    def _get_com_signal_type(self, signal: Signal) -> str:
        """Map DBC signal to Com signal type"""
        if signal.length <= 8:
            return 'uint8' if signal.value_type == SignalValueType.UNSIGNED else 'sint8'
        elif signal.length <= 16:
            return 'uint16' if signal.value_type == SignalValueType.UNSIGNED else 'sint16'
        elif signal.length <= 32:
            return 'uint32' if signal.value_type == SignalValueType.UNSIGNED else 'sint32'
        else:
            return 'uint64' if signal.value_type == SignalValueType.UNSIGNED else 'sint64'


def parse_dbc_file(file_path: str) -> DbcFile:
    """Parse a DBC file and return the structure"""
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    parser = DbcParser()
    return parser.parse(content)


def generate_autosar_config_from_dbc(file_path: str) -> Dict:
    """Parse DBC file and generate complete AUTOSAR configuration"""
    dbc = parse_dbc_file(file_path)
    generator = AutosarConfigGenerator(dbc)

    return {
        'CanIf': generator.generate_canif_config(),
        'Com': generator.generate_com_config(),
        'PduR': generator.generate_pdur_config(),
        'metadata': {
            'version': dbc.version,
            'node_count': len(dbc.nodes),
            'message_count': len(dbc.messages),
            'signal_count': sum(len(m.signals) for m in dbc.messages),
        }
    }


if __name__ == '__main__':
    import sys
    import json

    if len(sys.argv) < 2:
        print("Usage: python dbc_parser.py <dbc_file>")
        sys.exit(1)

    result = generate_autosar_config_from_dbc(sys.argv[1])
    print(json.dumps(result, indent=2))
