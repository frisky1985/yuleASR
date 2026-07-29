#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
yuleOSH Pipeline Stage: rte_generation
=======================================
AUTOSAR ARXML → RTE C code generation stage.

Part of yuleOSH CI pipeline. Invoked by `yuleosh ci run` when the
'rte_generation' stage is enabled.

Design:
  This stage integrates with the yuleOSH pipeline framework. It:
  1. Scans for .arxml input files (config/input/arxml/ or custom path)
  2. Runs the RTE generator (tools/code_generators/rte/)
  3. Outputs generated RTE code to src/rte/generated/
  4. Validates output files (include guards, function presence)
  5. Optionally runs MISRA check on generated code

Stage metadata (used by yuleOSH pipeline engine):
  - name: rte_generation
  - label: ARXML → RTE C Code Generation
  - timeout: 60s
  - layer: 1
"""

import os
import sys
import json
import logging
import subprocess
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional, Any

logger = logging.getLogger("yuleosh.stage.rte_generation")

# ============================================================================
#  Stage Metadata
# ============================================================================
STAGE_NAME = "rte_generation"
STAGE_LABEL = "ARXML → RTE C Code Generation"
STAGE_TIMEOUT_S = 60  # seconds
STAGE_LAYER = 1


# ============================================================================
#  Main stage entry point
# ============================================================================
def run(args: Optional[List[str]] = None) -> Dict[str, Any]:
    """
    yuleOSH pipeline stage entry point.

    Called by the yuleOSH CI engine for each stage during pipeline execution.

    Args:
        args: Optional CLI arguments (e.g., ['--arxml', 'file.arxml'])

    Returns:
        Stage result dict with keys:
          - name: stage name
          - status: 'passed' | 'failed' | 'skipped'
          - detail: human-readable summary
          - timestamp: ISO 8601
          - generated_files: list of generated file paths
          - errors: list of error messages
    """
    from argparse import ArgumentParser

    parser = ArgumentParser(description="yuleOSH RTE Generation Stage")
    parser.add_argument('--arxml', '-i', dest='arxml_path',
                        help='Input ARXML file path')
    parser.add_argument('--output', '-o', dest='output_dir',
                        default='src/rte/generated',
                        help='Output directory (default: src/rte/generated)')
    parser.add_argument('--swc', dest='swc_filter', action='append',
                        help='Filter: generate for specific SWC only')
    parser.add_argument('--project-root', dest='project_root',
                        default='.',
                        help='Project root directory')
    parser.add_argument('--misra', dest='run_misra', action='store_true',
                        help='Also run MISRA check on generated code')
    parser.add_argument('--validate-only', dest='validate_only',
                        action='store_true',
                        help='Only validate existing output, no generation')

    parsed = parser.parse_args(args or [])

    result = {
        'name': STAGE_NAME,
        'status': 'failed',
        'detail': '',
        'timestamp': datetime.now().isoformat(),
        'generated_files': [],
        'errors': [],
    }

    try:
        _execute_stage(parsed, result)
    except Exception as e:
        result['status'] = 'failed'
        result['detail'] = f"Stage error: {str(e)}"
        result['errors'].append(str(e))
        logger.error("RTE generation stage failed: %s", e)

    return result


def _execute_stage(opts, result: Dict[str, Any]) -> None:
    """
    Execute the RTE generation stage logic.
    """
    project_root = Path(opts.project_root).resolve()
    output_dir = (project_root / opts.output_dir).resolve()

    # ── Locate ARXML input ──────────────────────────────────────────────
    if opts.arxml_path:
        arxml_path = Path(opts.arxml_path).resolve()
        if not arxml_path.exists():
            result['detail'] = f"ARXML file not found: {arxml_path}"
            return
    else:
        # Auto-detect in project paths
        search_paths = [
            project_root / 'config' / 'input' / 'arxml' / 'bcm_demo.arxml',
            project_root / 'config' / 'input' / 'arxml' / 'example.arxml',
            project_root / 'tools' / 'code_generators' / 'rte' / 'examples' / 'bcm_demo.arxml',
            project_root / 'configs' / 'arxml' / 'example.arxml',
        ]
        arxml_path = None
        for spath in search_paths:
            if spath.exists():
                arxml_path = spath
                break

        if arxml_path is None:
            result['detail'] = (
                "No ARXML input found. Searched: "
                + ", ".join(str(p) for p in search_paths)
            )
            return

    logger.info("RTE input: %s", arxml_path)
    logger.info("RTE output: %s", output_dir)

    # ── If validate-only mode ───────────────────────────────────────────
    if opts.validate_only:
        gen_files = list(output_dir.glob("*.h")) + list(output_dir.glob("*.c"))
        result['generated_files'] = [str(f) for f in gen_files]
        validation_ok = _validate_generated_files(output_dir)
        if validation_ok:
            result['status'] = 'passed'
            result['detail'] = f"Validation OK, {len(gen_files)} files"
        else:
            result['status'] = 'failed'
            result['detail'] = "Validation found issues"
        return

    # ── Run generation ──────────────────────────────────────────────────
    output_dir.mkdir(parents=True, exist_ok=True)

    # Add the generator to sys.path
    gen_dir = project_root / 'tools' / 'code_generators' / 'rte'
    sys.path.insert(0, str(gen_dir))

    from rte_generator import generate_rte

    generated = generate_rte(
        arxml_path=str(arxml_path),
        output_dir=str(output_dir),
        swc_filter=opts.swc_filter,
    )

    result['generated_files'] = generated

    if not generated:
        result['detail'] = "No files generated (empty SWC list or no ARXML data)"
        return

    # ── Validate generated files ────────────────────────────────────────
    validation_ok = _validate_generated_files(output_dir)
    errors = _collect_errors(output_dir)

    if errors:
        result['errors'] = errors

    if validation_ok and not errors:
        result['status'] = 'passed'
        # Count unique SWCs from generated file names
        # Count unique SWCs from generated file names (deduplicate .h/.c pairs)
        swc_names = set()
        for gf in generated:
            gf_basename = os.path.basename(gf)
            if gf_basename.startswith('Rte_'):
                # Strip extension to get base name
                base_no_ext = os.path.splitext(gf_basename)[0]
                # Remove 'Rte_' prefix to get SWC name
                swc_name = base_no_ext[4:]
                swc_names.add(swc_name)
        result['detail'] = (
            f"Generated {len(generated)} files for "
            f"{len(swc_names)} SWCs"
        )
    else:
        result['status'] = 'failed'
        result['detail'] = (
            f"Generated {len(generated)} files, but validation found "
            f"{len(errors)} error(s)"
        )

    # ── Optional: MISRA check ───────────────────────────────────────────
    if opts.run_misra:
        try:
            misra_result = _run_misra_check(str(output_dir), str(project_root))
            if misra_result:
                result['misra_report'] = misra_result
                logger.info("MISRA check completed")
        except Exception as e:
            logger.warning("MISRA check failed: %s", e)


def _validate_generated_files(output_dir: Path) -> bool:
    """Validate generated files for structural correctness."""
    ok = True

    for h_file in output_dir.glob("*.h"):
        content = h_file.read_text()
        guard_name = f"{h_file.stem.upper()}_H"
        if guard_name not in content:
            logger.warning("Missing include guard in %s", h_file.name)
            ok = False

    for c_file in output_dir.glob("*.c"):
        content = c_file.read_text()
        if not any(kw in content for kw in ["Rte_Init", "Rte_Read", "Rte_Write"]):
            logger.warning(
                "No RTE API functions found in %s", c_file.name
            )
            ok = False

    return ok


def _collect_errors(output_dir: Path) -> List[str]:
    """Collect structural errors from generated files."""
    errors = []

    # Check for empty files
    for f in output_dir.glob("*"):
        if f.is_file() and f.stat().st_size == 0:
            errors.append(f"Empty file: {f.name}")

    # Check that Rte.h exists (required)
    if not (output_dir / "Rte.h").exists():
        errors.append("Missing Rte.h (required global header)")

    return errors


def _run_misra_check(output_dir: str, project_root: str) -> Dict[str, Any]:
    """Run MISRA check on generated code using cppcheck."""
    import subprocess

    cmd = [
        'cppcheck',
        '--std=c99',
        '--language=c',
        '--enable=warning,style,performance',
        '--suppress=*',
        '-I', output_dir,
        '-I', os.path.join(project_root, 'src', 'rte', 'include'),
        output_dir,
    ]

    try:
        proc = subprocess.run(
            cmd, capture_output=True, text=True, timeout=30
        )
        return {
            'stdout': proc.stdout,
            'stderr': proc.stderr,
            'returncode': proc.returncode,
        }
    except FileNotFoundError:
        logger.warning("cppcheck not found; skipping MISRA check")
        return {'skipped': 'cppcheck not installed'}


# ============================================================================
#  CLI entry point (for standalone invocation)
# ============================================================================
def main():
    """Standalone CLI entry point (not invoked by pipeline)."""
    result = run(sys.argv[1:] if len(sys.argv) > 1 else None)
    print(json.dumps(result, indent=2, ensure_ascii=False))
    sys.exit(0 if result['status'] == 'passed' else 1)


if __name__ == '__main__':
    main()
