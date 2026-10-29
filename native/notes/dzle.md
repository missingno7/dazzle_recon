# DZLE native port

- Scope: all application functions and initialized/BSS objects in `src/dzle.c`
  are present in `native/src/app/dzle.c`, in original order with address comments.
  The DZLE pending group is empty. `square()` is the original square-fill helper
  (there is no `fill_square` symbol). The Alt, Ctrl, and fade record values are
  preserved. Ctrl image implementations, including `diamonds`, belong to DZL2.
- DOS ABI objects: `_stklen`, `dzle_seg`, `keyboard_regs`, and the two saved
  interrupt-vector pointers remain omitted as specified by the existing native
  header convention. Native stack sizing and callbacks replace these objects.
- `dazzle_main`: preserves the original startup, parser/integrity, video/config,
  callback, resource/logo, palette, time-seed, Auto/Semi-Auto, and closing order.
  `dz_service()` replaces the empty countdown wait so the installed native timer
  can decrement it. `fflush(NULL)` replaces Borland `flushall`. `dz_srand` uses
  `dz_options.seed` in deterministic mode and the low word of `time(NULL)` normally.
  The final host caller receives 0 or configuration-failure status 14; the
  original closing-text exit service remains called in its original branch.
  The two original atexit registrations remain in place.
- `dazzle_main`: terminates `executable_path` after at most 79 characters, avoiding
  the original unterminated 80-byte `strncpy` result on long native paths. The
  always-true array/null comparison on `startup_directory` was removed.
  `tolower` arguments are converted to unsigned char, making non-ASCII bytes
  defined. Native `_getcwd`/`_chdir` retain the original error branches.
- `dazzle_main` platform additions: `platform/dzle_support.c/.h` supplies only the
  missing typematic, drive-selection, and signal-registration adapters. Typematic
  setup has no hardware side effect (host input repeat applies); drive selection
  uses `_chdrive`. Standard SIGABRT/FPE/ILL/SEGV handlers forward to the existing
  `signal_diagnostic` service with signal type 0 and registers NULL. DOSI must
  accept the absent DOS register frame. Existing `dzconio.h` supplies the text UI.
  No change to `dzplat.h` or other platform sources is required.
- `littleStars`, `wandering`: table entries use typed local adapters instead of
  incompatible function-pointer calls. They still ignore the unused rebuild
  word or return value exactly as the DOS caller does.
- `sine`, `grid`, `swap_pixel_rectangles`: enhanced EGA decisions use
  `video_backend == BACKEND_EGA_640X700`, including when a test logging callback
  replaces the plot routine. Swapping rounds negative Y down to even with
  arithmetic shift and multiplication, avoiding left shifts of negative values.
- `choose_color`, `formula1`: signed word products wrap before sign extension or
  arithmetic shifting. `formula1` explicitly wraps both multiplications, the X
  shift, and subtraction; thus a native 32-bit triple product cannot overflow.
  `sweep2` explicitly stores a wrapped endpoint sum before continuing.
- `ellipse_step`, `cycloids`, `daisies`: signed long shifts use an explicit
  arithmetic-right-shift helper. Fixed-point products wrap at 32 bits before
  that shift; cycloids also wraps the coordinate sum before long multiplication.
  Ordinary radii/gradients fit signed 32 bits in the four supported backends.
  The image routines still require configured screen dimensions and rebuild
  before stepping; driving shrinking arches beyond the original work limit is
  outside that contract.
- Floating-point functions (`ellipse_step`, `atom`, `bigstar`, `corkscrew`,
  `curves`, `cycloids`, `pedals`, `sine`, `squirals`): retain original float
  variables/stores, with explicit float-to-word truncation. Computation through
  float-only products/sums is promoted to double so SSE does not introduce extra
  float rounding before truncation. `corkscrew` retains the cast *before* adding
  the origin, and Sierpinski retains signed displacement division toward zero.
  Host double/libm is not an 8087 80-bit guarantee; near-integer truncation in
  trigonometric/fixed-point results still needs the planned DOS differential run.
- `cycloids`: its local endpoints are initialized to zero defensively. Every
  valid rebuild/step call assigns both in rebuild or next-curve setup before
  reading them; normal behavior and RNG consumption are unchanged.
