#!/usr/bin/env python3
"""
ComConfigGenerator - AUTOSAR COM Module Configuration Generator
===============================================================
Generates C configuration code from JSON/ARXML for COM module.

Usage:
    python com_config_generator.py --input examples/com_config_engine.json --output config/com/
    python com_config_generator.py --input examples/com_config_engine.json --output config/com/ --header-output include/autosar/classic/com/

Features:
    - Parse JSON/ARXML configuration files
    - Generate Com_Cfg.h and Com_Lcfg.c
    - Validate configuration consistency
    - Support engine data, vehicle dynamics, body control signals
    - Generate transmission mode configurations (ComTxModeTrue/ComTxModeFalse)
    - Support TMC (Transmission Mode Conditions)

Author: YuleTech AutoSAR Team
Version: 1.0.0
"""

import json
import argparse
import os
import sys
from datetime import datetime
from typing import Dict, List, Any, Optional
from dataclasses import dataclass, field, asdict
from enum import Enum

class SignalType(Enum):
    """AUTOSAR COM Signal Types"""
    BOOLEAN = "COM_BOOLEAN"
    UINT8 = "COM_UINT8"
    UINT16 = "COM_UINT16"
    UINT32 = "COM_UINT32"
    SINT8 = "COM_SINT8"
    SINT16 = "COM_SINT16"
    SINT32 = "COM_SINT32"
    FLOAT32 = "COM_FLOAT32"
    FLOAT64 = "COM_FLOAT64"
    UINT8_N = "COM_UINT8_N"

class Endianness(Enum):
    """Signal Endianness"""
    LITTLE = "COM_LITTLE_ENDIAN"
    BIG = "COM_BIG_ENDIAN"
    OPAQUE = "COM_OPAQUE"

class TransferProperty(Enum):
    """Transfer Property Types"""
    PENDING = "COM_PENDING"
    TRIGGERED = "COM_TRIGGERED"
    TRIGGERED_ON_CHANGE = "COM_TRIGGERED_ON_CHANGE"

class Direction(Enum):
    """IPdu Direction"""
    SEND = "COM_SEND"
    RECEIVE = "COM_RECEIVE"

class TxMode(Enum):
    """Transmission Modes"""
    DIRECT = "COM_MODE_DIRECT"
    PERIODIC = "COM_MODE_PERIODIC"
    MIXED = "COM_MODE_MIXED"
    NONE = "COM_MODE_NONE"

@dataclass
class SignalConfig:
    """Signal Configuration"""
    name: str
    signal_id: int
    bit_position: int
    bit_size: int
    signal_type: str
    endianness: str = "COM_LITTLE_ENDIAN"
    transfer_property: str = "COM_TRIGGERED_ON_CHANGE"
    init_value: Any = 0
    ipdu_id: int = 0
    notification: Optional[str] = None
    timeout: int = 0

@dataclass
class TxModeConfig:
    """Transmission Mode Configuration"""
    mode: str
    cycle_time_ms: int = 0
    repetition_period_ms: int = 0
    num_repetitions: int = 0
    time_offset_ms: int = 0
    repeating_enabled: bool = False

@dataclass
class TmcConfig:
    """Transmission Mode Condition Configuration"""
    signal_id: int
    threshold_value: int
    use_greater_than: bool = True
    is_configured: bool = False

@dataclass
class TxConfirmationConfig:
    """Transmission Confirmation Configuration"""
    enable_confirmation: bool = False
    tx_timeout_ms: int = 100
    max_retries: int = 3
    tx_confirmation_cb: Optional[str] = None
    tx_error_cb: Optional[str] = None
    tx_timeout_cb: Optional[str] = None

@dataclass
class IPduConfig:
    """I-PDU Configuration"""
    name: str
    ipdu_id: int
    length: int
    direction: str = "COM_SEND"
    ipdu_type: str = "COM_NORMAL"
    signal_processing: str = "COM_IMMEDIATE"
    signal_refs: List[int] = field(default_factory=list)
    signal_group_refs: List[int] = field(default_factory=list)
    ipdu_group_refs: List[int] = field(default_factory=list)
    tx_mode_false: TxModeConfig = field(default_factory=lambda: TxModeConfig(mode="COM_MODE_NONE"))
    tx_mode_true: TxModeConfig = field(default_factory=lambda: TxModeConfig(mode="COM_MODE_NONE"))
    use_tmc: bool = False
    tmc_config: Optional[TmcConfig] = None
    tx_confirmation: TxConfirmationConfig = field(default_factory=TxConfirmationConfig)
    timeout: int = 0
    callout: Optional[str] = None

