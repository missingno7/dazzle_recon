# Differential results and proposed application patches

The native application files were not edited. The three unified diffs below
are against this worktree and are left for the supervisor to apply. No commits
were made. DOS/model conditions are described in [README.md](README.md).

## Current failures

`REF_NO_CAPTURE=1 python -B -I tools/native.py test`: **17/19 pass**.
Only `test_ref_images` and `test_ref_palettes` fail. Image failures are nine
scenarios belonging to balls, diamonds and wedges; palette failure is one case.
The four other new differential runners pass, as do every existing native test.
The original resource EXE is absent from this worktree, so the existing DLOGO
reference test was configured to use the read-only copy at
`D:/Prog/dazzle_recon/assets/DAZZLE.EXE`, after checking its SHA-256 against
`evidence/originals.json`. No assets were copied or modified.

### Palette precision: DOSI

[dosi-fpu.patch](../../../build/workers/n-diff/dosi-fpu.patch) promotes the
argument multiplication to double at four `cos` sites in `build_ega_palette`.
SSE otherwise rounds `(phase + sample) * float_frequency` to float before the
double `cos` call; the Borland emulator keeps the product extended. Its operands
fit exactly in double, so this promotion suffices without a global FPU change.

For `vga_flags_0b`, seed 708Bh, flags A=1/M=1/F=0/4=1, buffer byte **1627** is
38h in DOS and 37h in native SSE. RNG count and other bytes agree. CRCs are
BCC90F63 / 51AAF887. Results with GCC 12.2 at -O2:

| Build | Palette cases |
|---|---|
| Native SSE, unchanged DOSI | 42/43 equal |
| Native `-mfpmath=387`, unchanged DOSI | 43/43 equal |
| Native SSE, explicit double argument products | 43/43 equal |

Long-double/libm replacement is unnecessary for these cases. All 134 extended
image summaries, including sine/cosine/square-root users, agree under SSE and
387 once the two defined-state policies below are applied. No trig-image
precision patch is proposed.

### Balls: read before checking the bitmap extent

[dzle-balls-padding.patch](../../../build/workers/n-diff/dzle-balls-padding.patch)
uses a zero-valued fetch outside the bitmap's extent, then continues the
original sampling loop. Native currently returns at that point. DOS performs
the fetch first and keeps walking over zero padding until a nonzero wrapped
sample is found. The early native return changes sample progression, later
restarts and RNG consumption. This fails the two long VGA seeds and long EGA
seed, while the 12-step case agrees.

This patch implements the reference driver's explicit zero-padding policy.
Actual DOS heap bytes outside the original allocation are not uniquely defined;
the reference now allocates and zeros the entire 64 KiB offset domain. The patch
does not claim arbitrary historical heap contents were zero. The supervisor
should record this policy in the DZLE port notes when applying it.

### Diamonds and wedges: the automatic gate is caller dependent

[dzl2-step-policy.patch](../../../build/workers/n-diff/dzl2-step-policy.patch)
chooses the controlled positive-gate policy: seed movement steps during
initialization, retain them on continuation. Native currently assigns zero on
every call and therefore refreshes the steps every time. Both images fail at
all three seeds, accounting for the other six image failures.

`gate_driver.c` calls unmodified DOS functions after setting the exact stack
slot to 0, 1, -1, 32767 or -32768. Positive values retain steps; nonpositive
values add six/seven refresh RNG calls in diamonds/wedges. The ordinary
uncontrolled driver also changed its output when its caller's frame changed.
Thus **neither universal zero nor universal positive is a recovered original
invariant**. The patch is a defined native policy matching the controlled
reference; it is not evidence that Auto always leaves that stack slot positive.
The supervisor should explicitly document the chosen policy when integrating.

Original-executable follow-up (n-gate, 2026-10-29):
[DZL2 stack measurements](../../notes/dzl2.md#original-program-stack-measurements-n-gate-2026-10-29)
confirm positive continuation gates in measured Auto and unmuted Semi-Auto
calls on VGA/EGA. Quiet continuous Semi-Auto wedges can preserve rebuild's
zero instead; native now models that caller path and explicit F8/F10 resume.
The notes identify the remaining instruction-phase-dependent stack overwrite
limit. These isolated positive-slot differential fixtures remain unchanged.

## Passing diagnostics

* RNG: 8,018 captured rand/random values plus initial/seed-width/time probes.
* Primitives: 7,540 complete color/line/ellipse sequence rows, including negative,
  zero, large and wrapped inputs, on both adapters.
* PCX: all 44 exact boundary/sentinel streams and returned lengths agree. The
  last-byte overread affects run splitting. Native's explicit `after_row`
  reproduces it; no PCX patch is needed. Unknown DOS heap padding is still not
  established to be zero.
* Fades/swaps: all sixteen summaries agree, including three fade routines,
  both wipe modes, all four rectangle reflections, and enhanced EGA odd Y.

`build/workers/n-diff/patchcheck.py` compiles scratch copies containing only the
three proposed application changes. All six differential runners pass there:
134 image, 43 palette, 7,540 primitive, 44 PCX and sixteen fade/swap rows, plus RNG.
`patchcheck387.py` repeats image/palette validation with 387 math. Scratch logs,
full streams, compilation logs, and DOS toolchain receipts remain under build/.
The complete patched image stream matches all **6,653,997** DOS records,
including per-plot RNG counts; the fade/swap stream matches **2,012,440** records.
The default failing runner reports first divergence at global plot 588651,
diamonds case 37, plot 160: DOS (5,13,152,3,RNG=19), native (39,32,180,3,RNG=25).
These are behavioural comparisons; no EXACT/RELOCATED reconstruction claim is
made by these tests.

## Harness findings and negative experiments

* The old DOS framebuffer used far-pointer indexing and aliased every 64 KiB.
  Huge indexing corrected its framebuffer model; old data is retained separately.
* Grid's original literal assembly-offset test cannot identify a logging
  callback. Only that probe is adapted in the diagnostic source copy; native
  grid itself agrees and needs no patch.
* BC++ 3.0 -ml -O1 primitive links reported TLINK `Out of memory` at the longer
  `diff-primitives` cwd. -Od initially appeared to fix it, but identical object
  hashes later failed at the long path and passed under `nd-prim`. Final captures
  all retain -O1. There is no evidence of an optimizer defect here.
* A double cast only on the palette builder's unphased fallback expression did
  not fix the differing case. Promoting the three phased expressions as well
  fixes the complete matrix.
