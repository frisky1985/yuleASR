#!/usr/bin/env python3
"""
DDS Configuration CLI Tool

Command-line interface for DDS configuration management:
    - Validate YAML configuration files
    - Generate C code from configuration
    - Convert between YAML and ARXML formats

Usage:
    dds-config validate <yaml>                    Validate configuration file
    dds-config generate <yaml> -o <out>           Generate C code
    dds-config convert --yaml2arxml <yaml> <arxml>  YAML to ARXML
    dds-config convert --arxml2yaml <arxml> <yaml>  ARXML to YAML
"""

import argparse
import sys
import os
import json
import logging
from pathlib import Path
from typing import Dict, List, Optional, Any, Tuple
from datetime import datetime

try:
    import yaml
    HAS_YAML = True
except ImportError:
    HAS_YAML = False

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('dds-config')


class DDSConfigError(Exception):
    """DDS Configuration error"""
    pass


class DDSConfigValidator:
    """Validates DDS configuration files"""
    
    # Schema definitions for validation
    QOS_POLICY_SCHEMA = {
        'reliability': ['BEST_EFFORT', 'RELIABLE'],
        'durability': ['VOLATILE', 'TRANSIENT_LOCAL', 'TRANSIENT', 'PERSISTENT'],
        'history': ['KEEP_LAST', 'KEEP_ALL'],
        'liveliness': ['AUTOMATIC', 'MANUAL_BY_TOPIC', 'MANUAL_BY_PARTICIPANT'],
        'destination_order': ['BY_RECEPTION_TIMESTAMP', 'BY_SOURCE_TIMESTAMP']
    }
    
    REQUIRED_TOP_LEVEL_KEYS = ['system', 'domains']
    REQUIRED_SYSTEM_KEYS = ['name', 'version']
    REQUIRED_DOMAIN_KEYS = ['id', 'name']
    REQUIRED_PARTICIPANT_KEYS = ['name']
    
    def __init__(self):
        self.errors: List[str] = []
        self.warnings: List[str] = []
    
    def validate(self, config: Dict[str, Any]) -> Tuple[bool, List[str], List[str]]:
        """
        Validate configuration dictionary
        
        Returns:
            Tuple of (is_valid, errors, warnings)
        """
        self.errors = []
        self.warnings = []
        
        # Validate top-level structure
        self._validate_top_level(config)
        
        # Validate system section
        if 'system' in config:
            self._validate_system(config['system'])
        
        # Validate domains
        if 'domains' in config:
            self._validate_domains(config['domains'])
        
        # Validate transport section (optional)
        if 'transport' in config:
            self._validate_transport(config['transport'])
        
        # Validate TSN section (optional)
        if 'tsn' in config:
            self._validate_tsn(config['tsn'])
        
        # Validate QoS profiles (optional)
        if 'qos_profiles' in config:
            self._validate_qos_profiles(config['qos_profiles'])
        
        return len(self.errors) == 0, self.errors, self.warnings
    
    def _validate_top_level(self, config: Dict[str, Any]):
        """Validate top-level configuration structure"""
        for key in self.REQUIRED_TOP_LEVEL_KEYS:
            if key not in config:
                self.errors.append(f"Missing required top-level key: '{key}'")
    
    def _validate_system(self, system: Dict[str, Any]):
        """Validate system section"""
        for key in self.REQUIRED_SYSTEM_KEYS:
            if key not in system:
                self.errors.append(f"Missing required system key: '{key}'")
        
        if 'name' in system and not isinstance(system['name'], str):
            self.errors.append("System 'name' must be a string")
        
        if 'version' in system and not isinstance(system['version'], str):
            self.errors.append("System 'version' must be a string")
    
    def _validate_domains(self, domains: List[Dict[str, Any]]):
        """Validate domains configuration"""
        if not isinstance(domains, list):
            self.errors.append("'domains' must be a list")
            return
        
        domain_ids = set()
        for idx, domain in enumerate(domains):
            # Check required keys
            for key in self.REQUIRED_DOMAIN_KEYS:
                if key not in domain:
                    self.errors.append(f"Domain[{idx}]: Missing required key '{key}'")
            
            # Check domain ID uniqueness
            if 'id' in domain:
                if domain['id'] in domain_ids:
                    self.errors.append(f"Domain[{idx}]: Duplicate domain ID '{domain['id']}'")
                domain_ids.add(domain['id'])
                
                if not isinstance(domain['id'], int) or domain['id'] < 0 or domain['id'] > 232:
                    self.errors.append(f"Domain[{idx}]: ID must be an integer between 0 and 232")
            
            # Validate participants
            if 'participants' in domain:
                self._validate_participants(domain['participants'], f"Domain[{idx}]")
    
    def _validate_participants(self, participants: List[Dict[str, Any]], context: str):
        """Validate participants configuration"""
        if not isinstance(participants, list):
            self.errors.append(f"{context}: 'participants' must be a list")
            return
        
        participant_names = set()
        for idx, participant in enumerate(participants):
            ctx = f"{context}.Participant[{idx}]"
            
            # Check required keys
            for key in self.REQUIRED_PARTICIPANT_KEYS:
                if key not in participant:
                    self.errors.append(f"{ctx}: Missing required key '{key}'")
            
            # Check name uniqueness
            if 'name' in participant:
                if participant['name'] in participant_names:
                    self.errors.append(f"{ctx}: Duplicate participant name '{participant['name']}'")
                participant_names.add(participant['name'])
            
            # Validate publishers
            if 'publishers' in participant:
                self._validate_publishers(participant['publishers'], ctx)
            
            # Validate subscribers
            if 'subscribers' in participant:
                self._validate_subscribers(participant['subscribers'], ctx)
    
    def _validate_publishers(self, publishers: List[Dict[str, Any]], context: str):
        """Validate publishers configuration"""
        if not isinstance(publishers, list):
            self.errors.append(f"{context}: 'publishers' must be a list")
            return
        
        for idx, pub in enumerate(publishers):
            ctx = f"{context}.Publisher[{idx}]"
            
            if 'topic' not in pub:
                self.errors.append(f"{ctx}: Missing required key 'topic'")
            
            if 'type' not in pub:
                self.warnings.append(f"{ctx}: Missing 'type' for topic '{pub.get('topic', 'unknown')}'")
            
            # Validate QoS if present
            if 'qos' in pub:
                self._validate_qos(pub['qos'], ctx)
    
    def _validate_subscribers(self, subscribers: List[Dict[str, Any]], context: str):
        """Validate subscribers configuration"""
        if not isinstance(subscribers, list):
            self.errors.append(f"{context}: 'subscribers' must be a list")
            return
        
        for idx, sub in enumerate(subscribers):
            ctx = f"{context}.Subscriber[{idx}]"
            
            if 'topic' not in sub:
                self.errors.append(f"{ctx}: Missing required key 'topic'")
            
            # Validate QoS if present
            if 'qos' in sub:
                self._validate_qos(sub['qos'], ctx)
    
    def _validate_qos(self, qos: Dict[str, Any], context: str):
        """Validate QoS configuration"""
        # Validate reliability
        if 'reliability' in qos:
            if qos['reliability'] not in self.QOS_POLICY_SCHEMA['reliability']:
                self.errors.append(
                    f"{context}.qos.reliability: Invalid value '{qos['reliability']}'. "
                    f"Must be one of: {self.QOS_POLICY_SCHEMA['reliability']}"
                )
        
        # Validate durability
        if 'durability' in qos:
            if qos['durability'] not in self.QOS_POLICY_SCHEMA['durability']:
                self.errors.append(
                    f"{context}.qos.durability: Invalid value '{qos['durability']}'"
                )
        
        # Validate history
        if 'history' in qos:
            if isinstance(qos['history'], dict):
                if 'kind' in qos['history']:
                    if qos['history']['kind'] not in self.QOS_POLICY_SCHEMA['history']:
                        self.errors.append(
                            f"{context}.qos.history.kind: Invalid value '{qos['history']['kind']}'"
                        )
            elif isinstance(qos['history'], str):
                if qos['history'] not in self.QOS_POLICY_SCHEMA['history']:
                    self.errors.append(
                        f"{context}.qos.history: Invalid value '{qos['history']}'"
                    )
        
        # Validate deadline
        if 'deadline' in qos:
            if isinstance(qos['deadline'], dict):
                if 'sec' not in qos['deadline'] and 'nanosec' not in qos['deadline']:
                    self.warnings.append(f"{context}.qos.deadline: Should have 'sec' or 'nanosec'")
            elif not isinstance(qos['deadline'], (int, float)):
                self.errors.append(f"{context}.qos.deadline: Must be a number or object with sec/nanosec")
    
    def _validate_transport(self, transport: Dict[str, Any]):
        """Validate transport configuration"""
        if 'kind' in transport:
            valid_kinds = ['UDP', 'TCP', 'SHM', 'TSN']
            if transport['kind'] not in valid_kinds:
                self.errors.append(f"transport.kind: Must be one of {valid_kinds}")
        
        if 'port_base' in transport:
            if not isinstance(transport['port_base'], int) or transport['port_base'] < 0 or transport['port_base'] > 65535:
                self.errors.append("transport.port_base: Must be an integer between 0 and 65535")
    
    def _validate_tsn(self, tsn: Dict[str, Any]):
        """Validate TSN configuration"""
        if not isinstance(tsn.get('enabled', True), bool):
            self.errors.append("tsn.enabled: Must be a boolean")
        
        if 'streams' in tsn:
            if not isinstance(tsn['streams'], list):
                self.errors.append("tsn.streams: Must be a list")
            else:
                for idx, stream in enumerate(tsn['streams']):
                    if 'name' not in stream:
                        self.errors.append(f"tsn.streams[{idx}]: Missing 'name'")
                    if 'priority' in stream:
                        if not isinstance(stream['priority'], int) or stream['priority'] < 0 or stream['priority'] > 7:
                            self.errors.append(f"tsn.streams[{idx}].priority: Must be 0-7")
    
    def _validate_qos_profiles(self, profiles: Dict[str, Any]):
        """Validate QoS profiles"""
        for name, profile in profiles.items():
            if not isinstance(profile, dict):
                self.errors.append(f"qos_profiles.{name}: Must be an object")
                continue
            self._validate_qos(profile, f"qos_profiles.{name}")


