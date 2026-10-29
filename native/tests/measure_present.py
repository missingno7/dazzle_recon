"""Windows CPU seconds / wall seconds (one core); pass an executable and its arguments."""
import ctypes
import json
import subprocess
import sys
import time
from ctypes import wintypes
from pathlib import Path


def main():
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
    kernel.GetProcessTimes.restype = wintypes.BOOL
    start = time.perf_counter()
    command = [str(Path(sys.argv[1]).resolve()), *sys.argv[2:]]
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        out, err = process.communicate(timeout=60)
        wall = time.perf_counter() - start
        times = [wintypes.FILETIME() for _ in range(4)]
        if not kernel.GetProcessTimes(int(process._handle), *map(ctypes.byref, times)):
            raise ctypes.WinError(ctypes.get_last_error())
        cpu = sum((t.dwHighDateTime << 32) | t.dwLowDateTime for t in times[2:]) / 1e7
        print(json.dumps(dict(command=sys.argv[1:], wall_seconds=round(wall, 3),
                              cpu_seconds=round(cpu, 3), one_core_percent=round(cpu / wall * 100, 2),
                              exit_code=process.returncode)))
        if out: print(out.decode(errors="replace").strip())
        if err: print(err.decode(errors="replace").strip())
        return process.returncode
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()


if __name__ == "__main__":
    raise SystemExit(main())
