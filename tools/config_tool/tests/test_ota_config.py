"""
Unit tests for ota_config module.

Author: Hermes Agent
Version: 1.0.0
"""

import unittest
import json
import tempfile
from pathlib import Path

import sys
sys.path.insert(0, str(Path(__file__).parent.parent))

from ota_config import (
    OTAConfig, PartitionConfig, ABPartitionLayout, OTACampaign,
    DownloadParams, SecurityVerification, ECUUpdate, PreCondition,
    InstallationParams, OTAState, PartitionType
)


class TestPartitionConfig(unittest.TestCase):
    """Test partition configuration."""

    def test_partition_creation(self):
        """Test creating partition config."""
        part = PartitionConfig(
            name="app_a",
            partition_type=PartitionType.APP_BANK_A,
            start_address=0x20000,
            size_bytes=0x40000,
            is_active=True,
            is_bootable=True
        )
        self.assertEqual(part.name, "app_a")
        self.assertEqual(part.start_address, 0x20000)
        self.assertTrue(part.is_active)

    def test_serialization(self):
        """Test partition serialization."""
        part = PartitionConfig(
            name="app_a",
            partition_type=PartitionType.APP_BANK_A,
            start_address=0x20000,
            size_bytes=0x40000
        )
        data = part.to_dict()
        self.assertEqual(data["name"], "app_a")
        self.assertEqual(data["start_address"], "0x00020000")

        part2 = PartitionConfig.from_dict(data)
        self.assertEqual(part.name, part2.name)
        self.assertEqual(part.start_address, part2.start_address)


class TestABPartitionLayout(unittest.TestCase):
    """Test A/B partition layout."""

    def setUp(self):
        """Set up test fixtures."""
        self.layout = ABPartitionLayout(name="test_layout")

    def test_layout_creation(self):
        """Test creating layout."""
        self.assertEqual(self.layout.name, "test_layout")
        self.assertEqual(self.layout.active_bank, "A")

    def test_get_active_partitions(self):
        """Test getting active partitions."""
        part_a = PartitionConfig(
            name="app_a", partition_type=PartitionType.APP_BANK_A,
            start_address=0x20000, size_bytes=0x40000, is_active=True
        )
        part_shared = PartitionConfig(
            name="bootloader", partition_type=PartitionType.BOOTLOADER,
            start_address=0x00000, size_bytes=0x20000, is_active=True
        )
        self.layout.bank_a_partitions = [part_a]
        self.layout.shared_partitions = [part_shared]

        active = self.layout.get_active_partitions()
        self.assertEqual(len(active), 2)
        self.assertIn(part_a, active)
        self.assertIn(part_shared, active)

    def test_serialization(self):
        """Test layout serialization."""
        part_a = PartitionConfig(
            name="app_a", partition_type=PartitionType.APP_BANK_A,
            start_address=0x20000, size_bytes=0x40000
        )
        self.layout.bank_a_partitions = [part_a]

        data = self.layout.to_dict()
        self.assertEqual(data["name"], "test_layout")
        self.assertEqual(len(data["bank_a_partitions"]), 1)

        layout2 = ABPartitionLayout.from_dict(data)
        self.assertEqual(self.layout.name, layout2.name)
        self.assertEqual(len(layout2.bank_a_partitions), 1)


class TestDownloadParams(unittest.TestCase):
    """Test download parameters."""

    def test_default_values(self):
        """Test default values."""
        params = DownloadParams()
        self.assertEqual(params.chunk_size_bytes, 4096)
        self.assertEqual(params.max_retries, 3)
        self.assertTrue(params.resume_supported)

    def test_serialization(self):
        """Test serialization."""
        params = DownloadParams(
            chunk_size_bytes=8192,
            max_retries=5,
            compression="zstd"
        )
        data = params.to_dict()
        self.assertEqual(data["chunk_size_bytes"], 8192)
        self.assertEqual(data["compression"], "zstd")

        params2 = DownloadParams.from_dict(data)
        self.assertEqual(params.chunk_size_bytes, params2.chunk_size_bytes)


