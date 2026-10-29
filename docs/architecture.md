# DAZZLE 5.0 architecture (startup worker)

Addresses are load-image `segment:offset`; linear = `16*segment+offset`. `DS:` means `29BF:`. Names are proposed, not recovered symbols or accepted byte matches. Static evidence is `reference/load_image.bin` and its relocation stream. Reproduce individual listings with `python tools/disasm.py ADDRESS --func`; scratch listings, probes, recordings and CRC transcription are in `build/workers/startup/`. Application and RTL names are separated in `evidence/names-startup.json`.

## Startup and subsystem call graph

```text
0000:0000 C0 startup [RTL]
  establish DS=29BF, PSP/environment, heap/stack; zero DS:482C..78C2
  0000:01EA initializer dispatcher (table DS:47FC..4826)
  066A:4D3D main                                  [linear 0B3DD]
    argv[0] -> DS:5D17; gettextinfo; register atexit handlers
    11C1:1F29 initialize defaults; 11C1:1D64 build math lookup tables
    1899:0932 save text/video state; 1899:0959 detect display adapter
    11C1:2EED parse arguments                     [14AFD]
      /? -> 11C1:0BED option help -> exit(10)
      /$ -> 11C1:0949 metadata check -> 11C1:0AB6 order form -> exit(0)
      /Z -> 1A72:11BB extract manual -> exit(501), DOS low byte F5h
    11C1:339F print startup information
    11C1:0949 check executable / load serial metadata [12559]
      11C1:0861 deRead -> fopen/fclose, 11C1:05C3 MZ-end calculation
        11C1:066C genCRC                          [1227C]
      11C1:052F report fatal check error -> exit(100)
    11C1:21A6 configure video dimensions, palette storage and backend pointers
    11C1:2D26 resolve/load configuration -> 11C1:29AE parse config file
    11C1:00C9 install input/timer callbacks; register 11C1:01C7 cleanup
    17F5:06EF prepare embedded logo/image resources [1863F]
    clear/initialize backend; srand((unsigned)time(NULL))
    while (!DS:5D80):
      DS:5DA0 != 0 -> 11C1:3539 Semi-Auto loop     [15149]
      DS:5DA0 == 0 -> 11C1:38C3 Auto loop          [154D3]
        selected image through far pointer DS:5C4B
        0CCD:4A82 display-boundary/fade orchestration [11752]
        11C1:18E7 process pending keyboard/mouse input [134F7]
        11C1:2515 cycle palette if timer threshold reached
    restore text mode via 1899:0BCD
    conditional 11C1:2161 closing graphic; 11C1:0D46 closing text / exit
  0000:11A8 exit [RTL] -> cleanup/terminators -> DOS INT 21h/AH=4Ch
```

This is a far-code/far-data application: `main` receives a far `argv`, library string/file pointers occupy four bytes, image callbacks are far pointers, and functions use RETF. That supports the **large/huge-model ABI**, rather than medium alone; it does not identify the exact compiler release or compilation flags. Borland's software floating-point `INT 34h..3Dh` sequences occur in the mathematical setup; treating them as ordinary interrupts/instructions loses the instruction stream. Runtime identity and matching remain the runtime/compiler workers' responsibility.

The program itself prints segment labels in its signal diagnostic (`11C1:02D8`): DZLE=`066A`, DZL2=`0B84`, DZFD=`0CCD`, DOSI=`11C1` (current CS), DVDT=`1899`, DHUF=`1A72`, DLOGO=`17F5`, DAZPCX=`19F0`. The `158D` segment contains the interactive menus. These are useful module boundaries, not a recovered linker map.

## Command line

`11C1:2EED` obtains DOS SwitchChar (`0000:12AD`), accepts it or `-`, normalizes the option letter through RTL `tolower` (`0000:1F66`), and walks concatenated switches. CS:`3333` holds 27 word keys, followed at CS:`3369` by their word case destinations. This accounts for all 24 manual options, plus `?`, an undocumented `Y`, and a consumed `-` separator. `/I` stores the argument index at DS:`5DC8` and the character index through an output parameter; `main` rechecks that character before trusting the bypass.

