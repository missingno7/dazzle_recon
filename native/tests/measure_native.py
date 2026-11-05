"""Twenty real seconds per effect after startup; production AUTO renderer on Windows."""
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import subprocess
import sys
import time


class Memory(ctypes.Structure):
    _fields_ = [("cb", wintypes.DWORD), ("PageFaultCount", wintypes.DWORD)] + [
        (name, ctypes.c_size_t) for name in ("PeakWorkingSetSize", "WorkingSetSize", "QuotaPeakPagedPoolUsage",
        "QuotaPagedPoolUsage", "QuotaPeakNonPagedPoolUsage", "QuotaNonPagedPoolUsage", "PagefileUsage", "PeakPagefileUsage", "PrivateUsage")]


def main():
    exe, scratch = (Path(p).resolve() for p in sys.argv[1:3])
    scratch.mkdir(parents=True, exist_ok=True)
    # The original startup has an 80-byte getcwd buffer. A long scratch name
    # otherwise consumes the first scripted key in its error prompt.
    run_directory = (Path(sys.argv[sys.argv.index("--run-dir")+1]).resolve()
                     if "--run-dir" in sys.argv else
                     Path(__file__).resolve().parents[2] / "build" / "workers" / "s-fx2")
    run_directory.mkdir(parents=True,exist_ok=True)
    if len(str(run_directory)) >= 80:
        raise SystemExit("Benchmark needs a shorter workspace path (original startup getcwd limit)")
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    psapi = ctypes.WinDLL("psapi", use_last_error=True)
    kernel.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
    kernel.GetProcessTimes.restype = wintypes.BOOL
    psapi.GetProcessMemoryInfo.argtypes = [wintypes.HANDLE, ctypes.POINTER(Memory), wintypes.DWORD]
    psapi.GetProcessMemoryInfo.restype = wintypes.BOOL
    # Original Ctrl program indices include the duplicate Alt record at 36.
    effects = {"cycloids": ("0",9), "sweep2": ("b",33), "ellipse": ("r",13),
               "arches": ("1",0), "daisies": ("q",10), "squirals": ("c",31),
               "corkscrew": ("8",7), "sine": ("k",27),
               "boxes": ("6",5), "puffs": ("d",22), "bars": ("4",3),
               "wandering": ("n",34), "bigstar": ("5",4), "littleStars": ("p",19), "cones": ("7",6),
               "yarnball": ("CTRL+F",42), "gumballs": ("CTRL+C",39),
               "hexagon": ("u",16), "hilbert": ("i",17), "spiral": ("l",28),
               "spiral1": ("z",29), "squares": ("x",30), "sweep": ("v",32),
               "zigstream": ("m",35),
               "atom": ("2",1), "balls": ("3",2), "curves": ("9",8), "diag1": ("w",11),
               "diag2": ("e",12), "formula1": ("t",14), "grid": ("y",15), "lines": ("o",18),
               "lotsadots": ("a",20), "pedals": ("s",21), "ribbons": ("f",23),
               "triangles": ("g",24), "tunnel": ("h",25), "sierpinski": ("j",26),
                "butterflies": ("CTRL+A",37), "diamonds": ("CTRL+B",38),
                "tiles": ("CTRL+D",40), "wedges": ("CTRL+E",41)}
    if "--effect" in sys.argv:
        names = sys.argv[sys.argv.index("--effect")+1].split(",")
        effects = {name: effects[name] for name in names}
    for effect, (key, image) in effects.items():
        measure(exe, scratch, run_directory, kernel, psapi, effect, key, image)


