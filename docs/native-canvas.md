# Native canvas: resolution-independent drawing (agreed design, 2026-11-03)

Decisions by the project owner (supersede the Hybrid overlay in docs/native-hybrid.md; Extended adapters stay):

| Topic | Decision |
|---|---|
| Source of truth | One canvas API used by all 42 images and 24 fades: plot/stamp, line, ellipse, fill, pixel read, XOR, rectangle copy, scroll, clear, pages and display start/split, in continuous coordinates. |
| Classic | The same API rendered at the original logical resolution into `dz_video.vram`; stays byte-exact against the DOS reference tests. |
| Colour | Full-resolution 8-bit index buffer; palette lookup on the GPU at presentation, so palette cycling animates everything already drawn. |
| Rasterization | CPU into the index buffer; the GPU only presents (existing indexed presenter). |
| Canvas size | Screen resolution (the physical output; resize restarts the current image). |
| Effect look | Decided per effect by rewriting its intention: continuous geometry (splines, lines, circles) drawn natively at full resolution; pixel/raster effects may stay pixel-based. |
| Per-effect choice | Each effect with a native version is selectable Faithful or Native in the settings (Images tab); default Native. |
| Pacing | Same visual tempo as the 486 original: timing is defined per original step, not per pixel. |
| Aspect | Native effects lay themselves out for the canvas's real aspect ratio; non-native effects use Original/Crop/Stretch or the Extended adapters. |
| Old code | Hybrid overlay removed once the canvas works; Extended adapters kept for non-native effects. |

Direction (owner, 2026-11-03): move gradually toward a fully native, efficient essence of the original program.
Efficiency follows the VGA model with modern equivalents: the image lives in a persistent GPU index texture
(video memory), the CPU uploads only what changed (dirty rectangles), display start and split are texture
coordinates, palette cycling is a palette upload with lookup on the GPU (DAC). GPU drawing is used only where a
measured effect needs it.

## Phases

