#!/usr/bin/env python3
"""
DTC Configuration Parser
Parses CSV/Excel configuration files and generates data structures
"""

import csv
import json
import re
from datetime import datetime
from typing import Dict, List, Any, Optional

class DTCConfigParser:
    """Parser for DTC configuration files"""
    
    def __init__(self):
        self.dtcs = []
        self.events = []
        self.freeze_frames = []
        self.extended_data = []
        self.indicators = []
        self.operation_cycles = []
        self.aging_configs = []
        
    def parse_csv(self, filepath: str) -> Dict[str, Any]:
        """Parse CSV configuration file"""
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Remove comments
        lines = [line for line in content.split('\n') if not line.strip().startswith('#')]
        content = '\n'.join(lines)
        
        # Parse sections
        sections = self._split_sections(content)
        
        # Parse DTC Basic Information
        if 'DTC Basic Information' in sections:
            self._parse_dtc_basic(sections['DTC Basic Information'])
        
        # Parse Event Configuration
        if 'Event Configuration' in sections:
            self._parse_events(sections['Event Configuration'])
        
        # Parse Freeze Frame Configuration
        if 'Freeze Frame Configuration' in sections:
            self._parse_freeze_frames(sections['Freeze Frame Configuration'])
        
        # Parse Extended Data Configuration
        if 'Extended Data Configuration' in sections:
            self._parse_extended_data(sections['Extended Data Configuration'])
        
        # Parse Indicator Configuration
        if 'Indicator Configuration' in sections:
            self._parse_indicators(sections['Indicator Configuration'])
        
        # Parse Operation Cycle Configuration
        if 'Operation Cycle Configuration' in sections:
            self._parse_operation_cycles(sections['Operation Cycle Configuration'])
        
        # Parse Aging Configuration
        if 'Aging Configuration' in sections:
            self._parse_aging(sections['Aging Configuration'])
        
        return self._build_config_dict()
    
    def _split_sections(self, content: str) -> Dict[str, str]:
        """Split content into sections"""
        sections = {}
        current_section = None
        current_content = []
        
        for line in content.split('\n'):
            line = line.strip()
            if line.startswith('#') and 'Configuration' in line:
                if current_section:
                    sections[current_section] = '\n'.join(current_content)
                current_section = line.lstrip('#').strip()
                current_content = []
            elif line and current_section:
                current_content.append(line)
        
        if current_section and current_content:
            sections[current_section] = '\n'.join(current_content)
        
        return sections
    
    def _parse_dtc_basic(self, content: str):
        """Parse DTC basic information"""
        reader = csv.DictReader(content.split('\n'))
        for row in reader:
            if not row.get('DTC_CODE'):
                continue
            
            dtc_code = int(row['DTC_CODE'], 16) if row['DTC_CODE'].startswith('0x') else int(row['DTC_CODE'])
            
            self.dtcs.append({
                'code': dtc_code,
                'name': self._generate_name(row.get('DESCRIPTION', '')),
                'description': row.get('DESCRIPTION', ''),
                'severity': row.get('SEVERITY', 'MEDIUM').upper(),
                'functional_unit': int(row.get('FUNCTIONAL_UNIT', '0x00'), 16) if row.get('FUNCTIONAL_UNIT', '').startswith('0x') else int(row.get('FUNCTIONAL_UNIT', 0)),
                'group': row.get('DTC_GROUP', 'ALL_DTCS').upper() + '_DTCS',
                'priority': int(row.get('PRIORITY', 1)),
                'kind': row.get('KIND', 'ALL_DTCS').upper(),
                'immediate_storage': row.get('IMMEDIATE_STORAGE', 'FALSE').upper() == 'TRUE',
                'aging_allowed': False,
                'aging_threshold': 0,
                'aging_cycle': 0
            })
    
    def _parse_events(self, content: str):
        """Parse event configuration"""
        reader = csv.DictReader(content.split('\n'))
        for row in reader:
            if not row.get('EVENT_ID'):
                continue
            
            dtc_code = int(row.get('DTC_CODE', '0'), 16) if row.get('DTC_CODE', '').startswith('0x') else 0
            
            self.events.append({
                'id': int(row.get('EVENT_ID', 0)),
                'name': row.get('EVENT_NAME', f'Event_{row.get("EVENT_ID", 0)}'),
                'dtc_code': dtc_code,
                'debounce_type': row.get('DEBOUNCE_TYPE', 'COUNTER').upper(),
                'debounce_failed_thr': int(row.get('DEBOUNCE_FAILED_THR', 127)),
                'debounce_passed_thr': int(row.get('DEBOUNCE_PASSED_THR', -128)),
                'immediate_storage': row.get('IMMEDIATE_STORAGE', 'FALSE').upper() == 'TRUE',
                'freeze_frame_record': 1,
                'freeze_frame_count': 1,
                'freeze_frame_dids': [],
                'extended_data_record': 1,
                'extended_data_count': 1,
                'extended_data_size': 0,
                'extended_data_rule': 'UPDATE'
            })
    
    def _parse_freeze_frames(self, content: str):
        """Parse freeze frame configuration"""
        reader = csv.DictReader(content.split('\n'))
        for row in reader:
            if not row.get('EVENT_ID'):
                continue
            
            event_id = int(row.get('EVENT_ID', 0))
            did_list = row.get('DID_LIST', '')
            
            # Parse DID list (format: "0x0100;0x0101;0x0105")
            dids = []
            if did_list:
                for did in did_list.split(';'):
                    did = did.strip()
                    if did.startswith('0x'):
                        dids.append(int(did, 16))
            
            # Update event with freeze frame info
            for event in self.events:
                if event['id'] == event_id:
                    event['freeze_frame_dids'] = dids
                    event['freeze_frame_record'] = int(row.get('RECORD_NUMBER', 1))
                    break
    
    def _parse_extended_data(self, content: str):
        """Parse extended data configuration"""
        reader = csv.DictReader(content.split('\n'))
        for row in reader:
            if not row.get('EVENT_ID'):
                continue
            
            event_id = int(row.get('EVENT_ID', 0))
            
            for event in self.events:
                if event['id'] == event_id:
                    event['extended_data_size'] = int(row.get('DATA_SIZE', 0))
                    event['extended_data_record'] = int(row.get('RECORD_NUMBER', 1))
                    event['extended_data_rule'] = row.get('UPDATE_RULE', 'UPDATE').upper()
                    break
    
    def _parse_indicators(self, content: str):
        """Parse indicator configuration"""
        reader = csv.DictReader(content.split('\n'))
        for row in reader:
            if not row.get('EVENT_ID'):
                continue
            
            self.indicators.append({
                'event_id': int(row.get('EVENT_ID', 0)),
                'indicator_id': int(row.get('INDICATOR_ID', 0)),
                'behavior': row.get('BEHAVIOR', 'CONTINUOUS').upper(),
                'failure_cycles': int(row.get('FAILURE_CYCLES', 1)),
                'healing_cycles': int(row.get('HEALING_CYCLES', 1))
            })
    
    def _parse_operation_cycles(self, content: str):
        """Parse operation cycle configuration"""
        reader = csv.DictReader(content.split('\n'))
        for row in reader:
            if not row.get('CYCLE_ID'):
                continue
            
            self.operation_cycles.append({
                'id': int(row.get('CYCLE_ID', 0)),
                'type': row.get('CYCLE_TYPE', 'IGNITION').upper(),
                'auto_start': row.get('AUTO_START', 'TRUE').upper() == 'TRUE'
            })
    
    def _parse_aging(self, content: str):
        """Parse aging configuration"""
        reader = csv.DictReader(content.split('\n'))
        for row in reader:
            if not row.get('DTC_CODE'):
                continue
            
            dtc_code = int(row['DTC_CODE'], 16) if row['DTC_CODE'].startswith('0x') else 0
            
            for dtc in self.dtcs:
                if dtc['code'] == dtc_code:
                    dtc['aging_allowed'] = row.get('AGING_ALLOWED', 'FALSE').upper() == 'TRUE'
                    dtc['aging_threshold'] = int(row.get('AGING_THRESHOLD', 0))
                    dtc['aging_cycle'] = int(row.get('AGING_CYCLE', 0))
                    break
    
    def _generate_name(self, description: str) -> str:
        """Generate C identifier name from description"""
        # Remove special characters and convert to uppercase
        name = re.sub(r'[^\w\s]', '', description)
        name = re.sub(r'\s+', '_', name.strip())
        return name.upper()[:50]  # Limit length
    
    def _build_config_dict(self) -> Dict[str, Any]:
        """Build complete configuration dictionary"""
        return {
            'project_name': 'AUTOSAR_DTC_Config',
            'version': {'major': 1, 'minor': 0, 'patch': 0},
            'generation_date': datetime.now().strftime('%Y-%m-%d'),
            'dtcs': self.dtcs,
            'events': self.events,
            'indicators': self.indicators,
            'operation_cycles': self.operation_cycles
        }
    
    def export_json(self, filepath: str):
        """Export configuration to JSON file"""
        config = self._build_config_dict()
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump(config, f, indent=2)
        return filepath


