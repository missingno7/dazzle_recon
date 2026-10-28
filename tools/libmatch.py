#!/usr/bin/env python3
"""Inspect Borland OMF libraries and locate member code in the DAZZLE load image.

This is an evidence tool, not a strict binary matcher: OMF FIXUPP bytes and
un-emitted holes are wildcards for extent location. Candidate full-extent hits
are checked against resolvable external near calls and far pointers when both
endpoints are found; unchecked relocations remain provisional evidence.
"""
from __future__ import annotations

import argparse
import json
import pathlib
import struct
from dataclasses import dataclass, field

ROOT = pathlib.Path(__file__).resolve().parent.parent
IMAGE = ROOT / 'reference' / 'load_image.bin'


def omf_records(data: bytes, start: int = 0, limit: int | None = None):
    """Yield (type, payload, file offset, next offset), validating record bounds."""
    pos = start
    end = len(data) if limit is None else min(limit, len(data))
    while pos + 3 <= end:
        typ = data[pos]
        length = int.from_bytes(data[pos + 1:pos + 3], 'little')
        nxt = pos + 3 + length
        if length < 1 or nxt > end:
            break
        yield typ, data[pos + 3:nxt - 1], pos, nxt
        pos = nxt


def pascal(data: bytes, pos: int):
    n = data[pos]
    return data[pos + 1:pos + 1 + n].decode('latin1'), pos + 1 + n


def index(data: bytes, pos: int):
    b = data[pos]
    if b & 0x80:
        return ((b & 0x7f) << 8) | data[pos + 1], pos + 2
    return b, pos + 1


def decode_dictionary(lib: bytes, dict_off: int, blocks: int):
    """Decode the OMF two-level dictionary (37 bucket heads per 512-byte block)."""
    entries = []
    for bi in range(blocks):
        base = dict_off + bi * 512
        block = lib[base:base + 512]
        if len(block) != 512:
            break
        for head in block[:37]:
            if not head:
                continue
            p = 2 * head
            if p >= 512:
                continue
            n = block[p]
            if not n or p + 1 + n + 2 > 512:
                continue
            name = block[p + 1:p + 1 + n].decode('latin1')
            page = int.from_bytes(block[p + 1 + n:p + 3 + n], 'little')
            entries.append((name, page))
    return entries


