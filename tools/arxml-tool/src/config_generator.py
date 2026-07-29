#!/usr/bin/env python3
"""
ARXML Configuration Generator
Generates AUTOSAR BSW configuration files from ARXML system description

从ARXML解析结果生成AUTOSAR BSW配置文件:
- ECUC配置 (EcucModuleConfigurationValues)
- BSW配置头文件 (.h)
- BSW配置源文件 (_Cfg.c)
- 链接时配置 (_Lcfg.c)
"""

import sys
import argparse
import json
import xml.etree.ElementTree as ET
from pathlib import Path
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Union, Any
from datetime import datetime
from jinja2 import Environment, FileSystemLoader, Template
import logging

# 配置日志
logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
logger = logging.getLogger(__name__)


@dataclass
class EcucParameter:
    """ECUC参数定义"""
    name: str
    value: Any
    type: str = "STRING"  # STRING, BOOLEAN, INTEGER, FLOAT, ENUM, FUNCTION-NAME
    definition: str = ""
    description: str = ""


@dataclass
class EcucContainer:
    """ECUC容器定义"""
    name: str
    definition: str = ""
    parameters: List[EcucParameter] = field(default_factory=list)
    sub_containers: List['EcucContainer'] = field(default_factory=list)
    description: str = ""


@dataclass
class ModuleConfig:
    """模块配置定义"""
    name: str
    module_def: str
    containers: List[EcucContainer] = field(default_factory=list)
    description: str = ""


