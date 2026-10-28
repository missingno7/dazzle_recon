"""Pinned real-mode Borland compiler. No linker, ambient CFG, or ambient headers.

python tools/bcc.py source.c --profile bc30 --flag=-O --fresh
The printed path is the object. Scratch/cache live only in build/workers/compiler.
Profiles are reproducing hypotheses, not claims of historical identification.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'build/workers/compiler'
RUNNER = Path('C:/tools/nmlgcdos/msdos.exe')
PROFILES = {name: (name, ['-ml', '-O1']) for name in ('bc20', 'bc30', 'bc31', 'tc30')}
PROFILES['dazzle'] = ('bc30', ['-ml', '-O1'])


def sha(data):
    return hashlib.sha256(data).hexdigest()


def compile_source(source, profile='dazzle', flags=None, fresh=False, headers=None):
    """Return a verified cached OBJ path, or compile in a new private directory.

    flags=None selects the profile; an explicit list replaces it. Extra local
    headers must be supplied as {DOS_basename: bytes}; all are hashed and copied.
    Fresh always invokes the compiler (use this for acceptance).
    """
    compiler, defaults = PROFILES[profile]
    flags = list(defaults if flags is None else flags)
    # Keep the API compile-only and the DOS command/environment deterministic.
    for flag in flags:
        if not flag.startswith('-') or any(c in flag for c in '\r\n@/\\:'):
            raise ValueError('unsupported flag/path: ' + flag)
        if flag.lower().startswith(('-i', '-l', '-o')) and not flag.startswith(('-O', '-Od')):
            raise ValueError('output/include/link path switches are managed by wrapper')
        if flag in ('-B', '-S', '-P'):
            raise ValueError('only native C-to-OMF compilation is supported')
    catalog = json.loads((ROOT / 'evidence/toolchains.json').read_text())
    product = catalog['products'][compiler]
    exe = 'TCC.EXE' if compiler == 'tc30' else 'BCC.EXE'
    binary = Path(product['paths']['bin']) / exe
    compiler_bytes = binary.read_bytes()
    expected = product['key_file_sha256']['files/BIN/' + exe]['sha256']
    if sha(compiler_bytes) != expected:
        raise ValueError('compiler SHA256 differs from evidence/toolchains.json')
    support = {}
    for name in ('DPMI16BI.OVL', 'DPMILOAD.EXE', 'DPMIMEM.DLL', 'DPMIRES.EXE'):
        path = binary.parent / name
        if path.exists():
            data = path.read_bytes()
            lock = product['key_file_sha256'].get('files/BIN/' + name)
            if lock and sha(data) != lock['sha256']:
                raise ValueError('support file SHA256 differs from lock: ' + name)
            support[name] = data
    inputs = {p.relative_to(product['paths']['include']).as_posix().upper(): p.read_bytes()
              for p in sorted(Path(product['paths']['include']).rglob('*')) if p.is_file()}
    local = headers or {}
    for name in local:
        if Path(name).name != name or len(name.split('.')[0]) > 8 or ':' in name:
            raise ValueError('local headers require DOS 8.3 basenames')
    source_bytes = Path(source).read_bytes()
    manifest = {'schema': 4, 'compiler': compiler, 'compiler_sha256': expected,
                'support_sha256': {k: sha(v) for k, v in support.items()},
                'runner_sha256': sha(RUNNER.read_bytes()), 'flags': flags,
                'source_sha256': sha(source_bytes),
                'input_mtime': 694224000,
                'headers': {k: sha(v) for k, v in inputs.items()},
                'local_headers': {k: sha(v) for k, v in local.items()},
                'command': [exe, '-c', '-IINCLUDE', *flags, 'PROBE.C'],
                'environment': {'PATH': '.', 'TEMP': '.', 'TMP': '.', 'TZ': 'UTC'}}
    key = sha(json.dumps(manifest, sort_keys=True).encode())
    cache = WORK / 'cache' / key
    obj = cache / 'PROBE.OBJ'
    receipt = cache / 'receipt.json'
    if not fresh and obj.is_file() and receipt.is_file():
        saved = json.loads(receipt.read_text())
        if saved.get('inputs') == manifest and sha(obj.read_bytes()) == saved.get('object_sha256'):
            return obj
    (WORK / 'runs').mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='bcc_', dir=WORK / 'runs'))
    (run / exe).write_bytes(compiler_bytes)
    for name, data in support.items():
        (run / name).write_bytes(data)
    (run / 'PROBE.C').write_bytes(source_bytes)
    for name, data in inputs.items():
        dest = run / 'INCLUDE' / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(data)
    for name, data in local.items():
        (run / name).write_bytes(data)
    for path in (run / 'PROBE.C', *(run / 'INCLUDE').rglob('*'),
                 *(run / name for name in local)):
        if path.is_file():
            os.utime(path, (manifest['input_mtime'], manifest['input_mtime']))
    env = dict(manifest['environment'])
    # Windows process creation needs these; do not inherit INCLUDE, LIB, BCC etc.
    for name in ('SystemRoot', 'WINDIR', 'COMSPEC'):
        if name in os.environ:
            env[name] = os.environ[name]
    result = subprocess.run([str(RUNNER), *manifest['command']], cwd=run, env=env,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=45)
    log = result.stdout.decode('cp437', errors='replace')
    (run / 'compile.log').write_text(log, encoding='utf-8')
    produced = next(iter(run.glob('*.OBJ')), None)
    if result.returncode or produced is None or 'Error ' in log:
        raise RuntimeError(f'compiler failed ({result.returncode}), {run}\n{log}')
    data = produced.read_bytes()
    cache.mkdir(parents=True, exist_ok=True)
    # Replacing the object and publishing its receipt last makes incomplete cache
    # entries harmless. A competing identical compile has identical inputs.
    temp = run / 'receipt.json'
    temp.write_text(json.dumps({'inputs': manifest, 'object_sha256': sha(data),
                               'run': str(run), 'log': log}, indent=2) + '\n')
    shutil.copyfile(produced, run / 'cache.obj')
    os.replace(run / 'cache.obj', obj)
    os.replace(temp, receipt)
    return obj


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('source', type=Path)
    ap.add_argument('--profile', choices=PROFILES, default='dazzle')
    ap.add_argument('--flag', action='append', help='replace profile flags; use --flag=-mm')
    ap.add_argument('--fresh', action='store_true')
    ap.add_argument('--header', action='append', type=Path, default=[],
                    help='copy and hash a local header (DOS basename)')
    args = ap.parse_args()
    try:
        headers = {p.name: p.read_bytes() for p in args.header}
        if len(headers) != len(args.header):
            raise ValueError('duplicate local header basename')
        print(compile_source(args.source, args.profile, args.flag, args.fresh, headers))
    except (OSError, ValueError, RuntimeError, subprocess.TimeoutExpired) as exc:
        ap.exit(2, str(exc) + '\n')


if __name__ == '__main__':
    main()
