"""Full-state Classic/Hybrid comparisons at identical virtual yields; no goldens."""
from pathlib import Path
import json
import os
import subprocess
import sys


def main():
    exe = Path(sys.argv[1]).resolve()
    scratch = Path(sys.argv[2]).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    receipts = []
    # Real observer allocations/presentation, with a small dummy output for speed.
    for mode, options in (("vga400", []), ("vga200", ["/T"]),
                          ("ega700", ["/E"]), ("ega350", ["/E", "/T"])):
        for name, key in (("cycloids", "0"), ("sine", "K"), ("sweep2", "B"), ("lotsadots", "A")):
            script = scratch / "run.keys"
            script.write_text(f"3 n\n3.2 ALT+F9\n8 ALT+{key}\n8.01 F10\n")
            times = ("8.2", "8.25", "8.5", "9", "10")
            outputs = []
            for quality in ("classic", "hybrid"):
                paths = [scratch / f"{mode}-{name}-{quality}-{t}.state" for t in times]
                args = [str(exe), "--quality", quality, "--window", "320x240", "--deterministic",
                        "--seed", "1", "--original", "absent.exe", "--script", str(script),
                        "--exit-after", "10.1", *options]
                for t, path in zip(times, paths):
                    args.extend(("--state", t, str(path)))
                result = subprocess.run(args, cwd=scratch, env=env, capture_output=True, timeout=60)
                assert result.returncode == 0, (args, result.stdout, result.stderr)
                outputs.append([p.read_bytes() for p in paths])
            assert outputs[0] == outputs[1], (mode, name, "logical state differs")
            assert len({data.split(b"END\n", 1)[1][16:16 + 524288] for data in outputs[0]}) > 1, (
                mode, name, "fixture did not progressively write VRAM")
            expected_image = {"cycloids": 9, "sine": 27, "sweep2": 33, "lotsadots": 20}[name]
            assert f"image={expected_image},".encode() in outputs[0][-1].split(b"END\n")[0], (mode, name)
            receipts.append(dict(mode=mode, image=name, yields=len(times), identical=True))
            print(mode, name, "full state identical at", ", ".join(times), flush=True)
    # Whole original Auto loop, overlays and transitions. Long enough for fades;
    # real observer is active (not --headless), canonical receipts are untouched.
    auto = []
    times = ("4", "8", "12", "18", "25", "35", "50", "60")
    for quality in ("classic", "hybrid"):
        args = [str(exe), "--quality", quality, "--window", "320x240", "--deterministic",
                "--seed", "1", "--original", "absent.exe", "--exit-after", "60.1"]
        paths = [scratch / f"auto-{quality}-{t}.state" for t in times]
        for t, path in zip(times, paths):
            args.extend(("--state", t, str(path)))
        result = subprocess.run(args, cwd=scratch, env=env, capture_output=True, timeout=180)
        assert result.returncode == 0, (result.stdout, result.stderr)
        auto.append([p.read_bytes() for p in paths])
    assert auto[0] == auto[1], "Auto logical state differs"
    fades = [dict(line.split(b"=", 1) for line in data.split(b"END\n")[0].splitlines()[1:])[b"fade"]
             for data in auto[0]]
    assert any(f != b"0" for f in fades), fades
    receipts.append(dict(mode="Auto", yields=len(times), identical=True, fades=[int(f) for f in fades]))
    (scratch / "equivalence.json").write_text(json.dumps(receipts, indent=2) + "\n")
    print("Auto with fades: full state identical at", ", ".join(times), flush=True)


if __name__ == "__main__":
    main()
