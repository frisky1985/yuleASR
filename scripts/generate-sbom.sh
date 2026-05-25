#!/usr/bin/env bash
# ============================================================================
# YuleTech AutoSAR BSW Platform - SBOM Generator
# 生成 SPDX 2.3 JSON 格式的 SBOM (Software Bill of Materials)
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUTPUT_FILE="${1:-${PROJECT_DIR}/sbom.json}"

# --- Project metadata ---
PROJECT_NAME="yuleASR"
PROJECT_ID="YuleTech-AutoSAR"
PROJECT_VERSION="1.0.0"
PROJECT_DESCRIPTION="YuleTech AutoSAR BSW Platform - Open Source BSW Implementation"
PROJECT_HOMEPAGE="https://github.com/frisky1985/yuleASR"
PROJECT_SUPPLIER="Shanghai Yule Electronics Technology Co., Ltd."
PROJECT_AUTHOR="YuleTech"
GIT_HASH="$(cd "${PROJECT_DIR}" && git rev-parse HEAD 2>/dev/null || echo "unknown")"
GIT_TAG="$(cd "${PROJECT_DIR}" && git describe --tags --always 2>/dev/null || echo "unknown")"
CREATION_DATE="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
NAMESPACE_BASE="https://github.com/frisky1985/yuleASR"

# --- Helper: SPDX identifier for a license ---
make_license_id() {
    local lic="$1"
    case "${lic}" in
        Apache-2.0)              echo "Apache-2.0" ;;
        MIT)                     echo "MIT" ;;
        "MIT/CC0")               echo "MIT AND CC0-1.0" ;;
        GPL-2.0-or-later)        echo "GPL-2.0-or-later" ;;
        "Apache-2.0 OR GPL-2.0-or-later") echo "Apache-2.0 OR GPL-2.0-or-later" ;;
        BSD-3-Clause)            echo "BSD-3-Clause" ;;
        "MIT OR Apache-2.0")     echo "MIT OR Apache-2.0" ;;
        "Apache-2.0 WITH LLVM-exception") echo "Apache-2.0 WITH LLVM-exception" ;;
        "")                      echo "NOASSERTION" ;;
        *)                       echo "${lic}" ;;
    esac
}

# --- Helper: create a single SPDX package entry ---
make_package() {
    local ref_id="$1"
    local name="$2"
    local version="$3"
    local supplier="$4"
    local license="$5"
    local copyright="$6"
    local summary="$7"
    local download="$8"
    local homepage="$9"
    local filename="${10}"

    local lic_id
    lic_id="$(make_license_id "${license}")"

    cat <<EOF
    {
      "SPDXID": "SPDXRef-${ref_id}",
      "name": "${name}",
      "versionInfo": "${version}",
      "supplier": "Organization: ${supplier}",
      "downloadLocation": "${download}",
      "homepage": "${homepage}",
      "packageFileName": "${filename}",
      "licenseConcluded": "${lic_id}",
      "licenseDeclared": "${lic_id}",
      "copyrightText": "${copyright}",
      "summary": "${summary}"
    }
EOF
}

# --- Helper: create a relationship entry ---
make_relationship() {
    local from="$1"
    local rel="$2"
    local to="$3"
    cat <<EOF
    {
      "spdxElementId": "SPDXRef-${from}",
      "relationshipType": "${rel}",
      "relatedSpdxElement": "SPDXRef-${to}"
    }
EOF
}

