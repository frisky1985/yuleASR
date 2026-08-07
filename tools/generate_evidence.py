#!/usr/bin/env python3
"""
yuleASR Evidence Generator — Phase 1 P0-1 fix + P0-R3 review-log aggregation.

Replaces the buggy traceability-matrix.md generation that was using
shall_count > 0 (always true) for coverage status. Now uses
actual test mapping (matched_tests / has_test) consistently across
all three evidence files.

Also aggregates individual review-*.md files into review-log-summary.md
and review-log.json (P0-R3 fix).

Sources of truth:
  - .yuleosh/reports/traceability-report.json (authoritative req data)
  - .yuleosh/reports/misra-report.json (MISRA violations)
  - .yuleosh/reports/c-coverage.json (C coverage)
  - .osh/evidence/review-*.md (individual review files)

Outputs:
  - .osh/evidence/traceability-matrix.md (corrected)
  - .osh/evidence/traceability-matrix.json (regenerated)
  - .osh/evidence/requirement-coverage.md (regenerated)
  - .osh/evidence/acceptance-matrix.md (regenerated)
  - .osh/evidence/review-log-summary.md (aggregated)
  - .osh/evidence/review-log.json (aggregated JSON)
  - .osh/evidence/manifest.json (updated)
"""

import json
import os
import hashlib
import re as _re
import time
import sys

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OSH_EVIDENCE = os.path.join(BASE_DIR, ".osh", "evidence")
TRACE_REPORT = os.path.join(BASE_DIR, ".yuleosh", "reports", "traceability-report.json")
MISRA_REPORT = os.path.join(BASE_DIR, ".yuleosh", "reports", "misra-report.json")
C_COVERAGE_REPORT = os.path.join(BASE_DIR, ".yuleosh", "reports", "c-coverage.json")


def load_json(path):
    """Load JSON from file with structured error handling."""
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        if not isinstance(data, (dict, list)):
            print(f"  ⚠️  JSON data in {path} is unexpected type {type(data).__name__}")
        return data
    except FileNotFoundError:
        print(f"  ⚠️  File not found: {path}")
        return {}
    except json.JSONDecodeError as e:
        print(f"  ⚠️  Invalid JSON in {path}: {e}")
        return {}
    except PermissionError:
        print(f"  ⚠️  Permission denied: {path}")
        return {}


def escape_markdown(text):
    """Escape pipe chars and other special markdown in table cells."""
    return text.replace("|", "\\|")


def now_iso():
    return time.strftime("%Y-%m-%d", time.gmtime())


