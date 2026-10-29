"""Strict single-function OMF verifier (16-bit Intel OMF, fail closed).

python tools/objcmp.py SRC.C FUNC SSSS:OOOO [--profile NAME] [--fresh]

Target size and bindings come from --map JSON, evidence/bindings/<source>.json (a list of
entries such as {"function":"f", "address":"066a:026b", "size":80, "symbols":{"_x":"DS:5df0"}}),
or, for new probes, the same object in a source comment /* objcmp: {...} */.
Size is an independently established *complete* extent, including switch tables.
Without it the result is UNKNOWN, never a prefix match. --size is also accepted.
An OBJ can be supplied instead of C. --dump shows its records/bytes/fixups.
"""
import argparse
from dataclasses import dataclass, asdict
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
META_RE = re.compile(r'/\*\s*objcmp:\s*(\{.*?\})\s*\*/', re.S)
BINDINGS = ROOT / 'evidence/bindings'


def metadata_entries(source):
    """Verifier metadata of SOURCE, in source order.

    Read from evidence/bindings/<file name>.json when present, otherwise from
    /* objcmp: {...} */ comments in the source. Both at once is an error.
    """
    source = Path(source)
    inline = [json.loads(m) for m in META_RE.findall(source.read_text(encoding='utf-8', errors='replace'))]
    sidecar = BINDINGS / (source.name.lower() + '.json')
    if sidecar.exists():
        if inline:
            raise ValueError(f'{source}: metadata both inline and in {sidecar}')
        return json.loads(sidecar.read_text(encoding='utf-8'))
    return inline


EMU_ABSOLUTE = {'FIARQQ': 0xFE32, 'FICRQQ': 0x0E32, 'FIDRQQ': 0x5C32, 'FIERQQ': 0x1632,
                'FISRQQ': 0x0632, 'FIWRQQ': 0xA23D, 'FJARQQ': 0x4000, 'FJCRQQ': 0xC000,
                'FJSRQQ': 0x8000}


class OMFError(ValueError):
    pass


class Reader:
    def __init__(self, data):
        self.data, self.pos = data, 0

    def take(self, count):
        if count < 0 or self.pos + count > len(self.data):
            raise OMFError('truncated OMF field')
        result = self.data[self.pos:self.pos + count]
        self.pos += count
        return result

    def byte(self):
        return self.take(1)[0]

    def word(self):
        return int.from_bytes(self.take(2), 'little')

    def index(self):
        first = self.byte()
        return ((first & 127) << 8 | self.byte()) if first & 128 else first

    def name(self):
        return self.take(self.byte()).decode('ascii', errors='strict')

    def __bool__(self):
        return self.pos < len(self.data)


@dataclass
class Fixup:
    segment: int
    offset: int
    location: int
    relative: bool
    frame: tuple
    target: tuple
    displacement: int


