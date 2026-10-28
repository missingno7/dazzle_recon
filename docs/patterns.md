# Image patterns and fade dispatch

Static inventory from `reference/load_image.bin`. Addresses use `SSSS:OOOO` with the load-image paragraph as the segment; `linear = 16*segment + offset`. `DS:OOOO` means `29BF:OOOO`. The byte count is the code extent through its `RETF`; helper code or gaps can follow before the next named table entry. Default image size is the configurable image-size value, not code length. Fade entries have no image-size field.

## Image algorithms

The image records are two packed DGROUP arrays. `DS:01FE` contains 36 Alt selections and `DS:041A` contains 6 Ctrl selections, each 15 bytes. All 42 callback-segment words and all 66 image/fade name-pointer-segment words correspond to entries in the MZ relocation list. Record offsets are `+0` far function pointer, `+4` default size word, `+6` far name pointer, and `+14` canonical ASCII key. The key column shows the table character plus its selection modifier; the stored record does not contain the full BIOS scan code. Input processing normalizes Alt/Ctrl keys before matching these records.

### Alt selections (36)

| Name | Selection key | Default size | Entry | Bytes | FP | Description guess |
|---|---:|---:|---:|---:|:---:|---|
| arches | `Alt+1 (record ASCII 0x31)` | 40 | `066A:0CCD` | 153 | No | Arched strokes or nested arch curves. |
| atom | `Alt+2 (record ASCII 0x32)` | 40 | `066A:0D66` | 304 | Yes | An orbiting atomic or electron-like figure. |
| balls | `Alt+3 (record ASCII 0x33)` | 40 | `066A:0E96` | 416 | No | Multiple moving or bouncing circles. |
| bars | `Alt+4 (record ASCII 0x34)` | 25 | `066A:1036` | 373 | No | Repeated horizontal or vertical bar forms. |
| bigstar | `Alt+5 (record ASCII 0x35)` | 5 | `066A:11AB` | 1147 | Yes | A large expanding or contracting star. |
| boxes | `Alt+6 (record ASCII 0x36)` | 75 | `066A:1634` | 419 | No | Nested or moving rectangular boxes. |
| cones | `Alt+7 (record ASCII 0x37)` | 20 | `066A:17DF` | 467 | No | Cone-like radial forms. |
| corkscrew | `Alt+8 (record ASCII 0x38)` | 20 | `066A:19B2` | 432 | Yes | A corkscrew-shaped spiral path. |
| curves | `Alt+9 (record ASCII 0x39)` | 80 | `066A:1B62` | 352 | Yes | Parametric or cyclic curves. |
| cycloids | `Alt+0 (record ASCII 0x30)` | 80 | `066A:1CC2` | 1107 | Yes | Cycloid or epicycloid curves. |
| daisies | `Alt+q (record ASCII 0x71)` | 60 | `066A:2115` | 1238 | Yes | Radial flower petals around a center. |
| diag1 | `Alt+w (record ASCII 0x77)` | 55 | `066A:25EB` | 172 | No | Diagonal line sweeps. |
| diag2 | `Alt+e (record ASCII 0x65)` | 55 | `066A:2697` | 185 | No | Crossed diagonal line sweeps. |
| ellipse | `Alt+r (record ASCII 0x72)` | 90 | `066A:2750` | 1548 | No | Elliptical outlines or filled ellipses. |
| formula1 | `Alt+t (record ASCII 0x74)` | 38 | `066A:2D5C` | 190 | No | A formula-driven parametric curve. |
| grid | `Alt+y (record ASCII 0x79)` | 30 | `066A:2E1A` | 313 | No | A repeated grid pattern. |
| hexagon | `Alt+u (record ASCII 0x75)` | 20 | `066A:2F53` | 119 | No | Hexagonal outlines or tiling. |
| hilbert | `Alt+i (record ASCII 0x69)` | 1 | `066A:2FCA` | 715 | No | A recursive Hilbert space-filling curve. |
| lines | `Alt+o (record ASCII 0x6F)` | 10 | `066A:3295` | 266 | No | A moving field of line segments. |
| littleStars | `Alt+p (record ASCII 0x70)` | 10 | `066A:339F` | 869 | No | Small star-like points or rays. |
| lotsadots | `Alt+a (record ASCII 0x61)` | 3 | `066A:3704` | 129 | No | A dense field of dots. |
| pedals | `Alt+s (record ASCII 0x73)` | 30 | `066A:3785` | 302 | Yes | Radial petal shapes (name spelling retained). |
| puffs | `Alt+d (record ASCII 0x64)` | 15 | `066A:38B3` | 534 | No | Rounded puff or bubble shapes. |
| ribbons | `Alt+f (record ASCII 0x66)` | 18 | `066A:3AC9` | 338 | No | Curving ribbon-like trails. |
| triangles | `Alt+g (record ASCII 0x67)` | 25 | `066A:4730` | 417 | No | Repeated or moving triangles. |
| tunnel | `Alt+h (record ASCII 0x68)` | 30 | `066A:48D1` | 181 | No | A perspective tunnel of nested shapes. |
| sierpinski | `Alt+j (record ASCII 0x6A)` | 20 | `066A:3C1B` | 311 | No | A Sierpinski-style triangular fractal. |
| sine | `Alt+k (record ASCII 0x6B)` | 9 | `066A:3D52` | 505 | Yes | A sine-wave path or band. |
| spiral | `Alt+l (record ASCII 0x6C)` | 35 | `066A:43B7` | 235 | No | A spiral path. |
| spiral1 | `Alt+z (record ASCII 0x7A)` | 40 | `066A:44A2` | 345 | No | A second spiral variant. |
| squares | `Alt+x (record ASCII 0x78)` | 50 | `066A:3F4B` | 354 | No | Nested or moving squares. |
| squirals | `Alt+c (record ASCII 0x63)` | 10 | `066A:40AD` | 770 | Yes | A squiral/spirograph-like curve (name spelling retained). |
| sweep | `Alt+v (record ASCII 0x76)` | 10 | `066A:45FB` | 107 | No | A sweeping line or band. |
| sweep2 | `Alt+b (record ASCII 0x62)` | 40 | `066A:4666` | 202 | No | A second sweep variant. |
| wandering | `Alt+n (record ASCII 0x6E)` | 40 | `066A:4986` | 456 | No | A wandering, randomized path. |
| zigstream | `Alt+m (record ASCII 0x6D)` | 7 | `066A:4B4E` | 495 | No | A stream of zigzag paths. |

