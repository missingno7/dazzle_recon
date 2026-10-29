#!/usr/bin/env python3
"""Decode and encode DAZZLE's four-plane DLOGO image resource using stdlib only."""

from __future__ import annotations

import argparse
import binascii
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
HEADER_SIZE = 0x80
PLANE_COUNT = 4
BITS_PER_PLANE = 1
RUN_MARKER = 0xC0
MAX_RUN = 63


@dataclass(frozen=True)
class LogoImage:
    width: int
    height: int
    palette: bytes
    pixels: bytes


def _pcx_rle_decode(data: bytes, expected_size: int) -> bytes:
    output = bytearray()
    offset = 0
    while len(output) < expected_size:
        if offset >= len(data):
            raise ValueError("truncated DLOGO run stream")
        code = data[offset]
        offset += 1
        if code & 0xC0 == RUN_MARKER:
            count = code & 0x3F
            if offset >= len(data):
                raise ValueError("truncated DLOGO run value")
            value = data[offset]
            offset += 1
            if count == 0:
                continue
            output.extend(bytes((value,)) * count)
        else:
            output.append(code)
        if len(output) > expected_size:
            raise ValueError("DLOGO run stream exceeds the declared image dimensions")
    if offset != len(data):
        raise ValueError(f"DLOGO resource has {len(data) - offset} unused run-stream bytes")
    return bytes(output)


def decode_resource(resource: bytes) -> LogoImage:
    """Decode a DLOGO/PCX-style static 16-color image resource."""
    if len(resource) < HEADER_SIZE:
        raise ValueError("DLOGO resource is shorter than its 128-byte header")
    if resource[:4] != b"\x0a\x05\x01\x01":
        raise ValueError("unsupported DLOGO header signature/version/encoding/plane depth")

    xmin, ymin, xmax, ymax = struct.unpack_from("<4H", resource, 4)
    if xmin or ymin or xmax < xmin or ymax < ymin:
        raise ValueError("unsupported DLOGO image bounds")
    width = xmax - xmin + 1
    height = ymax - ymin + 1
    hres, vres = struct.unpack_from("<2H", resource, 12)
    if (hres, vres) != (width, height):
        raise ValueError("DLOGO resolution fields do not match its image bounds")

    planes = resource[0x41]
    bytes_per_line = struct.unpack_from("<H", resource, 0x42)[0]
    resource_size = struct.unpack_from("<I", resource, 0x46)[0]
    frame_count = struct.unpack_from("<H", resource, 0x4A)[0]
    if planes != PLANE_COUNT or bytes_per_line * 8 != width:
        raise ValueError("unsupported DLOGO plane count or bytes-per-line value")
    if frame_count != 0:
        raise ValueError("animated/VGA DLOGO resources are not supported")
    if resource_size != len(resource):
        raise ValueError(f"DLOGO header size is {resource_size}, resource has {len(resource)} bytes")

    palette = resource[0x10:0x40]
    decoded_planar = _pcx_rle_decode(resource[HEADER_SIZE:], height * bytes_per_line * planes)
    pixels = bytearray(width * height)
    row_stride = bytes_per_line * planes
    for y in range(height):
        row = y * row_stride
        for x in range(width):
            byte_index = x >> 3
            bit_mask = 0x80 >> (x & 7)
            value = 0
            for plane in range(planes):
                if decoded_planar[row + plane * bytes_per_line + byte_index] & bit_mask:
                    value |= 1 << plane
            pixels[y * width + x] = value
    return LogoImage(width, height, palette, bytes(pixels))


def _pcx_rle_encode_row(data: bytes) -> bytes:
    """Encode one four-plane scanline with DAZZLE's observed run choices."""
    output = bytearray()
    offset = 0
    while offset < len(data):
        value = data[offset]
        end = offset + 1
        while end < len(data) and data[end] == value and end - offset < MAX_RUN:
            end += 1
        count = end - offset
        if count > 1 or value >= RUN_MARKER:
            output.extend((RUN_MARKER | count, value))
        else:
            output.append(value)
        offset = end
    return bytes(output)