class Object:
    def __init__(self, data):
        self.names = ['']
        self.segments = [None]
        self.groups = [None]
        self.externals = ['']
        self.publics = {}
        self.absolute_publics = {}
        self.commons = {}
        self.fixups = []
        self.records = []
        self.comments = []
        threads = [{}, {}]  # target, frame
        last = None
        pos = 0
        ended = False
        while pos < len(data):
            if ended or pos + 3 > len(data):
                raise OMFError('data after MODEND or truncated record')
            typ, length = data[pos], int.from_bytes(data[pos + 1:pos + 3], 'little')
            record = data[pos:pos + 3 + length]
            if not length or len(record) != 3 + length or sum(record) & 255:
                raise OMFError(f'bad OMF length/checksum at {pos:#x}')
            r = Reader(record[3:-1])
            self.records.append((typ, length))
            pos += len(record)
            if typ in (0x80, 0x82):
                r.name()
            elif typ == 0x88:
                self.comments.append(r.take(len(r.data)))
            elif typ == 0x96:
                while r:
                    self.names.append(r.name())
            elif typ == 0x98:
                attr = r.byte()
                if attr & 1 or (attr >> 5) == 0:
                    raise OMFError('32-bit/absolute SEGDEF unsupported')
                length = r.word()
                if attr & 2:
                    length = 65536
                name, cls, overlay = r.index(), r.index(), r.index()
                self.segments.append({'name': self.names[name], 'class': self.names[cls],
                                      'length': length, 'data': bytearray(length),
                                      'present': bytearray(length)})
            elif typ == 0x9a:
                name, members = self.names[r.index()], []
                while r:
                    if r.byte() != 255:
                        raise OMFError('unsupported GRPDEF component')
                    members.append(r.index())
                self.groups.append((name, members))
            elif typ in (0x8c, 0xb4):
                while r:
                    self.externals.append(r.name())
                    r.index()
            elif typ in (0xb0, 0xb8):
                def numeric():
                    leaf = r.byte()
                    if leaf <= 128:
                        return leaf
                    if leaf not in (129, 132, 136):
                        raise OMFError('unsupported communal numeric leaf')
                    return int.from_bytes(r.take({129: 2, 132: 3, 136: 4}[leaf]), 'little')
                while r:
                    name = r.name()
                    r.index()
                    kind = r.byte()
                    if kind not in (0x61, 0x62):
                        raise OMFError('unsupported COMDEF kind')
                    length = numeric()
                    if kind == 0x61:
                        length *= numeric()
                    self.externals.append(name)
                    self.commons[name] = length
            elif typ in (0x90, 0xb6):
                group, segment = r.index(), r.index()
                frame = r.word() if not segment else None
                while r:
                    name, offset = r.name(), r.word()
                    r.index()
                    if name in self.publics or name in self.absolute_publics:
                        raise OMFError('duplicate PUBDEF ' + name)
                    if segment:
                        self.publics[name] = (segment, offset, group)
                    else:
                        self.absolute_publics[name] = frame * 16 + offset
            elif typ in (0xa0, 0xa2):
                segment, offset = r.index(), r.word()
                payload = r.take(len(r.data) - r.pos)
                if typ == 0xa2:
                    contents, positions = self.iterated(payload)
                else:
                    contents, positions = payload, None
                seg = self.segments[segment]
                end = offset + len(contents)
                if end > seg['length'] or any(seg['present'][offset:end]):
                    raise OMFError('LEDATA/LIDATA out of bounds or overlapping')
                seg['data'][offset:end] = contents
                seg['present'][offset:end] = bytes([1]) * len(contents)
                last = (segment, offset, len(contents), positions)
            elif typ == 0x9c:
                while r:
                    first = r.byte()
                    if first & 128:
                        location = (first >> 2) & 15
                        if location not in (0, 1, 2, 3, 4):
                            raise OMFError(f'unsupported FIXUPP location {location}')
                        off = ((first & 3) << 8) | r.byte()
                        fd = r.byte()
                        if fd & 128:
                            frame = threads[1][(fd >> 4) & 3]
                        else:
                            method = (fd >> 4) & 7
                            if method > 5:
                                raise OMFError('unsupported frame method')
                            frame = (method, r.word() if method == 3 else
                                     r.index() if method < 3 else None)
                        if fd & 8:
                            target = threads[0][fd & 3]
                        else:
                            method = fd & 3
                            target = (method, r.word() if method == 3 else r.index())
                        displacement = 0 if fd & 4 else r.word()
                        if last is None:
                            raise OMFError('FIXUPP without data')
                        segment, base, count, positions = last
                        width = {0: 1, 1: 2, 2: 2, 3: 4, 4: 1}[location]
                        if positions is None:
                            offsets = [off]
                        else:
                            if not first & 64:
                                raise OMFError('self-relative LIDATA fixup unsupported')
                            offsets = positions.get(off, [])
                            if not offsets or any(positions.get(off + j) != [x + j for x in offsets]
                                                  for j in range(width)):
                                raise OMFError('LIDATA fixup is not a complete repeated literal')
                        for off in offsets:
                            if off + width > count:
                                raise OMFError('FIXUPP out of data bounds')
                            self.fixups.append(Fixup(segment, base + off, location,
                                                     not bool(first & 64), frame, target, displacement))
                    else:
                        is_frame = (first >> 6) & 1
                        method = (first >> 2) & (7 if is_frame else 3)
                        if first & 32 or method > (5 if is_frame else 3):
                            raise OMFError('unsupported FIXUPP thread')
                        datum = r.word() if method == 3 else r.index() if method < 3 else None
                        threads[is_frame][first & 3] = (method, datum)
            elif typ == 0x8a:
                r.take(len(r.data))
                ended = True
            elif typ in (0x94, 0x8e):  # line/type information has no load bytes
                r.take(len(r.data))
            else:
                raise OMFError(f'unsupported OMF record {typ:#x}')
            if r:
                raise OMFError(f'unconsumed record {typ:#x}')
        if not ended:
            raise OMFError('missing MODEND')

    @staticmethod
    def iterated(payload):
        """Expand 16-bit LIDATA and map encoded literal positions to all copies."""
        r = Reader(payload)

        def block(depth=0):
            if depth > 32:
                raise OMFError('LIDATA nesting limit')
            repeat, children = r.word(), r.word()
            data, mapping = bytearray(), {}
            if children:
                for _ in range(children):
                    part, locs = block(depth + 1)
                    for key, offsets in locs.items():
                        mapping[key] = [len(data) + x for x in offsets]
                    data.extend(part)
                    if len(data) > 65536:
                        raise OMFError('LIDATA expansion limit')
            else:
                count = r.byte()
                start = r.pos
                data.extend(r.take(count))
                mapping = {start + j: [j] for j in range(count)}
            if len(data) * repeat > 65536:
                raise OMFError('LIDATA expansion limit')
            return data * repeat, {key: [j * len(data) + x for j in range(repeat) for x in offsets]
                                   for key, offsets in mapping.items()}

        result, positions = bytearray(), {}
        while r:
            part, locs = block()
            for key, offsets in locs.items():
                positions[key] = [len(result) + x for x in offsets]
            result.extend(part)
            if len(result) > 65536:
                raise OMFError('LIDATA expansion limit')
        return result, positions

    def function(self, name):
        if name not in self.publics:
            name = '_' + name
        if name not in self.publics:
            raise OMFError('no PUBDEF for function ' + name)
        segment, start, _ = self.publics[name]
        seg = self.segments[segment]
        if seg['class'].upper() != 'CODE':
            raise OMFError('function is not in CODE')
        end = min([p[1] for p in self.publics.values() if p[0] == segment and p[1] > start]
                  + [seg['length']])
        if end <= start or not all(seg['present'][start:end]):
            raise OMFError('empty/incomplete function extent')
        return segment, start, end


