# DAZZLE application compiler: BC++ 3.0, large model, size optimization

The reproducing profile is **Borland C++ 3.00, `-c -ml -O1`**, with the
unmodified compiler SHA-256
`e56c6d228191e33665802e3affd9f44803af7c19c0c5a591c28ad10ffcdada6f`.
`tools/bcc.py` calls this profile `dazzle`. Nine independently sized application
functions reproduce 1,036 bytes: **64 EXACT and 972 RELOCATED**. These are
function-extent proofs, not a linked executable or translation-unit match.
All nine use the same profile; there is no evidence here for per-module flags.
BC30's `-O1` selects `-O -Ob -Oe -Os -k- -Z`; default register variables,
8086 instructions and no stack checking reproduce the sampled code.

This identifies the best-supported compiler/configuration among the four
acquired releases. It does **not** recover a unique historical command line:
several dormant options are indistinguishable on these functions. In particular,
`-a`, `-d`, and `-k` cannot be settled from these nine extents. Do not silently
promote those defaults into historical facts. Floating-point **link selection**
is emulation; the compile-only `-f` variants do not distinguish it.

Compiler identity is checked against `evidence/toolchains.json` on every call,
including cache hits. The other three compiler hashes are:

| Candidate | Compiler SHA-256 |
|---|---|
| BC++ 2.0 | `dd5744b99c6fa61dc1d95c580a1f1d6f1b3e97ae8b920dd6e9cfedce29f82fb6` |
| BC++ 3.1 | `87c6dfbad817eeca31c069043f08063f7122663e51932ad089e6d1c563450d3b` |
| TC++ 3.0 | `2548e4ba9c88b280f5348d9c19bf8ad5b354c581bffab91f78cbce3b55b8425d` |

## Accepted candidates and discriminators

Each file in `src/probes/` contains the target address, independently recovered
extent, bindings, status and profile. A number in the candidate columns is its
complete object extent when different. `R` means strict RELOCATED; `E` means EXACT.
Every column uses **that compiler's `-ml -O1`**, the same final source and bindings.

| Source / function | Address | Bytes | BC20 | BC30 | BC31 | TC30 |
|---|---|---:|---:|---:|---:|---:|
| `line_step.c` | `066A:000E` | 269 | 275 | R | R | 285 |
| `choose_color.c` | `066A:011B` | 336 | 337 | R | R | 342 |
| `square.c` | `066A:026B` | 80 | 79 | R | R | R |
| `mouse_position.c` | `11C1:0501` | 46 | R | R | R | R |
| `allocate.c` | `11C1:062B` | 65 | R | R | 56 | R |
| `beep.c` | `11C1:0E07` | 35 | R | R | R | R |
| `beep_float.c` | `11C1:1FBF` | 48 | 49 | R | R | 50 |
| `free_nodes.c` | `158D:00A2` | 93 | R | R | R | R |
| `text_cell.c` | `158D:0219` | 64 | 67 | E | 63 | 68 |

`allocate` is a useful BC30/BC31 discriminator: BC30 emits `MOV BX,2; MOV AX,SI;
XOR DX,DX; DIV BX; MOV SI,AX`, whereas BC31 emits `SHR SI,1`. BC31 also differs
in `text_cell`. Disabling BC31's `-Ol` alone does not restore either extent.
The release claim is supported by these code-generation differences, not by the
shared 1991 copyright banner. The concurrently produced RTL inventory independently
reports large-model BC++ 3.0 / TC++ 3.0 library candidates.

Extents were established from the original disassembly, branches, final returns
and next function starts, before comparison. `line_step` ends at `066A:011B`;
the function scanner's current 221-byte estimate ends too early. `choose_color`
contains 320 bytes through RETF and a **16-byte jump table** at `066A:025B`;
its complete extent ends at `026B`. These are deliberate corrections to heuristic
inventory sizes; `evidence/functions.json` was read, never changed.

## Observed code generation

- Far functions use RETF and stack arguments begin at BP+6. Parameters needing
  frames start with `PUSH BP; MOV BP,SP`; locals use `SUB SP,n`. Saved SI/DI are
  popped before `MOV SP,BP; POP BP; RETF` when local storage exists.
