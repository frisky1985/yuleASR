#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
yuleASR BSW Configuration Generator (Phase 6 closed loop)
==========================================================

Renders the pre-compile configuration headers (config/templates/*.h.j2)
from the single configuration source config/bsw_config.json and writes the
results to config/generated/.  Optionally validates the configuration
against its JSON Schema and orchestrates the ARXML-driven RTE generator.

Pipeline:
    config/bsw_config.json ──(JSON Schema)──► validation
              │
              ├──► Jinja2 templates (config/templates/*.h.j2)
              │        └──► config/generated/<Module>_Cfg.h
              │
              └──► RTE generator (tools/code_generators/rte)
                       └──► config/generated/rte/Rte*.{h,c}

Usage:
    python config/tools/generate_bsw_config.py                 # validate + generate
    python config/tools/generate_bsw_config.py --validate-only # schema check only
    python config/tools/generate_bsw_config.py --diff          # report drift vs committed
    python config/tools/generate_bsw_config.py --module BswM --module Os
    python config/tools/generate_bsw_config.py --no-rte --output /tmp/generated

Exit codes:
    0  success (or no drift in --diff mode)
    1  validation/generation error or configuration drift detected
    2  usage/environment error (missing files, unreadable JSON)
"""

import argparse
import difflib
import json
import logging
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Paths (repo-root relative, so the tool works from any CWD)
# ---------------------------------------------------------------------------
REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONFIG = REPO_ROOT / "config" / "bsw_config.json"
DEFAULT_SCHEMA = REPO_ROOT / "config" / "bsw_config_schema.json"
DEFAULT_TEMPLATES = REPO_ROOT / "config" / "templates"
DEFAULT_OUTPUT = REPO_ROOT / "config" / "generated"
RTE_GENERATOR_DIR = REPO_ROOT / "tools" / "code_generators" / "Rte"

LOG = logging.getLogger("generate_bsw_config")

# Deterministic build stamp for the RTE generator (its templates embed
# datetime.now()).  Overridable via SOURCE_DATE_EPOCH (reproducible-builds
# convention).  Keeps --diff / CI drift checks byte-stable.
DETERMINISTIC_DATETIME = (1970, 1, 1, 0, 0, 0)


# ---------------------------------------------------------------------------
# JSON helpers
# ---------------------------------------------------------------------------
def load_json(path: Path) -> Any:
    """Load a JSON document, raising a clear error on failure."""
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except OSError as e:
        raise SystemExit(f"[ENV-ERROR] Cannot read {path}: {e}")
    except ValueError as e:
        raise SystemExit(f"[ENV-ERROR] Invalid JSON in {path}: {e}")


def fallback_validate(cfg: Any) -> List[str]:
    """Minimal structural validation used when jsonschema is unavailable."""
    errors: List[str] = []
    if not isinstance(cfg, dict):
        return ["top level must be an object"]
    if "version" not in cfg:
        errors.append("missing required top-level key 'version'")
    modules = cfg.get("modules")
    if not isinstance(modules, dict) or not modules:
        errors.append("'modules' must be a non-empty object")
        return errors
    for name, node in modules.items():
        if not isinstance(node, dict):
            errors.append(f"modules.{name}: must be an object")
            continue
        if "name" not in node:
            errors.append(f"modules.{name}: missing 'name'")
        if not isinstance(node.get("enabled"), bool):
            errors.append(f"modules.{name}: 'enabled' must be a boolean")
    return errors


def validate_config(cfg: Any, schema_path: Path) -> Tuple[bool, List[str]]:
    """Validate the configuration, preferring jsonschema when installed."""
    try:
        import jsonschema  # type: ignore
    except ImportError:
        errors = fallback_validate(cfg)
        return (not errors), errors

    if not schema_path.exists():
        return True, []

    schema = load_json(schema_path)
    validator_cls = getattr(jsonschema, "Draft7Validator", None)
    if validator_cls is None:  # pragma: no cover - very old jsonschema
        errors = fallback_validate(cfg)
        return (not errors), errors
    validator = validator_cls(schema)
    errors = sorted(validator.iter_errors(cfg), key=lambda e: list(e.absolute_path))
    msgs = []
    for err in errors:
        loc = "/".join(str(p) for p in err.absolute_path) or "<root>"
        msgs.append(f"{loc}: {err.message}")
    return (not msgs), msgs


# ---------------------------------------------------------------------------
# Template discovery / rendering
# ---------------------------------------------------------------------------
def discover_templates(templates_dir: Path) -> List[Path]:
    """Return sorted *.j2 template paths directly inside templates_dir."""
    if not templates_dir.is_dir():
        raise SystemExit(f"[ENV-ERROR] Templates directory not found: {templates_dir}")
    return sorted(p for p in templates_dir.glob("*.j2") if p.is_file())


def module_name_from_template(tpl_path: Path) -> str:
    """BswM_Cfg.h.j2 -> BswM ; Can_Cfg.h.j2 -> Can."""
    return tpl_path.name.split("_Cfg")[0]


def display_path(path: Path) -> str:
    """Path relative to the repo root when possible, absolute otherwise.

    --output may point outside the repository (e.g. /tmp/generated for CI
    drift checks), so Path.relative_to(REPO_ROOT) alone would raise.
    """
    try:
        return str(path.resolve().relative_to(REPO_ROOT))
    except ValueError:
        return str(path)


def build_environment(templates_dir: Path):
    """Create the Jinja2 environment with alignment filters registered."""
    try:
        from jinja2 import Environment, FileSystemLoader
        from jinja2.exceptions import TemplateError
    except ImportError:
        raise SystemExit(
            "[ENV-ERROR] jinja2 is required: pip install jinja2"
        )

    # ChainableUndefined keeps `module.x.y | default(v)` working when the
    # whole chain is missing (Python 3.9-safe: jinja2>=2.11 provides it).
    undefined_cls = getattr(
        __import__("jinja2", fromlist=["ChainableUndefined"]),
        "ChainableUndefined",
        None,
    )
    env = Environment(
        loader=FileSystemLoader(str(templates_dir)),
        undefined=undefined_cls,
        keep_trailing_newline=True,
        autoescape=False,
    )

    # Register pad<N> filters used by the templates for column alignment.
    # The filter always guarantees at least one trailing space so that macro
    # names longer than the pad width never merge with their value.
    def _pad(width):
        def _f(v):
            s = str(v)
            return s.ljust(width) if len(s) < width else s + " "
        return _f
    
    for width in range(10, 61):
        env.filters[f"pad{width}"] = _pad(width)

    return env


def render_template(env, tpl_path: Path, cfg: Dict[str, Any]) -> str:
    """Render one template against the unified configuration document."""
    module_name = module_name_from_template(tpl_path)
    module_node = cfg.get("modules", {}).get(module_name, {})
    context = {
        "module": module_node,
        "cfg": cfg,
        "config_version": cfg.get("version", "unknown"),
        "project": cfg.get("project", {}),
    }
    return env.get_template(tpl_path.name).render(**context)


# ---------------------------------------------------------------------------
# Generation / drift detection
# ---------------------------------------------------------------------------
def generate(
    env,
    templates: List[Path],
    cfg: Dict[str, Any],
    output_dir: Path,
    module_filter: Optional[List[str]],
    write: bool = True,
) -> Tuple[List[str], List[str], bool]:
    """Render templates; write outputs unless write=False (diff mode).

    Returns (generated_paths, drift_reports, ok).
    """
    generated: List[str] = []
    drifts: List[str] = []
    ok = True

    enabled_map = cfg.get("modules", {})
    for tpl in templates:
        mod = module_name_from_template(tpl)
        if module_filter and mod not in module_filter:
            continue
        node = enabled_map.get(mod)
        if isinstance(node, dict) and node.get("enabled") is False:
            LOG.warning("module %s is disabled in config — still rendering (defaults apply)", mod)

        try:
            content = render_template(env, tpl, cfg)
        except Exception as e:  # jinja2.TemplateError and friends
            template_info = getattr(e, "filename", tpl.name)
            lineno = getattr(e, "lineno", None)
            where = f"{template_info}:{lineno}" if lineno else str(template_info)
            LOG.error("template render failed (%s): %s", where, e)
            ok = False
            continue

        out_file = output_dir / tpl.name.replace(".j2", "")
        out_display = display_path(out_file)
        if write:
            out_file.parent.mkdir(parents=True, exist_ok=True)
            out_file.write_text(content, encoding="utf-8")
            generated.append(out_display)
            print(f"  [OK]   {tpl.name:22s} -> {out_display}")
        else:
            if out_file.exists():
                committed = out_file.read_text(encoding="utf-8")
                if committed != content:
                    drifts.append(f"{out_display} differs from committed version")
                    diff = difflib.unified_diff(
                        committed.splitlines(keepends=True),
                        content.splitlines(keepends=True),
                        fromfile=f"committed/{out_file.name}",
                        tofile=f"generated/{out_file.name}",
                    )
                    sys.stdout.writelines(diff)
                    ok = False
                else:
                    print(f"  [OK]   {tpl.name:22s} (no drift)")
            else:
                drifts.append(f"{out_display} is not committed yet")
                print(f"  [NEW]  {tpl.name:22s} (not present in output dir)")
            generated.append(out_display)

    return generated, drifts, ok


# ---------------------------------------------------------------------------
# RTE generation integration
# ---------------------------------------------------------------------------
def _freeze_rte_datetime() -> None:
    """Patch rte_generator.datetime so generated RTE files are deterministic.

    The RTE generator embeds datetime.now() in every generated file, which
    would make two identical runs differ byte-wise and break drift checks.
    Replacing the module-level name with a frozen subclass keeps the source
    untouched while producing reproducible output.
    """
    import datetime as _dt

    epoch = 0
    import os
    sde = os.environ.get("SOURCE_DATE_EPOCH")
    if sde and sde.isdigit():
        epoch = int(sde)

    class _FrozenDateTime(_dt.datetime):
        @classmethod
        def now(cls, tz=None):  # noqa: N805 - classmethod on subclass
            return cls(*DETERMINISTIC_DATETIME, tzinfo=tz)

    # Exported for rte_generator's `from datetime import datetime` name.
    return _FrozenDateTime  # noqa: RET501 - returned for explicit binding


def run_rte_generation(cfg: Dict[str, Any], output_dir: Path, strict: bool) -> bool:
    """Invoke the ARXML RTE generator as configured in cfg['Rte']."""
    rte_cfg = cfg.get("Rte") or {}
    if not rte_cfg.get("enabled", False):
        LOG.info("RTE generation disabled in config — skipping")
        return True

    arxml = REPO_ROOT / rte_cfg.get("arxml", "config/input/arxml/example.arxml")
    if not arxml.exists():
        if strict:
            LOG.error("RTE input not found: %s", arxml)
            return False
        LOG.warning("RTE input not found (%s) — skipping RTE generation", arxml)
        return True

    sys.path.insert(0, str(RTE_GENERATOR_DIR))
    try:
        import rte_generator  # type: ignore
    except ImportError as e:
        if strict:
            LOG.error("Cannot import rte_generator: %s", e)
            return False
        LOG.warning("Cannot import rte_generator (%s) — skipping RTE generation", e)
        return True

    # Make the RTE generator deterministic (see _freeze_rte_datetime).
    frozen_dt = _freeze_rte_datetime()
    rte_generator.datetime = frozen_dt

    gen_flags = rte_cfg.get("generate", {})
    out_sub = output_dir / rte_cfg.get("output_subdir", "Rte")
    try:
        generated = rte_generator.generate_rte(
            arxml_path=str(arxml),
            output_dir=str(out_sub),
            swc_filter=rte_cfg.get("swc_filter") or None,
            generate_rte_h=gen_flags.get("rte_h", True),
            generate_rte_c=gen_flags.get("rte_c", True),
            generate_rte_type_h=gen_flags.get("rte_type_h", True),
            behavior_settings=rte_cfg.get("behavior"),
        )
        for g in generated:
            try:
                rel = Path(g).resolve().relative_to(REPO_ROOT)
            except ValueError:
                rel = Path(g)
            print(f"  [OK]   RTE: {rel}")
        return True
    except Exception as e:  # noqa: BLE001 - report and continue
        if strict:
            LOG.error("RTE generation failed: %s", e)
            return False
        LOG.warning("RTE generation failed (%s) — BSW headers are unaffected", e)
        return True


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="yuleASR BSW configuration generator: JSON + Jinja2 closed loop",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  %(prog)s                              # validate + generate into config/generated\n"
            "  %(prog)s --validate-only              # schema validation only\n"
            "  %(prog)s --diff                       # drift check vs committed outputs\n"
            "  %(prog)s --module BswM --module CanTp # generate only selected modules\n"
            "  %(prog)s --output /tmp/generated      # redirect output (CI drift check)\n"
        ),
    )
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG,
                        help=f"configuration source JSON (default: {DEFAULT_CONFIG})")
    parser.add_argument("--schema", type=Path, default=DEFAULT_SCHEMA,
                        help=f"JSON Schema for validation (default: {DEFAULT_SCHEMA})")
    parser.add_argument("--templates", type=Path, default=DEFAULT_TEMPLATES,
                        help=f"template directory (default: {DEFAULT_TEMPLATES})")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT,
                        help=f"output directory (default: {DEFAULT_OUTPUT})")
    parser.add_argument("--module", action="append", dest="modules", metavar="NAME",
                        help="restrict generation to a module (repeatable)")
    parser.add_argument("--validate-only", action="store_true",
                        help="validate configuration against the schema and exit")
    parser.add_argument("--no-schema", action="store_true",
                        help="skip JSON Schema validation")
    parser.add_argument("--diff", action="store_true",
                        help="render without writing; report differences against "
                             "committed files in the output directory (exit 1 on drift)")
    parser.add_argument("--no-rte", action="store_true",
                        help="skip the ARXML RTE generation step")
    parser.add_argument("--strict-rte", action="store_true",
                        help="fail when RTE generation is impossible or errors")
    parser.add_argument("-q", "--quiet", action="store_true",
                        help="suppress per-file output lines")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="verbose logging")
    return parser


def main(argv: Optional[List[str]] = None) -> int:
    args = build_arg_parser().parse_args(argv)
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else (logging.ERROR if args.quiet else logging.INFO),
        format="[gen-cfg] %(levelname)s %(message)s",
    )

    if not args.config.exists():
        print(f"[ENV-ERROR] Configuration not found: {args.config}", file=sys.stderr)
        return 2

    cfg = load_json(args.config)
    print(f"[OK] Loaded config: {args.config} (config version {cfg.get('version', '?')})")

    if not args.no_schema:
        valid, errors = validate_config(cfg, args.schema)
        if not valid:
            print(f"[FAIL] Configuration failed schema validation ({args.schema}):", file=sys.stderr)
            for err in errors:
                print(f"       - {err}", file=sys.stderr)
            return 1
        try:
            import jsonschema  # noqa: F401
            print(f"[OK] Schema validation passed ({args.schema.name})")
        except ImportError:
            print("[WARN] jsonschema not installed — used fallback structural validation")
    else:
        print("[SKIP] Schema validation disabled (--no-schema)")

    if args.validate_only:
        print("[OK] Validate-only mode: configuration is valid")
        return 0

    templates = discover_templates(args.templates)
    if not templates:
        print(f"[FAIL] No *.j2 templates found in {args.templates}", file=sys.stderr)
        return 2
    if not args.quiet:
        print(f"[OK] Discovered {len(templates)} template(s) in {args.templates}")

    env = build_environment(args.templates)
    mode = "drift check" if args.diff else "generation"
    if not args.quiet:
        print(f"[..] Running {mode} -> {args.output}")

    generated, drifts, ok = generate(
        env=env,
        templates=templates,
        cfg=cfg,
        output_dir=args.output,
        module_filter=args.modules,
        write=not args.diff,
    )

    if not args.no_rte and not args.diff:
        run_rte_generation(cfg, args.output, strict=args.strict_rte)

    if args.diff:
        if ok and not drifts:
            print("[OK] No configuration drift: committed outputs match config")
            return 0
        print(f"[FAIL] Configuration drift detected ({len(drifts)} file(s)):", file=sys.stderr)
        for d in drifts:
            print(f"       - {d}", file=sys.stderr)
        return 1

    if not ok:
        print("[FAIL] One or more templates failed to render", file=sys.stderr)
        return 1
    print(f"[OK] Generation complete: {len(generated)} file(s) in {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
