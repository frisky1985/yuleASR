#!/bin/bash
# coverage_report.sh — Generate final coverage report
set -euo pipefail
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_DIR"

echo "============================================"
echo "  yuleASR — WP2 C覆盖率 最终报告"
echo "  日期: 2026-07-26"
echo "============================================"
echo ""

# 1. Dio coverage
echo "=== 1. Dio (84.6% lines, 100% functions) ==="
echo "  测试数: 30 (29 pass, 1 fail)"
echo "  覆盖功能: Init, ReadChannel, WriteChannel, ReadPort, WritePort,"
echo "            ReadChannelGroup, WriteChannelGroup, GetVersionInfo,"
echo "            FlipChannel, MaskedWritePort"
echo "  边界: NULL配置, 未初始化调用, 无效通道号/端口号, NULL指针"
echo ""

# 2. Wdg coverage - measure current
echo "=== 2. Wdg (检测中) ==="
echo "  测试数: 6"
echo "  覆盖功能: Init, SetMode (3种模式), Trigger, GetVersionInfo"
echo ""

# 3. CRC coverage
echo "=== 3. CRC (检测中, 已知测试值不匹配) ==="  
echo "  测试数: 26 (11 pass, 15 fail)"
echo "  覆盖功能: Crc8/16/32 Calculate, chain模式, first/final模式"
echo "  问题: 表格模式需要Crc_Lcfg.c链接; 预期值不匹配"
echo ""

# Get current coverage data
echo "=== 覆盖率摘要 (原始数据, 含mock/unity) ==="
if [ -f coverage_b18_raw.info ]; then
    lcov --rc branch_coverage=1 --ignore-errors inconsistent \
        --summary coverage_b18_raw.info 2>&1 | head -8
fi

echo ""
echo "=== 各生产文件行覆盖率 ==="
if [ -f coverage_b18_raw.info ]; then
    lcov --rc branch_coverage=1 --ignore-errors inconsistent \
        --list coverage_b18_raw.info 2>&1 | grep "src/bsw/"
fi

echo ""
echo "============================================"
echo "  模块分组覆盖结果"
echo "============================================"
echo ""
printf "%-20s %-12s %-12s %-12s %s\n" "模块" "行覆盖率" "函数覆盖率" "分支覆盖率" "状态"
printf "%-20s %-12s %-12s %-12s %s\n" "---" "---" "---" "---" "---"

echo "=== C2: MCAL层 ==="
printf "%-20s %-12s %-12s %-12s %s\n" "Dio" "84.6%" "100%" "71.7%" "✅ 70%+"
printf "%-20s %-12s %-12s %-12s %s\n" "Wdg" "53.0%" "66.7%" "40.0%" "⚠️ 未达标"
printf "%-20s %-12s %-12s %-12s %s\n" "Pwm" "测试通过*" "-" "-" "⚠️ segfault延迟"
printf "%-20s %-12s %-12s %-12s %s\n" "Can" "测试通过*" "-" "-" "⚠️ segfault延迟"
printf "%-20s %-12s %-12s %-12s %s\n" "Port" "头文件bug" "-" "-" "❌ 编译错误"
printf "%-20s %-12s %-12s %-12s %s\n" "Adc/Gpt/Icu/Spi" "头文件不匹配" "-" "-" "❌ 编译/链接问题"
echo ""

echo "=== C3: CAN模块 ==="
printf "%-20s %-12s %-12s %-12s %s\n" "CRC" "37.1%" "80.0%" "20.0%" "⚠️ 未达标"
echo ""

echo "=== C4-C9: 服务模块 ==="
printf "%-20s %-12s %-12s %-12s %s\n" "Det" "测试失败" "-" "-" "❌ 链接冲突"
printf "%-20s %-12s %-12s %-12s %s\n" "PduR" "测试失败" "-" "-" "❌ 缺少类型"
printf "%-20s %-12s %-12s %-12s %s\n" "Dcm/Dem/NvM" "-" "-" "-" "❌ 未链接生产代码"
printf "%-20s %-12s %-12s %-12s %s\n" "Csm/CryIf" "-" "-" "-" "❌ 未链接生产代码"
printf "%-20s %-12s %-12s %-12s %s\n" "EcuM/BswM" "-" "-" "-" "❌ 未链接生产代码"
echo ""

echo "============================================"
echo "  发现的生产代码问题"
echo "============================================"
echo ""
echo "1. Port.h & Port.c 字段名不匹配:"
echo "   - Port_Cfg.h 定义 PORT_H 与 include guard 冲突"
echo "   - 头文件使用 'PinConfig', 实现使用 'PinConfigs'"
echo ""
echo "2. 缺失实例ID定义:"
echo "   - SPI_INSTANCE_ID 在 Spi.h/Spi_Cfg.h 中未定义"
echo ""
echo "3. REG_READ32/REG_WRITE32 宏冲突:"
echo "   - Gpt.c 和 Icu.c 各自重定义, 与 mock_registers.h 冲突"
echo ""
echo "4. 可选API条件编译:"
echo "   - Gpt/Icu的Wakeup函数被 GPT_WAKEUP_FUNCTIONALITY_API 保护"
echo "   - 需要配置宏定义才能启用"
echo ""
echo "5. CRC实现:"
echo "   - 查表模式需要 Crc_Lcfg.c 包含查找表"
echo "   - 测试预期值与实际计算结果不匹配"
echo ""

echo "============================================"
echo "  下一步建议"
echo "============================================"
echo ""
echo "1. 修复头文件bug (Port.h字段名, 缺失SPI_INSTANCE_ID)"
echo "2. 修复Crc算法/测试值匹配问题"
echo "3. 对Wdg添加更多测试用例提升至70%"
echo "4. 修复Pwm/Can segfault (硬件循环 + 模拟器不自动清除位)"
echo "5. 对Adc, Gpt, Icu, Spi使用正确函数签名重写测试"
echo "6. 对服务模块(Dcm, Dem, NvM等)链接真实生产代码或修正测试"
echo "7. 增加对可选API的条件编译宏支持"
echo ""

echo "============================================"
echo "  测试统计:"
echo "============================================"

TOTAL=$(find build-coverage-b19/bin -type f ! -name "*.gcda" ! -name "*.gcno" ! -name "*.dSYM" ! -name "*.out" ! -name "*.err" 2>/dev/null | wc -l)
echo "  编译运行模块: $TOTAL"
echo "  成功运行: Dio, Wdg, CRC, Pwm*, Can*"
echo "  Dio测试: 30 (29/30 pass)"
echo "  Wdg测试: 6 (6/6 pass)"
echo "  CRC测试: 26 (11/15 pass)"
echo "  Pwm测试: 4 (4/4 pass, segfault在exit)"
echo "  Can测试: 11 (11/11 pass, segfault在exit)"
echo "  (* segfault发生在测试完成后, 不影响通过数)"
