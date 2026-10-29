# Image and fade rendering classification

Source review, 2026-10-29. Design only; no application or test changes. The inventory is the
36 Alt and 6 Ctrl records in [dzle.c](../src/app/dzle.c) lines 1956-2001, checked against
[dosi.c](../src/app/dosi.c) selection/plot loops (1740-1806, 1850-1920). The 24 fade records
at DZLE:2002 map through [dzfd.c](../src/app/dzfd.c):2301 and its boundary handler at 2493.
Line numbers below refer to these native sources, not DOS load addresses.

## Reading the tables

- `P(s)` means `plot_callback(s)`: 0 is one translated point, 1 adds quadrant reflections,
  3 also exchanges X/Y. `P(D)` uses the caller's `draw_symmetry`. The exact clipping,
  duplicate stores, EGA parity rejection and address arithmetic are in
  [dvdt.c](../src/dvdt.c):35-137. Do not infer symmetry from the record's name or flags.
- **A** means accumulating writes, no framebuffer reads or erasure in the image itself.
  All 42 images are A. `balls` reads a resource bitmap, not VRAM. Shared helpers do not
  read pixels either. Every image can nevertheless overwrite another image's pixels.
  DOSI also issues an outer `P(D)`, including after functions that draw internally;
  preserve that point, even when it is a sentinel or an incidental center stamp.
- The record's `continuous` field permits EGA XOR selection; it is **not geometric
  continuity**. `X` in the risk column means this permission is set. VGA ignores the
  EGA raster op. Any actual XOR write uses the Classic fallback in
  [the Hybrid design](../../docs/native-hybrid.md).
- `S` = `screen_extent`; `W,H` = `screen_half_width,screen_half_height`. All retain their
  original values in Hybrid. Common risks `C` = word coordinates/counters, narrowed
  color arithmetic and wrapped plot addresses; never globally widen them. `F` = float
  state/truncation matters; `Q` = explicit fixed-point/wrapped products. Absence of a
  special wrap note does not authorize changing the common word semantics.
- Hybrid recommendations: **H** is a specific eligible stroke, **P** keeps the original
  raster stamps, **E** needs a separate Enhanced variant before increasing detail.
  Connected means within the stated run only; rebuild, teleport, wrap and separate
  reflection branches always end the run. A scan order across filled pixels is not a
  stroke. All H entries are recommendations, not implemented capabilities.
- Extended proposals use `instances` (same sized motifs at additional centers), `field`
  (explicit wider bounds with unchanged cell size), or `path` (family-specific bounds).
  These require an adapter and separate side state; until implemented, **Classic fallback
  in the original 4:3 viewport**. See the design for timing, palette and fade contracts.

## Alt images: 36 records, in table order

