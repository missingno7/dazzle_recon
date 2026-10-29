# Hybrid rendering and Extended widescreen

Design and CPU prototype, 2026-10-29. The source-derived inventory is
[patterns.md](../native/notes/patterns.md). Build on the concurrent indexed Classic
presenter and its viewport transform; retain `dz_display` / `dz_create_window` as
window/lifecycle ownership. The implementation status below distinguishes the
shipped CPU seams from the proposed GPU/Extended APIs.

## Three spaces and four composition policies

| Space / mode | Contract |
|---|---|
| Simulation | Existing logical indexed `dz_video.vram`, palette, word arithmetic, RNG and cooperative virtual clock remain authoritative. Reads always read these pixels. |
| Rasterization | Hybrid emits explicit continuous geometry into an indexed coverage layer at the current output pixel density, addressed by **VRAM position**, before display-start/split readout. |
| Presentation | The presenter maps the selected VRAM rows to display coordinates, applies the viewport transform, performs palette lookup, and fills bars/border. It owns window size, fullscreen, preview and DPI. |
| Classic | Original pixels, nearest sampling, original aspect and palette cycling. No geometry instrumentation may affect its logical state or reference conversion. |
| Hybrid | Original execution plus selected stroke observations. Dots, masks, bitmap art, scan fills and unsupported operations remain Classic. No bilinear filtering of the logical framebuffer. |
| Enhanced, later | An explicitly different per-algorithm variant may change precision, sampling, limits and buffers; it has its own fixtures. It is not enabled by choosing Hybrid. |

**Original** fits the original 4:3 display into the window (default); **Crop** uniformly
covers it and clips; **Stretch** fills independently in X/Y. Apply the same presenter
transform to Classic pixels and Hybrid geometry. Treat **Extended** as a composition
capability selected before this transform, not as another generic scale matrix.
Unsupported Extended content uses the Original 4:3 composition; do not silently stretch.

## Implementation status

Milestone 3 now has a complete **CPU Hybrid** path, selected with developer option
`--quality hybrid`. Classic remains the zero-valued default and allocates no Hybrid
arrays. The Classic indexed/RGBA presenters and `dz_video_rgba()` remain the reference.
No GPU compositor, shader build, ellipse emitter or Extended canvas is implemented.

`platform/hybrid.c` owns plain full-aperture arrays: 12-byte logical cells
(sequence, stroke, generation, saved-under index and flags), a 32-bit owner and
16-bit 4x4 subpixel mask per raster sample, and output-sized ARGB8888. The mask
is converted to coverage8 by `(popcount * 255 + 8) / 16`; keeping the bits makes
same-index coverage union exact rather than repeatedly blending alpha. The
palette is fetched only during composition and uses the integer RGB equation below.
Address-to-raster offsets and output column mappings are small precomputed arrays.

Every actual kernel store, clear lane, EGA planar bit store, mode-clear byte and
VRAM-directed logo/PCX decode store reports its old/new index after storing. Mode
clear uses its original `memset` in Classic and an observed loop in Hybrid. PCX
save functions only read VRAM; PCX input to VRAM is the logo decoder. Application
callbacks retain their identities, and no observation charges work, samples RNG,
yields, changes application globals or writes canonical memory. XOR is suppressed
only for actual EGA XOR; VGA ignores the simulated EGA raster-op register as before.
Mode/backend/logo entry, XOR, Quilt and tetrYs retire the entire layer. This is a
conservative fallback over the complete aperture, including possible word aliases.
Image rebuild/selection and fade entry cancel pending intent and continuity while
retaining ink until an actual overwrite or required retirement.

Cycloids declares the existing quantized straight joins and projects each actual
Bresenham step onto that join. Sine captures the existing double products once,
retains the original narrowing and applies the original EGA even-coordinate shift
to the fractional observation. Sweep2 starts a new primitive at every ray reset and
projects only the outer plot's reached point. Eight separate symmetry histories
follow the actual kernel branches; rejected/wrapped mappings provide no coverage.
Generation and sequence guards retire before reuse. Duplicate stores transfer both
cached tokens and branch history while preserving the first saved-under index.
Support is confined to the supplying cell and its one-cell halo. Competing inks
discard replacement masks without nesting underlays. Invalid top fragments reveal
canonical pixels; there is no hidden geometry stack or retained-vector replay.

The presenter uploads the resolved output-sized RGBA texture and uses its existing
renderer/window lifecycle. The same `dz_present_viewport` supplies bars, Crop and
Stretch; base and coverage share the exact start/split/aperture address and cell
fraction. Text remains Classic. Writes advance a presentation revision; palette,
display registers and output changes also invalidate composition. Unchanged frames
skip composition, upload and drawing. Forced exposes reuse the cached composition.
CPU allocation/cap or texture failure drops the layer and uses the Classic path.

Deliberate implementation choices relative to the proposed tiled/GPU design:

