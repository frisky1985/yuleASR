#!/usr/bin/env python3
"""DBC Importer - Parse CAN Database (.dbc) files"""

import re
from pathlib import Path
from typing import Dict, Any, Optional, List, Tuple


class DbcMessage:
    """Represents a CAN message from DBC"""
    def __init__(self, msg_id: int, name: str, dlc: int, transmitter: str):
        self.id = msg_id
        self.name = name
        self.dlc = dlc
        self.transmitter = transmitter
        self.signals: List[DbcSignal] = []


class DbcSignal:
    """Represents a CAN signal from DBC"""
    def __init__(self, name: str, start_bit: int, length: int, byte_order: str,
                 value_type: str, factor: float, offset: float, 
                 min_val: float, max_val: float, unit: str,
                 receivers: List[str]):
        self.name = name
        self.start_bit = start_bit
        self.length = length
        self.byte_order = byte_order  # 'Intel' or 'Motorola'
        self.value_type = value_type  # '+' or '-'
        self.factor = factor
        self.offset = offset
        self.min = min_val
        self.max = max_val
        self.unit = unit
        self.receivers = receivers


class DbcImporter:
    """Import CAN Database (.dbc) files"""
    
    # Pattern: BO_ message_id message_name: dlc transmitter
    BO_PATTERN = re.compile(
        r'BO_\s+(\d+)\s+(\w+)\s*:\s*(\d+)\s+(\w+)'
    )
    # Pattern: SG_ signal_name : start_bit|length@byte_order+value_type (factor,offset) [min|max] "unit" receivers
    SG_PATTERN = re.compile(
        r'SG_\s+(\w+)\s+:\s+(\d+)\|(\d+)@(\d+)([+-])\s+\(([^)]+)\)\s+\[([^\]]+)\]\s+"([^"]*)"\s+(.+)'
    )
    # CM_ pattern (comments)
    CM_PATTERN = re.compile(r'CM_\s+(SG_|BO_)\s+(\d+)\s+(\w+)\s+"(.+)"')
    # BA_ pattern (attribute definitions)
    BA_DEF_PATTERN = re.compile(r'BA_DEF_\s+\w+\s+"(\w+)"\s+(\w+)\s+(\d+|"[^"]*")')
    
    def __init__(self):
        self.messages: Dict[int, DbcMessage] = {}
        self.version: str = ""
        self.nodes: List[str] = []
        self.comments: Dict[str, str] = {}
    
    def parse(self, file_path: str) -> Dict[str, Any]:
        """Parse DBC file and return yuleASR-compatible CAN config"""
        self.messages.clear()
        self.nodes.clear()
        self.comments.clear()
        
        with open(file_path, 'r', encoding='utf-8', errors='replace') as f:
            content = f.read()
        
        lines = content.split('\n')
        
        current_msg = None
        for line in lines:
            line = line.strip()
            if not line or line.startswith('NS_') or line.startswith('BS_'):
                continue
            
            if line.startswith('VERSION'):
                self.version = line.split('"')[1] if '"' in line else ""
                continue
            
            if line.startswith('BU_:'):
                self.nodes = [n.strip() for n in line[3:].split() if n.strip()]
                continue
            
            # BO_ line
            bo_match = self.BO_PATTERN.search(line)
            if bo_match:
                msg_id = int(bo_match.group(1))
                msg_name = bo_match.group(2)
                dlc = int(bo_match.group(3))
                transmitter = bo_match.group(4)
                msg = DbcMessage(msg_id, msg_name, dlc, transmitter)
                self.messages[msg_id] = msg
                current_msg = msg
                continue
            
            # SG_ line
            sg_match = self.SG_PATTERN.search(line)
            if sg_match and current_msg:
                signal = DbcSignal(
                    name=sg_match.group(1),
                    start_bit=int(sg_match.group(2)),
                    length=int(sg_match.group(3)),
                    byte_order='Intel' if sg_match.group(4) == '1' else 'Motorola',
                    value_type=sg_match.group(5),
                    factor=float(sg_match.group(6).split(',')[0].strip()),
                    offset=float(sg_match.group(6).split(',')[1].strip()),
                    min_val=float(sg_match.group(7).split('|')[0].strip()),
                    max_val=float(sg_match.group(7).split('|')[1].strip()),
                    unit=sg_match.group(8),
                    receivers=sg_match.group(9).split()
                )
                current_msg.signals.append(signal)
        
        return self._to_config()
    
    def _to_config(self) -> Dict[str, Any]:
        """Convert parsed DBC to yuleASR CAN config format"""
        # Build CAN Controller config
        config = {
            "version": self.version or "1.0.0",
            "modules": {
                "Can": {
                    "name": "Can",
                    "enabled": True,
                    "version": "1.0.0",
                    "baudrate": 500000,
                    "controller_count": 1,
                    "dbc_version": self.version,
                    "messages_count": len(self.messages),
                    "nodes": self.nodes,
                }
            }
        }
        
        # Build Com (Communication) config with signals and IPDUs
        if self.messages:
            com_config = {
                "name": "Com",
                "enabled": True,
                "version": "1.0.0",
                "pdu_count": len(self.messages),
                "signal_count": sum(len(m.signals) for m in self.messages.values()),
                "dbc_messages": [],
            }
            
            for msg_id, msg in self.messages.items():
                msg_dict = {
                    "can_id": msg_id,
                    "name": msg.name,
                    "dlc": msg.dlc,
                    "transmitter": msg.transmitter,
                    "signals": []
                }
                for sig in msg.signals:
                    msg_dict["signals"].append({
                        "name": sig.name,
                        "start_bit": sig.start_bit,
                        "length": sig.length,
                        "byte_order": sig.byte_order,
                        "value_type": sig.value_type,
                        "factor": sig.factor,
                        "offset": sig.offset,
                        "min": sig.min,
                        "max": sig.max,
                        "unit": sig.unit,
                        "receivers": sig.receivers,
                    })
                com_config["dbc_messages"].append(msg_dict)
            
            config["modules"]["Com"] = com_config
        
        return config


def main():
    import sys
    import json
    
    if len(sys.argv) < 2:
        print("Usage: dbc_importer.py <input.dbc> [output.json]")
        return 1
    
    importer = DbcImporter()
    try:
        result = importer.parse(sys.argv[1])
        
        if len(sys.argv) >= 3:
            with open(sys.argv[2], 'w') as f:
                json.dump(result, f, indent=2)
            msg_count = len(result.get("modules", {}).get("Can", {}).get("dbc_version", ""))
            print(f"[OK] Imported {result['modules']['Can']['messages_count']} message(s) to {sys.argv[2]}")
            if "Com" in result["modules"]:
                print(f"[OK] {result['modules']['Com']['signal_count']} signal(s) mapped to Com module")
        else:
            print(json.dumps(result, indent=2))
        return 0
    except Exception as e:
        print(f"[FAIL] Import error: {e}")
        return 1


if __name__ == "__main__":
    exit(main())
