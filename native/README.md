# DAZZLE 5.0 native program

Quick start (Windows, from the repository root; tool locations in docs/native.md):

```powershell
python -B -I tools/native.py test                 # configure, build build/native/, run all tests
python -B -I tools/native.py run --original assets/DAZZLE.EXE   # run with the original's logo/manual
build/native/dazzle.exe -?                        # original command-line help; original options work as in DOS
```

`build/native/dazzle.exe` needs `SDL3.dll` beside it (the build copies it). Keys and options are those of the
original manual (`dazzle -z` extracts it when the original EXE is available). Alt+Enter toggles fullscreen.

Without `--slice`, the merged application runs its original startup, Auto and
Semi-Auto image loops, palette cycling, transitions and closing sequence. F1 opens
help and F2 opens the configuration editors and File Access menu.
Original resources are optional: use `--original PATH` for the closing logo and
`/Z` manual extraction. Missing resources produce a short message when needed.

Whole-program controls include startup time. `--exit-after SECONDS` and
`--frames N` request cooperative shutdown at a virtual deadline (frames are
70 Hz simulation retraces). `--speed X` changes wall pacing only; the simulation,
timer and RNG state at a virtual time are independent of presentation speed.
`--deterministic` uses virtual time without sleeping or reading the wall clock.
`--script FILE` accepts ordered `seconds key` lines, for example `3 ALT+F9`,
`8 ALT+J`, `8.2 F10`, `9 a`, `14 ESC`; `#` starts a comment.
Keys enter the BIOS queue at their timestamp and are handled at the original
application's next input checkpoint, including its startup queue drain.

Repeat `--snapshot SECONDS FILE` to capture canonical visible indices and full
active palette, display origin/split, image/fade, RNG/ticks and work counters.
The ASCII header ends in `END\n`, followed by raw indexed pixels (text mode uses
80x25 character/attribute pairs). `palette=` contains DAC RGB hex, or 16 EGA
attributes followed by the border byte. `fade` is the last effect's ASCII key,
or zero. A one-line FNV-1a receipt covers the complete file. The regression tests
pin SHA-256 hashes in `tests/integration-hashes.json`; these are native regression
receipts, not claims of DOS equivalence. Convert a graphics or CP437 text capture with:

```powershell
python -B -I native/tests/snapshot_png.py capture.snap capture.png
```

`python -B -I tools/native.py test` runs module tests and full executable tests
for VGA, EGA, `/T`, scripted input, resource/CLI controls, repeatability and speed
independence. See [integration notes](notes/integration.md) for fixes and the
windowed acceptance run. The older foundation details below document the
retained optional slice driver.

Original-program behavioural acceptance, including the distinction between F1
and ESC in help, is recorded in [acceptance notes](notes/acceptance.md).

## Foundation slice (Milestone A)

This C17/SDL3 slice progressively draws the original `sierpinski` and `lines` images,
with centered eight-way symmetry and palette cycling at the original BIOS tick pace.
The DOS reconstruction and its matching tools are untouched.

From the repository root, with the supplied Windows tool locations:

```powershell
python -B -I tools/native.py build
python -B -I tools/native.py test
python -B -I tools/native.py run --slice sierpinski --seed 2
python -B -I tools/native.py run --slice lines --exit-after 5
python -B -I tools/native.py run --headless --deterministic --seed 1 --steps 10000
```

