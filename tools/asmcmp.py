"""Fresh, pinned TASM 3.0 build followed by the existing strict OMF verifier.

python -I tools/asmcmp.py asm/seg1899.asm --out evidence/match/s-asm-1899.json
C input is also supported: BC30 -c -ml -O1 -S, then TASM /ml /m2.
Metadata is read from SOURCE; no candidate or oracle bytes are altered here.
"""
import argparse
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--all', action='store_true', help='accepted for objcmp CLI compatibility; always enabled')
    parser.add_argument('--out', type=Path)
    args = parser.parse_args()
    try:
        obj, receipt = assemble(args.source)
        profile = 'bc30 -c -ml -O1 -S; TASM30 /ml /m2' if args.source.suffix.lower() == '.c' else 'TASM30 /ml /m2'
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
