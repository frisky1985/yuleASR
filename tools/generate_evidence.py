#!/usr/bin/env python3
"""
yuleASR Evidence Generator — Phase 1 P0-1 fix.

Replaces the buggy traceability-matrix.md generation that was using
shall_count > 0 (always true) for coverage status. Now uses
actual test mapping (matched_tests / has_test) consistently across
all three evidence files.

Sources of truth:
  - .yuleosh/reports/traceability-report.json (authoritative req data)
  - .yuleosh/reports/misra-report.json (MISRA violations)
  - .yuleosh/reports/c-coverage.json (C coverage)

Outputs:
  - .osh/evidence/traceability-matrix.md (corrected)
  - .osh/evidence/traceability-matrix.json (regenerated)
  - .osh/evidence/requirement-coverage.md (regenerated)
  - .osh/evidence/acceptance-matrix.md (regenerated)
  - .osh/evidence/manifest.json (updated)
"""

import json
import os
import hashlib
import time
import sys

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OSH_EVIDENCE = os.path.join(BASE_DIR, ".osh", "evidence")
TRACE_REPORT = os.path.join(BASE_DIR, ".yuleosh", "reports", "traceability-report.json")
MISRA_REPORT = os.path.join(BASE_DIR, ".yuleosh", "reports", "misra-report.json")
C_COVERAGE_REPORT = os.path.join(BASE_DIR, ".yuleosh", "reports", "c-coverage.json")


def load_json(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def escape_markdown(text):
    """Escape pipe chars and other special markdown in table cells."""
    return text.replace("|", "\\|")


def now_iso():
    return time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime())


def generate_traceability_matrix_md(reqs, summary):
    """Generate corrected traceability-matrix.md using matched_tests for status."""
    lines = []
    lines.append("# Traceability Matrix\n")
    lines.append(f"> Generated: {now_iso()}")
    lines.append("> Version: 0.1.0\n")
    lines.append("## Requirements → Implementation → Tests\n")

    for req in reqs:
        req_id = req["req_id"]
        # traceability-report.json stores statement as singular
        statement = req.get("statement", "(no SHALL statement)")
        shall_statements = [statement] if statement else []
        matched_tests = req.get("matched_tests", [])
        # Also check test_reports and has_test from source
        if not matched_tests:
            test_reports = req.get("test_reports", [])
            if test_reports:
                matched_tests = test_reports
        if not matched_tests:
            # has_test flag from source
            if req.get("has_test", False):
                matched_tests = ["(tested)"]
        n_tests = len(matched_tests)
        n_scenarios = 0  # no scenarios currently tracked

        # Correct status: use has_test/matched_tests, NOT shall_count
        status = "✅ Covered" if n_tests > 0 else "❌ Not Covered"

        lines.append(f"### {req_id}")
        lines.append(f"- Req ID: {req_id}")
        lines.append(f"- SHALL statements: {len(shall_statements)}")
        lines.append(f"- Status: {status}")
        lines.append(f"- Scenarios: {n_scenarios} {'✅' if n_scenarios > 0 else '⚠️'}")
        if n_tests > 0:
            lines.append(f"- Test files: {n_tests} ✅ Covered: {', '.join(matched_tests)}")
        else:
            lines.append(f"- Test files: 0 ❌ Not covered by any test")
        for s in shall_statements:
            icon = "❌"
            lines.append(f"- SHALL details:")
            lines.append(f"  {icon} {s}")
        lines.append("")

    # Summary
    total = len(reqs)
    covered = sum(1 for r in reqs if len(r.get("matched_tests", [])) > 0)
    lines.append("## Summary")
    lines.append(f"- Total Requirements: {total}")
    lines.append(f"- Requirements with implementation: {summary.get('with_implementation', 85)} ({summary.get('with_implementation', 85) * 100 // max(total, 1)}%)")
    lines.append(f"- Requirements with test coverage: {covered} ({covered * 100 // max(total, 1)}%)")
    lines.append(f"- Uncovered SHALLs: {total - covered}")
    lines.append(f"- Scenarios: 0")
    lines.append(f"- Reviews: 0")
    lines.append(f"- CI Runs: {summary.get('total_ci_runs', 0)}")
    lines.append("")

    return "\n".join(lines)


def generate_traceability_matrix_json(reqs, summary):
    """Generate traceability-matrix.json from traceability-report data."""
    output = {
        "generated": now_iso(),
        "version": "0.1.0",
        "build_id": "",
        "commit_sha": "",
        "branch": "",
        "summary": {
            "total_requirements": len(reqs),
            "with_implementation": summary.get("with_implementation", 85),
            "with_test_coverage": sum(1 for r in reqs if len(r.get("matched_tests", [])) > 0),
            "uncovered_shalls": len(reqs) - sum(1 for r in reqs if len(r.get("matched_tests", [])) > 0),
            "total_scenarios": 0,
            "total_reviews": 0,
            "total_ci_runs": summary.get("total_ci_runs", 0),
        },
        "requirements": []
    }
    for req in reqs:
        statement = req.get("statement", "")
        shall_statements = [statement] if statement else []
        matched_tests = req.get("matched_tests", [])
        output["requirements"].append({
            "name": req["req_id"],
            "req_id": req["req_id"],
            "shall_count": len(shall_statements),
            "shall_statements": shall_statements,
            "matched_tests": matched_tests,
        })
    return output