# ---- Build the SBOM JSON ----
cat > "${OUTPUT_FILE}" <<EOF
{
  "spdxVersion": "SPDX-2.3",
  "dataLicense": "CC0-1.0",
  "SPDXID": "SPDXRef-DOCUMENT",
  "name": "${PROJECT_NAME} - SBOM",
  "documentNamespace": "${NAMESPACE_BASE}/sbom/${GIT_HASH}/${CREATION_DATE}",
  "creationInfo": {
    "created": "${CREATION_DATE}",
    "creators": [
      "Organization: ${PROJECT_SUPPLIER}",
      "Tool: generate-sbom.sh-1.0",
      "Person: YuleTech-SBOM-Generator"
    ]
  },
  "documentDescribes": [
    "SPDXRef-${PROJECT_ID}"
  ],
  "packages": [
    $(make_package "${PROJECT_ID}" \
      "${PROJECT_NAME}" \
      "${PROJECT_VERSION}" \
      "${PROJECT_SUPPLIER}" \
      "MIT" \
      "Copyright (c) 2024-2026 ${PROJECT_SUPPLIER}" \
      "${PROJECT_DESCRIPTION}" \
      "${NAMESPACE_BASE}" \
      "${PROJECT_HOMEPAGE}" \
      "${PROJECT_NAME}"),

    $(make_package "mbedtls" \
      "mbedTLS" \
      "2.28.8" \
      "Arm Limited" \
      "Apache-2.0" \
      "Copyright (c) 2006-2024, Arm Limited and Contributors" \
      "轻量级TLS/SSL库，用于MQTT模块的TLS/mTLS安全通信" \
      "${NAMESPACE_BASE}/tree/main/third_party/mbedtls" \
      "https://github.com/Mbed-TLS/mbedtls" \
      "third_party/mbedtls"),

    $(make_package "arm-gcc" \
      "ARM GCC Compiler Toolchain" \
      "10.3-2021.10" \
      "Arm Limited" \
      "GPL-2.0-or-later WITH GCC-exception-2.0" \
      "Copyright (c) 2009-2021, Arm Limited" \
      "ARM嵌入式GCC交叉编译工具链，用于目标平台 NXP S32K312 编译" \
      "https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain" \
      "https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain" \
      "arm-none-eabi-gcc"),

    $(make_package "cmake" \
      "CMake" \
      "4.3.2" \
      "Kitware, Inc." \
      "BSD-3-Clause" \
      "Copyright (c) 2000-2024 Kitware, Inc." \
      "跨平台构建系统，用于项目构建配置管理" \
      "https://cmake.org/download/" \
      "https://cmake.org" \
      "cmake"),

    $(make_package "make" \
      "GNU Make" \
      "4.3" \
      "Free Software Foundation, Inc." \
      "GPL-3.0-or-later" \
      "Copyright (c) 1988-2022 Free Software Foundation, Inc." \
      "自动化构建工具，用于执行构建规则" \
      "https://ftp.gnu.org/gnu/make/" \
      "https://www.gnu.org/software/make/" \
      "make"),

    $(make_package "jinja2" \
      "Jinja2" \
      "3.1.6" \
      "Pallets Project" \
      "BSD-3-Clause" \
      "Copyright (c) 2007-2024 Pallets" \
      "Python模板引擎，用于代码生成器" \
      "https://pypi.org/project/Jinja2/" \
      "https://jinja.palletsprojects.com" \
      "jinja2"),

    $(make_package "pytest" \
      "pytest" \
      "8.0.0" \
      "Holger Krekel and pytest contributors" \
      "MIT" \
      "Copyright (c) 2004-2024 Holger Krekel and pytest contributors" \
      "Python单元测试框架，用于测试代码生成器和工具" \
      "https://pypi.org/project/pytest/" \
      "https://pytest.org" \
      "pytest"),

    $(make_package "aes-modes" \
      "AES Modes" \
      "1.0" \
      "${PROJECT_SUPPLIER}" \
      "MIT" \
      "Copyright (c) 2024-2026 ${PROJECT_SUPPLIER}" \
      "AES加密模式实现，支持CBC/CTR/GCM/CCM/ECB/CFB/OFB" \
      "${NAMESPACE_BASE}/tree/main/third_party/crypto/aes_modes" \
      "" \
      "third_party/crypto/aes_modes"),

    $(make_package "blake2" \
      "Blake2" \
      "1.0" \
      "${PROJECT_SUPPLIER}" \
      "MIT AND CC0-1.0" \
      "Copyright (c) 2012-2024 Jean-Philippe Aumasson and Samuel Neves" \
      "Blake2哈希算法实现，支持Blake2b和Blake2s" \
      "${NAMESPACE_BASE}/tree/main/third_party/crypto/blake2" \
      "" \
      "third_party/crypto/blake2"),

    $(make_package "hash" \
      "Hash (SHA系列)" \
      "1.0" \
      "${PROJECT_SUPPLIER}" \
      "MIT" \
      "Copyright (c) 2024-2026 ${PROJECT_SUPPLIER}" \
      "SHA系列哈希算法实现，支持SHA-1/SHA-2/SHA-3" \
      "${NAMESPACE_BASE}/tree/main/third_party/crypto/hash" \
      "" \
      "third_party/crypto/hash"),

    $(make_package "unity" \
      "Unity Test Framework" \
      "2.6.0" \
      "ThrowTheSwitch.org" \
      "MIT" \
      "Copyright (c) 2007-2024 Mike Karlesky, Mark VanderVoord, Greg Williams" \
      "轻量级C语言单元测试框架，用于嵌入式系统测试" \
      "${NAMESPACE_BASE}/tree/main/third_party/test_frameworks/unity" \
      "http://www.throwtheswitch.org/unity" \
      "third_party/test_frameworks/unity"),

    $(make_package "yule-mbedtls-adapter" \
      "YuleTech mbedTLS Adapter" \
      "1.0" \
      "${PROJECT_SUPPLIER}" \
      "MIT" \
      "Copyright (c) 2024-2026 ${PROJECT_SUPPLIER}" \
      "AUTOSAR平台mbedTLS适配层，连接mbedTLS与AUTOSAR BSW" \
      "${NAMESPACE_BASE}/tree/main/third_party/yule-mbedtls-adapter" \
      "" \
      "third_party/yule-mbedtls-adapter")
  ],
  "relationships": [
    $(make_relationship "${PROJECT_ID}" "DEPENDS_ON" "mbedtls"),
    $(make_relationship "${PROJECT_ID}" "DEPENDS_ON" "arm-gcc"),
    $(make_relationship "${PROJECT_ID}" "DEPENDS_ON" "cmake"),
    $(make_relationship "${PROJECT_ID}" "DEPENDS_ON" "make"),
    $(make_relationship "${PROJECT_ID}" "DEPENDS_ON" "jinja2"),
    $(make_relationship "${PROJECT_ID}" "DEPENDS_ON" "pytest"),
    $(make_relationship "${PROJECT_ID}" "DEPENDS_ON" "aes-modes"),
    $(make_relationship "${PROJECT_ID}" "DEPENDS_ON" "blake2"),
    $(make_relationship "${PROJECT_ID}" "DEPENDS_ON" "hash"),
    $(make_relationship "${PROJECT_ID}" "DEPENDS_ON" "unity"),
    $(make_relationship "${PROJECT_ID}" "DEPENDS_ON" "yule-mbedtls-adapter"),
    $(make_relationship "${PROJECT_ID}" "CONTAINS" "mbedtls"),
    $(make_relationship "${PROJECT_ID}" "CONTAINS" "aes-modes"),
    $(make_relationship "${PROJECT_ID}" "CONTAINS" "blake2"),
    $(make_relationship "${PROJECT_ID}" "CONTAINS" "hash"),
    $(make_relationship "${PROJECT_ID}" "CONTAINS" "unity"),
    $(make_relationship "${PROJECT_ID}" "CONTAINS" "yule-mbedtls-adapter"),
    $(make_relationship "mbedtls" "DEPENDENCY_MANIFEST_OF" "${PROJECT_ID}"),
    $(make_relationship "arm-gcc" "BUILD_TOOL_OF" "${PROJECT_ID}"),
    $(make_relationship "cmake" "BUILD_TOOL_OF" "${PROJECT_ID}"),
    $(make_relationship "make" "BUILD_TOOL_OF" "${PROJECT_ID}"),
    $(make_relationship "jinja2" "DEPENDENCY_MANIFEST_OF" "${PROJECT_ID}"),
    $(make_relationship "pytest" "DEPENDENCY_MANIFEST_OF" "${PROJECT_ID}")
  ]
}
EOF

# Validate JSON
if command -v python3 &>/dev/null; then
    if python3 -c "import json; json.load(open('${OUTPUT_FILE}'))" 2>/dev/null; then
        echo "[✓] SBOM generated successfully: ${OUTPUT_FILE}"
        echo "[✓] JSON validation passed"
    else
        echo "[✗] JSON validation failed!"
        exit 1
    fi
else
    echo "[!] python3 not found for validation; output written to ${OUTPUT_FILE}"
fi

echo ""
echo "Document SPDXID: SPDXRef-DOCUMENT"
echo "Document Namespace: ${NAMESPACE_BASE}/sbom/${GIT_HASH}/${CREATION_DATE}"
echo "Packages included: $(cd "${PROJECT_DIR}" && python3 -c "import json; print(len([p for p in json.load(open('${OUTPUT_FILE}'))['packages']]))" 2>/dev/null || echo "?")"
echo "Relationships included: $(cd "${PROJECT_DIR}" && python3 -c "import json; print(len([r for r in json.load(open('${OUTPUT_FILE}'))['relationships']]))" 2>/dev/null || echo "?")"
