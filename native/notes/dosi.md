# DOSI native port

All 49 original function/address headers are retained in order. DOSI's pending
stubs are removed; the DOS reconstruction, assets and matching evidence are untouched.
The copied Auto/Semi-Auto control flow still calls the image/fade/menu/resource modules.

- `check_executable_integrity` / `deRead`: a native executable has no packed MZ CRC.
  The native path always passes, without calling `genCRC`, changing `computed_crc`,
  setting `/I`, or consuming RNG. Metadata is optional: a valid `--original` MZ
  supplies its resource base, expected CRC, serials and logo offset. An absent tail
  leaves zero metadata and never prevents native startup. Explicit `genCRC` remains
  callable and implements the original polynomial and `[0,length-1)` coverage.
- `deRead` / `read_mz_resource_base`: explicitly decode little-endian words and the
  ten-byte frame. The original wrote across separate globals with `&expected_crc`;
  native globals/pointers have different alignment, so fields are assigned separately.
- `install_runtime_callbacks` / `restore_runtime_callbacks`: register the ported timer
  and keyboard bodies with dzplat, and the mouse body through a mask adapter. Ctrl-Break, hard-error resumption,
  typematic and saved DOS vectors have no host side effects. Restore closes only
  DOSI-owned live streams, clears their pointers, and leaves standard streams open.
  Native SIGABRT/FPE/ILL/SEGV use a correctly typed one-argument adapter with saved
  handlers restored on teardown. DZLE should not register the three-argument DOS
  diagnostic directly with native `signal`.
- `probe_mouse` / `mouse_position`: use platform presence/counters and callback
  registration, retaining the 10 ms settling loop and status masking. Segment and
  offset arguments are obsolete. dzplat delivers every mouse motion/button
  event to its callback. A small adapter in dosi_support retains button edges and
  reads motion counters, applying the original 0x2a/0x2b event mask before calling
  DVDT's mouse_pending_callback. Motion aborts only when its mask bit is enabled.
- `keyboard_led_flags`: virtual flag bookkeeping, including `/W` and `/B` suppression;
  no hardware LEDs. The old flags are still returned for freeze/fade restoration.
- `interrupt_signal_result`: hardresume is a no-op and the original result 2 is kept.
  This is the original DOSI symbol; pending.c's unknown `_signal_result` is distinct
  and has not been edited.
- `matherr` / `signal_diagnostic`: use typed exception fields and optional register
  words, not struct aliasing or a nonexistent native CS. DOS segment-map reporting
  becomes an explanatory native message. MinGW's `matherr`/`exception` macros are
  undefined in DOSI so this handler cannot replace the CRT `_matherr` or pull the
  whole application into isolated DVDT tests.
- `freeze_display`, `semi_auto_loop`, `closing_graphic_and_music`: explicit cooperative
  service points replace ISR-only busy waits. Freeze's original valueless `int`
  returns now return the exit flag, eliminating an indeterminate native return value.
- `configure_video_backend`: restores the linewipe and ball allocations omitted by
  Milestone A. The original packed overlay at DS:5C4F is replaced by a real
  `PaletteMapSpec {ball_bitmap_height, ball_bitmap_width, ball_bitmap_pixels}`.
  This transpose is intentional: BALL indexes `x*height+y`. EGA's no-argument
  palette builder uses a typed adapter. No RNG calls are added.
- `initialize_palette_mapping` / `auto_loop`: word products, sums, unsigned
  comparisons, work-count postincrement and random argument expressions wrap
  explicitly at 16 bits. Negative configured image sizes compare as unsigned and
  therefore clamp to the maximum, just as the DOS C expression does.
- `auto_loop` / `semi_auto_loop`: eliminate out-of-bounds table pointers using the
  proven DOS data layout (Alt=DS:01FE, Ctrl=041A, fade=0474). The extra random index
  36 aliases Alt[35] through Ctrl[-1]. Auto's symmetry lookup for indices 36..41
  uses Ctrl[index-36], one later than the drawn record; index 42 reads the nonzero
  DGROUP word of fade[1].name and is represented as true. These quirks are preserved,
  not corrected. Static evidence supports them; DOS differential confirmation remains.
