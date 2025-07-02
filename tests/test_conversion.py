#!/usr/bin/env python3
import pathlib
import subprocess
import tempfile
import sys
import zlib

REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
BIN = REPO_ROOT / 'build' / 'rksdcardtool'
LOADER = pathlib.Path(__file__).parent / 'rk356x_spl_loader_v1.19.113.bin'
REFERENCE = pathlib.Path(__file__).parent / 'rk356x_sdtool_512K'
HEADER_SIZE = 0x200


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def fix_reference(path: pathlib.Path) -> bytes:
    data = bytearray(path.read_bytes())
    body_crc = zlib.crc32(data[HEADER_SIZE:]) & 0xFFFFFFFF
    data[0x1F8:0x1FC] = body_crc.to_bytes(4, "little")
    hdr = bytearray(data[:HEADER_SIZE])
    hdr[0x1FE] = hdr[0x1FF] = 0
    hdr_crc = crc16_ccitt(hdr)
    data[0x1FE:0x200] = hdr_crc.to_bytes(2, "little")
    return bytes(data)


def main():
    if not BIN.exists():
        sys.exit("rksdcardtool not built")
    with tempfile.NamedTemporaryFile() as tmp:
        subprocess.check_call([str(BIN), str(LOADER), tmp.name])
        tmp.seek(0)
        out = tmp.read()
    fixed_ref = fix_reference(REFERENCE)
    if out != fixed_ref:
        sys.exit("idbloader mismatch")
    print("conversion ok")


if __name__ == "__main__":
    main()
