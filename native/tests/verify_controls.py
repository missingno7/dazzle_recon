"""Bounded real-executable CLI, waits, resources, and transition checks."""
from pathlib import Path
import hashlib
import subprocess
import sys

exe, scratch, original = map(Path, sys.argv[1:4])
scratch.mkdir(parents=True, exist_ok=True)


def run(options, status=0):
    result = subprocess.run([str(exe), '--headless', '--deterministic', '--seed', '1',
                             '--original', 'absent.exe', *options],
                            cwd=scratch, capture_output=True, timeout=12)
    assert result.returncode == status, (options, result.returncode, result.stdout, result.stderr)
    return result.stdout + result.stderr


for options in ([], ['/W'], ['/E'], ['/T'], ['/E', '/T'], ['/B'], ['/D=8191']):
    run([*options, '--frames', '1'])
    run([*options, '--exit-after', '5'])
freeze = scratch / 'freeze.keys'
freeze.write_text('4 TAB\n')
run(['--script', str(freeze), '--exit-after', '6'])
idle = scratch / 'idle.keys'
idle.write_text('3 ALT+F9\n')
run(['--script', str(idle), '--exit-after', '9'])
run(['/A', '/M', '/F', '/W', '/Q', '/V', '/T', '/2', '/C', '/X', '/4', '/H', '/K', '/N', '/D=4', '--exit-after', '6'])
run(['/S'])
run(['/S=1'])
assert b'Additional Options' in run(['/?'], 10)
assert b'ORDER FORM' in run(['/$'])
assert b'manual unavailable' in run(['/Z'], 3)
assert b'Snapshot write failed' in run(['--snapshot', '0.1', str(scratch / 'absent' / 'bad.snap'), '--exit-after', '1'], 1)
assert b'Snapshot time not reached' in run(['--snapshot', '100', str(scratch / 'late.snap'), '--exit-after', '1'], 1)

# A real config argument, all application modules linked, rapid display changes.
config = scratch / 'Quick.DZL'
source = Path(__file__).resolve().parent
config.write_text('#10500\n$I [1]\n$X [1]\n$F [0]\n' +
                  ''.join(f'~{key} [1]\n' for key in '1234567890qwertyuiopasdfghjklzxcvbnm'))
path = scratch / 'boundary.snap'
run(['/W', str(config), '--snapshot', '9', str(path), '--exit-after', '10'])
header = path.read_bytes().split(b'END\n', 1)[0]
state = dict(line.split('=', 1) for line in header.decode().splitlines()[1:])
assert int(state['fade']) > 0 or int(state['pan']) > 0, state

if original.is_file():
    # The original file is read-only input; extraction is confined to scratch.
    run(['--original', str(original), '/Z'], 3)
    assert hashlib.sha256((scratch / 'DAZZLE.DOC').read_bytes()).hexdigest() == '53b47422b6977433d54ee92751a2b86a3d253a9b953d458eddedc1320dd1cce5'
    keys = scratch / 'escape.keys'
    keys.write_text('4 ESC\n')
    path = scratch / 'closing.snap'
    run(['--original', str(original), '--script', str(keys), '--snapshot', '5', str(path), '--exit-after', '6'])
    header, frame = path.read_bytes().split(b'END\n', 1)
    assert b'mode=3\n' in header and len(set(frame)) > 4, header
print('whole-program controls, options, waits, configuration and transitions passed')
