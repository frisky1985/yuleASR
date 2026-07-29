#!/usr/bin/env python3
"""
DoIP Configuration Generator

Generates AUTOSAR-compliant C configuration files for DoIP.
"""

import argparse
import json
import os
import sys
from datetime import datetime


class DoIPGenerator:
    """Generates DoIP C configuration files"""

    def __init__(self, config_data: dict):
        self.config = config_data

    def generate_lcfg(self) -> str:
        """Generate DoIP_Lcfg.c content"""
        lines = [
            f"/**",
            f" * @file DoIP_Lcfg.c",
            f" * @brief DoIP Link-time Configuration (Auto-generated)",
            f" * @generated {datetime.now().isoformat()}",
            f" */",
            f"",
            f'#include "DoIP.h"',
            f""
        ]
        return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description='DoIP Configuration Generator')
    parser.add_argument('--config', '-c', required=True, help='Input JSON configuration')
    parser.add_argument('--output', '-o', required=True, help='Output directory')

    args = parser.parse_args()

    with open(args.config, 'r') as f:
        config_data = json.load(f)

    generator = DoIPGenerator(config_data)

    os.makedirs(args.output, exist_ok=True)

    output_path = os.path.join(args.output, 'DoIP_Lcfg.c')
    with open(output_path, 'w') as f:
        f.write(generator.generate_lcfg())

    print(f"Generated: {output_path}")


if __name__ == '__main__':
    main()
