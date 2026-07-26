"""Comprehensive tests for DDS config validator"""

import sys
import os
import pytest
import tempfile
import json
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from dds_config_tool.parser import (
    DDSConfigParser, DDSConfiguration, DomainParticipantConfig,
    TopicConfig, TopicQos, ReliabilityQos, DurabilityQos,
    DeadlineQos, LatencyBudgetQos, LivelinessQos, HistoryQos,
    ResourceLimitsQos, LifespanQos,
)
from dds_config_tool.validator import DDSConfigValidator, ValidationError


class TestValidatorBasic:
    """Test basic validator functionality"""

    def test_validate_valid_config(self):
        """Test validating a valid config"""
        config = DDSConfiguration(
            name="TestConfig",
            version="1.0.0",
        )
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is True

    def test_validate_empty_name(self):
        """Test validating config with empty name"""
        config = DDSConfiguration(name="")
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is False
        # Should have an error about empty name
        names = [i.path for i in issues]
        assert "name" in names

    def test_validate_no_version_warning(self):
        """Test warning for no version"""
        config = DDSConfiguration(name="Test", version="")
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is True  # Warning, not error
        paths = [i.path for i in issues]
        assert "version" in paths

    def test_validate_no_domain_participants_warning(self):
        """Test warning for no domain participants"""
        config = DDSConfiguration(name="Test")
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is True
        paths = [i.path for i in issues]
        assert "domain_participants" in paths


class TestValidatorDomainParticipants:
    """Test domain participant validation"""

    def test_domain_participant_empty_name(self):
        """Test error for empty domain participant name"""
        config = DDSConfiguration(
            name="Test",
            domain_participants=[
                DomainParticipantConfig(name="", domain_id=0)
            ]
        )
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is False
        assert any("domain_participants[0].name" in i.path for i in issues)

    def test_domain_id_out_of_range(self):
        """Test error for domain ID out of range"""
        config = DDSConfiguration(
            name="Test",
            domain_participants=[
                DomainParticipantConfig(name="Domain1", domain_id=300)
            ]
        )
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is False
        assert any("domain_id" in i.path for i in issues)

    def test_negative_domain_id(self):
        """Test error for negative domain ID"""
        config = DDSConfiguration(
            name="Test",
            domain_participants=[
                DomainParticipantConfig(name="Domain1", domain_id=-1)
            ]
        )
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is False
        assert any("domain_id" in i.path for i in issues)


class TestValidatorTopics:
    """Test topic validation"""

    def test_topic_empty_name(self):
        """Test error for empty topic name"""
        dp = DomainParticipantConfig(
            name="Domain1",
            topics=[TopicConfig(name="", type_name="Test::Type")]
        )
        config = DDSConfiguration(name="Test", domain_participants=[dp])
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is False
        assert any("topics[0].name" in i.path for i in issues)

    def test_topic_no_type_warning(self):
        """Test warning for no type_name"""
        dp = DomainParticipantConfig(
            name="Domain1",
            topics=[TopicConfig(name="Topic1", type_name="")]
        )
        config = DDSConfiguration(name="Test", domain_participants=[dp])
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is True  # Warning, not error
        paths = [i.path for i in issues]
        assert any("type_name" in p for p in paths)

    def test_invalid_identifier_warning(self):
        """Test warning for non-C-identifier topic name"""
        dp = DomainParticipantConfig(
            name="Domain1",
            topics=[TopicConfig(name="123-bad-name!", type_name="Type")]
        )
        config = DDSConfiguration(name="Test", domain_participants=[dp])
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is True
        paths = [i.path for i in issues]
        assert any("name" in p for p in paths)