- Raster density is `ceil(content_width / logical_width)` by
  `ceil(content_height / physical_height)` samples per logical cell, retaining
  fractional viewport edges during scanout. This is at least output density.
  A first attempt using fractional global cache cells produced gaps at fractional
  Crop boundaries: a cached fragment could belong to a neighboring destination
  cell with a newer sequence. Integral cell subdivision removes that ambiguity.
- Plain contiguous arrays replace lazy tiles and dirty rectangles. A changed
  composition validates the full raster layer, then resolves the output; frozen
  frames reuse it. The bounded source halo still limits rasterization and duplicate
  retagging. CPU cost, measured below, is the remaining reason to optimize this.
- Top coverage retains 16 sample bits (six bytes with its owner), rather than five
  bytes of owner/coverage8. No RG8/mask GPU textures or staging arrays are needed.
- Application-owned Hybrid storage is capped at **320 MiB**, including metadata,
  mapping arrays and output. Allocation uses `size_t` after checking the bound.
  Over-cap density or allocation failure falls back to Classic; Classic is never
  downsampled. Additional raster limits are 32768 columns and 131072 rows.
  At 1920x1080 Original the arrays occupy 37.45 MiB in VGA400 and 51.96 MiB in
  EGA350/700. At 3840x2160 Crop they require 182.81 / 201.83 MiB respectively.
  SDL texture/driver memory is additional. Memory stays independent of run length.

Verification commands and receipts:

- `python -B -I tools/native.py test`: **33/33 passed**, including the new
  `test_hybrid`. Neither integration-hashes.json nor acceptance-reference.json
  was changed. DOS reconstruction files and immutable assets were untouched.
- `python -B -I native/tests/verify_hybrid.py build/native/dazzle.exe build/workers/s-hybrid/state`:
  80 full-state comparisons for four images in four modes at 8.2, 8.25, 8.5, 9
  and 10 virtual seconds; eight more comparisons in a 60-second Auto run with
  Gas cloud and Linewipe fades. All equal. Each image fixture changes full VRAM
  at all five samples, so these exercise progressive drawing, not idle selection.
  Receipts include the whole 524288-byte VRAM array, both palettes/attributes,
  RNG state and call count, charged work, virtual ns/ticks, draw diagnostics,
  page/backend and all display registers. Hybrid actually renders with a small
  dummy output during these tests; it is not a headless observer bypass.
- Unit tests cover horizontal/vertical/diagonal joins and physical footprint in
  VGA400/VGA200/EGA350/EGA700 with all three policies at fractional 701x479;
  dots above/below strokes, same-color writes, duplicate/competing ownership,
  partial/alternating clears, planar writes, generation retirement, palette
  recoloring with exact integer blending, both page origins, live partial-row
  wrap and split boundary, resize/cap fallback, EGA XOR twice, preserve-memory
  mode retirement, non-finite coordinates, independent symmetry branches and
  pending geometry during a timer yield. Presenter tests check unchanged frames,
  palette/register invalidation, forced redraws and output/policy resize.
- `capture_hybrid.py` writes paired PNGs and full-state receipts under
  `build/workers/s-hybrid/` at **1920x1080 and 2560x1440**, seed 1, Original.
  Cycloids/sine/mixed use 8.15 virtual seconds; sweep2 uses 9 seconds. The mixed
  case switches from sine to lotsadots without clearing. Additional `retired`
  captures at 12 seconds follow Quilt and clear. `captures.json` records size,
  policy, backend and time. Captures were inspected: thin smoothed joins,
  unchanged raster background and rectangular dots, and no stale strokes after
  retirement. Dense crossings show the specified bounded-quality canonical
  fallback; they do not preserve a hidden smooth stroke stack.

CPU measurements use Windows process kernel+user time, as a percentage of one
core, with the ordinary real-time clock, default hardware renderer and a
1920x1080 window. The script switches to sine at 8 seconds and F10 at 8.01.
`measure_hybrid.py` samples exactly 20 wall seconds, then lets the 20-virtual-second
run exit normally; `build/workers/s-hybrid/cpu-20s.json` contains commands and times.

| Quality | Wall sample | CPU in sample | One core | Wall to 20 virtual seconds | Total CPU |
|---|---:|---:|---:|---:|---:|
| Classic | 20.000 s | 1.656 s | 8.28% | 20.071 s | 1.703 s |
| CPU Hybrid | 20.000 s | 15.000 s | 75.00% | 34.795 s | 29.906 s |

These are single-run host measurements, taken sequentially without other test
processes. CPU Hybrid does not sustain real time for this dense workload.
The CPU reference can fall behind wall pacing on dense sine; this is a prototype
correctness path, not a low-power replacement for Classic. A GPU implementation
or measured raster/composition optimization should be the next performance work.

