#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ARXML Parser Core Module
支持 AUTOSAR R20-11 ARXML Schema 解析

作者: YuleTech BSW Team
版本: 1.0.0
"""

import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Any, Union
from enum import Enum
from pathlib import Path
import logging
import re

# 配置日志
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


# ============================================================================
# AUTOSAR 命名空间定义 (R20-11)
# ============================================================================
AUTOSAR_NS = {
    'AR': 'http://autosar.org/schema/r4.0',
    'xsi': 'http://www.w3.org/2001/XMLSchema-instance'
}

# 元素标签映射
AR_TAGS = {
    'PACKAGE': '{{{0}}}AR-PACKAGE'.format(AUTOSAR_NS['AR']),
    'ELEMENTS': '{{{0}}}ELEMENTS'.format(AUTOSAR_NS['AR']),
    'ECU_CONFIGURATION': '{{{0}}}ECU-CONFIGURATION'.format(AUTOSAR_NS['AR']),
    'APPLICATION_SW_COMPONENT': '{{{0}}}APPLICATION-SW-COMPONENT-TYPE'.format(AUTOSAR_NS['AR']),
    'COMPOSITION_SW_COMPONENT': '{{{0}}}COMPOSITION-SW-COMPONENT-TYPE'.format(AUTOSAR_NS['AR']),
    'SERVICE_SW_COMPONENT': '{{{0}}}SERVICE-SW-COMPONENT-TYPE'.format(AUTOSAR_NS['AR']),
    'INTERNAL_BEHAVIOR': '{{{0}}}SWC-INTERNAL-BEHAVIOR'.format(AUTOSAR_NS['AR']),
    'P_PORT_PROTOTYPE': '{{{0}}}P-PORT-PROTOTYPE'.format(AUTOSAR_NS['AR']),
    'R_PORT_PROTOTYPE': '{{{0}}}R-PORT-PROTOTYPE'.format(AUTOSAR_NS['AR']),
    'SENDER_RECEIVER_INTERFACE': '{{{0}}}SENDER-RECEIVER-INTERFACE'.format(AUTOSAR_NS['AR']),
    'CLIENT_SERVER_INTERFACE': '{{{0}}}CLIENT-SERVER-INTERFACE'.format(AUTOSAR_NS['AR']),
    'MODE_SWITCH_INTERFACE': '{{{0}}}MODE-SWITCH-INTERFACE'.format(AUTOSAR_NS['AR']),
    'DATA_ELEMENT': '{{{0}}}DATA-ELEMENT-PROTOTYPE'.format(AUTOSAR_NS['AR']),
    'DATA_TYPE': '{{{0}}}IMPLEMENTATION-DATA-TYPE'.format(AUTOSAR_NS['AR']),
    'APPLICATION_DATA_TYPE': '{{{0}}}APPLICATION-PRIMITIVE-DATA-TYPE'.format(AUTOSAR_NS['AR']),
    'RUNNABLE_ENTITY': '{{{0}}}RUNNABLE-ENTITY'.format(AUTOSAR_NS['AR']),
    'EVENT': '{{{0}}}RTE-EVENT'.format(AUTOSAR_NS['AR']),
}


# ============================================================================
# 异常类定义
# ============================================================================
class ARXMLParseError(Exception):
    """ARXML解析异常基类"""
    pass


class ARXMLValidationError(ARXMLParseError):
    """ARXML验证异常"""
    pass


class ARXMLNotFoundError(ARXMLParseError):
    """ARXML元素未找到异常"""
    pass


class ARXMLSchemaError(ARXMLParseError):
    """ARXML Schema错误"""
    pass


# ============================================================================
# 数据模型定义
# ============================================================================
@dataclass
class ARXMLBaseElement:
    """ARXML基础元素"""
    name: str
    uuid: Optional[str] = None
    short_name: Optional[str] = None
    
    def __post_init__(self):
        if not self.short_name and self.name:
            self.short_name = self.name


@dataclass
class DataType(ARXMLBaseElement):
    """数据类型定义"""
    category: str = "VALUE"
    base_type: Optional[str] = None
    data_constraint: Optional[str] = None
    compu_method: Optional[str] = None
    sw_data_def_props: Dict[str, Any] = field(default_factory=dict)
    # AUTOSAR SYMBOL-PROPS/SYMBOL — 覆盖默认短名的发射符号名
    symbol_name: Optional[str] = None
    # AUTOSAR TYPE-EMITTER — 非 RTE 的类型由外部工具发射，RTE 生成器应跳过
    type_emitter: Optional[str] = None


@dataclass
class DataElement(ARXMLBaseElement):
    """数据元素定义"""
    type_ref: Optional[str] = None
    data_type: Optional[DataType] = None
    is_queued: bool = False
    sw_addr_method: Optional[str] = None
    sw_calibration_access: Optional[str] = None


@dataclass
class PortInterface(ARXMLBaseElement):
    """端口接口定义"""
    interface_type: str = "SENDER_RECEIVER"
    data_elements: List[DataElement] = field(default_factory=list)
    operations: List[Dict[str, Any]] = field(default_factory=list)
    mode_groups: List[Dict[str, Any]] = field(default_factory=list)


@dataclass
class PortPrototype(ARXMLBaseElement):
    """端口原型定义"""
    port_type: str = "P_PORT"  # P_PORT 或 R_PORT
    interface_ref: Optional[str] = None
    port_interface: Optional[PortInterface] = None
    com_spec: Dict[str, Any] = field(default_factory=dict)


@dataclass
class RunnableEntity(ARXMLBaseElement):
    """可运行实体定义"""
    symbol: Optional[str] = None
    can_be_invoked_concurrently: bool = False
    minimum_start_interval: float = 0.0
    data_send_points: List[Dict[str, Any]] = field(default_factory=list)
    data_receive_points: List[Dict[str, Any]] = field(default_factory=list)
    server_call_points: List[Dict[str, Any]] = field(default_factory=list)
    access_points: List[Dict[str, Any]] = field(default_factory=list)


@dataclass
class RTEEvent(ARXMLBaseElement):
    """RTE事件定义"""
    event_type: str = "TIMING"
    start_on_event_ref: Optional[str] = None
    period_ms: Optional[float] = None
    source_ref: Optional[str] = None


@dataclass
class InternalBehavior(ARXMLBaseElement):
    """内部行为定义"""
    component_ref: Optional[str] = None
    runnables: List[RunnableEntity] = field(default_factory=list)
    events: List[RTEEvent] = field(default_factory=list)
    data_type_mappings: List[Dict[str, Any]] = field(default_factory=list)
    exclusive_areas: List[str] = field(default_factory=list)


@dataclass
class SoftwareComponent(ARXMLBaseElement):
    """软件组件定义"""
    component_type: str = "APPLICATION"
    ports: List[PortPrototype] = field(default_factory=list)
    internal_behaviors: List[InternalBehavior] = field(default_factory=list)
    composition_type: Optional[str] = None
    components_refs: List[str] = field(default_factory=list)
    connectors: List[Dict[str, Any]] = field(default_factory=list)


@dataclass
class ECUConfiguration(ARXMLBaseElement):
    """ECU配置定义"""
    ecu_id: Optional[str] = None
    ecu_extract_ref: Optional[str] = None
    modules: List[Dict[str, Any]] = field(default_factory=list)
    services: List[Dict[str, Any]] = field(default_factory=list)
    communication_matrix: Dict[str, Any] = field(default_factory=dict)


# ============================================================================
# ARXML解析器核心类
# ============================================================================
class ARXMLParser:
    """
    AUTOSAR R20-11 ARXML 解析器
    
    功能:
    - 解析ECU配置
    - 解析软件组件
    - 解析内部行为
    - 解析端口接口
    - 解析数据类型
    """
    
    def __init__(self, autosar_version: str = "R20-11"):
        """
        初始化解析器
        
        Args:
            autosar_version: AUTOSAR版本，默认R20-11
        """
        self.autosar_version = autosar_version
        self.ns = AUTOSAR_NS
        self._root: Optional[ET.Element] = None
        self._packages: Dict[str, ET.Element] = {}
        self._parsed_types: Dict[str, DataType] = {}
        self._parsed_interfaces: Dict[str, PortInterface] = {}
        self._parsed_components: Dict[str, SoftwareComponent] = {}
        
    def parse_file(self, file_path: Union[str, Path]) -> 'ARXMLParser':
        """
        解析ARXML文件
        
        Args:
            file_path: ARXML文件路径
            
        Returns:
            self 用于链式调用
            
        Raises:
            ARXMLParseError: 解析失败时抛出
        """
        file_path = Path(file_path)
        
        if not file_path.exists():
            raise ARXMLNotFoundError(f"文件不存在: {file_path}")
        
        try:
            logger.info(f"开始解析ARXML文件: {file_path}")
            tree = ET.parse(str(file_path))
            self._root = tree.getroot()
            
            # 提取命名空间
            self._extract_namespace()
            
            # 构建包索引
            self._build_package_index()
            
            logger.info(f"ARXML文件解析成功，找到 {len(self._packages)} 个包")
            return self
            
        except ET.ParseError as e:
            raise ARXMLParseError(f"XML解析失败: {e}")
        except Exception as e:
            raise ARXMLParseError(f"解析ARXML文件失败: {e}")
    
    def parse_string(self, xml_string: str) -> 'ARXMLParser':
        """
        从字符串解析ARXML
        
        Args:
            xml_string: XML字符串
            
        Returns:
            self 用于链式调用
        """
        try:
            logger.info("开始解析ARXML字符串")
            self._root = ET.fromstring(xml_string)
            self._extract_namespace()
            self._build_package_index()
            return self
        except ET.ParseError as e:
            raise ARXMLParseError(f"XML字符串解析失败: {e}")
    
    def _extract_namespace(self):
        """提取XML命名空间"""
        if self._root is None:
            return
            
        # 从根元素提取命名空间
        root_tag = self._root.tag
        if root_tag.startswith('{'):
            ns_uri = root_tag[1:root_tag.index('}')]
            self.ns['AR'] = ns_uri
            # 更新标签映射
            for key in AR_TAGS:
                if key != 'xsi':
                    AR_TAGS[key] = f'{{{ns_uri}}}{AR_TAGS[key].split("}")[-1]}'
    
    def _build_package_index(self):
        """构建AR包索引"""
        if self._root is None:
            return
            
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        package_tag = f'{{{ns_ar}}}AR-PACKAGE'
        
        for package in self._root.iter(package_tag):
            short_name_elem = package.find(f'{{{ns_ar}}}SHORT-NAME')
            if short_name_elem is not None:
                package_name = short_name_elem.text
                self._packages[package_name] = package
    
    def _get_text(self, element: Optional[ET.Element], tag: str, default: str = "") -> str:
        """安全获取子元素文本"""
        if element is None:
            return default
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        child = element.find(f'{{{ns_ar}}}{tag}')
        return child.text if child is not None else default
    
    def _get_attr(self, element: Optional[ET.Element], attr: str, default: str = "") -> str:
        """安全获取属性值"""
        if element is None:
            return default
        return element.get(attr, default)
    
    def _get_uuid(self, element: Optional[ET.Element]) -> Optional[str]:
        """获取UUID"""
        if element is None:
            return None
        return element.get('UUID')
    
    def find_package(self, package_name: str) -> Optional[ET.Element]:
        """
        查找指定名称的包
        
        Args:
            package_name: 包名
            
        Returns:
            包元素或None
        """
        return self._packages.get(package_name)
    
    def get_all_packages(self) -> List[str]:
        """
        获取所有包名
        
        Returns:
            包名列表
        """
        return list(self._packages.keys())
    
    # ========================================================================
    # 数据类型解析
    # ========================================================================
    def parse_data_types(self, package_name: Optional[str] = None) -> List[DataType]:
        """
        解析数据类型
        
        Args:
            package_name: 指定包名，None则解析所有包
            
        Returns:
            数据类型列表
        """
        data_types = []
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        
        if package_name:
            if package_name not in self._packages:
                return []
            packages = [self._packages[package_name]]
        else:
            packages = self._packages.values()
        
        for package in packages:
            elements = package.find(f'{{{ns_ar}}}ELEMENTS')
            if elements is None:
                continue
                
            # 解析应用数据类型
            for dtype_elem in elements:
                tag = dtype_elem.tag.split('}')[-1] if '}' in dtype_elem.tag else dtype_elem.tag
                
                if tag in ['APPLICATION-PRIMITIVE-DATA-TYPE', 'APPLICATION-ARRAY-DATA-TYPE',
                          'IMPLEMENTATION-DATA-TYPE', 'SW-BASE-TYPE']:
                    try:
                        data_type = self._parse_data_type_element(dtype_elem, tag)
                        if data_type:
                            data_types.append(data_type)
                            self._parsed_types[data_type.name] = data_type
                    except Exception as e:
                        logger.warning(f"解析数据类型失败: {e}")
        
        logger.info(f"解析完成，找到 {len(data_types)} 个数据类型")
        return data_types
    
    def _parse_data_type_element(self, elem: ET.Element, tag: str) -> Optional[DataType]:
        """解析单个数据类型元素"""
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        
        short_name = self._get_text(elem, 'SHORT-NAME')
        if not short_name:
            return None
        
        data_type = DataType(
            name=short_name,
            uuid=self._get_uuid(elem),
            short_name=short_name
        )
        
        # 根据类型解析详细信息
        if tag == 'APPLICATION-PRIMITIVE-DATA-TYPE':
            data_type.category = self._get_text(elem, 'CATEGORY', 'VALUE')
            # 解析数据约束和计算方法
            props = elem.find(f'{{{ns_ar}}}SW-DATA-DEF-PROPS')
            if props is not None:
                data_type.sw_data_def_props = self._parse_sw_data_def_props(props)
        
        elif tag == 'IMPLEMENTATION-DATA-TYPE':
            data_type.category = self._get_text(elem, 'CATEGORY', 'VALUE')
            base_type_ref = elem.find(f'{{{ns_ar}}}BASE-TYPE-REF')
            if base_type_ref is not None:
                data_type.base_type = base_type_ref.text
            # AUTOSAR TYPE-EMITTER: 标注由外部工具发射的类型
            data_type.type_emitter = self._get_text(elem, 'TYPE-EMITTER') or None
            # AUTOSAR SYMBOL-PROPS/SYMBOL: 发射符号名覆盖默认短名
            props = elem.find(f'{{{ns_ar}}}SW-DATA-DEF-PROPS')
            if props is not None:
                data_type.sw_data_def_props = self._parse_sw_data_def_props(props)
            if data_type.sw_data_def_props.get('symbol_name'):
                data_type.symbol_name = data_type.sw_data_def_props['symbol_name']
        
        return data_type
    
    def _parse_sw_data_def_props(self, props_elem: ET.Element) -> Dict[str, Any]:
        """解析SW数据定义属性"""
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        props = {}
        
        variants = props_elem.find(f'{{{ns_ar}}}SW-DATA-DEF-PROPS-VARIANTS')
        if variants is not None:
            variant = variants.find(f'{{{ns_ar}}}SW-DATA-DEF-PROPS-CONDITIONAL')
            if variant is not None:
                # 解析计算方法和数据约束
                compu_ref = variant.find(f'{{{ns_ar}}}COMPU-METHOD-REF')
                if compu_ref is not None:
                    props['compu_method'] = compu_ref.text
                    
                constraint_ref = variant.find(f'{{{ns_ar}}}DATA-CONSTR-REF')
                if constraint_ref is not None:
                    props['data_constraint'] = constraint_ref.text

                # AUTOSAR SYMBOL-PROPS/SYMBOL — 发射符号名
                symbol_props = variant.find(f'{{{ns_ar}}}SYMBOL-PROPS')
                if symbol_props is not None:
                    symbol = symbol_props.find(f'{{{ns_ar}}}SYMBOL')
                    if symbol is not None and symbol.text:
                        props['symbol_name'] = symbol.text.strip()

                # AUTOSAR IMPLEMENTATION-DATA-TYPE-REF — TYPE_REFERENCE 指向的实现类型
                impl_ref = variant.find(f'{{{ns_ar}}}IMPLEMENTATION-DATA-TYPE-REF')
                if impl_ref is not None:
                    props['impl_data_type_ref'] = impl_ref.text
        
        return props
    
    def get_data_type(self, type_name: str) -> Optional[DataType]:
        """获取已解析的数据类型"""
        return self._parsed_types.get(type_name)
    
    # ========================================================================
    # 端口接口解析
    # ========================================================================
    def parse_port_interfaces(self, package_name: Optional[str] = None) -> List[PortInterface]:
        """
        解析端口接口
        
        Args:
            package_name: 指定包名，None则解析所有包
            
        Returns:
            端口接口列表
        """
        interfaces = []
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        
        if package_name:
            if package_name not in self._packages:
                return []
            packages = [self._packages[package_name]]
        else:
            packages = self._packages.values()
        
        for package in packages:
            elements = package.find(f'{{{ns_ar}}}ELEMENTS')
            if elements is None:
                continue
                
            for elem in elements:
                tag = elem.tag.split('}')[-1] if '}' in elem.tag else elem.tag
                
                if tag in ['SENDER-RECEIVER-INTERFACE', 'CLIENT-SERVER-INTERFACE',
                          'MODE-SWITCH-INTERFACE', 'PARAMETER-INTERFACE']:
                    try:
                        interface = self._parse_port_interface_element(elem, tag)
                        if interface:
                            interfaces.append(interface)
                            self._parsed_interfaces[interface.name] = interface
                    except Exception as e:
                        logger.warning(f"解析端口接口失败: {e}")
        
        logger.info(f"解析完成，找到 {len(interfaces)} 个端口接口")
        return interfaces
    
    def _parse_port_interface_element(self, elem: ET.Element, tag: str) -> Optional[PortInterface]:
        """解析单个端口接口元素"""
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        
        short_name = self._get_text(elem, 'SHORT-NAME')
        if not short_name:
            return None
        
        interface_type = tag.replace('-INTERFACE', '').replace('-', '_')
        
        port_interface = PortInterface(
            name=short_name,
            uuid=self._get_uuid(elem),
            short_name=short_name,
            interface_type=interface_type
        )
        
        # 解析数据元素
        if tag == 'SENDER-RECEIVER-INTERFACE':
            data_elements = elem.find(f'{{{ns_ar}}}DATA-ELEMENTS')
            if data_elements is not None:
                for de_elem in data_elements:
                    data_element = self._parse_data_element(de_elem)
                    if data_element:
                        port_interface.data_elements.append(data_element)
        
        # 解析操作
        elif tag == 'CLIENT-SERVER-INTERFACE':
            operations = elem.find(f'{{{ns_ar}}}OPERATIONS')
            if operations is not None:
                for op_elem in operations:
                    operation = self._parse_operation_element(op_elem)
                    if operation:
                        port_interface.operations.append(operation)
        
        return port_interface
    
    def _parse_data_element(self, elem: ET.Element) -> Optional[DataElement]:
        """解析数据元素"""
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        
        short_name = self._get_text(elem, 'SHORT-NAME')
        if not short_name:
            return None
        
        data_element = DataElement(
            name=short_name,
            uuid=self._get_uuid(elem),
            short_name=short_name
        )
        
        # 解析类型引用
        type_ref = elem.find(f'{{{ns_ar}}}TYPE-TREF')
        if type_ref is not None:
            data_element.type_ref = type_ref.text
        
        return data_element
    
    def _parse_operation_element(self, elem: ET.Element) -> Optional[Dict[str, Any]]:
        """解析操作元素"""
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        
        short_name = self._get_text(elem, 'SHORT-NAME')
        if not short_name:
            return None
        
        operation = {
            'name': short_name,
            'uuid': self._get_uuid(elem),
            'arguments': []
        }
        
        # 解析参数
        arguments = elem.find(f'{{{ns_ar}}}ARGUMENTS')
        if arguments is not None:
            for arg_elem in arguments:
                arg_name = self._get_text(arg_elem, 'SHORT-NAME')
                if arg_name:
                    arg_info = {'name': arg_name}
                    type_ref = arg_elem.find(f'{{{ns_ar}}}TYPE-TREF')
                    if type_ref is not None:
                        arg_info['type_ref'] = type_ref.text
                    direction = self._get_text(arg_elem, 'DIRECTION', 'IN')
                    arg_info['direction'] = direction
                    operation['arguments'].append(arg_info)
        
        return operation
    
    def get_port_interface(self, interface_name: str) -> Optional[PortInterface]:
        """获取已解析的端口接口"""
        return self._parsed_interfaces.get(interface_name)
    
    # ========================================================================
    # 软件组件解析
    # ========================================================================
    def parse_software_components(self, package_name: Optional[str] = None) -> List[SoftwareComponent]:
        """
        解析软件组件
        
        Args:
            package_name: 指定包名，None则解析所有包
            
        Returns:
            软件组件列表
        """
        components = []
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        
        if package_name:
            if package_name not in self._packages:
                return []
            packages = [self._packages[package_name]]
        else:
            packages = self._packages.values()
        
        for package in packages:
            elements = package.find(f'{{{ns_ar}}}ELEMENTS')
            if elements is None:
                continue
                
            for elem in elements:
                tag = elem.tag.split('}')[-1] if '}' in elem.tag else elem.tag
                
                if tag in ['APPLICATION-SW-COMPONENT-TYPE', 'COMPOSITION-SW-COMPONENT-TYPE',
                          'SERVICE-SW-COMPONENT-TYPE', 'ECU-ABSTRACTION-SW-COMPONENT-TYPE',
                          'COMPLEX-DEVICE-DRIVER-SW-COMPONENT-TYPE']:
                    try:
                        component = self._parse_software_component_element(elem, tag)
                        if component:
                            components.append(component)
                            self._parsed_components[component.name] = component
                    except Exception as e:
                        logger.warning(f"解析软件组件失败: {e}")
        
        logger.info(f"解析完成，找到 {len(components)} 个软件组件")
        return components
    
    def _parse_software_component_element(self, elem: ET.Element, tag: str) -> Optional[SoftwareComponent]:
        """解析单个软件组件元素"""
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        
        short_name = self._get_text(elem, 'SHORT-NAME')
        if not short_name:
            return None
        
        component_type = tag.replace('-SW-COMPONENT-TYPE', '').replace('-', '_')
        
        component = SoftwareComponent(
            name=short_name,
            uuid=self._get_uuid(elem),
            short_name=short_name,
            component_type=component_type
        )
        
        # 解析端口
        ports = elem.find(f'{{{ns_ar}}}PORTS')
        if ports is not None:
            for port_elem in ports:
                port = self._parse_port_prototype(port_elem)
                if port:
                    component.ports.append(port)
        
        # 解析内部行为
        behaviors = elem.find(f'{{{ns_ar}}}INTERNAL-BEHAVIORS')
        if behaviors is not None:
            for behavior_elem in behaviors:
                behavior = self._parse_internal_behavior_element(behavior_elem)
                if behavior:
                    component.internal_behaviors.append(behavior)
        
        return component
    
    def _parse_port_prototype(self, elem: ET.Element) -> Optional[PortPrototype]:
        """解析端口原型"""
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        
        tag = elem.tag.split('}')[-1] if '}' in elem.tag else elem.tag
        
        short_name = self._get_text(elem, 'SHORT-NAME')
        if not short_name:
            return None
        
        port_type = 'P_PORT' if tag == 'P-PORT-PROTOTYPE' else 'R_PORT'
        
        port = PortPrototype(
            name=short_name,
            uuid=self._get_uuid(elem),
            short_name=short_name,
            port_type=port_type
        )
        
        # 解析接口引用
        if port_type == 'P_PORT':
            provided_intf = elem.find(f'{{{ns_ar}}}PROVIDED-INTERFACE-TREF')
            if provided_intf is not None:
                port.interface_ref = provided_intf.text
        else:
            required_intf = elem.find(f'{{{ns_ar}}}REQUIRED-INTERFACE-TREF')
            if required_intf is not None:
                port.interface_ref = required_intf.text
        
        return port
    
    def get_software_component(self, component_name: str) -> Optional[SoftwareComponent]:
        """获取已解析的软件组件"""
        return self._parsed_components.get(component_name)
    
    # ========================================================================
    # 内部行为解析
    # ========================================================================
    def _parse_internal_behavior_element(self, elem: ET.Element) -> Optional[InternalBehavior]:
        """解析内部行为元素"""
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        
        short_name = self._get_text(elem, 'SHORT-NAME')
        if not short_name:
            return None
        
        behavior = InternalBehavior(
            name=short_name,
            uuid=self._get_uuid(elem),
            short_name=short_name
        )
        
        # 解析组件引用
        component_ref = elem.find(f'{{{ns_ar}}}COMPONENT-REF')
        if component_ref is not None:
            behavior.component_ref = component_ref.text
        
        # 解析Runnable实体
        entities = elem.find(f'{{{ns_ar}}}RUNNABLES')
        if entities is not None:
            for runnable_elem in entities:
                runnable = self._parse_runnable_entity(runnable_elem)
                if runnable:
                    behavior.runnables.append(runnable)
        
        # 解析事件
        events = elem.find(f'{{{ns_ar}}}EVENTS')
        if events is not None:
            for event_elem in events:
                event = self._parse_rte_event(event_elem)
                if event:
                    behavior.events.append(event)
        
        return behavior
    
    def _parse_runnable_entity(self, elem: ET.Element) -> Optional[RunnableEntity]:
        """解析Runnable实体"""
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        
        short_name = self._get_text(elem, 'SHORT-NAME')
        if not short_name:
            return None
        
        runnable = RunnableEntity(
            name=short_name,
            uuid=self._get_uuid(elem),
            short_name=short_name,
            symbol=self._get_text(elem, 'SYMBOL'),
            can_be_invoked_concurrently=self._get_text(elem, 'CAN-BE-INVOKED-CONCURRENTLY') == 'true'
        )
        
        # 解析最小启动间隔
        min_interval = self._get_text(elem, 'MINIMUM-START-INTERVAL')
        if min_interval:
            try:
                runnable.minimum_start_interval = float(min_interval)
            except ValueError:
                pass
        
        return runnable
    
    def _parse_rte_event(self, elem: ET.Element) -> Optional[RTEEvent]:
        """解析RTE事件"""
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        
        tag = elem.tag.split('}')[-1] if '}' in elem.tag else elem.tag
        short_name = self._get_text(elem, 'SHORT-NAME')
        if not short_name:
            return None
        
        event_type = tag.replace('-EVENT', '').replace('-', '_')
        
        event = RTEEvent(
            name=short_name,
            uuid=self._get_uuid(elem),
            short_name=short_name,
            event_type=event_type
        )
        
        # 解析启动事件引用
        start_ref = elem.find(f'{{{ns_ar}}}START-ON-EVENT-REF')
        if start_ref is not None:
            event.start_on_event_ref = start_ref.text
        
        # 解析周期（如果是定时事件）
        if 'TIMING' in event_type:
            period = self._get_text(elem, 'PERIOD')
            if period:
                try:
                    # 从AUTOSAR时间格式解析 (如 "0.01")
                    event.period_ms = float(period) * 1000
                except ValueError:
                    pass
        
        return event
    
    # ========================================================================
    # ECU配置解析
    # ========================================================================
    def parse_ecu_configuration(self) -> Optional[ECUConfiguration]:
        """
        解析ECU配置
        
        Returns:
            ECU配置对象或None
        """
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        
        for package in self._packages.values():
            elements = package.find(f'{{{ns_ar}}}ELEMENTS')
            if elements is None:
                continue
                
            for elem in elements:
                tag = elem.tag.split('}')[-1] if '}' in elem.tag else elem.tag
                
                if tag == 'ECU-CONFIGURATION':
                    return self._parse_ecu_configuration_element(elem)
        
        logger.warning("未找到ECU配置")
        return None
    
    def _parse_ecu_configuration_element(self, elem: ET.Element) -> Optional[ECUConfiguration]:
        """解析ECU配置元素"""
        ns_ar = self.ns.get('AR', AUTOSAR_NS['AR'])
        
        short_name = self._get_text(elem, 'SHORT-NAME')
        if not short_name:
            return None
        
        ecu_config = ECUConfiguration(
            name=short_name,
            uuid=self._get_uuid(elem),
            short_name=short_name
        )
        
        # 解析ECU ID
        ecu_id = self._get_text(elem, 'ECU-ID')
        if ecu_id:
            ecu_config.ecu_id = ecu_id
        
        # 解析ECU Extract引用
        extract_ref = elem.find(f'{{{ns_ar}}}ECU-EXTRACT-REF')
        if extract_ref is not None:
            ecu_config.ecu_extract_ref = extract_ref.text
        
        return ecu_config
    
    # ========================================================================
    # 完整解析
    # ========================================================================
    def parse_all(self) -> Dict[str, Any]:
        """
        解析所有支持的元素
        
        Returns:
            包含所有解析结果的字典
        """
        result = {
            'data_types': self.parse_data_types(),
            'port_interfaces': self.parse_port_interfaces(),
            'software_components': self.parse_software_components(),
            'ecu_configuration': self.parse_ecu_configuration()
        }
        
        return result
    
    def validate(self) -> List[str]:
        """
        验证解析结果
        
        Returns:
            错误信息列表，空列表表示验证通过
        """
        errors = []
        
        # 验证数据类型引用
        for component in self._parsed_components.values():
            for port in component.ports:
                if port.interface_ref:
                    # 提取接口名
                    interface_name = port.interface_ref.split('/')[-1]
                    if interface_name not in self._parsed_interfaces:
                        errors.append(f"组件 '{component.name}' 的端口 '{port.name}' 引用了未解析的接口 '{interface_name}'")
        
        # 验证端口接口中的数据类型引用
        for interface in self._parsed_interfaces.values():
            for data_elem in interface.data_elements:
                if data_elem.type_ref:
                    type_name = data_elem.type_ref.split('/')[-1]
                    if type_name not in self._parsed_types:
                        errors.append(f"接口 '{interface.name}' 的数据元素 '{data_elem.name}' 引用了未解析的类型 '{type_name}'")
        
        return errors


# ============================================================================
# 工具函数
# ============================================================================
def parse_arxml(file_path: Union[str, Path]) -> ARXMLParser:
    """
    快速解析ARXML文件
    
    Args:
        file_path: ARXML文件路径
        
    Returns:
        解析器实例
        
    Example:
        parser = parse_arxml('/path/to/file.arxml')
        components = parser.parse_software_components()
    """
    parser = ARXMLParser()
    return parser.parse_file(file_path)


def parse_arxml_string(xml_string: str) -> ARXMLParser:
    """
    从字符串快速解析ARXML
    
    Args:
        xml_string: XML字符串
        
    Returns:
        解析器实例
    """
    parser = ARXMLParser()
    return parser.parse_string(xml_string)


# ============================================================================
# 主入口（用于命令行测试）
# ============================================================================
if __name__ == '__main__':
    import json
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python arxml_parser.py <arxml_file>")
        sys.exit(1)
    
    file_path = sys.argv[1]
    
    try:
        parser = parse_arxml(file_path)
        result = parser.parse_all()
        
        # 打印解析结果统计
        print(f"ARXML解析完成:")
        print(f"  - 数据类型: {len(result['data_types'])} 个")
        print(f"  - 端口接口: {len(result['port_interfaces'])} 个")
        print(f"  - 软件组件: {len(result['software_components'])} 个")
        print(f"  - ECU配置: {'找到' if result['ecu_configuration'] else '未找到'}")
        
        # 验证
        errors = parser.validate()
        if errors:
            print(f"\n验证警告 ({len(errors)} 项):")
            for error in errors:
                print(f"  - {error}")
        else:
            print("\n验证通过!")
            
    except ARXMLParseError as e:
        print(f"解析失败: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"未知错误: {e}")
        sys.exit(1)