class DDSCodeGenerator:
    """Generates C code from DDS configuration"""
    
    def __init__(self, config: Dict[str, Any]):
        self.config = config
        self.system_name = config.get('system', {}).get('name', 'DDS_System')
        self.version = config.get('system', {}).get('version', '1.0.0')
    
    def generate(self) -> Dict[str, str]:
        """
        Generate C code files
        
        Returns:
            Dictionary mapping filename to content
        """
        files = {}
        
        # Generate header file
        files['dds_config.h'] = self._generate_header()
        
        # Generate source file
        files['dds_config.c'] = self._generate_source()
        
        # Generate QoS configuration
        files['dds_qos_config.c'] = self._generate_qos_config()
        
        return files
    
    def _generate_header(self) -> str:
        """Generate header file"""
        header = f"""/**
 * @file dds_config.h
 * @brief Auto-generated DDS Configuration
 * @version {self.version}
 * @generated {datetime.now().isoformat()}
 * @warning This file is auto-generated. Do not edit manually.
 */

#ifndef DDS_CONFIG_H
#define DDS_CONFIG_H

#include "dds/core/dds_core.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {{
#endif

/* ============================================================================
 * System Configuration
 * ============================================================================ */

#define DDS_SYSTEM_NAME "{self.system_name}"
#define DDS_SYSTEM_VERSION "{self.version}"

/* ============================================================================
 * Domain Configuration
 * ============================================================================ */

#define DDS_NUM_DOMAINS {len(self.config.get('domains', []))}

/* Domain IDs */
"""
        
        for domain in self.config.get('domains', []):
            domain_name = domain['name'].upper()
            header += f"#define DDS_DOMAIN_ID_{domain_name} {domain['id']}U\n"
        
        header += """
/* ============================================================================
 * DomainParticipant Configuration
 * ============================================================================ */

"""
        
        participant_count = 0
        for domain in self.config.get('domains', []):
            participant_count += len(domain.get('participants', []))
        
        header += f"#define DDS_NUM_PARTICIPANTS {participant_count}\n\n"
        
        # Generate participant declarations
        for domain in self.config.get('domains', []):
            domain_name = domain['name'].upper()
            for participant in domain.get('participants', []):
                participant_name = participant['name'].upper()
                header += f"/* Participant: {participant['name']} (Domain {domain['id']}) */\n"
                header += f"extern dds_domain_participant_t *g_participant_{participant_name};\n"
        
        header += """
/* ============================================================================
 * Topic Configuration
 * ============================================================================ */

"""
        
        # Collect all topics
        topics = set()
        for domain in self.config.get('domains', []):
            for participant in domain.get('participants', []):
                for pub in participant.get('publishers', []):
                    topics.add((pub.get('topic'), pub.get('type', 'void*')))
                for sub in participant.get('subscribers', []):
                    topics.add((sub.get('topic'), sub.get('type', 'void*')))
        
        header += f"#define DDS_NUM_TOPICS {len(topics)}\n\n"
        
        for topic_name, topic_type in sorted(topics):
            if topic_name:
                topic_upper = topic_name.upper()
                header += f"/* Topic: {topic_name} (Type: {topic_type}) */\n"
                header += f"#define DDS_TOPIC_NAME_{topic_upper} \"{topic_name}\"\n"
                header += f"extern dds_topic_t *g_topic_{topic_name.lower()};\n\n"
        
        header += """
/* ============================================================================
 * QoS Profile Declarations
 * ============================================================================ */

"""
        
        # QoS profiles
        for profile_name in self.config.get('qos_profiles', {}).keys():
            profile_upper = profile_name.upper()
            header += f"extern const dds_qos_t g_qos_profile_{profile_name.lower()};\n"
        
        header += """
/* ============================================================================
 * Function Declarations
 * ============================================================================ */

/**
 * @brief Initialize DDS configuration
 * @return ETH_OK on success
 */
eth_status_t dds_config_init(void);

/**
 * @brief Deinitialize DDS configuration
 */
void dds_config_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* DDS_CONFIG_H */
"""
        return header
    
    def _generate_source(self) -> str:
        """Generate source file"""
        source = f"""/**
 * @file dds_config.c
 * @brief Auto-generated DDS Configuration Implementation
 * @version {self.version}
 * @generated {datetime.now().isoformat()}
 */

#include "dds_config.h"
#include <string.h>

/* ============================================================================
 * DomainParticipant Instances
 * ============================================================================ */

"""
        
        # Generate participant instances
        for domain in self.config.get('domains', []):
            for participant in domain.get('participants', []):
                participant_name = participant['name'].upper()
                source += f"dds_domain_participant_t *g_participant_{participant_name} = NULL;\n"
        
        source += """
/* ============================================================================
 * Topic Instances
 * ============================================================================ */

"""
        
        # Generate topic instances
        topics = set()
        for domain in self.config.get('domains', []):
            for participant in domain.get('participants', []):
                for pub in participant.get('publishers', []):
                    if pub.get('topic'):
                        topics.add(pub['topic'])
                for sub in participant.get('subscribers', []):
                    if sub.get('topic'):
                        topics.add(sub['topic'])
        
        for topic_name in sorted(topics):
            source += f"dds_topic_t *g_topic_{topic_name.lower()} = NULL;\n"
        
        source += """
/* ============================================================================
 * Configuration Initialization
 * ============================================================================ */

eth_status_t dds_config_init(void)
{
    eth_status_t status;
    
    /* Initialize runtime */
    status = dds_runtime_init();
    if (status != ETH_OK) {
        return status;
    }
    
"""
        
        # Generate participant creation
        for domain in self.config.get('domains', []):
            domain_id = domain['id']
            for participant in domain.get('participants', []):
                participant_name = participant['name'].upper()
                source += f"    /* Create participant: {participant['name']} */\n"
                source += f"    g_participant_{participant_name} = dds_create_participant({domain_id});\n"
                source += f"    if (g_participant_{participant_name} == NULL) {{\n"
                source += f"        return ETH_ERROR;\n"
                source += f"    }}\n\n"
        
        source += """    return ETH_OK;
}

void dds_config_deinit(void)
{
"""
        
        # Generate cleanup code
        for domain in self.config.get('domains', []):
            for participant in domain.get('participants', []):
                participant_name = participant['name'].upper()
                source += f"    if (g_participant_{participant_name}) {{\n"
                source += f"        dds_delete_participant(g_participant_{participant_name});\n"
                source += f"        g_participant_{participant_name} = NULL;\n"
                source += f"    }}\n"
        
        source += """
    dds_runtime_deinit();
}
"""
        return source
    
    def _generate_qos_config(self) -> str:
        """Generate QoS configuration"""
        qos_source = f"""/**
 * @file dds_qos_config.c
 * @brief Auto-generated QoS Configuration
 * @version {self.version}
 * @generated {datetime.now().isoformat()}
 */

#include "dds_config.h"
#include <string.h>

/* ============================================================================
 * QoS Profiles
 * ============================================================================ */

"""
        
        for profile_name, profile in self.config.get('qos_profiles', {}).items():
            qos_source += f"/* QoS Profile: {profile_name} */\n"
            qos_source += f"const dds_qos_t g_qos_profile_{profile_name.lower()} = {{\n"
            
            if 'reliability' in profile:
                qos_source += f"    .reliability = DDS_{profile['reliability']},\n"
            if 'durability' in profile:
                qos_source += f"    .durability = DDS_{profile['durability']},\n"
            if 'history' in profile:
                if isinstance(profile['history'], dict):
                    qos_source += f"    .history_kind = DDS_{profile['history'].get('kind', 'KEEP_LAST')},\n"
                    qos_source += f"    .history_depth = {profile['history'].get('depth', 1)},\n"
                else:
                    qos_source += f"    .history_kind = DDS_{profile['history']},\n"
            
            qos_source += "};\n\n"
        
        return qos_source


