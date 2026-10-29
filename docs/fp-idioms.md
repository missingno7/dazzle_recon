# BC++ 3.0 floating-point matching reference

Measured with the pinned **BC++ 3.00, `-c -ml -O1`**, profile `dazzle` in
[compiler.md](compiler.md). All examples below are observations of this profile,
not portable C rules. Probe source: [idioms.c](../build/workers/s-fp/idioms.c)
(86 functions); annotated object disassembly: `build/workers/s-fp/idioms.txt`;
complete `objcmp --dump`: `build/workers/s-fp/idioms-dump.json`.
The additional 12-function `controls.c` has one-axis reports and receipts nearby.

## Reading the bytes

`W` below means the object's `9B` immediately before an x87 opcode, with an
OFFSET16 fixup to **`FIDRQQ=0x5C32`** covering both bytes. Link-time addition
converts `9B D8` through `9B DF` into `CD 34` through `CD 3B`, preserving length.
`Q` means the separate **`90 9B`**, with **`FIWRQQ=0xA23D`**, which becomes
**`CD 3D`**. It is not an ordinary unbound NOP. Values were read from the
absolute PUBDEFs in the original `EMU.LIB/FPINIT`, not inferred from candidates.
`library.txt` records that inspection; `verification.json` records its hash.

For example, `W D9 46 FC` is object `9B D9 46 FC`, linked `CD 35 46 FC`,
and means `FLD dword [BP-4]`. Ordinary Capstone linear disassembly of the linked
bytes loses instruction boundaries. `build/workers/s-fp/inspect_fp.py` reverses
only these escapes for **display**; it never supplies bytes to the verifier.
`mm mm` below is a little-endian DS offset with its own OMF fixup. Frame offsets
are examples; local declarations and compiler temporaries change them.

## Constructs and characteristic bytes

Here `f`/`d`/`ld` are float/double/long double, and `i` is a 16-bit int.
Prologues/epilogues are omitted except where their size matters.

