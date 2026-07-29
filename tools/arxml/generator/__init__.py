#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ARXML Configuration Generator Package

将ARXML解析结果转换为C代码配置

支持模块:
- Com (通信模块)
- CanIf (CAN接口)
- NvM (NVRAM管理)
- PduR (PDU路由)
"""

from .config_generator import (
    ConfigGenerator,
    ConfigGeneratorBase,
    ComConfigGenerator,
    CanIfConfigGenerator,
    NvMConfigGenerator,
    PduRConfigGenerator,
    ComSignalConfig,
    ComIPduConfig,
    CanIfPduConfig,
    NvMBlockConfig,
    PduRRoutingPath,
)

__all__ = [
    'ConfigGenerator',
    'ConfigGeneratorBase',
    'ComConfigGenerator',
    'CanIfConfigGenerator',
    'NvMConfigGenerator',
    'PduRConfigGenerator',
    'ComSignalConfig',
    'ComIPduConfig',
    'CanIfPduConfig',
    'NvMBlockConfig',
    'PduRRoutingPath',
]

__version__ = '1.0.0'
__author__ = 'YuleTech BSW Team'
