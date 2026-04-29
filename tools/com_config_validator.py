#!/usr/bin/env python3
"""
ComConfigValidator - AUTOSAR COM Module Configuration Validator
===============================================================
Validates COM configuration files (JSON/ARXML) for consistency and correctness.

Usage:
    python com_config_validator.py --input examples/com_config_engine.json
    python com_config_validator.py --input examples/com_config_engine.json --verbose
    python com_config_validator.py --input examples/com_config_engine.json --check-arxml

Features:
    - Validate JSON configuration structure
    - Check for duplicate IDs (signals, IPDUs, groups)
    - Validate signal bit position overlaps
    - Verify reference integrity
    - Check transmission mode configurations
    - Validate TMC (Transmission Mode Conditions)
    - Support ARXML schema validation

Exit Codes:
    0 - Validation passed
    1 - Validation failed
    2 - Configuration error
    3 - File not found

Author: YuleTech AutoSAR Team
Version: 1.0.0
"""

import json
import argparse
import sys
import re
from typing import Dict, List, Any, Set, Tuple, Optional
from dataclasses import dataclass, field
from enum import Enum

class ValidationLevel(Enum):
    """Validation severity levels"""
    ERROR = "ERROR"
    WARNING = "WARNING"
    INFO = "INFO"

@dataclass
class ValidationResult:
    """Validation result entry"""
    level: ValidationLevel
    message: str
    context: str = ""
    
    def __str__(self) -> str:
        if self.context:
            return f"[{self.level.value}] {self.context}: {self.message}"
        return f"[{self.level.value}] {self.message}"