Not exercised: real per-monitor DPI/multi-monitor/preview hosts, actual allocator
exhaustion or device loss, and sustained 4K throughput. The storage-cap fallback
and output/policy resize are exercised without those hosts. The sequence wrap
guard is reviewed; the generation-wrap path is executed by the unit test.

Rejected approaches used MinGW GCC 12.2, C17, RelWithDebInfo
(`-O2 -g -DNDEBUG -Wall -Wextra`; assertions enabled with `-UNDEBUG` in tests).
Fractional global cache cells failed the VGA400 Crop continuity check at 701x479;
integral subdivision replaced them. Splitting subpixel coverage at color midpoints
punched holes inside otherwise solid strokes; indices now choose a band at the
raster center while coverage measures the complete footprint. Blind duplicate
retagging could make an already occluded halo valid again before a present;
retagging now checks the destination against the old supplying sequence. Tests
explicitly cover full opacity at a color boundary and this interrupted overwrite.
An initial endpoint-mode fixture used seeds 1..8, whose first RTL draws all chose
mode zero; seeds multiplied by 50 exercise both modes and the test asserts that.
Running captures concurrently with the full suite also hit the existing 30-second
`test_dzfd` timeout. The final full suite was run separately and passed 33/33.

## Explicit stroke seam

The first prototype is **cycloids + sine + sweep2**, with **lotsadots** drawn over the
result as the mixed-rendering control. Cycloids already joins quantized trig samples
with raster lines; sine has an explicit phase-driven trajectory; sweep2 tests progressive
line drawing, endpoint resets and caller symmetry. Cones/arches can add ellipse support
next. Avoid filled polygons and pixel-moving fades in the initial geometry whitelist.

```c
/* Presentation-only observations. Coordinates are original software coordinates,
   centered like draw_x/draw_y. Footprint is in physical logical pixels. */
typedef uint32_t dz_stroke_id;
dz_stroke_id dz_stroke_begin(enum dz_stroke_kind kind);
void dz_stroke_segment(dz_stroke_id id, double x0, double y0,
                      double x1, double y1, uint8_t index,
                      uint16_t symmetry, double footprint_x, double footprint_y);
void dz_stroke_ellipse(dz_stroke_id id, double cx, double cy,
                      double rx, double ry, uint8_t index, uint16_t symmetry);
void dz_stroke_break(dz_stroke_id id);
/* Private kernel observer, after an actual successful store; also used for bulk writes. */
void dz_hybrid_write(uint32_t address, uint8_t old_index, uint8_t new_index,
                     enum dz_write_kind kind, dz_stroke_id id);
```

`segment` declares intent and the next eligible prefix, **not permission to draw the
entire future segment**. The observer commits coverage only after the corresponding
original plot succeeds. A small per-variant context carries current primitive, previous
sample, branch and color interval; ordinary plots have no such context. No generic
pixel-stream connector exists. `stroke_break` is mandatory on rebuild, teleport, wrap,
new object, endpoint reset, rejected/unrepresentable address mapping, or symmetry branch
change. Reject non-finite coordinates. Shared line/ellipse helpers need explicit opt-in
from the image; a raster fill calling the same plot kernel does not become geometry.

| Variant | Arithmetic reuse and emission boundary |
|---|---|
| cycloids | Keep 64-sample trig, all int16 truncations, `mul32`/`asr32`, line loops and P(0) unchanged. Capture the **existing integer endpoints**, represented as doubles. Emit the straight joins already specified by `line_step`, never an ideal spline or a denser cycloid. Partition progress/color at each actual plotted step; per-point color remains discrete. Break between curves. |
| sine | Evaluate existing `amplitude_x * sin(sine_phase)` and `amplitude_y * cos(sine_phase)` once into the same expression type (double), retain the original int16 conversions, phase/amplitude updates and EGA even rounding. Capture those temporary results and post-rule index for a declared path to the previous sample. Do not call the image twice. Break at rebuild or a narrowing discontinuity; only accepted plot branches supply coverage. |
| sweep2 | Observe each initialization's `(0,0)` and existing end point. Emit only the prefix reached by the most recent **outer** P(D), not the whole ray on initialization. A new ray starts a new primitive. Its original endpoint-step/RNG/color code is unchanged. |
| lotsadots | No stroke calls. Two coordinate draws and any color-rule RNG draw stay intact; its actual stores retire overwritten geometry. `lines`, `atom`, `curves` and `pedals` also stay stamps initially; their anchor jumps must never be joined. |

Observation consumes no random numbers, calls no `dz_charge`, retrace, palette or input
service, and changes no original variable, including diagnostic symmetry globals.
`plot_symmetric` currently charges **before** storing (DVDT:105); presentations during
that charge must see only earlier committed geometry. Commit synchronously after each
store, before the next original yield. Never defer an entire image's geometry until it
returns: cycloids can present several times inside one call. Keep the original callback
identity/backend tests working; use kernel observation rather than replacing callbacks.