### Ctrl selections (6)

| Name | Selection key | Default size | Entry | Bytes | FP | Description guess |
|---|---:|---:|---:|---:|:---:|---|
| butterflies | `Ctrl+a (record ASCII 0x61)` | 50 | `0B84:0003` | 748 | No | Butterfly-like mirrored polygon forms. |
| diamonds | `Ctrl+b (record ASCII 0x62)` | 60 | `0B84:02EF` | 1079 | No | Repeated diamond forms. |
| gumballs | `Ctrl+c (record ASCII 0x63)` | 30 | `0B84:0726` | 756 | No | Rounded, gumball-like circles. |
| tiles | `Ctrl+d (record ASCII 0x64)` | 60 | `0B84:0A22` | 1017 | No | Repeated tile patterns. |
| wedges | `Ctrl+e (record ASCII 0x65)` | 60 | `0B84:0E1B` | 804 | No | Radial wedge or fan shapes. |
| yarnball | `Ctrl+f (record ASCII 0x66)` | 10 | `0B84:113F` | 835 | Yes | A tangled yarn-ball-like curve. |

## Fade and transition effects

The fade configuration table at `DS:0474` has 24 seven-byte rows: key byte, far name pointer, and enable word. All 24 enable words initialize to 1. Names and keys retain their exact case/spelling from DGROUP; lowercase key characters are the configured keys.

