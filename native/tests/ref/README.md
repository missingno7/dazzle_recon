# DOS reference diagnostics

These captures are labelled **reconstructed source compiled with BC++ 3.0, run under MS-DOS Player**. They do not run the untouched DAZZLE executable and are not original-executable references. The drivers compile reconstructed modules in an isolated diagnostic link; they do not edit `src/` or matching receipts.

## Build and run

From the repository root, the general interface is:

```powershell
python -B -I tools/refharness.py DRIVER.c [MODULE.c ...] `
  [--define MODULE:NAME[=VALUE]] [--input FILE] [--runner PATH] [--fresh]
```

The harness calls `tools/bcc.py` with BC++ 3.0 flags `-ml -O1`, stages the project headers, links with TLINK 5.0 and `C0L.OBJ`, then links `EMU.LIB`, `MATHL.LIB`, and `CL.LIB`. The toolchain files are checked against `evidence/toolchains.json`. A run and its `report.json`, stdout and generated files stay under `build/refharness/<name>/r_*`.

The capture commands were:

```powershell
python -B -I tools/refharness.py native/tests/ref/rng_driver.c --name rng --fresh

python -B -I tools/refharness.py native/tests/ref/images_driver.c src/dzle.c src/dzl2.c `
  --input native/tests/ref/scenarios/images.tsv `
  --define dzle.c:main=dzle_diagnostic_main `
  --define dzle.c:rand=ref_rand --define dzle.c:srand=ref_srand `
  --define dzl2.c:rand=ref_rand `
  --name images `
  --runner C:\tools\msdos\binary\i386_x64\msdos.exe `
  --timeout 180 --fresh

python -B -I tools/refharness.py native/tests/ref/palette_driver.c `
  native/tests/ref/palette_stubs.c src/dzle.c src/dosi.c `
  --input native/tests/ref/scenarios/palettes.tsv `
  --define dzle.c:main=dzle_palette_unused_main `
  --define dzle.c:rand=ref_rand --define dzle.c:srand=ref_srand `
  --define dosi.c:rand=ref_rand --define dosi.c:srand=ref_srand `
  --name palettes `
  --runner C:\tools\msdos\binary\i386_x64\msdos.exe `
  --timeout 180 --fresh
```

`tools/bcc.py` compiles with `C:\tools\nmlgcdos\msdos.exe`. The harness uses the selected runner for linking and execution. The default `nmlgcdos` build identifies itself as “MS-DOS Player (ia32, tuned for ReC98) for Win32-x64 console” and has SHA-256 `f7f6cb0a3e816c5edb13112d327c1bddbf7463fe7bf9a005ca1eb5317751bd02`. The alternate `C:\tools\msdos\binary\i386_x64\msdos.exe` identifies itself as “MS-DOS Player (i386) for Win32-x64 console” and has SHA-256 `0374576f0f970f5c342e482ed6d44d8e72bbfea3f063ccd3f38360ea81d5c53e`. Both belong to the MS-DOS Player source/release dated 2024-06-23. `nmlgcdos` completed the integer RNG capture, but stalled when executing x87 math; the `i386_x64` runner was used for the image and palette captures that initialize/use floating point. Known runner hashes are checked by `refharness.py`.

The pinned compiler and linker hashes are BC++ 3.0 `BCC.EXE` `e56c6d228191e33665802e3affd9f44803af7c19c0c5a591c28ad10ffcdada6f` and TLINK 5.0 `ac0ac8b0084e06e9916ee12602aa4fcba998f7b702fc2adf4a213564288147c7`. The run reports record hashes for the startup object, libraries, support files, sources, inputs and produced files.

## Captures

### RTL RNG

`data/rng.txt` contains 2,000 `rand()` results for each seed `0`, `1`, `12345`, and `0xFFFF`, plus the `random(n)` probes. The 8,000 captured values match:

```text
state = (state * 22695477 + 1) modulo 2^32
rand  = (state >> 16) & 0x7FFF
```

