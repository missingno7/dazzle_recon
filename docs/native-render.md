# Native Classic rendering

Milestone 2 and the Classic presentation part of milestone 4, 2026-10-29.
The DOS matching sources and the native application algorithms are unchanged.

## Three spaces

**Simulation space** is the original logical coordinate system and indexed
`dz_video` memory, DAC/attribute registers, display start, line-compare split,
page wrap and virtual timing. The presenter reads it without changing it. RNG,
plotting, palette cycling and work accounting do not depend on the output size.

**Rasterization space** is where display pixels are produced. Classic resolves
the original scanout to 320x400, 320x200 or 640x350 indices; text rasterizes to
720x400 RGBA. It does not render a larger simulation framebuffer when the window
grows. The Screen canvas produces uniform output-pixel indices, with Faithful blocks and
optional Native sine strokes sharing the same raster; see [native-canvas.md](native-canvas.md).

**Presentation space** is SDL's current render output in physical pixels
(`SDL_GetCurrentRenderOutputSize`), including an offscreen target when testing.
This avoids assuming that window coordinates equal output pixels on a HiDPI
display. There is no SDL logical presentation or fixed 640x480 destination.
Every mode has **4:3 display aspect**, including VGA 320x400 and text 720x400:
their source pixels are not square.

`dz_present_viewport(width, height, policy)` is the single pure transform. It
scales a 4x3 display rectangle, then centers it. ORIGINAL fits it inside the
output and clears the surrounding bars black. CROP covers the output and clips
the excess. STRETCH fills the output without preserving aspect. Fractional
boundaries are retained, including at tiny and odd output sizes. Nearest sampling
preserves palette entries; fractional scaling can give adjacent logical pixels
different physical widths, as expected.

## Chosen SDL path

`video.c` retains the unchanged `dz_video_rgba()` reference and simulated video
state. `present.c` owns scanout, textures, caching and the window presentation
lifecycle. Its small borrowed-renderer interface also allows the same production
presenter to render into test targets.

