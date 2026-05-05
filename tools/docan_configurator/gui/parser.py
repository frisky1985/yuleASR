#!/usr/bin/env python3
"""
Generic AUTOSAR Module Configuration Parser
Supports: CanTp, UDS, DoCan, DoIP, DTC configurations
"""

import csv
import json
import re
from datetime import datetime
from typing import Dict, List, Any, Optional

class AutosarConfigParser:
    """Generic parser for AUTOSAR module configurations"""
    
    def __init__(self, module_type: str):
        self.module_type = module_type.upper()
        self.config = {}
        
    def parse_csv(self, filepath: str) -> Dict[str, Any]:
        """Parse CSV configuration file based on module type"""
        parsers = {
            'CANTP': self._parse_cantp,
            'UDS': self._parse_uds,
            'DOCAN': self._parse_docan,
            'DOIP': self._parse_doip
        }
        
        parser = parsers.get(self.module_type)
        if parser:
            return parser(filepath)
        else:
            raise ValueError(f"Unknown module type: {self.module_type}")
    
    def _parse_csv_sections(self, filepath: str) -> Dict[str, List[Dict]]:
        """Parse CSV file into sections"""
        sections = {}
        current_section = None
        headers = []
        
        with open(filepath, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    # Section header
                    if 'Configuration' in line or 'CONFIGURATION' in line:
                        current_section = line.lstrip('#').strip()
                        sections[current_section] = []
                        headers = []
                    continue
                
                if current_section is None:
                    continue
                    
                parts = [p.strip() for p in line.split(',')]
                
                if not headers:
                    headers = parts
                else:
                    row = {}
                    for i, header in enumerate(headers):
                        if i < len(parts):
                            row[header] = parts[i]
                        else:
                            row[header] = ''
                    sections[current_section].append(row)
        
        return sections
    
    def _parse_cantp(self, filepath: str) -> Dict[str, Any]:
        """Parse CanTp configuration"""
        sections = self._parse_csv_sections(filepath)
        
        config = {
            'project_name': 'CanTp_Config',
            'version': {'major': 1, 'minor': 0, 'patch': 0},
            'generation_date': datetime.now().strftime('%Y-%m-%d'),
            'channels': [],
            'tx_nsdu': [],
            'rx_nsdu': [],
            'general': {}
        }
        
        # Parse channels
        for section_name, rows in sections.items():
            if 'Channel' in section_name:
                for row in rows:
                    config['channels'].append({
                        'id': int(row.get('CHANNEL_ID', 0)),
                        'name': row.get('CHANNEL_NAME', f"Channel_{row.get('CHANNEL_ID', 0)}"),
                        'can_type': row.get('CAN_TYPE', 'CAN_2_0'),
                        'tx_pdu_id': int(row.get('TX_PDUID', 0)),
                        'rx_pdu_id': int(row.get('RX_PDUID', 0)),
                        'addressing_mode': row.get('ADDRESSING_MODE', 'STANDARD'),
                        'max_payload': int(row.get('MAX_PAYLOAD', 8))
                    })
            elif 'TxNSdu' in section_name:
                for row in rows:
                    config['tx_nsdu'].append({
                        'id': int(row.get('NSDU_ID', 0)),
                        'name': row.get('NSDU_ID', f"TxNSdu_{row.get('NSDU_ID', 0)}"),
                        'channel_id': int(row.get('CHANNEL_ID', 0)),
                        'pdu_id': int(row.get('PDU_ID', 0)),
                        'address': int(row.get('TRANSPORT_LAYER_ADDR', '0x0'), 16),
                        'ta_type': row.get('TA_TYPE', 'PHYSICAL'),
                        'bs': int(row.get('BS', 8)),
                        'stmin': int(row.get('STMIN', 20)),
                        'wftmax': int(row.get('WFTMAX', 3)),
                        'tx_confirmation': row.get('TX_CONFIRMATION', 'NULL')
                    })
            elif 'RxNSdu' in section_name:
                for row in rows:
                    config['rx_nsdu'].append({
                        'id': int(row.get('NSDU_ID', 0)),
                        'name': row.get('NSDU_ID', f"RxNSdu_{row.get('NSDU_ID', 0)}"),
                        'channel_id': int(row.get('CHANNEL_ID', 0)),
                        'pdu_id': int(row.get('PDU_ID', 0)),
                        'address': int(row.get('TRANSPORT_LAYER_ADDR', '0x0'), 16),
                        'ta_type': row.get('TA_TYPE', 'PHYSICAL'),
                        'bs': int(row.get('BS', 8)),
                        'stmin': int(row.get('STMIN', 20)),
                        'wftmax': int(row.get('WFTMAX', 3)),
                        'rx_indication': row.get('RX_INDICATION', 'NULL')
                    })
            elif 'General' in section_name:
                for row in rows:
                    param = row.get('PARAMETER', '')
                    value = row.get('VALUE', '')
                    if param:
                        if value.upper() in ['TRUE', 'FALSE']:
                            config['general'][param.lower()] = value.upper() == 'TRUE'
                        elif value.isdigit():
                            config['general'][param.lower()] = int(value)
                        else:
                            config['general'][param.lower()] = value
        
        return config
    
    def _parse_uds(self, filepath: str) -> Dict[str, Any]:
        """Parse UDS configuration"""
        sections = self._parse_csv_sections(filepath)
        
        config = {
            'project_name': 'UDS_Config',
            'version': {'major': 1, 'minor': 0, 'patch': 0},
            'generation_date': datetime.now().strftime('%Y-%m-%d'),
            'sessions': [],
            'security_levels': [],
            'services': [],
            'dids': [],
            'rids': []
        }
        
        for section_name, rows in sections.items():
            if 'Session' in section_name:
                for row in rows:
                    config['sessions'].append({
                        'id': int(row.get('SESSION_ID', '0x0'), 16),
                        'name': row.get('SESSION_NAME', 'Default'),
                        'type': row.get('SESSION_TYPE', 'DEFAULT'),
                        'timeout': int(row.get('TIMEOUT_MS', 5000)),
                        'p2_max': int(row.get('P2_STAR_MAX', 50000)),
                        'p2_min': int(row.get('P2_STAR_MIN', 25000))
                    })
            elif 'Security' in section_name:
                for row in rows:
                    config['security_levels'].append({
                        'level': int(row.get('SECURITY_LEVEL', '0x0'), 16),
                        'key_size': int(row.get('KEY_SIZE', 4)),
                        'seed_size': int(row.get('SEED_SIZE', 4)),
                        'timeout': int(row.get('ACCESS_TIMEOUT_MS', 5000)),
                        'delay': int(row.get('DELAY_MS', 0))
                    })
            elif 'Service' in section_name:
                for row in rows:
                    subfuncs_str = row.get('SUBFUNCTIONS', '')
                    subfuncs = []
                    if subfuncs_str:
                        subfuncs = [int(s.strip(), 16) for s in subfuncs_str.split(',') if s.strip()]
                    
                    config['services'].append({
                        'id': int(row.get('SERVICE_ID', '0x0'), 16),
                        'name': row.get('SERVICE_NAME', 'Unknown'),
                        'in_default': row.get('SUPPORTED_IN_DEFAULT', 'FALSE').upper() == 'TRUE',
                        'in_extended': row.get('SUPPORTED_IN_EXTENDED', 'FALSE').upper() == 'TRUE',
                        'in_programming': row.get('SUPPORTED_IN_PROGRAMMING', 'FALSE').upper() == 'TRUE',
                        'subfunctions': subfuncs
                    })
            elif 'DID' in section_name or 'Data Identifier' in section_name:
                for row in rows:
                    config['dids'].append({
                        'id': int(row.get('DID_ID', '0x0'), 16),
                        'name': row.get('DID_NAME', 'Unknown'),
                        'size': int(row.get('SIZE_BYTES', 1)),
                        'read_sec': int(row.get('READ_SECURITY_LEVEL', '0x0'), 16),
                        'write_sec': int(row.get('WRITE_SECURITY_LEVEL', '0x0'), 16),
                        'session_mask': int(row.get('SESSION_MASK', '0x7'), 16)
                    })
            elif 'RID' in section_name or 'Routine' in section_name:
                for row in rows:
                    config['rids'].append({
                        'id': int(row.get('RID_ID', '0x0'), 16),
                        'name': row.get('RID_NAME', 'Unknown'),
                        'start': int(row.get('START_ROUTINE', 0)),
                        'stop': int(row.get('STOP_ROUTINE', 0)),
                        'results': int(row.get('REQUEST_RESULTS', 0)),
                        'security': int(row.get('SECURITY_LEVEL', '0x0'), 16)
                    })
        
        return config
    
    def _parse_docan(self, filepath: str) -> Dict[str, Any]:
        """Parse DoCan configuration"""
        sections = self._parse_csv_sections(filepath)
        
        config = {
            'project_name': 'DoCan_Config',
            'version': {'major': 1, 'minor': 0, 'patch': 0},
            'generation_date': datetime.now().strftime('%Y-%m-%d'),
            'connections': []
        }
        
        connections = {}
        
        for section_name, rows in sections.items():
            for row in rows:
                conn_id = int(row.get('CONN_ID', 0))
                
                if conn_id not in connections:
                    connections[conn_id] = {'id': conn_id}
                
                if 'Connection' in section_name:
                    connections[conn_id].update({
                        'name': row.get('CONN_NAME', f"Conn_{conn_id}"),
                        'req_id': int(row.get('REQUEST_CAN_ID', '0x0'), 16),
                        'resp_id': int(row.get('RESPONSE_CAN_ID', '0x0'), 16),
                        'func_id': int(row.get('FUNC_REQUEST_ID', '0x0'), 16),
                        'addressing': row.get('ADDRESSING_MODE', 'PHYSICAL')
                    })
                elif 'Timing' in section_name:
                    connections[conn_id].update({
                        'p2_max': int(row.get('P2_CAN_MAX', 150)),
                        'p2_min': int(row.get('P2_CAN_MIN', 25)),
                        'p2_star': int(row.get('P2_STAR_MAX', 5000)),
                        's3_server': int(row.get('S3_SERVER', 5000)),
                        's3_client': int(row.get('S3_CLIENT', 2000))
                    })
                elif 'Buffer' in section_name:
                    connections[conn_id].update({
                        'rx_buf': int(row.get('RX_BUFFER_SIZE', 4096)),
                        'tx_buf': int(row.get('TX_BUFFER_SIZE', 4096)),
                        'fc_bs': int(row.get('FC_BS', 8)),
                        'fc_stmin': int(row.get('FC_STMIN', 20))
                    })
        
        config['connections'] = list(connections.values())
        return config
    
    def _parse_doip(self, filepath: str) -> Dict[str, Any]:
        """Parse DoIP configuration"""
        sections = self._parse_csv_sections(filepath)
        
        config = {
            'project_name': 'DoIP_Config',
            'version': {'major': 1, 'minor': 0, 'patch': 0},
            'generation_date': datetime.now().strftime('%Y-%m-%d'),
            'entities': [],
            'activations': []
        }
        
        entities = {}
        
        for section_name, rows in sections.items():
            if 'Entity' in section_name and 'Routing' not in section_name:
                for row in rows:
                    entity_id = int(row.get('ENTITY_ID', 0))
                    
                    if entity_id not in entities:
                        entities[entity_id] = {'id': entity_id}
                    
                    if 'Entity Configuration' in section_name:
                        entities[entity_id].update({
                            'type': row.get('ENTITY_TYPE', 'NODE'),
                            'logical_addr': int(row.get('LOGICAL_ADDRESS', '0x0'), 16),
                            'ip': row.get('IP_ADDRESS', '192.168.1.1'),
                            'port': int(row.get('PORT', 13400))
                        })
                    elif 'Timing' in section_name:
                        entities[entity_id].update({
                            'tcp_initial': int(row.get('TCP_INITIAL', 2000)),
                            'node_discovery': int(row.get('NODE_DISCOVERY', 5000)),
                            'vehicle_discovery': int(row.get('VEHICLE_DISCOVERY', 5000)),
                            'vehicle_announce': int(row.get('VEHICLE_ANNOUNCEMENT', 500)),
                            'initial_inactivity': int(row.get('INITIAL_INACTIVITY', 2000)),
                            'broadcast_repeat': int(row.get('BROADCAST_REPEAT', 1000))
                        })
                    elif 'Powersave' in section_name:
                        entities[entity_id].update({
                            'powersave_timeout': int(row.get('POWERSAVE_TIMEOUT', 300000)),
                            'powersave_wake': row.get('POWERSAVE_WAKE_UP', 'TRUE').upper() == 'TRUE'
                        })
            
            elif 'Routing Activation' in section_name:
                for row in rows:
                    config['activations'].append({
                        'id': int(row.get('ACTIVATION_ID', 0)),
                        'entity_id': int(row.get('ENTITY_ID', 0)),
                        'type': row.get('ACTIVATION_TYPE', 'DEFAULT'),
                        'oem': int(row.get('OEM_SPECIFIC', '0x0'), 16),
                        'auth': row.get('AUTHENTICATION', 'FALSE').upper() == 'TRUE',
                        'confirm': row.get('CONFIRMATION', 'FALSE').upper() == 'TRUE'
                    })
        
        config['entities'] = list(entities.values())
        return config
    
    def export_json(self, filepath: str, config: Dict = None):
        """Export configuration to JSON file"""
        if config is None:
            config = self.config
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump(config, f, indent=2)
        return filepath


if __name__ == '__main__':
    import sys
    
    if len(sys.argv) < 3:
        print('Usage: python autosar_parser.py <module_type> <config.csv>')
        print('  module_type: CANTP, UDS, DOCAN, DOIP')
        sys.exit(1)
    
    parser = AutosarConfigParser(sys.argv[1])
    config = parser.parse_csv(sys.argv[2])
    
    output_file = sys.argv[2].replace('.csv', '_parsed.json')
    parser.export_json(output_file, config)
    print(f'Configuration parsed successfully: {output_file}')
