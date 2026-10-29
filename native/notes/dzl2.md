# DZL2 native port notes

- `diamonds` and `wedges`: rebuild explicitly writes a zero stack gate. Original-executable measurements below establish positive continuation gates for Auto and ordinary Semi-Auto, with a quiet continuous Semi-Auto exception for `wedges`. Native uses defined caller state instead of an uninitialized C local.
- `butterflies`: the wing product wraps to a signed 16-bit word before division, as on Borland; `multiply_word` makes that operation explicit.
- `yarnball`: aspect and radius calculations use native `float` storage like the source, but an 8087's extended intermediates could change a truncated 16-bit center/radius near an integer boundary; no DOS differential is available yet.
- `dzl2_seg`: native code has no DOS segment number, so the diagnostic getter returns zero.
- `tiles`: EGA scaling tests `video_backend == BACKEND_EGA_640X700`, independent of the plot callback address, as required by the native interface.

## Original-program stack measurements (n-gate, 2026-10-29)

The rule depends on the caller and `/Q`, rather than on VGA versus EGA. Both
images read **zero on every rebuild**. All measured Auto and unmuted Semi-Auto
continuations read **positive** words and retain their steps. Quiet Semi-Auto
`wedges` can read **zero repeatedly after a rebuild while F8 continuous mode is
already active**. Starting F8/F10 after idle instead encounters positive stack
remnants. Thus a universal positive policy misses a reachable original path.

The immutable input was `D:/Prog/dazzle_recon/reference/DAZZLE_U.EXE`, SHA-256
`2d2ce68047790c9e4605d55b1a0614701998abb25128177b6aaa51dadca92ff7`.
Each run copied it unchanged into `build/workers/n-gate/<run>/DAZZLE.EXE` and
used `/I /N`, `/V` or `/E`; early probes also used `/Q /W`. No code, data, RNG,
stack, or executable bytes were patched. Configs disable every image except
diamonds (`^b`) and/or wedges (`^e`). The default-fade controls retained the
original fade settings; other runs disabled fades and set the Auto pause to
zero to obtain repeated image entries. The wall-clock-seeded RNG was left alone.

DOSBox-X acceptance v3 SHA-256:
`e5b9a81acb208bab48cb01a7dac9691cd06674fac877abce54e4693ece6dbff9`;
normal core, 386, fixed 200000 cycles, headless SDL dummy video/audio. The
runtime load paragraph was `0823`, DS=`31E2`, SS=`396F`. Load-image addresses
below exclude that paragraph. `run.py` registers `on_exec` stack dumps **at the
comparison instruction**, before it executes. JSON registers locate BP, and the
binary dump supplies the gate, argument `[BP+6]`, and actual far return address.
`summarize.py` writes one `calls.csv` row per captured execution. Memory watches
and `writers.py` independently identify the preceding write in `writers.csv`.

| Image | Read instruction | Rebuild assignment |
|---|---|---|
| diamonds | `0B84:03D8 cmp word ptr [BP-2],0` | `0B84:02FF`, zero |
| wedges | `0B84:0EB0 cmp word ptr [BP-12h],0` | `0B84:0E2B`, zero |

The following are counts of rebuild/continuation executions, with observed
continuation values in hexadecimal. Auto rows use the four image-only runs;
Semi-Auto rows use `semi-normal-{vga,ega}` and exclude startup's Auto entries.
Every rebuild in these rows read zero.

| Caller | Adapter | diamonds rebuild/continue; continuation gate | wedges rebuild/continue; continuation gate |
|---|---|---|---|
| Auto | VGA | 55/59; `00D6..19E4` | 60/92; `2FB6` |
| Auto | EGA | 79/124; `0143..19E4` | 79/103; `2FB6` |
| Semi-Auto, sound enabled, F8 continuous | VGA | 2/3041; `00A3..4200` | 2/1493; `29F2` |
| Semi-Auto, sound enabled, F8 continuous | EGA | 2/1689; `0140..4200` | 2/478; `29F2` |

`semi-both-{vga,ega}` additionally exercised F10 from idle: diamonds 20/35
continuations, wedges 26/4, all positive. `auto-default-{vga,ega}` ran without
`/Q /W` and with the default fades; their 10/15 continuations were also positive.
The Auto call sites return to `11C1:3C5B` / `3C9A`; Semi-Auto selection rebuilds
return to `11C1:378A`, and continuations to `385B`. Argument counts change the
absolute slots: the `/Q /W` Auto diamonds/wedges slots were `396F:2FAC/2F9C`,
and Semi-Auto's were four bytes lower. The dumps, not fixed host addresses,
determine the slot for each captured call.

The last-writer traces explain the values:

- **Auto diamonds:** the first continuation reads the return CS `19E4h` pushed
  by `11C1:3C75`'s `fprintf(stdout,"\r")` call. Later continuations can read the
  preceding plot callback's X-plus local: `1899:04C3` on VGA, `0165/02A1` on
  EGA, storing `screen_half_width + draw_x` (or swapped `draw_y`). These values
  were positive throughout the captures. Semi-Auto diamonds uses the same
  plotter slot; the first F10/F8 call instead sees the key-dispatch local
  `4400h/4200h`, written at `11C1:1AE4`, or beep's saved BP (`11C1:0E07`).
- **Auto wedges:** `fprintf`'s prologue at RTL `0000:45EA` pushes its caller BP
  into the deeper gate slot (`2FB6h` in the `/Q /W` probes). Unlike diamonds,
  the outer plotter does not reach this slot. Repeated continuation calls can
  retain this word. The absolute BP changes with command-line stack layout.
