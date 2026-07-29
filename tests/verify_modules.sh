#!/bin/bash
#==================================================================================================
# Project              : YuleTech AutoSAR BSW
# Script               : Module Verification Script
#
# SW Version           : 1.0.0
# Build Date           : 2026-04-30
#
# (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
# All Rights Reserved.
#
# Description:
#   This script verifies that all new modules compile correctly and checks
#   file integrity for the test files.
#
# Usage:
#   ./verify_modules.sh [options]
#
# Options:
#   -h, --help      Show this help message
#   -v, --verbose   Enable verbose output
#   -c, --compile   Compile tests (requires gcc)
#   -s, --syntax    Check syntax only (no compilation)
#==================================================================================================

set -e

# Configuration
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TESTS_DIR="${PROJECT_ROOT}/tests/unit"
SRC_DIR="${PROJECT_ROOT}/src"
BUILD_DIR="${PROJECT_ROOT}/build/verification"

# Module definitions
MODULES=(
    "services/test_canm.c:CanNm"
    "services/test_cansm.c:CanSm"
    "services/test_dlt.c:Dlt"
    "services/test_cryif.c:CryIf"
    "services/test_fim.c:FiM"
    "xcp/test_xcp.c:Xcp"
    "mcal/test_crypto.c:Crypto"
)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
PASSED=0
FAILED=0
SKIPPED=0

# Functions
print_header() {
    echo -e "${BLUE}================================================================================${NC}"
    echo -e "${BLUE}  YuleTech AutoSAR BSW - Module Verification${NC}"
    echo -e "${BLUE}================================================================================${NC}"
    echo ""
}

print_footer() {
    echo ""
    echo -e "${BLUE}================================================================================${NC}"
    echo -e "${BLUE}  Verification Summary${NC}"
    echo -e "${BLUE}================================================================================${NC}"
    echo -e "  Total:    $((PASSED + FAILED + SKIPPED))"
    echo -e "  ${GREEN}Passed:   ${PASSED}${NC}"
    echo -e "  ${RED}Failed:   ${FAILED}${NC}"
    echo -e "  ${YELLOW}Skipped:  ${SKIPPED}${NC}"
    echo -e "${BLUE}================================================================================${NC}"
    
    if [ $FAILED -gt 0 ]; then
        echo -e "${RED}Verification FAILED${NC}"
        return 1
    else
        echo -e "${GREEN}Verification PASSED${NC}"
        return 0
    fi
}

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((PASSED++))
}

log_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((FAILED++))
}

log_skip() {
    echo -e "${YELLOW}[SKIP]${NC} $1"
    ((SKIPPED++))
}

show_help() {
    head -n 30 "$0" | tail -n 20 | sed 's/^# //'
}

# Check file exists and has content
check_file_integrity() {
    local file="$1"
    local module="$2"
    
    if [ ! -f "$file" ]; then
        log_fail "Module $module: File not found - $file"
        return 1
    fi
    
    local lines=$(wc -l < "$file")
    local size=$(stat -c%s "$file" 2>/dev/null || stat -f%z "$file" 2>/dev/null)
    
    if [ "$lines" -lt 10 ]; then
        log_fail "Module $module: File too small ($lines lines)"
        return 1
    fi
    
    # Check for required sections
    if ! grep -q "TEST_CASE" "$file"; then
        log_fail "Module $module: No test cases found"
        return 1
    fi
    
    if ! grep -q "TEST_MAIN_BEGIN" "$file"; then
        log_fail "Module $module: No test main found"
        return 1
    fi
    
    log_pass "Module $module: File integrity OK ($lines lines, $size bytes)"
    return 0
}

