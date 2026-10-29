# Natural TLINK build

**Partial.** `python -B -I tools/link.py` freshly builds all nine application
modules, verifies them, invokes stock TLINK 5.0, and writes
`evidence/match/link.json`. It currently exits **1 / LINK_ERROR**, with 54 undefined
symbols referenced by the read-only `src/seg158d.c`. No whole-image match is claimed.
The EXE which TLINK leaves after these errors is audited as **diagnostic output
ineligible for acceptance**; it must not be run or treated as a successful link.

## Command, inputs and switches

The script uses the pinned BC++ 3.0 `dazzle` profile (`-ml -O1`), staged project
headers, and TASM 3.0 `/ml /m2` for DVDT. Every invocation compiles/assembles afresh.
Because `bcc.py` stages each TU as `PROBE.C`, the link build supplies ordinary
`-zCDZ066A_TEXT` etc. segment names. Otherwise the identical public `PROBE_TEXT`
names would combine different TUs into one CS. These native SEGDEF names change
no code bytes: the **actual objects passed to TLINK** are proved as complete,
contiguous code contributions before linking, including their fixups.

Object order is C0L, 066A, 0B84, 0CCD, 11C1, 158D, 17F5, 1899, 19F0, 1A72.
Library order is the requested **CL.LIB, MATHL.LIB, EMU.LIB**. C0L/CL/MATHL come
from `C:/tools/borland-c++-3.0/files/LIB/LLIB`, EMU from `LIB/XLIB`, and TLINK
from `BIN`. Library/C0L/TLINK hashes are checked; compiler and assembler hashes
are checked by the existing wrappers. Source, header, tool, object, library,
runner, EXE and map hashes are recorded in the receipt or its build receipts.

Default TLINK switches are `/c /m /s /n`: case-significant symbols, public map,
detailed segment map, and no implicit default libraries. TLINK retains its normal
same-segment far-call rewrite; no object or output patching is performed.
Response-file newlines separate the four input fields: objects, EXE, MAP, libraries.
Switches share the object line. Do not also terminate those lines with commas.
`--switch` replaces the default switch list; `--out` selects another receipt.

## Proven contributions

All nine code TUs are **RELOCATED, same length**, totaling **87,060 bytes**.
17F5 has one separately reported zero linker-alignment byte after its 2,621-byte
code contribution. The initialized-data checks resolve every fixup and require
the original MZ relocation sites; no masks or prefix acceptance are used.

| Module | `_DATA` bytes / result | Near `_BSS` bytes / result | Far workspace |
|---|---:|---:|---|
| 066A | 2,467 RELOCATED | 6,149 EXACT size/bindings | — |
| 0B84 | 24 EXACT | 198 EXACT size/bindings | — |
| 0CCD | 430 EXACT | 4,928 EXACT size/bindings | — |
| 11C1 | 6,303 EXACT | 26 EXACT size/bindings | Four following bytes unresolved |
| 158D | 5,384 RELOCATED | No separate contribution indicated | Missing definitions |
| 17F5 | 544 EXACT | 42 EXACT size/bindings | — |
| 1899 | 160 EXACT | 726 EXACT size/bindings | — |
| 19F0 | 256 EXACT | 16 EXACT size/bindings | — |
| 1A72 | 911 RELOCATED | None | 45,446 zero bytes, size/public bindings agree |

The DATA total is **16,479 bytes**; the near-BSS declaration total is 12,085.
BSS results establish declared size and independently bound public offsets,
not emitted load-image bytes or natural placement. The far-workspace result
likewise does not prove its eventual linker frame. Ownership and unreferenced
object boundaries remain provisional census evidence. Names such as
`unused_path_state` and `logo_plane_state` describe reserved, unreferenced storage;
they are not claims to recovered source identifiers. Accepted ledgers are unchanged.

`segcmp.py --data` now gates complete DATA byte/fixup/MZ-site comparison, BSS
size and bound publics, and the far workspace where present. Its normal top-level
status continues to describe code, preserving existing code receipts. The link
script applies the same data checks to the actual C and assembly link objects.
`objcmp.compare(..., extent=...)` shares the fixup resolver with these checks and
does not interpret data as near-call instructions.

BC++ BSS order follows reverse **first declaration order**, including externs.
Accordingly the shared header lists BSS declarations in reverse address order.
The closing polygon vertices require six-word arrays in 0B84. Main's trig table
has 64 entries followed by the saved keyboard/timer vectors. Typed views replace
duplicate globals for record enable words, pointer words, shared callbacks,
stream elements, and palette fields. The pending 0CCD/17F5 header blocks were
moved; the 158D block remains local under the explicit read-only restriction.

The menu worker's final report is `build/workers/s-158d/notes.md` (17/17 strict
functions, 9,860-byte TU). Its initialized-data audit independently places the
final menu BEL/NUL string at **DS:39A4..39A6**. Thus 158D ends at **39A6** and
DLOGO's first zero word starts there, followed by state at 39A8/39AA/39AC and
its pool at 39AE. The provisional census's 39A4 boundary was two bytes early.
`segcmp.DATA_BOUNDS` records this correction; `evidence/dgroup.json` is unchanged.

