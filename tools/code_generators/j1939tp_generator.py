#!/usr/bin/env python3
"""
J1939Tp Configuration Generator

Generates AUTOSAR-compliant C configuration files for J1939 Transport Protocol
from JSON configuration input.

Usage:
    python j1939tp_generator.py --config j1939tp_config.json --output ../../src/bsw/services/j1939tp/src/
"""

import argparse
import json
import os
import sys
from dataclasses import dataclass
from typing import List, Dict, Optional
from datetime import datetime


@dataclass
class ConnectionConfig:
    """J1939Tp Connection Configuration"""
    sdu_id: int
    com_type: str  # BAM, CTS, DIRECT
    block_size: int = 8
    t1_timeout: int = 750
    t2_timeout: int = 1250
    t3_timeout: int = 1250
    t4_timeout: int = 1050
    tx_pdu_id: int = 0
    tx_dt_pdu_id: int = 0
    rx_pdu_id: int = 0


@dataclass
class PgConfig:
    """Parameter Group Configuration"""
    pg_id: int
    pdu_id: int
    direct_n_pdu: bool = False
    pg_is_variable: bool = False
    pg_length: int = 8
    direct_sdu: int = 0
    meta_data_length: int = 8


class J1939TpGenerator:
    """Generates J1939Tp C configuration files"""

    def __init__(self, config_data: Dict):
        self.config = config_data
        self.connections: List[ConnectionConfig] = []
        self.pgs: List[PgConfig] = []
        self._parse_config()

    def _parse_config(self):
        """Parse JSON configuration into data structures"""
        # Parse connections
        for conn in self.config.get('connections', []):
            self.connections.append(ConnectionConfig(
                sdu_id=conn.get('sdu_id', 0),
                com_type=conn.get('com_type', 'DIRECT'),
                block_size=conn.get('block_size', 8),
                t1_timeout=conn.get('t1_timeout', 750),
                t2_timeout=conn.get('t2_timeout', 1250),
                t3_timeout=conn.get('t3_timeout', 1250),
                t4_timeout=conn.get('t4_timeout', 1050),
                tx_pdu_id=conn.get('tx_pdu_id', 0),
                tx_dt_pdu_id=conn.get('tx_dt_pdu_id', 0),
                rx_pdu_id=conn.get('rx_pdu_id', 0)
            ))

        # Parse PGs
        for pg in self.config.get('pgs', []):
            self.pgs.append(PgConfig(
                pg_id=pg.get('pg_id', 0),
                pdu_id=pg.get('pdu_id', 0),
                direct_n_pdu=pg.get('direct_n_pdu', False),
                pg_is_variable=pg.get('pg_is_variable', False),
                pg_length=pg.get('pg_length', 8),
                direct_sdu=pg.get('direct_sdu', 0),
                meta_data_length=pg.get('meta_data_length', 8)
            ))

    def generate_lcfg(self) -> str:
        """Generate J1939Tp_Lcfg.c content"""
        lines = [
            f"/**",
            f" * @file J1939Tp_Lcfg.c",
            f" * @brief J1939Tp Link-time Configuration (Auto-generated)",
            f" * @generated {datetime.now().isoformat()}",
            f" */",
            f"",
            f'#include "J1939Tp.h"',
            f"",
            f"/*==================================================================================================",
            f" *                               Connection Configurations",
            f" *=================================================================================================*/",
            f"static const J1939Tp_ConnectionConfigType J1939Tp_Connections[{len(self.connections)}] = {{"
        ]

        for i, conn in enumerate(self.connections):
            lines.extend([
                f"    /* Connection {i} - {conn.com_type} */",
                f"    {{",
                f"        .SduId = {conn.sdu_id},",
                f"        .ComType = J1939TP_{conn.com_type.upper()},",
                f"        .BlockSize = {conn.block_size}U,",
                f"        .T1Timeout = {conn.t1_timeout}U,",
                f"        .T2Timeout = {conn.t2_timeout}U,",
                f"        .T3Timeout = {conn.t3_timeout}U,",
                f"        .T4Timeout = {conn.t4_timeout}U,",
                f"        .TxPduId = {conn.tx_pdu_id}U,",
                f"        .TxDtPduId = {conn.tx_dt_pdu_id}U,",
                f"        .RxPduId = {conn.rx_pdu_id}U",
                f"    }}{',' if i < len(self.connections) - 1 else ''}"
            ])

        lines.extend([
            f"}};",
            f"",
            f"/*==================================================================================================",
            f" *                                 PG Configurations",
            f" *=================================================================================================*/",
            f"static const J1939Tp_PgConfigType J1939Tp_PgConfigs[{len(self.pgs)}] = {{"
        ])

        for i, pg in enumerate(self.pgs):
            lines.extend([
                f"    /* PG {i} */",
                f"    {{",
                f"        .PgId = {pg.pg_id},",
                f"        .PduId = {pg.pdu_id}U,",
                f"        .DirectNPdu = {'TRUE' if pg.direct_n_pdu else 'FALSE'},",
                f"        .PgIsVariable = {'TRUE' if pg.pg_is_variable else 'FALSE'},",
                f"        .PgLength = {pg.pg_length}U,",
                f"        .DirectSdu = {pg.direct_sdu}U,",
                f"        .MetaDataLength = {pg.meta_data_length}U",
                f"    }}{',' if i < len(self.pgs) - 1 else ''}"
            ])

        lines.extend([
            f"}};",
            f"",
            f"/*==================================================================================================",
            f" *                              Module Configuration",
            f" *=================================================================================================*/",
            f"const J1939Tp_ConfigType J1939Tp_Config = {{",
            f"    .ConnectionCount = {len(self.connections)},",
            f"    .Connections = J1939Tp_Connections,",
            f"    .PgCount = {len(self.pgs)},",
            f"    .PgConfigs = J1939Tp_PgConfigs",
            f"}};",
            f""
        ])

        return '\n'.join(lines)

    def generate_cfg_header(self) -> str:
        """Generate J1939Tp_Cfg.h content"""
        lines = [
            f"/**",
            f" * @file J1939Tp_Cfg.h",
            f" * @brief J1939Tp Pre-compile Configuration (Auto-generated)",
            f" * @generated {datetime.now().isoformat()}",
            f" */",
            f"",
            f"#ifndef J1939TP_CFG_H",
            f"#define J1939TP_CFG_H",
            f"",
            f"/* Pre-compile Switches */",
            f"#define J1939TP_VERSION_INFO_API        {'STD_ON' if self.config.get('version_info_api', True) else 'STD_OFF'}",
            f"#define J1939TP_DEV_ERROR_DETECT        {'STD_ON' if self.config.get('dev_error_detect', True) else 'STD_OFF'}",
            f"#define J1939TP_BAM_TX_ENABLED          {'STD_ON' if self.config.get('bam_tx_enabled', True) else 'STD_OFF'}",
            f"#define J1939TP_BAM_RX_ENABLED          {'STD_ON' if self.config.get('bam_rx_enabled', True) else 'STD_OFF'}",
            f"#define J1939TP_CMDT_TX_ENABLED         {'STD_ON' if self.config.get('cmdt_tx_enabled', True) else 'STD_OFF'}",
            f"#define J1939TP_CMDT_RX_ENABLED         {'STD_ON' if self.config.get('cmdt_rx_enabled', True) else 'STD_OFF'}",
            f"",
            f"/* Timing Parameters */",
            f"#define J1939TP_MAIN_FUNCTION_PERIOD    {self.config.get('main_function_period', 10)}U",
            f"#define J1939TP_N_BROADCAST_TIME        {self.config.get('broadcast_time', 50)}U",
            f"",
            f"/* Maximum Values */",
            f"#define J1939TP_MAX_CONNECTIONS         {self.config.get('max_connections', len(self.connections))}U",
            f"#define J1939TP_MAX_PG                  {self.config.get('max_pg', len(self.pgs))}U",
            f"#define J1939TP_MAX_TX_CHANNELS         {self.config.get('max_tx_channels', 4)}U",
            f"#define J1939TP_MAX_RX_CHANNELS         {self.config.get('max_rx_channels', 4)}U",
            f"",
            f"/* External Configuration Reference */",
            f"extern const J1939Tp_ConfigType J1939Tp_Config;",
            f"",
            f"#endif /* J1939TP_CFG_H */",
            f""
        ]
        return '\n'.join(lines)

    def validate(self) -> List[str]:
        """Validate configuration and return list of errors"""
        errors = []

        if len(self.connections) == 0:
            errors.append("No connections defined")

        if len(self.pgs) == 0:
            errors.append("No PGs defined")

        for i, conn in enumerate(self.connections):
            if conn.sdu_id >= len(self.pgs):
                errors.append(f"Connection {i}: sdu_id {conn.sdu_id} exceeds PG count")

            if conn.com_type not in ['BAM', 'CTS', 'DIRECT']:
                errors.append(f"Connection {i}: invalid com_type '{conn.com_type}'")

        return errors


