#!/bin/bash
#
# check-licenses.sh - License Compliance Checker
#
# 自动扫描项目所有源代码文件的 License 头合规性。
# 检查每个文件的开头是否包含标准版权和许可证声明。
#
# Usage:
#   ./check-licenses.sh                    # 检查所有源文件
#   ./check-licenses.sh --strict           # 严格模式（包含 third_party）
#   ./check-licenses.sh --fix-header FILE  # 为指定文件添加 License 头（TODO）
#   ./check-licenses.sh --summary          # 只输出汇总报告
#
# Exit codes:
#   0 - 所有文件合规
#   1 - 存在缺失 License 头的文件
#   2 - 参数错误
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ─── 配置 ────────────────────────────────────────────────────────────────

# 需要检查的源文件扩展名
SOURCE_EXTS=(
    ".c" ".h" ".cpp" ".hpp" ".cc" ".hh"
    ".py" ".sh" ".go" ".rs" ".java" ".kt"
    ".js" ".ts" ".jsx" ".tsx" ".vue"
    ".yaml" ".yml" ".cmake" ".txt"
)

# 排除的目录/文件（正则表达式）
EXCLUDE_PATTERNS=(
    "^${PROJECT_DIR}/\.git/"
    "^${PROJECT_DIR}/build/"
    "^${PROJECT_DIR}/out/"
    "^${PROJECT_DIR}/node_modules/"
    "^${PROJECT_DIR}/\.venv/"
    "^${PROJECT_DIR}/venv/"
    "^${PROJECT_DIR}/__pycache__/"
    "^${PROJECT_DIR}/website/node_modules/"
    "^${PROJECT_DIR}/website/dist/"
    "\.gitkeep$"
    "\.gitignore$"
)

# 仅 --strict 模式下检查 third_party
STRICT_EXCLUDE=(
)

# 最小 License 头行数（防止空文件误报）
MIN_LICENSE_LINES=3

# ─── License 头模式 ─────────────────────────────────────────────────────
# 这些关键字用于检测文件中是否包含 License 声明

LICENSE_KEYWORDS=(
    "MIT License"
    "Apache License"
    "GNU General Public License"
    "GNU Lesser General Public License"
    "GNU Affero General Public License"
    "BSD [0-9]-Clause"
    "Boost Software License"
    "Mozilla Public License"
    "The Unlicense"
    "CC0"
    "Public Domain"
    "All rights reserved"
)

COPYRIGHT_KEYWORDS=(
    "Copyright"
    "©"
    "(c)"
)

# ─── 函数 ────────────────────────────────────────────────────────────────

usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Options:
  --strict      包含 third_party/ 目录的检查
  --summary     仅输出汇总报告
  --list-ok     列出合规的文件（默认只列出不合规的）
  --fix-header FILE  为指定文件添加 License 头（开发中）
  --help        显示此帮助信息

Exit codes:
  0  - 所有文件合规
  1  - 存在缺失 License 头的文件
  2  - 参数错误
EOF
    exit 0
}

log_info()  { echo "[INFO]  $*"; }
log_warn()  { echo "[WARN]  $*"; }
log_error() { echo "[ERROR] $*"; }

# 检查文件是否应被排除
is_excluded() {
    local file="$1"
    local -n patterns="$2"
    for pattern in "${patterns[@]}"; do
        if [[ "$file" =~ $pattern ]]; then
            return 0
        fi
    done
    return 1
}

# 检查文件是否有 License 头
has_license_header() {
    local file="$1"
    local header
    header=$(head -n 30 "$file" 2>/dev/null) || return 1

    local has_copyright=false
    local has_license=false

    for kw in "${COPYRIGHT_KEYWORDS[@]}"; do
        if echo "$header" | grep -iq "$kw"; then
            has_copyright=true
            break
        fi
    done

    for kw in "${LICENSE_KEYWORDS[@]}"; do
        if echo "$header" | grep -iqE "$kw"; then
            has_license=true
            break
        fi
    done

    $has_copyright && $has_license
    return $?
}

# 检查 LICENSE 文件是否存在
check_license_file() {
    local dir="$1"
    for f in LICENSE LICENSE.md LICENSE.txt; do
        [[ -f "${dir}/${f}" ]] && return 0
    done
    return 1
}

# ─── 主逻辑 ──────────────────────────────────────────────────────────────

STRICT_MODE=false
SUMMARY_ONLY=false
LIST_OK=false
FIX_HEADER=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --strict)    STRICT_MODE=true; shift ;;
        --summary)   SUMMARY_ONLY=true; shift ;;
        --list-ok)   LIST_OK=true; shift ;;
        --fix-header) FIX_HEADER="$2"; shift 2 ;;
        --help|-h)   usage ;;
        *)           log_error "未知参数: $1"; usage; exit 2 ;;
    esac
