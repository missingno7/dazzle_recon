"""Apply an identifier rename map to the reconstructed sources (names only; no code changes).

MAP.json: {"old_name": "new_name", ...} or {"old_name": {"name": "new_name", ...}, ...}.
Rewrites whole identifiers in src/*.c, include/*.h, asm/*.asm and the verifier metadata in
evidence/bindings/*.json (which spells C publics with a leading underscore). Refuses keywords,
duplicate targets and names that already exist. Run tools/check.py afterwards: renaming must not change a single byte.
"""
import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = ['src/*.c', 'include/*.h', 'asm/*.asm', 'evidence/bindings/*.json']
KEYWORDS = set('''asm auto break case cdecl char const continue default do double else enum extern far
float for goto huge if int interrupt long near pascal register return short signed sizeof static
struct switch typedef union unsigned void volatile while _cs _ds _es _ss'''.split())
IDENT = re.compile(r'^[A-Za-z][A-Za-z0-9_]*$')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('maps', nargs='+', type=Path)
    ap.add_argument('--dry-run', action='store_true')
    args = ap.parse_args()

    mapping = {}
    for path in args.maps:
        for old, new in json.loads(path.read_text(encoding='utf-8')).items():
            new = new['name'] if isinstance(new, dict) else new
            if old in mapping and mapping[old] != new:
                sys.exit(f'{old}: conflicting targets {mapping[old]} / {new}')
            mapping[old] = new
    files = sorted({p for pattern in FILES for p in ROOT.glob(pattern)})
    texts = {p: p.read_text(encoding='utf-8') for p in files}
    existing = set()
    for text in texts.values():
        existing.update(re.findall(r'[A-Za-z_][A-Za-z0-9_]*', text))

    errors = []
    targets = {}
    for old, new in mapping.items():
        if old not in existing:
            errors.append(f'{old}: not found')
        if not IDENT.match(new) or new in KEYWORDS or len(new) > 31:
            errors.append(f'{old} -> {new}: not a valid, unreserved identifier (<= 31 chars)')
        if new in existing and new not in mapping:
            errors.append(f'{old} -> {new}: name already in use')
        if new in targets:
            errors.append(f'{old} -> {new}: also the target of {targets[new]}')
        targets[new] = old
    if errors:
        sys.exit('\n'.join(errors))

    pattern = re.compile(r'(?<![A-Za-z0-9_])(_?)(' + '|'.join(map(re.escape, sorted(mapping, key=len, reverse=True)))
                         + r')(?![A-Za-z0-9_])')
    total = 0
    for path, text in texts.items():
        new_text, n = pattern.subn(lambda m: m.group(1) + mapping[m.group(2)], text)
        if n:
            total += n
            print(f'{path.relative_to(ROOT)}: {n}')
            if not args.dry_run:
                path.write_bytes(new_text.encode('utf-8'))
    print(f'{len(mapping)} names, {total} replacements' + (' (dry run)' if args.dry_run else ''))


if __name__ == '__main__':
    main()
