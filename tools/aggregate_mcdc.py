#!/usr/bin/env python3
"""Run instrumented tests and aggregate LLVM MC/DC records into one report.

Host-only MC/DC measurement for the ASIL modules.  Every CTest executable is
run with a **private** LLVM profile directory.  Exporting ``llvm-cov`` against
a single-test profile keeps the tool fast and, more importantly, correct: the
same source file is linked into many test binaries, so a globally merged
profile would make llvm-cov match every record against every binary.

Per-decision records are unioned by source location across all tests.  A
decision is counted once and is covered when any test exercises it.

Usage:
    python3 tools/aggregate_mcdc.py \
        --build-dir build-mcdc \
        --profile-dir build-mcdc/mcdc/profiles \
        --output .yuleosh/reports/mcdc-coverage.json
"""

import argparse
import datetime
import json
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def die(message):
    print(f"error: {message}", file=sys.stderr)
    sys.exit(1)


def run_command(cmd, cwd=None, env=None, timeout=None):
    return subprocess.run(
        cmd, cwd=cwd, env=env, timeout=timeout, capture_output=True, text=True
    )


def git_sha(root):
    result = run_command(["git", "rev-parse", "HEAD"], cwd=root)
    return result.stdout.strip() if result.returncode == 0 else "unknown"


def tool_version(binary):
    result = run_command([binary, "--version"])
    if result.returncode != 0:
        return "unknown"
    return (result.stdout or result.stderr).splitlines()[0].strip()


def properties_of(test):
    """CTest JSON reports properties as ``[{"name": ..., "value": ...}]``."""
    result = {}
    for item in test.get("properties") or []:
        if isinstance(item, dict) and "name" in item:
            result[item["name"]] = item.get("value")
    return result


def find_tests(build_dir):
    result = run_command(["ctest", "--test-dir", build_dir, "--show-only=json-v1"])
    if result.returncode != 0:
        die(f"ctest --show-only=json-v1 failed:\n{result.stderr.strip()}")
    try:
        tests = json.loads(result.stdout).get("tests", [])
    except json.JSONDecodeError as exc:
        die(f"cannot parse CTest JSON: {exc}")

    build_dir = os.path.realpath(build_dir)
    selected = []
    for test in tests:
        command = test.get("command") or []
        if not command:
            continue
        binary = command[0]
        if not os.path.isabs(binary):
            binary = os.path.join(build_dir, binary)
        binary = os.path.realpath(binary)
        # Python/monitor tests are not instrumented C executables.
        if not (binary.startswith(build_dir + os.sep) and os.path.isfile(binary)):
            continue
        props = properties_of(test)
        selected.append(
            {
                "name": test.get("name", os.path.basename(binary)),
                "command": list(command),
                "binary": binary,
                "working_dir": props.get("WORKING_DIRECTORY") or build_dir,
                "environment": props.get("ENVIRONMENT") or [],
                "timeout": props.get("TIMEOUT") or 300,
                "will_fail": bool(props.get("WILL_FAIL", False)),
                "skip_return_code": props.get("SKIP_RETURN_CODE"),
            }
        )
    return selected, len(tests), build_dir


def normalize_path(path, root):
    """Convert an LLVM coverage path to a repository-relative POSIX path."""
    if not os.path.isabs(path):
        path = os.path.join(root, path)
    real = os.path.realpath(path)
    try:
        rel = os.path.relpath(real, root)
    except ValueError:
        return None
    if rel == os.pardir or rel.startswith(os.pardir + os.sep):
        return None
    return rel.replace(os.sep, "/")


def decision_key(record):
    """Source range of an MC/DC decision.

    LLVM 18 exports ``mcdc_records`` as
    ``[line_start, column_start, line_end, column_end, expanded_file_id,
    kind, [condition_covered, ...]]``.
    """
    if isinstance(record, dict):  # future llvm-cov schema
        return (
            record.get("line_start", 0),
            record.get("column_start", 0),
            record.get("line_end", 0),
            record.get("column_end", 0),
        )
    return (record[0], record[1], record[2], record[3])


def conditions_of(record):
    if isinstance(record, dict):
        return record.get("conditions", [])
    return record[6] if len(record) > 6 else []


def safe_dir_name(name):
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", name)


