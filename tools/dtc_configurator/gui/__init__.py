"""
DTC Configurator Tool Package
AUTOSAR Diagnostic Trouble Code Management
"""

__version__ = '1.0.0'
__author__ = 'yuleASR'

from .config_parser import DTCConfigParser, validate_config

__all__ = ['DTCConfigParser', 'validate_config']