@dataclass
class IPduGroupConfig:
    """IPdu Group Configuration"""
    name: str
    group_id: int
    ipdu_refs: List[int] = field(default_factory=list)

@dataclass
class SignalGroupConfig:
    """Signal Group Configuration"""
    name: str
    group_id: int
    signal_refs: List[int] = field(default_factory=list)

@dataclass
class ComGlobalConfig:
    """Global COM Configuration"""
    dev_error_detect: bool = True
    version_info_api: bool = True
    enable_signal_group_array_api: bool = True
    max_signals: int = 128
    max_signal_groups: int = 32
    max_ipdus: int = 64
    max_ipdu_groups: int = 16
    max_ipdu_length: int = 64
    default_tx_timeout: int = 100
    default_max_retries: int = 3

class ComConfigGenerator:
    """COM Configuration Generator"""
    
    def __init__(self, config_data: Dict):
        self.config = config_data
        self.signals: List[SignalConfig] = []
        self.ipdus: List[IPduConfig] = []
        self.ipdu_groups: List[IPduGroupConfig] = []
        self.signal_groups: List[SignalGroupConfig] = []
        self.global_config: ComGlobalConfig = ComGlobalConfig()
        
    def parse_config(self):
        """Parse JSON configuration"""
        # Parse global configuration
        if 'global' in self.config:
            g = self.config['global']
            self.global_config = ComGlobalConfig(
                dev_error_detect=g.get('dev_error_detect', True),
                version_info_api=g.get('version_info_api', True),
                enable_signal_group_array_api=g.get('enable_signal_group_array_api', True),
                max_signals=g.get('max_signals', 128),
                max_signal_groups=g.get('max_signal_groups', 32),
                max_ipdus=g.get('max_ipdus', 64),
                max_ipdu_groups=g.get('max_ipdu_groups', 16),
                max_ipdu_length=g.get('max_ipdu_length', 64),
                default_tx_timeout=g.get('default_tx_timeout', 100),
                default_max_retries=g.get('default_max_retries', 3)
            )
        
        # Parse signals
        if 'signals' in self.config:
            for sig_data in self.config['signals']:
                self.signals.append(SignalConfig(
                    name=sig_data['name'],
                    signal_id=sig_data['id'],
                    bit_position=sig_data['bit_position'],
                    bit_size=sig_data['bit_size'],
                    signal_type=sig_data['type'],
                    endianness=sig_data.get('endianness', 'COM_LITTLE_ENDIAN'),
                    transfer_property=sig_data.get('transfer_property', 'COM_TRIGGERED_ON_CHANGE'),
                    init_value=sig_data.get('init_value', 0),
                    ipdu_id=sig_data.get('ipdu_id', 0),
                    notification=sig_data.get('notification'),
                    timeout=sig_data.get('timeout', 0)
                ))
        
        # Parse signal groups
        if 'signal_groups' in self.config:
            for grp_data in self.config['signal_groups']:
                self.signal_groups.append(SignalGroupConfig(
                    name=grp_data['name'],
                    group_id=grp_data['id'],
                    signal_refs=grp_data.get('signal_refs', [])
                ))
        
        # Parse IPdu groups
        if 'ipdu_groups' in self.config:
            for grp_data in self.config['ipdu_groups']:
                self.ipdu_groups.append(IPduGroupConfig(
                    name=grp_data['name'],
                    group_id=grp_data['id'],
                    ipdu_refs=grp_data.get('ipdu_refs', [])
                ))
        
        # Parse IPdus
        if 'ipdus' in self.config:
            for pdu_data in self.config['ipdus']:
                # Parse TxMode configurations
                tx_mode_false_data = pdu_data.get('tx_mode_false', {'mode': 'COM_MODE_NONE'})
                tx_mode_true_data = pdu_data.get('tx_mode_true', {'mode': 'COM_MODE_NONE'})
                
                tx_mode_false = TxModeConfig(
                    mode=tx_mode_false_data['mode'],
                    cycle_time_ms=tx_mode_false_data.get('cycle_time_ms', 0),
                    repetition_period_ms=tx_mode_false_data.get('repetition_period_ms', 0),
                    num_repetitions=tx_mode_false_data.get('num_repetitions', 0),
                    time_offset_ms=tx_mode_false_data.get('time_offset_ms', 0),
                    repeating_enabled=tx_mode_false_data.get('repeating_enabled', False)
                )
                
                tx_mode_true = TxModeConfig(
                    mode=tx_mode_true_data['mode'],
                    cycle_time_ms=tx_mode_true_data.get('cycle_time_ms', 0),
                    repetition_period_ms=tx_mode_true_data.get('repetition_period_ms', 0),
                    num_repetitions=tx_mode_true_data.get('num_repetitions', 0),
                    time_offset_ms=tx_mode_true_data.get('time_offset_ms', 0),
                    repeating_enabled=tx_mode_true_data.get('repeating_enabled', False)
                )
                
                # Parse TMC configuration
                tmc_config = None
                if 'tmc' in pdu_data:
                    tmc_data = pdu_data['tmc']
                    tmc_config = TmcConfig(
                        signal_id=tmc_data['signal_id'],
                        threshold_value=tmc_data.get('threshold_value', 0),
                        use_greater_than=tmc_data.get('use_greater_than', True),
                        is_configured=tmc_data.get('is_configured', False)
                    )
                
                # Parse Tx confirmation configuration
                tx_conf_data = pdu_data.get('tx_confirmation', {})
                tx_confirmation = TxConfirmationConfig(
                    enable_confirmation=tx_conf_data.get('enable_confirmation', False),
                    tx_timeout_ms=tx_conf_data.get('tx_timeout_ms', 100),
                    max_retries=tx_conf_data.get('max_retries', 3),
                    tx_confirmation_cb=tx_conf_data.get('tx_confirmation_cb'),
                    tx_error_cb=tx_conf_data.get('tx_error_cb'),
                    tx_timeout_cb=tx_conf_data.get('tx_timeout_cb')
                )
                
                self.ipdus.append(IPduConfig(
                    name=pdu_data['name'],
                    ipdu_id=pdu_data['id'],
                    length=pdu_data['length'],
                    direction=pdu_data.get('direction', 'COM_SEND'),
                    ipdu_type=pdu_data.get('type', 'COM_NORMAL'),
                    signal_processing=pdu_data.get('signal_processing', 'COM_IMMEDIATE'),
                    signal_refs=pdu_data.get('signal_refs', []),
                    signal_group_refs=pdu_data.get('signal_group_refs', []),
                    ipdu_group_refs=pdu_data.get('ipdu_group_refs', []),
                    tx_mode_false=tx_mode_false,
                    tx_mode_true=tx_mode_true,
                    use_tmc=pdu_data.get('use_tmc', False),
                    tmc_config=tmc_config,
                    tx_confirmation=tx_confirmation,
                    timeout=pdu_data.get('timeout', 0),
                    callout=pdu_data.get('callout')
                ))
    
    def generate_cfg_header(self) -> str:
        """Generate Com_Cfg.h content"""
        header = f'''/*
 * Com_Cfg.h
 * AUTOSAR COM Module - Configuration Header
 * Auto-generated by ComConfigGenerator on {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
 * DO NOT EDIT MANUALLY
 */

#ifndef COM_CFG_H
#define COM_CFG_H

/*==================[Pre-compile Configuration]============================*/

/* Development Error Detection */
#define COM_DEV_ERROR_DETECT                {"STD_ON" if self.global_config.dev_error_detect else "STD_OFF"}

/* Version Info API */
#define COM_VERSION_INFO_API                {"STD_ON" if self.global_config.version_info_api else "STD_OFF"}

/* Enable Signal Group Array API */
#define COM_ENABLE_SIGNAL_GROUP_ARRAY_API   {"STD_ON" if self.global_config.enable_signal_group_array_api else "STD_OFF"}

/* Enable MD for TMC Always/None */
#define COM_ENABLE_MDT_FOR_CYCLIC_TRANSMISSION STD_OFF

/* Optimization for supported platforms */
#define COM_OPTIMIZE_FOR_SIZE               STD_OFF
#define COM_OPTIMIZE_FOR_SPEED              STD_ON

/*==================[Configuration Constants]===============================*/

/* Maximum number of elements */
#define COM_MAX_SIGNALS                     {self.global_config.max_signals}u
#define COM_MAX_SIGNAL_GROUPS               {self.global_config.max_signal_groups}u
#define COM_MAX_IPDUS                       {self.global_config.max_ipdus}u
#define COM_MAX_IPDU_GROUPS                 {self.global_config.max_ipdu_groups}u

/* Maximum buffer sizes */
#define COM_MAX_IPDU_LENGTH                 {self.global_config.max_ipdu_length}u
#define COM_MAX_SHADOW_BUFFER_SIZE          256u
#define COM_MAX_RETRY_QUEUE_SIZE            16u  /*!< Maximum retry queue entries */

/* Transmission Confirmation Defaults */
#define COM_DEFAULT_TX_TIMEOUT              {self.global_config.default_tx_timeout}u /*!< Default TX timeout in ms */
#define COM_DEFAULT_MAX_RETRIES             {self.global_config.default_max_retries}u   /*!< Default max retry count */
#define COM_RETRY_DELAY_MS                  10u  /*!< Delay between retries in ms */

/*==================[Symbolic Names]========================================*/

/* IPdu Group IDs */
'''
        
        for grp in self.ipdu_groups:
            header += f'#define ComConf_ComIPduGroup_{grp.name}    {grp.group_id}u\n'
        
        header += '\n/* IPdu IDs */\n'
        for pdu in self.ipdus:
            header += f'#define ComConf_ComIPdu_{pdu.name}          {pdu.ipdu_id}u\n'
        
        header += '\n/* Signal IDs */\n'
        for sig in self.signals:
            header += f'#define ComConf_ComSignal_{sig.name}       {sig.signal_id}u\n'
        
        header += '\n/* Signal Group IDs */\n'
        for grp in self.signal_groups:
            header += f'#define ComConf_ComSignalGroup_{grp.name}   {grp.group_id}u\n'
        
        header += '''
/*==================[External Declarations]=================================*/

/* Configuration structure declared in Com_Lcfg.c */
extern const Com_ConfigType ComConfig;

#endif /* COM_CFG_H */
'''
        return header
    
    def generate_lcfg_source(self) -> str:
        """Generate Com_Lcfg.c content"""
        source = f'''/*
 * Com_Lcfg.c
 * COM Module Link-Time Configuration
 * Auto-generated by ComConfigGenerator on {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
 * DO NOT EDIT MANUALLY
 */

#include "Com.h"

/*==================[Buffer Declarations]===================================*/

'''
        
        # Generate buffer declarations for each IPDU
        for pdu in self.ipdus:
            source += f'static uint8 ComIPdu_{pdu.name}_Buffer[{pdu.length}];\n'
        
        # Generate shadow buffers for signal groups
        source += '\n/* Shadow Buffers for Signal Groups */\n'
        for grp in self.signal_groups:
            size = sum(self._get_signal_size(s) for s in grp.signal_refs)
            source += f'static uint8 ComShadowBuffer_{grp.name}[{max(size, 1)}];\n'
        
        # Generate IPdu Group configurations
        source += '''
/*==================[IPDU Group Configuration]================================*/

'''
        for grp in self.ipdu_groups:
            if grp.ipdu_refs:
                ref_str = ', '.join(f'ComConf_ComIPdu_{self._get_ipdu_name(r)}' for r in grp.ipdu_refs)
                source += f'static const Com_IPduIdType ComIPduGroup_{grp.name}_Refs[] = {{{ref_str}}};\n'
        
        source += '\n'
        if self.ipdu_groups:
            source += 'static const Com_IPduGroupConfigType ComIPduGroups[] = {\n'
            for grp in self.ipdu_groups:
                if grp.ipdu_refs:
                    source += f'''    {{
        .IpduGroupId = ComConf_ComIPduGroup_{grp.name},
        .IPduRefs = ComIPduGroup_{grp.name}_Refs,
        .NumIPdus = {len(grp.ipdu_refs)}
    }},
'''
                else:
                    source += f'''    {{
        .IpduGroupId = ComConf_ComIPduGroup_{grp.name},
        .IPduRefs = NULL_PTR,
        .NumIPdus = 0
    }},
'''
            source += '};\n'
        
        # Generate signal reference arrays for each IPDU
        source += '''
/*==================[Signal References]=====================================*/

'''
        for pdu in self.ipdus:
            if pdu.signal_refs:
                ref_str = ', '.join(f'ComConf_ComSignal_{self._get_signal_name(s)}' for s in pdu.signal_refs)
                source += f'static const Com_SignalIdType Com{pdu.name}_Signals[] = {{{ref_str}}};\n'
        
        # Generate signal group reference arrays
        source += '\n/*==================[Signal Group References]===============================*/\n\n'
        for grp in self.signal_groups:
            if grp.signal_refs:
                ref_str = ', '.join(f'ComConf_ComSignal_{self._get_signal_name(s)}' for s in grp.signal_refs)
                source += f'static const Com_SignalIdType Com{grp.name}_Signals[] = {{{ref_str}}};\n'
        
        # Generate IPDU configurations
        source += '''
/*==================[IPDU Configuration]====================================*/

static const Com_IPduConfigType ComIPdus[] = {
'''
        
        for pdu in self.ipdus:
            source += self._generate_ipdu_config(pdu)
        
        source += '''};

/*==================[Signal Configuration]=================================*/

'''
        
        # Generate initial values
        source += '/* Initial Values */\n'
        for sig in self.signals:
            c_type = self._signal_type_to_c_type(sig.signal_type)
            source += f'static const {c_type} ComInitValue_{sig.name} = {self._format_init_value(sig.init_value, sig.signal_type)};\n'
        
        # Generate signal configurations
        source += '''
/* Signal Configurations */
static const Com_SignalConfigType ComSignals[] = {
'''
        
        for sig in self.signals:
            pdu_name = self._get_ipdu_name(sig.ipdu_id)
            byte_offset = sig.bit_position // 8
            source += f'''    {{
        .SignalId = ComConf_ComSignal_{sig.name},
        .DataPtr = &ComIPdu_{pdu_name}_Buffer[{byte_offset}],
        .BitPosition = {sig.bit_position},
        .BitSize = {sig.bit_size},
        .Endianness = {sig.endianness},
        .SignalType = {sig.signal_type},
        .TransferProperty = {sig.transfer_property},
        .ComNotification = {sig.notification if sig.notification else 'NULL_PTR'},
        .Timeout = {sig.timeout},
        .InitValue = &ComInitValue_{sig.name}
    }},
'''
        
        source += '''};

/*==================[Signal Group Configuration]============================*/

'''
        
        if self.signal_groups:
            source += 'static const Com_SignalGroupConfigType ComSignalGroups[] = {\n'
            for grp in self.signal_groups:
                source += f'''    {{
        .SignalGroupId = ComConf_ComSignalGroup_{grp.name},
        .SignalRefs = Com{grp.name}_Signals,
        .NumSignals = {len(grp.signal_refs)},
        .ShadowBuffer = ComShadowBuffer_{grp.name},
        .ComNotification = NULL_PTR
    }},
'''
            source += '};\n'
        
        # Generate global configuration
        source += '''
/*==================[Global Configuration]==================================*/

const Com_ConfigType ComConfig = {
    .Signals = ComSignals,
    .NumSignals = ''' + str(len(self.signals)) + ''',
    .SignalGroups = ''' + ('ComSignalGroups' if self.signal_groups else 'NULL_PTR') + ''',
    .NumSignalGroups = ''' + str(len(self.signal_groups)) + ''',
    .IPdus = ComIPdus,
    .NumIPdus = ''' + str(len(self.ipdus)) + ''',
    .IPduGroups = ''' + ('ComIPduGroups' if self.ipdu_groups else 'NULL_PTR') + ''',
    .NumIPduGroups = ''' + str(len(self.ipdu_groups)) + '''
};

/*==================[End of File]==========================================*/
'''
        return source
    
    def _generate_ipdu_config(self, pdu: IPduConfig) -> str:
        """Generate a single IPDU configuration"""
        config = f'''    /* {pdu.name} IPDU - {pdu.tx_mode_false.mode} Mode */
    {{
        .IPduId = ComConf_ComIPdu_{pdu.name},
        .DataPtr = ComIPdu_{pdu.name}_Buffer,
        .Length = {pdu.length},
        .Direction = {pdu.direction},
        .Type = {pdu.ipdu_type},
        .SignalProcessing = {pdu.signal_processing},
        .SignalRefs = {f'Com{pdu.name}_Signals' if pdu.signal_refs else 'NULL_PTR'},
        .NumSignals = {len(pdu.signal_refs)},
        .SignalGroupRefs = NULL_PTR,
        .NumSignalGroups = {len(pdu.signal_group_refs)},
        .TxMode = {{
            .TxModeFalse = {{
                .Mode = {pdu.tx_mode_false.mode},
                .CycleTime = {pdu.tx_mode_false.cycle_time_ms},
                .RepetitionPeriod = {pdu.tx_mode_false.repetition_period_ms},
                .NumRepetitions = {pdu.tx_mode_false.num_repetitions},
                .TimeOffset = {pdu.tx_mode_false.time_offset_ms},
                .RepeatingEnabled = {str(pdu.tx_mode_false.repeating_enabled).upper()}
            }},
            .TxModeTrue = {{
                .Mode = {pdu.tx_mode_true.mode},
                .CycleTime = {pdu.tx_mode_true.cycle_time_ms},
                .RepetitionPeriod = {pdu.tx_mode_true.repetition_period_ms},
                .NumRepetitions = {pdu.tx_mode_true.num_repetitions},
                .TimeOffset = {pdu.tx_mode_true.time_offset_ms},
                .RepeatingEnabled = {str(pdu.tx_mode_true.repeating_enabled).upper()}
            }},
'''
        if pdu.tmc_config:
            config += f'''            .TmcConfig = {{
                .SignalId = {pdu.tmc_config.signal_id},
                .ThresholdValue = {pdu.tmc_config.threshold_value},
                .UseGreaterThan = {str(pdu.tmc_config.use_greater_than).upper()},
                .IsConfigured = {str(pdu.tmc_config.is_configured).upper()}
            }},
'''
        else:
            config += '''            .TmcConfig = {
                .SignalId = 0,
                .ThresholdValue = 0,
                .UseGreaterThan = TRUE,
                .IsConfigured = FALSE
            },
'''
        
        config += f'''            .UseTmc = {str(pdu.use_tmc).upper()}
        }},
        .IpduGroupRefs = NULL_PTR,
        .NumIpduGroups = {len(pdu.ipdu_group_refs)},
        .Timeout = {pdu.timeout},
        .ComIPduCallout = {pdu.callout if pdu.callout else 'NULL_PTR'},
        .TxConfirmation = {{
            .EnableConfirmation = {str(pdu.tx_confirmation.enable_confirmation).upper()},
            .TxTimeout = {pdu.tx_confirmation.tx_timeout_ms},
            .MaxRetries = {pdu.tx_confirmation.max_retries},
            .ComTxConfirmation = {pdu.tx_confirmation.tx_confirmation_cb if pdu.tx_confirmation.tx_confirmation_cb else 'NULL_PTR'},
            .ComTxErrorNotification = {pdu.tx_confirmation.tx_error_cb if pdu.tx_confirmation.tx_error_cb else 'NULL_PTR'},
            .ComTxTimeoutNotification = {pdu.tx_confirmation.tx_timeout_cb if pdu.tx_confirmation.tx_timeout_cb else 'NULL_PTR'}
        }}
    }},
'''
        return config
    
    def _get_signal_name(self, signal_id: int) -> str:
        """Get signal name by ID"""
        for sig in self.signals:
            if sig.signal_id == signal_id:
                return sig.name
        return f"UnknownSignal{signal_id}"
    
    def _get_ipdu_name(self, ipdu_id: int) -> str:
        """Get IPDU name by ID"""
        for pdu in self.ipdus:
            if pdu.ipdu_id == ipdu_id:
                return pdu.name
        return f"UnknownIPdu{ipdu_id}"
    
    def _get_signal_size(self, signal_id: int) -> int:
        """Get signal size in bytes by ID"""
        for sig in self.signals:
            if sig.signal_id == signal_id:
                return (sig.bit_size + 7) // 8
        return 1
    
    def _signal_type_to_c_type(self, signal_type: str) -> str:
        """Convert signal type to C type"""
        type_map = {
            'COM_BOOLEAN': 'boolean',
            'COM_UINT8': 'uint8',
            'COM_UINT16': 'uint16',
            'COM_UINT32': 'uint32',
            'COM_SINT8': 'sint8',
            'COM_SINT16': 'sint16',
            'COM_SINT32': 'sint32',
            'COM_FLOAT32': 'float32',
            'COM_FLOAT64': 'float64'
        }
        return type_map.get(signal_type, 'uint8')
    
    def _format_init_value(self, value: Any, signal_type: str) -> str:
        """Format initial value for C code"""
        if signal_type in ['COM_FLOAT32', 'COM_FLOAT64']:
            return f'{value}f' if isinstance(value, float) else f'{value}'
        return str(value)