| C construct / probe | Object bytes / operations | Matching implication |
|---|---|---|
| Return global `f`, `d`, `ld` (`load_*`) | `W D9 06 mm mm`, `W DD 06 mm mm`, `W DB 2E mm mm`; `CB` | Storage is 4 / 8 / 10 bytes; result is in ST(0), with no return-value store. |
| Store ST(0) into global `f`, `d`, `ld` (`store_*`) | `W D9 1E mm mm`, `W DD 1E mm mm`, `W DB 3E mm mm`; then `Q` in these probes | `FSTP` width identifies destination type. |
| Local `float a`, `double a`, `long double a` (`local_*`, address passed) | `SUB SP,4/8/10`; stores `W D9 5E FC` / `W DD 5E F8` / `W DB 7E F6`; reloads `W D9 46 FC` / `W DD 46 F8` / `W DB 6E F6` | Memory width is stronger type evidence than expression arithmetic. |
| `return i` as double (`i_to_d`) | `MOV AX,[BP+6]; MOV [BP-2],AX; W DF 46 FE` | Compiler copies even an existing stack parameter into a temporary, then **FILD word**. |
| Return global `int` as double (`global_i_to_d`) | `A1 mm mm; 89 46 FE; W DF 46 FE` | Do not replace this with direct `FILD [global]`. |
| `unsigned` to double (`u_to_d`) | Copy low word, zero upper word; `W DB 46 FC` | **FILD signed dword** of a zero-extended 16-bit value. |
| `long` to double (`l_to_d`) | Copy both words; `W DB 46 FC` | **FILD dword**, 4-byte temporary. |
| `unsigned long` to double (`ul_to_d`) | Copy both words, zero two upper words; `W DF 6E F8` | **FILD qword**, 8-byte temporary. |
| `(int)d`, `(long)d`, `(unsigned)d`, `(unsigned long)d` (`d_to_*`) | `W DD 46 06; 9A <F_FTOL@>` | All four isolated returns are 14 bytes, including frame/RETF. AX is used for 16-bit results; DX:AX for 32-bit results. |
| `(int)f`, `(int)ld` (`f_to_i`, `ld_to_i`) | `W D9 46 06` / `W DB 6E 06`, then the same `F_FTOL@` call | Cast width does not select a different helper. |
| `double` -> global int/unsigned vs long/unsigned long | Helper call, then `A3 mm mm`; 32-bit stores additionally `89 16 <symbol+2>` | Signedness cannot be recovered from this cast alone. Do not assume defined behavior for out-of-range C conversions. |
| `(int)d` converted back to double (`cast_round_trip`) | `F_FTOL@; MOV [BP-2],AX; W DF 46 FE` | A real integer conversion is not optimized away. |
| `(float)d` returned as double / assigned to double (`explicit_float`, `float_return_round`) | Just `FLD qword` / `FLD qword; FSTP qword` | This compiler does **not** force float rounding for these casts; a float memory assignment does. |
| `d + 0.5` (`add_half`) | `W DD 46 06; W D8 06 <pool>`; pool `00 00 00 3F` | Exactly representable double literals can use an **m32** pool operand. `D8` does not prove source `float`. |
| `d + 0.01` / `f + 0.01f` (`add_cent*`) | `W DC 06 <pool>` / `W D8 06 <pool>` | Pools: double `7B 14 AE 47 E1 7A 84 3F`; float `0A D7 23 3C`. Literal suffix matters. |
| `ld + 0.01L` (`add_cent_l`) | `FLD tbyte arg; W DB 2E <pool>; W DE C1` | 10-byte pool `0A D7 A3 70 3D 0A D7 A3 F8 3F`; no x87 m80 arithmetic operand. |
| `0.0`, `1.0` returns (`zero`, `one`) | `W D9 EE`, `W D9 E8` | FLDZ / FLD1; no literal-pool cell. |
| `d + sd` / `d + scd`, static double / static const double | `W DC 06 <static>` in both | Each named static has its own 8-byte initialized storage; `const` is not substituted with a literal here. |
| Repeated `0.01` literals (`same_literals`) | Both double assignments load the **same** `_DATA+14h` cell in this TU | Initialized static definitions precede the pool; identical numeric pool literals are shared in this probe. Float constant assignment uses two immediate MOVs instead. |
| `a-b-c`, `a*b+c` (`subtract`, `multiply_add`) | `FLD a; FSUB b; FSUB c` / `FLD a; FMUL b; FADD c` | These expressions stay left-associated; no fused operation. |
| `f * sin(f)` / `sin(f) * f` (`float_*_call`) | First form saves lhs with `W DB 7E F6`, reloads `W DB 6E F6`, then `W DE C9`; second uses `W D8 0E <f>` after call | 48 vs 32 bytes. The 10-byte spill is compiler-generated even when **no C long double exists**. |
| `d * sin(f)` / `sin(f) * d` (`lhs_call`, `rhs_call`) | Both call first, then `W DC 0E <d>` | Both 32 bytes here. Operand-order effects depend on types; do not apply the float rule to doubles. |
| `sin(a)+cos(a)` (`two_calls`) | Call sin; save result in 10-byte temp; call cos; reload; `W DE C1` | x87 stack values are spilled across calls. |
| `f += d` / `f=f+d` (`compound_f`, `expanded_f`) | `FLD f; FADD m64 d; FSTP m32 f; Q` | Both 18 bytes here. |
| `d *= f` / `d=d*f` (`compound_d`, `expanded_d`) | Both `FLD f; FMUL m64 d; FSTP m64 d`; **only expanded form appends `Q`** | 16 vs 18 bytes, also observed in `controls.c`. Preserve assignment syntax when a wait differs. |
| `f += sin(f)` / `f=f+sin(f)` (`f_*_call`) | 10-byte lhs spill, call, reload, FADDP, FSTP, `Q` | Both 55 bytes here. Compound syntax is not a universal wait suppression rule. |
| `register float/double t` vs plain (`register_*`, `plain_*`) | Identical 32-byte pairs; t is stored/reloaded at its declared width | `register` does not promise a permanent x87 stack register. |
| `1.0/(1+f)` or `1.0f/(1.0f+f)` (`reciprocal_3/5`) | `W D9 E8; W D8 06 <f>; W D9 E8; W DE F1` | 15-byte leaf. Original sine uses this denominator order/type. |
| `1.0/(f+1)`, `1.0/(f+1.0)`, **also** `1.0/(1.0+f)` | `FLD f; FLD1; FADDP; FLD1; FDIVRP` | 18-byte leaf. Swapping double literal `1.0` is insufficient; integer `1` or float `1.0f` changes frontend ordering. |

