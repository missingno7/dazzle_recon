"""Twenty wall-second Windows CPU samples; let each virtual run finish normally."""
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import subprocess
import sys
import time


def main():
    exe, scratch = map(lambda p: Path(p).resolve(), sys.argv[1:3])
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
    kernel.GetProcessTimes.restype = wintypes.BOOL

    def cpu(process):
        times = [wintypes.FILETIME() for _ in range(4)]
        if not kernel.GetProcessTimes(int(process._handle), *map(ctypes.byref, times)):
            raise ctypes.WinError(ctypes.get_last_error())
        return sum((t.dwHighDateTime << 32) | t.dwLowDateTime for t in times[2:]) / 1e7

    script = scratch / "sine.keys"
    script.write_text("3 n\n3.2 ALT+F9\n8 ALT+K\n8.01 F10\n")
    records = []
    for quality in ("classic", "hybrid"):
        command = [str(exe), "--window", "1920x1080", "--quality", quality, "--seed", "1",
                   "--script", str(script), "--exit-after", "20", "--original", "absent.exe", "/Q"]
        start = time.perf_counter()
        process = subprocess.Popen(command, cwd=scratch, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            time.sleep(max(0, 20 - (time.perf_counter() - start)))
            elapsed, used = time.perf_counter() - start, cpu(process)
            out, err = process.communicate(timeout=90)
            assert process.returncode == 0, (out, err)
            record = dict(quality=quality, wall_interval=elapsed, cpu_interval=used,
                          one_core_percent=used / elapsed * 100,
                          completion_wall=time.perf_counter() - start, completion_cpu=cpu(process),
                          command=command)
            records.append(record)
            print(json.dumps(record), flush=True)
        finally:
            if process.poll() is None:
                process.kill(); process.wait()
    (scratch / "cpu-20s.json").write_text(json.dumps(records, indent=2) + "\n")


if __name__ == "__main__":
    main()
