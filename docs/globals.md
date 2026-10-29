# DGROUP globals named during the readability pass

Formerly `g_XXXX` placeholders (XXXX = offset in DGROUP 29BF). Meanings were inferred from every use in the
reconstructed sources (names only; renaming changed no compiled byte). "Role-based" entries mark state whose
purpose varies between images or is unused.

| DS offset | Name | Meaning |
|---|---|---|
| 0479 | `fade_randomize_enabled` | Alias of fade_records[0].enabled, the rAndomize (A) setting; dispatch_fade_key tests it before the LFSR square transition. |
| 0480 | `fade_blister_enabled` | Alias of fade_records[1].enabled, the Blister (B) setting; gates fade_blister. |
| 0487 | `fade_countdown_enabled` | Alias of fade_records[2].enabled, the Countdown (C) setting; gates fade_countdown. |
| 048E | `fade_drip_enabled` | Alias of fade_records[3].enabled, the Drip (D) setting; gates drip_transition. |
| 0495 | `fade_exclusive_or_enabled` | Alias of fade_records[4].enabled, the Exclusive-or (E) setting; gates fade_exclusive_or. |
| 049C | `fade_firefly_enabled` | Alias of fade_records[5].enabled, the Firefly (F) setting; gates firefly_transition. |
| 04A3 | `fade_gas_cloud_enabled` | Alias of fade_records[6].enabled, the Gas cloud (G) setting; dispatch also requires VGA. |
| 04AA | `fade_spiral_enabled` | Alias of fade_records[7].enabled, the spIral (I) setting; gates spiral_transition. |
| 04B1 | `fade_juggle_palette_enabled` | Alias of fade_records[8].enabled, Juggle palette (J); gates fade_dac_to_black(0) and controls whether the fallback palette fade can retain the DAC high bits. |
| 04B8 | `fade_kleenup_enabled` | Alias of fade_records[9].enabled, the Kleenup (K) setting; gates cleanup_transition. |
| 04BF | `fade_linewipe_enabled` | Alias of fade_records[10].enabled, the Linewipe (L) setting; gates fade_linewipe. |
| 04C6 | `fade_mesh_enabled` | Alias of fade_records[11].enabled, the Mesh (M) setting; gates mesh_transition. |
| 04CD | `fade_fireworks_enabled` | Alias of fade_records[12].enabled, firewOrks (O); gates the palette/background reset followed by random_delay_transition's particle bursts. |
| 04D4 | `fade_pan_scroll_enabled` | Alias of fade_records[13].enabled, Pan/scroll (P); permits hidden-page display transitions and the optional page pan in gas_cloud. |
| 04DB | `fade_quilt_enabled` | Alias of fade_records[14].enabled, the Quilt (Q) setting; gates quilt_transition. |
| 04E2 | `fade_rectangles_enabled` | Alias of fade_records[15].enabled, the Rectangles (R) setting; gates fade_rectangles. |
| 04E9 | `fade_starburst_enabled` | Alias of fade_records[16].enabled, Starburst (S); gates both direct selection and the optional continuation after heXagon. |
| 04F0 | `fade_twist_enabled` | Alias of fade_records[17].enabled, the Twist (T) setting; gates fade_twist. |
| 04F7 | `fade_umbra_enabled` | Alias of fade_records[18].enabled, Umbra (U); dispatch also requires VGA. |
| 04FE | `fade_moving_bars_enabled` | Alias of fade_records[19].enabled, moVingbars (V); dispatch also requires VGA. |
| 0505 | `fade_wipe_enabled` | Alias of fade_records[20].enabled, the Wipe (W) setting; gates the randomly selected wipe orientation. |
| 050C | `fade_hexagon_enabled` | Alias of fade_records[21].enabled, the heXagon (X) setting; gates hexagon_transition. |
| 0513 | `fade_tetris_enabled` | Alias of fade_records[22].enabled, the tetrYs (Y) setting; gates fade_tetris. |
| 051A | `fade_ziti_enabled` | Alias of fade_records[23].enabled, Ziti (Z); dispatch also requires VGA. |
| 0A38 | `tiles_direction_x` | Four constant X steps (-1,0,1,0) used with the matching Y table to walk a rectangular tile outline. |
| 0A40 | `tiles_direction_y` | Four constant Y steps (0,-1,0,1) used with the matching X table to walk a rectangular tile outline. |
| 0A48 | `tiles_side_lengths` | Two randomized outline lengths, X then Y, each below half the corresponding drawing dimension; fill and outline share them, with enhanced-EGA Y scaling. |
| 0A4C | `yarnball_aspect_scale` | Initialized float 0.75; yarnball multiplies the screen half-width/half-height ratio by it to correct horizontal circle dimensions. |
| 0A50 | `countdown_digit_segments` | Five rows of zero-terminated, one-based rectangle IDs select the segments of each countdown digit in fade_countdown. |
| 0A8C | `countdown_segment_bounds` | Eight rectangle bounds (left, upper Y, right, lower Y) describe the countdown digit strokes; fade_countdown doubles these coordinates for enhanced EGA. |
| 0ACC | `ziti_sweep_patterns` | Eight rows select signed screen-edge multipliers, diagonal X/Y steps, trailing-line separation sign, and plotting symmetry for fade_ziti. |
| 0B4C | `moving_bars_patterns` | Eight rows select sweep endpoints, step direction, trailing-bar offset sign, cross-axis bounds, and plotting symmetry for fade_moving_bars. |
| 4DAE | `polygon_max_y` | Largest untranslated vertex Y, recomputed by draw_polygon on rebuild to bound the cached scanlines. |
| 4DB0 | `polygon_min_y` | Smallest untranslated vertex Y; subtracted while building scanlines and restored when plotting them. |
| 4DB2 | `polygon_origin_x` | Saves px[0] as the polygon's X translation while that array element is temporarily zero; added to every plotted vertex/span. |
| 4DB4 | `polygon_origin_y` | Saves py[0] as the polygon's Y translation while that array element is temporarily zero; added back during drawing. |
| 4DB6 | `polygon_row_limit` | Maximum cached scanline index, polygon_max_y minus polygon_min_y; scanline loops include index zero through this value. |
| 4DB8 | `ellipse_step_radius_x` | Long horizontal radius retained by ellipse_step; either the explicit radius_x or an aspect-corrected radius derived from radius_y. |
| 4DBC | `ellipse_step_radius_y` | Long vertical radius retained by ellipse_step from its radius_y argument during initialization. |
| 4DC0 | `ellipse_step_radius_x_sq` | Horizontal radius squared, used in the midpoint decision value and vertical-coordinate updates. |
| 4DC4 | `ellipse_step_radius_y_sq` | Vertical radius squared, used in the midpoint decision value and horizontal-coordinate updates. |
| 4DC8 | `ellipse_step_twice_x_sq` | Twice the squared horizontal radius; subtracted from the Y gradient whenever the iterator decreases draw_y. |
| 4DCC | `ellipse_step_twice_y_sq` | Twice the squared vertical radius; added to the X gradient whenever the iterator increases draw_x. |
| 4DD0 | `ellipse_step_error` | Signed long midpoint decision accumulator, updated in both ellipse regions to decide when to step the other coordinate. |
| 4DD4 | `ellipse_step_gradient_x` | Twice radius_y squared times current X; starts at zero, grows on X steps, and participates in the region switch. |
| 4DD8 | `ellipse_step_gradient_y` | Twice radius_x squared times current Y; starts at the top of the ellipse and decreases on Y steps. |
| 4DDE | `ellipse_trace_index` | Index into ellipse_direction_steps while constructing or replaying the cached quadrant outline; reset separately for fill and edge passes. |
| 4DE0 | `ellipse_trace_x` | Previous iterator X during cache construction, then the current positive quadrant X while replaying the ellipse trace. |
| 4DE2 | `ellipse_trace_y` | Previous iterator Y during cache construction, then the current positive quadrant Y while replaying the ellipse trace. |
| 535C | `ellipse_trace_done` | Initialization/completion flag passed to and returned by ellipse_step while draw_ellipse builds its direction cache. |
| 535E | `ellipse_fill_x` | Horizontal offset scanned downward across each filled ellipse row, initially one less than the cached quadrant boundary X. |
| 5360 | `ellipse_fill_min_x` | Inclusive lower limit for ellipse_fill_x: zero for mirrored half-span coloring, otherwise the negative starting X. |
| 5362 | `ellipse_fill_mode` | Cached flags masked with 0x1c; selects constant, per-pixel/mirrored, or per-row fill-color progression. |
| 5364 | `ellipse_fill_color` | Saved initial fill argument, restored at the beginning of rows for fill modes that advance color horizontally. |
| 5366 | `ellipse_trace_limit` | Cached terminal trace index produced on rebuild; both fill and outline loops replay directions through this limit. |
| 5368 | `arches_first_center_y` | Vertical center of the first ellipse in arches, fixed to screen_half_height minus one at rebuild. |
| 536A | `arches_first_radius_y` | Vertical radius of the first arch ellipse; starts at one or two times screen_half_height minus one and decreases by eight each call. |
| 536C | `arches_first_radius_x` | Fixed horizontal radius of the first arch ellipse, initialized to screen_half_width minus one. |
| 536E | `arches_second_center_x` | Horizontal center of the second ellipse in arches, fixed to screen_half_width minus one at rebuild. |
| 5370 | `arches_second_radius_y` | Fixed vertical radius of the second arch ellipse, initialized to screen_half_height minus one. |
| 5372 | `arches_second_radius_x` | Horizontal radius of the second arch ellipse; starts at one or two times screen_half_width minus one and decreases by eight each call. |
| 5374 | `atom_radius` | Radius used for atom's sine/cosine coordinates; starts at `screen_extent`/2 and decreases by `screen_extent`/20 after each phase sweep. |
| 5376 | `atom_phase` | Angular accumulator scaled by 0.0002 radians for atom; advanced once per horizontal stroke and reset when the sweep ends. |
| 5378 | `atom_stroke_index` | Modulo-16 call counter; zero computes a new arc point and the remaining calls advance draw_x to thicken it horizontally. |
| 537A | `balls_sample_y` | Y index into the `ball_bitmap_pixels` ball image, initially `ball_bitmap_height`/2 and advanced by the spiral sampling walk. |
| 537C | `balls_sample_x` | X index into the `ball_bitmap_pixels` ball image, initially `ball_bitmap_width`/2; image access uses X times `ball_bitmap_height` plus Y. |
| 537E | `balls_origin_x` | Random screen X translation for the current ball; the emitted coordinate adds balls_sample_x divided by `effect_parameter`. |
| 5380 | `balls_origin_y` | Random screen Y translation for the current ball; the emitted coordinate adds balls_sample_y divided by `effect_parameter`. |
| 5382 | `balls_run_progress` | Counts sampling steps within the current straight spiral run; reaching balls_run_length selects the next direction. |
| 5384 | `balls_run_length` | Sampling-run threshold, initialized to zero and increased by two after each four-direction cycle of the outward walk. |
| 5386 | `balls_step_x` | Current sampling X increment, direction_x[balls_direction] multiplied by the sampling stride in `effect_parameter`. |
| 5388 | `balls_step_y` | Current sampling Y increment, direction_y[balls_direction] multiplied by the sampling stride in `effect_parameter`. |
| 538A | `balls_direction` | Index zero through three into direction_x/direction_y; advances when a sampling run ends, with sentinel five forcing initial setup. |
| 538C | `bars_start_x` | X coordinate of the current bar-line start; random initially and incremented between lines when `effect_parameter` selects horizontal motion. |
| 538E | `bars_start_y` | Y coordinate of the current bar-line start; random initially and incremented between lines when `effect_parameter` selects vertical motion. |
| 5390 | `bars_end_x` | X endpoint passed to line_step, maintained as bars_start_x plus bars_span_x. |
| 5392 | `bars_end_y` | Y endpoint passed to line_step, maintained as bars_start_y plus bars_span_y. |
| 5394 | `bars_span_x` | Positive X displacement of every parallel bar line, chosen at rebuild from random(`screen_extent`/6) plus `screen_extent`/6. |
| 5396 | `bars_span_y` | Positive Y displacement of every parallel bar line, chosen at rebuild from random(`screen_extent`/6) plus `screen_extent`/6. |
| 5398 | `bars_line_done` | Initialization/completion flag passed to and returned by line_step; completion advances the line start and chooses the next color. |
| 539A | `bigstar_axis_x` | Horizontal axis endpoint of the current star scale; starts at 1.25 times screen_half_width minus one and halves with each smaller layer. |
| 539C | `bigstar_axis_y` | Vertical axis endpoint of the current star scale; starts at 1.5 times screen_half_height minus one and halves with each smaller layer. |
| 539E | `bigstar_diagonal_x` | Diagonal target X, initialized to bigstar_axis_x times 0.707 and halved together with the star axes. |
| 53A0 | `bigstar_diagonal_y` | Diagonal target Y, initialized to bigstar_axis_y times 0.707 and halved together with the star axes. |
| 53A2 | `bigstar_target_x` | Current line endpoint X; first sets the aspect-corrected seed-trace extent, then cycles among axis and diagonal star targets. |
| 53A4 | `bigstar_target_y` | Current line endpoint Y; first sets the seed-trace height, then cycles among vertical, diagonal, and horizontal star targets. |
| 53A6 | `bigstar_line_done` | Initialization/completion flag for line_step, used both to build the seed traces and to draw lines from them to star targets. |
| 53A8 | `bigstar_restart` | Requests a new star on the next call; set when the shrinking inner radius reaches four or less and cleared at initialization. |
| 53AA | `bigstar_segment_state` | State selecting the next axis/diagonal target and trace transition; values 0 through 6 are adjusted when either seed trace is exhausted. |
| 53AC | `bigstar_resize_pending` | Flag requesting halving of the star and rebuilding its seed traces; set initially and when both traces have been consumed. |
| 53AE | `bigstar_trace_step` | Stride through cached seed points, computed as one shifted by bigstar_spacing_shift; becomes two, then one as layers shrink. |
| 53B0 | `bigstar_spacing_shift` | Exponent controlling seed-point spacing; starts at two, decrements on each resize, and is clamped to zero. |
| 53B2 | `bigstar_color_mode` | Selects color progression: zero per trace cycle, one per plotted line point, or two with EGA's four-step color pacing. |
| 53B4 | `bigstar_steep_count` | Number of cached x1/y1 points built by walking from the origin toward the steeper seed-line endpoint. |
| 53B6 | `bigstar_shallow_count` | Number of cached x2/y2 points built by walking from the origin toward the shallower seed-line endpoint. |
| 53B8 | `bigstar_trace_index` | Current index into bigstar_traces; advances by bigstar_trace_step after a target cycle and controls when each trace is exhausted. |
| 53BA | `bigstar_use_steep_trace` | Boolean choosing the cached x1/y1 seed trace when set, otherwise x2/y2; changed by the star-target state machine. |
| 53BC | `bigstar_base_color` | Random starting palette index retained so the per-point color mode can restart each line with the same color. |
| 53BE | `bigstar_traces` | 175 StarTracePoint records holding two seed-line point sequences, x1/y1 and x2/y2, used as starts of the star's drawn lines. |
| 5936 | `bigstar_inner_radius` | Floating seed-trace radius, initially 0.3 to 0.5 times the initial star Y extent; halved with each layer until at most four. |
| 593A | `boxes_origin_x` | Fixed X origin of the current box fan; every completed line returns the pen here before advancing its perimeter target. |
| 593C | `boxes_origin_y` | Fixed Y origin of the current box fan; every completed line returns the pen here before advancing its perimeter target. |
| 593E | `boxes_width` | Random horizontal box extent, from random(`screen_extent`/4) plus `screen_extent`/4; determines the target's horizontal jump and traversal limit. |
| 5940 | `boxes_height` | Random vertical box extent, from random(`screen_extent`/4) plus `screen_extent`/4; determines the target's vertical traversal limit and return jump. |
| 5942 | `boxes_target_x` | X endpoint of the line from the fixed origin; moves across the rectangular perimeter as the box fan is drawn. |
| 5944 | `boxes_target_y` | Y endpoint of the line from the fixed origin; moves across the rectangular perimeter as the box fan is drawn. |
| 5946 | `boxes_line_done` | Initialization/completion flag for the current line_step traversal from the box origin to its perimeter target. |
| 5948 | `boxes_edge_state` | One-based perimeter state: horizontal jump, downward sweep, leftward sweep, vertical return, then request a new box. |
| 594A | `boxes_edge_progress` | Counts target advances on the current box edge; compared with boxes_height or boxes_width and reset at edge transitions. |
| 594C | `boxes_restart` | Set when the perimeter state machine finishes and cleared when a new random box is initialized. |
| 594E | `corkscrew_along` | Floating displacement along the diagonal (1,1), distance times (1 minus cos(phase)/4), before integer conversion to drawing coordinates. |
| 5952 | `corkscrew_across` | Floating displacement across the diagonal (-1,1), distance squared divided by `screen_extent` times sin(phase). |
| 5956 | `corkscrew_distance` | Growing floating distance parameter, reset to zero for each corkscrew and increased by corkscrew_growth_step each call. |
| 595A | `corkscrew_growth_step` | Random distance increment of 0.02 or 0.04 per generated point; controls how quickly the corkscrew expands. |
| 595E | `corkscrew_phase` | Sine/cosine phase in radians, reset to zero and increased by 0.02 per generated point. |
| 5962 | `corkscrew_origin_x` | Random X translation below `screen_extent`/2 for the current corkscrew, added to its along-minus-across displacement. |
| 5964 | `corkscrew_origin_y` | Random Y translation below `screen_extent`/2 for the current corkscrew, added to its along-plus-across displacement. |
| 5966 | `corkscrew_color_counter` | Modulo-16 point counter; every wrap advances plot_color, independently of the corkscrew's geometric phase. |
| 5968 | `curves_stroke_index` | Modulo-16 call counter; zero computes a new sine/cosine point and the other calls extend a horizontal stroke. |
| 596A | `curves_phase_x` | X cosine phase in units of 0.001 radians; starts at one, advances by curves_phase_step_x, and resets above 32000. |
| 596C | `curves_phase_y` | Y sine phase in units of 0.001 radians; starts at one, advances by curves_phase_step_y, and resets above 32000. |
| 596E | `curves_phase_step_x` | Random X-phase increment from two through nine, applied once per new curve point rather than every thickening call. |
| 5970 | `curves_phase_step_y` | Random Y-phase increment from four through nine, applied once per new curve point rather than every thickening call. |
| 5972 | `curves_amplitude` | Common sine/cosine amplitude for curves, initialized by truncating `screen_extent` divided by 1.5. |
| 5974 | `cycloids_start_phase` | Starting orbit phase in the 64-entry trig table, initially 48 and rotated by cycloids_phase_advance between curves; also the closure target. |
| 5976 | `cycloids_orbit_phase` | Current phase of the rolling-circle center, advanced modulo 64 by cycloids_orbit_step. |
| 5978 | `cycloids_spin_phase` | Rolling-circle spin accumulator, initially zero and advanced modulo 64 by the integer outer/rolling radius ratio. |
| 597A | `cycloids_pen_phase` | Nonnegative modulo-64 difference between orbit and spin phases; selects the tracing-point displacement in trig_table. |
| 597C | `cycloids_outer_radius` | Fixed-circle radius for the current cycloid family, initialized to three quarters of screen_half_height. |
| 597E | `cycloids_rolling_radius` | Random rolling-circle radius; subtracted from the outer radius to obtain the center orbit and divides it to obtain the spin rate. |
| 5980 | `cycloids_pen_radius` | Distance of the tracing point from the rolling-circle center; changed between curves by cycloids_pen_radius_step and reflected positive. |
| 5982 | `cycloids_orbit_radius` | Radius of the rolling-circle center's orbit, cycloids_outer_radius minus cycloids_rolling_radius. |
| 5984 | `cycloids_curve_limit` | Random limit from one through ten for cycloids_curve_index; reaching it regenerates the geometric parameters. |
| 5986 | `cycloids_color_per_point` | Boolean selecting color changes at every rasterized line point; zero instead advances the color between complete curves. |
| 5988 | `cycloids_pen_radius_step` | Signed change in tracing radius between curves, chosen as plus or minus one eighth of the initial pen radius. |
| 598A | `cycloids_phase_advance` | Random zero-through-four advance in the 64-entry starting phase between successive curves. |
| 598C | `cycloids_orbit_step` | Orbit-phase increment, initialized to one trig-table entry; also multiplied by the radius ratio to form cycloids_spin_step. |
| 598E | `cycloids_spin_step` | Spin-phase increment, integer cycloids_outer_radius/cycloids_rolling_radius times cycloids_orbit_step, applied modulo 64. |
| 5990 | `cycloids_center_x` | Current X component of the rolling-circle center, orbit radius times the current orbit-phase cosine, before aspect scaling. |
| 5992 | `cycloids_center_y` | Current Y component of the rolling-circle center, orbit radius times the current orbit-phase sine. |
| 5994 | `cycloids_pen_x` | Current X displacement of the tracing point from its rolling-circle center, pen radius times the relative-phase cosine. |
| 5996 | `cycloids_pen_y` | Current Y displacement of the tracing point from its rolling-circle center, pen radius times the relative-phase sine. |
| 5998 | `cycloids_line_done` | Initialization/completion flag for line_step while rasterizing the segment between consecutive parametric cycloid samples. |
| 599A | `cycloids_curve_index` | Zero-based curve index in the current family; incremented when preparing another completed-curve successor and checked against the random limit. |
| 599C | `cycloids_next_curve` | Flag set after drawing a closed curve; the next call clears it and advances the pen radius, start phase, color, and curve index. |
| 599E | `cycloids_aspect_scale` | 16.16 fixed-point horizontal aspect factor, (screen_half_width/screen_half_height) times 3/4, applied to final cycloid X coordinates. |
| 59A2 | `daisies_saved_x` | Saves the current ray's untranslated X while four reflected copies are plotted about the daisy center; then restores line_step state. |
| 59A4 | `daisies_saved_y` | Saves the current ray's untranslated Y while four reflected copies are plotted about the daisy center; then restores line_step state. |
| 59A6 | `daisies_edge_end_x` | Unscaled X endpoint of the next angular edge segment, computed from the fixed-point sine table and current daisy radius. |
| 59A8 | `daisies_edge_end_y` | Y endpoint of the next angular edge segment, computed from the fixed-point cosine table and current daisy radius. |
| 59AA | `daisies_center_x` | Random X translation, random(`screen_extent`/2) plus `screen_extent`/4, around which the four reflected ray copies are drawn. |
| 59AC | `daisies_center_y` | Random Y translation, random(`screen_extent`/2) plus `screen_extent`/4, around which the four reflected ray copies are drawn. |
| 59AE | `daisies_sample_index` | Current index into the ten-entry sine/cosine tables; advances by daisies_sample_step to select each new edge segment. |
| 59B0 | `daisies_line_done` | Initialization/completion flag for line_step, reused first to cache an angular edge and then to draw its rays toward the origin. |
| 59B2 | `daisies_edge_count` | Number of raster points cached in daisies_edge_points for the current angular edge; bounds the subsequent ray-drawing loop. |
| 59B4 | `daisies_work_index` | Loop index reused for building the ten fixed-point trig entries and for visiting every cached edge point; no separate geometric quantity. |
| 59B6 | `daisies_trig_index` | Index into the 64-entry trig_table while building the ten daisy samples: 0, 1, odd entries 3 through 15, then 16. |
| 59B8 | `daisies_arc_variant` | Random zero/one variant in the restricted-arc branch; selects sample bounds 2..7 or 3..6 and whether a two-sample step is permitted. |
| 59BA | `daisies_radius` | Current radius used for angular edge endpoints; starts between roughly `screen_extent`/6 and `screen_extent`/2 and alternates after each segment. |
| 59BC | `daisies_radius_step` | Signed radius change, initially radius shifted right by one through three; negated then added after every segment to alternate two radii. |
| 59BE | `daisies_sample_step` | Advance of the angular sample index, normally one and optionally two in one restricted-arc variant. |
| 59C0 | `daisies_color_per_ray` | Boolean choosing a color advance between rays; zero restores the base color for each ray and advances color along its points. |
| 59C2 | `daisies_base_color` | Random initial palette index retained so the per-point color variant starts each ray from the same masked color. |
| 59C4 | `daisies_edge_points` | 66 Point16 records caching the current angular edge with aspect-scaled X; each point becomes the start of a ray toward the origin. |
| 5ACC | `daisies_sample_start` | Initial angular table index: zero for the full quarter arc or two/three for restricted arcs; copied into daisies_sample_index at rebuild. |
| 5ACE | `daisies_sample_limit` | End threshold for the angular sample index: nine for a full quarter arc or seven/six for restricted arcs; reaching it starts a new daisy. |
| 5AD0 | `daisies_aspect_scale` | 16.16 fixed-point horizontal aspect factor, (screen_half_width/screen_half_height) times 3/4, applied when caching edge X coordinates. |
| 5AD4 | `daisies_cosine_table` | Ten long 16.16 cosine samples derived from trig_table; multiplied by daisies_radius to obtain Y endpoints. |
| 5AFC | `daisies_sine_table` | Ten long 16.16 sine samples derived from trig_table; multiplied by daisies_radius to obtain X endpoints before aspect scaling. |
| 5B24 | `diag2_color_counter` | Counts points in the variable-color variant; exceeding diag2_color_period resets it and calls choose_color(0). |
| 5B26 | `diag2_color_period` | Threshold for diagonal color-run length; incremented after an X wrap and cycled through zero to twenty. |
| 5B28 | `ellipse_line_done` | Initialization/completion flag used by the ellipse image's line_step calls when drawing radial or crossing lines through each boundary point. |
| 5B2A | `ellipse_region` | Midpoint ellipse image state: zero for the X-major region, one for the Y-major region, and two to initialize another ellipse. |
| 5B2C | `ellipse_draw_mode` | Random zero-through-two image variant: origin-centered rays, translated rays, or two crossing diameters through each boundary point. |
| 5B2E | `ellipse_offset_x` | Current nonnegative boundary X offset in the ellipse image's midpoint walk; starts at zero and grows until the ellipse is complete. |
| 5B30 | `ellipse_offset_y` | Current boundary Y offset in the ellipse image's midpoint walk; starts at the randomly chosen vertical radius and decreases below zero. |
| 5B32 | `ellipse_center_x` | Ellipse image X translation, zero for mode zero and otherwise a random coordinate in the middle half of `screen_extent`. |
| 5B34 | `ellipse_center_y` | Ellipse image Y translation, zero for mode zero and otherwise a random coordinate in the middle half of `screen_extent`. |
| 5B5A | `formula1_x_shift` | Random left shift of one or two applied to draw_x in formula1's color expression. |
| 5B5C | `formula1_y_square_factor` | Random multiplier from one through three for draw_y squared in formula1's color expression. |
| 5B5E | `formula1_color_shift` | Random final right shift from two through five applied to the polynomial color value before masking to the palette. |
| 5B60 | `grid_tile_right` | Inclusive rightmost X of the current eight-pixel grid tile, maintained as grid_tile_left plus seven. |
| 5B62 | `grid_tile_left` | Leftmost X of the current grid tile, saved from grid_next_x and restored whenever the raster walk starts another tile row. |
| 5B64 | `grid_pattern_index` | Index into grid_patterns; combines a selected 64-byte mask's base with the current pixel position and starts a new tile at each 64-byte boundary. |
| 5B66 | `grid_next_x` | X origin queued for the next eight-by-eight tile; advances by eight and wraps to zero at 0x60. |
| 5B68 | `grid_next_y` | Y origin queued for the next tile row; advances by eight when grid_next_x wraps, wraps after 0x60, and doubles for the enhanced EGA plotter. |
| 5B6A | `hexagon_vertical_limit` | Y threshold before the hexagon walk starts decrementing X; starts at one and increases whenever a new expanding run begins. |
| 5B6C | `hexagon_start_x` | Starting X for successive expanding hexagon runs; starts at one, increments after X becomes negative, and is copied into draw_x. |
| 5B6E | `hilbert_previous_x` | X endpoint of the previous Hilbert segment; copied into draw_x before the next line and updated after it finishes. |
| 5B70 | `hilbert_previous_y` | Y endpoint of the previous Hilbert segment; copied into draw_y before the next line and updated after it finishes. |
| 5B72 | `hilbert_target_x` | Accumulated X endpoint of the next Hilbert segment, advanced by the current recursive direction's X displacement. |
| 5B74 | `hilbert_target_y` | Accumulated Y endpoint of the next Hilbert segment, advanced by the current recursive direction's Y displacement. |
| 5B76 | `hilbert_depth` | Remaining Hilbert recursion depth, randomly initialized to three through five, decremented on entry and restored on return. |
| 5B78 | `hilbert_step_x` | X component of the current Hilbert step basis; initialized from screen extent and depth, then swapped and negated by recursion. |
| 5B7A | `hilbert_step_y` | Y component of the current Hilbert step basis; initially zero, then swapped and negated with hilbert_step_x to rotate recursive subcurves. |
| 5B7C | `hilbert_color_per_point` | Boolean choosing color changes after every line-step square; zero instead advances the color once per completed Hilbert segment. |
| 5B7E | `hilbert_stroke_width` | Side length of the square stamped at each Hilbert line point, chosen as 6 shifted right by the random depth selector: six, three, or one. |
| 5B80 | `hilbert_restart` | Flag set after a whole recursive Hilbert drawing finishes; the next outer call clears it and chooses a fresh curve. |
| 5B82 | `lines_stroke_index` | Modulo-four call counter; zero advances the bouncing base point and the other calls extend draw_y to make a four-point vertical stroke. |
| 5B84 | `lines_step_x` | Horizontal base-point velocity, plus or minus one; changes sign at the zero and `screen_extent` bounds. |
| 5B86 | `lines_step_y` | Vertical base-point velocity; initially one and randomized to signed magnitudes one through four when it bounces at either Y bound. |
| 5B88 | `lines_base_y` | Y coordinate of the bouncing base point, kept separate from draw_y so the extra stroke pixels do not affect the trajectory. |
| 5B8A | `pedals_radius` | Current sine/cosine radius; decreases once per eight-call stroke and resets to the next shrinking starting radius when exhausted. |
| 5B8C | `pedals_start_radius` | Starting radius for each inward sweep, initially 3*(`screen_extent`/4) and reduced by sixteen between sweeps until a new drawing starts. |
| 5B8E | `pedals_phase` | Angular accumulator in units of 0.0002 radians; reset at each sweep and advanced by 31 per new parametric point. |
| 5B90 | `pedals_stroke_index` | Modulo-eight call counter; zero computes a new polar point and the other calls thicken it by advancing both draw_x and draw_y. |
| 5B92 | `puffs_origin_x` | X coordinate of the origin for each puff line; initialized from [0,`screen_extent`), restored into draw_x before a new line, and increased by `puffs_drift_step` in the drifting variant. |
| 5B94 | `puffs_origin_y` | Y coordinate of the origin for each puff line; initialized from [0,`screen_extent`), restored into draw_y before a new line, and increased by `puffs_drift_step` in the drifting variant. |
| 5B96 | `puffs_end_x` | X endpoint passed to line_step; first offset from `puffs_origin_x` by `puffs_spread`, then chosen randomly around or above that origin according to `effect_parameter`. |
| 5B98 | `puffs_end_y` | Y endpoint passed to line_step; first offset from `puffs_origin_y` by `puffs_spread`, then chosen randomly around or above that origin according to `effect_parameter`. |
| 5B9A | `puffs_spread` | Coordinate range for choosing line endpoints relative to the puff origin; randomized from `screen_extent`/6 through twice that quantity minus one. Endpoints use offsets in [-spread,spread) or [0,spread), depending on `effect_parameter`. |
| 5B9C | `puffs_line_done` | Completion flag returned by line_step; initialized to one, passed back as the next call's initialize argument, and tested to select the next line's origin and endpoint. |
| 5B9E | `puffs_drift_step` | Byte-sized diagonal drift increment, randomly one or two coordinate units; added to both origin coordinates after each completed line when `effect_parameter` is zero. |
| 5B9F | `ribbons_run_step` | Call counter within the current horizontal ribbon run; post-incremented against `ribbons_run_length` and reset to zero when the pen moves to the next row. |
| 5BA1 | `ribbons_run_length` | Growing horizontal-run threshold; also subtracted from draw_x at each new row. Starts at zero, increases each row, and resets when it exceeds `ribbons_run_limit`. |
| 5BA3 | `ribbons_run_limit` | Upper threshold for `ribbons_run_length`; starts at 24, increases by a random zero through three after each run-length cycle, and wraps to zero above 44. |
| 5BA5 | `sierpinski_vertices` | Array of three Point16 triangle vertices. Initialization sets a right triangle with either zero or center/2 as the lower coordinate; each drawing call moves halfway toward a randomly selected vertex. |
| 5BB1 | `sierpinski_fixed_color` | Random Boolean controlling color changes: one retains the initial plot_color, while zero advances it whenever vertex zero is selected. |
| 5BCF | `squares_side_step` | Step counter along a square side; post-incremented against `squares_side_length`, then reset to zero as the direction advances. |
| 5BD1 | `squares_side_length` | Growing side-length threshold for the cardinal walk; starts at zero, increases after each four-direction circuit, and triggers a new random origin on reaching `squares_side_limit`. |
| 5BD3 | `squares_direction` | Direction index zero through three into direction_x and direction_y; advancing past three completes a square circuit and resets the index. |
| 5BD5 | `squares_step_x` | Current X increment from direction_x[`squares_direction`], added to draw_x each call; one of -1, zero, or one. |
| 5BD7 | `squares_step_y` | Current Y increment from direction_y[`squares_direction`], added to draw_y each call; one of -1, zero, or one. |
| 5BD9 | `squares_side_limit` | Random side-length threshold for restarting the square sequence; chosen from `screen_extent`/4 through twice that quantity minus one, and halved when `effect_parameter` is nonzero. |
| 5BDB | `spiral_leg_step` | Step counter along the current cardinal spiral leg; post-incremented against `spiral_leg_length` and reset at each turn. |
| 5BDD | `spiral_leg_length` | Leg-length threshold for the outward square spiral; starts at zero and increases on every second turn, when `spiral_direction` becomes even. |
| 5BDF | `spiral_color_phase` | Modulo-eight drawing-call counter, reset on spiral initialization; wrapping to zero advances plot_color, so color changes once every eight calls. |
| 5BE1 | `spiral_direction` | Cardinal direction index into direction_x and direction_y; incremented modulo four at each leg boundary, with parity controlling leg-length growth. |
| 5BE3 | `spiral_step_x` | Current X increment selected from direction_x[`spiral_direction`] at a turn and added to draw_x on each call. |
| 5BE5 | `spiral_step_y` | Current Y increment selected from direction_y[`spiral_direction`] at a turn and added to draw_y on each call. |
| 5BE7 | `spiral1_side_step` | Step counter along a diagonal side of the spiral1 diamond; post-incremented against `spiral1_side_length` and reset at each direction change. |
| 5BE9 | `spiral1_side_length` | Growing diagonal-side threshold; increases after each four-direction circuit and causes a restart when it reaches `spiral1_side_limit`. |
| 5BEB | `spiral1_direction` | Index zero through three selecting paired diagonal increments from diamond_x and diamond_y; wrapping completes a circuit, increases side length, shifts X, and advances color. |
| 5BED | `spiral1_step_x` | Current diagonal X increment, selected from diamond_x[`spiral1_direction`] and added to draw_x each call; either -1 or one. |
| 5BEF | `spiral1_step_y` | Current diagonal Y increment, selected from diamond_y[`spiral1_direction`] and added to draw_y each call; either -1 or one. |
| 5BF1 | `spiral1_side_limit` | Random threshold for restarting the growing diamond sequence; chosen from [0,3*(`screen_extent`/4)) and halved when `effect_parameter` is nonzero. |
| 5BF3 | `sweep_rise_counter` | Counts calls since the last Y increment; post-incremented against `sweep_rise_interval`, then reset to zero when draw_y rises. |
| 5BF5 | `sweep_rise_interval` | Threshold for spacing Y increments while X advances each call; starts at zero, increases after every full X sweep, and resets after exceeding `screen_extent`/2. With the post-increment comparison, Y rises every interval plus two calls. |
| 5BF7 | `sweep2_end_y` | Y endpoint passed to line_step for each sweep2 ray; assigned `screen_extent` whenever a new ray begins at (0,0). |
| 5BF9 | `sweep2_end_x` | X endpoint passed to line_step. New rays either increment it modulo `screen_extent`+2 or add `sweep2_end_x_step`, depending on `effect_parameter`. |
| 5BFB | `sweep2_end_x_step` | Signed change to `sweep2_end_x` between rays in the nonzero `effect_parameter` variant; starts at zero, increases by one per ray, and wraps from above `screen_extent` to -`screen_extent`. |
| 5BFD | `sweep2_line_done` | Completion result of line_step; a nonzero result starts the next ray, resets the pen to (0,0), changes the endpoint and color, and initializes the iterator. |
| 5BFF | `triangles_row_right` | Inclusive right X bound of the current triangle scanline; initialized to `triangles_row_left` and incremented after each completed row until it reaches `triangles_right_limit`. |
| 5C01 | `triangles_row_left` | Left X bound restored into draw_x at each new scanline; initialized randomly below `screen_extent`/2 and decremented after each row when `triangles_expand_left` is nonzero. |
| 5C03 | `triangles_active` | Flag indicating an existing triangle is being scanned; cleared at initialization, at the right-bound limit, or at the Y boundary, and set when a new random triangle begins. |
| 5C05 | `triangles_right_limit` | Maximum right X bound for a triangle; set to its initial left X plus eight and a random value below `screen_extent`/3, clamped to `screen_extent`. Reaching it ends the triangle. |
| 5C07 | `triangles_expand_left` | Controls whether the left bound moves one coordinate unit left after each row, while the right bound always grows rightward. Initially randomized in zero through two, then logically inverted at every new triangle, yielding alternating Boolean variants. |
| 5C09 | `tunnel_row_left` | Saved left X bound of each tunnel scanline; set to zero when a pass restarts and copied into draw_x after every completed row. |
| 5C0B | `tunnel_rows_left` | Remaining scanline count; initialized to `screen_extent`, decremented at each row boundary, and forced to zero on Y overflow. A nonpositive value restarts the pass. |
| 5C0D | `tunnel_right_limit` | Fixed right X bound, assigned `screen_extent` at each pass restart; `tunnel_right_edge_ptr` can point here for full-width rows instead of following draw_y. |
| 5C0F | `tunnel_right_edge_ptr` | Far pointer to the inclusive right X bound used by the row-completion test; randomly selects `tunnel_right_limit` for fixed-width rows or draw_y for rows that widen with Y. |
| 5C13 | `wandering_start_x` | X coordinate of the wandering line's first endpoint; updated by `wandering_start_dx` once per new segment and copied into draw_x before initializing line_step. |
| 5C15 | `wandering_start_y` | Y coordinate of the wandering line's first endpoint; updated by `wandering_start_dy` once per new segment and copied into draw_y before initializing line_step. |
| 5C17 | `wandering_end_x` | X coordinate of the second endpoint passed to line_step during both initialization and continuation; updated by `wandering_end_dx` only when starting a new segment. |
| 5C19 | `wandering_end_y` | Y coordinate of the second endpoint passed to line_step during both initialization and continuation; updated by `wandering_end_dy` only when starting a new segment. |
| 5C1B | `wandering_segments_left` | Countdown of new line segments before choosing fresh endpoint velocities; reset to a random multiple of four from four through 32 and decremented once per endpoint update. |
| 5C1D | `wandering_start_dx` | X velocity of the first endpoint, randomly -4 through four coordinate units per new segment; negated after its updated X falls below one or exceeds `screen_extent`. |
| 5C1F | `wandering_end_dx` | X velocity of the second endpoint, randomly -4 through four coordinate units per new segment; negated after its updated X falls below one or exceeds `screen_extent`. |
| 5C21 | `wandering_start_dy` | Y velocity of the first endpoint, randomly -4 through four coordinate units per new segment; negated after its updated Y falls below one or exceeds `screen_extent`. |
| 5C23 | `wandering_end_dy` | Y velocity of the second endpoint, randomly -4 through four coordinate units per new segment; negated after its updated Y falls below one or exceeds `screen_extent`. |
| 5C25 | `wandering_line_done` | Completion flag returned by line_step; while zero, continuation calls finish the existing segment. A true result allows endpoint motion, a color change, and initialization of the next segment; the size argument forces that setup even though this flag is cleared first. |
| 5C27 | `zigstream_run_counter` | Run counter reused by the two `effect_parameter` modes: counts upward against `zigstream_run_length` in the growing staircase, but counts downward from a random 12 through 23 calls between direction changes in the bouncing walk. |
| 5C29 | `zigstream_run_length` | Run-length threshold in the zero `effect_parameter` staircase variant; starts at zero and increases after each pair of horizontal/vertical runs. Unused by the other variant. |
| 5C2B | `zigstream_color_phase` | Modulo-eight drawing-call counter used by the zero `effect_parameter` staircase variant; wrapping to zero advances plot_color. Reset for a new path and unused by the other variant. |
| 5C2D | `zigstream_direction` | Zero-or-one direction index into zigstream_dx and zigstream_dy for the staircase variant: zero is vertical and one horizontal. Returning to zero increases the run-length threshold; unused by the bouncing variant. |
| 5C2F | `zigstream_step_x` | Current X increment added to draw_x. Selected from zigstream_dx in the staircase variant, or randomly -1/one and reflected at X boundaries in the bouncing variant. |
| 5C31 | `zigstream_step_y` | Current Y increment added to draw_y. Selected from zigstream_dy in the staircase variant, or randomly -1/zero/one and reflected at Y boundaries in the bouncing variant. |
| 5C4F | `ball_bitmap_height` | Shared shaded-ball bitmap Y extent and memory stride in balls and fade_blister; allocated as screen_half_height divided by a random 2..6. The PaletteMapSpec cast presents this first word as width, transposing generator axes relative to drawing. |
| 5C51 | `ball_bitmap_width` | Shared shaded-ball bitmap X extent in balls and fade_blister; allocated as screen_half_width divided by the same random 2..6. It is the height word of the PaletteMapSpec generator view. |
| 5C53 | `ball_bitmap_pixels` | Far allocation of the product of the two ball bitmap dimensions; initialize_palette_mapping fills its circular shaded color indices, then balls and fade_blister read nonzero pixels. |
| 5C57 | `linewipe_history` | Far buffer allocated with 0x7d8 bytes; fade_linewipe treats it as LineWipePoint records containing delayed line endpoints to erase before drawing replacements. |
| 5CB1 | `display_cleared` | Shared logical clear/dark-display flag: image drawing clears it, completed clearing fades and mode reset set it. Display-boundary handling uses it to decide whether a final palette fade is needed; it does not imply VRAM itself is zero after a palette fade. Menus alias it as menu_redraw_needed and set it when restoring/clearing graphics mode. |
| 5CB3 | `image_xor_mode` | Auto and Semi-Auto randomly enable it for image records marked continuous, then pass it to set_ega_raster_xor before drawing that image. |
| 5CB7 | `option_bang_requested` | Role-based: parse_command_line sets this flag for /!; no executable read exists in this build, so the intended feature cannot be established. |
| 5CBB | `image_work_count` | Shared drawing-work counter: auto_loop increments it per image call and helpers add work for internal plotting loops; exceeding `image_work_limit` selects another image. Some effects force a large value when finished; this is not an elapsed-time counter. |
| 5CBD | `image_work_limit` | Auto-loop work budget compared with `image_work_count`; randomized from the selected image's configured size times `screen_extent`, with an additional quarter of that product. |
| 5CBF | `diagonal_symmetry_selected` | Random 0/1 chosen for each new image in both drawing modes; combined with the image record flag to choose draw_symmetry 3 (also swap X/Y) instead of 1. |
| 5D8B | `background_alt_color` | Byte alias of the high half of background_fill_word; paired with its low byte to form the alternating VGA background clear pattern. Fade setup and fireworks tail erasure use that second color. |
| 5D8C | `ega_border_color` | EGA border palette value: randomized at display boundaries, forced to zero by hide_ega_border and palette resets, and supplied to the EGA palette upload through the shared DGROUP word. |
| 5D94 | `palette_pointer_offset` | Low-word alias of the shared far palette_buffer pointer; logo routines use it as BIOS ES:DX's DX and in MK_FP addresses, rather than allocating separate logo storage. |
| 5D96 | `palette_pointer_segment` | High-word alias of the shared far palette_buffer pointer; logo routines use it as BIOS ES and in MK_FP addresses. |
| 5D9E | `semi_auto_continuous` | F8 toggles this flag in poll_graphics_keys; semi_auto_loop clears it on entry and synthesizes another draw command while it remains set. |
| 5DA2 | `default_palette_cycle_ticks` | Base palette-cycle delay set by configure_video_backend: one BIOS tick for VGA and two for EGA; allocation initializes palette_cycle_ticks from it, multiplying VGA by four for 8086 palette I/O. |
| 5DAC | `rectangles_angle_index` | Rectangles fade rotation index into the 64-entry sine/cosine table; advanced modulo 64 and used by swap_rectangle to rotate all four corners. |
| 5DAE | `screen_extent` | Shared center-relative drawing bound and image-work scale set by configure_video_backend: 161/172/201 for VGA modes/options, 321/351 for EGA. Effects use it for coordinate limits and random geometry; it is not the full screen width or height. |
| 5DB8 | `palette_cycle_wrap_bytes` | Set to three times palette_entries; VGA cycle_palette wraps its byte offset at this limit (1728) back to 9, after the three reserved RGB entries. |
| 5DC2 | `option_plus_requested` | Role-based: /+ sets this flag, but there is no reader in this build. The manual associates /+ with ignoring TARGA+64 in a special build; this code does not establish an active effect. |
| 5DC4 | `targa64_mode` | Shared TARGA+64 mode guard: startup labels the adapter TARGA+64 when nonzero and drawing/fade routines suppress incompatible variants. No writer enables it in this EGA/VGA build. |
| 5DDC | `option_y_requested` | Role-based: parse_command_line sets this flag for /Y, but no executable reader or documented feature establishes its intended meaning. |
| 5DE8 | `palette_buffer_bytes` | Usable palette storage length: color_count+palette_entries for EGA (80), three times that sum for VGA (2496). Allocation adds three spare bytes; logo/display resets zero this length and palette fades wrap within it. |
| 6025 | `effect_parameter` | Role-based: shared scratch state has intentionally different meanings across images and fades: ball sampling scale, color-selection mode, movement variant, or plotting symmetry. A single narrower semantic name would misdescribe its uses. |
| 6032 | `butterflies_vertex_index` | Loop index 1..4 used to negate wing X offsets into the mirrored wing array. |
| 6034 | `butterflies_wing_x` | Six-word polygon X array for one butterfly wing: element 0 is the translation, elements 1..4 the changing vertex offsets, and element 5 the closing zero written by draw_polygon. |
| 6036 | `butterflies_wing_x1` | Scalar alias of `butterflies_wing_x`[1]; initialized to a small positive wing width and increased by 4 each update, also used as a divisor for the fourth vertex Y. |
| 6038 | `butterflies_wing_x2` | Scalar alias of `butterflies_wing_x`[2]; second wing vertex X offset is `butterflies_wing_x1` plus `butterflies_tip_offset`. |
| 603A | `butterflies_wing_x3` | Scalar alias of `butterflies_wing_x`[3]; third wing vertex X offset is negative half of `butterflies_wing_x2`. |
| 603C | `butterflies_wing_x4` | Scalar alias of `butterflies_wing_x`[4]; fourth wing vertex shares the third vertex's X offset. |
| 6040 | `butterflies_mirrored_x` | Six-word X array for the second wing, sharing the translation and negating the four nonzero vertex offsets of `butterflies_wing_x`; draw_polygon supplies its closing element. |
| 604C | `butterflies_wing_y` | Six-word polygon Y array shared by both butterfly wings, with translation at element 0, changing vertex offsets at 1..4, and a closing zero at 5. |
| 604E | `butterflies_wing_y1` | Scalar alias of `butterflies_wing_y`[1]; initialized below screen_half_height/16 and increased by 16 per update until the butterfly reaches its height or screen limit. |
| 6050 | `butterflies_wing_y2` | Scalar alias of `butterflies_wing_y`[2]; second wing vertex Y offset is copied from `butterflies_wing_y1`. |
| 6052 | `butterflies_wing_y3` | Scalar alias of `butterflies_wing_y`[3]; third wing vertex Y offset is negative half of `butterflies_wing_y2`. |
| 6054 | `butterflies_wing_y4` | Scalar alias of `butterflies_wing_y`[4]; fourth vertex Y offset is `butterflies_wing_x3`*`butterflies_wing_y1`/`butterflies_wing_x1`, also used in the full-height termination test. |
| 6058 | `butterflies_center_x` | Random butterfly X translation, selected within the central three quarters of screen_half_width and copied into both wing arrays and draw_x. |
| 605A | `butterflies_center_y` | Random butterfly Y translation, selected within the central three quarters of screen_half_height and copied into the shared wing Y array and draw_y. |
| 605C | `butterflies_fill_color` | Random color_count-bounded fill color chosen for each growth update and passed to both wing polygon draws. |
| 605E | `butterflies_edge_color` | Random edge color chosen when a butterfly starts and retained for both wing polygon draws throughout its growth. |
| 6060 | `butterflies_draw_flags` | Polygon drawing flags selected at initialization: basic fill/outline choice plus optional VGA row shading and changing edge colors. |
| 6062 | `butterflies_height_limit` | Random growth limit from screen_half_height/8 to just below 5*screen_half_height/8; exceeding it with `butterflies_wing_y1` requests a new butterfly. |
| 6064 | `butterflies_tip_offset` | Extra horizontal separation, random(screen_half_width/4)+8, added to the first wing vertex X to obtain the second vertex X. |
| 6066 | `butterflies_restart` | Set when wing growth exceeds its random height or screen limit; the next butterflies call initializes a new shape and clears this flag. |
| 6068 | `diamonds_origin_x` | First word of the contiguous diamond X polygon; draw_polygon treats it as translation. It moves by `diamonds_step_x` and bounces between zero and `diamonds_origin_limit_x`. |
| 606A | `diamonds_vertex1_x` | Diamond X polygon element 1, set to half the current full width `diamonds_width`. |
| 606C | `diamonds_vertex2_x` | Diamond X polygon element 2, set to the full width `diamonds_width`, the point opposite the translated origin. |
| 606E | `diamonds_vertex3_x` | Diamond X polygon element 3, set to the same half-width offset as element 1. |
| 6070 | `diamonds_closing_x` | Contiguous X polygon element 4; not directly read in diamonds, but draw_polygon(1,4,&`diamonds_origin_x`,...) writes it to zero to close the diamond. |
| 6072 | `diamonds_origin_y` | First word of the contiguous diamond Y polygon, used as translation; moves by `diamonds_step_y` and bounces between zero and `diamonds_origin_limit_y`. |
| 6074 | `diamonds_vertex1_y` | Diamond Y polygon element 1, set to half the full height `diamonds_height`. |
| 6076 | `diamonds_vertex2_y` | Diamond Y polygon element 2, set to zero so the opposite point is level with the translated origin. |
| 6078 | `diamonds_vertex3_y` | Diamond Y polygon element 3, set to negative `diamonds_vertex1_y`, placing the other tip symmetrically about the origin's Y. |
| 607A | `diamonds_closing_y` | Contiguous Y polygon element 4; draw_polygon(1,4,...,&`diamonds_origin_y`,...) writes this implicit closing element to zero. |
| 607C | `diamonds_width` | Full diamond X span, initialized between 8 and `diamonds_width_limit`, varied by `diamonds_width_step`, and reflected at those bounds; supplies full and half vertex offsets. |
| 607E | `diamonds_height` | Full diamond Y span, initialized between 8 and `diamonds_height_limit`, varied by `diamonds_height_step`, and reflected at those bounds; vertices use positive/negative half this span. |
| 6080 | `diamonds_step_x` | Signed per-update X translation step, magnitude random(screen_half_width/16)+4; negated when the origin leaves its allowed range. |
| 6082 | `diamonds_step_y` | Signed per-update Y translation step, magnitude random(screen_half_height/16)+4; negated at the Y origin bounds. |
| 6084 | `diamonds_width_step` | Random signed width change centered on zero with scale screen_half_width/4; negated when width leaves 8..`diamonds_width_limit`. |
| 6086 | `diamonds_height_step` | Random signed height change, also scaled from screen_half_width/4 in the original code; negated when height leaves 8..`diamonds_height_limit`. |
| 6088 | `diamonds_origin_limit_x` | Maximum diamond X translation, initialized to screen_half_width minus one quarter of it; used for initialization and reflection. |
| 608A | `diamonds_origin_limit_y` | Maximum diamond Y translation, initialized to screen_half_height minus one quarter of it; used for initialization and reflection. |
| 608C | `diamonds_width_limit` | Maximum diamond full width, screen_half_width/2; initialization and width reflection use this bound. |
| 608E | `diamonds_height_limit` | Maximum diamond full height, screen_half_height/2; initialization and height reflection use this bound. |
| 6090 | `gumballs_radius_y` | Vertical ellipse radius passed to draw_ellipse; randomly varied and reflected into 8..screen_half_height/4. A zero explicit X radius requests aspect correction. |
| 6092 | `gumballs_center_x` | Gumball center X, initialized in the central half of screen_half_width and moved by `gumballs_step_x` with bounds screen_half_width/8..screen_half_width. |
| 6094 | `gumballs_center_y` | Gumball center Y, initialized in the central half of screen_half_height and moved by `gumballs_step_y` with bounds screen_half_height/8..screen_half_height. |
| 6096 | `gumballs_radius_step` | Random signed change to the vertical radius each update, scaled from screen_half_height/4; reversed if the radius crosses its allowed bounds. |
| 6098 | `gumballs_step_x` | Random signed X center movement each update, scaled from screen_half_width/4 and reversed when the center crosses its bounds. |
| 609A | `gumballs_step_y` | Random signed Y center movement each update, scaled from screen_half_height/4 and reversed when the center crosses its bounds. |
| 609C | `gumballs_edge_color` | Random color_count-bounded edge color passed to draw_ellipse; the current plot_color supplies its fill color. |
| 609E | `gumballs_draw_flags` | Ellipse flags choose fill versus outline plus optional VGA fill gradients or changing edge colors; rebuilt each gumballs update. |
| 60A0 | `gumballs_shading_choice` | VGA-only random selector 0..3; chooses a random fill color or one of the three fill-gradient flag values 4,8,0x10. |
| 60A2 | `tiles_side_length` | Current side's pixel count: reused as the filled-row X countdown, then loaded from `tiles_side_lengths` for each of the four outline edges. |
| 60A4 | `tiles_edge_progress` | Pixel-step counter within an outline edge; enhanced EGA adds a second count for doubled vertical stepping. |
| 60A6 | `tiles_edge_index` | Outline edge index 0..3; indexes the direction tables and chooses alternating X/Y side lengths. |
| 60A8 | `tiles_move_x` | Random signed change to draw_x before placing a tile; reversed if the new origin is outside screen_half_width/4..screen_half_width-1. |
| 60AA | `tiles_move_y` | Random signed change to draw_y in the effect's working Y coordinate system; reversed at the tile origin bounds. |
| 60AC | `tiles_edge_step_x` | X step for the current outline edge, loaded from `tiles_direction_x` and added after each plotted point. |
| 60AE | `tiles_edge_step_y` | Y step for the current outline edge, loaded from `tiles_direction_y`; enhanced EGA applies a second Y step to preserve physical-row alignment. |
| 60B0 | `tiles_fill_rows_left` | Remaining filled-row Y count, initialized from `tiles_side_lengths`[1]; doubled and decremented twice per row for enhanced EGA. |
| 60B2 | `tiles_base_fill_color` | Persistent base fill color saved from plot_color; reused to restart per-pixel/per-row gradients and updated after filling a tile. |
| 60B4 | `tiles_saved_x` | Tile origin X saved before the optional filled-area scan and restored before drawing its outline. |
| 60B6 | `tiles_saved_y` | Tile origin Y saved before fill traversal and enhanced-EGA scaling, then restored for the outline stage. |
| 60B8 | `tiles_fill_color_mode` | Fill color mode: 0 keeps one color for EGA, VGA selects 1 to vary color along each row or 2 to vary it between rows. |
| 60BA | `tiles_y_extent` | Working Y extent: screen_half_height normally, half that for enhanced EGA; used for random placement, motion, and side lengths before conversion to doubled logical Y. |
| 60BC | `wedges_vertex_x` | Three persistent absolute triangle vertex X coordinates; initialized within the central half of screen_half_width and moved with reflection at the screen bounds. |
| 60C2 | `wedges_vertex_y` | Three persistent absolute triangle vertex Y coordinates; initialized within the central half of screen_half_height and moved with reflection at the screen bounds. |
| 60C8 | `wedges_step_x` | Three signed random X changes applied to corresponding triangle vertices, each reversed at X bounds before relative polygon coordinates are constructed. |
| 60CE | `wedges_step_y` | Three signed random Y changes applied to corresponding triangle vertices, each reversed at Y bounds before polygon drawing. |
| 60D4 | `yarnball_radius_cos30` | Integer radius projection `yarnball_radius_y`*`yarnball_cos30` (0.866); used as a Y circle-center offset and, after aspect scaling, as an X offset. |
| 60D6 | `yarnball_radius_sin30` | Integer radius projection `yarnball_radius_y`*`yarnball_sin30` (0.5); paired with the 0.866 projection for the diagonal positions of the surrounding circles. |
| 60D8 | `yarnball_x_cos30` | Aspect-corrected horizontal version of `yarnball_radius_cos30`; used with positive/negative `yarnball_radius_sin30` to place four surrounding circle centers. |
| 60DA | `yarnball_x_sin30` | Aspect-corrected horizontal version of `yarnball_radius_sin30`; used with positive/negative `yarnball_radius_cos30` to place another four circle centers. |
| 60DC | `yarnball_radius_x` | Current vertical radius times `yarnball_aspect_ratio`, giving the full horizontal center offset for the left and right surrounding circles. |
| 60DE | `yarnball_radius_y` | Vertical radius shared by all yarnball ellipses and by their center offsets; initialized by the selected arrangement and increased by `yarnball_radius_step` after each frame. |
| 60E0 | `yarnball_center_x` | Base X center of the circle cluster; randomized away from the origin in the full arrangement, zero for the sparse arrangement. |
| 60E2 | `yarnball_center_y` | Base Y center of the circle cluster; set beyond twice the initial radius in the full arrangement, zero in the sparse arrangement. |
| 60E4 | `yarnball_draw_flags` | Ellipse outline flags, initially 1; bit 0x20 enables edge-color progression, always set for sparse layout and toggled per frame for full layout. |
| 60E6 | `yarnball_full_ring` | Random 0..2 arrangement selector used as a boolean: nonzero visits all twelve surrounding center offsets, zero visits only indices 0,4,8 and uses a larger origin-centered layout. |
| 60E8 | `yarnball_sparse_shift` | Exponent counter initialized/reset to 1; sparse layout advances the circle index as 2 shifted left by this incrementing value, producing indices 4,8 then termination. |
| 60EA | `yarnball_radius_step` | Per-frame increment added to `yarnball_radius_y`; initialized to one and otherwise unchanged in the reconstructed code. |
| 60EC | `yarnball_aspect_ratio` | Horizontal/vertical correction factor screen_half_width/screen_half_height*`yarnball_aspect_scale`; scales radius projections and initial cluster placement. |
| 60F0 | `yarnball_cos30` | Float initialized to 0.866, approximately cos(30 degrees); multiplies the current radius to place the circle centers at the twelve surrounding angular positions. |
| 60F4 | `yarnball_sin30` | Float initialized to 0.5, sin(30 degrees); paired with the 0.866 coefficient for the surrounding circle centers. |
| 60F8 | `fireworks_origin_x` | Four burst-origin X values in center-relative pixels; randomized for displaced fireworks or zero for origin-centered bursts, then used to seed the 1/64-pixel particle coordinates. |
| 6100 | `fireworks_origin_y` | Four burst-origin Y values in center-relative pixels, paired with `fireworks_origin_x` when initializing all 64 particles of each burst. |
| 6108 | `fireworks_age` | Four burst ages measured in simulation updates; incremented after each active burst pass and compared with particle trail thresholds and next-burst launch delays. |
| 6110 | `fireworks_finished_particles` | Per-burst completed-particle counters; increment when a particle reaches its selected end threshold, and values above 63 deactivate that burst. |
| 6118 | `fireworks_colors` | Four per-burst palette indices initialized in 3..color_count-1; odd trail modes advance these colors after each update. |
| 6120 | `fireworks_launch_delay` | Per-burst age threshold for activating the next of four bursts, selected from screen_half_height/8 plus a random half-height interval; an early finished burst also launches its successor. |
| 6128 | `fireworks_active` | Four active-burst flags; only burst zero starts active, successors are enabled by age/completion conditions, and all-zero ends randomize_helper's simulation. |
| 612A | `fireworks_unused_active_view` | Role-based: header-only int[4] declaration has no definition or executable references. Its offset lies one word into `fireworks_active` and its declared extent overlaps `fireworks_gravity`; it may be a stale interior view, so no independent state meaning is claimed. |
| 6130 | `fireworks_gravity` | Four accumulated vertical velocity additions in 1/64-pixel update units; initialized to zero and increased by `fireworks_acceleration` after each active burst update. |
| 6138 | `fireworks_acceleration` | Shared vertical acceleration, 2 for gravity-enabled fireworks and 0 for the origin-centered alternative; used for head and delayed-tail motion. |
| 613A | `fireworks_background_a` | Tail erasure color for the first background phase, normally zero or background_fill_word's low byte in clear-to-background particle mode. |
| 613C | `fireworks_background_b` | Tail erasure color for the second VGA background phase, copied from background_fill_word's high byte when initializing background-aware fireworks. |
| 613E | `fireworks_trail_mode` | Mode 0 requests EGA raster XOR and delayed tails, 1 draws heads only, 2 erases tails with one background color, 3 keeps color-changing trails, and 4 erases using the alternating VGA background colors. |
| 6140 | `fireworks_background_x_mask` | X-coordinate bit selecting the second background color in trail mode 4: mask 4 for enhanced planar VGA, mask 1 for BIOS-resolution VGA. |
| 6142 | `fireworks_displaced_origins` | Initialization flag choosing randomized burst origins and acceleration 2; its zero alternative keeps all origins at (0,0) with no acceleration. |
| 6144 | `fireworks_particles` | Four arrays of 64 transition_particle_t records: head/tail coordinates and velocities in 1/64 pixels plus trail start/end update thresholds. Only randomize_helper executes accesses; the nearby FF_PART macro is unused. |
| 7344 | `fireworks_aspect_ratio` | Float screen_half_width/screen_half_height*0.75 used to scale initialized horizontal particle velocities for the display aspect. |
| 7348 | `quilt_grid_divisions` | Random subdivision count 2..8 used along both axes; quilt_transition derives tile dimensions and selects rectangle origins on this grid. |
| 734A | `quilt_flip_mode` | Random 0..3 passed as swap_pixel_rectangles' flip argument for the selected pair of quilt tiles. |
| 734C | `quilt_first_x` | First tile's center-relative X origin, selected as a random grid column times `quilt_tile_width`. |
| 734E | `quilt_first_y` | First tile's center-relative Y origin, selected as a random grid row times `quilt_tile_height`. |
| 7350 | `quilt_second_x` | Second tile's X origin selected independently on the quilt grid; passed to swap_pixel_rectangles with the first tile. |
| 7352 | `quilt_second_y` | Second tile's Y origin selected independently on the quilt grid. |
| 7354 | `quilt_tile_width` | Swapped rectangle width in logical pixels, screen_half_width divided by `quilt_grid_divisions`; also spaces the possible tile X origins. |
| 7356 | `quilt_tile_height` | Swapped rectangle height in logical pixels, screen_half_height divided by `quilt_grid_divisions`; also spaces the possible tile Y origins. |
| 7358 | `firefly_x_history` | Five previous plotted X coordinates in a ring buffer; initialized at the left edge and used to erase delayed positions before storing the next recursive-path point. |
| 7362 | `firefly_y_history` | Five Y coordinates paired with `firefly_x_history` in the Firefly delayed-erasure ring buffer, initialized at screen_half_height. |
| 736C | `firefly_path_state` | Seventeen-word state block accessed through FF_STATE/FF_LONG by firefly_transition: recursion depth, history index, colors, square size/background mode, and overlapping 16.16 step/position pairs. It is Firefly state despite the supplied inventory attributing its preceding macros to mesh_transition. |
| 738E | `tetris_column_active` | Twenty-one per-column flags used during the final Tetris clearing stage; a column is inactive when no nonzero sampled blocks remain. |
| 73B8 | `tetris_block_y` | Twenty-one current Y positions: first scan for a nonzero block in each column, then decrement per update while copying its color upward and erasing its trailing edge. |
| 73E2 | `tetris_block_colors` | Twenty-one sampled block palette indices; zero requests the next nonzero source block, and nonzero supplies the moving strip's color. |
| 740C | `tetris_source_y` | Twenty-one saved source-row positions for the current sampled blocks; `tetris_block_y` resets to this row after a block moves above the visible area, then searches onward. |
| 7436 | `saved_pan_draw_delay` | Shared save slot for draw_delay_count while it is forced to 1 during hidden-page drawing or plasma generation; restored by display-boundary panning or gas_cloud. |
| 7456 | `image_source_bytes_low` | Low-word alias of image_source_bytes, not a separate allocation; logo routines combine it with the split-buffer image_start_offset to locate the final VGA palette and its marker. |
