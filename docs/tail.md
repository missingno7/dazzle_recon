# DAZZLE appended tail

## Tail layout

The appended area is 78,251 bytes, beginning at file offset `0x13B01`. It contains a one-byte separator, a ten-byte metadata frame, a four-byte manual length, the LZH stream, and a separate DLOGO image resource:

| Tail offset | Size | Meaning |
|---:|---:|---|
| `0x00` | 1 | `84` separator byte; the executable's resource-base formula skips it |
| `0x01` | 4 | self-check CRC, little-endian `C7BD6D00` |
| `0x05` | 2 | `r1=500` |
| `0x07` | 2 | `r2=1000` |
| `0x09` | 2 | `image_data_offset=0x8802` |
| `0x0B` | 4 | uncompressed manual length, `84,927` |
| `0x0F` | 34,814 | LZH-compressed `DAZZLE.DOC` |
| `0x880D` | 43,422 | serialized DLOGO image resource |

The manual decoder consumes 34,814 bytes to produce the 84,927-byte `DAZZLE.DOC`. The frame's final word gives the image boundary: `src/dlogo.c` reads it as `image_data_offset`, then seeks to `resource_base + 10 + image_data_offset` before reading the image header. Thus `0x8802 = 4 + 34,814`: the four-byte manual length field plus the compressed stream. The image header starts at absolute file offset `0x1C30E`.

The preceding `84` is at the standard MZ end, file offset `0x13B01`. `read_mz_resource_base` in `src/dosi.c` computes `(e_cp-1)*512 + e_cblp + 1`, which is `0x13B02` for this file; metadata readers seek there. `genCRC` processes `base-1` bytes, so the CRC covers the 80,641-byte MZ and excludes the separator. Its separate purpose remains unassigned.

`deRead` loads the frame into contiguous fields: `expected_crc`, `serial_r1`, `serial_r2`, and `image_data_offset` (`DS:6027`, `602B`, `602D`, `602F`). `print_order_info` displays `r1` and `r2` as registration/order ID codes. No code that writes or assigns these values was found; they are read from the frame. The final word is assigned meaning, not reserved padding.

## LZH manual codec

The reconstructed decoder in `src/dhuf.c` and encoder/decoder in `tools/lzhuf.py` use the LZHUF-family LZSS plus adaptive-Huffman format. The original stream confirms these parameters:

- Dictionary window `N=4096`; lookahead `F=60`; threshold `2`, so matches encode lengths 3 through 60.
- The ring begins at `r=N-F=4036`; bytes `0..4035` start as ASCII spaces. Initial nodes are inserted from `r-1` through `r-F`, followed by `r`.
- Match roots use the first byte and a folded XOR of the next two: `N+1+key[0]+(((key[1]^key[2])^((key[1]^key[2])>>4))&15)*256`. Equal-length matches select the smaller backward distance.
- Adaptive Huffman has 314 symbols (`0..255` literals and `256..313` for match lengths 3..60), 627 tree nodes, and root 626. Initial leaf frequencies are one; the tree is rebuilt at root frequency `0x8000`.
- Match positions encode the upper six bits with a fixed prefix and append the low six bits. Prefix classes are 0/3 bits, 1-3/4 bits, 4-11/5 bits, 12-23/6 bits, 24-47/7 bits, and 48-63/8 bits. Bits are emitted and consumed most-significant first; the decoder's eight-bit position lookup is a logical bit window and may start partway through a file byte.

`tools/crc.py` implements the separate non-reflected executable CRC: polynomial `0x00102100`, initial state zero, no final XOR. `tools/mktail.py` regenerates the manual stream, metadata frame, and image resource. The default image input is decoded from the hash-pinned original distribution resource into indexed pixels and re-encoded; callers can supply an alternate indexed PNG with `--logo-png`.

## Embedded DLOGO image format

The 43,422-byte resource is consumed by the 17F5/DLOGO module (`load_logo_image`, `prepare_logo_image_resources`, `decode_logo_run`, `prepare_logo_chunk`, `dim_logo_pixels`, and `configure_logo_dac`). It has a 128-byte header and 43,294 bytes of encoded image data. The table records the original header values; unused and reserved bytes are zero.

