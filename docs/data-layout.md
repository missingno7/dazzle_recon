# DAZZLE data ownership census

[dgroup.json](../evidence/dgroup.json) partitions **77,248 physical bytes** into
529 disjoint, contiguous ranges: DGROUP `29BF:0000..79E0` and the far window
`1E81:0000..B3E0`. All intervals in this document are hexadecimal and **half-open**.
The oracle is the unchanged 201,542-byte load image, SHA-256
`3df021b0416039df3b48b03362d223f5f358fd951d18316845624fd304030d8a`.

This is **PROVISIONAL identification**, with UNKNOWN owners for five bytes;
there are **zero new accepted EXACT/RELOCATED bytes**. Application contribution
ownership is a proposal from contiguous placement, code references and compiler
rules, not recovered original PUBDEFs. A consumer of a shared global does not
necessarily define it. Residual `initialized_storage` ranges have an attributed
contribution but unresolved variable/constant boundaries. `symbols` are point
annotations and do not imply sizes. Confidence describes the evidence attached
to each boundary/name; it does not upgrade the proof level.

## Extents and stack

| DGROUP offsets | Interpretation |
|---|---|
| `0000..47EC` | Combined `_DATA`, including contribution padding |
| `47EC..47FC` | `_CVTSEG` and `_SCNSEG` vectors |
| `47FC..4826` | Seven six-byte `_INIT_` records |
| `4826..482C` | FPINIT's six-byte `_EXIT_` record |
| `482C..78C3` | Startup-cleared `_BSS` |
| `78C3..78D0` | Paragraph alignment, outside the serialized image |
| `78D0..79E0` | Static `_STACK`, load-image paragraph `314C` |

The old 30,550-byte initialized-data estimate is the serialized DGROUP suffix,
ending at **`7756`**, not the linker `_DATA` size. It includes 12,074 bytes of
startup-cleared BSS. Another 365 BSS bytes, 13 alignment bytes and 272 stack bytes
lie beyond the image. C0's clear bounds and its initialized `___brklvl` value
independently agree on `78C3`. Packed MZ minalloc is not used to invent an extent.

C0L declares a 128-byte STACK contribution; MATHL **EMUVARS declares a 272-byte
COMMON contribution with the same `_STACK` name/class**, and exports `__emu` at
zero. FPINIT's relocated segment operand at `0000:02AD` resolves to `314C`, while
its OFFSET references leave `__emu` at zero. A fresh TLINK 5.0 floating-point
probe produces one `0110`-byte `_STACK`, with `__emu` at zero, not concatenated
`0080+0110` allocations. The partition attributes the first 128 bytes to C0L and
the extension to EMUVARS; `stack_overlay` records EMUVARS' full overlapping extent.
Entry `SS:SP=314C:0080` is consistent with this arrangement. C0 moves SP before
emulator initialization. The application overrides `__stklen` at DS:`0094` with
`3000`; that runtime stack request is distinct from the static stack reservation.

## Application contributions

| Code window / module | Proposed `_DATA` | Proposed near `_BSS` | Far storage in frame `1E81` |
|---|---|---|---|
| `066A` / DZLE, main and Alt images | `0094..0A38` | `482C..6031` | None indicated |
| `0B84` / DZL2, Ctrl images | `0A38..0A50` | `6032..60F8` | None indicated |
| `0CCD` / DZFD, fades | `0A50..0BFE` | `60F8..7438` | None indicated |
| `11C1` / DOSI | `0BFE..249D` | `7438..7452` proved minimum; next four bytes unresolved | None indicated |
| `158D` / menus | `249E..39A4` | No separate contribution indicated | `0000..0259` |
| `17F5` / DLOGO | `39A4..3BC6` | `7456..7480` | None indicated |
| `1899` / DVDT, TASM | `3BC6..3C66` | `7480..7756` | None indicated |
| `19F0` / DAZPCX | `3C66..3D66` | `7756..7766` | None indicated |
| `1A72` / DHUF | `3D66..40F5` | No near contribution indicated | `025A..B3E0` |

These are data contribution boundaries, not code paragraph boundaries. Shared
callbacks/options/palette state at `5C33..6031` are provisionally in main's BSS,
even when DOSI/DVDT supplies most accesses. The private reference bands progress
in module/link order. In particular, DZL2 accesses the word at `60F6`, so its BSS
cannot end at `60F6`; fades begin at `60F8`. DOSI's 20-byte REGPACK begins `743E`
and reaches `7452`; first established DLOGO storage is `7456`. Leave that gap open.
Padding `6031..6032` is proposed from the next WORD-aligned contribution.

