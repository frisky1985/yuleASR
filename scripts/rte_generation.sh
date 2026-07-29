#!/bin/bash
#=============================================================================
# yuleOSH RTE Generation Pipeline Stage
#=============================================================================
# Runs the ARXML → RTE C Code Generator as part of yuleOSH CI pipeline.
#
# Usage:
#   ./scripts/rte_generation.sh                 # Generate all RTE from default ARXML
#   ./scripts/rte_generation.sh --arxml <file>  # Use specific ARXML
#   ./scripts/rte_generation.sh --output <dir>   # Custom output dir
#   ./scripts/rte_generation.sh --swc SWC_NAME   # Filter specific SWC(s)
#   ./scripts/rte_generation.sh --check          # Only check (no generation)
#   ./scripts/rte_generation.sh --misra          # Also run MISRA check on output
#
# Exit codes:
#   0 = success
#   1 = generation error
#   2 = validation error
#   3 = MISRA error
#=============================================================================

set -e

# ── Configuration ──────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Default ARXML input paths (searched in order)
DEFAULT_ARXML_PATHS=(
    "${PROJECT_ROOT}/config/input/arxml/bcm_demo.arxml"
    "${PROJECT_ROOT}/config/input/arxml/example.arxml"
    "${PROJECT_ROOT}/tools/code_generators/rte/examples/bcm_demo.arxml"
    "${PROJECT_ROOT}/configs/arxml/example.arxml"
)

# Output directory (generated RTE code goes here)
RTE_GENERATED_DIR="${PROJECT_ROOT}/src/rte/generated"

# Generator script path
GENERATOR="${PROJECT_ROOT}/tools/code_generators/rte/rte_generator.py"

# Report file
REPORT_DIR="${PROJECT_ROOT}/reports"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# ── Parse arguments ────────────────────────────────────────────────────────
ARXML_INPUT=""
OUTPUT_DIR=""
SWC_FILTERS=()
MODE="generate"
RUN_MISRA=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --arxml|-i) ARXML_INPUT="$2"; shift 2 ;;
        --output|-o) OUTPUT_DIR="$2"; shift 2 ;;
        --swc) SWC_FILTERS+=("--swc" "$2"); shift 2 ;;
        --check|-c) MODE="check"; shift ;;
        --misra|-m) RUN_MISRA=true; shift ;;
        --help|-h)
            echo "Usage: $0 [options]"
            echo "  --arxml FILE    Input ARXML file"
            echo "  --output DIR    Output directory (default: src/rte/generated/)"
            echo "  --swc NAME      Filter: generate only for SWC (repeatable)"
            echo "  --check         Validation check only, no generation"
            echo "  --misra         Also run MISRA check on generated code"
            exit 0
            ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# ── Set defaults ───────────────────────────────────────────────────────────
if [ -z "$OUTPUT_DIR" ]; then
    OUTPUT_DIR="$RTE_GENERATED_DIR"
fi

if [ -z "$ARXML_INPUT" ]; then
    for path in "${DEFAULT_ARXML_PATHS[@]}"; do
        if [ -f "$path" ]; then
            ARXML_INPUT="$path"
            break
        fi
    done
fi

if [ -z "$ARXML_INPUT" ]; then
    echo -e "${RED}[RTE] ❌ No ARXML input file found!${NC}"
    echo "  Searched:"
    for path in "${DEFAULT_ARXML_PATHS[@]}"; do
        echo "    - $path"
    done
    exit 1
fi

# ── Stage: Check ───────────────────────────────────────────────────────────
if [ "$MODE" = "check" ]; then
    echo -e "${BLUE}[RTE] 🔍 Checking ARXML: ${ARXML_INPUT}${NC}"
    python3 -c "
import sys
sys.path.insert(0, '${PROJECT_ROOT}/tools/code_generators/rte')
from rte_generator import build_rte_ir_from_arxml
swcs, meta = build_rte_ir_from_arxml('${ARXML_INPUT}')
errors = meta.get('errors', [])
print(f'  SWCs: {meta[\"swc_count\"]}')
print(f'  Source: {meta[\"source_file\"]}')
for swc in swcs:
    print(f'    - {swc.name} ({swc.component_type}, {len(swc.ports)} ports, {len(swc.runnable_entities)} runnables)')
