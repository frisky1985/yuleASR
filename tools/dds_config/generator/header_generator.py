"""
DDS Config C Header Generator
生成dds_generated_config.h头文件
"""

from pathlib import Path
from typing import Dict, List, Optional, Any
from dataclasses import dataclass, field
from datetime import datetime

from .template_engine import TemplateEngine, CodeStyle
from .code_optimizer import (
    CodeOptimizer, CommentGenerator, NamingConvention,
    SystemConfig, DomainConfig, ParticipantConfig, QoSProfile
)


@dataclass
class HeaderGenerationOptions:
    """头文件生成选项"""
    include_guard_prefix: str = "DDS_GENERATED"
    use_pragma_once: bool = False  # MISRA C:2012不推荐#pragma once
    generate_doxygen: bool = True
    generate_misra_comments: bool = True
    include_version_macro: bool = True
    include_timestamp: bool = True
    max_line_length: int = 80


class HeaderGenerator:
    """DDS配置C头文件生成器"""

    DEFAULT_TEMPLATE = "c_header.j2"

    def __init__(self,
                 template_engine: Optional[TemplateEngine] = None,
                 optimizer: Optional[CodeOptimizer] = None,
                 options: Optional[HeaderGenerationOptions] = None,
                 code_style: Optional[CodeStyle] = None):
        """
        初始化头文件生成器

        Args:
            template_engine: 模板引擎实例
            optimizer: 代码优化器实例
            options: 生成选项
            code_style: 代码风格配置
        """
        self.template_engine = template_engine or TemplateEngine(code_style=code_style)
        self.optimizer = optimizer or CodeOptimizer()
        self.options = options or HeaderGenerationOptions()
        self.naming = NamingConvention()
        self.comment_gen = CommentGenerator()

    def generate(self, config: SystemConfig, output_path: Optional[Path] = None) -> str:
        """
        生成头文件内容

        Args:
            config: 系统配置
            output_path: 输出路径（用于确定包含守卫）

        Returns:
            生成的C头文件内容
        """
        # 首先优化配置
        optimized_config = self.optimizer.optimize(config)

        # 构建模板上下文
        context = self._build_context(optimized_config, output_path)

        # 渲染模板
        try:
            content = self.template_engine.render(self.DEFAULT_TEMPLATE, context)
        except Exception:
            # 如果模板不存在，使用内置默认模板
            content = self._generate_with_builtin_template(context)

        return content

    def _build_context(self, config: SystemConfig, output_path: Optional[Path]) -> Dict[str, Any]:
        """构建模板上下文"""
        filename = output_path.name if output_path else "dds_generated_config.h"

        context = {
            # 文件信息
            'filename': filename,
            'description': f"DDS Configuration for {config.name}",
            'version': config.version,
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),

            # 包含守卫
            'include_guard': self.optimizer.generate_include_guard(filename),

            # 配置数据
            'system_name': config.name,
            'system_version': config.version,

            # Domain宏定义
            'domain_macros': self._generate_domain_macros(config.domains),

            # Topic宏定义
            'topic_macros': self._generate_topic_macros(config),

            # QoS Profile外部声明
            'qos_externs': self._generate_qos_externs(config.qos_profiles),

            # Participant外部声明
            'participant_externs': self._generate_participant_externs(config.domains),

            # 初始化/反初始化函数声明
            'init_function': 'dds_init_generated_config',
            'deinit_function': 'dds_deinit_generated_config',

            # 生成选项
            'options': self.options,

            # MISRA C合规标记
            'misra_compliant': True,
        }

        return context

    def _generate_domain_macros(self, domains: List[DomainConfig]) -> List[Dict[str, Any]]:
        """生成Domain ID宏定义"""
        macros = []
        for domain in domains:
            macro_name = self.naming.to_macro_name(domain.name, "DDS_DOMAIN_ID")
            macros.append({
                'name': macro_name,
                'value': domain.id,
                'description': f"Domain ID for {domain.name}"
            })
        return macros

    def _generate_topic_macros(self, config: SystemConfig) -> List[Dict[str, Any]]:
        """生成Topic名称宏定义"""
        macros = []
        seen_topics: set = set()

        for domain in config.domains:
            for participant in domain.participants:
                # Publisher topics
                for topic in participant.publishers:
                    if topic.name not in seen_topics:
                        macro_name = self.naming.to_macro_name(topic.name, "DDS_TOPIC")
                        macros.append({
                            'name': macro_name,
                            'value': topic.name,
                            'type_name': topic.type_name,
                            'domain_id': domain.id,
                            'description': f"Topic: {topic.name} ({topic.type_name})"
                        })
                        seen_topics.add(topic.name)

                # Subscriber topics
                for topic in participant.subscribers:
                    if topic.name not in seen_topics:
                        macro_name = self.naming.to_macro_name(topic.name, "DDS_TOPIC")
                        macros.append({
                            'name': macro_name,
                            'value': topic.name,
                            'type_name': topic.type_name,
                            'domain_id': domain.id,
                            'description': f"Topic: {topic.name} ({topic.type_name})"
                        })
                        seen_topics.add(topic.name)

        return macros

    def _generate_qos_externs(self, qos_profiles: Dict[str, QoSProfile]) -> List[Dict[str, Any]]:
        """生成QoS配置外部声明"""
        externs = []
        for name, profile in qos_profiles.items():
            var_name = self.naming.to_variable_name(name, "g_")
            externs.append({
                'name': var_name,
                'type': 'dds_qos_t',
                'description': f"QoS Profile: {name}",
                'qos': profile
            })
        return externs

    def _generate_participant_externs(self, domains: List[DomainConfig]) -> List[Dict[str, Any]]:
        """生成Participant外部声明"""
        externs = []
        for domain in domains:
            for participant in domain.participants:
                var_name = self.naming.to_variable_name(participant.name, "g_")
                externs.append({
                    'name': var_name,
                    'type': 'dds_domain_participant_t*',
                    'description': f"Domain Participant: {participant.name} (Domain {domain.id})"
                })
        return externs

    def _generate_with_builtin_template(self, context: Dict[str, Any]) -> str:
        """使用内置默认模板生成"""
        lines = []

        # 文件头部注释
        lines.append(self.comment_gen.generate_file_header(
            context['filename'],
            context['description'],
            version=context['version']
        ))

        # 包含守卫
        guard = context['include_guard']
        lines.append(f"#ifndef {guard}")
        lines.append(f"#define {guard}")
        lines.append("")

        # 系统头部包含
        lines.append("#include <stdint.h>")
        lines.append("#include <stdbool.h>")
        lines.append("#include \"dds/core/dds_core.h\"")
        lines.append("#include \"common/types/eth_types.h\"")
        lines.append("")

        # 版本宏定义
        if self.options.include_version_macro:
            lines.append(f"/* Generated for {context['system_name']} v{context['system_version']} */")
            lines.append(f"#define DDS_CONFIG_GENERATED_VERSION \"{context['system_version']}\"")
            lines.append("")

        # Domain ID宏定义
        if context['domain_macros']:
            lines.append(self.comment_gen.generate_section_comment("Domain ID Definitions"))
            for macro in context['domain_macros']:
                lines.append(f"/** @brief {macro['description']} */")
                lines.append(f"#define {macro['name']} ({macro['value']}U)")
                lines.append("")

        # Topic名称宏定义
        if context['topic_macros']:
            lines.append(self.comment_gen.generate_section_comment("Topic Name Definitions"))
            for macro in context['topic_macros']:
                lines.append(f"/** @brief {macro['description']} */")
                safe_value = macro['value'].replace('"', '\\"')
                lines.append(f"#define {macro['name']} \"{safe_value}\"")
                lines.append("")

        # QoS Profile外部声明
        if context['qos_externs']:
            lines.append(self.comment_gen.generate_section_comment("QoS Profile Declarations"))
            for extern in context['qos_externs']:
                lines.append(f"/** @brief {extern['description']} */")
                lines.append(f"extern const {extern['type']} {extern['name']};")
                lines.append("")

        # Participant外部声明
        if context['participant_externs']:
            lines.append(self.comment_gen.generate_section_comment("Domain Participant Declarations"))
            for extern in context['participant_externs']:
                lines.append(f"/** @brief {extern['description']} */")
                lines.append(f"extern {extern['type']} {extern['name']};")
                lines.append("")

        # 初始化/反初始化函数声明
        lines.append(self.comment_gen.generate_section_comment("Initialization Functions"))
        lines.append(self.comment_gen.generate_doxygen_comment(
            "Initialize all DDS generated configurations",
            returns="eth_status_t ETH_OK on success, error code on failure"
        ))
        lines.append(f"eth_status_t {context['init_function']}(void);")
        lines.append("")

        lines.append(self.comment_gen.generate_doxygen_comment(
            "Deinitialize all DDS generated configurations"
        ))
        lines.append(f"void {context['deinit_function']}(void);")
        lines.append("")

        # 包含守卫结束
        lines.append(f"#endif /* {guard} */")
        lines.append("")

        return '\n'.join(lines)

    def save(self, content: str, output_path: Path) -> None:
        """
        保存生成的头文件

        Args:
            content: 文件内容
            output_path: 输出路径
        """
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(content, encoding='utf-8')


