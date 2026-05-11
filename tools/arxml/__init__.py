"""
AUTOSAR ARXML Configuration Tool Package
=========================================

This package provides tools for parsing, validating, and generating
configuration from AUTOSAR ARXML files.

Modules:
    parser: ARXML parsing functionality
    generator: C code generation from ARXML
    checker: Integrity checking for ARXML files

Example:
    >>> from arxml.parser import ARXMLParser
    >>> parser = ARXMLParser()
    >>> parser.parse_file('example.arxml')
    >>> components = parser.parse_software_components()
"""

__version__ = '1.0.0'
__author__ = 'YuleTech'

# Make submodules available
__all__ = ['parser', 'generator', 'checker']

# Import main classes for convenience
try:
    from arxml.parser import ARXMLParser, parse_arxml
    from arxml.generator import ConfigGenerator
except ImportError:
    # Allow import to work even if dependencies are not installed
    pass
