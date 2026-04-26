"""
DDS Configuration Tool for Automotive Systems.

This package provides configuration management and code generation for:
- Diagnostic services (UDS/DCM/DoIP)
- E2E protection (AUTOSAR E2E)
- OTA updates (FOTA)

Author: Hermes Agent
Version: 1.0.0
"""

__version__ = "1.0.0"
__author__ = "Hermes Agent"

from diagnostic_config import DiagnosticConfig, DCMParams, DiagServiceConfig
from diagnostic_config import DIDDefinition, DTCConfig, SecurityPolicy, DoIPConfig
from e2e_config import E2EConfig, E2EProfileConfig, DataIDMapping, E2EProtection
from ota_config import OTAConfig, ABPartitionLayout, OTACampaign, ECUUpdate
from code_generator import CCodeGenerator, ConfigValidator

__all__ = [
    # Diagnostic
    'DiagnosticConfig',
    'DCMParams',
    'DiagServiceConfig',
    'DIDDefinition',
    'DTCConfig',
    'SecurityPolicy',
    'DoIPConfig',
    # E2E
    'E2EConfig',
    'E2EProfileConfig',
    'DataIDMapping',
    'E2EProtection',
    # OTA
    'OTAConfig',
    'ABPartitionLayout',
    'OTACampaign',
    'ECUUpdate',
    # Code Generation
    'CCodeGenerator',
    'ConfigValidator',
]
