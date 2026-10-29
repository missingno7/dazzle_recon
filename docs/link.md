# Natural TLINK build

**EXACT load image, successful natural link.** Run:

```powershell
python -B -I tools/link.py
```

The command freshly compiles all application sources, assembles DVDT, checks
all nine contiguous code TUs and all **229 function extents**, checks the data
contributions, and invokes unmodified TLINK 5.0. It exits zero only when the
complete serialized load image and relocation-site multiset equal the immutable
oracle. The result is in `evidence/match/link.json`, including input/tool/object
hashes, the generated EXE/map paths, per-module proofs and native header fields.
No objects, executable bytes, relocation words or file lengths are patched.

## Result and certificate scope

| Check | Result |
|---|---|
| Load image | **201,542 bytes EXACT**, zero differing ranges |
| SHA-256 | `3df021b0416039df3b48b03362d223f5f358fd951d18316845624fd304030d8a` |
| Relocation sites | **2,431**, set and multiset equal; none missing/extra |
| Application code | Nine TUs RELOCATED, same length; **87,060 bytes** |
| Function checks | **229/229 EXACT or RELOCATED** |
| Entry / stack | `0000:0000` / `314C:0080`, both equal recovered values |

Relocation equality is at physical load-image sites, `segment*16+offset`, with
multiplicity preserved. The native TLINK segment/offset tuples and their order
are different from the decoder's PKLITE relocation groups (0000/0FFF/1FFE).
The receipt reports those raw-list/multiset/set inequalities explicitly. These
are equivalent site representations; no load bytes or relocation targets are
normalized in the full-image byte comparison.

The certificate covers the serialized load image and loader relocations, which is
the project's acceptance target. Packed-file reproduction is out of scope (see
`packing.md`); MZ header allocation fields (minalloc/maxalloc) are not part of it. Application coverage remains separate from stock Borland RTL.

## Reproducing configuration

Compiler: pinned BC++ 3.0 `dazzle`, `-ml -O1`, with the source-local controls already
proved in the TUs. Because the compile wrapper stages sources as `PROBE.C`, link
builds supply distinct native CODE names (`-zCDZ066A_TEXT`, etc.) to prevent
accidental combination of unrelated `PROBE_TEXT` segments. The actual objects
sent to TLINK pass the complete TU and function checks. DVDT uses TASM 3.0
`/ml /m2`; its CODE contribution has **word alignment**.

Object order: **C0L, 066A, 0B84, 0CCD, 11C1, 158D, 17F5, 1899, 19F0, 1A72**.
Library order: **EMU.LIB, MATHL.LIB, CL.LIB**. C0L/CL/MATHL are stock files from
`C:/tools/borland-c++-3.0/files/LIB/LLIB`; EMU comes from `LIB/XLIB`, TLINK from
`BIN`, run under MS-DOS Player. Hashes are pinned/recorded by the build tools.

TLINK switches: **`/c /m /s /n`** (case significance, public/detailed map, no
implicit libraries). Normal same-segment far-call conversion remains enabled.
`/i` is absent. The response file has switches and objects on its first line,
then EXE, MAP and libraries on separate lines, without additional commas.
`--switch` replaces the switch list; `--out` selects another receipt.

The originally requested CL/MATHL/EMU order places RTL contributions incorrectly.
EMU/MATHL/CL is the reproducing configuration; this does not recover the original
build command line. The 54 menu undefined names are all resolved through shared
types, typed field views and the actual library/application identifiers.

## Data and native placement

| Module / physical storage | DATA bytes | Near BSS bytes |
|---|---:|---:|
| 066A | 2,467 RELOCATED | 6,149 |
| 0B84 | 24 EXACT | 198 |
| 0CCD | 430 EXACT | 4,928 |
| 11C1 | 6,303 EXACT | 30, including four unreferenced bytes |
| 158D | 5,384 RELOCATED | None |
| 17F5 | 544 EXACT | 42 |
| 1899 | 160 EXACT | 726, explicitly emitted zeros |
| 19F0 | 256 EXACT | 16 |
| 1A72 | 911 RELOCATED | None |
| Shared far workspace in DVDT | 46,048 zero bytes; 22 public bindings checked | — |

All DATA bytes, fixups and MZ-site coverage pass. BSS checks require the full
size, independently bound public offsets and any explicitly emitted zero bytes.
`segcmp.py --data` enables data gating for C TUs; `link.py` applies the same core
to the actual C and assembly link objects and refuses to link failed proofs.

The menu header integration preserves **far object access** to shared state,
including separately declared `alt_image_records` and `ctrl_image_records`.
Borland applies `far` only to the first declarator of a combined array declaration.
Image/fade row types are shared; integer/pointer field views allocate no duplicate
storage. The menu/stdio far views retain the required constant segment operands.

Initialized menu data ends at **DS:39A6**, including its BEL/NUL at 39A4..39A6.
DLOGO starts at 39A6 and its pool at 39AE. This corrects the provisional census's
two-byte ownership overlap; `segcmp.DATA_BOUNDS` records it.