Classic uses `SDL_CreateRenderer(window, NULL)`. On this machine it selects
Direct3D11, advertises native INDEX8 support and passes the equality suite.
The index texture is streaming `SDL_PIXELFORMAT_INDEX8`, nearest sampled, with
an attached `SDL_Palette`. SDL 3.4 introduced
[texture palettes](https://wiki.libsdl.org/SDL3/SDL_SetTexturePalette).
The [3.4.16 D3D11 implementation](https://github.com/libsdl-org/SDL/blob/release-3.4.16/src/render/direct3d11/SDL_render_d3d11.c)
uses R8_UNORM plus a palette shader internally. Application code therefore only
uploads the resolved one-byte indices and updates the 256-entry palette.

Scanout copies rows with `y > line_compare` restarting at row zero, otherwise
`display_start_row + y`, exactly like the reference. Each row can wrap in its
middle at `65536 * pixels_per_byte`. VGA colors use `(component & 63) * 255 / 63`
integer truncation, not bit replication. EGA uses attribute-indexed rgbRGB bits
with 170/85 contributions, including the reference's `index & 15` behavior.

AUTO probes native advertised formats and palette/texture creation, choosing
INDEXED when possible and RGBA on initialization failure. A later failure to
create a mode-sized INDEX8 texture also falls back under AUTO. Explicit INDEXED
reports initialization failure instead of silently selecting RGBA. The
[SDL renderer implementation](https://github.com/libsdl-org/SDL/blob/release-3.4.16/src/render/SDL_render.c)
can emulate unsupported INDEX8 textures using a CPU palette surface: this is
accepted only for the software test renderer, not as a hidden hardware fallback.
RGBA uses the unchanged CPU converter and the same viewport and nearest sampling.
Developer text uses RGBA with cached characters, attributes, cursor and blink
phase. Saver/preview text is cleared black without rasterizing or uploading text.

Alternatives evaluated:

| Choice | Evidence and decision |
|---|---|
| Default `SDL_CreateRenderer` | D3D11 on this host; native palette lookup and all equality cases pass. Chosen: no device ownership, shader assets or extra dependencies. |
| `SDL_CreateGPURenderer(NULL, window)` | The hidden-window test calls this API directly; selects GPU/D3D12 and passes all cases. Useful for Hybrid, unnecessary for Classic. |
| Custom palette fragment shader with SDL 3.4 GPU render states | [Supported by the GPU renderer](https://wiki.libsdl.org/SDL3/SDL_SetGPURenderState); would duplicate SDL's built-in Classic palette shader and require compiled shader formats. Deferred. |
| Explicit R8 index + palette texture | Already what SDL's native INDEX8 implementation provides. Owning these through SDL_GPU would add transfer buffers, samplers, resource lifetime and a shader without improving Classic fidelity. Deferred. |

Cached Classic visible rows are compared with `memcmp`; only changed rows are copied
into the contiguous index buffer. Changed frames upload that small, fixed-size
texture. Variable-height partial uploads did not establish a consistent CPU win
and were removed.
Palette-only changes do not upload indices.
Unchanged scanout, effective palette, mode, display registers, output size and
policy skip texture upload, drawing and `SDL_RenderPresent`. Expose/resize and
other window events invalidate presentation through one call in `input.c`;
redrawing reuses the texture. Screenshots force drawing before readback because
the backbuffer is not retained after present. The existing clock still invokes
presentation at the monitor rate in real time. No new loop, delay or vsync pacing
was added; virtual presentation counters keep their old meaning even on skips.

## Canvas seam

The former Hybrid seam is superseded by [native-canvas.md](native-canvas.md).
Both Faithful backends use the same canvas API. Classic retains the scanout and
nearest texture scaling described above. Screen keeps the entire physical aperture
in one persistent INDEX8 texture; dirty rectangles upload directly from its CPU
surface. Display start, split and ring wrap select one to three source rectangles
drawn at 1:1, without a visible-row gather. Faithful blocks use the original
policy rectangle and the renderer's probed nearest rules. Native sine uses the
whole output, preserving the mode's Original physical unit aspect and extending
its horizontal amplitude range with output width. Faithful-only bars remain renderer black,
independent of palette entry zero. Once Native ink exists, that full page is indexed
content, including its background. Crisp uses the existing GPU INDEX8/palette path.
The opt-in AA experiment uses CPU ARGB resolution of foreground index, underlying
index and coverage after lookup; even `--renderer indexed` uses this explicit AA
reference path. Ordinary crisp palette-only changes still upload no index data.
The historical shader exploration remains in [native-hybrid.md](native-hybrid.md).

Screen prefers SDL's GPU renderer, honoring explicit backend hints and falling
back to the normal renderer on creation failure. It coalesces sparse changes in
spatial cells; the GPU path bounds transfer calls by merging dense regions into
one rectangle. The D3D11 fallback uses a stable page-sized transfer for dense
changes. Palette-only and register-only frames upload no indices.
The current measurements, rejected upload experiments and explicit Faithful pan/
split rounding change are in [native-canvas.md, VGA-model presentation](native-canvas.md#vga-model-presentation-s-vram-2026-11-04).

Final 20-second samples on logical CPU 0 (percent of one core, 1080p/4K): Classic
3.05/4.69, Faithful Screen 5.08/3.36, Native sine 8.28/12.03, static Screen
0.16/0.39 with zero uploads. The performance target is only partly met: Faithful
is 1.67x Classic at 1080p and below Classic at 4K; Native remains higher. The
canvas doc records before/after, headless controls, raster-only costs and the
substantial host/driver timing spread. No Classic speedup is claimed.

## Verification

`python -B -I tools/native.py test`: **32/32 passed**. Integration and acceptance
hash files were not changed. The dummy-driver `test_present` uses SDL's software
renderer and offscreen ARGB targets, comparing every output pixel and alpha from
the production INDEXED and RGBA presenters: 216 comparisons, zero differences.
It also checks the pure viewport, unchanged frames, hidden VRAM writes, palette
changes, start/split changes, forced exposes and unforced output-size changes.
Text checks cover static non-blinking content, blinking attributes and the cursor.
CLI smoke checks additionally compared complete BMP files from AUTO/INDEXED/RGBA
for all nine graphics-mode/policy combinations at 700x300: equal. Fullscreen was
accepted and invalid renderer, presentation and window arguments returned 2.

Cases: VGA400 with start 817 and split 187 (including mid-row page wrap), a
VGA palette-only change, VGA200 with wrap, EGA350 with wrap and an attribute
palette change, text, saver text black and preview text black. Each uses all
three policies at 640x480, 1920x1080, 2560x1080, 1280x1024, 700x300, 1x1, 1x2,
2x1 and 641x479. The hardware suite also simulates absence of advertised INDEX8
support and verifies explicit INDEXED rejection plus successful AUTO/RGBA drawing.

Opt-in: `python -B -I native/tests/verify_present_backends.py` (hidden windows).
The script reports unavailable drivers, fails on differences/timeouts and kills
timed-out children. It accepts an executable followed by selected backend names.
SDL version: 3.4.16 MinGW x64, GCC 12.2, RelWithDebInfo.

| Renderer requested | Actual backend | Indexed vs RGBA |
|---|---|---|
| software / dummy (CTest) | software | 216/216 equal |
| default | direct3d11 | 216/216 equal |
| direct3d11 | direct3d11 | 216/216 equal |
| direct3d12 | direct3d12 | 216/216 equal |
| gpu (`SDL_CreateGPURenderer`) | gpu / direct3d12 | 216/216 equal |
| opengl | opengl | 216/216 equal |
| vulkan | vulkan | 216/216 equal; completed with the 180 s script limit |

No nearest-sampling or palette differences were observed on completed backends.
Vulkan's readback suite is much slower on this host: initial 60 s limits expired
while it was still progressing, so the opt-in script allows 180 s per backend.
The optional legacy `direct3d` backend was unavailable (`GetDeviceCaps(): NOTAVAILABLE`).
Actual per-monitor HiDPI changes and a multi-monitor/screensaver host were not
exercised in this worktree; output-pixel sizing and saver/preview text policy
are exercised independently of those hosts.

## CPU measurement

`measure_present.py` uses Windows `GetProcessTimes` (kernel + user seconds),
divided by elapsed wall time, expressed as **percent of one core**. Each process
runs for 20 real seconds, with the existing sleep-based clock and default 960x720
window/renderer. Before binaries were copied/linked from the untouched starting
tree; after binaries use AUTO/INDEXED. The static harness fills VGA400 with
`vram[i] = (uint8_t)i`, then calls `dz_delay_ms(20000)`; the same after harness is
available as `test_present.exe --static`.

| Workload | Before wall / CPU s | Before one core | After wall / CPU s | After one core |
|---|---:|---:|---:|---:|
| `--slice sierpinski --seed 1 --exit-after 20` | 20.095 / 5.031 | 25.04% | 20.054 / 0.828 | 4.13% |
| Static indexed image | 20.085 / 5.125 | 25.52% | 20.079 / 0.391 | 1.95% |

These are single-run host measurements, not a throughput guarantee; the earlier
reported ~14% baseline was not reproduced in this session. Before/after animated
receipts agree: steps 1137068, ticks 364, RNG calls 1137085, framebuffer `6abc9dc5`,
palette `9fc4ba0e`. Performance work has not changed the virtual run.


## Extended widescreen

Milestone 4 first adapters, 2026-11-03. Select the developer policy with
`--present extended` and the default `--canvas classic`. `DZ_PRESENT_EXTENDED`
appends to the existing enum, preserving the previous option values. All other
policies retain their existing paths. Extended with `--canvas screen` uses the
Original 4:3 viewport; the side adapters currently require Classic. Text also
uses Original geometry and its existing saver/preview black-screen behavior.

The center uses precisely the existing Classic texture, draw call and Original
viewport at the same output size. It is never resampled through a wider texture.
`extended.c/.h` owns separate indexed side apertures and side textures; the small
hooks in `present.c` prepare them, include their dirtiness, and draw them before
the unchanged center. The Screen canvas uses the separate Original fallback.
Both index and RGBA renderer choices are supported. Sides resolve the
same VGA DAC / EGA attributes as the center, so palette cycling, Juggle palette,
DAC-to-black and palette-to-background fades recolor all three regions together.

`dz_extended_layout` extends the Original rectangle horizontally by at most half
its height on either side: total display aspect is capped at **21:9**. The logical
X and Y densities remain those of the current original mode. At 1920x1080 the
center is 1440x1080 and each side is 240 pixels (53 1/3 VGA columns). At
2560x1080 the sides are 540 pixels each (120 VGA / 240 EGA columns), with a
20-pixel black strip at each outside edge. A 4:3 or portrait output gains no
columns. The visible side source edge can be fractional; it crops the side
canvas without changing its pixel density or the central origin. Sine fits its
entire figure inside the fully visible cells before any presentation cropping.
Unlike a single rounded 427-column texture, independent sides can keep the center's exact
Original scale and its exact SDL nearest-sampling behavior simultaneously.

Two adapters were selected from the design's first candidates:

| Image | Composition and cadence | Reason / limits |
|---|---|---|
| **lotsadots** (Alt+A, image 20) | Each central generator call contributes `side_columns * physical_height / extent^2` private points per side, doubled for diagonal symmetry, with a retained fractional budget. Points are uniform in the side field. The copied color rule evaluates original signed-word formulas at the continued global coordinates, or uses the private RNG / supplying central color for random / temporal rules. | Tests point density, independent randomness and a spatial color field across both seams. Samples stay rectangular Classic pixels. The density is the original symmetric field's expected density, with independently distributed hits. |
| **sine** (Alt+K, image 27) | One complete oscillator per side, centered in the fully visible logical side cells. Draw initial amplitudes from the original range; apply one constant scale to both axes, bounded by the full modulation envelope including swapped reflection branches. Each central generator call advances both once, with the original 0.01 phase step and modulation rates. EGA700 retains even-coordinate/parity handling, with an extra two software units reserved for rounding. | Keep a four-VGA-column / eight-EGA-column margin, plus the corresponding vertical margin. Pixel density is unchanged; only private motif amplitudes may shrink uniformly. Sides narrower than **15% of the original logical width** (48 VGA / 96 EGA columns) use plain Original for this image. This includes 16:10. No clipped motifs, interpolation or extra canonical plots. |

Side RNG uses an integer mixer/counter seeded from `(master seed, image selection
ordinal, image ID, side)`. It never calls `dz_rand`, charges work, advances the
clock, polls input or writes application globals. The two marked emitter calls
observe completed central generator steps, including the selection's initial
step. Lotsadots still generates a fixed maximum domain of 120 VGA / 240 EGA
columns per side even when the window shows fewer columns. Sine uses the visible
side width known at the image boundary, with the fractional outside cell excluded
from its fit. Its phase and sample cadence depend only on central calls; redraws
and exposes do not advance it. A resize clears side ink and resets the private
contexts using the new output dimensions. A resize too narrow for sine retires it
for the rest of that image; widening cannot restore it until a supported image
boundary. Before an output exists, the requested developer window dimensions
(or the normal 960x720 default) supply the fit.

The sine fit covers the entire evolving figure, rather than just its initial
ellipse. For initial amplitudes `A,B` and steps `sx=1/(1+A), sy=1/(1+B)`, the
private accumulated modulations are evaluated as bounded trigonometric sums:
`A + (cos(sx/2)-cos(mx+sx/2))/(2*sin(sx/2))` and
`B + (sin(my+sy/2)-sin(sy/2))/(2*sin(sy/2))`. Wrapped double phases avoid the
long-run drift/freeze of repeatedly accumulating float phases and amplitudes.
Their maximum absolute envelopes determine one fixed per-oscillator scale, no
larger than one, against both available radii. Both reflection axes use the larger
envelope, so enabling diagonal reflection cannot clip a previously fitting motif.
This preserves each curve's proportions throughout its motion without separately
clamping X/Y or flattening its extremes. All of this state is private; the original
central float arithmetic and canonical simulation are unchanged.

Sides use at most 1 MiB for two sparse indexed apertures, plus SDL side textures (at most 168000 indexed pixels / 672000 RGBA bytes). No output-sized
side CPU canvas or retained operation history is allocated.

Lifecycle and supported fades:

- Every actual image selection/rebuild increments the selection ordinal and
  resets the private contexts. Supported overlays retain existing indexed page
  ink just as the center does. First entry or recovery from fallback starts with
  the current mode's background clear pattern. Unsupported images retire both
  sides and use plain Original for the entire composition.
- **Juggle palette (`j`), DAC-to-black and palette-to-background** are supported
  because they change only shared palette registers. Ordinary cycling needs no
  pixel operation. The mode clear following a background fade clears both side
  apertures before the original mode setup, preserving the zeroed unused VGA
  aperture tail; alternating clear-byte lanes continue
  across the left and right edges with their correct phase. Clear never revives
  a retired adapter.
- **Pan/scroll and line-compare split** use the same physical row, page paragraph,
  `y > line_compare` restart rule, and `65536 * pixels_per_byte` address wrap as
  the central scanout, including a partial final aperture row. Each sparse side
  aperture uses the original stride, with distance from its central edge in place
  of central X. Supported overlays preserve visible and hidden page ink. There
  is no page-copy operation in the supported adapters; sampling/copy fades retire.
- All other dispatched fades (`a b c d e f g i k l m o q r s t u v w x y z`)
  retire before their first operation. This includes spatial wipes even when
  they could be described as clearing: no partial-wipe side mapping is supplied.
  Actual EGA XOR, rectangle swaps, logo entry and text/backend changes also retire.
  Fallback stays Original until the next supported image boundary. Palette fades,
  clears and completion of an unsupported fade cannot reenable it midway.
- Actual output size changes reset side ink/contexts; graphics mode changes retire
  it. Application mode-clear, menu, logo and backend boundaries have explicit
  guarded hooks. Original memory, palettes, readout registers and timing remain
  authoritative and untouched by all those observations.

Verification and artifacts:

- `python -B -I tools/native.py test`: **37/37 passed**, including `test_extended`.
  `integration-hashes.json` and `acceptance-reference.json` are unchanged.
- `test_extended` covers 16:9, 16:10, 21:9, 4:3, portrait and over-cap layout;
  exact central output with nonblack palette fixtures in every physical mode,
  indexed and RGBA, at eight output sizes including fractional boundaries;
  untouched canonical video/RNG/time during emitters; redraw independence and
  seed/ordinal repeatability; clear-byte phase, palette blackening, all fade
  guards and sticky retirement, supported overlays, unsupported image, resize,
  mode/XOR/text lifecycle, Hybrid Original fallback, hidden page, split and wrap.
  New containment tests accumulate **20000 samples** with five seeds, both
  reflection settings, both wide capture aspects and all four graphics modes,
  then check every side ink pixel against the complete horizontal/vertical
  margins and center symmetry. They also verify exact Original fallback at
  16:10, sticky retirement on a narrow resize, canonical state identity during
  sine emission, reset/replay identity with an extra expose midway, and recovery
  only at the next image boundary.
- `python -B -I native/tests/verify_extended.py build/native/dazzle.exe build/workers/s-wide/state`:
  **48 full-state comparisons equal**. Both adapters progress at five distinct
  virtual deadlines in VGA400, VGA200, EGA350 and EGA700 (40 comparisons), plus
  eight deadlines in a 60-second Auto run containing Gas cloud and Linewipe.
  Receipts compare the complete 524288-byte VRAM, both palettes, RNG state and
  call count, work units, virtual ns/ticks, drawing state, backend/page and display
  registers. These are actual wide dummy-renderer runs, not headless bypasses.
- `python -B -I native/tests/capture_extended.py build/native/dazzle.exe build/workers/s-wide/captures`:
  both adapters captured at **1920x1080 and 2560x1080**, seed 1, sine at 8.15 and
  lotsadots at 8.3 virtual seconds. Original/Extended PNGs, adjacent pairs (Original
  left), full-state receipts and `captures.json` are under that directory. The
  script asserts exact center pixels, complete state equality and the 21:9 bars.
  Captures were regenerated after the sine fit correction and all four pairs
  inspected again: complete centered sine figures with space before both side
  boundaries, unchanged central pixels and continued lotsadots color fields.
  Lotsadots PNG hashes are also compared with the previous accepted captures.

Rejected sine adapter: the first version centered motifs a fixed distance beyond
the central image and clipped them to the visible domain. Review of the original
2560x1080 capture identified vertical cuts at both side boundaries; these are
composition seams, even though the raster rectangles have no unpainted gaps.
The earlier visual acceptance was incorrect. The bounded fit above replaces that
adapter, and ink-containment tests now check the failure the nonblack background
fixture could not detect.

Negative result: MinGW GCC 12.2 / SDL3 3.4.16, RelWithDebInfo, ordinary separate
`SDL_RenderTexture` side rectangles dropped the outside column at 701x479 when
fractional destination widths were truncated independently. A two-triangle quad
showed the same failure; the observed behavior is consistent with SDL
shortcutting that quad to its rectangle path.
The side presenter now uses a four-triangle center fan with the same fractional
transform, avoiding that shortcut; the center retains its reference rectangle
call. The nonblack fixture checks for unpainted seams and outer-edge gaps.

Limits: only these two image adapters and the listed fades/readout operations
are supported. Sine falls back to Original on narrow sides and uses complete
independent figures on supported widths. Live allocator exhaustion, actual device
loss, sustained 4K performance, HiDPI monitor changes and screensaver/preview
hosts have not been exercised here. Combined Extended+Hybrid is deferred. SDL
side texture allocation/upload failure retires the composition to Original until
a later supported boundary. Hidden-window `test_extended --backend default`
(Direct3D11) and `--backend gpu` (SDL GPU renderer) both passed the same layout,
center identity, indexed/RGBA, palette, lifecycle and page-readout checks. Logs
are `build/workers/s-wide/default.log` and `gpu.log`. No real monitor/host
lifecycle is implied by these offscreen readback tests.
