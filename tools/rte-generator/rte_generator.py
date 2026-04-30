#!/usr/bin/env python3
"""
RTE Code Generator
Generates RTE code from ARXML system description
"""

import sys
import argparse
import xml.etree.ElementTree as ET
from pathlib import Path
from jinja2 import Environment, FileSystemLoader
import yaml

class ARXMLParser:
    """Parse ARXML system description files"""
    
    def __init__(self):
        self.swcs = []
        self.ports = []
        self.interfaces = []
        self.datatypes = []
        self.connections = []
    
    def parse(self, arxml_path):
        """Parse ARXML file and extract RTE information"""
        tree = ET.parse(arxml_path)
        root = tree.getroot()
        
        # Extract namespaces
        ns = {'autosar': 'http://autosar.org/schema/r4.0'}
        
        # Parse software components
        for swc in root.findall('.//autosar:APPLICATION-SW-COMPONENT-TYPE', ns):
            swc_info = {
                'name': swc.find('autosar:SHORT-NAME', ns).text,
                'ports': []
            }
            
            # Parse ports
            for port in swc.findall('.//autosar:P-PORT-PROTOTYPE', ns):
                port_info = {
                    'name': port.find('autosar:SHORT-NAME', ns).text,
                    'type': 'P-PORT',
                    'interface': port.find('autosar:PROVIDED-INTERFACE-TREF', ns).text
                }
                swc_info['ports'].append(port_info)
            
            for port in swc.findall('.//autosar:R-PORT-PROTOTYPE', ns):
                port_info = {
                    'name': port.find('autosar:SHORT-NAME', ns).text,
                    'type': 'R-PORT',
                    'interface': port.find('autosar:REQUIRED-INTERFACE-TREF', ns).text
                }
                swc_info['ports'].append(port_info)
            
            self.swcs.append(swc_info)
        
        # Parse sender-receiver interfaces
        for sri in root.findall('.//autosar:SENDER-RECEIVER-INTERFACE', ns):
            interface = {
                'name': sri.find('autosar:SHORT-NAME', ns).text,
                'type': 'SENDER-RECEIVER',
                'data_elements': []
            }
            
            for de in sri.findall('.//autosar:DATA-ELEMENT-PROTOTYPE', ns):
                de_info = {
                    'name': de.find('autosar:SHORT-NAME', ns).text,
                    'type': de.find('.//autosar:TYPE-TREF', ns).text
                }
                interface['data_elements'].append(de_info)
            
            self.interfaces.append(interface)
        
        # Parse client-server interfaces
        for csi in root.findall('.//autosar:CLIENT-SERVER-INTERFACE', ns):
            interface = {
                'name': csi.find('autosar:SHORT-NAME', ns).text,
                'type': 'CLIENT-SERVER',
                'operations': []
            }
            
            for op in csi.findall('.//autosar:CLIENT-SERVER-OPERATION', ns):
                op_info = {
                    'name': op.find('autosar:SHORT-NAME', ns).text,
                    'arguments': []
                }
                interface['operations'].append(op_info)
            
            self.interfaces.append(interface)
        
        return self


class RTEGenerator:
    """Generate RTE code from parsed ARXML"""
    
    def __init__(self, output_dir):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # Setup Jinja2 environment
        template_dir = Path(__file__).parent / "templates"
        self.env = Environment(loader=FileSystemLoader(template_dir))
    
    def generate(self, parser_result):
        """Generate all RTE files"""
        self.generate_rte_types_h(parser_result)
        self.generate_rte_c(parser_result)
        
        for swc in parser_result.swcs:
            self.generate_swc_rte_h(swc, parser_result)
    
    def generate_rte_types_h(self, parser_result):
        """Generate Rte_Type.h"""
        template = self.env.get_template('Rte_Type.h.j2')
        content = template.render(
            datatypes=parser_result.datatypes,
            interfaces=parser_result.interfaces
        )
        
        output_file = self.output_dir / "Rte_Type.h"
        output_file.write_text(content)
        print(f"Generated: {output_file}")
    
    def generate_rte_c(self, parser_result):
        """Generate Rte.c"""
        template = self.env.get_template('Rte.c.j2')
        content = template.render(
            swcs=parser_result.swcs,
            connections=parser_result.connections
        )
        
        output_file = self.output_dir / "Rte.c"
        output_file.write_text(content)
        print(f"Generated: {output_file}")
    
    def generate_swc_rte_h(self, swc, parser_result):
        """Generate Rte_<SwcName>.h"""
        template = self.env.get_template('Rte_Swc.h.j2')
        content = template.render(
            swc=swc,
            interfaces=parser_result.interfaces
        )
        
        output_file = self.output_dir / f"Rte_{swc['name']}.h"
        output_file.write_text(content)
        print(f"Generated: {output_file}")


def main():
    parser = argparse.ArgumentParser(description='RTE Code Generator')
    parser.add_argument('-i', '--input', required=True, help='Input ARXML file')
    parser.add_argument('-o', '--output', required=True, help='Output directory')
    parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
    
    args = parser.parse_args()
    
    # Parse ARXML
    print(f"Parsing ARXML: {args.input}")
    arxml_parser = ARXMLParser()
    result = arxml_parser.parse(args.input)
    
    print(f"Found {len(result.swcs)} SWCs, {len(result.interfaces)} interfaces")
    
    # Generate RTE code
    print(f"Generating RTE code to: {args.output}")
    generator = RTEGenerator(args.output)
    generator.generate(result)
    
    print("RTE generation complete!")


if __name__ == '__main__':
    main()