For symmetry, generate distinct reflected/swapped branches with the same flags and
order as `plot_pass`, and attach them to the actual stores it reports. Preserve duplicates,
edge suppression and doubled-EGA parity rejection in the logical run. The observer must
not call `plot_pass` a second time. Map software coordinates to physical pixel centers
(`half + coordinate + 0.5`; halve translated EGA700 Y before adding its physical 0.5),
then add the actual page base. The successful store address is the authority if word
addressing disagrees with this ordinary geometric mapping; that branch falls back.

Sweep a **one-physical-pixel box footprint** along the declared path, then transform it
to raster space. This preserves the original pixel's anisotropy rather than choosing a
one-monitor-pixel hairline. EGA700's accepted software rows are two units apart but each
store is one physical row. Thicker Hilbert segments later use their actual square stamp
size. Square caps/joins and a fixed 4x4 subpixel area-coverage rule are sufficient for a
CPU prototype; GPU coverage must use the same rule initially. Index changes split bands
at midpoints between committed samples, never interpolate indices. No endpoint or color
band beyond the committed prefix is visible. Ellipse emitters likewise need a reached
quadrant/arc interval, not a whole ellipse before its first delayed store.

## VRAM layer, ownership and overwrites

Use **one top fragment per raster pixel**: an 8-bit coverage and a 32-bit owner token in
CPU arrays. A token packs the source logical address (19 bits, covering 524288 pixels)
and a per-address generation (13 bits, reserve zero/terminal values). Per logical address
keep last-write sequence, generation, stroke ID, saved underlying index and flags (about
12 bytes). Every successful store, including an unchanged-color store, increments the
write sequence and generation. Before either counter wraps, retire the entire layer and
restart metadata; these counters never wrap into apparently valid old owners.

Rasterize only fragments supported by actual writes of the declared primitive. Choose
the nearest supplying plot cell along the reached path (ties follow store order); retain
its token and read its current index only while its generation is valid. Limit support
to one physical-cell halo beyond that source cell for the initial thin-stroke variants.
If a portion has no accepted supplying cell in that neighborhood, leave it Classic.
This bounds dirty-region invalidation and prevents an emitter bridging rejected samples.

For raster position `r`, let `q` be its destination logical cell, and `o` its supplying
owner cell. A cached fragment is usable only if:

```text
token.generation == meta[o].generation
and meta[o] still describes an eligible replace stroke
and sequence[q] <= sequence[o]
and coverage[r] != 0
```

The logical value at `o` supplies the palette index; replacing/erasing `o`, even with the
same index, invalidates the old token. A later write to `q` occludes the fragment even if
it never touched `o`. The antialias halo may cover an adjacent **older** pixel of another
color: color authority is its supplying plot, not an equality test against the destination
cell. Requiring the stroke index to equal every destination's nearest logical index would
make palette-over-base blending invisible there and preclude smoothing at cell edges.
No fragment survives loss of its supplying color or a subsequent destination overwrite.

Use newest valid sequence to select the single top fragment. For overlapping fragments
of the same primitive/index, union coverage (do not repeatedly alpha-blend it). Different
primitives replace the top fragment. Do not retain hidden vector stacks or resurrect an
old fragment when the top is invalidated; reveal the canonical raster there. This is an
intentional bounded-quality fallback at crossings, not a claim of exact vector layering.

Simply overlaying strokes on the already plotted staircase leaves the Classic jagged
silhouette underneath. To actually replace that silhouette, keep a **presentation-only
saved-under index** for eligible owned cells. On a cell's first eligible store, save its
old logical index and mark that current generation as replaceable. Compose over that
saved index inside the cell while its replacement mask is valid. Same-primitive,
same-index duplicate stores can transfer ownership to the new generation and retag that
cell's existing fragments; preserve the first saved-under value. This coalescing is
explicit primitive ownership, not a guess based on equal colors.

On a competing primitive/index in the same cell, discard its replacement mask and use
the current canonical cell; do not nest saved-under values. On an ordinary write, clear
the mask and use the new canonical index. If invalidation leaves a replaced cell without
any valid self-owned geometry, restore canonical presentation for the whole cell. Downgrade
that cell's retained fragments too, so discarded geometry cannot later uncover a saved
old color. Never copy saved-under bytes into `dz_video.vram` or return them from reads.
The initial compositor may disable replacement masks as a debugging overlay mode, but
that additive view is not the finished Hybrid silhouette.

On every write, mark the destination cell and the bounded supplying-owner halo dirty.
Before a present, validate those tiles, clear invalid top fragments, update replacement
masks, then upload resolved **index + coverage** (RG8) and the logical saved-under/mask
texture. Validity changes also dirty neighboring halo tiles. Uninstrumented writes must
retire the affected page **before** executing; a present-time byte diff cannot detect a
write that restores the same byte. The exact old/new write sites in this tree are
DVDT `store_pixel`, `clear_vram_words`, `ega_write_planar_row`, video mode memset, PCX
destination writes and logo decoding. This seam covers those too, not just image plots.
If the affected address set cannot be bounded safely (including wrapped aliases), retire
the entire aperture rather than assuming the nominal page contains every store.