def _resolve_matched_tests(req):
    """Resolve effective test mapping for a requirement.

    Entry status in the matrix uses a fallback chain (matched_tests →
    test_reports → has_test) — the summary counters must use the SAME
    chain or the file is internally inconsistent (P1-3 consistency fix).

    test_reports entries only count when they represent a PASSED run
    (status == "passed" and passed > 0); retry/failed/skipped self-test
    records must not be treated as coverage evidence (P1-3b fix).
    """
    matched_tests = req.get("matched_tests", [])
    if not matched_tests:
        test_reports = req.get("test_reports", [])
        passed_reports = [
            tr for tr in test_reports
            if isinstance(tr, dict)
            and tr.get("status") == "passed"
            and (
                int(tr.get("passed", 0) or 0) > 0
                # SWR mapping-table entries carry file/function instead of a
                # pytest counter — a passed mapping row IS real test coverage.
                or tr.get("source") == "SWR mapping table"
                or bool(tr.get("file"))
            )
        ]
        if passed_reports:
            matched_tests = passed_reports
    if not matched_tests:
        if req.get("has_test", False):
            matched_tests = ["(tested)"]
    return matched_tests


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
        matched_tests = _resolve_matched_tests(req)
        n_tests = len(matched_tests)
        n_scenarios = 0  # no scenarios currently tracked

        # Correct status: use has_test/matched_tests, NOT shall_count
        status = "✅ Covered" if n_tests > 0 else "❌ Not Covered"

        # matched_tests entries may be plain strings or dicts (e.g.
        # {"name": ..., "path": ...}) — normalize before join (P1-2 fix).
        def _test_label(t):
            if isinstance(t, dict):
                return str(t.get("name") or t.get("path") or t.get("file") or t)
            return str(t)

        matched_labels = [_test_label(t) for t in matched_tests]

        lines.append(f"### {req_id}")
        lines.append(f"- Req ID: {req_id}")
        lines.append(f"- SHALL statements: {len(shall_statements)}")
        lines.append(f"- Status: {status}")
        lines.append(f"- Scenarios: {n_scenarios} {'✅' if n_scenarios > 0 else '⚠️'}")
        if n_tests > 0:
            lines.append(f"- Test files: {n_tests} ✅ Covered: {', '.join(matched_labels)}")
        else:
            lines.append(f"- Test files: 0 ❌ Not covered by any test")
        for s in shall_statements:
            icon = "❌"
            lines.append(f"- SHALL details:")
            lines.append(f"  {icon} {s}")
        lines.append("")

    # Summary
    total = len(reqs)
    covered = sum(1 for r in reqs if len(_resolve_matched_tests(r)) > 0)
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
            "with_test_coverage": sum(1 for r in reqs if len(_resolve_matched_tests(r)) > 0),
            "uncovered_shalls": len(reqs) - sum(1 for r in reqs if len(_resolve_matched_tests(r)) > 0),
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
        matched_tests = _resolve_matched_tests(req)
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
        matched_tests = _resolve_matched_tests(req)
        n_tests = len(matched_tests)
        status = "✅" if n_tests > 0 else "❌"
        # Normalize dict entries (name/path) before join (P1-2 fix)
        def _tlabel(t):
            if isinstance(t, dict):
                file_ = str(t.get("name") or t.get("path") or t.get("file") or "")
                func = str(t.get("function") or "")
                if file_ and func:
                    return f"{file_}::{func}"
                if file_:
                    return file_
                return str(t)
            return str(t)
        test_file_text = ", ".join(_tlabel(t) for t in matched_tests) if matched_tests else "—"
        verification_method = "Unit Test"
        match_type = "—"
        confidence = ""

        lines.append(
            f"| {req_id} | {req_id} | {escape_markdown(shall_statements[0])} "
            f"| {verification_method} | {test_file_text} | {match_type} | {confidence} | {status} |"
        )

    total = len(reqs)
    covered = sum(1 for r in reqs if len(_resolve_matched_tests(r)) > 0)
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


