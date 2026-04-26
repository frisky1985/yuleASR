#!/bin/bash
# ============================================================================
# MISRA C:2012 Compliance Check Script
# Project: ETH-DDS Integration (Automotive Ethernet Stack)
# ============================================================================
# Usage: ./scripts/run_misra_check.sh [options] [file_or_directory...]
# Options:
#   --tool=pclint|cppcheck|both    Select analysis tool (default: both)
#   --target=new|legacy|all        Code target scope (default: new)
#   --format=console|html|xml|json Output format (default: console)
#   --strict                       Treat advisory as errors
#   --fix                          Run auto-fix script first
#   --ci                           CI mode (fail on any required rule violation)
#   --help                         Show this help message
# ============================================================================

set -e

# ----------------------------------------------------------------------------
# Configuration
# ----------------------------------------------------------------------------
PROJECT_ROOT="/home/admin/eth-dds-integration"
TOOLS_DIR="${PROJECT_ROOT}/tools/misra"
REPORTS_DIR="${PROJECT_ROOT}/reports/misra"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Default values
TOOL="both"
TARGET="new"
FORMAT="console"
STRICT=0
FIX=0
CI_MODE=0

# Target directories
NEW_CODE_DIRS=(
    "src/diagnostics"
    "src/crypto_stack"
    "src/ota"
    "src/bootloader"
    "src/autosar/e2e"
)

LEGACY_CODE_DIRS=(
    "src/dds"
    "src/tsn"
    "src/ethernet"
    "src/bsw"
    "src/nvm"
    "src/transport"
    "src/safety"
    "src/application"
    "src/platform"
    "src/common"
)

# ----------------------------------------------------------------------------
# Functions
# ----------------------------------------------------------------------------

print_help() {
    head -30 "$0" | tail -27
    exit 0
}

log_info() {
    echo "[INFO] $1"
}

log_warn() {
    echo "[WARN] $1"
}

log_error() {
    echo "[ERROR] $1" >&2
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --tool=*)
                TOOL="${1#*=}"
                shift
                ;;
            --target=*)
                TARGET="${1#*=}"
                shift
                ;;
            --format=*)
                FORMAT="${1#*=}"
                shift
                ;;
            --strict)
                STRICT=1
                shift
                ;;
            --fix)
                FIX=1
                shift
                ;;
            --ci)
                CI_MODE=1
                shift
                ;;
            --help)
                print_help
                ;;
            -*)
                log_error "Unknown option: $1"
                exit 1
                ;;
            *)
                break
                ;;
        esac
    done
    
    # Remaining arguments are files/directories
    CUSTOM_PATHS=("$@")
}

get_source_files() {
    local dirs=()
    local files=()
    
    if [[ ${#CUSTOM_PATHS[@]} -gt 0 ]]; then
        for path in "${CUSTOM_PATHS[@]}"; do
            if [[ -d "$path" ]]; then
                dirs+=("$path")
            elif [[ -f "$path" ]]; then
                files+=("$path")
            fi
        done
    else
        case $TARGET in
            new)
                dirs+=("${NEW_CODE_DIRS[@]}")
                ;;
            legacy)
                dirs+=("${LEGACY_CODE_DIRS[@]}")
                ;;
            all)
                dirs+=("${NEW_CODE_DIRS[@]}")
                dirs+=("${LEGACY_CODE_DIRS[@]}")
                ;;
        esac
    fi
    
    # Find all C files
    local all_files="${files[*]}"
    for dir in "${dirs[@]}"; do
        if [[ -d "${PROJECT_ROOT}/${dir}" ]]; then
            all_files+=" $(find "${PROJECT_ROOT}/${dir}" -name '*.c' -type f 2>/dev/null)"
        fi
    done
    
    echo "$all_files" | tr ' ' '\n' | sort -u | grep -v '^$'
}

run_pclint() {
    log_info "Running PC-lint Plus analysis..."
    
    local sources=$1
    local report_file="${REPORTS_DIR}/pclint_report_${TIMESTAMP}"
    
    if ! command -v pclp64 &> /dev/null; then
        log_warn "PC-lint Plus not found in PATH. Skipping."
        return 1
    fi
    
    mkdir -p "$REPORTS_DIR"
    
    local format_option=""
    case $FORMAT in
        html)
            format_option="-html"
            report_file="${report_file}.html"
            ;;
        xml)
            format_option="-xml"
            report_file="${report_file}.xml"
            ;;
        *)
            report_file="${report_file}.txt"
            ;;
    esac
    
    log_info "Report: $report_file"
    
    # Run PC-lint
    local exit_code=0
    if [[ $FORMAT == "console" ]]; then
        pclp64 "${TOOLS_DIR}/pclint_config.lnt" $format_option $sources 2>&1 | tee "$report_file" || exit_code=$?
    else
        pclp64 "${TOOLS_DIR}/pclint_config.lnt" $format_option $sources > "$report_file" 2>&1 || exit_code=$?
    fi
    
    # Parse results
    local error_count=$(grep -c "error" "$report_file" 2>/dev/null || echo 0)
    local warning_count=$(grep -c "warning" "$report_file" 2>/dev/null || echo 0)
    
    log_info "PC-lint results: $error_count errors, $warning_count warnings"
    
    if [[ $CI_MODE -eq 1 && $error_count -gt 0 ]]; then
        return 1
    fi
    
    return 0
}

