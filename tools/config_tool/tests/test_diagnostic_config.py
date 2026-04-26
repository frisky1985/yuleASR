"""
Unit tests for diagnostic_config module.

Author: Hermes Agent
Version: 1.0.0
"""

import unittest
import json
import tempfile
from pathlib import Path

import sys
sys.path.insert(0, str(Path(__file__).parent.parent))

from diagnostic_config import (
    DiagnosticConfig, DiagServiceConfig, DIDDefinition, DTCConfig,
    SecurityPolicy, DCMParams, DoIPConfig, DiagSessionType, DiagPriority,
    DiagServiceID, create_common_did, COMMON_DIDS
)


class TestDCMParams(unittest.TestCase):
    """Test DCM parameters."""
    
    def test_default_creation(self):
        """Test creating DCMParams with default values."""
        params = DCMParams(
            ecu_id=0x7E0,
            logical_address=0x0001,
            functional_address=0x7DF
        )
        self.assertEqual(params.ecu_id, 0x7E0)
        self.assertEqual(params.max_response_length, 4095)
        self.assertEqual(params.p2_timeout_ms, 50)
        self.assertTrue(params.tester_present_required)
    
    def test_to_dict(self):
        """Test serialization to dictionary."""
        params = DCMParams(
            ecu_id=0x7E0,
            logical_address=0x0001,
            functional_address=0x7DF
        )
        data = params.to_dict()
        self.assertEqual(data["ecu_id"], "0x07E0")
        self.assertEqual(data["logical_address"], "0x0001")
    
    def test_from_dict(self):
        """Test deserialization from dictionary."""
        data = {
            "ecu_id": "0x07E0",
            "logical_address": "0x0001",
            "functional_address": "0x07DF",
            "max_response_length": 2048,
            "p2_timeout_ms": 25
        }
        params = DCMParams.from_dict(data)
        self.assertEqual(params.ecu_id, 0x7E0)
        self.assertEqual(params.max_response_length, 2048)
        self.assertEqual(params.p2_timeout_ms, 25)


class TestDiagServiceConfig(unittest.TestCase):
    """Test diagnostic service configuration."""
    
    def test_service_creation(self):
        """Test creating a diagnostic service."""
        svc = DiagServiceConfig(
            service_id=DiagServiceID.READ_DATA_BY_ID.value,
            name="ReadDataByIdentifier",
            priority=DiagPriority.P1,
            supported_sessions=[DiagSessionType.DEFAULT, DiagSessionType.EXTENDED]
        )
        self.assertEqual(svc.service_id, 0x22)
        self.assertEqual(svc.name, "ReadDataByIdentifier")
        self.assertTrue(svc.enabled)
    
    def test_serialization(self):
        """Test serialization and deserialization."""
        svc = DiagServiceConfig(
            service_id=0x22,
            name="ReadDataByIdentifier",
            priority=DiagPriority.P1,
            supported_sessions=[DiagSessionType.DEFAULT]
        )
        data = svc.to_dict()
        svc2 = DiagServiceConfig.from_dict(data)
        self.assertEqual(svc.service_id, svc2.service_id)
        self.assertEqual(svc.name, svc2.name)


class TestDIDDefinition(unittest.TestCase):
    """Test DID definitions."""
    
    def test_did_creation(self):
        """Test creating a DID definition."""
        did = DIDDefinition(
            did=0xF190,
            name="VIN",
            description="Vehicle Identification Number",
            data_length=17,
            data_type="string"
        )
        self.assertEqual(did.did, 0xF190)
        self.assertEqual(did.read_security_level, 0)
        self.assertEqual(did.write_security_level, 1)
    
    def test_common_dids(self):
        """Test common DID creation."""
        did = create_common_did(0xF190)
        self.assertIsNotNone(did)
        self.assertEqual(did.name, "VIN")
        
        # Test non-existent DID
        did2 = create_common_did(0x0000)
        self.assertIsNone(did2)


