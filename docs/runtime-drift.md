# Segment 0 runtime residue

The reported **35 “version-drift” members are stock Borland 3.0 RTL transformed by TLINK**. Their 122 differing invariant bytes are 121 far-call opcodes (`9A -> 90`) and one far-jump opcode (`EA -> 90`). There are no unexplained invariant-byte changes in these members. A patched RTL or a custom rebuild is unnecessary to explain them.

All **100 application-classified entries (11,171 bounding-span bytes), nine assembly candidates, and eleven uncertain entries** in the segment-0 inventory lie in identified RTL contributions. None indicates a DAZZLE application module. The accompanying [per-entry evidence](../evidence/seg0-residue.json) covers all 378 inherited entries, including 252 already marked runtime and six embedded-data entries. These entries include internal labels; this does not establish 378 independent ABI functions.

This is **PROVISIONAL identification evidence**, not strict EXACT/RELOCATED acceptance. Across segment 0, 568 fixup checks pass and 558 remain unresolved, counting identical alternative members separately. Primarily data/group bindings still need independent proof. All 161 rewritten transfers have verified encodings and code targets; none is unresolved. Do not add these extents to strict accepted coverage.

The declared RTL contributions cover **`0000:0000..0000:66AA`**, continuously: 160 member alternatives at 143 distinct extents, 26,283 allocated bytes. The estimated `0x66A0` boundary cuts through SEARCHP's last call: `0000:669F = 90 0E E8 28 F3`, targeting `_strcpy` at `0000:59CC`; SEARCHP returns at `0000:66AA`. The following `mov ax,cs; retf` starts at `0000:66AB` / `066A:000B`. Relocation target paragraphs are addressing windows, not reliable contribution boundaries.

There are 66 declared but un-emitted storage bytes, all zero in the image: C0L `0260+2`; FPINIT `0264+52`, `02B8+4`; FARHEAP `2A0D+6`; SETARGV `37BC+2`. These are attributed storage, not compared emitted code. The six inventory data islands remain RTL tables, not application code.

The old scan's other gaps are also shipped members, including SQRT, CTRLBRK, GETDISK, TELL, STPCPY/_STPCPY, HARD, SETARGV, VIDINFO, CLOSE, FFLUSH, FNMERGE/FNSPLIT, FSEEK, GETCWD, MAKEPATH, WRITE, and XFCLOSE. The 16 bytes at `3469..3478` are CL.LIB REALCVT and SCANTOD jump thunks. Independent data placement resolves their initially ambiguous signatures: DGROUP `47EC..47F5` contains `79 05 4B 29 13 07 26 0A 1E 0A`, derived from the located MATHL REALCVT, CL CVTFAK, and MATHL SCANTOD vector contributions. Thus `__RealCvtVector = DS:47EC`, `__ScanTodVector = DS:47F0`; the thunks select those words and `+2`, `+4`.

## Why libmatch reported drift

`libmatch.py` masks a four-byte OMF pointer fixup, but leaves its preceding CALL/JMP opcode invariant. TLINK's same-segment conversion preserves the five-byte instruction extent:

| Object | Linked bytes | Meaning |
|---|---|---|
| `9A off16 seg16` | `90 0E E8 rel16` | NOP; PUSH CS; CALL near, preserving the far-return stack |
| `EA off16 seg16` | `90 90 E9 rel16` | Two NOPs; JMP near |

The displacement is relative to the end of the five-byte extent. The original segment-word MZ relocation disappears. C0L's changes at `014F` and `025B` use the same mechanism.

[rtldiff.py](../tools/rtldiff.py) locates complete declared code contributions, including emitted padding, relaxing an opcode only when the object has an associated POINTER16:16 fixup and opcode `9A`/`EA`. It then checks the entire replacement encoding, independently located target plus OMF displacement/addend, and removal of the segment relocation. Ordinary near/far references and known emulator absolute symbols are checked separately. Unresolved bindings are retained explicitly.

