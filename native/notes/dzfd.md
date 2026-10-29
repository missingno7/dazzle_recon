# DZFD native port

All 33 original functions and all file-scope objects from `src/dzfd.c` are in
`native/src/app/dzfd.c`, in original order with original address/size comments.
The four initialized pattern tables retain every original value. The DZFD
group alone was removed from `pending.c`. No unknown-group symbol was claimed.
The 24 Fade Enable entries comprise 23 dispatched effects plus Pan/Scroll.

## Defined behavior and intentional differences

- `dzfd_seg`: DOS returned CS for diagnostics; native returns zero, as does the
  existing DVDT compatibility getter. Segment identity has no native meaning.
- `wait_fade_tick`, `display_boundary_fade_and_pan`: DOS idle/release-slice
  interrupts become `dz_service()`; the same countdown, H/N/P handling and DOSI
  LED helper calls remain. DOSI owns input normalization and LED policy.
- `firefly_transition`: DOS reinterpreted adjacent `int` slots as unaligned
  `long` lvalues. `ff_get`/`ff_set` explicitly pack/unpack the same low/high
  words, avoiding native alignment/aliasing UB. Signed 32-bit multiplication
  wraps before signed division; coordinate updates wrap after division.
- `randomize_helper`: particle coordinates and accumulated work/gravity preserve
  signed word wrap explicitly. Negative fixed-point shifts use arithmetic
  helpers rather than implementation-defined host shifts.
- `drip_transition`: the temporary speed array is now an actual `uint32_t`
  array instead of a union array reinterpreted as `unsigned long *`. Zero
  initialization defines unused slots; all used speeds in the supported modes
  are initialized by the original seeding/midpoint passes before being read.
- `spiral_midpoint`: unsigned 32-bit profile averaging/jitter wrap is explicit;
  each random bound is reduced to its original word type.
- `fade_countdown`: repeated enhanced-EGA calls still double the shared stroke
  table in place. Unsigned shifting plus signed-word recovery defines the
  original negative shifts and eventual word wrap; the table is not silently
  reset by the application. Test fixtures reset it between independent runs.
- `swap_rectangle`: normal calls always use angles 0..63. Direct invalid calls
  with 64 or a negative index now return, preventing the original out-of-table
  trig read. The DOS result for such invalid calls is deliberately not claimed.
- `animate_line_compare_split`: the original reverse VGA `/T` loop increments
  odd `scanline` values, then its for-loop decrements them, repeating forever
  at 399. Native reverse `/T` steps by two (400,398,...,0); other paths retain
  their original step order. This is an intentional termination fix, inferred
  directly from the reconstructed control flow, not DOS differential evidence.
- `fade_dac_to_black`: normal dispatch remains VGA-only. A direct EGA call now
  limits the component walk to `palette_buffer_bytes`, avoiding the original
  240-byte RGB walk through an 80-byte EGA allocation. Normal VGA is unchanged.
- `fade_starburst`: allocation failure frees either successful allocation and
  returns after the DOSI fatal helper if it unexpectedly returns. Successful
  allocation retains the original RNG, particle and trail algorithms.
- `display_boundary_fade_and_pan`: `setmem` becomes C `memset`; it does not
  claim or remove the supervisor-owned unknown-group `setmem` stub.
- `randomize_helper`, `spiral_transition`, `fade_twist`, `swap_rectangle`: float
  storage and double math calls/constants retain their original types. The
  original 8087 could retain 80-bit intermediates before integer truncation;
  native SSE/libm receipts do not establish DOS equality near integer boundaries.

## Cooperative loops

Each added service point costs one platform work unit and allows pending timer
and input events to run. Palette cycling alone may perform no platform work
when the display is frozen/static.

- `wait_fade_tick`: countdown loop; service replaces the DOS idle interrupts.
- `randomize_helper`: active-burst loop; all heads/tails can be clipped.
- `random_delay_transition`: outer countdown loop between complete bursts.
- `quilt_transition`: outer countdown loop between tile swaps.
- `mesh_transition`: both the five-tick refinement wait and final 60-tick wait.
- `spiral_transition`: final 60-tick wait.
- `fade_linewipe`: outer countdown loop.
- `fade_starburst`: outer countdown loop.
- `fade_blister`: outer countdown loop; an all-zero bitmap performs no plots.
- `display_boundary_fade_and_pan`: Auto countdown/H-hold loop replaces DOS idle.

