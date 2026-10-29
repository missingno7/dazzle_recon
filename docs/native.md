# Native SDL3 port: design

Goal of the first native milestone: the reconstructed DAZZLE 5.0 program (src/, asm/dvdt.asm) running as a
native Windows SDL3 application with original behaviour, original resolution and original pacing. The DOS
matching build (src/, include/, asm/, tools/) is the verified reference and is never modified for the port.
High-resolution rendering, smoothing and a .scr screensaver are later milestones.

## Layout

```
native/
  CMakeLists.txt          C17, SDL3 (C:/tools/sdl3-3.4.16-mingw), MSYS2 MinGW-w64 GCC; ctest for tests/
  include/dazzle.h        native shared declarations, derived from include/dazzle.h, fixed-width types
  include/dzplat.h        platform interface used by the application code (below)
  src/app/<module>.c      ported modules, same names as src/: dzle dzl2 dzfd dosi menu dlogo dazpcx dhuf
  src/dvdt.c              C port of asm/dvdt.asm (same function names)
  src/platform/           SDL main loop, video memory + palette + text console, clock, input, sound,
                          Borland runtime replacements (rand/random/srand, conio, delay, sound, ...)
  tests/                  unit tests, DOS-reference differential tests, deterministic snapshot tests
```

`include/dzconst.h` (pure `#define`s) is shared with the DOS build via the include path. Each native function
keeps the original name and the `Original SSSS:OOOO, N bytes` header line, so native and DOS code stay
traceable. Native code is free of `far`/`near`, `MK_FP`, port I/O, interrupt vectors and Borland pragmas.

## Integer and floating-point semantics

Borland C++ 3.0 large model: `int`/`unsigned` 16 bits, `long` 32, pointers far. Native (MinGW x64): `int` 32,
`long` 32. Ported code declares globals, fields, parameters and locals with exact widths (`int16_t`,
`uint16_t`, `int32_t`, `uint32_t`, `uint8_t`, short typedefs in dazzle.h) and every expression is audited for
16-bit behaviour that 32-bit promotion changes: intermediate overflow/wrap, `uint16_t` arithmetic that now
promotes to signed `int` (comparisons after subtraction!), right shifts of negative values, narrowing, and
signed division. Fix with explicit casts or small helpers (`wrap16`, `mul16`), with a short comment where the
original relies on wrap. Compile with `-Wconversion -Wsign-compare` as an audit aid. Floating point: Borland
used the 8087 emulator (80-bit intermediates); evaluate whether `-mfpmath=387` / `long double` intermediates
reproduce reference results better than SSE before choosing. Undefined behaviour kept for byte matching
(uninitialised locals, out-of-range indexing such as the Ctrl branch indexing the Alt table) is replaced by
the behaviour the original binary actually exhibits, established by a reference test, and listed in
`docs/native-deviations.md` (faithful translation vs. intentional fix).

## Video memory model

One indexed byte array models display memory as **linear pixels**: VGA unchained 320x400 (plane = x&3,
offset = 80*y + x/4) and EGA planar 640 wide (offset = 80*y + x/8, bit 7-(x&7)) both map to
`pixel = row*width + x`, so a "paragraph" page offset (DS:7490 `page_paragraph`) is `16*para*4/width` rows (VGA)
or `16*para*8/width` rows (EGA) and the CRTC start address `80*y` is row y. Size = 64 KiB per plane (VGA:
262144 pixels = 819 rows of 320; EGA: 524288 pixels of 640), with 16-bit plane-offset wrap where the original
relies on it (clear_vram_words). Mode 13h (VGA /T) is 320x200 linear. EGA pixels hold 4-bit values; the EGA raster
op (GC 03h: replace / XOR, `set_ega_raster_xor`) applies to EGA plots. Text mode is a separate 80x25 cell buffer
(char, attribute), written by the conio replacements and by `MK_FP(0xB800,..)` call sites.

Presentation (main thread, at display refresh, independent of simulation): take `display_start_row`,
`line_compare` (rows below the split show from address 0), map indices through the VGA DAC (256 x 6-bit RGB) or
the EGA attribute registers (16 -> 6-bit EGA colour, plus border) to RGB, upload a streaming texture at the
physical resolution (320x400, 320x200, 640x350, text 720x400 with a CP437 8x16 font) and scale it into the
window with 4:3 display aspect (letterbox). Palette state is separate from pixels, so cycling animates what
is already drawn.

