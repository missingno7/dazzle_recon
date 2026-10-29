"""Whole-executable checks against cells/text captured from the immutable EXE."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import struct
import sys

exe, scratch = map(Path, sys.argv[1:3])
scratch.mkdir(parents=True, exist_ok=True)
reference = json.loads(Path(__file__).with_name('acceptance-reference.json').read_text())
env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')


def run(name, keys, times, options=(), status=0, headless=True, original='absent.exe'):
    script = scratch / (name + '.keys')
    script.write_text(''.join(f'{t} {key}\n' for t, key in keys))
    args = [str(exe), '--deterministic', '--seed', '1', '--original', str(original),
            '--script', str(script), '--exit-after', '12', *options]
    if headless:
        args.append('--headless')
    paths = [scratch / f'{name}-{t}.snap' for t in times]
    for t, path in zip(times, paths):
        args.extend(('--snapshot', str(t), str(path)))
    result = subprocess.run(args, cwd=scratch, env=env, capture_output=True, timeout=15)
    assert result.returncode == status, (name, result.returncode, result.stderr)
    return result, [p.read_bytes().split(b'END\n', 1)[1] for p in paths]


keys = [(4, 'F1'), (5, 'PageDown'), (6, 'PageDown'), (7, 'PageDown'),
        (8, 'PageDown'), (9, 'PageUp'), (10, 'F1')]
_, pages = run('help', keys, [4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 10.5])
for page, expected in zip(pages[:5], reference['help_cells_sha256']):
    assert hashlib.sha256(page).hexdigest() == expected
assert pages[5] == pages[3]  # PgUp returns to page 4.
assert len(pages[6]) == 320 * 400  # F1 resumes drawing.

_, panels = run('config', [(5, 'F2'), (6, 'PageDown'), (7, 'PageDown'),
                          (8, 'PageDown'), (9, 'F10')], [5.5, 6.5, 7.5, 8.5, 9.5])
for panel, expected in zip(panels[:4], reference['config_cells_sha256']):
    assert hashlib.sha256(panel).hexdigest() == expected
assert len(panels[4]) == 320 * 400

result, pages = run('cli-help', [(3, 'SPACE')], [2], ['/?'], status=10, headless=False)
assert pages[0][:23 * 160:2].decode('cp437') == reference['cli_help_first_page_chars']
text = result.stdout.replace(b'\r', b'').decode('cp437')
position = text.index('If an "inputfile"')
for line in reference['cli_help_second_page_lines']:
    position = text.index(line, position) + len(line)

# Minimal resource metadata, no executable code or copyrighted image payload.
# The supplied original identifies itself as r1=500/r2=1000; preserve those IDs
# even on machines where the optional distribution EXE is absent.
metadata = bytearray(39)
metadata[:2] = b'MZ'
struct.pack_into('<HH', metadata, 2, 28, 1)
struct.pack_into('<HH', metadata, 29 + 4, 500, 1000)
original = scratch / 'order-metadata.exe'
original.write_bytes(metadata)
result, _ = run('order', [], [], ['/$'], original=original)
text = result.stdout.replace(b'\r', b'')
form = text[text.index(b'Software'):text.index(b'\x0c') + 1]
assert hashlib.sha256(form).hexdigest() == reference['order_form_lf_sha256']
print('Original help/menu cells, paging, return keys, CLI help and exact order-form text passed')
