"""Render indexed graphics or CP437 text snapshots without third-party packages."""
from pathlib import Path
import re
import struct
import sys
import zlib


def ega_color(color):
    return bytes((((color >> 2) & 1) * 170 + ((color >> 5) & 1) * 85,
                  ((color >> 1) & 1) * 170 + ((color >> 4) & 1) * 85,
                  (color & 1) * 170 + ((color >> 3) & 1) * 85))


def text_rows(frame, fields):
    """Use the same CC0 font, 9th-column extension and blink as video.c.

    Snapshots contain cells, not the hardware cursor; the cursor is omitted.
    """
    font_path = Path(__file__).resolve().parents[1] / 'src/platform/font_cp437.h'
    font = [bytes(int(v, 16) for v in re.findall(r'0x([0-9a-fA-F]{2})', row))
            for row in re.findall(r'\{([^{}]+)\}', font_path.read_text(encoding='utf-8'))]
    if len(font) != 256 or any(len(row) != 16 for row in font):
        raise ValueError('expected 256 CP437 glyphs with 16 rows')
    if len(frame) != 4000:
        raise ValueError('expected 80x25 text character/attribute pairs')
    palette = [ega_color(c) for c in (0, 1, 2, 3, 4, 5, 20, 7, 56, 57, 58, 59, 60, 61, 62, 63)]
    blink_on = (int(fields.get('ns', 0)) // 500_000_000) % 2 == 0
    for y in range(400):
        row = bytearray(b'\0')
        for column in range(80):
            offset = ((y // 16) * 80 + column) * 2
            ch, attr = frame[offset:offset+2]
            bits = font[ch][y % 16] if blink_on or not (attr & 128) else 0
            for x in range(9):
                ink = (x < 8 or 0xc0 <= ch <= 0xdf) and (bits >> (7-x if x < 8 else 0)) & 1
                row.extend(palette[attr & 15 if ink else (attr >> 4) & 7])
        yield bytes(row)


def convert(source, destination):
    header, frame = Path(source).read_bytes().split(b'END\n', 1)
    fields = dict(line.split('=', 1) for line in header.decode().splitlines()[1:])
    mode, width, height = (int(fields[k]) for k in ('mode', 'width', 'height'))
    palette = bytes.fromhex(fields['palette'])
    if mode == 0:
        rows = b''.join(text_rows(frame, fields))
        width, height = 720, 400
    else:
        if mode == 3:
            colors = [ega_color(c) for c in palette[:16]]
        else:
            colors = [bytes(v * 255 // 63 for v in palette[i:i+3]) for i in range(0, 768, 3)]
        rows = b''.join(b'\0' + b''.join(colors[i] for i in frame[y*width:(y+1)*width]) for y in range(height))

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))

    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
    Path(destination).write_bytes(png + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))


if __name__ == '__main__':
    convert(*sys.argv[1:3])