run_cppcheck() {
    log_info "Running Cppcheck analysis..."
    
    local sources=$1
    local report_file="${REPORTS_DIR}/cppcheck_report_${TIMESTAMP}"
    
    if ! command -v cppcheck &> /dev/null; then
        log_warn "Cppcheck not found in PATH. Skipping."
        return 1
    fi
    
    mkdir -p "$REPORTS_DIR"
    
    # Build cppcheck options
    local cppcheck_opts="--enable=all --force --inline-suppr"
    cppcheck_opts="$cppcheck_opts --suppressions-list=${TOOLS_DIR}/cppcheck_suppressions.xml"
    
    if [[ -f "${TOOLS_DIR}/misra.json" ]]; then
        cppcheck_opts="$cppcheck_opts --addon=${TOOLS_DIR}/misra.json"
    fi
    
    case $FORMAT in
        html)
            cppcheck_opts="$cppcheck_opts --template=html"
            report_file="${report_file}.html"
            ;;
        xml)
            cppcheck_opts="$cppcheck_opts --xml"
            report_file="${report_file}.xml"
            ;;
        json)
            cppcheck_opts="$cppcheck_opts --template='{\"file\":\"{file}\",\"line\":{line},\"column\":{column},\"severity\":\"{severity}\",\"id\":\"{id}\",\"message\":\"{message}\"}'"
            report_file="${report_file}.json"
            ;;
        *)
            report_file="${report_file}.txt"
            ;;
    esac
    
    log_info "Report: $report_file"
    
    # Create file list
    local file_list="${REPORTS_DIR}/source_files_${TIMESTAMP}.txt"
    echo "$sources" | tr ' ' '\n' > "$file_list"
    
    # Run cppcheck
    local exit_code=0
    if [[ $FORMAT == "console" ]]; then
        cppcheck $cppcheck_opts --file-list="$file_list" 2>&1 | tee "$report_file" || exit_code=$?
    else
        cppcheck $cppcheck_opts --file-list="$file_list" > "$report_file" 2>&1 || exit_code=$?
    fi
    
    # Parse results
    local error_count=0
    if [[ $FORMAT == "xml" ]]; then
        error_count=$(grep -oP '(?<=<error )' "$report_file" | wc -l)
    else
        error_count=$(grep -c "\[error\]" "$report_file" 2>/dev/null || echo 0)
    fi
    
    log_info "Cppcheck results: $error_count issues found"
    
    if [[ $CI_MODE -eq 1 && $error_count -gt 0 ]]; then
        return 1
    fi
    
    return 0
}

run_autofix() {
    log_info "Running automatic fixes..."
    
    local sources=$1
    local fix_script="${PROJECT_ROOT}/scripts/fix_misra_issues.py"
    
    if [[ ! -f "$fix_script" ]]; then
        log_warn "Auto-fix script not found: $fix_script"
        return 1
    fi
    
    if ! command -v python3 &> /dev/null; then
        log_warn "Python3 not found. Skipping auto-fix."
        return 1
    fi
    
    python3 "$fix_script" --rules=20.7,17.7 --apply $sources
}

generate_summary() {
    log_info "Generating summary report..."
    
    local summary_file="${REPORTS_DIR}/misra_summary_${TIMESTAMP}.json"
    
    cat > "$summary_file" << EOF
{
    "timestamp": "$(date -Iseconds)",
    "project": "ETH-DDS Integration",
    "target": "$TARGET",
    "tools_used": "$TOOL",
    "configuration": {
        "strict_mode": $STRICT,
        "ci_mode": $CI_MODE
    },
    "reports": {
        "pclint": "${REPORTS_DIR}/pclint_report_${TIMESTAMP}.*",
        "cppcheck": "${REPORTS_DIR}/cppcheck_report_${TIMESTAMP}.*"
    }
}
EOF
    
    log_info "Summary: $summary_file"
}

# ----------------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------------

main() {
    log_info "MISRA C:2012 Compliance Check"
    log_info "Project: ETH-DDS Integration"
    log_info "Timestamp: $TIMESTAMP"
    
    # Parse arguments
    parse_args "$@"
    
    # Create reports directory
    mkdir -p "$REPORTS_DIR"
    
    # Get source files
    log_info "Target scope: $TARGET"
    local sources=$(get_source_files)
    local source_count=$(echo "$sources" | wc -l)
    log_info "Analyzing $source_count source files..."
    
    if [[ -z "$sources" ]]; then
        log_error "No source files found!"
        exit 1
    fi
    
    # Run autofix if requested
    if [[ $FIX -eq 1 ]]; then
        run_autofix "$sources"
    fi
    
    # Run analysis tools
    local overall_exit=0
    
    if [[ "$TOOL" == "pclint" || "$TOOL" == "both" ]]; then
        if ! run_pclint "$sources"; then
            overall_exit=1
        fi
    fi
    
    if [[ "$TOOL" == "cppcheck" || "$TOOL" == "both" ]]; then
        if ! run_cppcheck "$sources"; then
            overall_exit=1
        fi
    fi
    
    # Generate summary
    generate_summary
    
    # Final status
    if [[ $overall_exit -eq 0 ]]; then
        log_info "MISRA check completed successfully!"
    else
        log_error "MISRA check found violations!"
        if [[ $CI_MODE -eq 1 ]]; then
            exit 1
        fi
    fi
    
    exit 0
}

main "$@"
