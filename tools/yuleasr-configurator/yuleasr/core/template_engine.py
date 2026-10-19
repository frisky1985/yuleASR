"""Jinja2 template engine with AUTOSAR-specific custom filters."""

from __future__ import annotations

import logging
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional

LOG = logging.getLogger(__name__)


@dataclass
class RenderResult:
    """Result of rendering a single template."""
    template_name: str
    output_name: str
    content: str
    module_name: str


def _to_upper_snake(name: str) -> str:
    """Convert CamelCase or mixedCase to UPPER_SNAKE_CASE."""
    s1 = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1_\2", name)
    return re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", s1).upper()


def _to_lower_snake(name: str) -> str:
    return _to_upper_snake(name).lower()


def _c_bool(value: Any) -> str:
    return "STD_ON" if value else "STD_OFF"


def _c_uint(value: Any) -> str:
    return f"({value}U)"


def _c_hex(value: Any, width: int = 8) -> str:
    return f"(0x{int(value):0{width}X}U)"


def _pad_factory(width: int):
    def _pad(value: Any) -> str:
        s = str(value)
        return s.ljust(width) if len(s) < width else s + " "
    return _pad


class TemplateEngine:
    """Jinja2 environment pre-configured with AUTOSAR filters and globals."""

    def __init__(self, templates_dir: Path):
        self._templates_dir = templates_dir
        self._env = self._build_environment()

    def _build_environment(self):
        try:
            from jinja2 import Environment, FileSystemLoader
        except ImportError:
            raise SystemExit("[ENV-ERROR] jinja2 is required: pip install jinja2")

        undefined_cls = getattr(
            __import__("jinja2", fromlist=["ChainableUndefined"]),
            "ChainableUndefined",
            None,
        )
        env = Environment(
            loader=FileSystemLoader(str(self._templates_dir)),
            undefined=undefined_cls,
            keep_trailing_newline=True,
            autoescape=False,
        )

        env.filters["upper_snake"] = _to_upper_snake
        env.filters["lower_snake"] = _to_lower_snake
        env.filters["c_bool"] = _c_bool
        env.filters["c_uint"] = _c_uint
        env.filters["c_hex"] = _c_hex

        for width in range(10, 61):
            env.filters[f"pad{width}"] = _pad_factory(width)

        return env

    def discover_templates(self, subdir: Optional[str] = None) -> List[Path]:
        search_dir = self._templates_dir / subdir if subdir else self._templates_dir
        if not search_dir.is_dir():
            return []
        return sorted(p for p in search_dir.rglob("*.j2") if p.is_file())

    @staticmethod
    def module_name_from_template(tpl_path: Path) -> str:
        name = tpl_path.stem
        for suffix in ("_Cfg.h", "_Cfg.c", "_cfg.h", "_cfg.c"):
            if name.endswith(suffix):
                return name[: -len(suffix)]
        return name.split("_Cfg")[0]

    def render_template(
        self,
        tpl_path: Path,
        config: Dict[str, Any],
    ) -> RenderResult:
        module_name = self.module_name_from_template(tpl_path)
        modules = config.get("modules", {})
        module_node = modules.get(module_name, {})

        if not module_node:
            for key, val in modules.items():
                if key.lower() == module_name.lower():
                    module_node = val
                    module_name = key
                    break

        rel_path = tpl_path.relative_to(self._templates_dir)
        modules_lookup = dict(modules)
        for k, v in modules.items():
            modules_lookup[k.lower()] = v
        context = {
            "module": module_node,
            "modules": modules_lookup,
            "cfg": config,
            "config_version": config.get("version", "unknown"),
            "project": config.get("project", {}),
            "target": config.get("target", {}),
        }

        content = self._env.get_template(str(rel_path)).render(**context)
        output_name = tpl_path.stem.replace(".j2", "")

        return RenderResult(
            template_name=tpl_path.name,
            output_name=output_name,
            content=content,
            module_name=module_name,
        )

    def render_all(
        self,
        config: Dict[str, Any],
        module_filter: Optional[List[str]] = None,
    ) -> List[RenderResult]:
        templates = self.discover_templates()
        results = []
        for tpl in templates:
            mod = self.module_name_from_template(tpl)
            if module_filter and mod not in module_filter:
                continue
            try:
                result = self.render_template(tpl, config)
                results.append(result)
            except Exception as e:
                LOG.error("template render failed for %s: %s", tpl.name, e)
                raise
        return results
