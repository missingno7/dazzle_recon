#!/usr/bin/env python3
"""Fresh BC++ 3.0/TASM 3.0 build, natural TLINK 5.0 link, strict image audit.

python -B -I tools/link.py
Exit 0 requires the entire load image and relocation-site multiset to equal
the immutable oracle. A failed build/link still writes evidence/match/link.json.
No objects, executable bytes, relocation words, or image lengths are patched.
"""
import argparse
from collections import Counter, defaultdict
import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / 'tools'))
import asmcmp
import bcc
import objcmp
import segcmp

MODULES = ('066A', '0B84', '0CCD', '11C1', '158D', '17F5', '1899', '19F0', '1A72')
DEFAULT_SWITCHES = ('/c', '/m', '/s', '/n')
WORK = ROOT / 'build/workers/s-link'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def relative(path):
    return Path(path).resolve().relative_to(ROOT).as_posix()


def mz(data):
    if len(data) < 28 or data[:2] != b'MZ':
        raise ValueError('candidate is not a complete MZ header')
    fields = struct.unpack_from('<14H', data)
    names = ('magic', 'cblp', 'cp', 'crlc', 'cparhdr', 'minalloc', 'maxalloc',
             'ss', 'sp', 'csum', 'ip', 'cs', 'lfarlc', 'ovno')
    header = dict(zip(names, fields))
    extent = (header['cp']-1)*512 + (header['cblp'] or 512)
    start = header['cparhdr']*16
    end_relocs = header['lfarlc'] + header['crlc']*4
    if not 28 <= end_relocs <= start <= extent <= len(data):
        raise ValueError('candidate MZ extents are invalid')
    relocations = []
    for at in range(header['lfarlc'], end_relocs, 4):
        off, seg = struct.unpack_from('<HH', data, at)
        if seg*16+off+2 > extent-start:
            raise ValueError('candidate relocation outside load image')
        relocations.append((seg, off))
    return header, data[start:extent], relocations


def difference_ranges(candidate, expected, owners):
    """All raw differing ranges, split by independently attributed owner."""
    rows = []
    active = None
    for at in range(max(len(candidate), len(expected))):
        different = (at >= min(len(candidate), len(expected)) or candidate[at] != expected[at])
        if different:
            owner = next((name for lo, hi, name in owners if lo <= at < hi),
                         'unattributed oracle padding' if at < len(expected) else 'outside oracle')
            if active is None or active['owner'] != owner:
                if active is not None:
                    rows.append(active)
                active = {'start': at, 'end_exclusive': at+1, 'owner': owner}
            else:
                active['end_exclusive'] = at+1
        elif active is not None:
            rows.append(active)
            active = None
    if active is not None:
        rows.append(active)
    for row in rows:
        lo, hi = row['start'], row['end_exclusive']
        row.update(size=hi-lo, expected_hex=expected[lo:min(hi, lo+16)].hex(),
                   candidate_hex=candidate[lo:min(hi, lo+16)].hex())
    return rows


