"""One-command strict gate: natural link, every function receipt, every TU receipt, progress.

Exit 0 only if the link is EXACT and every function/TU receipt is EXACT or RELOCATED.
Receipts are regenerated in evidence/match/ (use --scratch to write them under build/check/).
"""
import argparse
import json
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STRICT = {'EXACT', 'RELOCATED'}
# source, function receipt, TU paragraph
MODULES = [
    ('src/dzle.c', 'r066a', '066A'),
    ('src/dzl2.c', 'r0b84', '0B84'),
    ('src/dzfd.c', 'r0ccd', '0CCD'),
    ('src/dosi.c', 'r11c1', '11C1'),
    ('src/menu.c', 'r158d', '158D'),
    ('src/dlogo.c', 'r158d-17f5', '17F5'),
    ('src/dazpcx.c', 's-asm-19f0', '19F0'),
    ('src/dhuf.c', 's-asm-1a72', '1A72'),
]
ASM = ('asm/dvdt.asm', 's-asm-1899', '1899', '1899:0006', '1899:1574')


def run(args):
    p = subprocess.run([sys.executable, '-B', '-I', *args], cwd=ROOT, capture_output=True, text=True)
    return p.returncode, (p.stdout + p.stderr).strip()


def status(path, key):
    try:
        data = json.loads(path.read_text(encoding='utf-8'))
    except (OSError, ValueError):
        return False, 'no receipt'
    if key == 'results':
        bad = [r['function'] for r in data.get('results', []) if r['status'] not in STRICT]
        n = len(data.get('results', []))
        return n > 0 and not bad, f'{n - len(bad)}/{n} functions' + (f' FAIL {bad}' if bad else '')
    return data.get('status') in STRICT, data.get('status', '?')


def module_jobs(out):
    jobs = []
    for src, rec, para in MODULES:
        jobs.append((f'{para} functions', ['tools/objcmp.py', src, '--all', '--fresh', '--out', str(out / f'{rec}.json')],
                     out / f'{rec}.json', 'results'))
        jobs.append((f'{para} TU', ['tools/segcmp.py', para, src, '--out', str(out / f'seg-{para.lower()}.json')],
                     out / f'seg-{para.lower()}.json', 'status'))
    src, rec, para, start, end = ASM
    jobs.append((f'{para} functions', ['tools/asmcmp.py', src, '--out', str(out / f'{rec}.json')],
                 out / f'{rec}.json', 'results'))
    jobs.append((f'{para} TU', ['tools/asmcmp.py', src, '--segment', para, '--start', start, '--end', end,
                                '--out', str(out / f'seg-{para.lower()}.json')],
                 out / f'seg-{para.lower()}.json', 'status'))
    return jobs


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--scratch', action='store_true', help='write receipts to build/check/ instead of evidence/match/')
    ap.add_argument('--link-only', action='store_true', help='only the whole-image link proof (fastest)')
    args = ap.parse_args()
    out = ROOT / ('build/check' if args.scratch else 'evidence/match')
    out.mkdir(parents=True, exist_ok=True)

    ok = True
    code, text = run(['tools/link.py', '--out', str(out / 'link.json')])
    link_ok = code == 0
    print(f'{"ok  " if link_ok else "FAIL"} link: ' + (text.splitlines()[-2] if link_ok else text[-2000:]))
    ok &= link_ok
    if not args.link_only:
        jobs = module_jobs(out)
        # objcmp and segcmp of one source share a compiler cache entry: run them in sequence per module
        pairs = [jobs[i:i + 2] for i in range(0, len(jobs), 2)]

        def do(pair):
            return [(label, run(cmd), receipt, key) for label, cmd, receipt, key in pair]

        with ThreadPoolExecutor(max_workers=4) as pool:
            for results in pool.map(do, pairs):
                for label, (code, text), receipt, key in results:
                    good, detail = status(receipt, key)
                    good &= code == 0
                    ok &= good
                    print(f'{"ok  " if good else "FAIL"} {label}: {detail}' + ('' if good else '\n' + text[-1500:]))
        if not args.scratch:
            run(['tools/progress.py'])
    print('ALL STRICT' if ok else 'GATE FAILED')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