`F_FTOL@` is at **`0000:0FBE`**. Its original 44-byte RTL body saves the
control word, sets rounding-control bits with `OR byte [BP-1],0Ch`, executes
**`FISTP qword [BP-0Ah]`** (`CD 3B 7E F6`), restores the control word and
returns DX:AX. Thus ordinary casts use truncation via a call, rather than direct
FIST in the application. A separate BC30 **`-ms -O1`** control produces near
**`N_FTOL@`**, `E8 rel16`, BP+4 parameters and RET; this is not the DAZZLE ABI.
Adding `-ff` after `-ml -O1` did not change any of the 12 controls' code or
generate inline FIST.

## Comparisons

For double parameters `a,b`, all six relation probes use:

```text
W DD 46 06       FLD qword [BP+6]
W DC 5E 0E       FCOMP qword [BP+0Eh]
W DD 7E FE       FSTSW word [BP-2]  (Capstone prints WAIT; FNSTSW)
Q                wait for status store
8B 46 FE 9E      MOV AX,[BP-2]; SAHF
```

| Source predicate | Jump to false arm |
|---|---|
| `a < b`, `a <= b` | `73` JAE, `77` JA |
| `a == b`, `a != b` | `75` JNE, `74` JE |
| `a > b`, `a >= b` | `76` JBE, `72` JB |

Returning the boolean adds `MOV AX,1; JMP; XOR AX,AX`; each complete relation
probe is 37 bytes. A branch assigning `gi=1` is 36 bytes. Comparing against
zero/one can instead load the constant and use `W DE D9` FCOMPP, so the relation
is reversed: `if (f>0)` uses JAE to skip the body after comparing **0 against f**.
The float-global, boolean, negated, chained and all-six-relation probes yielded
**SAHF, not a TEST of AH**. No compiler TEST-status idiom was established for this
profile; a TEST mask in other code needs separate evidence. These are byte
observations, not a claim of modern NaN-comparison semantics.

## Math calls and far arguments

Vendor `math.h` plus `-O1` emits far calls to **`_sin`, `_cos`, `_sqrt`, `_atan`,
`_pow`, `_floor`, `_fabs`**. Each one-argument return wrapper is 26 bytes.
MATHL members are SIN, COS, SQRT, ATAN, POW, FLOOR (POW's `_pow` PUBDEF is at
member offset **6Dh**, after `__EXPL`). Existing `evidence/seg0-residue.json`
identifies `_sqrt=0000:0E33`, **`_sin=0000:0E75`**, **`_cos=0000:0F6E`**.
No DAZZLE address is asserted here for unobserved atan/pow/floor/fabs entries.
Do not bind sin/cos to `066A:72DC` / `066A:78E4`: those apparent calls arise from
disassembling emulator escape payloads as ordinary instructions.

```text
W DD 46 06       FLD double parameter
83 EC 08         SUB SP,8
W DD 5E F8       FSTP qword [BP-8]   ; outgoing by-value double
Q
9A oo oo ss ss   far call; OMF POINTER32 fixup / MZ segment relocation
83 C4 08         ADD SP,8
```

The callee returns ST(0); the caller removes arguments. `pow(a,b)` loads/stores
**b first, then a**, and cleans 16 bytes. `target(float_arg,int_arg)` also handles
the second argument first: int -> temporary -> FILD -> outgoing double; then
float -> FLD m32 -> outgoing double. `target(other(),sin(a))` calls sin before
other. These are observed evaluation orders, not C language guarantees.

