"""
DDS Configuration Parser Module

This module provides parsing, validation, and conversion capabilities
for DDS configuration files in YAML and ARXML formats.
"""

from .config_model import (
    DDSConfig,
    SystemInfo,
    Domain,
    Participant,
    Publisher,
    Subscriber,
    Topic,
    QoS,
    Transport,
    TSN,
    TSNStream,
    Security,
)

from .yaml_parser import (
    YAMLParser,
    YAMLParseError,
    parse_yaml_file,
    parse_yaml_string,
    write_yaml_file,
    YAMLWriter,
)

from .validator import (
    SchemaValidator,
    ValidationReport,
    ValidationError,
    ValidationWarning,
    validate_config,
    validate_yaml_file,
)

from .arxml_converter import (
    ARXMLConverter,
    ARXMLConversionError,
    yaml_to_arxml,
    arxml_to_yaml,
)

__version__ = "1.0.0"

__all__ = [
    # Config models
    "DDSConfig",
    "SystemInfo",
    "Domain",
    "Participant",
    "Publisher",
    "Subscriber",
    "Topic",
    "QoS",
    "Transport",
    "TSN",
    "TSNStream",
    "Security",
    # YAML Parser
    "YAMLParser",
    "YAMLWriter",
    "YAMLParseError",
    "parse_yaml_file",
    "parse_yaml_string",
    "write_yaml_file",
    # Validator
    "SchemaValidator",
    "ValidationReport",
    "ValidationError",
    "ValidationWarning",
    "validate_config",
    "validate_yaml_file",
    # ARXML Converter
    "ARXMLConverter",
    "ARXMLConversionError",
    "yaml_to_arxml",
    "arxml_to_yaml",
]
