#!/bin/bash
# OpenSpec Migration Script
# 将openspec/changes/目录下的规范迁移到openspec/modules/目录

set -e

echo "=== OpenSpec Migration Script ==="
echo ""

OPENSPEC_DIR="openspec"
CHANGES_DIR="$OPENSPEC_DIR/changes"
MODULES_DIR="$OPENSPEC_DIR/modules"

# 创建modules目录结构
mkdir -p "$MODULES_DIR"

# 定义主要模块列表
MODULES=(
    "com:Com:通信模块"
    "dcm:Dcm:诊断通信管理器"
    "dem:Dem:诊断事件管理器"
    "docan:DoCan:CAN诊断"
    "doip:DoIp:IP诊断"
    "integration-tests:IntegrationTests:集成测试"
    "nvm:NvM:非易失性存储管理器"
    "os:Os:操作系统集成"
    "pdur:PduR:PDU路由器"
    "rte:Rte:RTE生成器"
)

# 迁移dev-com-module -> modules/com/
if [ -d "$CHANGES_DIR/dev-com-module" ]; then
    echo "Migrating dev-com-module -> modules/com/"
    mkdir -p "$MODULES_DIR/com/specs"
    cp -f "$CHANGES_DIR/dev-com-module/specs/Com_spec.md" "$MODULES_DIR/com/specs/" 2>/dev/null || true
    if [ -f "$CHANGES_DIR/dev-com-module/tasks.md" ]; then
        cp -f "$CHANGES_DIR/dev-com-module/tasks.md" "$MODULES_DIR/com/" 2>/dev/null || true
    fi
    echo "  ✓ Com module migrated"
fi

# 迁移dev-dcm-dem-module -> modules/dcm/, modules/dem/
if [ -d "$CHANGES_DIR/dev-dcm-dem-module" ]; then
    echo "Migrating dev-dcm-dem-module -> modules/dcm/, modules/dem/"
    mkdir -p "$MODULES_DIR/dcm/specs"
    mkdir -p "$MODULES_DIR/dem/specs"
    cp -f "$CHANGES_DIR/dev-dcm-dem-module/specs/Dcm_spec.md" "$MODULES_DIR/dcm/specs/" 2>/dev/null || true
    cp -f "$CHANGES_DIR/dev-dcm-dem-module/specs/Dem_spec.md" "$MODULES_DIR/dem/specs/" 2>/dev/null || true
    if [ -f "$CHANGES_DIR/dev-dcm-dem-module/tasks.md" ]; then
        cp -f "$CHANGES_DIR/dev-dcm-dem-module/tasks.md" "$MODULES_DIR/dcm/" 2>/dev/null || true
        cp -f "$CHANGES_DIR/dev-dcm-dem-module/tasks.md" "$MODULES_DIR/dem/" 2>/dev/null || true
    fi
    echo "  ✓ Dcm module migrated"
    echo "  ✓ Dem module migrated"
fi

# 迁移dev-doip-docan-module -> modules/doip/, modules/docan/
if [ -d "$CHANGES_DIR/dev-doip-docan-module" ]; then
    echo "Migrating dev-doip-docan-module -> modules/doip/, modules/docan/"
    mkdir -p "$MODULES_DIR/doip/specs"
    mkdir -p "$MODULES_DIR/docan/specs"
    cp -f "$CHANGES_DIR/dev-doip-docan-module/specs/DoIp_spec.md" "$MODULES_DIR/doip/specs/" 2>/dev/null || true
    cp -f "$CHANGES_DIR/dev-doip-docan-module/specs/DoCan_spec.md" "$MODULES_DIR/docan/specs/" 2>/dev/null || true
    if [ -f "$CHANGES_DIR/dev-doip-docan-module/tasks.md" ]; then
        cp -f "$CHANGES_DIR/dev-doip-docan-module/tasks.md" "$MODULES_DIR/doip/" 2>/dev/null || true
        cp -f "$CHANGES_DIR/dev-doip-docan-module/tasks.md" "$MODULES_DIR/docan/" 2>/dev/null || true
    fi
    echo "  ✓ DoIp module migrated"
    echo "  ✓ DoCan module migrated"
fi