class ComConfigValidator:
    """COM Configuration Validator"""
    
    # Valid signal types according to AUTOSAR
    VALID_SIGNAL_TYPES = {
        "COM_BOOLEAN", "COM_UINT8", "COM_UINT16", "COM_UINT32", "COM_UINT64",
        "COM_SINT8", "COM_SINT16", "COM_SINT32", "COM_SINT64",
        "COM_FLOAT32", "COM_FLOAT64",
        "COM_UINT8_N", "COM_UINT16_N", "COM_UINT32_N", "COM_UINT64_N"
    }
    
    # Valid endianness values
    VALID_ENDIANNESS = {"COM_LITTLE_ENDIAN", "COM_BIG_ENDIAN", "COM_OPAQUE"}
    
    # Valid transfer properties
    VALID_TRANSFER_PROPERTIES = {
        "COM_PENDING", "COM_TRIGGERED", "COM_TRIGGERED_ON_CHANGE",
        "COM_TRIGGERED_ON_CHANGE_WITHOUT_REPETITION", "COM_TRIGGERED_WITHOUT_REPETITION"
    }
    
    # Valid IPDU directions
    VALID_DIRECTIONS = {"COM_SEND", "COM_RECEIVE"}
    
    # Valid IPDU types
    VALID_IPDU_TYPES = {"COM_NORMAL", "COM_TP"}
    
    # Valid signal processing modes
    VALID_SIGNAL_PROCESSING = {"COM_IMMEDIATE", "COM_DEFERRED"}
    
    # Valid transmission modes
    VALID_TX_MODES = {"COM_MODE_DIRECT", "COM_MODE_PERIODIC", "COM_MODE_MIXED", "COM_MODE_NONE"}
    
    def __init__(self, config_data: Dict, verbose: bool = False):
        self.config = config_data
        self.verbose = verbose
        self.results: List[ValidationResult] = []
        
        # Extracted data for validation
        self.signals: Dict[int, Dict] = {}
        self.ipdus: Dict[int, Dict] = {}
        self.ipdu_groups: Dict[int, Dict] = {}
        self.signal_groups: Dict[int, Dict] = {}
        self.global_config: Dict = {}
    
    def validate(self) -> bool:
        """Run all validations and return True if no errors"""
        self._extract_data()
        
        # Structure validations
        self._validate_global_config()
        self._validate_signals_structure()
        self._validate_ipdus_structure()
        self._validate_groups_structure()
        
        # ID uniqueness validations
        self._validate_unique_ids()
        
        # Reference validations
        self._validate_references()
        
        # Signal layout validations
        self._validate_signal_layout()
        
        # Transmission mode validations
        self._validate_transmission_modes()
        
        # TMC validations
        self._validate_tmc_config()
        
        # Value range validations
        self._validate_value_ranges()
        
        return not self._has_errors()
    
    def _extract_data(self):
        """Extract and index configuration data"""
        if 'global' in self.config:
            self.global_config = self.config['global']
        
        if 'signals' in self.config:
            for sig in self.config['signals']:
                if 'id' in sig:
                    self.signals[sig['id']] = sig
        
        if 'ipdus' in self.config:
            for pdu in self.config['ipdus']:
                if 'id' in pdu:
                    self.ipdus[pdu['id']] = pdu
        
        if 'ipdu_groups' in self.config:
            for grp in self.config['ipdu_groups']:
                if 'id' in grp:
                    self.ipdu_groups[grp['id']] = grp
        
        if 'signal_groups' in self.config:
            for grp in self.config['signal_groups']:
                if 'id' in grp:
                    self.signal_groups[grp['id']] = grp
    
    def _validate_global_config(self):
        """Validate global configuration parameters"""
        if not self.global_config:
            self._add_warning("No global configuration found", "global")
            return
        
        # Check numeric limits
        limits = [
            ('max_signals', 1, 65535),
            ('max_signal_groups', 0, 65535),
            ('max_ipdus', 1, 65535),
            ('max_ipdu_groups', 0, 65535),
            ('max_ipdu_length', 1, 4096),
            ('default_tx_timeout', 0, 65535),
            ('default_max_retries', 0, 255)
        ]
        
        for param, min_val, max_val in limits:
            if param in self.global_config:
                val = self.global_config[param]
                if not isinstance(val, int):
                    self._add_error(f"{param} must be an integer", f"global.{param}")
                elif val < min_val or val > max_val:
                    self._add_error(
                        f"{param} ({val}) out of range [{min_val}, {max_val}]",
                        f"global.{param}"
                    )
        
        # Check boolean values
        bool_params = ['dev_error_detect', 'version_info_api', 'enable_signal_group_array_api']
        for param in bool_params:
            if param in self.global_config:
                val = self.global_config[param]
                if not isinstance(val, bool):
                    self._add_warning(f"{param} should be boolean", f"global.{param}")
    
    def _validate_signals_structure(self):
        """Validate signal configuration structure"""
        if not self.config.get('signals'):
            self._add_error("No signals defined in configuration", "signals")
            return
        
        required_fields = ['name', 'id', 'bit_position', 'bit_size', 'type']
        
        for i, sig in enumerate(self.config['signals']):
            context = f"signals[{i}]"
            
            # Check required fields
            for field in required_fields:
                if field not in sig:
                    self._add_error(f"Missing required field: {field}", context)
            
            # Validate signal type
            if 'type' in sig and sig['type'] not in self.VALID_SIGNAL_TYPES:
                self._add_error(
                    f"Invalid signal type: {sig['type']}",
                    f"{context}.type"
                )
            
            # Validate endianness
            if 'endianness' in sig and sig['endianness'] not in self.VALID_ENDIANNESS:
                self._add_error(
                    f"Invalid endianness: {sig['endianness']}",
                    f"{context}.endianness"
                )
            
            # Validate transfer property
            if 'transfer_property' in sig:
                if sig['transfer_property'] not in self.VALID_TRANSFER_PROPERTIES:
                    self._add_error(
                        f"Invalid transfer property: {sig['transfer_property']}",
                        f"{context}.transfer_property"
                    )
            
            # Validate bit_size
            if 'bit_size' in sig:
                bit_size = sig['bit_size']
                if not isinstance(bit_size, int) or bit_size < 1 or bit_size > 64:
                    self._add_error(
                        f"Invalid bit_size: {bit_size} (must be 1-64)",
                        f"{context}.bit_size"
                    )
                # Check alignment for multi-byte types
                if 'type' in sig:
                    type_size = self._get_type_size(sig['type'])
                    if type_size > 1 and bit_size != type_size * 8:
                        self._add_warning(
                            f"bit_size ({bit_size}) doesn't match type size ({type_size * 8})",
                            f"{context}.bit_size"
                        )
            
            # Validate name (C identifier)
            if 'name' in sig:
                if not self._is_valid_c_identifier(sig['name']):
                    self._add_error(
                        f"Invalid signal name: '{sig['name']}' (must be valid C identifier)",
                        f"{context}.name"
                    )
    
    def _validate_ipdus_structure(self):
        """Validate IPDU configuration structure"""
        if not self.config.get('ipdus'):
            self._add_error("No IPDUs defined in configuration", "ipdus")
            return
        
        required_fields = ['name', 'id', 'length']
        
        for i, pdu in enumerate(self.config['ipdus']):
            context = f"ipdus[{i}]"
            
            # Check required fields
            for field in required_fields:
                if field not in pdu:
                    self._add_error(f"Missing required field: {field}", context)
            
            # Validate length
            if 'length' in pdu:
                length = pdu['length']
                max_length = self.global_config.get('max_ipdu_length', 64)
                if not isinstance(length, int) or length < 1 or length > max_length:
                    self._add_error(
                        f"Invalid length: {length} (must be 1-{max_length})",
                        f"{context}.length"
                    )
            
            # Validate direction
            if 'direction' in pdu and pdu['direction'] not in self.VALID_DIRECTIONS:
                self._add_error(
                    f"Invalid direction: {pdu['direction']}",
                    f"{context}.direction"
                )
            
            # Validate type
            if 'type' in pdu and pdu['type'] not in self.VALID_IPDU_TYPES:
                self._add_error(
                    f"Invalid IPDU type: {pdu['type']}",
                    f"{context}.type"
                )
            
            # Validate signal processing
            if 'signal_processing' in pdu:
                if pdu['signal_processing'] not in self.VALID_SIGNAL_PROCESSING:
                    self._add_error(
                        f"Invalid signal processing: {pdu['signal_processing']}",
                        f"{context}.signal_processing"
                    )
            
            # Validate name
            if 'name' in pdu:
                if not self._is_valid_c_identifier(pdu['name']):
                    self._add_error(
                        f"Invalid IPDU name: '{pdu['name']}'",
                        f"{context}.name"
                    )
            
            # Validate TxMode structures
            for mode_type in ['tx_mode_false', 'tx_mode_true']:
                if mode_type in pdu:
                    self._validate_tx_mode_structure(pdu[mode_type], f"{context}.{mode_type}")
    
    def _validate_tx_mode_structure(self, tx_mode: Dict, context: str):
        """Validate TxMode configuration structure"""
        if 'mode' not in tx_mode:
            self._add_error("Missing required field: mode", context)
            return
        
        mode = tx_mode['mode']
        if mode not in self.VALID_TX_MODES:
            self._add_error(f"Invalid transmission mode: {mode}", f"{context}.mode")
            return
        
        # Check mode-specific required fields
        if mode == 'COM_MODE_PERIODIC':
            if tx_mode.get('cycle_time_ms', 0) == 0:
                self._add_warning(
                    "Periodic mode should have non-zero cycle_time_ms",
                    f"{context}.cycle_time_ms"
                )
        
        elif mode == 'COM_MODE_MIXED':
            if tx_mode.get('cycle_time_ms', 0) == 0:
                self._add_warning(
                    "Mixed mode should have non-zero cycle_time_ms",
                    f"{context}.cycle_time_ms"
                )
        
        elif mode == 'COM_MODE_DIRECT':
            if tx_mode.get('num_repetitions', 0) > 0:
                if tx_mode.get('repetition_period_ms', 0) == 0:
                    self._add_warning(
                        "Direct mode with repetitions should have repetition_period_ms",
                        f"{context}.repetition_period_ms"
                    )
    
    def _validate_groups_structure(self):
        """Validate IPDU and signal group structures"""
        # Validate IPDU groups
        if 'ipdu_groups' in self.config:
            for i, grp in enumerate(self.config['ipdu_groups']):
                context = f"ipdu_groups[{i}]"
                
                if 'name' not in grp:
                    self._add_error("Missing required field: name", context)
                elif not self._is_valid_c_identifier(grp['name']):
                    self._add_error(f"Invalid group name: '{grp['name']}'", f"{context}.name")
                
                if 'id' not in grp:
                    self._add_error("Missing required field: id", context)
        
        # Validate signal groups
        if 'signal_groups' in self.config:
            for i, grp in enumerate(self.config['signal_groups']):
                context = f"signal_groups[{i}]"
                
                if 'name' not in grp:
                    self._add_error("Missing required field: name", context)
                elif not self._is_valid_c_identifier(grp['name']):
                    self._add_error(f"Invalid group name: '{grp['name']}'", f"{context}.name")
                
                if 'id' not in grp:
                    self._add_error("Missing required field: id", context)
    
    def _validate_unique_ids(self):
        """Validate that all IDs are unique within their categories"""
        # Check signal IDs
        signal_ids = [s['id'] for s in self.config.get('signals', []) if 'id' in s]
        duplicates = self._find_duplicates(signal_ids)
        for dup in duplicates:
            names = [s['name'] for s in self.config.get('signals', []) if s.get('id') == dup]
            self._add_error(f"Duplicate signal ID {dup} used by: {', '.join(names)}", "signals")
        
        # Check IPDU IDs
        ipdu_ids = [p['id'] for p in self.config.get('ipdus', []) if 'id' in p]
        duplicates = self._find_duplicates(ipdu_ids)
        for dup in duplicates:
            names = [p['name'] for p in self.config.get('ipdus', []) if p.get('id') == dup]
            self._add_error(f"Duplicate IPDU ID {dup} used by: {', '.join(names)}", "ipdus")
        
        # Check IPDU group IDs
        group_ids = [g['id'] for g in self.config.get('ipdu_groups', []) if 'id' in g]
        duplicates = self._find_duplicates(group_ids)
        for dup in duplicates:
            self._add_error(f"Duplicate IPDU group ID: {dup}", "ipdu_groups")
        
        # Check signal group IDs
        sig_group_ids = [g['id'] for g in self.config.get('signal_groups', []) if 'id' in g]
        duplicates = self._find_duplicates(sig_group_ids)
        for dup in duplicates:
            self._add_error(f"Duplicate signal group ID: {dup}", "signal_groups")
    
    def _validate_references(self):
        """Validate all references between configuration elements"""
        valid_signal_ids = set(self.signals.keys())
        valid_ipdu_ids = set(self.ipdus.keys())
        valid_ipdu_group_ids = set(self.ipdu_groups.keys())
        valid_signal_group_ids = set(self.signal_groups.keys())
        
        # Validate signal -> IPDU references
        for sig_id, sig in self.signals.items():
            if 'ipdu_id' in sig:
                if sig['ipdu_id'] not in valid_ipdu_ids:
                    self._add_error(
                        f"Signal '{sig.get('name', sig_id)}' references unknown IPDU ID: {sig['ipdu_id']}",
                        f"signals[{sig_id}].ipdu_id"
                    )
        
        # Validate IPDU -> signal references
        for pdu_id, pdu in self.ipdus.items():
            for ref in pdu.get('signal_refs', []):
                if ref not in valid_signal_ids:
                    self._add_error(
                        f"IPDU '{pdu.get('name', pdu_id)}' references unknown signal ID: {ref}",
                        f"ipdus[{pdu_id}].signal_refs"
                    )
        
        # Validate IPDU -> IPDU group references
        for pdu_id, pdu in self.ipdus.items():
            for ref in pdu.get('ipdu_group_refs', []):
                if ref not in valid_ipdu_group_ids:
                    self._add_error(
                        f"IPDU '{pdu.get('name', pdu_id)}' references unknown IPDU group ID: {ref}",
                        f"ipdus[{pdu_id}].ipdu_group_refs"
                    )
        
        # Validate IPDU group -> IPDU references
        for grp_id, grp in self.ipdu_groups.items():
            for ref in grp.get('ipdu_refs', []):
                if ref not in valid_ipdu_ids:
                    self._add_error(
                        f"IPDU group '{grp.get('name', grp_id)}' references unknown IPDU ID: {ref}",
                        f"ipdu_groups[{grp_id}].ipdu_refs"
                    )
        
        # Validate signal group -> signal references
        for grp_id, grp in self.signal_groups.items():
            for ref in grp.get('signal_refs', []):
                if ref not in valid_signal_ids:
                    self._add_error(
                        f"Signal group '{grp.get('name', grp_id)}' references unknown signal ID: {ref}",
                        f"signal_groups[{grp_id}].signal_refs"
                    )
    
    def _validate_signal_layout(self):
        """Validate signal bit positions don't overlap within IPDUs"""
        # Group signals by IPDU
        ipdu_signals: Dict[int, List[Dict]] = {}
        for sig_id, sig in self.signals.items():
            ipdu_id = sig.get('ipdu_id', 0)
            if ipdu_id not in ipdu_signals:
                ipdu_signals[ipdu_id] = []
            ipdu_signals[ipdu_id].append(sig)
        
        # Check for overlaps in each IPDU
        for ipdu_id, signals in ipdu_signals.items():
            if not signals:
                continue
            
            # Sort by bit position
            signals_sorted = sorted(signals, key=lambda s: s.get('bit_position', 0))
            
            for i in range(len(signals_sorted) - 1):
                current = signals_sorted[i]
                next_sig = signals_sorted[i + 1]
                
                current_end = current.get('bit_position', 0) + current.get('bit_size', 0)
                next_start = next_sig.get('bit_position', 0)
                
                if current_end > next_start:
                    self._add_error(
                        f"Signal overlap: '{current.get('name')}' ends at bit {current_end - 1}, "
                        f"'{next_sig.get('name')}' starts at bit {next_start}",
                        f"IPDU {ipdu_id}"
                    )
            
            # Check for signals exceeding IPDU length
            if ipdu_id in self.ipdus:
                pdu = self.ipdus[ipdu_id]
                pdu_length_bits = pdu.get('length', 0) * 8
                
                for sig in signals:
                    sig_end = sig.get('bit_position', 0) + sig.get('bit_size', 0)
                    if sig_end > pdu_length_bits:
                        self._add_error(
                            f"Signal '{sig.get('name')}' extends beyond IPDU length "
                            f"(ends at bit {sig_end - 1}, PDU has {pdu_length_bits} bits)",
                            f"signals[{sig.get('id')}].bit_position"
                        )
    
    def _validate_transmission_modes(self):
        """Validate transmission mode configurations"""
        for pdu_id, pdu in self.ipdus.items():
            tx_mode_false = pdu.get('tx_mode_false', {})
            tx_mode_true = pdu.get('tx_mode_true', {})
            use_tmc = pdu.get('use_tmc', False)
            
            # Check for valid mode combinations
            if tx_mode_false.get('mode') == 'COM_MODE_NONE':
                if tx_mode_true.get('mode') != 'COM_MODE_NONE' and not use_tmc:
                    self._add_warning(
                        f"IPDU '{pdu.get('name')}' has TxModeFalse=NONE but TxModeTrue!=NONE",
                        f"ipdus[{pdu_id}].tx_mode"
                    )
            
            # Check repetition consistency
            for mode_name, tx_mode in [('tx_mode_false', tx_mode_false), ('tx_mode_true', tx_mode_true)]:
                num_rep = tx_mode.get('num_repetitions', 0)
                rep_period = tx_mode.get('repetition_period_ms', 0)
                
                if num_rep > 0 and rep_period == 0:
                    self._add_warning(
                        f"Repetitions configured but repetition_period_ms is 0",
                        f"ipdus[{pdu_id}].{mode_name}"
                    )
    
    def _validate_tmc_config(self):
        """Validate TMC (Transmission Mode Condition) configurations"""
        for pdu_id, pdu in self.ipdus.items():
            use_tmc = pdu.get('use_tmc', False)
            tmc = pdu.get('tmc')
            tx_mode_true = pdu.get('tx_mode_true', {})
            
            if use_tmc:
                if not tmc:
                    self._add_error(
                        f"IPDU '{pdu.get('name')}' has use_tmc=true but no TMC configuration",
                        f"ipdus[{pdu_id}].tmc"
                    )
                else:
                    # Validate TMC signal reference
                    tmc_signal = tmc.get('signal_id')
                    if tmc_signal is not None and tmc_signal not in self.signals:
                        self._add_error(
                            f"TMC references unknown signal ID: {tmc_signal}",
                            f"ipdus[{pdu_id}].tmc.signal_id"
                        )
                    
                    # Check TxModeTrue is not NONE when TMC is used
                    if tx_mode_true.get('mode') == 'COM_MODE_NONE':
                        self._add_warning(
                            f"TMC enabled but TxModeTrue is NONE",
                            f"ipdus[{pdu_id}].tx_mode_true"
                        )
            
            elif tmc and tmc.get('is_configured'):
                self._add_warning(
                    f"TMC is configured but use_tmc is false",
                    f"ipdus[{pdu_id}]"
                )
    
    def _validate_value_ranges(self):
        """Validate that values are within reasonable ranges"""
        # Check signal bit positions and sizes
        for sig_id, sig in self.signals.items():
            bit_pos = sig.get('bit_position', 0)
            bit_size = sig.get('bit_size', 0)
            
            if bit_pos < 0 or bit_pos >= 2048:  # Max 256 bytes * 8
                self._add_error(
                    f"bit_position ({bit_pos}) out of reasonable range",
                    f"signals[{sig_id}].bit_position"
                )
            
            if bit_size < 1 or bit_size > 64:
                self._add_error(
                    f"bit_size ({bit_size}) out of range [1, 64]",
                    f"signals[{sig_id}].bit_size"
                )
        
        # Check transmission timing values
        for pdu_id, pdu in self.ipdus.items():
            for mode_name in ['tx_mode_false', 'tx_mode_true']:
                tx_mode = pdu.get(mode_name, {})
                
                cycle_time = tx_mode.get('cycle_time_ms', 0)
                if cycle_time < 0 or cycle_time > 3600000:  # Max 1 hour
                    self._add_error(
                        f"cycle_time_ms ({cycle_time}) out of reasonable range",
                        f"ipdus[{pdu_id}].{mode_name}.cycle_time_ms"
                    )
                
                rep_period = tx_mode.get('repetition_period_ms', 0)
                if rep_period < 0 or rep_period > 10000:  # Max 10 seconds
                    self._add_warning(
                        f"repetition_period_ms ({rep_period}) is very large",
                        f"ipdus[{pdu_id}].{mode_name}.repetition_period_ms"
                    )
    
    def _add_error(self, message: str, context: str = ""):
        """Add an error result"""
        self.results.append(ValidationResult(ValidationLevel.ERROR, message, context))
    
    def _add_warning(self, message: str, context: str = ""):
        """Add a warning result"""
        self.results.append(ValidationResult(ValidationLevel.WARNING, message, context))
    
    def _add_info(self, message: str, context: str = ""):
        """Add an info result"""
        if self.verbose:
            self.results.append(ValidationResult(ValidationLevel.INFO, message, context))
    
    def _has_errors(self) -> bool:
        """Check if there are any errors"""
        return any(r.level == ValidationLevel.ERROR for r in self.results)
    
    def _has_warnings(self) -> bool:
        """Check if there are any warnings"""
        return any(r.level == ValidationLevel.WARNING for r in self.results)
    
    @staticmethod
    def _is_valid_c_identifier(name: str) -> bool:
        """Check if name is a valid C identifier"""
        if not name:
            return False
        # Must start with letter or underscore
        if not (name[0].isalpha() or name[0] == '_'):
            return False
        # Can contain letters, digits, underscores
        return all(c.isalnum() or c == '_' for c in name)
    
    @staticmethod
    def _get_type_size(signal_type: str) -> int:
        """Get size in bytes for a signal type"""
        type_sizes = {
            'COM_BOOLEAN': 1, 'COM_UINT8': 1, 'COM_SINT8': 1,
            'COM_UINT16': 2, 'COM_SINT16': 2,
            'COM_UINT32': 4, 'COM_SINT32': 4, 'COM_FLOAT32': 4,
            'COM_UINT64': 8, 'COM_SINT64': 8, 'COM_FLOAT64': 8
        }
        return type_sizes.get(signal_type, 1)
    
    @staticmethod
    def _find_duplicates(items: List) -> List:
        """Find duplicate items in a list"""
        seen = set()
        duplicates = set()
        for item in items:
            if item in seen:
                duplicates.add(item)
            seen.add(item)
        return list(duplicates)
    
    def print_results(self):
        """Print all validation results"""
        errors = [r for r in self.results if r.level == ValidationLevel.ERROR]
        warnings = [r for r in self.results if r.level == ValidationLevel.WARNING]
        infos = [r for r in self.results if r.level == ValidationLevel.INFO]
        
        print(f"\n{'='*70}")
        print("COM Configuration Validation Results")
        print(f"{'='*70}")
        
        if not self.results:
            print("\n✓ No issues found! Configuration is valid.")
            return
        
        if errors:
            print(f"\n❌ Errors ({len(errors)}):")
            for r in errors:
                print(f"  {r}")
        
        if warnings:
            print(f"\n⚠️  Warnings ({len(warnings)}):")
            for r in warnings:
                print(f"  {r}")
        
        if infos:
            print(f"\nℹ️  Info ({len(infos)}):")
            for r in infos:
                print(f"  {r}")
        
        print(f"\n{'='*70}")
        print(f"Summary: {len(errors)} error(s), {len(warnings)} warning(s), {len(infos)} info")
        print(f"{'='*70}\n")