class ARXMLConverter:
    """Converts between YAML and ARXML formats"""
    
    def yaml_to_arxml(self, config: Dict[str, Any]) -> str:
        """Convert YAML configuration to ARXML format"""
        arxml = """<?xml version="1.0" encoding="UTF-8"?>
<AUTOSAR xmlns="http://autosar.org/schema/r4.0">
  <AR-PACKAGES>
    <AR-PACKAGE>
      <SHORT-NAME>DDS_Configuration</SHORT-NAME>
"""
        
        # System configuration
        system = config.get('system', {})
        arxml += f"""      <ELEMENTS>
        <SYSTEM>
          <SHORT-NAME>{system.get('name', 'DDS_System')}</SHORT-NAME>
          <ADMIN-DATA>
            <SDGS>
              <SDG GID="Version">{system.get('version', '1.0.0')}</SDG>
            </SDGS>
          </ADMIN-DATA>
"""
        
        # Add domains
        arxml += "          <MAPPINGS>\n"
        for domain in config.get('domains', []):
            arxml += f"""            <ECU-TO-DDS-DOMAIN-MAPPING>
              <SHORT-NAME>Domain_{domain['name']}</SHORT-NAME>
              <DDS-DOMAIN-ID>{domain['id']}</DDS-DOMAIN-ID>
"""
            for participant in domain.get('participants', []):
                arxml += f"""              <DDS-PARTICIPANT-REF DEST="DDS-PARTICIPANT">{participant['name']}</DDS-PARTICIPANT-REF>
"""
            arxml += "            </ECU-TO-DDS-DOMAIN-MAPPING>\n"
        arxml += "          </MAPPINGS>\n"
        arxml += "        </SYSTEM>\n"
        
        # Add DDS participants
        for domain in config.get('domains', []):
            for participant in domain.get('participants', []):
                arxml += f"""        <DDS-PARTICIPANT>
          <SHORT-NAME>{participant['name']}</SHORT-NAME>
          <DOMAIN-REF DEST="DDS-DOMAIN">{domain['name']}</DOMAIN-REF>
"""
                # Add publishers
                for pub in participant.get('publishers', []):
                    arxml += f"""          <PUBLISHER>
            <SHORT-NAME>Pub_{pub.get('topic', 'Unknown')}</SHORT-NAME>
            <TOPIC-NAME>{pub.get('topic', '')}</TOPIC-NAME>
            <TYPE-NAME>{pub.get('type', '')}</TYPE-NAME>
          </PUBLISHER>
"""
                # Add subscribers
                for sub in participant.get('subscribers', []):
                    arxml += f"""          <SUBSCRIBER>
            <SHORT-NAME>Sub_{sub.get('topic', 'Unknown')}</SHORT-NAME>
            <TOPIC-NAME>{sub.get('topic', '')}</TOPIC-NAME>
            <TYPE-NAME>{sub.get('type', '')}</TYPE-NAME>
          </SUBSCRIBER>
"""
                arxml += "        </DDS-PARTICIPANT>\n"
        
        # Add QoS profiles
        for profile_name, profile in config.get('qos_profiles', {}).items():
            arxml += f"""        <DDS-QOS-PROFILE>
          <SHORT-NAME>{profile_name}</SHORT-NAME>
"""
            if 'reliability' in profile:
                arxml += f"          <RELIABILITY>{profile['reliability']}</RELIABILITY>\n"
            if 'durability' in profile:
                arxml += f"          <DURABILITY>{profile['durability']}</DURABILITY>\n"
            arxml += "        </DDS-QOS-PROFILE>\n"
        
        arxml += """      </ELEMENTS>
    </AR-PACKAGE>
  </AR-PACKAGES>
</AUTOSAR>
"""
        return arxml
    
    def arxml_to_yaml(self, arxml_content: str) -> Dict[str, Any]:
        """Convert ARXML to YAML configuration"""
        # Simplified ARXML parsing
        import xml.etree.ElementTree as ET
        
        try:
            root = ET.fromstring(arxml_content)
        except ET.ParseError as e:
            raise DDSConfigError(f"Failed to parse ARXML: {e}")
        
        config = {
            'system': {'name': 'DDS_System', 'version': '1.0.0'},
            'domains': []
        }
        
        # Extract namespace
        ns = {'': 'http://autosar.org/schema/r4.0'}
        
        # Find system
        system_elem = root.find('.//SYSTEM', ns)
        if system_elem is not None:
            short_name = system_elem.find('SHORT-NAME', ns)
            if short_name is not None:
                config['system']['name'] = short_name.text
        
        return config