def generate_sample_config() -> str:
    """Generate sample JSON configuration"""
    sample = {
        "version_info_api": True,
        "dev_error_detect": True,
        "bam_tx_enabled": True,
        "bam_rx_enabled": True,
        "cmdt_tx_enabled": True,
        "cmdt_rx_enabled": True,
        "main_function_period": 10,
        "broadcast_time": 50,
        "max_connections": 8,
        "max_pg": 32,
        "max_tx_channels": 4,
        "max_rx_channels": 4,
        "connections": [
            {
                "sdu_id": 0,
                "com_type": "CTS",
                "block_size": 8,
                "t1_timeout": 750,
                "t2_timeout": 1250,
                "t3_timeout": 1250,
                "t4_timeout": 1050,
                "tx_pdu_id": 100,
                "tx_dt_pdu_id": 101,
                "rx_pdu_id": 102
            },
            {
                "sdu_id": 1,
                "com_type": "BAM",
                "tx_pdu_id": 103,
                "tx_dt_pdu_id": 104,
                "rx_pdu_id": 105
            }
        ],
        "pgs": [
            {"pg_id": 0, "pdu_id": 200, "pg_length": 8},
            {"pg_id": 1, "pdu_id": 201, "pg_length": 8},
            {"pg_id": 2, "pdu_id": 202, "pg_is_variable": True, "pg_length": 1785},
            {"pg_id": 3, "pdu_id": 203, "pg_is_variable": True, "pg_length": 1785}
        ]
    }
    return json.dumps(sample, indent=4)