| Manual option | Case in `11C1:` | Direct effect / destination |
|---|---|---|
| `$` | `2FC4` | Order form, metadata-only check, exit |
| `2` | `2FF5` | DS:5DD8=1, split palette updates |
| `A` | `3007` | DS:5DE0=1, Alternate |
| `B` | `3010` | DS:5DCA=1, Bashful |
| `C` | `3019` | DS:5DB4=1, BIOS palette uploads (plotting remains direct; graphics worker) |
| `D[=n]` | `3022` | DS:5DC6=256 bare; numeric parse, cap 8191 |
| `I` | `3107` | DS:5DC8=argv index, suppress CRC generation/comparison |
| `M` | `311D` | DS:5DE0=0, DS:5DDA=1; clears Alternate at this point |
| `N` | `312C` | DS:5DCC=1, suppress disk-head park |
| `Q` | `3135` | DS:5DBE=1, suppress programmed tones |
| `S[=n]` | `313E` | DS:5DCE=1; seconds stored as 18*n ticks in DS:5D86; cap 3600 |
| `W` | `3212` | DS:5DBC=1; bypass self-check in main, suppress messages/LEDs |
| `X` | `321A` | DS:5DD4=1; CPU probe `1899:0FF5` can set it automatically |
| `Z` | `322A` | `1A72:11BB`, manual extraction |
| `E` | `30EC` | DS:7480=1, force EGA |
| `H` | `30FE` | DS:5DDE=1, hide EGA border |
| `K` | `3114` | DS:5DD2=1, retain typematic |
| `T` | `3202` | DS:5DE4=1, BIOS/tall aspect |
| `+` | `2FEC` | DS:5DC2=1, ignore TARGA option flag |
| `!` | `2FE3` | DS:5CB7=1, monitor screen-saver interface |
| `4` | `2FFE` | DS:5DD0=1, four EGA colors |
| `F` | `30F5` | DS:5DE2=1, fixed palette |
| `V` | `320A` | DS:7482=1, force VGA |
| `*` | `3239` | Return -1, suppress default config load; bare `*` takes the filename path |

`/?` enters `0BED`; unknown switches also print help. Hidden `/Y` only sets DS:5DDC at `3222`; no consumer was found in the scratch static listing, so no behavior is assigned. A/M precedence is order-dependent in the parser: `-am` leaves Mad set and Alternate clear; `-ma` leaves both flags set. No special two-switch arbitration was found.

## Configuration and menus

`11C1:2D26` resolves DS:5C5F (80-byte input filename), initially `MTDAZZLE.DZL` (string DS:200F, default pointer DS:5FFA). It handles drive/path changes, extension completion and loading; the sentinel return -1 skips the load. `11C1:0D85` uppercases a filename and rejects `.EXE`, `.BAT`, `.COM` (substring tests); wildcard directory handling is in `158D:1701` and the file menu `158D:1BAE`.

`11C1:29AE` opens text input, reads with `fgets(...,80,...)` into a local buffer, truncates at `;`, skips leading spaces/tabs, and lowercases the remaining line. Its five-key switch at `11C1:2B30` dispatches `#`, `~`, `^`, grave accent, `$`:

| Parser | Meaning and storage |
|---|---|
| `11C1:256E` | `#` unsigned version; warns below 10410; malformed version may return failure |
| `11C1:25D6` | `~` image key + bracketed size; search 36 Alt records |
| `11C1:26CA` | `^` image key + bracketed size; search 6 Ctrl records |
| `11C1:27BE` | Ordinary fade key and bracketed Y/N; search 24 fade records |
| `11C1:28BE` | `$I` -> DS:5CC5 minimum; `$X` -> DS:5CC3 maximum; `$F` -> DS:5DA8, multiplied by 18 |
| `11C1:2B44` | Write configuration using `#10500`, `~%c [%03d]`, `^%c [%03d]`, `%c [%c]`, and `$I/$X/$F` formats |

Unknown/malformed non-version entries can be ignored; this is not a strict grammar validator. The grave-accent dispatch jumps straight to the next input line (`29D8`), applying no entry; do not infer support for arbitrary Shift-bound images from the manual's token description.

