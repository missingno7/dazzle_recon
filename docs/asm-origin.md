# DVDT, DAZPCX and DHUF source origin

This pass reconstructs **67 non-overlapping function extents / 12,586 bytes**.
Fresh verification proves **60 functions / 10,096 bytes**: 16 EXACT (732 bytes)
and 44 RELOCATED (9,364 bytes). Seven candidates / 2,490 bytes remain
PROVISIONAL. These are **function-extent proofs**, not translation-unit,
natural-link, relocation-order, or full-EXE proofs. No RTL functions are counted.

| Original module | Source | Verdict | Strict result |
|---|---|---|---|
| DVDT, 1899 | `asm/seg1899.asm`, `src/seg1899.c` | Hand-authored assembly is strongly supported for 36 entries; standalone module is the best-supported packaging, not uniquely proved. Two EGA palette entries remain ambiguous. | 36/38, 5,229/5,486 bytes |
| DAZPCX, 19F0 | `src/seg19f0.c` | Compiled C, BC30 `-ml -O1`; port writes are expressible with DOS.H intrinsics and register pseudovariables. No standalone assembly justified. | 6/9, 1,122/2,083 bytes |
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

The assembly verdict applies to **0006..0C12** and **0D14..1573**, with all
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

TASM 3.0 assembles all 36 complete extents, including both tables and every
MZ segment relocation, with **`TASM.EXE /ml /m2 PROBE.ASM,PROBE.OBJ,PROBE.LST`**.
Public C aliases have leading underscores; VGA ports have symbolic constants,
data references have external symbols and independent bindings, and branches
use labels. There are no instruction-byte blobs, INCBIN, ORG padding holes or
post-build patches. The two CODE contributions surround the unresolved palette
entries; their arrangement is not a claim about an original OMF module layout.

This establishes a reproducing assembler configuration, **not a unique historical
assembler version**. An all-assembly body embedded in a C shell can be
indistinguishable from a standalone assembler function in the linked image.

### Ambiguous EGA palette functions

**0C13..0C7B (105 bytes)** and **0C7C..0D13 (152 bytes)** save four palette
bytes as words, substitute reserved colors, upload, and restore the saved bytes.
Their common prefix resembles compiled C. The BIOS variant saves SI without
using it; AH is cleared only at the first byte-to-word conversion. The direct
variant keeps ES:SI live through explicit retrace/CLI/STI/attribute-controller
operations. Neither a BP frame nor port I/O resolves their packaging.

The C candidates in `src/seg1899.c` are explicitly PROVISIONAL. Native
pseudoregister C gives 109 bytes for the BIOS function; replacing the BIOS
operations with inline ASM gave 112. The final mixed C/ASM probe gives
109/186 bytes versus 105/152. This does **not** justify moving those bodies
to standalone assembly merely to obtain equality. Their origin remains open.

## C evidence and unresolved candidates

DAZPCX reproduces the CS getter, VGA palette preparation, complete header
writer, full RLE encoder, linear row writer and save controller from C.
The native port builtin needs an explicit word assignment to AX before
selecting a variable plane; otherwise its byte parameter spills the counter.
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
| 1899:0C13 | Origin ambiguous; object 109 vs oracle 105 |
| 1899:0C7C | Origin ambiguous; object 186 vs oracle 152 |
| 19F0:004D | Final palette conversion loop loses the initial jump at 019E; 403 vs 405 |
| 19F0:037E | Full 169-byte extent, but SI/DI allocation for scanline and plane is reversed |
| 19F0:0488 | Register allocation for row/plane/column counters; 383 vs 387 |
| 1A72:00C2 | At 0139, immediate word store of 1 replaces MOV AX,1/store; 741 vs 742 |
| 1A72:0E40 | Intrinsic dictionary fill uses PUSH/POP ES and AL-to-AH duplication; 532 vs 530 |

All seven have source metadata carrying `level: PROVISIONAL` and a blocker.
No behavioral execution certificate is claimed for them. No mismatch, prefix,
masked comparison or normalized similarity is counted as a match.

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
| Replacing DOS.H's byte-port prototype with `void __outportb__(int,int)` or an unprototyped declaration, native BC30 | Compiler rejects both as a bad intrinsic call; final source uses the real prototype and AX pseudovariable |
| `-c -ml -O1 -Ob-` | BC30 rejects `-Ob-` as an incorrect command-line option; not a code-generation result |

TASM without `/ml` uppercases PUBDEFs and cannot satisfy the named bindings.
Symmetric XCHG operands must be ordered AH,AL / CH,CL to select the original
encoding under this assembler. These are ordinary instruction spellings, not
byte overrides. Initial static listings with `python -I tools/disasm.py` failed
because Capstone is in the user site; the scratch inspector explicitly adds that
trusted installed path. No tool or oracle was changed to handle these issues.

## Reproduction and verifier gaps

Run **`python -I build/workers/s-asm/finalize.py`**. It performs four fresh builds
and invokes the unmodified strict verifier, checks the contiguous owned extents,
and writes:

- `evidence/match/s-asm-1899.json`: assembly 36/36, C candidates 0/2;
- `evidence/match/s-asm-19f0.json`: 6/9;
- `evidence/match/s-asm-1a72.json`: 18/20.

For the C segments, the commands are
`python -I tools/objcmp.py src/seg19f0.c --all --fresh --out evidence/match/s-asm-19f0.json`
and the analogous command for `seg1a72.c`. Assembly and inline C use
`build/workers/s-asm/build_inline.py SOURCE --out REPORT`. That small adapter
calls **`objcmp.Object` and `objcmp.check_all` unchanged**, reading metadata from
the text source while passing the real TASM object. The final reports record
source/object/oracle hashes; scratch receipts record commands, tool and header
hashes. Native C receipts are supplied by the existing compiler wrapper.

Two central gaps remain: `bcc.py` rejects `-B` (flags replace rather than append
to the profile), and `objcmp.py --all` always reads metadata from its positional
source, so an OBJ cannot supply a separate text metadata source in that mode.
Neither tool was edited. The adapter does not resolve, patch or compare bytes
itself. The original load image is checked against its existing oracle hash by
objcmp on every verification.

BC30 SHA-256: `e56c6d228191e33665802e3affd9f44803af7c19c0c5a591c28ad10ffcdada6f`.
TASM30 SHA-256: `c338721a4280f0302a4ba5505a94da2057fa155a83fa512379eecc83738013df`.
Both use `C:/tools/nmlgcdos/msdos.exe`; the scratch adapter verifies the two
tool hashes before building. No DOSBox process was started.

Next work is the seven listed blockers, then source/data contribution ownership
and a natural link. Shared integration interfaces are the VGA callback globals
and coordinates named in the assembler bindings, the private 128-byte
`struct pcx_header`, and DHUF's far arrays/state in frame 1E81. No shared header,
accepted-state ledger, function inventory, tool, asset or reference file was edited.
