"""Render a graphics snapshot without SDL or third-party Python packages."""
from pathlib import Path
import struct
import sys
import zlib


def convert(source, destination):
    header, frame = Path(source).read_bytes().split(b'END\n', 1)
    fields = dict(line.split('=', 1) for line in header.decode().splitlines()[1:])
    mode, width, height = (int(fields[k]) for k in ('mode', 'width', 'height'))
    palette = bytes.fromhex(fields['palette'])
    assert mode != 0, 'text snapshots contain character/attribute cells, not pixels'
    if mode == 3:
        colors = [bytes((((c >> 2) & 1) * 170 + ((c >> 5) & 1) * 85,
                         ((c >> 1) & 1) * 170 + ((c >> 4) & 1) * 85,
                         (c & 1) * 170 + ((c >> 3) & 1) * 85)) for c in palette[:16]]
    else:
        colors = [bytes(v * 255 // 63 for v in palette[i:i+3]) for i in range(0, 768, 3)]
    rows = b''.join(b'\0' + b''.join(colors[i] for i in frame[y*width:(y+1)*width]) for y in range(height))

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))

    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
    Path(destination).write_bytes(png + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))


if __name__ == '__main__':
    convert(*sys.argv[1:3])