F1 opens `158D:05DC` (help), using page pointers DS:24B8..24DC and text starting DS:2535. F2 opens `158D:2256` (configuration shell); descriptor pointers DS:24E5..2509 select the image editor (`158D:0C7C`), fade editor (`158D:1050`), Auto customization (`158D:1381`), or File Access (`158D:1BAE`). The latter calls config load/save on F4; its directory path is `158D:1701` (F3). `158D:07A7` handles menu navigation and `158D:0219` renders text. DS:249E/24A0 identify active help/config menus; DS:24A2 is tested to prohibit certain actions while menus are active. File status literals EXIST/READ ONLY/NOT FOUND are DS:398A/3990/399A. These menu entry identifications are static, not a full interactive key-by-key acceptance test.

## Main state, input, timing, sound

Auto (`11C1:38C3`) clamps min>=1 and max>=min, chooses a random image count in that inclusive interval, selects from enabled/nonzero-size image records, adjusts overlays for Alternate/Mad and display state, then draws through DS:5C4B. Palette cycling and pending input are serviced between drawing steps. `0CCD:4A82` owns screen boundaries, pause/fade handling and calls into the fade subsystem; DS:5DA8 supplies the Auto pause. Show-mode timeout is decremented by the timer and acted on at display boundaries. Semi-Auto (`11C1:3539`) converts normalized Alt/Ctrl selections to the same image pointer, handles F8 continuous mode (DS:5D9E), F9 random choice, F10 draw/resume, and Alt+F10 reinitialization. DS:5DA0 selects Semi-Auto; zero is initial Auto.

`11C1:18E7` is the common input dispatcher. It checks pending mouse abort, exit, and `/B` before interpreting keys; thus **any pending key under `/B` exits**. It polls `bioskey(1)` at RTL `0000:2553`, consumes a key through `0000:2578`, lowercases ASCII, and normalizes Alt top-row and Ctrl-letter selection codes. Tables `11C1:1D44` (ASCII) and `11C1:1CFC` (extended BIOS keys) lead to:

| Feature | Implementation |
|---|---|
| ESC, ENTER | `1A3F` / `1A25`; DS:5D80 exit request (ENTER observes menu/startup conditions) |
| TAB, SPACE, N | `1A06`, `1A11`, `1A16` -> `11C1:1830` freeze/clear state; TAB also switches palette cycling |
| F1/F2 | `1B01` / `1B10` -> the menu entry points above |
| F5 | `1B29` -> `19F0:060B` PCX save (blocked in startup/menu contexts) |
| +/- | `1994` / `19D4` adjust DS:5DA6 cycle interval |
| PgUp/PgDn | `1BA9` / `1BEB` adjust VGA DS:5DA4 step, with boundary tones |
| Shift+F10 | `1C57` calls selected palette generator and uploads current palette window |
| Ctrl+F10 | `1C27` -> `11C1:134A` changes background/reserved palette mappings |
| Alt+F9 | `1CA8` toggles DS:5DA0, queues clear (`n`), and plays mode tone |
| Fade keys, P/H | `0CCD:4715` selects/enables fades; `4AC4..4B43` rejects pan for incompatible state or Semi-Auto and clears consumed P. At `4CA7..4CAE`, H continually reloads pause countdown to 7 ticks; N forces zero. Semi-Auto's command table treats H as a no-op (`11C1:387B`). |

`11C1:00C9` saves vectors with RTL getvect and installs **INT 1Ch**=`1899:14C7` and **INT 9**=`1899:154C`. INT 9 increments DS:5D7E before invoking the saved BIOS handler; INT 1Ch increments DS:5D7A, decrements countdown DS:5D78 and show time DS:5D86, periodically notices queued BIOS keys, then chains to the saved timer vector. `1899:155F` is a mouse callback setting DS:5D7E/5D7C. `11C1:0501` registers it through INT 33h/function 0Ch; `11C1:0442` probes/reset-detects the mouse. `11C1:01C7` restores vectors, Ctrl-Break state, mouse and typematic state on exit.

LED control is `11C1:000A`: it edits BIOS keyboard flags through far pointer DS:0C02=`0040:0017`, with masks Scroll=10h, Num=20h, Caps=40h, and invokes BIOS keyboard service. `/W` and `/B` cause early return. Timer/countdown and fade paths select solid/blinking Scroll Lock; fade/palette paths use Num/Caps indications. There is no need to assume a separate raw keyboard-controller LED command routine for this helper.

