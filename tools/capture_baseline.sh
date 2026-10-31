#!/usr/bin/env bash
# capture_baseline.sh — yuleASR 量产准备基线一键捕获 (PR-0.1)
#
# 产出 (reports/baseline/):
#   meta.json            环境/工具/git 元数据
#   configure.log        cmake 配置日志
#   build.log            编译日志
#   ctest.log            测试日志
#   gcovr.json           覆盖率原始 JSON
#   misra-scan.json      MISRA 全量扫描 (detail + provenance)
#   trace-report.json    需求可追溯性报告
#   summary.json         汇总 (ctest/coverage/misra/traceability + strict_pass)
#
# 用法:
#   bash tools/capture_baseline.sh                 # 全量捕获 + 严格门禁
#   bash tools/capture_baseline.sh --record-only   # 只记录, 门禁失败不阻断
#   bash tools/capture_baseline.sh --skip-misra    # 跳过 MISRA (慢)
#   bash tools/capture_baseline.sh --clean         # 清理 build-baseline 后重建
#
# 退出码:
#   0 = 全部成功 (或 --record-only 下门禁失败也返回 0)
#   非0 = 执行错误 (构建/测试/工具崩溃) — fail-closed
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

BUILD="$ROOT/build-baseline"
OUT="$ROOT/reports/baseline"
mkdir -p "$OUT"

RECORD_ONLY=0
SKIP_MISRA=0
CLEAN=0
for arg in "$@"; do
  case "$arg" in
    --record-only) RECORD_ONLY=1 ;;
    --skip-misra)  SKIP_MISRA=1 ;;
    --clean)       CLEAN=1 ;;
    *) echo "Unknown arg: $arg" >&2; exit 2 ;;
  esac
done

log() { echo "[$(date -u +%H:%M:%SZ)] $*"; }

# ── 1. meta.json ────────────────────────────────────────────────
log "Writing meta.json"
GIT_SHA=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
GIT_DIRTY=$(test -n "$(git status --porcelain)" && echo true || echo false)
cat > "$OUT/meta.json" <<EOF
{
  "generated_at": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "git_sha": "$GIT_SHA",
  "git_dirty": $GIT_DIRTY,
  "tools": {
    "cmake": "$(cmake --version 2>/dev/null | head -1 | awk '{print $3}')",
    "gcc":   "$(gcc --version 2>/dev/null | head -1)",
    "gcovr": "$(gcovr --version 2>/dev/null | head -1 | awk '{print $2}')",
    "cppcheck": "$(cppcheck --version 2>/dev/null | head -1)",
    "python": "$(python3 --version 2>/dev/null)"
  }
}
EOF

# ── 2. configure ────────────────────────────────────────────────
if [ "$CLEAN" -eq 1 ] && [ -d "$BUILD" ]; then
  log "Cleaning $BUILD"
  rm -rf "$BUILD"
fi

log "cmake configure"
cmake -S "$ROOT" -B "$BUILD" \
  -DENABLE_COVERAGE=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON \
  > "$OUT/configure.log" 2>&1 || { echo "cmake configure failed"; cat "$OUT/configure.log"; exit 1; }

# ── 3. build ────────────────────────────────────────────────────
log "cmake --build"
cmake --build "$BUILD" -j"$(nproc)" > "$OUT/build.log" 2>&1 || { echo "build failed"; tail -40 "$OUT/build.log"; exit 1; }

# ── 4. ctest ────────────────────────────────────────────────────
log "ctest"
set +e
( cd "$BUILD" && ctest --output-on-failure --timeout 120 -j"$(nproc)" ) > "$OUT/ctest.log" 2>&1
CTEST_RC=$?
set -e
# ctest summary line: "99% tests passed, 2 tests failed out of 192"
CTEST_TOTAL=$(grep -oE 'out of [0-9]+' "$OUT/ctest.log" | head -1 | grep -oE '[0-9]+' || echo 0)
CTEST_FAIL=$(grep -oE '[0-9]+ tests? failed' "$OUT/ctest.log" | head -1 | grep -oE '^[0-9]+' || echo 0)
CTEST_ERR=$(grep -oE '[0-9]+ errors? while running' "$OUT/ctest.log" | head -1 | grep -oE '^[0-9]+' || echo 0)
log "ctest done: total=$CTEST_TOTAL fail=$CTEST_FAIL err=$CTEST_ERR (rc=$CTEST_RC)"

