#!/usr/bin/env python3
"""
AUTOSAR ARXML Configuration Tool - Main Entry Point
====================================================

This tool provides a unified interface for ARXML parsing, configuration
generation, and integrity checking.

Commands:
    parse       Parse ARXML file to JSON
    generate    Generate C configuration from ARXML or JSON
    check       Check ARXML file integrity

Usage:
    python arxml_tool.py parse <file.arxml> [--output parsed.json]
    python arxml_tool.py generate <file.arxml|parsed.json> --output-dir ./generated
    python arxml_tool.py check <file.arxml> [--report report.txt]

Examples:
    # Parse ARXML file
    python arxml_tool.py parse configs/arxml/example.arxml --output parsed.json

    # Generate configuration
    python arxml_tool.py generate parsed.json --output-dir ./generated

    # Check integrity
    python arxml_tool.py check configs/arxml/example.arxml --report integrity_report.txt
"""

import sys
import os
from pathlib import Path

# Add parser/generator/checker directories to path
sys.path.insert(0, str(Path(__file__).parent / 'parser'))
sys.path.insert(0, str(Path(__file__).parent / 'generator'))
sys.path.insert(0, str(Path(__file__).parent / 'checker'))

def show_help():
    """Show help message."""
    print(__doc__)
    print("\nOptions:")
    print("    -h, --help      Show this help message")
    print("    -v, --version   Show version information")
    print()
    print("For detailed help on a command:")
    print("    python arxml_tool.py <command> --help")


def show_version():
    """Show version information."""
    print("AUTOSAR ARXML Configuration Tool v1.0.0")
    print("Copyright (c) 2026 YuleTech")
    print()
    print("Components:")
    print("    - ARXML Parser v1.0.0")
    print("    - Config Generator v1.0.0")
    print("    - Integrity Checker v1.0.0")


def parse_command(args):
    """Execute parse command."""
    try:
        from arxml_parser import parse_arxml
        import json
        import argparse
        
        parser = argparse.ArgumentParser(
            description='Parse ARXML file to JSON',
            prog='arxml_tool.py parse'
        )
        parser.add_argument('input', help='Input ARXML file')
        parser.add_argument('--output', '-o', default='parsed.json',
                          help='Output JSON file (default: parsed.json)')
        
        cmd_args = parser.parse_args(args)
        
        # Parse ARXML
        print(f"Parsing {cmd_args.input}...")
        arxml_parser = parse_arxml(cmd_args.input)
        result = arxml_parser.parse_all()
        
        # Save to JSON
        with open(cmd_args.output, 'w', encoding='utf-8') as f:
            json.dump(result, f, indent=2, default=str)
        
        print(f"✓ Parsed successfully: {cmd_args.output}")
        print(f"  Found {len(result.get('software_components', []))} software components")
        print(f"  Found {len(result.get('data_types', []))} data types")
        print(f"  Found {len(result.get('port_interfaces', []))} port interfaces")
        
        return 0
        
    except Exception as e:
        print(f"✗ Error: {e}", file=sys.stderr)
        return 1


def generate_command(args):
    """Execute generate command."""
    try:
        from config_generator import ConfigGenerator
        import json
        import argparse
        
        parser = argparse.ArgumentParser(
            description='Generate C configuration from ARXML or JSON',
            prog='arxml_tool.py generate'
        )
        parser.add_argument('input', help='Input ARXML or JSON file')
        parser.add_argument('--output-dir', '-o', default='./generated',
                          help='Output directory (default: ./generated)')
        
        cmd_args = parser.parse_args(args)
        
        input_path = Path(cmd_args.input)
        
        if input_path.suffix.lower() == '.json':
            # Direct JSON input
            generator = ConfigGenerator(output_dir=cmd_args.output_dir)
            generated = generator.generate_from_json(cmd_args.input)
        else:
            # ARXML input - need to parse first
            from arxml_parser import parse_arxml
            
            print(f"Parsing {cmd_args.input}...")
            arxml_parser = parse_arxml(cmd_args.input)
            result = arxml_parser.parse_all()
            
            # Save to temp JSON
            temp_json = Path(cmd_args.output_dir) / 'temp_parsed.json'
            temp_json.parent.mkdir(parents=True, exist_ok=True)
            
            with open(temp_json, 'w', encoding='utf-8') as f:
                json.dump(result, f, indent=2, default=str)
            
            # Generate from temp JSON
            generator = ConfigGenerator(output_dir=cmd_args.output_dir)
            generated = generator.generate_from_json(str(temp_json))
            
            # Clean up temp file
            temp_json.unlink()
        
        print(f"✓ Configuration generated successfully in: {cmd_args.output_dir}")
        print("\nGenerated files:")
        for name, path in generated.items():
            print(f"  ✓ {name}: {path}")
        
        return 0
        
    except Exception as e:
        print(f"✗ Error: {e}", file=sys.stderr)
        return 1


def check_command(args):
    """Execute check command."""
    try:
        import subprocess
        import argparse
        
        parser = argparse.ArgumentParser(
            description='Check ARXML file integrity',
            prog='arxml_tool.py check'
        )
        parser.add_argument('input', help='Input ARXML file')
        parser.add_argument('--report', '-r', default=None,
                          help='Output report file (default: stdout)')
        parser.add_argument('--summary', '-s', action='store_true',
                          help='Show summary only')
        
        cmd_args = parser.parse_args(args)
        
        # Build checker command
        checker_script = Path(__file__).parent / 'checker' / 'integrity_checker.py'
        check_cmd = [sys.executable, str(checker_script), cmd_args.input]
        
        if cmd_args.report:
            check_cmd.extend(['-o', cmd_args.report])
        
        if cmd_args.summary:
            check_cmd.append('-s')
        
        # Run checker
        result = subprocess.run(check_cmd, capture_output=True, text=True)
        
        print(result.stdout)
        if result.stderr:
            print(result.stderr, file=sys.stderr)
        
        return result.returncode
        
    except Exception as e:
        print(f"✗ Error: {e}", file=sys.stderr)
        return 1


def main():
    """Main entry point."""
    if len(sys.argv) < 2:
        show_help()
        return 1
    
    command = sys.argv[1].lower()
    args = sys.argv[2:]
    
    if command in ('-h', '--help'):
        show_help()
        return 0
    
    if command in ('-v', '--version'):
        show_version()
        return 0
    
    if command == 'parse':
        return parse_command(args)
    elif command == 'generate':
        return generate_command(args)
    elif command == 'check':
        return check_command(args)
    else:
        print(f"Unknown command: {command}", file=sys.stderr)
        print("\nAvailable commands:")
        print("    parse       Parse ARXML file to JSON")
        print("    generate    Generate C configuration from ARXML or JSON")
        print("    check       Check ARXML file integrity")
        print("\nUse --help for more information.")
        return 1


if __name__ == '__main__':
    sys.exit(main())
