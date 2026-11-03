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
   Open item for this phase: antialiasing in an indexed buffer (e.g. index + coverage + underlying index, blended
   after palette lookup) versus crisp native strokes; decide with the first native effects.

## Implementation status

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

All four runs completed normally in 20.08–20.13 wall seconds and left no processes running.
The Screen canvas's own dynamic storage for VGA400/Original is 5,109,120 bytes at 1080p
and 20,413,440 bytes at 4K, excluding the logical receipt, static bookkeeping and SDL
resources. Peak process private memory also includes mapping-time target/readback buffers
and driver allocation retention. These are single host samples; an earlier exploratory
batch ranged from 14–16% Classic and 23–47% Screen, so the table is not a performance
guarantee. The Faithful result is identical; Screen spends more CPU and memory maintaining
the output-resolution aperture and uploading output-resolution indices.