def run_and_export(test, args, profile_root):
    """Run one test with its own profile, then export and collect coverage."""
    test_dir = os.path.join(profile_root, safe_dir_name(test["name"]))
    shutil.rmtree(test_dir, ignore_errors=True)
    os.makedirs(test_dir, exist_ok=True)

    env = os.environ.copy()
    for assignment in test["environment"]:
        if "=" in assignment:
            key, value = assignment.split("=", 1)
            env[key] = value
    env["LLVM_PROFILE_FILE"] = os.path.join(test_dir, "profile-%p.profraw")

    try:
        result = run_command(
            test["command"],
            cwd=test["working_dir"],
            env=env,
            timeout=float(test["timeout"]),
        )
    except subprocess.TimeoutExpired:
        return {"name": test["name"], "status": "timeout", "records": {}}

    skipped = (
        test["skip_return_code"] is not None
        and result.returncode == int(test["skip_return_code"])
    )
    passed = result.returncode == 0
    if test["will_fail"]:
        passed = not passed
    if skipped:
        return {"name": test["name"], "status": "skipped", "records": {}}
    if not passed:
        return {
            "name": test["name"],
            "status": f"failed(rc={result.returncode})",
            "records": {},
        }

    profraws = [
        os.path.join(test_dir, item)
        for item in os.listdir(test_dir)
        if item.endswith(".profraw")
    ]
    if not profraws:
        return {"name": test["name"], "status": "no-profile", "records": {}}

    profdata = os.path.join(test_dir, "merged.profdata")
    merge = run_command([args.llvm_profdata, "merge", "-sparse"] + profraws + ["-o", profdata])
    if merge.returncode != 0:
        return {"name": test["name"], "status": "merge-failed", "records": {}}

    export = run_command(
        [
            args.llvm_cov,
            "export",
            test["binary"],
            f"-instr-profile={profdata}",
            "--format=text",
        ]
    )
    if export.returncode != 0:
        return {"name": test["name"], "status": "export-failed", "records": {}}

    try:
        data = json.loads(export.stdout)
    except json.JSONDecodeError:
        return {"name": test["name"], "status": "bad-json", "records": {}}

    records = {}
    grouped = {}
    for entry in data.get("data", []):
        for coverage_file in entry.get("files", []):
            path = normalize_path(coverage_file.get("filename", ""), args.root)
            if path is None:
                continue
            for record in coverage_file.get("mcdc_records", []):
                key = (path,) + decision_key(record)
                grouped.setdefault(key, []).append(conditions_of(record))

    duplicates = 0
    for key, vectors in grouped.items():
        # Multiple records with the same source range come from macro
        # expansions.  llvm-cov's own summary excludes them because the
        # expansion site cannot be attributed uniquely; do the same.
        if len(vectors) > 1:
            duplicates += 1
            continue
        for index, covered in enumerate(vectors[0]):
            condition_key = key + (index,)
            records[condition_key] = records.get(condition_key, False) or bool(covered)

    return {
        "name": test["name"],
        "status": "passed",
        "records": records,
        "duplicate_decisions": duplicates,
    }


def summarize(records):
    per_file = {}
    for key, covered in records.items():
        entry = per_file.setdefault(key[0], {"count": 0, "covered": 0})
        entry["count"] += 1
        if covered:
            entry["covered"] += 1

    files = []
    for path in sorted(per_file):
        entry = per_file[path]
        count = entry["count"]
        covered = entry["covered"]
        files.append(
            {
                "file": path,
                "mcdc": {
                    "count": count,
                    "covered": covered,
                    "percent": (100.0 * covered / count) if count else 0.0,
                },
            }
        )

    total_count = sum(item["mcdc"]["count"] for item in files)
    total_covered = sum(item["mcdc"]["covered"] for item in files)
    totals = {
        "count": total_count,
        "covered": total_covered,
        "percent": (100.0 * total_covered / total_count) if total_count else 0.0,
    }
    return files, totals


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", default=os.path.join(BASE_DIR, "build-mcdc"))
    parser.add_argument(
        "--profile-dir",
        default=os.path.join(BASE_DIR, "build-mcdc", "mcdc", "profiles"),
    )
    parser.add_argument(
        "--output",
        default=os.path.join(BASE_DIR, ".yuleosh", "reports", "mcdc-coverage.json"),
    )
    parser.add_argument("--root", default=BASE_DIR)
    parser.add_argument("--llvm-cov", default="llvm-cov")
    parser.add_argument("--llvm-profdata", default="llvm-profdata")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    args = parser.parse_args()

    args.root = os.path.realpath(args.root)
    profile_root = os.path.realpath(args.profile_dir)
    os.makedirs(profile_root, exist_ok=True)

    tests, total_tests, build_dir = find_tests(args.build_dir)
    if not tests:
        die(f"no instrumented test executables found under {build_dir}")

    print(
        f"MC/DC: running {len(tests)} instrumented tests "
        f"({total_tests - len(tests)} non-C tests skipped)",
        file=sys.stderr,
    )

    with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        results = list(pool.map(lambda test: run_and_export(test, args, profile_root), tests))

    records = {}
    statuses = {}
    excluded_macro_decisions = 0
    for result in results:
        statuses[result["status"]] = statuses.get(result["status"], 0) + 1
        excluded_macro_decisions += result.get("duplicate_decisions", 0)
        for key, covered in result["records"].items():
            records[key] = records.get(key, False) or covered

    failed = [
        result["name"]
        for result in results
        if result["status"] not in ("passed", "skipped")
    ]
    for name in failed[:10]:
        print(f"  ❌ {name}", file=sys.stderr)
    if failed:
        die(f"{len(failed)} test(s) did not produce valid MC/DC data")

    files, totals = summarize(records)
    if totals["count"] == 0:
        die("no MC/DC decisions measured — instrumentation or profile export is broken")

    report = {
        "format": "mcdc-v1",
        "provenance": {
            "git_sha": git_sha(args.root),
            "generated_at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
            "measurement": "host",
            "tool": {
                "clang": tool_version(os.environ.get("MCDC_CC", "clang")),
                "llvm_cov": tool_version(args.llvm_cov),
            },
            "tests_run": len(tests),
            "test_status": statuses,
            "excluded_macro_decisions": excluded_macro_decisions,
        },
        "files": files,
        "totals": totals,
    }

    output = os.path.abspath(args.output)
    os.makedirs(os.path.dirname(output), exist_ok=True)
    with open(output, "w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2)
        handle.write("\n")

    print(
        f"MC/DC: {totals['covered']}/{totals['count']} decisions "
        f"({totals['percent']:.1f}%) across {len(files)} files -> {output}",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
