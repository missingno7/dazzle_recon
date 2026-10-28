# Graphics architecture (static evidence)

Analysis of `reference/load_image.bin`, SHA-256
`3df021b0416039df3b48b03362d223f5f358fd951d18316845624fd304030d8a`.
Addresses are load-image paragraphs, not runtime relocated segments; linear address
is `16*segment+offset`. `DS:xxxx` means `29BF:xxxx` (linear `0x29BF0+xxxx`).
Names/prototypes are **PROVISIONAL semantic identifications**, not accepted byte
matches. No candidate was compiled and no coverage is claimed. The accompanying
`evidence/names-graphics.json` supplies individual names and linear addresses.

## Adapter selection and options

`1899:0932` (`0x192C2`) saves the original mode with INT 10h/AX=0F00h
(AL masked with 7Fh) to DS:7486, cursor shape with AX=0300h to DS:748A,
and the byte at 0040:0084 to DS:748C. `1899:0959` (`0x192E9`) detects the
adapter: AX=1A00h, BX=0, INT 10h; AL must return 1Ah. Its CS:0982 table
maps active display BL values 1,2,4,5,6,7,8,0Bh,0Ch to internal codes
1,2,4,5,6,7,8,0Bh,0Ch; other values become zero. A zero result falls back
to AX=1200h, BX=0010h. BL remaining 10h means unsupported; otherwise
`BL&3` is saved in DS:748E. If 0040:0087 bit 3 is clear, saved BH bit 0
selects internal 5 (mono EGA) versus 4 (color EGA).

Internal 4 and 8 are the drawing backends. The CS:0A3B dispatch warns for
code 5 with DS:3BCA, `Mono EGA is not supported`, and rejects other unsupported
types with DS:3C04, `Color EGA or VGA required, sorry`. Code 7 instead enters
0A63 and becomes 8 without printing the adjacent mono-VGA error string.
After errors, force flags permit continuation; final assignments are `/E` then
`/V`, so **/V wins when both are present**. No memory-capacity rejection was
found using the saved DS:748E value.

The parser's two parallel word tables at `11C1:3333` / `3369`
(`0x14F43` / `0x14F79`) establish these effects, independently of the manual:

| Option | Parser / global | Effect verified downstream |
|---|---|---|
| /E | 11C1:30EC; DS:7480 | Force internal adapter 4 after detection/errors. |
| /V | 11C1:320A; DS:7482 | Force internal adapter 8 after detection/errors. |
| /T | 11C1:3202; DS:5DE4 | Select physical-aspect coordinate mapping; EGA remains BIOS 10h, VGA remains chained BIOS 13h. VGA pan/scroll is disabled at 0CCD:4AF4. |
| /C | 11C1:3019; DS:5DB4 | Select BIOS **palette** uploads. Plot/read functions remain direct-memory backends. |
| /2 | 11C1:2FF5; DS:5DD8 | Select the two-part VGA palette writer, including with /C or /X. |
| /X | 11C1:321A; DS:5DD4 | Select byte OUT loops instead of 80186+ REP OUTSB / ENTER / LEAVE writers; VGA initial cycle threshold is multiplied by four. |
| /4 | 11C1:2FFE; DS:5DD0 | EGA palette builder repeats a four-color sequence; framebuffer still has four planes and 16 indices. |
| /H | 11C1:30FE; DS:5DDE | Force EGA border color DS:5D8C to zero during panel reset (0CCD:4EB8). |

If /X is absent, `1899:0FF5` (`0x19985`) tests FLAGS high bits and a shift
by 33 to distinguish 8086/88 from later CPUs; the parser enables /X itself
on the older CPU (`11C1:3304–3324`). This is not an adapter test.

## Modes, memory, and display window

`11C1:21A6` (`0x13DB6`), `void far configure_video(int allocate)`, installs
the backend pointers. All pointers below are offset:segment pairs in DGROUP.
DS:5DB0 / 5DB2 are **half-width / half-height**, not full dimensions.

| Backend | Half-size | Plot DS:5C3F | Read DS:5C3B | Mode DS:5C43 |
|---|---|---|---|---|
| EGA default | 320,350 | 1899:012D | 1899:075D | 1899:0AA5 |
| EGA /T | 320,175 | 1899:02E8 | 1899:07F5 | 1899:0AA5 |
| VGA default | 160,200 | 1899:048B | 1899:088B | 1899:0AFC |
| VGA /T | 160,100 | 1899:0622 | 1899:08F1 | 1899:0B9E |

