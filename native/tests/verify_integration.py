"""Full native executable regressions, not DOS equivalence evidence."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

exe, scratch, case = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3]
scratch.mkdir(parents=True, exist_ok=True)
source = Path(__file__).resolve().parent


def run(name, options, at='6', limit='8'):
    path = scratch / (name + '.snap')
    path.unlink(missing_ok=True)
    args = [str(exe), '--headless', '--deterministic', '--seed', '1',
            '--original', 'absent-original.exe', '--snapshot', at, str(path),
            '--exit-after', limit, *options]
    result = subprocess.run(args, cwd=scratch, capture_output=True, timeout=12)
    assert result.returncode == 0, (args, result.returncode, result.stdout, result.stderr)
    data = path.read_bytes()
    header, pixels = data.split(b'END\n', 1)
    state = dict(line.split('=', 1) for line in header.decode().splitlines()[1:])
    assert len(pixels) == int(state['bytes'])
    assert int(state['ns']) == int(float(at) * 1e9)
    assert len(set(pixels)) > 4, state  # actual progressively drawn graphics
    print(case, hashlib.sha256(data).hexdigest(), state['mode'], state['image'], state['fade'])
    return data, state


options = {'vga': [], 'ega': ['/E'], 'tall': ['/T'],
           'keys': ['--script', str(source / 'integration.keys')],
           'repeat': [], 'speed': []}[case]
data, state = run(case, options, at='11', limit='16') if case == 'keys' else run(case, options)
assert int(state['mode']) == (3 if case == 'ega' else 2 if case == 'tall' else 1)
if case == 'keys':
    assert state['semi_auto'] == '1' and state['fade'] == str(ord('a')), state
if case in ('repeat', 'speed'):
    other, _ = run(case + '-again', ['--speed', '4'] if case == 'speed' else [])
    assert other == data, 'simulation depends on repeat/presentation speed'
else:
    expected = json.loads((source / 'integration-hashes.json').read_text())
    assert hashlib.sha256(data).hexdigest() == expected[case]
