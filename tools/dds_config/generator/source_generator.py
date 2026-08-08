"""
DDS Config C Source Generator
生成dds_generated_config.c源文件
"""

from pathlib import Path
from typing import Dict, List, Optional, Any
from dataclasses import dataclass
from datetime import datetime

from .template_engine import TemplateEngine, CodeStyle
from .code_optimizer import (
    CodeOptimizer, CommentGenerator, NamingConvention,
    SystemConfig, DomainConfig, ParticipantConfig, QoSProfile, TopicConfig
)


@dataclass
class SourceGenerationOptions:
    """源文件生成选项"""
    generate_static_init: bool = True  # 静态初始化vs动态初始化
    generate_error_check: bool = True  # 生成错误检查代码
    generate_callbacks: bool = True    # 生成默认回调函数
    use_const_qos: bool = True         # QoS使用const存储
    max_init_functions: int = 10       # 最大初始化函数数量
    indent_size: int = 4


class SourceGenerator:
    """DDS配置C源文件生成器"""

    DEFAULT_TEMPLATE = "c_source.j2"

    def __init__(self,
                 template_engine: Optional[TemplateEngine] = None,
                 optimizer: Optional[CodeOptimizer] = None,
                 options: Optional[SourceGenerationOptions] = None,
                 code_style: Optional[CodeStyle] = None):
        """
        初始化源文件生成器

        Args:
            template_engine: 模板引擎实例
            optimizer: 代码优化器实例
            options: 生成选项
            code_style: 代码风格配置
        """
        self.template_engine = template_engine or TemplateEngine(code_style=code_style)
        self.optimizer = optimizer or CodeOptimizer()
        self.options = options or SourceGenerationOptions()
        self.naming = NamingConvention()
        self.comment_gen = CommentGenerator()

    def generate(self, config: SystemConfig, header_path: str = "dds_generated_config.h") -> str:
        """
        生成源文件内容

        Args:
            config: 系统配置
            header_path: 对应头文件路径

        Returns:
            生成的C源文件内容
        """
        # 优化配置
        optimized_config = self.optimizer.optimize(config)

        # 构建模板上下文
        context = self._build_context(optimized_config, header_path)

        # 渲染模板
        try:
            content = self.template_engine.render(self.DEFAULT_TEMPLATE, context)
        except Exception:
            # 如果模板不存在，使用内置默认模板
            content = self._generate_with_builtin_template(context)

        return content

    def _build_context(self, config: SystemConfig, header_path: str) -> Dict[str, Any]:
        """构建模板上下文"""
        context = {
            # 文件信息
            'filename': "dds_generated_config.c",
            'header_path': header_path,
            'description': f"DDS Configuration Implementation for {config.name}",
            'version': config.version,
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),

            # 系统信息
            'system_name': config.name,
            'system_version': config.version,

            # QoS配置实例化
            'qos_definitions': self._generate_qos_definitions(config.qos_profiles),

            # DomainParticipant初始化
            'participant_init': self._generate_participant_init(config.domains),

            # Topic注册
            'topic_registrations': self._generate_topic_registrations(config),

            # Publisher配置
            'publisher_configs': self._generate_publisher_configs(config),

            # Subscriber配置
            'subscriber_configs': self._generate_subscriber_configs(config),

            # 回调函数定义
            'callback_functions': self._generate_callback_functions(config),

            # 初始化/反初始化函数实现
            'init_function_impl': self._generate_init_function(config),
            'deinit_function_impl': self._generate_deinit_function(config),

            # 生成选项
            'options': self.options,
        }

        return context

    def _generate_qos_definitions(self, qos_profiles: Dict[str, QoSProfile]) -> List[Dict[str, Any]]:
        """生成QoS配置定义"""
        definitions = []
        for name, profile in qos_profiles.items():
            var_name = self.naming.to_variable_name(name, "g_")
            definitions.append({
                'name': var_name,
                'type': 'dds_qos_t',
                'is_const': self.options.use_const_qos,
                'reliability': self._map_reliability(profile.reliability),
                'durability': self._map_durability(profile.durability),
                'deadline_ms': profile.deadline_sec * 1000 + profile.deadline_nanosec // 1000000,
                'latency_budget_ms': profile.latency_budget_ms,
                'history_depth': profile.history_depth,
                'description': f"QoS Profile: {name}"
            })
        return definitions

    def _generate_participant_init(self, domains: List[DomainConfig]) -> List[Dict[str, Any]]:
        """生成DomainParticipant初始化代码"""
        inits = []
        for domain in domains:
            for participant in domain.participants:
                var_name = self.naming.to_variable_name(participant.name, "g_")
                inits.append({
                    'variable': var_name,
                    'domain_id_macro': self.naming.to_macro_name(domain.name, "DDS_DOMAIN_ID"),
                    'qos_profile': participant.qos_profile,
                    'description': f"Initialize participant {participant.name}"
                })
        return inits

    def _generate_topic_registrations(self, config: SystemConfig) -> List[Dict[str, Any]]:
        """生成Topic注册代码"""
        registrations = []
        seen_topics: set = set()

        for domain in config.domains:
            for participant in domain.participants:
                participant_var = self.naming.to_variable_name(participant.name, "g_")

                for topic in participant.publishers + participant.subscribers:
                    if topic.name not in seen_topics:
                        topic_macro = self.naming.to_macro_name(topic.name, "DDS_TOPIC")
                        var_name = self.naming.to_variable_name(f"topic_{topic.name}", "g_")

                        registrations.append({
                            'variable': var_name,
                            'participant': participant_var,
                            'topic_macro': topic_macro,
                            'type_name': topic.type_name,
                            'qos_profile': topic.qos_profile,
                            'description': f"Register topic {topic.name}"
                        })
                        seen_topics.add(topic.name)

        return registrations

    def _generate_publisher_configs(self, config: SystemConfig) -> List[Dict[str, Any]]:
        """生成Publisher配置"""
        configs = []
        for domain in config.domains:
            for participant in domain.participants:
                participant_var = self.naming.to_variable_name(participant.name, "g_")

                for topic in participant.publishers:
                    topic_macro = self.naming.to_macro_name(topic.name, "DDS_TOPIC")
                    var_name = self.naming.to_variable_name(f"pub_{topic.name}", "g_")

                    configs.append({
                        'variable': var_name,
                        'participant': participant_var,
                        'topic_macro': topic_macro,
                        'qos_profile': topic.qos_profile,
                        'description': f"Publisher for {topic.name}"
                    })

        return configs

    def _generate_subscriber_configs(self, config: SystemConfig) -> List[Dict[str, Any]]:
        """生成Subscriber配置"""
        configs = []
        for domain in config.domains:
            for participant in domain.participants:
                participant_var = self.naming.to_variable_name(participant.name, "g_")

                for topic in participant.subscribers:
                    topic_macro = self.naming.to_macro_name(topic.name, "DDS_TOPIC")
                    var_name = self.naming.to_variable_name(f"sub_{topic.name}", "g_")

                    configs.append({
                        'variable': var_name,
                        'participant': participant_var,
                        'topic_macro': topic_macro,
                        'qos_profile': topic.qos_profile,
                        'description': f"Subscriber for {topic.name}"
                    })

        return configs

    def _generate_callback_functions(self, config: SystemConfig) -> List[Dict[str, Any]]:
        """生成默认回调函数"""
        if not self.options.generate_callbacks:
            return []

        callbacks = []

        # 数据接收回调
        callbacks.append({
            'name': 'dds_generated_on_data_received',
            'params': 'const void *data, uint32_t size, void *user_data',
            'return_type': 'void',
            'description': 'Default data received callback',
            'body': '''    /* MISRA C:2012 Deviation - user_data may be unused */
    (void)user_data;
    
    /* TODO: Implement data processing */
    if ((data != NULL) && (size > 0U))
    {
        /* Process received data */
    }'''
        })

        # 写入完成回调
        callbacks.append({
            'name': 'dds_generated_on_write_complete',
            'params': 'void *user_data',
            'return_type': 'void',
            'description': 'Default write complete callback',
            'body': '''    /* MISRA C:2012 Deviation - user_data may be unused */
    (void)user_data;
    
    /* TODO: Implement write completion handling */'''
        })

        # Participant连接状态回调
        callbacks.append({
            'name': 'dds_generated_on_participant_connected',
            'params': 'dds_domain_participant_t *participant, void *user_data',
            'return_type': 'void',
            'description': 'Default participant connected callback',
            'body': '''    /* MISRA C:2012 Deviation - parameters may be unused */
    (void)participant;
    (void)user_data;
    
    /* TODO: Implement connection handling */'''
        })

        return callbacks

    def _generate_init_function(self, config: SystemConfig) -> Dict[str, Any]:
        """生成初始化函数实现"""
        body_lines = []
        indent = " " * self.options.indent_size

        # 函数开始
        body_lines.append(f"{indent}eth_status_t status = ETH_OK;")
        body_lines.append(f"{indent}(void)status; /* 可能未使用 */")
        body_lines.append("")

        # 初始化DDS运行时
        body_lines.append(f"{indent}/* Initialize DDS runtime */")
        body_lines.append(f"{indent}status = dds_runtime_init();")
        if self.options.generate_error_check:
            body_lines.append(f"{indent}if (status != ETH_OK)")
            body_lines.append(f"{indent}{{")
            body_lines.append(f"{indent}{indent}return status;")
            body_lines.append(f"{indent}}}")
        body_lines.append("")

        # 创建Domain Participants
        body_lines.append(f"{indent}/* Create Domain Participants */")
        for domain in config.domains:
            for participant in domain.participants:
                var_name = self.naming.to_variable_name(participant.name, "g_")
                domain_macro = self.naming.to_macro_name(domain.name, "DDS_DOMAIN_ID")

                body_lines.append(f"{indent}{var_name} = dds_create_participant({domain_macro});")
                if self.options.generate_error_check:
                    body_lines.append(f"{indent}if ({var_name} == NULL)")
                    body_lines.append(f"{indent}{{")
                    body_lines.append(f"{indent}{indent}(void)dds_deinit_generated_config();")
                    body_lines.append(f"{indent}{indent}return ETH_ERROR;")
                    body_lines.append(f"{indent}}}")
        body_lines.append("")

        # 设置Participant QoS
        body_lines.append(f"{indent}/* Configure Participant QoS */")
        for domain in config.domains:
            for participant in domain.participants:
                var_name = self.naming.to_variable_name(participant.name, "g_")
                qos_var = self.naming.to_variable_name(participant.qos_profile, "g_")

                body_lines.append(f"{indent}(void)dds_set_participant_qos({var_name}, &{qos_var});")
        body_lines.append("")

        # 返回成功
        body_lines.append(f"{indent}return ETH_OK;")

        return {
            'name': 'dds_init_generated_config',
            'return_type': 'eth_status_t',
            'parameters': 'void',
            'description': 'Initialize all DDS generated configurations',
            'body': '\n'.join(body_lines)
        }

    def _generate_deinit_function(self, config: SystemConfig) -> Dict[str, Any]:
        """生成反初始化函数实现"""
        body_lines = []
        indent = " " * self.options.indent_size

        body_lines.append(f"{indent}/* Delete Domain Participants in reverse order */")

        # 收集所有participants并反序删除
        all_participants = []
        for domain in config.domains:
            for participant in domain.participants:
                all_participants.append(participant)

        for participant in reversed(all_participants):
            var_name = self.naming.to_variable_name(participant.name, "g_")
            body_lines.append(f"{indent}if ({var_name} != NULL)")
            body_lines.append(f"{indent}{{")
            body_lines.append(f"{indent}{indent}(void)dds_delete_participant({var_name});")
            body_lines.append(f"{indent}{indent}{var_name} = NULL;")
            body_lines.append(f"{indent}}}")

        body_lines.append("")
        body_lines.append(f"{indent}/* Deinitialize DDS runtime */")
        body_lines.append(f"{indent}dds_runtime_deinit();")

        return {
            'name': 'dds_deinit_generated_config',
            'return_type': 'void',
            'parameters': 'void',
            'description': 'Deinitialize all DDS generated configurations',
            'body': '\n'.join(body_lines)
        }

    def _generate_with_builtin_template(self, context: Dict[str, Any]) -> str:
        """使用内置默认模板生成"""
        lines = []
        indent = " " * self.options.indent_size

        # 文件头部注释
        lines.append(self.comment_gen.generate_file_header(
            context['filename'],
            context['description'],
            version=context['version']
        ))

        # 头文件包含
        lines.append(f"#include \"{context['header_path']}\"")
        lines.append("")

        # QoS配置定义
        if context['qos_definitions']:
            lines.append(self.comment_gen.generate_section_comment("QoS Profile Definitions"))
            for qos in context['qos_definitions']:
                const_kw = "const " if qos['is_const'] else ""
                lines.append(f"/** @brief {qos['description']} */")
                lines.append(f"{const_kw}{qos['type']} {qos['name']} = {{")
                lines.append(f"{indent}.reliability = {qos['reliability']},")
                lines.append(f"{indent}.durability = {qos['durability']},")
                lines.append(f"{indent}.deadline_ms = {qos['deadline_ms']}U,")
                lines.append(f"{indent}.latency_budget_ms = {qos['latency_budget_ms']}U,")
                lines.append(f"{indent}.history_depth = {qos['history_depth']}U")
                lines.append("};")
                lines.append("")

        # DomainParticipant定义
        if context['participant_init']:
            lines.append(self.comment_gen.generate_section_comment("Domain Participant Instances"))
            for init in context['participant_init']:
                lines.append(f"/** @brief {init['description']} */")
                lines.append(f"dds_domain_participant_t *{init['variable']} = NULL;")
                lines.append("")

        # 回调函数定义
        if context['callback_functions']:
            lines.append(self.comment_gen.generate_section_comment("Default Callback Functions"))
            for cb in context['callback_functions']:
                lines.append(self.comment_gen.generate_doxygen_comment(
                    cb['description'],
                    returns="void"
                ))
                lines.append(f"static {cb['return_type']} {cb['name']}({cb['params']})")
                lines.append("{")
                lines.append(cb['body'])
                lines.append("}")
                lines.append("")

        # 初始化函数
        init = context['init_function_impl']
        lines.append(self.comment_gen.generate_section_comment("Initialization Functions"))
        lines.append(self.comment_gen.generate_doxygen_comment(
            init['description'],
            returns="eth_status_t ETH_OK on success, error code on failure"
        ))
        lines.append(f"{init['return_type']} {init['name']}({init['parameters']})")
        lines.append("{")
        lines.append(init['body'])
        lines.append("}")
        lines.append("")

        # 反初始化函数
        deinit = context['deinit_function_impl']
        lines.append(self.comment_gen.generate_doxygen_comment(deinit['description']))
        lines.append(f"{deinit['return_type']} {deinit['name']}({deinit['parameters']})")
        lines.append("{")
        lines.append(deinit['body'])
        lines.append("}")
        lines.append("")

        return '\n'.join(lines)

    def save(self, content: str, output_path: Path) -> None:
        """
        保存生成的源文件

        Args:
            content: 文件内容
            output_path: 输出路径
        """
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(content, encoding='utf-8')

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
