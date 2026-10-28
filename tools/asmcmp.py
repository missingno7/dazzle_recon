"""Fresh, pinned TASM 3.0 build followed by the existing strict OMF verifier.

python -I tools/asmcmp.py asm/seg1899.asm --out evidence/match/s-asm-1899.json
python -I tools/asmcmp.py asm/seg1899.asm --segment 1899 --start 1899:0006 --end 1899:1574 --out evidence/match/seg-1899.json
C input is also supported: BC30 -c -ml -O1 -S, then TASM /ml /m2.
Metadata is read from SOURCE; no candidate or oracle bytes are altered here.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
RUNNER = Path('C:/tools/nmlgcdos/msdos.exe')
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / 'tools'))
import objcmp
import segcmp


def sha(data):
    return hashlib.sha256(data).hexdigest()


def assemble(source):
    """Build once in an isolated directory; return object and hashed receipt."""
    source = Path(source).resolve()
    if source.suffix.lower() not in ('.asm', '.c'):
        raise ValueError('source must be .ASM or .C')
    product = json.loads((ROOT / 'evidence/toolchains.json').read_text())['products']['bc30']
    bindir = Path(product['paths']['bin'])
    inputs = {'PROBE' + source.suffix.upper(): source.read_bytes()}
    names = ['TASM.EXE']
    if source.suffix.lower() == '.c':
        names += ['BCC.EXE']
        names += [n for n in ('DPMI16BI.OVL', 'DPMILOAD.EXE', 'DPMIMEM.DLL', 'DPMIRES.EXE')
                  if (bindir / n).exists()]
        inc = Path(product['paths']['include'])
        inputs.update({'INCLUDE/' + p.relative_to(inc).as_posix(): p.read_bytes()
                       for p in sorted(inc.rglob('*')) if p.is_file()})
    binaries = {}
    for name in names:
        data = (bindir / name).read_bytes()
        lock = product['key_file_sha256'].get('files/BIN/' + name)
        if name in ('TASM.EXE', 'BCC.EXE') and lock is None:
            raise ValueError('missing toolchain lock for ' + name)
        if lock and sha(data) != lock['sha256']:
            raise ValueError('SHA256 differs from evidence/toolchains.json: ' + name)
        binaries[name] = data
    work = ROOT / 'build/workers/s-asm/asmcmp/runs'
    work.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='tasm_', dir=work))
    for name, data in {**inputs, **binaries}.items():
        path = run / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        os.utime(path, (694224000, 694224000))
    env = {'PATH': '.', 'TEMP': '.', 'TMP': '.', 'TZ': 'UTC'}
    env.update({k: os.environ[k] for k in ('SystemRoot', 'WINDIR', 'COMSPEC') if k in os.environ})
    commands = []
    if source.suffix.lower() == '.c':
        commands.append(['BCC.EXE', '-c', '-ml', '-O1', '-IINCLUDE', '-S', 'PROBE.C'])
    commands.append(['TASM.EXE', '/ml', '/m2', 'PROBE.ASM,PROBE.OBJ,PROBE.LST'])
    logs = []
    for command in commands:
        result = subprocess.run([str(RUNNER), *command], cwd=run, env=env,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=45)
        logs.append(result.stdout.decode('cp437', errors='replace'))
        (run / 'compile.log').write_text('\n'.join(logs), encoding='utf8', newline='\n')
        if result.returncode or '**Fatal**' in logs[-1]:
            raise RuntimeError(f'build failed in {run}\n{logs[-1]}')
    obj = run / 'PROBE.OBJ'
    receipt = run / 'receipt.json'
    receipt.write_text(json.dumps({
        'source': str(source), 'commands': commands,
        'inputs_sha256': {n: sha(d) for n, d in inputs.items()},
        'tools_sha256': {n: sha(d) for n, d in binaries.items()},
        'runner_sha256': sha(RUNNER.read_bytes()), 'environment': env,
        'input_mtime': 694224000, 'object_sha256': sha(obj.read_bytes())
    }, indent=2) + '\n', encoding='utf8', newline='\n')
    return obj, receipt


def compare_segment(source, object_path, selector, start, end, profile):
    """Compare one complete CODE contribution using segcmp's strict core.

    Explicit oracle bounds include alignment/padding and must be established
    independently of the object length. A public-table view exposes the entire
    contribution as one extent; bytes and fixups are unchanged. In particular,
    near calls within the contribution retain their actual TU displacements.
    """
    entries, mapping, conflicts = segcmp.read_metadata([source.resolve()])
    obj = objcmp.Object(object_path.read_bytes())
    paragraph, code_index, code = segcmp.choose_code_segment(obj, selector, entries)
    start_seg, start_off = segcmp.parse_address(start)
    end_seg, end_off = segcmp.parse_address(end)
    original_length = end_seg * 16 + end_off - (start_seg * 16 + start_off)
    if start_seg != paragraph or original_length <= 0:
        raise ValueError('segment bounds must begin in the selected paragraph and have positive length')
    if start_off + max(code['length'], original_length) > 65536:
        raise ValueError('segment bounds exceed the target CS window')
    inventory, _, _, _ = segcmp.inventory_modules()
    objcmp.check_known_bindings(mapping, inventory)
    image, relocations = objcmp.load_oracle()
    extents = segcmp.public_extents(obj, code_index, code['length'])
    publics = []
    for extent in extents:
        if extent['synthetic']:
            continue
        public = extent['names'][0]
        address, size, entry = segcmp.function_address_and_size(
            public, extent['start'], extent['end'] - extent['start'],
            paragraph, start_off, entries)
        result, _ = segcmp.compare_one(obj, public, address, size, image, relocations, mapping)
        publics.append({'function': entry['function'] if entry else segcmp.display_public(extent['names']),
                        'address': address, 'size': size,
                        'status': str(result['status']).upper() if entry else 'UNKNOWN',
                        'reason': result.get('reason', ''),
                        'object_offset': extent['start'],
                        'object_size': extent['end'] - extent['start']})

    view = copy.copy(obj)
    view.publics = {name: value for name, value in obj.publics.items() if value[0] != code_index}
    whole = '__asmcmp_whole_segment__'
    view.publics[whole] = (code_index, 0, 0)
    result, resolved = segcmp.compare_one(view, whole, start, original_length,
                                         image, relocations, mapping)
    hole = next((i for i, present in enumerate(code['present']) if not present), None)
    if hole is not None:
        result = {'status': 'MISMATCH', 'offset': hole,
                  'reason': f'object CODE segment has an un-emitted byte at +{hole:#x}'}
    status = str(result['status']).upper()
    first = None
    if status not in segcmp.STRICT:
        offset = result.get('offset', 0)
        linear = start_seg * 16 + start_off + offset
        owner, nearest = segcmp.segment_owner(linear, offset, extents, entries, paragraph)
        first = {'offset': offset, 'address': segcmp.address_text(start_seg, start_off + offset),
                 'function': owner, 'nearest_public': nearest, 'reason': result.get('reason', ''),
                 'object_byte': f'{resolved[offset]:02X}' if offset < len(resolved) else None,
                 'image_byte': f'{image[linear]:02X}' if linear < len(image) else None}
    data = segcmp.data_summary(obj, code_index)
    return {
        'schema': 1, 'kind': 'translation-unit-segment',
        'source': source.resolve().relative_to(ROOT).as_posix(),
        'sources': [source.resolve().relative_to(ROOT).as_posix()], 'profile': profile,
        'source_sha256': sha(source.read_bytes()),
        'object': str(object_path), 'object_sha256': sha(object_path.read_bytes()),
        'segment': {'name': code['name'], 'paragraph': f'{paragraph:04X}',
                    'start': start.upper(), 'start_linear': start_seg * 16 + start_off,
                    'start_derivation': 'explicit --start oracle binding',
                    'object_length': code['length'],
                    'original_extent': {'end': end.upper(), 'end_linear': end_seg * 16 + end_off,
                                        'length': original_length,
                                        'end_derivation': 'explicit --end oracle binding (exclusive)',
                                        'length_matches': code['length'] == original_length}},
        'status': status, 'reason': result.get('reason', ''), 'first_difference': first,
        'proof': 'one contiguous CODE contribution; complete extent and all fixups; no per-function repositioning',
        'metadata_conflicts': conflicts, 'results': publics,
        'segment_extents': [{'start': 0, 'end': code['length'], 'public': 'whole CODE contribution',
                             'synthetic': True, 'status': status, 'reason': result.get('reason', '')}],
        'data_contributions': data['segments'], 'literal_pool': data['literal_pool'],
        'code_emitted_bytes': sum(code['present']),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--all', action='store_true', help='accepted for objcmp CLI compatibility; always enabled')
    parser.add_argument('--out', type=Path)
    parser.add_argument('--segment', metavar='PARAGRAPH_OR_NAME',
                        help='compare the entire CODE contribution in one extent')
    parser.add_argument('--start', help='independent oracle start SSSS:OOOO (required with --segment)')
    parser.add_argument('--end', help='independent exclusive oracle end SSSS:OOOO (required with --segment)')
    args = parser.parse_args()
    if bool(args.segment) != bool(args.start and args.end) or (not args.segment and (args.start or args.end)):
        parser.error('--segment requires both --start and --end; bounds require --segment')
    try:
        obj, receipt = assemble(args.source)
        profile = 'bc30 -c -ml -O1 -S; TASM30 /ml /m2' if args.source.suffix.lower() == '.c' else 'TASM30 /ml /m2'
        if args.segment:
            report = compare_segment(args.source, obj, args.segment, args.start, args.end, profile)
            report['build_receipt'] = str(receipt)
            if args.out:
                args.out.parent.mkdir(parents=True, exist_ok=True)
                args.out.write_text(json.dumps(report, indent=2) + '\n', encoding='utf8', newline='\n')
            segcmp.print_result(report)
            raise SystemExit(0 if report['status'] in segcmp.STRICT else 1)
        try:
            objcmp.check_all(SimpleNamespace(source=args.source, out=args.out, profile=profile),
                             objcmp.Object(obj.read_bytes()), obj)
        except SystemExit:
            if args.out:
                report = json.loads(args.out.read_text())
                report['build_receipt'] = str(receipt)
                report['object_sha256'] = sha(obj.read_bytes())
                args.out.write_text(json.dumps(report, indent=1) + '\n', encoding='utf8', newline='\n')
            raise
    except (OSError, ValueError, KeyError, RuntimeError, subprocess.TimeoutExpired) as exc:
        parser.exit(2, 'UNKNOWN: ' + str(exc) + '\n')


if __name__ == '__main__':
    main()