| Key / image / source line | Category; primitives and coordinate production | Continuity and dependence | Size / arithmetic evidence | Hybrid recommendation | Extended strategy |
|---|---|---|---|---|---|
| 1 arches, 419 | Curves/parametric; two `draw_ellipse` outline families, flags `0x21` (per-point edge colors) | Each cached quadrant is connected; separate ellipses/centers; A | W,H radii, shrink by 8; ellipse cache; C, possible doubled large EGA radius | H: each ellipse quadrant, retain discrete color runs; reject cache-spill case | Instances of arches at extra centers; retain circular aspect |
| 2 atom, 434 | Point/raster; trig anchor followed by 15 horizontal increments, caller P(D) | **16-pixel horizontal stamp**, jumps to next polar anchor; A | S radius, phase step and 16-call counter; EGA divides generated Y by 2; C,F | P: stamps; no joining successive polar anchors; E: distinct continuous orbit variant | Instances; do not stretch the orbit |
| 3 balls, 462 | Point/raster, bitmap art; expanding cardinal resource-sampling walk, skip zero samples, P(D) | Sample order does not describe a stroke; A/resource reads | Bitmap W,H/stride, uint16 sample coordinates and column-major offset wrap; C,X | P: shaded bitmap samples | Instances with original bitmap scale |
| 4 bars, 503 | Line-based; `line_step`, parallel segments with random fixed spans, outer P(D) | Connected inside each line; completion resets pen **before outer plot**, so capture actual stores; A | S origins/spans/termination, unsigned endpoint words; C,X | H: committed prefix of each parallel segment | Field of segments with explicit horizontal origin range |
| 5 bigstar, 527 | Line-based; float seed slope, two cached line traces, ray `line_step`, P(D) | Separate rays and halved star layers; per-ray/per-point/EGA color modes; A | W,H aspect projections; `bigstar_traces[175]`; C,F | H: individual rays, never join cached endpoint enumeration; E: larger cache/detail | Instances of unchanged stars |
| 6 boxes, 625 | Line-based; fan from fixed origin to rectangular perimeter, `line_step`, P(D) | Ray-local; completion resets origin and color before outer plot; A | S box size/perimeter and word counters; C,X | H: actual visible prefixes of fan rays | Instances / extra fan centers |
| 7 cones, 672 | Curves/parametric; stack of `draw_ellipse(...SHAPE_OUTLINE)` at incremented centers/radii | Each ring connected; rings are distinct, no surface mesh; A | W,H limits; `cone_directions[4]` of three variants; ellipse cache; C | H: separate ring outlines | Instances of complete cone stacks |
| 8 corkscrew, 710 | Curves/parametric; float growth and phase, trig along/across offsets, outer P(D) | Explicit evolving parametric path; break on restart and narrowing discontinuity; A | S origin, growth and quadratic distance term; C,F | H: consecutive samples of this path, with original per-16-call colors | Path: additional translated coils, same local pitch/aspect |
| 9 curves, 736 | Point/raster; independent trig phases produce an anchor, then 15 X increments | **Horizontal 16-pixel stamps**, not joined curve samples; phase resets at 32000; A | S amplitude; 16-call counter and word phases; C,F | P: stamps; E: an explicitly redesigned connected curve | Instances; no guessed Lissajous path |
| 0 cycloids, 765 | Curves/parametric + lines; 64-entry trig table, orbit/pen sum, 16.16 X aspect, explicit raster joins P(0) | Connected polygonal rolling curve; close at phase condition, break between curves; A | W,H radii; `trig_table[64]`; wrapped `mul32`/arithmetic shift; C,Q | H **prototype**: existing line joins, same quantized endpoints; separate color run per logical point when required | Instances of complete curves; E later for denser angular sampling |
| q daisies, 836 | Line-based; trig-derived edge cached with `line_step`, then four manually reflected ray fans P(1) | Each center-bound ray continuous, endpoint enumeration is not; A | S radii/centers; aspect Q; trig caches [10] each, `daisies_edge_points[66]`; C,Q | H: ray-local strokes, preserve explicit reflection order and color runs; E: cache/detail | Instances; retain petal aspect |
| w diag1, 906 | Procedural texture; X++/Y-- diagonal scan, coordinate-based color rules, P(D) | Adjacent only within a scan; wraps enumerate a raster field; A | S wrap; `choose_color` word squares/products and signed shifts; C,X | P: raster color field | Field: repeat/evaluate original word-domain tiles, with a defined seam |
| e diag2, 931 | Procedural texture; X++/Y++ scan with per-diagonal or varying color periods | Wrapped raster runs, no geometric object; A | S wrap, color period cycles 0..20; C,X | P | Field: original cadence/cell size, explicit restart at side seams |
| r ellipse, 963 | Line-based; midpoint boundary selects rays/diameters, inner `line_step`, P(1) | **Ray fans, not simply an ellipse outline**; reset each ray/diameter; A | S centers/radii, int32 squares and midpoint errors, word line state; C | H: existing rays and diameters only | Instances of ray fans |
| t formula1, 1100 | Procedural texture; growing X columns scan Y; shifted quadratic determines index | Scan adjacency is a color field; A | S bounds; signed **16-bit Y square and multiplied quadratic**, narrowing before arithmetic shift; C | P; E only with a separately specified arithmetic field | Classic fallback first; later field of original wrap-domain tiles |
| y grid, 1123 | Point/raster; four fixed 8x8 masks, skip set cells, tile cursor | Mask art; row/tile jumps, not a path; A | `grid_patterns[256]`, fixed 96-unit tile region; doubled EGA Y spacing; C,X | P | Field: repeat masks on same 8-pixel lattice |
| u hexagon, 1165 | Line-based / raster contour; integer vertical then diagonal runs, P(D) | Run-local connected contours; restart at X<0 and outer completion; A | S limits, incremented start/vertical limit; C,X | H later: explicitly declared vertical/diagonal runs; never join resets | Instances / explicit new side contours |
| i hilbert, 1189 | Fractal/recursive + filled stamps; recursive Hilbert segments, `line_step` + `square`, optional raster background | Segment path is continuous; background scan is not; A; can reset EGA XOR after background | S step; depth 3..5, stamp width `6 >> step`; direction arrays [4]; C,X | H later: thick Hilbert segments with original square footprint; keep background pixelated; E: more depth | Instances of complete curves; no depth increase for width alone |
| o lines, 1283 | Point/raster; bouncing anchor (unit X, random Y velocity) + three Y increments, P(D) | **4-pixel vertical stamp**; no joining anchor trajectories; A | S bounces, 4-call counter; C | P; E: explicit bouncing-path variant only | Instances of original stamped walks |
| p littleStars, 1322 | Line-based; two line families, eight manual sign/swap copies around random center, P(1) | Separate lines/reflections; clipped points do not connect branches; A | S size/placement, word spacing and center sums; C | H: each explicit line branch; preserve per-copy/per-point colors | Instances at additional centers |
| a lotsadots, 1411 | Point/raster; two fresh random coordinates per point plus chosen color rule | **Unrelated points**, A; color rule may consume another RNG draw | S random domain; choose_color word math; C | P **mixed-rendering control**; never connect | Field later with independent side RNG, constant point density |
| s pedals, 1423 | Point/raster; inward trig anchor, seven (+1,+1) steps | **8-pixel diagonal stamp**, then a polar jump; A | S radius, decreases/resets, word phase and 8-call counter; C,F,X | P: keep stamp identity; E: continuous polar variant | Instances |
| d puffs, 1457 | Line-based; random endpoints about fixed/drifting origin, `line_step`, P(D) | Each ray connected; origin/end reset before outer plot on completion; A | S spread/bounds, word endpoints, signed char drift; C | H: actual ray prefixes, per-step index bands | Instances / wider origin field |
| f ribbons, 1492 | Polygon/filled raster; lengthening horizontal runs with row shifts and random wraps | Scan bands; random per-call color event, no contour primitive; A | Fixed run limits 24..44, S wrap; C,X | P; E later for explicit filled-band geometry | Field of bands; preserve run sizes |
| g triangles, 1792 | Polygon/filled; widening integer row bounds, outer P(D) | Fill scans jump to next row/wedge; A | S origins/right clamp; alternating left expansion/per-row color; C,X | P fill; E later for triangle edges with palette bands | Instances of wedges at same local sizes |
| h tunnel, 1834 | Polygon/filled raster; scanline right edge fixed or aliased to draw_y, P(D) | Rectangular/triangular fill enumeration, not a path; A | S row count/right bound; live `tunnel_right_edge_ptr`; C,X | P | Field: separately bounded scan regions; Classic fallback until adapted |
| j sierpinski, 1521 | Fractal/recursive category, chaos game; random vertex, integer half displacement, P(D) | **Unrelated samples of a point set**, not connecting edges; A | `sierpinski_vertices[3]`, S placement; displacement narrows then /2 truncates toward zero; C | P; E later for deeper point density, no strokes | Instances of triangles with independent side state |
| k sine, 1547 | Curves/parametric; sin/cos with independently drifting float amplitudes, P(D) | One sampled trajectory; break at rebuild, wrap/discontinuity; A | S amplitudes; float phase/modulators; EGA rounds X,Y down to even; C,F | H **prototype**: declared successive path samples, reuse pre-truncation values with actual-write support | Instances of unchanged oscillators, no X-amplitude stretch |
| l spiral, 1678 | Line-based; cardinal walk with legs lengthened every second turn, P(D) | Connected spiral legs, break at random restart; A | S restart, leg/counter words, direction [4], color per 8 calls; C,X | H: explicitly bounded cardinal legs, retain color bands | Instances |
| z spiral1, 1704 | Line-based; expanding diamond circuits, diagonal increments, P(D) | Within circuit; X shift/new circuit must be an explicit break; A | S side limit and random restart; `diamond_x/y[4]`; C,X | H: four diagonal sides only | Instances |
| x squares, 1577 | Line-based; growing cardinal circuits and X shift, P(D) | Side-local, break at circuit shift/restart; A | S side limits; direction [4], TARGA forces variant; C,X | H: circuit sides, no bridge across shift | Instances |
| c squirals, 1612 | Line-based + parametric construction; trig rotating endpoints with decaying scale, `line_step`, manual copies P(1) | Separate square-spiral edges and reflection branches; A | S radius/center; float decay/angle, integer endpoints; C,F | H: each explicit edge; no spline replacing the square spiral | Instances |
| v sweep, 1739 | Line-based / raster contour; X++ and interval-driven Y increment, P(D) | Connected staircase within a sweep, reset to origin between sweeps; A | S interval limit; word counters; C,X | H later: declared sweep trace preserving interval kinks, not a guessed straight ray | Path: independent extra rays with adapted horizontal endpoints |
| b sweep2, 1758 | Line-based; origin-to-top rays via `line_step`, P(D) | Ray-local; endpoint sequence may jump/sign-change; A | S endpoint and signed velocity sequence/modulo; C | H **prototype**: progressive existing ray, break at each initialization | Path later: explicit wider X endpoint domain, same Y height |
| n wandering, 1867 | Line-based; moving/bouncing endpoints, `line_step`, P(D) | Segment-local, moving origins do not connect; A | S bounds, velocities -4..4 for 4..32 segments; C | H: individual committed segments | Path: adapter-specific wider bounce bounds |
| m zigstream, 1907 | Line-based / raster walk; growing orthogonal steps or random runs with X +/-1 and Y -1..1 | Run-local; bends are intentional, random restart breaks; A | S bounces, runs 12..23, 8-step color cadence; fixed directions [2]; C | H later: explicit run strokes; keep lattice character as default | Path: independent side walks, original step scale |