## Exit state and integration

- Ordinary dispatched effects leave display start at zero and line compare off.
  Gas Cloud may deliberately leave page 2000 selected and display start 401;
  its repeat return lets the display-boundary code finish the page flow.
- `animate_vertical_display_start` ends at the requested physical row. Raw split
  animation ends at 0 in reverse, 349 EGA or 399 VGA forward. It deliberately
  leaves that split until the subsequent mode setup; it is not a standalone
  cleanup operation. TARGA+64 bypasses split animation as in DOS.
- Completed boundary clearing resets page/start to zero through mode setup,
  disables line compare, rebuilds/uploads the palette, sets `display_cleared`,
  and resets pan/key/palette elapsed state. Hidden-page preparation instead
  returns in pan state 2 with reduced draw delay. A repeating fade and fatal
  input interruption retain the original early-return state.
- Palette fade buffers become zero after a complete DAC fade; the last direct
  upload may precede that final zero pass. The boundary's explicit zero upload
  guarantees black before rebuilding the palette. The moving-window fade
  changes the selected window, not the whole allocation or framebuffer.
- No platform additions or `dzplat.h` changes are needed. Targeted declarations
  were added to `native/include/dazzle.h` for formerly undeclared DZFD routines.
- DOSI still needs its full input dispatcher and linewipe/ball allocations/map
  initialization integrated. Tests supply a guarded line history and synthetic
  nonzero ball fixture because foundation DOSI defers them. Test-local versions
  of DZLE square/hexagon/rectangle-swap copy the reconstructed algorithms; the
  real DVDT kernels, DOSI palettes/cycling and DZLE line/color code are exercised.
  No other porter's files were edited.

## Verification

- `python -B -I tools/native.py test`: all five CTest entries pass; DZFD takes
  approximately four seconds headless. CMake was configured once with
  `-DDAZZLE_CONVERSION_WARNINGS=ON` and retains that setting. The completed DZFD
  source and test compile without conversion warnings.
- `python -B -I build/workers/p-dzfd/sanitize.py`: independently compiles the
  actual DZFD test/source with GCC `-fsanitize=undefined,bounds,bounds-strict,float-cast-overflow`
  and `-fsanitize-undefined-trap-on-error`; exit 0. No sanitizer runtime library
  is installed, so trap mode is used. Existing core dependency objects are not
  instrumented by that extra build. Palette, bitmap and history write canaries
  are checked in every fixture; EGA indexed memory remains below 16.
- Every eligible keyed fade completes twice with identical framebuffer, active
  palette, BIOS tick, RNG, cleared-state and repeat-state receipts in VGA 400,
  VGA 200, EGA 700 software rows and EGA 350 software rows. A watchdog bounds
  each fixture below 100000 virtual BIOS ticks; baseline maximum is 6828.
- Additional tests cover multiple fireworks/firefly/blister seeds, striped
  Firefly erase, both wipe branches, N interruption of every eligible fade,
  static-palette waits, an empty ball bitmap, palette windows, H held beyond its
  initial two-tick pause then released by N, P hidden-page drawing/reveal,
  Semi-Auto P behavior, show-duration expiry, and both pan/split directions in
  all four modes.
- Receipt aggregates (FNV-1a over ordered little-endian 64-bit fields: key,
  framebuffer, palette, ticks, RNG, cleared, repeat) are pinned in the test:
  VGA 400 `5d408dba`, VGA 200 `8466aea4`, EGA 700 `7c84ddd8`, EGA 350 `385cbfbe`.
- These are native regression hashes, not DOS differential matches. A future
  DOS-reference run should check fixed-point/float truncation, ball-map fixture
  replacement and whole-program input/page integration.

## Seed-1 per-effect receipts

FNV-1a covers all 524288 indexed bytes. VGA palette hashes cover 768 DAC bytes;
EGA palette hashes cover the 16 active attributes followed by the border byte.
The drawn source fixture, palette, RNG setup and seed are specified in the test.
The synthetic ball fixture is part of these hashes. P is tested as orchestration
above rather than dispatched as an ordinary effect. EGA excludes G/J/U/V/Z just
as the original dispatcher does. The original manual does not enumerate all
fade names; the reconstructed dispatch/table identifies them.