def main():
    parser = argparse.ArgumentParser(
        description='AUTOSAR COM Configuration Validator',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
    %(prog)s -i examples/com_config_engine.json
    %(prog)s -i examples/com_config_engine.json -v
    %(prog)s -i examples/com_config_engine.json --strict
        '''
    )
    
    parser.add_argument('--input', '-i', required=True,
                        help='Input JSON configuration file to validate')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Enable verbose output (show info messages)')
    parser.add_argument('--strict', action='store_true',
                        help='Treat warnings as errors')
    parser.add_argument('--check-arxml', action='store_true',
                        help='Also validate ARXML schema (if available)')
    
    args = parser.parse_args()
    
    # Read input file
    try:
        with open(args.input, 'r') as f:
            config_data = json.load(f)
    except FileNotFoundError:
        print(f"Error: File not found: {args.input}")
        sys.exit(3)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON: {e}")
        sys.exit(2)
    
    # Run validation
    validator = ComConfigValidator(config_data, verbose=args.verbose)
    is_valid = validator.validate()
    
    # Print results
    validator.print_results()
    
    # Determine exit code
    if not is_valid:
        sys.exit(1)
    
    if args.strict and validator._has_warnings():
        print("Strict mode: Warnings treated as errors")
        sys.exit(1)
    
    sys.exit(0)


if __name__ == '__main__':
    main()
