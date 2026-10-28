#!/usr/bin/env python3
"""Decode the pinned DAZZLE.EXE PKLITE 1.12 image and preserve its tail."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ASSET = ROOT / "assets" / "DAZZLE.EXE"
PACKED_SHA256 = "fc68905570e0f139cafc0bc0f75af87445c6fb29f53f5f02018fa9b71f714ee8"
TABLE_OFFSET = 0x2B2
STREAM_OFFSET = 0x2F0
MZ_HEADER_WORDS = (
    "magic", "bytes_in_last_page", "pages", "relocation_count",
    "header_paragraphs", "min_alloc", "max_alloc", "ss", "sp",
    "checksum", "ip", "cs", "relocation_table_offset", "overlay",
)


def sha256(data: bytes | bytearray) -> str:
    return hashlib.sha256(data).hexdigest()


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


class BitStream:
    """PKLITE's little-endian, low-bit-first word buffer."""

    def __init__(self, data: bytes, offset: int):
        self.data = data
        self.pos = offset
        self.bits = self.word()
        self.left = 16

    def byte(self) -> int:
        if self.pos >= len(self.data):
            raise ValueError("truncated packed stream")
        value = self.data[self.pos]
        self.pos += 1
        return value

    def word(self) -> int:
        return self.byte() | (self.byte() << 8)

    def bit(self) -> int:
        value = self.bits & 1
        self.bits >>= 1
        self.left -= 1
        # The stub prefetches the next word immediately when the current one
        # runs out, including when the next stream item is a literal byte.
        if self.left == 0:
            self.bits = self.word()
            self.left = 16
        return value


def parse_header(data: bytes) -> dict[str, int]:
    words = struct.unpack_from("<14H", data)
    return dict(zip(MZ_HEADER_WORDS, words))


def mz_file_extent(header: dict[str, int]) -> int:
    pages = header["pages"]
    last = header["bytes_in_last_page"] or 512
    if pages == 0 or last > 512:
        raise ValueError("invalid MZ page count")
    return (pages - 1) * 512 + last


