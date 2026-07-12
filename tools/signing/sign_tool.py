#!/usr/bin/env python3
"""
yuleASR Secure Boot — 离线签名工具

用途:
  1. 生成 ECDSA P-256 密钥对
  2. 对固件映像签名
  3. 导出 DER 公钥供编译进 bootloader

用法:
  python3 tools/signing/sign_tool.py genkey          # 生成新密钥对
  python3 tools/signing/sign_tool.py sign firmware.bin v1.3.0  # 签名固件
  python3 tools/signing/sign_tool.py dumpkey sbl     # 导出 DER 公钥头文件
"""

import argparse
import hashlib
import struct
import os
import sys

try:
    from cryptography.hazmat.primitives import hashes
    from cryptography.hazmat.primitives.asymmetric import ec
    from cryptography.hazmat.primitives import serialization
    CRYPTO_AVAIL = True
except ImportError:
    CRYPTO_AVAIL = False
    print("[WARN] cryptography not installed. Install: pip install cryptography")
    print("       Fallback: use OpenSSL CLI:")
    print("         openssl ecparam -genkey -name prime256v1 -out private.pem")
    print("         openssl ec -in private.pem -pubout -out public.pem")

KEY_DIR = os.path.join(os.path.dirname(__file__), "keys")

BOOT_IMAGE_MAGIC = 0x314C4259  # 'YBL1'

def ensure_key_dir():
    os.makedirs(KEY_DIR, exist_ok=True)

def genkey():
    if not CRYPTO_AVAIL:
        print("Using OpenSSL fallback...")
        ensure_key_dir()
        os.system(f"openssl ecparam -genkey -name prime256v1 "
                  f"-out {KEY_DIR}/ec_private.pem")
        os.system(f"openssl ec -in {KEY_DIR}/ec_private.pem -pubout "
                  f"-out {KEY_DIR}/ec_public.pem")
        print(f"Keys generated in {KEY_DIR}/")
        return
    ensure_key_dir()
    private_key = ec.generate_private_key(ec.SECP256R1())
    with open(f"{KEY_DIR}/ec_private.pem", "wb") as f:
        f.write(private_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        ))
    public_key = private_key.public_key()
    with open(f"{KEY_DIR}/ec_public.pem", "wb") as f:
        f.write(public_key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo,
        ))
    # Also export raw DER for bootloader
    der = public_key.public_bytes(
        encoding=serialization.Encoding.DER,
        format=serialization.PublicFormat.SubjectPublicKeyInfo,
    )
    with open(f"{KEY_DIR}/ec_public.der", "wb") as f:
        f.write(der)
    print(f"[OK] Keys generated in {KEY_DIR}/")
    print(f"     Private: {KEY_DIR}/ec_private.pem  (KEEP SECRET)")
    print(f"     Public:  {KEY_DIR}/ec_public.pem")
    print(f"     DER:     {KEY_DIR}/ec_public.der  (compile into Boot_Verify.c)")

def sign_image(bin_path, version_str):
    """Sign a firmware binary with ECDSA P-256 + SHA-256."""
    if not CRYPTO_AVAIL:
        print("FALLBACK: Using OpenSSL...")
        _sign_openssl(bin_path, version_str)
        return

    # Parse version
    parts = version_str.lstrip("v").split(".")
    version = (int(parts[0]) << 24) | (int(parts[1]) << 16) | (int(parts[2]) << 8)

    # Read payload
    with open(bin_path, "rb") as f:
        payload = f.read()

    payload_hash = hashlib.sha256(payload).digest()

    # Load private key
    with open(f"{KEY_DIR}/ec_private.pem", "rb") as f:
        private_key = serialization.load_pem_private_key(f.read(), password=None)

    # Sign
    signature = private_key.sign(payload_hash, ec.ECDSA(hashes.SHA256()))
    # ECDSA signature is ASN.1 DER; convert to raw r||s format (64 bytes)
    sig_raw = _der_sig_to_raw(signature)

    # Build output file: [Header(64B)][Payload][Trailer(128B)]
    output_path = bin_path + ".signed"
    with open(output_path, "wb") as f:
        # Header
        hdr = struct.pack("<IIII", BOOT_IMAGE_MAGIC, 0, 0x03, version)
        hdr += struct.pack("<I", len(payload))
        hdr += payload_hash
        hdr += b'\x00' * 12  # reserved
        f.write(hdr)
        # Payload
        f.write(payload)
        # Trailer
        trail = sig_raw
        trail += struct.pack("<I", 0x01)  # algo ECDSA_P256
        trail += struct.pack("<I", 0)     # signing time (unset)
        trail += b'\x00' * 56             # reserved
        f.write(trail)

    print(f"[OK] Signed image: {output_path}")
    print(f"     Version: {version_str} (0x{version:08X})")
    print(f"     Payload: {len(payload)} bytes")
    print(f"     SHA-256: {payload_hash.hex()}")