def load_config(file_path: str) -> Dict[str, Any]:
    """Load configuration from YAML or JSON file"""
    path = Path(file_path)
    
    if not path.exists():
        raise DDSConfigError(f"File not found: {file_path}")
    
    content = path.read_text(encoding='utf-8')
    
    if path.suffix in ['.yaml', '.yml']:
        if not HAS_YAML:
            raise DDSConfigError("PyYAML is required for YAML files. Install with: pip install pyyaml")
        try:
            return yaml.safe_load(content)
        except yaml.YAMLError as e:
            raise DDSConfigError(f"Failed to parse YAML: {e}")
    elif path.suffix == '.json':
        try:
            return json.loads(content)
        except json.JSONDecodeError as e:
            raise DDSConfigError(f"Failed to parse JSON: {e}")
    else:
        # Try YAML first, then JSON
        if HAS_YAML:
            try:
                return yaml.safe_load(content)
            except yaml.YAMLError:
                pass
        try:
            return json.loads(content)
        except json.JSONDecodeError as e:
            raise DDSConfigError(f"Failed to parse file: {e}")


def save_config(config: Dict[str, Any], file_path: str, format: str = 'yaml'):
    """Save configuration to file"""
    path = Path(file_path)
    path.parent.mkdir(parents=True, exist_ok=True)
    
    if format in ['yaml', 'yml']:
        if not HAS_YAML:
            raise DDSConfigError("PyYAML is required for YAML output")
        content = yaml.dump(config, default_flow_style=False, sort_keys=False, allow_unicode=True)
    else:
        content = json.dumps(config, indent=2)
    
    path.write_text(content, encoding='utf-8')


