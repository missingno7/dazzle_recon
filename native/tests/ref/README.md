# DOS/native differential diagnostics

These references are **reconstructed C compiled with Borland C++ 3.0, run under
MS-DOS Player**, with software callbacks. They are behavioural diagnostics, not
original-executable runs, binary matches, or hardware VGA/EGA tests.

## Run and reproduce

```powershell
python -B -I tools/native.py test
python -B -I native/tests/ref/capture.py images --update
python -B -I native/tests/ref/capture.py palettes --update
python -B -I native/tests/ref/capture.py primitives --update
python -B -I native/tests/ref/capture.py pcx --update
python -B -I native/tests/ref/capture.py gates --update
python -B -I native/tests/ref/capture.py fades --update
# Full-log comparison without replacing expected data:
python -B -I native/tests/ref/capture.py images --compare build/workers/n-diff/native-plot.bin
```

CMake's existing glob registers six runners: test_ref_images, test_ref_palettes,
test_ref_rng, test_ref_primitives, test_ref_pcx, test_ref_fades. Paths are derived
from the compiled source location; CTest's working directory is irrelevant.
Scratch files are created under build/workers/n-diff/. All fields of every
expected row are compared. On image/fade failure the complete logs locate the
first unequal plot: global/per-case index, scenario, x, y, colour, symmetry,
and RNG call count. REF_NO_CAPTURE=1 skips that extra diagnostic. On a clean
build with no DOS log cache, a failed test prints the regeneration command;
REF_REGENERATE_LOGS=1 also allows the executable to start that capture itself.

DOS captures are cached against source/input fingerprints. A cold image/fade
capture can exceed CTest's 30-second limit: prewarm with `capture.py images`
and `capture.py fades`, or run a failing native executable directly. Passing
runners need no DOS toolchain or network. `--force` rebuilds fresh; `--update`
replaces compact data only from DOS output. Every run's report.json records
sources, inputs, flags, compiler/linker/runner/library hashes and products.
data/captures.json preserves those hashes, driver-header hashes and full-log
count/CRC validation for the submitted captures; checked-in text uses LF.
Reconstructed modules retain BC++ 3.0 -ml -O1 with C0L.OBJ, EMU.LIB, MATHL.LIB,
and CL.LIB. tools/refharness.py also stages shared driver headers.

The compiler runner is C:/tools/nmlgcdos/msdos.exe (SHA-256
f7f6cb0a3e816c5edb13112d327c1bddbf7463fe7bf9a005ca1eb5317751bd02).
Execution/linking use C:/tools/msdos/binary/i386_x64/msdos.exe (SHA-256
0374576f0f970f5c342e482ed6d44d8e72bbfea3f063ccd3f38360ea81d5c53e),
because the former stalls on x87 math. BC++ 3.0 BCC.EXE is
 e56c6d228191e33665802e3affd9f44803af7c19c0c5a591c28ad10ffcdada6f;
TLINK 5.0 is ac0ac8b0084e06e9916ee12602aa4fcba998f7b702fc2adf4a213564288147c7.
Short nd-* artifact names avoid a TLINK current-directory-path failure:
identical objects linked at a shorter path after a misleading Out of memory
error. Changing optimization was neither the cause nor the fix.

## Coverage and model

imgmodel.h is compiled by both DOS/native drivers; compat.h bridges their ABI.
Native algorithms link from native/src/app/. Callback doubles replace only the
DOS driver's services (palette-cycle, key-polling and raster-op sink).

scenarios/images.tsv has 134 cases: the original 42 seed/12-step cases, two
additional seeds per image with Auto symmetries 1 and 3, then enhanced EGA sine,
grid, tiles, balls, ellipse, cycloids, daisies and squares. Lightweight iterators
run 60000 continuation calls to reach slow wraps/restarts in atom, hexagon,
tunnel and sweep. Whole-shape routines use 8--128 calls, producing thousands to
hundreds of thousands of plots. Each scenario records adapter, seed, steps,
symmetry and tick period. draw(1) has no extra caller plot; each draw(0) is followed
by one caller plot, preserving the original harness convention.

