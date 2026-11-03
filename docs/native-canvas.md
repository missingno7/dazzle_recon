# Native canvas: resolution-independent drawing (agreed design, 2026-11-03)

Decisions by the project owner (supersede the Hybrid overlay in docs/native-hybrid.md; Extended adapters stay):

| Topic | Decision |
|---|---|
| Source of truth | One canvas API used by all 42 images and 24 fades: plot/stamp, line, ellipse, fill, pixel read, XOR, rectangle copy, scroll, clear, pages and display start/split, in continuous coordinates. |
| Classic | The same API rendered at the original logical resolution into `dz_video.vram`; stays byte-exact against the DOS reference tests. |
| Colour | Full-resolution 8-bit index buffer; palette lookup on the GPU at presentation, so palette cycling animates everything already drawn. |
| Rasterization | CPU into the index buffer; the GPU only presents (existing indexed presenter). |
| Canvas size | Screen resolution (the physical output; resize restarts the current image). |
| Effect look | Decided per effect by rewriting its intention: continuous geometry (splines, lines, circles) drawn natively at full resolution; pixel/raster effects may stay pixel-based. |
| Per-effect choice | Each effect with a native version is selectable Faithful or Native in the settings (Images tab); default Native. |
| Pacing | Same visual tempo as the 486 original: timing is defined per original step, not per pixel. |
| Aspect | Native effects lay themselves out for the canvas's real aspect ratio; non-native effects use Original/Crop/Stretch or the Extended adapters. |
| Old code | Hybrid overlay removed once the canvas works; Extended adapters kept for non-native effects. |

## Phases

1. **Canvas API with the Classic backend.** Every write/read path that touches display memory today (dvdt.c kernels,
   line/ellipse helpers, fades' direct reads/writes/copies, clears, mode set, page/display start, PCX save, logo)
   goes through the canvas. No visual or timing change: existing tests and DOS-reference comparisons stay exact.
2. **Screen-resolution canvas, Faithful rendering.** The canvas owns a screen-resolution index buffer with the
   VRAM aperture's page layout scaled; a faithful plot stamps the block covering one original pixel (block edges
   use the presenter's nearest-sampling rounding), reads sample the block, rectangle/scroll/copy operations scale.
   Oracle: for the same run, the screen-resolution canvas must equal the Classic presenter's output at that size,
   and RNG, work and virtual time must equal Classic.
3. **Native effects, one at a time.** Per effect: rewrite its intention against the canvas in canvas pixel units
   (aspect-aware layout, same visual tempo), keep the Faithful path selectable, add its own tests and captures.
   Open item for this phase: antialiasing in an indexed buffer (e.g. index + coverage + underlying index, blended
   after palette lookup) versus crisp native strokes; decide with the first native effects.
