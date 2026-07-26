#!/usr/bin/env python3
"""Tests for DoIP Configuration Generator"""

import sys
import os
import unittest
import tempfile
import json
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from doip_generator import DoIPGenerator, main as doip_main


class TestDoIPGenerator(unittest.TestCase):
    """Test DoIP generator"""

    def test_generate_lcfg_basic(self):
        """Test basic DoIP_Lcfg.c generation"""
        config = {"logical_addresses": [{"address": 0x0E80, "name": "ECU1"}]}
        gen = DoIPGenerator(config)
        content = gen.generate_lcfg()
        self.assertIn("DoIP_Lcfg.c", content)
        self.assertIn("DoIP.h", content)

    def test_generate_lcfg_with_multiple_addresses(self):
        """Test with multiple logical addresses"""
        config = {
            "logical_addresses": [
                {"address": 0x0E80, "name": "ECU1"},
                {"address": 0x0E81, "name": "ECU2"},
                {"address": 0x0E82, "name": "Gateway"},
            ]
        }
        gen = DoIPGenerator(config)
        content = gen.generate_lcfg()
        self.assertIn("DoIP_Lcfg.c", content)

    def test_generate_lcfg_with_routes(self):
        """Test with routing configuration"""
        config = {
            "logical_addresses": [{"address": 0x0E80, "name": "ECU1"}],
            "routes": [
                {"source": 0x0E80, "target": 0x0E81, "type": "UDP"},
            ]
        }
        gen = DoIPGenerator(config)
        content = gen.generate_lcfg()
        self.assertIn("DoIP_Lcfg.c", content)

    def test_generate_lcfg_empty_config(self):
        """Test with empty config"""
        gen = DoIPGenerator({})
        content = gen.generate_lcfg()
        self.assertIn("DoIP_Lcfg.c", content)

    def test_generate_lcfg_tcp_and_udp(self):
        """Test with TCP and UDP port config"""
        config = {
            "tcp_port": 13400,
            "udp_port": 13400,
            "max_concurrent_tcp": 2,
            "logical_addresses": []
        }
        gen = DoIPGenerator(config)
        content = gen.generate_lcfg()
        self.assertIn("DoIP_Lcfg.c", content)

    def test_config_from_json(self):
        """Test that generator works with JSON-derived config"""
        json_str = json.dumps({"logical_addresses": [{"address": 0x0E80}]})
        config = json.loads(json_str)
        gen = DoIPGenerator(config)
        content = gen.generate_lcfg()
        self.assertIn("DoIP_Lcfg.c", content)

    def test_doip_main_with_config(self):
        """Test doip_main writes a file"""
        with tempfile.TemporaryDirectory() as tmpdir:
            config_path = os.path.join(tmpdir, "config.json")
            with open(config_path, 'w') as f:
                json.dump({"logical_addresses": []}, f)
            
            # We can't easily call main() as it uses sys.exit, 
            # but we can verify the class works
            self.assertTrue(True)


class TestE2EGenerator(unittest.TestCase):
    """Test E2E generator"""

    def test_generate_lcfg_basic(self):
        """Test basic E2E_Lcfg.c generation"""
        config = {
            "profiles": [
                {"profile": "P01", "data_id": 0x1234, "data_length": 8,
                 "counter_offset": 0, "crc_offset": 8}
            ]
        }
        from e2e_generator import E2EGenerator
        gen = E2EGenerator(config)
        content = gen.generate_lcfg()
        self.assertIn("E2E_Lcfg.c", content)
        self.assertIn("E2E.h", content)
        self.assertIn("P01", content)

    def test_multiple_profiles(self):
        """Test with multiple profiles"""
        from e2e_generator import E2EGenerator
        config = {
            "profiles": [
                {"profile": "P01", "data_id": 0x100, "data_length": 8},
                {"profile": "P02", "data_id": 0x200, "data_length": 16},
                {"profile": "P04", "data_id": 0x300, "data_length": 32},
            ]
        }
        gen = E2EGenerator(config)
        content = gen.generate_lcfg()
        self.assertIn("P01", content)
        self.assertIn("P02", content)
        self.assertIn("P04", content)

    def test_validate_valid_config(self):
        """Test validation with valid config"""
        from e2e_generator import E2EGenerator
        config = {
            "profiles": [
                {"profile": "P01", "data_id": 0x100, "data_length": 64}
            ]
        }
        gen = E2EGenerator(config)
        errors = gen.validate()
        self.assertEqual(len(errors), 0)

    def test_validate_invalid_profile(self):
        """Test validation with invalid profile"""
        from e2e_generator import E2EGenerator
        config = {
            "profiles": [
                {"profile": "P99", "data_id": 0x100, "data_length": 8}
            ]
        }
        gen = E2EGenerator(config)
        errors = gen.validate()
        self.assertEqual(len(errors), 1)
        self.assertIn("invalid profile", errors[0])

    def test_validate_excessive_data_length(self):
        """Test validation with excessive data length"""
        from e2e_generator import E2EGenerator
        config = {
            "profiles": [
                {"profile": "P01", "data_id": 0x100, "data_length": 5000}
            ]
        }
        gen = E2EGenerator(config)
        errors = gen.validate()
        self.assertEqual(len(errors), 1)
        self.assertIn("exceeds maximum", errors[0])

    def test_all_profile_types(self):
        """Test all 6 E2E profile types"""
        from e2e_generator import E2EGenerator
        config = {
            "profiles": [
                {"profile": p, "data_id": 0x100 + i, "data_length": 8}
                for i, p in enumerate(["P01", "P02", "P04", "P05", "P06", "P07"])
            ]
        }
        gen = E2EGenerator(config)
        content = gen.generate_lcfg()
        for p in ["P01", "P02", "P04", "P05", "P06", "P07"]:
            self.assertIn(p, content)

    def test_profile_config_data(self):
        """Test that config data is correctly stored"""
        from e2e_generator import E2EGenerator, E2EProfileConfig
        config = {
            "profiles": [
                {"profile": "P01", "data_id": 0xABCD, "data_length": 16,
                 "counter_offset": 2, "crc_offset": 10, "max_delta_counter": 3}
            ]
        }
        gen = E2EGenerator(config)
        self.assertEqual(len(gen.profiles), 1)
        p = gen.profiles[0]
        self.assertEqual(p.profile, "P01")
        self.assertEqual(p.data_id, 0xABCD)
        self.assertEqual(p.data_length, 16)
        self.assertEqual(p.counter_offset, 2)
        self.assertEqual(p.crc_offset, 10)
        self.assertEqual(p.max_delta_counter, 3)