def address(text):
    seg, off = text.split(':')
    result = (0x29bf if seg.upper() == 'DS' else int(seg, 16), int(off, 16))
    if not all(0 <= value <= 65535 for value in result):
        raise ValueError('16-bit segment:offset required')
    return result


def number(value):
    return int(value, 0) if isinstance(value, str) else int(value)


def check_known_bindings(mapping, inventory):
    """Reject a known RTL symbol assigned to a different routine/entry point.

    Supplied maps remain necessary for unnamed application/data symbols. The
    read-only RTL inventory is an additional guard, not a source of guessed
    bindings or function extents.
    """
    known = {}
    for row in inventory.get('functions', []):
        if row.get('class') == 'runtime':
            for name in row.get('name', '').split('|'):
                known.setdefault(name, set()).add(row['linear'])
    for name, value in mapping.get('symbols', {}).items():
        if name in known:
            seg, off = address(value)
            if seg * 16 + off not in known[name]:
                raise ValueError('binding contradicts evidence/functions.json: ' + name)


def near_call_sites(raw, origin):
    """Offsets of E8 rel16 instructions found by linear disassembly of raw."""
    try:
        import capstone
    except ImportError:
        sys.path.insert(0, 'C:/tools/capstone-5.0.3')
        import capstone
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    return [ins.address - origin for ins in md.disasm(raw, origin)
            if ins.bytes[0] == 0xE8 and ins.size == 3]


