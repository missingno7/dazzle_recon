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
grows. Future Hybrid layers belong here, upstream of the shared viewport.

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

Cached visible rows are compared with `memcmp`; only changed rows are copied
into the contiguous index buffer. Palette-only changes do not upload indices.
Unchanged scanout, effective palette, mode, display registers, output size and
policy skip texture upload, drawing and `SDL_RenderPresent`. Expose/resize and
other window events invalidate presentation through one call in `input.c`;
redrawing reuses the texture. Screenshots force drawing before readback because
the backbuffer is not retained after present. The existing clock still invokes
presentation at the monitor rate in real time. No new loop, delay or vsync pacing
was added; virtual presentation counters keep their old meaning even on skips.

## Hybrid seam (design only)

Keep the indexed Classic raster as the fidelity reference. Hybrid will need
higher-resolution render targets for additional layers and index/coverage data,
then a custom fragment shader that resolves index + palette and composites
coverage in the intended order. Color interpolation must happen after palette
lookup; interpolating palette indices would change colors and palette cycling.
INDEX8 itself cannot be a render target. Use appropriate GPU index/coverage
targets (or separate R8/coverage layers), a final color target, and the existing
presentation transform. Do not increase simulation coordinates or RNG work.

Use the GPU renderer's `SDL_CreateGPURenderState` and its additional sampler/
storage bindings when implementing that composite. On Windows its Vulkan path
needs SPIR-V and its D3D12 path needs DXIL SM6.0. SDL_GPU also exposes DXBC SM5.1
for D3D12, but the 3.4.16 GPU renderer's built-in shaders and renderer creation
properties select DXIL, SPIR-V or MSL; DXBC alone is not a drop-in choice for this
renderer. The ordinary D3D11/OpenGL renderers do not accept GPU render states.
See [shader formats](https://wiki.libsdl.org/SDL3/SDL_GPUShaderFormat) and the
[GPU renderer shader selection](https://github.com/libsdl-org/SDL/blob/release-3.4.16/src/render/gpu/SDL_shaders_gpu.c).

Local tooling inventory: neither `dxc`, `glslangValidator`, `spirv-cross` nor
`shadercross` was found in MSYS2's mingw64/usr bin directories; no dxc was found
under `C:\tools`. However the installed Windows SDK **does** provide
`C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\dxc.exe`
(also SDK 10.0.22621.0), `dxcompiler.dll`, `dxil.dll` and `fxc.exe`.
The 26100 compiler reports DXC 1.8.2502.11 and its help lists SPIR-V codegen,
so offline HLSL -> DXIL/SPIR-V has a local candidate without an MSYS2 install.
No Hybrid shader has been implemented or compiled here. FXC can generate DXBC;
Windows also supplies `d3dcompiler_47.dll`.
[SDL_shadercross](https://github.com/libsdl-org/SDL_shadercross) is an alternative
offline/runtime translation tool; it uses SPIRV-Cross and DXC for the corresponding
formats. Adding its DLL/runtime dependencies is unnecessary for Classic.

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