## Current natural-link diagnostic

The failed-link EXE is 199,638 bytes; its complete serialized load image is
188,886 bytes against the oracle's 201,542. **150,539 positions differ**, counting
missing suffix bytes. The first difference is load offset **00001**, C0's DGROUP
operand: expected `BF`, obtained `9B` (DGROUP **29BF** versus **299B**).
The receipt reports full differing-byte counts and the first three differing
ranges for each independently attributed owner. It does not normalize addresses
or shorten the candidate image to manufacture equality.

There are **2,217 relocations versus 2,431**. Ordered segment/offset lists,
segment/offset multisets and sets, and physical-site multisets and sets all differ.
There are 2,392 missing and 2,178 extra physical sites with the current shifted
layout. No relocation mismatch is waived.

Candidate header: CS:IP `0000:0000` agrees; SS:SP **3127:0080** differs from
**314C:0080**. Header paragraphs `02A0`, relocation-table offset `003E`, minalloc
`031B`, maxalloc `FFFF`, checksum/overlay zero, pages `0186`, last-page bytes
`01D6`. Only entry/stack have recovered pre-pack reference values; other original
header fields remain unproved as explained in `packing.md`.

The map currently puts `_TEXT` at `00000..066B5`, versus the identified oracle
end `066AB`; app code then begins ten bytes late. EMU starts `1BAD0` versus
`1BAC0`; FAR_DATA starts `1E820` versus frame `1E810`; DGROUP starts `299B0`
versus `29BF0`. These are failed-link diagnostics, not isolated mismatch counts
for otherwise resolved modules.

## Remaining work and negative probes

1. Finish the read-only menu integration. All **54 remaining undefined names**
   are menu references: library/application helper aliases, shared-state views,
   record-field views, and its six far-workspace objects (`directory_tail`,
   `directory_find_data`, `directory_path`, `editable_images`,
   `editable_image_count`, `error_screen_backup`). The exact list is in the
   receipt. Several declarations deliberately require **far object accesses**;
   replacing them with the header's near declarations breaks code matching.
   No menu source edits or stand-in definitions were made in this task.
2. Resolve near BSS `7452..7456` and the far boundary byte `0259`. Do not invent
   padding objects for them. `-zEFAR_DATA` gives DHUF one native public FAR_DATA
   segment, but its paragraph alignment/combine policy with the missing menu
   contribution still needs proof. The DHUF binding is currently `1E81:025A`.
3. Reassess library extraction/order after those symbols resolve. Requested
   CL/MATHL/EMU does not yet reproduce the RTL order. An EMU/MATHL/CL diagnostic
   changes the EXE and RTL order but still fails all 54 names; it is not a match.
4. Recover serialization of the BSS suffix. Default TLINK ends this failed image
   at the initialized exit records. `/i` serializes all segments, including the
   stack, producing a 201,600-byte load image, not the oracle. The oracle ends
   exactly at DVDT's row-table end, DS:7756; original assembly emission of zero
   BSS is a hypothesis to test after placement is correct. Do not trim output.

Controlled probes used the same BC30 `-ml -O1` / TASM `/ml /m2` / TLINK 5.0 inputs:

- `/n`, `/d`, `/3`, and `/e` each leave the failed EXE byte-identical to the
  `/c /m /s` control. `/d` only diagnoses duplicate library symbols; `/3` enables
  32-bit processing and does not solve this layout. `/i` changes serialization.
  Saved results: `build/workers/s-link/switch-sweep.json`.
- Ordinary indexed scalar aliases into 0B84's vertex arrays add a three-byte
  reload. Integer-address views preserve the original optimizer behavior.
- Making the fade timer view nonvolatile removes a three-byte reload. The
  volatile lvalue view retains the original code without duplicating storage.
- Near `_streams` views shorten DHUF from 5,017 to 4,960 bytes; explicit far
  pointer construction still gives 4,981. Redeclaring the existing near array
  as far is rejected. Hiding the unused near declaration while including stdio,
  then declaring the same stock `_streams` symbol as far, reproduces all 5,017.
- `-a-` does not change FAR_DATA's paragraph alignment; `-a1`/`-a2` are rejected.
  TASM 3.0 rejects an `ALIAS <a> = <b>` directive. No fabricated OMF aliases were used.
- A response file with switches alone on line one prints help; commas plus
  field-separating newlines cause `Unable to open file 'dazzle.map'`. The script
  uses the verified response syntax above.

Verifier checks in `build/workers/s-link/verify-comparators.py` pass: immutable
MZ control, one-byte mutation, duplicate relocation with an unchanged site set,
DATA mutation, missing DATA relocation, wrong callback target, wrong BSS size,
and moved BSS public. The control's equality is a verifier test only.
`git diff --check` also passes. No assets/reference files or accepted ledgers
were modified, and no background processes were started.
