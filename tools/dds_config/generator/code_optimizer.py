"""
DDS Config Code Optimizer
代码优化器：去除重复配置、合并QoS Profile、规范化命名
"""

from typing import Dict, List, Set, Any, Optional, Tuple
from dataclasses import dataclass, field
from collections import defaultdict
import hashlib
import re


@dataclass
class QoSProfile:
    """QoS配置概要"""
    name: str
    reliability: str = "BEST_EFFORT"
    durability: str = "VOLATILE"
    deadline_sec: int = 0
    deadline_nanosec: int = 0
    latency_budget_ms: int = 0
    history_kind: str = "KEEP_LAST"
    history_depth: int = 1
    liveliness: str = "AUTOMATIC"

    def to_hash(self) -> str:
        """生成唯一哈希，用于去重"""
        content = f"{self.reliability}|{self.durability}|{self.deadline_sec}|{self.deadline_nanosec}|{self.history_kind}|{self.history_depth}"
        return hashlib.md5(content.encode()).hexdigest()[:12]

    def is_equivalent(self, other: 'QoSProfile') -> bool:
        """检查两个QoS配置是否等效"""
        return (self.reliability == other.reliability and
                self.durability == other.durability and
                self.deadline_sec == other.deadline_sec and
                self.deadline_nanosec == other.deadline_nanosec and
                self.history_kind == other.history_kind and
                self.history_depth == other.history_depth)


@dataclass
class TopicConfig:
    """主题配置"""
    name: str
    type_name: str
    qos_profile: str
    domain_id: int


@dataclass
class ParticipantConfig:
    """Participant配置"""
    name: str
    domain_id: int
    qos_profile: str
    publishers: List[TopicConfig] = field(default_factory=list)
    subscribers: List[TopicConfig] = field(default_factory=list)


@dataclass
class DomainConfig:
    """域配置"""
    id: int
    name: str
    participants: List[ParticipantConfig] = field(default_factory=list)


@dataclass
class SystemConfig:
    """系统配置"""
    name: str
    version: str
    domains: List[DomainConfig] = field(default_factory=list)
    qos_profiles: Dict[str, QoSProfile] = field(default_factory=dict)


class NamingConvention:
    """C语言命名规范"""
    @staticmethod
    def to_macro_name(name: str, prefix: str = "DDS") -> str:
        """转换为宏定义命名（全大写下划线）"""
        import re
        # 如果已经有前缀，不要重复添加
        if name.startswith(f"{prefix}_"):
            return name
        # 插入下划线在大写字母前
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
        s2 = re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1)
        # 替换非字母数字为下划线，转大写
        result = re.sub(r'[^a-zA-Z0-9_]', '_', s2).upper()
        return f"{prefix}_{result}" if prefix else result
    @staticmethod
    def to_variable_name(name: str, prefix: str = "g_") -> str:
        """转换为变量命名（小写下划线）"""
        import re
        # 如果已经有前缀，不要重复添加
        if name.startswith(prefix):
            return name
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
        s2 = re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1)
        result = re.sub(r'[^a-zA-Z0-9_]', '_', s2).lower()
        return f"{prefix}{result}"

    @staticmethod
    def to_struct_name(name: str) -> str:
        """转换为结构体命名（小写下划线 + _t）"""
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
        s2 = re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1)
        result = re.sub(r'[^a-zA-Z0-9_]', '_', s2).lower()
        return f"{result}_t"

    @staticmethod
    def to_function_name(name: str, prefix: str = "dds") -> str:
        """转换为函数命名（小写下划线）"""
        s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
        s2 = re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1)
        result = re.sub(r'[^a-zA-Z0-9_]', '_', s2).lower()
        return f"{prefix}_{result}"


