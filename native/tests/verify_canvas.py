"""Whole-program state and production nearest-output oracle; no recorded goldens."""
from pathlib import Path
import json
import os
import re
import subprocess
import sys


def main():
    exe, scratch = (Path(p).resolve() for p in sys.argv[1:3])
    scratch.mkdir(parents=True, exist_ok=True)
    quick = "--quick" in sys.argv
    sizes = ("641x479",) if quick else ("1920x1080", "2560x1440", "3840x2160", "1366x768", "641x479")
    policies = ("original",) if quick else ("original", "crop", "stretch")
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    if env.get("DAZZLE_CANVAS_ORACLE_BACKEND", "software") != "software":
        env.pop("SDL_VIDEODRIVER")
    receipts = []
    for mode, options in (("vga400", []), ("vga200", ["/T"]), ("ega350", ["/E", "/T"]), ("ega700", ["/E"])):
        for scenario in ("auto", "keys"):
            times = ("4", "8", "12", "18", "25", "35", "50", "60") if scenario == "auto" else ("8.2", "8.5", "9.5", "10.2", "13.8", "19.5", "24.5", "28")
            script = scratch / "canvas.keys"
            # Explicit clears interrupt Quilt before the next image selection.
            # Auto above exercises hidden-page drawing and pan/split.
            script.write_text("3 n\n3.2 ALT+F9\n8 ALT+K\n8.01 F10\n9 q\n10.5 n\n13.2 ALT+A\n13.21 F10\n14 n\n18 ALT+0\n18.01 F10\n24 y\n")
            base = [str(exe), "--headless", "--deterministic", "--seed", "1", "--original", "absent.exe", "--exit-after", str(float(times[-1]) + .1), *options]
            if scenario == "keys":
                base += ["--script", str(script)]
            baseline = []
            for canvas, size, policy in [("classic", sizes[0], policies[0])] + [("screen", s, p) for s in sizes for p in policies]:
                paths = [scratch / f"{mode}-{scenario}-{canvas}-{size}-{policy}-{t}.state" for t in times]
                args = base + ["--canvas", canvas, "--window", size, "--present", policy]
                for t, path in zip(times, paths):
                    args += ["--state", t, str(path)]
                run = subprocess.run(args, cwd=scratch, env=env, capture_output=True, timeout=180)
                assert run.returncode == 0, (args, run.returncode, run.stdout[-1000:], run.stderr)
                states = [p.read_bytes() for p in paths]
                if canvas == "classic":
                    baseline = states
                else:
                    assert baseline == states, (mode, scenario, size, policy, "logical state differs")
                    count = run.stdout.count(b"canvas oracle ns=")
                    assert count == len(times), (mode, scenario, count, run.stdout[-1000:])
                    headers = [dict(line.split(b"=", 1) for line in s.split(b"END\n")[0].splitlines()[1:]) for s in states]
                    if scenario == "auto":
                        assert any(h[b"fade"] != b"0" for h in headers)
                    else:
                        images = {int(h[b"image"].split(b",")[0]) for h in headers}
                        assert {27, 20, 9} <= images, (mode, images)
                        assert {ord("q"), ord("y")} <= {int(h[b"fade"]) for h in headers}, (mode, headers)
                    scanout = [(int(a), int(b)) for a, b in re.findall(rb"start=(\d+) split=(\d+)", run.stdout)]
                    if mode == "vga400" and scenario == "auto":
                        assert any(a > 0 for a, b in scanout) and any(b < 399 for a, b in scanout), scanout
                    receipts.append(dict(mode=mode, scenario=scenario, size=size, policy=policy, comparisons=count, scanout=scanout,
                                         canvas_bytes=[int(n) for n in re.findall(rb"bytes=(\d+)", run.stdout)],
                                         images=[h[b"image"].decode() for h in headers], fades=[int(h[b"fade"]) for h in headers]))
                    print(mode, scenario, size, policy, count, "state/pixels equal", flush=True)
    (scratch / "equivalence.json").write_text(json.dumps(receipts, indent=2) + "\n")


if __name__ == "__main__":
    main()
