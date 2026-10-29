"""Configure/build/test/run the SDL3 port without changing the DOS toolchain."""
from pathlib import Path
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build" / "native"
VS = Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake")
SDL = Path(r"C:\tools\sdl3-3.4.16-mingw\SDL3-3.4.16")
BIN = Path(r"C:\msys64\mingw64\bin")


def generator(env):
    """Prefer Ninja; its Windows completion pipe can hang under restricted tokens."""
    requested = env.get("DAZZLE_CMAKE_GENERATOR")
    cache = BUILD / "CMakeCache.txt"
    cached = None
    if cache.exists():
        cached = next((line.split("=", 1)[1] for line in cache.read_text().splitlines()
                       if line.startswith("CMAKE_GENERATOR:INTERNAL=")), None)
    if requested:
        selected = requested
    elif cached == "MinGW Makefiles":
        selected = cached
    else:
        probe = BUILD / "ninja-probe"
        probe.mkdir(parents=True, exist_ok=True)
        (probe / "build.ninja").write_text("rule probe\n  command = cmd.exe /d /c exit 0\nbuild probe: probe\n")
        try:
            subprocess.run([str(VS / "Ninja" / "ninja.exe"), "-C", str(probe)],
                           env=env, check=True, capture_output=True, timeout=3)
            selected = "Ninja"
        except subprocess.TimeoutExpired:
            print("Ninja completion pipe unavailable; using MinGW Makefiles for this build.", flush=True)
            selected = "MinGW Makefiles"
    if cached and cached != selected:
        cache.unlink()  # CMake's compiler probe may have left an incomplete Ninja cache.
    return selected


def main():
    args = sys.argv[1:]
    command = args.pop(0) if args else "build"
    if command not in ("build", "test", "run") or (args and command != "run"):
        raise SystemExit("usage: python -B -I tools/native.py [build|test|run ARGS]")
    env = os.environ.copy()
    env["PATH"] = os.pathsep.join(map(str, (BIN, VS / "CMake" / "bin", VS / "Ninja", SDL / "x86_64-w64-mingw32" / "bin"))) + os.pathsep + env.get("PATH", "")
    selected = generator(env)
    make = VS / "Ninja" / "ninja.exe" if selected == "Ninja" else BIN / "mingw32-make.exe"

    def run(argv):
        if argv[0] in ("cmake", "ctest"):
            argv[0] = VS / "CMake" / "bin" / (argv[0] + ".exe")
        subprocess.run(list(map(str, argv)), cwd=ROOT, env=env, check=True)

    run(["cmake", "-S", ROOT / "native", "-B", BUILD, "-G", selected,
         f"-DCMAKE_C_COMPILER={BIN / 'gcc.exe'}", f"-DCMAKE_PREFIX_PATH={SDL}",
         f"-DCMAKE_MAKE_PROGRAM={make}", "-DCMAKE_BUILD_TYPE=RelWithDebInfo"])
    run(["cmake", "--build", BUILD, "--parallel"])
    if command == "test":
        run(["ctest", "--test-dir", BUILD, "--output-on-failure"])
    elif command == "run":
        run([BUILD / "dazzle.exe", *args])


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.returncode) from None