The main TU contains **all** Alt, Ctrl and fade definition records:
`01FE..041A` (36 x 15), `041A..0474` (6 x 15), `0474..051C` (24 x 7).
Their 66 names are in its pool, `070B..091D`; the callback's code module does not
own the record/name merely because the callback points there. Other anchors:

- Main strings start `051C`; numeric pool `06AF..070B` precedes the image names.
  DOSI's initialized freeze flags, BIOS pointer and CRC polynomial begin at
  `0BFE`, `0C00`, `0C02`, `0C06`; its text begins at `0C18`.
- Menus own the five 9-byte help descriptors at `24B8`, five configuration
  descriptors at `24E5`, and text beginning `2535`.
- DVDT has mode/edge state `3BC6..3BCA`, six DOS `$`-terminated messages through
  `3C5F`, disk state through `3C66`, and 350 row-offset words `749A..7756`.
- DHUF has two 256-byte position tables `3D66..3F66`, bit input/output state and
  two 17-word tables through `3FB0`, then a filename pointer and text.

The JSON lists **254 application strings** separately, retaining short/duplicate
strings, original CP437 content, and `$` terminators. 233 have recorded direct
pointer references from code or relocated data. `literal_order` gives their
storage order and original reference sites; code references are a deliberately
limited static instruction-pattern census, not an exhaustive disassembly.
Numeric pool cells and unresolved initialized storage remain separately typed.
Candidate `objcmp.segments` bindings are preserved with their source/function:
for example DOSI `DS:0F23` and `DS:200F` locate a *reconstructed object's* pool,
not the original TU's `_DATA` beginning.

## Stock RTL contributions

The checksum-validating OMF reader loads C0L plus stock BC++ 3.0 CL/MATHL/EMU.
Located code contributions from [seg0-residue.json](../evidence/seg0-residue.json)
seed a constraint pass over segment and external FIXUPPs. DGROUP-framed OFFSETs
subtract the object's addend/displacement; references must agree modulo 16 bits.
Data-only members enter through independently referenced PUBDEFs. No expected
bytes come from a candidate application build.

All **1,821 emitted invariant data bytes agree**, and all **40 data fixups**
resolve to the original operands with the required MZ relocations. This is a
placement cross-check, not strict acceptance: the seed code identities and
inferred placement dependencies retain PROVISIONAL status. Zero/un-emitted BSS
alone is not identifying evidence. The JSON retains 175 participating object
alternatives, every declared non-code segment (including zero-length segments),
publics, library pages/hashes, constraints and data comparison results.

| Library/member | `_DATA` interval | `_BSS` interval |
|---|---|---|
| C0L | `0000..0093` | Empty |
| EMU/FPINIT | `4100..4102` | Empty |
| EMU/PROTFLAG | `4110..4113` | Empty |
| EMU/DEFLT87 | `4114..4116` | Empty |
| MATHL/HUGEVAL | `4116..413A` | Empty |
| MATHL/MATHWHY | `413A..417D` | Empty |
| MATHL/SCANTOD | `417E..4196` | Empty |
| MATHL/FPERR | `4196..4230` | Empty |
| MATHL/_POW10 | `4230..42B0` | Empty |
| MATHL/SQRT | `42B0..42BD` | Empty |
| MATHL/SIN | `42BE..42CA` | Empty |
| MATHL/_MATHERR | `42CA..42D8` | Empty |
| MATHL/COS | `42D8..42E4` | Empty |
| CL/ATEXIT | `42E4..42E6` | `7766..77E6` |
| CL/CTYPE | `42E6..43E7` | Empty |
| CL/CTRLBRK | Empty | `77E6..77EA` |
| CL/EXIT | `43E8..43F4` | Empty |
| CL/FILES | `43F4..4584` | Empty |
| CL/FILES2 | `4584..45AE` | Empty |
| CL/FMODE | `45AE..45B2` | Empty |
| CL/FPSTKLEN | `45B2..45B4` | Empty |
| CL/IOERROR | `45B4..460F` | Empty |
| CL/MKNAME | `4610..4619` | `77EA..77F7` |
| CL/RAND | `461A..461E` | Empty |
| CL/SCANNER | `461E..469E` | Empty |
| CL/VPRINTER | `469E..4705` | Empty |
| CL/WSCROLL | `4706..4708` | Empty |
| CL/CRTINIT | `4708..4720` | Empty |
| CL/CVTFAK | `4720..4751` | Empty |
| CL/DELAY | `4752..4756` | Empty |
| CL/FBRK | `4756..4758` | Empty |
| CL/KBHIT | `4758..475A` | Empty |
| CL/SETARGV | `475A..476A` | Empty |
| CL/SETENVP | `476A..476E` | Empty |
| CL/SIGNAL | `476E..4795` | `7808..7810` |
| CL/PUTC | `4796..4797` | `7812..7813` |
| CL/SETVBUF | `4798..479C` | Empty |
| CL/TIMECVT | `479C..47A8` | Empty |
| CL/TZSET | `47A8..47DB` | `7814..781C` |
| CL/SEARCHP | `47DC..47EB` | `781C..78C3` |
| CL/SIGDATA | Empty | `77F8..77FC` |
| CL/HARDERR | Empty | `77FC..7808` |
| CL/GETC | Empty | `7810..7811` |

