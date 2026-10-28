#!/usr/bin/env python3
"""Build a conservative segment map from the pinned DAZZLE load image."""

from __future__ import annotations

import hashlib
import json
import re
import struct
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
IMAGE_PATH = ROOT / "reference" / "load_image.bin"
ORACLE_PATH = ROOT / "evidence" / "oracle.json"
OUTPUT_PATH = ROOT / "evidence" / "segments.json"

SEGMENT_HINTS = {
    0x0000: ("code", "Unpacked entry is CS:IP 0000:0000; start bytes are a 16-bit startup sequence."),
    0x066A: ("code", "Starts with push CS/call/stack cleanup/RETF code."),
    0x0B84: ("code", "Starts with MOV AX,CS; RETF followed by a BP-frame routine."),
    0x0CCD: ("data", "Starts with a table of 16-bit values before a far-return routine."),
    0x11C1: ("code", "Starts with a function epilogue and BP-frame routine."),
    0x158D: ("code", "Starts with a function epilogue and BP-frame routine."),
    0x17F5: ("code", "Starts with a short jump and function epilogue."),
    0x1899: ("code", "Starts with push CS and a far-return function epilogue."),
    0x19F0: ("code", "Starts with RETF stubs and MOV AX,CS/RETF sequences."),
    0x1A72: ("code", "Starts with function epilogue instructions."),
    0x1BAC: ("data", "Starts with a monotonic 16-bit lookup table (step 0x0c)."),
    0x1E24: ("code", "A startup far call targets 1E24:0430; region ends at the zero-filled 1E81 target."),
    0x1E81: ("bss/data", "Segment-target value has 211 references; its first 0x100 bytes are zero."),
    0x29BF: ("data", "Startup loads this paragraph into DS; Borland copyright string begins at byte offset 0x29bf4."),
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def word(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def hexn(value: int | None) -> str | None:
    return None if value is None else f"0x{value:x}"


def main() -> None:
    image = IMAGE_PATH.read_bytes()
    oracle = json.loads(ORACLE_PATH.read_text(encoding="utf-8"))
    if len(image) != oracle["load_image"]["size"] or sha256(image) != oracle["load_image"]["sha256"]:
        raise SystemExit("load_image.bin does not match evidence/oracle.json")

    relocations = oracle["relocations"]["ordered_segment_offset"]
    target_counts: Counter[int] = Counter()
    for relocation in relocations:
        site = relocation["segment"] * 16 + relocation["offset"]
        target_counts[word(image, site)] += 1

    first = image[:3]
    if first[:1] != b"\xBA" or (0, 1) not in {
        (r["segment"], r["offset"]) for r in relocations
    }:
        raise SystemExit("startup does not match the expected MOV DX,seg DGROUP form")
    dgroup = word(image, 1)
    dgroup_offset = dgroup * 16
    if dgroup_offset >= len(image):
        raise SystemExit("startup DGROUP paragraph lies outside the load image")

    candidates = sorted(target_counts)
    segments = []
    for index, paragraph in enumerate(candidates):
        start = paragraph * 16
        next_start = candidates[index + 1] * 16 if index + 1 < len(candidates) else len(image)
        kind, basis = SEGMENT_HINTS.get(
            paragraph,
            ("unknown", "Relocation target paragraph; no direct startup/disassembly classification."),
        )
        in_image = start < len(image)
        segments.append({
            "paragraph_base": paragraph,
            "paragraph_base_hex": hexn(paragraph),
            "byte_offset": start,
            "byte_offset_hex": hexn(start),
            "size_estimate": max(0, next_start - start) if in_image else None,
            "size_estimate_hex": hexn(max(0, next_start - start)) if in_image else None,
            "kind_guess": "DGROUP" if paragraph == dgroup else kind,
            "relocation_target_count": target_counts[paragraph],
            "basis": basis,
            "boundary_caveat": "Size is the interval to the next distinct relocation target paragraph, not a proven linker segment extent.",
            "start_bytes_hex": image[start:start + 16].hex(" ") if in_image else None,
        })

    dgroup_size = len(image) - dgroup_offset
    initialized_data = {
        "paragraph_base": dgroup,
        "byte_offset": dgroup_offset,
        "byte_offset_hex": hexn(dgroup_offset),
        "size_estimate": dgroup_size,
        "size_estimate_hex": hexn(dgroup_size),
        "basis": "DGROUP occupies the load-image suffix from the startup segment immediate to the end of the decompressed image; strings and zero-terminated data occur in this range.",
        "caveat": "This is an initialized load-image span, not a recovered linker .DATA size; separate code and data targets occur earlier in the image.",
    }

    code_end = 0x1E81 * 16
    stack = oracle["stack"]
    stack_offset = stack["ss"] * 16
    image_end = len(image)
    patterns = re.compile(rb"[\x20-\x7e]{5,}")
    strings = []
    for match in patterns.finditer(image):
        value = match.group().decode("latin-1")
        if "borland" in value.lower() or ("copyright" in value.lower() and "borland" in value.lower()):
            strings.append({"offset": match.start(), "offset_hex": hexn(match.start()), "text": value})

    result = {
        "schema": 1,
        "load_image": {
            "size": len(image),
            "size_hex": hexn(len(image)),
            "sha256": sha256(image),
        },
        "startup": {
            "offset": 0,
            "bytes_hex": first.hex(" "),
            "instruction_bytes": "BA iw",
            "interpretation": "MOV DX,imm16; the immediate word at image offset 1 has an MZ relocation and is the DGROUP paragraph.",
            "dgroup_paragraph": dgroup,
            "dgroup_paragraph_hex": hexn(dgroup),
            "dgroup_byte_offset": dgroup_offset,
            "dgroup_byte_offset_hex": hexn(dgroup_offset),
        },
        "segments": segments,
        "dgroup_initialized_data_estimate": initialized_data,
        "code_end_estimate": {
            "byte_offset": code_end,
            "byte_offset_hex": hexn(code_end),
            "basis": "Last code-classified relocation-target paragraph is 0x1e24; the next target 0x1e81 begins with a zero-filled data region.",
            "caveat": "An upper boundary estimate from relocation targets and bytes, not a fully recovered linker map.",
        },
        "stack": {
            "paragraph_base": stack["ss"],
            "paragraph_base_hex": hexn(stack["ss"]),
            "byte_offset": stack_offset,
            "byte_offset_hex": hexn(stack_offset),
            "entry_sp": stack["sp"],
            "entry_sp_hex": hexn(stack["sp"]),
            "offset_after_image_end": stack_offset - image_end,
            "minimum_entry_stack_window_bytes": stack["sp"],
            "basis": "PKLITE footer; the stack base is beyond the initialized load image.",
            "caveat": "The original stack allocation extent is not encoded in the footer; minalloc remains the packed MZ value in oracle.json.",
        },
        "relocation_target_histogram": [
            {"paragraph": p, "paragraph_hex": hexn(p), "count": n}
            for p, n in sorted(target_counts.items())
        ],
        "copyright_strings": strings,
    }
    OUTPUT_PATH.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"DGROUP: {dgroup:04x} ({dgroup_offset:#x}), initialized span estimate {dgroup_size} bytes")
    print(f"code end estimate: {code_end:#x}; target paragraphs: {len(candidates)}")
    print(f"copyright strings: {len(strings)}; wrote {OUTPUT_PATH.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