def compare(obj, name, addr, size, image, relocations, mapping, extent=None):
    # Explicit full-segment extents also support initialized data. Callers must
    # establish the bounds independently and check emitted-byte coverage.
    segment, start, end = obj.function(name) if extent is None else extent
    seg, off = addr
    base = seg * 16 + off
    raw = bytes(obj.segments[segment]['data'][start:end])
    if len(raw) != size:
        # Resolve a diagnostic copy only; the independently sized target still
        # fails unconditionally. This result can never promote a prefix match.
        diagnostic, resolved = compare(obj, name, addr, len(raw), image, relocations, mapping,
                                       extent=extent)
        expected = image[base:base + size]
        first = next((i for i, pair in enumerate(zip(resolved, expected)) if pair[0] != pair[1]),
                     min(len(raw), size))
        return {'status': 'mismatch', 'reason': f'full extent: object {len(raw)}, oracle {size}',
                'object_size': len(raw), 'target_size': size, 'offset': first,
                'diagnostic_status': diagnostic['status']}, resolved
    if base < 0 or base + size > len(image) or off + size > 65536:
        raise ValueError('target extent outside load image/segment')
    placements = dict(mapping.get('segments', {}))
    # Placement of the selected function is a user-supplied binding. This also
    # binds its own local labels and CS switch table, never other data segments.
    if off < start:
        raise ValueError('function placement implies negative segment contribution')
    inferred = (seg, off - start)
    own_name = obj.segments[segment]['name']
    if own_name in placements and address(placements[own_name]) != inferred:
        raise ValueError('conflicting code segment placement')
    placements[own_name] = f'{seg:04x}:{off-start:04x}'
    groups = {'DGROUP': '29bf:0000', **mapping.get('groups', {})}
    symbols = mapping.get('symbols', {})
    # Absolute PUBDEFs of BC++ 3.0 XLIB/EMU.LIB (8087 emulator fixup constants);
    # explicit metadata bindings override these toolchain defaults.
    absolute_symbols = {**EMU_ABSOLUTE, **mapping.get('absolute_symbols', {})}
    if set(symbols) & set(mapping.get('absolute_symbols', {})):
        raise ValueError('conflicting absolute/address symbol binding')

    def binding(kind, index):
        if kind == 0:
            key = obj.segments[index]['name']
            value = placements.get(key)
        elif kind == 1:
            key = obj.groups[index][0]
            value = groups.get(key)
        elif kind == 2:
            key = obj.externals[index]
            value = symbols.get(key)
        elif kind == 3:
            return index, 0
        else:
            raise ValueError('invalid binding method')
        if value is None:
            raise ValueError('unbound ' + key)
        result = address(value)
        if kind == 1 and result[1]:
            raise ValueError('group binding must have offset zero')
        return result

    publics_at = {(p[0], p[1]): n for n, p in obj.publics.items()}
    known = {**symbols, **mapping.get('functions', {})}
    resolved = bytearray(raw)
    touched = set()
    absolute_touched = set()
    segment_sites = set()
    unbound, details, site_errors = [], [], []
    for fix in obj.fixups:
        if fix.segment != segment:
            continue
        width = {0: 1, 1: 2, 2: 2, 3: 4, 4: 1}[fix.location]
        if fix.offset + width <= start or fix.offset >= end:
            continue
        at = fix.offset - start
        is_absolute = (fix.target[0] == 2 and obj.externals[fix.target[1]] in absolute_symbols
                       and fix.location == 1 and not fix.relative)
        overlap = touched.intersection(range(at, at + width))
        # The 8087 emulator's segment-override forms carry two OFFSET16 fixups
        # to absolute symbols whose words overlap (e.g. FISRQQ at k, FJSRQQ at
        # k+1 over 9B 36 D9). The linker adds them in record order; that is the
        # only overlap accepted, and the result is still compared strictly.
        if at < 0 or at + width > size or (overlap and not (is_absolute and overlap <= absolute_touched)):
            raise ValueError('overlapping fixups or fixup crossing function extent')
        touched.update(range(at, at + width))
        if is_absolute:
            absolute_touched.update(range(at, at + width))
        if fix.location in (0, 4):
            raise ValueError('byte/high-byte fixups parsed but not verifiable (carry ambiguous)')
        rewrite_at = None
        if fix.location == 3 and not fix.relative:
            call_at = at - 1
            image_call = base + call_at
            if (call_at >= 0 and call_at + 5 <= size and
                    image[image_call:image_call + 3] == b'\x90\x0e\xe8'):
                if at != call_at + 1 or raw[call_at] != 0x9a:
                    return {'status': 'mismatch',
                            'reason': 'TLINK far-call rewrite lacks 9A plus adjacent far-pointer fixup',
                            'offset': max(call_at, 0)}, bytes(resolved)
                object_call = start + call_at
                if any(other is not fix and other.segment == segment and
                       other.offset < object_call + 5 and
                       other.offset + {0: 1, 1: 2, 2: 2, 3: 4, 4: 1}[other.location] > object_call
                       for other in obj.fixups):
                    return {'status': 'mismatch',
                            'reason': 'overlapping object fixup at TLINK far-call rewrite',
                            'offset': call_at}, bytes(resolved)
                if any(reloc in relocations for reloc in range(image_call, image_call + 5)):
                    return {'status': 'mismatch',
                            'reason': 'TLINK far-call rewrite has an MZ relocation at the call site',
                            'offset': call_at}, bytes(resolved)
                rewrite_at = call_at
        relocation_at = base + at + (2 if fix.location == 3 else 0)
        if fix.location in (2, 3) and rewrite_at is None:
            segment_sites.add(relocation_at)
            if relocation_at not in relocations:
                site_errors.append(at)
        if fix.target[0] == 2 and obj.externals[fix.target[1]] in absolute_symbols:
            key = obj.externals[fix.target[1]]
            if fix.relative or fix.location != 1:
                raise ValueError('absolute external requires segment-relative OFFSET16')
            # OMF absolute PUBDEFs have no relocatable frame to subtract.
            # Example: EMU.LIB FIDRQQ=5c32 adds to 9b d9 -> cd 35.
            value = number(absolute_symbols[key])
            # resolved == raw here unless an earlier overlapping absolute fixup applied.
            addend = int.from_bytes(resolved[at:at + 2], 'little')
            resolved[at:at + 2] = ((value + fix.displacement + addend) & 65535).to_bytes(2, 'little')
            details.append({'offset': at, 'absolute_symbol': key, 'value': value,
                            'fixup': asdict(fix)})
            continue
        # A reference into this module's own code segment (e.g. a far call to a
        # function defined LATER in the file) is resolved through the object
        # layout, which is only right once every preceding function has its
        # exact size. When it lands exactly on a public of this segment outside
        # the function and that public's original address is independently
        # known (symbols / --all metadata), bind it to that address instead.
        rebind = None
        if fix.target[0] == 0 and fix.target[1] == segment and fix.location in (1, 3):
            obj_target = (fix.displacement + int.from_bytes(raw[at:at + 2], 'little')) & 0xFFFF
            callee = publics_at.get((segment, obj_target))
            if callee in known and not start <= obj_target < end:
                rebind = (callee, address(known[callee]))
        try:
            if rebind:
                target_seg, target_off = rebind[1]
            else:
                target_seg, target_off = binding(*fix.target)
            fm, fi = fix.frame
            if fm == 4:
                frame_seg = seg
            elif fm == 5:
                frame_seg = target_seg
            else:
                frame_seg, _ = binding(fm, fi)
            target = target_seg * 16 + target_off + (0 if rebind else fix.displacement)
            if rebind:
                details.append({'offset': at, 'own_segment_rebind': rebind[0],
                                'target': known[rebind[0]]})
            if rewrite_at is not None:
                offset_addend = 0 if rebind else int.from_bytes(raw[at:at + 2], 'little')
                segment_addend = int.from_bytes(raw[at + 2:at + 4], 'little')
                resolved_off = target - frame_seg * 16 + offset_addend
                resolved_seg = (frame_seg + segment_addend) & 65535
                call_off = off + rewrite_at
                rel16 = int.from_bytes(image[base + rewrite_at + 3:
                                             base + rewrite_at + 5], 'little', signed=True)
                near_target = (call_off + 5 + rel16) & 65535
                if (target_seg != seg or frame_seg != seg or resolved_seg != seg or
                        not 0 <= resolved_off < 65536 or resolved_off != near_target):
                    return {'status': 'mismatch',
                            'reason': 'TLINK far-call rewrite target is not the same-segment near target',
                            'offset': rewrite_at,
                            'target': f'{target_seg:04x}:{target_off:04x}',
                            'frame': f'{frame_seg:04x}', 'near_target': f'{near_target:04x}'}, bytes(resolved)
                linked = image[base + rewrite_at:base + rewrite_at + 5]
                resolved[rewrite_at:rewrite_at + 5] = linked
                details.append({'offset': rewrite_at, 'target': f'{seg:04x}:{resolved_off:04x}',
                                'rewrite': 'TLINK same-segment far call to near call',
                                'fixup': asdict(fix)})
                continue
            if fix.relative:
                if fix.location != 1:
                    raise ValueError('unsupported self-relative fixup type')
                if not 0 <= target - seg * 16 < 65536:
                    raise ValueError('near target is outside the placed CS window')
                value = target - (base + at + 2)
            else:
                value = target - frame_seg * 16
                if fix.location in (1, 3) and not 0 <= value < 65536:
                    raise ValueError('offset target is outside its bound frame')
            addend = 0 if rebind else int.from_bytes(raw[at:at + 2], 'little')
            if fix.location == 2:
                # BASE16 relocates the frame, not the target offset.
                value = frame_seg + addend
                resolved[at:at + 2] = (value & 65535).to_bytes(2, 'little')
            else:
                resolved[at:at + 2] = ((value + addend) & 65535).to_bytes(2, 'little')
                if fix.location == 3:
                    seg_addend = int.from_bytes(raw[at + 2:at + 4], 'little')
                    resolved[at + 2:at + 4] = ((frame_seg + seg_addend) & 65535).to_bytes(2, 'little')
            details.append({'offset': at, 'target': f'{target_seg:04x}:{target_off:04x}',
                            'frame': f'{frame_seg:04x}', 'fixup': asdict(fix)})
        except ValueError as exc:
            if str(exc).startswith('unbound '):
                unbound.append(str(exc))
            else:
                return {'status': 'mismatch', 'reason': str(exc), 'offset': at}, bytes(resolved)
    # Intra-object near calls (E8 rel16, no fixup): BCC resolves calls to functions
    # defined earlier in the same file itself, so the displacement depends on the
    # object layout (stubs, earlier mismatched functions). Rebind such a call only
    # when it lands exactly on a public of this code segment whose original
    # address is independently known (symbols / --all metadata); the image
    # displacement must then reach that address. Anything else stays raw.
    for at in (near_call_sites(raw, start)
               if obj.segments[segment]['class'].upper() == 'CODE' else []):
        if touched.intersection(range(at, at + 3)):
            continue
        rel = int.from_bytes(raw[at + 1:at + 3], 'little')
        target_obj = (start + at + 3 + rel) & 0xFFFF
        if start <= target_obj < end:
            continue
        callee = publics_at.get((segment, target_obj))
        if callee is None or callee not in known:
            continue
        cseg, coff = address(known[callee])
        if cseg != seg:
            return {'status': 'mismatch', 'offset': at,
                    'reason': f'near call to {callee} bound outside this segment'}, bytes(resolved)
        resolved[at + 1:at + 3] = ((coff - (off + at + 3)) & 0xFFFF).to_bytes(2, 'little')
        details.append({'offset': at, 'intra_object_call': callee, 'target': known[callee]})
    actual_sites = {x for x in relocations if base - 1 <= x < base + size}
    if actual_sites != segment_sites:
        return {'status': 'mismatch', 'reason': 'MZ relocation coverage differs',
                'offset': min(site_errors) if site_errors else 0,
                'missing_object_sites': sorted(actual_sites - segment_sites),
                'extra_object_sites': sorted(segment_sites - actual_sites)}, bytes(resolved)
    # Unbound fields are never masked and never accepted, even if raw bytes equal.
    if unbound:
        return {'status': 'unbound', 'reason': ', '.join(sorted(set(unbound)))}, bytes(resolved)
    expected = image[base:base + size]
    differences = [i for i, (a, b) in enumerate(zip(resolved, expected)) if a != b]
    if differences:
        at = differences[0]
        return {'status': 'mismatch', 'offset': at, 'reason': f'byte +{at:#x}: '
                f'object {resolved[at]:02x}, image {expected[at]:02x}'}, bytes(resolved)
    return {'status': 'EXACT' if raw == expected else 'RELOCATED', 'size': size,
            'fixups': details, 'proof': 'function extent only'}, bytes(resolved)