def _sign_openssl(bin_path, version_str):
    payload_hash = hashlib.sha256(open(bin_path, "rb").read()).hexdigest()
    # Create hash file
    with open("/tmp/boot_hash.bin", "wb") as f:
        f.write(bytes.fromhex(payload_hash))
    os.system(f"openssl dgst -sha256 -sign {KEY_DIR}/ec_private.pem "
              f"-out /tmp/boot_sig.der /tmp/boot_hash.bin")
    # Convert DER to raw r||s
    import subprocess
    result = subprocess.run(
        ["openssl", "asn1parse", "-inform", "DER", "-in", "/tmp/boot_sig.der"],
        capture_output=True, text=True
    )
    print(result.stdout)
    # For production, parse the DER output to extract r and s integers
    print("[WARN] Raw signature extraction from OpenSSL output not implemented.")
    print("       Use cryptography library or a manual DER parser.")

def _der_sig_to_raw(der_sig):
    """Convert DER-encoded ECDSA signature to raw 64-byte r||s format."""
    from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature
    r, s = decode_dss_signature(der_sig)
    # r and s are integers, pad to 32 bytes each
    r_bytes = r.to_bytes(32, byteorder='big')
    s_bytes = s.to_bytes(32, byteorder='big')
    return r_bytes + s_bytes

def dumpkey(target):
    """Export a public key as a C header for compilation."""
    key_name = f"g_boot_pubkey_{target}"
    der_path = f"{KEY_DIR}/ec_public.der"
    if not os.path.exists(der_path):
        print(f"[ERR] DER key not found: {der_path}")
        print("       Run 'genkey' first.")
        return
    with open(der_path, "rb") as f:
        der = f.read()
    # Output as C array
    header = f"/* Auto-generated: g_boot_pubkey_{target} ({len(der)} bytes DER) */\n"
    header += f"const uint8_t {key_name}_data[] = {{\n    "
    for i, b in enumerate(der):
        header += f"0x{b:02X}"
        if i < len(der) - 1:
            header += ", "
        if (i + 1) % 12 == 0:
            header += "\n    "
    header += "\n};\n"
    header += f"const Boot_PubKey {key_name} = {{\n"
    header += f"    .data   = {key_name}_data,\n"
    header += f"    .length = sizeof({key_name}_data)\n"
    header += f"}};\n"
    out_path = f"{KEY_DIR}/{target}_pubkey.h"
    with open(out_path, "w") as f:
        f.write(header)
    print(f"[OK] Public key header: {out_path}")
    print(f"     Copy into Boot_Verify.c or #include from key store.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="yuleASR Secure Boot Signing Tool")
    sub = parser.add_subparsers(dest="cmd")

    p_gen = sub.add_parser("genkey", help="Generate ECDSA P-256 key pair")
    p_sign = sub.add_parser("sign", help="Sign a firmware binary")
    p_sign.add_argument("bin_path", help="Path to firmware .bin file")
    p_sign.add_argument("version", help="Semantic version (e.g. v1.3.0)")

    p_dump = sub.add_parser("dumpkey", help="Export DER public key as C header")
    p_dump.add_argument("target", choices=["sbl", "app"],
                        help="Key target (sbl or app)")

    args = parser.parse_args()
    if args.cmd == "genkey":
        genkey()
    elif args.cmd == "sign":
        if not os.path.exists(f"{KEY_DIR}/ec_private.pem"):
            print("[ERR] No private key found. Run 'genkey' first.")
            sys.exit(1)
        sign_image(args.bin_path, args.version)
    elif args.cmd == "dumpkey":
        dumpkey(args.target)
    else:
        parser.print_help()