## Ctrl images: 6 records, [dzl2.c](../src/app/dzl2.c)

| Key / image / source line | Category; primitives / coordinates | Continuity / dependence | Size / arithmetic evidence | Hybrid recommendation | Extended strategy |
|---|---|---|---|---|---|
| Ctrl+A butterflies, 129 | Polygon/filled; two mirrored five-vertex `draw_polygon` wings, integer growth | Each edge connected; fill rows are raster; separate growing shapes; A | W,H placement/growth; three vertex arrays [6] (translation + closing slot); **multiply_word wraps product before division**; C | H later: wing outlines only; keep per-row fills; E: smooth fill boundary | Instances of whole butterflies |
| Ctrl+B diamonds, 181 | Polygon/filled; moving/resizing four-vertex polygon; random fill/edge/shading | Edge-local, no joining successive diamonds; A | W,H limits; contiguous origin/vertex/closing globals passed as arrays; modeled positive caller continuation gate; C | H later: outline edges, discrete fill bands; E for fills | Instances with separate polygon context |
| Ctrl+C gumballs, 269 | Polygon/filled + ellipses; random center/radius walk, `draw_ellipse` | Ring quadrants connected; successive balls distinct; A | W,H radius/placement, aspect-derived X, ellipse [700]; C | H: outline-only calls; P for shaded fills; E for filled ellipse boundary | Instances of same aspect balls |
| Ctrl+D tiles, 320 | Polygon/filled raster; optional nested fill loops then four integer rectangle edges | Edges connected separately, fill is raster; A | W,H, `tiles_y_extent`, directions [4], side lengths [2]; physical-row work then EGA Y doubling; C | H later: four edges; P per-pixel/per-row interior | Field of same sized tiles with explicit new origins |
| Ctrl+E wedges, 424 | Polygon/filled; three moving integer vertices, relative triangle array, `draw_polygon` | Separate edges/shapes; fill raster; A | W,H; persistent step/vertex [3], temporary closed polygon [4]; **caller/timer continuation gate changes RNG consumption**; C | H later: outline edges; do not touch gate/tick sampling; E for fill boundary | Instances with independent gate/state; initially Classic fallback |
| Ctrl+F yarnball, 504 | Curves/parametric; base ellipse and 3 or 12 neighboring centers, float aspect/30-degree projections | Separate rings; cached quadrant replay, not a continuous path across centers; A | W,H; radius growth, float projection/truncation, ellipse [700], sparse shift; C,F | H: ring outlines, split colors when edge-color flag toggles | Instances of entire ring clusters; retain spacing/aspect |

