"""
跨模块引用校验引擎

检查配置中的跨模块引用是否有效（dangling reference detection）。
"""

from typing import Dict, List, Any, Optional
from dataclasses import dataclass
from enum import Enum
from .reference_validator import CROSS_MODULE_REFERENCES, REFERENCE_GRAPH, REVERSE_REFERENCE_GRAPH


class IssueSeverity(Enum):
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"


class IssueType(Enum):
    DANGLING_REFERENCE = "dangling_reference"
    DISABLED_DEPENDENCY = "disabled_dependency"
    MISSING_MODULE = "missing_module"
    INCONSISTENT_COUNT = "inconsistent_count"


@dataclass
class ReferenceIssue:
    """引用校验问题"""
    severity: IssueSeverity
    issue_type: IssueType
    source_module: str
    source_field: str
    target_module: str
    target_field: str
    message: str
    suggestion: Optional[str] = None

    def to_dict(self) -> Dict[str, Any]:
        return {
            "severity": self.severity.value,
            "type": self.issue_type.value,
            "source": f"{self.source_module}.{self.source_field}",
            "target": f"{self.target_module}.{self.target_field}",
            "message": self.message,
            "suggestion": self.suggestion,
        }


def get_nested_value(config: Dict[str, Any], path: str) -> Any:
    """
    从配置中获取嵌套字段的值。
    路径格式："general.num_controllers" 或 "blocks"
    """
    parts = path.split(".")
    current = config
    for part in parts:
        if isinstance(current, dict):
            current = current.get(part)
        else:
            return None
        if current is None:
            return None
    return current


def check_module_enabled(config: Dict[str, Any], module_name: str) -> bool:
    """检查模块是否启用"""
    module_config = config.get(module_name, {})
    return module_config.get("enabled", False)


def validate_cross_module_references(config: Dict[str, Any]) -> List[ReferenceIssue]:
    """
    校验跨模块引用

    Args:
        config: 完整配置字典 {module_name: module_config}

    Returns:
        引用问题列表
    """
    issues: List[ReferenceIssue] = []

    for src_module, src_field, tgt_module, tgt_field in CROSS_MODULE_REFERENCES:
        src_config = config.get(src_module, {})
        tgt_config = config.get(tgt_module, {})

        # 检查源模块是否启用
        if not src_config.get("enabled", False):
            continue

        # 检查目标模块是否存在且启用
        if tgt_module not in config:
            issues.append(ReferenceIssue(
                severity=IssueSeverity.ERROR,
                issue_type=IssueType.MISSING_MODULE,
                source_module=src_module,
                source_field=src_field,
                target_module=tgt_module,
                target_field=tgt_field,
                message=f"{src_module} 引用了 {tgt_module}，但 {tgt_module} 未在配置中",
                suggestion=f"添加 {tgt_module} 模块配置或禁用 {src_module}",
            ))
            continue

        if not tgt_config.get("enabled", False):
            issues.append(ReferenceIssue(
                severity=IssueSeverity.ERROR,
                issue_type=IssueType.DISABLED_DEPENDENCY,
                source_module=src_module,
                source_field=src_field,
                target_module=tgt_module,
                target_field=tgt_field,
                message=f"{src_module}.{src_field} 引用了 {tgt_module}，但 {tgt_module} 未启用",
                suggestion=f"启用 {tgt_module} 或禁用 {src_module}",
            ))
            continue

        # 检查数量一致性（如果源字段是整数计数）
        src_value = get_nested_value(src_config, src_field)
        tgt_value = get_nested_value(tgt_config, tgt_field)

        if isinstance(src_value, int) and isinstance(tgt_value, int):
            if src_value > tgt_value:
                issues.append(ReferenceIssue(
                    severity=IssueSeverity.ERROR,
                    issue_type=IssueType.INCONSISTENT_COUNT,
                    source_module=src_module,
                    source_field=src_field,
                    target_module=tgt_module,
                    target_field=tgt_field,
                    message=f"{src_module}.{src_field}={src_value} 超过 {tgt_module}.{tgt_field}={tgt_value}",
                    suggestion=f"增加 {tgt_module}.{tgt_field} 或减少 {src_module}.{src_field}",
                ))

    return issues


def get_module_dependencies(config: Dict[str, Any], module_name: str) -> List[str]:
    """
    获取模块的直接依赖（它引用的模块）

    Args:
        config: 完整配置
        module_name: 模块名

    Returns:
        依赖的模块名列表
    """
    deps = []
    if module_name in REFERENCE_GRAPH:
        for ref in REFERENCE_GRAPH[module_name]:
            target = ref["target"]
            if target in config and config[target].get("enabled", False):
                deps.append(target)
    return deps


def get_module_dependents(config: Dict[str, Any], module_name: str) -> List[str]:
    """
    获取模块的依赖者（引用它的模块）

    用于评估"禁用此模块会影响谁"。

    Args:
        config: 完整配置
        module_name: 模块名

    Returns:
        依赖此模块的模块名列表
    """
    dependents = []
    if module_name in REVERSE_REFERENCE_GRAPH:
        for ref in REVERSE_REFERENCE_GRAPH[module_name]:
            source = ref["source"]
            if source in config and config[source].get("enabled", False):
                dependents.append(source)
    return dependents


def check_disable_impact(config: Dict[str, Any], module_name: str) -> List[ReferenceIssue]:
    """
    检查禁用某模块的影响

    Args:
        config: 完整配置
        module_name: 要禁用的模块名

    Returns:
        影响问题列表
    """
    issues = []
    dependents = get_module_dependents(config, module_name)

    for dependent in dependents:
        issues.append(ReferenceIssue(
            severity=IssueSeverity.WARNING,
            issue_type=IssueType.DISABLED_DEPENDENCY,
            source_module=dependent,
            source_field="*",
            target_module=module_name,
            target_field="*",
            message=f"禁用 {module_name} 将影响 {dependent}（它依赖此模块）",
            suggestion=f"同时禁用 {dependent} 或保留 {module_name} 启用",
        ))

    return issues


def validate_config_references(config: Dict[str, Any]) -> Dict[str, Any]:
    """
    完整的引用校验入口

    Args:
        config: 完整配置

    Returns:
        {
            "valid": bool,
            "issues": [...],
            "summary": {
                "errors": int,
                "warnings": int,
                "info": int
            }
        }
    """
    issues = validate_cross_module_references(config)

    summary = {
        "errors": sum(1 for i in issues if i.severity == IssueSeverity.ERROR),
        "warnings": sum(1 for i in issues if i.severity == IssueSeverity.WARNING),
        "info": sum(1 for i in issues if i.severity == IssueSeverity.INFO),
    }

    return {
        "valid": summary["errors"] == 0,
        "issues": [i.to_dict() for i in issues],
        "summary": summary,
    }