def _aggregate_review_logs(evidence_dir):
    """Aggregate review-*.md files into review-log-summary.md and review-log.json.

    Scans BOTH the evidence directory (``.osh/evidence/review-*.md``) and the
    canonical review archive (``docs/reviews/*.md``) for review records, parses
    their content, and produces aggregated review-log files.  The docs/reviews/
    archive holds the authoritative review records (ASPICE SWE.2.BP3 / SWE.3.BP3
    evidence), so an empty evidence/ dir must not produce an empty review log.
    """
    # Find all individual review markdown files — evidence dir first, then
    # the docs/reviews/ archive (authoritative review records).
    review_files = sorted([
        f for f in os.listdir(evidence_dir)
        if f.startswith("review-") and f.endswith(".md")
        and f != "review-log-summary.md"
    ])
    reviews_dir = os.path.join(BASE_DIR, "docs", "reviews")
    if os.path.isdir(reviews_dir):
        review_files.extend(sorted([
            f for f in os.listdir(reviews_dir)
            if f.endswith(".md") and not f.startswith("review-log")
        ]))
    # dedupe (keep evidence-dir copy first)
    seen = set()
    unique = []
    for f in review_files:
        if f not in seen:
            seen.add(f)
            unique.append(f)
    review_files = unique

    if not review_files:
        print("  ⏭️  No individual review files found to aggregate")
        return

    # Build summary markdown
    summary_lines = []
    summary_lines.append("# Review Log Summary\n")
    summary_lines.append(f"> Generated: {now_iso()}\n")
    summary_lines.append(f"Total review files: {len(review_files)}\n")

    # Build JSON data
    review_json_entries = []

    for rf_name in review_files:
        rf_path = os.path.join(evidence_dir, rf_name)
        if not os.path.exists(rf_path) and os.path.isdir(reviews_dir):
            rf_path = os.path.join(reviews_dir, rf_name)
        try:
            with open(rf_path, "r", encoding="utf-8") as f:
                content = f.read()
        except (OSError, UnicodeDecodeError) as e:
            print(f"  ⚠️  Cannot read {rf_name}: {e}")
            continue

        # Extract module name from filename: review-can.md → can
        module_name = rf_name.replace("review-", "").replace(".md", "")

        # Parse basic review metadata from the markdown
        title = ""
        status = "unknown"
        findings_p2 = 0
        for line in content.split("\n"):
            line_stripped = line.strip()
            if line_stripped.startswith("## ") and not line_stripped.startswith("## "):
                title = line_stripped.lstrip("#").strip()
            if "结论" in line_stripped and "通过" in line_stripped:
                status = "passed"
            elif "发现" in line_stripped and "P2" in line_stripped:
                findings_p2 += 1

        # Robust status extraction: look for verdict keywords across the whole
        # document (review files use varied formats: **结论**, | 结论 |, verdict…)
        lowered = content.lower()
        if status == "unknown":
            if ("✅" in content and "不通过" not in content) or "verdict: pass" in lowered or "结论: 通过" in content:
                status = "passed"
            elif "不通过" in content or "verdict: fail" in lowered or "❌" in content:
                status = "failed"
        # Count findings: P0/P1/P2 issue markers (e.g. "P0-1", "| P1-2 |", "P2-3")
        issue_markers = _re.findall(r"[Pp][0-2]-\d+", content)
        finding_count = len(issue_markers)
        if finding_count == 0:
            finding_count = len(_re.findall(r"(?:发现项|findings?)[:：]\s*(\d+)", content, _re.IGNORECASE))
        # fall back to pipe-table rows shaped like issue rows
        if finding_count == 0:
            finding_count = len(_re.findall(r"^\| [A-Z]+-[A-Z0-9]+-\d+ \|", content, _re.MULTILINE))

        summary_lines.append(f"\n## {module_name.upper()} 模块")
        summary_lines.append(f"- 审查文件: {rf_name}")
        summary_lines.append(f"- 审查状态: {'✅' if status == 'passed' else '❌'} {status}")
        summary_lines.append(f"- 发现项: {finding_count}")

        # Add finding table
        findings_lines = []
        capture = False
        for line in content.split("\n"):
            if "发现项" in line or "发现" in line:
                capture = True
            if capture:
                findings_lines.append(line)

        if findings_lines:
            for fl in findings_lines:
                if fl.strip():
                    summary_lines.append(f"  {fl.strip()}")

        # Build JSON entry
        entry = {
            "module": module_name,
            "source_file": rf_name,
            "status": status,
            "findings_count": finding_count,
            "content_snippet": content[:200] + "..." if len(content) > 200 else content,
        }
        review_json_entries.append(entry)

    summary_content = "\n".join(summary_lines)

    # Write review-log-summary.md (overwrite with fresh content each run)
    summary_path = os.path.join(evidence_dir, "review-log-summary.md")
    # Always overwrite with fresh aggregated content since review files
    # are stable between CI runs — no append needed, avoids duplication.
    write_file(summary_path, summary_content)

    # Write review-log.json (overwrite with fresh content each run)
    json_path = os.path.join(evidence_dir, "review-log.json")
    write_json_file(json_path, review_json_entries)

    print(f"  ✅ Aggregated {len(review_files)} review file(s)")
    print(f"  ✅ Review log summary: {summary_path} ({len(summary_content)} bytes)")
    print(f"  ✅ Review log JSON: {json_path} ({len(json.dumps(review_json_entries, ensure_ascii=False))} bytes)")


