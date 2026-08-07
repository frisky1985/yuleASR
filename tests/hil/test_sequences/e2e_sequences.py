"""
E2E protection HIL sequences — simulation-mode validation.

验证 E2E 保护序列定义（Profile 1 CRC8 + Counter + DataID）
与真实 src/bsw/services/e2e 产品实现常量一致。
"""

from conftest import load_src_headers

E2E_HEADER_NAMES = ("E2E_P01.h", "E2E.h")


def _e2e_headers():
    return [h for h in load_src_headers(("E2E", "e2e")) if h.name in E2E_HEADER_NAMES]


def test_e2e_profile1_header_exists():
    assert _e2e_headers(), "未找到 E2E_P01.h / E2E.h 产品头文件"


def test_e2e_p01_constants_match_autosar():
    """E2E Profile 1 关键常量必须符合 AUTOSAR E2E 规范。"""
    text = "\n".join(
        h.read_text(encoding="utf-8", errors="replace") for h in _e2e_headers()
    )
    assert "E2E_P01_COUNTER_MAX" in text, "counter 上限常量必须存在"
    assert "E2E_P01_CRC8_POLYNOMIAL" in text, "CRC8 多项式常量必须存在"
    assert "E2E_P01_DATAID_BOTH" in text, "DataID mode 常量必须存在"


def test_e2e_p01_api_surface():
    """E2E_P01 必须提供 Protect/Check 对称 API。"""
    text = "\n".join(
        h.read_text(encoding="utf-8", errors="replace") for h in _e2e_headers()
    )
    assert "E2E_P01Protect" in text
    assert "E2E_P01Check" in text


def test_e2e_p01_source_implements_protect():
    """真实 E2E_P01.c 必须实现 Protect（非空实现）。"""
    src_root = __import__("conftest").PROJECT_ROOT
    impl = src_root / "src" / "bsw" / "services" / "e2e" / "src" / "E2E_P01.c"
    assert impl.exists(), "E2E_P01.c 不存在"
    text = impl.read_text(encoding="utf-8")
    assert "Std_ReturnType E2E_P01Protect" in text
    # 实现必须写 CRC 与 counter（非空壳）
    assert "State->Counter" in text and "crc" in text.lower()
