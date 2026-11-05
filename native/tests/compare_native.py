"""Fresh Classic pixel oracle, presentation centre equality, and review gallery.

No recorded reference pixels, masks, or Native-to-Native expected images.
All expected pixels come from a fresh Classic run. Limits cover on-ink metrics;
background equality is secondary. Ramp distances use 3..255 or 3..15 cyclically,
with reserved indices literal. Within-cell is Euclidean <=1 (axial neighbours);
best integer shift searches +/-2 cells. Raw Stretch has both coordinate units 1.
"""
from pathlib import Path
from collections import Counter
import argparse
import hashlib
import json
import math
import os
import struct
import subprocess
import zlib

NAMES = ('arches atom balls bars bigstar boxes cones corkscrew curves cycloids daisies '
         'diag1 diag2 ellipse formula1 grid hexagon hilbert lines littleStars lotsadots '
         'pedals puffs ribbons triangles tunnel sierpinski sine spiral spiral1 squares '
         'squirals sweep sweep2 wandering zigstream butterflies diamonds gumballs tiles wedges yarnball').split()
NATIVE = {0, 3, 4, 5, 6, 7, 9, 10, 13, 16, 17, 19, 22, 27, 28, 29, 30, 31,
          32, 33, 34, 35, 38, 41}
POLICIES = dict(original=0, crop=1, stretch=2, extended=3)


def png(path, w, h, rows):
    def chunk(k, b):
        return struct.pack('>I', len(b))+k+b+struct.pack('>I', zlib.crc32(k+b))
    data = b''.join(b'\0'+row for row in rows)
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))+
                     chunk(b'IDAT', zlib.compress(data))+chunk(b'IEND', b''))


def indexed_png(path, data, w, h, edge=None, palette=None):
    # Fixed diagnostic index palette: zero is black, distinct indices stay distinct.
    colors = [bytes((0, 0, 0))]+[bytes(((i*97)%192+63, (i*57)%192+63, (i*23)%192+63)) for i in range(1, 256)]
    if palette: colors = [palette[i:i+3] for i in range(0, 768, 3)]
    rows = []
    for y in range(h):
        row = bytearray(b''.join(colors[c] for c in data[y*w:(y+1)*w]))
        if edge:
            for x in edge:
                if 0 <= x < w:
                    row[3*x:3*x+3] = b'\xff\xff\xff' if y%12 < 6 else b'\x60\x60\x60'
        rows.append(bytes(row))
    png(path, w, h, rows)


def run(args, e, mode, style, policy, w, h, full=False, seed=None):
    if seed is None: seed = 3 if e == 33 else 1
    prefix = args.scratch/f'{NAMES[e]}-{mode}-{style}-{policy}-{w}x{h}-s{seed}'
    cmd = [str(args.exe), str(e), str(mode), style, str(POLICIES[policy]), str(w), str(h),
           str(seed), str(prefix), str(int(full))]
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    r = subprocess.run(cmd, env=env, capture_output=True, timeout=90)
    assert r.returncode == 0, (cmd, r.returncode, r.stdout[-1000:], r.stderr)
    records = []; cycles = None
    for line in r.stdout.decode().splitlines():
        if line.startswith("RASTER cycles="): cycles = int(line.split("=")[1]); continue
        parts = line.split()
        if len(parts) != 16 or not parts[0].isdigit():
            continue
        n, phase, lw, lh, x, y, vw, vh, ns, rng, state, instance = parts[:12]
        record = dict(phase=phase, lw=int(lw), lh=int(lh), rect=list(map(float, (x, y, vw, vh))),
                      ns=int(ns), rng=int(rng), random_state=int(state), instance=int(instance),
                      clip=list(map(float, parts[12:])))
        for ext in ('idx', 'logical', 'state', 'pal'):
            p = Path(f'{prefix}-{n}.{ext}')
            record[ext] = p.read_bytes()
            p.unlink()
        if cycles is not None: record['raster_cycles'] = cycles
        records.append(record)
    assert len(records) == (6 if full else 3), (cmd, r.stdout)
    return records


def edges(r):
    x, y, w, h = r
    return tuple(math.ceil(v-.5) for v in (x, y, x+w, y+h))