def decode(data: bytes) -> tuple[bytes, list[tuple[int, int]], dict]:
    if sha256(data) != PACKED_SHA256:
        raise ValueError("input SHA-256 does not match the pinned DAZZLE.EXE")
    if len(data) < STREAM_OFFSET or data[:2] != b"MZ":
        raise ValueError("input is not the pinned packed MZ executable")

    header = parse_header(data)
    packed_mz_end = mz_file_extent(header)
    if packed_mz_end <= STREAM_OFFSET:
        raise ValueError("packed MZ extent does not contain the decompressor")
    if packed_mz_end > len(data):
        raise ValueError("packed MZ extent exceeds the file")

    # The copied decompressor starts at packed file offset 0xA4. Its CS-based
    # table references at 0x020E, 0x0219, 0x0224 and 0x0234 therefore resolve
    # to 0x2B2, 0x2BD, 0x2C8 and 0x2D8 in the packed file.
    table = data[TABLE_OFFSET:TABLE_OFFSET + 0x3E]
    if len(table) != 0x3E:
        raise ValueError("truncated PKLITE length/distance tables")
    stream = BitStream(data, STREAM_OFFSET)
    image = bytearray()
    tokens = {"literals": 0, "matches": 0, "segment_markers": 0}

    while True:
        if stream.bit() == 0:
            image.append(stream.byte() ^ stream.left)
            tokens["literals"] += 1
            continue

        code = (stream.bit() << 1) | stream.bit()
        if code < 2:
            code = (code << 1) | stream.bit()
            if code:
                code = (code << 1) | stream.bit()
                if code >= 5:
                    code = (code << 1) | stream.bit()
            if code <= 12:
                length = table[code]
            else:
                code = ((code & 3) << 1) | stream.bit()
                if code >= 5:
                    code = (code << 1) | stream.bit()
                if code <= 12:
                    length = table[11 + code]
                else:
                    code = ((code & 3) << 1) | stream.bit()
                    if code >= 5:
                        code = (code << 1) | stream.bit()
                    length = table[22 + code]
                if length == 25:
                    extra = stream.byte()
                    if extra == 0xFF:
                        break
                    if extra == 0xFE:
                        tokens["segment_markers"] += 1
                        continue
                    length += extra
        else:
            length = code

        high = 0
        if length != 2 and stream.bit() == 0:
            code = (stream.bit() << 2) | (stream.bit() << 1) | stream.bit()
            if code >= 2:
                code = (code << 1) | stream.bit()
                if code >= 8:
                    code = (code << 1) | stream.bit()
                    if code >= 23:
                        code = (code << 1) | stream.bit()
                        high = code & 0xDF
                    else:
                        high = table[38 + code]
                else:
                    high = table[38 + code]
            else:
                high = table[38 + code]

        distance = (high << 8) | stream.byte()
        if distance == 0 or distance > len(image) or length < 2:
            raise ValueError(
                f"invalid backreference at packed offset {stream.pos:#x}: "
                f"distance={distance:#x}, length={length}, output={len(image):#x}"
            )
        for _ in range(length):
            image.append(image[-distance])
        tokens["matches"] += 1
        if len(image) > 0x100000:
            raise ValueError("decompressed image exceeds 1 MiB bound")

    relocation_stream_offset = stream.pos
    relocations: list[tuple[int, int]] = []
    segment = 0
    relocation_groups = []
    while True:
        count_offset = stream.pos
        count = stream.word()
        if count == 0xFFFF:
            break
        if count == 0:
            relocation_groups.append({"segment": segment, "count": 0})
            continue
        relocation_groups.append({"segment": segment, "count": count})
        for _ in range(count):
            offset = stream.word()
            site = segment * 16 + offset
            if site + 2 > len(image):
                raise ValueError(
                    f"relocation {segment:04x}:{offset:04x} outside image"
                )
            relocations.append((segment, offset))
        segment += 0x0FFF

    footer_offset = stream.pos
    footer = {
        "ss": stream.word(), "sp": stream.word(),
        "cs": stream.word(), "ip": stream.word(),
    }
    if stream.pos != packed_mz_end:
        raise ValueError(
            f"stub consumed through {stream.pos:#x}, MZ extent is {packed_mz_end:#x}"
        )

    metadata = {
        "packed_mz_header": header,
        "packed_mz_extent": packed_mz_end,
        "relocation_stream_offset": relocation_stream_offset,
        "relocation_terminator_offset": footer_offset - 2,
        "footer_offset": footer_offset,
        "footer": footer,
        "tokens": tokens,
        "relocation_groups": relocation_groups,
        "consumed_bytes": stream.pos,
    }
    return bytes(image), relocations, metadata


def relocation_hash(items: list[tuple[int, int]]) -> str:
    packed = b"".join(struct.pack("<HH", segment, offset) for segment, offset in items)
    return sha256(packed)


def make_unpacked_exe(image: bytes, relocations: list[tuple[int, int]],
                      footer: dict[str, int], packed_header: dict[str, int],
                      tail: bytes) -> bytes:
    relocation_offset = 0x1C
    minimum_header_bytes = relocation_offset + 4 * len(relocations)
    header_paragraphs = (minimum_header_bytes + 15) // 16
    header_bytes = header_paragraphs * 16
    if header_paragraphs > 0xFFFF:
        raise ValueError("MZ header too large")

    mz_image_size = header_bytes + len(image)
    pages = (mz_image_size + 511) // 512
    last_page = mz_image_size % 512
    if pages > 0xFFFF:
        raise ValueError("unpacked MZ exceeds the page-count field")

    words = [
        0x5A4D, last_page, pages, len(relocations), header_paragraphs,
        packed_header["min_alloc"], packed_header["max_alloc"],
        footer["ss"], footer["sp"], 0, footer["ip"], footer["cs"],
        relocation_offset, 0,
    ]
    header = bytearray(header_bytes)
    struct.pack_into("<14H", header, 0, *words)
    for index, (segment, offset) in enumerate(relocations):
        struct.pack_into("<HH", header, relocation_offset + 4 * index,
                         offset, segment)
    return bytes(header) + image + tail