| Mode | Key | Framebuffer | Palette | Ticks | RNG | Cleared | Repeat |
|---|---|---|---|---:|---:|---:|---:|
| VGA | a | `7e07e8df` | `4036d113` | 868 | 13 | 0 | 0 |
| VGA | b | `3c890b72` | `208c10c2` | 288 | 41 | 0 | 0 |
| VGA | c | `cc172fb2` | `940c0276` | 2503 | 14 | 0 | 1 |
| VGA | d | `c8ded750` | `20b775c2` | 378 | 645 | 0 | 0 |
| VGA | e | `f0012faf` | `2fab7207` | 858 | 14 | 0 | 0 |
| VGA | f | `6abc9dc5` | `e6591729` | 1829 | 15 | 0 | 0 |
| VGA | g | `b73fe267` | `ae816a9c` | 444 | 15 | 0 | 1 |
| VGA | i | `c31c4adb` | `24f181ff` | 596 | 14 | 0 | 0 |
| VGA | j | `fac5dc59` | `ef287265` | 89 | 14 | 1 | 0 |
| VGA | k | `e3308116` | `30a41cdd` | 2103 | 15 | 0 | 0 |
| VGA | l | `bd6a753a` | `992e215b` | 139 | 107 | 0 | 0 |
| VGA | m | `faadb4c5` | `869b79a7` | 208 | 16 | 0 | 0 |
| VGA | o | `b84a4c81` | `4198f343` | 601 | 4715 | 0 | 0 |
| VGA | q | `9e1ecb15` | `e3c65556` | 309 | 125 | 0 | 0 |
| VGA | r | `d1d9e09e` | `208c10c2` | 131 | 15 | 0 | 0 |
| VGA | s | `78d4739b` | `208c10c2` | 102 | 596 | 0 | 0 |
| VGA | t | `2bb172ad` | `48834d4d` | 323 | 15 | 0 | 0 |
| VGA | u | `038d5d45` | `899f5699` | 208 | 15 | 0 | 1 |
| VGA | v | `59d25545` | `9bba645d` | 130 | 17 | 0 | 1 |
| VGA | w | `4c79f1c5` | `bacc47e7` | 112 | 15 | 0 | 1 |
| VGA | x | `1f977f9f` | `208c10c2` | 795 | 1015 | 0 | 0 |
| VGA | y | `2d75d5c5` | `3912b3bd` | 1442 | 24 | 1 | 0 |
| VGA | z | `6abc9dc5` | `1dbe15db` | 892 | 15 | 1 | 0 |
| VGA/T | a | `06de7ed8` | `1f90d626` | 356 | 13 | 0 | 0 |
| VGA/T | b | `f747faaf` | `208c10c2` | 278 | 61 | 0 | 0 |
| VGA/T | c | `27dbeade` | `b11404b1` | 1254 | 14 | 0 | 1 |
| VGA/T | d | `7796770b` | `239eb64c` | 189 | 645 | 0 | 0 |
| VGA/T | e | `75d6037b` | `ea700d22` | 542 | 14 | 0 | 0 |
| VGA/T | f | `6abc9dc5` | `e6591729` | 1829 | 15 | 0 | 0 |
| VGA/T | g | `e18ddab6` | `f9b54e84` | 202 | 13 | 0 | 0 |
| VGA/T | i | `cc8ac339` | `40c58ec5` | 388 | 14 | 0 | 0 |
| VGA/T | j | `3f7f700f` | `ef287265` | 89 | 14 | 1 | 0 |
| VGA/T | k | `f9791836` | `ea3b5f81` | 893 | 15 | 0 | 0 |
| VGA/T | l | `b13267af` | `992e215b` | 139 | 117 | 0 | 0 |
| VGA/T | m | `03859a45` | `4a44906a` | 154 | 16 | 0 | 0 |
| VGA/T | o | `9955b7bb` | `2d7658ab` | 524 | 8883 | 0 | 0 |
| VGA/T | q | `21894826` | `e3c65556` | 309 | 235 | 0 | 0 |
| VGA/T | r | `56386b93` | `208c10c2` | 84 | 15 | 0 | 0 |
| VGA/T | s | `42b396ed` | `208c10c2` | 102 | 716 | 0 | 0 |
| VGA/T | t | `478420de` | `07d0543c` | 282 | 15 | 0 | 0 |
| VGA/T | u | `44b2d92d` | `899f5699` | 116 | 15 | 0 | 1 |
| VGA/T | v | `7b29fc85` | `9bba645d` | 88 | 17 | 0 | 1 |
| VGA/T | w | `7ee347c5` | `125e7107` | 56 | 15 | 0 | 1 |
| VGA/T | x | `d0f573a4` | `208c10c2` | 617 | 1355 | 0 | 0 |
| VGA/T | y | `1a93ba05` | `97b55535` | 568 | 24 | 1 | 0 |
| VGA/T | z | `6abc9dc5` | `2f85d351` | 474 | 15 | 1 | 0 |
| EGA | a | `3aeace17` | `fc6c2a1a` | 2013 | 8 | 0 | 0 |
| EGA | b | `8540ab40` | `86fd89f1` | 309 | 20 | 0 | 0 |
| EGA | c | `618a4005` | `5000a512` | 4311 | 9 | 0 | 1 |
| EGA | d | `599c0de0` | `d3b43582` | 1270 | 1270 | 0 | 0 |
| EGA | e | `c4709bc1` | `8dc63ea5` | 2394 | 9 | 0 | 0 |
| EGA | f | `6abc9dc5` | `56f560c0` | 6828 | 9 | 0 | 0 |
| EGA | i | `3736bc47` | `6f26e133` | 829 | 9 | 0 | 0 |
| EGA | k | `4a435b75` | `fcc2ad79` | 6381 | 10 | 0 | 0 |
| EGA | l | `1b9912d1` | `3c6aff6a` | 224 | 127 | 0 | 0 |
| EGA | m | `bcb39345` | `74f40af7` | 465 | 11 | 0 | 0 |
| EGA | o | `6abc9dc5` | `913781ef` | 351 | 1565 | 0 | 0 |
| EGA | q | `e84d5012` | `70e524d3` | 312 | 195 | 0 | 0 |
| EGA | r | `c42c1f15` | `fa841b18` | 628 | 10 | 0 | 0 |
| EGA | s | `49101fe5` | `fa841b18` | 172 | 801 | 0 | 0 |
| EGA | t | `ebeca9bd` | `6f26e133` | 358 | 10 | 0 | 0 |
| EGA | w | `dc8b5ac5` | `70e524d3` | 191 | 10 | 0 | 1 |
| EGA | x | `473b117c` | `fa841b18` | 2039 | 551 | 0 | 0 |
| EGA | y | `673c87c5` | `56f560c0` | 4998 | 19 | 1 | 0 |
| EGA/T | a | `4fa6682a` | `6f26e133` | 950 | 8 | 0 | 0 |
| EGA/T | b | `60715c41` | `d443ea1a` | 280 | 26 | 0 | 0 |
| EGA/T | c | `c8fe05c5` | `5000a512` | 4311 | 9 | 0 | 1 |
| EGA/T | d | `3aeccfc6` | `72a6ebf7` | 635 | 1270 | 0 | 0 |
| EGA/T | e | `3253e82a` | `c80dae90` | 2005 | 9 | 0 | 0 |
| EGA/T | f | `6abc9dc5` | `56f560c0` | 6828 | 9 | 0 | 0 |
| EGA/T | i | `44b9f5a1` | `86fd89f1` | 553 | 9 | 0 | 0 |
| EGA/T | k | `1f97f0e5` | `05253116` | 4305 | 10 | 0 | 0 |
| EGA/T | l | `1b3613b2` | `3c6aff6a` | 224 | 127 | 0 | 0 |
| EGA/T | m | `41a912c5` | `d443ea1a` | 288 | 11 | 0 | 0 |
| EGA/T | o | `6abc9dc5` | `99f1b05a` | 353 | 2599 | 0 | 0 |
| EGA/T | q | `28725986` | `70e524d3` | 312 | 195 | 0 | 0 |
| EGA/T | r | `0a1c2ec3` | `fa841b18` | 522 | 10 | 0 | 0 |
| EGA/T | s | `0986e3d4` | `fa841b18` | 172 | 1045 | 0 | 0 |
| EGA/T | t | `6da0b314` | `ec6f5a50` | 335 | 10 | 0 | 0 |
| EGA/T | w | `dc8b5ac5` | `70e524d3` | 191 | 10 | 0 | 1 |
| EGA/T | x | `39ff556c` | `fa841b18` | 1731 | 699 | 0 | 0 |
| EGA/T | y | `6abc9dc5` | `cb34f7cc` | 1991 | 19 | 1 | 0 |