def center_checks(args):
    receipts = []
    modes = (0,) if args.quick else range(4)
    sizes = ((640, 480), (960, 540)) if args.quick else ((641, 479), (1920, 1080), (2560, 1440))
    for mode in modes:
        for w, h in sizes:
            for e in (range(42) if not args.effects else [int(x) for x in args.effects.split(",")]):
                for style in ('faithful', 'native') if e in NATIVE else ('faithful',):
                    a = run(args, e, mode, style, 'original', w, h, True)
                    b = run(args, e, mode, style, 'extended', w, h, True)
                    for original, extended in zip(a, b):
                        assert original['state'] == extended['state'], (NAMES[e], mode, style, original['phase'], 'state')
                        left, top, cw, ch = map(int, original['clip'])
                        right, bottom = left+cw, top+ch
                        for y in range(top, bottom):
                            assert original['idx'][y*w+left:y*w+right] == extended['idx'][y*w+left:y*w+right], (
                                NAMES[e], mode, style, w, h, original['phase'], 'centre row', y)
                        if w*3 == h*4:
                            assert original['idx'] == extended['idx'], (NAMES[e], style, '4:3')
                        side = sum(c != 0 for y in range(top, bottom) for c in
                                   extended['idx'][y*w:y*w+left]+extended['idx'][y*w+right:(y+1)*w])
                        area = (w-right+left)*(bottom-top)
                        receipts.append(dict(effect=NAMES[e], mode=mode, style=style, size=[w, h],
                            phase=original['phase'], ns=original['ns'], side_pixels=side, side_area=area,
                            side_percent=100*side/area if area else 0,
                            state_sha256=hashlib.sha256(original['state']).hexdigest()))
                print('centre exact', NAMES[e], mode, f'{w}x{h}', flush=True)
    (args.scratch/'centres.json').write_text(json.dumps(receipts, indent=2)+'\n')


def reduce_grid(data, w, h, lw, lh, rect):
    x, y, vw, vh = rect
    xs = [min(w-1, max(0, int(x+(i+.5)*vw/lw))) for i in range(lw)]
    ys = [min(h-1, max(0, int(y+(i+.5)*vh/lh))) for i in range(lh)]
    return bytes(data[j*w+i] for j in ys for i in xs)


def ink_bits(data, w):
    # Four zero cells per row prevent horizontal shifts from crossing a row edge.
    return int.from_bytes(b''.join(bytes(bool(c) for c in data[y:y+w])+b'\0'*4
                                   for y in range(0, len(data), w)), 'little')