The far workspace is physically declared in the already established assembly
TU. Its **paragraph-aligned native FAR_DATA segment** begins at `1E81:0000`.
Menu objects occupy 0000..0259; native `ALIGN 2` supplies the boundary byte;
packed DHUF arrays/scalars then occupy 025A..B3E0. Named word/byte reservations
express their types and dimensions. The C algorithms retain far declarations.
This assembly storage is necessary for the reproduced packed layout: separate
BC30 FAR_DATA contributions each have paragraph alignment, placing DHUF at 0260
and shifting DGROUP. Physical definition ownership is a reproducing choice,
not proof that DVDT historically defined the menu/Huffman variables.

Four real startup-cleared bytes at **DS:7452..7456** have no identified source
name/type/owner. They are explicitly reserved as `reserved_program_state[4]`
in DOSI, between the 20-byte REGPACK and logo storage. This represents the
bounded unreferenced allocation; it makes no semantic/ownership claim and adds
no algorithm coverage. Existing opaque reserved state has the same limitation.
The full-image equality certificate is independent of such source-name choices.

| Native region | Half-open load-image extent |
|---|---|
| Stock `_TEXT` | `00000..066AB` |
| Application TUs | `066AB..1BAC0`, including one logo/DVDT alignment byte |
| EMU / math code and alignment | `1BAC0..1E810` |
| Shared far workspace | `1E810..29BF0` |
| DGROUP | Paragraph `29BF`; DATA ends 47EB, vector alignment at 47EC |
| `_CVTSEG` / `_SCNSEG` | DS `47EC..47F0` / `47F0..47FC` |
| `_INIT_` / `_EXIT_` | DS `47FC..4826` / `4826..482C` |
| Startup-cleared BSS | DS `482C..78C3` |
| Static COMMON stack | `314C0..315D0`, size 0110 |

The seven native INIT records and FPINIT's EXIT record match byte-for-byte,
including fixups. C0L's 0080-byte stack and EMUVARS' 0110-byte COMMON reservation
combine into one 0110-byte stack. DVDT emits zero BSS through its 350-word row
table: **DS:7756 / load offset 31346 is the exact serialized end**. Later C/RTL
BSS and the stack reserve memory without extending the file. No output trimming
or masked/prefix acceptance is involved.

Native MZ fields: file size 212,294; header paragraphs `02A0` (10,752 bytes),
relocation offset `003E`, count `097F`, pages `019F`, last-page bytes `0146`,
minalloc `0029`, maxalloc `FFFF`, checksum/overlay zero. Header provenance beyond
the recovered entry/stack is not established by the PKLITE decoder.

## Resolved differences and negative results

All probes used the pinned BC30/TASM30/TLINK50 inputs above unless stated.

- Canonicalizing the call bound at `0000:4332` to **flushall**, instead of the
  recovered name clrscr, removes an unwanted CLRSCR member. Exporting the actual
  application **matherr** prevents extraction of stock MATHERR. Unifying the
  menu's `_fstrchr` spelling with **strchr** prevents a duplicate 61-byte member.
  These changes preserve every application function byte/fixup proof.
- A byte-aligned DVDT starts at 18995 rather than 18996, shifting three code TUs.
  TASM's native word alignment produces the required single zero byte.
- Separate C far segments produce size B3E6 rather than B3E0. `-a-` does not
  change their paragraph SEGDEF alignment; `-a1`/`-a2` are rejected. `-Fc` instead
  produces 22 separate paragraph-aligned COMDEF allocations, not frame 1E81.
  Source-local `-a` changes do not select alignment for one deferred BSS object.
- Missing the four reserved BSS bytes leaves 845 differing bytes, a four-byte
  image shortfall, and wrong stack placement. Reserving the bounded allocation
  leaves four bytes in two DVDT OFFSET operands. Explicit
  **`OFFSET DGROUP:_ega_row_offsets`** fixes their frame, yielding EXACT.
- Using nonvolatile timer views removes a three-byte load; ordinary indexed
  scalar aliases into control-image arrays add a three-byte reload. The retained
  volatile/integer-address views reproduce the original optimizer behavior.
- Near stdio views shorten DHUF from 5,017 to 4,960 bytes; explicit far-pointer
  construction gives 4,981. Hiding the unused near declaration and declaring the
  same stock `_streams` symbol as far restores all 5,017 bytes.
- With the final objects, removing `/n`, or adding `/3`, `/d` or `/e`, leaves the
  **entire native EXE identical**. `/i` gives a 202,192-byte load image, including
  the extra 650 allocated bytes; it fails the complete-image check. Results:
  `build/workers/s-link/continue/final-switches.json`.
- TASM 3.0 rejects the ALIAS directive probe. Switches alone on response line one
  print help; combining commas with field-separating newlines misparses fields.

Verifier mutation tests reject changed DATA bytes/targets/relocation sites,
wrong BSS sizes/publics, and duplicate relocations even when the site set is
unchanged. Shared-workspace coverage and emitted BSS are also checked. No
accepted ledger, asset or reference file was changed. No background processes
were started. There are **no remaining differing load-image ranges**.

## Relocation order vs. the packed original (integrator check)

The linked EXE's 2,431 relocations equal the oracle as a set and multiset of linear sites. Their order
differs from the oracle list only because the oracle order comes from PKLITE's compressed relocation stream,
which encodes relocations in 0xFFF-paragraph windows (segments 0000, 0FFF, 1FFE). A **stable** partition of the
TLINK order by those windows reproduces the oracle order exactly; sorting within windows does not. So the
natural TLINK order is consistent with the original, and the pre-pack order is recoverable only up to that
stable partition (which is all PKLITE preserves).