For the 35 legacy drift members, comparison against the independent checksum-validating `objcmp.Object` reader found **zero mask disagreements** with libmatch. There is no evidence that an unmasked linker-resolved operand caused any of those 122 invariant differences. Ordinary raw-object/image operand changes are OMF resolution; the JSON gives every differing byte a fixup or rewrite cause. For example, `FIDRQQ = 5C32` changes object word `D99B` (`9B D9`) to `35CD` (`CD 35`), an emulator interrupt substitution, not source-version drift.

Relaxing opcodes alone would introduce false identities. Target checks reject DEL/FFMALLOC/GFREEMEM at `30BA`, GEXIT/GGETMEM at `30CC`/`30DA`, and the swapped FNMERGE/FNSPLIT candidates at `4374`/`439F`. Vector bindings reject eleven additional thunk lookalikes. The final 3.0 audit has no ambiguous placements; identical alias members remain explicit alternatives.

An independent TASM 3.0 / TLINK 5.0 probe, with the external declared outside `_TEXT`, assembles `CALLER.OBJ` code as `9A 00 00 00 00 EA 00 00 00 00`. Linking a second object defining the target in `_TEXT` produces exactly `90 0E E8 05 00 90 90 E9 00 00 CB`. No byte patching is involved. Inputs and logs are in `build/workers/s-seg0/linkprobe/`. The linker has the catalog SHA-256 `ac0ac8b0084e06e9916ee12602aa4fcba998f7b702fc2adf4a213564288147c7`.

## Every legacy drift member

Offsets below are hexadecimal **within the member** and identify every differing invariant byte. Each is the opcode rewrite above; SETENVP `24` is the jump. Member placements, full extents, old/new fields, target checks, and 3.1 source counterpart line candidates are retained in JSON. Source line candidates identify counterpart occurrences, sometimes declarations or macro expansions; they are not claims to have recovered the exact original source line.

| Member | Image base | Full size | Differing invariant-byte offsets |
|---|---:|---:|---|
| CL.LIB EXIT | `1150` | 147 | `25, 2E, 33, 4D` |
| CL.LIB MKNAME | `157B` | 180 | `33, 4B, 9C` |
| CL.LIB SETUPIO | `1DEE` | 182 | `46, 73, 80, AD` |
| CL.LIB STIME | `1EB6` | 152 | `1A, 27, 33, 4B, 57, 68` |
| CL.LIB CALLOC | `259D` | 95 | `30, 4D` |
| CL.LIB FBRK | `2E39` | 323 | `48` |
| CL.LIB GETCH | `2F7C` | 92 | `33` |
| CL.LIB HARDERR | `30E8` | 294 | `76, A2, D4` |
| CL.LIB INT86 | `320E` | 202 | `B` |
| CL.LIB SCROLL | `362E` | 398 | `82, A8, E3, 114, 13A` |
| CL.LIB SETENVP | `38FF` | 88 | `D, 24` |
| CL.LIB SIGNAL | `3957` | 1073 | `51, C3, 135, 1A7, 219, 237, 2EC, 32D, 353, 36C, 38C, 3F1, 402` |
| CL.LIB CLOSEALL | `3F37` | 86 | `29` |
| CL.LIB FCLOSE | `407F` | 171 | `2A, 48, 5F, 96` |
| CL.LIB FGETS | `4236` | 156 | `4F` |
| CL.LIB FLUSHALL | `4332` | 66 | `27` |
| CL.LIB FOPEN | `43CA` | 522 | `F7, 127, 15B, 16D` |
| CL.LIB FREAD | `45F0` | 386 | `74, CF` |
| CL.LIB GETC | `4957` | 456 | `27, 6E, 15B, 170` |
| CL.LIB OPEN | `4DC8` | 412 | `52, C9, ED, FF, 127, 159` |
| CL.LIB PUTC | `4FCE` | 962 | `12, 6C, BA, 10B, 144, 16D, 18C, 232, 262, 27A, 2B1, 2D1, 310, 328, 3A8` |
| CL.LIB READ | `5390` | 209 | `42, 8F, B4` |
| CL.LIB SETVBUF | `5461` | 283 | `67, 82, CE` |
| CL.LIB SPLITPTH | `557C` | 727 | `15, 2D, 4A, 118, 140` |
| CL.LIB SPRINTF | `5853` | 131 | `18` |
| CL.LIB TIMECVT | `5B3C` | 814 | `7, 12D` |
| CL.LIB TZSET | `5E6A` | 715 | `C, 26, D6, EB, 105, 11F, 13A, 178, 1C1` |
| CL.LIB XFFLUSH | `62FD` | 56 | `23` |
| CL.LIB SEARCHP | `6335` | 886 | `63, C7, 155, 1AA, 1ED, 36A` |
| MATHL.LIB SCANTOD | `0713` | 917 | `247` |
| MATHL.LIB XCVT | `0AA8` | 520 | `F9, 117, 144` |
| MATHL.LIB FPERR | `0CB0` | 175 | `9E, A6` |
| MATHL.LIB SIN | `0E75` | 80 | `46` |
| MATHL.LIB _MATHERR | `0EC5` | 169 | `83` |
| MATHL.LIB COS | `0F6E` | 80 | `46` |

