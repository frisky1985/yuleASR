"""JSON Schema Draft-7 validation engine with per-module schema registry."""

from __future__ import annotations

import json
import logging
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional

LOG = logging.getLogger(__name__)


@dataclass
class ValidationError:
    """A single validation error."""
    path: str
    message: str
    severity: str = "error"

    def __str__(self) -> str:
        return f"[{self.severity.upper()}] {self.path}: {self.message}"


@dataclass
class ValidationResult:
    """Collection of validation errors."""
    errors: List[ValidationError] = field(default_factory=list)

    @property
    def ok(self) -> bool:
        return not any(e.severity == "error" for e in self.errors)

    @property
    def error_count(self) -> int:
        return sum(1 for e in self.errors if e.severity == "error")

    @property
    def warning_count(self) -> int:
        return sum(1 for e in self.errors if e.severity == "warning")

    def add(self, path: str, message: str, severity: str = "error") -> None:
        self.errors.append(ValidationError(path=path, message=message, severity=severity))

    def merge(self, other: "ValidationResult") -> None:
        self.errors.extend(other.errors)

    def __str__(self) -> str:
        if self.ok and not self.warning_count:
            return "validation passed"
        lines = [f"{self.error_count} error(s), {self.warning_count} warning(s)"]
        for e in self.errors:
            lines.append(f"  {e}")
        return "\n".join(lines)


class SchemaRegistry:
    """Loads and caches per-module JSON Schema files.

    Schemas are discovered from a directory tree::

        schemas/
        ├── base_schema.json
        ├── system_schema.json
        ├── mcal/
        │   ├── mcu_schema.json
        │   └── ...
        ├── ecual/
        │   └── ...
        └── services/
            └── ...
    """

    def __init__(self, schema_dir: Path):
        self._schema_dir = schema_dir
        self._cache: Dict[str, Any] = {}
        self._module_schemas: Dict[str, Path] = {}
        self._discover_schemas()

    def _discover_schemas(self) -> None:
        """Walk the schema directory and index all *_schema.json files."""
        if not self._schema_dir.is_dir():
            LOG.warning("schema directory not found: %s", self._schema_dir)
            return
        for p in self._schema_dir.rglob("*_schema.json"):
            name = p.stem.replace("_schema", "")
            self._module_schemas[name.lower()] = p

    def _load_schema(self, path: Path) -> Dict[str, Any]:
        if str(path) not in self._cache:
            with open(path, "r", encoding="utf-8") as f:
                self._cache[str(path)] = json.load(f)
        return self._cache[str(path)]

    def get_module_schema(self, module_name: str) -> Optional[Dict[str, Any]]:
        key = module_name.lower()
        path = self._module_schemas.get(key)
        if path is None:
            return None
        return self._load_schema(path)

    def list_modules(self) -> List[str]:
        return sorted(self._module_schemas.keys())

    def validate_module(self, module_name: str, config: Dict[str, Any]) -> ValidationResult:
        schema = self.get_module_schema(module_name)
        if schema is None:
            result = ValidationResult()
            result.add(module_name, f"no schema found for module '{module_name}'", "warning")
            return result
        return self._validate_against(config, schema, prefix=module_name)

    def validate_all(self, config: Dict[str, Any]) -> ValidationResult:
        result = ValidationResult()
        modules = config.get("modules", {})
        if not isinstance(modules, dict):
            result.add("modules", "'modules' must be a mapping")
            return result

        for mod_name, mod_cfg in modules.items():
            if not isinstance(mod_cfg, dict):
                result.add(f"modules.{mod_name}", "must be a mapping")
                continue
            mod_result = self.validate_module(mod_name, mod_cfg)
            mod_result.errors = [
                ValidationError(
                    path=f"modules.{mod_name}.{e.path}" if e.path != mod_name else f"modules.{mod_name}",
                    message=e.message,
                    severity=e.severity,
                )
                for e in mod_result.errors
            ]
            result.merge(mod_result)
        return result

    def _validate_against(
        self, data: Any, schema: Dict[str, Any], prefix: str = ""
    ) -> ValidationResult:
        try:
            import jsonschema
        except ImportError:
            result = ValidationResult()
            result.add(prefix, "jsonschema not installed — skipping validation", "warning")
            return result

        validator_cls = getattr(jsonschema, "Draft7Validator", None)
        if validator_cls is None:
            result = ValidationResult()
            result.add(prefix, "jsonschema too old — need Draft7Validator", "warning")
            return result

        validator = validator_cls(schema)
        result = ValidationResult()
        for err in sorted(validator.iter_errors(data), key=lambda e: list(e.absolute_path)):
            loc = "/".join(str(p) for p in err.absolute_path) or prefix
            path = f"{prefix}.{loc}" if loc != prefix else prefix
            result.add(path, err.message)
        return result


def fallback_validate(config: Dict[str, Any]) -> ValidationResult:
    """Minimal structural validation when jsonschema is unavailable."""
    result = ValidationResult()
    if not isinstance(config, dict):
        result.add("<root>", "top level must be a mapping")
        return result
    if "version" not in config:
        result.add("version", "missing required key 'version'")
    modules = config.get("modules")
    if not isinstance(modules, dict) or not modules:
        result.add("modules", "'modules' must be a non-empty mapping")
        return result
    for name, node in modules.items():
        if not isinstance(node, dict):
            result.add(f"modules.{name}", "must be a mapping")
            continue
        if "name" not in node:
            result.add(f"modules.{name}", "missing 'name'")
        if not isinstance(node.get("enabled"), bool):
            result.add(f"modules.{name}", "'enabled' must be a boolean")
    return result