VGA uses 320x400 pixels; enhanced EGA uses 640x700 logical / 640x350 physical
coordinates. EGA writes reject odd translated Y and halve even Y; reads halve
both parities. Symmetry edges/endpoints follow the documented assembly model.
Scheduled ticks increment palette_elapsed_ticks and decrement nonzero countdown/
show timers. cycle_palette is the driver's simple offset increment/tick reset.
No SDL/real-time/retrace/BIOS events enter these scenarios.

images.txt compares setup fields, plot count, tuple CRC-32, RNG/cycle counts,
initialization/half/final framebuffer CRCs, and final palette ticks. Tuple CRCs
cover x,y,colour,symmetry as four little-endian words (CRC starts FFFFFFFF,
final xor FFFFFFFF). Full PLOT.BIN records are **14 bytes**: case word, x/y words,
colour/symmetry words, uint32 RNG-call count. RNG count is excluded from tuple
CRC. Full logs stay in ignored build output.

Reference-model corrections are explicit:

* The old far framebuffer pointer aliased every 64 KiB. Huge indexing now covers
  the full framebuffer. Original capture/input remain in data/images-legacy.txt
  and scenarios/images-legacy.tsv, solely as historical evidence.
* The ball map preserves the original driver's explicit height/width transpose.
  Its complete 64 KiB wrapped offset domain is zero-filled before map creation,
  so fetch-before-bounds-check sees defined padding rather than DOS heap bytes.
* Diamonds/wedges continuation shims set the exact uninitialized stack gate to
  positive 1, retaining steps. Initialization keeps its original zero assignment.
  This is a controlled policy, not proof of a universal original stack value.
* Grid's literal assembly-offset 012Dh identity check cannot recognize a logging
  callback. capture.py creates a scratch DZLE copy replacing only its two probes
  with EGA-model callback equality; the copy hash appears in its receipt. Native
  grid's backend check is correct. src/dzle.c stays untouched.

palettes.tsv retains eleven cases and adds all sixteen /A /M /F /4 combinations
on each adapter: 43 cases. Each new case includes the full buffer, length/CRC,
first 24 bytes and RNG count, including header/trailing bytes. VGA's original
builder is named build_ega_palette; EGA uses generate_palette.

rng.txt checks 8000 actual RTL rand results (four seeds), eighteen signed/
unsigned/long random(n) results, the initial unseeded value, srand narrowing,
and the captured time seed without consulting the native clock. Counted DOS
wrappers use the independently checked 32-bit LCG (state*22695477+1; result
(state>>16)&7fff).

primitives_driver.c emits 7540 rows: choose_color modes 0--8 (the requested
select_point_color), both adapters, signed coordinate extremes, bounded line/
ellipse sequences with zero/negative/large inputs, wrapped deltas, and 32-bit
ellipse overflow state. Negative radii are bounded to 512 samples.

fade_driver.c tests wipe in both modes, exclusive-or and moving bars on both
adapters, plus all four rectangle-swap reflections with odd/negative origins:
16 cases. The nonempty framebuffer fixture is
(offset*7+offset/101)&color_index_mask. Stream/count/RNG/cycle/framebuffer/tick
fields are compared. Raster-XOR remains an inert sink, matching the original
reference model: these tests do not validate hardware XOR or fade pacing.

## Undefined-state probes

Gate diagnostics call unmodified DZL2 after writing its exact local slot.
Diagnostic-link disassembly confirms BP-2 (diamonds) and BP-12h (wedges), or
SP-10 / SP-26 before argument and far-return pushes. data/gates.txt probes
0,1,-1,32767,-32768, three calls each. Positive retains steps; zero/negative
consumes six/seven refresh RNG calls. Both are real DOS behaviours; no unique
caller-independent automatic value exists.

PCX diagnostics supply two known sentinel bytes beyond lengths
0,1,2,3,4,62,63,64,65,80,320, using 07h/C7h and equal/different following bytes.
All 44 exact encoded streams/return lengths are in data/pcx.txt. Length two of
07h encodes C2 07 with equal padding, C1 07 07 otherwise: the last comparison
really precedes the bound check. Native after_row matches every case. This
does not establish a universal zero byte beyond DOS heap-allocated rows.

See [DIFFERENCES.md](DIFFERENCES.md) for diagnosed failures and proposed patches.