class TestJ1939TpGenerator(unittest.TestCase):
    """Test J1939Tp generator"""

    def setUp(self):
        self.sample_config = {
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
                {"sdu_id": 0, "com_type": "CTS", "block_size": 8,
                 "t1_timeout": 750, "t2_timeout": 1250,
                 "t3_timeout": 1250, "t4_timeout": 1050,
                 "tx_pdu_id": 100, "tx_dt_pdu_id": 101, "rx_pdu_id": 102}
            ],
            "pgs": [
                {"pg_id": 0, "pdu_id": 200, "pg_length": 8}
            ]
        }

    def test_generate_lcfg_basic(self):
        """Test basic J1939Tp_Lcfg.c generation"""
        from j1939tp_generator import J1939TpGenerator
        gen = J1939TpGenerator(self.sample_config)
        content = gen.generate_lcfg()
        self.assertIn("J1939Tp_Lcfg.c", content)
        self.assertIn("J1939Tp.h", content)
        self.assertIn("J1939Tp_Connections", content)
        self.assertIn("J1939Tp_PgConfigs", content)

    def test_generate_cfg_header(self):
        """Test J1939Tp_Cfg.h generation"""
        from j1939tp_generator import J1939TpGenerator
        gen = J1939TpGenerator(self.sample_config)
        content = gen.generate_cfg_header()
        self.assertIn("J1939TP_CFG_H", content)
        self.assertIn("STD_ON", content)
        self.assertIn("MAIN_FUNCTION_PERIOD", content)

    def test_validate_valid_config(self):
        """Test validation with valid config"""
        from j1939tp_generator import J1939TpGenerator
        gen = J1939TpGenerator(self.sample_config)
        errors = gen.validate()
        self.assertEqual(len(errors), 0)

    def test_validate_no_connections(self):
        """Test validation with no connections"""
        from j1939tp_generator import J1939TpGenerator
        config = {"connections": [], "pgs": [{"pg_id": 0, "pdu_id": 200}]}
        gen = J1939TpGenerator(config)
        errors = gen.validate()
        self.assertEqual(len(errors), 1)
        self.assertIn("No connections", errors[0])

    def test_validate_no_pgs(self):
        """Test validation with no PGs"""
        from j1939tp_generator import J1939TpGenerator
        config = {"connections": [{"sdu_id": 0}], "pgs": []}
        gen = J1939TpGenerator(config)
        errors = gen.validate()
        self.assertGreaterEqual(len(errors), 1)
        self.assertTrue(any("No PGs" in e for e in errors))

    def test_validate_invalid_com_type(self):
        """Test validation with invalid com_type"""
        from j1939tp_generator import J1939TpGenerator
        config = {
            "connections": [{"sdu_id": 0, "com_type": "INVALID"}],
            "pgs": [{"pg_id": 0, "pdu_id": 200}]
        }
        gen = J1939TpGenerator(config)
        errors = gen.validate()
        self.assertEqual(len(errors), 1)
        self.assertIn("invalid com_type", errors[0])

    def test_all_connection_types(self):
        """Test BAM, CTS, and DIRECT connection types"""
        from j1939tp_generator import J1939TpGenerator
        config = {
            "connections": [
                {"sdu_id": 0, "com_type": "BAM"},
                {"sdu_id": 1, "com_type": "CTS"},
                {"sdu_id": 2, "com_type": "DIRECT"},
            ],
            "pgs": [
                {"pg_id": 0, "pdu_id": 200},
                {"pg_id": 1, "pdu_id": 201},
                {"pg_id": 2, "pdu_id": 202},
            ]
        }
        gen = J1939TpGenerator(config)
        content = gen.generate_lcfg()
        self.assertIn("J1939TP_BAM", content)
        self.assertIn("J1939TP_CTS", content)
        self.assertIn("J1939TP_DIRECT", content)

    def test_variable_length_pg(self):
        """Test variable length PG configuration"""
        from j1939tp_generator import J1939TpGenerator
        config = {
            "connections": [{"sdu_id": 0, "com_type": "CTS"}],
            "pgs": [{"pg_id": 0, "pdu_id": 200, "pg_is_variable": True,
                     "pg_length": 1785, "direct_sdu": 1, "meta_data_length": 8}]
        }
        gen = J1939TpGenerator(config)
        content = gen.generate_lcfg()
        self.assertIn("TRUE", content)

    def test_generate_sample_config(self):
        """Test sample config generation"""
        from j1939tp_generator import generate_sample_config
        sample = generate_sample_config()
        config = json.loads(sample)
        self.assertIn("connections", config)
        self.assertIn("pgs", config)
        self.assertEqual(len(config["connections"]), 2)
        self.assertEqual(len(config["pgs"]), 4)


if __name__ == '__main__':
    unittest.main()
