# DAZZLE.EXE 5.0 packed format findings

## Packed image and appended tail

The pinned input is 158,892 bytes (SHA-256 `fc68905570e0f139cafc0bc0f75af87445c6fb29f53f5f02018fa9b71f714ee8`). Its MZ header has `e_cblp=0x0101` and `e_cp=0x009e`, giving a standard MZ extent of `(158-1)*512+257 = 80,641` bytes (`0x13b01`). The remaining 78,251 bytes are appended data, from file offset `0x13b01`, SHA-256 `878315d83c1f3d24399e768d4ee71d031e5bc930f08a7261b44a22b29a2e57b8`.

The tail is a custom-framed pair of resources, not an opaque archive. It begins `84 00 6d bd c7 f4 01 e8 03 02 88 bf 4b 01 00 ...`: one separator byte, a ten-byte metadata frame, a four-byte uncompressed manual length, 34,814 bytes of LZH data, and a 43,422-byte serialized DLOGO image. The manual expands to 84,927 bytes. The frame contains the packed-MZ CRC, two registration values, and the image-header offset.

## How the program locates and reads the resource

The DOS startup code copies its startup pathname into DGROUP:5d17. A DOSBox-X memory snapshot after startup showed `C:\DAZZLE_P.EXE` there when the scratch copy had that name. In the extraction routine, load-image offset `0x1b94f` passes far pointer `29bf:5d17` and mode `rb` (DGROUP offset `0x405f`) to the Borland stdio open path. The output path is far pointer `29bf:4040`, which points to `DAZZLE.DOC` at load-image offset `0x2dc30`; it is opened with mode `wb` at DGROUP offset `0x4062`.

The resource base is computed from the executable's MZ header. At load-image offset `0x121d3`, the program reads the first three 16-bit header words, then calculates `(e_cp-1)*512 + e_cblp + 1` into DGROUP:5d68/5d6a. For this packed file that is `80,642` (`0x13b02`), one byte past the standard MZ extent. Thus the tail's leading `0x84` byte is skipped; the following ten bytes form the frame: CRC `C7BD6D00`, `r1=500`, `r2=1000`, and `image_data_offset=0x8802`. At `0x1b9c3` the extraction routine adds ten to the stored base, seeks there through the stdio path at `0x47f4`, and reads a four-byte text size through `0x46e4`. At tail offset `0x0B` those bytes are `bf 4b 01 00`, or 84,927. The 34,814-byte LZH stream starts at tail offset `0x0F` (absolute `0x13b10`) and ends where the DLOGO resource starts at tail offset `0x880D` (absolute `0x1c30e`). The decoder and extraction strings are at load-image offsets `0x1b560` onward and `0x2dc3b` through `0x2dcc3`.

The Borland RTL open wrapper at `0x4f64` issues DOS `INT 21h/AH=3Dh`; the seek wrapper at `0x1552` issues DOS `INT 21h/AH=42h`. The extraction routine calls the stdio layer rather than those wrappers directly. The image loader in the 17F5/DLOGO module reads the final frame word as `image_data_offset`, seeks to `resource_base + 10 + image_data_offset`, and parses a 128-byte PCX-derived header. Its static resource is 640x350, 16-color, four 1-bit planes, with 80 bytes per plane per row; see `docs/tail.md` for the header, palette, planar layout, and run encoding.

## Unpacked file placement

The reconstructed MZ image is 211,302 bytes (`0x33966`) including its 9,760-byte relocation header and 201,542-byte load image. `DAZZLE_U.EXE` preserves the 78,251-byte tail verbatim immediately after that new MZ extent. Its recalculated MZ boundary is also one byte before the resource frame, so the code's `+1` base rule reaches the preserved tail at the corresponding relative position.

## Runtime checks

At the packed stub's recovered entry (`0823:0000`), the DOSBox-X acceptance dump matched the decoded image after applying all 2,431 relocations for load segment `0x0823`: 201,542 bytes compared, zero differences. The unpacked MZ also reached that entry with an identical relocated image in a separate acceptance run.

The requested short smoke run with `dazzle -a -m -b -w` did not match: at a 3.2-second capture, the packed executable displayed its graphic, while `DAZZLE_U.EXE` had returned to the DOS prompt. The unpacked executable did reach the application entry; the reason for the different subsequent behavior remains unresolved. Captures are `build/workers/unpack/final-packed-screen.png` and `build/workers/unpack/final-unpacked-screen.png`.
