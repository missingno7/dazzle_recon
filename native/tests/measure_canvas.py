"""Windows CPU/peak memory over twenty real seconds, production executable."""
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
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    psapi = ctypes.WinDLL("psapi", use_last_error=True)
    kernel.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
    kernel.GetProcessTimes.restype = wintypes.BOOL
    psapi.GetProcessMemoryInfo.argtypes = [wintypes.HANDLE, ctypes.POINTER(Memory), wintypes.DWORD]
    psapi.GetProcessMemoryInfo.restype = wintypes.BOOL
    script = scratch / "sine.keys"
    script.write_text("3 n\n3.2 ALT+F9\n8 ALT+K\n8.01 F10\n")
    records = []
    for size in ("1920x1080", "3840x2160"):
        for canvas in ("classic", "screen"):
            state = scratch / f"{size}-{canvas}.state"
            command = [str(exe), "--window", size, "--canvas", canvas, "--seed", "1", "--script", str(script),
                       "--exit-after", "20", "--original", "absent.exe", "--state", "19.9", str(state), "/Q"]
            start = time.perf_counter()
            process = subprocess.Popen(command, cwd=scratch, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            peak_ws = peak_private = 0
            try:
                while time.perf_counter() - start < 20 and process.poll() is None:
                    memory = Memory(); memory.cb = ctypes.sizeof(memory)
                    if psapi.GetProcessMemoryInfo(int(process._handle), ctypes.byref(memory), memory.cb):
                        peak_ws = max(peak_ws, memory.PeakWorkingSetSize)
                        peak_private = max(peak_private, memory.PrivateUsage)
                    time.sleep(.05)
                elapsed = time.perf_counter() - start
                times = [wintypes.FILETIME() for _ in range(4)]
                assert kernel.GetProcessTimes(int(process._handle), *map(ctypes.byref, times))
                used = sum((t.dwHighDateTime << 32) | t.dwLowDateTime for t in times[2:]) / 1e7
                out, err = process.communicate(timeout=60)
                assert process.returncode == 0, (out, err)
                record = dict(size=size, canvas=canvas, wall_seconds=elapsed, cpu_seconds=used,
                              one_core_percent=100 * used / elapsed, peak_working_set=peak_ws, peak_private=peak_private,
                              completion_wall=time.perf_counter() - start, command=command)
                records.append(record)
                print(json.dumps(record), flush=True)
            finally:
                if process.poll() is None:
                    process.kill(); process.wait()
        assert (scratch / f"{size}-classic.state").read_bytes() == (scratch / f"{size}-screen.state").read_bytes()
    (scratch / "cpu-memory-20s.json").write_text(json.dumps(records, indent=2) + "\n")


if __name__ == "__main__":
    main()
