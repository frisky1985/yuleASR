#!/usr/bin/env python3
"""
DDS Configuration CLI Tool
Enhanced command-line interface for DDS configuration management
"""

import argparse
import sys
import os
import json
import time
import yaml
from pathlib import Path
from typing import Optional, Dict, Any
from datetime import datetime

# Add parent directory to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from codegen.config_validator import ConfigValidator, ValidationResult
from codegen.dds_idl_parser import DDSIDLParser
from codegen.dds_type_generator import DDSTypeGenerator
from codegen.autosar_arxml_parser import AutosarARXMLParser
from codegen.rte_generator import RTEGenerator, RTEConfig


class DDSConfigCLI:
    """DDS Configuration Command Line Interface"""
    
    def __init__(self):
        self.parser = argparse.ArgumentParser(
            prog='dds-config',
            description='DDS Configuration Management Tool',
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog="""
Examples:
  dds-config validate config.yaml
  dds-config generate --input config.yaml --output generated/
  dds-config convert config.yaml --to-json
  dds-config monitor --config config.yaml
            """
        )
        self.setup_subcommands()
    
    def setup_subcommands(self):
        """Setup subcommands"""
        subparsers = self.parser.add_subparsers(dest='command', help='Available commands')
        
        # Validate command
        validate_parser = subparsers.add_parser(
            'validate',
            help='Validate configuration file'
        )
        validate_parser.add_argument('config', help='Configuration file path')
        validate_parser.add_argument('--schema', help='JSON schema file for validation')
        validate_parser.add_argument('--strict', action='store_true', 
                                     help='Treat warnings as errors')
        validate_parser.add_argument('--output', '-o', help='Output validation report file')
        
        # Generate command
        generate_parser = subparsers.add_parser(
            'generate',
            help='Generate code from configuration'
        )
        generate_parser.add_argument('--input', '-i', required=True, 
                                     help='Input configuration file')
        generate_parser.add_argument('--output', '-o', required=True, 
                                     help='Output directory')
        generate_parser.add_argument('--type', choices=['c', 'cpp', 'idl', 'rte', 'all'],
                                     default='all', help='Type of code to generate')
        generate_parser.add_argument('--dds-impl', choices=['fastdds', 'cyclonedds', 'rti'],
                                     default='fastdds', help='DDS implementation')
        generate_parser.add_argument('--os', choices=['freertos', 'linux', 'qnx'],
                                     default='freertos', help='Target operating system')
        generate_parser.add_argument('--safety', choices=['QM', 'ASIL_A', 'ASIL_B', 'ASIL_C', 'ASIL_D'],
                                     default='QM', help='Safety integrity level')
        
        # Convert command
        convert_parser = subparsers.add_parser(
            'convert',
            help='Convert between configuration formats'
        )
        convert_parser.add_argument('input', help='Input configuration file')
        convert_parser.add_argument('--to-json', action='store_true', 
                                    help='Convert to JSON format')
        convert_parser.add_argument('--to-yaml', action='store_true',
                                    help='Convert to YAML format')
        convert_parser.add_argument('--to-arxml', action='store_true',
                                    help='Convert to ARXML format')
        convert_parser.add_argument('--output', '-o', help='Output file path')
        
        # Monitor command
        monitor_parser = subparsers.add_parser(
            'monitor',
            help='Monitor DDS configuration runtime status'
        )
        monitor_parser.add_argument('--config', '-c', required=True,
                                    help='Configuration file to monitor')
        monitor_parser.add_argument('--interval', '-i', type=int, default=5,
                                    help='Update interval in seconds')
        monitor_parser.add_argument('--metrics', nargs='+',
                                    choices=['latency', 'throughput', 'discovery', 'errors'],
                                    default=['latency', 'throughput'],
                                    help='Metrics to monitor')
        monitor_parser.add_argument('--output', '-o', choices=['text', 'json'],
                                    default='text', help='Output format')
        
        # Diff command
        diff_parser = subparsers.add_parser(
            'diff',
            help='Compare two configuration files'
        )
        diff_parser.add_argument('config1', help='First configuration file')
        diff_parser.add_argument('config2', help='Second configuration file')
        diff_parser.add_argument('--format', choices=['text', 'json', 'html'],
                                 default='text', help='Output format')
        
        # Template command
        template_parser = subparsers.add_parser(
            'template',
            help='Create configuration from template'
        )
        template_parser.add_argument('--type', required=True,
                                     choices=['minimal', 'automotive', 'sensor-network', 
                                             'adas', 'powertrain', 'infotainment'],
                                     help='Template type')
        template_parser.add_argument('--output', '-o', required=True,
                                     help='Output file path')
        template_parser.add_argument('--name', help='System name')
        
        # Parse IDL command
        idl_parser = subparsers.add_parser(
            'parse-idl',
            help='Parse DDS IDL file'
        )
        idl_parser.add_argument('idl_file', help='IDL file to parse')
        idl_parser.add_argument('--output', '-o', help='Output JSON file')
        idl_parser.add_argument('--generate-types', action='store_true',
                                help='Generate type code')
        
        # Parse ARXML command
        arxml_parser = subparsers.add_parser(
            'parse-arxml',
            help='Parse AUTOSAR ARXML file'
        )
        arxml_parser.add_argument('arxml_file', help='ARXML file to parse')
        arxml_parser.add_argument('--output', '-o', help='Output JSON file')
        arxml_parser.add_argument('--generate-rte', action='store_true',
                                  help='Generate RTE code')
    
    def run(self, args=None):
        """Run the CLI"""
        parsed_args = self.parser.parse_args(args)
        
        if not parsed_args.command:
            self.parser.print_help()
            return 1
        
        try:
            handler = getattr(self, f'handle_{parsed_args.command.replace("-", "_")}')
            return handler(parsed_args)
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            return 1
    
    def handle_validate(self, args):
        """Handle validate command"""
        print(f"Validating configuration: {args.config}")
        
        validator = ConfigValidator(args.schema)
        result = validator.validate_file(args.config)
        
        # Print results
        print(f"\n{'='*60}")
        print(f"Validation Result: {'PASSED' if result.is_valid else 'FAILED'}")
        print(f"{'='*60}")
        print(f"Errors:   {len(result.get_errors())}")
        print(f"Warnings: {len(result.get_warnings())}")
        print(f"Info:     {len([i for i in result.issues if i.level.value == 'info'])}")
        print(f"{'='*60}\n")
        
        # Print issues
        for issue in result.issues:
            if args.strict and issue.level.value == 'warning':
                issue_level = 'ERROR'
            else:
                issue_level = issue.level.value.upper()
            
            print(f"[{issue_level}] {issue.code}")
            print(f"  Path: {issue.path or 'root'}")
            print(f"  Message: {issue.message}")
            if issue.suggestion:
                print(f"  Suggestion: {issue.suggestion}")
            print()
        
        # Write report if requested
        if args.output:
            report = {
                'timestamp': datetime.now().isoformat(),
                'config_file': args.config,
                'is_valid': result.is_valid and not (args.strict and result.get_warnings()),
                **result.to_dict()
            }
            with open(args.output, 'w') as f:
                json.dump(report, f, indent=2)
            print(f"Report written to: {args.output}")
        
        return 0 if result.is_valid else 1
    
    def handle_generate(self, args):
        """Handle generate command"""
        print(f"Generating code from: {args.input}")
        print(f"Output directory: {args.output}")
        print(f"DDS Implementation: {args.dds_impl}")
        print(f"Target OS: {args.os}")
        print(f"Safety Level: {args.safety}")
        
        # Load configuration
        with open(args.input, 'r') as f:
            config = yaml.safe_load(f)
        
        os.makedirs(args.output, exist_ok=True)
        
        generated_files = []
        
        # Generate DDS type code
        if args.type in ['all', 'cpp']:
            print("\nGenerating DDS type code...")
            generator = DDSTypeGenerator(dds_impl=args.dds_impl)
            
            # Parse types from config
            if 'types' in config:
                # Create temporary IDL content
                idl_content = self._generate_idl_from_config(config)
                generator.parser.parse_string(idl_content)
                
                files = {
                    'dds_types.hpp': generator._generate_header(),
                    'dds_types.cpp': generator._generate_source(),
                    'dds_type_support.hpp': generator._generate_type_support(),
                    'dds_serialization.hpp': generator._generate_serialization()
                }
                
                for filename, content in files.items():
                    filepath = os.path.join(args.output, filename)
                    with open(filepath, 'w') as f:
                        f.write(content)
                    generated_files.append(filepath)
                    print(f"  Generated: {filepath}")
        
        # Generate RTE code if ARXML available
        if args.type in ['all', 'rte']:
            print("\nGenerating RTE code...")
            rte_config = RTEConfig(
                target_os=args.os,
                dds_implementation=args.dds_impl,
                safety_level=args.safety
            )
            # RTE generation would need ARXML input
            print("  (RTE generation requires ARXML input)")
        
        # Generate CMakeLists.txt
        if args.type in ['all', 'cpp']:
            cmake_content = self._generate_cmake(config, args)
            cmake_path = os.path.join(args.output, 'CMakeLists.txt')
            with open(cmake_path, 'w') as f:
                f.write(cmake_content)
            generated_files.append(cmake_path)
            print(f"  Generated: {cmake_path}")
        
        print(f"\n{'='*60}")
        print(f"Generated {len(generated_files)} files")
        print(f"{'='*60}")
        
        return 0
    
    def handle_convert(self, args):
        """Handle convert command"""
        print(f"Converting: {args.input}")
        
        # Load input file
        with open(args.input, 'r') as f:
            if args.input.endswith('.json'):
                config = json.load(f)
            else:
                config = yaml.safe_load(f)
        
        # Determine output format
        if args.to_json:
            output_content = json.dumps(config, indent=2)
            ext = 'json'
        elif args.to_arxml:
            output_content = self._convert_to_arxml(config)
            ext = 'arxml'
        else:  # default to yaml
            output_content = yaml.dump(config, default_flow_style=False, sort_keys=True)
            ext = 'yaml'
        
        # Determine output path
        if args.output:
            output_path = args.output
        else:
            base = os.path.splitext(args.input)[0]
            output_path = f"{base}.{ext}"
        
        # Write output
        with open(output_path, 'w') as f:
            f.write(output_content)
        
        print(f"Output written to: {output_path}")
        return 0
    
    def handle_monitor(self, args):
        """Handle monitor command"""
        print(f"Monitoring configuration: {args.config}")
        print(f"Update interval: {args.interval}s")
        print(f"Metrics: {', '.join(args.metrics)}")
        print("\nPress Ctrl+C to stop\n")
        
        try:
            while True:
                metrics = self._collect_metrics(args)
                
                if args.output == 'json':
                    print(json.dumps(metrics))
                else:
                    self._print_metrics(metrics)
                
                time.sleep(args.interval)
        except KeyboardInterrupt:
            print("\nMonitoring stopped")
        
        return 0
    
    def handle_diff(self, args):
        """Handle diff command"""
        print(f"Comparing configurations:")
        print(f"  Left:  {args.config1}")
        print(f"  Right: {args.config2}\n")
        
        # Load both configs
        def load_config(path):
            with open(path, 'r') as f:
                if path.endswith('.json'):
                    return json.load(f)
                return yaml.safe_load(f)
        
        config1 = load_config(args.config1)
        config2 = load_config(args.config2)
        
        # Compare
        validator = ConfigValidator()
        differences = validator.compareObjects(config1, config2, '', [])
        
        # Print results
        if not differences:
            print("Configurations are identical")
            return 0
        
        added = [d for d in differences if d['type'] == 'added']
        removed = [d for d in differences if d['type'] == 'removed']
        modified = [d for d in differences if d['type'] == 'modified']
        
        if added:
            print(f"Added ({len(added)}):")
            for d in added:
                print(f"  + {d['path']}")
        
        if removed:
            print(f"\nRemoved ({len(removed)}):")
            for d in removed:
                print(f"  - {d['path']}")
        
        if modified:
            print(f"\nModified ({len(modified)}):")
            for d in modified:
                print(f"  ~ {d['path']}")
                print(f"    - {json.dumps(d.get('oldValue'))}")
                print(f"    + {json.dumps(d.get('newValue'))}")
        
        return 0
    
    def handle_template(self, args):
        """Handle template command"""
        templates = {
            'minimal': self._minimal_template,
            'automotive': self._automotive_template,
            'sensor-network': self._sensor_network_template,
            'adas': self._adas_template,
            'powertrain': self._powertrain_template,
            'infotainment': self._infotainment_template
        }
        
        template_fn = templates.get(args.type)
        if not template_fn:
            print(f"Unknown template: {args.type}")
            return 1
        
        config = template_fn(args.name or f"{args.type}_system")
        
        # Write output
        output_content = yaml.dump(config, default_flow_style=False, sort_keys=True)
        with open(args.output, 'w') as f:
            f.write(output_content)
        
        print(f"Created {args.type} template: {args.output}")
        return 0
    
    def handle_parse_idl(self, args):
        """Handle parse-idl command"""
        print(f"Parsing IDL file: {args.idl_file}")
        
        parser = DDSIDLParser()
        if not parser.parse_file(args.idl_file):
            print("Failed to parse IDL file")
            return 1
        
        # Output parsed types
        types_dict = parser.to_dict()
        
        if args.output:
            with open(args.output, 'w') as f:
                json.dump(types_dict, f, indent=2)
            print(f"Output written to: {args.output}")
        else:
            print(json.dumps(types_dict, indent=2))
        
        # Generate type code if requested
        if args.generate_types:
            output_dir = os.path.join(os.path.dirname(args.idl_file), 'generated')
            generator = DDSTypeGenerator()
            files = {
                'dds_types.hpp': generator._generate_header(),
                'dds_types.cpp': generator._generate_source()
            }
            os.makedirs(output_dir, exist_ok=True)
            for filename, content in files.items():
                filepath = os.path.join(output_dir, filename)
                with open(filepath, 'w') as f:
                    f.write(content)
                print(f"Generated: {filepath}")
        
        return 0
    
    def handle_parse_arxml(self, args):
        """Handle parse-arxml command"""
        print(f"Parsing ARXML file: {args.arxml_file}")
        
        parser = AutosarARXMLParser()
        if not parser.parse_file(args.arxml_file):
            print("Failed to parse ARXML file")
            return 1
        
        # Output parsed data
        arxml_dict = parser.to_dict()
        
        if args.output:
            with open(args.output, 'w') as f:
                json.dump(arxml_dict, f, indent=2)
            print(f"Output written to: {args.output}")
        else:
            print(json.dumps(arxml_dict, indent=2))
        
        # Generate RTE if requested
        if args.generate_rte:
            output_dir = os.path.join(os.path.dirname(args.arxml_file), 'generated_rte')
            generator = RTEGenerator()
            files = generator.generate_from_arxml(args.arxml_file, output_dir)
            print(f"Generated {len(files)} RTE files in: {output_dir}")
        
        return 0
    
    # Helper methods
    def _generate_idl_from_config(self, config: Dict) -> str:
        """Generate IDL content from config types"""
        idl_lines = ['module DDSGenerated {']
        
        for type_def in config.get('types', []):
            if type_def.get('kind') == 'struct':
                idl_lines.append(f"    struct {type_def['name']} {{")
                for field in type_def.get('fields', []):
                    key_attr = '@key ' if field.get('key') else ''
                    opt_attr = '@optional ' if field.get('optional') else ''
                    idl_lines.append(f"        {key_attr}{opt_attr}{field['type']} {field['name']};")
                idl_lines.append("    };")
            elif type_def.get('kind') == 'enum':
                idl_lines.append(f"    enum {type_def['name']} {{")
                members = type_def.get('members', [])
                for i, member in enumerate(members):
                    suffix = ',' if i < len(members) - 1 else ''
                    idl_lines.append(f"        {member['name']}{suffix}")
                idl_lines.append("    };")
        
        idl_lines.append('};')
        return '\n'.join(idl_lines)
    
    def _generate_cmake(self, config: Dict, args) -> str:
        """Generate CMakeLists.txt"""
        return f'''cmake_minimum_required(VERSION 3.10)
project({config.get('system', {}).get('name', 'DDSApp')})

set(CMAKE_CXX_STANDARD 14)

# Find DDS
find_package(fastrtps REQUIRED)

# Generated types library
add_library(dds_types
    dds_types.cpp
)

target_link_libraries(dds_types
    fastrtps
)

# Application
add_executable(${{PROJECT_NAME}}
    main.cpp
)

target_link_libraries(${{PROJECT_NAME}}
    dds_types
)
'''
    
    def _convert_to_arxml(self, config: Dict) -> str:
        """Convert config to ARXML format"""
        # Simplified ARXML generation
        return f'''<?xml version="1.0" encoding="UTF-8"?>
<AUTOSAR>
  <AR-PACKAGES>
    <AR-PACKAGE>
      <SHORT-NAME>DDS_{config.get('system', {}).get('name', 'Config')}</SHORT-NAME>
    </AR-PACKAGE>
  </AR-PACKAGES>
</AUTOSAR>'''
    
    def _collect_metrics(self, args) -> Dict:
        """Collect runtime metrics (simulated)"""
        import random
        return {
            'timestamp': datetime.now().isoformat(),
            'latency_ms': round(random.uniform(0.5, 5.0), 2),
            'throughput_kbps': round(random.uniform(100, 1000), 2),
            'participants': random.randint(2, 8),
            'topics': random.randint(1, 5)
        }
    
    def _print_metrics(self, metrics: Dict):
        """Print metrics to console"""
        print(f"[{metrics['timestamp']}] "
              f"Latency: {metrics['latency_ms']:>6.2f}ms | "
              f"Throughput: {metrics['throughput_kbps']:>8.2f} KB/s | "
              f"Participants: {metrics['participants']} | "
              f"Topics: {metrics['topics']}")
    
    # Template generators
    def _minimal_template(self, name: str) -> Dict:
        return {
            'version': '1.0.0',
            'system': {'name': name, 'version': '1.0.0'},
            'domains': [{'id': 0, 'name': 'Default'}],
            'topics': []
        }
    
    def _automotive_template(self, name: str) -> Dict:
        return {
            'version': '1.0.0',
            'system': {'name': name, 'version': '1.0.0'},
            'domains': [
                {'id': 0, 'name': 'Powertrain'},
                {'id': 1, 'name': 'Chassis'}
            ],
            'topics': [
                {'name': 'VehicleSpeed', 'type': 'SpeedData', 'domain_id': 0},
                {'name': 'EngineTemp', 'type': 'TemperatureData', 'domain_id': 0}
            ],
            'qos_profiles': [
                {'name': 'reliable', 'reliability': {'kind': 'RELIABLE'}}
            ]
        }
    
    def _adas_template(self, name: str) -> Dict:
        return {
            'version': '1.0.0',
            'system': {'name': name, 'description': 'ADAS Perception System'},
            'domains': [{'id': 10, 'name': 'ADAS'}],
            'topics': [
                {'name': 'CameraObjects', 'type': 'ObjectArray', 'qos_profile': 'sensor'},
                {'name': 'RadarTracks', 'type': 'TrackArray', 'qos_profile': 'sensor'},
                {'name': 'FusionOutput', 'type': 'FusedObjects', 'qos_profile': 'control'}
            ],
            'qos_profiles': [
                {'name': 'sensor', 'reliability': {'kind': 'BEST_EFFORT'}, 'deadline': {'period': '50ms'}},
                {'name': 'control', 'reliability': {'kind': 'RELIABLE'}, 'deadline': {'period': '20ms'}}
            ]
        }
    
    def _powertrain_template(self, name: str) -> Dict:
        return {
            'version': '1.0.0',
            'system': {'name': name, 'description': 'Powertrain Control'},
            'domains': [{'id': 20, 'name': 'Powertrain'}],
            'topics': [
                {'name': 'EngineSpeed', 'type': 'RPMData'},
                {'name': 'TorqueRequest', 'type': 'TorqueData'},
                {'name': 'TransmissionState', 'type': 'GearData'}
            ]
        }
    
    def _infotainment_template(self, name: str) -> Dict:
        return {
            'version': '1.0.0',
            'system': {'name': name, 'description': 'Infotainment System'},
            'domains': [{'id': 30, 'name': 'Infotainment'}],
            'topics': [
                {'name': 'MediaStatus', 'type': 'MediaInfo'},
                {'name': 'NavigationData', 'type': 'NavData'},
                {'name': 'HMICommands', 'type': 'HMIInput'}
            ]
        }
    
    def _sensor_network_template(self, name: str) -> Dict:
        return {
            'version': '1.0.0',
            'system': {'name': name},
            'domains': [{'id': 0, 'name': 'Sensors'}],
            'topics': [
                {'name': 'Temperature', 'type': 'SensorData', 'qos_profile': 'sensor'},
                {'name': 'Pressure', 'type': 'SensorData', 'qos_profile': 'sensor'},
                {'name': 'Humidity', 'type': 'SensorData', 'qos_profile': 'sensor'}
            ]
        }


def main():
    cli = DDSConfigCLI()
    sys.exit(cli.run())


if __name__ == '__main__':
    main()