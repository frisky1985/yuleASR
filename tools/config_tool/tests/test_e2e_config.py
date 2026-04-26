"""
Unit tests for e2e_config module.

Author: Hermes Agent
Version: 1.0.0
"""

import unittest
import json
import tempfile
from pathlib import Path

import sys
sys.path.insert(0, str(Path(__file__).parent.parent))

from e2e_config import (
    E2EConfig, E2EProfileConfig, DataIDMapping, E2EProtection, E2EMonitoring,
    E2EProfile, E2EDataIDMode, CRCPolynomial, CRCType,
    STANDARD_CRC_POLYNOMIALS, get_recommended_profile
)


class TestCRCPolynomial(unittest.TestCase):
    """Test CRC polynomial definitions."""
    
    def test_standard_polynomials(self):
        """Test standard CRC polynomials."""
        crc8 = STANDARD_CRC_POLYNOMIALS[CRCType.CRC_8]
        self.assertEqual(crc8.polynomial, 0x1D)
        self.assertEqual(crc8.initial_value, 0xFF)
        
        crc16 = STANDARD_CRC_POLYNOMIALS[CRCType.CRC_16]
        self.assertEqual(crc16.polynomial, 0x1021)
        
        crc32 = STANDARD_CRC_POLYNOMIALS[CRCType.CRC_32]
        self.assertEqual(crc32.polynomial, 0x04C11DB7)
    
    def test_serialization(self):
        """Test CRC polynomial serialization."""
        crc = CRCPolynomial(
            name="TestCRC",
            crc_type=CRCType.CRC_8,
            polynomial=0x1D,
            initial_value=0xFF,
            final_xor=0xFF,
            input_reflected=False,
            result_reflected=False
        )
        data = crc.to_dict()
        self.assertEqual(data["name"], "TestCRC")
        self.assertEqual(data["polynomial"], "0x1D")
        
        crc2 = CRCPolynomial.from_dict(data)
        self.assertEqual(crc.name, crc2.name)
        self.assertEqual(crc.polynomial, crc2.polynomial)


class TestE2EProfileConfig(unittest.TestCase):
    """Test E2E profile configuration."""
    
    def test_profile_creation(self):
        """Test creating E2E profile config."""
        profile = E2EProfileConfig(
            profile=E2EProfile.PROFILE_5,
            crc_offset=0,
            counter_offset=2,
            data_id=0x0100,
            data_length=16
        )
        self.assertEqual(profile.profile, E2EProfile.PROFILE_5)
        self.assertEqual(profile.data_id, 0x0100)
    
    def test_serialization(self):
        """Test profile serialization."""
        profile = E2EProfileConfig(
            profile=E2EProfile.PROFILE_5,
            data_id=0x0100,
            data_length=16
        )
        data = profile.to_dict()
        self.assertEqual(data["profile"], 5)
        self.assertEqual(data["data_id"], "0x0100")
        
        profile2 = E2EProfileConfig.from_dict(data)
        self.assertEqual(profile.profile, profile2.profile)
        self.assertEqual(profile.data_id, profile2.data_id)


class TestDataIDMapping(unittest.TestCase):
    """Test Data ID mappings."""
    
    def test_mapping_creation(self):
        """Test creating Data ID mapping."""
        mapping = DataIDMapping(
            data_id=0x0100,
            name="VehicleSpeed",
            source_component="Sensor_ECU",
            target_component="Powertrain_ECU",
            data_type="uint32",
            signal_name="Speed",
            profile=E2EProfile.PROFILE_5,
            safety_level="ASIL_B"
        )
        self.assertEqual(mapping.data_id, 0x0100)
        self.assertEqual(mapping.safety_level, "ASIL_B")
        self.assertTrue(mapping.enabled)
    
    def test_serialization(self):
        """Test mapping serialization."""
        mapping = DataIDMapping(
            data_id=0x0100,
            name="VehicleSpeed",
            source_component="Sensor_ECU",
            target_component="Powertrain_ECU",
            data_type="uint32",
            signal_name="Speed"
        )
        data = mapping.to_dict()
        mapping2 = DataIDMapping.from_dict(data)
        self.assertEqual(mapping.data_id, mapping2.data_id)
        self.assertEqual(mapping.name, mapping2.name)


class TestE2EProtection(unittest.TestCase):
    """Test E2E protection configuration."""
    
    def test_protection_creation(self):
        """Test creating E2E protection."""
        profile_cfg = E2EProfileConfig(
            profile=E2EProfile.PROFILE_5,
            data_id=0x0100,
            data_length=16
        )
        protection = E2EProtection(
            name="SpeedSignalProtection",
            source="Sensor_ECU",
            destination="Powertrain_ECU",
            data_id=0x0100,
            profile_config=profile_cfg
        )
        self.assertEqual(protection.data_id, 0x0100)
        self.assertEqual(protection.window_size, 3)