- `square` assigns its repeatedly used size parameter to SI, the outer loop
  counter to DI, and keeps the inner counter at BP-2. `allocate` keeps its
  working size in SI. Disabling register allocation breaks these functions.
- Calls to external far functions use `9A offset16 segment16`. The segment half
  must have an MZ relocation. The application also uses `PUSH CS; CALL rel16`
  for known functions in the same code segment, for example the line routine
  called from `066A:03CE`. Isolated declarations/TU context can change that form.
- Large model makes ordinary data pointers four bytes. `free_nodes` copies both
  words of a pointer and uses LES to follow its next link. Medium model makes
  this source 47 bytes and does not reproduce the function. Huge model introduces
  additional DS setup/global accesses and fails all nine probes.
- The color switch decrements the selector, performs an unsigned bound check,
  shifts BX once, and jumps through a CS-relative word table after RETF. The
  table's address and all eight entries are OMF fixups; none are masked.
- `abs` comes from the vendor `stdlib.h` intrinsic macro `__abs__`, producing
  CWD/XOR/SUB. A hand declaration of `abs` instead produced an external call and
  is a refuted source hypothesis.
- Color calculations first multiply **16-bit ints**, then sign-extend into
  `unsigned long` locals. Moving the cast before multiplication changes behavior
  and bytes. Unsigned long shifts call `F_LXURSH@` at `0000:13D6`; signed
  `F_LXRSH@` is a different routine at `0000:13B6`. Multiplication calls
  `F_LXMUL@` at `0000:11E3`, division calls `F_LDIV@` at `0000:12E7`.
- `random(colors)` expands the vendor macro, calling `rand`, performing long
  multiplication, and dividing by 32768. The constant is pushed before `rand`;
  it is not a parameter to `rand`.
- `beep_float` rounds a float by adding 0.5f and calls `F_FTOL@` at `0000:0FBE`.
  Its compiler object contains FWAIT/x87 bytes with two `FIDRQQ` OFFSET16 fixups.
  The independently hash-checked **EMU.LIB** absolute PUBDEF is `FIDRQQ=0x5C32`:
  adding it changes `9B D9` to `CD 35` and `9B D8` to `CD 34`, without masking or
  inserting bytes. FP87.LIB defines that symbol as zero and would preserve x87
  opcodes. The source's `_DATA` constant is bound at `DS:201C`, where the oracle
  contains `00 00 00 3F` (0.5f). Its target and opcode fixups are all explicit.

Runtime helper identities are cross-checked against the read-only function
inventory when available. An early scratch color experiment used signed long
locals and incorrectly assigned `F_LXRSH@` to `13D6`. That experiment is invalid,
even though a caller-only numeric substitution could make bytes equal. The final
source uses unsigned longs and the correct `F_LXURSH@` binding. The new inventory
guard explicitly rejects the old mapping. Early exploratory result files are
diagnostics; only `final_*` and fresh final-source verification support acceptance.

## One-axis options and negative results

Unless stated otherwise, each row means **BC30 `-ml -O1` followed by exactly the
listed option**, using the final nine sources. Option order matters. Adding `-O`
after `-O1` leaves its other enabled optimizations in place; it does not test
plain `-O`. Scratch JSON records full flag arrays and source hashes.