With no `srand`, the first result is `346`, matching the RTL's initial state of 1. `srand` accepts a 16-bit `unsigned`: seeds `1` and `0x10001` both first return `346`. DAZZLE's `(unsigned)time(NULL)` seeding likewise keeps the low 16 bits of the seconds value. The saved `time_value` and `time_seed_first` are from the capture run and will vary on a rerun.

The installed Borland `stdlib.h` defines `random(num)` as `((long)rand() * num) / (RAND_MAX + 1)` with `RAND_MAX` equal to `0x7FFF`; its final result is narrowed to 16-bit `int`. `rng.txt` preserves signed, unsigned (including limits at and above 32768), and long-argument outputs, including edge cases caused by 16-bit result narrowing and 32-bit multiplication.

### Image routines

`scenarios/images.tsv` fixes 42 runs: the 36 `alt_image_records` followed by the six `ctrl_image_records`, seed `0x4100` through `0x4129`, 12 continuation steps, symmetry 3, and one modeled timer tick per 37 plot callbacks. Each image runs `draw(1)`, then twelve `draw(0)` calls, each followed by one `plot_callback(draw_symmetry)` call.

`data/images.txt` has plot counts, CRC-32 of each `(x,y,color,symmetry)` stream, RNG and palette-cycle call counts, and framebuffer CRCs after initialization, halfway and at completion. Plot tuple fields are 16-bit little-endian values. Full callback records and the first 32 callbacks per image are in the corresponding `build/refharness/images/r_*/PLOT.BIN` and `PLOT.LOG`; each binary record is 10 bytes: image index, x, y, color, symmetry, each a little-endian word. The tuple CRC excludes the image index. The 654,656 full records were checked against the per-image counts and CRCs in `data/images.txt`.

The driver models VGA 320x400 state with a 320x400 indexed buffer, center `(160,200)`, the configured palette dimensions, page paragraph and bounds. Its plot callback follows the symmetric edge and endpoint rules in `docs/graphics.md`; read-pixel reads the model buffer. A timer tick increments `palette_elapsed_ticks`, `countdown_ticks`, and `show_duration_ticks`; the palette-cycle equivalent runs only where the image routine calls it. The RNG calls in image and palette runs are routed through a counted wrapper implementing the captured RTL LCG.

This is a software model of the VGA assembly kernel, not execution of `asm/dvdt.asm` or hardware VGA. It does not emulate retrace timing, BIOS input, interrupt scheduling, register/latch behavior, or actual DAC uploads. The image palette-upload callback is inert, and the buffer begins zeroed. The results are diagnostic for reconstructed C code plus the documented model semantics.

### Palette builders

`scenarios/palettes.tsv` covers eleven cases: the VGA `build_ega_palette` path with base, `/A`, `/M`, `/A`+`/M`, and `/F` settings; and EGA `generate_palette` with base, `/A`, `/M`, `/F`, `/4`, and `/4`+`/F`. `data/palettes.txt` records each generated-buffer CRC-32 and first 24 bytes. It also stores complete 2,499-byte buffers for `vga_base` and `vga_alternate_mad`, both exercising the floating-point palette builder. The DOS outputs include the small header/trailing bytes in those CRCs and full buffers.

`palette_stubs.c` supplies no-op hardware, mode, input, menu, and unrelated image symbols needed to link `src/dosi.c` and `src/dzle.c`. The tested palette builders and their math and RNG routines come from reconstructed source; DAC output is not performed. The VGA path was run with the i386 MS-DOS Player to execute Borland x87 math.

## Limits

The DOS EXEs are temporary diagnostic links of reconstructed modules, not a rebuilt or untouched DAZZLE program. Image/palette RNG counts are exposed with a counted replacement using the separately captured RTL recurrence. The image pixels depend on the documented software model of the original assembly plot behavior. Palette results involving `cos()` are specific to BC++ 3.0's math runtime and the recorded MS-DOS Player build. The full plot streams remain in ignored build output; only compact summaries are retained under `data/`.
