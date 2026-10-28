# DVDT, DAZPCX and DHUF source origin

This pass reconstructs **67 non-overlapping function extents / 12,586 bytes**.
Fresh verification proves **64 functions / 10,909 bytes**: 16 EXACT (732 bytes)
and 48 RELOCATED (10,177 bytes). Three candidates / 1,677 bytes remain
PROVISIONAL. These are **function-extent proofs**, not translation-unit,
natural-link, relocation-order, or full-EXE proofs. No RTL functions are counted.

| Original module | Source | Verdict | Strict result |
|---|---|---|---|
| DVDT, 1899 | `asm/seg1899.asm`; declarations in `src/seg1899.c` | Hand-authored assembly is the best-supported origin for all 38 entries, including the two EGA palette writers tested against C. Standalone module packaging is supported, not uniquely proved. | 38/38, 5,486/5,486 bytes |
| DAZPCX, 19F0 | `src/seg19f0.c` | Compiled C, BC30 `-ml -O1`; ordinary DOS.H port intrinsics reproduce both planar writers. No standalone assembly justified. | 8/9, 1,678/2,083 bytes |
| DHUF, 1A72 | `src/seg1a72.c` | Compiled C with explicitly far objects; BC30 `-ml -O1` reproduces 18 entries. BC31 remains indistinguishable on those entries. Native C and the tested assembler path agree on them. | 18/20, 3,745/5,017 bytes |

## Independently established boundaries

The paragraph estimates are not physical ownership boundaries. Call operands
retain the actual CS and are stronger evidence than a linear-address census:

- DVDT starts at **1899:0006**. Bytes 0000..0004 are the previous module's
  epilogue and 0005 is alignment. The mouse callback's RETF is **1899:1570**
  (the same linear byte as 19F0:0000). The DVDT CS getter is
  **1899:1571..1573**, proved by the far call at **11C1:0394**. The census
  incorrectly canonicalizes that getter to 19F0:0001.
- DAZPCX starts at **19F0:0004**, a three-byte `MOV AX,CS; RETF`, called at
  **11C1:03F1**. Its last function is **060B..0826**, including the complete
  epilogue beyond the estimated 0820 boundary.
- DHUF starts at **1A72:0007**, also a three-byte CS getter, called at
  **11C1:03C1**. The extraction entry is **11BB..139F**, not a separate
  function at each apparent ENTER signature inside it.

Other important census corrections: DVDT 012D..02E7, 02E8..048A,
048B..0621, 0622..075C and 101A..133C are whole functions. The apparent
ENTER prologues inside them are instruction operands or manual stack-slot
construction. DVDT 0959 includes both genuine word jump tables (0982, 0A3B);
their case labels are not functions. DHUF **0C69..0DC5** is one Huffman
update function, **0E40..1051** is one decoder, and **1052..118D** is one
progress initializer. Reports partition each owned extent without gaps or overlap.

## DVDT: evidence for hand-authored assembly

The assembly verdict applies to **0006..1573**, with all
function starts and complete extents in the source metadata. It includes the
plasma routines at 101A/133D; geometric semantics alone do not make them C.

- The RGB quantizer uses BX as a three-byte source index, CX/LOOP, explicit
  LDS/LES and DS preservation. The planar writer preserves/reuses DI across
  four `REPNE MOVSB` transfers and explicitly restores the sequencer mask.
- Symmetry plotters deliberately use PF after `TEST [BP+6],3`; discarded
  ES latch reads precede color stores. X bit halves select VGA planes. The
  delay loop masks CH on every iteration. These are register/flag contracts,
  not evidence inferred merely from the presence of OUT instructions.
- REP palette writers use ENTER/LEAVE and 186 string I/O; separate 8086
  versions use PUSH BP/MOV BP,SP and MOV/INC/OUT/LOOP. CPU detection preserves
  FLAGS and distinguishes historical shift behavior. The near CMOS helper
  has its own AL input/output contract.
- Plasma code uses SHR/ADC rounded midpoints, ROR-based state updates and
  the high half of signed IMUL. It builds plot arguments by pushing AX,
  temporarily establishing BP, and writing `[BP+2]`. It also uses SI/DI in
  the recursive entry without a normal C callee-save sequence. Its saved
  registers, early epilogues and recursive PUSH CS/CALL forms fit a manual
  assembler implementation.
- Timer/keyboard handlers save only the registers they use and chain the
  saved vectors with the interrupt FLAGS already on the stack. The mouse
  callback uses BP to establish DS without changing AX.

