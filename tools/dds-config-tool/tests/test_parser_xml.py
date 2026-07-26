"""Tests for DDS XML config parser"""

import sys
import os
import pytest
import tempfile
import json
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from dds_config_tool.parser import (
    DDSConfigParser, DDSConfiguration, DomainParticipantConfig,
    TopicConfig, TopicQos, ReliabilityQos,
)


class TestDDSXmlParser:
    """Test XML config parsing"""

    def test_parse_simple_xml(self):
        """Test parsing simple XML config"""
        xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<dds_config name="TestXMLConfig" version="2.0.0" description="A test config">
    <domain_participant name="Domain1" domain_id="0">
        <topics>
            <topic name="SensorData" type_name="Sensor::Type"/>
        </topics>
    </domain_participant>
</dds_config>"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.xml', delete=False) as f:
            f.write(xml_content)
            temp_path = f.name
        try:
            parser = DDSConfigParser()
            config = parser.parse(temp_path)
            assert isinstance(config, DDSConfiguration)
            assert config.name == "TestXMLConfig"
            assert config.version == "2.0.0"
            assert config.description == "A test config"
            assert len(config.domain_participants) == 1
            assert config.domain_participants[0].name == "Domain1"
            assert len(config.domain_participants[0].topics) == 1
            assert config.domain_participants[0].topics[0].name == "SensorData"
        finally:
            Path(temp_path).unlink()

    def test_parse_xml_with_qos(self):
        """Test parsing XML with QoS configuration"""
        xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<dds_config name="QoSTest">
    <domain_participant name="Domain1" domain_id="0">
        <qos>
            <reliability kind="RELIABLE" max_blocking_time_sec="0" max_blocking_time_nsec="100000000"/>
            <durability kind="TRANSIENT_LOCAL"/>
            <history kind="KEEP_LAST" depth="10"/>
            <resource_limits max_samples="100" max_instances="10" max_samples_per_instance="10"/>
        </qos>
        <topics>
            <topic name="Topic1" type_name="Test::Type">
                <qos>
                    <reliability kind="BEST_EFFORT"/>
                    <history kind="KEEP_LAST" depth="1"/>
                </qos>
            </topic>
        </topics>
    </domain_participant>
</dds_config>"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.xml', delete=False) as f:
            f.write(xml_content)
            temp_path = f.name
        try:
            parser = DDSConfigParser()
            config = parser.parse(temp_path)
            dp = config.domain_participants[0]
            # Check domain-level QoS
            assert dp.qos.reliability.kind == "RELIABLE"
            assert dp.qos.durability.kind == "TRANSIENT_LOCAL"
            assert dp.qos.history.kind == "KEEP_LAST"
            assert dp.qos.history.depth == 10
            assert dp.qos.resource_limits.max_samples == 100
            # Check topic-level QoS
            topic = dp.topics[0]
            assert topic.qos.reliability.kind == "BEST_EFFORT"
            assert topic.qos.history.kind == "KEEP_LAST"
            assert topic.qos.history.depth == 1
        finally:
            Path(temp_path).unlink()

    def test_parse_xml_multiple_domains(self):
        """Test parsing XML with multiple domain participants"""
        xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<dds_config name="MultiDomain">
    <domain_participant name="ECU1" domain_id="0">
        <topics>
            <topic name="TopicA" type_name="TypeA"/>
        </topics>
    </domain_participant>
    <domain_participant name="ECU2" domain_id="1">
        <topics>
            <topic name="TopicB" type_name="TypeB"/>
        </topics>
    </domain_participant>