## Reproducing candidate and release limits

Use the **unmodified large-model BC++ 3.0 C0L.OBJ, CL.LIB, MATHL.LIB, and EMU.LIB**, with TLINK 5.0's normal same-segment far-transfer conversion. The installed paths and full input hashes are in the JSON and [toolchain acquisition notes](toolchain-acquisition.md). The library SHA-256 values are:

| Input | SHA-256 |
|---|---|
| CL.LIB | `fb0c553665e662171332c2cfd2b8645ba9a050f518d4c862ba0088f0c19c67d6` |
| MATHL.LIB | `473c2a4770725ce26b9fe124fdc76d4f7138c1f2d125207c11854d478ad411f2` |
| EMU.LIB | `af793e19c28bec1ed4326ae0a4ab0cbbdc46406e8d47dcfe52702dd912bdc95b` |
| C0L.OBJ | `90cbc3d90e412d03ad4f32ad452cb744e3b7e80e7ab5939b48ea7025960e9918` |

Turbo C++ 3.0 is an equally good code candidate. Its MATHL/EMU/C0L inputs are identical. Of 588 CL members, only DOSDFREE, EXEC, SEARCHP, and UTIME have different complete object bytes; **only EXEC differs in emitted contents**. The TC++ EXEC object is byte-identical to the downloaded BC30P1 `EXEC.OBL` (SHA-256 `889681ddd1c50dc4e6c9fc22cb61f67feda63400327723acf82fff57cc6af763`). EXEC is not located in DAZZLE, so its presence in a library cannot determine DAZZLE's product label or patch level. Both 3.0 scans give the same 163 located contribution alternatives across the image, including the three separate EMU/math-region contributions. This does not identify the compiler used for DAZZLE's own source.

The locally supplied 3.1 sources are more extensive than the earlier “95 files” description: **CLIBSRC plus CLIBSRC1/2/3 contain 376 files**. Every one of the 35 drift members has a counterpart in these packages or MATHSRC. They are references for source meaning, not evidence that DAZZLE used the 3.1 library.

Two rebuilds directly test that distinction. BC++ 3.0 `BCC.EXE` (`e56c6d228191e33665802e3affd9f44803af7c19c0c5a591c28ad10ffcdada6f`), with the 3.1 RTLINSRC internal headers and 3.1 public headers, compiles 3.1 SETUPIO.C and XFFLUSH.C to **182-byte and 56-byte code contributions whose raw code, code fixup records, and external lists equal the stock 3.0 objects**. Flags come from the supplied RTL configuration:

