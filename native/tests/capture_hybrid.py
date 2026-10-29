"""Repeatable developer captures at an exact virtual deadline (not frame count)."""
from pathlib import Path
import json
import os
import subprocess
import struct
import sys


def main():
    exe = Path(sys.argv[1]).resolve()
    scratch = Path(sys.argv[2]).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    captures = []
    for size in ("1920x1080", "2560x1440"):
        for image, keys, time in (
                ("cycloids", "8 ALT+0\n8.01 F10\n", "8.15"),
                ("sine", "8 ALT+K\n8.01 F10\n", "8.15"),
                ("sweep2", "8 ALT+B\n8.01 F10\n", "9"),
                ("mixed", "8 ALT+K\n8.01 F10\n8.1 ALT+A\n8.11 F10\n", "8.15"),
                ("retired", "8 ALT+K\n8.01 F10\n8.2 q\n9 n\n", "12")):
            script = scratch / f"{image}.keys"
            script.write_text("3 n\n3.2 ALT+F9\n" + keys)
            states = []
            for quality in ("classic", "hybrid"):
                png = scratch / f"{image}-{size}-{quality}.png"
                state = png.with_suffix(".state")
                args = [str(exe), "--deterministic", "--seed", "1", "--quality", quality,
                        "--window", size, "--original", "absent.exe", "--script", str(script),
                        "--exit-after", time, "--state", time, str(state), "--screenshot", str(png)]
                result = subprocess.run(args, cwd=scratch, env=env, capture_output=True, timeout=180)
                assert result.returncode == 0, (args, result.stdout, result.stderr)
                assert struct.unpack(">II", png.read_bytes()[16:24]) == tuple(map(int, size.split("x")))
                states.append(state.read_bytes())
                captures.append(dict(image=image, size=size, quality=quality, seed=1,
                                     virtual_seconds=time, policy="original", file=png.name,
                                     renderer="SDL software / dummy"))
                print(png.name, flush=True)
            assert states[0] == states[1], (image, size, "logical state differs")
    (scratch / "captures.json").write_text(json.dumps(captures, indent=2) + "\n")


if __name__ == "__main__":
    main()
