"""
YAML Parser for DDS Configuration

This module provides YAML parsing capabilities with error position reporting,
support for the dds_config.yaml format, and conversion to DDSConfig objects.
"""

import os
import re
from pathlib import Path
from typing import Dict, List, Optional, Any, Tuple, Union

import yaml
from yaml import YAMLError, MarkedYAMLError

try:
    from yaml import CLoader as Loader, CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper

from .config_model import DDSConfig, SystemInfo, Domain, Participant, Topic, QoS


class YAMLParseError(Exception):
    """YAML parsing error with position information"""
    
    def __init__(self, message: str, line: int = None, column: int = None, 
                 context: str = None, file_path: str = None):
        self.message = message
        self.line = line
        self.column = column
        self.context = context
        self.file_path = file_path
        super().__init__(self._format_message())
    
    def _format_message(self) -> str:
        parts = [self.message]
        if self.file_path:
            parts.append(f"File: {self.file_path}")
        if self.line is not None:
            parts.append(f"Line: {self.line}" + (f":{self.column}" if self.column else ""))
        if self.context:
            parts.append(f"Context: {self.context}")
        return " | ".join(parts)
    
    def __str__(self) -> str:
        return self._format_message()


class YAMLParser:
    """
    YAML parser for DDS configuration files
    
    Features:
    - Parse YAML files with position tracking
    - Convert to DDSConfig dataclasses
    - Detailed error reporting
    - Support for custom YAML constructors
    """
    
    # Supported top-level keys
    TOP_LEVEL_KEYS = [
        "system", "domains", "transport", "tsn", "security", 
        "qos_profiles", "topics"
    ]
    
    def __init__(self, file_path: Optional[str] = None):
        self.file_path = file_path
        self.warnings: List[str] = []
        self.yaml_content: Optional[str] = None
    
    def parse_file(self, file_path: str) -> DDSConfig:
        """
        Parse YAML file and return DDSConfig
        
        Args:
            file_path: Path to YAML file
            
        Returns:
            DDSConfig object
            
        Raises:
            YAMLParseError: If parsing fails
            FileNotFoundError: If file doesn't exist
        """
        self.file_path = file_path
        
        if not os.path.exists(file_path):
            raise FileNotFoundError(f"Configuration file not found: {file_path}")
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                self.yaml_content = f.read()
        except IOError as e:
            raise YAMLParseError(f"Failed to read file: {e}", file_path=file_path)
        
        return self.parse_string(self.yaml_content)
    
    def parse_string(self, yaml_string: str) -> DDSConfig:
        """
        Parse YAML string and return DDSConfig
        
        Args:
            yaml_string: YAML content as string
            
        Returns:
            DDSConfig object
            
        Raises:
            YAMLParseError: If parsing fails
        """
        self.yaml_content = yaml_string
        self.warnings = []
        
        try:
            # Parse YAML with custom loader for better error handling
            raw_data = self._safe_load_yaml(yaml_string)
        except MarkedYAMLError as e:
            raise YAMLParseError(
                f"YAML syntax error: {e.problem}",
                line=e.problem_mark.line + 1 if e.problem_mark else None,
                column=e.problem_mark.column + 1 if e.problem_mark else None,
                context=e.context,
                file_path=self.file_path
            )
        except YAMLError as e:
            raise YAMLParseError(
                f"YAML parsing error: {e}",
                file_path=self.file_path
            )
        
        # Validate and convert to DDSConfig
        return self._convert_to_config(raw_data)
    
    def _safe_load_yaml(self, yaml_string: str) -> Dict[str, Any]:
        """
        Safely load YAML with custom constructors
        
        Args:
            yaml_string: YAML content
            
        Returns:
            Parsed YAML as dictionary
        """
        # Custom loader that preserves line numbers
        class DDSConfigLoader(Loader):
            pass
        
        # Add custom constructors if needed
        def timestamp_constructor(loader, node):
            """Handle timestamp values"""
            return loader.construct_scalar(node)
        
        DDSConfigLoader.add_implicit_resolver(
            'timestamp',
            re.compile(r'^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}'),
            ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9']
        )
        DDSConfigLoader.add_constructor('timestamp', timestamp_constructor)
        
        return yaml.load(yaml_string, Loader=DDSConfigLoader)
    
    def _convert_to_config(self, data: Dict[str, Any]) -> DDSConfig:
        """
        Convert raw YAML data to DDSConfig
        
        Args:
            data: Raw YAML dictionary
            
        Returns:
            DDSConfig object
            
        Raises:
            YAMLParseError: If configuration is invalid
        """
        if not isinstance(data, dict):
            raise YAMLParseError(
                "Configuration must be a YAML object (dictionary)",
                file_path=self.file_path
            )
        
        # Check required top-level keys
        if "system" not in data:
            raise YAMLParseError(
                "Missing required top-level key: 'system'",
                file_path=self.file_path
            )
        
        if "domains" not in data:
            raise YAMLParseError(
                "Missing required top-level key: 'domains'",
                file_path=self.file_path
            )
        
        # Warn about unknown keys
        for key in data.keys():
            if key not in self.TOP_LEVEL_KEYS:
                self.warnings.append(f"Unknown top-level key: '{key}'")
        
        try:
            return DDSConfig.from_dict(data)
        except (KeyError, TypeError, ValueError) as e:
            raise YAMLParseError(
                f"Configuration conversion error: {e}",
                file_path=self.file_path
            )
    
    def get_warnings(self) -> List[str]:
        """Get list of warnings from parsing"""
        return self.warnings
    
    def get_line_context(self, line: int, context_lines: int = 2) -> Optional[str]:
        """
        Get context around a specific line
        
        Args:
            line: Line number (1-indexed)
            context_lines: Number of context lines before and after
            
        Returns:
            Context string with line numbers
        """
        if not self.yaml_content:
            return None
        
        lines = self.yaml_content.split('\n')
        start = max(0, line - context_lines - 1)
        end = min(len(lines), line + context_lines)
        
        context_parts = []
        for i in range(start, end):
            marker = ">>> " if i == line - 1 else "    "
            context_parts.append(f"{marker}{i+1:4d}: {lines[i]}")
        
        return '\n'.join(context_parts)


