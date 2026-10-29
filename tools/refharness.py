#!/usr/bin/env python3
"""Build reconstructed DOS diagnostic drivers with BC++ 3.0 and run via MS-DOS Player.

python -B -I tools/refharness.py DRIVER.c [MODULE.c ...] [--define MODULE:NAME[=VALUE]]
The build is isolated under build/refharness/. This is not a matching-build path.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / 'tools'))
import bcc

RUNNER = Path('C:/tools/nmlgcdos/msdos.exe')
RUNNER_HASHES = {
    str(RUNNER).replace('\\', '/').casefold():
        'f7f6cb0a3e816c5edb13112d327c1bddbf7463fe7bf9a005ca1eb5317751bd02',
    'c:/tools/msdos/binary/i386_x64/msdos.exe':
        '0374576f0f970f5c342e482ed6d44d8e72bbfea3f063ccd3f38360ea81d5c53e',
}
PRODUCT = json.loads((ROOT / 'evidence/toolchains.json').read_text())['products']['bc30']
PRODUCT_ROOT = Path(PRODUCT['paths']['expanded_files'])


def sha(data):
    return hashlib.sha256(data).hexdigest()


def relative(path):
    try:
        return Path(path).resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return str(path)


def locked_tool(relpath):
    path = PRODUCT_ROOT / Path(relpath)
    lock = PRODUCT['key_file_sha256'].get('files/' + relpath.replace('\\', '/'))
    if not lock:
        raise ValueError('toolchain lock is missing ' + relpath)
    data = path.read_bytes()
    if sha(data) != lock['sha256']:
        raise ValueError('toolchain input differs from evidence/toolchains.json: ' + relpath)
    return path, data


def parse_defines(rows, sources):
    result = {str(source.resolve()).casefold(): [] for source in sources}
    lookup = {}
    for source in sources:
        lookup[source.name.casefold()] = source
        lookup[str(source.resolve()).casefold()] = source
        try:
            lookup[source.resolve().relative_to(ROOT).as_posix().casefold()] = source
        except ValueError:
            pass
    for row in rows:
        selector, sep, macro = row.partition(':')
        if not sep or not selector or not re.fullmatch(r'[A-Za-z_]\w*(?:=[^\\/:\r\n]*)?', macro):
            raise ValueError('define must be MODULE:NAME[=VALUE]: ' + row)
        source = lookup.get(selector.casefold())
        if source is None:
            raise ValueError('define selector does not match a driver/module: ' + selector)
        result[str(source.resolve()).casefold()].append('-D' + macro)
    return result


def run(args):
    runner = args.runner
    runner_key = str(runner).replace('\\', '/').casefold()
    runner_hash = sha(runner.read_bytes())
    expected_runner_hash = RUNNER_HASHES.get(runner_key)
    if expected_runner_hash and runner_hash != expected_runner_hash:
        raise ValueError('MS-DOS Player hash differs from pinned runner hash: ' + str(runner))
    sources = [args.driver, *args.modules]
    for source in sources:
        if not source.is_file():
            raise ValueError('source not found: ' + str(source))
    resolved = [source.resolve() for source in sources]
    if len({str(p).casefold() for p in resolved}) != len(resolved):
        raise ValueError('driver and module source paths must be unique')
    defines = parse_defines(args.define, resolved)
    headers = {path.name.upper(): path.read_bytes()
               for path in sorted((ROOT / 'include').glob('*.h'))}
    # Drivers can share model/ABI headers. bcc.py stages the source in an isolated
    # directory, so its quoted local includes must be staged there as well.
    for path in sorted(args.driver.parent.glob('*.h')):
        if len(path.stem) <= 8 and len(path.suffix) <= 4 and path.name.upper() not in headers:
            headers[path.name.upper()] = path.read_bytes()
    name = re.sub(r'[^A-Za-z0-9_.-]+', '_', args.name or args.driver.stem)
    run_root = ROOT / 'build/refharness' / name
    run_root.mkdir(parents=True, exist_ok=True)
    run_dir = Path(tempfile.mkdtemp(prefix='r_', dir=run_root))

    input_files = []
    names = set()
    for source in args.input:
        if not source.is_file():
            raise ValueError('input file not found: ' + str(source))
        dos_name = source.name.upper()
        if len(dos_name.split('.')[0]) > 8 or len(dos_name.split('.')) > 2:
            raise ValueError('DOS input name must be 8.3: ' + dos_name)
        if dos_name in names:
            raise ValueError('duplicate DOS input basename: ' + dos_name)
        names.add(dos_name)
        data = source.read_bytes()
        (run_dir / dos_name).write_bytes(data)
        input_files.append({'source': relative(source), 'name': dos_name,
                            'size': len(data), 'sha256': sha(data)})

    objects = []
    rows = []
    base_flags = ['-ml', '-O1', *args.flag]
    for index, source in enumerate(resolved):
        flags = [*base_flags, *defines[str(source).casefold()]]
        obj = bcc.compile_source(source, 'dazzle', flags, fresh=args.fresh, headers=headers)
        dest_name = f'R{index:02d}.OBJ'
        shutil.copyfile(obj, run_dir / dest_name)
        objects.append(dest_name)
        rows.append({'source': relative(source), 'source_sha256': sha(source.read_bytes()),
                     'defines': defines[str(source).casefold()], 'flags': flags,
                     'object': dest_name, 'object_sha256': sha((run_dir / dest_name).read_bytes()),
                     'compiler_receipt': relative(obj.parent / 'receipt.json')})

    required = {
        'TLINK.EXE': 'BIN/TLINK.EXE',
        'C0L.OBJ': 'LIB/LLIB/C0L.OBJ',
        'CL.LIB': 'LIB/LLIB/CL.LIB',
        'MATHL.LIB': 'LIB/LLIB/MATHL.LIB',
        'EMU.LIB': 'LIB/XLIB/EMU.LIB',
    }
    tool_hashes = {}
    for name_on_disk, path in required.items():
        relpath = path.replace('/', '\\')
        _, data = locked_tool(relpath)
        (run_dir / name_on_disk).write_bytes(data)
        tool_hashes[name_on_disk] = sha(data)
    for name_on_disk in ('DPMI16BI.OVL', 'DPMILOAD.EXE', 'DPMIMEM.DLL', 'DPMIRES.EXE'):
        path = PRODUCT_ROOT / 'BIN' / name_on_disk
        if path.is_file():
            data = path.read_bytes()
            lock = PRODUCT['key_file_sha256'].get('files/BIN/' + name_on_disk)
            if lock and sha(data) != lock['sha256']:
                raise ValueError('toolchain support input differs from lock: ' + name_on_disk)
            (run_dir / name_on_disk).write_bytes(data)
            tool_hashes[name_on_disk] = sha(data)
    library_order = ['EMU.LIB', 'MATHL.LIB', 'CL.LIB']
    response = ('/c /m /s /n C0L.OBJ+' + '+'.join(objects) + '\r\n'
                'REF.EXE\r\nREF.MAP\r\n' + '+'.join(library_order) + '\r\n')
    (run_dir / 'LINK.RSP').write_bytes(response.encode('ascii'))
    env = {'PATH': '.', 'TEMP': '.', 'TMP': '.', 'TZ': 'UTC'}
    env.update({key: os.environ[key] for key in ('SystemRoot', 'WINDIR', 'COMSPEC')
                if key in os.environ})
    link = subprocess.run([str(runner), 'TLINK.EXE', '@LINK.RSP'], cwd=run_dir,
                          env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          timeout=45)
    link_log = link.stdout.decode('cp437', errors='replace')
    (run_dir / 'link.log').write_text(link_log, encoding='utf-8', newline='\n')
    if link.returncode or not (run_dir / 'REF.EXE').is_file():
        report = {'status': 'LINK_ERROR', 'run_dir': relative(run_dir),
                  'link_exit_code': link.returncode, 'link_log': link_log,
                  'sources': rows, 'tool_hashes': tool_hashes, 'response': response}
        (run_dir / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
        raise RuntimeError(f'link failed; see {relative(run_dir / "link.log")}\n{link_log}')

    proc = subprocess.run([str(runner), 'REF.EXE'], cwd=run_dir, env=env,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          timeout=args.timeout)
    output = proc.stdout.decode('cp437', errors='replace')
    (run_dir / 'stdout.txt').write_text(output, encoding='utf-8', newline='\n')
    produced = []
    input_names = {row['name'] for row in input_files}
    for path in sorted(run_dir.iterdir()):
        if path.is_file() and path.name.upper() not in input_names and path.name.upper() not in {
                'REF.EXE', 'REF.MAP', 'LINK.RSP', 'TLINK.EXE',
                'C0L.OBJ', 'CL.LIB', 'MATHL.LIB', 'EMU.LIB', 'LINK.LOG',
                'DPMI16BI.OVL', 'DPMILOAD.EXE', 'DPMIMEM.DLL', 'DPMIRES.EXE',
                'STDOUT.TXT', 'REPORT.JSON'} and path.suffix.upper() not in {'.OBJ'}:
            data = path.read_bytes()
            produced.append({'name': path.name, 'size': len(data), 'sha256': sha(data)})
    report = {
        'schema': 1,
        'status': 'PASS' if proc.returncode == 0 else 'RUN_ERROR',
        'label': 'reconstructed source compiled with BC++ 3.0, run under MS-DOS Player',
        'run_dir': relative(run_dir),
        'compiler': 'Borland C++ 3.0, -ml -O1 via tools/bcc.py',
        'compiler_sha256': bcc.sha((PRODUCT_ROOT / 'BIN/BCC.EXE').read_bytes()),
        'compiler_runner': str(bcc.RUNNER),
        'compiler_runner_sha256': sha(bcc.RUNNER.read_bytes()),
        'tlink_sha256': tool_hashes['TLINK.EXE'],
        'msdos_player': str(runner), 'msdos_player_sha256': runner_hash,
        'link_command': [str(runner), 'TLINK.EXE', '@LINK.RSP'],
        'link_response': response,
        'link_exit_code': link.returncode,
        'run_command': [str(runner), 'REF.EXE'], 'run_exit_code': proc.returncode,
        'sources': rows, 'toolchain_hashes': tool_hashes,
        'libraries': library_order, 'inputs': input_files,
        'files_written_by_program': produced,
        'stdout_file': 'stdout.txt',
    }
    (run_dir / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    sys.stdout.write(output)
    print(f'\n[refharness] {report["status"]}; artifacts: {relative(run_dir)}', file=sys.stderr)
    if proc.returncode:
        raise RuntimeError('DOS program exited with status ' + str(proc.returncode))
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('driver', type=Path)
    parser.add_argument('modules', nargs='*', type=Path)
    parser.add_argument('--define', action='append', default=[], metavar='MODULE:NAME[=VALUE]',
                        help='compile-time define for a source basename or repository-relative path')
    parser.add_argument('--flag', action='append', default=[], help='extra BC++ compiler switch')
    parser.add_argument('--input', action='append', default=[], type=Path,
                        help='copy a DOS 8.3 input file to the run directory')
    parser.add_argument('--name', help='artifact subdirectory name (defaults to driver basename)')
    parser.add_argument('--timeout', type=int, default=180, help='DOS program timeout in seconds')
    parser.add_argument('--runner', type=Path, default=RUNNER,
                        help='MS-DOS Player binary (defaults to C:/tools/nmlgcdos/msdos.exe)')
    parser.add_argument('--fresh', action='store_true', help='bypass the bcc.py object cache')
    args = parser.parse_args()
    args.driver = args.driver.resolve()
    args.modules = [path.resolve() for path in args.modules]
    args.input = [path.resolve() for path in args.input]
    args.runner = args.runner.resolve()
    try:
        run(args)
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as exc:
        parser.exit(2, str(exc) + '\n')


if __name__ == '__main__':
    main()
