#!/usr/bin/env python3
"""
E2E Configuration Generator

Generates AUTOSAR-compliant C configuration files for E2E Protection
from JSON configuration input.
"""

import argparse
import json
import os
import sys
from dataclasses import dataclass
from typing import List, Dict, Optional
from datetime import datetime


@dataclass
class E2EProfileConfig:
    """E2E Profile Configuration"""
    profile: str  # P01, P02, P04, P05, P06, P07
    data_id: int
    data_length: int
    counter_offset: int
    crc_offset: int
    max_delta_counter: int = 1


class E2EGenerator:
    """Generates E2E C configuration files"""

    PROFILE_TYPES = {
        'P01': 'E2E_P01ConfigType',
        'P02': 'E2E_P02ConfigType',
        'P04': 'E2E_P04ConfigType',
        'P05': 'E2E_P05ConfigType',
        'P06': 'E2E_P06ConfigType',
        'P07': 'E2E_P07ConfigType',
    }

    def __init__(self, config_data: Dict):
        self.config = config_data
        self.profiles: List[E2EProfileConfig] = []
        self._parse_config()

    def _parse_config(self):
        """Parse JSON configuration"""
        for p in self.config.get('profiles', []):
            self.profiles.append(E2EProfileConfig(
                profile=p.get('profile', 'P01'),
                data_id=p.get('data_id', 0),
                data_length=p.get('data_length', 8),
                counter_offset=p.get('counter_offset', 0),
                crc_offset=p.get('crc_offset', 8),
                max_delta_counter=p.get('max_delta_counter', 1)
            ))

    def generate_lcfg(self) -> str:
        """Generate E2E_Lcfg.c content"""
        lines = [
            f"/**",
            f" * @file E2E_Lcfg.c",
            f" * @brief E2E Link-time Configuration (Auto-generated)",
            f" * @generated {datetime.now().isoformat()}",
            f" */",
            f"",
            f'#include "E2E.h"',
            f""
        ]

        # Generate config for each profile type
        profile_groups = {}
        for p in self.profiles:
            if p.profile not in profile_groups:
                profile_groups[p.profile] = []
            profile_groups[p.profile].append(p)

        for profile_type, configs in profile_groups.items():
            type_name = self.PROFILE_TYPES.get(profile_type, f'E2E_{profile_type}ConfigType')
            lines.extend([
                f"",
                f"/* {profile_type} Configurations */",
                f"static const {type_name} E2E_{profile_type}_Configs[{len(configs)}] = {{"
            ])

            for i, cfg in enumerate(configs):
                lines.extend([
                    f"    {{",
                    f"        .DataID = 0x{cfg.data_id:04X},",
                    f"        .DataLength = {cfg.data_length},",
                    f"        .CounterOffset = {cfg.counter_offset},",
                    f"        .CRCOffset = {cfg.crc_offset},",
                    f"        .MaxDeltaCounterInit = {cfg.max_delta_counter}",
                    f"    }}{',' if i < len(configs) - 1 else ''}"
                ])

            lines.append("};")

        lines.append("")
        return '\n'.join(lines)

    def validate(self) -> List[str]:
        """Validate configuration"""
        errors = []
        for i, p in enumerate(self.profiles):
            if p.profile not in self.PROFILE_TYPES:
                errors.append(f"Profile {i}: invalid profile '{p.profile}'")
            if p.data_length > 4096:
                errors.append(f"Profile {i}: data_length {p.data_length} exceeds maximum")
        return errors


def main():
    parser = argparse.ArgumentParser(description='E2E Configuration Generator')
    parser.add_argument('--config', '-c', required=True, help='Input JSON configuration')
    parser.add_argument('--output', '-o', required=True, help='Output directory')

    args = parser.parse_args()

    with open(args.config, 'r') as f:
        config_data = json.load(f)

    generator = E2EGenerator(config_data)

    errors = generator.validate()
    if errors:
        print("Configuration errors:")
        for e in errors:
            print(f"  - {e}")
        sys.exit(1)

    os.makedirs(args.output, exist_ok=True)

    output_path = os.path.join(args.output, 'E2E_Lcfg.c')
    with open(output_path, 'w') as f:
        f.write(generator.generate_lcfg())

    print(f"Generated: {output_path}")
    print(f"Profiles configured: {len(generator.profiles)}")


if __name__ == '__main__':
    main()