class TestE2EConfig(unittest.TestCase):
    """Test E2E configuration manager."""
    
    def setUp(self):
        """Set up test fixtures."""
        self.config = E2EConfig()
    
    def test_add_profile(self):
        """Test adding E2E profile."""
        profile = E2EProfileConfig(
            profile=E2EProfile.PROFILE_5,
            data_id=0x0100,
            data_length=16
        )
        self.config.add_profile(profile)
        self.assertIn(E2EProfile.PROFILE_5, self.config.profiles)
    
    def test_add_data_id_mapping(self):
        """Test adding Data ID mapping."""
        mapping = DataIDMapping(
            data_id=0x0100,
            name="VehicleSpeed",
            source_component="Sensor_ECU",
            target_component="Powertrain_ECU",
            data_type="uint32",
            signal_name="Speed"
        )
        self.config.add_data_id_mapping(mapping)
        self.assertEqual(len(self.config.data_id_mappings), 1)
    
    def test_remove_data_id_mapping(self):
        """Test removing Data ID mapping."""
        mapping = DataIDMapping(
            data_id=0x0100, name="VehicleSpeed",
            source_component="A", target_component="B",
            data_type="uint32", signal_name="Speed"
        )
        self.config.add_data_id_mapping(mapping)
        result = self.config.remove_data_id_mapping(0x0100)
        self.assertTrue(result)
        self.assertEqual(len(self.config.data_id_mappings), 0)
        
        # Test removing non-existent
        result = self.config.remove_data_id_mapping(0xFFFF)
        self.assertFalse(result)
    
    def test_default_profiles(self):
        """Test getting default profiles."""
        defaults = self.config.get_default_profiles()
        self.assertEqual(len(defaults), 6)
        self.assertIn(E2EProfile.PROFILE_1, defaults)
        self.assertIn(E2EProfile.PROFILE_7, defaults)
    
    def test_serialization(self):
        """Test full configuration serialization."""
        # Setup config
        profile = E2EProfileConfig(
            profile=E2EProfile.PROFILE_5,
            data_id=0x0100,
            data_length=16
        )
        self.config.add_profile(profile)
        
        mapping = DataIDMapping(
            data_id=0x0100, name="VehicleSpeed",
            source_component="A", target_component="B",
            data_type="uint32", signal_name="Speed"
        )
        self.config.add_data_id_mapping(mapping)
        
        # Serialize
        data = self.config.to_dict()
        
        # Deserialize
        config2 = E2EConfig()
        config2.from_dict(data)
        
        self.assertEqual(len(config2.profiles), 1)
        self.assertEqual(len(config2.data_id_mappings), 1)
    
    def test_file_io(self):
        """Test saving and loading from file."""
        # Setup config
        profile = E2EProfileConfig(
            profile=E2EProfile.PROFILE_5,
            data_id=0x0100,
            data_length=16
        )
        self.config.add_profile(profile)
        
        # Save to temp file
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            temp_path = f.name
        
        try:
            self.config.save_to_file(temp_path)
            
            # Load from file
            config2 = E2EConfig()
            config2.load_from_file(temp_path)
            
            self.assertEqual(len(config2.profiles), 1)
            self.assertIn(E2EProfile.PROFILE_5, config2.profiles)
        finally:
            Path(temp_path).unlink()
    
    def test_validation(self):
        """Test configuration validation."""
        # Empty config should be valid (no requirements)
        errors = self.config.validate()
        self.assertEqual(len(errors), 0)
    
    def test_validation_duplicate_data_ids(self):
        """Test validation catches duplicate Data IDs."""
        self.config.add_data_id_mapping(DataIDMapping(
            data_id=0x0100, name="Speed",
            source_component="A", target_component="B",
            data_type="uint32", signal_name="Speed"
        ))
        self.config.add_data_id_mapping(DataIDMapping(
            data_id=0x0100, name="Torque",
            source_component="A", target_component="B",
            data_type="uint32", signal_name="Torque"
        ))
        
        errors = self.config.validate()
        self.assertIn("Duplicate Data IDs found in mappings", errors)
    
    def test_validation_undefined_profile(self):
        """Test validation catches undefined profiles."""
        self.config.add_data_id_mapping(DataIDMapping(
            data_id=0x0100, name="Speed",
            source_component="A", target_component="B",
            data_type="uint32", signal_name="Speed",
            profile=E2EProfile.PROFILE_5
        ))
        
        errors = self.config.validate()
        self.assertIn("Data ID 0x0100 references undefined profile PROFILE_5", errors)


class TestGetRecommendedProfile(unittest.TestCase):
    """Test recommended profile selection."""
    
    def test_asil_d_small_data(self):
        """Test ASIL-D with small data."""
        profile = get_recommended_profile("ASIL_D", 16)
        self.assertEqual(profile, E2EProfile.PROFILE_5)
    
    def test_asil_d_large_data(self):
        """Test ASIL-D with large data."""
        profile = get_recommended_profile("ASIL_D", 32)
        self.assertEqual(profile, E2EProfile.PROFILE_7)
    
    def test_qm_small_data(self):
        """Test QM with small data."""
        profile = get_recommended_profile("QM", 8)
        self.assertEqual(profile, E2EProfile.PROFILE_1)
    
    def test_asil_b_medium_data(self):
        """Test ASIL-B with medium data."""
        profile = get_recommended_profile("ASIL_B", 16)
        self.assertEqual(profile, E2EProfile.PROFILE_5)


if __name__ == "__main__":
    unittest.main()
