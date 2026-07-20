#!/usr/bin/env python3
"""Generate traceability-matrix.md from JSON with C source file mappings."""

import json
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent


def generate_md(data):
    lines = []
    lines.append("# Traceability Matrix")
    lines.append("")
    lines.append(f"> Generated: {data.get('generated', 'unknown')}")
    lines.append(f"> Version: {data.get('mapping_version', data.get('version', '0.1.0'))}")
    lines.append("")
    lines.append("## Requirements → Implementation → Tests")
    lines.append("")

    for req in data["requirements"]:
        name = req["name"]
        shall_count = req.get("shall_count", 0)
        status = "✅ Covered" if req.get("matched_tests") else "⚠️ Uncovered"
        impl_by = req.get("implemented_by", [])
        tests = req.get("matched_tests", [])
        shalls = req.get("shall_statements", [])

        lines.append(f"### {name}")
        lines.append(f"- Req ID: {req.get('req_id', name)}")
        lines.append(f"- SHALL statements: {shall_count}")
        lines.append(f"- Status: {status}")

        # Show implementation source files
        if impl_by:
            for s in impl_by:
                lines.append(f"- 📄 Impl: `{s}`")
        else:
            lines.append(f"- Impl: ⚠️ No C source mapping")

        # Show test files
        if tests:
            for t in tests:
                lines.append(f"- 🧪 Test: `{t}`")
        else:
            lines.append(f"- Test: ⚠️ No test mapping")

        # Show SHALL details
        for s in shalls:
            lines.append(f"- SHALL: {s}")

        lines.append("")

    # Summary
    lines.append("## Summary")
    lines.append("")
    s = data.get("summary", {})
    lines.append(f"- Total requirements: {s.get('total_requirements', 0)}")
    lines.append(f"- With C implementation: {s.get('with_implementation', 0)}")
    lines.append(f"- With test coverage: {s.get('with_test_coverage', 0)}")
    lines.append(f"- Uncovered SHALLs: {s.get('uncovered_shalls', 0)}")

    return "\n".join(lines)


def main():
    trace_path = PROJECT_ROOT / ".yuleosh" / "audit" / "traceability-matrix.json"

    with open(trace_path) as f:
        data = json.load(f)

    md_content = generate_md(data)

    out_paths = [
        PROJECT_ROOT / ".yuleosh" / "audit" / "traceability-matrix.md",
        PROJECT_ROOT / ".osh" / "evidence" / "traceability-matrix.md",
    ]

    for out_path in out_paths:
        with open(out_path, "w") as f:
            f.write(md_content)

    print("✅ Updated traceability-matrix.md (both locations)")


if __name__ == "__main__":
    main()