class TestSecurityVerification(unittest.TestCase):
    """Test security verification settings."""

    def test_default_values(self):
        """Test default values."""
        security = SecurityVerification()
        self.assertTrue(security.signature_required)
        self.assertEqual(security.signature_algorithm, "ecdsa-p256")
        self.assertTrue(security.verify_version_rollback)

    def test_serialization(self):
        """Test serialization."""
        security = SecurityVerification(
            signature_required=True,
            hash_algorithm="sha384",
            root_cert_hash="abc123"
        )
        data = security.to_dict()
        self.assertEqual(data["hash_algorithm"], "sha384")
        self.assertEqual(data["root_cert_hash"], "abc123")


class TestECUUpdate(unittest.TestCase):
    """Test ECU update configuration."""

    def test_ecu_update_creation(self):
        """Test creating ECU update."""
        update = ECUUpdate(
            ecu_id=0x0101,
            name="Test_ECU",
            hardware_version="1.0.0",
            firmware_version_from="1.0.0",
            firmware_version_to="1.1.0",
            package_file="test.epkg",
            package_size=1024,
            package_hash="sha256:abc"
        )
        self.assertEqual(update.ecu_id, 0x0101)
        self.assertEqual(update.firmware_version_to, "1.1.0")

    def test_serialization(self):
        """Test serialization."""
        update = ECUUpdate(
            ecu_id=0x0101, name="Test_ECU",
            hardware_version="1.0.0",
            firmware_version_from="1.0.0",
            firmware_version_to="1.1.0",
            package_file="test.epkg",
            package_size=1024,
            package_hash="sha256:abc"
        )
        data = update.to_dict()
        self.assertEqual(data["ecu_id"], "0x0101")
        self.assertEqual(data["name"], "Test_ECU")

        update2 = ECUUpdate.from_dict(data)
        self.assertEqual(update.ecu_id, update2.ecu_id)
        self.assertEqual(update.name, update2.name)


class TestOTACampaign(unittest.TestCase):
    """Test OTA campaign configuration."""

    def test_campaign_creation(self):
        """Test creating campaign."""
        campaign = OTACampaign(
            campaign_id="CAMP-001",
            name="Test Campaign",
            description="Test update campaign",
            priority="high"
        )
        self.assertEqual(campaign.campaign_id, "CAMP-001")
        self.assertEqual(campaign.priority, "high")
        self.assertEqual(campaign.rollout_strategy, "wave")

    def test_serialization(self):
        """Test serialization."""
        campaign = OTACampaign(
            campaign_id="CAMP-001",
            name="Test Campaign",
            description="Test",
            priority="normal"
        )
        data = campaign.to_dict()
        self.assertEqual(data["campaign_id"], "CAMP-001")

        campaign2 = OTACampaign.from_dict(data)
        self.assertEqual(campaign.campaign_id, campaign2.campaign_id)