- `draw_ellipse`: direct native indexing initially failed GCC bounds traps for
  enhanced EGA arches, seed 7996. The original binary stores directions at
  DS:4DE4 + 2*index (`066A:08EC/0903/0911`) and tests DS:535C at `066A:0935`.
  Therefore entry 700 aliases `ellipse_trace_done`: its nonzero Y-major direction
  terminates caching, leaving limit 701. Inclusive playback also reads entry 701,
  DS:535E (`ellipse_fill_x`). A small accessor maps these two original aliases
  explicitly, preserving state and pixels without native out-of-bounds access.
  A focused 349-by-638 ellipse test fixes the 701-entry behavior. Remaining
  fixed-array accesses have bounds checks active in optimized builds.
- `balls`: DOS reads the bitmap at the *word-wrapped* column-major offset before
  checking X/Y. Native retains valid offset aliases and the original later
  coordinate check, but returns without changing the pen/color when the offset
  escapes the allocation or the bitmap is absent. This intentionally avoids an
  unrelated DOS heap read, whose incidental byte can otherwise change loop
  termination. No RNG calls are added. A DOS heap/reference check is needed to
  characterize that original out-of-allocation case; the deviation is explicit.

## Diamonds handoff to the DZL2 porter

- `diamonds` is **0B84:02EF**, not in segment 066A or the DZLE pending group.
  Its gate is a stack local `[BP-2]`, not a register: `02F2` allocates six bytes,
  `02FF` writes zero only on initialize, and `03D8` compares it signed against
  zero. There are no other writes to that local in the function. A persistent
  countdown, new random draw, or unconditional refresh would change the original.
- For an uninterrupted normal drawing loop, let P be the caller SP before
  pushing the one-word image argument. The gate occupies P-10. The following
  one-word plot call uses exactly the same stack position for `PLOT_X_PLUS`
  (`asm/dvdt.asm`, equate 2), writing half-width plus X (or swapped Y). Diamonds
  leaves a bounded positive origin, so the ordinary next call sees a positive
  gate and keeps its velocity/dimension steps. Auto calls are `11C1:3C57/3C96`,
  the following plots `3C60/3C9F`; Semi-Auto uses `3786/3849/3857` and `3860`.
  Palette cycling's far upload return CS also occupies P-10 and is the positive
  segment 11C1 (`11C1:2561`); Auto's initial fprintf return CS does likewise.
- Thus `step_refresh_gate = initialize ? 0 : 1` is a deterministic replacement
  for the ordinary no-input loop's **branch decision**, not a claim that the DOS
  local stores 1. Unusual input-service paths and asynchronous ISR stack writes
  can leave different residue; a DOS reference should settle that boundary.
  No DZL2 implementation/stub was edited, honoring the module ownership rule.

## Verification

- `python -B -I tools/native.py test`: all five CTest entries pass, including
  `test_dzle`; the original slice receipts remain unchanged. Main's complete
  runtime flow awaits the other module ports; pending services still abort.
- Conversion build: CMake cache `DAZZLE_CONVERSION_WARNINGS=ON`; DZLE additionally
  passes GCC 12.2 `-std=c17 -Wall -Wextra -Wconversion -Wsign-conversion
  -fsyntax-only` with no diagnostics. Word stores/narrowing are intentional;
  dimension-limited sums/products and positive bounded shifts were inspected.
- `test_dzle` locks framebuffer FNV-1a and RNG counts for all 36 Alt entries:
  seed 1, VGA 400, rebuild plus eight calls, no timer injection or palette
  cycling, full 524288-byte indexed VRAM. Balls uses a fixed nonzero fixture
  with the configured dimensions because DOSI owns bitmap allocation/generation.
  These are native regression receipts, not DOS-equivalence hashes.
- Stress: every Alt entry, four backends, 32 seeds `1999*k`, rebuild plus eight
  calls (4608 cases), plus square/polygon/fill/outline/ellipse-cache/swap/backend,
  signed-formula, and escaped/wrapped bitmap checks. GCC 12.2 accepts and passes
  `-fsanitize=undefined,bounds -fsanitize-undefined-trap-on-error
  -fno-sanitize-recover=all` without requiring a MinGW sanitizer runtime DLL.
  Scratch sanitizer build/logs are under `build/workers/dzle/`.

- balls (supervisor, from n-diff): sampling outside the ball bitmap reads zero padding and keeps walking, as DOS does
  (DOS fetched before the extent check). Native policy: the area outside the allocation reads as zero; the
  original heap bytes there were not defined. Verified against the DOS reference driver (native/tests/ref/DIFFERENCES.md).