class CodeOptimizer:
    """DDS配置代码优化器"""

    def __init__(self, naming: Optional[NamingConvention] = None):
        self.naming = naming or NamingConvention()
        self._qos_cache: Dict[str, QoSProfile] = {}
        self._topic_cache: Dict[str, TopicConfig] = {}
        self._optimization_stats = {
            'qos_merged': 0,
            'duplicates_removed': 0,
            'names_normalized': 0
        }

    def optimize(self, config: SystemConfig) -> SystemConfig:
        """
        执行完整的配置优化

        Args:
            config: 原始系统配置

        Returns:
            优化后的系统配置
        """
        optimized = SystemConfig(
            name=config.name,
            version=config.version
        )

        # 1. 优化QoS Profiles
        optimized.qos_profiles = self._optimize_qos_profiles(config.qos_profiles)

        # 2. 优化Domains和Participants
        optimized.domains = self._optimize_domains(config.domains, optimized.qos_profiles)

        return optimized

    def _optimize_qos_profiles(self, profiles: Dict[str, QoSProfile]) -> Dict[str, QoSProfile]:
        """优化QoS Profiles：合并等效配置"""
        # 按哈希分组
        hash_groups: Dict[str, List[QoSProfile]] = defaultdict(list)
        for name, profile in profiles.items():
            hash_key = profile.to_hash()
            hash_groups[hash_key].append(profile)

        # 选择每组的代表（第一个）并重命名
        optimized: Dict[str, QoSProfile] = {}
        name_mapping: Dict[str, str] = {}  # 原名 -> 新名

        for hash_key, group in hash_groups.items():
            if len(group) > 1:
                self._optimization_stats['qos_merged'] += len(group) - 1

            # 使用第一个作为基础，采用最短名称
            representative = min(group, key=lambda p: len(p.name))
            new_name = self.naming.to_variable_name(representative.name, "qos_")

            optimized[new_name] = QoSProfile(
                name=new_name,
                reliability=representative.reliability,
                durability=representative.durability,
                deadline_sec=representative.deadline_sec,
                deadline_nanosec=representative.deadline_nanosec,
                history_kind=representative.history_kind,
                history_depth=representative.history_depth
            )

            # 记录映射关系
            for profile in group:
                name_mapping[profile.name] = new_name

        self._qos_cache = name_mapping
        return optimized

    def _optimize_domains(self, domains: List[DomainConfig],
                          qos_profiles: Dict[str, QoSProfile]) -> List[DomainConfig]:
        """优化Domain配置"""
        optimized_domains = []

        for domain in domains:
            # 只转换一次，避免重复前缀
            domain_name = domain.name
            if not domain_name.startswith("DDS_DOMAIN_"):
                domain_name = self.naming.to_macro_name(domain.name, "DDS_DOMAIN")
            
            opt_domain = DomainConfig(
                id=domain.id,
                name=domain_name
            )

            for participant in domain.participants:
                opt_participant = self._optimize_participant(participant, qos_profiles)
                opt_domain.participants.append(opt_participant)

            optimized_domains.append(opt_domain)

        return optimized_domains

    def _optimize_participant(self, participant: ParticipantConfig,
                               qos_profiles: Dict[str, QoSProfile]) -> ParticipantConfig:
        """优化Participant配置"""
        # 规范化名称（只转换一次，避免重复前缀）
        normalized_name = participant.name
        if not normalized_name.startswith("g_"):
            normalized_name = self.naming.to_variable_name(participant.name)
        self._optimization_stats['names_normalized'] += 1

        # 映射QoS名称
        qos_name = self._qos_cache.get(participant.qos_profile,
                                       participant.qos_profile)

        opt_participant = ParticipantConfig(
            name=normalized_name,
            domain_id=participant.domain_id,
            qos_profile=qos_name
        )

        # 优化Publisher
        for pub in participant.publishers:
            opt_pub = self._optimize_topic(pub)
            if opt_pub.name not in [t.name for t in opt_participant.publishers]:
                opt_participant.publishers.append(opt_pub)
            else:
                self._optimization_stats['duplicates_removed'] += 1

        # 优化Subscriber
        for sub in participant.subscribers:
            opt_sub = self._optimize_topic(sub)
            if opt_sub.name not in [t.name for t in opt_participant.subscribers]:
                opt_participant.subscribers.append(opt_sub)
            else:
                self._optimization_stats['duplicates_removed'] += 1

        return opt_participant

    def _optimize_topic(self, topic: TopicConfig) -> TopicConfig:
        """优化Topic配置"""
        # 规范化名称（只转换一次）
        normalized_name = topic.name
        if not normalized_name.startswith("g_"):
            normalized_name = self.naming.to_variable_name(topic.name)

        # 映射QoS名称
        qos_name = self._qos_cache.get(topic.qos_profile, topic.qos_profile)

        return TopicConfig(
            name=normalized_name,
            type_name=topic.type_name,
            qos_profile=qos_name,
            domain_id=topic.domain_id
        )

    def get_stats(self) -> Dict[str, int]:
        """获取优化统计信息"""
        return self._optimization_stats.copy()

    def generate_include_guard(self, filename: str) -> str:
        """生成包含守卫宏名称"""
        base = filename.replace('.', '_').replace('-', '_').upper()
        # 移除已有的后缀避免重复
        if base.endswith('_H'):
            return base
        return f"{base}_H"

    def sanitize_comment(self, text: str) -> str:
        """清洗注释文本，防止注释结束符串混淆"""
        # 移除可能导致注释提早结束的字符序列
        text = text.replace('*/', '* /')
        text = text.replace('/*', '/ *')
        return text


class CommentGenerator:
    """MISRA C规范注释生成器"""

    @staticmethod
    def generate_file_header(filename: str, description: str,
                             author: str = "Auto-generated",
                             version: str = "1.0") -> str:
        """生成文件头部注释"""
        from datetime import datetime
        lines = [
            "/**",
            f" * @file {filename}",
            f" * @brief {description}",
            f" * @version {version}",
            f" * @date {datetime.now().strftime('%Y-%m-%d')}",
            f" * @author {author}",
            " *",
            " * @note This file is auto-generated. Do not modify manually.",
            " * @copyright Copyright (c) 2026",
            " */",
            ""
        ]
        return '\n'.join(lines)

    @staticmethod
    def generate_section_comment(title: str) -> str:
        """生成章节分隔注释"""
        separator = "=" * 70
        lines = [
            "",
            f"/* {separator} */",
            f"/* {title:^70} */",
            f"/* {separator} */",
            ""
        ]
        return '\n'.join(lines)

    @staticmethod
    def generate_doxygen_comment(brief: str, params: Optional[Dict[str, str]] = None,
                                  returns: Optional[str] = None) -> str:
        """生成Doxygen格式注释"""
        lines = ["/**", f" * @brief {brief}"]

        if params:
            for param, desc in params.items():
                lines.append(f" * @param {param} {desc}")

        if returns:
            lines.append(f" * @return {returns}")

        lines.append(" */")
        return '\n'.join(lines)
