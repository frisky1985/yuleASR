#!/bin/bash
#=============================================================================
# MISRA C:2012 Compliance Check Script for COM Module
#=============================================================================
# Project: ETH-DDS Integration (AutoSAR Classic COM Module)
# Standard: MISRA C:2012 Amendment 2
# Tool: cppcheck with MISRA addon
# Version: 1.0
# Date: 2026-04-29
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
COM_SRC_DIR="${PROJECT_ROOT}/src/autosar/classic/com"
COM_INC_DIR="${PROJECT_ROOT}/include/autosar/classic/com"
REPORTS_DIR="${PROJECT_ROOT}/reports"
MISRA_CONFIG="${PROJECT_ROOT}/.misra_config"
SUPPRESSIONS_FILE="${PROJECT_ROOT}/tools/misra/cppcheck_suppressions.xml"

# Output files
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
REPORT_XML="${REPORTS_DIR}/misra_com_check_${TIMESTAMP}.xml"
REPORT_TXT="${REPORTS_DIR}/misra_com_check_${TIMESTAMP}.txt"
REPORT_HTML="${REPORTS_DIR}/misra_com_check_${TIMESTAMP}.html"
SUMMARY_FILE="${REPORTS_DIR}/misra_com_summary_${TIMESTAMP}.md"

echo "============================================================================="
echo "  MISRA C:2012 Compliance Check - COM Module"
echo "============================================================================="
echo ""
echo "Project Root: ${PROJECT_ROOT}"
echo "Source Dir:   ${COM_SRC_DIR}"
echo "Reports Dir:  ${REPORTS_DIR}"
echo "Timestamp:    ${TIMESTAMP}"
echo ""

# Create reports directory if not exists
mkdir -p "${REPORTS_DIR}"

# Check if cppcheck is available
if ! command -v cppcheck &> /dev/null; then
    echo -e "${RED}ERROR: cppcheck not found in PATH${NC}"
    echo "Please install cppcheck or use: sudo snap install cppcheck"
    exit 1
fi

CPPCHECK_VERSION=$(cppcheck --version)
echo "Tool: ${CPPCHECK_VERSION}"
echo ""

#=============================================================================
# MISRA Rule Categories
#=============================================================================
# Required rules - must be compliant (non-compliant requires deviation)
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

# Advisory rules - should be followed (deviation recommended but not required)
declare -a ADVISORY_RULES=(
    "misra-c2012-5.9"
    "misra-c2012-8.13"
    "misra-c2012-12.1"
    "misra-c2012-15.5"
    "misra-c2012-17.8"
)

#=============================================================================
# Function to run cppcheck
#=============================================================================
run_cppcheck() {
    local output_format="$1"
    local output_file="$2"
    local extra_args="$3"
    
    echo -e "${BLUE}Running cppcheck MISRA analysis...${NC}"
    
    cppcheck \
        --enable=all \
        --std=c11 \
        --language=c \
        --platform=unix64 \
        -I "${PROJECT_ROOT}/include" \
        -I "${PROJECT_ROOT}/include/autosar/classic/com" \
        --suppress=missingIncludeSystem \
        --suppress=unusedFunction \
        --suppress=unmatchedSuppression \
        --inline-suppr \
        --force \
        --max-configs=10 \
        ${extra_args} \
        "${COM_SRC_DIR}" \
        2> "${output_file}"
    
    return $?
}

#=============================================================================
# Run XML analysis for detailed report
#=============================================================================
echo -e "${YELLOW}Phase 1: Generating XML report...${NC}"
run_cppcheck "xml" "${REPORT_XML}" "--xml --xml-version=2"
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ XML report generated: ${REPORT_XML}${NC}"
else
    echo -e "${YELLOW}⚠ cppcheck completed with findings (see ${REPORT_XML})${NC}"
fi

#=============================================================================
# Run text analysis for console output
#=============================================================================
echo ""
echo -e "${YELLOW}Phase 2: Generating text report...${NC}"
run_cppcheck "text" "${REPORT_TXT}" ""
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Text report generated: ${REPORT_TXT}${NC}"
else
    echo -e "${YELLOW}⚠ cppcheck completed with findings (see ${REPORT_TXT})${NC}"
fi

#=============================================================================
# Parse results and generate summary
#=============================================================================
echo ""
echo -e "${YELLOW}Phase 3: Analyzing results...${NC}"

# Count violations by severity
if [ -f "${REPORT_XML}" ]; then
    ERROR_COUNT=$(grep -o 'severity="error"' "${REPORT_XML}" 2>/dev/null | wc -l)
    WARNING_COUNT=$(grep -o 'severity="warning"' "${REPORT_XML}" 2>/dev/null | wc -l)
    STYLE_COUNT=$(grep -o 'severity="style"' "${REPORT_XML}" 2>/dev/null | wc -l)
    PERFORMANCE_COUNT=$(grep -o 'severity="performance"' "${REPORT_XML}" 2>/dev/null | wc -l)
    PORTABILITY_COUNT=$(grep -o 'severity="portability"' "${REPORT_XML}" 2>/dev/null | wc -l)
    INFORMATION_COUNT=$(grep -o 'severity="information"' "${REPORT_XML}" 2>/dev/null | wc -l)