def main():
    parser = argparse.ArgumentParser(description='AUTOSAR COM Configuration Generator')
    parser.add_argument('--input', '-i', required=True, help='Input JSON configuration file')
    parser.add_argument('--output', '-o', required=True, help='Output directory for generated files')
    parser.add_argument('--header-output', help='Output directory for header files (defaults to output)')
    parser.add_argument('--validate-only', action='store_true', help='Only validate, do not generate')
    
    args = parser.parse_args()
    
    # Read input file
    try:
        with open(args.input, 'r') as f:
            config_data = json.load(f)
    except FileNotFoundError:
        print(f"Error: Input file '{args.input}' not found")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in input file: {e}")
        sys.exit(1)
    
    # Parse and generate
    generator = ComConfigGenerator(config_data)
    generator.parse_config()
    
    # Validate configuration
    validator = ConfigValidator(generator)
    errors = validator.validate()
    
    if errors:
        print("Configuration validation failed:")
        for error in errors:
            print(f"  - {error}")
        sys.exit(1)
    
    print("Configuration validation passed!")
    
    if args.validate_only:
        print("Validation-only mode, skipping generation.")
        return
    
    # Generate files
    os.makedirs(args.output, exist_ok=True)
    header_output = args.header_output or args.output
    os.makedirs(header_output, exist_ok=True)
    
    # Write Com_Cfg.h
    cfg_header_path = os.path.join(header_output, 'Com_Cfg.h')
    with open(cfg_header_path, 'w') as f:
        f.write(generator.generate_cfg_header())
    print(f"Generated: {cfg_header_path}")
    
    # Write Com_Lcfg.c
    lcfg_path = os.path.join(args.output, 'Com_Lcfg.c')
    with open(lcfg_path, 'w') as f:
        f.write(generator.generate_lcfg_source())
    print(f"Generated: {lcfg_path}")
    
    print(f"\nGeneration complete!")
    print(f"  Signals: {len(generator.signals)}")
    print(f"  IPdus: {len(generator.ipdus)}")
    print(f"  IPdu Groups: {len(generator.ipdu_groups)}")
    print(f"  Signal Groups: {len(generator.signal_groups)}")


