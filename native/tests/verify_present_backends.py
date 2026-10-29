"""Opt-in hidden-window comparisons; unavailable backends are reported, failures fail."""
from pathlib import Path
import os
import subprocess
import sys


def main():
    exe = Path(sys.argv[1] if len(sys.argv) > 1 else "build/native/test_present.exe").resolve()
    env = os.environ.copy()
    env.pop("SDL_VIDEODRIVER", None)
    env.pop("SDL_RENDER_DRIVER", None)
    failed = False
    for backend in (sys.argv[2:] or ("default", "direct3d11", "direct3d12", "vulkan", "gpu", "opengl")):
        print(f"--- {backend} ---", flush=True)
        try:
            result = subprocess.run([str(exe), "--backend", backend], env=env, timeout=180)
        except subprocess.TimeoutExpired:
            print(f"TIMEOUT {backend} (child killed)", flush=True)
            failed = True
            continue
        if result.returncode not in (0, 77):
            failed = True
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