Other selected RTL members have empty `_DATA`/`_BSS`; they are explicitly listed
in `rtl_members`, rather than allocated invented storage. C0L's `_FARDATA` and
`_FARBSS` are zero length, and the selected stock RTL has no nonempty FAR_DATA or
FAR_BSS contribution. EMUVARS' nonempty special segment is the stack described above.

`47EC..47EE` is MATHL REALCVT `_CVTSEG`; `47EE..47F0` is CL CVTFAK `_CVTSEG`.
MATHL SCANTOD and CL CVTFAK supply six-byte `_SCNSEG` contributions at `47F0` and
`47F6`. Initializers appear in contribution order FPINIT, SETUPIO, CRTINIT, DELAY,
SETARGV, SETENVP, TZSET; FPINIT alone supplies the exit record. Their code targets
were checked. All intervening padding is separately attributed to TLINK.

## Far frame 1E81

All **46,048 bytes** from linear `1E810` to `29BF0` are serialized zero. The 211
relocated references to frame `1E81` split into **34 menu sites and 177 DHUF sites**;
none comes from another application module or RTL. Shared frame references prove
the coordinate system, not the original SEGDEF names or combine flags.

| Offsets | Owner and proposed object |
|---|---|
| `0000..01D4` | 158D, apparent 3 x 78-word saved error-screen buffer |
| `01D4..01D8` | 158D, directory list tail pointer |
| `01D8..0203` | 158D, 43-byte `ffblk` for findfirst/findnext |
| `0203..0207` | 158D, selected Alt/Ctrl record-array pointer |
| `0207..0257` | 158D, 80-byte working-directory buffer |
| `0257..0259` | 158D, selected record count word |
| `0259..025A` | UNKNOWN, alignment or storage |
| `025A..0740` | DHUF, 627-word Huffman son array |
| `0740..0E9A` | DHUF, 941-word Huffman parent array |
| `0E9A..1382` | DHUF, 628-word Huffman frequency array |
| `1382..2383` | DHUF, 4,097 prefix-length bytes |
| `2383..6385` | DHUF, 8,193 right-child words |
| `6385..8387` | DHUF, 4,097 parent-node words |
| `8387..A389` | DHUF, 4,097 left-child words |
| `A389..A38D` | DHUF, match length and position words |
| `A38D..B3C8` | DHUF, 4,155 dictionary bytes |
| `B3C8..B3D8` | DHUF, progress step/next and compressed/expanded sizes, four dwords |
| `B3D8..B3E0` | DHUF, output/input FILE far pointers |

The first buffer's *allocation* is inferred from the next field, with medium
confidence. The original code at `158D:2505/265E` accesses frame offset
`009C*row + 2*column`, with absolute rows 22..24. Those accesses exceed the proposed
`01D4`-byte buffer and overlap the later DHUF area; this is a real static
contradiction to a simple correctly indexed 3-row buffer, not evidence to enlarge
the allocation across other objects. No execution or behavior claim was made.

## BC++ 3.0 ordering probes and negatives