EGA mode setup `1899:0AA5` (`0x19435`) takes one word: 1 sets BIOS mode
10h, 0 sets 90h (preserve video RAM), 2 skips the BIOS call; all cases write
AX=0205h to port 3CEh, selecting graphics-controller write mode 2. The
default 640×700 is a **software coordinate space**: the default plotter rejects
odd translated Y and divides even Y by two; physical output remains 640×350.
No EGA CRTC change to a physical 700-line mode exists in this setup.

VGA setup `1899:0AFC` (`0x1948C`) starts with BIOS mode 13h (argument
nonzero) or 93h (zero). It then performs these indexed read/modify/writes:

| Port / index | Operation | Result |
|---|---|---|
| 3C4h / 04h | `(old & F7h) | 04h` | Disable chain-4 and odd/even memory addressing. |
| 3CEh / 05h | `old & E7h` | Clear odd/even and read-mode bits; retain the mode-13h 256-color shift setting. |
| 3CEh / 06h | `old & FDh` | Clear chain odd/even. |
| 3D4h / 09h | `old & 60h` | Clear repeat count and double scanning; retain high vertical bits. |
| 3D4h / 14h | `old & BFh` | Disable doubleword addressing. |
| 3D4h / 17h | `old | 40h` | Enable byte addressing. |