class TestValidatorQoS:
    """Test QoS validation"""

    def test_reliability_kind_valid(self):
        """Test that valid reliability kinds pass"""
        for kind in ["BEST_EFFORT", "RELIABLE"]:
            qos = TopicQos(reliability=ReliabilityQos(kind=kind))
            dp = DomainParticipantConfig(
                name="Domain1",
                topics=[TopicConfig(name="Topic1", type_name="T", qos=qos)]
            )
            config = DDSConfiguration(name="Test", domain_participants=[dp])
            validator = DDSConfigValidator()
            is_valid, issues = validator.validate(config)
            assert is_valid is True

    def test_reliability_kind_invalid(self):
        """Test that invalid reliability kinds fail"""
        qos = TopicQos(reliability=ReliabilityQos(kind="UNRELIABLE"))
        dp = DomainParticipantConfig(
            name="Domain1",
            topics=[TopicConfig(name="Topic1", type_name="T", qos=qos)]
        )
        config = DDSConfiguration(name="Test", domain_participants=[dp])
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is False
        assert any("reliability" in i.path for i in issues)

    def test_durability_kind_invalid(self):
        """Test invalid durability kind"""
        qos = TopicQos(durability=DurabilityQos(kind="INVALID"))
        dp = DomainParticipantConfig(name="D1", topics=[TopicConfig(name="T", type_name="Ty", qos=qos)])
        config = DDSConfiguration(name="Test", domain_participants=[dp])
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is False
        assert any("durability" in i.path for i in issues)

    def test_liveliness_kind_invalid(self):
        """Test invalid liveliness kind"""
        qos = TopicQos(liveliness=LivelinessQos(kind="INVALID"))
        dp = DomainParticipantConfig(name="D1", topics=[TopicConfig(name="T", type_name="Ty", qos=qos)])
        config = DDSConfiguration(name="Test", domain_participants=[dp])
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is False
        assert any("liveliness" in i.path for i in issues)

    def test_history_kind_invalid(self):
        """Test invalid history kind"""
        qos = TopicQos(history=HistoryQos(kind="INVALID"))
        dp = DomainParticipantConfig(name="D1", topics=[TopicConfig(name="T", type_name="Ty", qos=qos)])
        config = DDSConfiguration(name="Test", domain_participants=[dp])
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is False
        assert any("history" in i.path for i in issues)

    def test_history_depth_zero(self):
        """Test error for KEEP_LAST with depth 0"""
        qos = TopicQos(history=HistoryQos(kind="KEEP_LAST", depth=0))
        dp = DomainParticipantConfig(name="D1", topics=[TopicConfig(name="T", type_name="Ty", qos=qos)])
        config = DDSConfiguration(name="Test", domain_participants=[dp])
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is False

    def test_resource_limits_warning(self):
        """Test warning for inconsistent resource limits"""
        qos = TopicQos(
            resource_limits=ResourceLimitsQos(max_samples=10, max_samples_per_instance=20)
        )
        dp = DomainParticipantConfig(name="D1", topics=[TopicConfig(name="T", type_name="Ty", qos=qos)])
        config = DDSConfiguration(name="Test", domain_participants=[dp])
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is True  # Warning
        paths = [i.path for i in issues]
        assert any("resource_limits" in p for p in paths)

    def test_keep_all_no_limits_warning(self):
        """Test warning for KEEP_ALL without resource limits"""
        qos = TopicQos(history=HistoryQos(kind="KEEP_ALL"))
        dp = DomainParticipantConfig(name="D1", topics=[TopicConfig(name="T", type_name="Ty", qos=qos)])
        config = DDSConfiguration(name="Test", domain_participants=[dp])
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is True
        paths = [i.path for i in issues]
        assert any("resource_limits" in p for p in paths)

    def test_nsec_range_valid(self):
        """Test valid nanosecond range"""
        qos = TopicQos(
            deadline=DeadlineQos(period_sec=1, period_nsec=500000000)
        )
        dp = DomainParticipantConfig(name="D1", topics=[TopicConfig(name="T", type_name="Ty", qos=qos)])
        config = DDSConfiguration(name="Test", domain_participants=[dp])
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is True

    def test_nsec_range_invalid(self):
        """Test invalid nanosecond range (too large)"""
        qos = TopicQos(
            deadline=DeadlineQos(period_sec=0, period_nsec=2000000000)
        )
        dp = DomainParticipantConfig(name="D1", topics=[TopicConfig(name="T", type_name="Ty", qos=qos)])
        config = DDSConfiguration(name="Test", domain_participants=[dp])
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is False

    def test_duplicate_domain_names(self):
        """Test error for duplicate domain participant names"""
        config = DDSConfiguration(
            name="Test",
            domain_participants=[
                DomainParticipantConfig(name="Domain", domain_id=0),
                DomainParticipantConfig(name="Domain", domain_id=1),
            ]
        )
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is False
        assert any("重复" in i.message for i in issues)

    def test_duplicate_topic_names(self):
        """Test error for duplicate topic names"""
        dp = DomainParticipantConfig(
            name="Domain1",
            topics=[
                TopicConfig(name="Topic1", type_name="Ty"),
                TopicConfig(name="Topic1", type_name="Ty2"),
            ]
        )
        config = DDSConfiguration(name="Test", domain_participants=[dp])
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)
        assert is_valid is False
        assert any("重复" in i.message for i in issues)


class TestValidatorFile:
    """Test file-based validation"""

    def test_validate_file(self):
        """Test validate_file method"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump({"name": "TestConfig", "domain_participants": []}, f)
            temp_path = f.name
        try:
            validator = DDSConfigValidator()
            is_valid, issues = validator.validate_file(temp_path)
            assert is_valid is True
        finally:
            Path(temp_path).unlink()

    def test_validate_file_not_found(self):
        """Test validate_file with non-existent file"""
        validator = DDSConfigValidator()
        is_valid, issues = validator.validate_file("/nonexistent.json")
        assert is_valid is False
        assert any("解析文件失败" in i.message for i in issues)

    def test_validator_is_valid_identifier(self):
        """Test _is_valid_identifier method"""
        validator = DDSConfigValidator()
        assert validator._is_valid_identifier("validName") is True
        assert validator._is_valid_identifier("_validName") is True
        assert validator._is_valid_identifier("123invalid") is False
        assert validator._is_valid_identifier("") is False
        assert validator._is_valid_identifier("name-with-dashes") is False
        assert validator._is_valid_identifier("name.with.dots") is False

