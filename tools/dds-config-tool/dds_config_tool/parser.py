"""
DDS配置文件解析器
支持XML和JSON格式的DDS配置文件解析
"""

import json
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any, Dict, List, Optional, Union
from dataclasses import dataclass, field
from enum import Enum


class QosPolicyType(Enum):
    """QoS策略类型"""
    RELIABILITY = "reliability"
    DURABILITY = "durability"
    DEADLINE = "deadline"
    LATENCY_BUDGET = "latency_budget"
    LIVELINESS = "liveliness"
    HISTORY = "history"
    RESOURCE_LIMITS = "resource_limits"
    LIFESPAN = "lifespan"


@dataclass
class ReliabilityQos:
    """可靠性QoS策略"""
    kind: str = "BEST_EFFORT"  # BEST_EFFORT 或 RELIABLE
    max_blocking_time_sec: int = 0
    max_blocking_time_nsec: int = 0


@dataclass
class DurabilityQos:
    """持久性QoS策略"""
    kind: str = "VOLATILE"  # VOLATILE, TRANSIENT_LOCAL, TRANSIENT, PERSISTENT


@dataclass
class DeadlineQos:
    """截止时间QoS策略"""
    period_sec: int = 0
    period_nsec: int = 0


@dataclass
class LatencyBudgetQos:
    """延迟预算QoS策略"""
    duration_sec: int = 0
    duration_nsec: int = 0


@dataclass
class LivelinessQos:
    """活跃性QoS策略"""
    kind: str = "AUTOMATIC"  # AUTOMATIC, MANUAL_BY_PARTICIPANT, MANUAL_BY_TOPIC
    lease_duration_sec: int = 0
    lease_duration_nsec: int = 0


@dataclass
class HistoryQos:
    """历史QoS策略"""
    kind: str = "KEEP_LAST"  # KEEP_LAST 或 KEEP_ALL
    depth: int = 1


@dataclass
class ResourceLimitsQos:
    """资源限制QoS策略"""
    max_samples: int = 0
    max_instances: int = 0
    max_samples_per_instance: int = 0


@dataclass
class LifespanQos:
    """生命周期QoS策略"""
    duration_sec: int = 0
    duration_nsec: int = 0


@dataclass
class TopicQos:
    """主题QoS配置"""
    reliability: ReliabilityQos = field(default_factory=ReliabilityQos)
    durability: DurabilityQos = field(default_factory=DurabilityQos)
    deadline: DeadlineQos = field(default_factory=DeadlineQos)
    latency_budget: LatencyBudgetQos = field(default_factory=LatencyBudgetQos)
    liveliness: LivelinessQos = field(default_factory=LivelinessQos)
    history: HistoryQos = field(default_factory=HistoryQos)
    resource_limits: ResourceLimitsQos = field(default_factory=ResourceLimitsQos)
    lifespan: LifespanQos = field(default_factory=LifespanQos)


@dataclass
class TopicConfig:
    """主题配置"""
    name: str
    type_name: str
    qos: TopicQos = field(default_factory=TopicQos)
    description: str = ""


@dataclass
class DomainParticipantConfig:
    """域参与者配置"""
    name: str
    domain_id: int = 0
    qos: TopicQos = field(default_factory=TopicQos)
    topics: List[TopicConfig] = field(default_factory=list)
    description: str = ""


@dataclass
class DDSConfiguration:
    """DDS配置根节点"""
    name: str = ""
    version: str = "1.0.0"
    description: str = ""
    domain_participants: List[DomainParticipantConfig] = field(default_factory=list)
    default_qos: TopicQos = field(default_factory=TopicQos)