TASM 3.0 assembles all 38 complete extents, including both tables and every
MZ segment relocation, with **`TASM.EXE /ml /m2 PROBE.ASM,PROBE.OBJ,PROBE.LST`**.
Public C aliases have leading underscores; VGA ports have symbolic constants,
data references have external symbols and independent bindings, and branches
use labels. There are no instruction-byte blobs, INCBIN, ORG padding holes or
post-build patches. The palette functions now occupy their original position in
one CODE contribution. This is not a proof of the original OMF record layout.

This establishes a reproducing assembler configuration, **not a unique historical
assembler version**. An all-assembly body embedded in a C shell can be
indistinguishable from a standalone assembler function in the linked image.

### EGA palette functions: assembler reconstruction selected

**0C13..0C7B (105 bytes)** and **0C7C..0D13 (152 bytes)** save four palette
bytes as words, substitute reserved colors, upload, and restore the saved bytes.
Their common prefix resembles compiled C. The BIOS variant saves SI without
using it; AH is cleared only at the first byte-to-word conversion. The direct
variant keeps ES:SI live through explicit retrace/CLI/STI/attribute-controller
operations. Neither a BP frame nor port I/O resolves their packaging.

Both entries were transcribed into readable TASM using the module's established
stack/register conventions and independent data bindings. They verify
**RELOCATED: 105 and 152 bytes** under TASM30 `/ml /m2`; all previous 36 matches
remain intact. Stack slots have PAL_ names, attribute-port waits have descriptive
labels, and the source keeps the unused SI save visible.

C was tested separately: native BC30 `-c -ml -O1` produces 109 bytes for the
BIOS writer; inline BIOS statements gave 112. The complete C candidates through
`-c -ml -O1 -S` and TASM30 `/ml /m2` give 109/186 versus 105/152.
Native `-c -ml -O2` gives 109/193. Probe source is retained at
`build/workers/s-asm/palette-c-baseline.c`, with reports beside it.

Exact assembler output alone does not prove origin. The decision combines that
test with the surrounding hand-authored module, the shared SI-preserving frame,
the deliberate AH lifetime across four word saves, and ES:SI remaining live
through the direct port sequence. Hand-authored assembly is the best-supported
reconstruction; a C shell containing an all-assembly body remains historically
indistinguishable. The prior C implementations are removed from `src/seg1899.c`,
which now retains interface declarations only.

## C evidence and unresolved candidates

DAZPCX reproduces the CS getter, VGA palette preparation, complete header
writer, full RLE encoder, linear row writer and save controller from C.
Both planar writers now use plain `outportb(0x3cf,plane++)` **four times**, then
reset `plane=0`. BC30 deletes the fourth increment before the reset, but retains
the word load needed for the expression's old value. Reconstructing that last
write as `outportb(0x3cf,plane)` instead makes the load byte-sized and changes
register eligibility; explicit AX assignments also distort allocation. The four
post-increments reproduce the full **169- and 387-byte** original functions.
Correcting the cycling offset from unsigned to **signed int** reproduces the
original far-pointer indexing sequence in the VGA palette preparer.

DHUF reproduces its fatal path, dictionary initialization, prefix maintenance,
tree deletion, bit/byte/multi-bit readers, bit writer, adaptive Huffman setup,
reconstruction, update, character/position decoding, progress and extraction
controller. The far objects form a coherent layout in **1E81**: Huffman tables,
prefix bytes, hashed right-child roots, parent/left-child arrays, dictionary and
stream/progress state. Comma-separated Borland declarations do not propagate
`far` to every declarator; the maintained source spells each one separately.
The early dictionary helpers and bit writer are encoder-side support, not CRC
helpers. No upstream source identity is claimed.

| Address | Remaining blocker; strict verifier outcome |
|---|---|
| 19F0:004D | Final palette conversion loop loses the initial jump at 019E; 403 vs 405 |
| 1A72:00C2 | At 0139, immediate word store of 1 replaces MOV AX,1/store; 741 vs 742 |
| 1A72:0E40 | Intrinsic dictionary fill uses PUSH/POP ES and AL-to-AH duplication; 532 vs 530 |

All three have source metadata carrying `level: PROVISIONAL` and a blocker.
No behavioral execution certificate is claimed for them. No mismatch, prefix,
masked comparison or normalized similarity is counted as a match.

The residues are specific instruction-selection/source-shape blockers:

- **19F0:019E:** the oracle has `EB 3A`, jumping to the first loop test. Native
  C starts the LES/body there; its full function is 403 bytes. A pre-increment
  loop is 408 bytes. An explicit early-break loop is 405 bytes but puts CMP at
  019E. An inclusive bound is also 405 bytes, but uses CMP 47/JLE instead of
  CMP 48/JL. Multiplying 16*3, casting `sizeof(pcx->palette)` to int, reversing
  the comparison, or negating `i>=48` repeats the 403-byte output. `-O-` gives
  405 bytes by retaining a different redundant
  jump after the final case, not the required entry jump. The `-S`/TASM path is
  407 bytes: two earlier backward branches remain JGE/short-JMP pairs, while
  the required final-loop entry jump is still absent.
- **1A72:0139:** native C emits `C7 46 F6 01 00`; the oracle emits
  `B8 01 00 / 89 46 F6`. Initializing both prefix variables repeats 741 bytes.
  Initializing `common=cmp` instead makes a 742-byte function, but loads the
  stack variable (`8B 46 FE`) instead of immediate 1. `-Oa` does not propagate
  the constant here. A bounded outer loop is 731 bytes and changes control
  flow. Native, `-B`, and `-S`/TASM do not select the oracle's literal sequence.
- **1A72:0E5F:** the oracle uses CX=count, DI=offset, AX=segment, ES=AX,
  AX=2020h, REP STOSW. The tested intrinsic loads the segment first, uses
  PUSH/POP ES, and copies AL to AH. Passing 2020h instead of a space and adding
  `-Oi` leave the 532-byte fill/decoder shape. `setmem` gives 537 bytes; a C
  word-fill loop gives 568. The exact six-instruction inline-ASM block reserves
  DI for the whole function, spills the ring position, grows the frame from
  12 to 14 bytes and the function to 545. Explicit `register` and pseudoregister
  setup do not avoid that reservation, under native or TASM compilation.

Further work needs an independent source/compiler discriminator for these
constructs; adding dead stores, artificial control flow or opcode overrides
solely to manufacture the missing bytes is not acceptance evidence.

## Tested negatives

Unless noted, C tests use the pinned BC30 **`-c -ml -O1`** profile, with
ordinary DOS.H/STDIO.H and the complete candidate translation unit.

| Hypothesis / exact context | Outcome |
|---|---|
| Native C vs `-c -ml -O1 -B` for DHUF, before the rejected fill experiment | Same 18/20, 3,745 strict bytes; neither path identifies historical `-B` use |
| Six-instruction decoder fill as localized inline ASM, BC30 `-c -ml -O1 -B` | Fill itself equals, but DI becomes unavailable to C; decoder becomes 545 bytes and moves ring position to the stack. Reverted. |
| `#pragma intrinsic memset` instead of explicit `__memset__`, native BC30 | Same 532-byte decoder; original fill ordering still not reproduced |
| DAZPCX via BC30 `-c -ml -O1 -B`, automatic TASM invocation | EGA palette preparer 409 bytes; forward-jump NOPs appear |
| DAZPCX via BC30 `-c -ml -O1 -S -IINCLUDE`, then TASM30 `/ml /m2` | Preparer 407 bytes, still not 405; no additional accepted functions |
| DHUF via the same `-S` / TASM30 `/ml /m2` path | Same 18/20, 3,745 bytes |
| BC31 `-c -ml -O1`, pre-final C snapshot | DHUF 18/20, 3,745 bytes; DAZPCX 1/9, 3 bytes |
| BC20 and TC30, each `-c -ml -O1`, same snapshot | Each DHUF 1/20, 84 bytes; DAZPCX 0/9 |
| Explicit register declarations, declaration order and signed/unsigned column/plane variants in DAZPCX | Did not settle row-writer allocation; unsigned column also enlarged planar output. Reverted. |
| Replacing DOS.H's byte-port prototype with `void __outportb__(int,int)` or an unprototyped declaration, native BC30 | Compiler rejects both as a bad intrinsic call; an AX-based intermediate probe was later replaced by four post-increments |
| `-c -ml -O1 -Ob-` | BC30 rejects `-Ob-` as an incorrect command-line option; not a code-generation result |

Continuation probes used the same pinned BC30 `-c -ml -O1` unless their exact
additional option is named. Source snapshots and explicit transformations are in
`build/workers/s-asm/continue_probes.py`; compiler receipts preserve each input.
The earlier port-prototype/AX experiments above are historical negatives; the
maintained row writers now use the real DOS.H prototype and four post-increments.