class TestOTAConfig(unittest.TestCase):
    """Test OTA configuration manager."""

    def setUp(self):
        """Set up test fixtures."""
        self.config = OTAConfig()

    def test_add_partition_layout(self):
        """Test adding partition layout."""
        layout = ABPartitionLayout(name="test")
        self.config.add_partition_layout(layout)
        self.assertEqual(len(self.config.partition_layouts), 1)

    def test_get_partition_layout(self):
        """Test getting partition layout."""
        layout = ABPartitionLayout(name="test_layout")
        self.config.add_partition_layout(layout)

        found = self.config.get_partition_layout("test_layout")
        self.assertIsNotNone(found)
        self.assertEqual(found.name, "test_layout")

        not_found = self.config.get_partition_layout("nonexistent")
        self.assertIsNone(not_found)

    def test_add_campaign(self):
        """Test adding campaign."""
        campaign = OTACampaign(
            campaign_id="CAMP-001",
            name="Test",
            description="Test campaign"
        )
        self.config.add_campaign(campaign)
        self.assertEqual(len(self.config.campaigns), 1)

    def test_remove_campaign(self):
        """Test removing campaign."""
        campaign = OTACampaign(campaign_id="CAMP-001", name="Test", description="Test")
        self.config.add_campaign(campaign)

        result = self.config.remove_campaign("CAMP-001")
        self.assertTrue(result)
        self.assertEqual(len(self.config.campaigns), 0)

        result = self.config.remove_campaign("NONEXISTENT")
        self.assertFalse(result)

    def test_get_campaign(self):
        """Test getting campaign."""
        campaign = OTACampaign(campaign_id="CAMP-001", name="Test", description="Test")
        self.config.add_campaign(campaign)

        found = self.config.get_campaign("CAMP-001")
        self.assertIsNotNone(found)
        self.assertEqual(found.campaign_id, "CAMP-001")

    def test_default_partition_layout(self):
        """Test getting default partition layout."""
        layout = self.config.get_default_partition_layout(8)
        self.assertEqual(layout.name, "default")
        self.assertGreater(len(layout.bank_a_partitions), 0)
        self.assertGreater(len(layout.shared_partitions), 0)

    def test_serialization(self):
        """Test full configuration serialization."""
        # Setup config
        layout = ABPartitionLayout(name="test")
        self.config.add_partition_layout(layout)

        campaign = OTACampaign(campaign_id="CAMP-001", name="Test", description="Test")
        self.config.add_campaign(campaign)

        # Serialize
        data = self.config.to_dict()

        # Deserialize
        config2 = OTAConfig()
        config2.from_dict(data)

        self.assertEqual(len(config2.partition_layouts), 1)
        self.assertEqual(len(config2.campaigns), 1)

    def test_file_io(self):
        """Test saving and loading from file."""
        # Setup config
        layout = ABPartitionLayout(name="test")
        self.config.add_partition_layout(layout)

        # Save to temp file
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            temp_path = f.name

        try:
            self.config.save_to_file(temp_path)

            # Load from file
            config2 = OTAConfig()
            config2.load_from_file(temp_path)

            self.assertEqual(len(config2.partition_layouts), 1)
        finally:
            Path(temp_path).unlink()

    def test_validation_no_layouts(self):
        """Test validation catches missing layouts."""
        errors = self.config.validate()
        self.assertIn("No partition layouts configured", errors)

    def test_validation_duplicate_campaign_ids(self):
        """Test validation catches duplicate campaign IDs."""
        self.config.add_partition_layout(ABPartitionLayout(name="test"))
        self.config.add_campaign(OTACampaign(campaign_id="CAMP-001", name="Test", description="Test"))
        self.config.add_campaign(OTACampaign(campaign_id="CAMP-001", name="Test2", description="Test2"))

        errors = self.config.validate()
        self.assertIn("Duplicate campaign IDs found", errors)

    def test_validation_duplicate_ecu_ids(self):
        """Test validation catches duplicate ECU IDs in campaign."""
        self.config.add_partition_layout(ABPartitionLayout(name="test"))

        campaign = OTACampaign(campaign_id="CAMP-001", name="Test", description="Test")
        campaign.ecu_updates = [
            ECUUpdate(ecu_id=0x0101, name="ECU1", hardware_version="1.0",
                     firmware_version_from="1.0", firmware_version_to="2.0",
                     package_file="a.epkg", package_size=100, package_hash="sha256:a"),
            ECUUpdate(ecu_id=0x0101, name="ECU2", hardware_version="1.0",
                     firmware_version_from="1.0", firmware_version_to="2.0",
                     package_file="b.epkg", package_size=100, package_hash="sha256:b")
        ]
        self.config.add_campaign(campaign)

        errors = self.config.validate()
        self.assertIn("Campaign CAMP-001 has duplicate ECU IDs", errors)

    def test_validation_overlapping_partitions(self):
        """Test validation catches overlapping partitions."""
        layout = ABPartitionLayout(name="test")
        layout.bank_a_partitions = [
            PartitionConfig(name="part1", partition_type=PartitionType.APP_BANK_A,
                          start_address=0x20000, size_bytes=0x40000),
            PartitionConfig(name="part2", partition_type=PartitionType.APP_BANK_A,
                          start_address=0x30000, size_bytes=0x40000)  # Overlaps with part1
        ]
        self.config.add_partition_layout(layout)

        errors = self.config.validate()
        self.assertTrue(any("overlap" in e.lower() for e in errors))


if __name__ == "__main__":
    unittest.main()
