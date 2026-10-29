#!/usr/bin/env python3
"""Build DAZZLE's one-byte marker, self-check frame, and compressed DOC tail."""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path


TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from crc import crc32
from lzhuf import encode


MARKER = b"\x84"
DEFAULT_R1 = 500
DEFAULT_R2 = 1000
ORIGINAL_IMAGE_OFFSET = 0x8802  # 4-byte size field + 34,814-byte LZH stream.
ORIGINAL_EXE = Path(__file__).resolve().parents[1] / "assets" / "DAZZLE.EXE"
ORIGINAL_SIZE = 158_892
ORIGINAL_SHA256 = "fc68905570e0f139cafc0bc0f75af87445c6fb29f53f5f02018fa9b71f714ee8"


def u16(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset:offset + 2], "little")


def mz_extent(data: bytes) -> int:
    if len(data) < 0x1E or data[:2] != b"MZ":
        raise ValueError("packed input is not an MZ executable")
    last = u16(data, 2)
    pages = u16(data, 4)
    if pages == 0 or last > 512:
        raise ValueError("invalid MZ page-count fields")
    return (pages - 1) * 512 + (last or 512)


def resource_base(data: bytes) -> int:
    """Apply DAZZLE's `(e_cp-1)*512+e_cblp+1` resource-base formula."""
    if len(data) < 6 or data[:2] != b"MZ":
        raise ValueError("packed input is not an MZ executable")
    last = u16(data, 2)
    pages = u16(data, 4)
    if pages == 0 or last > 512:
        raise ValueError("invalid MZ page-count fields")
    return (pages - 1) * 512 + last + 1


def load_original_image_resource() -> bytes:
    """Read the separate, non-text image payload after the compressed manual."""
    original = ORIGINAL_EXE.read_bytes()
    if len(original) != ORIGINAL_SIZE or hashlib.sha256(original).hexdigest() != ORIGINAL_SHA256:
        raise ValueError("pinned original distribution failed its size/SHA-256 check")
    extent = mz_extent(original)
    base = resource_base(original)
    if base != extent + 1 or original[extent:base] != MARKER:
        raise ValueError("original MZ boundary/leading resource marker is inconsistent")
    frame = original[base:base + 10]
    if len(frame) != 10:
        raise ValueError("original distribution has a truncated metadata frame")
    if struct.unpack_from("<I", frame)[0] != crc32(original[:extent]):
        raise ValueError("original metadata CRC does not cover the packed MZ extent")
    offset = struct.unpack_from("<H", frame, 8)[0]
    if offset != ORIGINAL_IMAGE_OFFSET:
        raise ValueError("pinned original image offset changed")
    image_start = base + 10 + offset
    compressed_start = base + 14
    if offset < 4 or compressed_start + offset - 4 != image_start:
        raise ValueError("original image offset does not follow the LZH stream")
    if image_start > len(original):
        raise ValueError("original embedded image resource starts outside the file")
    return original[image_start:]


def build_tail(packed_mz: bytes, manual: bytes, *, r1: int = DEFAULT_R1,
               r2: int = DEFAULT_R2, image_resource: bytes | None = None,
               marker: bytes = MARKER) -> bytes:
    """Build marker + metadata + manual LZH stream + embedded image resource.

    The image payload is not in DAZZLE.DOC. Unless supplied explicitly, the
    immutable distribution's separate image payload is used as its source.
    """
    extent = mz_extent(packed_mz)
    if extent != len(packed_mz):
        raise ValueError(f"packed MZ extent is {extent}, file has {len(packed_mz)} bytes")
    base = resource_base(packed_mz)
    if base != extent + 1:
        raise ValueError(f"DAZZLE resource base is {base}, expected MZ extent + 1 ({extent + 1})")
    if len(marker) != 1:
        raise ValueError("the leading marker must be exactly one byte")
    for name, value in (("r1", r1), ("r2", r2)):
        if not 0 <= value <= 0xFFFF:
            raise ValueError(f"{name} must fit in an unsigned 16-bit word")
    if len(manual) > 0xFFFFFFFF:
        raise ValueError("manual is too large for DAZZLE's 32-bit length field")

    compressed = encode(manual)
    image_offset = 4 + len(compressed)
    if image_offset > 0xFFFF:
        raise ValueError("manual stream and length exceed the frame's 16-bit image offset")
    if image_resource is None:
        image_resource = load_original_image_resource()

    frame = struct.pack("<IHHH", crc32(packed_mz), r1, r2, image_offset)
    return (marker + frame + struct.pack("<I", len(manual)) + compressed
            + image_resource)


def _main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("packed_mz", type=Path)
    parser.add_argument("manual_doc", type=Path)
    parser.add_argument("output_tail", type=Path)
    parser.add_argument("--r1", type=int, default=DEFAULT_R1)
    parser.add_argument("--r2", type=int, default=DEFAULT_R2)
    parser.add_argument("--image-resource", type=Path,
                        help="separate embedded image payload; defaults to the pinned original's payload")
    args = parser.parse_args()
    packed_mz = args.packed_mz.read_bytes()
    manual = args.manual_doc.read_bytes()
    image_resource = args.image_resource.read_bytes() if args.image_resource else None
    tail = build_tail(packed_mz, manual, r1=args.r1, r2=args.r2,
                      image_resource=image_resource)
    args.output_tail.parent.mkdir(parents=True, exist_ok=True)
    args.output_tail.write_bytes(tail)
    compressed_size = struct.unpack_from("<H", tail, 9)[0] - 4
    print(f"tail: {len(tail)} bytes; CRC {crc32(packed_mz):08X}; "
          f"manual {len(manual)} -> {compressed_size} LZH bytes; "
          f"image resource {len(tail) - 15 - compressed_size} bytes")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(_main())
    except (OSError, ValueError) as exc:
        print(f"mktail.py: {exc}", file=sys.stderr)
        raise SystemExit(2)
