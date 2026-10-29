# Original-program behavioural acceptance (2026-10-29)

The requested flows pass the checks below. This is behavioural acceptance, not a
binary match or a claim that two clock-seeded drawing runs have identical pixels.

Reference: an immutable copy of `D:/Prog/dazzle_recon/assets/DAZZLE.EXE`, SHA-256
`fc68905570e0f139cafc0bc0f75af87445c6fb29f53f5f02018fa9b71f714ee8`.
It ran without `/I` under DOSBox-X v2026.08.31, normal core, 486, fixed 26800
cycles, `svga_s3`, US keyboard. The pinned acceptance executable has SHA-256
`e5b9a81acb208bab48cb01a7dac9691cd06674fac877abce54e4693ece6dbff9`.
The stock `Release SDL2/dosbox-x.exe` also completed the F1/PgDn/PgUp/ESC flow
using `autotype -w 5 -p 1 f1 pagedown pageup esc esc` (`stock-help/`).

All paths below are relative to `build/workers/n-accept/`. Each DOS run contains
its configuration, timed input script, event log, AVI captures, screenshots,
80x25 character/attribute dumps and a receipt. Native runs contain their key
scripts, indexed snapshots, rendered PNGs and stdout/stderr. DOS timestamps use
emulator time; native timestamps use application virtual time. DOSBox's first
captured frame follows roughly 1.01 seconds of startup. AVI extraction accounts
for that offset; menu comparisons use the independent B800 dumps at the stated
input checkpoints. Graphics were checked for flow and effects at corresponding
key checkpoints. DOSBox doubles VGA pixels horizontally in its captures.

| Scenario | Evidence: original / native | Verdict |
|---|---|---|
| 1. Startup, Auto, `/?`, `/$` | `help/dos/t2.png` / `help/native/t1.png`, then `t4`; `cli-help-visible/`, `cli-order-visible/`, `cli-order-text/dos/OUTPUT.TXT` | PASS: loading text leads into Auto; help wording on both pages agrees; the complete 1,783-byte order-form text agrees after CR/LF normalization, including IDs 00500/01000. |
| 2. F1 help, PgDn/PgUp, ESC | `help-all/{dos,native}/t4.5` through `t10.5`; `help-markers/dos/t10.png`, `closing-native/logo.snap`, `stock-help/` | PASS after the text correction below: all five help pages have identical 4,000-byte cell buffers; PgUp returns to the preceding page, F1 resumes drawing, ESC exits through the logo and closing text. |
| 3. F2 editors, File Access, ESC/F10 | `config-walk/`, `config-edit/`, `config-load-verify/`, `config-{1..5}-{esc,f10}/` | PASS: both image lists, fades, Auto limits/pause, save and reload work; the edited 2,423-byte MTDAZZLE.DZL files agree exactly; every panel's ESC quits the program and F10 returns to drawing. File Access displays each platform's actual path. |
| 4. Drawing keys and closing | `drawing/`, `semi-auto/`, `pcx-{clear,ega,tall}/`, `help-markers/dos/t21.png` / `closing-native/final.png` | PASS for flow: N clears, Space resumes, Tab freezes/toggles cycling, letter fades/P/H, +/- and PgUp/PgDn, Shift/Ctrl+F10, F5 and Alt+F9/Alt-image/F8/F9/F10 work. ESC shows the logo then registration text. PCX details are below. |
| 5. Options | `option-{bashful,show,window,quiet,alternate,mad,fixed,ega,tall,delay,config}/` | PASS for observed flow: /B's F1 exits without credits; /S=3 draws once and exits; /W hides startup messages; /Q retains visuals; /A, /M and /F change palette behavior; /E and /T select their resolutions; /D=2000 slows drawing; CUSTOM.DZL is loaded. |
| 6. Mouse with a driver | `mouse-motion/dos/` / `mouse-motion/native-present/`; `mouse-click/dos/` / `mouse-click/native-present/`; `mouse-bashful-motion/dos/` / `mouse-bashful-motion/nm/` | PASS: ordinary Auto ignores motion, a button press starts closing, /B includes motion and exits immediately without credits. |

## Correction and regression coverage