def validate_config(config: Dict[str, Any]) -> List[str]:
    """Validate configuration and return list of errors"""
    errors = []
    
    # Check DTC codes are unique
    dtc_codes = [dtc['code'] for dtc in config.get('dtcs', [])]
    if len(dtc_codes) != len(set(dtc_codes)):
        errors.append('Duplicate DTC codes found')
    
    # Check event IDs are unique
    event_ids = [event['id'] for event in config.get('events', [])]
    if len(event_ids) != len(set(event_ids)):
        errors.append('Duplicate event IDs found')
    
    # Check DTC codes match between events and DTCs
    valid_dtc_codes = set(dtc_codes)
    for event in config.get('events', []):
        if event['dtc_code'] not in valid_dtc_codes:
            errors.append(f"Event {event['id']} references unknown DTC 0x{event['dtc_code']:06X}")
    
    # Check operation cycle references
    valid_cycles = set(cycle['id'] for cycle in config.get('operation_cycles', []))
    for dtc in config.get('dtcs', []):
        if dtc['aging_allowed'] and dtc['aging_cycle'] not in valid_cycles:
            errors.append(f"DTC 0x{dtc['code']:06X} references unknown aging cycle {dtc['aging_cycle']}")
    
    return errors


if __name__ == '__main__':
    import sys
    
    if len(sys.argv) < 2:
        print('Usage: python config_parser.py <config.csv>')
        sys.exit(1)
    
    parser = DTCConfigParser()
    config = parser.parse_csv(sys.argv[1])
    
    # Validate
    errors = validate_config(config)
    if errors:
        print('Configuration errors:')
        for error in errors:
            print(f'  - {error}')
        sys.exit(1)
    
    # Export to JSON
    output_file = sys.argv[1].replace('.csv', '_parsed.json')
    parser.export_json(output_file)
    print(f'Configuration parsed successfully: {output_file}')