def crosscheck_result(image: bytes, relocations: list[tuple[int, int]],
                      footer: dict[str, int], dump_path: Path,
                      registers_path: Path) -> dict:
    dump = dump_path.read_bytes()
    snapshot = json.loads(registers_path.read_text(encoding="utf-8"))
    registers = snapshot["registers"]
    cs = int(registers["cs"], 16)
    ss = int(registers["ss"], 16)
    sp = int(registers["esp"], 16) & 0xFFFF
    ip = int(registers["eip"], 16) & 0xFFFF
    ax = int(registers["eax"], 16) & 0xFFFF
    load_segment = (cs - footer["cs"]) & 0xFFFF
    if cs != ((load_segment + footer["cs"]) & 0xFFFF):
        raise ValueError("runtime CS does not agree with the stub footer")
    if ss != ((load_segment + footer["ss"]) & 0xFFFF) or sp != footer["sp"]:
        raise ValueError("runtime stack does not agree with the stub footer")
    if ip != footer["ip"] or ax != 0:
        raise ValueError("runtime snapshot is not at the recovered entry with AX=0")

    expected = bytearray(image)
    for segment, offset in relocations:
        site = segment * 16 + offset
        value = u16(expected, site)
        struct.pack_into("<H", expected, site, (value + load_segment) & 0xFFFF)
    dump_address = int(snapshot["dump_address"], 16)
    if dump_address != ((cs << 4) + ax):
        raise ValueError("runtime dump address is not CS:AX at the entry point")

    compare_length = min(len(expected), len(dump))
    differences = [i for i in range(compare_length) if expected[i] != dump[i]]
    byte_equal = len(expected) == len(dump) and not differences
    if len(expected) != len(dump):
        differences.append(compare_length)
    return {
        "status": "byte-equal" if byte_equal else "differences",
        "byte_equal": byte_equal,
        "difference_count": len(differences),
        "first_difference_offsets": [f"0x{x:x}" for x in differences[:16]],
        "observed_load_segment": load_segment,
        "dump_address": f"0x{dump_address:08x}",
        "dump_size": len(dump),
        "expected_relocated_sha256": sha256(expected),
        "dump_sha256": sha256(dump),
        "entry_registers": registers,
        "register_snapshot": str(registers_path.relative_to(ROOT)),
        "dump_file": str(dump_path.relative_to(ROOT)),
        "runner": "DOSBox-X acceptance build v3, 2026.08.31",
        "runner_sha256": "e5b9a81acb208bab48cb01a7dac9691cd06674fac877abce54e4693ece6dbff9",
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", type=Path, default=ASSET)
    parser.add_argument("--crosscheck-dump", type=Path)
    parser.add_argument("--crosscheck-registers", type=Path)
    args = parser.parse_args()
    if bool(args.crosscheck_dump) != bool(args.crosscheck_registers):
        parser.error("--crosscheck-dump and --crosscheck-registers must be supplied together")
    packed = args.input.read_bytes()
    image, relocations, meta = decode(packed)

    packed_header = meta["packed_mz_header"]
    packed_end = meta["packed_mz_extent"]
    tail = packed[packed_end:]
    unpacked_exe = make_unpacked_exe(
        image, relocations, meta["footer"], packed_header, tail
    )
    unique_relocations = sorted(set(relocations))
    relocation_bytes = b"".join(
        struct.pack("<HH", segment, offset) for segment, offset in relocations
    )
    exe_header = parse_header(unpacked_exe)
    new_header_bytes = exe_header["header_paragraphs"] * 16
    crosscheck = {
        "status": "not-run",
        "method": "DOSBox-X acceptance-build on_exec dump at the recovered unpacked entry point",
        "result": None,
    }
    if args.crosscheck_dump:
        crosscheck = {
            "method": "DOSBox-X acceptance-build on_exec dump at the recovered unpacked entry point",
            "result": crosscheck_result(
                image, relocations, meta["footer"],
                args.crosscheck_dump.resolve(), args.crosscheck_registers.resolve(),
            ),
        }
        crosscheck["status"] = crosscheck["result"]["status"]

    oracle = {
        "schema": 1,
        "packed_file": {
            "path": "assets/DAZZLE.EXE",
            "size": len(packed),
            "sha256": sha256(packed),
        },
        "pklite": {
            "version": "1.12",
            "version_evidence": "packed word at 0x1c is 0x310c; sibling PKLITE 1.03 sample uses 0x3103",
            "extra": True,
            "extra_evidence": "stub XORs each literal with the remaining control-bit count (DL)",
            "large": True,
            "large_evidence": "stub has segment rollover markers and segmented copy/relocation loops; decoded load image exceeds 64 KiB",
            "ident_word_offset": "0x1c",
            "ident_word": "0x310c",
        },
        "decoder": {
            "table_offset": "0x2b2",
            "stream_offset": "0x2f0",
            "table_origin_evidence": "stub is copied from file offset 0x0a4 to CS:0000; CS lookups at 0x020e/0x0219/0x0224/0x0234 map to packed offsets 0x2b2/0x2bd/0x2c8/0x2d8",
            "packed_mz_extent": meta["packed_mz_extent"],
            "relocation_stream_offset": meta["relocation_stream_offset"],
            "footer_offset": meta["footer_offset"],
            "consumed_bytes": meta["consumed_bytes"],
            "token_counts": meta["tokens"],
            "relocation_groups": meta["relocation_groups"],
        },
        "load_image": {
            "size": len(image),
            "sha256": sha256(image),
        },
        "entry": {"cs": meta["footer"]["cs"], "ip": meta["footer"]["ip"]},
        "stack": {"ss": meta["footer"]["ss"], "sp": meta["footer"]["sp"]},
        "recovered_mz_header": {
            "header_paragraphs": exe_header["header_paragraphs"],
            "header_size": new_header_bytes,
            "relocation_count": len(relocations),
            "relocation_table_offset": "0x1c",
            "min_alloc": packed_header["min_alloc"],
            "min_alloc_source": "retained from packed MZ header; original pre-PKLITE value is not separately encoded in the footer",
            "max_alloc": packed_header["max_alloc"],
            "max_alloc_source": "retained from packed MZ header; original pre-PKLITE value is not separately encoded in the footer",
            "bytes_in_last_page": exe_header["bytes_in_last_page"],
            "pages": exe_header["pages"],
        },
        "relocations": {
            "count": len(relocations),
            "ordered_segment_offset": [
                {"segment": segment, "offset": offset}
                for segment, offset in relocations
            ],
            "ordered_sha256": relocation_hash(relocations),
            "ordered_encoding": "concatenated little-endian u16 segment, u16 offset pairs in stream order",
            "unique_count": len(unique_relocations),
            "sorted_set_sha256": relocation_hash(unique_relocations),
            "sorted_set_encoding": "unique (segment, offset) pairs sorted numerically, then little-endian u16 segment, u16 offset",
            "duplicates": len(relocations) - len(unique_relocations),
        },
        "tail": {
            "offset": packed_end,
            "size": len(tail),
            "sha256": sha256(tail),
        },
        "unpacked_exe": {
            "size": len(unpacked_exe),
            "mz_image_size": new_header_bytes + len(image),
            "sha256": sha256(unpacked_exe),
        },
        "crosscheck": crosscheck,
    }

    (ROOT / "reference").mkdir(exist_ok=True)
    (ROOT / "evidence").mkdir(exist_ok=True)
    (ROOT / "reference" / "load_image.bin").write_bytes(image)
    (ROOT / "reference" / "DAZZLE_U.EXE").write_bytes(unpacked_exe)
    (ROOT / "evidence" / "oracle.json").write_text(
        json.dumps(oracle, indent=2) + "\n", encoding="utf-8"
    )
    print(f"load image: {len(image)} bytes sha256={sha256(image)}")
    print(f"relocations: {len(relocations)} ordered; {len(unique_relocations)} unique")
    print(f"tail: {len(tail)} bytes at {packed_end:#x} sha256={sha256(tail)}")
    print(f"unpacked EXE: {len(unpacked_exe)} bytes sha256={sha256(unpacked_exe)}")
    print(f"entry {meta['footer']['cs']:04x}:{meta['footer']['ip']:04x}, "
          f"stack {meta['footer']['ss']:04x}:{meta['footer']['sp']:04x}")


if __name__ == "__main__":
    main()
