#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ECUC Configuration Model - AUTOSAR R4.0

符合AUTOSAR标准的ECUC配置数据模型，支持：
- EcucModuleConfigurationValues
- EcucContainerValue
- 各种参数类型 (BOOLEAN, INTEGER, FLOAT, STRING, ENUM, FUNCTION_NAME等)
- 容器嵌套
"""

from dataclasses import dataclass, field
from typing import List, Optional, Union, Any
from enum import Enum, auto
from abc import ABC, abstractmethod


class EcucParamType(Enum):
    """ECUC参数类型枚举"""
    BOOLEAN = "BOOLEAN"
    INTEGER = "INTEGER"
    FLOAT = "FLOAT"
    STRING = "STRING"
    ENUM = "ENUM"
    FUNCTION_NAME = "FUNCTION-NAME"
    LINKER_SYMBOL = "LINKER-SYMBOL"
    MACRO = "MACRO"


class EcucRefType(Enum):
    """ECUC引用类型枚举"""
    REFERENCE = "REFERENCE"
    CHOICE_REFERENCE = "CHOICE-REFERENCE"
    INSTANCE_REFERENCE = "INSTANCE-REFERENCE"
    SYMBOLIC_NAME_REFERENCE = "SYMBOLIC-NAME-REFERENCE"


@dataclass
class EcucDefinitionRef:
    """ECUC定义引用"""
    dest: str  # 目标类型，如 "ECUC-BOOLEAN-PARAM-DEF", "ECUC-CONTAINER-DEF"
    value: str  # 引用路径，如 "/AUTOSAR/EcucDefs/Os/OsAppMode"
    
    def __str__(self) -> str:
        return self.value


@dataclass
class EcucIndex:
    """ECUC索引值"""
    value: int
    
    def __str__(self) -> str:
        return str(self.value)


class EcucParameterValue(ABC):
    """ECUC参数值抽象基类"""
    
    def __init__(self, definition_ref: EcucDefinitionRef, index: Optional[EcucIndex] = None):
        self.definition_ref = definition_ref
        self.index = index
    
    @property
    @abstractmethod
    def tag_name(self) -> str:
        """返回ARXML标签名"""
        pass
    
    @property
    @abstractmethod
    def value_tag(self) -> str:
        """返回值标签名"""
        pass


@dataclass
class EcucBooleanParamValue(EcucParameterValue):
    """ECUC布尔参数值"""
    value: bool
    
    def __init__(self, definition_ref: EcucDefinitionRef, value: bool, index: Optional[EcucIndex] = None):
        super().__init__(definition_ref, index)
        self.value = value
    
    @property
    def tag_name(self) -> str:
        return "ECUC-NUMERICAL-PARAM-VALUE"
    
    @property
    def value_tag(self) -> str:
        return "VALUE"
    
    def get_value_str(self) -> str:
        return "true" if self.value else "false"


@dataclass
class EcucIntegerParamValue(EcucParameterValue):
    """ECUC整数参数值"""
    value: int
    
    def __init__(self, definition_ref: EcucDefinitionRef, value: int, index: Optional[EcucIndex] = None):
        super().__init__(definition_ref, index)
        self.value = value
    
    @property
    def tag_name(self) -> str:
        return "ECUC-NUMERICAL-PARAM-VALUE"
    
    @property
    def value_tag(self) -> str:
        return "VALUE"
    
    def get_value_str(self) -> str:
        return str(self.value)


@dataclass
class EcucFloatParamValue(EcucParameterValue):
    """ECUC浮点数参数值"""
    value: float
    
    def __init__(self, definition_ref: EcucDefinitionRef, value: float, index: Optional[EcucIndex] = None):
        super().__init__(definition_ref, index)
        self.value = value
    
    @property
    def tag_name(self) -> str:
        return "ECUC-NUMERICAL-PARAM-VALUE"
    
    @property
    def value_tag(self) -> str:
        return "VALUE"
    
    def get_value_str(self) -> str:
        return f"{self.value:.6f}"


@dataclass
class EcucStringParamValue(EcucParameterValue):
    """ECUC字符串参数值"""
    value: str
    
    def __init__(self, definition_ref: EcucDefinitionRef, value: str, index: Optional[EcucIndex] = None):
        super().__init__(definition_ref, index)
        self.value = value
    
    @property
    def tag_name(self) -> str:
        return "ECUC-TEXTUAL-PARAM-VALUE"
    
    @property
    def value_tag(self) -> str:
        return "VALUE"
    
    def get_value_str(self) -> str:
        return self.value


@dataclass
class EcucEnumParamValue(EcucParameterValue):
    """ECUC枚举参数值"""
    value: str
    
    def __init__(self, definition_ref: EcucDefinitionRef, value: str, index: Optional[EcucIndex] = None):
        super().__init__(definition_ref, index)
        self.value = value
    
    @property
    def tag_name(self) -> str:
        return "ECUC-TEXTUAL-PARAM-VALUE"
    
    @property
    def value_tag(self) -> str:
        return "VALUE"
    
    def get_value_str(self) -> str:
        return self.value


@dataclass
class EcucFunctionNameParamValue(EcucParameterValue):
    """ECUC函数名参数值"""
    value: str
    
    def __init__(self, definition_ref: EcucDefinitionRef, value: str, index: Optional[EcucIndex] = None):
        super().__init__(definition_ref, index)
        self.value = value
    
    @property
    def tag_name(self) -> str:
        return "ECUC-FUNCTION-NAME-DEF"
    
    @property
    def value_tag(self) -> str:
        return "VALUE"
    
    def get_value_str(self) -> str:
        return self.value


@dataclass
class EcucLinkerSymbolParamValue(EcucParameterValue):
    """ECUC链接器符号参数值"""
    value: str
    
    def __init__(self, definition_ref: EcucDefinitionRef, value: str, index: Optional[EcucIndex] = None):
        super().__init__(definition_ref, index)
        self.value = value
    
    @property
    def tag_name(self) -> str:
        return "ECUC-LINKER-SYMBOL-DEF"
    
    @property
    def value_tag(self) -> str:
        return "VALUE"
    
    def get_value_str(self) -> str:
        return self.value


@dataclass
class EcucMacroParamValue(EcucParameterValue):
    """ECUC宏参数值"""
    value: str
    
    def __init__(self, definition_ref: EcucDefinitionRef, value: str, index: Optional[EcucIndex] = None):
        super().__init__(definition_ref, index)
        self.value = value
    
    @property
    def tag_name(self) -> str:
        return "ECUC-MACRO-NAME-DEF"
    
    @property
    def value_tag(self) -> str:
        return "VALUE"
    
    def get_value_str(self) -> str:
        return self.value


# 参数值联合类型
EcucParamValue = Union[
    EcucBooleanParamValue,
    EcucIntegerParamValue,
    EcucFloatParamValue,
    EcucStringParamValue,
    EcucEnumParamValue,
    EcucFunctionNameParamValue,
    EcucLinkerSymbolParamValue,
    EcucMacroParamValue
]


@dataclass
class EcucReferenceValue:
    """ECUC引用值"""
    definition_ref: EcucDefinitionRef
    value_ref: str  # 引用的目标路径
    index: Optional[EcucIndex] = None
    
    @property
    def tag_name(self) -> str:
        return "ECUC-REFERENCE-VALUE"


@dataclass
class EcucInstanceReferenceValue:
    """ECUC实例引用值"""
    definition_ref: EcucDefinitionRef
    context_ref: Optional[str] = None  # 上下文引用
    target_ref: str = ""  # 目标引用
    index: Optional[EcucIndex] = None
    
    @property
    def tag_name(self) -> str:
        return "ECUC-INSTANCE-REFERENCE-VALUE"


@dataclass
class EcucSymbolicNameReferenceValue:
    """ECUC符号名引用值"""
    definition_ref: EcucDefinitionRef
    value_ref: str
    index: Optional[EcucIndex] = None
    
    @property
    def tag_name(self) -> str:
        return "ECUC-SYMBOLIC-NAME-REFERENCE-VALUE"


@dataclass
class EcucChoiceReferenceValue:
    """ECUC选择引用值"""
    definition_ref: EcucDefinitionRef
    value_ref: str
    index: Optional[EcucIndex] = None
    
    @property
    def tag_name(self) -> str:
        return "ECUC-CHOICE-REFERENCE-VALUE"


# 引用值联合类型
EcucRefValue = Union[
    EcucReferenceValue,
    EcucInstanceReferenceValue,
    EcucSymbolicNameReferenceValue,
    EcucChoiceReferenceValue
]


class EcucContainerValue:
    """ECUC容器值 - 支持嵌套容器"""
    
    def __init__(
        self,
        definition_ref: EcucDefinitionRef,
        short_name: str,
        index: Optional[EcucIndex] = None,
        parameter_values: Optional[List[EcucParamValue]] = None,
        reference_values: Optional[List[EcucRefValue]] = None,
        sub_containers: Optional[List['EcucContainerValue']] = None
    ):
        self.definition_ref = definition_ref
        self.short_name = short_name
        self.index = index
        self.parameter_values = parameter_values or []
        self.reference_values = reference_values or []
        self.sub_containers = sub_containers or []
    
    @property
    def tag_name(self) -> str:
        return "ECUC-CONTAINER-VALUE"
    
    def add_parameter(self, param: EcucParamValue) -> 'EcucContainerValue':
        """添加参数值"""
        self.parameter_values.append(param)
        return self
    
    def add_reference(self, ref: EcucRefValue) -> 'EcucContainerValue':
        """添加引用值"""
        self.reference_values.append(ref)
        return self
    
    def add_sub_container(self, container: 'EcucContainerValue') -> 'EcucContainerValue':
        """添加子容器"""
        self.sub_containers.append(container)
        return self
    
    def get_parameter_by_def(self, def_path: str) -> Optional[EcucParamValue]:
        """通过定义路径获取参数值"""
        for param in self.parameter_values:
            if param.definition_ref.value == def_path:
                return param
        return None
    
    def get_sub_container_by_name(self, name: str) -> Optional['EcucContainerValue']:
        """通过短名称获取子容器"""
        for container in self.sub_containers:
            if container.short_name == name:
                return container
        return None


@dataclass
class EcucModuleConfigurationValues:
    """ECUC模块配置值 - 顶层配置容器"""
    
    short_name: str
    definition_ref: EcucDefinitionRef  # 模块定义引用
    implementation_config_variant: str = "VARIANT-PRE-COMPILE"
    module_description: Optional[str] = None
    admin_data: Optional[Any] = None
    containers: List[EcucContainerValue] = field(default_factory=list)
    
    # ARXML包结构相关
    package_name: str = "ECUC"  # 默认包名
    
    def __post_init__(self):
        if self.containers is None:
            self.containers = []
    
    @property
    def tag_name(self) -> str:
        return "ECUC-MODULE-CONFIGURATION-VALUES"
    
    def add_container(self, container: EcucContainerValue) -> 'EcucModuleConfigurationValues':
        """添加容器"""
        self.containers.append(container)
        return self
    
    def get_container_by_name(self, name: str) -> Optional[EcucContainerValue]:
        """通过短名称获取容器"""
        for container in self.containers:
            if container.short_name == name:
                return container
        return None
    
    def get_containers_by_def(self, def_path: str) -> List[EcucContainerValue]:
        """通过定义路径获取所有匹配的容器"""
        return [c for c in self.containers if c.definition_ref.value == def_path]


@dataclass
class EcucQuery:
    """ECUC查询对象 - 用于查询配置"""
    module_config: Optional[EcucModuleConfigurationValues] = None
    
    def find_container(self, path: str) -> Optional[EcucContainerValue]:
        """通过路径查找容器"""
        if not self.module_config:
            return None
        
        parts = path.split('/')
        current: Optional[EcucContainerValue] = None
        
        for part in parts:
            if not part:
                continue
            if current is None:
                current = self.module_config.get_container_by_name(part)
            else:
                current = current.get_sub_container_by_name(part)
            
            if current is None:
                return None
        
        return current
    
    def find_parameter(self, container_path: str, param_def: str) -> Optional[EcucParamValue]:
        """在指定容器路径下查找参数"""
        container = self.find_container(container_path)
        if container:
            return container.get_parameter_by_def(param_def)
        return None


# =============================================================================
# 便捷工厂函数
# =============================================================================

def create_definition_ref(dest: str, value: str) -> EcucDefinitionRef:
    """创建定义引用"""
    return EcucDefinitionRef(dest=dest, value=value)


def create_boolean_param(def_path: str, value: bool, dest: str = "ECUC-BOOLEAN-PARAM-DEF") -> EcucBooleanParamValue:
    """创建布尔参数"""
    def_ref = create_definition_ref(dest, def_path)
    return EcucBooleanParamValue(def_ref, value)


def create_integer_param(def_path: str, value: int, dest: str = "ECUC-INTEGER-PARAM-DEF") -> EcucIntegerParamValue:
    """创建整数参数"""
    def_ref = create_definition_ref(dest, def_path)
    return EcucIntegerParamValue(def_ref, value)


def create_float_param(def_path: str, value: float, dest: str = "ECUC-FLOAT-PARAM-DEF") -> EcucFloatParamValue:
    """创建浮点数参数"""
    def_ref = create_definition_ref(dest, def_path)
    return EcucFloatParamValue(def_ref, value)


def create_string_param(def_path: str, value: str, dest: str = "ECUC-STRING-PARAM-DEF") -> EcucStringParamValue:
    """创建字符串参数"""
    def_ref = create_definition_ref(dest, def_path)
    return EcucStringParamValue(def_ref, value)


def create_enum_param(def_path: str, value: str, dest: str = "ECUC-ENUMERATION-PARAM-DEF") -> EcucEnumParamValue:
    """创建枚举参数"""
    def_ref = create_definition_ref(dest, def_path)
    return EcucEnumParamValue(def_ref, value)


def create_function_name_param(def_path: str, value: str, dest: str = "ECUC-FUNCTION-NAME-DEF") -> EcucFunctionNameParamValue:
    """创建函数名参数"""
    def_ref = create_definition_ref(dest, def_path)
    return EcucFunctionNameParamValue(def_ref, value)


def create_reference_value(def_path: str, value_ref: str, dest: str = "ECUC-REFERENCE-DEF") -> EcucReferenceValue:
    """创建引用值"""
    def_ref = create_definition_ref(dest, def_path)
    return EcucReferenceValue(def_ref, value_ref)


def create_container(
    def_path: str,
    short_name: str,
    dest: str = "ECUC-CONTAINER-DEF",
    params: Optional[List[EcucParamValue]] = None,
    refs: Optional[List[EcucRefValue]] = None,
    sub_containers: Optional[List[EcucContainerValue]] = None
) -> EcucContainerValue:
    """创建容器"""
    def_ref = create_definition_ref(dest, def_path)
    return EcucContainerValue(
        definition_ref=def_ref,
        short_name=short_name,
        parameter_values=params or [],
        reference_values=refs or [],
        sub_containers=sub_containers or []
    )


def create_module_config(
    short_name: str,
    module_def_path: str,
    package_name: str = "ECUC",
    config_variant: str = "VARIANT-PRE-COMPILE"
) -> EcucModuleConfigurationValues:
    """创建模块配置"""
    def_ref = create_definition_ref("ECUC-MODULE-DEF", module_def_path)
    return EcucModuleConfigurationValues(
        short_name=short_name,
        definition_ref=def_ref,
        package_name=package_name,
        implementation_config_variant=config_variant
    )