class TestDiagnosticConfig(unittest.TestCase):
    """Test diagnostic configuration manager."""
    
    def setUp(self):
        """Set up test fixtures."""
        self.config = DiagnosticConfig()
    
    def test_add_service(self):
        """Test adding diagnostic services."""
        svc = DiagServiceConfig(
            service_id=0x10,
            name="DiagnosticSessionControl",
            priority=DiagPriority.P0,
            supported_sessions=[DiagSessionType.DEFAULT]
        )
        self.config.add_service(svc)
        self.assertEqual(len(self.config.services), 1)
        self.assertEqual(self.config.services[0].service_id, 0x10)
    
    def test_remove_service(self):
        """Test removing diagnostic services."""
        svc = DiagServiceConfig(
            service_id=0x10,
            name="DiagnosticSessionControl",
            priority=DiagPriority.P0,
            supported_sessions=[]
        )
        self.config.add_service(svc)
        result = self.config.remove_service(0x10)
        self.assertTrue(result)
        self.assertEqual(len(self.config.services), 0)
        
        # Test removing non-existent service
        result = self.config.remove_service(0xFF)
        self.assertFalse(result)
    
    def test_add_did(self):
        """Test adding DIDs."""
        did = DIDDefinition(
            did=0xF190,
            name="VIN",
            description="Vehicle Identification Number",
            data_length=17,
            data_type="string"
        )
        self.config.add_did(did)
        self.assertEqual(len(self.config.dids), 1)
    
    def test_default_services(self):
        """Test getting default services."""
        defaults = self.config.get_default_services()
        self.assertGreater(len(defaults), 0)
        
        service_ids = [s.service_id for s in defaults]
        self.assertIn(DiagServiceID.DIAGNOSTIC_SESSION_CONTROL.value, service_ids)
        self.assertIn(DiagServiceID.READ_DATA_BY_ID.value, service_ids)
    
    def test_serialization(self):
        """Test full configuration serialization."""
        # Setup config
        self.config.dcm_params = DCMParams(0x7E0, 0x0001, 0x7DF)
        self.config.add_service(DiagServiceConfig(
            service_id=0x10, name="SessionControl", priority=DiagPriority.P0,
            supported_sessions=[]
        ))
        
        # Serialize
        data = self.config.to_dict()
        
        # Deserialize
        config2 = DiagnosticConfig()
        config2.from_dict(data)
        
        self.assertEqual(config2.dcm_params.ecu_id, 0x7E0)
        self.assertEqual(len(config2.services), 1)
    
    def test_file_io(self):
        """Test saving and loading from file."""
        # Setup config
        self.config.dcm_params = DCMParams(0x7E0, 0x0001, 0x7DF)
        self.config.add_service(DiagServiceConfig(
            service_id=0x10, name="SessionControl", priority=DiagPriority.P0,
            supported_sessions=[]
        ))
        
        # Save to temp file
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            temp_path = f.name
        
        try:
            self.config.save_to_file(temp_path)
            
            # Load from file
            config2 = DiagnosticConfig()
            config2.load_from_file(temp_path)
            
            self.assertEqual(config2.dcm_params.ecu_id, 0x7E0)
            self.assertEqual(len(config2.services), 1)
        finally:
            Path(temp_path).unlink()
    
    def test_validation(self):
        """Test configuration validation."""
        # Empty config should have errors
        errors = self.config.validate()
        self.assertIn("DCM parameters not configured", errors)
        self.assertIn("No diagnostic services configured", errors)
        
        # Valid config
        self.config.dcm_params = DCMParams(0x7E0, 0x0001, 0x7DF)
        self.config.add_service(DiagServiceConfig(
            service_id=0x10, name="SessionControl", priority=DiagPriority.P0,
            supported_sessions=[]
        ))
        errors = self.config.validate()
        self.assertEqual(len(errors), 0)
    
    def test_validation_duplicate_service_ids(self):
        """Test validation catches duplicate service IDs."""
        self.config.dcm_params = DCMParams(0x7E0, 0x0001, 0x7DF)
        self.config.add_service(DiagServiceConfig(
            service_id=0x10, name="Service1", priority=DiagPriority.P0,
            supported_sessions=[]
        ))
        self.config.add_service(DiagServiceConfig(
            service_id=0x10, name="Service2", priority=DiagPriority.P1,
            supported_sessions=[]
        ))
        
        errors = self.config.validate()
        self.assertIn("Duplicate service IDs found", errors)


class TestDoIPConfig(unittest.TestCase):
    """Test DoIP configuration."""
    
    def test_default_values(self):
        """Test default DoIP configuration."""
        config = DoIPConfig()
        self.assertTrue(config.enabled)
        self.assertEqual(config.local_port, 13400)
        self.assertEqual(config.max_connections, 8)
    
    def test_serialization(self):
        """Test serialization."""
        config = DoIPConfig(
            enabled=True,
            local_port=13401,
            vin="WBA12345678901234"
        )
        data = config.to_dict()
        self.assertEqual(data["local_port"], 13401)
        self.assertEqual(data["vin"], "WBA12345678901234")


class TestSecurityPolicy(unittest.TestCase):
    """Test security policy configuration."""
    
    def test_policy_creation(self):
        """Test creating security policy."""
        policy = SecurityPolicy(
            level=1,
            name="Programming",
            max_attempts=3,
            delay_timer_ms=10000
        )
        self.assertEqual(policy.level, 1)
        self.assertEqual(policy.max_attempts, 3)
    
    def test_serialization(self):
        """Test serialization."""
        policy = SecurityPolicy(level=1, name="Level1")
        data = policy.to_dict()
        policy2 = SecurityPolicy.from_dict(data)
        self.assertEqual(policy.level, policy2.level)
        self.assertEqual(policy.name, policy2.name)


if __name__ == "__main__":
    unittest.main()