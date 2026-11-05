# Native canvas: resolution-independent drawing (agreed design, 2026-11-03)

Decisions by the project owner (supersede the Hybrid overlay in docs/native-hybrid.md; Extended follows the unclipped-original follow-up below):

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
| Aspect | Presentation maps both styles through the same viewport; Extended exposes original horizontal overflow at unchanged scale and centre. |
| Old code | Hybrid overlay and independent Extended side adapters are removed. Their trials remain historical evidence. |

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

Phase 3's uniform surface, Native sine pilot and two subsequent effect batches
are implemented below (17 Native effects).
The VGA-model presentation section at the end supersedes the earlier gather/upload
implementation and Faithful pan oracle; ordinary unpanned Faithful remains exact.
The phase-2 description and measurements in this section are historical; its
fixed-height padded storage has been replaced. The later presentation/comparison
follow-ups supersede phase-3 spread/path choices.

Phases 1 and 2 implemented, 2026-11-03. Classic remains the developer default.
Select the second backend with `--canvas screen`; `--canvas classic` selects the
reference backend. The Windows screensaver selects its canvas from the saved
per-effect styles and enabled image sizes: Screen when any enabled image has a
Native implementation selected or Extended is active, Classic otherwise. Extended
retains Native/Faithful choices and enables their controls. The settings
dialog shows per-effect style choices and defaults every record to Native.

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
Its design is marked historical in native-hybrid.md. The independent Extended side
adapters are also retired; Extended now uses the unclipped Screen rule below.
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

Historical implementation evidence: spread/gain/anchoring and real-angle path
choices below are superseded by the presentation and comparison follow-ups.
Earlier measurements and rejected trials remain as recorded; source is in git
history. No historical performance number is a measurement of the current paths.

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

Historical implementation evidence: spread/gain/anchoring and real-angle path
choices below are superseded by the presentation and comparison follow-ups.
Earlier measurements and rejected trials remain as recorded; source is in git
history. No historical performance number is a measurement of the current paths.

Implemented 2026-11-04, in order: **cycloids, sweep2, ellipse, arches, daisies,
squirals, corkscrew**. These join sine in `available` and default to Native on
Screen. Each remains independently selectable through `--native`; `none` keeps
the complete Faithful canvas oracle. 
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
its completion-word spill/prefix. The physical-store follow-up below extends this
to thin-ellipse cache failures and predicts the complete bounded Auto instance;
unbounded Semi-Auto/F10 arches stays Faithful. Degenerate Native outlines use
Native points in the same spread layout. No ideal complete curve replaces a
truncated logical trace. Other original word arithmetic and caches are unchanged.

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
| arches (before store audit) | 0.062 | 0.641 | 0.062 | 0.984 |
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
| arches (before store audit) | 11.70% | 12.09% | 5.70% | 12.64% |
| daisies | 11.63% | 18.42% | 3.90% | 23.10% |
| squirals | 11.86% | 10.77% | 6.63% | 23.01% |
| corkscrew | 9.21% | 10.37% | 10.92% | 17.47% |
| sine control | 11.62% | 17.79% | 12.64% | 18.70% |

These first-batch tables precede the physical-store audit. Their arches samples
mixed layouts and are superseded by the declared whole-instance policy and fresh
raster table below; they are retained as historical measurements.

The largest Native/sine ratio in these samples is about **1.24x** (4K daisies); no effect exceeds
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


## Second Native effects batch

Historical implementation evidence: spread/gain/anchoring and real-angle path
choices below are superseded by the presentation and comparison follow-ups.
Earlier measurements and rejected trials remain as recorded; source is in git
history. No historical performance number is a measurement of the current paths.