</dds_config>"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.xml', delete=False) as f:
            f.write(xml_content)
            temp_path = f.name
        try:
            parser = DDSConfigParser()
            config = parser.parse(temp_path)
            assert len(config.domain_participants) == 2
            assert config.domain_participants[0].name == "ECU1"
            assert config.domain_participants[1].name == "ECU2"
        finally:
            Path(temp_path).unlink()

    def test_parse_xml_minimal(self):
        """Test parsing minimal XML config"""
        xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<dds_config name="Minimal"/>"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.xml', delete=False) as f:
            f.write(xml_content)
            temp_path = f.name
        try:
            parser = DDSConfigParser()
            config = parser.parse(temp_path)
            assert config.name == "Minimal"
            assert len(config.domain_participants) == 0
        finally:
            Path(temp_path).unlink()

    def test_parse_invalid_xml(self):
        """Test parsing invalid XML"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.xml', delete=False) as f:
            f.write("<invalid>unclosed")
            temp_path = f.name
        try:
            parser = DDSConfigParser()
            with pytest.raises(ValueError):
                parser.parse(temp_path)
        finally:
            Path(temp_path).unlink()

    def test_parse_xml_full_qos_all_types(self):
        """Test parsing XML with all QoS types"""
        xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<dds_config name="FullQoS">
    <domain_participant name="D1" domain_id="0">
        <qos>
            <reliability kind="BEST_EFFORT"/>
            <durability kind="VOLATILE"/>
            <deadline period_sec="0" period_nsec="0"/>
            <latency_budget duration_sec="0" duration_nsec="1000000"/>
            <liveliness kind="AUTOMATIC" lease_duration_sec="10" lease_duration_nsec="0"/>
            <history kind="KEEP_ALL"/>
            <resource_limits max_samples="0" max_instances="0" max_samples_per_instance="0"/>
            <lifespan duration_sec="0" duration_nsec="0"/>
        </qos>
        <topics>
            <topic name="T1" type_name="Ty"/>
        </topics>
    </domain_participant>
</dds_config>"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.xml', delete=False) as f:
            f.write(xml_content)
            temp_path = f.name
        try:
            parser = DDSConfigParser()
            config = parser.parse(temp_path)
            dp = config.domain_participants[0]
            assert dp.qos.latency_budget.duration_nsec == 1000000
            assert dp.qos.liveliness.lease_duration_sec == 10
            assert dp.qos.history.kind == "KEEP_ALL"
            assert dp.qos.lifespan.duration_sec == 0
        finally:
            Path(temp_path).unlink()

    def test_parse_xml_no_topics(self):
        """Test parsing XML with domain but no topics"""
        xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<dds_config name="NoTopics">
    <domain_participant name="D1" domain_id="0">
    </domain_participant>
</dds_config>"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.xml', delete=False) as f:
            f.write(xml_content)
            temp_path = f.name
        try:
            parser = DDSConfigParser()
            config = parser.parse(temp_path)
            assert len(config.domain_participants[0].topics) == 0
        finally:
            Path(temp_path).unlink()

    def test_parse_xml_default_qos(self):
        """Test parsing XML with default QoS"""
        xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<dds_config name="DefaultQoSTest">
    <default_qos>
        <reliability kind="RELIABLE"/>
        <history kind="KEEP_LAST" depth="5"/>
    </default_qos>
    <domain_participant name="D1" domain_id="0">
        <topics>
            <topic name="T1" type_name="Ty"/>
        </topics>
    </domain_participant>
</dds_config>"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.xml', delete=False) as f:
            f.write(xml_content)
            temp_path = f.name
        try:
            parser = DDSConfigParser()
            config = parser.parse(temp_path)
            assert config.default_qos.reliability.kind == "RELIABLE"
            assert config.default_qos.history.depth == 5
        finally:
            Path(temp_path).unlink()


class TestDDSJsonParserExtended:
    """Extended JSON parser tests"""

    def test_parse_json_with_full_qos(self):
        """Test parsing JSON with full QoS"""
        data = {
            "name": "FullConfig",
            "domain_participants": [{
                "name": "D1",
                "domain_id": 0,
                "qos": {
                    "reliability": {"kind": "RELIABLE"},
                    "durability": {"kind": "TRANSIENT"},
                    "deadline": {"period_sec": 1, "period_nsec": 0},
                    "liveliness": {"kind": "MANUAL_BY_TOPIC", "lease_duration_sec": 5},
                },
                "topics": [{
                    "name": "T1",
                    "type_name": "Type1",
                    "qos": {
                        "history": {"kind": "KEEP_ALL"},
                    }
                }]
            }]
        }
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(data, f)
            temp_path = f.name
        try:
            parser = DDSConfigParser()
            config = parser.parse(temp_path)
            dp = config.domain_participants[0]
            assert dp.qos.reliability.kind == "RELIABLE"
            assert dp.qos.durability.kind == "TRANSIENT"
            assert dp.qos.liveliness.kind == "MANUAL_BY_TOPIC"
            assert dp.topics[0].qos.history.kind == "KEEP_ALL"
        finally:
            Path(temp_path).unlink()

    def test_to_dict_roundtrip(self):
        """Test to_dict roundtrip"""
        data = {
            "name": "Roundtrip",
            "domain_participants": [{
                "name": "D1", "domain_id": 0,
                "topics": [{"name": "T1", "type_name": "Ty"}]
            }]
        }
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(data, f)
            temp_path = f.name
        try:
            parser = DDSConfigParser()
            config = parser.parse(temp_path)
            result = parser.to_dict()
            assert result["name"] == "Roundtrip"
            assert len(result["domain_participants"]) == 1
            dp = result["domain_participants"][0]
            assert dp["name"] == "D1"
            assert dp["topics"][0]["name"] == "T1"
        finally:
            Path(temp_path).unlink()

    def test_to_dict_no_config(self):
        """Test to_dict with no parsed config"""
        parser = DDSConfigParser()
        result = parser.to_dict()
        assert result == {}

    def test_get_errors_empty(self):
        """Test get_errors returns empty list initially"""
        parser = DDSConfigParser()
        errors = parser.get_errors()
        assert errors == []