| Axis | Observed result / conclusion |
|---|---|
| `-mm` replacing `-ml` | `free_nodes` 47, `allocate` 56, `mouse_position` 44: rejects ordinary near data pointers. |
| `-mh` replacing `-ml` | All fail; line 560, color 518, square 129, free list 121. |
| `-Od` | line 287, color 348, allocator 67, float 50; rejects no optimization. |
| `-O-` | Disables jump optimization only: line 281, color 344, allocator 67. |
| `-O2`, `-G` | Both break square (82), beep (39), free list (94), color (387), float (52). Speed mode uses `ADD SP,2` where size mode uses `POP CX`. |
| `-Z-` | Restores redundant loads: line 275, color 339, float 50. Reload suppression is required. |
| `-O-b` | line 275, color 339: dead-store elimination required. |
| `-O-e` | line still 269 but parameters use BX/CX instead of DX/BX; text writer 66. Global register allocation required. |
| `-1`, `-2` | Square begins `ENTER 2,0` and is 76 bytes; free list 90, mouse 43. Reject both enabled CPU modes; profile remains 8086. |
| `-N` | Adds 11 bytes to every function, including stack-check call/fixups. Rejected; `-N-` preserves all nine. |
| `-r-` | Square 90, allocator 71, line different instructions at the same length, text 62. Rejected; `-r` preserves all nine. |
| `-k`, `-k-` | All nine unchanged; each already needs a frame. Synthetic leaf control: `-k` adds PUSH BP/MOV BP,SP/POP BP, growing 4 bytes to 8. Historical setting unresolved. |
| `-a` | Nine unchanged. Synthetic `{char;int;}` control changes sizeof/BSS from 3 to 4; it does not align these functions. Historical setting unresolved. |
| `-d` | Nine unchanged. Duplicate-string control reduces `_DATA` from 28 to 18 bytes and both pointers select the same literal. Historical setting unresolved. |
| `-p` | Pascal publics become uppercase; original function names fail lookup. Synthetic `add(a,b)` reverses parameter slots and ends in `RETF 4` rather than RETF. Application C convention is caller cleanup. |
| `-u`, `-u-` | `-u` preserves all nine. `-u-` removes leading underscores from publics/externals and leaves scalar code unchanged; supplied underscore bindings become **unbound**, never a match. EXE bytes alone do not prove symbol spelling. |
| `-f`, `-f-`, `-f87` | Same compile-only code, fixups and inspected comments, including FIDRQQ. These switches cannot establish FP link selection with `-c`. Original bytes require EMU's absolute fixup values, not FP87's zero values. |
| `-O-c`, `-O-g`, `-O-i`, `-O-l`, `-O-m`, `-O-p`, `-O-v` | Each preserves all nine when individually disabled after `-O1`; these options are not independently established by this sample. |

Before the settled profile, BC30 `-ml -O` produced line 277 and color 342;
`-ml -O -Z` produced line 271 and color 339. The combination
`-ml -O -Ob -Z` reproduced color but retained the wrong line parameter register
assignment; `-O1` also supplies global register allocation. Source forms matter:
omitting either of the first two explicit `else return 0` statements in the final
line source removes two bytes per omission under `-O1`; adding the third makes
271 bytes. Conditional-expression rewrites of the error calculation also differed.
Those source changes were rejected rather than patched after compilation.

The broader early sweeps are retained under `build/workers/compiler/`:
`model_results.txt`, `options_results.txt`, `axes_results.json`, `discriminate.txt`,
and cached receipts with the exact source copies, compiler hash, flags and log.
For BC20/TC30 `-Od` is rejected by the compiler; `-Ob-` is also rejected by BC30.
The correct suboption disable syntax is `-O-b`, not `-Ob-`.
BC31 one-axis disables around `-ml -O1` (`-O-l/-b/-e/-c/-g/-i/-m/-p/-v`,
`-Z-`, `-k`, `-k-`, `-r-`, `-Od`) never make both allocator and text writer match.
The corresponding TC30 controls never reproduce the line/color/text set.