def audit_image(data, oracle, image, census, modules):
    header, actual, relocs = mz(data)
    expected = [(r['segment'], r['offset'])
                for r in oracle['relocations']['ordered_segment_offset']]
    actual_sites = [seg*16+off for seg, off in relocs]
    expected_sites = [seg*16+off for seg, off in expected]
    owners = [(0, 0x66AB, 'Borland RTL/C0L'),
              (0x1BAC0, 0x1E810, 'Borland EMU/MATHL'),
              (0x29BF0+0x39A4, 0x29BF0+0x39A6, '158D:_DATA')]
    for module in modules:
        code = module['code']['segment']
        owners.append((code['start_linear'], code['original_extent']['end_linear'], module['module']))
    for row in census['ranges']:
        owners.append((row['linear'], row['linear_end_exclusive'], row['owner']+':'+row['object_segment']))
    differences = difference_ranges(actual, image, owners)
    counts = defaultdict(int)
    first_ranges = defaultdict(list)
    for row in differences:
        counts[row['owner']] += row['size']
        if len(first_ranges[row['owner']]) < 3:
            first_ranges[row['owner']].append(row)
    a, e = Counter(actual_sites), Counter(expected_sites)
    return {
        'status': 'EXACT' if actual == image and a == e else 'MISMATCH',
        'load_image': {'status': 'EXACT' if actual == image else 'MISMATCH',
                       'size': len(actual), 'expected_size': len(image),
                       'sha256': sha(actual), 'expected_sha256': sha(image),
                       'differing_bytes': sum(counts.values()), 'range_count': len(differences),
                       'differing_bytes_by_owner': dict(counts),
                       'first_difference': differences[0] if differences else None,
                       'first_ranges_by_owner': dict(first_ranges)},
        'relocations': {'count': len(relocs), 'expected_count': len(expected),
                        'comparison_basis': 'linear load-image sites (segment*16+offset), preserving multiplicity',
                        'ordered_segment_offset_equal': relocs == expected,
                        'segment_offset_multiset_equal': Counter(relocs) == Counter(expected),
                        'segment_offset_set_equal': set(relocs) == set(expected),
                        'linear_site_multiset_equal': a == e,
                        'linear_site_set_equal': set(a) == set(e),
                        'missing_linear_sites': sorted((e-a).elements()),
                        'extra_linear_sites': sorted((a-e).elements())},
        'header': {'candidate': header,
                   'entry_matches_recovered': (header['cs'], header['ip']) ==
                                              (oracle['entry']['cs'], oracle['entry']['ip']),
                   'stack_matches_recovered': (header['ss'], header['sp']) ==
                                              (oracle['stack']['ss'], oracle['stack']['sp']),
                   'limit': 'Original pre-PKLITE header is not fully recovered; see docs/packing.md.'}}


def build_modules(run, image, relocations):
    headers = {h.name.upper(): h.read_bytes() for h in sorted((ROOT / 'include').glob('*.h'))}
    inventory, _, starts, ends = segcmp.inventory_modules()
    rows = []
    for paragraph in MODULES:
        para = int(paragraph, 16)
        source = ROOT / ('asm/seg1899.asm' if paragraph == '1899'
                         else f'src/seg{paragraph.lower()}.c')
        before = sha(source.read_bytes())
        flags = None
        if paragraph == '1899':
            objpath, build_receipt = asmcmp.assemble(source)
            start, end = '1899:0006', '1899:1574'
            profile = 'TASM30 /ml /m2'
        else:
            # Distinct TU names are necessary: bcc's compile-only wrapper uses
            # PROBE.C for every module. -zC supplies ordinary native SEGDEF names
            # rather than accidentally combining all nine PROBE_TEXT segments.
            flags = [*bcc.PROFILES['dazzle'][1], '-zC' + 'DZ'+paragraph+'_TEXT']
            objpath = bcc.compile_source(source, 'dazzle', flags, fresh=True, headers=headers)
            build_receipt = objpath.parent / 'receipt.json'
            _, off, _ = segcmp.derive_start(para, starts, ends, None)
            end_linear, _ = segcmp.derive_end(para, para*16+off, starts, inventory)
            start = f'{para:04X}:{off:04X}'
            end = f'{end_linear//16:04X}:{end_linear%16:04X}'
            profile = 'dazzle (BC++ 3.0 -ml -O1), distinct native CODE segment name'
        if before != sha(source.read_bytes()):
            raise RuntimeError(f'{relative(source)} changed during its build; retry')
        report = asmcmp.compare_segment(source, objpath, paragraph, start, end, profile)
        # segcmp permits independently recognizable TLINK trailing alignment.
        # Recheck that precise zero gap for this contiguous comparison too.
        extent = report['segment']['original_extent']
        length = report['segment']['object_length']
        gap = extent['length']-length
        gap_start = report['segment']['start_linear']+length
        if (0 < gap < 16 and not any(image[gap_start:gap_start+gap]) and
                (extent['end_linear'] % 16 == 0 or (gap == 1 and extent['end_linear'] % 2 == 0))):
            proven = asmcmp.compare_segment(source, objpath, paragraph, start,
                                            f'{gap_start//16:04X}:{gap_start%16:04X}', profile)
            if proven['status'] in segcmp.STRICT:
                report = proven
                report['segment']['original_extent']['alignment_padding_bytes'] = gap
        obj = objcmp.Object(objpath.read_bytes())
        _, mapping, _ = segcmp.read_metadata([source])
        code_name = report['segment']['name']
        mapping['segments'][code_name] = start
        data = segcmp.compare_data(obj, para, image, relocations, mapping)
        dest = run / f'M{paragraph}.OBJ'
        shutil.copyfile(objpath, dest)
        functions_path = run / f'F{paragraph}.JSON'
        functions_log = io.StringIO()
        with contextlib.redirect_stdout(functions_log):
            try:
                objcmp.check_all(SimpleNamespace(source=source, out=functions_path, profile=profile),
                                 obj, dest)
            except SystemExit as exc:
                function_exit = exc.code
        (run / f'F{paragraph}.LOG').write_text(functions_log.getvalue(), encoding='utf8')
        function_results = json.loads(functions_path.read_text())['results']
        row = {'module': paragraph, 'source': relative(source), 'source_sha256': before,
               'object': relative(dest), 'object_sha256': sha(dest.read_bytes()),
               'build_receipt': relative(build_receipt), 'flags': flags,
               'code': report, 'data': data,
               'function_verification': {
                   'status': 'PASS' if function_exit == 0 else 'FAIL',
                   'passed': sum(r['status'] in segcmp.STRICT for r in function_results),
                   'total': len(function_results), 'receipt': relative(functions_path)}}
        rows.append(row)
        print(f'{paragraph}: code {report["status"]}; data ' +
              ', '.join(r['segment']+' '+r['status'] for r in data), flush=True)
    return rows