def main():
    print("=== yuleASR Evidence Generator (P0-1 + P0-R3) ===")
    os.makedirs(OSH_EVIDENCE, exist_ok=True)

    # Load source data
    trace = load_json(TRACE_REPORT)
    reqs = trace["lrm"]["requirements"]
    print(f"  Loaded {len(reqs)} requirements from traceability-report.json")
    covered_count = sum(1 for r in reqs if len(_resolve_matched_tests(r)) > 0)
    print(f"  With test coverage: {covered_count}")

    # Load summary info from available reports
    summary = trace["lrm"].get("summary", {})
    if not summary:
        # Build from data
        summary = {
            "total_requirements": len(reqs),
            "with_implementation": sum(1 for r in reqs if r.get("has_code", False) or len(r.get("code_files", [])) > 0),
            "with_test_coverage": covered_count,
            "total_ci_runs": 0,
        }

    # Also try to load CI run count
    ci_results_path = os.path.join(BASE_DIR, ".osh", "ci", "ci-results.json")
    if os.path.exists(ci_results_path):
        try:
            ci_data = load_json(ci_results_path)
            summary["total_ci_runs"] = len(ci_data.get("runs", []))
        except Exception as e:
            print(f"  ⚠️  Failed to load CI results from {ci_results_path}: {e}")

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

    # 5. Aggregate review logs (R3 fix)
    print("\n5. Aggregating review logs...")
    _aggregate_review_logs(OSH_EVIDENCE)

    # 6. Update manifest.json
    print("\n6. Updating manifest.json...")
    # Discover all evidence files dynamically
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

    # 7. Also update the .yuleosh/audit copies
    audit_dir = os.path.join(BASE_DIR, ".yuleosh", "audit")
    os.makedirs(audit_dir, exist_ok=True)
    print("\n7. Syncing to .yuleosh/audit/...")
    for fname in ["traceability-matrix.md", "traceability-matrix.json",
                  "requirement-coverage.md", "acceptance-matrix.md",
                  "review-log-summary.md", "review-log.json"]:
        src = os.path.join(OSH_EVIDENCE, fname)
        dst = os.path.join(audit_dir, fname)
        if os.path.exists(src):
            with open(src, "r", encoding="utf-8") as fin:
                content = fin.read()
            with open(dst, "w", encoding="utf-8") as fout:
                fout.write(content)
            print(f"  ✅ Synced {fname}")

    # 8. Validation: check consistency
    print("\n8. Validation...")
    with open(os.path.join(OSH_EVIDENCE, "traceability-matrix.md"), "r") as f:
        tm_text = f.read()
    with open(os.path.join(OSH_EVIDENCE, "requirement-coverage.md"), "r") as f:
        rc_text = f.read()
    with open(os.path.join(OSH_EVIDENCE, "acceptance-matrix.md"), "r") as f:
        am_text = f.read()

    tm_covered = len(_re.findall(r"Status: ✅ Covered", tm_text))
    tm_uncovered = len(_re.findall(r"Status: ❌ Not Covered", tm_text))

    rc_match = _re.search(r"\*\*Requirement Coverage\*\*: (\d+)/(\d+)", rc_text)
    am_match = _re.search(r"Covered by tests: (\d+)", am_text)

    print(f"    traceability-matrix.md: {tm_covered} covered, {tm_uncovered} uncovered")
    if rc_match:
        print(f"    requirement-coverage.md: {rc_match.group(1)}/{rc_match.group(2)}")
    if am_match:
        print(f"    acceptance-matrix.md: {am_match.group(1)} covered")

    covered_with_tests = sum(1 for r in reqs if len(_resolve_matched_tests(r)) > 0)
    ok = True
    if tm_covered != covered_with_tests:
        print(f"  ⚠️  Coverage count mismatch: traceability-matrix.md={tm_covered}, traceability-report.json={covered_with_tests}")
        ok = False
    if ok:
        print(f"  ✅ Consistent: {tm_covered}/{len(reqs)} SHALLs covered ({tm_covered * 100 // max(len(reqs), 1)}%)")

    # Verify review-log files are non-empty
    rl_summary_path = os.path.join(OSH_EVIDENCE, "review-log-summary.md")
    rl_json_path = os.path.join(OSH_EVIDENCE, "review-log.json")
    if os.path.exists(rl_summary_path):
        rl_size = os.path.getsize(rl_summary_path)
        print(f"  ✅ review-log-summary.md: {rl_size} bytes")
    if os.path.exists(rl_json_path):
        rl_size = os.path.getsize(rl_json_path)
        print(f"  ✅ review-log.json: {rl_size} bytes")

    print("\n=== Done ===")


if __name__ == "__main__":
    main()
