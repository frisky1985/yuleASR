"""
DDS代码生成器
生成符合microdds API的C代码
"""

import os
from pathlib import Path
from typing import Dict, List, Optional, Union
from jinja2 import Environment, FileSystemLoader, PackageLoader, select_autoescape

from .parser import (
    DDSConfiguration, DomainParticipantConfig, TopicConfig, TopicQos,
    ReliabilityQos, DurabilityQos, DeadlineQos, LatencyBudgetQos,
    LivelinessQos, HistoryQos, ResourceLimitsQos, LifespanQos
)


class DDSCodeGenerator:
    """DDS代码生成器"""

    def __init__(self, template_dir: Optional[Union[str, Path]] = None):
        """
        初始化代码生成器

        Args:
            template_dir: 自定义模板目录，为None时使用内置模板
        """
        if template_dir:
            self.env = Environment(
                loader=FileSystemLoader(template_dir),
                autoescape=select_autoescape(['html', 'xml']),
                trim_blocks=True,
                lstrip_blocks=True
            )
        else:
            # 使用包内模板
            self.env = Environment(
                loader=PackageLoader('dds_config_tool', 'templates'),
                autoescape=select_autoescape(['html', 'xml']),
                trim_blocks=True,
                lstrip_blocks=True
            )

        # 注册自定义过滤器
        self.env.filters['to_c_identifier'] = self._to_c_identifier
        self.env.filters['to_upper'] = self._to_upper
        self.env.filters['to_lower'] = self._to_lower

        self.generated_files: List[str] = []

    def generate(
        self,
        config: DDSConfiguration,
        output_dir: Union[str, Path],
        prefix: str = "dds"
    ) -> List[str]:
        """
        生成所有DDS配置代码

        Args:
            config: DDS配置对象
            output_dir: 输出目录
            prefix: 文件名前缀

        Returns:
            List[str]: 生成的文件列表
        """
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        self.generated_files = []

        # 准备模板上下文
        context = self._prepare_context(config, prefix)

        # 生成头文件
        self._generate_header(config, output_dir, prefix, context)

        # 生成域配置源文件
        self._generate_domain_config(config, output_dir, prefix, context)

        # 生成主题配置源文件
        self._generate_topic_config(config, output_dir, prefix, context)

        # 生成QoS配置源文件
        self._generate_qos_config(config, output_dir, prefix, context)

        return self.generated_files

    def _prepare_context(self, config: DDSConfiguration, prefix: str) -> Dict:
        """准备Jinja2模板上下文"""
        return {
            "config": config,
            "prefix": prefix,
            "prefix_upper": prefix.upper(),
            "prefix_lower": prefix.lower(),
            "header_guard": f"{prefix.upper()}_CONFIG_H",
        }

    def _generate_header(
        self,
        config: DDSConfiguration,
        output_dir: Path,
        prefix: str,
        context: Dict
    ):
        """生成头文件"""
        template = self.env.get_template("header.h.j2")
        content = template.render(**context)

        output_file = output_dir / f"{prefix}_config.h"
        with open(output_file, "w", encoding="utf-8") as f:
            f.write(content)

        self.generated_files.append(str(output_file))

    def _generate_domain_config(
        self,
        config: DDSConfiguration,
        output_dir: Path,
        prefix: str,
        context: Dict
    ):
        """生成域配置源文件"""
        template = self.env.get_template("domain_config.c.j2")
        content = template.render(**context)

        output_file = output_dir / f"{prefix}_domain_config.c"
        with open(output_file, "w", encoding="utf-8") as f:
            f.write(content)

        self.generated_files.append(str(output_file))

    def _generate_topic_config(
        self,
        config: DDSConfiguration,
        output_dir: Path,
        prefix: str,
        context: Dict
    ):
        """生成主题配置源文件"""
        template = self.env.get_template("topic_config.c.j2")
        content = template.render(**context)

        output_file = output_dir / f"{prefix}_topic_config.c"
        with open(output_file, "w", encoding="utf-8") as f:
            f.write(content)

        self.generated_files.append(str(output_file))

    def _generate_qos_config(
        self,
        config: DDSConfiguration,
        output_dir: Path,
        prefix: str,
        context: Dict
    ):
        """生成QoS配置源文件"""
        template = self.env.get_template("qos_config.c.j2")
        content = template.render(**context)

        output_file = output_dir / f"{prefix}_qos_config.c"
        with open(output_file, "w", encoding="utf-8") as f:
            f.write(content)

        self.generated_files.append(str(output_file))

    def generate_batch(
        self,
        config_files: List[Union[str, Path]],
        output_dir: Union[str, Path],
        prefix: str = "dds"
    ) -> Dict[str, List[str]]:
        """
        批量生成代码

        Args:
            config_files: 配置文件列表
            output_dir: 输出目录
            prefix: 文件名前缀

        Returns:
            Dict[str, List[str]]: 每个配置文件生成的文件列表
        """
        from .parser import DDSConfigParser
        from .validator import DDSConfigValidator

        output_dir = Path(output_dir)
        results = {}

        parser = DDSConfigParser()
        validator = DDSConfigValidator()

        for config_file in config_files:
            config_file = Path(config_file)

            # 解析配置
            try:
                config = parser.parse(config_file)
            except Exception as e:
                results[str(config_file)] = [f"解析失败: {e}"]
                continue

            # 验证配置
            is_valid, errors = validator.validate(config)
            if not is_valid:
                results[str(config_file)] = [f"验证失败: {e.message}" for e in errors]
                continue

            # 生成代码
            config_output_dir = output_dir / config_file.stem
            generated = self.generate(config, config_output_dir, prefix)
            results[str(config_file)] = generated

        return results

    def _to_c_identifier(self, value: str) -> str:
        """转换为C标识符"""
        if not value:
            return ""

        # 替换非法字符为下划线
        result = ""
        for char in value:
            if char.isalnum() or char == '_':
                result += char
            else:
                result += '_'

        # 确保以字母或下划线开头
        if result and not (result[0].isalpha() or result[0] == '_'):
            result = '_' + result

        return result

    def _to_upper(self, value: str) -> str:
        """转换为大写"""
        return value.upper()

    def _to_lower(self, value: str) -> str:
        """转换为小写"""
        return value.lower()

    def get_supported_templates(self) -> List[str]:
        """获取支持的模板列表"""
        try:
            return self.env.list_templates()
        except Exception:
            return []
