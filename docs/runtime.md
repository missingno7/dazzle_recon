# Runtime identification

## Finding

The image is best explained by the **large memory model and the Borland C++ / Turbo C++ 3.0-era RTL**. The shipped BC++ 3.0 and Turbo C++ 3.0 candidates tie on the C0L startup code and library extent-hit totals; the available image evidence does not distinguish their product labels. BC++ 3.1 and BC++ 2.0 are poorer candidates. This is a runtime identification, not a strict byte-match claim.

The only runtime copyright marker found is `Borland C++ - Copyright 1991 Borland Intl.` at load-image offset `0x29BF4`, or `DS:0004` in DGROUP `29BF:0000`.

## Startup and model comparison

The comparison reads each shipped C0 object's emitted `_TEXT` bytes at image offset 0 and emitted `_DATA` at DGROUP offset 0. It masks OMF FIXUPP fields and ignores un-emitted holes; no additional MZ-relocation masking is applied. Counts are invariant bytes equal / invariant bytes compared. No candidate C0 object is a strict full-byte match.

| Candidate | C0S | C0M | C0C | C0L | C0H |
|---|---:|---:|---:|---:|---:|
| BC++ 2.0 | 74/447 | 78/450 | 146/406 | 155/409 | 136/400 |
| BC++ 3.0 | 81/535 | 75/535 | 258/486 | **484/486** | 138/477 |
| BC++ 3.1 | 43/588 | 49/588 | 33/539 | 36/539 | 39/530 |
| Turbo C++ 3.0 | 81/535 | 75/535 | 258/486 | **484/486** | 138/477 |

The two fixed-byte differences for BC++ 3.0 and Turbo C++ 3.0 C0L are at object offsets `0x014F` and `0x025B`: the library object has far-call opcodes (`9A`), while the linked image has the linker's same-segment rewrite (`90`, followed by `push cs; call near`). Their C0L object files are byte-identical. C0C is a distant second. The C0 `_DATA` fixed-byte comparison is 145/145 for C0C and C0L in the 3.0/3.1 candidates, 143/145 for C0H, and 85/166 for C0S/C0M; BC++ 2.0 C0C/C0L is 143/145. The image starts `BA BF 29`, loading DGROUP paragraph `0x29BF`; application code uses far calls while its globals are DS-relative, consistent with the large model.

This is a negative startup result for every other shipped model/release combination: none matches the linked startup code exactly. These comparisons use the toolchain’s supplied C0 objects, so no compile flags apply.

## Library scan

`tools/libmatch.py` parses the OMF library dictionary and member records, including LEDATA, expanded LIDATA, FIXUPP, PUBDEF, and external references. Its "full-extent hit" means every emitted byte outside FIXUPP fields in a complete code-segment extent agrees at the reported image offset. FIXUPP bytes and un-emitted holes are ignored. This evidence is **not labeled strict EXACT or RELOCATED** because not every relocation target is verified. Cross-member checks cover resolvable external near calls and far pointers; all checks listed below passed, while the remaining references lacked a unique matched target.

| Shipped candidate libraries | Full-extent member hits | Version-drift partials | Fragment-only partials | Hit extent bytes (sum; duplicates included) | External target checks pass / unresolved / mismatch |
|---|---:|---:|---:|---:|---:|
| BC++ 2.0 large | 70 | 13 | 44 | 7,266 | 4 / 120 / 0 |
| BC++ 3.0 large | **110** | 35 | 25 | **25,040** | 64 / 404 / 0 |
| BC++ 3.1 large | 100 | 26 | 38 | 21,067 | 12 / 401 / 0 |
| Turbo C++ 3.0 large | **110** | 35 | 25 | **25,040** | 64 / 404 / 0 |

Per-library full-extent hits (CL / MATHL / EMU / FP87 / GRAPHICS): BC++ 2.0 `65 / 1 / 2 / 2 / 0`; BC++ 3.0 and Turbo C++ 3.0 `98 / 6 / 4 / 2 / 0`; BC++ 3.1 `88 / 6 / 4 / 2 / 0`. FP87's two hits are duplicate EMU087/E87TRAP code; FP87INIT is fragment-only. The software floating-point emulator is therefore present through EMU.LIB. GRAPHICS.LIB has no full or partial code hits in any candidate, so the BGI graphics runtime is not indicated.

The 108 distinct full-extent module names in the 3.0 CL.LIB, MATHL.LIB, and EMU.LIB candidates are:

