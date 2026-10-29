#!/usr/bin/env python3
"""DAZZLE's non-reflected CRC over the packed MZ extent."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path


POLYNOMIAL = 0x00102100
MASK32 = 0xFFFFFFFF


def crc32(data: bytes) -> int:
    """Return DAZZLE's CRC (initial state 0, non-reflected, no final XOR)."""
    table = []
    for value in range(256):
        entry = value << 24
        for _ in range(8):
            entry = ((entry << 1) ^ (POLYNOMIAL if entry & 0x80000000 else 0)) & MASK32
        table.append(entry)

    crc = 0
    for byte in data:
        index = ((crc >> 24) ^ byte) & 0xFF
        crc = (((crc << 8) & MASK32) ^ table[index]) & MASK32
    return crc


def _main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("packed_mz", type=Path,
                        help="file containing exactly the packed MZ bytes (no tail)")
    args = parser.parse_args()
    data = args.packed_mz.read_bytes()
    print(f"{crc32(data):08X} ({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(_main())
    except OSError as exc:
        print(f"crc.py: {exc}", file=sys.stderr)
        raise SystemExit(2)
