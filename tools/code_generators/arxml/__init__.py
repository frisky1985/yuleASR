"""
yuleASR ARXML Configuration Generator

用于生成符合AUTOSAR R4.0标准的ECUC配置文件
"""

__version__ = "1.0.0"
__author__ = "yuleASR Team"

from .ecuc_config_model import (
    EcucModuleConfigurationValues,
    EcucContainerValue,
    EcucBooleanParamValue,
    EcucIntegerParamValue,
    EcucFloatParamValue,
    EcucStringParamValue,
    EcucEnumParamValue,
    EcucFunctionNameParamValue,
    EcucDefinitionRef
)

from .arxml_ecuc_generator import ArxmlEcucGenerator

__all__ = [
    "EcucModuleConfigurationValues",
    "EcucContainerValue",
    "EcucBooleanParamValue",
    "EcucIntegerParamValue",
    "EcucFloatParamValue",
    "EcucStringParamValue",
    "EcucEnumParamValue",
    "EcucFunctionNameParamValue",
    "EcucDefinitionRef",
    "ArxmlEcucGenerator"
]
