"""
yuleASR Evidence Pack — Project-specific override for CI Layer 3.

CI L3 does:
    sys.path.insert(0, os.path.join(project_dir, "src"))
    from evidence import pack as evidence_pack
    evidence_pack.generate_evidence(project_dir)

This module overrides the generic yuleOSH evidence pack and delegates
to the project's own tools/generate_evidence.py, which correctly reads
traceability-report.json (where matched_tests are already populated).
"""

import os
import sys
import subprocess


def generate_evidence(project_dir: str = None) -> list:
    """Generate evidence using the project-specific script.

    Loads from traceability-report.json which already has correct
    SHALL-to-test mappings.
    """
    if project_dir is None:
        project_dir = os.environ.get("OSH_HOME", os.getcwd())

    generate_script = os.path.join(project_dir, "tools", "generate_evidence.py")
    if not os.path.exists(generate_script):
        # Fall back to yuleOSH generic evidence pack
        print("  ⚠️  Project-specific generate_evidence.py not found, falling back to generic")
        from yuleosh.evidence.compliance import generate_evidence as _generic_gen
        return _generic_gen(project_dir=project_dir)

    print(f"  📦 yuleASR evidence: running project-specific generator...")
    result = subprocess.run(
        [sys.executable, generate_script],
        cwd=project_dir,
        capture_output=True,
        text=True,
    )
    print(result.stdout)
    if result.returncode != 0:
        print(f"  ❌ Generator failed (exit {result.returncode})")
        if result.stderr:
            print(f"     stderr: {result.stderr[:500]}")
        # Fall back to generic on failure
        from yuleosh.evidence.compliance import generate_evidence as _generic_gen
        return _generic_gen(project_dir=project_dir)

    return []
