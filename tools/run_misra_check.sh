#!/bin/bash
#=============================================================================
# MISRA C:2012 Compliance Check Script - BSW Full Scan
#=============================================================================
# Project: YuleTech AutoSAR BSW Platform
# Standard: MISRA C:2012 Amendment 2
# Tool: cppcheck with MISRA addon
# Version: 2.0
# Date: 2026-10-13
#=============================================================================

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BSW_SRC_DIR="${PROJECT_ROOT}/src/bsw"
REPORTS_DIR="${PROJECT_ROOT}/reports"

# Allow overriding the target module/directory via argument
TARGET_DIR="${1:-${BSW_SRC_DIR}}"

# Output files
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
REPORT_XML="${REPORTS_DIR}/misra_bsw_check_${TIMESTAMP}.xml"
REPORT_TXT="${REPORTS_DIR}/misra_bsw_check_${TIMESTAMP}.txt"
SUMMARY_FILE="${REPORTS_DIR}/misra_bsw_summary_${TIMESTAMP}.md"

echo "============================================================================="
echo "  MISRA C:2012 Compliance Check - BSW"
echo "============================================================================="
echo ""
echo "Project Root: ${PROJECT_ROOT}"
echo "Target Dir:   ${TARGET_DIR}"
echo "Reports Dir:  ${REPORTS_DIR}"
echo "Timestamp:    ${TIMESTAMP}"
echo ""

# Create reports directory if not exists
mkdir -p "${REPORTS_DIR}"

# Check if cppcheck is available
if ! command -v cppcheck &> /dev/null; then
    echo -e "${RED}ERROR: cppcheck not found in PATH${NC}"
    echo "Please install cppcheck or use: brew install cppcheck"
    exit 1
fi

CPPCHECK_VERSION=$(cppcheck --version)
echo "Tool: ${CPPCHECK_VERSION}"
echo ""

# Verify target directory exists
if [ ! -d "${TARGET_DIR}" ]; then
    echo -e "${RED}ERROR: Target directory not found: ${TARGET_DIR}${NC}"
    exit 1
fi

# Build include paths from BSW source tree
BSW_INCLUDES=""
for inc_dir in $(find "${PROJECT_ROOT}/src/bsw" -type d -name "include" 2>/dev/null); do
    BSW_INCLUDES="${BSW_INCLUDES} -I${inc_dir}"
done
# Add common types include
if [ -d "${PROJECT_ROOT}/src/common" ]; then
    BSW_INCLUDES="${BSW_INCLUDES} -I${PROJECT_ROOT}/src/common"
fi

#=============================================================================
# MISRA Rule Categories
#=============================================================================
declare -a REQUIRED_RULES=(
    "misra-c2012-1.1" "misra-c2012-1.3"
    "misra-c2012-2.1" "misra-c2012-2.2"
    "misra-c2012-3.1"
    "misra-c2012-5.1"
    "misra-c2012-8.2" "misra-c2012-8.4" "misra-c2012-8.6" "misra-c2012-8.8"
    "misra-c2012-9.1"
    "misra-c2012-10.1" "misra-c2012-10.3" "misra-c2012-10.4"
    "misra-c2012-11.1" "misra-c2012-11.3" "misra-c2012-11.6" "misra-c2012-11.8"
    "misra-c2012-12.2"
    "misra-c2012-13.1" "misra-c2012-13.2" "misra-c2012-13.5"
    "misra-c2012-14.4"
    "misra-c2012-15.6" "misra-c2012-15.7"
    "misra-c2012-16.1" "misra-c2012-16.3" "misra-c2012-16.4"
    "misra-c2012-17.2" "misra-c2012-17.4" "misra-c2012-17.7"
    "misra-c2012-18.1" "misra-c2012-18.3" "misra-c2012-18.6"
    "misra-c2012-20.7"
    "misra-c2012-21.3" "misra-c2012-21.6"
)

declare -a ADVISORY_RULES=(
    "misra-c2012-5.9"
    "misra-c2012-8.13"
    "misra-c2012-12.1"
    "misra-c2012-15.5"
    "misra-c2012-17.8"
)

#=============================================================================
# Run cppcheck
#=============================================================================
run_cppcheck() {
    local output_file="$1"
    local extra_args="$2"

    echo -e "${BLUE}Running cppcheck MISRA analysis on ${TARGET_DIR}...${NC}"

    eval cppcheck \
        --enable=all \
        --std=c11 \
        --language=c \
        --platform=unix64 \
        ${BSW_INCLUDES} \
        --suppress=missingIncludeSystem \
        --suppress=missingInclude \
        --suppress=unusedFunction \
        --suppress=unmatchedSuppression \
        --inline-suppr \
        --force \
        --max-configs=10 \
        ${extra_args} \
        "${TARGET_DIR}" \
        2> "${output_file}"

    return $?
}

