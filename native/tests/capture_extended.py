"""Paired Original/Extended PNGs and full-state identity at exact virtual deadlines."""
from pathlib import Path
import json
import os
import subprocess
import sys
import struct
import zlib


def read_png(path):
    # SDL_SavePNG emits noninterlaced RGB/RGBA8. Keep this capture tool dependency free.
    data = path.read_bytes()
    w, h, depth, kind, _, _, interlace = struct.unpack(">IIBBBBB", data[16:29])
    assert depth == 8 and kind in (2, 6) and interlace == 0
    compressed, offset = bytearray(), 8
    while offset < len(data):
        length, = struct.unpack(">I", data[offset:offset+4])
        if data[offset+4:offset+8] == b"IDAT":
            compressed.extend(data[offset+8:offset+8+length])
        offset += length + 12
    raw = zlib.decompress(compressed)
    bpp, rows, previous = (3 if kind == 2 else 4), [], bytearray(w * (3 if kind == 2 else 4))
    stride = w * bpp
    for y in range(h):
        method = raw[y*(stride+1)]
        row = bytearray(raw[y*(stride+1)+1:(y+1)*(stride+1)])
        for x in range(stride):
            a, b, c = row[x-bpp] if x >= bpp else 0, previous[x], previous[x-bpp] if x >= bpp else 0
            p = a + b - c
            pa, pb, pc = abs(p-a), abs(p-b), abs(p-c)
            predictor = (0, a, b, (a+b)//2, a if pa <= pb and pa <= pc else b if pb <= pc else c)[method]
            row[x] = (row[x] + predictor) & 255
        rows.append(bytes(row) if bpp == 3 else bytes(v for x,v in enumerate(row) if x % 4 != 3))
        previous = row
    return w, h, rows


def write_pair(path, w, h, left, right):
    def chunk(kind, payload):
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind+payload))
    path.write_bytes(b"\x89PNG\r\n\x1a\n" +
                     chunk(b"IHDR", struct.pack(">IIBBBBB", w*2, h, 8, 2, 0, 0, 0)) +
                     chunk(b"IDAT", zlib.compress(b"".join(b"\0"+a+b for a,b in zip(left,right)))) +
                     chunk(b"IEND", b""))


def main():
    exe = Path(sys.argv[1]).resolve()
    scratch = Path(sys.argv[2]).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    receipts = []
    for w, h in ((1920, 1080), (2560, 1080)):
        for image, key, time in (("lotsadots", "A", "8.3"), ("sine", "K", "8.15")):
            script = scratch / f"{image}.keys"
            script.write_text(f"3 n\n3.2 ALT+F9\n8 ALT+{key}\n8.01 F10\n")
            states, pictures = [], []
            for policy in ("original", "extended"):
                png = scratch / f"{image}-{w}x{h}-{policy}.png"
                state = png.with_suffix(".state")
                args = [str(exe), "--deterministic", "--seed", "1", "--present", policy,
                        "--window", f"{w}x{h}", "--original", "absent.exe", "--script", str(script),
                        "--exit-after", time, "--state", time, str(state), "--screenshot", str(png)]
                run = subprocess.run(args, cwd=scratch, env=env, capture_output=True, timeout=120)
                assert run.returncode == 0, (args, run.stdout, run.stderr)
                states.append(state.read_bytes())
                pw, ph, rows = read_png(png)
                assert (pw, ph) == (w, h)
                pictures.append(rows)
                print(png.name, flush=True)
            assert states[0] == states[1], (image, w, "logical state differs")
            center = int((w - h * 4 / 3) / 2)
            assert all(a[center*3:(w-center)*3] == b[center*3:(w-center)*3]
                       for a,b in zip(*pictures))
            assert pictures[0] != pictures[1]
            if w == 2560:
                assert all(not any(row[:60]) and not any(row[-60:]) for row in pictures[1])
            write_pair(scratch / f"{image}-{w}x{h}-pair.png", w, h, *pictures)
            receipts.append(dict(image=image, width=w, height=h, seed=1, virtual_seconds=time,
                                 renderer="SDL software / dummy", full_state_identical=True,
                                 central_pixels_identical=True, cap_21_9=True))
    (scratch / "captures.json").write_text(json.dumps(receipts, indent=2) + "\n")


if __name__ == "__main__":
    main()
