"""Full original-program state identity and per-effect review captures."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys


def main():
    exe, scratch = (Path(p).resolve() for p in sys.argv[1:3])
    scratch.mkdir(parents=True, exist_ok=True)
    quick = "--quick" in sys.argv
    captures = "--captures" in sys.argv
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    effects = {"sine": "k", "cycloids": "0", "sweep2": "b", "ellipse": "r",
               "arches": "1", "daisies": "q", "squirals": "c", "corkscrew": "8"}
    if "--effect" in sys.argv:
        name = sys.argv[sys.argv.index("--effect")+1]
        effects = {name: effects[name]}
    for effect, key in effects.items():
        verify_effect(exe, scratch, quick, captures, env, effect, key)


def verify_effect(exe, scratch, quick, captures, env, effect, key):
    script = scratch / f"{effect}.keys"
    script.write_text(f"3 n\n3.2 ALT+F9\n8 ALT+{key.upper()}\n8.01 F10\n9 q\n10.5 n\n13.2 ALT+{key.upper()}\n13.21 F10\n15 y\n19 n\n20 ALT+A\n20.01 F10\n")
    if captures:
        script.write_text(f"3 n\n3.2 ALT+F9\n8 n\n8.1 ALT+{key.upper()}\n8.11 F10\n")
    times = ("8.2", "8.5", "9.5", "10.2", "13.8", "15.5", "18.5", "21")
    records = []
    modes = (("vga400", []),) if quick or captures else (("vga400", []), ("vga200", ["/T"]), ("ega350", ["/E", "/T"]), ("ega700", ["/E"]))
    sizes = ("1920x1080", "3840x2160") if captures else (("641x479",) if quick else ("641x479", "1920x1080", "3840x2160"))
    for mode, options in modes:
        for size in sizes:
            baseline = None
            styles = [("faithful", ["--native", "none"]), ("crisp", ["--native", effect])]
            if not captures: styles.append(("aa", ["--native", "all", "--strokes", "aa"]))
            for style, args in styles:
                prefix = scratch / f"{effect}-{mode}-{size}-{style}"
                run_times = ("9.5",) if captures else times
                paths = [Path(f"{prefix}-{t}.state") for t in run_times]
                command = [str(exe), "--headless", "--deterministic", "--seed", sys.argv[sys.argv.index("--seed")+1] if "--seed" in sys.argv else "1", "--original", "absent.exe", "--canvas", "screen", "--window", size, "--script", str(script), "--exit-after", str(float(run_times[-1])+.01), *options, *args]
                for t, path in zip(run_times, paths):
                    command += ["--state", t, str(path)]
                run_env = dict(env, DAZZLE_NATIVE_STATE=str(prefix))
                if captures:
                    run_env["DAZZLE_NATIVE_CAPTURE"] = str(prefix)+".png"
                run = subprocess.run(command, cwd=scratch, env=run_env, capture_output=True, timeout=180)
                assert run.returncode == 0, (command, run.returncode, run.stdout[-2000:], run.stderr)
                state = [(p.read_bytes(), Path(f"{prefix}-{i}.globals").read_bytes()) for i, p in enumerate(paths)]
                if baseline is None:
                    baseline = state
                else:
                    for i, (a, b) in enumerate(zip(baseline, state)):
                        assert a == b, (mode, size, style, run_times[i], "state" if a[0] != b[0] else "application globals")
                records.append(dict(effect=effect, mode=mode, size=size, style=style, times=run_times,
                                    native_observations=[dict(image=int(i), ink=int(n), parameter=int(p)) for i,n,p in re.findall(rb"NATIVE image=(\d+) ink=(\d+) parameter=(-?\d+)",run.stdout)],
                                    seed=int(command[command.index("--seed")+1]), hashes=[hashlib.sha256(a+b).hexdigest() for a,b in state]))
                print(effect, mode, size, style, len(run_times), "full state equal", flush=True)
    (scratch / f"{effect}-identity.json").write_text(json.dumps(records, indent=2)+"\n")


if __name__ == "__main__":
    main()
