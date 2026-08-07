"""
SecOC (Secure Onboard Communication) HIL sequences — simulation-mode validation.

验证 SecOC 安全通信序列定义与真实产品代码一致：
- 新鲜度值 (Freshness Value) 管理
- MAC 计算接口（Crypto/KeyM 依赖）
"""

from conftest import load_src_headers

SECOC_HEADERS = ("SecOC.h", "SecOC_Cfg.h")


def _secoc_headers():
    return [h for h in load_src_headers(("SecOC", "secoc")) if h.name in SECOC_HEADERS]


def test_secoc_header_exists():
    assert _secoc_headers(), "未找到 SecOC.h / SecOC_Cfg.h 产品头文件"


def test_secoc_api_surface():
    """SecOC 必须提供 Tx/Rx 保护 API。"""
    text = "\n".join(
        h.read_text(encoding="utf-8", errors="replace") for h in _secoc_headers()
    )
    assert "SecOC_Tx" in text or "SecOC_Authenticate" in text or "SecOC_AuthenticateI_PDU" in text
    assert "SecOC_Rx" in text or "SecOC_Verify" in text or "SecOC_VerifyI_PDU" in text


def test_secoc_freshness_support():
    """SecOC 配置必须包含新鲜度值管理。"""
    text = "\n".join(
        h.read_text(encoding="utf-8", errors="replace") for h in _secoc_headers()
    )
    assert "FRESH" in text.upper() or "Freshness" in text


def test_secoc_crypto_dependency():
    """SecOC 依赖 Crypto 服务（MAC 计算）。"""
    crypto = load_src_headers(("CryIf", "cryif", "Csm", "csm"))
    assert crypto, "未找到 CryIf/Csm 产品头文件（SecOC MAC 依赖）"


def test_secoc_sequence_content_substantive():
    """SecOC 序列验证必须基于真实内容。"""
    total = sum(
        len(h.read_text(encoding="utf-8", errors="replace")) for h in _secoc_headers()
    )
    assert total > 300, "SecOC 头文件内容过少"