def run_link(switches):
    image, relocations = objcmp.load_oracle()
    oracle = json.loads((ROOT / 'evidence/oracle.json').read_text())
    census = json.loads((ROOT / 'evidence/dgroup.json').read_text())
    WORK.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='link_', dir=WORK))
    report = {'schema': 1, 'kind': 'natural-TLINK-load-image', 'status': 'BUILD_ERROR',
              'run': relative(run), 'oracle_sha256': sha((ROOT / 'evidence/oracle.json').read_bytes()),
              'headers_sha256': {relative(h): sha(h.read_bytes()) for h in sorted((ROOT / 'include').glob('*.h'))},
              'switches': switches, 'modules': [], 'comparison': None}
    report['tools_sha256'] = {name: sha((ROOT/'tools'/name).read_bytes())
                              for name in ('link.py', 'bcc.py', 'asmcmp.py', 'segcmp.py', 'objcmp.py')}
    try:
        report['modules'] = build_modules(run, image, relocations)
        if any(m['code']['status'] not in segcmp.STRICT or
               not m['code']['segment']['original_extent']['length_matches'] or
               m['function_verification']['status'] != 'PASS' or
               any(d['status'] not in segcmp.STRICT for d in m['data'])
               for m in report['modules']):
            report['status'] = 'VERIFICATION_ERROR'
            return report
        base = Path('C:/tools/borland-c++-3.0/files')
        inputs = {'TLINK.EXE': base/'BIN/TLINK.EXE', 'C0L.OBJ': base/'LIB/LLIB/C0L.OBJ',
                  'CL.LIB': base/'LIB/LLIB/CL.LIB', 'MATHL.LIB': base/'LIB/LLIB/MATHL.LIB',
                  'EMU.LIB': base/'LIB/XLIB/EMU.LIB'}
        locks = {Path(p).name: v for p, v in census['provenance']['libraries'].items()}
        locks.update({'C0L.OBJ': census['provenance']['C0L_sha256'],
                      'TLINK.EXE': 'ac0ac8b0084e06e9916ee12602aa4fcba998f7b702fc2adf4a213564288147c7'})
        inputs.update({n: base/'BIN'/n for n in ('DPMI16BI.OVL', 'DPMILOAD.EXE', 'DPMIMEM.DLL', 'DPMIRES.EXE')})
        report['toolchain_inputs_sha256'] = {}
        for name, path in inputs.items():
            payload = path.read_bytes()
            if name in locks and sha(payload) != locks[name]:
                raise ValueError('toolchain input differs from lock: '+name)
            (run/name).write_bytes(payload)
            report['toolchain_inputs_sha256'][name] = sha(payload)
        objects = ['C0L.OBJ', *(f'M{p}.OBJ' for p in MODULES)]
        libraries = ['EMU.LIB', 'MATHL.LIB', 'CL.LIB']
        response = (' '.join(switches)+' '+'+'.join(objects)+'\r\nDAZZLE.EXE\r\n'
                    'DAZZLE.MAP\r\n'+'+'.join(libraries)+'\r\n')
        (run/'LINK.RSP').write_bytes(response.encode('ascii'))
        env = {'PATH': '.', 'TEMP': '.', 'TMP': '.', 'TZ': 'UTC'}
        env.update({k: os.environ[k] for k in ('SystemRoot', 'WINDIR', 'COMSPEC') if k in os.environ})
        command = [str(bcc.RUNNER), 'TLINK.EXE', '@LINK.RSP']
        process = subprocess.run(command, cwd=run, env=env, stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT, timeout=45)
        log = process.stdout.decode('cp437', errors='replace')
        (run/'link.log').write_text(log, encoding='utf8', newline='\n')
        report.update(command=command, object_order=objects, library_order=libraries,
                      environment=env, runner_sha256=sha(bcc.RUNNER.read_bytes()),
                      response_file=relative(run/'LINK.RSP'), response=response,
                      linker_exit_code=process.returncode, linker_log=log,
                      unresolved_symbols=sorted(set(re.findall(r'Undefined symbol (\S+)', log))))
        if (run/'DAZZLE.MAP').exists():
            report['map'] = relative(run/'DAZZLE.MAP')
            report['map_sha256'] = sha((run/'DAZZLE.MAP').read_bytes())
        report['status'] = 'LINK_ERROR'
        if (run/'DAZZLE.EXE').exists():
            report['executable'] = relative(run/'DAZZLE.EXE')
            report['executable_sha256'] = sha((run/'DAZZLE.EXE').read_bytes())
            report['comparison'] = audit_image((run/'DAZZLE.EXE').read_bytes(), oracle, image,
                                                census, report['modules'])
            report['comparison']['eligible_for_acceptance'] = process.returncode == 0
            if process.returncode:
                report['comparison']['limit'] = ('Diagnostic output from a failed TLINK invocation; '
                                                  'undefined fixups are unresolved. No match claim.')
            if process.returncode == 0:
                report['status'] = report['comparison']['status']
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as exc:
        report['error'] = str(exc)
    return report