def main():
    parser = argparse.ArgumentParser(description='J1939Tp Configuration Generator')
    parser.add_argument('--config', '-c', required=True, help='Input JSON configuration file')
    parser.add_argument('--output', '-o', required=True, help='Output directory for generated files')
    parser.add_argument('--generate-sample', action='store_true', help='Generate sample configuration file')

    args = parser.parse_args()

    if args.generate_sample:
        sample_path = os.path.join(os.path.dirname(args.config), 'j1939tp_config_sample.json')
        with open(sample_path, 'w') as f:
            f.write(generate_sample_config())
        print(f"Sample configuration generated: {sample_path}")
        return

    # Load configuration
    try:
        with open(args.config, 'r') as f:
            config_data = json.load(f)
    except FileNotFoundError:
        print(f"Error: Configuration file not found: {args.config}")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in configuration file: {e}")
        sys.exit(1)

    # Create generator
    generator = J1939TpGenerator(config_data)

    # Validate configuration
    errors = generator.validate()
    if errors:
        print("Configuration errors:")
        for error in errors:
            print(f"  - {error}")
        sys.exit(1)

    # Ensure output directory exists
    os.makedirs(args.output, exist_ok=True)

    # Generate files
    lcfg_path = os.path.join(args.output, 'J1939Tp_Lcfg.c')
    with open(lcfg_path, 'w') as f:
        f.write(generator.generate_lcfg())
    print(f"Generated: {lcfg_path}")

    cfg_header_path = os.path.join(os.path.dirname(args.output), '../include/J1939Tp_Cfg.h')
    os.makedirs(os.path.dirname(cfg_header_path), exist_ok=True)
    with open(cfg_header_path, 'w') as f:
        f.write(generator.generate_cfg_header())
    print(f"Generated: {cfg_header_path}")

    print("\nGeneration complete!")
    print(f"  Connections: {len(generator.connections)}")
    print(f"  PGs: {len(generator.pgs)}")


if __name__ == '__main__':
    main()