def cmd_validate(args):
    """Validate command handler"""
    try:
        config = load_config(args.config_file)
        validator = DDSConfigValidator()
        is_valid, errors, warnings = validator.validate(config)
        
        if warnings:
            logger.warning("Warnings:")
            for warning in warnings:
                logger.warning(f"  - {warning}")
        
        if is_valid:
            logger.info(f"Configuration is valid: {args.config_file}")
            return 0
        else:
            logger.error("Configuration validation failed:")
            for error in errors:
                logger.error(f"  - {error}")
            return 1
    except DDSConfigError as e:
        logger.error(f"Error: {e}")
        return 1


def cmd_generate(args):
    """Generate command handler"""
    try:
        config = load_config(args.config_file)
        
        # Validate first
        validator = DDSConfigValidator()
        is_valid, errors, warnings = validator.validate(config)
        
        if not is_valid:
            logger.error("Configuration validation failed:")
            for error in errors:
                logger.error(f"  - {error}")
            return 1
        
        # Generate code
        generator = DDSCodeGenerator(config)
        files = generator.generate()
        
        # Create output directory
        output_dir = Path(args.output)
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Write files
        for filename, content in files.items():
            file_path = output_dir / filename
            file_path.write_text(content, encoding='utf-8')
            logger.info(f"Generated: {file_path}")
        
        logger.info(f"Code generation complete. Output: {output_dir}")
        return 0
    except DDSConfigError as e:
        logger.error(f"Error: {e}")
        return 1