- `allocate`: a request below its minimum returns NULL instead of an uninitialized
  local; normal callers retain the original allocation-halving behavior.
- Configuration numeric parsers: parse into a temporary, reduce to the original
  word and then interpret signedness. Failed numeric conversion is ignored (or
  reports bad version), instead of using the original uninitialized local after
  scanf returned zero. Values beyond the host strtoul range saturate before word
  reduction; that malformed-input corner is not a promised DOS equivalence.
- `reject_exec_filename` / `resolve_startup_configuration`: reject DOS executable
  suffixes case-insensitively while retaining native filename spelling; do not
  switch DOS drives. Try the working directory, executable directory and PATH.
  Filenames/paths are bounded to the existing 80-byte fields, avoiding DOS buffer
  overruns. The original erroneous test of a drive-buffer pointer is omitted.
- Text functions: all DOSI text goes through `dz_dosi_printf`, which writes the
  dzconio console and stdout without conio.c changes. LF is rendered as CRLF on
  the text screen. `command_line_help` skips its inter-page key wait when headless,
  so CLI `/?` and unknown options terminate with the original status 10.
- Sound helpers: sound/delay/nosound call dzplat; `/Q` also guards isolated
  `beep_float`. Float note stores, palette frequencies and the original double
  calculations remain; host libm/8087 extended intermediates can still move a
  truncated palette/map result at an integer boundary. No DOS float equivalence
  is claimed by native unit tests.
- `main`: consumes native `--` options and forwards DOS switches/config paths in
  their original order, including DOS `-h`. MinGW CRT wildcard expansion is
  disabled so `/?` and bare `*` reach the DOS parser literally. Uses `atexit(dz_shutdown)` on exit()
  paths. `--trace-rng` prints the count on full-program exit. Slice behavior and
  its regression receipts are unchanged. Full-program frame/time limits are
  cooperative at DOSI checkpoints and currently measure from native initialization;
  screenshots are captured at a limit before credits, or at exit otherwise.

Integration handoff:

- `dazzle_main(int argc, char **argv)` is declared in dazzle.h. Until DZLE is merged,
  slice.c contains a clearly marked pending entry with the original banner
  strings that permits CLI parsing then
  reports the missing port and returns 2. Delete that function when taking DZLE's
  real entry; main.c already calls it. DZLE must keep deterministic/explicit seed
  selection through dz_options rather than reseeding from wall-clock time.
- The new private platform addition is `platform/dosi_support.[ch]`; dzplat.h is
  unchanged. No conio changes are needed for DOSI. Other modules should use a
  shared stdout policy if conio's owner adds one, to avoid future double echo.
- Resource consumers must honor dz_options.original_exe even when `/W` or `/S`
  bypasses the integrity call. Missing original resources do not fail the native CRC.

Verification:

- `python -B -I tools/native.py test`: all five CTest entries pass, including DOSI.
- MinGW GCC 12.2, C17, -O2 -g, CMake `-DDAZZLE_CONVERSION_WARNINGS=ON`:
  a clean build completes without warnings.
- DOSI tests cover every switch (exit switches in child processes), ordering and
  numerical limits, mixed-case filename config round trip in a temporary directory,
  malformed settings, keys/freeze/palette actions, callback lifecycle/mouse masks, signals,
  metadata decoding, an independently calculated CRC, native argv filtering and
  limits, scripted Semi-Auto input, and fixed-seed Auto selection/work/symmetry.
  Seed 1 selects 0,15,30,4,1,32,9,39,26,30,5,24 with 43 RNG calls and two stub fades;
  seeds 23 and 2 separately exercise indices 36 and 42. These are deterministic
  native checks with unported modules stubbed, not DOS-reference acceptance.
- Initial negative build: MinGW aliases matherr to `_matherr`, causing CRT startup
  to extract DOSI and collide with DVDT's isolated fixture. Undefining the alias
  fixes that dependency; a COFF weak pending entry also failed resolution, so the
  temporary entry is explicit and must be removed on integration. An executable
  CLI smoke test also found CRT globbing changed `/?` into root-directory names;
  `_dowildcard=0` fixes both it and the bare `*` no-autoload argument.

Changes remain uncommitted per the final shared worker instruction.