The option definitions and disable syntax were checked against Borland's
[BC++ 3.0 User's Guide, Appendix A, printed pages 174 onward](https://www.bitsavers.org/pdf/borland/borland_C%2B%2B/Borland_C%2B%2B_Version_3.0_Users_Guide_1991.pdf).
Experiments, rather than the manual's option descriptions, establish the results
above. Synthetic controls measure option effects; they are not application matches.

## Per-module options

Module `158D` now reproduces its complete **9,860-byte code contribution**
(`158D:0004..2688`) with BC++ 3.00's `dazzle` profile, **`-ml -O1`**.
All 17 full function extents match: 64 EXACT and 9,796 RELOCATED bytes.
The previously short directory browser, file dialog and configuration dispatcher
need no additional option changes. The full-code receipt is
[seg-158d.json](../evidence/match/seg-158d.json), and the fresh function receipt is
[r158d.json](../evidence/match/r158d.json).

The hypothesis was that disabling jump optimization would restore the 13 missing
bytes (redundant jumps, call suffixes and an unfolded conditional). A module-wide
experiment removed source-local optimizer pragmas, then used `objcmp --flag=-ml
--flag=-O1` followed by every combination of `-O-`, `-O-b`, `-O-e` and `-Z-`
(16 builds). Every non-default combination breaks existing matches, including
early functions before the three in question. Expanded earlier functions can
make later function placement UNKNOWN; those rows are not treated as matches.
The source-local controls must not be allowed to silently reset the option
being tested. BC30 accepts `-O-b`/`-O-e`, rather than `-Ob-`/`-Oe-`.

The same axes were tested one function at a time, with pragmas restored after
the function. These are complete emitted lengths of the original candidates;
**none of the option variants below matches**, even where length agrees.

| Local option after `-O1` | Directory (1,095) | File dialog (1,704) | Dispatcher (572) |
|---|---:|---:|---:|
| Unchanged | 1,092 | 1,696 | 570 |
| `-O-` | 1,095 | 1,713 | 574 |
| `-O-b` | 1,119 | 1,722 | 575 |
| `-O-e` | 1,114 | 1,696 | 570 |
| `-Z-` | 1,095 | 1,704 | 579 |

All 15 nonempty combinations of these four local disables were checked for each
function. In particular, local `-O-` restores the dispatcher's desired JE/JMP
at `2425/2427`, but adds another JE/JMP around case `8000` at `243F`. This is
not evidence for a historical jump-optimization disable in these three routines.
Exact flags, source copies and per-function receipts are under
`build/workers/s-158d/options-module-*` and `options-local-*`; their summary JSONs
retain all 16 module and 45 local experiments.

Structured source under the unchanged `-O1` closes all three:

- Directory browser: separate `free_nodes(); return;` exits let BC30 share the
  cleanup suffix. The original redundant near JMP at `1928` is then emitted;
  explicit gotos to a preselected common cleanup had removed it.
- File dialog: F3 copies `"*.*"` into the search specification and enters the
  directory chooser directly. Only the wildcard-path arm first resets the
  configuration filename to its default. This corrects a real control-flow
  error in the earlier candidate and restores the four-byte call suffix.
  Separate unrecognized-action assignment/return arms reproduce the two
  original short JMPs at `1EE1/1EE3` through compiler suffix sharing.
- Dispatcher: an explicit `if (update_key_state() != 0) { cleanup; return; }
  else { switch (...) ... }`, with the default arm's own cleanup/return,
  reproduces `JE; JMP` at `2425/2427`. Named FAR `alt_image_records` and
  `ctrl_image_records` replace numeric segment constants, supplying the two
  required MZ relocations at `22BB/2307` as well as the matching bytes.

No new function-local pragmas or jumps to immediately following labels were
introduced for these three functions. Existing reproducing controls elsewhere
in `158D` remain: `-d` for the TU's literal pool, local `-O-` for `help_screen`,
and local `-Z- -O-` for `menu_error_message`, restored afterward. The previously
proved `17F5` TU similarly retains its local `-Z-` for `configure_logo_dac`.
These are proven reproducing configurations; they do not establish that the
historical source used those exact pragmas or a unique command line.

## Using the verifier

```text
python -I tools/objcmp.py src/probes/line_step.c line_step 066A:000E --fresh
RELOCATED: 269 bytes, all fixups bound

python -I tools/objcmp.py src/probes/text_cell.c text_cell 158D:0219 --fresh
EXACT: 64 bytes, all fixups bound

python -I tools/bcc.py src/probes/square.c --profile dazzle --fresh
<isolated cache path>/PROBE.OBJ
```

Profiles `bc20`, `bc30`, `bc31`, `tc30` all default to `-ml -O1`; `dazzle` selects
BC30. Repeated `--flag=...` arguments **replace** the profile flags. Use all
required options, e.g. `--flag=-ml --flag=-O1 --flag=-Z-`. `--fresh` bypasses the
cache and is required for acceptance. `--header path.h` copies and hashes an
explicit local header; CLI and Python API support this. Source is copied as
`PROBE.C`, standard headers into `INCLUDE`, with no ambient CFG/header paths.
The native C-to-OMF path is used; inline assembly via `-B` is deliberately rejected.

A new source needs an independent extent and all referenced addresses, either in
`evidence/bindings/<source>.json` (one entry per function; probes may instead use an inline
`/* objcmp: {...} */` comment), or `--size` and `--map` JSON.
Without an extent the result is UNKNOWN. Source metadata must refer to the same
target address. The function scanner's heuristic extents are not imported
automatically. Map keys are `symbols` (exact OMF names), `segments` (object segment
contribution origins), `groups`, and `absolute_symbols` (vendor absolute values).
`DS:xxxx` means `29BF:xxxx`, independently checked against the startup instruction
and its relocation. `_DATA` placement is never inferred from candidate contents.
The selected function's supplied placement binds its own code segment and labels.

The OMF reader checks record lengths/checksums, SEGDEF, GRPDEF, EXTDEF, PUBDEF
(including absolute symbols), COMDEF, LEDATA, nested LIDATA, and FIXUPP with
persistent target/frame threads. Repeated LIDATA literals repeat their fixups.
LIDATA fixup locations address encoded literal bytes, with fixups applied before
repetition, as specified in [TIS OMF 1.1, FIXUPP and LIDATA records](https://refspecs.linuxfoundation.org/elf/TIS1.1.pdf).
The next public in the CODE segment, or its full segment end, establishes the
object extent; a private function or extra literal after it may conservatively
cause a larger extent and failure. Prefer one function per probe file.
32-bit/absolute SEGDEF, unsupported records, byte/high-byte fixups and unsupported
self-relative forms fail closed; they are not silently ignored or masked.

Comparison resolves every fixup from supplied bindings. Every non-fixup byte and
resolved field must equal the oracle over the complete extent. Segment/pointer
fixups require exactly the same MZ relocation sites, with no missing or extra
sites, and the segment words must equal the mapped paragraphs. Near calls require
a bound target inside the placed CS window. DGROUP offsets require explicit
symbol addresses. Unbound fields never pass, even when their raw words happen
to match. Known RTL symbol misbindings are rejected against `functions.json`.
EXACT additionally requires the raw object bytes to equal the image; otherwise
fully resolved equality is RELOCATED. Size mismatches remain failures even if a
diagnostic prefix or candidate-sized comparison would equal. Diagnostics show
the first differing instruction with Capstone, side by side.

Exit codes: 0 = EXACT/RELOCATED; 1 = mismatch/unbound; 2 = UNKNOWN/input error.
`--json` emits the result, including successful fixup targets; `--dump` shows OMF
segments, publics and fixups. `python -I` uses the trusted installed Capstone at
`C:/tools/capstone-5.0.3` if the module is not on the isolated interpreter's path.

Cache keys cover compiler bytes, ordered flags, source bytes, all standard/local
header bytes, support-file hashes, runner hash and the fixed environment.
Compiler and catalogued support files are checked against the toolchain lock;
support/header/runner files without existing catalogue pins are recorded by hash
in each receipt, not claimed independently pinned. Source/header mtimes are fixed
to 1992-01-01 UTC for deterministic OMF dependency comments. Cache object hashes
are verified; corruption triggers compilation. Isolated copies, objects, logs and
receipts stay under `build/workers/compiler/`. The wrapper launches no persistent
runner. Measured fresh compile: about **0.087 s**; checked cache hit: **0.016 s**.

Validation: `python -B -I build/workers/compiler/strict_checks.py` passes **78
checks**, including fresh CLI verification of all nine candidates, both extent
errors, final-byte/table-tail corruption, missing/extra MZ relocations, wrong far
paragraph/DS binding, unbound near calls, CS range checks, absolute FIDRQQ proof,
FIXUPP threads, BASE16, nested/repeated LIDATA with fixups, malformed checksums,
unsupported fixups, compiler hash rejection, local-header cache invalidation,
deterministic fresh objects and cache corruption recovery. Synthetic fixtures
are separate tests of the verifier, never counted toward application coverage.
Fresh release/option/model receipts are `final_versions.json`, `final_axes.json`
and `final_models.json` in the same scratch directory. No assets, references,
accepted ledger or other worker files were modified; nothing was committed.
