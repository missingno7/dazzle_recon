# Native pacing calibration

## Recommendation

Use `131,311,000` work units per virtual second for the 486DX2-66 reference speed. Charge `2,000` units for one VGA or default EGA plot-kernel callback, plus `11` units for each `/D=n` `LOOP` iteration. Charge `226` units for an empty BIOS keyboard poll. At `/D=1`, this predicts about 65,296 plot callbacks/s and 581,021 polls/s; at `/D=256`, about 27,266 callbacks/s. The measured default VGA and EGA callback rates differ by only 0.08%, so this measurement supports one shared plot base cost.

The current native settings (`12,000` units/s and `1 + n` units per callback) imply about 6,000 callback/s at the default `/D=1`, roughly 10.9 times slower than the measured 486DX2-66 kernel rate. This note recommends the calibration; it does not change `native/`.

## Method and DOSBox-X settings

DOSBox-X's CPU settings guide gives these fixed-cycle equivalents for the period machines; DOSBox Staging documents the commonly rounded values (6,000 / 25,000 / 50,000). These are DOSBox cycle-budget settings, not cycle-accurate physical CPU simulations. [DOSBox-X CPU settings](https://github.com/joncampbell123/dosbox-x/wiki/Guide%3ACPU-settings-in-DOSBox%E2%80%90X/485de54003f5a523afa2a4fd144e67714e4d1ed3), [DOSBox Staging CPU settings](https://www.dosbox-staging.org/0.83/manual/system/cpu/).

| Machine class | DOSBox-X `cputype` | Fixed cycles |
|---|---:|---:|
| 386DX-33 | `386` | 6,075 |
| 486DX2-66 | `486` | 23,880 |
| Pentium-90 | `pentium` | 52,000 |

All runs used DOSBox-X v2026.08.31 acceptance build `dosbox-x-v3.exe` (SHA-256 `e5b9a81acb208bab48cb01a7dac9691cd06674fac877abce54e4693ece6dbff9`), `core=normal`, `machine=svga_s3`, `cycles=fixed N`, and dummy SDL video/audio drivers. Configurations were copied from `build/workers/unpack/final-unpacked.conf` and adapted in `build/workers/n-pace/measure.py`. The runner has a timeout and reaps the DOSBox-X process on success or failure.

For the kernel test, `build/workers/n-pace/timing_driver.c` was compiled with the locked Borland C++ 3.0 tools, assembled with TASM 3.0, and linked directly with `asm/dvdt.asm`. The driver repeatedly calls the actual VGA 320x400 kernel or the actual default EGA 640x350 kernel at fixed coordinates and symmetry 3. It reads BIOS ticks from `0040:006C`; each sample lasted 180 ticks (about 9.89 seconds). The EGA test uses `ega_plot_symmetric_640x350`, `screen_half_height=175`, and an even in-range point. Rates are `callback_count * 18.2065 / elapsed_BIOS_ticks`; a callback is one invocation of the symmetric plot kernel, not a count of the reflected pixel stores it performs.

For the original-program run, each DOSBox-X instance ran a fresh copy of `reference/DAZZLE_U.EXE` with `/I` (plus `/E` for EGA), without an `MTDAZZLE.DZL` file, so DAZZLE used its default Auto settings. The instrumentation timestamped entry to `main`, watched writes to the current image-function pointer at runtime `DS:5C4B`, and recorded entry to the fade routines. Each Auto-rate sample covers 60 seconds after `main` entry. The program's time-based RNG makes each run a single stochastic sample.

For the visual run, DOSBox-X captured video and FFmpeg extracted one frame at each second from +1 through +10 seconds after `main` entry. All 60 frames are retained locally under `build/workers/n-pace/screenshots/`, grouped into six `machine-mode` folders. For example, [486DX2-66 VGA +1s](../build/workers/n-pace/screenshots/486DX2-66-VGA/second_01.png), [+2s](../build/workers/n-pace/screenshots/486DX2-66-VGA/second_02.png), and [+10s](../build/workers/n-pace/screenshots/486DX2-66-VGA/second_10.png). The +1s frame is still the DOS title screen, the first pattern is visible by +2s, and the +10s frame has a much denser, nearly filled pattern. Each folder also contains `second_03.png` through `second_09.png`.

## Plot-kernel throughput

The measured rates below use default draw delay `/D=1`.

| Machine class | VGA 320x400 calls/s | EGA 640x350 (`/E`) calls/s |
|---|---:|---:|
| 386DX-33 | 20,072.97 | 20,091.28 |
| 486DX2-66 | 65,296.20 | 65,348.39 |
| Pentium-90 | 108,618.56 | 108,955.48 |

At the 486DX2-66 setting, the delay sweep was:

| `/D=n` | VGA calls/s | EGA `/E` calls/s |
|---:|---:|---:|
| 1 | 65,296.20 | 65,348.39 |
| 8 | 62,861.38 | 62,934.51 |
| 32 | 55,788.36 | 55,859.87 |
| 128 | 38,479.34 | 38,533.45 |
| 256 | 27,218.11 | 27,259.58 |

Fitting elapsed microseconds per callback against `n` gives:

| Kernel | Fitted fixed callback time | Fitted time per `LOOP` iteration |
|---|---:|---:|
| VGA | 15.2344 us | 0.084009 us |
| EGA `/E` | 15.2187 us | 0.083850 us |

The shared 2,000-unit plot base and 11-unit loop cost approximate both fits within the noise of these BIOS-tick samples. The per-iteration cost is about 0.55% of the fixed callback cost. The sweep directly tested `/D=1` through `/D=256`; applying the same linear cost through the accepted maximum (`8191`) is an extrapolation.

The same 486DX2-66 driver measured `bioskey(1)` at 581,676 polls/s. That is 1.719 us per empty poll; relative to the fitted VGA callback time, it corresponds to about 226 plot-base units.

## Original Auto-mode observations

These are 60-second runs of the unpacked original. “Images/min” is the number of writes to DAZZLE's current image-function pointer `DS:5C4B`, counted in the 60-second window. It is a direct image-selection count, not an estimate derived from plot calls. “First fade” is elapsed time from entry to `main` to the first observed fade routine entry.

| Machine class | Mode | Image-function selections/min | First fade after `main` |
|---|---|---:|---:|
| 386DX-33 | VGA 320x400 | 25 | 33.84 s |
| 386DX-33 | EGA 640x350 (`/E`) | 27 | 16.70 s |
| 486DX2-66 | VGA 320x400 | 38 | 31.67 s |
| 486DX2-66 | EGA 640x350 (`/E`) | 41 | 23.37 s |
| Pentium-90 | VGA 320x400 | 27 | 15.59 s |
| Pentium-90 | EGA 640x350 (`/E`) | 48 | 23.32 s |

Fade selection and image complexity vary with the time-seeded RNG, so these end-to-end values do not rise monotonically with CPU class. In particular, a separate 486DX2-66 EGA visual capture entered its first fade at 10.96 seconds, versus 23.37 seconds in the one-minute Auto-rate run. The direct kernel sweep is the steadier calibration; these full-program runs are a sanity check for the overall pace.

## Native work-unit mapping and verification

| Native operation | Recommended work units |
|---|---:|
| VGA or default EGA plot-kernel callback | 2,000 base |
| Each draw-delay `LOOP` iteration (`/D=n`) | 11 |
| Empty `bioskey(1)` / `kbhit` poll | 226 |
| `DZ_WORK_UNITS_PER_SECOND` at the 486DX2-66 reference | 131,311,000 |

Thus one default callback at `/D=1` costs 2,011 units; a callback at `/D=256` costs 4,816. Keep bulk clears, pixel reads, palette work, retrace waits, and `delay(ms)` on their own existing charge paths: this experiment measured only plot callbacks, empty BIOS keyboard polls, and the plot delay loop. No `dzplat.h` interface change is needed; the native integrator can adjust the existing rate constant and internal `dz_charge()` call amounts in `native/src/platform/platform.h`, `native/src/dvdt.c`, and `native/src/platform/input.c`.

After applying the recommendation, build and run deterministic native samples from the repository root:

```powershell
cmake -S native -B build/native -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_PREFIX_PATH=C:/tools/sdl3-3.4.16-mingw/SDL3-3.4.16
cmake --build build/native
build/native/dazzle.exe --mode vga400 --deterministic --seed 1 --speed 1 --exit-after 10 --trace-rng --screenshot build/native/vga10.bmp
build/native/dazzle.exe --mode ega350 --deterministic --seed 1 --speed 1 --exit-after 10 --trace-rng --screenshot build/native/ega10.bmp /E
```

The exit line reports `ticks=`; expect about 182 ticks at 10 seconds and about 1,092 at 60 seconds. The screenshots should show a progressing pattern at the same broad pace as the DOS captures; use `build/workers/n-pace/screenshots/` for the original one-second checkpoints. Original DOS runs seed from the clock, so compare progression and timing rather than pixel identity. The native `--snapshot SECONDS FILE` option also records virtual nanoseconds, mode, image/fade state, RNG calls, timer ticks, and charged work for fixed-seed checkpoints.

## Measurement artifacts

Raw records, adapted DOSBox-X configs, driver build report, captures, and the screenshot sets are retained under `build/workers/n-pace/`. The key machine-readable summaries are `plot-measurements.json`, `image-rate-measurements.json`, and `visual-measurements.json`. These files live under ignored `build/` and are local audit artifacts; the method and reported numbers are recorded here.