| Header offset | Size | Value and meaning |
|---:|---:|---|
| `0x00` | 4 | `0A 05 01 01`: PCX manufacturer 10, version 5, RLE encoding, 1 bit per plane |
| `0x04` | 8 | xmin=0, ymin=0, xmax=639, ymax=349; dimensions 640x350 |
| `0x0C` | 4 | horizontal and vertical resolution: 640, 350 |
| `0x10` | 48 | 16 RGB palette entries, in index order below |
| `0x40` | 1 | reserved, zero |
| `0x41` | 1 | four color planes |
| `0x42` | 2 | 80 bytes per plane per scanline |
| `0x44` | 2 | palette information, zero |
| `0x46` | 4 | custom resource byte size: `0xA99E` = 43,422 |
| `0x4A` | 2 | frame count: zero (static image) |
| `0x4C` | 52 | reserved, zero |

The palette contains these VGA-style 8-bit RGB values:

| Index | RGB | Index | RGB | Index | RGB | Index | RGB |
|---:|---|---:|---|---:|---|---:|---|
| 0 | `000000` | 1 | `0000AA` | 2 | `00AA00` | 3 | `00AAAA` |
| 4 | `AA0000` | 5 | `AA00AA` | 6 | `AA5500` | 7 | `AAAAAA` |
| 8 | `555555` | 9 | `5555FF` | 10 | `55FF55` | 11 | `55FFFF` |
| 12 | `FF5555` | 13 | `FF55FF` | 14 | `FFFF55` | 15 | `FFFFFF` |

The image body is one continuous PCX-style RLE stream, which expands to `350 * 4 * 80 = 112,000` bytes. Each scanline consists of plane 0 (80 bytes), plane 1, plane 2, then plane 3. Each plane byte covers eight horizontal pixels, most-significant bit first; the four plane bits form the indexed pixel value, with plane 0 as index bit 0. A byte with top two bits `11` (`0xC0..0xFF`) contains a repeat count in its low six bits and is followed by the repeated value. All other bytes are literals. The original uses repeat counts 1 through 63; a lone literal `>=0xC0` is represented as a count-one run.

The encoder greedily selects maximal runs up to 63 and restarts run matching at each 320-byte four-plane scanline boundary, although the decoder reads the stream continuously. This run selection reproduces the original body exactly. `tools/logo.py` maps the planar stream to indexed pixels and reads/writes indexed PNG with only the Python standard library and `zlib`. The decoded artwork is saved to `build/workers/dz-tail/logo.png`; it is copyrighted and is not committed.

## Reproduction evidence

Using `assets/DAZZLE.EXE` only as the immutable oracle:

```powershell
python -I tools/crc.py build/workers/dz-tail/original-packed-mz.exe
python -I tools/lzhuf.py decode build/workers/dz-tail/original-manual.lzh build/workers/dz-tail/decoded.doc --length 84927
python -I tools/lzhuf.py encode assets/DAZZLE.DOC build/workers/dz-tail/encoded-manual.lzh
python -I tools/logo.py decode build/workers/dz-tail/original-image-resource.bin build/workers/dz-tail/logo.png
python -I tools/logo.py encode build/workers/dz-tail/logo.png build/workers/dz-tail/reencoded-image-resource.bin
python -I tools/mktail.py build/workers/dz-tail/original-packed-mz.exe assets/DAZZLE.DOC build/workers/dz-tail/rebuilt-tail.bin
```

Results: CRC `C7BD6D00`; decoded manual equals `assets/DAZZLE.DOC` byte-for-byte; encoded LZH is 34,814 bytes and equals the original stream (first difference: none); logo round-trip is 43,422 bytes and equals the original resource (first difference: none); regenerated tail is 78,251 bytes and equals the original tail (SHA-256 `878315d83c1f3d24399e768d4ee71d031e5bc930f08a7261b44a22b29a2e57b8`). Detailed scratch artifacts and comparisons are in `build/workers/dz-tail/`.

Negative results: using the vanilla first-byte-only LZHUF tree as matcher first differed at compressed byte `0x5E6`; the alternative tree produced 332 different tied match positions. Adding DAZZLE's folded second/third-byte root hash produces an exact stream. For DLOGO, RLE-encoding all planes as one region first differed at image-body byte `0x2F`; resetting runs per 80-byte plane first differed at `0x0F`. Resetting at each 320-byte four-plane row matches exactly. The final 43,422 bytes are not manual data: the DLOGO loader addresses them using `image_data_offset`.

`tools/pack.py` regenerates the tail by default after PKLITE produces the packed MZ, and checks the result against the immutable original. `--original-tail-fallback` retains the explicitly labeled verbatim-tail fallback.