| Logical operation | Required Hybrid behavior / Classic fallback |
|---|---|
| Ordinary point, bitmap, scan fill, plot above/below existing stroke | Successful store advances ownership and clears destination replacement. Earlier dots remain below a later eligible stroke only where its committed coverage reaches; later dots occlude all older coverage in their entire cell. No reordering by image type. |
| Clear/erase, including color zero and alternating fill bytes | Invalidate every actually written source/target cell. Bulk clears use the exact byte/lane/address extent; no assumed screen-sized rectangle or magic meaning for index zero. |
| Pixel read | Read canonical VRAM only, with original charge and address rules. No GPU readback, coverage or saved-under color in the simulation. |
| EGA XOR | Retire affected page before entering an XOR operation and suppress emission until replace mode resumes. Execute original read/modify/write and duplicate symmetry stores. XOR of antialias coverage is undefined here; do not approximate it. Turning XOR off does not restore retired geometry. |
| Page copy | No generic page-copy routine is currently used by these images. Future prototype default: invalidate destination and display copied Classic pixels. Preserve source if unchanged. Later, only an exact indexed address permutation may copy fragments, saved-under and ownership together, translating tokens/sequences to new destination stores. Respect overlap order and partial copies. |
| Quilt / pixel swaps | Prototype: retire affected active page before the first read/write; run Classic. A later adapter must mirror every ordered reflected swap, including overlap and partial interruption; moving the layer once per requested rectangle is insufficient. |
| tetrYs / sample-dependent reconstruction | Retire active page before coarsening. Resulting sampled-color blocks are raster art, not transformed old vectors. |
| Display start, pan, line compare | No VRAM movement. Sample the same VRAM layer/base addresses using the presenter's exact mapping below. Keep hidden-page geometry. |
| Memory scroll / unsupported block operation | Retire destination or whole affected page before mutation; Classic. Do not confuse this with display-start scrolling. |
| Palette cycle / palette fade / EGA attribute change | Keep indices, coverage and ownership. Recompose using current hardware palette; a black palette or `display_cleared` flag does not invalidate pixels. |
| Image change / Auto overlays | Break current primitive but keep existing page geometry unless the actual program clears it. Later writes retire it normally; clearing all geometry on every selection would destroy overlay history. |
| Mode or backend switch, text/menu/logo entry | Retire layer, masks and pending intent even on a preserve-memory mode switch; rebuild mapping. Canonical memory/palette semantics remain the original ones. Text and bitmap logo output use Classic. |
| Resize / DPI / Original-Crop-Stretch change | Reallocate raster density and retire old geometry/masks; show canonical pixels until new strokes arrive. No simulation restart, extra draws, RNG or virtual work. Retained-vector replay is outside the prototype. |
| Allocation, shader/device or ownership failure | Drop the layer and masks, use the existing Classic indexed presenter (or its RGBA reference fallback). No change to simulation options or image/fade selection. |

### Addressing and memory at 4K

The layer covers the **full active VRAM aperture**, not only visible rows or two named
pages. Page 2000 is row 400 in unchained VGA; EGA page 1750 is row 350. Aperture sizes
are 262144 VGA400, 65536 VGA200, and 524288 EGA indexed pixels. The last row is partial.
Pack raster tiles by VRAM row, with the final row masked to its valid cells. Keep a
shared mapping helper with the presenter; do not independently round page origins.

For physical display row `y`, exactly as `dz_video_rgba`:

```text
row = y > line_compare ? y - line_compare - 1 : display_start_row + y
address = (row * logical_width + x) % aperture_pixels
```

Map the output subpixel's fractional position **within that logical cell** into its
raster tile. Both the Classic lookup and coverage use the same address, including at
the last partial row and the split's strict `>` boundary. Clip between disjoint display
strips; never join strokes across the split or aperture wrap. There is currently no
horizontal PEL pan; a future subpixel pan requires a shared extension, not an assumption.

Use raster densities `sx = content_width / logical_width`, `sy = content_height /
physical_height`, with fractional cell edges shared by all tiles. The following are
conservative full-aperture allocations at 3840x2160 output (ceil partial rows). CPU owner
+ coverage costs 5 bytes/sample, GPU resolved RG8 costs 2, full staging costs 2. Add about
6 MiB ownership metadata, 1 MiB mask/underlay GPU storage and up to 32 MiB CPU reference
output. SDL/driver swapchain allocations are additional, backend-dependent memory.

