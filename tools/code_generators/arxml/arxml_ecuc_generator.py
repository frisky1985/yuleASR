#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ARXML ECUC Generator - AUTOSAR R4.0

生成符合AUTOSAR R4.0标准的ARXML配置文件，支持：
- AR-PACKAGES结构
- ECUC-MODULE-CONFIGURATION-VALUES
- ECUC-CONTAINER-VALUE (包含嵌套)
- ECUC-PARAMETER-VALUES (多种类型)
- ECUC-REFERENCE-VALUES
- 正确的命名空间处理
"""

import xml.etree.ElementTree as ET
from xml.dom import minidom
from typing import Optional, List, Any
from datetime import datetime

from ecuc_config_model import (
    EcucModuleConfigurationValues,
    EcucContainerValue,
    EcucParameterValue,
    EcucBooleanParamValue,
    EcucIntegerParamValue,
    EcucFloatParamValue,
    EcucStringParamValue,
    EcucEnumParamValue,
    EcucFunctionNameParamValue,
    EcucLinkerSymbolParamValue,
    EcucMacroParamValue,
    EcucReferenceValue,
    EcucInstanceReferenceValue,
    EcucSymbolicNameReferenceValue,
    EcucChoiceReferenceValue,
    EcucDefinitionRef,
    EcucIndex,
    EcucParamValue,
    EcucRefValue,
    create_module_config,
    create_container,
    create_boolean_param,
    create_integer_param,
    create_string_param,
    create_enum_param,
    create_reference_value,
)


class ArxmlEcucGenerator:
    """ARXML ECUC配置生成器"""
    
    # AUTOSAR R4.0 命名空间
    AUTOSAR_NS = "http://autosar.org/schema/r4.0"
    AUTOSAR_SCHEMA = "http://autosar.org/schema/r4.0 autosar_4-2-2.xsd"
    
    # 命名空间前缀
    NS_PREFIX = "{http://autosar.org/schema/r4.0}"
    
    def __init__(
        self,
        admin_data: Optional[Any] = None,
        file_version: str = "1.0.0",
        company: str = "YuleTech",
        author: Optional[str] = None
    ):
        self.admin_data = admin_data
        self.file_version = file_version
        self.company = company
        self.author = author or "ARXML Generator"
        self.generated_modules: List[EcucModuleConfigurationValues] = []
    
    def register_module(self, module: EcucModuleConfigurationValues) -> 'ArxmlEcucGenerator':
        """注册模块配置"""
        self.generated_modules.append(module)
        return self
    
    def _create_element(self, tag: str, text: Optional[str] = None) -> ET.Element:
        """创建XML元素"""
        elem = ET.Element(f"{{{self.AUTOSAR_NS}}}{tag}")
        if text is not None:
            elem.text = text
        return elem
    
    def _create_sub_element(self, parent: ET.Element, tag: str, text: Optional[str] = None) -> ET.Element:
        """创建子元素"""
        elem = ET.SubElement(parent, f"{{{self.AUTOSAR_NS}}}{tag}")
        if text is not None:
            elem.text = text
        return elem
    
    def _set_attribute(self, elem: ET.Element, name: str, value: str) -> None:
        """设置元素属性"""
        elem.set(name, value)
    
    def _create_definition_ref_element(self, parent: ET.Element, def_ref: EcucDefinitionRef) -> ET.Element:
        """创建DEFINITION-REF元素"""
        elem = self._create_sub_element(parent, "DEFINITION-REF")
        self._set_attribute(elem, "DEST", def_ref.dest)
        elem.text = def_ref.value
        return elem
    
    def _create_index_element(self, parent: ET.Element, index: EcucIndex) -> ET.Element:
        """创建INDEX元素"""
        return self._create_sub_element(parent, "INDEX", str(index.value))
    
    def _create_short_name_element(self, parent: ET.Element, name: str) -> ET.Element:
        """创建SHORT-NAME元素"""
        return self._create_sub_element(parent, "SHORT-NAME", name)
    
    def generate_module_config(self, module: EcucModuleConfigurationValues) -> str:
        """
        生成单个模块的ARXML配置
        
        Args:
            module: 模块配置对象
            
        Returns:
            格式化的ARXML字符串
        """
        self.register_module(module)
        return self.to_string(pretty=True)
    
    def generate(self) -> ET.Element:
        """生成完整的AUTOSAR XML文档"""
        # 创建根元素
        root = ET.Element(f"{{{self.AUTOSAR_NS}}}AUTOSAR")
        
        # 添加命名空间声明
        root.set("xmlns", self.AUTOSAR_NS)
        root.set(f"xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance")
        root.set("xsi:schemaLocation", self.AUTOSAR_SCHEMA)
        
        # 创建AR-PACKAGES
        ar_packages = self._create_sub_element(root, "AR-PACKAGES")
        
        # 按包分组模块
        modules_by_package: dict = {}
        for module in self.generated_modules:
            pkg = module.package_name
            if pkg not in modules_by_package:
                modules_by_package[pkg] = []
            modules_by_package[pkg].append(module)
        
        # 为每个包创建结构
        for package_name, modules in modules_by_package.items():
            self._generate_package(ar_packages, package_name, modules)
        
        return root
    
    def _generate_package(
        self,
        parent: ET.Element,
        package_name: str,
        modules: List[EcucModuleConfigurationValues]
    ) -> ET.Element:
        """生成AR-PACKAGE结构"""
        ar_package = self._create_sub_element(parent, "AR-PACKAGE")
        self._create_short_name_element(ar_package, package_name)
        
        # 添加UUID（可选）
        # self._create_sub_element(ar_package, "UUID", self._generate_uuid())
        
        # 创建ELEMENTS容器
        elements = self._create_sub_element(ar_package, "ELEMENTS")
        
        # 添加所有模块配置
        for module in modules:
            self._generate_module_config(elements, module)
        
        return ar_package
    
    def _generate_module_config(
        self,
        parent: ET.Element,
        module: EcucModuleConfigurationValues
    ) -> ET.Element:
        """生成ECUC-MODULE-CONFIGURATION-VALUES"""
        module_elem = self._create_sub_element(parent, module.tag_name)
        
        # SHORT-NAME
        self._create_short_name_element(module_elem, module.short_name)
        
        # DEFINITION-REF
        self._create_definition_ref_element(module_elem, module.definition_ref)
        
        # IMPLEMENTATION-CONFIG-VARIANT
        self._create_sub_element(
            module_elem,
            "IMPLEMENTATION-CONFIG-VARIANT",
            module.implementation_config_variant
        )
        
        # MODULE-DESCRIPTION (可选)
        if module.module_description:
            self._create_sub_element(
                module_elem,
                "MODULE-DESCRIPTION",
                module.module_description
            )
        
        # CONTAINERS
        if module.containers:
            containers_elem = self._create_sub_element(module_elem, "CONTAINERS")
            for container in module.containers:
                self._generate_container(containers_elem, container)
        
        return module_elem
    
    def _generate_container(
        self,
        parent: ET.Element,
        container: EcucContainerValue
    ) -> ET.Element:
        """生成ECUC-CONTAINER-VALUE (支持嵌套)"""
        container_elem = self._create_sub_element(parent, container.tag_name)
        
        # SHORT-NAME
        self._create_short_name_element(container_elem, container.short_name)
        
        # DEFINITION-REF
        self._create_definition_ref_element(container_elem, container.definition_ref)
        
        # INDEX (可选)
        if container.index:
            self._create_index_element(container_elem, container.index)
        
        # PARAMETER-VALUES
        if container.parameter_values:
            param_values_elem = self._create_sub_element(container_elem, "PARAMETER-VALUES")
            for param in container.parameter_values:
                self._generate_parameter_value(param_values_elem, param)
        
        # REFERENCE-VALUES
        if container.reference_values:
            ref_values_elem = self._create_sub_element(container_elem, "REFERENCE-VALUES")
            for ref in container.reference_values:
                self._generate_reference_value(ref_values_elem, ref)
        
        # SUB-CONTAINERS (嵌套容器)
        if container.sub_containers:
            sub_containers_elem = self._create_sub_element(container_elem, "SUB-CONTAINERS")
            for sub_container in container.sub_containers:
                self._generate_container(sub_containers_elem, sub_container)
        
        return container_elem
    
    def _generate_parameter_value(
        self,
        parent: ET.Element,
        param: EcucParameterValue
    ) -> ET.Element:
        """生成ECUC参数值"""
        param_elem = self._create_sub_element(parent, param.tag_name)
        
        # DEFINITION-REF
        self._create_definition_ref_element(param_elem, param.definition_ref)
        
        # INDEX (可选)
        if param.index:
            self._create_index_element(param_elem, param.index)
        
        # VALUE - 根据参数类型处理
        if isinstance(param, (EcucBooleanParamValue, EcucIntegerParamValue, EcucFloatParamValue)):
            # 数值类型
            value_elem = self._create_sub_element(param_elem, param.value_tag)
            value_elem.text = param.get_value_str()
        elif isinstance(param, (EcucStringParamValue, EcucEnumParamValue)):
            # 文本类型
            value_elem = self._create_sub_element(param_elem, param.value_tag)
            value_elem.text = param.get_value_str()
        elif isinstance(param, (EcucFunctionNameParamValue, EcucLinkerSymbolParamValue, EcucMacroParamValue)):
            # 特殊类型
            value_elem = self._create_sub_element(param_elem, param.value_tag)
            value_elem.text = param.get_value_str()
        
        return param_elem
    
    def _generate_reference_value(
        self,
        parent: ET.Element,
        ref: EcucRefValue
    ) -> ET.Element:
        """生成ECUC引用值"""
        ref_elem = self._create_sub_element(parent, ref.tag_name)
        
        # DEFINITION-REF
        self._create_definition_ref_element(ref_elem, ref.definition_ref)
        
        # INDEX (可选)
        if ref.index:
            self._create_index_element(ref_elem, ref.index)
        
        # 根据引用类型处理
        if isinstance(ref, EcucReferenceValue):
            # VALUE-REF
            value_ref = self._create_sub_element(ref_elem, "VALUE-REF")
            value_ref.set("DEST", "ECUC-CONTAINER-VALUE")
            value_ref.text = ref.value_ref
            
        elif isinstance(ref, EcucInstanceReferenceValue):
            # CONTEXT-REF (可选)
            if ref.context_ref:
                context_ref = self._create_sub_element(ref_elem, "CONTEXT-REF")
                context_ref.set("DEST", "ECUC-CONTAINER-VALUE")
                context_ref.text = ref.context_ref
            
            # TARGET-REF
            target_ref = self._create_sub_element(ref_elem, "TARGET-REF")
            target_ref.set("DEST", "ECUC-CONTAINER-VALUE")
            target_ref.text = ref.target_ref
            
        elif isinstance(ref, EcucSymbolicNameReferenceValue):
            # VALUE-REF
            value_ref = self._create_sub_element(ref_elem, "VALUE-REF")
            value_ref.set("DEST", "ECUC-CONTAINER-VALUE")
            value_ref.text = ref.value_ref
            
        elif isinstance(ref, EcucChoiceReferenceValue):
            # VALUE-REF
            value_ref = self._create_sub_element(ref_elem, "VALUE-REF")
            value_ref.set("DEST", "ECUC-CONTAINER-VALUE")
            value_ref.text = ref.value_ref
        
        return ref_elem
    
    def to_string(self, pretty: bool = True) -> str:
        """输出为字符串"""
        root = self.generate()
        
        if pretty:
            # 使用minidom进行美化
            rough_string = ET.tostring(root, encoding='unicode')
            reparsed = minidom.parseString(rough_string)
            # 移除空白节点
            self._remove_whitespace_nodes(reparsed.documentElement)
            pretty_xml = reparsed.documentElement.toprettyxml(indent="  ")
            # 添加XML声明
            return '<?xml version="1.0" encoding="UTF-8"?>\n' + pretty_xml
        else:
            return ET.tostring(root, encoding='unicode')
    
    def _remove_whitespace_nodes(self, node):
        """移除XML中的空白节点"""
        for child in list(node.childNodes):
            if child.nodeType == child.TEXT_NODE and not child.data.strip():
                node.removeChild(child)
            elif child.hasChildNodes():
                self._remove_whitespace_nodes(child)
    
    def save(self, filepath: str, pretty: bool = True) -> None:
        """保存到文件"""
        xml_content = self.to_string(pretty=pretty)
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(xml_content)
    
    def clear(self) -> 'ArxmlEcucGenerator':
        """清空已注册的模块"""
        self.generated_modules.clear()
        return self


# =============================================================================
# 使用示例和测试
# =============================================================================

def create_sample_os_config() -> EcucModuleConfigurationValues:
    """创建示例OS模块配置"""
    # 创建模块
    os_config = create_module_config(
        short_name="Os",
        module_def_path="/AUTOSAR/EcucDefs/Os",
        config_variant="VARIANT-PRE-COMPILE"
    )
    
    # 创建OsCounter容器
    counter_def = create_container(
        def_path="/AUTOSAR/EcucDefs/Os/OsCounter",
        short_name="OsCounter_SystemCounter",
        dest="ECUC-CONTAINER-DEF"
    )
    
    # 添加计数器参数
    counter_def.add_parameter(create_integer_param(
        def_path="/AUTOSAR/EcucDefs/Os/OsCounter/OsCounterMaxAllowedValue",
        value=4294967295
    ))
    counter_def.add_parameter(create_integer_param(
        def_path="/AUTOSAR/EcucDefs/Os/OsCounter/OsCounterMinCycle",
        value=1
    ))
    counter_def.add_parameter(create_integer_param(
        def_path="/AUTOSAR/EcucDefs/Os/OsCounter/OsCounterTicksPerBase",
        value=1000
    ))
    counter_def.add_parameter(create_enum_param(
        def_path="/AUTOSAR/EcucDefs/Os/OsCounter/OsCounterType",
        value="HARDWARE"
    ))
    
    os_config.add_container(counter_def)
    
    # 创建OsTask容器
    task_def = create_container(
        def_path="/AUTOSAR/EcucDefs/Os/OsTask",
        short_name="OsTask_InitTask",
        dest="ECUC-CONTAINER-DEF"
    )
    
    # 添加任务参数
    task_def.add_parameter(create_integer_param(
        def_path="/AUTOSAR/EcucDefs/Os/OsTask/OsTaskActivation",
        value=1
    ))
    task_def.add_parameter(create_boolean_param(
        def_path="/AUTOSAR/EcucDefs/Os/OsTask/OsTaskAutostart",
        value=True
    ))
    task_def.add_parameter(create_integer_param(
        def_path="/AUTOSAR/EcucDefs/Os/OsTask/OsTaskPriority",
        value=1
    ))
    task_def.add_parameter(create_enum_param(
        def_path="/AUTOSAR/EcucDefs/Os/OsTask/OsTaskSchedule",
        value="FULL"
    ))
    
    # 添加子容器OsTaskAutostart
    autostart_container = create_container(
        def_path="/AUTOSAR/EcucDefs/Os/OsTask/OsTaskAutostartRef",
        short_name="OsTaskAutostart",
        dest="ECUC-CONTAINER-DEF"
    )
    
    # 添加应用模式引用
    autostart_container.add_reference(create_reference_value(
        def_path="/AUTOSAR/EcucDefs/Os/OsTask/OsTaskAutostartRef/OsTaskAppModeRef",
        value_ref="/ECUC/Os/OsAppMode/OsAppMode_OS"
    ))
    
    task_def.add_sub_container(autostart_container)
    
    os_config.add_container(task_def)
    
    # 创建更多任务
    idle_task = create_container(
        def_path="/AUTOSAR/EcucDefs/Os/OsTask",
        short_name="OsTask_IdleTask",
        dest="ECUC-CONTAINER-DEF"
    )
    idle_task.add_parameter(create_integer_param(
        def_path="/AUTOSAR/EcucDefs/Os/OsTask/OsTaskPriority",
        value=0
    ))
    os_config.add_container(idle_task)
    
    return os_config


def create_sample_com_config() -> EcucModuleConfigurationValues:
    """创建示例COM模块配置"""
    com_config = create_module_config(
        short_name="Com",
        module_def_path="/AUTOSAR/EcucDefs/Com"
    )
    
    # 添加ComConfig容器
    com_general = create_container(
        def_path="/AUTOSAR/EcucDefs/Com/ComConfig",
        short_name="ComConfig",
        dest="ECUC-CONTAINER-DEF"
    )
    
    # 添加通用参数
    com_general.add_parameter(create_integer_param(
        def_path="/AUTOSAR/EcucDefs/Com/ComConfig/ComMaxIPduCnt",
        value=32
    ))
    com_general.add_parameter(create_boolean_param(
        def_path="/AUTOSAR/EcucDefs/Com/ComGeneral/ComDevErrorDetect",
        value=True
    ))
    com_general.add_parameter(create_boolean_param(
        def_path="/AUTOSAR/EcucDefs/Com/ComGeneral/ComVersionInfoApi",
        value=True
    ))
    
    # 添加子容器 - IPdu
    ipdu = create_container(
        def_path="/AUTOSAR/EcucDefs/Com/ComConfig/ComIPdu",
        short_name="ComIPdu_EngineData",
        dest="ECUC-CONTAINER-DEF"
    )
    ipdu.add_parameter(create_integer_param(
        def_path="/AUTOSAR/EcucDefs/Com/ComConfig/ComIPdu/ComIPduHandleId",
        value=0
    ))
    ipdu.add_parameter(create_enum_param(
        def_path="/AUTOSAR/EcucDefs/Com/ComConfig/ComIPdu/ComIPduDirection",
        value="SEND"
    ))
    ipdu.add_parameter(create_string_param(
        def_path="/AUTOSAR/EcucDefs/Com/ComConfig/ComIPdu/ComIPduSignalProcessing",
        value="DEFERRED"
    ))
    
    com_general.add_sub_container(ipdu)
    com_config.add_container(com_general)
    
    return com_config


def main():
    """主函数 - 演示生成器使用"""
    print("=" * 60)
    print("AUTOSAR ECUC ARXML Generator Demo")
    print("=" * 60)
    
    # 创建生成器
    generator = ArxmlEcucGenerator(
        company="YuleTech",
        author="ARXML Generator v1.0"
    )
    
    # 创建OS配置
    print("\n[1] Creating Os module configuration...")
    os_config = create_sample_os_config()
    generator.register_module(os_config)
    print(f"    - Module: {os_config.short_name}")
    print(f"    - Containers: {len(os_config.containers)}")
    
    # 创建COM配置
    print("\n[2] Creating Com module configuration...")
    com_config = create_sample_com_config()
    generator.register_module(com_config)
    print(f"    - Module: {com_config.short_name}")
    print(f"    - Containers: {len(com_config.containers)}")
    
    # 生成XML
    print("\n[3] Generating ARXML content...")
    xml_output = generator.to_string(pretty=True)
    print(f"    - Generated {len(xml_output)} characters")
    
    # 保存文件
    output_file = "ecuc_config.arxml"
    print(f"\n[4] Saving to file: {output_file}")
    generator.save(output_file)
    print(f"    - File saved successfully")
    
    # 显示预览
    print("\n[5] ARXML Preview (first 2000 chars):")
    print("-" * 60)
    print(xml_output[:2000])
    print("-" * 60)
    print("... (truncated)")
    
    print("\n" + "=" * 60)
    print("Demo completed successfully!")
    print("=" * 60)
    
    return generator


if __name__ == "__main__":
    main()