class ConfigValidator:
    """Configuration Validator"""
    
    def __init__(self, generator: ComConfigGenerator):
        self.gen = generator
        self.errors: List[str] = []
    
    def validate(self) -> List[str]:
        """Validate configuration and return list of errors"""
        self._validate_signal_ids()
        self._validate_ipdu_ids()
        self._validate_group_ids()
        self._validate_signal_references()
        self._validate_ipdu_references()
        self._validate_bit_positions()
        self._validate_tx_mode_config()
        self._validate_tmc_config()
        return self.errors
    
    def _validate_signal_ids(self):
        """Check for duplicate signal IDs"""
        seen_ids = set()
        for sig in self.gen.signals:
            if sig.signal_id in seen_ids:
                self.errors.append(f"Duplicate signal ID: {sig.signal_id} for signal '{sig.name}'")
            seen_ids.add(sig.signal_id)
            
            if sig.signal_id >= self.gen.global_config.max_signals:
                self.errors.append(f"Signal ID {sig.signal_id} ('{sig.name}') exceeds max signals ({self.gen.global_config.max_signals})")
    
    def _validate_ipdu_ids(self):
        """Check for duplicate IPDU IDs"""
        seen_ids = set()
        for pdu in self.gen.ipdus:
            if pdu.ipdu_id in seen_ids:
                self.errors.append(f"Duplicate IPDU ID: {pdu.ipdu_id} for IPDU '{pdu.name}'")
            seen_ids.add(pdu.ipdu_id)
            
            if pdu.ipdu_id >= self.gen.global_config.max_ipdus:
                self.errors.append(f"IPDU ID {pdu.ipdu_id} ('{pdu.name}') exceeds max IPdus ({self.gen.global_config.max_ipdus})")
    
    def _validate_group_ids(self):
        """Check for duplicate group IDs"""
        seen_ids = set()
        for grp in self.gen.ipdu_groups:
            if grp.group_id in seen_ids:
                self.errors.append(f"Duplicate IPDU group ID: {grp.group_id} for group '{grp.name}'")
            seen_ids.add(grp.group_id)
    
    def _validate_signal_references(self):
        """Validate that signal references exist"""
        valid_ids = {sig.signal_id for sig in self.gen.signals}
        
        for pdu in self.gen.ipdus:
            for ref in pdu.signal_refs:
                if ref not in valid_ids:
                    self.errors.append(f"IPDU '{pdu.name}' references unknown signal ID: {ref}")
        
        for grp in self.gen.signal_groups:
            for ref in grp.signal_refs:
                if ref not in valid_ids:
                    self.errors.append(f"Signal group '{grp.name}' references unknown signal ID: {ref}")
    
    def _validate_ipdu_references(self):
        """Validate that IPDU references exist"""
        valid_ids = {pdu.ipdu_id for pdu in self.gen.ipdus}
        
        for sig in self.gen.signals:
            if sig.ipdu_id not in valid_ids:
                self.errors.append(f"Signal '{sig.name}' references unknown IPDU ID: {sig.ipdu_id}")
        
        for grp in self.gen.ipdu_groups:
            for ref in grp.ipdu_refs:
                if ref not in valid_ids:
                    self.errors.append(f"IPDU group '{grp.name}' references unknown IPDU ID: {ref}")
    
    def _validate_bit_positions(self):
        """Validate signal bit positions don't overlap within an IPDU"""
        ipdu_signals = {}
        for sig in self.gen.signals:
            if sig.ipdu_id not in ipdu_signals:
                ipdu_signals[sig.ipdu_id] = []
            ipdu_signals[sig.ipdu_id].append(sig)
        
        for ipdu_id, signals in ipdu_signals.items():
            signals.sort(key=lambda s: s.bit_position)
            for i in range(len(signals) - 1):
                current_end = signals[i].bit_position + signals[i].bit_size
                next_start = signals[i + 1].bit_position
                if current_end > next_start:
                    pdu_name = self.gen._get_ipdu_name(ipdu_id)
                    self.errors.append(
                        f"Signal overlap in IPDU '{pdu_name}': "
                        f"'{signals[i].name}' ends at bit {current_end - 1}, "
                        f"'{signals[i + 1].name}' starts at bit {next_start}"
                    )
    
    def _validate_tx_mode_config(self):
        """Validate transmission mode configuration"""
        for pdu in self.gen.ipdus:
            if pdu.tx_mode_false.mode == "COM_MODE_NONE" and pdu.tx_mode_true.mode != "COM_MODE_NONE":
                self.errors.append(f"IPDU '{pdu.name}': TxModeFalse is NONE but TxModeTrue is not NONE")
            
            if pdu.tx_mode_false.mode == "COM_MODE_PERIODIC" and pdu.tx_mode_false.cycle_time_ms == 0:
                self.errors.append(f"IPDU '{pdu.name}': Periodic mode requires non-zero cycle time")
            
            if pdu.tx_mode_false.mode == "COM_MODE_MIXED" and pdu.tx_mode_false.cycle_time_ms == 0:
                self.errors.append(f"IPDU '{pdu.name}': Mixed mode requires non-zero cycle time")
    
    def _validate_tmc_config(self):
        """Validate transmission mode condition configuration"""
        valid_signal_ids = {sig.signal_id for sig in self.gen.signals}
        
        for pdu in self.gen.ipdus:
            if pdu.use_tmc and pdu.tmc_config:
                if pdu.tmc_config.signal_id not in valid_signal_ids:
                    self.errors.append(
                        f"IPDU '{pdu.name}': TMC references unknown signal ID {pdu.tmc_config.signal_id}"
                    )
                
                if pdu.tx_mode_true.mode == "COM_MODE_NONE":
                    self.errors.append(
                        f"IPDU '{pdu.name}': TMC enabled but TxModeTrue is NONE"
                    )


if __name__ == '__main__':
    main()
