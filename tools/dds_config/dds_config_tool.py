#!/usr/bin/env python3
"""
DDS Configuration Tool - Main Entry Point

This tool provides a comprehensive GUI for configuring DDS (Data Distribution Service)
settings for automotive Ethernet applications with AUTOSAR integration.

Features:
- Domain configuration (ID, discovery protocols)
- Topic configuration (name, data type, QoS)
- QoS configuration (Reliability, Durability, etc.)
- Transport layer configuration
- Security configuration (Authentication, Encryption)
- E2E protection profiles
- AUTOSAR integration (EcuM, BswM, SoAd, PduR mapping)

Author: DDS Integration Team
Version: 1.0.0
"""

import sys
import os
import argparse
import json
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Optional, Dict, Any

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    from PyQt5.QtWidgets import (
        QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
        QTabWidget, QMenuBar, QMenu, QAction, QFileDialog, QMessageBox,
        QStatusBar, QToolBar, QLabel, QPushButton, QLineEdit
    )
    from PyQt5.QtCore import Qt, QSettings
    from PyQt5.QtGui import QIcon, QKeySequence
    PYQT_AVAILABLE = True
except ImportError:
    PYQT_AVAILABLE = False
    print("Warning: PyQt5 not available. Running in CLI mode.")

# Import our modules
try:
    from gui.main_window import MainWindow
    from parser.xml_parser import XmlParser
    from parser.json_parser import JsonParser
    from validator.config_validator import ConfigValidator
    from generator.c_generator import CGenerator
except ImportError as e:
    print(f"Import error: {e}")