#=============================================================================
# Phase 1: XML report
#=============================================================================
echo -e "${YELLOW}Phase 1: Generating XML report...${NC}"
run_cppcheck "${REPORT_XML}" "--xml --xml-version=2" || true
if [ -f "${REPORT_XML}" ]; then
    echo -e "${GREEN}✓ XML report generated: ${REPORT_XML}${NC}"
else
    echo -e "${YELLOW}⚠ No XML report generated${NC}"
fi

#=============================================================================
# Phase 2: Text report
#=============================================================================
echo ""
echo -e "${YELLOW}Phase 2: Generating text report...${NC}"
run_cppcheck "${REPORT_TXT}" "" || true
if [ -f "${REPORT_TXT}" ]; then
    echo -e "${GREEN}✓ Text report generated: ${REPORT_TXT}${NC}"
else
    echo -e "${YELLOW}⚠ No text report generated${NC}"
fi

#=============================================================================
# Phase 3: Analyze results
#=============================================================================
echo ""
echo -e "${YELLOW}Phase 3: Analyzing results...${NC}"

if [ -f "${REPORT_XML}" ]; then
    ERROR_COUNT=$(grep -o 'severity="error"' "${REPORT_XML}" 2>/dev/null | wc -l | tr -d ' ')
    WARNING_COUNT=$(grep -o 'severity="warning"' "${REPORT_XML}" 2>/dev/null | wc -l | tr -d ' ')
    STYLE_COUNT=$(grep -o 'severity="style"' "${REPORT_XML}" 2>/dev/null | wc -l | tr -d ' ')
    PERFORMANCE_COUNT=$(grep -o 'severity="performance"' "${REPORT_XML}" 2>/dev/null | wc -l | tr -d ' ')
    PORTABILITY_COUNT=$(grep -o 'severity="portability"' "${REPORT_XML}" 2>/dev/null | wc -l | tr -d ' ')
else
    ERROR_COUNT=0; WARNING_COUNT=0; STYLE_COUNT=0
    PERFORMANCE_COUNT=0; PORTABILITY_COUNT=0
fi

echo ""
echo "============================================================================="
echo "  MISRA Compliance Summary"
echo "============================================================================="
echo ""
echo "Violations by Severity:"
echo "  Errors:       ${ERROR_COUNT}"
echo "  Warnings:     ${WARNING_COUNT}"
echo "  Style:        ${STYLE_COUNT}"
echo "  Performance:  ${PERFORMANCE_COUNT}"
echo "  Portability:  ${PORTABILITY_COUNT}"
echo ""

# Generate summary markdown
cat > "${SUMMARY_FILE}" << EOF
# MISRA C:2012 Compliance Check Summary

## Check Information
| Property | Value |
|:---------|:------|
| Date | $(date '+%Y-%m-%d %H:%M:%S') |
| Tool | ${CPPCHECK_VERSION} |
| Standard | MISRA C:2012 Amendment 2 |
| Target | ${TARGET_DIR} |

## Violations Summary
| Severity | Count | Status |
|:---------|:------|:-------|
| Error | ${ERROR_COUNT} | $([ ${ERROR_COUNT} -eq 0 ] && echo "✓ PASS" || echo "✗ FAIL") |
| Warning | ${WARNING_COUNT} | $([ ${WARNING_COUNT} -eq 0 ] && echo "✓ PASS" || echo "⚠ REVIEW") |
| Style | ${STYLE_COUNT} | $([ ${STYLE_COUNT} -eq 0 ] && echo "✓ PASS" || echo "⚠ REVIEW") |

## Reports
- XML: ${REPORT_XML}
- Text: ${REPORT_TXT}
EOF

echo -e "${GREEN}✓ Summary generated: ${SUMMARY_FILE}${NC}"

#=============================================================================
# Final status
#=============================================================================
echo ""
echo "============================================================================="
echo "  Final Status"
echo "============================================================================="
echo ""

if [ ${ERROR_COUNT} -eq 0 ]; then
    echo -e "${GREEN}✓ REQUIRED RULES: COMPLIANT${NC}"
else
    echo -e "${RED}✗ REQUIRED RULES: NON-COMPLIANT (${ERROR_COUNT} errors)${NC}"
fi

if [ ${WARNING_COUNT} -eq 0 ] && [ ${STYLE_COUNT} -eq 0 ]; then
    echo -e "${GREEN}✓ ADVISORY RULES: COMPLIANT${NC}"
else
    echo -e "${YELLOW}⚠ ADVISORY RULES: REVIEW REQUIRED${NC}"
fi

echo ""
echo "Reports: ${REPORT_XML}, ${REPORT_TXT}, ${SUMMARY_FILE}"
echo ""

[ ${ERROR_COUNT} -eq 0 ] && exit 0 || exit 1