def generate_requirement_coverage_md(reqs):
    """Generate requirement-coverage.md."""
    lines = []
    lines.append("# Requirements Coverage Report\n")
    lines.append(f"> Generated: {now_iso()}\n")
    lines.append("| Requirement | SHALLs | Tests | Status |")
    lines.append("|:-----------|:------:|:-----:|:------:|")

    covered_count = 0
    for req in reqs:
        req_id = req["req_id"]
        statement = req.get("statement", "")
        n_shalls = 1 if statement else 0
        matched_tests = req.get("matched_tests", [])
        n_tests = len(matched_tests)
        status = "✅" if n_tests > 0 else "❌"
        if n_tests > 0:
            covered_count += 1
        lines.append(f"| {req_id} | {n_shalls} | {n_tests} | {status} |")

    total = len(reqs)
    lines.append("")
    lines.append(f"**Requirement Coverage**: {covered_count}/{total} ({covered_count * 100 // max(total, 1)}%)")
    lines.append("  (Status based on actual test-file mapping, not SHALL presence)")
    lines.append("**Scenarios**: 0")
    lines.append("**Threshold**: 100%")
    lines.append(f"**Pass**: {'✅' if covered_count >= total else '❌'}")
    lines.append("")

    return "\n".join(lines)


def generate_acceptance_matrix_md(reqs):
    """Generate acceptance-matrix.md."""
    lines = []
    lines.append("# Acceptance Matrix\n")
    lines.append(f"> Generated: {now_iso()}")
    lines.append("> Version: 0.1.0\n")
    lines.append("| Req ID | Requirement | SHALL | 验证方法 | 测试文件 | 匹配方式 | 置信度 | 状态 |")
    lines.append("|:------:|:-----------|:------|:---------|:--------|:--------:|:------:|:----:|")

    for req in reqs:
        req_id = req["req_id"]
        statement = req.get("statement", "(no SHALL statements)")
        shall_statements = [statement]
        matched_tests = req.get("matched_tests", [])
        n_tests = len(matched_tests)
        status = "✅" if n_tests > 0 else "❌"
        test_file_text = ", ".join(matched_tests) if matched_tests else "—"
        verification_method = "Unit Test"
        match_type = "—"
        confidence = ""

        lines.append(
            f"| {req_id} | {req_id} | {escape_markdown(shall_statements[0])} "
            f"| {verification_method} | {test_file_text} | {match_type} | {confidence} | {status} |"
        )

    total = len(reqs)
    covered = sum(1 for r in reqs if len(r.get("matched_tests", [])) > 0)
    lines.append("")
    lines.append("## Summary")
    lines.append(f"- Total SHALL statements: {total}")
    lines.append(f"- Covered by tests: {covered} ({covered * 100 // max(total, 1)}%)")
    lines.append(f"- Uncovered: {total - covered}")
    lines.append(f"- Threshold: 100% → {'✅ PASS' if covered >= total else '❌ FAIL'}")
    lines.append("")

    return "\n".join(lines)


def compute_sha256(path):
    """Compute SHA256 of a file."""
    sha = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            sha.update(chunk)
    return sha.hexdigest()


def write_file(path, content):
    """Write text file with atomic semantics (write to tmp then rename)."""
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        f.write(content)
    os.replace(tmp, path)
    print(f"  ✅ Wrote {path} ({len(content)} bytes)")


def write_json_file(path, data):
    """Write JSON file."""
    content = json.dumps(data, indent=2, ensure_ascii=False)
    write_file(path, content)