# 迁移dev-integration-tests -> modules/integration-tests/
if [ -d "$CHANGES_DIR/dev-integration-tests" ]; then
    echo "Migrating dev-integration-tests -> modules/integration-tests/"
    mkdir -p "$MODULES_DIR/integration-tests/specs"
    cp -f "$CHANGES_DIR/dev-integration-tests/specs/IntegrationTests_spec.md" "$MODULES_DIR/integration-tests/specs/" 2>/dev/null || true
    if [ -f "$CHANGES_DIR/dev-integration-tests/tasks.md" ]; then
        cp -f "$CHANGES_DIR/dev-integration-tests/tasks.md" "$MODULES_DIR/integration-tests/" 2>/dev/null || true
    fi
    echo "  ✓ IntegrationTests module migrated"
fi

# 迁移dev-nvm-enhancement -> modules/nvm/
if [ -d "$CHANGES_DIR/dev-nvm-enhancement" ]; then
    echo "Migrating dev-nvm-enhancement -> modules/nvm/"
    mkdir -p "$MODULES_DIR/nvm/specs"
    cp -f "$CHANGES_DIR/dev-nvm-enhancement/specs/NvM_spec.md" "$MODULES_DIR/nvm/specs/" 2>/dev/null || true
    if [ -f "$CHANGES_DIR/dev-nvm-enhancement/tasks.md" ]; then
        cp -f "$CHANGES_DIR/dev-nvm-enhancement/tasks.md" "$MODULES_DIR/nvm/" 2>/dev/null || true
    fi
    echo "  ✓ NvM module migrated"
fi

# 迁移dev-os-integration -> modules/os/
if [ -d "$CHANGES_DIR/dev-os-integration" ]; then
    echo "Migrating dev-os-integration -> modules/os/"
    mkdir -p "$MODULES_DIR/os/specs"
    cp -f "$CHANGES_DIR/dev-os-integration/specs/Os_Integration_spec.md" "$MODULES_DIR/os/specs/" 2>/dev/null || true
    if [ -f "$CHANGES_DIR/dev-os-integration/tasks.md" ]; then
        cp -f "$CHANGES_DIR/dev-os-integration/tasks.md" "$MODULES_DIR/os/" 2>/dev/null || true
    fi
    echo "  ✓ Os module migrated"
fi

# 迁移dev-pdu-router -> modules/pdur/
if [ -d "$CHANGES_DIR/dev-pdu-router" ]; then
    echo "Migrating dev-pdu-router -> modules/pdur/"
    mkdir -p "$MODULES_DIR/pdur/specs"
    cp -f "$CHANGES_DIR/dev-pdu-router/specs/PduR_spec.md" "$MODULES_DIR/pdur/specs/" 2>/dev/null || true
    if [ -f "$CHANGES_DIR/dev-pdu-router/tasks.md" ]; then
        cp -f "$CHANGES_DIR/dev-pdu-router/tasks.md" "$MODULES_DIR/pdur/" 2>/dev/null || true
    fi
    echo "  ✓ PduR module migrated"
fi

# 迁移dev-rte-generator -> modules/rte/
if [ -d "$CHANGES_DIR/dev-rte-generator" ]; then
    echo "Migrating dev-rte-generator -> modules/rte/"
    mkdir -p "$MODULES_DIR/rte/specs"
    cp -f "$CHANGES_DIR/dev-rte-generator/specs/RteGenerator_spec.md" "$MODULES_DIR/rte/specs/" 2>/dev/null || true
    if [ -f "$CHANGES_DIR/dev-rte-generator/tasks.md" ]; then
        cp -f "$CHANGES_DIR/dev-rte-generator/tasks.md" "$MODULES_DIR/rte/" 2>/dev/null || true
    fi
    echo "  ✓ Rte module migrated"
fi

echo ""
echo "=== Migration Complete ==="
echo ""
echo "Migrated modules:"
find "$MODULES_DIR" -name "*.md" -type f | wc -l | xargs echo "  Total spec files:"
find "$MODULES_DIR" -type d -mindepth 1 -maxdepth 1 | sort
echo ""
echo "Next steps:"
echo "  1. Review migrated files in $MODULES_DIR/"
echo "  2. Run: git add openspec/modules/"
echo "  3. Run: git commit -m 'feat: migrate openspec modules from master branch'"