## Execution and time

Single-threaded and cooperative: the original control flow runs on the main thread exactly as written
(main -> auto_loop -> image steps -> plot_callback ...). The platform regains control inside the calls the
program makes constantly: the plot/read kernels, palette uploads, retrace waits, `bioskey`/`kbhit`, `delay`,
`sound`. Each such call charges **virtual time** (work units: a plot costs a base amount plus
`draw_delay_count` loop iterations, a poll a small amount, a retrace wait advances to the next 70 Hz
boundary, `delay(ms)` advances ms). When virtual time crosses a pending event, the platform:

1. runs the ported timer ISR body (graphics_timer_tick_isr: palette_elapsed_ticks++, countdowns, show time,
   BIOS-key notice) once per elapsed 18.2065 Hz tick;
2. drains SDL events into the BIOS keyboard buffer (scan<<8|ascii, 16 entries) and runs the keyboard ISR
   effect (input_pending++), mouse motion/buttons into the mouse callback effect;
3. presents a frame if the wall-clock frame interval has elapsed;
4. in real-time mode sleeps while virtual time is ahead of wall time (no busy waiting; a 240 Hz display does
   not change simulation speed). In deterministic mode it never sleeps or reads the wall clock.

Loops in the original that wait on ISR-updated variables without calling into the platform get an explicit
`dz_service()` call in the native copy (listed in native-deviations.md). The work-unit rate ("CPU speed") is a
single calibrated constant, chosen so drawing speed resembles the original on period hardware (calibrated
against DOSBox-X); `--speed` scales it.

## Randomness

Borland `rand`: `seed = seed*22695477 + 1; return (seed >> 16) & 0x7FFF`; `random(n) = (int)((long)rand()*n/32768)`;
`srand(s)` sets seed = s. Verified against the original RTL by a DOS reference run. Normal mode seeds from
`time(NULL)` like the original; deterministic mode takes `--seed`. The RNG counts calls (`--trace-rng`) so
divergence can be located. The port must consume random numbers in the original order.

## Platform services

| Original | Native |
|---|---|
| INT 1Ch / INT 9 / INT 33h callbacks | ISR bodies ported to C, driven by the clock / SDL events |
| bioskey, kbhit, getch | BIOS keyboard buffer fed from SDL key events (BIOS scan codes) |
| sound/nosound/delay | SDL audio square wave (PC speaker); delay advances virtual time |
| conio text output, B800 writes | 80x25 text console rendered in the window; stdout echo for CLI help |
| fopen etc. | stdio with native paths; configuration MTDAZZLE.DZL in the working directory |
| self-check CRC, disk park, CPU probe, LEDs, typematic, Ctrl-Break, vectors | omitted; callers get the values that keep the original control flow and RNG order (documented) |
| manual (/Z) and closing logo | read from the appended tail of an original DAZZLE.EXE supplied at run time (`--original PATH`, default next to the executable or assets/DAZZLE.EXE); not embedded |
| PCX save (F5) | writes the same PCX from the native framebuffer |

Function-pointer identity checks (e.g. `plot_callback == ega_plot_symmetric_640x700`) become explicit backend
properties (`video_mode` enum / flags) with the same effect.

## Testing

- Unit tests for kernels and helpers (plot/read/symmetry edges, palette writers, rgb8_to_ega, plasma, RNG).
- DOS reference tests: small BC++ 3.0 drivers built from the **reconstructed sources** (tools/refharness.py, a
  diagnostic build separate from the matching build) and run under MS-DOS Player. They install logging
  backends (plot/read/palette callbacks into a canonical framebuffer), seed the RNG, inject timer ticks on a
  fixed schedule and print hashes/logs. The native test runner executes the same scenario with the native code;
  compare exactly and report the first divergence (step, x, y, colour, RNG call count).
- Deterministic native snapshots (`--deterministic --seed N --script FILE --snapshot T`): canonical framebuffer,
  palette, display start/split, current image/fade, RNG count, tick count.
- Occasional end-to-end comparison with the original program under DOSBox-X for pacing and flow.