def encode_resource(image: LogoImage) -> bytes:
    """Encode indexed 4-plane pixels to the exact DLOGO static-image format."""
    width, height = image.width, image.height
    if width <= 0 or height <= 0 or width > 0x10000 or height > 0x10000:
        raise ValueError("image dimensions must fit positive 16-bit PCX bounds")
    if width % 8:
        raise ValueError("DLOGO's four-plane image width must be divisible by 8")
    if len(image.palette) != 16 * 3:
        raise ValueError("DLOGO requires exactly 16 RGB palette entries")
    if len(image.pixels) != width * height:
        raise ValueError("indexed pixel count does not match image dimensions")
    if any(pixel > 15 for pixel in image.pixels):
        raise ValueError("DLOGO pixels must be 4-bit palette indices")

    bytes_per_line = width // 8
    if bytes_per_line > 0xFFFF:
        raise ValueError("DLOGO row size exceeds its 16-bit header field")

    compressed_rows = bytearray()
    for y in range(height):
        row_pixels = image.pixels[y * width:(y + 1) * width]
        planar_row = bytearray()
        for plane in range(PLANE_COUNT):
            for byte_index in range(bytes_per_line):
                value = 0
                first_x = byte_index * 8
                for bit in range(8):
                    if row_pixels[first_x + bit] & (1 << plane):
                        value |= 0x80 >> bit
                planar_row.append(value)
        compressed_rows.extend(_pcx_rle_encode_row(planar_row))

    resource_size = HEADER_SIZE + len(compressed_rows)
    if resource_size > 0xFFFFFFFF:
        raise ValueError("DLOGO resource exceeds its 32-bit size field")
    header = bytearray(HEADER_SIZE)
    header[0:4] = b"\x0a\x05\x01\x01"
    struct.pack_into("<4H", header, 4, 0, 0, width - 1, height - 1)
    struct.pack_into("<2H", header, 12, width, height)
    header[0x10:0x40] = image.palette
    header[0x41] = PLANE_COUNT
    struct.pack_into("<H", header, 0x42, bytes_per_line)
    struct.pack_into("<H", header, 0x44, 0)
    struct.pack_into("<I", header, 0x46, resource_size)
    struct.pack_into("<H", header, 0x4A, 0)
    return bytes(header + compressed_rows)


