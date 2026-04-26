"""
Unit tests for code_generator module.

Author: Hermes Agent
Version: 1.0.0
"""

import unittest
import tempfile
from pathlib import Path

import sys
sys.path.insert(0, str(Path(__file__).parent.parent))

from diagnostic_config import DiagnosticConfig, DiagServiceConfig, DIDDefinition, DCMParams
from diagnostic_config import DiagPriority, DiagSessionType
from e2e_config import E2EConfig, E2EProfileConfig, DataIDMapping, E2EProfile
from ota_config import OTAConfig, ABPartitionLayout, OTACampaign, ECUUpdate
from code_generator import CCodeGenerator, ConfigValidator


class TestCCodeGenerator(unittest.TestCase):
    """Test C code generator."""

    def setUp(self):
        """Set up test fixtures."""
        self.generator = CCodeGenerator()

    def test_generate_diagnostic_header(self):
        """Test diagnostic header generation."""
        config = DiagnosticConfig()
        config.dcm_params = DCMParams(0x7E0, 0x0001, 0x7DF)
        config.add_service(DiagServiceConfig(
            service_id=0x10, name="SessionControl", priority=DiagPriority.P0,
            supported_sessions=[DiagSessionType.DEFAULT]
        ))

        code = self.generator.generate_diagnostic_header(config)

        # Check for expected content
        self.assertIn("diagnostic_config.h", code)
        self.assertIn("DCM_ECU_ID", code)
        self.assertIn("0x07E0", code)
        self.assertIn("DCM_SVC_SESSIONCONTROL", code)

    def test_generate_e2e_header(self):
        """Test E2E header generation."""
        config = E2EConfig()
        profile = E2EProfileConfig(
            profile=E2EProfile.PROFILE_5,
            data_id=0x0100,
            data_length=16
        )
        config.add_profile(profile)
        config.add_data_id_mapping(DataIDMapping(
            data_id=0x0100, name="VehicleSpeed",
            source_component="A", target_component="B",
            data_type="uint32", signal_name="Speed"
        ))

        code = self.generator.generate_e2e_header(config)

        self.assertIn("e2e_config.h", code)
        self.assertIn("E2E_P5", code)
        self.assertIn("E2E_DATAID_VEHICLESPEED", code)

    def test_generate_ota_header(self):
        """Test OTA header generation."""
        config = OTAConfig()
        layout = config.get_default_partition_layout(8)
        config.add_partition_layout(layout)

        code = self.generator.generate_ota_header(config)

        self.assertIn("ota_config.h", code)
        self.assertIn("OTA_STATE_IDLE", code)
        self.assertIn("OTA_PART_DEFAULT_APP_A_START", code)

    def test_save_all(self):
        """Test saving all generated files."""
        diag_config = DiagnosticConfig()
        diag_config.dcm_params = DCMParams(0x7E0, 0x0001, 0x7DF)

        e2e_config = E2EConfig()
        ota_config = OTAConfig()

        with tempfile.TemporaryDirectory() as tmpdir:
            generated = self.generator.save_all(
                diag_config=diag_config,
                e2e_config=e2e_config,
                ota_config=ota_config,
                output_dir=tmpdir
            )

            self.assertEqual(len(generated), 3)

            # Check files exist
            for filepath in generated:
                self.assertTrue(Path(filepath).exists())


class TestConfigValidator(unittest.TestCase):
    """Test configuration validator."""

    def setUp(self):
        """Set up test fixtures."""
        self.validator = ConfigValidator()

    def test_validate_diagnostic_valid(self):
        """Test validating valid diagnostic config."""
        config = DiagnosticConfig()
        config.dcm_params = DCMParams(0x7E0, 0x0001, 0x7DF)
        config.add_service(DiagServiceConfig(
            service_id=0x10, name="SessionControl", priority=DiagPriority.P0,
            supported_sessions=[]
        ))

        errors = self.validator.validate_diagnostic(config)
        self.assertEqual(len(errors), 0)

    def test_validate_diagnostic_invalid(self):
        """Test validating invalid diagnostic config."""
        config = DiagnosticConfig()
        # Missing DCM params and services

        errors = self.validator.validate_diagnostic(config)
        self.assertIn("DCM parameters not configured", errors)
        self.assertIn("No diagnostic services configured", errors)

    def test_validate_all(self):
        """Test validating all configurations."""
        diag_config = DiagnosticConfig()
        diag_config.dcm_params = DCMParams(0x7E0, 0x0001, 0x7DF)
        diag_config.add_service(DiagServiceConfig(
            service_id=0x10, name="SessionControl", priority=DiagPriority.P0,
            supported_sessions=[]
        ))

        e2e_config = E2EConfig()
        ota_config = OTAConfig()
        ota_config.add_partition_layout(ABPartitionLayout(name="test"))
        ota_config.security_verification.root_cert_hash = "sha256:test123"

        results = self.validator.validate_all(
            diag_config=diag_config,
            e2e_config=e2e_config,
            ota_config=ota_config
        )

        # All should be valid
        for config_type, errors in results.items():
            self.assertEqual(len(errors), 0, f"{config_type} has errors: {errors}")


if __name__ == "__main__":
    unittest.main()