def disassembly_pair(candidate, expected, offset, origin):
    try:
        try:
            import capstone
        except ImportError:
            sys.path.insert(0, 'C:/tools/capstone-5.0.3')
            import capstone
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
        def rows(data):
            result = []
            for ins in md.disasm(data, origin):
                local = ins.address - origin
                if local <= offset < local + ins.size or (result and local < offset + 12):
                    result.append(f'{ins.address:04x} {ins.bytes.hex(" "):<24} '
                                  f'{ins.mnemonic} {ins.op_str}')
                if local > offset + 12:
                    break
            return result or ['<end or undecodable data>']
        left, right = rows(candidate), rows(expected)
        return '\n'.join(f'{a:<56} | {b}' for a, b in
                         __import__('itertools').zip_longest(left, right, fillvalue=''))
    except ImportError:
        return 'Capstone unavailable; install capstone for instruction diagnostics.'


def load_oracle():
    oracle = json.loads((ROOT / 'evidence/oracle.json').read_text())
    image = (ROOT / 'reference/load_image.bin').read_bytes()
    if hashlib.sha256(image).hexdigest() != oracle['load_image']['sha256']:
        raise ValueError('load image SHA256 differs from oracle')
    relocations = {r['segment'] * 16 + r['offset']
                   for r in oracle['relocations']['ordered_segment_offset']}
    if len(relocations) != len(oracle['relocations']['ordered_segment_offset']):
        raise ValueError('duplicate MZ relocations need explicit loader treatment')
    if image[:3] != bytes.fromhex('ba bf 29') or 1 not in relocations:
        raise ValueError('DGROUP default not supported by startup oracle')
    return image, relocations