Implemented 2026-11-04, in order: **boxes, puffs, bars, wandering, bigstar,
littleStars, cones, yarnball, gumballs**. These join the first eight in `available`;
settings discover them without dialog changes. Lattice and filled-polygon effects
remain later batches. Intentions and width choices are in
[patterns.md](../native/notes/patterns.md#native-effects-second-batch).

### Geometry, schedule and layout

`effects/segments.c` handles the ray/segment families and littleStars' manual
copies. Each logical plot draws only its next short continuous interval through
`dz_canvas_stroke`, at the existing original-pixel-area weight. It never joins
rounded iterator pixels or separate rays, or repaints a completed ray. Boxes,
puffs and bars reset the pen **before** the outer plot: the unplotted endpoint
interval stays omitted and the reset origin receives its own current-colour point.
Wandering includes its endpoint and breaks at every new origin.

Bigstar uses the actual cache-selected endpoint, including stale entries surviving
a shorter seed enumeration. It never joins the seed cache or substitutes an ideal
complete star. Its group/point/EGA colour modes remain at their original call sites.
LittleStars keeps `++,+-,-+,--` order then four swapped copies, including per-copy
colour increments on clipped copies. A skipped copy breaks only its own stroke.

Cones, yarnball and gumball outlines extend the existing `ellipses.c` renderer.
Real angles follow continuous major-axis progress, with output-dependent <=1/8-pixel
chord error. Original caches still control logical work, colour and copy schedules,
rather than geometric density. The four imminent manual copies share their
arc endpoints and subdivision count; no arc list is retained. Zero-radius rings
are Native points at their spread centres, with all logical stores physically
suppressed. Unsafe cache traces require an explicit whole-instance Faithful
fallback; no ideal complete ring replaces a truncated trace. Actual EGA XOR stores
stay Faithful, even with a pending callback; VGA still ignores EGA's raster flag.

`effects/gumballs.c` also implements all four fill modes as opaque output-row spans
bounded by the real ellipse. Every original row retains its colour-band count;
each logical sample owns the corresponding interval of the analytic span. Solid,
horizontal, mirrored-horizontal and per-row colours remain palette indices, with
the same reveal order and forced second-store reflection. Transposed symmetry
solves band inequalities into at most two spans per output row. The small
`dz_canvas_native_span` writes the existing selected-page aperture and dirty bounds.
There are no capsules or per-pixel distance tests for filled interiors. Fills are
opaque in the frozen AA experiment too; no AA work was added.

Segment fans spread their origins and horizontal excursions together, preserving
their negative spaces in proportion. Cones spreads stack centres, retaining local
ellipse proportions. Gumballs spreads its centre field while circles stay circular.
Yarnball anchors each mirrored cluster's inner edge and spends added half-width on
neighbour-centre spacing; its sparse central layout spreads neighbour offsets.
At 4:3 the gain is one. No tiling, extra instances, randomness, retained geometry
or allocation per plot is introduced. Logical state/charges always run unchanged.

Rejected trials (GCC 12.2, C17 RelWithDebInfo `-O2 -g`, SDL 3.4.16): multiplying all
yarnball centres by width gain opened a new central band; the anchored layout
replaces it. Initially keeping gumball fills Faithful moved only its Native
outlines; analytic spans now share their centre layout. An initial test expected
17 on EGA rather than its correct masked index 1; only the test was corrected.
Windows rejected a relink while the oracle executable was running; rebuilding
after completion succeeded.
A same-owner unplotted outer point could enter a subsequent degenerate ring;
ring entry now clears that pending point, with a regression test. Ring timing
cases were repeated after this lifecycle fix and endpoint sharing; superseded
window cases are in `window-before-restart/`. An unshared-arc raster trial reached
2.703 CPU s at 4K, above the earlier sine control's 2x threshold; its repeated
endpoint/count calculations were removed. Final table values use the shared
version; host variation prevents attributing the entire timing delta to that
change. All 12 ring capture PNGs remain byte-identical. Rejected trials are not
included in the final tables.

### Verification and capture review

Initial `python -B -I tools/native.py test`: **42/42 passed**, including unchanged Classic
pinned integration/acceptance hashes and all six DOS-reference suites.
`test_native_effects` checks real reset-prefix/endpoint geometry, independent
pixel-distance/colour oracles for all bigstar modes and littleStars' ordered copies
and clipping, analytic ring continuity/aspect in all four DOS modes, and all four
gumball fill modes with/without swapped symmetry. Fills cover the exact analytic
ellipse union without holes; solid colours are exact. Sizes: 641x479, 1440x1080,
1920x1080, 3840x2160. Every effect exercises production drawing, both pages,
rounded pan/split and resize/restart. Yarnball bounds also pin its inner edge
and expenditure of added half-width. All **24 fades** run with each of the 17
Native effects' ink, comparing full logical state/time/RNG, hidden pages and erasure.

`verify_native.py` compares full receipts plus 440 POD globals, normalized selection
records and palette allocation bytes. Nine effects, four modes, three sizes, three
styles and eight deadlines passed **2592 observations / 1728 Native-versus-Faithful
comparisons**, including Quilt/tetrYs and a later Faithful overlay.
`verify_canvas.py` with `--native none` passed **960 state/pixel comparisons**.
A second full yarnball run with seed 3 verifies its sparse branch: another
288 observations / 192 comparisons, all equal. The original Ctrl program indices
include the duplicate Alt record at 36:
gumballs/yarnball receipts use 39/42; Native selection bits remain 38/41.

Raw captures, labelled unscaled adjacent pairs and previews at **1920x1080 and
3840x2160**, seed 1, virtual **9.5 s**, are in `build/workers/s-fx2/captures/`.
[Open the review page](../build/workers/s-fx2/captures/review.html).
Faithful/Original is left, Native crisp right. Both sizes were inspected: straight
fan/band/star segments and continuous cone/ring contours read as the same effects
with clearer geometry. Dense intersections retain intentional indexed moire.
Yarnball's corrected clusters use the width without a new inter-cluster band.
Mixed gumball accumulations show outlines and shaded fills. Additional paused
single-ball pairs: `ball-single/` (seed 1, outline) and `ball-seed4/` (seed 4,
horizontal shaded fill, flags 6); `yarn-seed3/` shows the sparse layout. Seed/time
are identical within each pair.

Receipts under `build/workers/s-fx2/`: `test-final.log`, `identity/`,
`canvas-final/equivalence.json`, `captures/`, `raster-summary.json`, `raster/`,
`window-probes.log`, `window/`. Reproduce:

```powershell
python -B -I tools/native.py test
python -B -I native/tests/verify_native.py build/native/native_oracle.exe build/workers/s-fx2/identity --effect boxes,puffs,bars,wandering,bigstar,littleStars,cones,yarnball,gumballs
python -B -I native/tests/verify_canvas.py build/native/canvas_oracle.exe build/workers/s-fx2/canvas-final
python -B -I native/tests/verify_native.py build/native/native_oracle.exe build/workers/s-fx2/captures --captures --effect boxes,puffs,bars,wandering,bigstar,littleStars,cones,yarnball,gumballs
python -B -I native/tests/verify_native.py build/native/native_oracle.exe build/workers/s-fx2/ball-seed4 --captures --single --effect gumballs --seed 4
```

### CPU: isolated raster work

Same first-batch method: `canvas_bench`, Windows process kernel+user CPU seconds
between exact virtual 8.5/28.5 observations, dummy software canvas configured once
at warmup, no presentation/uploads. Seed 1, VGA400, Original, crisp, GCC 12.2
RelWithDebInfo, SDL 3.4.16. Classic/Faithful/Native logical receipts agree at both
endpoints. Runs were sequential. These include simulation plus raster work;
Windows' 15.625 ms quantum limits precision.

| Effect | 1080p Faithful CPU s | 1080p Native CPU s | 4K Faithful CPU s | 4K Native CPU s |
|---|---:|---:|---:|---:|
| boxes | 0.109 | 0.984 | 0.141 | 1.609 |
| puffs | 0.141 | 1.031 | 0.188 | 1.656 |
| bars | 0.141 | 1.016 | 0.188 | 1.641 |
| wandering | 0.141 | 0.984 | 0.188 | 1.547 |
| bigstar (before store audit) | 0.078 | 0.500 | 0.094 | 0.844 |
| littleStars | 0.078 | 0.469 | 0.094 | 0.750 |
| cones (before store audit) | 0.141 | 1.328 | 0.172 | 2.453 |
| yarnball (before store audit) | 0.078 | 0.453 | 0.094 | 0.719 |
| gumballs | 0.078 | 0.375 | 0.109 | 0.609 |
| sine control | 0.172 | 0.953 | 0.203 | 1.250 |

These are pre-audit samples; the changed bigstar/cones/yarnball paths and a fresh
sine control are remeasured in the follow-up table below. In these earlier samples,
all new effects are below **2x Native sine**. Cones is highest: 1.39x at 1080p,
1.96x at 4K, with four ordered quadrant callbacks plus global symmetry per ring
step. Each draws only its next short arc. Rings avoid per-pixel distance loops;
gumball fills use spans and rays draw their next short interval.

```powershell
# Repeat per effect. Classic control is required for receipt comparisons.
python -B -I native/tests/measure_vram.py build/native/canvas_bench.exe build/workers/s-fx2/raster --effect boxes --workload classic --timing deterministic
python -B -I native/tests/measure_vram.py build/native/canvas_bench.exe build/workers/s-fx2/raster --effect boxes --workload faithful --timing raster --raster
python -B -I native/tests/measure_vram.py build/native/canvas_bench.exe build/workers/s-fx2/raster --effect boxes --workload native --timing raster --raster
```

### CPU: repeated production windows

Two independent repetitions of every effect/size/style use the production
`dazzle.exe`, Screen/Original, crisp, VGA400, seed 1 and **Direct3D11** for
comparability with the first batch. Each sample starts after virtual 8.5 s and
measures 20 real seconds using Windows process kernel+user CPU; startup is excluded.
Only one DAZZLE test/benchmark runs at a time, with no concurrent captures,
compilation or oracle runs. The exact 8.5/28.5 state receipts are retained.
`window-probes.log` confirms actual 1920x1080 and 3840x2160 Direct3D11 outputs
and Native ink through the production CLI/presenter. The default Screen GPU
backend was not separately remeasured here.

All **80 accepted samples** (ten effects including sine, two sizes, two styles,
two repetitions) completed. All **80 warm/end full-state comparisons** agree
between Native and Faithful. Ranges below are the two observations, in percent of
one core, rather than a confidence interval. CPU seconds, wall seconds, commands
and peak memory are retained in `window-summary.json` and per-case receipts.

| Effect | 1080p Faithful, one core | 1080p Native, one core | 4K Faithful, one core | 4K Native, one core |
|---|---:|---:|---:|---:|
| boxes | 7.18-11.85% | 17.64-20.35% | 16.15-17.77% | 19.51-26.42% |
| puffs | 7.18-8.81% | 11.71-19.66% | 10.30-19.64% | 18.89-32.98% |
| bars | 9.83-14.73% | 17.56-22.31% | 14.28-18.01% | 25.60-29.41% |
| wandering | 12.95-15.52% | 14.97-22.46% | 18.41-18.57% | 19.51-24.11% |
| bigstar (before store audit) | 9.36-10.07% | 12.33-15.38% | 18.81-20.11% | 17.79-24.78% |
| littleStars | 5.86-11.93% | 15.14-15.60% | 11.55-16.53% | 15.91-22.53% |
| cones (before store audit) | 11.07-11.93% | 16.93-20.12% | 13.49-15.68% | 27.15-35.33% |
| yarnball (before store audit) | 7.26-12.87% | 12.48-13.02% | 12.25-18.64% | 13.96-17.08% |
| gumballs | 5.39-10.69% | 7.88-8.03% | 11.31-14.59% | 21.68-23.17% |
| sine control | 9.28-13.18% | 10.76-16.61% | 13.96-15.37% | 17.55-23.40% |

The affected rows are historical, before the physical-store fixes. Production
window samples were not repeated for this follow-up; the new isolated raster
results below measure the corrected paths.

Using the most conservative cross-repeat comparison (each effect's maximum
against sine's minimum), three ranges cross 2x: **1080p bars 2.07x, 1080p wandering
2.09x, 4K cones 2.01x**. Within the same repetition their maximum ratios are
1.63x, 1.39x and 1.55x respectively. The corresponding isolated raster ratios
are 1.07x, 1.03x and 1.96x. Whole-window totals include changed-region bookkeeping,
transfers, presentation and driver work as well as the raster lane; both Faithful
and Native repeat ranges show substantial host/driver variation. Wide segment
fields touch more separated regions than sine. Cones retains eight global copies
where sine uses four, plus output-dependent subdivision for its smallest rings
and four ordered quadrant calls. Each still draws only its next short arc.
These measurements explain the threshold crossings without treating window totals
as a function-level raster profile; individual driver costs remain unprofiled.

The ring lifecycle fix and shared endpoint work caused the affected ring cases to
be recollected; `window-before-restart/` keeps the superseded observations. The
unchanged segment and sine paths retain their valid samples. No other DAZZLE,
compilation, capture or oracle workload ran during a timed window sample.

```powershell
$env:SDL_RENDER_DRIVER='direct3d11'
# Run each once into repeat1 and repeat2, sequentially.
python -B -I native/tests/measure_native.py build/native/dazzle.exe build/workers/s-fx2/window/repeat1 --effect boxes,puffs,bars,wandering,bigstar,littleStars,cones,yarnball,gumballs,sine
python -B -I native/tests/measure_native.py build/native/dazzle.exe build/workers/s-fx2/window/repeat2 --effect boxes,puffs,bars,wandering,bigstar,littleStars,cones,yarnball,gumballs,sine
Remove-Item Env:SDL_RENDER_DRIVER
```


Unverified: live DPI/monitor migration, device loss, allocation exhaustion and an
actual screensaver/preview host. No screensaver/window/input/presenter source,
DOS matching source/evidence/assets or pinned hash file was edited for this batch.

### Physical-store follow-up

The reported cones specks were the zero-radius first ring of each stack. Ring
entry left Native inactive, so its logical quartet and outer plot reached Screen
at unspread coordinates. A degenerate ring now queues four ordered Native points
and its Native outer point at the spread centre; every logical store still runs,
with its physical stamp suppressed. The reported centre crop is clean at both
1080p and 4K. Nonpositive radii do not authorize mixing Faithful and Native ink.

The audit found four effects leaking Faithful stores and one reverse leak:

| Effect | Cause and correction |
|---|---|
| cones | Zero-radius rings bypassed Native. They now use Native points in the same layout as the stack. |
| arches | A thin positive ellipse can reach zero Y in the cache's first region, where the original misses completion and spills past 700 words. The former radius bound missed this, allowing one family to be Faithful and the other Native. A pure midpoint-count prediction now checks every quarter reachable within the Auto work budget before any ink. An unsafe instance is wholly Faithful. |
| bigstar | The terminal completion branch left the outer plot at the old pen with no adapter. It now queues the same endpoint in the spread Native layout. |
| yarnball | Initialization returns before drawing a ring, but its outer plot stored the unrelated previous pen. It now consumes a Native no-ink callback; the logical store and charge remain. |
| sweep2 | An unplotted cyclic callback survived a same-owner rebuild into the accelerating Faithful variant. Prepare now discards pending ink before selecting either path. This was Native ink leaking into a declared Faithful instance. |

The other twelve effects had no physical-store leaks. Actual EGA XOR and
accelerating sweep2 remain explicit whole-instance fallbacks. Arches additionally
reports cache spill or unbounded execution: **all Semi-Auto/F10 arches instances
stay Faithful**. There is no finite work limit with which to rule out a later
malformed cache. Supporting that trace in Native needs an owner decision;
switching layouts at its first failure is not acceptable. Cache-safe bounded Auto
instances remain Native. Prediction does not change original globals, random
draws, iterator execution or time charges. Auto's initializer, first continuation
and post-incremented word work comparison are included in the forecast.

`dz_canvas_store_audit` is an optional pointer to two counters. The Screen stamp
boundary counts actual unsuppressed Faithful physical stores after clipping and
unchanged-store filtering; the existing suppression boundary counts consumed
Native plot groups, including intentional no-ink callbacks. Normal execution
leaves the pointer null. There is no per-Native-pixel counter or allocation.
`dz_native_instance` exposes explicit fallback reasons independently of pending
callbacks, so a missing adapter cannot silently classify itself as Faithful.

`test_native_stores` runs all 17 complete generators **including their outer
plots**, first frames, unplotted rebuilds, restarts and long runs in both VGA400
and VGA200. CTest uses 960x540, seeds 1..32 and 2 virtual seconds; `--long` uses
1920x1080, seeds 1..64 and four 20-virtual-second runs per effect. Variant masks
assert all cone directions/corners, ellipse fan/colour modes, gumball fills and
outlines, yarnball layouts/edge modes, bigstar/littleStars colours, squiral copies
and the remaining binary selections. Bounded Auto arches is exercised separately
through repeated work-budget restarts. Explicit accelerating, unbounded, cache
spill and EGA XOR fallbacks must consume zero Native callbacks. A deliberately
removed sine adapter proves the physical counter detects missing suppression.
The sweep2 stale-callback regression fails against the previous sweep2 source.

Final verification: **44/44 CTests passed**. The extended audit completed
**83,739,119 generator calls, zero Faithful stores in Native instances, zero
Native callbacks in Faithful instances**. The first extended fixture incorrectly
rebuilt arches without resetting the controller's work count; resetting that
test-only count to match Auto corrected the fixture, without a product change.
A final combined fade run exceeded CTest's 30-second limit (the earlier run took
20.95 seconds). The two effect batches now run in separate test executables,
preserving all 24 fades per effect and the Pan checks without raising the timeout.
All 17 effects passed identity: **4896 observations / 3264 comparisons**; extra
seed-3 sweep2 and yarnball runs add **576 / 384**, for **5472 / 3648** total.
The `--native none` canvas oracle again passed **960 state/pixel comparisons**;
Classic pinned integration/acceptance files remain unchanged.

Fresh Faithful/Native captures for all five affected effects at both sizes are
in `build/workers/s-fx2/leaks/captures/`; the affected second-batch main capture
files are refreshed too. The
[follow-up review page](../build/workers/s-fx2/leaks/review.html) includes the exact
reported cones crop before/after and enlarged outer-store checkpoints. The latter
use a labelled unique indexed test palette, the same seed/time in both builds,
and stop at bigstar completion or yarnball initialization: later ink can hide
these stores in a normal 9.5-second capture. Both resolutions and the areas around
the figures were inspected. The cones specks are gone; no detached replacement
ink appears. Arches' Semi-Auto pairs and sweep2's accelerating seed-1 pairs are
identical by declared fallback.

Fresh isolated raster CPU seconds per 20 virtual seconds follow the same
8.5/28.5-second method, seed, compiler and software-canvas setup above. Runs were
sequential with no concurrent DAZZLE, build, capture or oracle process. Logical
endpoint receipts agree for all Classic/Faithful/selected-Native cases.

| Effect | 1080p Faithful CPU s | 1080p selected Native CPU s | 4K Faithful CPU s | 4K selected Native CPU s |
|---|---:|---:|---:|---:|
| cones | 0.266 | 2.344 | 0.438 | 4.406 |
| arches (Semi-Auto, wholly Faithful) | 0.094 | 0.094 | 0.125 | 0.125 |
| bigstar | 0.172 | 0.984 | 0.203 | 1.656 |
| yarnball | 0.109 | 1.031 | 0.188 | 1.563 |
| sine control | 0.266 | 1.547 | 0.328 | 2.188 |

Arches reports `native_ink: [0,0]`; this row measures its declared fallback,
not Native ellipse cost. Cones is **1.52x sine at 1080p, 2.01x at 4K**. It retains
eight global copies versus sine's four, four ordered quadrant callbacks, and
output-density subdivision of small rings; zero-radius points now also use the
Native path. Each callback still draws only its next interval. The fresh sine
control is slower too, so these observations do not isolate the cost of the
small fixes from host variation. Other remeasured effects are below sine.
The marked older raster/window rows are pre-audit observations. Window samples
were not rerun for this follow-up; they are not claims about the corrected ink.

Evidence under `build/workers/s-fx2/leaks/`: `test-final.log`,
`audit-long-fixed.log`, `id/`, `id-seed3/`, `canvas/`, `captures/`,
`raster-summary.json`, and `sweep-before-proof.log`. Reproduce:

```powershell
python -B -I tools/native.py test
build/native/test_native_stores.exe --long
python -B -I native/tests/verify_native.py build/native/native_oracle.exe build/workers/s-fx2/leaks/id
python -B -I native/tests/verify_native.py build/native/native_oracle.exe build/workers/s-fx2/leaks/id-seed3 --effect sweep2,yarnball --seed 3
python -B -I native/tests/verify_canvas.py build/native/canvas_oracle.exe build/workers/s-fx2/leaks/canvas
```

## Presentation follow-up: Extended is the unclipped original

On 2026-11-05 the owner replaced the earlier "one figure, spread wider" decision.
All output transforms come from `dz_present_viewport`, via the canvas mapping.
Original fits the DOS 4:3 composition with black sides. Stretch transforms that
same picture and pen on both axes (circles become ovals). Crop uses Faithful's
uniform cover scale and overflow. Extended retains Original's scale and centre
and removes only horizontal clipping. There is no gain or per-effect spreading.

For Faithful, `plot_pass` observes genuine horizontal rejects before its unchanged
logical gates and stamps their real reflected quadrants in physical side columns.
Vertical gates, EGA parity, pages and physical address bounds still apply. Wrapped
word values never wrap to the other side. No logical store, read, RNG, charge or
`symmetry_edge` changes. Native continues through the same space. Every byte in
the 4:3 centre must equal Original, including pans/splits; at 4:3 the whole canvas
is equal. Untouched side pixels are black even with a coloured palette zero.

VGA400 usually has half-width/extent 160/201 (some palette options use 172), so
substantial side ink is possible. Extent-bound fields leave the strip from about
201 to 213 at 16:9 black; original compositions that genuinely reach farther are
not artificially capped. VGA200 is 160/161 and EGA350 320/321: only a narrow edge is possible.
EGA700 is 320/351 with doubled software Y and can expose modest side ink. Each
effect's actual reach determines its sides; no missing geometry is invented.
Lotsadots loses its old adapter and retains only actual original plot positions.

Extended selects Screen, redirecting an explicit Classic request with a message.
The saver selects Screen for Extended or an enabled image with available Native
selected, Classic otherwise. It preserves styles and keeps Style controls enabled;
old INI values remain valid. The independent side system, hooks, tests and capture
scripts are removed. Their measurements/rejected trials remain historical below
and in git history. Fade logic is unchanged; completed clears/erasers discard side
ink without making sides logical state. See native-render.md for the retired
side-adapter evidence and native/notes/screensaver.md for host checks.

## Native vs Classic comparison

The earlier s-pres report counted black background as agreement and gave broad
area/distance limits. It failed to identify a systematic colour-placement bug;
that conclusion and those limits are superseded. The five documents were restored
from HEAD before these targeted updates, preserving earlier measurements and
rejected trials. The integrator's corrected logical outline sample `(index-1)`
is retained; the later work below removes interval-count and cap compensation
errors around that sample.

Owner review of that rule, 2026-11-05: following the integer positions of sine and
corkscrew traced the original pixel staircase at 4K, which looked wrong. A staircase
is quantization, not the figure. Both now draw the real values the original truncates,
each moved half a cell toward zero (`dz_native_untruncated`), because `(int)v` selects
a cell centred about half a cell nearer zero. That curve is within half a cell of
every sample. Across seeds 1-8 at 4K, best shift is zero and at least 99.1% of Classic
ink is within one cell (99.97% for sine and 99.99% for corkscrew on VGA400 average), so
the figure is unchanged. Exact-cell overlap falls by design, to about 86% and 78% on
VGA400, because a smooth line of one cell does not cover every staircase cell;
within-cell is the shape guard for these two. Rule: where the original computes a real
value and truncates it into dense samples, draw the real curve. Straight edges the
original draws between sparse vertices stay straight.

Cycloids was the one exception the owner chose to smooth, the same day. Its 64 table
positions approximate a rolling curve, and the corners between them are visible at 4K.
A centripetal Catmull-Rom spline now passes through those positions, using their real
values before truncation. The neighbours come from the same table, so no lag or extra
state is needed, and `line_step` still sets the sample count of each span. The spline
bows away from the original chords between positions, most for a large pen radius, so
exact-cell overlap and colour fall by design, to about 71% and 67% on VGA400 at 4K.
At 4K best shift stays zero, and the lowest within-cell share is 95.3% on VGA400 and 91.7% on EGA350. The rule stands that
straight edges stay straight; this is a choice for a curve the original approximated.

### Reproduction and mapping convention

```powershell
python -B -I tools/native.py test
python -B -I native/tests/compare_native.py build/native/effect_oracle.exe build/workers/s-pres/recheck --pixels --distances
python -B -I native/tests/compare_native.py build/native/effect_oracle.exe build/workers/s-pres/recheck-centres --centres
python -B -I native/tests/compare_native.py build/native/effect_oracle.exe build/workers/s-pres/compare --gallery
```

Every expected image is a fresh **Classic logical index buffer**, with the same
seed, virtual time and complete logical state as Native. Nothing is derived from
a Native reference image. There are no masks, output-size branches or logical
pixel substitutions. The comparison fixture uses the original Auto work budget
and diagonal symmetry where the record permits it, after first/restart exercises;
it is a reproducible image fixture, not the DOSI random image-selection sequence.
The separate whole-program identity test exercises the real controller.

The full matrix is **1,224 comparisons**: all 17 effects, seeds **1..8**, VGA400,
VGA200 and EGA350, at raw, 2x-window and 4K sizes. Raw uses **Stretch at the logical
grid** (320x400, 320x200, 640x350): production mapping with both coordinate units 1.
The previous claim that raw comparison was impossible was wrong. The other runs
use Original and nearest-centre reduction to the logical grid. A 2x window means
640x800 / 640x400 / 1280x700; Original fits 640x480 / 533.333x400 / 933.333x700,
so the smallest grid pitch is 1.2 / 1.667 / 1.458 physical pixels, respectively.
This deliberately exposes sampling phase; it is not two samples on every axis.
At 4K the fitted area is 2880x2160. Actual viewport values are printed at full
precision and recorded with grid pitches.

Logical pixel `(i,j)` maps to `viewport.origin + (i+.5,j+.5)*grid_pitch`.
The reflection axis passes through logical pixel `(screen_half_width,
screen_half_height)`, not between its two central pixels. EGA700 has doubled
software Y and parity rejection; the requested raw suite is the three grids above.
The exact-centre/state suite still covers all four backends. Native and Faithful
use `dz_present_viewport` through the one canvas mapping.

JSON reports Classic/Native ink counts, overlap in **both** directions, exact
colour and within-1/within-2 on shared ink, raw and cyclic signed histograms,
Euclidean one-cell ink proximity in both directions, the best local integer shift
(search +/-2), all pixel-difference categories, and maximum same-colour distance.
Background-inclusive equality is secondary. Zero is background by index.
Cycling ramps are 3..255 (253 entries) or 3..15 (13): shortest signed cyclic distance
is used only when both indices are in the ramp; reserved 1/2 remain literal. Both
histograms are retained. A colour absent from the entire final Classic image gives
infinite same-colour distance (`null` plus the absent indices in JSON); it is not
silently omitted. Continuous interpolation/fill or different overlap coverage can
retain a colour the Classic raster completely overwrote.

### Before and after, on ink

VGA400, seed 1, 4K Original reduced to the grid. Each pair is **Classic-ink overlap /
exact colour on shared ink**, percent. Before already includes the integrator's
outline-start fix. Sweep2 uses seed 1 consistently here; the earlier scratch script
used seed 3 for that effect.

| Effect | Before O / C | After O / C |
|---|---:|---:|
| arches | 96.6 / 59.1 | 96.8 / 99.5 |
| cones | 98.9 / 87.5 | 98.4 / 96.3 |
| yarnball | 78.0 / 65.6 | 97.2 / 97.6 |
| gumballs | 99.3 / 81.6 | 99.8 / 98.5 |
| bars | 100.0 / 76.4 | 99.8 / 94.5 |
| boxes | 99.5 / 10.5 | 100.0 / 99.9 |
| puffs | 96.6 / 6.6 | 97.5 / 99.2 |
| wandering | 98.7 / 95.8 | 99.4 / 98.9 |
| bigstar | 98.5 / 61.6 | 99.2 / 95.9 |
| littleStars | 98.4 / 32.9 | 99.6 / 94.5 |
| cycloids | 66.5 / 33.5 | 98.0 / 98.5 |
| corkscrew | 58.3 / 74.9 | 100.0 / 100.0 |
| ellipse | 97.6 / 44.5 | 99.5 / 97.3 |
| daisies | 99.5 / 17.9 | 100.0 / 96.4 |
| squirals | 71.6 / 68.3 | 98.0 / 99.0 |
| sweep2 | 96.7 / 79.3 | 99.8 / 96.6 |
| sine | 68.4 / 81.8 | 100.0 / 99.8 |

The full per-mode/per-seed report is
`build/workers/s-pres/pixels-final/pixels.json`; it includes counts and histograms,
not just these percentages. The following minima are independent minima over all
eight seeds and three modes (they need not occur in the same frame).

| Effect | Raw O / C | 2x O / C | 4K O / C | Traced residual mechanism |
|---|---:|---:|---:|---|
| arches | 99.7 / 99.2 | 78.1 / 76.7 | 95.5 / 97.3 | analytic/midpoint contours; reduction phase |
| cones | 99.5 / 99.7 | 85.9 / 75.5 | 96.7 / 95.0 | radius-1 midpoint corners; contour overlap and phase |
| yarnball | 99.0 / 98.9 | 81.1 / 78.1 | 96.6 / 97.5 | older ring visible at different conic boundaries; phase |
| gumballs | 98.5 / 97.9 | 96.7 / 96.1 | 98.3 / 97.8 | real fill boundary versus cached row/pole pixels |
| bars | 100.0 / 100.0 | 98.8 / 70.3 | 99.8 / 94.5 | nearest reduction phase at ray/edge crossings |
| boxes | 100.0 / 100.0 | 99.6 / 82.6 | 99.9 / 95.1 | nearest reduction phase at ray/edge crossings |
| puffs | 100.0 / 100.0 | 79.4 / 81.0 | 94.3 / 97.5 | nearest reduction phase at ray/edge crossings |
| wandering | 100.0 / 100.0 | 84.4 / 73.1 | 97.0 / 91.8 | nearest reduction phase at ray/edge crossings |
| bigstar | 100.0 / 100.0 | 94.1 / 61.8 | 98.5 / 95.4 | nearest reduction phase at ray/edge crossings |
| littleStars | 100.0 / 100.0 | 93.5 / 60.5 | 98.5 / 92.6 | nearest reduction phase at ray/edge crossings |
| cycloids | 56.7 / 49.4 | 55.1 / 46.3 | 56.5 / 49.4 | spline through table positions bows off the chords; within-cell is the guard |
| corkscrew | 71.4 / 89.5 | 65.2 / 84.6 | 71.5 / 88.5 | smooth real curve through truncated cells; within-cell is the guard |
| ellipse | 100.0 / 100.0 | 95.1 / 53.3 | 98.7 / 86.2 | nearest reduction phase at ray/edge crossings |
| daisies | 100.0 / 90.8 | 98.0 / 61.6 | 99.6 / 88.2 | approved surface fills holes, including over older flowers; phase |
| squirals | 100.0 / 100.0 | 82.6 / 74.7 | 95.9 / 95.7 | nearest reduction phase at ray/edge crossings |
| sweep2 | 100.0 / 100.0 | 87.0 / 57.6 | 99.5 / 90.3 | nearest reduction phase at ray/edge crossings |
| sine | 84.0 / 91.0 | 73.0 / 84.1 | 83.0 / 90.0 | smooth real curve through truncated cells; within-cell is the guard |

### Mechanisms, fixes and remaining differences

- **Sample ownership:** `line_step(...,1)` stores its start; subsequent calls
  advance before plotting. Colour changes before/after that plot differ by caller.
  A sample now owns the interval between adjacent midpoints (`dz_native_bounds`).
  Internal colour boundaries use butt cuts; round caps remain only at actual ends.
  Growing rays reveal at most the current pen cell. Boxes/puffs/bars still omit
  their unplotted terminal sample and stamp the reset pen. Bigstar uses the actual
  cached point (including old entries), and littleStars retains conditional copies
  and per-copy colours. This fixes the shared interval/round-cap lead.
- **Outline schedule:** 256 cached positions describe 255 intervals. Dividing by
  256 compressed the colour schedule. Then relying on the next round cap to reclaim
  half a step caused a tangent-dependent lag when pen weight was projected. The
  same explicit midpoint rule now applies to arcs; the corrected logical sample
  index remains `(index-1)`. Arches seed-1 4K exact is 99.5%, versus 59.1% before.
  The Native safety predictor's region-switch term also had its sign reversed:
  it now uses original `(yy-xx)/4`. A test compares 100 predictions with actual
  original `ellipse_step` runs, including thin traces that never finish.
- **Paths:** cycloids joins the original separately truncated 64-entry orbit/pen
  table contributions and fixed-point X results; a truer rolling curve was a
  different figure. Squirals uses original integer vertices. Ellipse fans use the
  original integer boundary endpoints. Yarnball receives the actual staged integer
  centres and uses the original rounded derived radius. Sine/corkscrew connect the
  positions after the original truncation/even-coordinate operations. Their extra
  Native ink is interpolated between sparse samples, particularly EGA350, not a
  second displaced figure. Sine's minimum Native-ink overlap is about 74%, while
  its Classic-ink overlap is 100%; both directions remain visible in the report.
- **Copy order:** squirals' rotated pair was reversed. Equal colours hid this in a
  completed picture. A first-copy test fails the old order and verifies original
  `(cx+saved_y,cy-saved_x)` before its opposite. The correction changes intermediate
  reveal order; complete-image metrics remain unchanged.
- **Weight and numeric mapping:** each pen axis is a transformed logical pixel
  cell, with a one-physical-pixel floor for small previews. Its normal weight is
  `max(abs(dx),abs(dy))/hypot(dx,dy)` in pixel-cell units: one minor-axis cell,
  not a fitted diameter. Arches raw excess ink drops from 46.15% to about zero.
  Mapping divisions now occur in double (the former float division turned 5.4
  into 5.400000095). The shallow line (0,0)->(80,-1) exposed a lost half tie at
  (40,-1); condition-aware span rounding and original directional `error>=0`
  ownership fix it. **4,880 translated lines**, all octants and constant/per-step
  colours, compare exactly against real Classic stores at the raw production map.
- **Small circles:** integer radius 1 starts with error `1/4-1 == -1`. Its first
  step makes zero and strict `error>0` keeps Y=1, so Classic draws (1,1), outside
  the unit conic. In VGA200 seed 5, all **88** missing cones pixels (of 16,020
  Classic ink) trace to this radius-1 corner, with the Classic trace model itself
  checked byte-exact. There is no radius-specific exception or global thickening.
- **Large colour deltas:** puffs' raw -252 is cyclic +1 (255->3). Yarnball's -191
  is cyclic +62, not an adjacent wrap: at raw (-56,-120), Classic first writes 3
  on radius 16x27, then 194 on radius 17x29 at local (13,18). The conic at X=13
  has Y about 18.69, so different contour coverage can expose the older ring.
  Gumballs' +78 example (74,62) is Classic 33 / Native 111, with 111 immediately
  lower-left in Classic: a newer shaded ball replaces an older solid ball at a
  boundary. The systematic band warp was fixed by keeping colour cells at their
  logical coordinates and clipping to the real ellipse, extending only the edge
  cell. Remaining boundary/older-ink differences are still counted as wrong colour.
- **Reduced sampling:** at 4K VGA400, X centres align (pitch 9) but Y pitch 5.4
  leaves nearest sample offsets up to 0.4 physical pixel, or 0.074 logical cell.
  This can change the last overlapping ray at a thin band boundary. Ellipse's
  worst 4K case (VGA400 seed 3) is 86.2% exact, mainly neighbouring indices;
  its raw case is essentially exact and best shift remains zero. Puffs' minimum
  4K Classic overlap is 94.3% in EGA350 seed 1: sparse thin rays and diagonal nearest
  neighbours, with no raw displacement. These are reported, not masked. At 2x,
  every effect except corkscrew/gumballs/sine has a case below 90% exact or 95%
  Classic overlap; the table identifies all of them. Those failures of literal
  equality remain a measured limit of nearest reduction.

### Daisies' retained filled holes

The former analytic arc did not follow its cached chord endpoints. Native now
fills between the **actual original cached rays**, then paints each ray's own
sample cell in the original order. Repeated truncated endpoints own the same
angular cell, so the later ray wins. A temporary rays-only ablation (always against
Classic, never used as expected output) is 100% exact on shared raw ink for all
24 mode/seed cases. Production retains the approved surface. Thus all remaining
raw shared-colour differences in this matrix are caused by that surface covering
holes through which older flowers were visible, not by an unexplained colour lead.
The ablation is restored; `daisies-rays-only/` is diagnostic evidence only.

For the seed-1 VGA400 4K comparison, Native-only ink is 2,690
cells. Of 2,680 enclosed four-neighbour Classic background cells, 2,672
are inked by Native; the conservative eight-neighbour count is 1,812 of
1,820. These are explicit connectivity measurements of the composite image,
not a mask or a claim that every enclosed region is an isolated petal hole.
Shared wrong-colour pixels are 2,568; older artwork showing
through a newer flower's holes also contributes. The worst filled-surface 4K
exact score is 88.2% (VGA200 seed 8), while the same case's raw ray-only comparison
is exact in shared colour. Its filled appearance remains deliberate.

### Regression guards and review evidence

`native-pixel-limits.json` contains on-ink floors shared across modes/seeds, with
per-effect causes. Raw primitive colour/overlap floors are 99.9%, backed by the
exact line cases; conic, sparse-sample and approved surface differences have their
own stated budgets. 4K separately guards both overlap directions and colour.
The broader 2x phase envelope is paired with strict raw/4K guards and one-cell
proximity, so it cannot excuse a shared algorithmic bias. Raw best shift must be
zero; every effect/mode must actually exercise Native, not only declared fallbacks.
These are regression envelopes over classified variants, not universal analytic
accuracy bounds. All residual counts and histograms remain in the report.

The fast CTest form is **612 comparisons**: all eight raw seeds in all three modes,
plus 2x and 4K for seeds 1/2. Two temporary mutations prove the colour guards:
ending intervals with round caps give boxes only **8.6%** exact shared ink;
counting positions as intervals gives arches **45.0%**. Both retain about 99.9%
ink overlap and unchanged logical state, but fail the colour floors. Both source
files and executable were restored. Receipts: `mutations/receipt.json`.

The complete gallery is `build/workers/s-pres/compare/<effect>/`, with 4K
1-original-faithful, 2-original-native, 3-extended-faithful, 4-extended-native,
5-stretch-native, Classic grid, Native reduced grid and the difference image.
There is also a 1920x1080 Classic/Faithful, Native Original, Stretch and Extended
set for every effect. Extended has marked and unmarked versions at both sizes.
The manifest contains 153 same-state captures. Contact sheets include unscaled 256px
crops at figures and the 4:3 boundary. All 17 were visually inspected, including
empty space: no extra figures, leading ray overshoot or new gaps were found.
Stretch overviews and unscaled crops were also reviewed for all 17 effects: the
geometry and logical-cell pen transform together, with no one-axis hairlines.
The intentional filled daisies remain visible. Gallery layouts remain for the owner to judge.

The running source/geometry questions and rejected diagnostic trials are retained
in `build/workers/s-pres/deeper-issues.md`. The unresolved aesthetic choice is
whether a later design may smooth original integer-step paths; this work follows
the stated original-position contract.

### Actual Extended side coverage

The following is the seed-1 Auto-budget fixture at 1920x1080, diagonal enabled
where permitted (sweep2 seed 3), percent of the two side areas with nonzero ink.
It is one reproducible valid Auto variant, not a claim that every variant reaches
that much. First/restart/long/pan/split cases are included in the exact-centre
receipts; the table below is the Auto observation only.

| Effect | Faithful side % | Native side % |
|---|---:|---:|
| arches | 5.824 | 5.802 |
| atom | 0.000 | - |
| balls | 0.000 | - |
| bars | 0.000 | 0.000 |
| bigstar | 0.000 | 0.000 |
| boxes | 0.000 | 0.000 |
| cones | 19.348 | 19.155 |
| corkscrew | 1.410 | 1.542 |
| curves | 0.000 | - |
| cycloids | 0.000 | 0.000 |
| daisies | 0.000 | 0.000 |
| diag1 | 16.751 | - |
| diag2 | 5.823 | - |
| ellipse | 2.620 | 2.568 |
| formula1 | 0.000 | - |
| grid | 0.000 | - |
| hexagon | 0.000 | - |
| hilbert | 0.000 | - |
| lines | 0.000 | - |
| littleStars | 1.439 | 1.442 |
| lotsadots | 2.686 | - |
| pedals | 0.000 | - |
| puffs | 1.387 | 1.383 |
| ribbons | 9.478 | - |
| triangles | 25.882 | - |
| tunnel | 0.000 | - |
| sierpinski | 0.000 | - |
| sine | 0.000 | 0.000 |
| spiral | 11.391 | - |
| spiral1 | 14.100 | - |
| squares | 8.710 | - |
| squirals | 0.532 | 0.503 |
| sweep | 0.800 | - |
| sweep2 | 9.299 | 9.259 |
| wandering | 0.000 | 0.000 |
| zigstream | 0.000 | - |
| butterflies | 17.488 | - |
| diamonds | 1.982 | - |
| gumballs | 0.000 | 0.000 |
| tiles | 0.000 | - |
| wedges | 1.628 | - |
| yarnball | 0.000 | 0.000 |

The earlier probe's "stays inside" list is not an invariant: **triangles,
sierpinski, tiles and wedges** have genuine side ink in these runs (some only in
later images). Source `plot_callback(3)` also exchanges X/Y: raw-coordinate-only
reach misses those copies. Triangles already inks 25.882% of the side area in this
fixture. Sierpinski can choose an extent above 160; boxes can reach 165 from its
origin plus width even though this seed does not. No per-effect black-side filter
was added to preserve the earlier classification.

Balls and lines are both zero-sided in the displayed Auto fixture. They were
rechecked in source: balls can reach `hw + bitmap_width/2` (stride-dependent), and
its permitted swapped Y copies may also extend; lines bounces against
`screen_extent`, not `screen_half_width`. More seeded observations are recorded
with the final validation. A zero side snapshot is not proof of an inside-only
algorithm.

| Mode | Half-width / usual extent | Effects with side ink (of 42) | Mean Faithful side % | Largest fixture |
|---|---|---:|---:|---|
| VGA400 | 160 / 201 | 20 | 3.776 | triangles 25.882% |
| VGA200 | 160 / 161 | 11 | 0.394 | arches 10.996% |
| EGA350 | 320 / 321 | 10 | 0.144 | arches 3.908% |
| EGA700 | 320 / 351 | 13 | 0.535 | triangles 9.319% |

Extent-bound fields have little reach beyond hw in VGA200/EGA350, as expected.
Original half-width/height compositions and swapped copies can reach farther,
including arches in those modes. Do not impose `screen_extent` as an artificial
clip: the outer 16:9 strip stays black **where the actual logic does not reach it**.
Some compositions genuinely reach beyond +/-201. The centre remains byte-exact
with Original in every case; this is not spreading.

### Current raster CPU and verification

Raster CPU was measured only for the 17 changed adapters, at 4K, seeds 1..8,
using per-thread CPU cycles around a bounded Auto generator (geometry and raster
work), with fresh Classic controls and exact logical-state equality. Capture,
viewport setup, PNG generation and presentation are outside that timer. Values
below are median **added million cycles per Auto image**, Native minus Classic;
Auto work differs by effect, so these are not cross-effect frame-rate comparisons.
Machine frequency/cache variation remains; small Original/Extended reversals do
not establish a speedup. Accelerating sweep2 fallbacks are excluded (four Native
seeds); other rows use eight Native seeds. Receipts: `raster-auto/cycles.json`.

| Effect | Original added Mcycles | Extended added Mcycles |
|---|---:|---:|
| arches | 205.16 | 242.70 |
| bars | 45.21 | 50.19 |
| bigstar | 93.26 | 95.31 |
| boxes | 120.02 | 128.41 |
| cones | 126.22 | 165.11 |
| corkscrew | 29.16 | 31.26 |
| cycloids | 33.23 | 36.77 |
| daisies | 434.87 | 415.17 |
| ellipse | 143.38 | 141.79 |
| gumballs | 22.82 | 28.03 |
| littleStars | 143.50 | 142.59 |
| puffs | 43.14 | 48.07 |
| sine | 18.29 | 21.09 |
| squirals | 111.57 | 117.57 |
| sweep2 | 44.68 | 48.87 |
| wandering | 93.06 | 101.18 |
| yarnball | 58.05 | 59.47 |

The older Semi-Auto/F10 CPU fixture is rejected for arches: its unbounded run
reaches original malformed cache index 702. That is not a Native performance
result; the negative log is retained. Earlier batch CPU measurements above remain
historical and are not re-labelled as measurements of these changed paths.

Final `python -B -I tools/native.py test`: **46/46 passed** (382.93 seconds),
including the 612 on-ink cases, exact 4:3/wide centre checks, unchanged Classic
integration/acceptance hashes, original stores, sampling fades and all 24 fades
under all four presentations with Native ink. The physical audit passed
**16,761,046 calls**, `faithful_in_native=0`, `native_in_fallback=0`.
The full `--native none` canvas oracle passed **960** state/scanout comparisons.
The full image-centre suite has **4,248** exact state/centre pairs (42 Faithful plus
17 Native, four backends, three sizes, six first/restart/Auto/long/pan/split phases);
modified arc groups were refreshed separately. Current CTest also covers the
whole-canvas equality at a 4:3 window. No centre exception or threshold is used.

The final full real-program identity run passed **4,896 state observations**:
17 effects, four modes, three sizes (641x479, 1920x1080, 3840x2160), three styles
and eight deadlines. All 612 configurations preserve complete Classic logical
state; receipts are in `identity-full/`. An earlier broad CTest run while other
oracle processes were busy timed out `test_dzfd`; its negative log is retained
as `ctest-followup.log`. The subsequent quiet required gate passed all 46 tests
without changing a timeout or weakening a check.

The built `dazzle.scr` was exercised as a real Win32 saver in Original and
Extended on the **2560x1440** display. Both `/c` dialogs showed enabled Style/All
Native/All Faithful controls and the plain Extended label; `/s` and the GPU captures
completed normally. Test hooks isolate input and capture a fixed virtual deadline;
the actual saver/presenter runs. Captures/receipts: `host/` and `host-current.log`.
4K review images are software-oracle captures, not a claim of a physical 4K host.
All started saver processes were closed. DOS source/assets and pinned reference
JSON files are unchanged; no commit was made.

Seeded balls/lines recheck (`side-recheck/sides.json`), VGA400 16:9 Auto-budget
fixture: balls has side ink for seeds 3/4/6 (maximum **10.770%**, seed 4); lines for
seeds 2/4/5/6/7/8 (maximum **6.690%**, seed 8). Both therefore continue past the
edge when the original variant reaches it; seed 1's black sides are correct.
