# DZL2 native port notes

- `diamonds` and `wedges`: the reconstructed C reads `step_refresh_gate` without initializing it on non-initialize calls. Native initializes it to zero so each call refreshes the movement steps; this makes the original indeterminate stack read defined. A DOS-reference run should confirm the usual binary stack value takes this path.
- `butterflies`: the wing product wraps to a signed 16-bit word before division, as on Borland; `multiply_word` makes that operation explicit.
- `yarnball`: aspect and radius calculations use native `float` storage like the source, but an 8087's extended intermediates could change a truncated 16-bit center/radius near an integer boundary; no DOS differential is available yet.
- `dzl2_seg`: native code has no DOS segment number, so the diagnostic getter returns zero.
- `tiles`: EGA scaling tests `video_backend == BACKEND_EGA_640X700`, independent of the plot callback address, as required by the native interface.

- diamonds/wedges step_refresh_gate (supervisor, from n-diff): the original reads an uninitialised stack word whose
  value depends on the caller frame. Native policy: positive gate - seed the movement steps on rebuild and keep them
  on continuation (matches the DOS functions run with a controlled positive slot). Not proven to be what Auto mode
  always left on the stack; see native/tests/ref/DIFFERENCES.md and native/tests/ref/data/gates.txt.