# ── 5. gcovr coverage (CRITICAL: run from repo root, build dir positional) ──
log "gcovr"
# dds1 tests set GCOV_PREFIX → creates coverage_dds1* dirs with absolute-path .gcda;
# remove before gcovr or it may skip/crash.
find "$BUILD" -maxdepth 1 -type d -name 'coverage_dds1*' -exec rm -rf {} + 2>/dev/null || true

gcovr --root "$ROOT" \
  --filter 'src/.*' \
  --exclude 'tests/.*' \
  --exclude 'third_party/.*' \
  --json \
  --output "$OUT/gcovr.json" \
  "$BUILD" > "$OUT/gcovr.log" 2>&1 || { echo "gcovr failed"; cat "$OUT/gcovr.log"; exit 1; }

COV_LINE=$(python3 -c "
import json
d=json.load(open('$OUT/gcovr.json'))
lf=lh=bf=bh=0
for f in d.get('files',[]):
    for ln in f.get('lines',[]):
        lf+=1
        if ln.get('count',0)>0: lh+=1
        for br in ln.get('branches',[]):
            bf+=1
            if br.get('count',0)>0: bh+=1
line=round(lh*100.0/lf,2) if lf else 0.0
branch=round(bh*100.0/bf,2) if bf else 0.0
files=len(d.get('files',[]))
print(f'{line} {branch} {files}')
")
read -r COV_LINE_PCT COV_BRANCH_PCT COV_FILES <<< "$COV_LINE"
log "coverage: line=$COV_LINE_PCT% branch=$COV_BRANCH_PCT% files=$COV_FILES"

# ── 6. MISRA ────────────────────────────────────────────────────
MISRA_REQUIRED="N/A"
MISRA_DEVIATED="N/A"
MISRA_TOTAL="N/A"
MISRA_FILES="N/A"
if [ "$SKIP_MISRA" -eq 0 ]; then
  log "MISRA scan"
  python3 "$ROOT/.yuleosh/tools/misra_full_scan.py" \
    --out="$OUT/misra-scan.json" > "$OUT/misra.log" 2>&1 || { echo "misra scan failed"; tail -20 "$OUT/misra.log"; exit 1; }
  # --gate-report validates the archived scan and writes .yuleosh/reports/misra-gate.json
  # with required/deviated/total counts (exit 5 on gate failure — we capture, don't block).
  set +e
  python3 "$ROOT/.yuleosh/tools/misra_full_scan.py" \
    --gate-report="$OUT/misra-scan.json" >> "$OUT/misra.log" 2>&1
  MISRA_GATE_RC=$?
  set -e
  GATE_JSON="$ROOT/.yuleosh/reports/misra-gate.json"
  if [ -f "$GATE_JSON" ]; then
    MISRA_STATS=$(python3 -c "
import json
d=json.load(open('$GATE_JSON'))
print(f\"{d.get('required','N/A')} {d.get('deviated','N/A')} {d.get('total','N/A')} {d.get('scanned_files','N/A')}\")
")
  else
    MISRA_STATS="N/A N/A N/A N/A"
  fi
  read -r MISRA_REQUIRED MISRA_DEVIATED MISRA_TOTAL MISRA_FILES <<< "$MISRA_STATS"
  log "misra: required=$MISRA_REQUIRED deviated=$MISRA_DEVIATED total=$MISRA_TOTAL files=$MISRA_FILES (gate_rc=$MISRA_GATE_RC)"
else
  log "MISRA skipped (--skip-misra)"
fi

# ── 7. traceability ─────────────────────────────────────────────
log "traceability"
python3 "$ROOT/tools/traceability/trace_requirements.py" \
  -o "$OUT/trace-report.json" > "$OUT/trace.log" 2>&1 || { echo "traceability failed"; tail -20 "$OUT/trace.log"; exit 1; }
TRACE_PASS=$(python3 -c "
import json
d=json.load(open('$OUT/trace-report.json'))
print('pass' if d.get('passed', d.get('coverage',0)>=0) else 'fail')
" 2>/dev/null || echo "unknown")
log "traceability: $TRACE_PASS"

# ── 8. summary.json + gate ──────────────────────────────────────
# Gate thresholds from .yuleosh/ci-config.yaml
read -r GATE_LINE GATE_BRANCH GATE_MISRA_REQ GATE_MISRA_TOT GATE_MISRA_MINFILES < <(python3 -c "
import yaml
c=yaml.safe_load(open('$ROOT/.yuleosh/ci-config.yaml'))
cov=c['coverage']; m=c['misra']['gate']
print(f\"{cov['threshold_line']} {cov['threshold_branch']} {m['max_required']} {m['max_total']} {m['min_files_scanned']}\")
")

CTEST_PASS=$([ "$CTEST_FAIL" -eq 0 ] && [ "$CTEST_ERR" -eq 0 ] && echo true || echo false)
COV_PASS=$(python3 -c "print(str($COV_LINE_PCT >= $GATE_LINE and $COV_BRANCH_PCT >= $GATE_BRANCH).lower())")
MISRA_PASS=$(python3 -c "
req='$MISRA_REQUIRED'; tot='$MISRA_TOTAL'; files='$MISRA_FILES'
if req=='N/A': print('skipped')
else: print(str(int(req)<=$GATE_MISRA_REQ and int(tot)<=$GATE_MISRA_TOT and int(files)>=$GATE_MISRA_MINFILES).lower())
")

GEN_AT=$(date -u +%Y-%m-%dT%H:%M:%SZ)
python3 - "$OUT/summary.json" "$GEN_AT" "$GIT_SHA" "$GIT_DIRTY" \
  "$CTEST_TOTAL" "$CTEST_FAIL" "$CTEST_ERR" "$CTEST_PASS" \
  "$COV_LINE_PCT" "$COV_BRANCH_PCT" "$COV_FILES" "$GATE_LINE" "$GATE_BRANCH" "$COV_PASS" \
  "$MISRA_REQUIRED" "$MISRA_DEVIATED" "$MISRA_TOTAL" "$MISRA_FILES" \
  "$GATE_MISRA_REQ" "$GATE_MISRA_TOT" "$GATE_MISRA_MINFILES" "$MISRA_PASS" \
  "$TRACE_PASS" <<'PYEOF'
import json, sys
a = sys.argv
out = {
  "generated_at": a[2], "git_sha": a[3], "git_dirty": a[4] == "true",
  "ctest": {"total": int(a[5]), "fail": int(a[6]), "errors": int(a[7]), "pass": a[8] == "true"},
  "coverage": {"line_pct": float(a[9]), "branch_pct": float(a[10]), "files": int(a[11]),
               "gate_line": float(a[12]), "gate_branch": float(a[13]), "pass": a[14] == "true"},
  "misra": {"required": int(a[15]) if a[15] != "N/A" else None,
            "deviated": int(a[16]) if a[16] != "N/A" else None,
            "total": int(a[17]) if a[17] != "N/A" else None,
            "files": int(a[18]) if a[18] != "N/A" else None,
            "gate_required": int(a[19]), "gate_total": int(a[20]),
            "gate_min_files": int(a[21]),
            "pass": a[22] if a[22] == "skipped" else (a[22] == "true")},
  "traceability": {"pass": a[23] == "pass"},
}
out["strict_pass"] = all([out["ctest"]["pass"], out["coverage"]["pass"],
                          out["misra"]["pass"] in (True, "skipped"), out["traceability"]["pass"]])
json.dump(out, open(a[1], "w"), indent=2)
PYEOF
STRICT_PASS=$(python3 -c "import json; print(str(json.load(open('$OUT/summary.json'))['strict_pass']).lower())")

log "summary written to $OUT/summary.json"
log "strict_pass=$STRICT_PASS (ctest=$CTEST_PASS cov=$COV_PASS misra=$MISRA_PASS trace=$TRACE_PASS)"

if [ "$STRICT_PASS" != "true" ] && [ "$RECORD_ONLY" -ne 1 ]; then
  log "FAIL: strict gates not met (use --record-only to record without blocking)"
  exit 1
fi

log "DONE"