Fresh native BC++ **3.00** builds use `-c -ml -O1`, BCC SHA-256
`e56c6d228191e33665802e3affd9f44803af7c19c0c5a591c28ad10ffcdada6f`.
Probe source, full segment bytes, PUBDEFs, flags and object hashes are embedded
in `probe_observations`; full build receipts remain in `build/workers/s-data/`.

- **BSS is reverse declaration order**, not the TC 2.0/TC++ 1.00 identifier hash.
  Three independently shuffled 27-name TUs each emit the exact reverse sequence,
  as verified from PUBDEF offsets. No COMDEF is emitted by these native C probes.
  Initialized objects remain in definition order. A static-global probe also
  gives reverse allocation; addresses are established by its pointer fixups.
- **Objects are byte-packed by default.** `int q; char y; int x` in emitted order
  places them at `0,2,3`. Adding `-a` word-aligns non-char objects: the mixed
  `d,c,b[3],a,z` BSS probe changes `a` from offset 9 to 10, `z` from 11 to 12.
  Do not introduce alignment inside the packed image/fade records.
- **Definitions and local aggregate images are emitted when encountered, then
  the literal pool.** With all globals first, `order.c` has globals `0000..0014`,
  an automatic array's initializer image `0014..001A`, then the pool. Repeated
  `"DUPLICATE"` remains at distinct offsets `001A`, `0024`, `0037`.
  The stronger inherited rule "all globals, then all local aggregates" fails:
  `mix.c` puts a first function's aggregate at 0, a later global pointer at 4,
  and a second function's aggregate at 8. Its pool is `FIRST` at `000C`, numeric
  0.75 at `0012`, `GLOBAL_LATE` at `0016`, `LAST` at `0022`. Repeated numeric 0.75
  shares one cell. Strings and numeric constants can interleave in pool order.
- **Explicit uninitialized `far` objects are not necessarily FAR_BSS.** The
  all-uninitialized probe emits zero-filled class FAR_DATA, in reverse declaration
  order, in a generated private segment. With two initialized far definitions,
  each gets a segment and remaining uninitialized far objects append to the last.
  These default generated per-TU segment names do **not** explain the shared
  `1E81` frame across menus and DHUF. Preserve unknown original segment naming,
  combine policy or centralized definition ownership; do not assert an ordinary
  separate-default-TU natural link has already been reconstructed.
- Stock CL/STKLEN is **rejected** at DS:`0094`: it contains `1000`, oracle has
  `3000`, and C0's reference binds the symbol there. This is an application override.
- An initial compiler run under the longer scratch path
  `s-data/compiler/runs/...` failed opening `dpmimem.dll`; moving the same wrapper
  to `s-data/runs/...` succeeded. The first link probe lacked DPMI support files
  and exited 36; copying the stock support files produced exit 0. Neither failure
  is a compiler-layout result. No DAZZLE executable was run.

## Reproduction and remaining work

`python -I tools/dgroup.py` regenerates only `evidence/dgroup.json`, verifies
oracle/object hashes, constraint agreement, complete partitions, RTL invariant
data/fixups and the embedded BSS probe order. To repeat the fresh probes use
`python -I build/workers/s-data/probes.py`, then
`python -I build/workers/s-data/link.py`. The link command is TLINK 5.0 `/m
C0L.OBJ PROBE.OBJ,PROBE.EXE,PROBE.MAP,EMU.LIB MATHL.LIB CL.LIB`; its map proves
COMMON-stack behavior independently of DAZZLE. Validation is static plus compiler
and linker probes only. Original/reference files and reconstructed sources were
not modified by this task.

The map is ready for TU data reconstruction, with explicit debt: prove original
application definition ownership, recover residual object names/sizes, settle
`7452..7456` and far `0259`, and reproduce the shared far frame with native object
SEGDEFs. Probe the menu buffer indexing before treating its proposed type as
settled. No source/link-order padding should be fabricated from this census as
strict matching proof.

`python -I build/workers/s-data/verify.py` independently checks the 529-range
partition and serialized hashes, 66 record/name pointers, 211 far relocations,
1,821 RTL invariant bytes, 40 resolved data fixups, all three 27-symbol BSS
permutations, duplicated strings and the COMMON stack map: **PASS**. A raw word
search of the complete code area finds no `7450`, `7452` or `7454` operand, so
static direct/address-argument references do not settle the four-byte owner gap.
