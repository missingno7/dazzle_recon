#!/usr/bin/env python3
"""Strict DAZZLE packaging through PKLITE 1.12 Professional and the original tail."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ORIGINAL = ROOT / "assets" / "DAZZLE.EXE"
LOAD_IMAGE = ROOT / "reference" / "load_image.bin"
ORACLE = ROOT / "evidence" / "oracle.json"
SCRATCH = ROOT / "build" / "workers" / "dz-pack"
DEFAULT_PKLITE = Path(r"C:\tools\pklite-1.12-pro\PKLITE.EXE")
DEFAULT_RUNNER = Path(r"C:\tools\nmlgcdos\msdos.exe")

ORIGINAL_SIZE = 158_892
ORIGINAL_SHA256 = "fc68905570e0f139cafc0bc0f75af87445c6fb29f53f5f02018fa9b71f714ee8"
PACKED_MZ_SIZE = 80_641
PACKED_MZ_SHA256 = "46e3699558773b23e91a86ada46164f5909fed966e819d2ec9eb96dc487aed57"
TAIL_SIZE = 78_251
TAIL_SHA256 = "878315d83c1f3d24399e768d4ee71d031e5bc930f08a7261b44a22b29a2e57b8"
LOAD_IMAGE_SIZE = 201_542
LOAD_IMAGE_SHA256 = "3df021b0416039df3b48b03362d223f5f358fd951d18316845624fd304030d8a"
RELOCATION_COUNT = 2_431
PKLITE_WORD = 0x310C


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def u16(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset:offset + 2], "little")


def mz_extent(data: bytes) -> int:
    if len(data) < 0x1E or data[:2] != b"MZ":
        raise ValueError("input is not a complete MZ executable")
    last = u16(data, 2)
    pages = u16(data, 4)
    if pages == 0 or last > 512:
        raise ValueError("invalid MZ page-count fields")
    return (pages - 1) * 512 + (last or 512)


def first_difference(actual: bytes, expected: bytes) -> int | None:
    shared = min(len(actual), len(expected))
    for offset in range(shared):
        if actual[offset] != expected[offset]:
            return offset
    return None if len(actual) == len(expected) else shared


def is_within(path: Path, parent: Path) -> bool:
    resolved_parent = parent.resolve()
    return path == resolved_parent or resolved_parent in path.parents


def fail(message: str) -> None:
    raise RuntimeError(message)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("unpacked_exe", type=Path)
    parser.add_argument("output_exe", type=Path)
    parser.add_argument("--pklite", type=Path, default=DEFAULT_PKLITE,
                        help="PKLITE 1.12 Professional PKLITE.EXE")
    parser.add_argument("--runner", type=Path, default=DEFAULT_RUNNER,
                        help="MS-DOS Player executable")
    parser.add_argument("--timeout", type=int, default=120,
                        help="DOS runner timeout in seconds")
    args = parser.parse_args()

    input_path = args.unpacked_exe.resolve()
    output_path = args.output_exe.resolve()
    if input_path == output_path or output_path == ORIGINAL.resolve():
        fail("refusing to overwrite the input or immutable original asset")
    if is_within(output_path, ROOT / "assets") or is_within(output_path, ROOT / "reference"):
        fail("refusing to write under immutable assets/ or derived reference/")
    if not args.pklite.is_file():
        fail(f"PKLITE executable not found: {args.pklite}; provide 1.12 Professional")
    if not args.runner.is_file():
        fail(f"DOS runner not found: {args.runner}")

    input_data = input_path.read_bytes()
    input_extent = mz_extent(input_data)
    if input_extent > len(input_data):
        fail("input MZ extent exceeds the input file")
    mz_input = input_data[:input_extent]
    header_size = u16(mz_input, 8) * 16
    reloc_count = u16(mz_input, 6)
    reloc_offset = u16(mz_input, 0x18)
    if header_size > len(mz_input) or reloc_offset + reloc_count * 4 > header_size:
        fail("input MZ header does not contain its declared relocation table")
    if reloc_count != RELOCATION_COUNT:
        fail(f"expected {RELOCATION_COUNT} relocations, found {reloc_count}")
    actual_sites = []
    for index in range(reloc_count):
        offset = reloc_offset + index * 4
        reloc_offset_in_segment = u16(mz_input, offset)
        segment = u16(mz_input, offset + 2)
        site = segment * 16 + reloc_offset_in_segment
        if site + 2 > len(mz_input) - header_size:
            fail(f"relocation {index} points outside the load image")
        actual_sites.append(site)
    oracle = json.loads(ORACLE.read_text(encoding="utf-8"))
    expected_sites = [entry["segment"] * 16 + entry["offset"]
                      for entry in oracle["relocations"]["ordered_segment_offset"]]
    if sorted(actual_sites) != sorted(expected_sites):
        fail("input relocation sites do not match the recovered relocation multiset")

    image = mz_input[header_size:]
    if len(image) != LOAD_IMAGE_SIZE or sha256(image) != LOAD_IMAGE_SHA256:
        fail("input load image does not match the pinned DAZZLE load image")
    if not LOAD_IMAGE.is_file() or LOAD_IMAGE.read_bytes() != image:
        fail("input load image differs from reference/load_image.bin")

    original = ORIGINAL.read_bytes()
    if len(original) != ORIGINAL_SIZE or sha256(original) != ORIGINAL_SHA256:
        fail("pinned original distribution file failed its size/SHA-256 check")
    expected_mz = original[:PACKED_MZ_SIZE]
    tail = original[PACKED_MZ_SIZE:]
    if (sha256(expected_mz) != PACKED_MZ_SHA256 or len(tail) != TAIL_SIZE
            or sha256(tail) != TAIL_SHA256):
        fail("pinned original MZ/tail boundary failed its size/SHA-256 check")

    SCRATCH.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="pack-", dir=SCRATCH) as temporary:
        stage = Path(temporary)
        shutil.copyfile(args.pklite, stage / "PKLITE.EXE")
        (stage / "DAZZLE.EXE").write_bytes(mz_input)
        command = [str(args.runner.resolve()), "PKLITE.EXE", "-e", "-a",
                   "DAZZLE.EXE", "PACKED.EXE"]
        try:
            result = subprocess.run(
                command, cwd=stage, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, timeout=args.timeout, check=False,
            )
        except subprocess.TimeoutExpired:
            fail(f"DOS runner timed out after {args.timeout} seconds")
        log = result.stdout.decode("cp437", errors="replace")
        (SCRATCH / "last-pack.log").write_text(log, encoding="utf-8")
        if result.returncode != 0:
            print(log, end="" if log.endswith("\n") else "\n")
            fail(f"PKLITE returned {result.returncode}; -e requires the 1.12 Professional build")
        packed_path = stage / "PACKED.EXE"
        if not packed_path.is_file():
            fail("PKLITE returned success but did not create PACKED.EXE")
        packed = packed_path.read_bytes()

    if len(packed) < 0x1E or u16(packed, 0x1C) != PKLITE_WORD:
        found = f"0x{u16(packed, 0x1C):04x}" if len(packed) >= 0x1E else "missing"
        fail(f"output PKLITE word is {found}, expected 0x{PKLITE_WORD:04x} (1.12 -e large)")
    packed_extent = mz_extent(packed)
    if packed_extent != len(packed):
        fail(f"PKLITE output has an unexpected overlay: file={len(packed)} MZ={packed_extent}")
    offset = first_difference(packed, expected_mz)
    if offset is not None:
        expected_byte = f"0x{expected_mz[offset]:02x}" if offset < len(expected_mz) else "EOF"
        actual_byte = f"0x{packed[offset]:02x}" if offset < len(packed) else "EOF"
        fail(f"packed MZ differs at 0x{offset:x}: candidate={actual_byte}, oracle={expected_byte}; "
             f"candidate size={len(packed)}, oracle size={len(expected_mz)}")
    if sha256(packed) != PACKED_MZ_SHA256:
        fail("packed MZ SHA-256 differs despite byte comparison")

    final = packed + tail
    if final != original:
        fail("full output differs after appending the separately preserved original resource tail")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(final)
    print(f"EXACT: {output_path} ({len(final)} bytes, SHA-256 {sha256(final)})")
    print(f"Appended resource from original distribution: {len(tail)} bytes, SHA-256 {sha256(tail)}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, RuntimeError) as exc:
        print(f"pack.py: {exc}", file=sys.stderr)
        raise SystemExit(2)