def check_all(args, obj, path):
    """Whole-TU mode: every metadata comment naming a function is one target.

    Symbol bindings of all entries are merged (a conflicting binding is an
    error), so each function sees the file's complete binding set.
    """
    entries = metadata_entries(args.source)
    entries = [e for e in entries if 'function' in e]
    if not entries:
        raise ValueError('no objcmp metadata with "function" in source')
    merged = {'symbols': {}, 'segments': {}, 'groups': {}, 'absolute_symbols': {}}
    for e in entries:
        for kind, table in merged.items():
            for k, v in e.get(kind, {}).items():
                if str(table.setdefault(k, v)).lower() != str(v).lower():
                    raise ValueError(f'conflicting {kind} binding for {k}: {table[k]} vs {v}')
    image, relocations = load_oracle()
    inventory = ROOT / 'evidence/functions.json'
    known = json.loads(inventory.read_text()) if inventory.exists() else None
    results, ok = [], 0
    functions = {'_' + e['function']: e['address'] for e in entries}
    for e in entries:
        mapping = {**e, **{k: v for k, v in merged.items() if v}, 'functions': functions}
        try:
            if known is not None:
                check_known_bindings(mapping, known)
            result, _ = compare(obj, e['function'], address(e['address']),
                                number(e['size']), image, relocations, mapping)
        except (ValueError, KeyError, IndexError) as exc:
            result = {'status': 'UNKNOWN', 'reason': str(exc)}
        result = {'function': e['function'], 'address': e['address'],
                  'size': number(e['size']), 'status': result['status'],
                  'reason': result.get('reason', ''), 'offset': result.get('offset')}
        ok += result['status'] in ('EXACT', 'RELOCATED')
        results.append(result)
        print(f"{result['status']:<9} {e['address']} {e['function']:<28} {result['size']:>5}"
              + (f"  {result['reason']}" if result['status'] not in ('EXACT', 'RELOCATED') else ''))
    print(f'{ok}/{len(results)} matched ({sum(r["size"] for r in results if r["status"] in ("EXACT", "RELOCATED"))} bytes)')
    if args.out:
        args.out.write_text(json.dumps({'source': str(args.source).replace('\\', '/'),
                                        'profile': args.profile, 'object': str(path),
                                        'results': results}, indent=1) + '\n', encoding='utf-8', newline='\n')
    raise SystemExit(0 if ok == len(results) else 1)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('source', type=Path)
    ap.add_argument('function', nargs='?')
    ap.add_argument('address', nargs='?')
    ap.add_argument('--profile', default='dazzle')
    ap.add_argument('--flag', action='append')
    ap.add_argument('--map', type=Path)
    ap.add_argument('--size', type=lambda s: int(s, 0))
    ap.add_argument('--fresh', action='store_true')
    ap.add_argument('--header', action='append', type=Path, default=[])
    ap.add_argument('--dump', action='store_true')
    ap.add_argument('--json', action='store_true')
    ap.add_argument('--all', action='store_true',
                    help='compile once, check every /* objcmp: {"function":..} */ entry in the file')
    ap.add_argument('--out', type=Path, help='with --all: write per-function results as JSON')
    ap.add_argument('--prelude', type=Path,
                    help='C text compiled before SOURCE (stubs for same-segment functions owned elsewhere)')
    args = ap.parse_args()
    try:
        mapping = json.loads(args.map.read_text()) if args.map else {}
        if args.source.suffix.lower() == '.obj':
            path = args.source
        else:
            entries = metadata_entries(args.source)
            entry = next((e for e in entries if e.get('function') == args.function), entries[0] if entries else None)
            if entry:
                mapping = {**entry, **mapping}
            sys.path.insert(0, str(Path(__file__).resolve().parent))
            sys.dont_write_bytecode = True
            from bcc import compile_source
            headers = {p.name: p.read_bytes() for p in args.header}
            if len(headers) != len(args.header):
                raise ValueError('duplicate local header basename')
            # Project headers (include/*.h) are always staged for #include "dazzle.h".
            for h in sorted((ROOT / 'include').glob('*.h')):
                headers.setdefault(h.name.upper(), h.read_bytes())
            source = args.source
            if args.prelude:
                # Prelude text (e.g. stub definitions of earlier functions of the
                # same segment) is compiled ahead of the source; metadata is still
                # read from the source only.
                text = args.prelude.read_text() + '\n' + args.source.read_text()
                tmp = ROOT / 'build/objcmp_prelude' / hashlib.sha256(text.encode()).hexdigest()[:16]
                tmp.mkdir(parents=True, exist_ok=True)
                source = tmp / args.source.name
                source.write_text(text)
            path = compile_source(source, args.profile, args.flag, args.fresh, headers)
        obj = Object(path.read_bytes())
        if args.dump:
            print(json.dumps({'object': str(path), 'records': obj.records,
                              'segments': [{**s, 'data': bytes(s['data']).hex(),
                                            'present': bytes(s['present']).hex()}
                                           for s in obj.segments[1:]],
                              'groups': obj.groups, 'externals': obj.externals,
                              'publics': obj.publics,
                              'absolute_publics': obj.absolute_publics, 'commons': obj.commons,
                              'fixups': [asdict(f) for f in obj.fixups]}, indent=2))
            if not args.function:
                return
        if args.all:
            return check_all(args, obj, path)
        if not args.function or not args.address:
            raise ValueError('function and address are required')
        addr = address(args.address)
        if 'address' in mapping and address(mapping['address']) != addr:
            raise ValueError('metadata extent belongs to a different target address')
        size = args.size if args.size is not None else mapping.get('size')
        if size is None:
            print('UNKNOWN: independently established target --size / metadata required')
            raise SystemExit(2)
        size = number(size)
        if size <= 0:
            raise ValueError('positive target size required')
        oracle = json.loads((ROOT / 'evidence/oracle.json').read_text())
        image = (ROOT / 'reference/load_image.bin').read_bytes()
        if hashlib.sha256(image).hexdigest() != oracle['load_image']['sha256']:
            raise ValueError('load image SHA256 differs from oracle')
        relocations = {r['segment'] * 16 + r['offset']
                       for r in oracle['relocations']['ordered_segment_offset']}
        if len(relocations) != len(oracle['relocations']['ordered_segment_offset']):
            raise ValueError('duplicate MZ relocations need explicit loader treatment')
        if image[:3] != bytes.fromhex('ba bf 29') or 1 not in relocations:
            raise ValueError('DGROUP default not supported by startup oracle')
        inventory = ROOT / 'evidence/functions.json'
        if inventory.exists():
            check_known_bindings(mapping, json.loads(inventory.read_text()))
        result, code = compare(obj, args.function, addr, size, image, relocations, mapping)
        result.update({'object': str(path), 'function': args.function,
                       'address': args.address, 'profile': args.profile})
        if args.json:
            print(json.dumps(result, indent=2))
        else:
            print(result['status'] + ': ' + result.get('reason', f'{size} bytes, all fixups bound'))
            if result['status'] == 'mismatch':
                target = addr[0] * 16 + addr[1]
                print('resolved object                                           | oracle')
                print(disassembly_pair(code, image[target:target + size],
                                       result.get('offset', min(len(code), size)), addr[1]))
        raise SystemExit(0 if result['status'] in ('EXACT', 'RELOCATED') else 1)
    except (OSError, ValueError, KeyError, IndexError, RuntimeError, subprocess.TimeoutExpired) as exc:
        ap.exit(2, 'UNKNOWN: ' + str(exc) + '\n')


if __name__ == '__main__':
    main()
