"""Smart validation and feedback system for LLM-generated configurations."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple
from datetime import datetime

# Paths
REPO_ROOT = Path(__file__).resolve().parents[4]
FEEDBACK_DIR = REPO_ROOT / "tools" / "yuleasr-configurator" / "knowledge_base" / "feedback"


class ConfigValidator:
    """Validate LLM-generated configurations against schema."""

    def __init__(self):
        self.validation_errors: List[Dict[str, Any]] = []

    def validate_against_schema(
        self,
        module_name: str,
        config: Dict[str, Any],
        schema: Dict[str, Any]
    ) -> Tuple[bool, List[str]]:
        """
        Validate configuration against JSON Schema.

        Returns:
            (is_valid, list_of_errors)
        """
        errors = []

        # Check required fields
        required_fields = schema.get("required", [])
        for field in required_fields:
            if field not in config:
                errors.append(f"Missing required field: {field}")

        # Check field types
        properties = schema.get("properties", {})
        for field_name, field_value in config.items():
            if field_name not in properties:
                errors.append(f"Unknown field: {field_name}")
                continue

            field_schema = properties[field_name]
            expected_type = field_schema.get("type")

            if expected_type and not self._check_type(field_value, expected_type):
                errors.append(
                    f"Field '{field_name}' expected type '{expected_type}', "
                    f"got '{type(field_value).__name__}'"
                )

            # Check enum values
            if "enum" in field_schema and field_value not in field_schema["enum"]:
                errors.append(
                    f"Field '{field_name}' value '{field_value}' not in "
                    f"allowed values: {field_schema['enum']}"
                )

            # Check numeric ranges
            if expected_type in ["integer", "number"]:
                if "minimum" in field_schema and field_value < field_schema["minimum"]:
                    errors.append(
                        f"Field '{field_name}' value {field_value} below minimum "
                        f"{field_schema['minimum']}"
                    )
                if "maximum" in field_schema and field_value > field_schema["maximum"]:
                    errors.append(
                        f"Field '{field_name}' value {field_value} above maximum "
                        f"{field_schema['maximum']}"
                    )

        return len(errors) == 0, errors

    def _check_type(self, value: Any, expected_type: str) -> bool:
        """Check if value matches expected type."""
        type_map = {
            "string": str,
            "integer": int,
            "number": (int, float),
            "boolean": bool,
            "array": list,
            "object": dict,
        }

        expected_python_type = type_map.get(expected_type)
        if not expected_python_type:
            return True

        return isinstance(value, expected_python_type)

    def auto_fix_errors(
        self,
        config: Dict[str, Any],
        schema: Dict[str, Any],
        errors: List[str]
    ) -> Dict[str, Any]:
        """Attempt to automatically fix common errors."""
        fixed_config = config.copy()

        for error in errors:
            # Fix type errors
            if "expected type 'integer'" in error:
                field_name = error.split("'")[1]
                if field_name in fixed_config:
                    try:
                        fixed_config[field_name] = int(fixed_config[field_name])
                    except (ValueError, TypeError):
                        pass

            # Fix boolean strings
            elif "expected type 'boolean'" in error:
                field_name = error.split("'")[1]
                if field_name in fixed_config:
                    val = fixed_config[field_name]
                    if isinstance(val, str):
                        fixed_config[field_name] = val.lower() in ["true", "1", "yes"]

            # Fix numeric ranges
            elif "below minimum" in error:
                field_name = error.split("'")[1]
                min_val = int(error.split("minimum ")[-1])
                if field_name in fixed_config:
                    fixed_config[field_name] = min_val

            elif "above maximum" in error:
                field_name = error.split("'")[1]
                max_val = int(error.split("maximum ")[-1])
                if field_name in fixed_config:
                    fixed_config[field_name] = max_val

        return fixed_config


class FeedbackCollector:
    """Collect and analyze user feedback on LLM-generated configurations."""

    def __init__(self):
        self.feedback_dir = FEEDBACK_DIR
        self.feedback_dir.mkdir(parents=True, exist_ok=True)

    def record_feedback(
        self,
        module_name: str,
        user_request: str,
        original_config: Dict[str, Any],
        user_modified_config: Optional[Dict[str, Any]],
        applied: bool,
        timestamp: Optional[str] = None
    ):
        """Record user feedback on a configuration."""
        feedback = {
            "timestamp": timestamp or datetime.now().isoformat(),
            "module": module_name,
            "request": user_request,
            "original_config": original_config,
            "user_modified_config": user_modified_config,
            "applied": applied,
            "changes_made": self._diff_configs(original_config, user_modified_config) if user_modified_config else []
        }

        # Save to file
        feedback_file = self.feedback_dir / f"feedback_{module_name}_{int(datetime.now().timestamp())}.json"
        with open(feedback_file, "w", encoding="utf-8") as f:
            json.dump(feedback, f, indent=2, ensure_ascii=False)

        return feedback

    def _diff_configs(
        self,
        original: Dict[str, Any],
        modified: Dict[str, Any]
    ) -> List[Dict[str, Any]]:
        """Calculate differences between original and modified configs."""
        changes = []

        all_keys = set(list(original.keys()) + list(modified.keys()))

        for key in all_keys:
            if key not in original:
                changes.append({"type": "added", "field": key, "value": modified[key]})
            elif key not in modified:
                changes.append({"type": "removed", "field": key})
            elif original[key] != modified[key]:
                changes.append({
                    "type": "modified",
                    "field": key,
                    "old_value": original[key],
                    "new_value": modified[key]
                })

        return changes

    def analyze_feedback(self, module_name: Optional[str] = None) -> Dict[str, Any]:
        """Analyze collected feedback to identify patterns."""
        feedback_files = list(self.feedback_dir.glob("feedback_*.json"))

        if module_name:
            feedback_files = [f for f in feedback_files if module_name in f.name]

        total = len(feedback_files)
        applied_count = 0
        modified_count = 0
        common_changes = {}

        for feedback_file in feedback_files:
            with open(feedback_file) as f:
                feedback = json.load(f)

            if feedback["applied"]:
                applied_count += 1

            if feedback["changes_made"]:
                modified_count += 1
                for change in feedback["changes_made"]:
                    field = change["field"]
                    common_changes[field] = common_changes.get(field, 0) + 1

        return {
            "total_feedback": total,
            "applied_rate": applied_count / total if total > 0 else 0,
            "modification_rate": modified_count / total if total > 0 else 0,
            "common_changes": dict(sorted(common_changes.items(), key=lambda x: x[1], reverse=True)[:10])
        }

    def generate_improved_examples(self, module_name: str) -> List[Dict[str, str]]:
        """Generate improved few-shot examples based on feedback."""
        feedback_files = list(self.feedback_dir.glob(f"feedback_{module_name}_*.json"))

        improved_examples = []

        for feedback_file in feedback_files:
            with open(feedback_file) as f:
                feedback = json.load(f)

            # Only use feedback where user modified the config
            if feedback["user_modified_config"]:
                example = {
                    "request": feedback["request"],
                    "config": f"```yaml\n{json.dumps(feedback['user_modified_config'], indent=2)}\n```",
                    "source": "user_feedback",
                    "timestamp": feedback["timestamp"]
                }
                improved_examples.append(example)

        return improved_examples


# Global instances
validator = ConfigValidator()
feedback_collector = FeedbackCollector()


def validate_config(module_name: str, config: Dict[str, Any], schema: Dict[str, Any]) -> Tuple[bool, List[str]]:
    """Validate configuration against schema."""
    return validator.validate_against_schema(module_name, config, schema)


def auto_fix_config(config: Dict[str, Any], schema: Dict[str, Any], errors: List[str]) -> Dict[str, Any]:
    """Attempt to auto-fix configuration errors."""
    return validator.auto_fix_errors(config, schema, errors)


def record_feedback(
    module_name: str,
    user_request: str,
    original_config: Dict[str, Any],
    user_modified_config: Optional[Dict[str, Any]],
    applied: bool
):
    """Record user feedback."""
    return feedback_collector.record_feedback(
        module_name, user_request, original_config, user_modified_config, applied
    )


def get_feedback_analysis(module_name: Optional[str] = None) -> Dict[str, Any]:
    """Get feedback analysis."""
    return feedback_collector.analyze_feedback(module_name)