def main():
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, 'reconfigure'):
            stream.reconfigure(errors='backslashreplace')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--switch', action='append', help='replace default TLINK switches; repeat as needed')
    parser.add_argument('--out', type=Path, default=ROOT/'evidence/match/link.json')
    args = parser.parse_args()
    switches = args.switch if args.switch is not None else list(DEFAULT_SWITCHES)
    if any(not re.fullmatch(r'/[A-Za-z0-9=]+', s) for s in switches):
        parser.error('invalid TLINK switch')
    report = run_link(switches)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    temp = args.out.with_suffix('.tmp')
    temp.write_text(json.dumps(report, indent=2)+'\n', encoding='utf8', newline='\n')
    os.replace(temp, args.out)
    print(f'{report["status"]}: {args.out}')
    if report.get('error'):
        print(report['error'])
    if report.get('unresolved_symbols'):
        print(f'{len(report["unresolved_symbols"])} undefined symbols; see receipt/link.log')
    if report.get('comparison'):
        comparison = report['comparison']
        load = comparison['load_image']
        relocations = comparison['relocations']
        print(f'Load image {load["status"]}: {load["size"]}/{load["expected_size"]} bytes; '
              f'{load["differing_bytes"]} differing bytes')
        print(f'Relocations {relocations["count"]}/{relocations["expected_count"]}; '
              f'site multiset equal: {relocations["linear_site_multiset_equal"]}; '
              f'site set equal: {relocations["linear_site_set_equal"]}')
    raise SystemExit(0 if report['status'] == 'EXACT' else 1)


if __name__ == '__main__':
    main()