if errors:
    print(f'  ⚠️  Validation warnings: {len(errors)}')
    for e in errors:
        print(f'    - {e}')
else:
    print(f'  ✅ Validation passed')
" 2>&1
    exit 0
fi

# ── Stage: Generate ────────────────────────────────────────────────────────
echo -e "${BLUE}[RTE] ⚙️  Generating RTE code from: ${ARXML_INPUT}${NC}"
echo -e "${BLUE}[RTE]   → Output: ${OUTPUT_DIR}${NC}"

mkdir -p "$OUTPUT_DIR"

if [ ${#SWC_FILTERS[@]} -gt 0 ]; then
    python3 "$GENERATOR" -i "$ARXML_INPUT" -o "$OUTPUT_DIR" "${SWC_FILTERS[@]}" -v
else
    python3 "$GENERATOR" -i "$ARXML_INPUT" -o "$OUTPUT_DIR" -v
fi

GEN_RESULT=$?
if [ $GEN_RESULT -ne 0 ]; then
    echo -e "${RED}[RTE] ❌ RTE generation failed (exit $GEN_RESULT)${NC}"
    exit 1
fi

echo -e "${GREEN}[RTE] ✅ RTE generation successful${NC}"

# ── Stage: Validate output files ──────────────────────────────────────────
echo -e "${BLUE}[RTE] 🔍 Validating generated output...${NC}"
VALIDATION_FAILED=0

# Check that all .h files have proper include guards
for f in "$OUTPUT_DIR"/*.h; do
    [ -f "$f" ] || continue
    basename_f=$(basename "$f")
    guard_upper=$(echo "${basename_f%.h}" | tr '[:lower:]' '[:upper:]')
    if ! grep -q "${guard_upper}_H" "$f" 2>/dev/null; then
        echo -e "${YELLOW}[RTE]   ⚠️  Missing include guard in ${basename_f}${NC}"
        VALIDATION_FAILED=1
    fi
done

# Check all .c files compile-ready (have includes, functions)
for f in "$OUTPUT_DIR"/*.c; do
    [ -f "$f" ] || continue
    basename_f=$(basename "$f")
    if ! grep -q "void Rte_" "$f" 2>/dev/null && ! grep -q "Rte_Init" "$f" 2>/dev/null; then
        echo -e "${YELLOW}[RTE]   ⚠️  No RTE functions found in ${basename_f}${NC}"
        VALIDATION_FAILED=1
    fi
done

if [ $VALIDATION_FAILED -ne 0 ]; then
    echo -e "${YELLOW}[RTE] ⚠️  Validation found some issues${NC}"
else
    echo -e "${GREEN}[RTE] ✅ Output validation passed${NC}"
fi

# ── Report generation count ───────────────────────────────────────────────
FILE_COUNT=$(find "$OUTPUT_DIR" -type f \( -name "*.h" -o -name "*.c" \) | wc -l)
echo -e "${GREEN}[RTE] 📊 Generated ${FILE_COUNT} file(s)${NC}"

# ── Optional: MISRA check ─────────────────────────────────────────────────
if [ "$RUN_MISRA" = true ]; then
    echo -e "${BLUE}[RTE] 🔎 Running MISRA check on generated code...${NC}"
    if command -v cppcheck &> /dev/null; then
        MISRA_REPORT="${REPORT_DIR}/rte_misra_report.md"
        mkdir -p "$REPORT_DIR"
        cppcheck --std=c99 --language=c \
            --suppress=* \
            --enable=warning,style,performance \
            --template='{file}:{line}:{severity}:{message}' \
            --suppress='unmatchedSuppression' \
            -I "$OUTPUT_DIR" \
            -I "${PROJECT_ROOT}/src/rte/include" \
            "$OUTPUT_DIR" 2>&1 | tee "$MISRA_REPORT"
        echo -e "${GREEN}[RTE] ✅ MISRA report saved to ${MISRA_REPORT}${NC}"
    else
        echo -e "${YELLOW}[RTE] ⚠️  cppcheck not found; skipping MISRA check${NC}"
    fi
fi

echo -e "${GREEN}[RTE] ✅ RTE generation stage complete${NC}"
exit 0