`floor(a)` is a call; `(int)floor(a)` adds `F_FTOL@` (31 bytes total).
`abs(a)` with vendor `stdlib.h` is integer `CWD; XOR AX,DX; SUB AX,DX`.
If a is double, it first calls FTOL, then performs integer abs (19 bytes).
`fabs(a)` calls `_fabs`; **`__fabs__(a)`** is the 12-byte intrinsic wrapper,
`FLD a; W D9 E1` FABS. `-O1` alone did not turn the ordinary fabs call intrinsic.

`Q` can occur after intervening integer instructions, immediately before a call
or before consuming a stored result. Consecutive x87 operations already have
their own FIDRQQ escapes. Do not add inline waits or assembly to compensate for
source/type differences.

## Worked application examples

Fresh function-extent proofs, **857 application bytes RELOCATED**:

| Candidate | Oracle extent | Result |
|---|---|---|
| [sine.c](../build/workers/s-fp/sine.c) | `066A:3D52..3F4B`, 505 bytes | RELOCATED; all fixups and relocation sites bound |
| [curves.c](../build/workers/s-fp/curves.c) | `066A:1B62..1CC2`, 352 bytes | RELOCATED; all fixups and relocation sites bound |

Sine keeps **seven global floats**, not local double state: amplitude_x/y at
`DS:5BB3/5BB7`, phase at `5BBB`, modulation phases at `5BBF/5BC3`, steps at
`5BC7/5BCB`. Rebuild zeros the three phases, advances the color and chooses each
amplitude as `limit/8 + random(limit/8)`. Steps are `1.0/(1+amplitude)`.
The left float operands in `amplitude*sin(phase)` and `amplitude+=sin(modulation)`
explain the compiler's 10-byte spills; the frame is 12 bytes (2-byte integer
conversion temp plus shared 10-byte spill). The phase step **0.01 is double**,
loaded from `DS:06F3`, even though phase is float. The pointer comparison against
`1899:012D` (`ega_plot_symmetric_640x700`) compares segment before offset and
clears the low bits of x/y for that renderer. It does not call the callback.

Curves has **two float locals**, a/b, and 16-bit global angles, increments,
radius and counter (`DS:5968..5972`). Rebuild sets both angles to 1, radius to
`(int)(limit/1.5)`, counter to 255, and random increments to 2..9 / 4..9.
After increment, **`if ((counter = counter & 15) == 0)`** produces the original
MOV/AND/MOV/OR sequence. Every sixteenth call stores `angle*0.001` into each
float local, then computes `x=(int)(cos(a)*radius)+4`,
`y=(int)(sin(b)*radius)+4`. Moving `+4` inside the cast changes the conversion.
Angles reset above 32000; color advances when `y&2`. Other calls increment x.

The isolated curves candidate declares `extern const float curves_divisor` for
the **existing 1.5f pool cell at `DS:06BF`**. Its only newly emitted pool
cell is double 0.001 at `DS:06E7`. This avoids inventing intervening data/padding
to force two noncontiguous constants into one isolated `_DATA` contribution.
When integrating the original TU, restore literal `1.5` and its pool
ownership, then verify the complete TU again. This external declaration is a
function-proof binding, not recovered original symbol spelling or a natural-link
claim. `verify.py` separately checks both 8-byte emitted pools against the oracle,
the separated 1.5f bytes, and both EMU absolute PUBDEF values.

## Reproduction and negative results

The unmodified tools normally write into another worker's scratch directory.
To honor this task's write restriction, `run.py` imports `bcc`, changes only its
in-memory `WORK` to `build/workers/s-fp/compiler`, then executes the requested
unmodified CLI. No compiler/verifier source was edited.

```text
python -I build/workers/s-fp/run.py bcc build/workers/s-fp/idioms.c --profile dazzle --fresh
python -I build/workers/s-fp/run.py objcmp build/workers/s-fp/idioms.c --dump
python -I build/workers/s-fp/run.py objcmp build/workers/s-fp/sine.c --all --fresh
python -I build/workers/s-fp/run.py objcmp build/workers/s-fp/curves.c --all --fresh
python -I build/workers/s-fp/verify.py
```

The middle two verification commands are the same tool paths as
`python -I tools/objcmp.py FILE --all --fresh`, with scratch redirected only.
Results: `sine-result.json`, `curves-result.json`, and detailed fresh
`verification.json`. Compiler receipts retain source, compiler hash, ordered
flags, headers and logs. No matches were added to the accepted ledger.