def _png_chunk(kind: bytes, data: bytes) -> bytes:
    crc = binascii.crc32(kind + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", crc)


def write_png(image: LogoImage, path: Path) -> None:
    if len(image.palette) % 3 or len(image.palette) > 768:
        raise ValueError("PNG palette must contain at most 256 RGB entries")
    if len(image.pixels) != image.width * image.height:
        raise ValueError("indexed pixel count does not match image dimensions")
    if any(pixel >= len(image.palette) // 3 for pixel in image.pixels):
        raise ValueError("pixel index is outside the PNG palette")

    scanlines = bytearray()
    row_bytes = (image.width + 1) // 2
    for y in range(image.height):
        row = image.pixels[y * image.width:(y + 1) * image.width]
        scanlines.append(0)  # PNG filter None.
        for x in range(0, image.width, 2):
            high = row[x]
            low = row[x + 1] if x + 1 < image.width else 0
            scanlines.append((high << 4) | low)
    expected = image.height * (row_bytes + 1)
    if len(scanlines) != expected:
        raise ValueError("internal indexed PNG row packing error")

    ihdr = struct.pack(">IIBBBBB", image.width, image.height, 4, 3, 0, 0, 0)
    png = (PNG_SIGNATURE + _png_chunk(b"IHDR", ihdr)
           + _png_chunk(b"PLTE", image.palette)
           + _png_chunk(b"IDAT", zlib.compress(bytes(scanlines), 9))
           + _png_chunk(b"IEND", b""))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


def _paeth(left: int, above: int, upper_left: int) -> int:
    estimate = left + above - upper_left
    dl = abs(estimate - left)
    da = abs(estimate - above)
    dul = abs(estimate - upper_left)
    if dl <= da and dl <= dul:
        return left
    return above if da <= dul else upper_left


def read_indexed_png(data: bytes) -> LogoImage:
    """Read a non-interlaced indexed PNG (4-bit or 8-bit pixels)."""
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError("input is not a PNG file")
    offset = len(PNG_SIGNATURE)
    width = height = bit_depth = color_type = None
    palette = None
    compressed = bytearray()
    saw_end = False
    while offset < len(data):
        if offset + 12 > len(data):
            raise ValueError("truncated PNG chunk")
        length = struct.unpack_from(">I", data, offset)[0]
        kind = data[offset + 4:offset + 8]
        start = offset + 8
        end = start + length
        if end + 4 > len(data):
            raise ValueError("PNG chunk length exceeds input")
        payload = data[start:end]
        expected_crc = struct.unpack_from(">I", data, end)[0]
        if binascii.crc32(kind + payload) & 0xFFFFFFFF != expected_crc:
            raise ValueError(f"PNG {kind.decode('ascii', 'replace')} chunk CRC mismatch")
        if kind == b"IHDR":
            if length != 13 or width is not None:
                raise ValueError("invalid or repeated PNG IHDR")
            width, height, bit_depth, color_type, compression, filtering, interlace = struct.unpack(
                ">IIBBBBB", payload)
            if compression or filtering or interlace:
                raise ValueError("unsupported PNG compression, filter method, or interlace")
            if color_type != 3 or bit_depth not in (4, 8):
                raise ValueError("input PNG must use indexed color at 4-bit or 8-bit depth")
        elif kind == b"PLTE":
            palette = bytes(payload)
        elif kind == b"tRNS":
            if any(alpha != 255 for alpha in payload):
                raise ValueError("transparent palette entries cannot be represented by DLOGO")
        elif kind == b"IDAT":
            compressed.extend(payload)
        elif kind == b"IEND":
            if length != 0:
                raise ValueError("invalid PNG IEND")
            saw_end = True
            offset = end + 4
            break
        offset = end + 4

    if not saw_end or offset != len(data) or width is None or palette is None:
        raise ValueError("PNG is incomplete or has trailing data")
    if not palette or len(palette) % 3 or len(palette) > 768:
        raise ValueError("invalid PNG palette")

    row_bytes = (width * bit_depth + 7) // 8
    try:
        filtered = zlib.decompress(bytes(compressed))
    except zlib.error as exc:
        raise ValueError(f"invalid PNG compressed image data: {exc}") from exc
    if len(filtered) != height * (row_bytes + 1):
        raise ValueError("PNG decompressed data length does not match its dimensions")
    decoded = bytearray(height * row_bytes)
    cursor = 0
    for y in range(height):
        filter_type = filtered[cursor]
        cursor += 1
        scanline = bytearray(filtered[cursor:cursor + row_bytes])
        cursor += row_bytes
        prior = decoded[(y - 1) * row_bytes:y * row_bytes] if y else b"\x00" * row_bytes
        for x in range(row_bytes):
            left = scanline[x - 1] if x else 0
            above = prior[x]
            upper_left = prior[x - 1] if x else 0
            if filter_type == 1:
                scanline[x] = (scanline[x] + left) & 0xFF
            elif filter_type == 2:
                scanline[x] = (scanline[x] + above) & 0xFF
            elif filter_type == 3:
                scanline[x] = (scanline[x] + ((left + above) >> 1)) & 0xFF
            elif filter_type == 4:
                scanline[x] = (scanline[x] + _paeth(left, above, upper_left)) & 0xFF
            elif filter_type != 0:
                raise ValueError(f"unsupported PNG filter {filter_type}")
        decoded[y * row_bytes:(y + 1) * row_bytes] = scanline

    pixels = bytearray(width * height)
    for y in range(height):
        row = decoded[y * row_bytes:(y + 1) * row_bytes]
        if bit_depth == 8:
            pixels[y * width:(y + 1) * width] = row[:width]
        else:
            for x in range(width):
                byte = row[x >> 1]
                pixels[y * width + x] = (byte >> 4) & 0x0F if x & 1 == 0 else byte & 0x0F
    if max(pixels, default=0) >= len(palette) // 3:
        raise ValueError("PNG pixel index exceeds its palette")
    return LogoImage(width, height, palette, bytes(pixels))


def _main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="operation", required=True)
    decoder = subparsers.add_parser("decode", help="decode a DLOGO resource to indexed PNG")
    decoder.add_argument("resource", type=Path)
    decoder.add_argument("output_png", type=Path)
    encoder = subparsers.add_parser("encode", help="encode an indexed PNG to a DLOGO resource")
    encoder.add_argument("input_png", type=Path)
    encoder.add_argument("output_resource", type=Path)
    args = parser.parse_args()
    if args.operation == "decode":
        image = decode_resource(args.resource.read_bytes())
        write_png(image, args.output_png)
        print(f"decoded: {image.width}x{image.height}, {len(image.palette) // 3} colors -> {args.output_png}")
    else:
        image = read_indexed_png(args.input_png.read_bytes())
        resource = encode_resource(image)
        args.output_resource.parent.mkdir(parents=True, exist_ok=True)
        args.output_resource.write_bytes(resource)
        print(f"encoded: {image.width}x{image.height}, {len(image.pixels)} indexed pixels -> {len(resource)} bytes")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(_main())
    except (OSError, ValueError, zlib.error) as exc:
        print(f"logo.py: {exc}", file=sys.stderr)
        raise SystemExit(2)