## Shared limits that must not become output-resolution globals

`line_step` (DZLE:118) doubles narrowed signed-word deltas and retains word error state.
`choose_color` (140) deliberately wraps coordinate products. `draw_polygon` (175) caches
one min/max X per row in two **350-word arrays**, closes the supplied vertex list in place,
and has solid, per-row and per-pixel index modes. `draw_ellipse` (304) replays a
**700-word direction cache**; indices 700/701 intentionally alias completion/fill globals
for the observed large-EGA arches case. Preserve that logical quirk; use Classic there
until a geometry emitter models its actual prefix. Do not draw an ideal full ellipse
over a prematurely terminated logical trace. These limits also affect Ctrl images.

DOS sizes come from DOSI:1083-1171: VGA W/H=160/200 or 160/100, EGA=320/350
(doubled software Y) or 320/175. S is mode/option dependent, not simply display width.
The bitmap derives from W,H and its scale divisor. No image becomes resolution-safe by
substituting monitor dimensions for any of these values. Enhanced must separately audit
every listed cache, counter, product, recursion depth and termination rule.

## Fades: all 24 records

All spatial fades also service palette/time/input; that does not make them palette-only.
Policy **I** = ordinary logical writes invalidate target and supplying-owner fragments;
draw the fade's own pixels classically. **F** = retire the active page's layer before the
operation and keep that operation Classic. **T** = keep layer in VRAM space and transform
the readout with the exact display-address mapping. These policies include interrupted
and repeated fades. Global palette clearing at DZFD:2252/2579 is handled separately.

