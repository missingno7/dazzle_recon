# Translation-unit segment verifier

`tools/segcmp.py` accepts an OMF CODE segment name or its target paragraph and
one or more C files. It concatenates the files in the given order, compiles the
combined source once with the pinned `dazzle` profile through `tools/bcc.py`,
then compares the entire emitted code segment against the immutable load image.
It imports `Object`, `compare`, `check_known_bindings`, and `load_oracle` from
`tools/objcmp.py`; it does not mask bytes or provide a separate relocation
implementation. Bindings are merged from every input file's `objcmp` metadata.

```text
python -I tools/segcmp.py 0B84 src/dzl2.c --out evidence/match/seg-0b84.json
python -I tools/segcmp.py 066A src/dzle.c --out evidence/match/seg-066a.json
python -I tools/segcmp.py _TEXT src/unit.c --start 066A:000B
```

The default start for 066A comes from
`evidence/seg0-residue.json`: its identified RTL interval ends at linear
`0x66AB`, immediately after byte `0000:66AA`. This is `066A:000B`, also recorded
in `docs/runtime-drift.md`. For other paragraphs, the tool takes the first
inventory address and checks it against the preceding segment's last classified
extent where available. Use `--start SSSS:OOOO` when the module start needs an
explicit binding. The expected end is the next known module start, including
bytes not assigned to a public function; the final module uses the function
inventory's code-area end as an upper bound. A different object length is a
whole-segment mismatch, even if its common prefix agrees.

The whole-segment pass resolves each public-bounded range using `objcmp.compare`
at its contiguous module placement. It includes the leading static/unnamed range
when the first public starts after byte zero. The JSON also records each public's
independent function-extent result at its metadata address, the first byte issue
and nearest public, and every non-CODE contribution. `_DATA`/`_BSS` entries give
segment size and emitted versus uninitialized bytes. Separate literal/constant
segments are counted by OMF name/class; printable NUL-terminated runs in data
are listed as candidates, not asserted to be a complete literal-pool map.

## Probe results

The C form

```c
static unsigned dzle_seg(void) { return _CS; }
```

was compiled fresh with BC++ 3.0, `-c -IINCLUDE -ml -O1` (the `dazzle`
profile). Its sole CODE contribution is three bytes, `8C C8 CB` (`MOV AX,CS;
RETF`), and the object has no PUBDEF for the static function. The same return
expression in `src/dzl2.c`'s public `dzl2_seg` also gives the exact
three-byte function at `0B84:0000`.

The `0B84` translation unit has a `0x149A`-byte CODE segment, equal in length to
the original extent `0B84:0000..0CCD:000A`, and the whole segment is
`RELOCATED`. Public results are one EXACT and six RELOCATED. Its `_DATA` and
`_BSS` contributions are both zero; no separate literal segment or string
candidate was found. The start is `0B84:0000`, immediately after the previous
classified extent in 066A.

The requested 066A input currently stops at compilation. BC++ 3.0 reports
incompatible redeclarations of `g_5d80`, `g_5c3f`, and `g_5c41`: `part1.c`
declares them `unsigned int`, while `part2.c` redeclares them `int`. `segcmp`
writes a `COMPILE_ERROR` segment result and does not fabricate per-public or
byte comparison results. The merged metadata also flags `_sin` and `_cos` as
having addresses in both segment 0000 and 066A, so those names are quarantined
rather than resolved by input order. The shared C sources were left untouched.

`tools/status.py` reads `evidence/match/seg-*.json` separately from function
match files. The code-segment table has a **TU-level proven** column; a strict
whole-segment result takes precedence for functions within its recorded module
range, while a failed TU attempt leaves independent strict function results
intact. Per-public results from a segment file also participate in latest-result
deduplication by address.