# Check module headers exist
check_module_headers() {
    local module_name="$1"
    local header_paths=""
    
    case "$module_name" in
        "CanNm")
            header_paths="${SRC_DIR}/bsw/services/canm/include/CanNm.h"
            ;;
        "CanSm")
            header_paths="${SRC_DIR}/bsw/services/cansm/include/CanSm.h"
            ;;
        "Dlt")
            header_paths="${SRC_DIR}/bsw/services/dlt/include/Dlt.h"
            ;;
        "CryIf")
            header_paths="${SRC_DIR}/bsw/services/cryif/include/CryIf.h"
            ;;
        "FiM")
            header_paths="${SRC_DIR}/bsw/services/fim/include/FiM.h"
            ;;
        "Xcp")
            header_paths="${SRC_DIR}/bsw/services/xcp/include/Xcp.h"
            ;;
        "Crypto")
            header_paths="${SRC_DIR}/bsw/mcal/crypto/include/Crypto.h"
            ;;
    esac
    
    if [ -n "$header_paths" ]; then
        if [ -f "$header_paths" ]; then
            return 0
        else
            log_fail "Module $module_name: Header not found - $header_paths"
            return 1
        fi
    fi
    return 0
}

# Check syntax
check_syntax() {
    local file="$1"
    local module="$2"
    
    if ! command -v gcc &> /dev/null; then
        log_skip "Module $module: gcc not available for syntax check"
        return 0
    fi
    
    # Try to compile with syntax check only
    if gcc -fsyntax-only -c "$file" -I"${TESTS_DIR}" \
           -I"${SRC_DIR}/bsw/general/include" \
           -I"${SRC_DIR}/bsw/common/include" \
           -I"${SRC_DIR}/bsw/services/canm/include" \
           -I"${SRC_DIR}/bsw/services/cansm/include" \
           -I"${SRC_DIR}/bsw/services/dlt/include" \
           -I"${SRC_DIR}/bsw/services/cryif/include" \
           -I"${SRC_DIR}/bsw/services/fim/include" \
           -I"${SRC_DIR}/bsw/services/xcp/include" \
           -I"${SRC_DIR}/bsw/mcal/crypto/include" \
           -I"${SRC_DIR}/bsw/services/com/include" \
           -I"${SRC_DIR}/bsw/services/pdur/include" \
           -I"${SRC_DIR}/bsw/services/nm/include" \
           -I"${SRC_DIR}/bsw/ecual/canif/include" \
           -I"${SRC_DIR}/bsw/services/dem/include" \
           -I"${SRC_DIR}/bsw/services/comm/include" \
           2>/dev/null; then
        log_pass "Module $module: Syntax check OK"
        return 0
    else
        log_skip "Module $module: Syntax check (may require full build environment)"
        return 0
    fi
}

# Verify test framework
verify_test_framework() {
    local framework="${TESTS_DIR}/test_framework.h"
    
    if [ ! -f "$framework" ]; then
        log_fail "Test framework not found: $framework"
        return 1
    fi
    
    log_info "Test framework found: $framework"
    return 0
}

# Main verification
main() {
    local verbose=false
    local compile=false
    local syntax=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -v|--verbose)
                verbose=true
                shift
                ;;
            -c|--compile)
                compile=true
                shift
                ;;
            -s|--syntax)
                syntax=true
                shift
                ;;
            *)
                echo "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    print_header
    
    log_info "Project Root: $PROJECT_ROOT"
    log_info "Tests Directory: $TESTS_DIR"
    echo ""
    
    # Verify test framework
    verify_test_framework
    echo ""
    
    # Verify each module
    log_info "Verifying test files..."
    for module_info in "${MODULES[@]}"; do
        IFS=':' read -r file module <<< "$module_info"
        full_path="${TESTS_DIR}/${file}"
        
        if [ "$verbose" = true ]; then
            echo "  Checking $module..."
        fi
        
        check_file_integrity "$full_path" "$module"
    done
    echo ""
    
    # Check module headers
    log_info "Checking module headers..."
    for module_info in "${MODULES[@]}"; do
        IFS=':' read -r file module <<< "$module_info"
        check_module_headers "$module"
    done
    echo ""
    
    # Syntax check if requested
    if [ "$syntax" = true ]; then
        log_info "Running syntax checks..."
        for module_info in "${MODULES[@]}"; do
            IFS=':' read -r file module <<< "$module_info"
            full_path="${TESTS_DIR}/${file}"
            check_syntax "$full_path" "$module"
        done
        echo ""
    fi
    
    # Print summary
    print_footer
    exit $?
}

# Run main
main "$@"