`11C1:0E07` is the `/Q`-guarded tone helper: RTL `sound` (`0000:3D88`), `delay` (`0000:29A6`), `nosound` (`0000:3DB4`). Mode-entry chirp is `11C1:006F`; another tone sequence is `11C1:1FEF`. Warning/error output has other beep paths. Drawing delay is DS:5DC6, initially 1, bare `/D`=256; the drawing/fade subsystem consumes it. RNG is Borland `srand`/`rand` at `0000:1683/1694`, seeded from `time(NULL)` (`0000:1EF6`) at `main`'s `5033..503B`; DS:461A is RTL random state. Timer-based palette cycling is gated by DS:5D7A >= DS:5DA6 in `11C1:2515`.

## Data structures and globals

| DS offset | Meaning / direct evidence |
|---|---|
| `01FE` | 36 Alt-image records, stride 15; `041A` is 6 Ctrl-image records. Record +0 far draw function, +4 size, +6 far name string, +10 word (initially 1), +12 word (initially 0), +14 ASCII key. Last two word meanings remain provisional. Config parsing writes +4 and compares +14; Semi-Auto loads +0. |
| `0474` | 24 fade records, stride 7: +0 ASCII key, +1 far name string, +5 enable word. `27BE` sets +5. |
| `5C33..5C4A` | Far callbacks installed by `21A6`: display origin (`5C33`), palette upload (`5C37`), read pixel (`5C3B`), symmetric plot (`5C3F`), set mode (`5C43`), palette generator (`5C47`). Detailed signatures are in the graphics worker's `docs/graphics.md`. |
| `5C4B` | Current far image function; initialized from first record by `1F29`, changed by mode loops. |
| `5C5F`, `5CC7`, `5D17` | 80-byte config filename, saved working directory, executable pathname respectively. |
| `5CC3/5CC5` | Maximum/minimum images (defaults 9/3); config `$X/$I`, Auto clamps them. |
| `5D68` | 32-bit executable resource base (MZ end +1). |
| `5D70/72/74` | Skip Ctrl-Break control (no writer found) / keyboard-hook / timer-hook flags; `5DEA` saves original Ctrl-Break setting; `5FF2`/`5FF6` saved INT 9/1Ch far vectors. |
| `5D78/7A/7C/7E` | Countdown; elapsed palette ticks; mouse abort flag; pending-input counter. |
| `5D80/82/84/86/8E` | Exit request; saved freeze/LED state; startup-config failure; show ticks remaining; startup-active flag. |
| `5D90/92` | Previous/current normalized key/command words. |
| `5D94` | Far palette buffer; `5DEC` current cycling offset. |
| `5D98/9A/9C` | Fade/Ctrl-image/Alt-image counts: 24/6/36 (written by main). |
| `5D9E/5DA0/5DA4/5DA6/5DA8` | Continuous draw; Semi-Auto flag; VGA palette step; cycle interval; Auto pause ticks. |
| `5DB0/5DB2` | Half-width/half-height; VGA setup sets 160 and 100 or 200. |
| `5DBC/5DBE/5DCA/5DCE` | Window/quiet/Bashful/Show flags; remaining option fields are in the option table. |
| `5D88` | Maximum accepted image-size value, used by both image config parsers. |
| `6021/6027/602B/602D` | Computed CRC32-state / expected CRC / serial IDs r1/r2. |
| `7480/7482/7484` | Forced EGA/forced VGA/detected adapter code; code 8 selects VGA branch. |

## Integrity check

The normal unpacked-file failure **is an on-disk CRC mismatch**, not a failure to reach C0/main. `11C1:0949` resolves the running executable path, warns on a different basename, calls `deRead`, and compares DS:6027..602A against DS:6021..6024 at `0A77..0A88`. A mismatch calls `11C1:052F`, prints the antiviral error and `bad ckval` reason, then exits **100**. Renaming alone produces a warning, not this abort.

`11C1:05C3` computes `base=(e_cp-1)*512+e_cblp+1` from the first three MZ words. `11C1:0861` opens that executable in binary mode, optionally computes the CRC, seeks to `base`, and reads the ten-byte metadata frame. The expected checksum is its first little-endian dword, loaded into DS:6027; r1/r2 follow. For the original, `base=0x13B02`, expected=`C7BD6D00`, r1=500, r2=1000, and the trailing word is `image_data_offset=0x8802`. The DLOGO loader at `17F5:000B` adds this offset to `base+10` before reading the embedded image header; the value is four length bytes plus the 34,814-byte LZH stream. The preceding byte at file offset `0x13B01` is `84` and is outside the CRC region.