def measure(exe, scratch, run_directory, kernel, psapi, effect, key, image):
    key = key if "+" in key else f"ALT+{key.upper()}"
    script = scratch / f"{effect}.keys"
    script.write_text(f"3 n\n3.2 ALT+F9\n8 n\n8.1 {key}\n8.11 F10\n")
    if effect == "arches":
        # Continuous F10 eventually drives original signed radii negative and
        # overruns the DOS direction cache, including in Classic. Rebuild rings.
        with script.open("a") as file:
            for second in range(9,29): file.write(f"{second} ALT+1\n{second+.01} F10\n")
    records = []
    selected_size = sys.argv[sys.argv.index("--size")+1] if "--size" in sys.argv else None
    selected_style = sys.argv[sys.argv.index("--style")+1] if "--style" in sys.argv else None
    for size in ("1920x1080", "3840x2160"):
        if selected_size and size != selected_size: continue
        for style, args in (("faithful", ["--native", "none"]), ("crisp", ["--native", effect])):
            if selected_style and style != selected_style: continue
            state = scratch / f"{effect}-{size}-{style}.state"
            warm = scratch / f"{effect}-{size}-{style}-warm.state"
            if warm.exists(): warm.unlink()
            command = [str(exe), "--window", size, "--canvas", "screen", "--seed", sys.argv[sys.argv.index("--seed")+1] if "--seed" in sys.argv else "1", "--script", str(script),
                       "--exit-after", "28.6", "--original", "absent.exe", "--state", "8.5", str(warm),
                       "--state", "28.5", str(state), "/Q", *args]
            env = dict(__import__("os").environ)
            env.pop("SDL_VIDEODRIVER", None)
            env["SDL_AUDIODRIVER"] = "dummy"
            launched = time.perf_counter()
            process = subprocess.Popen(command, cwd=run_directory, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            peak_ws = peak_private = 0
            def cpu_time():
                times = [wintypes.FILETIME() for _ in range(4)]
                assert kernel.GetProcessTimes(int(process._handle), *map(ctypes.byref, times))
                return sum((t.dwHighDateTime << 32) | t.dwLowDateTime for t in times[2:]) / 1e7
            try:
                # Measure twenty wall seconds of the effect, after original startup.
                while not warm.exists():
                    assert process.poll() is None and time.perf_counter()-launched < 90
                    time.sleep(.01)
                before = cpu_time()
                start = time.perf_counter()
                while time.perf_counter()-start < 20 and process.poll() is None:
                    memory = Memory(); memory.cb = ctypes.sizeof(memory)
                    if psapi.GetProcessMemoryInfo(int(process._handle), ctypes.byref(memory), memory.cb):
                        peak_ws = max(peak_ws, memory.PeakWorkingSetSize)
                        peak_private = max(peak_private, memory.PrivateUsage)
                    time.sleep(.05)
                elapsed = time.perf_counter()-start
                used = cpu_time()-before
                forced = style == "aa" and process.poll() is None
                if forced: process.kill()  # CPU reference can fall behind wall time; the sample is complete.
                out, err = process.communicate(timeout=150)
                assert forced or process.returncode == 0, (out, err)
                for path in (warm,state):
                    header = path.read_bytes().split(b"END\n",1)[0]
                    fields = dict(line.split(b"=",1) for line in header.splitlines()[1:])
                    assert fields[b"image"].split(b",")[0] == str(image).encode() and fields[b"fade"] == b"0", (path,header)
                record = dict(effect=effect, size=size, style=style, wall_seconds=elapsed, cpu_seconds=used,
                              one_core_percent=100*used/elapsed, peak_working_set=peak_ws, peak_private=peak_private,
                              stopped_after_sample=forced, warmup_wall=start-launched, completion_wall=time.perf_counter()-launched, cwd=str(run_directory), command=command)
                records.append(record)
                (scratch / f"{effect}-cpu-memory-20s.json").write_text(json.dumps(records, indent=2)+"\n")
                print(json.dumps(record), flush=True)
            finally:
                if process.poll() is None:
                    process.kill(); process.wait()
        if not selected_style:
            assert (scratch / f"{effect}-{size}-faithful.state").read_bytes() == (scratch / f"{effect}-{size}-crisp.state").read_bytes()
        # AA identity is covered at exact virtual deadlines by verify_native.py;
        # a twenty-second wall sample need not reach its 28.5 virtual deadline.


if __name__ == "__main__":
    main()