def parse_yaml_file(file_path: str) -> Tuple[DDSConfig, List[str]]:
    """
    Convenience function to parse YAML file
    
    Args:
        file_path: Path to YAML file
        
    Returns:
        Tuple of (DDSConfig, warnings)
        
    Raises:
        YAMLParseError: If parsing fails
        FileNotFoundError: If file doesn't exist
    """
    parser = YAMLParser()
    config = parser.parse_file(file_path)
    return config, parser.get_warnings()


def parse_yaml_string(yaml_string: str, file_path: str = None) -> Tuple[DDSConfig, List[str]]:
    """
    Convenience function to parse YAML string
    
    Args:
        yaml_string: YAML content as string
        file_path: Optional file path for error reporting
        
    Returns:
        Tuple of (DDSConfig, warnings)
        
    Raises:
        YAMLParseError: If parsing fails
    """
    parser = YAMLParser(file_path=file_path)
    config = parser.parse_string(yaml_string)
    return config, parser.get_warnings()


class YAMLWriter:
    """
    Write DDSConfig to YAML format
    
    Features:
    - Pretty-print YAML with comments
    - Custom ordering of keys
    - Preserve structure
    """
    
    def __init__(self, default_flow_style: bool = False, 
                 allow_unicode: bool = True, sort_keys: bool = False):
        self.default_flow_style = default_flow_style
        self.allow_unicode = allow_unicode
        self.sort_keys = sort_keys
    
    def write(self, config: DDSConfig, file_path: Optional[str] = None) -> str:
        """
        Write DDSConfig to YAML
        
        Args:
            config: DDSConfig object
            file_path: Optional file path to write to
            
        Returns:
            YAML string
        """
        data = config.to_dict()
        
        yaml_string = yaml.dump(
            data,
            Dumper=Dumper,
            default_flow_style=self.default_flow_style,
            allow_unicode=self.allow_unicode,
            sort_keys=self.sort_keys,
            width=120,
            indent=2
        )
        
        if file_path:
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(yaml_string)
        
        return yaml_string
    
    def write_with_comments(self, config: DDSConfig, 
                           comments: Optional[Dict[str, str]] = None) -> str:
        """
        Write DDSConfig to YAML with inline comments
        
        Args:
            config: DDSConfig object
            comments: Dictionary mapping paths to comment strings
            
        Returns:
            YAML string with comments
        """
        yaml_string = self.write(config)
        
        if comments:
            lines = yaml_string.split('\n')
            commented_lines = []
            
            for line in lines:
                commented_lines.append(line)
                
                # Add comments based on path matching
                for path, comment in comments.items():
                    if path in line and not line.strip().startswith('#'):
                        indent = len(line) - len(line.lstrip())
                        commented_lines.append(' ' * indent + f"# {comment}")
            
            yaml_string = '\n'.join(commented_lines)
        
        return yaml_string


def write_yaml_file(config: DDSConfig, file_path: str, 
                    include_comments: bool = True) -> None:
    """
    Convenience function to write DDSConfig to YAML file
    
    Args:
        config: DDSConfig object
        file_path: Path to output file
        include_comments: Whether to include explanatory comments
    """
    writer = YAMLWriter()
    
    if include_comments:
        comments = {
            "system:": "System configuration",
            "domains:": "DDS Domains",
            "transport:": "Network transport configuration",
            "tsn:": "Time-Sensitive Networking configuration",
            "security:": "DDS Security configuration",
        }
        yaml_string = writer.write_with_comments(config, comments)
    else:
        yaml_string = writer.write(config)
    
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(yaml_string)