class DDSConfigParser:
    """DDS配置文件解析器"""

    def __init__(self):
        self.config: Optional[DDSConfiguration] = None
        self.errors: List[str] = []

    def parse(self, file_path: Union[str, Path]) -> DDSConfiguration:
        """
        解析配置文件

        Args:
            file_path: 配置文件路径 (支持.xml或.json)

        Returns:
            DDSConfiguration: 解析后的配置对象

        Raises:
            ValueError: 文件格式不支持或解析失败
            FileNotFoundError: 文件不存在
        """
        file_path = Path(file_path)

        if not file_path.exists():
            raise FileNotFoundError(f"配置文件不存在: {file_path}")

        suffix = file_path.suffix.lower()

        if suffix == ".xml":
            return self._parse_xml(file_path)
        elif suffix == ".json":
            return self._parse_json(file_path)
        else:
            raise ValueError(f"不支持的文件格式: {suffix}")

    def _parse_xml(self, file_path: Path) -> DDSConfiguration:
        """解析XML配置文件"""
        try:
            tree = ET.parse(file_path)
            root = tree.getroot()

            config = DDSConfiguration()
            config.name = root.get("name", "DDS_Config")
            config.version = root.get("version", "1.0.0")
            config.description = root.get("description", "")

            # 解析默认QoS
            default_qos_elem = root.find("default_qos")
            if default_qos_elem is not None:
                config.default_qos = self._parse_qos_from_xml(default_qos_elem)

            # 解析域参与者
            for dp_elem in root.findall("domain_participant"):
                dp = self._parse_domain_participant_from_xml(dp_elem)
                config.domain_participants.append(dp)

            self.config = config
            return config

        except ET.ParseError as e:
            raise ValueError(f"XML解析失败: {e}")

    def _parse_json(self, file_path: Path) -> DDSConfiguration:
        """解析JSON配置文件"""
        try:
            with open(file_path, "r", encoding="utf-8") as f:
                data = json.load(f)

            config = DDSConfiguration()
            config.name = data.get("name", "DDS_Config")
            config.version = data.get("version", "1.0.0")
            config.description = data.get("description", "")

            # 解析默认QoS
            if "default_qos" in data:
                config.default_qos = self._parse_qos_from_dict(data["default_qos"])

            # 解析域参与者
            for dp_data in data.get("domain_participants", []):
                dp = self._parse_domain_participant_from_dict(dp_data)
                config.domain_participants.append(dp)

            self.config = config
            return config

        except json.JSONDecodeError as e:
            raise ValueError(f"JSON解析失败: {e}")

    def _parse_domain_participant_from_xml(self, elem: ET.Element) -> DomainParticipantConfig:
        """从XML元素解析域参与者"""
        dp = DomainParticipantConfig(
            name=elem.get("name", ""),
            domain_id=int(elem.get("domain_id", 0)),
            description=elem.get("description", "")
        )

        # 解析QoS
        qos_elem = elem.find("qos")
        if qos_elem is not None:
            dp.qos = self._parse_qos_from_xml(qos_elem)

        # 解析主题
        topics_elem = elem.find("topics")
        if topics_elem is not None:
            for topic_elem in topics_elem.findall("topic"):
                topic = self._parse_topic_from_xml(topic_elem)
                dp.topics.append(topic)

        return dp

    def _parse_domain_participant_from_dict(self, data: Dict) -> DomainParticipantConfig:
        """从字典解析域参与者"""
        dp = DomainParticipantConfig(
            name=data.get("name", ""),
            domain_id=data.get("domain_id", 0),
            description=data.get("description", "")
        )

        # 解析QoS
        if "qos" in data:
            dp.qos = self._parse_qos_from_dict(data["qos"])

        # 解析主题
        for topic_data in data.get("topics", []):
            topic = self._parse_topic_from_dict(topic_data)
            dp.topics.append(topic)

        return dp

    def _parse_topic_from_xml(self, elem: ET.Element) -> TopicConfig:
        """从XML元素解析主题"""
        topic = TopicConfig(
            name=elem.get("name", ""),
            type_name=elem.get("type_name", ""),
            description=elem.get("description", "")
        )

        # 解析QoS
        qos_elem = elem.find("qos")
        if qos_elem is not None:
            topic.qos = self._parse_qos_from_xml(qos_elem)

        return topic

    def _parse_topic_from_dict(self, data: Dict) -> TopicConfig:
        """从字典解析主题"""
        topic = TopicConfig(
            name=data.get("name", ""),
            type_name=data.get("type_name", ""),
            description=data.get("description", "")
        )

        # 解析QoS
        if "qos" in data:
            topic.qos = self._parse_qos_from_dict(data["qos"])

        return topic

    def _parse_qos_from_xml(self, elem: ET.Element) -> TopicQos:
        """从XML元素解析QoS"""
        qos = TopicQos()

        # Reliability
        reliability_elem = elem.find("reliability")
        if reliability_elem is not None:
            qos.reliability = ReliabilityQos(
                kind=reliability_elem.get("kind", "BEST_EFFORT"),
                max_blocking_time_sec=int(reliability_elem.get("max_blocking_time_sec", 0)),
                max_blocking_time_nsec=int(reliability_elem.get("max_blocking_time_nsec", 0))
            )

        # Durability
        durability_elem = elem.find("durability")
        if durability_elem is not None:
            qos.durability = DurabilityQos(
                kind=durability_elem.get("kind", "VOLATILE")
            )

        # Deadline
        deadline_elem = elem.find("deadline")
        if deadline_elem is not None:
            qos.deadline = DeadlineQos(
                period_sec=int(deadline_elem.get("period_sec", 0)),
                period_nsec=int(deadline_elem.get("period_nsec", 0))
            )

        # Latency Budget
        latency_elem = elem.find("latency_budget")
        if latency_elem is not None:
            qos.latency_budget = LatencyBudgetQos(
                duration_sec=int(latency_elem.get("duration_sec", 0)),
                duration_nsec=int(latency_elem.get("duration_nsec", 0))
            )

        # Liveliness
        liveliness_elem = elem.find("liveliness")
        if liveliness_elem is not None:
            qos.liveliness = LivelinessQos(
                kind=liveliness_elem.get("kind", "AUTOMATIC"),
                lease_duration_sec=int(liveliness_elem.get("lease_duration_sec", 0)),
                lease_duration_nsec=int(liveliness_elem.get("lease_duration_nsec", 0))
            )

        # History
        history_elem = elem.find("history")
        if history_elem is not None:
            qos.history = HistoryQos(
                kind=history_elem.get("kind", "KEEP_LAST"),
                depth=int(history_elem.get("depth", 1))
            )

        # Resource Limits
        resource_elem = elem.find("resource_limits")
        if resource_elem is not None:
            qos.resource_limits = ResourceLimitsQos(
                max_samples=int(resource_elem.get("max_samples", 0)),
                max_instances=int(resource_elem.get("max_instances", 0)),
                max_samples_per_instance=int(resource_elem.get("max_samples_per_instance", 0))
            )

        # Lifespan
        lifespan_elem = elem.find("lifespan")
        if lifespan_elem is not None:
            qos.lifespan = LifespanQos(
                duration_sec=int(lifespan_elem.get("duration_sec", 0)),
                duration_nsec=int(lifespan_elem.get("duration_nsec", 0))
            )

        return qos

    def _parse_qos_from_dict(self, data: Dict) -> TopicQos:
        """从字典解析QoS"""
        qos = TopicQos()

        # Reliability
        if "reliability" in data:
            r = data["reliability"]
            qos.reliability = ReliabilityQos(
                kind=r.get("kind", "BEST_EFFORT"),
                max_blocking_time_sec=r.get("max_blocking_time_sec", 0),
                max_blocking_time_nsec=r.get("max_blocking_time_nsec", 0)
            )

        # Durability
        if "durability" in data:
            d = data["durability"]
            qos.durability = DurabilityQos(
                kind=d.get("kind", "VOLATILE")
            )

        # Deadline
        if "deadline" in data:
            d = data["deadline"]
            qos.deadline = DeadlineQos(
                period_sec=d.get("period_sec", 0),
                period_nsec=d.get("period_nsec", 0)
            )

        # Latency Budget
        if "latency_budget" in data:
            lb = data["latency_budget"]
            qos.latency_budget = LatencyBudgetQos(
                duration_sec=lb.get("duration_sec", 0),
                duration_nsec=lb.get("duration_nsec", 0)
            )

        # Liveliness
        if "liveliness" in data:
            l = data["liveliness"]
            qos.liveliness = LivelinessQos(
                kind=l.get("kind", "AUTOMATIC"),
                lease_duration_sec=l.get("lease_duration_sec", 0),
                lease_duration_nsec=l.get("lease_duration_nsec", 0)
            )

        # History
        if "history" in data:
            h = data["history"]
            qos.history = HistoryQos(
                kind=h.get("kind", "KEEP_LAST"),
                depth=h.get("depth", 1)
            )

        # Resource Limits
        if "resource_limits" in data:
            rl = data["resource_limits"]
            qos.resource_limits = ResourceLimitsQos(
                max_samples=rl.get("max_samples", 0),
                max_instances=rl.get("max_instances", 0),
                max_samples_per_instance=rl.get("max_samples_per_instance", 0)
            )

        # Lifespan
        if "lifespan" in data:
            ls = data["lifespan"]
            qos.lifespan = LifespanQos(
                duration_sec=ls.get("duration_sec", 0),
                duration_nsec=ls.get("duration_nsec", 0)
            )

        return qos

    def get_errors(self) -> List[str]:
        """获取解析过程中的错误列表"""
        return self.errors

    def to_dict(self, config: Optional[DDSConfiguration] = None) -> Dict[str, Any]:
        """将配置转换为字典"""
        if config is None:
            config = self.config

        if config is None:
            return {}

        return {
            "name": config.name,
            "version": config.version,
            "description": config.description,
            "default_qos": self._qos_to_dict(config.default_qos),
            "domain_participants": [
                self._domain_participant_to_dict(dp)
                for dp in config.domain_participants
            ]
        }

    def _qos_to_dict(self, qos: TopicQos) -> Dict:
        """将QoS转换为字典"""
        return {
            "reliability": {
                "kind": qos.reliability.kind,
                "max_blocking_time_sec": qos.reliability.max_blocking_time_sec,
                "max_blocking_time_nsec": qos.reliability.max_blocking_time_nsec
            },
            "durability": {
                "kind": qos.durability.kind
            },
            "deadline": {
                "period_sec": qos.deadline.period_sec,
                "period_nsec": qos.deadline.period_nsec
            },
            "latency_budget": {
                "duration_sec": qos.latency_budget.duration_sec,
                "duration_nsec": qos.latency_budget.duration_nsec
            },
            "liveliness": {
                "kind": qos.liveliness.kind,
                "lease_duration_sec": qos.liveliness.lease_duration_sec,
                "lease_duration_nsec": qos.liveliness.lease_duration_nsec
            },
            "history": {
                "kind": qos.history.kind,
                "depth": qos.history.depth
            },
            "resource_limits": {
                "max_samples": qos.resource_limits.max_samples,
                "max_instances": qos.resource_limits.max_instances,
                "max_samples_per_instance": qos.resource_limits.max_samples_per_instance
            },
            "lifespan": {
                "duration_sec": qos.lifespan.duration_sec,
                "duration_nsec": qos.lifespan.duration_nsec
            }
        }

    def _domain_participant_to_dict(self, dp: DomainParticipantConfig) -> Dict:
        """将域参与者转换为字典"""
        return {
            "name": dp.name,
            "domain_id": dp.domain_id,
            "description": dp.description,
            "qos": self._qos_to_dict(dp.qos),
            "topics": [
                {
                    "name": t.name,
                    "type_name": t.type_name,
                    "description": t.description,
                    "qos": self._qos_to_dict(t.qos)
                }
                for t in dp.topics
            ]
        }
