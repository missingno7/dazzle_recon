# DAZZLE.EXE 5.0 packed format findings

## Packed image and appended tail

The pinned input is 158,892 bytes (SHA-256 `fc68905570e0f139cafc0bc0f75af87445c6fb29f53f5f02018fa9b71f714ee8`). Its MZ header has `e_cblp=0x0101` and `e_cp=0x009e`, giving a standard MZ extent of `(158-1)*512+257 = 80,641` bytes (`0x13b01`). The remaining 78,251 bytes are appended data, from file offset `0x13b01`, SHA-256 `878315d83c1f3d24399e768d4ee71d031e5bc930f08a7261b44a22b29a2e57b8`.

The tail is opaque binary data rather than a standalone named archive: all 256 byte values occur, it contains no useful readable strings, and it has no visible `-lh?-` LHA method tag. It begins `84 00 6d bd c7 f4 01 e8 03 02 88 bf 4b 01 00 ...`. The application code identifies an LZH extraction path and a fixed output name, `DAZZLE.DOC`, so the tail is best classified as an embedded, custom-framed LZH resource stream. The ten-byte frame fields are not decoded here.

## How the program locates and reads the resource

The DOS startup code copies its startup pathname into DGROUP:5d17. A DOSBox-X memory snapshot after startup showed `C:\DAZZLE_P.EXE` there when the scratch copy had that name. In the extraction routine, load-image offset `0x1b94f` passes far pointer `29bf:5d17` and mode `rb` (DGROUP offset `0x405f`) to the Borland stdio open path. The output path is far pointer `29bf:4040`, which points to `DAZZLE.DOC` at load-image offset `0x2dc30`; it is opened with mode `wb` at DGROUP offset `0x4062`.

The stream base is computed from the executable's MZ header. At load-image offset `0x121d3`, the program reads the first three 16-bit header words, then calculates `(e_cp-1)*512 + e_cblp + 1` into DGROUP:5d68/5d6a. For this packed file that is `80,642` (`0x13b02`), one byte past the standard MZ extent. Thus the tail's leading byte `0x84` is skipped; the following ten bytes form the frame. At `0x1b9c3` the extraction routine adds ten to the stored base, seeks there through the stdio path at `0x47f4`, and reads a four-byte text size through `0x46e4`. At tail-relative offset 11 those four bytes are `bf 4b 01 00`, or 84,927 bytes. The compressed portion then begins at tail-relative offset 15 and occupies 78,236 bytes. The LZH decoder and its error/report strings are at load-image offsets `0x1b560` onward and `0x2dc3b` through `0x2dcc3` (`getLZH data missing`, file access/seek/read errors, `Extracting`, and `Extracted`).

The Borland RTL open wrapper at `0x4f64` issues DOS `INT 21h/AH=3Dh`; the seek wrapper at `0x1552` issues `INT 21h/AH=42h`. The extraction routine calls the stdio layer rather than those wrappers directly. The resource path is statically tied to the executable name buffer and the calculated MZ end; the ten-byte frame's internal fields and the compressed stream's exact LZH variant remain unidentified.

## Unpacked file placement

The reconstructed MZ image is 211,302 bytes (`0x33966`) including its 9,760-byte relocation header and 201,542-byte load image. `DAZZLE_U.EXE` preserves the 78,251-byte tail verbatim immediately after that new MZ extent. Its recalculated MZ boundary is also one byte before the resource frame, so the code's `+1` base rule reaches the preserved tail at the corresponding relative position.

## Runtime checks

At the packed stub's recovered entry (`0823:0000`), the DOSBox-X acceptance dump matched the decoded image after applying all 2,431 relocations for load segment `0x0823`: 201,542 bytes compared, zero differences. The unpacked MZ also reached that entry with an identical relocated image in a separate acceptance run.

The requested short smoke run with `dazzle -a -m -b -w` did not match: at a 3.2-second capture, the packed executable displayed its graphic, while `DAZZLE_U.EXE` had returned to the DOS prompt. The unpacked executable did reach the application entry; the reason for the different subsequent behavior remains unresolved. Captures are `build/workers/unpack/final-packed-screen.png` and `build/workers/unpack/final-unpacked-screen.png`.
