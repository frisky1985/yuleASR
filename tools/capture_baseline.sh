#!/usr/bin/env bash
#
# PR-0.1 Baseline capture — all production metrics from one reproducible command.
#
# Outputs (all under reports/baseline/, gitignored):
#   meta.json        git sha/branch/dirty + tool versions
#   configure.log    cmake configure
#   build.log        compile
#   ctest.log        unit/integration tests
#   ctest.xml        JUnit results
#   gcovr.json       raw gcovr data
#   c-coverage.json  canonical coverage evidence
#   coverage-gate.log / misra-scan.json / misra-gate.json
#   trace.log / trace-review.log
#   summary.json     machine-readable roll-up
#
# Usage:
#   bash tools/capture_baseline.sh [--record-only] [--skip-misra] [--clean]
#
#   --record-only  capture even when gates fail (baseline establishment)
#   --skip-misra   skip cppcheck (fast iteration; never valid for a delivery gate)
#   --clean        wipe the build directory first (from-scratch reproducibility)
#
# Exit codes: 0 = all steps captured and gates passed; 1 = gate/step failure
# (unless --record-only, which still fails on execution errors such as configure/build).

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/reports/baseline"
BUILD="${BASELINE_BUILD_DIR:-$ROOT/build-baseline}"
JOBS="${BASELINE_JOBS:-4}"

STRICT=1
SKIP_MISRA=0
CLEAN=0

for arg in "$@"; do
    case "$arg" in
        --record-only) STRICT=0 ;;
        --skip-misra)  SKIP_MISRA=1 ;;
        --clean)       CLEAN=1 ;;
        -h|--help)     sed -n '3,20p' "$0"; exit 0 ;;
        *) echo "unknown argument: $arg" >&2; exit 2 ;;
    esac
done

mkdir -p "$OUT"

run() {
    local name="$1"; shift
    {
        echo "## $(date -u +%Y-%m-%dT%H:%M:%SZ) $name"
        echo "\$ $*"
        "$@"
    } >>"$OUT/$name.log" 2>&1
    return $?
}

python3 - "$ROOT" "$OUT/meta.json" <<'PY'
import datetime, json, subprocess, sys, platform

root, out = sys.argv[1], sys.argv[2]

def sh(*cmd):
    try:
        return subprocess.check_output(cmd, cwd=root, text=True, stderr=subprocess.DEVNULL).strip()
    except Exception:
        return "unknown"

def ver(*cmd):
    try:
        lines = subprocess.check_output(cmd, cwd=root, text=True, stderr=subprocess.STDOUT).splitlines()
        return lines[0].strip() if lines else "unknown"
    except Exception:
        return "unknown"

dirty = sh("git", "status", "--porcelain")
meta = {
    "schema": 1,
    "generated_at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
    "git_sha": sh("git", "rev-parse", "HEAD"),
    "git_branch": sh("git", "rev-parse", "--abbrev-ref", "HEAD"),
    "git_dirty": dirty not in ("", "unknown"),
    "git_dirty_files": len([l for l in dirty.splitlines() if l.strip()]),
    "host": platform.node(),
    "platform": platform.platform(),
    "tools": {
        "cmake": ver("cmake", "--version"),
        "gcc": ver("gcc", "--version"),
        "gcovr": ver("gcovr", "--version"),
        "cppcheck": ver("cppcheck", "--version"),
        "python": ver("python3", "--version"),
    },
}
with open(out, "w") as f:
    json.dump(meta, f, indent=1)
PY
meta_rc=$?

if [[ $CLEAN -eq 1 ]]; then
    rm -rf "$BUILD"
fi

run configure cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DENABLE_COVERAGE=ON
configure_rc=$?
if [[ $configure_rc -ne 0 ]]; then
    echo "FATAL: configure failed, see $OUT/configure.log" >&2
    exit 1
fi

run build cmake --build "$BUILD" --parallel "$JOBS"
build_rc=$?
if [[ $build_rc -ne 0 ]]; then
    echo "FATAL: build failed, see $OUT/build.log" >&2
    exit 1
fi

run ctest ctest --test-dir "$BUILD" --output-on-failure --output-junit "$OUT/ctest.xml"
ctest_rc=$?

# tests/unit/dds1 sets GCOV_PREFIX, leaving absolute-path-prefixed .gcda trees
# that make gcov bail out (rc=5). Remove them before aggregating.
find "$BUILD" -maxdepth 1 -type d -name 'coverage_dds1*' -exec rm -rf {} + 2>/dev/null || true

# gcovr must run from the repo root and take the build dir as a search path:
# running from inside the build dir with --root + --filter yields 0 files.
run gcovr bash -c "cd '$ROOT' && gcovr --root '$ROOT' --filter 'src/.*' --exclude 'tests/.*' --exclude 'third_party/.*' --json --output '$OUT/gcovr.json' '$BUILD'"
gcovr_rc=$?

run coverage-json python3 "$ROOT/tools/generate_c_coverage_json.py" "$OUT/gcovr.json" "$OUT/c-coverage.json" --root "$ROOT"
coverage_json_rc=$?

run coverage-gate python3 "$ROOT/tools/check_coverage_gate.py" "$OUT/c-coverage.json"
coverage_rc=$?

misra_rc=0
if [[ $SKIP_MISRA -eq 1 ]]; then
    echo "misra: skipped (--skip-misra)" | tee -a "$OUT/misra-scan.log" >/dev/null
