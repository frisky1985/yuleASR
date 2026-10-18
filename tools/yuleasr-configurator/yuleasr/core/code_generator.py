"""Code generation pipeline orchestrator.

Pipeline: load → validate → resolve → render → write
"""

from __future__ import annotations

import difflib
import logging
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional

from yuleasr.core.config_loader import ConfigLoadError, load_config
from yuleasr.core.schema_engine import SchemaRegistry, ValidationResult, fallback_validate
from yuleasr.core.template_engine import RenderResult, TemplateEngine

LOG = logging.getLogger(__name__)


@dataclass
class GenerationReport:
    """Summary of a generation run."""
    rendered: List[str] = field(default_factory=list)
    written: List[str] = field(default_factory=list)
    drifts: List[str] = field(default_factory=list)
    validation: Optional[ValidationResult] = None
    ok: bool = True
    errors: List[str] = field(default_factory=list)


class CodeGenerator:
    """Orchestrates the full config → C code pipeline."""

    def __init__(
        self,
        config_path: Path,
        templates_dir: Path,
        output_dir: Path,
        schema_dir: Optional[Path] = None,
    ):
        self._config_path = config_path
        self._templates_dir = templates_dir
        self._output_dir = output_dir
        self._schema_dir = schema_dir

    def run(
        self,
        module_filter: Optional[List[str]] = None,
        write: bool = True,
        validate: bool = True,
    ) -> GenerationReport:
        report = GenerationReport()

        config = self._load(report)
        if not report.ok:
            return report

        if validate:
            self._validate(config, report)
            if not report.ok:
                return report

        engine = TemplateEngine(self._templates_dir)
        try:
            results = engine.render_all(config, module_filter=module_filter)
        except Exception as e:
            report.ok = False
            report.errors.append(f"template rendering failed: {e}")
            return report

        for r in results:
            report.rendered.append(r.output_name)

        if write:
            self._write(results, report)
        else:
            self._check_drift(results, report)

        return report

    def _load(self, report: GenerationReport) -> Dict[str, Any]:
        try:
            return load_config(self._config_path)
        except ConfigLoadError as e:
            report.ok = False
            report.errors.append(str(e))
            return {}

    def _validate(self, config: Dict[str, Any], report: GenerationReport) -> None:
        if self._schema_dir and self._schema_dir.is_dir():
            registry = SchemaRegistry(self._schema_dir)
            result = registry.validate_all(config)
        else:
            result = fallback_validate(config)

        report.validation = result
        if not result.ok:
            report.ok = False
            for e in result.errors:
                if e.severity == "error":
                    report.errors.append(str(e))

    def _write(self, results: List[RenderResult], report: GenerationReport) -> None:
        self._output_dir.mkdir(parents=True, exist_ok=True)
        for r in results:
            out_path = self._output_dir / r.output_name
            out_path.parent.mkdir(parents=True, exist_ok=True)
            out_path.write_text(r.content, encoding="utf-8")
            report.written.append(str(out_path))
            LOG.info("wrote %s", out_path)

    def _check_drift(self, results: List[RenderResult], report: GenerationReport) -> None:
        for r in results:
            out_path = self._output_dir / r.output_name
            if not out_path.exists():
                report.drifts.append(f"{r.output_name} not present (new)")
                continue
            committed = out_path.read_text(encoding="utf-8")
            if committed != r.content:
                report.drifts.append(f"{r.output_name} differs from committed")
                report.ok = False
                diff = difflib.unified_diff(
                    committed.splitlines(keepends=True),
                    r.content.splitlines(keepends=True),
                    fromfile=f"committed/{r.output_name}",
                    tofile=f"generated/{r.output_name}",
                )
                sys.stdout.writelines(diff)