| Policy / largest mode | Raster atlas samples | Atlas CPU + GPU + staging | Including reference output/metadata |
|---|---:|---:|---:|
| Original, EGA350 (content 2880x2160) | 14.58 M | 125 MiB | about 164 MiB |
| Stretch, EGA350 (3840x2160) | 19.43 M | 167 MiB | about 206 MiB |
| Crop, EGA350 (full content 3840x2880) | 25.91 M | 222 MiB | about 261 MiB |
| Original, VGA400 | 12.75 M | 109 MiB | about 148 MiB |
| Original, VGA200 | 6.38 M | 55 MiB | about 94 MiB |

Allocate tiles lazily and upload only dirty rectangles. Cap application-owned Hybrid
storage at **320 MiB** for the prototype; if density/aperture/backend constraints exceed
the cap, use Classic. Do not downsample Classic to pay for the layer. A single top layer
keeps this bound independent of how long the screensaver runs. CPU reference rendering
is correctness scaffolding; no 4K throughput claim is made.

## Palette compositor and SDL 3.4 implementation path

Reuse the presenter's original-index and hardware-palette textures. Hybrid adds the
validated VRAM RG8 overlay and logical saved-under/mask texture. In the fragment shader,
compute the existing viewport/display-address mapping, nearest-fetch the canonical
index, choose the saved underlying index only for a valid replacement mask, then fetch
coverage at raster density. Convert both indices through the **current** palette:

```text
base = palette[replacement_mask ? saved_under_index : logical_index]
ink  = palette[stroke_index]
out8[channel] = (ink8[channel] * coverage8
                + base8[channel] * (255 - coverage8) + 127) / 255
```

Use the same VGA `(dac & 63) * 255 / 63` integer conversion and EGA attribute/rgbRGB
conversion as Classic. Explicit encoded-RGB arithmetic is the initial reference rule;
do not silently introduce sRGB target conversion or linear-light blending. Coverage is
antialiasing of the stroke only. Indices use point loads/nearest sampling and integer
decode (`round(sample.r * 255)` for UNORM); they are never linearly filtered. Return
opaque output with renderer blending disabled for the final composition quad.