- **CL.LIB (98):** ATEXIT, CHDIR, CLEARERR, DOSFATTR, DOSFIND, DOSGDRIV, DOSVECT, F_LXMUL, F_SCOPY, GETCBRK, GETCURDI, GETDATE, GETSWIT, GETVECT, H_LDIV, H_LLSH, H_LRSH, H_LURSH, H_PADD, IOERROR, ISATTY, LONGTOA, LSEEK, N_LXMUL, N_PCMP, N_SCOPY, RAND, READA, REMOVE, SCANNER, SCANTOL, SEGREAD, SETDATE, SRCHPATH, TOLOWER, UNLINK, VPRINTER, VRAM, BIOSKEY, BIOS_KEY, COLOR, CPRINTF, CRTINIT, CVTFAK, DELAY, FARHEAP, GOTOXY, GPTEXT, INTR, IOCTL, KBHIT, MOVETEXT, PUTCH, SCANTOD, SCREEN, SETBLOCK, SOUND, WHEREXY, ATOL, BDOS, CHMODA, CLOSEA, CREATA, EOF, FDOPEN, FINDFIRS, FPRINTF, FWRITE, GETENV, MEMCPY, MEMSET, MOVMEM, OPENA, PRINTF, SSCANF, STRCAT, STRCHR, STRCPY, STRICMP, STRLEN, STRLWR, STRNCPY, STRSTR, STRUPR, WRITEA, FMEMCPY, FMEMSET, FMOVMEM, FSTRCAT, FSTRCHR, FSTRCPY, FSTRICMP, FSTRLEN, FSTRLWR, FSTRNCPY, FSTRSTR, FSTRUPR, FTOLOWER.
- **MATHL.LIB (6):** LDTRUNC, REALCVT, _POW10, POW10D, POW10L, FFTOL.
- **EMU.LIB (4):** FPINIT, EMU086, EMU087, E87TRAP.

For BC++ 3.0, the 35 aligned partials that indicate version drift are CL.LIB `EXIT, MKNAME, SETUPIO, STIME, CALLOC, FBRK, GETCH, HARDERR, INT86, SCROLL, SETENVP, SIGNAL, CLOSEALL, FCLOSE, FGETS, FLUSHALL, FOPEN, FREAD, GETC, OPEN, PUTC, READ, SETVBUF, SPLITPTH, SPRINTF, TIMECVT, TZSET, XFFLUSH, SEARCHP` and MATHL.LIB `SCANTOD, XCVT, FPERR, SIN, _MATHERR, COS`. Turbo C++ 3.0 has the same drift list. BC++ 2.0 drift partials are CL.LIB `CALLOC, CPRINTF, SPRINTF, STIME, TIMECVT, EOF, GETCH, HARDERR, INT86, TZSET, VRAM` and MATHL.LIB `REALCVT, SCANTOD`. BC++ 3.1 drift partials are CL.LIB `EXIT, IOERROR, STIME, CPRINTF, GETCH, GPTEXT, HARDERR, INT86, SCREEN, SCROLL, SETENVP, CLOSEALL, FGETS, FLUSHALL, FOPEN, FREAD, GETC, OPEN, READ, SPRINTF, TIMECVT, XFFLUSH` and MATHL.LIB `SCANTOD, XCVT, FPERR, _MATHERR`. These are aligned partial-byte runs, not full-extent hits. The 3.0 fragment-only partials are CL.LIB `CTRLBRK, STPCPY, TOUPPER, WILD, _STPCPY, SETARGV, CTIME, DIRECT, FFLUSH, FSEEK, GETCWD, LDIVT, MAKEPATH, STRDUP, STRRCHR, VSCANF, WRITE, XFCLOSE, FSTRDUP, FSTRRCHR, FTOUPPER`; MATHL.LIB `_MATHERL, SINL, COSL`; and FP87.LIB `FP87INIT`.

The matched CL modules cover console/BIOS and DOS calls, strings and memory helpers, and formatted/stream I/O (`PRINTF`, `FPRINTF`, `FWRITE`, `SSCANF`). The aligned stream-I/O partials (`FCLOSE`, `FOPEN`, `FREAD`, `FGETS`, `GETC`, `OPEN`, `PUTC`, `READ`, `SETVBUF`) are evidence of additional stdio-family code with version drift. Math conversion/power helpers match as full extents; SIN/COS are drift partials.

## Function inventory

`evidence/functions.json` covers load-image code offsets `0x00000..0x1E810`. It combines near and far call targets (including `push cs; call near`), BP/ENTER prologues, public RTL symbols, computed EMU opcode targets, and static/keyed jump-table targets. Its **42 data islands / 1,553 bytes** include the EMU086 dispatch table, EMU087 offset tables, compiler switch tables, and E87 constants/metadata; these are excluded from byte totals. Function `size`/`end` values are bounding spans, and entries whose span crosses an island carry `data_overlap_bytes` plus an `overlaps_data_island:` evidence tag.

| Class / measure | Count | Bytes |
|---|---:|---:|
| RTL entry starts / identified RTL code extents | 469 | 22,631 |
| RTL public-symbol starts | 153 | - |
| Application functions | 390 | 85,687 |
| Assembly candidates | 42 | 7,683 |
| Uncertain starts | 246 | 14,464 |
| Glue starts | 8 | 12 |
| Data islands inside code | 42 | 1,553 |

The RTL start count includes 84 unique EMU086 dispatch targets; these can be handler labels rather than separate ABI routines. The union byte totals collapse overlapping extents and exclude data islands. Application and uncertain spans are inferred from control flow, prologues, and dispatch targets and remain estimates.
