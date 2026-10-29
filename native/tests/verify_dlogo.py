"""Compare the native logo framebuffer to tools/logo.py's live decoder."""
from pathlib import Path
import runpy
import subprocess
import sys


def main() -> int:
    native_test = Path(sys.argv[1])
    executable = Path(sys.argv[2])
    workdir = Path(sys.argv[3])
    if not executable.is_file():
        print("logo reference skipped: original executable is absent")
        return 0

    subprocess.run([str(native_test)], cwd=workdir, check=True)
    binary = executable.read_bytes()
    last_page_bytes = int.from_bytes(binary[2:4], "little")
    page_count = int.from_bytes(binary[4:6], "little")
    if binary[:2] != b"MZ" or not page_count:
        raise AssertionError("original executable has no valid MZ header")
    base = (page_count - 1) * 512 + last_page_bytes + 1
    image_offset = base + 10 + int.from_bytes(binary[base + 8:base + 10], "little")
    resource_size = int.from_bytes(binary[image_offset + 0x46:image_offset + 0x4A], "little")
    resource = binary[image_offset:image_offset + resource_size]

    root = Path(__file__).resolve().parents[2]
    decoder = runpy.run_path(str(root / "tools" / "logo.py"))["decode_resource"]
    image = decoder(resource)
    native_pixels = (workdir / "dlogo_frame.raw").read_bytes()
    if (image.width, image.height) != (640, 350):
        raise AssertionError(f"unexpected reference dimensions: {image.width}x{image.height}")
    if native_pixels != image.pixels:
        mismatch = next((i for i, (a, b) in enumerate(zip(native_pixels, image.pixels)) if a != b), None)
        raise AssertionError(f"native logo differs from tools/logo.py at pixel {mismatch}")
    (workdir / "dlogo_frame.raw").unlink()
    print(f"native logo matches tools/logo.py: {image.width}x{image.height}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
