"""
yuleASR HIL test sequences — shared simulation-mode helpers.

HIL 序列定义验证（simulation mode）：
在无硬件环境下，验证 HIL 测试序列定义与真实产品代码一致
（UDS SID / CAN ID / E2E 配置 / SecOC 参数），保证测试序列
与 BSW 实现同步，硬件就绪后序列可直接复用。
"""

import re
from pathlib import Path

HIL_DIR = Path(__file__).resolve().parent.parent  # tests/hil
PROJECT_ROOT = HIL_DIR.parent.parent


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def extract_defines(text: str) -> dict:
    """Extract `#define NAME value` pairs from C source text."""
    defines = {}
    for m in re.finditer(r"#define\s+([A-Za-z_][A-Za-z0-9_]*)\s+([^/\n]+)", text):
        name = m.group(1).strip()
        value = m.group(2).strip().split("//")[0].strip()
        defines[name] = value
    return defines


def load_hil_c_sources() -> list[Path]:
    return sorted(HIL_DIR.glob("*.c"))


def load_src_headers(module_keywords: tuple[str, ...]) -> list[Path]:
    """Load real BSW headers matching module keywords (e.g. Dcm/Can/E2E)."""
    headers = []
    src_root = PROJECT_ROOT / "src"
    if not src_root.exists():
        return headers
    for h in src_root.rglob("*.h"):
        if any(k in h.name for k in module_keywords):
            headers.append(h)
    return headers
