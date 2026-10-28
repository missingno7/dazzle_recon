"""Read-only OMF/RTL drift audit; candidate identity evidence, never acceptance.

python -I tools/rtldiff.py --out build/workers/s-seg0/audit.json
Only library FIXUPP-associated far CALL/JMP opcodes may be relaxed for location.
Every such rewrite is then checked for its complete encoding and code target.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / 'tools'))
import libmatch as lm
from objcmp import Object


def digest(data):
    return hashlib.sha256(data).hexdigest()


def objects(path):
    data = path.read_bytes()
    lib = lm.library_members(path)
    for page, (name, records) in sorted(lib['modules'].items()):
        raw = data[records[0][2]:records[-1][3]]
        yield name, page, Object(raw), digest(raw)


def locate(path, image):
    rows = []
    for name, page, obj, sha in objects(path):
        for si, seg in enumerate(obj.segments[1:], 1):
            if seg['class'].upper() != 'CODE' or not seg['length']:
                continue
            raw, mask = bytes(seg['data']), bytearray(seg['present'])
            fixes = [f for f in obj.fixups if f.segment == si]
            for f in fixes:
                width = {0: 1, 1: 2, 2: 2, 3: 4, 4: 1}[f.location]
                mask[f.offset:f.offset + width] = bytes(width)
                if f.location == 3 and f.offset and raw[f.offset - 1] in (0x9a, 0xea):
                    mask[f.offset - 1] = 0
            # Full declared contribution, including all emitted padding.
            if sum(mask) < 8 and name not in ('REALCVT', 'SCANTOD'):
                continue
            hits = lm.find_masked(image, raw, mask)
            for base in hits:
                rows.append({'library': str(path), 'member': name, 'page': page,
                             'object_sha256': sha, 'segment_index': si,
                             'base': base, 'end': base + len(raw), 'size': len(raw),
                             'fixed_bytes': sum(mask), 'holes': seg['present'].count(0),
                             'location_count': len(hits), '_object': obj, '_mask': mask})
    return rows


def conversion_vectors(rows, image):
    """Bind tiny thunks using independently located math/vector contributions."""
    def contribution(member, library):
        return next((r for r in rows if r['member'] == member and
                     Path(r['library']).name == library and r['location_count'] == 1), None)
    real = contribution('REALCVT', 'MATHL.LIB')
    scan = contribution('SCANTOD', 'MATHL.LIB')
    fake = contribution('CVTFAK', 'CL.LIB')
    if any(r is None for r in (real, scan, fake)):
        return {}
    def vector(row, name):
        seg = next(s for s in row['_object'].segments[1:] if s['name'] == name)
        return b''.join((row['base'] + int.from_bytes(seg['data'][i:i + 2], 'little')).to_bytes(2, 'little')
                        for i in range(0, seg['length'], 2))
    signature = vector(real, '_CVTSEG') + vector(fake, '_CVTSEG') + vector(scan, '_SCNSEG')
    positions = []
    p = 0x29bf0
    while (p := image.find(signature, p)) >= 0:
        positions.append(p)
        p += 1
    if len(positions) != 1:
        return {}
    p = positions[0]
    return {'signature': signature.hex(' '), 'linear': p,
            'derivation': 'MATHL REALCVT _CVTSEG + CL CVTFAK _CVTSEG + MATHL SCANTOD _SCNSEG, code offsets relocated to located contributions',
            'symbols': {'__RealCvtVector': p, '__ScanTodVector': p + 4}}


def audit(rows, image, relocs, vectors):
    exports = defaultdict(set)
    absolutes = defaultdict(set)
    placements = defaultdict(set)
    for name, val in vectors.get('symbols', {}).items():
        exports[name].add(val)
    for row in rows:
        if row['location_count'] != 1:
            continue
        obj, si, base = row['_object'], row['segment_index'], row['base']
        placements[row['library'], row['page'], si].add(base)
        for name, (s, off, _) in obj.publics.items():
            if s == si:
                exports[name].add(base + off)
        for name, val in obj.absolute_publics.items():
            absolutes[name].add(val)
    for row in rows:
        obj, si, base = row['_object'], row['segment_index'], row['base']
        raw = bytes(obj.segments[si]['data'])
        rewrites, fixes, differences = [], [], []
        for i, (a, b) in enumerate(zip(raw, image[base:row['end']])):
            if a != b:
                differences.append({'offset': i, 'object': a, 'image': b})
        for f in obj.fixups:
            if f.segment != si:
                continue
            at, site = f.offset, base + f.offset
            width = {0: 1, 1: 2, 2: 2, 3: 4, 4: 1}[f.location]
            target_name, target_values = None, set()
            if f.target[0] == 2:
                target_name = obj.externals[f.target[1]]
                target_values = exports.get(target_name, set())
            elif f.target[0] == 0:
                target_name = obj.segments[f.target[1]]['name']
                target_values = placements.get((row['library'], row['page'], f.target[1]), set())
            else:
                target_name = str(f.target)
            addend = int.from_bytes(raw[at:at + min(width, 2)], 'little')
            expected = (next(iter(target_values)) + f.displacement + addend
                        if len(target_values) == 1 else None)
            actual, kind = None, 'offset_or_base_fixup'
            opcode = raw[at - 1] if at else None
            invalid_opcode = (f.location == 3 and at and opcode in (0x9a, 0xea) and
                              image[site - 1] not in (opcode, 0x90))
            rewritten = f.location == 3 and at and opcode in (0x9a, 0xea) and image[site - 1] == 0x90
            if rewritten:
                encoding = image[site - 1:site + 4]
                prefix = b'\x90\x0e\xe8' if opcode == 0x9a else b'\x90\x90\xe9'
                valid = encoding[:3] == prefix
                actual = (site + 4 + int.from_bytes(encoding[3:5], 'little', signed=True)) & 65535
                # All rewritten members in this audit reside in _TEXT at CS=0.
                kind = 'far_call_to_push_cs_near' if opcode == 0x9a else 'far_jump_to_near'
                rewrites.append({'offset': at - 1, 'linear': site - 1,
                                 'object_opcode': f'{opcode:02x}', 'image_bytes': encoding.hex(' '),
                                 'kind': kind, 'target_symbol': target_name,
                                 'expected_target': expected, 'actual_target': actual,
                                 'result': ('pass' if valid and expected == actual and site + 2 not in relocs else
                                            'unresolved' if valid and expected is None and site + 2 not in relocs else 'FAIL'),
                                 'mz_relocation_removed': site + 2 not in relocs})
            elif f.location == 3:
                actual = int.from_bytes(image[site:site + 2], 'little') + 16 * int.from_bytes(image[site + 2:site + 4], 'little')
                kind = 'far_pointer'
            elif f.location == 1 and f.relative:
                actual = (site + 2 + int.from_bytes(image[site:site + 2], 'little', signed=True)) & 65535
                kind = 'near_relative'
            elif f.location == 1 and expected is not None:
                # Code OFFSET16 references framed to _TEXT, CS=0.
                if ((f.frame[0] == 1 and obj.groups[f.frame[1]][0] == 'DGROUP') or
                        (f.frame[0] == 5 and target_name in vectors.get('symbols', {}))):
                    actual = 0x29bf0 + int.from_bytes(image[site:site + 2], 'little')
                    kind = 'dgroup_offset'
                elif (f.frame[0] == 0 and obj.segments[f.frame[1]]['name'] == '_TEXT') or f.frame[0] == 5:
                    frame_base = 0
                    if f.target[0] == 0 and obj.segments[f.target[1]]['name'] != '_TEXT':
                        frame_base = next(iter(target_values))
                    actual = frame_base + int.from_bytes(image[site:site + 2], 'little')
                    kind = 'code_offset'
            if target_name in absolutes and len(absolutes[target_name]) == 1 and f.location == 1 and not f.relative:
                expected = (next(iter(absolutes[target_name])) + f.displacement + addend) & 65535
                actual = int.from_bytes(image[site:site + 2], 'little')
                kind = 'absolute_symbol_addition'
            result = ('pass' if actual == expected else 'FAIL') if expected is not None and actual is not None else 'unresolved'
            if invalid_opcode or (rewritten and rewrites[-1]['result'] == 'FAIL'):
                result = 'FAIL'
            fixes.append({'offset': at, 'location': f.location, 'relative': f.relative,
                          'frame': f.frame, 'target': f.target, 'target_symbol': target_name,
                          'displacement': f.displacement, 'object_bytes': raw[at:at + width].hex(' '),
                          'image_bytes': image[site:site + width].hex(' '), 'kind': kind,
                          'expected_target': expected, 'actual_target': actual, 'result': result})
        row['pubdefs'] = [{'name': n, 'offset': off, 'linear': base + off}
                          for n, (s, off, _) in obj.publics.items() if s == si]
        row['rewrites'], row['fixups'], row['differences'] = rewrites, fixes, differences
        fixed_diffs = [d for d in differences if row['_mask'][d['offset']]]
        row['unexplained_fixed_differences'] = fixed_diffs
        row['proof_level'] = 'PROVISIONAL'
        row['reason'] = 'Complete contribution location; unresolved data/group bindings preclude strict acceptance.'
        del row['_object'], row['_mask']
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root', type=Path, default=Path('C:/tools/borland-c++-3.0/files/LIB'))
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    image = (ROOT / 'reference/load_image.bin').read_bytes()
    oracle = json.loads((ROOT / 'evidence/oracle.json').read_text())
    assert digest(image) == oracle['load_image']['sha256']
    relocs = {r['segment'] * 16 + r['offset'] for r in oracle['relocations']['ordered_segment_offset']}
    def find(name):
        found = list(args.root.rglob(name))
        if len(found) != 1:
            raise ValueError(f'expected one {name} under {args.root}: {found}')
        return found[0]
    libraries = [find(n) for n in ('CL.LIB', 'MATHL.LIB', 'EMU.LIB')]
    rows = [r for p in libraries for r in locate(p, image)]
    # Startup also participates in code-symbol binding.
    path = find('C0L.OBJ')
    if path.exists():
        obj = Object(path.read_bytes())
        for si, seg in enumerate(obj.segments[1:], 1):
            if seg['class'].upper() == 'CODE' and seg['length']:
                mask = bytearray(seg['present'])
                for f in obj.fixups:
                    if f.segment == si:
                        w = {0: 1, 1: 2, 2: 2, 3: 4, 4: 1}[f.location]
                        mask[f.offset:f.offset + w] = bytes(w)
                        if f.location == 3 and f.offset and seg['data'][f.offset - 1] in (0x9a, 0xea):
                            mask[f.offset - 1] = 0
                if not all(not m or seg['data'][i] == image[i] for i, m in enumerate(mask)):
                    continue
                rows.append({'library': str(path), 'member': 'C0L', 'page': 0,
                             'object_sha256': digest(path.read_bytes()), 'segment_index': si,
                             'base': 0, 'end': seg['length'], 'size': seg['length'],
                             'fixed_bytes': sum(mask), 'holes': seg['present'].count(0),
                             'location_count': 1, '_object': obj, '_mask': mask})
    vectors = conversion_vectors(rows, image)
    rejected = []
    for _ in range(4):
        counts = Counter((r['library'], r['page'], r['segment_index']) for r in rows)
        for r in rows:
            r['location_count'] = counts[r['library'], r['page'], r['segment_index']]
        checked = audit([dict(r) for r in rows], image, relocs, vectors)
        failures = [r for r in checked if any(f['result'] == 'FAIL' for f in r['fixups'])]
        if not failures:
            break
        rejected.extend(failures)
        bad = {(r['library'], r['page'], r['segment_index'], r['base']) for r in failures}
        rows = [r for r in rows if (r['library'], r['page'], r['segment_index'], r['base']) not in bad]
    else:
        raise ValueError('candidate rejection did not converge')
    rows = checked
    summary = {'contributions': len(rows), 'unique_locations': sum(r['location_count'] == 1 for r in rows),
               'rejected_candidates': len(rejected),
               'rewrites': dict(Counter(x['result'] for r in rows for x in r['rewrites'])),
               'fixups': dict(Counter(x['result'] for r in rows for x in r['fixups'])),
               'unexplained_fixed_differences': sum(len(r['unexplained_fixed_differences']) for r in rows)}
    report = {'schema': 1, 'image_sha256': digest(image), 'summary': summary,
              'libraries': [{'path': str(p), 'sha256': digest(p.read_bytes())} for p in libraries],
              'conversion_vectors': vectors, 'rejected_candidates': rejected,
              'contributions': sorted(rows, key=lambda r: (r['base'], r['member']))}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(summary))


if __name__ == '__main__':
    main()
