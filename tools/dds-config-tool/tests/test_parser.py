"""
测试DDS配置解析器
"""

import pytest
import json
import tempfile
from pathlib import Path

from dds_config_tool.parser import DDSConfigParser, DDSConfiguration


class TestDDSConfigParser:
    """测试配置解析器"""

    def test_parse_json_config(self):
        """测试解析JSON配置"""
        config_data = {
            "name": "TestConfig",
            "version": "1.0.0",
            "domain_participants": [
                {
                    "name": "TestDomain",
                    "domain_id": 0,
                    "topics": [
                        {
                            "name": "TestTopic",
                            "type_name": "Test::Type"
                        }
                    ]
                }
            ]
        }

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(config_data, f)
            temp_path = f.name

        try:
            parser = DDSConfigParser()
            config = parser.parse(temp_path)

            assert isinstance(config, DDSConfiguration)
            assert config.name == "TestConfig"
            assert config.version == "1.0.0"
            assert len(config.domain_participants) == 1
            assert config.domain_participants[0].name == "TestDomain"
        finally:
            Path(temp_path).unlink()

    def test_parse_invalid_file(self):
        """测试解析无效文件"""
        parser = DDSConfigParser()

        with pytest.raises(FileNotFoundError):
            parser.parse("/nonexistent/file.xml")

    def test_parse_unsupported_format(self):
        """测试不支持的文件格式"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
            f.write("test")
            temp_path = f.name

        try:
            parser = DDSConfigParser()
            with pytest.raises(ValueError):
                parser.parse(temp_path)
        finally:
            Path(temp_path).unlink()


class TestDDSConfigValidation:
    """测试配置验证"""

    def test_validate_valid_config(self):
        """测试验证有效配置"""
        from dds_config_tool.validator import DDSConfigValidator

        config = DDSConfiguration(
            name="TestConfig",
            domain_participants=[]
        )

        validator = DDSConfigValidator()
        is_valid, issues = validator.validate(config)

        assert is_valid is True
