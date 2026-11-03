"""Twenty real seconds of sine after startup; production AUTO renderer on Windows."""
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
    run_directory = Path(__file__).resolve().parents[2] / "build" / "workers" / "s-native"
    run_directory.mkdir(parents=True,exist_ok=True)
    if len(str(run_directory)) >= 80:
        raise SystemExit("Benchmark needs a shorter workspace path (original startup getcwd limit)")
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    psapi = ctypes.WinDLL("psapi", use_last_error=True)
    kernel.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
    kernel.GetProcessTimes.restype = wintypes.BOOL
    psapi.GetProcessMemoryInfo.argtypes = [wintypes.HANDLE, ctypes.POINTER(Memory), wintypes.DWORD]
    psapi.GetProcessMemoryInfo.restype = wintypes.BOOL
    script = scratch / "sine.keys"
    script.write_text("3 n\n3.2 ALT+F9\n8 n\n8.1 ALT+K\n8.11 F10\n")
    records = []
    selected_size = sys.argv[sys.argv.index("--size")+1] if "--size" in sys.argv else None
    selected_style = sys.argv[sys.argv.index("--style")+1] if "--style" in sys.argv else None
    for size in ("1920x1080", "3840x2160"):
        if selected_size and size != selected_size: continue
        for style, args in (("faithful", ["--native", "none"]), ("crisp", ["--native", "sine"])):
            if selected_style and style != selected_style: continue
            state = scratch / f"{size}-{style}.state"
            warm = scratch / f"{size}-{style}-warm.state"
            if warm.exists(): warm.unlink()
            command = [str(exe), "--window", size, "--canvas", "screen", "--seed", "1", "--script", str(script),
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
                # Measure twenty wall seconds of sine, after original startup.
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
                    assert fields[b"image"].split(b",")[0] == b"27" and fields[b"fade"] == b"0", (path,header)
                record = dict(size=size, style=style, wall_seconds=elapsed, cpu_seconds=used,
                              one_core_percent=100*used/elapsed, peak_working_set=peak_ws, peak_private=peak_private,
                              stopped_after_sample=forced, warmup_wall=start-launched, completion_wall=time.perf_counter()-launched, cwd=str(run_directory), command=command)
                records.append(record)
                (scratch / "cpu-memory-20s.json").write_text(json.dumps(records, indent=2)+"\n")
                print(json.dumps(record), flush=True)
            finally:
                if process.poll() is None:
                    process.kill(); process.wait()
        if not selected_style:
            assert (scratch / f"{size}-faithful.state").read_bytes() == (scratch / f"{size}-crisp.state").read_bytes()
        # AA identity is covered at exact virtual deadlines by verify_native.py;
        # a twenty-second wall sample need not reach its 28.5 virtual deadline.


if __name__ == "__main__":
    main()
