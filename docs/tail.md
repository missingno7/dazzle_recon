# DAZZLE appended tail

## Tail layout

The appended area is 78,251 bytes, beginning at file offset `0x13B01`. It is a one-byte separator, a ten-byte metadata frame, a four-byte manual length, the LZH stream, and a separate serialized image resource:

| Tail offset | Size | Meaning |
|---:|---:|---|
| `0x00` | 1 | `84` separator byte; the application skips it |
| `0x01` | 4 | self-check CRC, little-endian `C7BD6D00` |
| `0x05` | 2 | `r1=500` |
| `0x07` | 2 | `r2=1000` |
| `0x09` | 2 | `image_data_offset=0x8802` |
| `0x0B` | 4 | uncompressed manual length, `84,927` |
| `0x0F` | 34,814 | LZH-compressed `DAZZLE.DOC` |
| `0x880D` | 43,422 | embedded serialized image resource |

The commonly reported 78,236-byte “compressed manual” range (`0x0F` through EOF) includes the 43,422-byte image resource. The decoder consumes exactly the first 34,814 bytes for the 84,927-byte manual. The frame's final word proves the boundary: `src/dlogo.c` reads it as `image_data_offset`, then seeks to `resource_base + 10 + image_data_offset` before reading the embedded image header. Thus `0x8802 = 4 + 34,814`: the four-byte manual length field plus the compressed stream. The image header starts at absolute file offset `0x1C30E`.

The preceding `84` is at the standard MZ end, file offset `0x13B01`. `read_mz_resource_base` in `src/dosi.c` computes `(e_cp-1)*512 + e_cblp + 1`, which is `0x13B02` for this file; metadata readers seek directly there. `genCRC` is passed that base and processes `base-1` bytes, so the CRC covers the 80,641-byte MZ and excludes the `84` separator. The value is not consumed by the reconstructed readers; “separator” describes its position, while its specific purpose remains unassigned.

`deRead` loads the ten-byte frame into contiguous fields: `expected_crc`, `serial_r1`, `serial_r2`, and `image_data_offset` (`DS:6027`, `602B`, `602D`, `602F`). `print_order_info` displays `r1` and `r2` as registration/order ID codes. No code that writes or assigns these values was found; they are read from the frame. The trailing word is therefore assigned meaning, not reserved padding.

## Codec

The recovered decoder in `src/dhuf.c` uses a Yoshizaki LZHUF-style LZSS plus adaptive-Huffman stream. Yoshizaki's LZHUF source is a useful structural reference ([LZHUF.C, e-n-f/lzss](https://github.com/e-n-f/lzss/blob/master/LZHUF.C)); DAZZLE's encoder match tree has a further root hash recovered from its `lzh_insert_node` routine.

- Dictionary window `N=4096`; lookahead `F=60`; threshold `2`, so matches encode lengths 3 through 60.
- The ring begins at `r=N-F=4036`; bytes `0..4035` are initialized to ASCII space. Initial nodes are inserted from `r-1` through `r-F`, followed by `r`.
- Match roots use the first byte and a folded XOR of the next two: `N+1+key[0]+(((key[1]^key[2])^((key[1]^key[2])>>4))&15)*256`. Equal-length matches select the smaller backward distance. This matches the original stream; prefix-length caching in the recovered C routine only avoids repeated comparisons.
- Adaptive Huffman has 314 symbols (`0..255` literals and `256..313` for match lengths 3..60), 627 tree nodes, and root 626. Initial leaf frequencies are one; the tree is rebuilt at root frequency `0x8000`.
- Match positions encode the upper six bits with a fixed prefix and append the low six bits. Prefix classes are 0/3 bits, 1–3/4 bits, 4–11/5 bits, 12–23/6 bits, 24–47/7 bits, and 48–63/8 bits. Bits are emitted and consumed most-significant first; the decoder's eight-bit position lookup is a logical bit window and may start partway through a file byte.

`tools/lzhuf.py` implements both directions. `tools/crc.py` implements the program's separate non-reflected CRC: polynomial `0x00102100`, initial state zero, no final XOR. `tools/mktail.py` builds marker, frame, length, and the encoded manual, then adds the embedded image resource. Because the image payload is not part of `DAZZLE.DOC` or the packed MZ, the default image source is the hash-pinned original distribution; callers may pass an explicit payload with `--image-resource`.

## Reproduction evidence

Using `assets/DAZZLE.EXE` only as the immutable oracle:

```powershell
python -I tools/crc.py build/workers/dz-tail/original-packed-mz.exe
python -I tools/lzhuf.py decode build/workers/dz-tail/original-manual.lzh build/workers/dz-tail/decoded.doc --length 84927
python -I tools/lzhuf.py encode assets/DAZZLE.DOC build/workers/dz-tail/encoded-manual.lzh
python -I tools/mktail.py build/workers/dz-tail/original-packed-mz.exe assets/DAZZLE.DOC build/workers/dz-tail/rebuilt-tail.bin
```

Results: CRC `C7BD6D00`; decoded manual equals `assets/DAZZLE.DOC` byte-for-byte; encoded output is exactly 34,814 bytes and byte-equal to the original LZH segment (first difference: none); regenerated tail is exactly 78,251 bytes and byte-equal to the original tail (SHA-256 `878315d83c1f3d24399e768d4ee71d031e5bc930f08a7261b44a22b29a2e57b8`). Detailed scratch artifacts and comparisons are in `build/workers/dz-tail/`.

Negative result: using the vanilla first-byte-only LZHUF tree as the matcher is insufficient. `python -I` comparison against the original stream first differed at compressed byte `0x5E6`; the alternative tree produced 332 different tied match positions. Adding DAZZLE's folded second/third-byte root hash produced an exact stream. Treating all 78,236 bytes after the length field as LZH is also incorrect: the final 43,422 bytes are addressed by `image_data_offset` and read by the logo/image loader.

`tools/pack.py` now regenerates this tail by default and verifies it against the pinned original after PKLITE has produced the packed MZ. `--original-tail-fallback` retains the previous verbatim-tail behavior and is explicitly labeled as a fallback. Regeneration does not remove the preserved-image-resource input dependency.

The corresponding claims in `docs/architecture.md` and `docs/format.md` predate this result: they call `0x8802` unassigned and describe the full 78,236-byte suffix as compressed manual data. They should be reconciled with this layout in a later documentation pass.
