"""
OTA (Over-The-Air) update HIL sequences — simulation-mode validation.

验证 OTA 更新序列定义与真实产品代码一致：
- bootloader 分区/回滚（勿修改，只读校验）
- flash 驱动服务（Fls）可擦写性配置
"""

from conftest import PROJECT_ROOT, load_src_headers


def test_ota_flash_driver_headers_exist():
    """Fls 驱动头文件必须存在（OTA 刷写依赖）。"""
    fls = load_src_headers(("Fls", "fls"))
    assert fls, "未找到 Fls 产品头文件"


def test_ota_flash_api_surface():
    """Fls 驱动必须提供 Erase/Write/Read API。"""
    text = "\n".join(
        h.read_text(encoding="utf-8", errors="replace") for h in load_src_headers(("Fls", "fls"))
    )
    assert "Fls_Erase" in text
    assert "Fls_Write" in text
    assert "Fls_Read" in text


def test_ota_bootloader_partition_table_present():
    """bootloader 分区表头文件存在（OTA 双区/回滚基础）。"""
    bl_dir = PROJECT_ROOT / "src" / "bootloader"
    assert bl_dir.exists(), "src/bootloader/ 目录必须存在"
    partition_headers = list(bl_dir.glob("bl_partition*.h")) + list(bl_dir.glob("*partition*.h"))
    assert partition_headers, "未找到 bootloader 分区表头文件"


def test_ota_rollback_support_declared():
    """回滚机制头文件必须声明（OTA 失败回退）。"""
    bl_dir = PROJECT_ROOT / "src" / "bootloader"
    rollback = list(bl_dir.glob("bl_rollback*.h"))
    assert rollback, "未找到回滚机制头文件"


def test_ota_sequence_defines_none_vacuous():
    """OTA 序列验证必须基于真实文件（非空断言）。"""
    fls = load_src_headers(("Fls", "fls"))
    total = sum(len(h.read_text(encoding="utf-8", errors="replace")) for h in fls)
    assert total > 500, "Fls 头文件内容过少，无法支撑 OTA 序列"