- **Help page 5:** native omitted “far” in “Please refer to DAZZLE.DOC for far
  more details.” `help-all/dos/t8.5.text` and `src/menu.c` establish the original
  wording. Restored the word in `native/src/app/menu.c`; all five complete help
  cell buffers now agree. No change to ESC: the original deliberately exits the
  program there. F1 is the documented help-return key.
- **Text PNG conversion:** `native/tests/snapshot_png.py` formerly rejected text
  mode. It now uses the same CC0 CP437 font as the native renderer, 720x400 output,
  character attributes, blinking and ninth-column box joins. Snapshots contain no
  cursor shape/position, so exported PNGs omit the hardware cursor. Unscii glyph
  shapes differ from DOSBox's BIOS font; the menu characters and attributes agree.
- **Tests:** `acceptance-reference.json` records hashes derived from original
  B800 dumps, not native builds. `acceptance_reference` exercises the actual
  executable's five help pages, paging/return, four configuration panels, CLI
  help and order text. `test_menu` covers ESC/F10 for every panel and ESC in help.
  `test_acceptance` drives the real SDL mouse event handler and INT 33h mask;
  its optional `--capture-mouse motion|click|bashful ORIGINAL.EXE` runs the whole
  application with an event at virtual second 5. It also checks that F10 release
  raises the pending IRQ notice without enqueuing another BIOS word, stopping
  Semi-Auto drawing. `snapshot_png` checks glyphs,
  attributes, blink, box joins, VGA and EGA conversion.

## PCX comparison and limits

| Saved mode | Original / native bytes | Header result |
|---|---:|---|
| VGA 320x400, drawing (`drawing/*/*0001.pcx`, case-insensitive) | 102924 / 43247 | All 128 header bytes agree: 8 bpp, 1 plane, 320 bytes/line. |
| VGA 320x400, cleared (`pcx-clear/*/*0001.pcx`) | 5702 / 5697 | All 128 header bytes agree. Decoding gives two residual original pixels versus a completely clear native page. |
| EGA 640x350 (`pcx-ega/`) | 62565 / 117702 | 1 bpp, 4 planes, 80 bytes/plane/line; non-palette header fields agree. RGB palette entries differ between the random runs. |
| VGA /T 320x200 (`pcx-tall/`) | 28114 / 23399 | All 128 header bytes agree: 8 bpp, 1 plane, 320 bytes/line. |

Compressed lengths depend on the pixels and are not equality assertions. The
existing native PCX writer also deliberately omits the original's indeterminate
769th VGA palette byte and defines its row-buffer overread as zero (see
`dazpcx.md`). This acceptance does not certify PCX file byte equality. The decoded
native closing logo was separately compared with `tools/logo.py` and agrees for
all 224,000 indexed pixels. Audio fidelity was not compared; /Q's visual flow and
mute path were checked, with the existing DOSI sound tests retained.
Native key scripts enqueue BIOS words, without physical key-release events;
F10 therefore keeps drawing until the next scripted input. Actual SDL release
handling is covered separately; these script captures do not certify held-key
timing. The original capture scripts include explicit presses and releases.

## Reproduction, negatives and cleanup

- `python -B -I tools/native.py test`: **25/25 pass**, with conversion warnings
  enabled and `DAZZLE_ORIGINAL_EXE` set to the scratch copy; real logo/manual
  resource checks ran. `build/workers/n-accept/test.log` records the run. The
  warnings are three existing intentional fallthrough warnings in DZLE.
- `python -B -I build/workers/n-accept/accept.py CASE...` reruns scratch captures;
  cases and scripts remain in that directory. `stock-help/dosbox.conf` reproduces
  the stock AUTOTYPE check. All launched DOSBox-X and native processes were reaped.
- Initial `pgdn` mapper names were rejected: DOSBox uses `pagedown`/`pageup`.
  The corrected runs supersede that attempt. Initial `keyboardlayout=auto`
  inherited Czech layout; the original read scan/ASCII `06FD` for the physical 5
  key and ignored it in a numeric field. US layout gives exact edited config
  equality. Neither issue justified a native code change.
- Initial concatenated AVI extraction used a wrong startup offset and could show
  the next help page; corrected PNGs agree with the timed raw text dumps. A stale
  preview also suggested missing logo art; fresh rendering and pixel comparison
  disproved it. The first /B mouse capture used an 85-character current path and
  hit the inherited 80-byte getcwd limit; the valid shorter-path rerun is `nm/`.