class QoSStructGenerator:
    """QoS结构体定义生成器"""

    def generate_struct_definition(self, name: str, profile: QoSProfile) -> str:
        """生成单个QoS结构体定义"""
        lines = [
            f"/** @brief QoS Profile: {name} */",
            f"static const dds_qos_t {name} = {{",
            f"    .reliability = {self._map_reliability(profile.reliability)},",
            f"    .durability = {self._map_durability(profile.durability)},",
            f"    .deadline_ms = {profile.deadline_sec * 1000 + profile.deadline_nanosec // 1000000}U,",
            f"    .latency_budget_ms = {profile.latency_budget_ms}U,",
            f"    .history_depth = {profile.history_depth}U",
            "};",
            ""
        ]
        return '\n'.join(lines)

    def _map_reliability(self, reliability: str) -> str:
        """映射可靠性设置"""
        mapping = {
            'BEST_EFFORT': 'DDS_QOS_RELIABILITY_BEST_EFFORT',
            'RELIABLE': 'DDS_QOS_RELIABILITY_RELIABLE'
        }
        return mapping.get(reliability, 'DDS_QOS_RELIABILITY_BEST_EFFORT')

    def _map_durability(self, durability: str) -> str:
        """映射持久性设置"""
        mapping = {
            'VOLATILE': 'DDS_QOS_DURABILITY_VOLATILE',
            'TRANSIENT_LOCAL': 'DDS_QOS_DURABILITY_TRANSIENT_LOCAL',
            'TRANSIENT': 'DDS_QOS_DURABILITY_TRANSIENT',
            'PERSISTENT': 'DDS_QOS_DURABILITY_PERSISTENT'
        }
        return mapping.get(durability, 'DDS_QOS_DURABILITY_VOLATILE')