else
    run misra-scan python3 "$ROOT/.yuleosh/tools/misra_full_scan.py" --out="$OUT/misra-scan.json"
    scan_rc=$?
    if [[ $scan_rc -ne 0 ]]; then
        misra_rc=$scan_rc
    else
        run misra-gate python3 "$ROOT/.yuleosh/tools/misra_full_scan.py" --gate-report="$OUT/misra-scan.json"
        misra_rc=$?
        [[ -f "$ROOT/.yuleosh/reports/misra-gate.json" ]] && cp "$ROOT/.yuleosh/reports/misra-gate.json" "$OUT/misra-gate.json"
    fi
fi

run trace python3 "$ROOT/tools/check_traceability_gate.py"
trace_rc=$?
run trace-review python3 "$ROOT/tools/check_traceability_gate.py" --require-review
trace_review_rc=$?

CTEST_RC=$ctest_rc COVERAGE_RC=$coverage_rc MISRA_RC=$misra_rc \
TRACE_RC=$trace_rc TRACE_REVIEW_RC=$trace_review_rc SKIP_MISRA=$SKIP_MISRA \
python3 - "$OUT" <<'PY'
import json, os, sys, xml.etree.ElementTree as ET

out = sys.argv[1]

def load(name):
    path = os.path.join(out, name)
    if not os.path.exists(path):
        return None
    try:
        with open(path) as f:
            return json.load(f)
    except Exception:
        return None

def ctest_counts():
    path = os.path.join(out, "ctest.xml")
    if not os.path.exists(path):
        return {"total": None, "failures": None, "errors": None}
    try:
        root = ET.parse(path).getroot()
    except Exception:
        return {"total": None, "failures": None, "errors": None}
    cases = root.iter("testcase")
    total = failures = errors = 0
    for case in cases:
        total += 1
        if case.find("failure") is not None:
            failures += 1
        if case.find("error") is not None:
            errors += 1
    return {"total": total, "failures": failures, "errors": errors}

meta = load("meta.json") or {}
cov = load("c-coverage.json")
misra = load("misra-gate.json")
tests = ctest_counts()

rc = {
    "ctest": int(os.environ.get("CTEST_RC", "1")),
    "coverage": int(os.environ.get("COVERAGE_RC", "1")),
    "misra": int(os.environ.get("MISRA_RC", "1")),
    "trace": int(os.environ.get("TRACE_RC", "1")),
    "trace_review": int(os.environ.get("TRACE_REVIEW_RC", "1")),
    "misra_skipped": os.environ.get("SKIP_MISRA") == "1",
}

summary = {
    "schema": 1,
    "generated_at": meta.get("generated_at"),
    "git_sha": meta.get("git_sha"),
    "git_branch": meta.get("git_branch"),
    "git_dirty": meta.get("git_dirty"),
    "tools": meta.get("tools"),
    "ctest": {
        "status": "pass" if rc["ctest"] == 0 else "fail",
        **tests,
    },
    "coverage": {
        "status": "pass" if rc["coverage"] == 0 else "fail",
        "line_rate": (cov or {}).get("line_rate"),
        "branch_rate": (cov or {}).get("branch_rate"),
        "files": (cov or {}).get("total_files"),
    },
    "misra": {
        "status": "skipped" if rc["misra_skipped"] else ("pass" if rc["misra"] == 0 else "fail"),
        "required": (misra or {}).get("required"),
        "deviated": (misra or {}).get("deviated"),
        "total": (misra or {}).get("total"),
        "scanned_files": (misra or {}).get("scanned_files"),
    },
    "traceability": {
        "status": "pass" if rc["trace"] == 0 else "fail",
        "require_review_status": "pass" if rc["trace_review"] == 0 else "fail",
    },
    "exit_codes": rc,
}
summary["overall"] = {
    "strict_pass": (
        rc["ctest"] == 0 and rc["coverage"] == 0 and not rc["misra_skipped"]
        and rc["misra"] == 0 and rc["trace"] == 0 and rc["trace_review"] == 0
    )
}

with open(os.path.join(out, "summary.json"), "w") as f:
    json.dump(summary, f, indent=1)

print("== baseline summary ==")
print(f"  git              {summary['git_sha'][:8]} (dirty={summary['git_dirty']})")
print(f"  ctest            {summary['ctest']['status']} total={tests['total']} fail={tests['failures']} err={tests['errors']}")
print(f"  coverage         {summary['coverage']['status']} line={summary['coverage']['line_rate']} branch={summary['coverage']['branch_rate']} files={summary['coverage']['files']}")
print(f"  misra            {summary['misra']['status']} required={summary['misra']['required']} deviated={summary['misra']['deviated']} total={summary['misra']['total']} files={summary['misra']['scanned_files']}")
print(f"  traceability     {summary['traceability']['status']} (require-review: {summary['traceability']['require_review_status']})")
print(f"  strict_pass      {summary['overall']['strict_pass']}")
PY
summary_rc=$?

final_rc=0
[[ $meta_rc -ne 0 ]] && final_rc=1
[[ $gcovr_rc -ne 0 ]] && final_rc=1
[[ $coverage_json_rc -ne 0 ]] && final_rc=1
[[ $coverage_rc -ne 0 ]] && final_rc=1
[[ $ctest_rc -ne 0 ]] && final_rc=1
[[ $summary_rc -ne 0 ]] && final_rc=1
if [[ $SKIP_MISRA -eq 0 && $misra_rc -ne 0 ]]; then final_rc=1; fi
[[ $trace_rc -ne 0 ]] && final_rc=1
[[ $trace_review_rc -ne 0 ]] && final_rc=1

if [[ $STRICT -eq 0 ]]; then
    echo "--record-only: gates recorded, not enforced (execution errors still fail)"
    exit 0
fi
exit $final_rc