def metrics(classic, native, w, colors=256):
    ci = sum(bool(c) for c in classic); ni = sum(bool(n) for n in native)
    raw = Counter(n-c for c, n in zip(classic, native) if c and n)
    signed = Counter()
    ramp = colors-3
    for c, n in zip(classic, native):
        if not c or not n: continue
        d = n-c
        if 3 <= c < colors and 3 <= n < colors:
            d = (d+ramp//2) % ramp-ramp//2
        signed[d] += 1
    shared = sum(raw.values()); equal_ink = raw[0]
    cb = ink_bits(classic, w); nb = ink_bits(native, w); stride = (w+4)*8
    def shifted(bits, dx, dy):
        n = 8*dx+stride*dy
        return bits << n if n >= 0 else bits >> -n
    shifts = [(dx, dy, (cb & shifted(nb, dx, dy)).bit_count())
              for dy in range(-2, 3) for dx in range(-2, 3)]
    best = max(shifts, key=lambda v: (v[2], -abs(v[0])-abs(v[1])))
    def expanded(bits):
        return bits | (bits << 8) | (bits >> 8) | (bits << stride) | (bits >> stride)
    pct = lambda n, d: 100*n/d if d else 100.0
    equal = sum(c==n for c,n in zip(classic,native))
    return dict(classic_ink=ci, native_ink=ni, shared_ink=shared,
        classic_overlap=pct(shared, ci), native_overlap=pct(shared, ni),
        exact_colour=pct(equal_ink, shared),
        within_1=pct(sum(n for d,n in signed.items() if abs(d)<=1), shared),
        within_2=pct(sum(n for d,n in signed.items() if abs(d)<=2), shared),
        signed_histogram=dict(sorted(signed.items())), raw_signed_histogram=dict(sorted(raw.items())),
        classic_within_cell=pct((cb & expanded(nb)).bit_count(), ci),
        native_within_cell=pct((nb & expanded(cb)).bit_count(), ni),
        best_shift=list(best[:2]), best_shift_overlap=pct(best[2], ci),
        equal_background_included=equal, differing_background_included=len(classic)-equal,
        native_only=ni-shared, classic_only=ci-shared, wrong_colour=shared-equal_ink)


def colour_distances(classic, native, w):
    points = {}
    for i,c in enumerate(classic):
        if c: points.setdefault(c,[]).append((i%w,i//w))
    def tree(p, axis=0):
        if not p: return None
        p.sort(key=lambda v:v[axis]); k=len(p)//2
        return (p[k],axis,tree(p[:k],1-axis),tree(p[k+1:],1-axis))
    trees = {c:tree(p) for c,p in points.items()}
    def nearest(node,x,y,best=math.inf):
        if node is None: return best
        (px,py),axis,left,right=node
        best=min(best,(x-px)**2+(y-py)**2)
        delta=(x-px) if axis==0 else (y-py)
        near,far=(left,right) if delta<0 else (right,left)
        best=nearest(near,x,y,best)
        if delta*delta<best: best=nearest(far,x,y,best)
        return best
    maximum=0; missing=set(); worst=None
    for i,n in enumerate(native):
        if not n or n==classic[i]: continue
        if n not in trees: missing.add(n); continue
        d=nearest(trees[n],i%w,i//w)
        if d>maximum: maximum=d; worst=[i%w,i//w,n]
    return dict(max_same_colour_distance=None if missing else math.sqrt(maximum),
                max_present_colour_distance=math.sqrt(maximum),
                colours_absent_in_classic=sorted(missing),farthest_native_ink=worst)


def differences(path, classic, native, w, h):
    colors = {(False, True): b'\x00\xff\xff', (True, False): b'\xff\x40\x40', (True, True): b'\xff\xff\x00'}
    diff = [b'\0\0\0' if c == n else colors[bool(c), bool(n)] for c, n in zip(classic, native)]
    png(path, w, h, [b''.join(diff[y*w:(y+1)*w]) for y in range(h)])


def pixel_checks(args):
    records = []
    limits_path = Path(__file__).with_name('native-pixel-limits.json')
    limits = json.loads(limits_path.read_text()) if args.check else {}
    modes = [int(x) for x in args.modes.split(',')]
    effects = sorted(NATIVE) if not args.effects else [int(x) for x in args.effects.split(',')]
    seeds = [int(x) for x in args.seeds.split(',')]
    failures = []
    for mode in modes:
        lw, lh = (640, 350) if mode >= 2 else (320, 200 if mode == 1 else 400)
        for e in effects:
            for seed in seeds:
                reference = run(args, e, mode, 'classic', 'original', lw, lh, seed=seed)[2]
                sizes = [('raw', lw, lh, 'stretch'), ('2x', 2*lw, 2*lh, 'original'), ('4k', 3840, 2160, 'original')]
                if args.scales: sizes = [s for s in sizes if s[0] in args.scales.split(',')]
                elif args.quick: sizes = sizes if seed in (1,2) else sizes[:1]
                for label, w, h, policy in sizes:
                    a = run(args, e, mode, 'native', policy, w, h, seed=seed)[2]
                    assert a['state'] == reference['state'], (NAMES[e], mode, seed, label, 'logical state')
                    reduced = a['idx'] if label == 'raw' else reduce_grid(a['idx'], w, h, lw, lh, a['rect'])
                    m = metrics(reference['idx'], reduced, lw, 16 if mode>=2 else 256)
                    if args.distances: m.update(colour_distances(reference['idx'],reduced,lw))
                    record = dict(effect=NAMES[e], mode=mode, seed=seed, scale=label, policy=policy, size=[w,h],
                                  ns=a['ns'], instance=a['instance'], logical_grid=[lw,lh], viewport=a['rect'],
                                  grid_pitch=[a['rect'][2]/lw,a['rect'][3]/lh], **m)
                    records.append(record)
                    if args.images:
                        stem = args.scratch/f'{NAMES[e]}-{mode}-s{seed}-{label}'
                        stem.with_suffix('.classic.idx').write_bytes(reference['idx'])
                        stem.with_suffix('.native.idx').write_bytes(reduced)
                        indexed_png(stem.with_suffix('.classic.png'), reference['idx'], lw, lh)
                        indexed_png(stem.with_suffix('.native.png'), reduced, lw, lh)
                        differences(stem.with_suffix('.diff.png'),reference['idx'],reduced,lw,lh)
                    if args.check:
                        limit = limits['effects'][NAMES[e]][label]
                        for metric, floor in limit.items():
                            if m[metric]+1e-8 < floor: failures.append((NAMES[e],mode,seed,label,metric,m[metric],floor))
                        if label == 'raw' and m['best_shift'] != [0,0]:
                            failures.append((NAMES[e],mode,seed,'raw','integer shift',m['best_shift']))
                print(NAMES[e], mode, seed, 'overlap/exact',
                      ' '.join(f"{r['scale']}:{r['classic_overlap']:.1f}/{r['exact_colour']:.1f}" for r in records[-len(sizes):]),flush=True)
                (args.scratch/'pixels.json').write_text(json.dumps(records,indent=2)+'\n')
    if args.check:
        for e in effects:
            for mode in modes:
                assert any(r['effect']==NAMES[e] and r['mode']==mode and r['instance']==1 for r in records), (NAMES[e],mode,'no Native instance')
    assert not failures, failures


def gallery(args):
    manifest = []; items = []
    for e in (sorted(NATIVE) if not args.effects else map(int,args.effects.split(','))):
        name = NAMES[e]; folder = args.scratch/name; folder.mkdir(exist_ok=True)
        seed = 3 if e == 33 else 1
        reference = run(args,e,0,'classic','original',320,400,seed=seed)[2]
        indexed_png(folder/'classic-grid.png',reference['idx'],320,400,palette=reference['pal'])
        for number, policy, style in [(1,'original','faithful'),(2,'original','native'),
                                       (3,'extended','faithful'),(4,'extended','native'),(5,'stretch','native')]:
            a = run(args,e,0,style,policy,3840,2160,seed=seed)[2]
            assert a['state'] == reference['state'], (name,policy,style,'state')
            file = folder/f'{number}-{policy}-{style}.png'
            indexed_png(file,a['idx'],3840,2160,palette=a['pal'])
            if policy == 'extended':
                left,_,right,_ = edges(a['rect'])
                indexed_png(folder/f'{number}-{policy}-{style}-edges.png',a['idx'],3840,2160,(left,right-1),a['pal'])
            if number == 2:
                reduced = reduce_grid(a['idx'],3840,2160,320,400,a['rect'])
                indexed_png(folder/'native-grid.png',reduced,320,400,palette=a['pal'])
                differences(folder/'difference.png',reference['idx'],reduced,320,400)
            manifest.append(dict(effect=name,seed=seed,ns=a['ns'],policy=policy,style=style,size=[3840,2160],
                state_sha256=hashlib.sha256(a['state']).hexdigest()))
        for style, policy in [('faithful','original'),('native','original'),('native','stretch'),('native','extended')]:
            a = run(args,e,0,style,policy,1920,1080,seed=seed)[2]
            assert a['state'] == reference['state'], (name,'1080',policy,style,'state')
            file = folder/f'1080-{policy}-{style}.png'
            indexed_png(file,a['idx'],1920,1080,palette=a['pal'])
            if policy == 'extended':
                left,_,right,_ = edges(a['rect'])
                indexed_png(folder/f'1080-{policy}-{style}-edges.png',a['idx'],1920,1080,(left,right-1),a['pal'])
            manifest.append(dict(effect=name,seed=seed,ns=a['ns'],policy=policy,style=style,size=[1920,1080],
                state_sha256=hashlib.sha256(a['state']).hexdigest()))
        items.append(f'<h2>{name}</h2>'+''.join(f'<a href="{name}/{p.name}">{p.stem}</a><br>' for p in sorted(folder.glob('*.png'))))
        print('gallery',name,flush=True)
    (args.scratch/'index.html').write_text('<!doctype html><meta charset="utf-8"><title>Native comparison</title>'
        '<style>body{background:#151515;color:#ddd;font:16px system-ui}a{color:#8cf}</style>'
        '<h1>Original Faithful / Native; Extended Faithful / Native, 1080p and 4K</h1>'
        '<p>Same seed and Auto boundary per effect. Extended has marked and unmarked versions. '
        'Difference: red Classic only, cyan Native only, yellow shared ink with different indices.</p>'+''.join(items))
    (args.scratch/'gallery.json').write_text(json.dumps(manifest,indent=2)+'\n')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('exe', type=Path)
    p.add_argument('scratch', type=Path)
    p.add_argument('--quick', action='store_true')
    p.add_argument('--seeds', default='1,2,3,4,5,6,7,8')
    p.add_argument('--modes', default='0,1,2')
    p.add_argument('--effects')
    p.add_argument('--scales')
    p.add_argument('--images', action='store_true')
    p.add_argument('--distances', action='store_true')
    p.add_argument('--centres', action='store_true')
    p.add_argument('--pixels', action='store_true')
    p.add_argument('--gallery', action='store_true')
    p.add_argument('--check', action='store_true')
    args = p.parse_args()
    args.exe = args.exe.resolve(); args.scratch = args.scratch.resolve()
    args.scratch.mkdir(parents=True, exist_ok=True)
    if args.centres: center_checks(args)
    if args.pixels: pixel_checks(args)
    if args.gallery: gallery(args)


if __name__ == '__main__':
    main()
