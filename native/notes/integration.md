# Merged native integration

The complete native program now runs without an original executable. F1/F2
remain the supplied pending menu stubs. No menu.c, conio.c, text.c, dzconio.h,
DOS source, matching evidence, assets or shared dzplat.h was changed. Work is
uncommitted as requested.

## Fixes (symptom, cause, fix)

- Startup exited immediately when resources were absent. DLOGO's optional load
  set `exit_requested`, which skipped Auto. Missing logo loading now returns
  quietly; the closing branch reports its absence and continues to closing
  text. Invalid optional logo headers/dimensions are discarded without aborting
  startup. `/Z` without the original prints a short unavailable message and keeps
  the parser's status 3, creating no manual. Valid extraction remains unchanged.
- Full-program `--exit-after`/`--frames` could run indefinitely during drawing,
  countdowns or blocking reads. DOSI's checkpoints were the only limit checks.
  New private `platform/testctl.[ch]` hooks the clock at exact virtual deadlines,
  latches exit/input, clears countdowns, and releases a blocking BIOS read with
  Escape. DOSI's old entry points delegate to this file. No forceful host exit
  or wall-clock watchdog is needed for normal termination. Credits still run;
  a requested stop skips their interactive hold/music after rendering the logo.
- ESC skipped the closing logo: the decoder mistook the already-set application
  exit flag for a decoding error. DLOGO saves/restores that flag and tracks actual
  decoding failure separately, restoring the original closing-image behavior.
  DOSI explicitly presents the decoded image before holding it. The resource
  regression compares the full real four-plane logo against the independent
  existing decoder; the executable test captures it after scripted ESC.
- A VGA mode clear consumed 10.7 virtual seconds, delaying the first drawing and
  each boundary. DVDT charged every bulk byte at delayed-plot cost. Clear and
  planar-row transfers now charge one unit per 64 bytes, plus entry overhead,
  while retaining the original stores, retrace and control flow. This is a
  native pacing estimate, not period-hardware calibration. Only VGA Gas Cloud
  and Fireworks timing/palette receipts changed in the fade fixtures; aggregate
  receipts were re-recorded as `b4684d97`, `8d86c7b5`, `7c84ddd8`, `385cbfbe`.
  All other per-effect receipts and the retained slice receipts stayed equal.
- `--speed` changed how much drawing happened between BIOS ticks. Speed was
  inside the work-to-virtual-time conversion. Work cost is now fixed; speed
  scales virtual-to-wall pacing and presentation intervals only, as required
  for identical simulation state at equal work/time. Real throttling uses
  `SDL_DelayNS`; `SDL_DelayPrecise` spun during the small service intervals.
- `--seed` was ignored for real windowed runs: DZLE reseeded with `time(NULL)`.
  Main already chooses the time seed when none is supplied. DZLE now always uses
  that chosen seed, preserving ordinary time seeding and honoring explicit ones.
- Native closing text uses DOS `exit()`, bypassing main's output error return.
  The closing exit helper now finishes captures and propagates snapshot failures
  as status 1. Capture finalization is idempotent across main and atexit.

## Controls and evidence

- Snapshots contain visible indexed scanout after display start, split and plane
  wrap, full DAC or EGA attributes/border, image index, last dispatched fade key,
  RNG call count, BIOS ticks, charged work, image work, pan and Semi-Auto state.
  Snapshot/key deadlines split clock advances; timestamps are rounded to the
  nearest nanosecond. Up to 256 ordered script keys and 64 snapshots keep the
  controller small. No SDL event timestamp or host time enters deterministic
  execution. See native/README.md for format and examples.
- Tests pin SHA-256 of the full header+frame in
  `native/tests/integration-hashes.json`: default VGA, EGA, VGA `/T`, and a script
  with N, Alt+F9, Alt+J, F10, A fade, N, F10 and ESC. Two subprocesses give
  identical snapshots; `--speed 4` gives an identical full snapshot at 6 virtual
  seconds, including work count. These guard native regressions only.
- Additional executable tests exercise every requested DOS option, explicit
  configuration loading with rapid Auto boundaries, one-panel/timed Show,
  startup/graphics/EGA/T/Semi-Auto/frozen stop limits, help/order form, missing
  resources, real manual extraction and a real closing-logo capture. The C
  snapshot test checks known visible rows across VGA/EGA origin/split changes.
- `python -B -I tools/native.py test`: **all 21 tests passed**, including the
  scanout and output-failure checks (10.26 seconds on the final full run).
  `git diff --check` passed. Build cache:
  `DAZZLE_CONVERSION_WARNINGS=ON`, GCC 12.2 / C17 / -O2 -g. There are no conversion
  warnings; the three existing DZLE implicit-fallthrough warnings are unchanged.
  Resource tests read `D:/Prog/dazzle_recon/assets/DAZZLE.EXE` because the ignored
  binary is absent in this worktree; it was not copied into or modified in assets.
- A negative audit attempt tried to relink dazzle.exe while its windowed run was
  active; Windows denied replacement of the running executable. After that
  process exited, the same build and test command passed.

## Real windowed run

`build/workers/n-integ/window_probe.py` ran the normal windowed executable with
seed 1, scripted input and `--exit-after 60 --screenshot window-60.bmp`, taking
13 virtual-time snapshots. It exited 0 after **60.5 real seconds**; total CPU
was **6.765625 seconds** (about **11.2% of one core**), with five-second CPU
samples in `window-run.json`. The loop sleeps, and the process/window are closed.

Visual inspection of converted captures confirmed progressive Sierpinski at
12/13 seconds, cycling palettes, the A transition's growing colored rectangles
at 18/22 seconds, and further overlaid Auto images at 59 seconds. Captures,
stdout receipts, CPU samples and the BMP remain in `build/workers/n-integ/`.
The scripted switch into Semi-Auto, drawing, fade interruption, return to Auto
and later N were handled while the window continued presenting.

No public interface changes are required from other porters. Existing module
notes' DOS floating-point and stack-residue uncertainties remain; this work
does not claim new DOS differential equivalence or change those policies.