With the SDL **GPU renderer**, get its device with `SDL_GetGPURendererDevice`, create the
fragment shader with `SDL_CreateGPUShader`, and attach it through
`SDL_CreateGPURenderState`. Set uniforms with `SDL_SetGPURenderStateFragmentUniforms`,
draw the existing viewport quad, then reset custom state to NULL before other UI draws.
These are 3.4 APIs; an ordinary SDL D3D11 renderer does not acquire custom shader support
merely because it runs on a GPU. Keep the presenter's existing renderer/device ownership;
if it exposes no GPU render-state seam, use the CPU reference compositor and upload its
output through that presenter. [SDL GPU device access](https://wiki.libsdl.org/SDL3/SDL_GetGPURendererDevice),
[custom render state](https://wiki.libsdl.org/SDL3/SDL_CreateGPURenderState).

Use the index texture as the renderer's primary texture (fragment sampler slot 0), and
additional bindings for palette, overlay and replacement mask/underlay (slots 1..3;
shader sampler count includes the primary). Share the presenter's upload command buffer
or flush/synchronize before mixing direct GPU uploads with queued renderer draws. Do
not sample textures while their transfer writes are incomplete. Use SDL's shader ABI:
fragment input `COLOR0` plus `TEXCOORD0`, output `SV_Target`; HLSL sampled resources in
space2 and uniforms in space3. SPIR-V uses sets 2 and 3 respectively, consecutive
bindings. Check actual formats/device limits rather than assuming integer texture
support; normalized RG8/RGBA8 plus explicit decode is sufficient.
[SDL shader bindings](https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader),
[SDL 3.4 render-effect example](https://github.com/libsdl-org/SDL/blob/release-3.4.0/test/testgpurender_effects.c).

| Windows backend / tooling | Evidence and action |
|---|---|
| SDL GPU D3D12 | DXBC **SM5.1** or DXIL **SM6.0**; `ps_5_0` D3D11 bytecode is not the SDL GPU contract. Runtime `D3DCompile` from `d3dcompiler_47.dll` can supply DXBC `ps_5_1`; the small space2/space3 probe here passed. Prefer packaging generated bytecode with readable HLSL source. [SDL formats](https://wiki.libsdl.org/SDL3/SDL_GPUShaderFormat), [Microsoft D3DCompile](https://learn.microsoft.com/en-us/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dcompile). |
| SDL GPU Vulkan | SPIR-V needed; DXBC cannot be handed to Vulkan. Supply a precompiled artifact from glslang/glslc or shadercross/DXC; otherwise retain presenter's supported Classic path or CPU Hybrid compositor. |
| Local offline inventory | `C:/tools/CATALOG.md` has no shader toolchain entry; SDL3 3.4.16 headers/libs are locally present. MinGW GCC 12.2, d3dcompiler headers/import libraries, System32 and SysWOW64 `d3dcompiler_47.dll`, and SDK `fxc.exe` in Windows Kits 10 versions 22621/26100 are present. Both SDK fxc and runtime D3DCompile compiled the same `ps_5_1` probe successfully. This does **not** verify GPU device creation or drawing. |
| MSYS2 | Installed package manifests contain Vulkan headers/loader but no glslang, shaderc, SPIRV-Cross, DXC or SDL_shadercross; selected bin directories/PATH also have no compilers. MSYS pacman itself failed to create its signal pipe under this restricted token. Upstream packages [glslang](https://packages.msys2.org/packages/mingw-w64-x86_64-glslang), [shaderc/glslc](https://packages.msys2.org/packages/mingw-w64-x86_64-shaderc) are available but were not installed. No offline SPIR-V build was established. |
| SDL_shadercross | Not installed locally. It can translate HLSL/SPIR-V and has an offline CLI, but requires the corresponding dependencies: SPIRV-Cross, DXC for DXIL, or Windows d3dcompiler_47 for DXBC. It is a future build tool choice, not an SDL3 core dependency already supplied here. [Upstream README](https://github.com/libsdl-org/SDL_shadercross). |

Frame order: finish original writes up to the current yield; resolve dirty geometry and
mask tiles; upload indices/layer/masks and current palette; apply viewport and display
address uniforms; compose; present. Geometry/indices/masks change on writes. Palette
uploads require recomposition even with no new pixels. Pan/split/viewport changes require
recomposition, not simulation. Freeze with none of these changes can reuse the previous
composition. A whole RGB screenshot can never substitute for indexed retained geometry.

The CPU reference implements identical address mapping, point sampling, validity checks,
coverage quantization, palette conversion and integer blend equation into output-sized
RGBA. Share pure mapping/palette helpers with the GPU presenter's reference where
possible. Keep `dz_video_rgba()` and canonical snapshot serialization unchanged. Hybrid
visual captures use a separate output path; metadata must state output size and policy.

## Verification before enabling a variant

| Check | Required evidence |
|---|---|
| Logical identity | Run existing `test_dzle`, `test_dzl2`, `test_dzfd`, `test_ref_images`, `test_ref_fades`, `test_ref_primitives`, `test_ref_rng`, palette tests and whole-program acceptance/integration with observer off/on. No re-recording. Compare full VRAM, full palette/attributes, RNG state **and** call count, charged work, virtual ns/ticks, draw state, display start and split at fixed yields. A framebuffer hash alone cannot prove RNG/timing identity. |
| Progressive geometry | Fixed seeds cover cycloids per-curve/per-point colors, sine rebuild/even-EGA rounding, sweep2 both endpoint modes and every symmetry. Pause at inner-loop yields; no future segment appears. Verify caps at reset/clip, separate reflection branches, no bridge in lotsadots or stamps. |
| Footprint/aspect | Horizontal, vertical, diagonal and ellipse quadrants in VGA400/VGA200/EGA350/EGA700; 1:1 and fractional/4K viewport scales, Original/Crop/Stretch. The same original pixel footprint maps through the same transform as Classic. |
| Ownership/erasure | Stroke over dot, dot over stroke, crossing strokes, same-color ordinary overwrite, duplicate owned stores, partial zero clear, alternating background, XOR twice, interrupted Quilt/tetrYs, image overlays, page copy overlap, sequence/generation retirement. Check no geometry resurrection and no stale saved-under color. |
| VRAM readout | Both page bases, display-start motion, line_compare boundary and off state, first row below split, aperture wrap/partial last row, hidden-page clears. Compare CPU and GPU output with only palette/origin changing. |
| Visual comparison | Classic and Hybrid captures with identical seed, key script and **virtual time** (not frame number or wall time). Canonical snapshots must match byte-for-byte; image differences must lie in declared geometry/support cells. Compare GPU/CPU Hybrid RGBA (exact integer compositor; documented tolerance only if a later coverage implementation changes). |
| Operational fit | Resize, DPI, preview child, fullscreen/multi-monitor lifecycle, shader failure, device loss and allocation cap. No extra windows/event loop; all simulation results stay independent of presentation backend and speed. |

The implementation status records the prototype evidence against this table.
Extend unsupported operations only with their own invariants and cases.

## Extended widescreen: adapt composition per family

Keep the original canvas centered and immutable. Expose a **separate indexed composition
canvas** to presentation, at the same logical pixel density/aspect, with an integer
central origin and extra columns on each side. For 16:9, a 320-wide 4:3 image needs about
426.67 composition columns at the same display height: choose 427 and define asymmetric
53/54-column margins with one shared subpixel viewport transform. EGA similarly needs
about 853.33 columns (choose 854). Preserve the exact central source rectangle; rounding
belongs to the composition transform, never W/H/S or original VRAM stride.

The first adapters should use **additional motif instances**, generated in side contexts
with copied parameters or a private deterministic RNG derived from (master seed, image
selection ordinal, family, side). Do not consume `dz_rand`, modify its state, run the
original function again against globals, or call original charging/palette/input hooks.
Side progress follows recorded virtual work/sample ordinals from the central algorithm;
frame rate and number of monitors cannot change either simulation. Keep side instance
state separate from the original globals, caches, plot callbacks and continuation gates.

| Family | Adaptation / first candidates | Limits and fallback |
|---|---|---|
| Continuous centered motifs | **sine**, then **cycloids**: place additional same sized oscillators/rolling curves in side regions, with explicit clipped domains and independent phase/context. Share palette and local aspect; do not increase X amplitude or angular detail. | Prototype side support is independent; no invented bridge through central raster. Arches/cones/yarnball later replicate whole motifs. |
| Lines/fans | **sweep2** third: a dedicated side-ray context with new horizontal endpoint range/centers and original height. Boxes/puffs/littleStars can later distribute unchanged motif centers across width. | Preserve per-ray timing/density in private contexts; wider bounce/endpoint math belongs to each adapter, not global S. |
| Point sets/bitmap art | Lotsadots can populate side fields at the same points-per-area density; balls/blister use extra original-scale stamps; Sierpinski uses complete triangles. | Remain pixelated. Extra samples/private RNG are Extended-only output. No inferred lines or extra canonical draws. |
| Tiled/procedural raster | Grid extends its 8x8 mask lattice; diag1/diag2/ribbons/tiles need explicit side field/cadence rules. Formula1 may repeat the original signed-word domain. | Word overflow is part of the texture; blindly evaluating an unbounded wider formula changes it. Classic fallback until a seam and index rule are specified. |
| Filled/polygon motifs | Extra butterflies/diamonds/gumballs/wedges at unchanged size; fresh per-context polygon spans. | No global enlargement of [350]/[700] caches; wedges' timer/caller gate is not shared with side simulations. Fallback initially. |
| Recursive / framebuffer dependent | Hilbert instances can keep depth/stamp size. Plasma, Quilt and tetrYs need independent side readback surfaces and explicit operation mappings. | Classic fallback for the whole composition until implemented; stretching, mirroring the last column, or executing canonical pixels twice is not adaptation. |

The engine capability descriptor needs image/family ID, composition bounds and central
rectangle, side-state reset/step hooks, explicit primitive kind, shared palette epoch,
VRAM page/row mapping, write/invalidate/copy hooks and a supported-fade mask. The last
mask controls **presentation fallback only**; it must not suppress original fade selection,
alter random draws or alter timing. Extended can coexist with Classic or Hybrid ink.

Palette-only Juggle/background fades apply over the whole extended canvas. A viewport
pan/split can remain Extended only if each side has the same page lifecycle, physical
row origin and split readout as the center. Clear/page-copy support must copy or retire
the corresponding side indexed cells and geometry together. Unsupported pixel-moving,
sampling, XOR or spatial-wipe fades switch the **whole composition** to centered Original
Classic before entry, discard side state, and stay there until the next supported image
boundary. Return at a boundary, not midway through a fade. A short presentation-only
transition may hide the width change later; it must not call simulation delay or palette
fades. Never leave colorful side panels standing while a central clearing fade runs.

Do not offer a general wider original simulation canvas in the first phase. An eventual
Enhanced/Extended algorithm may own one after its array sizes, arithmetic, readback and
fades have been separately adapted. The central original simulation remains available
as the verification reference throughout.

## Evidence and unverified work

- Source audit covers 36 Alt + 6 Ctrl + 24 fade records and actual primitive/dispatch
  implementations. A mechanical inventory check is recorded under `build/workers/s-design/`.
- Offline shader probe: Windows SDK 10.0.26100 x64 `fxc /T ps_5_1 /E main` on
  `probe.hlsl` succeeded; direct System32 `D3DCompile` returned HRESULT 0 and a code blob
  for the same source. Space2 textures/samplers and space3 uniforms were exercised.
- The unchanged `tools/native.py test` was run in an isolated source copy under
  `build/workers/s-design/validation/` so reference tests could not write another worker's
  files. Build succeeded; 23/31 tests passed there. Eight CLI cases initially failed
  because nested scratch cwd paths exceeded the original startup's **80-byte directory
  buffer** (DZLE:2048). `recheck.py` ran those same unmodified acceptance, controls and
  six integration scripts with shorter scratch paths: **8/8 passed**, with original
  expectation JSON unchanged. Logs: `native-test.log`, `native-recheck.log`. This is a
  path-bound test setup result, not a renderer regression or a source fix.
- At the design stage, no GPU rendering/device/shader-binding smoke test, Hybrid
  screenshot, performance measurement or Extended simulation was performed. The
  implementation status above records the subsequent CPU prototype work. The original
  design audit changed no source, tests, matching evidence or distribution files.
