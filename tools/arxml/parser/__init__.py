#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ARXML Parser Module - AUTOSAR R20-11 ARXML Schema Parser

本模块提供AUTOSAR R20-11 ARXML文件的解析功能，支持:
- ECU配置解析 (EcuConfiguration)
- 软件组件解析 (SoftwareComponent)
- 内部行为解析 (InternalBehavior)
- 端口接口解析 (PortInterface)
- 数据类型解析 (DataType)

示例:
    from arxml.parser import ARXMLParser, parse_arxml
    
    # 方式1: 使用解析器类
    parser = ARXMLParser()
    parser.parse_file('/path/to/file.arxml')
    components = parser.parse_software_components()
    
    # 方式2: 使用便捷函数
    parser = parse_arxml('/path/to/file.arxml')
    result = parser.parse_all()
"""

from .arxml_parser import (
    # 主要解析器类
    ARXMLParser,
    
    # 便捷函数
    parse_arxml,
    parse_arxml_string,
    
    # 异常类
    ARXMLParseError,
    ARXMLValidationError,
    ARXMLNotFoundError,
    ARXMLSchemaError,
    
    # 数据模型
    ARXMLBaseElement,
    DataType,
    DataElement,
    PortInterface,
    PortPrototype,
    RunnableEntity,
    RTEEvent,
    InternalBehavior,
    SoftwareComponent,
    ECUConfiguration,
    
    # 常量
    AUTOSAR_NS,
    AR_TAGS,
)

__version__ = '1.0.0'
__author__ = 'YuleTech BSW Team'

__all__ = [
    # 解析器
    'ARXMLParser',
    'parse_arxml',
    'parse_arxml_string',
    
    # 异常
    'ARXMLParseError',
    'ARXMLValidationError',
    'ARXMLNotFoundError',
    'ARXMLSchemaError',
    
    # 数据模型
    'ARXMLBaseElement',
    'DataType',
    'DataElement',
    'PortInterface',
    'PortPrototype',
    'RunnableEntity',
    'RTEEvent',
    'InternalBehavior',
    'SoftwareComponent',
    'ECUConfiguration',
    
    # 常量
    'AUTOSAR_NS',
    'AR_TAGS',
]