```text
BCC.EXE -a- -c -ml -k- -O1 -Vs -w -w-asm -X -zC_TEXT -zR_DATA -zTDATA -IINCLUDE SETUPIO.C
BCC.EXE -a- -c -ml -k- -O1 -Vs -w -w-asm -X -zC_TEXT -zR_DATA -zTDATA -IINCLUDE XFFLUSH.C
```

SETUPIO also gives the identical code/fixups with 3.0 public headers. Its four affected calls correspond to SETUPIO.C lines 34/36/37/39 (`isatty`, `setvbuf`, `isatty`, `setvbuf`); XFFLUSH's call is line 36. Thus a 3.1 source rebuild can reproduce these unchanged routines, but is unnecessary to explain the image. The whole 3.1 RTL was not rebuilt. Source inputs, commands, compiler logs, and object comparisons are under `build/workers/s-seg0/rebuild/`, with results also embedded in JSON.

## Patch investigation and negative results

Downloaded and CRC-extracted the original Borland-authored [BC30P1](https://ftp.zx.net.nz/pub/archive/ftp.icm.edu.pl/pub/coast/msdos/borland/bc30p1.zip), [BC30P2](https://ftp.zx.net.nz/pub/archive/ftp.icm.edu.pl/pub/coast/msdos/borland/bc30p2.zip), and [BC30P3](https://ftp.zx.net.nz/pub/archive/ftp.icm.edu.pl/pub/coast/msdos/borland/bc30p3.zip) packages into `C:\tools\download\bc30-patches\`. Their included README.PAT files and patch contents establish: P1 replaces EXEC to preserve DX; P2 patches BC/BCC/TCW initialization code; P3 replaces DPMILOAD for DesqView compatibility. None supplies a replacement for any drifted member. No patched installation is needed for this finding, and the original installations were left untouched.

The [WinWorld “3.0 Patch Disk”](https://winworldpc.com/download/21f25044-d12e-11eb-9de1-0200008a0da4) was also acquired; its published SHA-1 `f3ab5844c4c216bef599ff089fec71ec0a5027f0` matches. The disk contains only TCW.EXE, IMPLIBW.EXE, WINSIGHT.EXE, and WSIHOOK.DLL, not a DOS RTL update. Archive hashes, URLs, and file lists are in JSON. It does not explain the residue.

Other recorded negatives: stock BC++ 3.1 C0L fails the invariant-byte comparison; the 3.1 libraries leave substantially more residue even with rewrite-aware location (126 uniquely located contributions versus 163 for either 3.0 candidate). Rebuilding XFFLUSH with 3.0 public headers and 3.1 internal headers fails because `_nfile.h`/`_NFILE_` are absent; 3.1 public headers resolve that dependency. In the initial linker probe, putting EXTRN inside `_TEXT` let TASM itself emit short transfers, so that variant did not test TLINK rewriting; the outside-segment EXTRN variant above does.

## Rechecking

```powershell
python -I tools/rtldiff.py --out build/workers/s-seg0/audit.json
python -I tools/rtldiff.py --root C:/tools/turbo-c++-3.0/files/LIB --out build/workers/s-seg0/tc30.json
python -I tools/rtldiff.py --root C:/tools/borland-c++-3.1/files/LIB --out build/workers/s-seg0/bc31.json
```

The first two report 163 unique contribution locations, 20 rejected candidates, 161 passing rewrites, and zero unexplained fixed differences. Their full-image fixup totals are 763 pass / 777 unresolved; the segment-0 subset is 568 / 558. These sums include alias alternatives. The audit checks the image hash against `evidence/oracle.json` before proceeding. `build/workers/s-seg0/deliver.py` generated the per-entry evidence; its assertions require continuous declared coverage, all 35 legacy drift identities, and passing rewrite checks.

The next proof obligation is independent data/group placement and complete natural linking, including zero-fill policy and all relocation bindings. No change to `evidence/functions.json`, accepted-match ledgers, or the existing tools was made by this task.