`genCRC` (`11C1:066C`) builds a 256-entry dword table on its stack from polynomial **00102100h** (DS:0C06), uses an 8192-byte heap buffer (halving down toward 512 if allocation fails), seeks to file position zero and processes **[0,base-1)**. Its last read count becomes zero at `base-1`; therefore it covers precisely the standard packed MZ extent, including header and relocation area, excluding the appended tail. Initialization=0, non-reflected, no final XOR:

```c
T[i] = i << 24;  // repeat 8 times:
T[i] = (T[i] << 1) ^ ((old_T & 0x80000000) ? 0x00102100 : 0);
crc = (crc << 8) ^ T[(crc >> 24) ^ byte]; // uint32_t wraparound
```

This is the executable's particular 32-bit CRC-state algorithm, not standard reflected CRC-32. Independent Python transcription (`check_crc.py`) and guest-memory observations agree:

| Input | Covered bytes | Computed | Expected | Result |
|---|---:|---|---|---|
| Immutable packed EXE | 80,641 (`13B01h`) | C7BD6D00 | C7BD6D00 | Pass |
| Unpacked MZ | 211,302 (`33966h`) | 223ECE00 | C7BD6D00 | Fail |

`/I` sets DS:5DC8: `deRead` still finds/loads metadata but skips `genCRC`, and `0949` skips the comparison. `/W` (DS:5DBC) or `/S` bypasses `0949` in main altogether (`066A:4E54..4E69`); the manual explicitly documents I/W. The `/S` bypass is additional static evidence, not a manual promise. `/Z` explicitly resets DS:5DC8 to zero at `1A72:120B` and forces the check even after `/I`; `/$` explicitly selects metadata-only checking.

## Manual extraction, PCX and closing sequences

`/Z` enters `1A72:11BB` (`1B8DB`): after obtaining metadata it opens DS:5D17 as `rb` and DS:4040=`DAZZLE.DOC` as `wb`, recalculates the MZ end, seeks to `base+10`, reads a four-byte uncompressed length, and decodes at `1A72:0E40` (`1B560`). The length is 84,927 and compressed bytes start at `base+14` (packed file `13B10h`). `0E40` calls adaptive-Huffman setup/character/position helpers at `09FE/DC6/DFB`, maintains a 4,096-byte sliding dictionary with a 60-byte lookahead, and writes decoded bytes through stdio. `1A72:1052/118E` report extraction progress. The reconstructed encoder/decoder in `tools/lzhuf.py` reproduces the original 34,814-byte stream exactly. The checksum/serial ten-byte frame is distinct from the following four-byte text length. Successful extraction calls `exit(501)` at `1A72:1395..1399` (DOS receives low byte F5h); the parser's subsequent `exit(3)` is not reached.

F5's `19F0:060B` (`1A50B`) formats DS:3C8B=`daz_%04d.pcx`, searches for an unused number, creates a file and associates a binary stream. It allocates header/row/palette buffers: `19F0:01E2` writes a 128-byte PCX header (manufacturer 0Ah, version 5, encoding 1), `0007` prepares VGA palette data, `037E` reads EGA planes, `0427` handles linear VGA /T rows, and `0488` gathers enhanced VGA planes. `19F0:02A6` encodes rows using PCX RLE run markers (C0h). During save it uses backend origin DS:5C33 to expose the full page and restores it afterward. This identifies the subsystem and mode integration; all PCX modes and wraparound at 9999 have not been dynamically tested.

The appended static logo is a PCX-derived 128-byte header plus planar RLE. Its header declares 640x350 pixels, one bit per plane, four planes, 80 bytes per plane per scanline, a 16-entry RGB palette, total resource size 43,422 (`0xA99E`), and frame count zero. `decode_logo_run` (`17F5:0451`) expands high-bit `11` bytes as a six-bit repeat count followed by a value; other bytes are literals. The decoder starts at byte 128 and emits four 80-byte planes for each of 350 rows. `tools/logo.py` maps those MSB-first plane bits to indexed pixels and writes/reads indexed PNG with the Python standard library. The exact encoder splits runs at 320-byte four-plane row boundaries, uses runs for repeats and literal bytes at or above `C0h`, and reproduces all 43,294 body bytes and the full 43,422-byte resource exactly. See `docs/tail.md` for the palette and byte layout.