class ConfigTemplates:
    """配置文件模板管理器"""

    # 配置头文件模板
    CFG_H_TEMPLATE = '''/*
 * {{ module_name }}_Cfg.h
 * 
 * 自动生成的配置文件 - {{ timestamp }}
 * 来源: {{ source_arxml }}
 */

#ifndef {{ module_name.upper() }}_CFG_H
#define {{ module_name.upper() }}_CFG_H

/*==================[inclusions]=============================================*/
#include "Std_Types.h"

/*==================[macros]=================================================*/
{% for param in global_params %}
/** {{ param.description }} */
#define {{ param.name }} {{ param.value }}
{% endfor %}

/*==================[type definitions]=======================================*/
{% for type_def in type_definitions %}
/** {{ type_def.description }} */
typedef {{ type_def.base_type }} {{ type_def.name }};
{% endfor %}

/*==================[external data declarations]=============================*/
{% for extern in extern_declarations %}
extern {{ extern.type }} {{ extern.name }};
{% endfor %}

/*==================[external function declarations]=========================*/
{% for func in function_declarations %}
/** {{ func.description }} */
extern {{ func.return_type }} {{ func.name }}({{ func.params }});
{% endfor %}

#endif /* {{ module_name.upper() }}_CFG_H */
'''

    # 配置源文件模板
    CFG_C_TEMPLATE = '''/*
 * {{ module_name }}_Cfg.c
 * 
 * 自动生成的配置源文件 - {{ timestamp }}
 * 来源: {{ source_arxml }}
 */

#include "{{ module_name }}_Cfg.h"
{% for include in additional_includes %}
#include "{{ include }}"
{% endfor %}

/*==================[internal data]==========================================*/
{% for data in internal_data %}
/** {{ data.description }} */
static {{ data.type }} {{ data.name }} = {{ data.initializer }};
{% endfor %}

/*==================[external data definitions]==============================*/
{% for data in external_data %}
/** {{ data.description }} */
{{ data.type }} {{ data.name }} = {{ data.initializer }};
{% endfor %}
'''

    # 链接时配置模板 (_Lcfg.c)
    LCFG_C_TEMPLATE = '''/*
 * {{ module_name }}_Lcfg.c
 * 
 * 链接时配置 - {{ timestamp }}
 * 来源: {{ source_arxml }}
 * 
 * 此文件包含可在链接时配置的静态数据表
 */

#include "{{ module_name }}_Cfg.h"
{% for include in additional_includes %}
#include "{{ include }}"
{% endfor %}

/*==================[version check]==========================================*/
#define {{ module_name.upper() }}_LCFG_SW_MAJOR_VERSION {{ version.major }}
#define {{ module_name.upper() }}_LCFG_SW_MINOR_VERSION {{ version.minor }}
#define {{ module_name.upper() }}_LCFG_SW_PATCH_VERSION {{ version.patch }}

/*==================[link-time configuration tables]=========================*/
{% for table in config_tables %}
/** {{ table.description }} */
const {{ table.element_type }} {{ table.name }}[{{ table.size }}] = {
{% for row in table.rows %}
    { {% for col in row %}{{ col }}{% if not loop.last %}, {% endif %}{% endfor %} }{% if not loop.last %},{% endif %}
{% endfor %}
};
{% endfor %}

/*==================[configuration pointer]==================================*/
{% if config_pointer %}
/** 配置数据指针 - 运行时指向此配置 */
const {{ module_name }}_ConfigType* {{ module_name }}_ConfigPtr = &{{ config_pointer.name }};
{% endif %}
'''

    # PBcfg模板 (每芯片配置)
    PB_CFG_C_TEMPLATE = '''/*
 * {{ module_name }}_PBcfg.c
 * 
 * 邮编配置 - {{ timestamp }}
 * 针对: {{ ecu_name }}
 */

#include "{{ module_name }}_Cfg.h"

/*==================[post-build configuration]===============================*/
{% for cfg in post_build_configs %}
/** {{ cfg.description }} */
const {{ cfg.type }} {{ cfg.name }} = {
{% for member in cfg.members %}
    .{{ member.name }} = {{ member.value }}{% if not loop.last %},{% endif %}
{% endfor %}
};
{% endfor %}
'''

    # ECUC ARXML模板
    ECUC_ARXML_TEMPLATE = '''<?xml version="1.0" encoding="UTF-8"?>
<AUTOSAR xmlns="http://autosar.org/schema/r4.0">
  <ADMIN-DATA>
    <LANGUAGE>EN</LANGUAGE>
  </ADMIN-DATA>
  <AR-PACKAGES>
    <AR-PACKAGE>
      <SHORT-NAME>EcucModuleConfiguration</SHORT-NAME>
      <ELEMENTS>
        <ECUC-MODULE-CONFIGURATION-VALUES>
          <SHORT-NAME>{{ module_name }}</SHORT-NAME>
          <DEFINITION-REF DEST="ECUC-MODULE-DEF">/{{ module_def }}</DEFINITION-REF>
          <IMPLEMENTATION-CONFIG-VARIANT>VARIANT-LINK-TIME</IMPLEMENTATION-CONFIG-VARIANT>
          <MODULE-DESCRIPTION-REF DEST="BSW-IMPLEMENTATION">/BSW/{{ module_name }}</MODULE-DESCRIPTION-REF>
          {% for container in containers %}
          <CONTAINERS>
            <ECUC-CONTAINER-VALUE>
              <SHORT-NAME>{{ container.name }}</SHORT-NAME>
              <DEFINITION-REF DEST="ECUC-PARAM-CONF-CONTAINER-DEF">/{{ container.definition }}</DEFINITION-REF>
              {% for param in container.parameters %}
              <PARAMETER-VALUES>
                <ECUC-{% if param.type == 'FUNCTION-NAME' %}FUNCTION-NAME-{% endif %}DEF-{{ 'REF' if param.type == 'ENUM' else 'VALUE' }}>
                  <DEFINITION-REF DEST="ECUC-{{ param.type }}-PARAM-DEF">/{{ param.definition }}</DEFINITION-REF>
                  <VALUE>{{ param.value }}</VALUE>
                </ECUC-{% if param.type == 'FUNCTION-NAME' %}FUNCTION-NAME-{% endif %}DEF-{{ 'REF' if param.type == 'ENUM' else 'VALUE' }}>
              </PARAMETER-VALUES>
              {% endfor %}
            </ECUC-CONTAINER-VALUE>
          </CONTAINERS>
          {% endfor %}
        </ECUC-MODULE-CONFIGURATION-VALUES>
      </ELEMENTS>
    </AR-PACKAGE>
  </AR-PACKAGES>
</AUTOSAR>
'''


