#!/usr/bin/env python3
"""Regenerate DOS diagnostics; compare full 14-byte image records on demand.

No expected output is obtained from a native build. The cache fingerprint covers
the DOS driver, shared model headers, reconstructed modules, input and harness.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import re

ROOT = Path(__file__).resolve().parents[3]
REF = ROOT / "native/tests/ref"
BUILD = ROOT / "build/workers/n-diff"
RUNNER = "C:/tools/msdos/binary/i386_x64/msdos.exe"


def recipe(kind):
    if kind == "fades":
        return ("fade_driver.c", ["native/tests/ref/fade_stubs.c", "src/dzle.c", "src/dzl2.c", "src/dzfd.c"],
                ["dzle.c:main=dzle_diagnostic_main", "dzle.c:rand=ref_rand", "dzle.c:srand=ref_srand",
                 "dzl2.c:rand=ref_rand", "dzfd.c:rand=ref_rand"], None, "FADE.TXT")
    if kind == "gates":
        return ("gate_driver.c", ["src/dzle.c", "src/dzl2.c"],
                ["dzle.c:main=dzle_gate_unused_main", "dzle.c:rand=ref_rand", "dzle.c:srand=ref_srand",
                 "dzl2.c:rand=ref_rand"], None, "GATES.TXT")
    if kind == "pcx":
        return "pcx_driver.c", ["src/dazpcx.c"], [], None, "PCX.TXT"
    if kind == "images":
        modules = ["src/dzle.c", "src/dzl2.c"]
        defines = ["dzle.c:main=dzle_diagnostic_main", "dzle.c:rand=ref_rand",
                   "dzle.c:srand=ref_srand", "dzl2.c:rand=ref_rand"]
        return "images_driver.c", modules, defines, "IMAGES.TSV", "IMAGE.TXT"
    if kind == "palettes":
        return ("palette_driver.c", ["native/tests/ref/palette_stubs.c", "src/dzle.c", "src/dosi.c"],
                ["dzle.c:main=dzle_palette_unused_main", "dzle.c:rand=ref_rand", "dzle.c:srand=ref_srand",
                 "dosi.c:rand=ref_rand", "dosi.c:srand=ref_srand"], "PALETTES.TSV", "PALETTE.TXT")
    if kind == "primitives":
        return ("primitives_driver.c", ["src/dzle.c", "src/dzl2.c"],
                ["dzle.c:main=dzle_diagnostic_main", "dzle.c:rand=ref_rand", "dzle.c:srand=ref_srand",
                 "dzl2.c:rand=ref_rand"], None, "PRIMS.TXT")
    raise ValueError(kind)


def capture(kind, force=False):
    driver, modules, defines, scenario, product = recipe(kind)
    inputs = [REF / driver, Path(__file__), ROOT / "tools/refharness.py", ROOT / "tools/bcc.py",
              ROOT / "evidence/toolchains.json", *map(lambda p: ROOT / p, modules),
              *sorted((ROOT / "include").glob("*.h"))]
    # Hash only the driver's actual quoted model includes, not native-only QA
    # helpers or other independent drivers' headers.
    pending = [REF / driver]
    while pending:
        source = pending.pop()
        for name in re.findall(r'^\s*#include\s+"([^"]+)"', source.read_text(), re.M):
            path = source.parent / name
            if path.is_file() and path not in inputs:
                inputs.append(path); pending.append(path)
    if scenario:
        inputs.append(REF / "scenarios" / scenario.lower())
    fingerprint = hashlib.sha256()
    for path in inputs:
        fingerprint.update(path.relative_to(ROOT).as_posix().encode())
        fingerprint.update(path.read_bytes())
    digest = fingerprint.hexdigest()
    receipt = BUILD / (kind + "-capture.json")
    if not force and receipt.exists():
        cached = json.loads(receipt.read_text())
        run = ROOT / cached["run_dir"]
        if cached["fingerprint"] == digest and (run / product).exists() and (
                kind != "images" or (run / "PLOT.BIN").exists()):
            return run, product
    # TLINK's DOS cwd buffer fails at long paths with a misleading OOM error.
    name = "nd-" + kind[:4]
    if kind == "images":
        # The real grid checks the assembly entry's literal offset 012Dh. A
        # logging callback cannot have that address. Adapt only this kernel
        # identity probe in a diagnostic copy; record that copy in the receipt.
        source = (ROOT / "src/dzle.c").read_text()
        old = "FP_SEG(plot_callback) == FP_SEG(rgb8_to_ega_palette) && FP_OFF(plot_callback) == 0x012d"
        assert source.count(old) == 2
        model = BUILD / "dzle.c"
        BUILD.mkdir(parents=True, exist_ok=True)
        model.write_text(source.replace(old, "plot_callback == ega_plot_symmetric_640x700"), newline="\n")
        modules = [str(model) if module == "src/dzle.c" else module for module in modules]
    command = [sys.executable, "-B", "-I", str(ROOT / "tools/refharness.py"), str(REF / driver),
               *modules, "--name", name, "--runner", RUNNER, "--timeout", "900", "--fresh"]
    for define in defines:
        command += ["--define", define]
    if scenario:
        command += ["--input", str(REF / "scenarios" / scenario.lower())]
    subprocess.run(command, cwd=ROOT, check=True)
    run = max((ROOT / "build/refharness" / name).glob("r_*"), key=lambda p: p.stat().st_mtime_ns)
    report = json.loads((run / "report.json").read_text())
    assert report["status"] == "PASS"
    BUILD.mkdir(parents=True, exist_ok=True)
    receipt.write_text(json.dumps({"fingerprint": digest, "run_dir": run.relative_to(ROOT).as_posix()}, indent=2) + "\n")
    return run, product


def compare(dos, native, kind):
    record = struct.Struct("<HhhhhI")
    scenarios = ([s.split() for s in (REF / "scenarios/images.tsv").read_text().splitlines()
                  if s.strip() and not s.startswith("#")] if kind == "images" else
                 [["VGA" if i < 8 else "EGA", ("wipe_clear", "wipe_columns", "exclusive_or", "moving_bars",
                   "swap0", "swap1", "swap2", "swap3")[i % 8]] for i in range(16)])
    per_case = 0
    previous = None
    with dos.open("rb") as a, native.open("rb") as b:
        index = 0
        while True:
            x, y = a.read(record.size), b.read(record.size)
            if x:
                case = record.unpack(x)[0]
                per_case = per_case + 1 if case == previous else 0
                previous = case
            if x != y:
                def describe(data):
                    if not data:
                        return "<end of stream>"
                    case, px, py, color, symmetry, rng = record.unpack(data)
                    return f"case={case} ({' '.join(scenarios[case])}) x={px} y={py} colour={color & 65535} symmetry={symmetry} rand_calls={rng}"
                print(f"FIRST DIVERGENT PLOT global_index={index}, case_plot_index={per_case}")
                print(" DOS: " + describe(x))
                print("host: " + describe(y))
                return 1
            if not x:
                print(f"all {index} full plot records equal; mismatch is in non-stream state")
                return 0
            index += 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("kind", choices=["images", "palettes", "primitives", "pcx", "gates", "fades"])
    parser.add_argument("--update", action="store_true", help="replace compact checked-in DOS output")
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--compare", type=Path)
    args = parser.parse_args()
    run, product = capture(args.kind, args.force)
    print("DOS capture: " + str(run))
    if args.update:
        shutil.copyfile(run / product, REF / "data" / (args.kind + ".txt"))
    if args.compare:
        return compare(run / "PLOT.BIN", args.compare, args.kind)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