| Continuation hypothesis | Outcome |
|---|---|
| EGA palettes as TASM30 `/ml /m2`, established DVDT conventions | Both strict RELOCATED, 257 bytes; all 38 DVDT extents match |
| EGA palette C, BC30 `-c -ml -O2` | 109/193 vs 105/152 bytes; no new matches |
| EGA palette C, BC30 `-c -ml -O1 -S`, TASM30 `/ml /m2` | 109/186 vs 105/152 bytes |
| Plain, explicitly unsigned, or masked byte-port arguments | Do not reproduce the row writers; plain byte reads spill the plane counter. Four post-increments resolve both functions. |
| Legacy `outportb(int,int)` without DOS.H | Emits calls, not the direct OUT sequence; EGA writer 208 vs 169 bytes. Keeping DOS.H causes a prototype redeclaration error. |
| DHUF `-c -ml -O1 -Oi` | Still 741/532 at the two targets; progress initializer also differs |
| DHUF `-c -ml -O1 -Ot` | Insert 787 vs 742 bytes; 2/20 match, 103 bytes |
| DHUF `-c -ml -O1 -Z-` | Insert 759 vs 742 bytes; 3/20 match, 187 bytes |
| DHUF `-c -ml -O1 -O-` | Insert 738 vs 742 bytes; 7/20 match, 1,363 bytes |
| DHUF `-c -ml -O1 -Oa`, literal or copied prefix initialization | Repeats the corresponding 741-byte / wrong-load results; 18/20 match, 3,745 bytes |
| DAZPCX `-c -ml -O1 -r-` | Does not retain the needed counter registers; 1/9 match, 3 bytes |

TASM without `/ml` uppercases PUBDEFs and cannot satisfy the named bindings.
Symmetric XCHG operands must be ordered AH,AL / CH,CL to select the original
encoding under this assembler. These are ordinary instruction spellings, not
byte overrides. Initial static listings with `python -I tools/disasm.py` failed
because Capstone is in the user site; the scratch inspector explicitly adds that
trusted installed path. No tool or oracle was changed to handle these issues.

## Reproduction

The assembly gate is now a central tool, with one command:

```text
python -I tools/asmcmp.py asm/seg1899.asm --out evidence/match/s-asm-1899.json
```

Result: **38/38 matched (5,486 bytes)**. `--all` is optional and accepted for
objcmp CLI compatibility; asmcmp always checks all metadata entries.

`tools/asmcmp.py` performs a fresh TASM30 **`/ml /m2`** build in an isolated
scratch directory and calls **`objcmp.Object` / `objcmp.check_all` unchanged**.
It reads the producer paths and expected hashes from **`evidence/toolchains.json`**,
verifies TASM before execution, fixes input timestamps, clears ambient compiler
environment variables, and records source/tool/runner/object hashes, command
lines and the build log. Its `--out` report links the hashed receipt. A `.c`
input additionally verifies BC30, copies and hashes headers, and runs
**`BCC.EXE -c -ml -O1 -IINCLUDE -S PROBE.C`** before the same TASM command.
The native C acceptance commands remain:

```text
python -I tools/objcmp.py src/seg19f0.c --all --fresh --out evidence/match/s-asm-19f0.json
python -I tools/objcmp.py src/seg1a72.c --all --fresh --out evidence/match/s-asm-1a72.json
```

Results: **8/9 (1,678 bytes)** and **18/20 (3,745 bytes)** respectively.
`python -I build/workers/s-asm/finalize.py` runs all three fresh commands, verifies
the contiguous owned extents and metadata levels, and checks that every match
in the three committed reports remains matched. The current reports preserve
**all 60 previously committed matches**, adding four RELOCATED functions /
813 bytes. Source/object/oracle hashes accompany the results.

`python -I build/workers/s-asm/verify_asmcmp_guards.py` passed two negative checks:
a scratch copy with one changed TASM byte is rejected against the unchanged
catalog hash before execution, and changing the BIOS writer's extent metadata
from 105 to 104 is rejected by objcmp with a nonzero exit status and a full-extent
mismatch. The adapter does not resolve, normalize, patch or compare bytes itself.
No changes to `objcmp.py` or `bcc.py` were needed.

BC30 SHA-256: `e56c6d228191e33665802e3affd9f44803af7c19c0c5a591c28ad10ffcdada6f`.
TASM30 SHA-256: `c338721a4280f0302a4ba5505a94da2057fa155a83fa512379eecc83738013df`.
The runner is `C:/tools/nmlgcdos/msdos.exe`. No DOSBox process was started.

Next work is the three precise source/compiler blockers above, followed by data
contribution ownership and a natural link. The remaining code is integer-only;
the new FP idiom reference supplies no discriminator here. Shared integration
interfaces remain the VGA callback globals, the private 128-byte
`struct pcx_header`, and DHUF's far arrays/state in frame 1E81. Apart from the
requested new `tools/asmcmp.py`, this work changed no tool, shared header,
accepted-state ledger, function inventory, asset or reference file.
