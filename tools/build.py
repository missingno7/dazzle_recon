#!/usr/bin/env python3
"""Link DAZZLE and append a freshly generated self-checking resource tail."""

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
LINK_REPORT = ROOT / "build" / "link.json"
OUTPUT = ROOT / "build" / "DAZZLE.EXE"


def run(command):
    return subprocess.run(command, cwd=ROOT, check=False).returncode


def main():
    (ROOT / "build").mkdir(parents=True, exist_ok=True)
    link = [sys.executable, "-B", "-I", "tools/link.py", "--out", "build/link.json"]
    if run(link):
        raise RuntimeError("link.py failed; see build/link.json")

    report = json.loads(LINK_REPORT.read_text(encoding="utf-8"))
    if report.get("status") != "EXACT" or not report.get("executable"):
        raise RuntimeError("link receipt is not EXACT or has no executable")
    mz_path = (ROOT / report["executable"]).resolve()
    try:
        mz_path.relative_to(ROOT)
    except ValueError as exc:
        raise RuntimeError("link receipt executable is outside the project") from exc
    mz = mz_path.read_bytes()
    if hashlib.sha256(mz).hexdigest() != report.get("executable_sha256"):
        raise RuntimeError("linked executable does not match its receipt hash")

    worker_dir = ROOT / "build" / "workers" / "build"
    worker_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="tail-", dir=worker_dir) as temp_dir:
        tail_path = Path(temp_dir) / "DAZZLE.tail"
        tail_command = [sys.executable, "-B", "-I", "tools/mktail.py",
                        str(mz_path), "assets/DAZZLE.DOC", str(tail_path)]
        if run(tail_command):
            raise RuntimeError("mktail.py failed")
        tail = tail_path.read_bytes()

    result = mz + tail
    temp_output = OUTPUT.with_suffix(".tmp")
    temp_output.write_bytes(result)
    temp_output.replace(OUTPUT)
    print(f"MZ: {len(mz)} bytes, sha256 {hashlib.sha256(mz).hexdigest()}")
    print(f"Tail: {len(tail)} bytes")
    print(f"{OUTPUT.relative_to(ROOT)}: {len(result)} bytes, sha256 "
          f"{hashlib.sha256(result).hexdigest()}")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError, json.JSONDecodeError) as exc:
        print(f"build.py: {exc}", file=sys.stderr)
        raise SystemExit(1)