def cmd_convert(args):
    """Convert command handler"""
    converter = ARXMLConverter()
    
    try:
        if args.yaml2arxml:
            # YAML to ARXML
            config = load_config(args.input)
            arxml_content = converter.yaml_to_arxml(config)
            Path(args.output).write_text(arxml_content, encoding='utf-8')
            logger.info(f"Converted YAML to ARXML: {args.output}")
        
        elif args.arxml2yaml:
            # ARXML to YAML
            arxml_content = Path(args.input).read_text(encoding='utf-8')
            config = converter.arxml_to_yaml(arxml_content)
            save_config(config, args.output, 'yaml')
            logger.info(f"Converted ARXML to YAML: {args.output}")
        
        else:
            logger.error("Please specify --yaml2arxml or --arxml2yaml")
            return 1
        
        return 0
    except DDSConfigError as e:
        logger.error(f"Error: {e}")
        return 1


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        prog='dds-config',
        description='DDS Configuration Tool - CLI for managing DDS configurations'
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # Validate command
    validate_parser = subparsers.add_parser(
        'validate',
        help='Validate a DDS configuration file'
    )
    validate_parser.add_argument(
        'config_file',
        help='Path to YAML/JSON configuration file'
    )
    validate_parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Enable verbose output'
    )
    
    # Generate command
    generate_parser = subparsers.add_parser(
        'generate',
        help='Generate C code from configuration'
    )
    generate_parser.add_argument(
        'config_file',
        help='Path to YAML/JSON configuration file'
    )
    generate_parser.add_argument(
        '-o', '--output',
        default='generated',
        help='Output directory for generated files (default: generated)'
    )
    
    # Convert command
    convert_parser = subparsers.add_parser(
        'convert',
        help='Convert between YAML and ARXML formats'
    )
    convert_parser.add_argument(
        '--yaml2arxml',
        action='store_true',
        help='Convert YAML to ARXML'
    )
    convert_parser.add_argument(
        '--arxml2yaml',
        action='store_true',
        help='Convert ARXML to YAML'
    )
    convert_parser.add_argument(
        'input',
        help='Input file path'
    )
    convert_parser.add_argument(
        'output',
        help='Output file path'
    )
    
    args = parser.parse_args()
    
    if args.command is None:
        parser.print_help()
        return 1
    
    if args.command == 'validate' and args.verbose:
        logger.setLevel(logging.DEBUG)
    
    # Route to command handler
    handlers = {
        'validate': cmd_validate,
        'generate': cmd_generate,
        'convert': cmd_convert
    }
    
    return handlers[args.command](args)


if __name__ == '__main__':
    sys.exit(main())
