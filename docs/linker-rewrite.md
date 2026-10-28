# TLINK same-segment far-call rewrite

## Verifier rule

The worker copy in `objcmp.py` accepts a linker rewrite only when a location-type-3
FIXUPP covers the four bytes immediately after an object `9A`, and the image has
`90 0E E8 rel16` at that address. The resolved target and frame must both be the
call site's code paragraph; the resolved far-pointer offset must equal
`(site_offset + 5 + signed_rel16) mod 65536`. No MZ relocation may occur anywhere
in the five-byte rewritten call. The fixup is excluded from expected MZ relocation
sites, and the resolved comparison substitutes the proved linked call bytes. The
raw object differs, so a successful rewrite is `RELOCATED`.

All other far-pointer fixups retain the ordinary relocation check. A wrong target,
wrong opcode, non-far-pointer fixup, relocation at the rewrite, or overlapping
fixup remains a mismatch. `verify_rewrite.py` exercises the success and failure
paths.

## C0L and application evidence

The pinned BC++ 3.0 C0L object at
`build/workers/toolchain/smoke/bc30/C0L.OBJ` contains two far-pointer fixups whose
linked bytes are rewrites in `reference/load_image.bin`:

| Image site | Object target binding | Linked bytes | Near target | MZ relocation |
|---|---|---|---|---|
| `0000:014F` | `_exit` = `0000:11A8` | `90 0E E8 54 10` | `0000:11A8` | none |
| `0000:025B` | `__exit` = `0000:11B7` | `90 0E E8 57 0F` | `0000:11B7` | none |

Both compare as `RELOCATED`. The neighboring C0L call at `0000:0149` remains a
normal far call: `_main` binds to `066A:4D3D`, its image bytes start `9A 3D 4D
6A 06`, and its segment word still has the expected MZ relocation at
`0000:014C`. Removing that relocation makes comparison fail. This checks that
the exception does not change other relocation expectations.

The application test source is under `build/workers/tooling/app_call/`. Separate
caller and target translation units were both compiled as `PROBE.C` with the
locked BC++ 3.0 compiler and the `dazzle` flags `-ml -O1`:

```text
BCC.EXE -c -IINCLUDE -ml -O1 PROBE.C
TLINK.EXE /n /m CALLER.OBJ+TARGET.OBJ,APPTEST.EXE,APPTEST.MAP,,
```

The caller object has `9A` plus a far-pointer FIXUPP to `_target`; the target is
at `0000:0006` in the shared `PROBE_TEXT` code paragraph. TLINK 5.0 emitted
`90 0E E8 01 00` at `0000:0000` with no MZ relocations. The worker verifier
returns `RELOCATED`; changing the target binding or adding a relocation at the
site fails. The compiler hash and object files are retained in `app_call/`.

Reproduce the focused checks with:

```text
python -B -I build/workers/tooling/verify_rewrite.py
```

## Limit on enumerating the stated 22 image sites

The two C0L sites above are source-proven. The final image contains 179 complete
`90 0E E8 rel16` instruction sequences in the code area; the linked bytes alone
do not distinguish a rewritten far call from an otherwise identical near-call
sequence. The available `assets/` contain the executable and documentation, but
not the application's original OMF objects or a TLINK map. This work therefore
does not substantiate or localize all 22 stated sites; the original application
OMF objects or linker map are needed to finish that list.
