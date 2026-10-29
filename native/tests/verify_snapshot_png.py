"""Exercise CP437 text conversion and retain VGA/EGA PNG support."""
from pathlib import Path
import importlib.util
import struct
import sys
import zlib

spec = importlib.util.spec_from_file_location('snapshot_png', Path(__file__).with_name('snapshot_png.py'))
renderer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(renderer)
scratch = Path(sys.argv[1])
scratch.mkdir(parents=True, exist_ok=True)


def render(mode, width, height, frame, palette, ns=0):
    source, target = scratch / 'test.snap', scratch / 'test.png'
    source.write_bytes((f'DAZZLE-SNAPSHOT 1\nmode={mode}\nwidth={width}\nheight={height}\n'
                        f'ns={ns}\npalette={palette.hex()}\nEND\n').encode() + frame)
    renderer.convert(source, target)
    png = target.read_bytes()
    assert png[:8] == b'\x89PNG\r\n\x1a\n'
    assert struct.unpack('>II', png[16:24]) == ((720, 400) if mode == 0 else (width, height))
    # The converter deliberately uses one IDAT and unfiltered RGB rows.
    length = struct.unpack('>I', png[33:37])[0]
    raw = zlib.decompress(png[41:41+length])
    stride = (720 if mode == 0 else width) * 3 + 1

    def pixel(x, y):
        offset = y * stride + 1 + x * 3
        return tuple(raw[offset:offset+3])
    return pixel


cells = bytearray(b' \x17' * 2000)
cells[:6] = bytes((0xdb, 0x4e, 0xc4, 0x1f, 0xdb, 0x8a))
pixel = render(0, 80, 25, cells, b'')
assert pixel(0, 0) == (255, 255, 85)  # full block, bright yellow foreground
assert pixel(8, 0) == (255, 255, 85)  # DB is in the C0..DF ninth-column extension range
assert pixel(9+8, 8) == (255, 255, 255)  # horizontal box line joins adjacent cells
assert pixel(3*9, 0) == (0, 0, 170)  # space uses blue background
assert pixel(2*9, 0) == (85, 255, 85)
pixel = render(0, 80, 25, cells, b'', ns=500_000_000)
assert pixel(2*9, 0) == (0, 0, 0)  # blink attribute hides ink in the off phase
palette = bytes((0, 0, 0, 63, 0, 0)) + bytes(762)
assert render(1, 1, 1, b'\1', palette)(0, 0) == (255, 0, 0)
assert render(3, 1, 1, b'\1', bytes((0, 20)) + bytes(15))(0, 0) == (170, 85, 0)
print('CP437 text, box joins, attributes, blink, VGA and EGA PNG conversion passed')