done

# 切换到项目目录
cd "$PROJECT_DIR"

# 初始化计数器
total_files=0
ok_files=0
fail_files=0
missing_license_dirs=()
checked_dirs=()

echo "========================================"
echo " YuleTech License Compliance Checker"
echo "========================================"
echo "项目路径: ${PROJECT_DIR}"
echo "严格模式: ${STRICT_MODE}"
echo ""

# ── 阶段 1: 检查第三方 LICENSE 文件 ─────────────────────────────────────
echo "─── 阶段 1: 检查 LICENSE 文件 ───"

if $STRICT_MODE; then
    THIRD_PARTY_DIRS=(
        "$PROJECT_DIR/third_party"
    )
else
    THIRD_PARTY_DIRS=()
fi

# 检查 third_party 下的所有一级和二级子目录（组件级别）
# 扫描深度: third_party/*/ (一级) 和 third_party/*/*/ (二级)
# 避免检查 src/, include/ 等源码子目录
while IFS= read -r -d '' dir; do
    checked_dirs+=("$dir")
    if check_license_file "$dir"; then
        log_info "  [OK]  $(realpath --relative-to="$PROJECT_DIR" "$dir")/LICENSE"
    else
        log_error "[MISS] $(realpath --relative-to="$PROJECT_DIR" "$dir")/ 缺少 LICENSE 文件"
        missing_license_dirs+=("$dir")
    fi
done < <(find "$PROJECT_DIR/third_party" -mindepth 1 -maxdepth 2 -type d \
    ! -path '*/.git/*' ! -path '*/node_modules/*' ! -path '*/src' ! -path '*/include' ! -path '*/tests' \
    -print0 2>/dev/null || true)

# 也检查项目根目录
if check_license_file "$PROJECT_DIR"; then
    log_info "  [OK]  项目根目录/LICENSE"
else
    log_error "[MISS] 项目根目录缺少 LICENSE 文件"
    missing_license_dirs+=("$PROJECT_DIR")
fi

echo ""

# ── 阶段 2: 检查源文件 License 头 ──────────────────────────────────────
echo "─── 阶段 2: 检查源文件 License 头 ───"

# 构建 find 参数
find_args=()
for ext in "${SOURCE_EXTS[@]}"; do
    find_args+=(-o -name "*${ext}")
done
unset 'find_args[0]'  # 移除第一个 -o

# 收集要检查的文件
file_list=$(mktemp)
trap 'rm -f "$file_list"' EXIT

find "$PROJECT_DIR" -type f \( "${find_args[@]}" \) \
    ! -path '*/.git/*' \
    ! -path '*/build/*' \
    ! -path '*/node_modules/*' \
    ! -path '*/__pycache__/*' \
    ! -path '*/.venv/*' \
    ! -path '*/venv/*' \
    ! -path '*/website/node_modules/*' \
    ! -path '*/website/dist/*' \
    ! -name '.gitkeep' \
    ! -name '.gitignore' \
    > "$file_list" 2>/dev/null || true

# 如果非严格模式，排除 third_party
if ! $STRICT_MODE; then
    grep -v "^${PROJECT_DIR}/third_party/" "$file_list" > "${file_list}.tmp"
    mv "${file_list}.tmp" "$file_list"
fi

total_files=$(wc -l < "$file_list" | tr -d ' ')

while IFS= read -r file; do
    if [[ -z "$file" ]]; then continue; fi

    if has_license_header "$file"; then
        if $LIST_OK; then
            echo "  [OK]  $(realpath --relative-to="$PROJECT_DIR" "$file")"
        fi
        ((ok_files++))
    else
        rel_path=$(realpath --relative-to="$PROJECT_DIR" "$file")
        echo "  [MISS] $rel_path"
        ((fail_files++))
    fi
done < "$file_list"

echo ""
echo "─── 结果汇总 ───"
echo "  总文件数:      ${total_files}"
echo "  合规:          ${ok_files}"
echo "  不合规:        ${fail_files}"

if $STRICT_MODE; then
    echo ""
    echo "  LICENSE 文件缺失目录:"
    if [[ ${#missing_license_dirs[@]} -eq 0 ]]; then
        echo "    无"
    else
        for d in "${missing_license_dirs[@]}"; do
            echo "    [MISS] $(realpath --relative-to="$PROJECT_DIR" "$d")/"
        done
    fi
fi

echo ""
echo "========================================"

if [[ ${#missing_license_dirs[@]} -gt 0 ]] && $STRICT_MODE; then
    log_warn "存在 ${#missing_license_dirs[@]} 个目录缺少 LICENSE 文件"
fi

if [[ $fail_files -gt 0 ]]; then
    log_warn "存在 ${fail_files} 个源文件缺少 License 头"
    exit 1
fi

log_info "所有检查通过！"
exit 0