else
    ERROR_COUNT=0
    WARNING_COUNT=0
    STYLE_COUNT=0
    PERFORMANCE_COUNT=0
    PORTABILITY_COUNT=0
    INFORMATION_COUNT=0
fi

# Trim whitespace
ERROR_COUNT=$(echo "$ERROR_COUNT" | tr -d ' ')
WARNING_COUNT=$(echo "$WARNING_COUNT" | tr -d ' ')
STYLE_COUNT=$(echo "$STYLE_COUNT" | tr -d ' ')

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
echo "  Information:  ${INFORMATION_COUNT}"
echo ""

# Generate summary markdown file
cat > "${SUMMARY_FILE}" << EOF
# MISRA C:2012 Compliance Check Summary

## Check Information
| Property | Value |
|:---------|:------|
| Date | $(date '+%Y-%m-%d %H:%M:%S') |
| Tool | ${CPPCHECK_VERSION} |
| Standard | MISRA C:2012 Amendment 2 |
| Module | COM (Classic AutoSAR) |

## Violations Summary
| Severity | Count | Status |
|:---------|:------|:-------|
| Error | ${ERROR_COUNT} | $([ ${ERROR_COUNT} -eq 0 ] && echo "✓ PASS" || echo "✗ FAIL") |
| Warning | ${WARNING_COUNT} | $([ ${WARNING_COUNT} -eq 0 ] && echo "✓ PASS" || echo "⚠ REVIEW") |
| Style | ${STYLE_COUNT} | $([ ${STYLE_COUNT} -eq 0 ] && echo "✓ PASS" || echo "⚠ REVIEW") |
| Performance | ${PERFORMANCE_COUNT} | $([ ${PERFORMANCE_COUNT} -eq 0 ] && echo "✓ PASS" || echo "⚠ REVIEW") |
| Portability | ${PORTABILITY_COUNT} | $([ ${PORTABILITY_COUNT} -eq 0 ] && echo "✓ PASS" || echo "⚠ REVIEW") |
| Information | ${INFORMATION_COUNT} | - |

## Files Checked
EOF

# Add list of checked files
for file in "${COM_SRC_DIR}"/*.c; do
    echo "- $(basename "$file")" >> "${SUMMARY_FILE}"
done

cat >> "${SUMMARY_FILE}" << EOF

## Generated Reports
- XML Report: ${REPORT_XML}
- Text Report: ${REPORT_TXT}

## Compliance Status
$([ ${ERROR_COUNT} -eq 0 ] && echo "**✓ COMPLIANT** - No required rule violations detected." || echo "**✗ NON-COMPLIANT** - Required rule violations detected.")

$([ ${WARNING_COUNT} -eq 0 ] && echo "**✓ COMPLIANT** - No advisory rule violations detected." || echo "**⚠ ADVISORY** - Advisory rule violations require review.")

---
*Generated by ETH-DDS MISRA Compliance Checker*
EOF

echo -e "${GREEN}✓ Summary generated: ${SUMMARY_FILE}${NC}"

#=============================================================================
# Check specific MISRA rules
#=============================================================================
echo ""
echo -e "${YELLOW}Phase 4: Checking specific MISRA rules...${NC}"

# Check for specific MISRA violations in the report
if [ -f "${REPORT_XML}" ]; then
    echo ""
    echo "Required Rule Violations:"
    for rule in "${REQUIRED_RULES[@]}"; do
        count=$(grep -o "${rule}" "${REPORT_XML}" 2>/dev/null | wc -l | tr -d ' ')
        if [ "$count" -gt 0 ]; then
            echo -e "  ${RED}✗${NC} ${rule}: ${count} violation(s)"
        else
            echo -e "  ${GREEN}✓${NC} ${rule}: No violations"
        fi
    done
    
    echo ""
    echo "Advisory Rule Violations:"
    for rule in "${ADVISORY_RULES[@]}"; do
        count=$(grep -o "${rule}" "${REPORT_XML}" 2>/dev/null | wc -l | tr -d ' ')
        if [ "$count" -gt 0 ]; then
            echo -e "  ${YELLOW}⚠${NC} ${rule}: ${count} violation(s)"
        else
            echo -e "  ${GREEN}✓${NC} ${rule}: No violations"
        fi
    done
fi

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
    echo "  All required MISRA C:2012 rules are satisfied."
else
    echo -e "${RED}✗ REQUIRED RULES: NON-COMPLIANT${NC}"
    echo "  ${ERROR_COUNT} error-level violations detected."
    echo "  See ${REPORT_XML} for details."
fi

if [ ${WARNING_COUNT} -eq 0 ] && [ ${STYLE_COUNT} -eq 0 ]; then
    echo -e "${GREEN}✓ ADVISORY RULES: COMPLIANT${NC}"
else
    echo -e "${YELLOW}⚠ ADVISORY RULES: REVIEW REQUIRED${NC}"
    echo "  ${WARNING_COUNT} warnings, ${STYLE_COUNT} style issues detected."
fi

echo ""
echo "Reports generated:"
echo "  - ${REPORT_XML}"
echo "  - ${REPORT_TXT}"
echo "  - ${SUMMARY_FILE}"
echo ""
echo "============================================================================="

# Exit with appropriate code
if [ ${ERROR_COUNT} -gt 0 ]; then
    exit 1
else
    exit 0
fi