| Hypothesis / exact BC30 flags | Observation / rejected shortcut |
|---|---|
| Sine, `-ml -O1`: initial `random(limit/8)+limit/8`, double denominator constants | 511 bytes. Sum used ADD DX,AX instead of ADD AX,DX. Reversing sum operands fixed integer registers, but not length. |
| Sine, `-ml -O1`: `1.0/(1.0+amplitude)` or `1.0/(amplitude+1)` | Still 511 bytes. Correct **`1.0/(1+amplitude)`** saves 3 bytes twice, yielding 505. |
| Curves, `-ml -O1`: separate `counter &=15; if(counter==0)` | 351 bytes; memory AND and separate CMP. |
| Curves, `-ml -O1`: `if((counter &=15)==0)` | 346 bytes; memory AND and direct Jcc. Explicit binary assignment in the condition yields 352. |
| `controls.c`, `-ml -O1 -ff` | All 12 control bodies unchanged; no inline FIST or TEST-status pattern. |
| `controls.c`, `-ml -O1 -r-` | Integer-register/plain probes remain 38 bytes but frames grow 2 -> 4 bytes and instruction/register choices change. Other 10 bodies unchanged. |
| `controls.c`, `-ml -Od` | Cast 14 -> 16 bytes; compare 37 -> 39; compound/expanded stores grow 4 bytes. Register/plain integer pair remains equal. See `controls-Od.txt` and `axes.json`. |
| `controls.c`, `-ms -O1` | Cast uses N_FTOL@, near call, BP+4 and RET (12 bytes). Useful ABI discriminator, rejected DAZZLE configuration. |

Synthetic probes establish code-generation examples only. The two worked examples
are strict **function** proofs, with independent target extents and explicit
bindings; they do not establish original TU data order, runtime-library coverage,
a linked executable, or behavioral validation of all FP algorithms.

## Yarnball: preserve a postincrement inside the shift expression

`src/dzl2.c` now matches yarnball's complete **859-byte** extent at
`0B84:113F`, including the 24-byte switch table. The final difference was integer
code inside the FP-heavy routine, rather than an FP opcode or emulator fixup.
With BC++ 3.00 **`-c -ml -O1`**, these source forms are not byte-equivalent:

| Source | Resolved instruction bytes at `0B84:1437` |
|---|---|
| `i = 2 << g_60e8; ++g_60e8;` | `B8 02 00` MOV AX,2; `8A 0E E8 60` MOV CL,[60E8] |
| **`i = 2 << g_60e8++;`** | **`8B 0E E8 60` MOV CX,[60E8]; `B8 02 00` MOV AX,2** |

Both continue with `D3 E0` SHL AX,CL; `89 46 FE` MOV [BP-2],AX;
`FF 06 E8 60` INC word [60E8]. Keeping the postincrement within the right
operand produces the original **word load into CX before the constant load**;
the separate statements produce a byte load into CL after the constant.
No volatile qualifier, artificial dependency, assembly or compiler-option change
is needed. This is an observed expression-tree effect, not a general rule that
postincrement always evaluates before the left operand.

The separate-statement baseline had the same 859-byte extent and differed only
within those first seven bytes. Previous signedness, volatile, declaration-order,
register-local and alternate-shift attempts did not match (the original worker's
`build/workers/r0b84/notes.txt` records them). Scratch copies and before/after
instruction evidence are in `build/workers/s-fp/yarnball-before.c`,
`yarnball-postincrement.c` and `yarnball-order.json`.

Fresh verification with the unmodified `tools/objcmp.py` CLI, through the
scratch-redirecting wrapper described above:

```text
python -I build/workers/s-fp/run.py objcmp src/dzl2.c --all --fresh --out evidence/match/r0b84.json
7/7 matched (5274 bytes): 3 EXACT, 5271 RELOCATED
```

All six previously matched functions retain their statuses and extents; yarnball
is now RELOCATED with every fixup bound. The lesson for a final register-order
mismatch is to check whether a neighboring increment originally belonged inside
the expression before changing variable types or register declarations.