The wrapper prepends MinGW GCC 12, Visual Studio CMake/Ninja, and SDL3 3.4.16 to
the child PATH, configures `build/native`, and builds. `test` also runs CTest.
Ninja is preferred. A three-second probe detects its process-completion pipe hang
under restricted Windows tokens; that environment uses native `mingw32-make.exe`
with CMake's **MinGW Makefiles** generator. The cached generator is reused.
Set `DAZZLE_CMAKE_GENERATOR=Ninja` to explicitly select Ninja outside that runner.
The [Ninja Windows subprocess implementation](https://github.com/ninja-build/ninja/blob/v1.12.1/src/subprocess-win32.cc)
uses named pipes even for console-pool commands. No tool binaries or permissions are changed.

CMake builds one `dazzle_core` static library, `dazzle.exe`, and an executable/CTest
entry for every `native/tests/test_*.c`. New sources and tests are detected by glob.
SDL3.dll is copied beside each executable. Assertion checks stay enabled in tests.
For an integer-conversion audit, configure with `-DDAZZLE_CONVERSION_WARNINGS=ON`;
the wrapper preserves this cache setting. The shared `dzplat.h` currently emits an
existing nested-comment warning because its introductory comment contains `src/app/*.c`.

| Option | Meaning |
|---|---|
| `--slice sierpinski\|lines` | Temporary image driver (default: sierpinski). |
| `--mode vga400\|vga200\|ega350\|ega700` | VGA 320x400 default, VGA 320x200, or EGA physical 640x350 with 350/700 software rows. |
| `--deterministic` | Virtual time only; no wall-clock reads, audio, or sleeps. Default seed 1. |
| `--seed N` | Accepts a 32-bit number, truncated to the RTL's 16-bit srand argument; the internal LCG is 32-bit. Normal mode uses the original time seed. |
| `--headless` | No window or audio device. |
| `--renderer auto\|indexed\|rgba` | Default `auto`: native INDEX8 + palette when supported, otherwise CPU RGBA. `indexed` requires support; `rgba` uses the reference conversion. Text uses RGBA. |
| `--present original\|crop\|stretch` | Default `original`: centered 4:3 with black bars. `crop`: cover the window with a centered 4:3 image and crop overflow. `stretch`: fill with distortion. Applies to graphics and text. |
| `--window WxH` | Initial window size, e.g. `--window 1920x1080`; resizable, default 960x720. Presentation uses SDL's actual output pixels, including display density. |
| `--fullscreen` | Start fullscreen; Alt+Enter still toggles it. |
| `--speed X` | Wall pacing multiplier; virtual work, BIOS timer and retraces remain fixed. |
| `--steps N` | Number of image steps after the original rebuild-and-plot call. |
| `--frames N` | Stop after N virtual presentations after slice setup; 70 Hz in deterministic mode. |
| `--exit-after SECONDS` | Stop after this much virtual drawing time (real time when throttled). |
| `--original PATH` | Optional original manual/logo resources; default `assets/DAZZLE.EXE`. No resource is read by this slice. |
| `--screenshot FILE.bmp` | Save an SDL render capture before closing (requires a window). |
| `--trace-rng` | Accepted; RNG call counts are always included in the final receipt. |

Escape and window close exit through the same BIOS-key dispatcher. Alt+Enter
toggles fullscreen. The resizable window uses a 4:3 display aspect with letterboxing
and nearest-neighbor pixels. Letter/number keys, F1–F12, Shift/Ctrl/Alt combinations,
navigation and numeric keypad keys are translated to BIOS scan/ASCII words; the
16-entry queue implements read, non-consuming peek, and shift/toggle flags.
Only Escape has an application action in this temporary driver.

The Classic presenter uploads visible indices separately from the palette and
uses nearest sampling. Static images skip upload and present until a change,
expose or resize; presentation keeps the existing clock pacing. Screensaver and
preview policies show black during internal text modes. See
[rendering decisions and verification](../docs/native-render.md) for the space
definitions, GPU backend results and future Hybrid requirements.

Opt-in hidden-window GPU equality checks and 20-second CPU measurements:

```powershell
python -B -I native/tests/verify_present_backends.py # default, D3D11/12, Vulkan, GPU, OpenGL
python -B -I native/tests/measure_present.py build/native/dazzle.exe --slice sierpinski --seed 1 --exit-after 20
python -B -I native/tests/measure_present.py build/native/test_present.exe --static
```

The backend script accepts an executable followed by optional backend names.
Unavailable drivers print `UNAVAILABLE`; mismatches or a 180-second timeout fail
the script, and timed-out children are killed. Normal CTest uses only the SDL
software renderer under the dummy video driver.

The platform implements indexed video memory, VGA DAC and EGA 64-color scanout,
display-start rows and line-compare splits, an 80x25 text buffer with scrolling and
CP437 8x16 glyphs (9th column duplicated for C0–DF), BIOS time, 70 Hz retrace,
mouse counters, and a queued SDL3 float audio stream generating a square wave.
Text uses [viznut's Unscii-16](https://github.com/viznut/unscii), Public Domain/CC0;
four absent symbols have small original CC0 substitute glyphs. Font provenance
and the GPL `unscii-16-full` exclusion are recorded in the font header.

`DZ_WORK_UNITS_PER_SECOND` in `src/platform/platform.h` is the single initial
throughput estimate: 12000, giving about 6000 plot-callback calls/second with
`draw_delay_count=1` (before retrace waits). Pacing calibration against period
hardware remains open. Polls/audio/presentation are serviced every virtual ms;
presentation follows the monitor rate in real time. `dz_service()` and BIOS polling
advance virtual time, so waits remain cooperative. ISR BIOS peeks cannot reenter
the clock. SDL bootstrap, text-output and test helpers are internal declarations
in `src/platform/platform.h`. The only shared `dzplat.h` changes are a comment
clarifying the RTL's 16-bit srand argument and explicit 32-bit product wrap in
`dz_random` (required for its existing long-argument path).

The copied application functions retain original order, names and address/size
comments: DOSI's palette builders (`build_ega_palette` is the original **VGA**
builder), randomize state, math table initialization, backend configuration and
palette cycling; DZLE's `line_step`, `choose_color`, `lines`, and `sierpinski`.
The requested color primitive is named `choose_color` in the reconstruction.
All DVDT routines and timer/keyboard/mouse handler bodies come from n-dvdt's
`src/dvdt.c`; no temporary kernel implementation remains.

Native `include/dazzle.h` converts the complete shared declarations, records and
callbacks to fixed-width integers and ordinary pointers. DOS `text_info` has a
portable record. Deliberately dropped ABI-only material: `DAZZLE_SHARED` and
`DAZZLE_MENU` access tricks, far/near/huge/interrupt qualifiers, `_stklen`, code
segment getters, the DOS `main` prototype, Borland `strlen`/`malloc`/`free`/`tolower`
redeclarations and `_streams`/`_osmajor` aliases, physical `bios_keyboard_flags`,
`keyboard_regs`/`mouse_regs`, saved interrupt-vector pointers, palette pointer
segment/offset macros, `far_callback`, and the Borland standard-stream aliases.
Palette uploads now take one native byte pointer; mode/plot callbacks use a
`uint16_t` argument and the original no-argument EGA builder has a typed adapter.
Native headers describe future modules too; declarations do not imply those
modules are implemented.

Milestone-specific departures from full DOS startup are deliberate: the slice
driver selects an image directly, omitting configuration/menu/fade/show/logo
flows; the complete input dispatcher is deferred. BIOS LEDs are virtual state,
and the mouse-abort callback is not installed by the slice. Mode/palette bootstrap
is unthrottled with the simulation clock restarted afterward, avoiding charging
a 128000-byte initial clear at plot speed. `configure_video_backend` defers the
unused linewipe/ball allocations and ball map; it retains the divisor RNG call
and bitmap dimensions. A typed adapter replaces the EGA no-argument callback
cast. These choices do not change the two copied image algorithms' RNG order.
Float trig uses host libm and explicit original float stores; 8087 intermediate
precision and DOS-reference image/palette equivalence still need differential
validation. Signed products in `choose_color`, Bresenham word arithmetic and
Sierpinski's signed displacement division explicitly preserve 16-bit behavior.

Regression receipts (GCC 12.2, seed 1, deterministic/headless, VGA 400, 10000 steps,
one rebuild-and-plot first; FNV-1a over **all 524288 indexed bytes** and **768 DAC
bytes**, respectively):

| Slice | Framebuffer | Palette | RNG calls | BIOS ticks |
|---|---|---|---:|---:|
| sierpinski | `6abc9dc5` | `31b01fc5` | 10017 | 34 |
| lines | `1f227748` | `31b01fc5` | 38 | 34 |

These are native regression hashes, not evidence of DOS equivalence. The platform
test fixes seed 1's first ten rand values to
`346,130,10982,1090,11656,7117,17595,6415,22948,31126`. CTest also checks rational
BIOS tick/retrace conversion, key/modifier/queue behavior, palette and split
scanout, text, Bresenham and wrapped color products, deterministic slices,
palette changes without framebuffer writes, and close-to-Escape behavior.
The four-second windowed seed-2 Sierpinski capture is
`build/workers/n-found/sierpinski-window.bmp`; the process closed afterward.

`build/workers/n-found/verify_rng.c` additionally compared all 8000 captured rand
values (four seeds) and all 18 signed/unsigned/long random cases with n-ref's
`native/tests/ref/data/rng.txt`: equal. This also establishes that srand truncates
to 16 bits and random(long) wraps its product at 32 bits before signed division.
The comparison receipt is `build/workers/n-found/rng-verification.txt`.