class ConfigGenerator:
    """
    AUTOSAR BSW配置文件生成器
    从ARXML或JSON配置生成C代码和ARXML配置文件
    """

    def __init__(self, output_dir: Union[str, Path], template_dir: Optional[Path] = None):
        """
        初始化配置生成器
        
        Args:
            output_dir: 输出目录路径
            template_dir: 自定义模板目录 (可选)
        """
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # 设置Jinja2环境
        if template_dir and template_dir.exists():
            self.env = Environment(loader=FileSystemLoader(template_dir))
        else:
            # 使用内置模板
            self.env = Environment(loader=FileSystemLoader('.'))
        
        self.templates = ConfigTemplates()
        self.generated_files: List[Path] = []
        
    def _get_timestamp(self) -> str:
        """获取当前时间戳"""
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    def generate_from_arxml(self, arxml_path: Union[str, Path], 
                           module_name: Optional[str] = None) -> List[Path]:
        """
        从ARXML文件生成配置文件
        
        Args:
            arxml_path: ARXML文件路径
            module_name: 模块名 (可选，默认从文件推断)
            
        Returns:
            生成的文件路径列表
        """
        arxml_path = Path(arxml_path)
        if not arxml_path.exists():
            raise FileNotFoundError(f"ARXML文件不存在: {arxml_path}")
        
        # 解析ARXML
        logger.info(f"解析ARXML: {arxml_path}")
        tree = ET.parse(arxml_path)
        root = tree.getroot()
        
        # 提取命名空间
        ns = {'autosar': 'http://autosar.org/schema/r4.0'}
        
        # 获取模块配置
        if not module_name:
            # 尝试从ARXML推断模块名
            module_elem = root.find('.//autosar:ECUC-MODULE-CONFIGURATION-VALUES', ns)
            if module_elem is not None:
                name_elem = module_elem.find('autosar:SHORT-NAME', ns)
                if name_elem is not None:
                    module_name = name_elem.text
        
        if not module_name:
            module_name = arxml_path.stem
        
        # 提取ECUC配置
        config = self._extract_ecuc_config(root, ns, module_name)
        
        # 生成配置文件
        return self.generate_module_config(config, str(arxml_path))

    def _extract_ecuc_config(self, root: ET.Element, ns: Dict[str, str], 
                            module_name: str) -> ModuleConfig:
        """从ARXML提取ECUC配置"""
        config = ModuleConfig(name=module_name, module_def=f"{module_name}")
        
        # 查找所有容器
        for container in root.findall('.//autosar:ECUC-CONTAINER-VALUE', ns):
            name_elem = container.find('autosar:SHORT-NAME', ns)
            if name_elem is None:
                continue
            
            container_name = name_elem.text
            def_ref = container.find('autosar:DEFINITION-REF', ns)
            container_def = def_ref.text if def_ref is not None else ""
            
            ecuc_container = EcucContainer(name=container_name, definition=container_def)
            
            # 提取参数
            for param in container.findall('.//autosar:PARAMETER-VALUES/*', ns):
                param_def = param.find('autosar:DEFINITION-REF', ns)
                param_value = param.find('autosar:VALUE', ns)
                
                if param_def is not None and param_value is not None:
                    # 推断参数类型
                    param_type = self._infer_param_type(param_value.text)
                    
                    param_obj = EcucParameter(
                        name=param_def.text.split('/')[-1] if param_def.text else "",
                        value=param_value.text,
                        type=param_type,
                        definition=param_def.text if param_def.text else ""
                    )
                    ecuc_container.parameters.append(param_obj)
            
            config.containers.append(ecuc_container)
        
        return config

    def _infer_param_type(self, value: str) -> str:
        """推断参数类型"""
        if value is None:
            return "STRING"
        
        value = value.strip()
        
        # 检查布尔值
        if value.lower() in ('true', 'false'):
            return "BOOLEAN"
        
        # 检查整数
        try:
            int(value)
            return "INTEGER"
        except ValueError:
            pass
        
        # 检查浮点数
        try:
            float(value)
            return "FLOAT"
        except ValueError:
            pass
        
        # 检查枚举 (使用大写字母和下划线)
        if value.isupper() and '_' in value:
            return "ENUM"
        
        # 检查函数名 (以Callback结尾或包含特定模式)
        if value.endswith('Callback') or value.endswith('Notification'):
            return "FUNCTION-NAME"
        
        return "STRING"

    def generate_from_json(self, json_path: Union[str, Path]) -> List[Path]:
        """
        从JSON配置文件生成
        
        Args:
            json_path: JSON配置文件路径
            
        Returns:
            生成的文件路径列表
        """
        json_path = Path(json_path)
        with open(json_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        # 解析JSON为ModuleConfig
        config = self._parse_json_config(data)
        
        # 生成配置文件
        return self.generate_module_config(config, str(json_path))

    def _parse_json_config(self, data: Dict) -> ModuleConfig:
        """解析JSON配置"""
        config = ModuleConfig(
            name=data.get('name', 'Unknown'),
            module_def=data.get('module_def', ''),
            description=data.get('description', '')
        )
        
        for container_data in data.get('containers', []):
            container = EcucContainer(
                name=container_data.get('name', ''),
                definition=container_data.get('definition', ''),
                description=container_data.get('description', '')
            )
            
            for param_data in container_data.get('parameters', []):
                param = EcucParameter(
                    name=param_data.get('name', ''),
                    value=param_data.get('value'),
                    type=param_data.get('type', 'STRING'),
                    definition=param_data.get('definition', ''),
                    description=param_data.get('description', '')
                )
                container.parameters.append(param)
            
            config.containers.append(container)
        
        return config

    def generate_module_config(self, config: ModuleConfig, 
                              source: str = "") -> List[Path]:
        """
        生成模块配置文件套件
        
        Args:
            config: 模块配置对象
            source: 来源文件路径 (用于注释)
            
        Returns:
            生成的文件路径列表
        """
        self.generated_files = []
        module_name = config.name
        
        logger.info(f"生成模块配置: {module_name}")
        
        # 生成头文件
        self._generate_cfg_h(config, source)
        
        # 生成源文件
        self._generate_cfg_c(config, source)
        
        # 生成链接时配置
        self._generate_lcfg_c(config, source)
        
        # 生成ECUC ARXML
        self._generate_ecuc_arxml(config, source)
        
        return self.generated_files

    def _generate_cfg_h(self, config: ModuleConfig, source: str):
        """生成配置头文件"""
        template = Template(self.templates.CFG_H_TEMPLATE)
        
        # 准备模板数据
        global_params = []
        for container in config.containers:
            for param in container.parameters:
                # 转换值为C宏定义
                value = self._convert_to_c_macro_value(param)
                global_params.append({
                    'name': f"{config.name.upper()}_{container.name.upper()}_{param.name.upper()}",
                    'value': value,
                    'description': f"{param.definition}"
                })
        
        content = template.render(
            module_name=config.name,
            timestamp=self._get_timestamp(),
            source_arxml=source,
            global_params=global_params,
            type_definitions=[],  # 可根据需要扩展
            extern_declarations=[],
            function_declarations=[]
        )
        
        output_file = self.output_dir / f"{config.name}_Cfg.h"
        output_file.write_text(content, encoding='utf-8')
        self.generated_files.append(output_file)
        logger.info(f"  生成: {output_file}")

    def _generate_cfg_c(self, config: ModuleConfig, source: str):
        """生成配置源文件"""
        template = Template(self.templates.CFG_C_TEMPLATE)
        
        content = template.render(
            module_name=config.name,
            timestamp=self._get_timestamp(),
            source_arxml=source,
            additional_includes=[],
            internal_data=[],
            external_data=[]
        )
        
        output_file = self.output_dir / f"{config.name}_Cfg.c"
        output_file.write_text(content, encoding='utf-8')
        self.generated_files.append(output_file)
        logger.info(f"  生成: {output_file}")

    def _generate_lcfg_c(self, config: ModuleConfig, source: str):
        """生成链接时配置文件"""
        template = Template(self.templates.LCFG_C_TEMPLATE)
        
        # 构建配置表
        config_tables = []
        for container in config.containers:
            if container.parameters:
                rows = []
                for param in container.parameters:
                    value = self._convert_to_c_value(param)
                    rows.append([f'"{param.name}"', value, f'"{param.type}"'])
                
                config_tables.append({
                    'name': f"{config.name}_{container.name}_ConfigTable",
                    'element_type': 'EcucParameterConfigType',
                    'size': len(container.parameters),
                    'description': f"{container.name} configuration parameters",
                    'rows': rows
                })
        
        content = template.render(
            module_name=config.name,
            timestamp=self._get_timestamp(),
            source_arxml=source,
            version={'major': 1, 'minor': 0, 'patch': 0},
            config_tables=config_tables,
            config_pointer=None,
            additional_includes=['Ecuc_Types.h']
        )
        
        output_file = self.output_dir / f"{config.name}_Lcfg.c"
        output_file.write_text(content, encoding='utf-8')
        self.generated_files.append(output_file)
        logger.info(f"  生成: {output_file}")

    def _generate_ecuc_arxml(self, config: ModuleConfig, source: str):
        """生成ECUC ARXML配置文件"""
        template = Template(self.templates.ECUC_ARXML_TEMPLATE)
        
        content = template.render(
            module_name=config.name,
            module_def=config.module_def,
            containers=[
                {
                    'name': c.name,
                    'definition': c.definition,
                    'parameters': [
                        {
                            'name': p.name,
                            'value': p.value,
                            'type': p.type,
                            'definition': p.definition
                        }
                        for p in c.parameters
                    ]
                }
                for c in config.containers
            ]
        )
        
        output_file = self.output_dir / f"{config.name}_Config.arxml"
        output_file.write_text(content, encoding='utf-8')
        self.generated_files.append(output_file)
        logger.info(f"  生成: {output_file}")

    def _convert_to_c_macro_value(self, param: EcucParameter) -> str:
        """将参数值转换为C宏定义值"""
        if param.type == "BOOLEAN":
            return "TRUE" if param.value.lower() == "true" else "FALSE"
        elif param.type in ("INTEGER", "FLOAT"):
            return str(param.value)
        elif param.type == "STRING":
            return f'"{param.value}"'
        else:
            return str(param.value)

    def _convert_to_c_value(self, param: EcucParameter) -> str:
        """将参数值转换为C初始化值"""
        if param.type == "BOOLEAN":
            return "TRUE" if param.value.lower() == "true" else "FALSE"
        elif param.type == "STRING":
            return f'"{param.value}"'
        else:
            return str(param.value)

    def generate_batch(self, configs: List[ModuleConfig], 
                      source: str = "") -> Dict[str, List[Path]]:
        """
        批量生成多个模块配置
        
        Args:
            configs: 模块配置列表
            source: 来源文件路径
            
        Returns:
            每个模块生成的文件映射
        """
        results = {}
        for config in configs:
            files = self.generate_module_config(config, source)
            results[config.name] = files
        return results


def main():
    """命令行入口"""
    parser = argparse.ArgumentParser(
        description='ARXML Configuration Generator - 生成AUTOSAR BSW配置文件',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
使用示例:
  %(prog)s -i system.arxml -o ./generated
  %(prog)s -i config.json -o ./generated --format json
  %(prog)s -i system.arxml -o ./generated -m Can --verbose
        '''
    )
    
    parser.add_argument('-i', '--input', required=True, 
                       help='输入文件路径 (ARXML或JSON)')
    parser.add_argument('-o', '--output', required=True, 
                       help='输出目录路径')
    parser.add_argument('-f', '--format', choices=['arxml', 'json', 'auto'],
                       default='auto', help='输入文件格式 (默认自动检测)')
    parser.add_argument('-m', '--module', 
                       help='指定模块名 (仅用于ARXML)')
    parser.add_argument('-t', '--templates', 
                       help='自定义模板目录')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='详细输出')
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # 创建生成器
    template_dir = Path(args.templates) if args.templates else None
    generator = ConfigGenerator(args.output, template_dir)
    
    # 确定格式
    input_path = Path(args.input)
    if args.format == 'auto':
        if input_path.suffix.lower() == '.json':
            args.format = 'json'
        else:
            args.format = 'arxml'
    
    try:
        # 执行生成
        if args.format == 'json':
            generated = generator.generate_from_json(input_path)
        else:
            generated = generator.generate_from_arxml(input_path, args.module)
        
        print(f"\n✓ 成功生成 {len(generated)} 个文件:")
        for f in generated:
            print(f"  - {f}")
        
        return 0
        
    except Exception as e:
        logger.error(f"生成失败: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
