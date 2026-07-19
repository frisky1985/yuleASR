"""
DDS配置验证器
验证配置文件的合法性
"""

from typing import List, Optional, Set, Tuple, Union
from dataclasses import dataclass
from pathlib import Path

from .parser import (
    DDSConfiguration, DomainParticipantConfig, TopicConfig, TopicQos,
    ReliabilityQos, DurabilityQos, DeadlineQos, LatencyBudgetQos,
    LivelinessQos, HistoryQos, ResourceLimitsQos, LifespanQos
)


@dataclass
class ValidationError:
    """验证错误"""
    path: str  # 错误路径如 "domain_participants[0].topics[1].qos.reliability.kind"
    message: str
    severity: str = "ERROR"  # ERROR, WARNING, INFO


class DDSConfigValidator:
    """DDS配置验证器"""

    # 有效的QoS策略值
    VALID_RELIABILITY_KINDS = {"BEST_EFFORT", "RELIABLE"}
    VALID_DURABILITY_KINDS = {"VOLATILE", "TRANSIENT_LOCAL", "TRANSIENT", "PERSISTENT"}
    VALID_LIVELINESS_KINDS = {"AUTOMATIC", "MANUAL_BY_PARTICIPANT", "MANUAL_BY_TOPIC"}
    VALID_HISTORY_KINDS = {"KEEP_LAST", "KEEP_ALL"}

    def __init__(self):
        self.errors: List[ValidationError] = []
        self.warnings: List[ValidationError] = []

    def validate(self, config: DDSConfiguration) -> Tuple[bool, List[ValidationError]]:
        """
        验证DDS配置

        Args:
            config: DDS配置对象

        Returns:
            Tuple[bool, List[ValidationError]]: (是否有效, 错误列表)
        """
        self.errors = []
        self.warnings = []

        # 验证基础配置
        self._validate_base_config(config)

        # 验证域参与者
        self._validate_domain_participants(config)

        # 检查重复
        self._check_duplicates(config)

        all_issues = self.errors + self.warnings
        is_valid = len(self.errors) == 0

        return is_valid, all_issues

    def _validate_base_config(self, config: DDSConfiguration):
        """验证基础配置"""
        if not config.name:
            self.errors.append(ValidationError(
                path="name",
                message="配置名称不能为空",
                severity="ERROR"
            ))

        if not config.version:
            self.warnings.append(ValidationError(
                path="version",
                message="配置版本未指定",
                severity="WARNING"
            ))

        if not config.domain_participants:
            self.warnings.append(ValidationError(
                path="domain_participants",
                message="未配置任何域参与者",
                severity="WARNING"
            ))

    def _validate_domain_participants(self, config: DDSConfiguration):
        """验证域参与者"""
        for i, dp in enumerate(config.domain_participants):
            path = f"domain_participants[{i}]"
            self._validate_domain_participant(dp, path)

    def _validate_domain_participant(self, dp: DomainParticipantConfig, path: str):
        """验证单个域参与者"""
        if not dp.name:
            self.errors.append(ValidationError(
                path=f"{path}.name",
                message="域参与者名称不能为空",
                severity="ERROR"
            ))

        # 验证domain_id范围 (DDS标准为0-232)
        if dp.domain_id < 0 or dp.domain_id > 232:
            self.errors.append(ValidationError(
                path=f"{path}.domain_id",
                message=f"域ID {dp.domain_id} 超出有效范围 (0-232)",
                severity="ERROR"
            ))

        # 验证QoS
        self._validate_qos(dp.qos, f"{path}.qos")

        # 验证主题
        for j, topic in enumerate(dp.topics):
            topic_path = f"{path}.topics[{j}]"
            self._validate_topic(topic, topic_path)

    def _validate_topic(self, topic: TopicConfig, path: str):
        """验证主题"""
        if not topic.name:
            self.errors.append(ValidationError(
                path=f"{path}.name",
                message="主题名称不能为空",
                severity="ERROR"
            ))

        if not topic.type_name:
            self.warnings.append(ValidationError(
                path=f"{path}.type_name",
                message=f"主题 '{topic.name}' 的类型名称未指定",
                severity="WARNING"
            ))

        # 验证主题名称格式
        if topic.name and not self._is_valid_identifier(topic.name):
            self.warnings.append(ValidationError(
                path=f"{path}.name",
                message=f"主题名称 '{topic.name}' 可能不符合C标识符命名规范",
                severity="WARNING"
            ))

        # 验证QoS
        self._validate_qos(topic.qos, f"{path}.qos")

    def _validate_qos(self, qos: TopicQos, path: str):
        """验证QoS配置"""
        # Reliability
        if qos.reliability.kind not in self.VALID_RELIABILITY_KINDS:
            self.errors.append(ValidationError(
                path=f"{path}.reliability.kind",
                message=f"无效的可靠性类型: {qos.reliability.kind}",
                severity="ERROR"
            ))

        # Durability
        if qos.durability.kind not in self.VALID_DURABILITY_KINDS:
            self.errors.append(ValidationError(
                path=f"{path}.durability.kind",
                message=f"无效的持久性类型: {qos.durability.kind}",
                severity="ERROR"
            ))

        # Liveliness
        if qos.liveliness.kind not in self.VALID_LIVELINESS_KINDS:
            self.errors.append(ValidationError(
                path=f"{path}.liveliness.kind",
                message=f"无效的活跃性类型: {qos.liveliness.kind}",
                severity="ERROR"
            ))

        # History
        if qos.history.kind not in self.VALID_HISTORY_KINDS:
            self.errors.append(ValidationError(
                path=f"{path}.history.kind",
                message=f"无效的历史类型: {qos.history.kind}",
                severity="ERROR"
            ))

        if qos.history.kind == "KEEP_LAST" and qos.history.depth < 1:
            self.errors.append(ValidationError(
                path=f"{path}.history.depth",
                message="KEEP_LAST历史类型的深度必须大于0",
                severity="ERROR"
            ))

        # 验证资源限制一致性
        self._validate_resource_limits_consistency(qos, path)

        # 验证时间值
        self._validate_time_values(qos, path)

    def _validate_resource_limits_consistency(self, qos: TopicQos, path: str):
        """验证资源限制的一致性"""
        rl = qos.resource_limits

        # 如果设置了max_samples_per_instance, 应小于等于max_samples
        if rl.max_samples > 0 and rl.max_samples_per_instance > 0:
            if rl.max_samples_per_instance > rl.max_samples:
                self.warnings.append(ValidationError(
                    path=f"{path}.resource_limits",
                    message="max_samples_per_instance 应小于等于 max_samples",
                    severity="WARNING"
                ))

        # KEEP_ALL历史类型需要资源限制
        if qos.history.kind == "KEEP_ALL":
            if rl.max_samples == 0 and rl.max_instances == 0:
                self.warnings.append(ValidationError(
                    path=f"{path}.resource_limits",
                    message="KEEP_ALL历史类型建议设置资源限制",
                    severity="WARNING"
                ))

    def _validate_time_values(self, qos: TopicQos, path: str):
        """验证时间值的有效性"""
        # 验证纳秒值范围 (0-999999999)
        time_fields = [
            (qos.reliability.max_blocking_time_nsec, f"{path}.reliability.max_blocking_time_nsec"),
            (qos.deadline.period_nsec, f"{path}.deadline.period_nsec"),
            (qos.latency_budget.duration_nsec, f"{path}.latency_budget.duration_nsec"),
            (qos.liveliness.lease_duration_nsec, f"{path}.liveliness.lease_duration_nsec"),
            (qos.lifespan.duration_nsec, f"{path}.lifespan.duration_nsec"),
        ]

        for value, field_path in time_fields:
            if value < 0 or value >= 1_000_000_000:
                self.errors.append(ValidationError(
                    path=field_path,
                    message=f"纳秒值 {value} 超出有效范围 (0-999999999)",
                    severity="ERROR"
                ))

    def _check_duplicates(self, config: DDSConfiguration):
        """检查重复项"""
        domain_names: Set[str] = set()

        for i, dp in enumerate(config.domain_participants):
            # 检查重复的域参与者名称
            if dp.name in domain_names:
                self.errors.append(ValidationError(
                    path=f"domain_participants[{i}].name",
                    message=f"重复的域参与者名称: {dp.name}",
                    severity="ERROR"
                ))
            domain_names.add(dp.name)

            # 检查重复的主题名称
            topic_names: Set[str] = set()
            for j, topic in enumerate(dp.topics):
                if topic.name in topic_names:
                    self.errors.append(ValidationError(
                        path=f"domain_participants[{i}].topics[{j}].name",
                        message=f"重复的主题名称: {topic.name}",
                        severity="ERROR"
                    ))
                topic_names.add(topic.name)

    def _is_valid_identifier(self, name: str) -> bool:
        """检查是否是有效的C标识符"""
        if not name:
            return False

        # 第一个字符必须是字母或下划线
        if not (name[0].isalpha() or name[0] == '_'):
            return False

        # 其余字符只能是字母、数字或下划线
        for char in name:
            if not (char.isalnum() or char == '_'):
                return False

        return True

    def validate_file(self, file_path: Union[str, Path]) -> Tuple[bool, List[ValidationError]]:
        """
        直接验证配置文件

        Args:
            file_path: 配置文件路径

        Returns:
            Tuple[bool, List[ValidationError]]: (是否有效, 错误列表)
        """
        from .parser import DDSConfigParser

        parser = DDSConfigParser()
        try:
            config = parser.parse(file_path)
            return self.validate(config)
        except Exception as e:
            self.errors = [ValidationError(
                path="",
                message=f"解析文件失败: {e}",
                severity="ERROR"
            )]
            return False, self.errors