These writes plus the plot/read formulas prove the enhanced mode is unchained
320×400×256. It uses four interleaved planes: plane=`X&3`, byte offset=
`80*Y+(X>>2)`. VGA /T's `1899:0B9E` (`0x1952E`) only sets 13h/93h and
uses byte offset `320*Y+X`. Register terminology is checked against IBM's
[VGA/XGA Technical Reference, §§2-54, 2-64, 2-72–75](https://bitsavers.trailing-edge.com/pdf/ibm/pc/cards/IBM_VGA_XGA_Technical_Reference_Manual_May92.pdf).

DS:7490 is a **paragraph offset** added to A000h by every plot/read routine:
EGA page offsets 0 / 06D6h mean byte offsets 0 / 28000; VGA offsets 0 / 07D0h
mean 0 / 32000 **per plane**. Thus EGA needs 56000 bytes per plane for two
640×350 pages, VGA 64000 for two 320×400 pages (256000 bytes across planes).
The panel transition at `0CCD:4B27–4B5B` (`0x117F7`) draws on the second
page while the display still starts at page zero. `0CCD:4051` (`0x10D21`)
then increments/decrements a physical row offset and calls DS:5C33.

DS:5C33 targets `1899:0EBF` (`0x1984F`),
`void far set_display_start(unsigned x8, unsigned physical_y)`:
it computes `start=80*physical_y+floor(x8/8)`, waits for the next vertical
retrace, and writes start high/low to 3D4h indices 0Ch/0Dh. It computes an
X remainder but does not program horizontal PEL panning. The traced pan callers
pass X=0: the demonstrated operation is smooth vertical scrolling between
adjacent pages, not proof of smooth horizontal panning.

`1899:0E40` / `0E73` (`0x197D0` / `0x19803`) set the EGA/VGA line-compare
split. Both take one scanline word and wait for retrace. EGA writes index 18h
low byte, index 07h=`0Fh | ((line>>8 & 1)<<4)`, index 09h=0; VGA preserves
other bits of 07h/09h and replaces line bits 8/9 there. `0CCD:40BA`
(`0x10D8A`) animates that split over 350 or 400 physical lines (with /T
adjustment). These are separate from /2, which splits palette transfers.

`1899:0BCD` (`0x1955D`), `void far restore_video(int restore_mode)`, resets
GC bit mask to FFh and write mode to zero; if requested, restores the saved
BIOS mode, text row/font setting and cursor shape.

## Drawing ABI and algorithms

Public routines here return with RETF and use caller cleanup. First word argument
is `[BP+6]`, subsequent words +8,+Ah,...; far pointers occupy offset then segment.
Arguments are pushed right-to-left. Borland's same-segment `PUSH CS; CALL near`
still creates the far-return frame; it does **not** imply a near C interface.
Word results use AX (pixel results AL with AH=0). Void routines leave AX
incidental. DS is DGROUP except explicitly saved/restored far-buffer operations.
The reusable interface is stateful and non-reentrant.

**Symmetric plot kernels, `1899:012D/02E8/048B/0622`.** Signature
`void far plot_symmetric(unsigned symmetry)`; read DS:5DF0=X, 5DEE=Y,
5DB0=CX, 5DB2=CY, 7488=color, 7490=page, 5DC6=delay; DS:3BC8/3BC9 are
temporary edge flags. For each pass, form `xp=CX+X`, `xm=CX-X`,
`yp=CY+Y`, `ym=CY-Y`, rejecting the pass if any sum/difference is signed
negative. EGA enhanced also requires even yp/ym and halves both. Let
`fx=(CX<=X)`, `fy=(CY<=Y)`: write `(xp,yp)` only if neither flag; if bit 0
of symmetry is set, also write `(xp,ym)` if !fx, `(xm,yp)` if !fy and
`(xm,ym)` unconditionally. If `(symmetry&3)==3`, repeat with X/Y exchanged
and symmetry forced to 1. This is the literal edge logic, including asymmetric
endpoint behavior; it is not a general rectangular clipping API. Arithmetic is
16-bit. Before plotting, the kernels run the delay loop using DS:5DC6;
`LOOP` means a zero count wraps rather than skipping it.

For EGA, each write sets GC index 08h to `80h>>(physical_X&7)`, computes
`80*physical_Y+(physical_X>>3)`, reads VRAM to load the latches and writes
the low color byte in write mode 2. VGA enhanced sets sequencer index 02h
to `1<<(physical_X&3)` and uses the unchained offset above; VGA /T directly
stores the byte. VGA also performs a read before the store, but does not
combine that read with color in software. No implicit color arithmetic occurs
in these stores.

**Pixel reads, `1899:075D/07F5/088B/08F1`.** Signature
`unsigned far read_pixel(int x,int y)`; arguments +6/+8, same centers/page.
EGA enhanced halves translated Y without requiring parity; EGA /T does not.
Both select GC read-map 0,1,2,3 and extract the bit at `7-(X&7)` into result
bits 0..3. Negative translated coordinates branch to the epilogue without
defining AL as a sentinel. They do not check upper bounds. VGA enhanced
selects both sequencer map mask and GC read map using `X&3`, then reads one
byte; VGA /T reads `320*Y+X`. The VGA readers have no coordinate rejection.
Callers must supply valid coordinates; SDL must preserve this contract or
explicitly distinguish a safer API from the original one.

**Raster operation, `11C1:0E2A` (`0x12A3A`).**
`void far ega_set_xor(int enabled)` takes +6 and writes GC index 03h to
18h for nonzero, zero otherwise (XOR versus replace). The latch read in
EGA plotting makes this a hardware read/modify/write operation. Calls such
as `0CCD:0E38` enable it for effects. This is not proof that VGA byte
plotters XOR; they simply store their color byte.

**Clear, `1899:0ACD` (`0x1945D`).**
`void far clear_vram_words(unsigned pattern,unsigned count,unsigned page_para)`
uses +6/+8/+Ah, waits for retrace, enables all planes with port
3C4h/AX=0F02h, sets ES=A000h+page_para and does `CLD; REP STOSW`
from DI=0. No clipping; hardware state inherited from the selected mode matters.
Enhanced VGA mode initialization passes count=FA00h, page=0; /T passes 7D00h.
The former is **64000 word stores with 16-bit DI wrapping**, not a linear
128000-byte allocation. A page-clear caller at `0CCD:0DCA` passes
count=3E80h (16000 words). EGA initialization uses the BIOS mode clear.

**Line iterator, `066A:000E` (`0x066AE`).**
`int far line_step(int end_x,int end_y,int initialize)` (+6/+8/+Ah) reads
and updates DS:5DF0/5DEE and uses DS:482C/482E doubled deltas,
4830/4832 signs, 4834 error. Initialization computes `D=2*abs(end-current)`
and `error=Dminor-Dmajor/2`; returns 1 only for a zero-length line, otherwise
0 without advancing. A continuation advances the major coordinate by its
sign, advances the minor one when error>=0, subtracts Dmajor on that minor
step and adds Dminor each step. It returns 1 on reaching the major-axis
endpoint; callers plot the current point themselves. Ties choose X-major.
No hardware, color or clipping is inside this function.

**Square fill, `066A:026B` (`0x0690B`).**
`void far fill_square(int side,unsigned symmetry)` (+6/+8) reads/writes
the shared X/Y and calls DS:5C3F for side×side points: advance Y in the
inner loop, restore it after each column, advance X in the outer loop,
restore X at the end. Nonpositive side draws nothing. It polls the palette
timer after columns; backend plotting supplies the edge behavior.

**Polygon fill/outline, `066A:02BB` (`0x0695B`).**
`void far polygon(int rebuild,int n,int far *x,int far *y,unsigned fill_color,
unsigned edge_color,unsigned flags)` has words at +6,+8, far pointers
+Ah/+Eh, colors +12h/+14h, flags +16h. Arrays are mutable: save x[0]/y[0]
as translation, zero that first vertex and append a zero closing vertex at n.
On rebuild, find min/max Y and use the line iterator on every edge to collect
one left/right X extent per row in DS:4836/4AF2. Bit 1 of flags fills each
inclusive span; bit 0 draws the edges. For `flags&0Ch`, value 4 advances fill
color per pixel, value 8 per row, and other values keep it constant; bit 10h advances edge
color per plotted point, wrapping to index 3 at color_count. The fill is a
single min/max span per row, so this is not a general even/odd concave fill.
It calls the plot backend with DS:5CAF symmetry and polls palette cycling.

**Ellipse iterator, `066A:05D8` (`0x06C78`).**
`int far ellipse_step(int radius_y,int radius_x,int initialize)` uses +6/+8/+Ah,
shared X/Y, and 32-bit state DS:4DB8..4DDC. If radius_x=0, derive it from
radius_y times `(half_width/half_height)*DS:06AF` (float 0.75), rounding
up only when fractional part exceeds DS:06B3 (0.5). Start `(0,radius_y)`;
use squared radii, doubled squared radii, derivative/error increments and
two midpoint regions, switching when the X derivative reaches the Y
derivative. Stop when Y becomes zero. This is portable geometry; it plots
nothing itself. `066A:089B` (`0x06F3B`) builds a step-direction table from
this iterator and uses it for filled/outlined ellipses through the plot API.

**Per-point color, `066A:011B` (`0x067BB`).**
`void far select_point_color(unsigned mode)` takes +6, uses X/Y,
DS:5DB6=color_count, DS:7488=current_color and EGA counter DS:5DAA.
Modes 1..8 select respectively `R(N)`, abs(X), abs(Y), abs(X)+abs(Y),
`(sx2+sy2)>>5`, `(sx2-sy2)>>5`, `(low16(Y*Y) arithmetic>>5)-abs(X)`,
and `abs(low16(X*Y)) arithmetic>>5`. Here sx2/sy2 are **sign-extended
low 16-bit products**, not mathematical squares; modes 5/6 use a logical
32-bit right shift. Other modes increment the color (EGA only every fourth
call). Finally AND with N-1 and clamp values below 3 to 3. Preserve the
original truncations/wraps in matching code. Here and in palette generation,
`R(n)=floor((long)rand()*n/32768)`: the calls to RTL 0000:11E3 (multiply)
and 12E7 (signed quotient, not remainder) establish scaled random selection.
Borland rand/math/shift helpers
are RTL dependencies, not application graphics coverage.

## Palettes, cycling, keys, and fades

DS:5D94 is the far palette allocation, 5DEC the current **byte offset** into
it, 5DB6 the framebuffer index count (16/256), 5DBA the generated extent
(64 EGA entries / 576 VGA entries, including three reserved entries),
5DB8=3*5DBA, 5DE8 the buffer size
(80 EGA bytes / 2496 VGA bytes). Builders append a repeated tail so a
contiguous window can be uploaded without rotating pixels.

The palette pointer DS:5C37 is installed by `11C1:21A6`:

| Target | Behavior (one far buffer argument at +6) |
|---|---|
| 1899:0C13 (`0x195A3`) | EGA BIOS AX=1002h, ES:DX table, 16 palette bytes plus border. |
| 1899:0C7C (`0x1960C`) | EGA direct, retrace wait then attribute-controller 3C0h index/data pairs, 15 down to 0 and index 31h for border. |
| 1899:0D14 (`0x196A4`) | VGA /C: BIOS AX=1012h, BX=3, CX=FDh, ES:DX RGB data. |
| 1899:0D27 (`0x196B7`) | VGA /C /2: BIOS counts 7Eh at index 3, then 7Fh at 81h. |
| 1899:0D48 (`0x196D8`) | VGA normal: next retrace, 3C8h=3, REP OUTSB of 2F7h=759 bytes to 3C9h. |
| 1899:0D74 (`0x19704`) | VGA /2: two retrace waits, index 3/count 17Ah=378 bytes then index 81h/count 17Dh=381 bytes. |
| 1899:0DBF (`0x1974F`) | VGA /X: same full upload using MOV/INC/OUT/LOOP. |
| 1899:0DEE (`0x1977E`) | VGA /X /2: same two-part upload using byte loops. |

The EGA writers temporarily replace table bytes 0,1,2,16 with
DS:5D8A (background index-0 mapping), 5D6C, 5D6E (fixed colors), 5D8C
(border), then restore the table. VGA cycling updates entries 3..255;
`17F5:030F` (`0x1825F`) uploads reserved starting entries via BIOS 1012h.
VGA initialization also uses 1012h directly, first for 256 entries from
A000:0000, then for nine entries from the palette buffer. **No AX=1000h
single-EGA-register path was established**; the found EGA uploads use 1002h.

There is a literal anomaly in BIOS /2: after the first 126 RGB entries,
`1899:0D38` adds **007Eh bytes** to DX, not 017Ah bytes. The direct split
writers advance SI by 378 bytes correctly. Record this difference rather
than silently repairing it; visual consequences require a future /C /2 run.
No BIOS routine-preservation assumptions were tested here.

EGA generator `11C1:0E45` (`0x12A55`), `void far build_ega_palette(int live)`,
reserves the first three bytes and generates 61 additional palette codes from
three independent bouncing 2-bit component counters with randomized periods
and phases. Red levels map to bits 5/2, green to 4/1, blue to 3/0. /4
overrides this with four randomized codes repeated by `i&3`. It appends a
16-byte tail copied from offset 3. `1899:0006` (`0x18996`) separately converts
RGB8 triples to EGA codes: thresholds 33h,77h,BBh produce component levels
0,1,2,3 in those same bit pairs (used by PCX loading).

VGA generator `11C1:13EF` (`0x12FFF`), `void far build_vga_palette(int live)`,
initializes reserved colors and generates 573 RGB triples after the first
three entries. Normally each channel is
`max(0,trunc(A*cos((phase+i)*frequency)+A+1)-31)` with random phase and
amplitude A (23..45 or 47), frequencies approximately
(.025,.033,.04), Alternate (.075,.087,.1), or Mad (.15,.175,.2).
The /F branch uses amplitude 47 and zero phases. A rare grayscale branch
instead repeats `trunc(31*cos(i*frequency)+31)` into all three components.
It appends 768 bytes from offset 9. The floating instructions are Borland
INT 34h..3Dh emulation forms; the called RTL `0000:0F6E` has a native FCOS
branch, so this is **cosine**, not a port-I/O sequence in a linear decoder.

`11C1:2515` (`0x14125`), `void far cycle_palette(void)`, skips uploads when
DS:0BFE!=0 and DS:0C00==0 (frozen/static state). Otherwise advance 5DEC by
1 EGA byte, wrapping at 64 to 3; or by VGA DS:5DA4 bytes, wrapping at 1728
to 9. Upload `palette+offset`, then zero DS:5D7A. Timer callback
`1899:14C7` (`0x19E57`, IRET) increments 5D7A; the application polls it
against DS:5DA6 before calling cycle_palette. This is a BIOS INT 1Ch tick
counter, not retrace frequency; the callback itself does not upload palettes.

Key dispatch `11C1:18E7` (`0x134F7`) and CS:1CFC/1D44 tables prove:

| Key | Handler | Control |
|---|---|---|
| + | 11C1:1994 | Reduce cycle threshold t by floor(t/2) if t>2, else by 1; minimum 1. |
| - | 11C1:19D4 | Increase t by floor(t/2) if t>2, else by 1; maximum 64. |
| PgUp (4900h) | 11C1:1BA9 | VGA/menu guard, add 3 to step, maximum 57 bytes = 19 entries. |
| PgDn (5100h) | 11C1:1BEB | Subtract 3 from step, minimum 3 bytes = 1 entry. |
| Shift+F10 (5D00h) | 11C1:1C57 | Call selected palette builder with live=1, then upload current window. |
| Ctrl+F10 (6700h) | 11C1:1C27 | Call 11C1:134A (`0x12F5A`) to regenerate reserved/background mappings and upload. |

The current `architecture.md` snapshot reverses the Shift/Ctrl labels in its
key summary. The raw table pairs are 5D00h→1C57 and 6700h→1C27; the labels
above agree with the manual and BIOS scan codes. That other worker's file was
left unchanged.

Normal initial threshold is EGA 2 ticks, VGA 1 (4 with /X); initial VGA
step is 3 bytes (`11C1:1FA8`). Both direct VGA variants and EGA direct
upload wait for 3DAh bit 3 to become clear and then set: next retrace edge,
not just a test for already being in retrace. They enable interrupts with
STI and do not restore the incoming IF. BIOS uploads rely on BIOS timing.

Fade `0CCD:448B` (`0x1115B`), `void far fade_dac_to_black(int mask_high_bits)`,
iterates the entire VGA buffer, saturating each nonzero byte downward by one
until all zero. It optionally first ANDs bytes with 3Fh or ORs them with C0h,
uploads on alternating passes, polls cycling/input, and marks DS:5CB1 when
finished. `0CCD:4583` (`0x11253`) provides another palette fade: replace two
walking windows with the background component, wrapping buffer pointers and
uploading each step (EGA discrete colors or VGA RGB triples); VGA may
delegate to 448B. The panel reset at `0CCD:4D5E` zeros the whole palette and
DS:5D8A/5D8C, uploads black, then reinitializes mode/palette before drawing.
Thus palette fades and VRAM clear are distinct operations.

## Source origin and portable boundary

The `066A` iterators, fills and color selector have Borland C fingerprints:
BP frames, C-style locals, SI/DI saves, caller cleanup, switch tables,
compiler long arithmetic helpers and floating-emulator/math calls. Keep
these in **src/**, along with palette generation/cycling, input policy and
transition algorithms. No assembly is justified merely because a routine is
geometric or performance-sensitive.

The `1899` plot/read kernels, mode/clear/display-window setters, DAC writers,
CPU probe and tick ISR strongly support **assembly-origin kernels**:
deliberate byte halves/register allocation, parity-flag symmetry dispatch,
latch reads whose values are discarded, hard-coded port sequences, preserved
DS around LDS/REP OUTSB, ENTER/LEAVE versus explicit 8086 fallbacks, and
hand-built stack argument slots in the neighboring recursive drawing code.
BP frames alone do not distinguish them from C. The EGA palette writers'
C-like local save/restore plus direct assembler blocks remain **unknown**
between complete assembly and C with inline assembly; defer that file choice
until a compiler/source probe, rather than treating I/O alone as proof.

For SDL3 later, replace the mode/palette/plot/read/display-start/line-compare
pointer targets plus clear, raster-op, save/restore and PCX planar-row transfer.
Use indexed software surfaces (two pages where supported), indexed palette
uploads with reserved entries, an explicit displayed window and split line,
and a tick source. Retain geometry, symmetry/edge rules, pixel color arithmetic,
palette construction/cycling and fade policy as portable application code.
The current plot entry combines portable symmetry with hardware stores: first
reproduce it as found; later separate those parts while keeping the same
coordinate/parity contract. `1899:101A/133D` are recursive image subdivision
and seeding, despite residing beside the hardware code; they call the shared
read/plot API and belong above the portable boundary, not in a VGA replacement.

## Verification and remaining limits

- Static disassembly and raw-word decoding only; no DOSBox or background process
  was started. Original/reference files were read only.
- `python tools/disasm.py 066A:000E -n 0x120` and focused `1899` / `11C1`
  listings were cross-checked against the pointer assignments, parser/key
  tables and byte offsets. Scratch contains `scan.py`, `image.lst` and palette
  listings. Its optional `fp` view length-preservingly respells emulator escapes
  for analysis; it is never a candidate or an expected-byte source.
- `python -I tools/disasm.py ...` initially failed because Capstone is installed
  only in user site-packages. Scratch scanner uses `-I` plus an explicit installed
  Capstone path. No compiler or flags were involved in these static probes.
- Negative findings: /C did not select BIOS pixel plotting; /4 did not reduce
  framebuffer depth; no physical EGA 700-line setup, smooth horizontal pan
  programming, or established EGA AX=1000h upload was found in the traced layer.
- BIOS /2 pointer anomaly, out-of-contract pixel reads, adapter-error presentation,
  and exact fade pacing remain untested dynamically. Static source-origin guesses
  are not compiler matches. Matching reconstruction should next isolate these
  kernels and verify their complete extents and relocations through the project's
  acceptance gate; no SDL3 implementation has been started.