1. **Canvas API with the Classic backend.** Every write/read path that touches display memory today (dvdt.c kernels,
   line/ellipse helpers, fades' direct reads/writes/copies, clears, mode set, page/display start, PCX save, logo)
   goes through the canvas. No visual or timing change: existing tests and DOS-reference comparisons stay exact.
2. **Screen-resolution canvas, Faithful rendering.** The canvas owns a screen-resolution index buffer with the
   VRAM aperture's page layout scaled; a faithful plot stamps the block covering one original pixel (block edges
   use the presenter's nearest-sampling rounding), reads sample the block, rectangle/scroll/copy operations scale.
   Oracle: for the same run, the screen-resolution canvas must equal the Classic presenter's output at that size,
   and RNG, work and virtual time must equal Classic.
3. **Native effects, one at a time.** Per effect: rewrite its intention against the canvas in canvas pixel units
   (aspect-aware layout, same visual tempo), keep the Faithful path selectable, add its own tests and captures.
   Owner decision after the pilot: crisp is the default and shipped style. Indexed
   antialiasing remains only as an experimental developer option; no further AA work.

## Implementation status

Phase 3's uniform surface, Native sine pilot and first seven-effect batch are implemented below.
The VGA-model presentation section at the end supersedes the earlier gather/upload
implementation and Faithful pan oracle; ordinary unpanned Faithful remains exact.
The phase-2 description and measurements in this section are historical; its
fixed-height padded storage has been replaced. The phase-3 section is authoritative.

Phases 1 and 2 implemented, 2026-11-03. Classic remains the developer and screensaver
default. Select the second backend with `--canvas screen`; `--canvas classic` selects
the reference backend. No Native effect or settings-dialog change is included.

### API and retained execution

`native/src/platform/canvas.c/.h` owns all native display-memory access. The interface
provides original-coordinate plotting/readback, literal address stores, EGA mask/XOR,
spans, filled rectangles, overlapping rectangle moves, full-memory clears, page selection,
display start/split and borrowed logical-memory views for DAC bootstrap and PCX export.
The legacy logo decoder's destination stores also pass through the interface, including
RLE runs spilling across rows. Only canvas.c names `dz_video.vram` in native/src.

The existing shared `line_step`, `draw_ellipse` and square/fade loops remain readable
application algorithms, with their address headers and original point order. They emit
through the existing plot/read callbacks into the canvas. They do not acquire new geometry,
sampling, RNG calls or charges. The page-base addition, wrapped DI and mode-13h byte-swap
address calculation moved from the native DVDT helper into the canvas unchanged. All
existing virtual-time/work charges remain at their original call sites.

Hybrid's source, hooks, quality field/option and dedicated scripts/tests are removed.
Its design is marked historical in native-hybrid.md. Extended's independent side adapters
remain operational with Classic; **Extended + Screen uses Original**, with no side allocation.
Text retains the existing presenter and saver/preview policy.

### Mapping rules

The viewport is still `dz_present_viewport`, using the presenter's actual physical output
size. Original covers the fitted 4:3 content area, Crop clips the covering rectangle, and
Stretch covers the output. The visible Screen texture excludes black bars and is drawn
at its integer output rectangle, at exactly 1:1.

At a mode/output/policy change, the canvas renders a temporary **coordinate ramp** with
the active SDL nearest renderer: logical `(x,y)` contains integer `1 + y*W + x`, encoded
losslessly in RGB. The existing viewport is used. One readback yields the exact source
column and row selected for every output pixel, including backend rounding at fractional
edges. This probes coordinates only; it never reads or resamples old application ink,
calls RNG or services the simulation clock. Both software and Direct3D11 were verified.

Let `nx[x]` be the number of visible output columns selecting logical column x, with a
minimum of one storage sample for cropped/unsampled columns. Let `ny` be the largest
number of output rows selecting any logical row, also at least one. For the aperture
capacity `C = 65536 * pixels_per_byte`, allocate `ceil(C/W)` original rows, each with
`ny` raster rows and a stride of `sum(nx)`. Pixel address a owns the block at:

```
row = a / W; column = a % W
offset = row * ny * stride + sum(nx[0:column])
block = nx[column] columns by ny rows
```

A store fills that index block, and a read takes its first sample. The logical
524288-byte receipt is maintained as well: it preserves inactive-mode memory and makes
the unchanged canonical hashes, PCX reads and full-state receipts available. Guard
samples preserve reads even when the output is smaller than the logical raster.

For visible output row y, use the probed logical row sy. As before, the original memory
row is `sy > split ? sy-split-1 : start+sy`; each source pixel address wraps modulo C.
Gather the corresponding stored block values using the probed column runs. This also
handles a wrap in the middle of a row. Fixed-height row padding is deliberate: simply
shifting a fractionally scaled aperture by start rows would shift its nearest-rounding
phase and fail the oracle. The gathered visible index canvas has the exact original
nearest block edges after every pan or split, independently of page alignment.

Per-aperture-row versions avoid gathering unchanged rows; visible rows are compared
before changing the upload revision. The existing palette cache, INDEX8 texture/palette
path and idle presentation skipping are retained. RGBA remains an explicit/fallback
renderer path and resolves the same visible indices. Palette-only changes upload no
index data on INDEX8.

### Resize, limits and exceptions

Resize or a presentation-policy change discards the old scaled storage and clears the
logical aperture. It requests one current-image rebuild at the next safe outer image
boundary in Auto, Semi-Auto (including paused drawing) or the slice driver. That boundary
clears again and calls the original image initializer. While an old generator finishes
on the stack, the visible canvas stays cleared. No old content is resampled. Mode setup
instead imports the current logical memory after its original clear/preserve operation;
it does not introduce an extra image restart. Text mode invalidates any old scaled mirror.

There are **no fade-specific pixel-equality exceptions**: XOR, Quilt's ordered overlapping
swaps, tetrYs/plasma reads, clears and display-address changes all retain their original
operations. Screen blocks contain one exact index, including for cropped/hidden pixels.
Geometry changes intentionally start a new image and therefore are outside the fixed-size
state-equality oracle. The brief cleared interval before that boundary is defined above.

Transient mapping readback and permanent canvas allocations each have a 256 MiB limit;
allocation/readback failure reports a presenter error. No reduced-density canvas is used.
Other SDL hardware backends, live DPI/monitor migration and screensaver-host resize were
not exercised here; their canvas sizing uses the same physical-output query.

### Verification

`python -B -I tools/native.py test`: **38/38 passed**. All six `test_ref_*` sources and
their reference data are unchanged and pass. `integration-hashes.json` and
`acceptance-reference.json` are unchanged. Extended's center/side, lifecycle and page tests
still pass; its old Hybrid fallback assertion now checks the documented Screen fallback.

`verify_canvas.py` runs the complete original program with seed 1, comparing Classic and
Screen at eight exact virtual deadlines per run. It compares full state files (the entire
logical aperture/registers/palettes, RNG state/calls, work units, time and image/fade/draw
diagnostics) and every pixel of the production nearest Classic and 1:1 Screen presenters.
The Auto fixture includes fades, hidden pages, display-start scrolling and line-compare
splits; the Semi-Auto fixture selects sine, lotsadots and cycloids and invokes Quilt/tetrYs.
VGA200 retains the original disabled pan behavior.

The full matrix is VGA400, VGA200, EGA350 and EGA700; Original, Crop and Stretch;
1920x1080, 2560x1440, 3840x2160, 1366x768 and 641x479: **960 exact-time comparisons per
renderer, zero state or pixel differences**. The oracle executable substitutes only the
periodic presentation callback: the real screen store is active throughout, while uploads
and readbacks happen at observation deadlines. Original application execution, timer/input
services and charges are retained. Regular production presentation is used for the CPU runs.

CTest runs the smaller whole-program matrix (64 comparisons). `test_canvas` additionally
checks literal stores, XOR, overlapping rectangle moves, logo-style stores, palette changes,
mid-row wrap, start/split, INDEX8/RGBA equality, idle skips, mode invalidation and resize/policy
clears with a single rebuild at the slice boundary.

Reproduce the extended oracle and hardware selection:

```powershell
python -B -I native/tests/verify_canvas.py build/native/canvas_oracle.exe build/workers/s-canvas/oracle
$env:DAZZLE_CANVAS_ORACLE_BACKEND='direct3d11'
python -B -I native/tests/verify_canvas.py build/native/canvas_oracle.exe build/workers/s-canvas/oracle-d3d11
Remove-Item Env:DAZZLE_CANVAS_ORACLE_BACKEND
```

Receipts: `build/workers/s-canvas/oracle-final/equivalence.json` (software) and
`oracle-d3d11-full/equivalence.json` (Direct3D11), plus unchanged canonical test receipts.

### CPU and memory

`measure_canvas.py` runs the production executable for 20 real seconds per case, sequentially,
with seed 1, Original policy and the default AUTO/Direct3D11 renderer. The scripted whole
program switches to Semi-Auto sine at eight virtual seconds. Windows `GetProcessTimes`
reports kernel + user time; percent is of one core. Memory is process peak working set and
the largest sampled private committed size, sampled every 50 ms. These include SDL/driver
resources and the transient coordinate probe, rather than just canvas arrays. The state
files at 19.9 virtual seconds agree between backends at both sizes.

Performance results are recorded by `build/workers/s-canvas/measure-final/cpu-memory-20s.json`.

| Output | Canvas | CPU seconds / wall seconds | One core | Peak working set (MiB) | Peak sampled private (MiB) |
|---|---|---:|---:|---:|---:|
| 1920x1080 | Classic | 2.625 / 20.040 | 13.10% | 67.61 | 72.32 |
| 1920x1080 | Screen | 5.312 / 20.032 | 26.52% | 102.51 | 111.12 |
| 3840x2160 | Classic | 2.828 / 20.037 | 14.11% | 67.58 | 71.51 |
| 3840x2160 | Screen | 7.734 / 20.040 | 38.59% | 174.54 | 251.49 |

All four runs completed normally in 20.08Ă˘â‚¬â€ś20.13 wall seconds and left no processes running.
The Screen canvas's own dynamic storage for VGA400/Original is 5,109,120 bytes at 1080p
and 20,413,440 bytes at 4K, excluding the logical receipt, static bookkeeping and SDL
resources. Peak process private memory also includes mapping-time target/readback buffers
and driver allocation retention. These are single host samples; an earlier exploratory
batch ranged from 14Ă˘â‚¬â€ś16% Classic and 23Ă˘â‚¬â€ś47% Screen, so the table is not a performance
guarantee. The Faithful result is identical; Screen spends more CPU and memory maintaining
the output-resolution aperture and uploading output-resolution indices.


## Phase 3: uniform surface and Native sine pilot

This subsection records the 2026-11-03 pilot. The first-batch section below
supersedes its list of available effects; its canvas and VGA presentation contracts remain current.

Implemented 2026-11-03. `native/scr/` is unchanged. Classic remains the default
backend. With Screen, converted effects default to Native: `--native sine` and
`--native all` enable the one available Native implementation; `--native none`
selects Faithful sine. Other images remain Faithful.
`--strokes crisp|aa` selects the experiment. Both options require `--canvas screen`.
The per-image settings column is deferred until stroke/composition choice; this
pilot does not change saver settings.

### One uniform physical raster

Each full aperture page is `output_width * output_height` square physical pixels,
with one-byte indices and an output-width stride. Enough complete pages cover the
original aperture capacity. The final partial DOS page has a complete physical
page allocation for Faithful guard reads; Native scanout wraps at the original
aperture extent rounded to physical rows, `round(C/W * output_height/H)`. Native page
origins are rounded from `paragraph * 16 * pixels_per_byte / logical_width` original
rows. The original logical receipt still wraps at exactly 65536 plane bytes.
There are no repeated raster samples or per-row padding inside a page.

The existing nearest-coordinate probe yields monotonic X/Y cell boundaries inside
the policy's content rectangle. A Faithful write fills that actual physical block;
unsampled/cropped cells keep a logical guard value. Reads sample the block centre.
Native strokes draw directly into this same raster across the whole output, with
the original mode's physical unit aspect, then the effect's explicit width-dependent
amplitude gain. They never sample an enlarged Faithful image.
There is no overlay, retained geometry, resampling of old ink or ownership queue.
Later writes overwrite earlier ink, even if the logical receipt already holds the
same index. EGA XOR resolves and modifies each actual destination sample.

Scanout now always shifts the uniform raster by
`round(display_start_row * output_height / logical_height)` physical rows. A split
restarts at zero at `round((line_compare+1) * output_height / logical_height)`;
`line_compare >= logical_height` disables it. Pages and coverage move together.
Fractional Faithful block phases can therefore differ while start/split are active;
returning to unpanned/unsplit scanout restores the original stored block boundaries.
Within full pages, the row displacement from the Faithful nearest map is bounded
by twice the maximum difference between a probed row boundary and the corresponding
uniform boundary, plus one rounding pixel. At the requested 1080p/4K Original wide and Stretch sizes this fits within one
logical block of row-phase error; very small outputs can differ by up to three
physical rounding rows. Crop/letterboxed output can have a larger
bound because its Faithful vertical rectangle differs from the full Native output.
Native wraps the partial final DOS row at a whole rounded physical row; it does
not introduce a horizontal mid-row tear through a Native stroke. The resulting
wrap-phase displacement is bounded by one original row plus half a physical row.
The original program's page/reveal operations use the first two full pages.
Classic arbitrary mid-row wrap remains exact. No pixel-equality claim is made for a
mixed Native image against a Faithful image.

Resize/policy changes discard all physical ink and AA metadata, clear the logical
receipt and request the existing safe-boundary image restart. Mode changes import
the logical receipt after the original mode operation. Text invalidates the raster.
Mapping readback and permanent canvas allocation retain separate 256 MiB limits;
there is no lower-resolution fallback.

### Fades, reads and clears

`dz_canvas_clear` covers the entire physical aperture and logical receipt, including
all page margins. Literal page clears also clear the corresponding full physical
pages; alternating-byte background patterns use the first byte in the margins,
while the original literal loop writes its exact pattern inside Faithful blocks.
Rectangle fills/moves extend touching logical page edges to physical page edges
when Native content is present. Moves copy the raster (and both AA channels), with
an overlap-safe temporary rectangle and nearest mapping when rounded sizes differ.

Every sampling callback reads the canvas. To preserve the program schedule, it
also reads the unchanged logical receipt with the same single original work charge.
The logical colour controls branches, assignment/empty-column tests, time and RNG;
the canvas colour controls reconstructed pixels. Quilt exchanges canvas samples
in the original ordered reflected swaps. tetrYs carries a parallel array of sampled
canvas block colours. Plasma carries parallel corner/edge colours down the same
recursive tree, reusing each original noise word. This is necessary: allowing
Native pixels to decide tetrYs's empty-column traversal would change virtual work.
No extra random draws, timer service or work charges are introduced.

Crisp reads return the stored index. AA reads choose the foreground at coverage
>=128, otherwise the underlying index; they never quantize blended RGB. Faithful
writes, XOR and sampling reconstruction flatten AA at the written cells. Palette
fades retain the index and AA channels and recolour the entire physical canvas.
A completed spatial eraser (`v/w/y/z`, when its original `display_cleared` is set)
clears the entire raster to index zero, without altering the scheduling receipt.
This prevents Native ink in logically empty tetrYs columns from surviving its end.
Interrupted fades retain only intentional untouched pixels, with margins already
handled at entry.

`F` below means: clear the area outside the Faithful rectangle on **all pages**
once at actual dispatch; execute the original operations inside it against the
canvas. This is a defined fallback for unadapted procedural/sample effects, rather
than pretending to extend their fixed arrays. `W` means the original erasure/paint
coordinates cover the whole output grid, with no viewport margins left behind.

| Key / fade | Canvas-wide behaviour |
|---|---|
| a rAndomize | F; square stamps and per-pixel EGA XOR overwrite actual canvas cells. |
| b Blister | F; original bitmap stamps replace canvas blocks. |
| c Countdown | F; digits stay pixel art; outside-panel XOR reads actual destination indices. |
| d Drip | F; original column paint/erasers, no fictitious pixel dragging. |
| e Exclusive-or | F; actual per-pixel XOR/replace on the canvas. |
| f Firefly | F; heads and every trailing eraser stamp overwrite the canvas. |
| g Gas cloud | F; parallel canvas-colour plasma sampling; full hidden-page clear and uniform reveal. |
| i spIral | F; original independent radial line stamps overwrite canvas cells. |
| j Juggle palette | Retain the whole raster/AA channels; shared palette animates every page/pixel. |
| k Kleenup | F; original XOR then replace fans operate on canvas indices. |
| l Linewipe | F; old-line erasers/XOR and replacement lines overwrite canvas cells. |
| m Mesh | F; original sparse lattice stamps; no connecting strokes. |
| o firewOrks | F until its original mode reset; reset discards all raster/AA ink, then original particles. |
| p Pan/scroll | Preserve all pages/channels; rounded physical-row start/split scanout above. |
| q Quilt | F; ordered centre-sampled canvas cell exchanges, including row/column reversals/reflections. |
| r Rectangles | F; new outline paint/zero strokes, not a rotation of old content. |
| s Starburst | F; original heads and all delayed zero-tail stamps. |
| t Twist | F; repeated original polygon edges paint the canvas; no image-sheet warp. |
| u Umbra | F; original edge fans overwrite canvas cells. |
| v moVingbars | W; full-output bar paint and trailing zero bars; full-raster clear on completed erasure. |
| w Wipe | W; full-output coloured columns or zero rows; full-raster clear on completed erasure. |
| x heXagon | F; original contours/XOR; a chained Starburst gets its own F entry. |
| y tetrYs | F; canvas-colour coarsening/rising blocks on original logical control traversal; full-raster clear on completion. |
| z Ziti | W; full-output diagonal front/trailing zero front; full-raster clear on completed erasure. |

### Continuous sine and stroke choices

The original `sine()` still selects both amplitudes with the same two `dz_random`
calls, updates the same float phase/modulators/amplitudes and applies the same
zero-X colour rule and EGA coordinate rounding. The new prepare hook observes
amplitude/phase **before truncation**. At the original outer plot, the original
kernels still update the logical receipt and symmetry diagnostics with the same
charge; only their Faithful raster stamps are suppressed. Native raster work then
runs without advancing the simulation. Other images and all selection logic stay
on their original paths. There is no additional randomness, even private randomness.

`effects/sine.c` interpolates amplitudes between successive original states and
evaluates sine/cosine of the continuous phase. It joins those parametric samples
with round-ended capsule strokes. A second-derivative bound limits chord error to
1/8 output pixel (4096-subdivision guard). Initialisation, page changes and resize
rebuilds break the path. Sign reflections and exchanged-axis branches follow the
original symmetry flags and colour order; they never join separate branches.

Owner review corrected the pilot's coordinate metric and rejected its tiling.
There is now **one centred figure**, with the same vertical range as Faithful /
Original. A logical unit is not a physical square: the original 4:3 mapping gives
these X/Y unit ratios before any widescreen amplitude change:

| Mode | Original software coordinates | Physical X unit / Y unit |
|---|---|---:|
| VGA400 | 320x400 | 5/3 = 1.666667 |
| VGA200 | 320x200 | 5/6 = 0.833333 |
| EGA350 | 640x350 | 35/48 = 0.729167 |
| EGA700 | 640x700 (physical rows halved) | 35/24 = 1.458333 |

**General "spread wider" policy:** for Native effects whose original geometry is
scaled by screen width, keep the original physical unit metric and vertical range,
and scale the horizontal parameter range with the real canvas width. For output
`W,H`, software half sizes `HW,HH`, fitted Original dimensions `W0,H0`, use:

```
H0 = min(H, 3*W/4); W0 = 4*H0/3
physical_x_unit = W0/(2*HW); physical_y_unit = H0/(2*HH)
horizontal_range_gain = W/W0
output_x = W/2 + x * physical_x_unit * horizontal_range_gain
output_y = H/2 + y * physical_y_unit
```

For sine, this widens its independent horizontal amplitude range (including
reflected/swapped branches) without changing oscillator state, random choices,
phase/modulator rates, colour events or time. On 4:3 the gain is one and the
figure has Original proportions. At constant height, a 1920-wide canvas gives
4/3 of Original's horizontal range, and 2560 gives 16/9. There are no repeated
figures or envelope fit that reduces height. Portrait/narrow outputs use the
fitted Original height, with no extra horizontal gain. The raster remains a
uniform square-output-pixel grid; its logical coordinate units have the original
mode's physical aspect. Stroke diameter is still the square root of the fitted
Original pixel area, at least one physical pixel (VGA400: about 3.49 px at 1080p,
6.97 px at 4K).

Crisp capsules intersect physical pixel-centre scanlines and write contiguous index
spans. Palette lookup remains on the existing GPU INDEX8 path. AA adds two bytes per
raster/visible pixel (coverage and underlying index), with an opaque interior span
and a one-pixel coverage fringe. The presenter first looks up both indices, then
blends their RGB with integer rounding. This pilot intentionally implements that
reference on the CPU, using ARGB upload even when INDEXED was requested.

AA stores one underlying index, not an unbounded history of blended colours.
A later different partial stroke collapses the old edge to its dominant index
before compositing; repeated segments of the same colour retain maximum coverage.
This approximation prevents palette-dependent RGB from entering the canvas, but
can simplify multicolour edge intersections. Fades/readback flatten to a dominant
index. Crisp needs neither approximation nor additional coverage lifetime rules.
**Owner decision: crisp is the default and shipped style.** It keeps connected
contours, exact palette indices and simple fades/XOR/readback. `--strokes aa`
remains explicitly experimental and developer-only; its existing CPU reference
and overlap/readback approximation are frozen. No further AA work is planned here.

### Evidence and limits

The regenerated six full-resolution PNGs and labelled adjacent pairs live in
`build/workers/s-native/review/captures/`: Faithful / Original left, Native crisp
right, at **1920x1080, 2560x1080 and 3840x2160**. Each uses seed 1, virtual 8.7 s
and the same scripted clear/selection. Pair widths are 3840, 5120 and 7680 pixels,
with a 56-pixel caption above the original unscaled images. All three pairs were
inspected: one complete lens, matching Original height, restored unit proportions
and the specified extra horizontal reach. Smaller previews are also supplied.
The six raw PNGs are byte-identical before and after the performance changes
(`review/captures-before/` versus `review/captures/`), including Faithful output.

Verification/measurement receipts and reproduction commands follow below.


`python -B -I tools/native.py test`: **41/41 passed**. Classic integration and
acceptance pinned files were not edited; all six DOS-reference suites pass.
`test_native` checks 48 size/policy/style combinations (480x320, 641x479,
1280x720, portrait 720x1280, 1920x1080, 3840x2160; four policies; two styles),
Original-unit ellipse aspect, continuous ink, same-index overwrite, both pages,
rounded pan/split and partial-aperture wrap, page/whole clears and resize.
It also compares crisp capsules to an independent per-pixel Euclidean-distance
oracle, and checks every AA presenter pixel against the CPU blend formula before
and after a palette-only change, for both renderer selections.
`test_native_fades` completes all 23 dispatched fades with Native sine in both
styles, plus the actual Pan boundary with visible/hidden Native pages. It compares
full logical video state, time and RNG; checks margin fallback, completed eraser
clears, all-page/AA clears and existing allocation canaries. Existing tests retain
the interrupted/repeated fade variants and safe-boundary restart coverage.

`native_oracle` substitutes only periodic presentation, keeping the actual original
program/clock/input/work accounting. At exact virtual deadlines it writes the
full logical state receipt plus **440 declared POD application globals**, symbolic
current-image function identity, image/fade configuration values and all allocated
palette-buffer bytes. Raw pointer addresses, resource strings and unused heap
cache storage are excluded; pointer-bearing selection records are normalized to
their values/function identity. Both the original sine state and every scheduling
field are compared, not only a hash of visible ink. The four modes, three sizes,
three styles and eight deadlines give **288 full-state observations** (192 Native
vs Faithful comparisons); all agree, including Quilt/tetrYs and a later Faithful
lotsadots overlay. The default CTest fixture repeats 24 observations at 641x479.
The full Faithful nearest oracle gives **960 exact state/pixel comparisons** over
four modes, three policies and five sizes, including page pans/splits and byte wrap.

Current review receipts: `build/workers/s-native/review/test-final.log`,
`review/id/identity.json`, `review/oracle/equivalence.json`,
`review/captures/identity.json`, `review/hardware.log` and
`review/performance-summary.json`. Earlier pilot receipts remain in their original
`build/workers/s-native/` directories. Commands:

```powershell
python -B -I tools/native.py test
python -B -I native/tests/verify_native.py build/native/native_oracle.exe build/workers/s-native/review/id
python -B -I native/tests/verify_canvas.py build/native/canvas_oracle.exe build/workers/s-native/review/oracle
python -B -I native/tests/verify_native.py build/native/native_oracle.exe build/workers/s-native/review/captures --captures
python -B -I native/tests/measure_native.py build/native/dazzle.exe build/workers/s-native/review/measure
```

### Owner-review verification and cost

The new aspect regression uses **actual original sine evolution and plot kernels**
for 5000 steps with seed 1, separately rendering Faithful and Native. It covers all
four modes at 640x480, 1440x1080, 1920x1080, 2560x1080 and 3840x2160. Bounding box
height must agree within quantization/stroke-footprint tolerance; bounding box
aspect, divided by the specified horizontal gain, must agree within 3.5%. This
would fail both the old uniform-logical-unit mapping and the old tiled layout.
Representative 4:3 bounds at 1440x1080:

| Mode | Faithful bounds | Native bounds | Faithful / Native aspect |
|---|---:|---:|---:|
| VGA400 | 689x273 | 696x276 | 2.524 / 2.522 |
| VGA200 | 553x437 | 562x442 | 1.265 / 1.271 |
| EGA350 | 547x497 | 552x498 | 1.101 / 1.108 |
| EGA700 | 587x269 | 592x270 | 2.182 / 2.193 |

These include the different rectangular DOS pixel and round native stroke
footprints; Native keeps continuous pre-truncation coordinates. The full bounds
receipt is `build/workers/s-native/review/aspect.log`.

Profiling uses a separate GCC 12.2 `-O2 -g -pg` build, SDL 3.4.16, the same seed,
script, 28.6-virtual-second program run and production renderer. Windows gprof
needs `-Wl,--disable-dynamicbase` on this profiling executable: the initial ASLR
attempt collected records at relocated addresses but produced no symbol samples.
The corrected profile identifies `capsule_span` and repeated `sincos` as the main
Native application-code costs (about 22% and 18% of sampled main-image time).
Those percentages exclude SDL/driver DLL attribution and include profiler
overhead; production CPU numbers below use the ordinary uninstrumented build.
Profiles: `review/profile-native/flat.txt`, `review/profile-faithful/flat.txt`.

The original tiled pilot measured Native/Faithful at 39.30%/20.74% of one core at
1080p, 65.81%/33.53% at 4K. One figure alone changes that workload substantially.
The new **before** executable already contains the corrected aspect and single
spread figure, so before/after performance changes compare identical geometry.
Twenty-real-second samples start after virtual 8.5, with Semi-Auto sine active
throughout. Seed 1, Original policy, AUTO/Direct3D11, GCC 12.2 RelWithDebInfo;
Windows process kernel+user time, peak working set and private bytes sampled every
50 ms. Startup is excluded from CPU deltas; process peak working set includes it.
No other DAZZLE benchmark/profile runs execute concurrently. Single-host samples
are evidence of this run, rather than a throughput guarantee.

| Output | Style | Original tiled pilot, one core | Corrected geometry control | Shipped rerun | Shipped CPU / wall s | Peak WS / private MiB |
|---|---|---:|---:|---:|---:|---:|
| 1920x1080 | Faithful Screen | 20.74% | 17.16% | 20.90% | 4.188 / 20.032 | 102.45 / 110.10 |
| 1920x1080 | Native crisp | 39.30% | 23.16% | 23.25% | 4.656 / 20.027 | 102.49 / 111.25 |
| 3840x2160 | Faithful Screen | 33.53% | 31.89% | 25.12% | 5.031 / 20.028 | 177.76 / 260.77 |
| 3840x2160 | Native crisp | 65.81% | 35.56% | 30.50% | 6.109 / 20.029 | 177.64 / 252.45 |


The clear saving is the requested change from three smaller figures to one
correctly proportioned figure. The corrected control and shipped renderer use
the same implementation; their spread also shows host/driver timing variability.
Do not attribute the difference between those repeats to an extra optimization.
All valid before/after 28.5-second logical receipts are byte-identical, SHA-256
`bf3afa4c13d53911586b2b20b59f06c1a7f4e35d57e73941e4465aa7c710d12e`.
The shipped cases completed virtual 28.6 in approximately 28.68 wall seconds.

Additional cheap-win hypotheses were implemented, checked for unchanged PNGs and
measured, then **rejected** because they did not give a consistent overall win:

- Per-output-row versions, conditional copies and partial GPU uploads reduced
  some 4K costs, but the 1080p Native sample rose from 23.16% to 27.93%, and 4K peak
  working set rose from about 178 to 228 MiB. Driver/SDL resource retention was
  not isolated. Partial uploads were removed.
- Full GPU uploads with per-write row bookkeeping, then presentation-only row
  comparisons, had mixed results (1080p Native up to 29.65%, 4K up to 44.55%).
  Geometry/trig sharing and skipping the unused crisp inner capsule did not
  establish a net benefit either. Those optional changes were removed as well.
  The shipped canvas/presenter retain the pilot's simple copy/upload behaviour;
  this review does not add per-pixel metadata or expose new renderer state.
- A deeply nested benchmark directory exceeded the original startup's 80-byte
  `getcwd` buffer. Its prompt consumed a scripted key and left Auto active;
  the misleading low sample is discarded. `measure_native.py` now uses a short
  run directory and verifies sine/no-fade headers at both endpoints before
  accepting a record. This is a harness fix, not a change to application timing.

Receipts: `review/performance-summary.json`, `review/measure-before/`,
`review/measure-valid/` (shipped), with rejected experiments kept under their
named directories for inspection. No AA timing/implementation development was
performed in this review; existing AA regression checks remain active.


Limits: this is one Native effect; other images remain Faithful. Quilt/tetrYs
sample and reconstruct indices rather than transporting subpixel AA history;
Plasma shares its original control traversal. Cropped/unsampled Faithful cells use
logical guards. Only the requested physical sizes/policies and the documented
software oracle/production D3D11 measurements are claimed here; live monitor/DPI
migration, device loss, allocation exhaustion and screensaver-host lifecycle were
not newly exercised. No screensaver UI or DOS matching sources/evidence were changed.

## VGA-model presentation (s-vram, 2026-11-04)

Screen now keeps one persistent GPU INDEX8 texture for the **complete uniform
aperture**, including hidden pages and the final guard page. CPU writes rasterize
directly into that aperture and union changed bounds in 256-pixel spatial cells
(shifts and one union per stamp/stroke; at most two at a ring wrap). Intersecting
bounds coalesce once per frame. Sparse regions remain small. Within a single page,
the GPU renderer merges more than two regions, or a changed area above one eighth
of the page, into one bounding rectangle. The D3D11 compatibility path instead
coalesces dense writes to a stable page-sized rectangle. The presenter uploads the resulting
rectangles with `SDL_UpdateTexture(rect)`, using the aperture's
original stride. There is no visible index buffer, row gather, padded-row remapping,
row version table or comparison of output-sized rows. A mode/size change prepares
address-to-cell and edge tables; plotting uses lookups, shifts and a dirty-rectangle
union, with no prefix sums or division. Revision bookkeeping occurs at observation/
upload time, rather than at every plot. Full clears and page/rectangle copies mark
their complete destination, including Native margins and AA channels.

Display start, split and ring wrap draw one to three 1:1 textured rectangles. They
change no indices and cause no uploads. Palette-only changes call the existing
SDL palette API and upload no indices. Expose redraws reuse the texture; an idle
image skips upload, drawing and present. A second presenter with an older revision
uploads the full aperture rather than accidentally consuming another presenter's
dirty receipt. Explicit RGBA/AUTO fallback and experimental AA resolve only the
dirty rectangle, except that palette changes must resolve the complete aperture.

Screen prefers `SDL_CreateGPURenderer` (GPU/D3D12 on this host); creation failure
falls back to the normal renderer. Explicit `SDL_RENDER_DRIVER` hints are honored.
Classic keeps the normal default renderer (D3D11 here). Screen textures use STATIC
access with rectangle updates, avoiding the GPU renderer's otherwise unused CPU
streaming-lock bitmap; Classic/text retain their existing streaming textures.
SDL owns all devices, shaders and transfer resources. No new pacing, swap interval,
application thread, SDL_GPU resource manager or shader assets are introduced.

### Rounding contract and oracles

**Changed deliberately:** Faithful pan/split now uses the same uniform physical
row shift as Native. Start is `round(start_row * output_height / logical_height)`;
split is `round((line_compare+1) * output_height / logical_height)` and restarts
at physical row zero. Wrap is at `round(C / logical_width * output_height /
logical_height)`, rounded to a complete physical row. This also replaces Faithful's
old exact horizontal mid-row DOS wrap with the uniform raster's whole-row wrap.
Fractional nearest block phase during pan/split can therefore differ from Classic.
The phase-3 displacement bounds above apply to Faithful too; Crop/vertical bars
can produce larger phase differences because faithful content differs from the
full output height. This is the cost of moving existing ink by texture coordinates
instead of reconstructing each logical row. Returning to start zero with split
disabled restores the exact probed nearest boundaries. Classic retains its exact
original scanout, including arbitrary mid-row wrap.

The whole-program oracle continues to compare **all logical state** at every
deadline, including pan/split. Unpanned/unsplit Faithful pixels must still equal
Classic exactly. Pan/split pixels instead compare against an independent test-only
CPU sampler of the rounded uniform aperture; the production path never calls it.
Native geometry, stroke coverage, overwrite, page, fade, state/time/RNG and AA tests
retain their existing expected results. No pinned integration or acceptance hashes
are re-recorded. Resize retains the safe-boundary restart and cleared interval;
physical stores are suppressed while that restart is pending.

Classic keeps its small (at most 224000-index) visible-row cache, exact CPU address
wrap and original fixed-size upload. Partial row uploads were tried and removed:
the measured CPU benefit was inconsistent, and restoring the original upload did
not eliminate the host's timing spread. No extra per-plot tracking is added to
Classic. Screen upload counters are developer receipts; Classic's original upload
calls remain uninstrumented. No Classic presentation speedup is claimed.

### Verification and reproduction

`python -B -I tools/native.py test`: **41/41 passed**, including software rendering,
DOS differential fixtures, all 24 Native fades and unchanged pinned hashes.
The full `verify_canvas.py` matrix passed **960 comparisons per renderer** on
software, Direct3D11 and SDL GPU: four modes, three policies, five sizes through
4K, Auto and scripted fades/images, eight exact deadlines per run. Both hardware
backends also pass `test_present` (216 indexed/RGBA comparisons), `test_canvas`
(including one-pixel and separated-region upload byte counts, palette/register/expose without index uploads,
partial-update persistence and a three-quad wrap), and `test_native` (including
unchanged crisp distance/coverage and AA RGB expectations). Native page origin
is prepared on configure/page selection, with no per-stroke page division.
The stroke span helper also replaces repeated libm min/max/ceil/floor calls with
comparisons and clipped nonnegative integer rounding. All six Faithful/crisp PNGs
at 1920x1080, 2560x1080 and 3840x2160 are byte-identical before and after this
arithmetic change (`caps-before/`, `caps-final-last/`); the mathematical capsule and
AA RGB regression expectations are unchanged.
Native strokes mark one clipped bounding rectangle, rather than bookkeeping every
scanline. Hardware tests additionally compare partial stroke uploads at tile edges,
clipped corners and the physical ring wrap against the full CPU raster.

```powershell
python -B -I tools/native.py test
python -B -I native/tests/verify_present_backends.py build/native/test_present.exe direct3d11 gpu
python -B -I native/tests/verify_present_backends.py build/native/test_canvas.exe direct3d11 gpu
python -B -I native/tests/verify_present_backends.py build/native/test_native.exe direct3d11 gpu
$env:DAZZLE_CANVAS_ORACLE_BACKEND='gpu' # also software, direct3d11
python -B -I native/tests/verify_canvas.py build/native/canvas_oracle.exe build/workers/s-vram/oracle-gpu
Remove-Item Env:DAZZLE_CANVAS_ORACLE_BACKEND
python -B -I native/tests/measure_vram.py build/native/canvas_bench.exe build/workers/s-vram/repro --backend default --affinity 1 --raster
```

Receipts are in `build/workers/s-vram/`: `test.log`, `hardware-*.log`, the three
`vga-final-*/equivalence.json` files, `paired.json`, `controls.json`,
`final-window/cpu-20s.json` and the final summary. The earlier unpinned D3D11 and
GPU experiments are retained in their named directories. Timing uses GCC 12.2
RelWithDebInfo and SDL 3.4.16, seed 1,
Original presentation, the production program/clock/presenter with a test-only CPU
observer. Startup is excluded: the two endpoints are virtual 8.5 and 28.5 seconds,
both verified as Semi-Auto sine with no fade. Static uses a fixed indexed pattern
after a one-second warmup. Runs are sequential, with no concurrent DAZZLE tests or
profiles. Final window output dimensions and renderer are queried at both endpoints.
Windows process kernel+user CPU deltas are percentages of one core over 20 seconds;
the timer quantum is 15.625 ms (0.078 percentage points per sample).

Deterministic headless measures simulation CPU for the same 20 virtual seconds.
Real-time headless includes sleep-based pacing; windowed adds canvas rasterization,
upload and SDL/driver presentation. The extra deterministic `raster` diagnostic
configures a Screen surface at the warm endpoint, then runs CPU rasterization
without uploads/draws; it helps separate stroke work from driver cost. Complete
logical receipts are compared between canvases **within the same clock mode**;
headless/windowed startup poll-work totals differ even before this change.
The final controls and paired runs set Windows process affinity to logical CPU 0.
Before/after pairs are interleaved by case. This reduces scheduling variation but
does not eliminate driver/host variability: even the unchanged Classic source
showed a large spread, including a same-executable-path before/after check. The
tables therefore report observations, not a guaranteed speedup or isolated proof
that every before/after CPU delta came from this patch.

### Upload experiments and limits

The coalescer trades transfer bytes for CPU/driver overhead on dense pages.
Sparse writes still upload only their small regions; palette/register-only frames
never upload indices. D3D11's dense fallback uploads a whole physical page; GPU uses
the bounding rectangle. This is not a claim that transfer bandwidth is minimal. Hidden pages remain
resident and are not re-uploaded just because display start changes.

Negative results (GCC 12.2, C17 `-O2 -g -DNDEBUG`, SDL 3.4.16, D3D11; 20-second
1080p sine probes, same seed/script and unchanged state):

| Upload strategy | CPU, one core | Index MB / 20 s | Calls / 20 s |
|---|---:|---:|---:|
| Single exact canvas-wide union | 9.14% | 544.6 | 1200 |
| Same union, STATIC instead of STREAMING | 7.97% | 544.6 | 1200 |
| Round union sizes to 128x64 allocation buckets | 11.25% | 583.5 | 1200 |
| Spatial regions, overlap coalescing only | 12.03% | 453.4 | 11822 |
| Spatial regions plus dense page coalescing | 6.56% | 2411.5 | 1516 |

Size bucketing was removed. STATIC did not establish a D3D11 CPU win; it is used
on Screen to avoid an unused CPU lock bitmap in the
[GPU renderer](https://github.com/libsdl-org/SDL/blob/release-3.4.16/src/render/gpu/SDL_render_gpu.c#L282).
These single probes have host variability,
but smaller byte counts alone did not deliver the requested CPU saving. SDL's
[D3D11 update implementation](https://github.com/libsdl-org/SDL/blob/release-3.4.16/src/render/direct3d11/SDL_render_d3d11.c#L1349)
creates/maps/copies/releases a staging texture for each update, including partial
updates; STATIC uses the same update path. The coalescer's stable dense-page shape
is a response to that overhead, not a change to canvas resolution or raster samples.
The timing residual below includes SDL/driver work; individual DLL costs were not
CPU-profiled, so staging allocation is an implementation-backed explanation and
optimization hypothesis, rather than a measured attribution of every CPU cycle.

One further Native experiment precomputed capsule edge reciprocals. All 41 tests
and all six capture PNGs remained exact, but the pinned raster-only samples changed
from 4.38%/7.66% (1080p/4K) to 4.53%/7.50%. There was no useful net win; it was
removed. Receipts: `reciprocal-test.log`, `caps-reciprocal/`, `reciprocal-raster/`.
The final span arithmetic keeps division and its original rounding order.

### Final CPU results

All entries below are 20-second samples on logical CPU 0. Before/after graphics
receipts at both endpoints are byte-identical (12 complete state comparisons).
The simulation and real-time headless columns are shared Classic controls: headless
does not allocate a Screen raster. Static has no simulation work. The final window
batch uses the shipped defaults: D3D11 for Classic, GPU/D3D12 for Screen.

| Output | Workload | Before window, one core | Final window, one core | Simulation | Real-time headless |
|---|---|---:|---:|---:|---:|
| 1920x1080 | Classic | 3.12% | 3.05% | 0.62% | 1.25% |
| 1920x1080 | Faithful Screen | 3.83% | 5.08% | 0.62% | 1.25% |
| 1920x1080 | Native crisp sine | 14.92% | 8.28% | 0.62% | 1.25% |
| 1920x1080 | Static Screen | 0.70% | 0.16% | 0.00% | 0.23% |
| 3840x2160 | Classic | 5.86% | 4.69% | 0.70% | 1.64% |
| 3840x2160 | Faithful Screen | 5.16% | 3.36% | 0.70% | 1.64% |
| 3840x2160 | Native crisp sine | 13.67% | 12.03% | 0.70% | 1.64% |
| 3840x2160 | Static Screen | 0.47% | 0.39% | 0.00% | 0.62% |

The result is **partial against the performance targets**: Faithful is 1.67x
Classic at 1080p (slightly above the roughly 1.5x target), 0.72x at 4K. Native is
still 2.72x/2.57x Classic, despite its lower measured total than before. Static
performs zero index uploads, creates zero textures and stays below 0.4% in the
final Screen samples. Its residual is predominantly the existing pacing/event
service; separate-process noise can make it smaller than the headless control.

The pacing increment is about 0.63/0.94 percentage points. Native's CPU-only raster
lane is 4.38%/7.66%, including simulation but excluding uploads/draws. After removing
that lane and the pacing increment, approximately 3.3/3.4 points remain in the
windowed Native path (coalescing, SDL transfers, drawing/present and driver work).
These are differences between runs, not an instrumented function-level profile.
Native geometry/trigonometry therefore remains a substantial cost; a future change
should measure sharing the sine adapter's repeated geometry/trig work or GPU
strokes while preserving the CPU sampling/fade contract. The SDL uploader still
allocates temporary transfer resources; reusable transfer buffers are another
possible next step if the `SDL_UpdateTexture` restriction is revisited.

Final Screen traffic over 20 seconds: Faithful 544.6 MB / 2179.3 MB; Native
737.3 MB / 2940.3 MB (1080p / 4K). Faithful uses 1200 update calls at both sizes;
Native 1201. No aperture textures are recreated during the samples. Whole-page transfer
and many-small-region trials are retained above rather than presented as wins.
Classic's unchanged control still ranged widely in exploratory runs, even with
affinity and a same-path binary swap (3.12-8.13% at 1080p). Treat the table as this
host's observations, with no claim that the measured Classic variation is a code
speedup. Final receipts: `final-report.txt`, `final-summary.json`, `final-window/`,
`paired/`, `controls/` and `samepath.json`.

Unverified: live DPI/monitor migration, device loss, allocation exhaustion and an
actual screensaver/preview host. `native/scr/`, window/input code, application
algorithms, DOS matching files, assets and pinned hash files are unchanged.

## First Native effects batch

Implemented 2026-11-04, in order: **cycloids, sweep2, ellipse, arches, daisies,
squirals, corkscrew**. These join sine in `available` and default to Native on
Screen. Each remains independently selectable through `--native`; `none` keeps
the complete Faithful canvas oracle. Saver settings/UI are separate work.
The per-effect intentions and width decisions are recorded in
[patterns.md](../native/notes/patterns.md#native-effects-first-batch).

### Geometry, schedule and layout

The original generators and iterators still execute every logical store with
their original charges and diagnostics. A single pending plot callback replaces
only its physical stamps. The DVDT hook consumes that callback after the logical
kernel; fades/mode teardown discard an unplotted pending outer point, and the
callback's image owner prevents a direct selection from consuming old ink. The small
DZLE hooks supply original continuous parameters or mark the beginning/next step
of a ray. No new random stream, clock, per-plot allocation, scene list or retained
geometry is introduced. Ellipse and arches share `effects/ellipses.c`; each other
effect has its own small module. `geometry.c` only maps immediate coordinates,
symmetry, stroke weight and normalized original line progress.

Cycloids evaluates orbit and rolling-pen angles throughout each original table
interval. Arches evaluates real ellipse angles, parameterized by continuous
major-axis progress through the original quadrant schedule. Corkscrew evaluates
interpolated float distance/phase before integer truncation. These curves use
output-dependent subdivision with a 1/8-pixel chord-error bound. Zero-length
logical intervals still emit their nonzero continuous arc/edge when necessary.
Ellipse's boundary rays and daisies' angular petal rays are true straight spans;
squirals computes its edge vertices from float radius/scale/angle. None joins
enlarged Bresenham pixels or rounded trig-table vertices.

Every logical colour step owns its next short span/arc; original manual copies,
global reflection/swap order, restarts and overlaps remain ordered. Ray completion
does not repaint an entire ray. All strokes use `dz_canvas_stroke`, whose existing
subpixel capsule scan converter writes horizontal index spans and unions dirty
rectangles. Long strokes already have direct scan conversion; no per-pixel
distance loop is used in crisp interiors. Width is sine's original-pixel-area
diameter, at least one output pixel. Palette lookup and uploads retain the VGA
model described above.

There is one composition, with the original number of symmetry branches. At 4:3
every mode retains its Original physical unit proportions. Cycloids retains its
circular rolling metric and can use previously clipped side space. Ellipse,
daisies and squirals spread centre placement while retaining local shape scale;
this adds no instances. Sweep2 widens endpoint range, arches widens its
horizontal axes/centres, and corkscrew widens its along-axis displacement/origin
while retaining the transverse excursion. Original height is preserved. These
are effect decisions, independent of Faithful's Original/Crop/Stretch viewport.

Two whole-instance fallbacks are deliberate. Accelerating sweep2 eventually
wraps its signed doubled Bresenham deltas: that variant remains Faithful; the
cyclic variant is Native. Arches rejects initial quadrants whose conservative
`ceil(hypot(rx,ry))+2` bound reaches the 700-word cache. This admits doubled
EGA350 outlines but leaves the doubled large-EGA700 instance Faithful, preserving
its completion-word spill/prefix. Degenerate nonpositive arches outlines use
their original stores. No ideal complete curve replaces a truncated logical
trace. Other original word arithmetic and caches are unchanged.

### Verification and visual review

`python -B -I tools/native.py test`: **42/42 passed**. Classic integration and
acceptance hashes and all six DOS-reference suites pass without re-recording.
The extended `test_native_fades` completes all 24 fades with each effect's real
ink, including the actual Pan boundary/hidden page, and compares complete logical
video state, time and RNG. Crisp is checked for all eight effects; the existing
AA sine control remains. Margin/whole-page erasure and allocation canaries pass.

`test_native_effects` independently compares complete sweep2 and squirals ink to
Euclidean capsule distance, checks analytic rays/petals and dense real-curve
samples, and checks circular cycloids in all four DOS modes. Sizes are 641x479,
1440x1080, 1920x1080 and 3840x2160. It exercises each actual effect on both pages,
rounded pan/split and resize restart, and checks both explicit fallback variants.
The existing sine tests retain their six-size/four-policy/two-style coverage.

`verify_native.py` compares full receipts plus 440 POD globals, normalized image
selection and palette allocation bytes. Eight effects, four modes, three sizes,
three styles and eight deadlines give **2304 observations / 1536 Native-versus-
Faithful comparisons**, all equal, including Quilt/tetrYs and a later Faithful
overlay. A second full sweep2 run with seed 3 exercises its Native cyclic branch:
another **288 observations / 192 comparisons**, all equal. Seed 1 exercises its
Faithful accelerating fallback. `verify_canvas.py` with `--native none` still
passes **960 full state/pixel observations** across modes, policies and sizes.

Raw captures, labelled full-size adjacent pairs and previews are under
`build/workers/s-fx1/captures-final/`, with Native sweep2 in `captures-sweep/`.
Each pair uses the same seed and virtual **9.5 s**, at **1920x1080 and 3840x2160**;
Faithful/Original is left and Native crisp right. Seed is 1 except sweep2's
Native review pair (3). Seed-1 sweep2 captures also document its identical
Faithful fallback. Both sizes were inspected for every effect, including an
unscaled 4K sweep detail: smooth rolling lobes, rounded arches, clean ellipse and
petal fans, straight rotating square edges and continuous corkscrew coils read
as the same effects with clearer geometry. Sweep's dense indexed bands retain
some crisp moire where rays nearly coincide; its individual rays are straight.
No tiling, extra figures or thick logical-pixel staircases were introduced.

Receipts: `test-final.log`, `id-final/*-identity.json`, `sweep-id/`,
`oracle-final/equivalence.json`, `captures-final/` and `captures-sweep/`, all under
`build/workers/s-fx1/`. Reproduction:

```powershell
python -B -I tools/native.py test
python -B -I native/tests/verify_native.py build/native/native_oracle.exe build/workers/s-fx1/id-final
python -B -I native/tests/verify_native.py build/native/native_oracle.exe build/workers/s-fx1/sweep-id --effect sweep2 --seed 3
python -B -I native/tests/verify_native.py build/native/native_oracle.exe build/workers/s-fx1/captures-final --captures
python -B -I native/tests/verify_native.py build/native/native_oracle.exe build/workers/s-fx1/captures-sweep --captures --effect sweep2 --seed 3
python -B -I native/tests/verify_canvas.py build/native/canvas_oracle.exe build/workers/s-fx1/oracle-final
```

### CPU: isolated raster work

Windows process kernel+user CPU seconds for **20 virtual seconds**, from exact
8.5/28.5 observations in the existing `canvas_bench` raster seam. Original
application/clock/input/work are unchanged, a software canvas is configured once
at warmup, and presentation/upload is excluded. This isolates geometry/stores
from desktop/driver timing. GCC 12.2 RelWithDebInfo, crisp, Original policy, seed
1 (sweep2: 3). Classic, Faithful and Native logical receipts agree at both
deadlines for every case. Timings have Windows' roughly 1/64-second granularity;
small Faithful denominators make ratios noisy.

| Effect | 1080p Faithful CPU s | 1080p Native CPU s | 4K Faithful CPU s | 4K Native CPU s |
|---|---:|---:|---:|---:|
| cycloids | 0.047 | 0.312 | 0.047 | 0.438 |
| sweep2 (cyclic) | 0.094 | 0.547 | 0.125 | 0.906 |
| ellipse | 0.078 | 0.516 | 0.109 | 0.828 |
| arches | 0.062 | 0.641 | 0.062 | 0.984 |
| daisies | 0.078 | 0.531 | 0.094 | 0.969 |
| squirals | 0.094 | 0.578 | 0.109 | 0.938 |
| corkscrew | 0.109 | 0.641 | 0.109 | 0.953 |
| sine control | 0.188 | 0.953 | 0.203 | 1.297 |

Every new effect is below Native sine's absolute raster CPU cost, hence none
exceeds the requested 2x-sine threshold. Fan and manual-copy effects execute
several ordered symmetry branches per original logical step, but each growing
ray only rasterizes its next short span. CPU increases relative to Faithful's
small block stores are visible here; they are not whole-program/driver ratios.
There is no per-plot allocation. Receipts: `raster-summary.json`,
`raster-final.log`, `raster/<effect>/`, with full commands in each JSON record.

Arches' benchmark reselects **ALT+1, F10 once per virtual second** in both
styles. Indefinite F10 otherwise drives original signed radii negative and
eventually aborts at direction-cache index 702 even in Classic; no Native fix
was applied to that logical behavior. The rejected initial raster fixture also
showed Windows video bootstrap waiting without a dummy driver; the raster seam
now explicitly uses `SDL_VIDEODRIVER=dummy`. These are recorded negative trials,
not included in the table. To reproduce one raster pair (repeat per effect):

```powershell
python -B -I native/tests/measure_vram.py build/native/canvas_bench.exe build/workers/s-fx1/raster --effect cycloids --workload classic --timing deterministic
python -B -I native/tests/measure_vram.py build/native/canvas_bench.exe build/workers/s-fx1/raster --effect cycloids --workload faithful --timing raster --raster
python -B -I native/tests/measure_vram.py build/native/canvas_bench.exe build/workers/s-fx1/raster --effect cycloids --workload native --timing raster --raster
```

This batch does not exercise live DPI/monitor migration, device loss, allocation
exhaustion or a screensaver/preview host. `native/scr/`, window/input/presenter
sources, DOS matching sources/evidence/assets and pinned hashes were not edited.

### CPU: production window samples

Twenty **real seconds** per case after virtual 8.5 s, using the production
`dazzle.exe` window/presenter, crisp, Screen/Original, seed 1 (cyclic sweep2: 3).
Final Native runs use Direct3D11; the Faithful controls used AUTO/Direct3D11 on
this Windows host. `canvas_bench` probes independently confirm actual
1920x1080 and 3840x2160 Direct3D11 outputs, and `INK native=1 parameter=0`
confirms production sweep2 takes its Native branch. Native was remeasured after
the final pending-owner and exchanged-axis layout fixes. The existing Faithful
controls retain the same logical/raster behavior. CPU is process kernel+user
from GetProcessTimes; samples last about 20.02-20.05 wall seconds. Startup is
excluded. Every warm/end state receipt agrees byte-for-byte between Faithful
and final Native, including the complete logical aperture, palette, RNG, work
and exact virtual time.

| Effect | 1080p Faithful, one core | 1080p Native, one core | 4K Faithful, one core | 4K Native, one core |
|---|---:|---:|---:|---:|
| cycloids | 4.29% | 6.40% | 7.72% | 9.28% |
| sweep2 (cyclic) | 7.65% | 9.91% | 14.90% | 11.39% |
| ellipse | 7.88% | 15.91% | 15.60% | 14.98% |
| arches | 11.70% | 12.09% | 5.70% | 12.64% |
| daisies | 11.63% | 18.42% | 3.90% | 23.10% |
| squirals | 11.86% | 10.77% | 6.63% | 23.01% |
| corkscrew | 9.21% | 10.37% | 10.92% | 17.47% |
| sine control | 11.62% | 17.79% | 12.64% | 18.70% |

The largest Native/sine ratio is about **1.24x** (4K daisies); no effect exceeds
2x sine in either the raster or production samples. These are single-host
observations, with substantial desktop/driver variation: Native sine's 4K
repeat changed from 13.34% to 18.70%, and ellipse's 1080p repeat from 6.47% to
15.91%. Do not attribute those differences to the small dispatch/layout fixes.
The much steadier isolated raster table is the evidence for geometry cost;
window totals include dirty transfers, presentation and driver work. Dense fans
and spiral copies change more widely separated canvas regions than sine.

One exploratory ellipse 4K pair had unequal end-state receipts and a delayed
completion; it was rejected, then repeated successfully before the final Native
batch. That discarded pair is in `measure/`; valid controls and final Native
samples are separated in `window-controls.json`, `measure-final/`,
`measure-current/` and `window-summary.json`. All are under
`build/workers/s-fx1/`; full commands, exact CPU/wall seconds, peak working set
and sampled private memory remain in the JSON files. The final Native samples
have `logical_state_equal: true`. The actual-output/Native-branch probes are
`sweep-production.log` and `sweep-production-4k.log`.

```powershell
$env:SDL_RENDER_DRIVER='direct3d11'
python -B -I native/tests/measure_native.py build/native/dazzle.exe build/workers/s-fx1/measure --effect cycloids
python -B -I native/tests/measure_native.py build/native/dazzle.exe build/workers/s-fx1/measure --effect sweep2 --seed 3
```

The [capture review page](../build/workers/s-fx1/review.html) links both unscaled
pairs for every effect. Work is uncommitted. All launched applications and helper
processes are stopped after verification.