- **Unmuted Semi-Auto wedges:** the beep after rebuild (`11C1:37AF`) calls
  RTL delay (`0000:29A6`). Its near call at `0000:29EF` leaves return IP `29F2h`
  in the slot. All 1,971 measured sound-enabled continuous calls kept this word.
- **Quiet Semi-Auto wedges:** beep skips sound/delay/nosound, so a rebuild's
  zero survives an immediate continuous draw. `quiet-final-vga` captured 1,466
  zero and 408 positive continuations across four selections; EGA captured 598
  zero and 218 positive. The positive group follows F10/F8 from idle; in the
  watched portion the writer is the timer ISR's `1899:14CA push es`, leaving
  `396Fh` (VGA) or `3C94h` (EGA). The zero groups' last writer remains the
  rebuild assignment at `0B84:0E2B`, across hundreds of calls on each adapter.

Native `wedges` now keeps the **sign** of this caller remnant: Auto or unmuted
Semi-Auto selects positive; quiet Semi-Auto rebuild selects zero; explicit F10
or F8 resume selects positive. An automatic F8 continuation preserves the sign
unless a virtual timer tick occurred between image calls, which selects positive.
`previous_graphics_key == F8` distinguishes a physical/scripted F8 command from
the loop's synthesized F8 after rebuild. `diamonds` retains the positive policy.
This reproduces the measured caller-write paths without depending on host stack
layout or consuming any additional random numbers. The tick counter is sampled
after the image finishes, so interrupts during its deeper polygon drawing do
not spuriously overwrite the modeled slot. Existing differential image
fixtures deliberately exercise positive-slot isolated calls, so their expected
data is unchanged. New `test_dzl2` checks the mode/mute/rebuild/resume paths:
EGA continuation consumes two shading RNG calls, or nine with the zero wedge
gate's seven step draws. Tests also distinguish a tick during polygon drawing
from one between image calls.

**Timer takeover and timing limit:** the quiet word is not guaranteed to remain
zero indefinitely. `quiet-normal-vga/calls.csv` changes from zero to `050Ah` at
16492.177335 ms without another rebuild. That run did not watch writes, so its
particular writer is unproven. The stack layout independently explains an IRQ
takeover: for caller SP=S, wedges reads S-26; the outer plotter saves BP, reserves
eight bytes and pushes SI/DI, leaving SP=S-20. The interrupt's saved return IP is
then at S-26, exactly the gate. `050Ah` is an instruction boundary inside VGA's
plotter; both VGA/EGA plotters use this frame and positive code offsets. This
identifies a concrete possible writer, rather than assuming zero is persistent.

The native model uses its existing cooperative virtual clock to reproduce that
positive takeover when a tick lands between draws. It does not reproduce a
particular emulated CPU instruction phase, numeric saved IP, or guarantee the
same takeover call number as DOSBox-X: that would require instruction-level CPU
timing beyond the native platform's execution model. This timing difference is
intentional; the caller writes and the sign change at a clock boundary are
defined and deterministic. No claim of universal historical stack contents or
exact DOS timing is made. Adapter selection alone does not determine the gate.

Reproduction, from this worktree (all evidence is under `build/workers/n-gate/`):

```powershell
python -B -I build/workers/n-gate/run.py semi-normal-vga both vga semi --normal --continuous --watch 3c6a6 --watch 3c696 --ms 22000
python -B -I build/workers/n-gate/run.py quiet-final-vga wedges vga semi --normal --quiet --continuous --enable-at 1 --watch 3c68e --watch-start 10500 --ms 22000
python -B -I build/workers/n-gate/summarize.py semi-normal-vga quiet-final-vga
python -B -I build/workers/n-gate/writers.py semi-normal-vga quiet-final-vga
```

Replace `vga` with `ega` for paired runs. Each directory includes the exact EXE
hash, launch arguments, config, keyboard events, register/stack dumps, logs,
PID and stopped receipt. Sampling values/counts can vary because the original
time seed and IRQ phase are uncontrolled. `summary.json` inventories the final
evidence and verifies the immutable input and stopped processes.

Negative/limited probes: Python's default environment lacked Capstone; the
isolated disassembler used `C:/tools/capstone-5.0.3` explicitly. A startup-wide
VGA memory watch hit its 999,999-row cap before the desired calls
(`quiet-trace-vga`); final watches start later. Early Semi-Auto selections were
sometimes drained while entering the mode, so the final quiet scripts enable
F8 after the second selection. `semi-cont-vga`, `semi-both-ega`, and
`semi-trace-vga` exceeded the 55-second host bound and were killed/reaped; their
captured prefixes remain valid observations, and no absence-of-calls conclusion
uses them. All other DOSBox-X runs exited at their bounded script end. Every
started process stopped. Two further late-watch probes (`quiet-irq-vga1/2`,
fixed cycles 199991/200003) saw only persistent zero after continuous rebuild;
they did not reproduce the rare `050Ah` takeover.

Validation: `python -B -I tools/native.py test` passed **31/31**, repeated with
`DAZZLE_CONVERSION_WARNINGS=ON` and `DAZZLE_ORIGINAL_EXE` set to the read-only
original resource EXE: **31/31**. No new DZL2 conversion warnings; the existing
DZLE fallthrough and differential-harness conversion warnings remain. Logs:
`native-test.log`, `native-test-conversion.log`, `native-test-final.log`.
No shared interface changes,
DOS source changes, expectation rebaselines, or commits.