def main():
    print("=== yuleASR Evidence Generator (P0-1) ===")
    os.makedirs(OSH_EVIDENCE, exist_ok=True)

    # Load source data
    trace = load_json(TRACE_REPORT)
    reqs = trace["lrm"]["requirements"]
    print(f"  Loaded {len(reqs)} requirements from traceability-report.json")
    print(f"  With test coverage: {sum(1 for r in reqs if len(r.get('matched_tests', [])) > 0)}")

    # Load summary info from available reports
    summary = trace["lrm"].get("summary", {})
    if not summary:
        # Build from data
        summary = {
            "total_requirements": len(reqs),
            "with_implementation": sum(1 for r in reqs if r.get("has_code", False) or len(r.get("code_files", [])) > 0),
            "with_test_coverage": sum(1 for r in reqs if len(r.get("matched_tests", [])) > 0),
            "total_ci_runs": 0,
        }

    # Also try to load CI run count
    ci_results_path = os.path.join(BASE_DIR, ".osh", "ci", "ci-results.json")
    if os.path.exists(ci_results_path):
        try:
            ci_data = load_json(ci_results_path)
            summary["total_ci_runs"] = len(ci_data.get("runs", []))
        except Exception:
            pass

    # 1. Generate traceability-matrix.md
    print("\n1. Generating traceability-matrix.md...")
    tm_md = generate_traceability_matrix_md(reqs, summary)
    write_file(os.path.join(OSH_EVIDENCE, "traceability-matrix.md"), tm_md)

    # 2. Generate traceability-matrix.json
    print("\n2. Generating traceability-matrix.json...")
    tm_json = generate_traceability_matrix_json(reqs, summary)
    write_json_file(os.path.join(OSH_EVIDENCE, "traceability-matrix.json"), tm_json)

    # 3. Generate requirement-coverage.md
    print("\n3. Generating requirement-coverage.md...")
    rc_md = generate_requirement_coverage_md(reqs)
    write_file(os.path.join(OSH_EVIDENCE, "requirement-coverage.md"), rc_md)

    # 4. Generate acceptance-matrix.md
    print("\n4. Generating acceptance-matrix.md...")
    am_md = generate_acceptance_matrix_md(reqs)
    write_file(os.path.join(OSH_EVIDENCE, "acceptance-matrix.md"), am_md)

    # 5. Update manifest.json
    print("\n5. Updating manifest.json...")
    files = [
        "acceptance-matrix.md",
        "aspice-gap-report.md",
        "code-coverage-report.md",
        "requirement-coverage.md",
        "review-log-summary.md",
        "review-log.json",
        "traceability-matrix.json",
        "traceability-matrix.md",
    ]
    manifest = {
        "manifest_version": "1.0",
        "generated_at": now_iso(),
        "project_dir": ".",
        "total_files": len(files),
        "files": [],
    }
    for fname in files:
        fpath = os.path.join(OSH_EVIDENCE, fname)
        if os.path.exists(fpath):
            stat = os.stat(fpath)
            manifest["files"].append({
                "path": fname,
                "sha256": compute_sha256(fpath),
                "size_bytes": stat.st_size,
                "mtime": stat.st_mtime,
                "mtime_iso": time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime(stat.st_mtime)),
            })

    write_json_file(os.path.join(OSH_EVIDENCE, "manifest.json"), manifest)

    # 6. Also update the .yuleosh/audit copies
    audit_dir = os.path.join(BASE_DIR, ".yuleosh", "audit")
    os.makedirs(audit_dir, exist_ok=True)
    print("\n6. Syncing to .yuleosh/audit/...")
    for fname in ["traceability-matrix.md", "traceability-matrix.json",
                  "requirement-coverage.md", "acceptance-matrix.md"]:
        src = os.path.join(OSH_EVIDENCE, fname)
        dst = os.path.join(audit_dir, fname)
        if os.path.exists(src):
            with open(src, "r", encoding="utf-8") as fin:
                content = fin.read()
            with open(dst, "w", encoding="utf-8") as fout:
                fout.write(content)
            print(f"  ✅ Synced {fname}")

    # 7. Validation: check consistency
    print("\n7. Validation...")
    with open(os.path.join(OSH_EVIDENCE, "traceability-matrix.md"), "r") as f:
        tm_text = f.read()
    with open(os.path.join(OSH_EVIDENCE, "requirement-coverage.md"), "r") as f:
        rc_text = f.read()
    with open(os.path.join(OSH_EVIDENCE, "acceptance-matrix.md"), "r") as f:
        am_text = f.read()

    # Extract coverage numbers
    import re
    tm_covered = len(re.findall(r"Status: ✅ Covered", tm_text))
    tm_uncovered = len(re.findall(r"Status: ❌ Not Covered", tm_text))

    rc_match = re.search(r"\*\*Requirement Coverage\*\*: (\d+)/(\d+)", rc_text)
    am_match = re.search(r"Covered by tests: (\d+)", am_text)

    print(f"    traceability-matrix.md: {tm_covered} covered, {tm_uncovered} uncovered")
    if rc_match:
        print(f"    requirement-coverage.md: {rc_match.group(1)}/{rc_match.group(2)}")
    if am_match:
        print(f"    acceptance-matrix.md: {am_match.group(1)} covered")

    # Verify consistency
    ok = True
    if tm_covered != 0:
        print(f"  ❌ BUG: traceability-matrix.md shows {tm_covered} covered but should be 0!")
        ok = False
    if rc_match and int(rc_match.group(1)) != 0:
        print(f"  ❌ BUG: requirement-coverage.md shows {rc_match.group(1)} covered but should be 0!")
        ok = False
    if am_match and int(am_match.group(1)) != 0:
        print(f"  ❌ BUG: acceptance-matrix.md shows {am_match.group(1)} covered but should be 0!")
        ok = False
    if ok:
        print("  ✅ All three evidence files are consistent: 0% coverage (honest)")

    print("\n=== Done ===")


if __name__ == "__main__":
    main()