| Name | Key | Enabled by default | Entry | Bytes | FP | Description guess |
|---|---:|:---:|---:|---:|:---:|---|
| rAndomize | `a (record ASCII 0x61)` | Yes | `0CCD:0E31` | 330 | No | Randomize or reshuffle the displayed palette or image state. |
| Blister | `b (record ASCII 0x62)` | Yes | `0CCD:4202` | 649 | No | A spreading blister or bubble-like transition. |
| Countdown | `c (record ASCII 0x63)` | Yes | `0CCD:1F99` | 587 | No | A timed/counting transition, possibly clearing in stages. |
| Drip | `d (record ASCII 0x64)` | Yes | `0CCD:1AA1` | 544 | No | A drip-like screen wipe. |
| Exclusive-or | `e (record ASCII 0x65)` | Yes | `0CCD:2D07` | 281 | No | An XOR-style change using the EGA raster operation. |
| Firefly | `f (record ASCII 0x66)` | Yes | `0CCD:1220` | 1210 | No | Scattered moving points that resemble fireflies. |
| Gas cloud | `g (record ASCII 0x67)` | Yes | `0CCD:0D6B` | 198 | No | A cloud-like transition, possibly a palette or display-start effect. |
| spIral | `i (record ASCII 0x69)` | Yes | `0CCD:16DA` | 734 | Yes | A spiral-shaped reveal or wipe. |
| Juggle palette | `j (record ASCII 0x6A)` | Yes | `0CCD:448B` | 248 | No | A palette-only color rearrangement/fade. |
| Kleenup | `k (record ASCII 0x6B)` | Yes | `0CCD:1CC1` | 698 | No | A cleanup pass that clears or replaces remaining pixels. |
| Linewipe | `l (record ASCII 0x6C)` | Yes | `0CCD:3437` | 958 | No | A line-by-line wipe. |
| Mesh | `m (record ASCII 0x6D)` | Yes | `0CCD:0F7B` | 677 | No | A mesh/grid-based reveal. |
| firewOrks | `o (record ASCII 0x6F)` | Yes | `0CCD:4583` | 402 | No | A fireworks-like palette transition; also calls Juggle palette at 0CCD:448B. |
| Pan/scroll | `p (record ASCII 0x70)` | Yes | `0CCD:4051` | 105 | No | Scrolls the displayed window vertically through the display-start callback. |
| Quilt | `q (record ASCII 0x71)` | Yes | `0CCD:0BFA` | 369 | No | A quilt/tile-based transition; calls the rectangle-swap helper. |
| Rectangles | `r (record ASCII 0x72)` | Yes | `0CCD:3326` | 273 | No | A transition using moving or nested rectangles. |
| Starburst | `s (record ASCII 0x73)` | Yes | `0CCD:37F5` | 1625 | No | A starburst-shaped reveal or collapse. |
| Twist | `t (record ASCII 0x74)` | Yes | `0CCD:2E20` | 626 | Yes | A twisting screen transformation. |
| Umbra | `u (record ASCII 0x75)` | Yes | `0CCD:24CD` | 413 | No | An umbra/shadow-like wipe. |
| moVingbars | `v (record ASCII 0x76)` | Yes | `0CCD:2672` | 804 | No | Moving bars sweep across the image. |
| Wipe | `w (record ASCII 0x77)` | Yes | `0CCD:3E4E` | 344 | No | A general pixel or region wipe. |
| heXagon | `x (record ASCII 0x78)` | Yes | `0CCD:3FA6` | 171 | No | A hexagon-based reveal; calls the image hexagon routine. |
| tetrYs | `y (record ASCII 0x79)` | Yes | `0CCD:2996` | 881 | No | A falling-block/tetromino-like transition. |
| Ziti | `z (record ASCII 0x7A)` | Yes | `0CCD:21E4` | 745 | No | A zigzag or folded-strip transition (name-derived guess). |

## Dispatch

- `11C1:18E7` handles keyboard input using the extended-key table at `11C1:1CFC` and ASCII table at `11C1:1D44`; it normalizes image selections before Semi-Auto or Auto searches the appropriate record list.
- `11C1:3539` is the Semi-Auto selection/draw loop; `11C1:38C3` is the Auto selection/draw loop. Both call the chosen far image function through `DS:5C4B` and use `DS:5C3F` for plotting in the drawing backend.
- `0CCD:4715` normalizes lowercase `a`-`z`, indexes a 26-word jump table at `0CCD:4A4E`, checks each matched effect's enable word, and calls its handler. There are 23 switch cases; `h` and `n` reach the default. `Pan/scroll` (`p`) is special-cased by `0CCD:4A82`, which calls `0CCD:4051` to animate display start via `DS:5C33`.
- `firewOrks` also calls `0CCD:448B` (`Juggle palette`). `callees` in the JSON gives shared/direct application targets by address; `DS:` entries are indirect callback slots.

## Counts and FP classification

Inventory totals: **42 image algorithms** (36 Alt and 6 Ctrl) and **24 fade/transition effects**, plus 5 dispatcher/orchestration routines. 10 image algorithms and 2 fades contain Borland software-FP emulator sequences (`INT 34h`-`3Dh`) or x87 instructions; the remaining entries are integer/fixed-point by this static scan. The names and behavior descriptions are sourced from the binary strings where present; descriptions are explicitly guesses from names and observed callees.

The inventory is static only. Function addresses come from relocated far pointers in the image records or the fade key switch; lengths come from instruction/return boundaries. No runtime execution was used. Machine-readable evidence, including name DS offsets, exact keys, default sizes, callees, FP flags, confidence and per-entry evidence, is in [`evidence/patterns.json`](../evidence/patterns.json).