def library_members(path: pathlib.Path):
    data = path.read_bytes()
    if not data or data[0] != 0xf0:
        raise ValueError(f'{path}: not an OMF library (expected LIBHDR F0)')
    rec_len = int.from_bytes(data[1:3], 'little')
    page_size = rec_len + 3
    dict_off = int.from_bytes(data[3:7], 'little')
    dict_blocks = int.from_bytes(data[7:9], 'little')
    dict_entries = decode_dictionary(data, dict_off, dict_blocks)
    # The directory is authoritative for names and page locations. The module
    # stream itself is read from every page between the header and LIBEND.
    modules = {}
    pos = page_size
    while pos < dict_off:
        if data[pos] == 0:
            pos += 1
            continue
        if data[pos] == 0xf1:
            break
        recs = list(omf_records(data, pos, dict_off))
        if not recs:
            break
        module_records = []
        mod_end = None
        for rec in recs:
            module_records.append(rec)
            if rec[0] in (0x8a, 0x8b):
                mod_end = rec[3]
                break
        if mod_end is None:
            # A malformed or nonstandard member: advance to next page.
            pos = ((pos // page_size) + 1) * page_size
            continue
        try:
            _, payload, _, _ = module_records[0]
            name, _ = pascal(payload, 0)
        except (IndexError, UnicodeDecodeError):
            name = f'page_{pos // page_size}'
        modules[pos // page_size] = (name, module_records)
        pos = ((mod_end + page_size - 1) // page_size) * page_size
    by_page = {page: (name, records) for page, (name, records) in modules.items()}
    for name, page in dict_entries:
        if page in by_page:
            by_page[page][0]  # module name sanity, retain LHEADR name
    unresolved_dictionary = [(name, page) for name, page in dict_entries if page not in by_page]
    return {
        'path': str(path), 'page_size': page_size, 'dict_offset': dict_off,
        'dict_blocks': dict_blocks, 'dictionary': dict_entries,
        'modules': modules, 'unresolved_dictionary': unresolved_dictionary,
    }


@dataclass
class Segment:
    index: int
    name: str
    cls: str
    length: int
    data: bytearray = field(default_factory=bytearray)
    mask: bytearray = field(default_factory=bytearray)
    pubdefs: list = field(default_factory=list)
    fixups: list = field(default_factory=list)

    def ensure(self, end: int):
        if end > len(self.data):
            pad = end - len(self.data)
            self.data.extend(b'\0' * pad)
            self.mask.extend(b'\0' * pad)


def _expand_lidata(blob: bytes, pos: int, wide: bool):
    out = bytearray()
    count_size = 4 if wide else 2
    while pos < len(blob):
        if pos + count_size + 2 > len(blob):
            raise ValueError('truncated LIDATA block')
        repeat = int.from_bytes(blob[pos:pos + count_size], 'little')
        children = int.from_bytes(blob[pos + count_size:pos + count_size + 2], 'little')
        pos += count_size + 2
        if children == 0:
            n = blob[pos]
            pos += 1
            literal = blob[pos:pos + n]
            if len(literal) != n:
                raise ValueError('truncated LIDATA literal')
            pos += n
            out.extend(literal * repeat)
        else:
            child = bytearray()
            for _ in range(children):
                sub, pos = _expand_lidata(blob, pos, wide)
                child.extend(sub)
            out.extend(child * repeat)
    return bytes(out), pos


def _fixup_loc_size(loc):
    return {0: 1, 1: 2, 2: 2, 3: 4, 4: 1, 5: 2, 9: 4, 11: 6, 13: 4}.get(loc, 1)


def _skip_index(blob, pos):
    return index(blob, pos)[1]


def parse_fixups(payload: bytes, wide: bool, threads=None):
    """Return the fixup subrecords with their iterated-data relative offsets."""
    out = []
    p = 0
    threads = threads if threads is not None else {}
    thread_frame = threads.setdefault('frame', {})
    thread_target = threads.setdefault('target', {})
    while p < len(payload):
        lead = payload[p]
        p += 1
        if not (lead & 0x80):
            # THREAD subrecord: method, thread number, optional datum.
            method = (lead >> 2) & 7
            is_target = not bool(lead & 0x40)
            number = lead & 3
            datum = None
            if is_target:
                method &= 3  # target thread high method bit comes from FIXDAT.P
            if (is_target and method in (0, 1, 2)) or \
                    (not is_target and method in (0, 1, 2)):
                datum, p = index(payload, p)
            (thread_target if is_target else thread_frame)[number] = (method, datum)
            continue
        if p >= len(payload):
            break
        loc = (lead >> 2) & 0x0f
        # The 10-bit offset is the high two bits in LOCAT's low byte followed
        # by the next byte (an unusual ordering in Intel OMF).
        rel = ((lead & 3) << 8) | payload[p]
        p += 1
        fixdat = payload[p]
        p += 1
        frame_thread = bool(fixdat & 0x80)
        target_thread = bool(fixdat & 0x08)
        no_disp = bool(fixdat & 0x04)
        mode = bool(lead & 0x40)
        frame = None
        target = None
        if frame_thread:
            frame = thread_frame.get((fixdat >> 4) & 3)
        else:
            frame_method = (fixdat >> 4) & 7
            if frame_method in (0, 1, 2):
                frame, p = index(payload, p)
            else:
                frame = (frame_method, None)
        target_method = ((fixdat >> 2) & 1) * 4 + (fixdat & 3)
        if target_thread:
            target = thread_target.get(fixdat & 3)
            target_method = ((target[0] & 3) | (4 if no_disp else 0)) if target else None
            target_index = target[1] if target else None
        else:
            if target_method in (0, 1, 2, 4, 5, 6):
                target_index, p = index(payload, p)
                target = target_index
            else:
                target = None
                target_index = None
        disp = None
        if not no_disp:
            n = 4 if wide else 2
            if p + n > len(payload):
                break
            disp = int.from_bytes(payload[p:p + n], 'little', signed=True)
            p += n
        out.append({'offset': rel, 'loc': loc, 'width': _fixup_loc_size(loc),
                    'mode': 'segment' if mode else 'self_relative',
                    'frame': frame, 'target': target,
                    'target_method': target_method, 'target_index': target_index,
                    'displacement': disp})
    return out


def parse_module(records):
    wide = False
    names = []
    segments = {}
    extdefs = []
    threads = {'frame': {}, 'target': {}}
    current = None
    module_name = ''
    for typ, payload, _, _ in records:
        if typ in (0x80, 0x82):
            module_name, _ = pascal(payload, 0)
        elif typ == 0x96:
            p = 0
            while p < len(payload):
                name, p = pascal(payload, p)
                names.append(name)
        elif typ in (0x98, 0x99):
            wide = bool(typ & 1)
            p = 0
            segno = len(segments) + 1
            while p < len(payload):
                attr = payload[p]
                p += 1
                if wide:
                    length = int.from_bytes(payload[p:p + 4], 'little')
                    p += 4
                else:
                    length = int.from_bytes(payload[p:p + 2], 'little')
                    p += 2
                ni, p = index(payload, p)
                ci, p = index(payload, p)
                oi, p = index(payload, p)
                nm = names[ni - 1] if ni and ni <= len(names) else f'N{ni}'
                cl = names[ci - 1] if ci and ci <= len(names) else f'N{ci}'
                segments[segno] = Segment(segno, nm, cl, length)
                segno += 1
        elif typ in (0x8c, 0x8d):
            p = 0
            while p < len(payload):
                nm, p = pascal(payload, p)
                typeidx, p = index(payload, p)
                extdefs.append(nm)
        elif typ in (0x90, 0x91):
            p = 0
            if len(payload) < 2:
                continue
            grp, p = index(payload, p)
            segidx, p = index(payload, p)
            if segidx == 0:
                if p + 2 > len(payload):
                    continue
                p += 2  # absolute frame number
            while p < len(payload):
                nm, p = pascal(payload, p)
                if typ == 0x91:
                    if p + 4 > len(payload):
                        break
                    off = int.from_bytes(payload[p:p + 4], 'little')
                    p += 4
                else:
                    if p + 2 > len(payload):
                        break
                    off = int.from_bytes(payload[p:p + 2], 'little')
                    p += 2
                if p >= len(payload):
                    break
                typidx, p = index(payload, p)
                if segidx in segments:
                    segments[segidx].pubdefs.append((nm, off))
        elif typ in (0xa0, 0xa1, 0xa2, 0xa3):
            iterated = typ in (0xa2, 0xa3)
            wide = bool(typ & 1)
            p = 0
            segidx, p = index(payload, p)
            off_n = 4 if wide else 2
            off = int.from_bytes(payload[p:p + off_n], 'little')
            p += off_n
            if iterated:
                blob, _ = _expand_lidata(payload, p, wide)
            else:
                blob = payload[p:]
            if segidx in segments:
                seg = segments[segidx]
                seg.ensure(off + len(blob))
                seg.data[off:off + len(blob)] = blob
                seg.mask[off:off + len(blob)] = b'\x01' * len(blob)
                current = (segidx, off, len(blob))
        elif typ in (0x9c, 0x9d):
            wide = bool(typ & 1)
            fixes = parse_fixups(payload, wide, threads)
            if current and current[0] in segments:
                segidx, base, extent = current
                seg = segments[segidx]
                for f in fixes:
                    # FIXUPP offsets refer to current LEDATA/LIDATA payload.
                    f = dict(f, data_offset=base + f['offset'])
                    seg.fixups.append(f)
                    a = base + f['offset']
                    b = min(len(seg.mask), a + f['width'])
                    for k in range(max(a, 0), b):
                        seg.mask[k] = 0
    return module_name, list(segments.values()), extdefs


def _code_segment(seg: Segment):
    s = (seg.name + ' ' + seg.cls).upper()
    return any(x in s for x in ('CODE', 'TEXT', 'C_TEXT', 'FAR_TEXT'))


def find_masked(haystack: bytes, needle: bytes, mask: bytes):
    if not needle:
        return []
    # Anchor with the longest fixed run, then verify the complete masked span.
    fixed = [i for i, b in enumerate(mask) if b]
    if not fixed:
        return []
    best = []
    a = prev = fixed[0]
    for i in fixed[1:] + [None]:
        if i is None or i != prev + 1:
            if len(best) < prev - a + 1:
                best = (a, prev + 1)
            if i is not None:
                a = i
        if i is not None:
            prev = i
    a, b = best
    anchor = needle[a:b]
    hits = []
    p = 0
    while True:
        q = haystack.find(anchor, p)
        if q < 0:
            break
        start = q - a
        p = q + 1
        if start < 0 or start + len(needle) > len(haystack):
            continue
        if all(not mask[j] or haystack[start + j] == needle[j]
               for j in range(len(needle))):
            hits.append(start)
    return hits


def _best_partial_alignment(runs, extent):
    by_delta = {}
    for run in runs:
        for hit in run['image_offsets']:
            delta = hit - run['segment_offset']
            by_delta.setdefault(delta, []).append(
                (run['segment_offset'], run['segment_offset'] + run['length']))
    best = {'delta': None, 'bytes': 0, 'runs': 0, 'longest': 0}
    for delta, spans in by_delta.items():
        merged = []
        for a, b in sorted(set(spans)):
            if merged and a <= merged[-1][1]:
                merged[-1] = (merged[-1][0], max(merged[-1][1], b))
            else:
                merged.append((a, b))
        coverage = sum(b - a for a, b in merged)
        longest = max((b - a for a, b in merged), default=0)
        if coverage > best['bytes']:
            best = {'delta': delta, 'bytes': coverage,
                    'runs': len(merged), 'longest': longest}
    threshold = max(24, int(extent * 0.25))
    best['status'] = ('version_drift' if best['bytes'] >= threshold and
                      (best['runs'] >= 2 or best['longest'] >= 32)
                      else 'fragment_only')
    return best


def module_code_candidates(name, segments, image):
    result = []
    for seg in segments:
        if not _code_segment(seg):
            continue
        # Ignore leading/trailing bytes outside emitted data. These buffers are
        # zero-filled only where linker-style segment holes occur.
        emitted = [i for i, b in enumerate(seg.mask) if b]
        if not emitted:
            continue
        lo, hi = min(emitted), max(emitted) + 1
        raw = bytes(seg.data[lo:hi])
        mask = bytes(seg.mask[lo:hi])
        if len(raw) < 8:
            continue
        hits = find_masked(image, raw, mask)
        # If the entire member span is not found, retain the largest DATA runs
        # as partial-drift evidence.
        runs = []
        p = lo
        while p < hi:
            while p < hi and not seg.mask[p]:
                p += 1
            q = p
            while q < hi and seg.mask[q]:
                q += 1
            if q - p >= 12:
                run = bytes(seg.data[p:q])
                rhits = []
                pos = 0
                while (pos := image.find(run, pos)) >= 0:
                    rhits.append(pos)
                    pos += 1
                if rhits:
                    runs.append({'segment_offset': p, 'length': q - p,
                                 'image_offsets': rhits})
            p = q + 1
        if hits or runs:
            alignment = _best_partial_alignment(runs, hi - lo) if runs and not hits else None
            result.append({'segment': seg.name, 'class': seg.cls,
                           'segment_index': seg.index,
                           'segment_length': seg.length,
                           'emitted': [lo, hi], 'extent': hi - lo,
                           'full_extent_offsets': hits, 'partial_runs': runs,
                           'partial_alignment': alignment,
                           'pubdefs': [{'name': n, 'offset': o}
                                       for n, o in seg.pubdefs]})
    return result


def inspect_library(path: pathlib.Path, image: bytes):
    lib = library_members(path)
    extent_hits = []
    for page, (member_name, records) in sorted(lib['modules'].items()):
        modname, segments, extdefs = parse_module(records)
        found = module_code_candidates(modname or member_name, segments, image)
        if found:
            extent_hits.append({'member': member_name, 'module': modname, 'page': page,
                                'segments': found})
    return {
        'library': str(path), 'page_size': lib['page_size'],
        'dictionary_symbols': len(lib['dictionary']),
        'dictionary_resolved_symbols': len(lib['dictionary']) - len(lib['unresolved_dictionary']),
        'dictionary_unresolved_symbols': len(lib['unresolved_dictionary']),
        'members': len(lib['modules']), 'members_with_hits': len(extent_hits),
        'full_extent_hit_members': sum(any(s['full_extent_offsets'] for s in m['segments'])
                                       for m in extent_hits),
        'partial_only_members': sum(not any(s['full_extent_offsets'] for s in m['segments'])
                                    and any(s['partial_runs'] for s in m['segments'])
                                    for m in extent_hits),
        'version_drift_members': sum(
            not any(s['full_extent_offsets'] for s in m['segments']) and
            any(s['partial_alignment'] and s['partial_alignment']['status'] == 'version_drift'
                for s in m['segments']) for m in extent_hits),
        'fragment_only_members': sum(
            not any(s['full_extent_offsets'] for s in m['segments']) and
            any(s['partial_runs'] for s in m['segments']) and
            not any(s['partial_alignment'] and s['partial_alignment']['status'] == 'version_drift'
                    for s in m['segments']) for m in extent_hits),
        'full_extent_hit_bytes': sum(s['extent'] for m in extent_hits for s in m['segments']
                                     if s['full_extent_offsets']),
        'extent_hits': extent_hits,
    }


def analyze_libraries(paths, image):
    """Match every library member and cross-check resolvable external near calls."""
    reports = [inspect_library(p, image) for p in paths]
    exports = {}
    parsed = []
    full_extent_members = set()
    partial_members = set()
    matched_bytes = 0
    for path, report in zip(paths, reports):
        lib = library_members(path)
        by_page = lib['modules']
        report_by_page = {m['page']: m for m in report['extent_hits']}
        for page, (member, records) in sorted(by_page.items()):
            modname, segments, extdefs = parse_module(records)
            result = report_by_page.get(page)
            if not result:
                continue
            full_extent_any = False
            for seginfo in result['segments']:
                seg = next((s for s in segments if s.index == seginfo['segment_index']), None)
                if not seg:
                    continue
                lo, hi = seginfo['emitted']
                if seginfo['full_extent_offsets']:
                    full_extent_any = True
                    matched_bytes += hi - lo
                    if len(seginfo['full_extent_offsets']) == 1:
                        image_seg_base = seginfo['full_extent_offsets'][0] - lo
                        for name, off in seg.pubdefs:
                            if lo <= off < hi:
                                exports.setdefault(name, set()).add(image_seg_base + off)
                        parsed.append((report, member, page, modname, seg, extdefs,
                                       image_seg_base, lo, hi))
                elif seginfo['partial_runs']:
                    partial_members.add((str(path), page))
            if full_extent_any:
                full_extent_members.add((str(path), page))
    checks = []
    unresolved = 0
    mismatches = 0
    for report, member, page, modname, seg, extdefs, base, lo, hi in parsed:
        for f in seg.fixups:
            if f['target_method'] not in (2, 6) or not f['target_index']:
                continue
            ext_i = f['target_index'] - 1
            if ext_i < 0 or ext_i >= len(extdefs):
                continue
            name = extdefs[ext_i]
            targets = exports.get(name, set())
            if len(targets) != 1:
                unresolved += 1
                continue
            site = base + f['data_offset']
            expected = next(iter(targets))
            actual = None
            if f['loc'] == 1 and f['width'] == 2 and f['mode'] == 'self_relative':
                if 0 <= site <= len(image) - 2:
                    disp = int.from_bytes(image[site:site + 2], 'little', signed=True)
                    actual = site + 2 + disp
            elif f['loc'] == 3 and f['width'] == 4:
                if 0 <= site <= len(image) - 4:
                    off = int.from_bytes(image[site:site + 2], 'little')
                    segword = int.from_bytes(image[site + 2:site + 4], 'little')
                    actual = segword * 16 + off
            if actual is None:
                continue
            passed = actual == expected
            if not passed:
                mismatches += 1
            checks.append({'library': report['library'], 'member': member,
                           'symbol': name, 'site': f'{site:05x}',
                           'expected': f'{expected:05x}', 'actual': f'{actual:05x}',
                           'result': 'pass' if passed else 'mismatch'})
    return {'libraries': reports,
            'totals': {'full_extent_hit_members': len(full_extent_members),
                       'partial_only_members': len(partial_members - full_extent_members),
                       'version_drift_members': sum(x['version_drift_members'] for x in reports),
                       'fragment_only_members': sum(x['fragment_only_members'] for x in reports),
                       'full_extent_hit_bytes': matched_bytes,
                       'resolved_external_target_checks': len(checks),
                       'unresolved_external_target_checks': unresolved,
                       'target_mismatches': mismatches},
            'target_checks': checks}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('libraries', nargs='*', type=pathlib.Path)
    ap.add_argument('--inspect', type=pathlib.Path,
                    help='print OMF module and segment inventory for one library')
    ap.add_argument('--json', type=pathlib.Path, help='write machine-readable results')
    ap.add_argument('--image', type=pathlib.Path, default=IMAGE)
    args = ap.parse_args()
    image = args.image.read_bytes()
    if args.inspect:
        lib = library_members(args.inspect)
        rows = []
        for page, (name, recs) in sorted(lib['modules'].items()):
            modname, segs, extdefs = parse_module(recs)
            rows.append({'page': page, 'member': name, 'module': modname,
                         'segments': [{'index': s.index, 'name': s.name,
                                       'class': s.cls, 'length': s.length,
                                       'emitted': sum(s.mask),
                                       'pubdefs': s.pubdefs,
                                       'fixups': len(s.fixups)} for s in segs],
                         'externals': extdefs})
        report = {'library': str(args.inspect), 'dictionary_symbols': len(lib['dictionary']),
                  'dictionary_sample': lib['dictionary'][:20], 'members': rows}
    else:
        report = analyze_libraries(args.libraries, image)
    text = json.dumps(report, indent=2)
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(text + '\n', encoding='utf-8')
    if args.inspect or not args.json:
        print(text)
    else:
        for lib in report['libraries']:
            print(f"{lib['library']}: full-extent-hits={lib['full_extent_hit_members']}, "
                  f"partial-only={lib['partial_only_members']}, "
                  f"members={lib['members']}, full-extent-hit-bytes={lib['full_extent_hit_bytes']}")
        print('cross-member references: ' + json.dumps(report['totals']))


if __name__ == '__main__':
    main()