| Key / record -> function / line | Category; actual operations and coordinate continuity | State, limits, wrap evidence | High-resolution layer policy; Extended |
|---|---|---|---|
| a rAndomize -> randomize_transition, 390 | Procedural texture; LFSR visits square cells, halved sizes, `square` P(1); requests EGA XOR | No explicit pixel reads; XOR is read/modify/write on EGA; word LFSR 0x6000, S cell grid, sizes 64 down to 2/4 | I on replace VGA; F on EGA XOR. Extended needs a wider cell-grid adapter; otherwise fallback |
| b Blister -> fade_blister, 2136 | Point/raster bitmap stamps at random positions; nonzero resource samples P(1/3), stride 1/2 | Bitmap dimensions, W,H origins, color offsets; no VRAM reads | I; never smooth bitmap. Extended instances possible, not yet adapted |
| c Countdown -> fade_countdown, 1042 | Point/raster pixel-art digits from rectangle-membership tables; full scans, outside-panel EGA XOR | Fixed panel 40x60, digit 24x44; tables [5][6], [8][4]; **doubled in place on every EGA700 invocation**, word wrap | I on replace; F on XOR. Keep digits raster; Extended requires full-width outer mask, centered original digit |
| d Drip -> drip_transition, 834 | Procedural texture; randomized midpoint fall speeds paint previous Y when each column advances; **does not drag old pixels** | No VRAM reads; 16.16 profiles `fall_speed[641]`, `column_y[641]`, width 2W; original stops at finished_columns >= W | I; no transform of surviving geometry. Extended needs new profiles and explicit stop rule; fallback |
| e Exclusive-or -> fade_exclusive_or, 1561 | Stateful/framebuffer-dependent on EGA; shrinking line pairs P(1), even pass count | EGA XOR implicit pixel reads; integer line state and W/H bound; VGA replaces | F on XOR, I on VGA. No attempt to XOR coverage or restore retired strokes; Extended fallback |
| f Firefly -> firefly_transition, 573 | Fractal/recursive + erasure; orthogonal 16.16 path, square head and five-position trailing eraser/recolor | No explicit reads; stamp sizes 1..8 (EGA doubled), depth 6..9; word slots [17], history [5]; wrapped fixed-point products | I on every stamp/eraser. Keep trail raster; Extended fallback pending recursion/path adaptation |
| g Gas cloud -> gas_cloud, 358 | Procedural texture + stateful recursion; `seed_plasma` reads assignment pixels, writes subdivision points; optional hidden-page clear and vertical reveal | VGA only; local rotate/add RNG (not dz_rand); recursive midpoint/word noise; page 0/2000, physical scroll target 2H+1 | F for generated page; T for reveal of any surviving other page. Extended fallback pending wider plasma/border adapter |
| i spIral -> spiral_transition, 744 | Line-based; 360 independent 16.16 radial samples grow by 1.5 and join old/new radius via lines | No reads/erase; `spiral_x/y[360]`, float seed trig, W,H aspect/limits; fixed-point signed halves | I; fade remains raster, not a single spiraling stroke. Extended fallback |
| j Juggle palette -> fade_dac_to_black(0), 2198 | **Pure palette**; set high bits then decrement palette bytes/upload (masked by DAC) | VGA only; palette buffer/count/offset; no pixel moves/reads/erases | Retain all geometry; current palette animates indices. Extended palette shared across whole canvas |
| k Kleenup -> cleanup_transition, 896 | Line-based + ROP; colored fans to edges, first EGA XOR pass then replace | W,H origins/edges, choose_color wrapped math, two passes; no explicit read | F on XOR; I on replace. Extended needs wider edge targets; fallback |
| l Linewipe -> fade_linewipe, 1757 | Line-based + stateful erasure; old line from ring buffer erased with zero or EGA XOR, new reflected line added | History allocation 250 endpoints, trail 82..248, S bounces; no explicit read, XOR implicit | I for zero erasure; F for actual XOR. Keep fade raster; Extended fallback |
| m Mesh -> mesh_transition, 449 | Point/raster lattice; halve spacing from 256/512 to 1, coordinate/color rules | W,H/S bounds, word spacing and choose_color products; no reads/erase | I; do not join sparse mesh dots. Extended needs wider lattice coverage; fallback |
| o firewOrks -> random_delay_transition / randomize_helper, 316/119 | Point/raster particles; dispatcher first palette-fades, resets page/mode/background, then 4x64 timed bursts, delayed tails erase/recolor or XOR | Fixed particles [4][64], positions/velocities **signed 1/64 word wrap**, gravity; W,H aspect; mode reset at dispatcher 2406 | F on mode reset and any XOR. Retain palette only until reset; particle trails stay raster. Extended fallback |
| p Pan/scroll -> display_boundary_fade_and_pan, 2493 | Display addressing, not a dispatch case: draw hidden page, change display start; optional moving line-compare split | Physical EGA350/VGA400 pages (para 1750/2000); retrace and hardware register masks; later palette fade/clear may follow | T for both layer and base, same row/split/wrap. Clear retires only written addresses; Extended requires matching side pages or fallback |
| q Quilt -> quilt_transition / swap_pixel_rectangles, 334 / DZLE:385 | **Pixel moving/readback**; random tiles exchanged with optional row/column reversal, reflected P(1) | 2..8 subdivisions, W,H tile sizes; EGA700 aligns/steps two software rows; sequential swaps can overlap | F **prototype**. Later transform fragments and ownership per actual ordered cell swaps (including reflections), not whole-screen transform; Extended fallback |
| r Rectangles -> fade_rectangles / swap_rectangle, 1722/1673 | Line-based; expanding rotated outlines, optional smaller zero-color outline; **no pixel swap despite helper name** | S size, trig [64], angle +/- modulo 64; no reads; erasure is fresh plotting | I for paint/zero outlines. Extended fallback; a future geometric fade may emit each edge separately |
| s Starburst -> fade_starburst, 1866 | Point/raster + erasure; outward 16.16 heads and delayed zero-color tails, P(1) | Dynamic arrays of 75..149 heads/tails, signed 32-bit state; W,H bounds; no reads | I, keep points raster. Extended side-particle adapter later |
| t Twist -> fade_twist, 1619 | Line-based; four float corners repeatedly move toward neighbors, rasterized edges P(0/1) | Four corner/next arrays each, S initial square, float weights/truncation; no reads/erase | I; do not transform the previous image as a rotating sheet. Extended fallback |
| u Umbra -> fade_umbra, 1243 | Line-based; opposing edge-to-edge fans, P(0/1), decreasing VGA colors | VGA-only dispatch; W,H bounds, line pairs and palette offset; no reads | I; remaining geometry only where not overwritten. Extended wider endpoint adapter later |
| v moVingbars -> fade_moving_bars, 1340 | Point/raster filled bars; paint columns/rows, optional delayed zero bar (some VGA variants leave trail) | VGA-only dispatch; [8][7] table; W,H lengths/trail separation; no reads | I including every eraser store. Extended full-width spans needed; fallback |
| w Wipe -> fade_wipe, 1961 | Point/raster; reflected colored columns **or zero rows** | W,H bounds; EGA700 skips odd software rows; no reads | I; every written cell retires geometry. Extended full-width loops needed; fallback |
| x heXagon -> hexagon_transition, 2024 | Raster contour; repeated expanding integer hexagon passes P(1), optional EGA XOR | S stop and original hexagon word state; may chain Starburst in dispatcher | F on XOR, I otherwise; evaluate chained effect independently. Extended fallback |
| y tetrYs -> fade_tetris, 1448 | **Pixel readback / reconstruction**; sampled-color symmetric squares coarsen picture, sampled blocks rise with zero trailing edges | W,H; block levels 4/5; four per-column arrays [21]; read colors control empty-column traversal | F before first sample; never move pre-coarsening vectors with blocks. Extended requires new column arrays and readback canvas; fallback |
| z Ziti -> fade_ziti, 1127 | Line-based raster wipe; diagonal colored front + delayed zero front P(table symmetry) | VGA-only dispatch; [8][8] table; W,H bounds and trail width; no reads | I; no translation of old geometry. Extended full-width front adapter later |

Only **j** is palette-only among the 24. Palette-to-background (2252, including its
alternative `fade_dac_to_black(1)`) is also palette-only until the caller's subsequent
mode setup/clear; a black palette and `display_cleared=1` do not prove VRAM was erased.
The ordinary image set has no page-copy primitive. Pan changes readout; Quilt moves
individual pixels; tetrYs reconstructs sampled blocks. A future generic page-copy API
still needs the explicit Hybrid rule in the design.

## Decisions and verification limits

Prototype: **cycloids, sine, sweep2**, followed by **lotsadots** over existing geometry
to exercise mixed rendering and overwrite retirement. Arches/cones are good second-stage
ellipse emitters once cache prefixes and EGA rejection are covered. Do not prioritize
filled shaded polygons or framebuffer-driven fades for the first prototype.

Classification is a source audit, not a claim that any Hybrid/Extended variant exists
or has passed differential tests. The accompanying design records the unchanged native
test run and offline shader probe. Extended strategies above are proposed compositions;
unadapted fades and images use Classic fallback, without changing selection or RNG.
