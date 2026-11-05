"""20 virtual seconds of production effects/static; Windows CPU deltas.

Realtime/window cases take 20 wall seconds after warmup. Deterministic headless
reports CPU per the same 20 simulation seconds, isolating algorithm cost.
"""
from pathlib import Path
import ctypes
from ctypes import wintypes
import json
import os
import re
import subprocess
import sys


def main():
    exe, scratch = (Path(p).resolve() for p in sys.argv[1:3])
    effect = sys.argv[sys.argv.index("--effect")+1] if "--effect" in sys.argv else "sine"
    # Original Ctrl program indices include the duplicate Alt record at 36.
    key, image = {"sine": ("k",27), "cycloids": ("0",9), "sweep2": ("b",33),
        "ellipse": ("r",13), "arches": ("1",0), "daisies": ("q",10),
        "squirals": ("c",31), "corkscrew": ("8",7),
               "boxes": ("6",5), "puffs": ("d",22), "bars": ("4",3),
               "wandering": ("n",34), "bigstar": ("5",4), "littleStars": ("p",19), "cones": ("7",6),
        "yarnball": ("CTRL+F",42), "gumballs": ("CTRL+C",39),
        "hexagon": ("u",16), "hilbert": ("i",17), "spiral": ("l",28),
        "spiral1": ("z",29), "squares": ("x",30), "sweep": ("v",32),
        "zigstream": ("m",35)}[effect]
    if "--effect" in sys.argv: scratch /= effect
    scratch.mkdir(parents=True, exist_ok=True)
    run_dir = Path(__file__).resolve().parents[2] / "build/workers/s-lines"
    run_dir.mkdir(parents=True, exist_ok=True)
    assert len(str(run_dir)) < 80  # Original getcwd buffer.
    key = key if "+" in key else f"ALT+{key.upper()}"
    script = scratch / f"{effect}.keys"
    script.write_text(f"3 n\n3.2 ALT+F9\n8 n\n8.1 {key}\n8.11 F10\n")
    if effect == "arches":
        # Continuous F10 eventually drives original signed radii negative and
        # overruns the DOS direction cache, including in Classic. Rebuild rings.
        with script.open("a") as file:
            for second in range(9,29): file.write(f"{second} ALT+1\n{second+.01} F10\n")
    records = []
    backend = sys.argv[sys.argv.index("--backend")+1] if "--backend" in sys.argv else "direct3d11"
    env = dict(os.environ, SDL_AUDIODRIVER="dummy", SDL_RENDER_DRIVER=backend)
    if backend == "default": env.pop("SDL_RENDER_DRIVER",None)
    env.pop("SDL_VIDEODRIVER", None)
    def selected(option, value):
        return option not in sys.argv or sys.argv[sys.argv.index(option)+1] == value
    for size in ("1920x1080", "3840x2160"):
        if not selected("--size", size): continue
        for workload, canvas, native in (("classic", "classic", "none"),
                                         ("faithful", "screen", "none"),
                                         ("native", "screen", effect),
                                         ("static-classic", "classic", "none"),
                                         ("static-screen", "screen", "none")):
            if not selected("--workload", workload): continue
            timings = ["deterministic", "realtime", "window"]
            if "--raster" in sys.argv and workload in ("faithful", "native"): timings.append("raster")
            for timing in timings:
                if not selected("--timing", timing): continue
                if workload.startswith("static"):
                    args = ["--static", size, canvas, timing]
                else:
                    seed = sys.argv[sys.argv.index("--seed")+1] if "--seed" in sys.argv else "1"
                    args = ["--window", size, "--canvas", canvas, "--seed", seed, "--script", str(script),
                            "--exit-after", "28.6", "--original", "absent.exe", "/Q"]
                    if canvas == "screen": args += ["--native", native]
                    if timing != "window": args += ["--headless"]
                    if timing in ("deterministic", "raster"): args += ["--deterministic"]
                    for t in ("8.5", "28.5"):
                        args += ["--state", t, str(scratch / f"{size}-{workload}-{timing}-{t}.state")]
                run_env = dict(env)
                if timing == "raster":
                    run_env["DAZZLE_BENCH_RASTER"] = "1"
                    run_env["SDL_VIDEODRIVER"] = "dummy"
                command = [str(exe), *args]
                process = subprocess.Popen(command,cwd=run_dir,env=run_env,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
                affinity = int(sys.argv[sys.argv.index("--affinity")+1],0) if "--affinity" in sys.argv else None
                try:
                    if affinity is not None:
                        kernel = ctypes.WinDLL("kernel32",use_last_error=True)
                        kernel.SetProcessAffinityMask.argtypes = [wintypes.HANDLE,ctypes.c_size_t]
                        kernel.SetProcessAffinityMask.restype = wintypes.BOOL
                        assert kernel.SetProcessAffinityMask(int(process._handle),affinity), ctypes.get_last_error()
                    out,err = process.communicate(timeout=90)
                    result = subprocess.CompletedProcess(command,process.returncode,out,err)
                finally:
                    if process.poll() is None: process.kill(); process.wait()
                assert result.returncode == 0, (args, result.stdout, result.stderr)
                points = re.findall(rb"BENCH ns=(\d+) cpu=([\d.]+) wall=([\d.]+) image=(\d+)", result.stdout)
                assert len(points) == 2, result.stdout
                if not workload.startswith("static"): assert all(p[3] == str(image).encode() for p in points), points
                used = float(points[1][1])-float(points[0][1])
                wall = float(points[1][2])-float(points[0][2])
                record = dict(effect=effect, size=size, workload=workload, timing=timing, cpu_seconds=used,
                              wall_seconds=wall, one_core_percent=used/20*100,
                              command=command,affinity_mask=affinity)
                record["native_ink"] = [int(n) for n in re.findall(rb"INK native=(\d+)",result.stdout)]
                uploads = re.findall(rb"uploads=(\d+) bytes=(\d+) textures=(\d+)", result.stdout)
                if uploads:
                    for i, name in enumerate(("uploads", "upload_bytes", "textures")):
                        record[name] = int(uploads[1][i])-int(uploads[0][i])
                outputs = re.findall(rb"OUTPUT (\d+x\d+) backend=(\w+)",result.stdout)
                if outputs:
                    assert len(outputs) == 2 and all(s.decode() == size for s, _ in outputs), outputs
                    record["actual_output"] = outputs[0][0].decode()
                    record["backend"] = outputs[0][1].decode()
                if not workload.startswith("static"):
                    for t in ("8.5", "28.5"):
                        path = scratch / f"{size}-{workload}-{timing}-{t}.state"
                        fields = dict(line.split(b"=",1) for line in path.read_bytes().split(b"END\n",1)[0].splitlines()[1:])
                        assert fields[b"fade"] == b"0", (path, fields)
                        control = "deterministic" if timing == "raster" else timing
                        canonical = scratch / f"{size}-classic-{control}-{t}.state"
                        assert path.read_bytes() == canonical.read_bytes(), (path,"state differs")
                records.append(record)
                print(json.dumps(record), flush=True)
                (scratch / "cpu-20s.json").write_text(json.dumps(records, indent=2)+"\n")


if __name__ == "__main__":
    main()