`11C1:2161` calls `17F5:051C` for the closing graphic, then the `/Q`-guarded musical sequence and keyboard wait. Main suppresses that path for Show, Bashful, or startup-config failure; `/W` permits the graphic while the closing text (`11C1:0D46`) is suppressed. `11C1:0AB6` prints registration/order information (r1/r2 included when requested); the ten-day requirement is a text string at DS:0532. No elapsed-ten-day enforcement was identified in this startup/control-flow pass; that is not proof of absence in every code/data path.

## Runtime verification and negative results

All DOSBox-X runs used scratch copies named `DAZZLE.EXE`, the pinned acceptance v3 binary (SHA-256 `e5b9a81acb208bab48cb01a7dac9691cd06674fac877abce54e4693ece6dbff9`), SDL dummy video/audio, normal core, 386, fixed 200000 cycles, svga_s3 and 16 MB. Configurations/events/register dumps are under the named scratch run directories; the runner bounds execution and kills on timeout. No compiler/candidate build was used.

- `python -I build/workers/startup/check_crc.py`: original passes, unpacked fails as tabulated above; guest DS:6021/6027 values independently agree.
- `run_probe.py unpacked-normal '-a -m -b -n'`: reaches `0949`, exits at 1399.544 emulated ms with 100; exit stack return `19E4:05C0` normalizes to `11C1:05C0`, immediately after `exit(100)` in the fatal-check routine. Never reaches drawing loop.
- `run_probe.py unpacked-i-only '/I -a -m -b -n'`: reaches metadata checker but does not generate CRC, reaches main loop at 3208.997 ms and exits normally only after injected Escape at 5500 ms. Recording plus extracted `unpacked-i-only/screen.png` show graphics. This tests `/I` without `/W`.
- `run_probe.py packed-normal '-a -m -b -n'`: CRC passes, main loop at 3266.834 ms, normal exit after scripted Escape. `/N` keeps disk parking out of these paired runs.
- `run_probe.py packed-doc '/Z'`: extracted 84,927 bytes, byte-equal to `assets/DAZZLE.DOC`, SHA-256 `53b47422b6977433d54ee92751a2b86a3d253a9b953d458eddedc1320dd1cce5`; observed `exit(501)`. Negative `unpacked-doc '/I /Z'`: extraction clears the bypass and fails CRC with status 100 before creating output.
- `unpacked-i-video`: `/I -a -m -b -w` reaches loop at 1045.386 ms; `screen.png` extracted from DOSBox's `DX-CAPTURE /V` recording shows graphics. Cleanup Escape was sent at 5500 ms.
- **Earlier `/W` failure is not reproduced.** `unpacked-w` with `-a -m -b -w` reaches loop at 1043.767 ms and has exit flag zero at 3200 ms. It exits only after the probe's injected function-key sequence at 4000 ms. Thus CRC explains default startup failure, but cannot explain the earlier `/W` screenshot in `docs/format.md`; that discrepancy remains a capture/launch-state issue to reproduce separately, not evidence of a PKLITE dependency.
- Negative probe: acceptance `on_exec ... dump ds:5c5f ...` is invalid syntax (offset expression must use its supported register/linear forms); that first attempt timed out and was killed. Subsequent probes used linear `37A7F` and verified DS=31E2. Acceptance `keydown f11/f12,p` sends **guest events**, bypassing host screenshot bindings (`MAPPER_AcceptanceKey` calls the key event directly); under `/B` it aborts DAZZLE. Subsequent screenshots use `DX-CAPTURE` and ffmpeg, with no capture keys.

The working `/I` and `/W` runs disfavor entry-register/PSP or mandatory PKLITE-specific dependencies as explanations for normal startup failure. They are not a general proof that all environments or entry states are equivalent. Remaining work is full fade/graphics dispatch recovery, complete menu acceptance, and independent RTL signature confirmation; no matches or coverage have been promoted by this worker.