class DDSConfigTool:
    """Main DDS Configuration Tool class."""
    
    VERSION = "1.0.0"
    APP_NAME = "DDS Configuration Tool"
    
    def __init__(self):
        self.current_file: Optional[str] = None
        self.config_data: Dict[str, Any] = {}
        self.settings = QSettings("DDSIntegration", "DDSConfigTool") if PYQT_AVAILABLE else None
        
    def run_gui(self):
        """Launch the GUI application."""
        if not PYQT_AVAILABLE:
            print("Error: PyQt5 is required for GUI mode.")
            print("Install with: pip install PyQt5")
            sys.exit(1)
            
        app = QApplication(sys.argv)
        app.setApplicationName(self.APP_NAME)
        app.setApplicationVersion(self.VERSION)
        
        # Set application style
        app.setStyle('Fusion')
        
        # Create and show main window
        window = MainWindow()
        window.show()
        
        sys.exit(app.exec_())
    
    def run_cli(self, args):
        """Run in CLI mode."""
        if args.command == 'validate':
            return self.validate_config(args.input)
        elif args.command == 'generate':
            return self.generate_code(args.input, args.output, args.format)
        elif args.command == 'convert':
            return self.convert_config(args.input, args.output)
        elif args.command == 'template':
            return self.create_template(args.output, args.type)
        else:
            print(f"Unknown command: {args.command}")
            return 1
    
    def validate_config(self, input_file: str) -> int:
        """Validate a configuration file."""
        print(f"Validating configuration: {input_file}")
        
        validator = ConfigValidator()
        
        try:
            if input_file.endswith('.xml'):
                parser = XmlParser()
                config = parser.parse(input_file)
            elif input_file.endswith('.json'):
                parser = JsonParser()
                config = parser.parse(input_file)
            else:
                print("Error: Unsupported file format. Use .xml or .json")
                return 1
            
            result = validator.validate(config)
            
            if result.is_valid:
                print("Configuration is valid!")
                return 0
            else:
                print("Configuration validation failed:")
                for error in result.errors:
                    print(f"  - {error}")
                return 1
                
        except Exception as e:
            print(f"Error: {e}")
            return 1
    
    def generate_code(self, input_file: str, output_dir: str, format: str) -> int:
        """Generate C code from configuration."""
        print(f"Generating {format} code from: {input_file}")
        print(f"Output directory: {output_dir}")
        
        try:
            # Parse input
            if input_file.endswith('.xml'):
                parser = XmlParser()
                config = parser.parse(input_file)
            elif input_file.endswith('.json'):
                parser = JsonParser()
                config = parser.parse(input_file)
            else:
                print("Error: Unsupported file format")
                return 1
            
            # Generate code
            generator = CGenerator()
            generator.generate(config, output_dir)
            
            print("Code generation complete!")
            return 0
            
        except Exception as e:
            print(f"Error: {e}")
            return 1
    
    def convert_config(self, input_file: str, output_file: str) -> int:
        """Convert between XML and JSON formats."""
        print(f"Converting {input_file} to {output_file}")
        
        try:
            # Parse input
            if input_file.endswith('.xml'):
                parser = XmlParser()
                config = parser.parse(input_file)
            elif input_file.endswith('.json'):
                with open(input_file, 'r') as f:
                    config = json.load(f)
            else:
                print("Error: Unsupported input format")
                return 1
            
            # Write output
            if output_file.endswith('.xml'):
                parser = XmlParser()
                parser.write(config, output_file)
            elif output_file.endswith('.json'):
                with open(output_file, 'w') as f:
                    json.dump(config, f, indent=2)
            else:
                print("Error: Unsupported output format")
                return 1
            
            print("Conversion complete!")
            return 0
            
        except Exception as e:
            print(f"Error: {e}")
            return 1
    
    def create_template(self, output_file: str, template_type: str) -> int:
        """Create a configuration template."""
        print(f"Creating {template_type} template: {output_file}")
        
        templates_dir = Path(__file__).parent / "templates"
        
        if template_type == "automotive_basic":
            template_file = templates_dir / "automotive_basic.xml"
        elif template_type == "automotive_safety":
            template_file = templates_dir / "automotive_safety.xml"
        else:
            print(f"Error: Unknown template type: {template_type}")
            return 1
        
        try:
            import shutil
            shutil.copy(template_file, output_file)
            print(f"Template created: {output_file}")
            return 0
        except Exception as e:
            print(f"Error: {e}")
            return 1


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="DDS Configuration Tool for Automotive Ethernet",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                          Launch GUI
  %(prog)s gui                      Launch GUI
  %(prog)s validate -i config.xml   Validate configuration
  %(prog)s generate -i config.xml -o ./output  Generate C code
  %(prog)s convert -i config.xml -o config.json  Convert format
  %(prog)s template -o new_config.xml -t automotive_basic  Create template
        """
    )
    
    parser.add_argument('--version', action='version', version=f'%(prog)s {DDSConfigTool.VERSION}')
    
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # GUI command (default)
    gui_parser = subparsers.add_parser('gui', help='Launch GUI (default)')
    
    # Validate command
    validate_parser = subparsers.add_parser('validate', help='Validate configuration file')
    validate_parser.add_argument('-i', '--input', required=True, help='Input configuration file')
    
    # Generate command
    generate_parser = subparsers.add_parser('generate', help='Generate C code')
    generate_parser.add_argument('-i', '--input', required=True, help='Input configuration file')
    generate_parser.add_argument('-o', '--output', required=True, help='Output directory')
    generate_parser.add_argument('-f', '--format', default='c', choices=['c', 'h', 'all'],
                                help='Output format')
    
    # Convert command
    convert_parser = subparsers.add_parser('convert', help='Convert between formats')
    convert_parser.add_argument('-i', '--input', required=True, help='Input file')
    convert_parser.add_argument('-o', '--output', required=True, help='Output file')
    
    # Template command
    template_parser = subparsers.add_parser('template', help='Create configuration template')
    template_parser.add_argument('-o', '--output', required=True, help='Output file')
    template_parser.add_argument('-t', '--type', default='automotive_basic',
                                choices=['automotive_basic', 'automotive_safety'],
                                help='Template type')
    
    args = parser.parse_args()
    
    tool = DDSConfigTool()
    
    # If no command specified, launch GUI
    if args.command is None:
        tool.run_gui()
    elif args.command == 'gui':
        tool.run_gui()
    else:
        sys.exit(tool.run_cli(args))


if __name__ == '__main__':
    main()
