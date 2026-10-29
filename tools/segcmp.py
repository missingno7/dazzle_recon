#!/usr/bin/env python3
"""Compile C sources as one DAZZLE translation unit and verify its code segment."""
import argparse
from collections import defaultdict
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'build/workers/tooling/segcmp'
META_RE = re.compile(r'/\*\s*objcmp:\s*(\{.*?\})\s*\*/', re.S)
MERGED_KINDS = ('symbols', 'segments', 'groups', 'absolute_symbols')
STRICT = {'EXACT', 'RELOCATED'}
# The full menu literal pool proves its final BEL/NUL at DS:39A4..39A6.
# The provisional census attributed that word to DLOGO; see docs/link.md.
DATA_BOUNDS = {0x158D: (0x249E, 0x39A6), 0x17F5: (0x39A6, 0x3BC6)}


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f'cannot load {path}')
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


bcc = load_module('segcmp_bcc', ROOT / 'tools/bcc.py')
objcmp = load_module('segcmp_objcmp', ROOT / 'tools/objcmp.py')


def int_value(value):
    return int(value, 0) if isinstance(value, str) else int(value)


def parse_address(value):
    seg, off = str(value).split(':', 1)
    return (int(seg, 16), int(off, 16))


def address_text(seg, off):
    return f'{seg:04X}:{off:04X}'


def read_metadata(paths):
    entries = []
    for path in paths:
        content = path.read_text(encoding='utf-8', errors='replace')
        for raw in META_RE.findall(content):
            entry = json.loads(raw)
            if 'function' in entry:
                entry = dict(entry)
                entry['_source'] = path.relative_to(ROOT).as_posix()
                entries.append(entry)
    if not entries:
        raise ValueError('no objcmp metadata with "function" in the input files')

    merged = {kind: {} for kind in MERGED_KINDS}
    conflicted = {kind: set() for kind in MERGED_KINDS}
    conflicts = []
    for entry in entries:
        for kind in MERGED_KINDS:
            table = merged[kind]
            for key, value in entry.get(kind, {}).items():
                if key in conflicted[kind]:
                    continue
                if key not in table:
                    table[key] = value
                elif str(table[key]).lower() != str(value).lower():
                    previous = table.pop(key)
                    conflicted[kind].add(key)
                    conflicts.append({'kind': kind, 'symbol': key,
                                      'values': [previous, value]})

    functions = {}
    conflicted_functions = set()
    for entry in entries:
        if not entry.get('address'):
            continue
        name = str(entry['function'])
        key = name if name.startswith('_') else '_' + name
        if key in conflicted_functions:
            continue
        if key not in functions:
            functions[key] = entry['address']
        elif str(functions[key]).lower() != str(entry['address']).lower():
            previous = functions.pop(key)
            conflicted_functions.add(key)
            conflicts.append({'kind': 'functions', 'symbol': key,
                              'values': [previous, entry['address']]})
    return entries, {**merged, 'functions': functions}, conflicts


def metadata_for_public(public, entries, paragraph):
    names = {public.lower()}
    if public.startswith('_'):
        names.add(public[1:].lower())
    candidates = []
    for entry in entries:
        if str(entry.get('function', '')).lower() not in names or not entry.get('address'):
            continue
        try:
            seg, _ = parse_address(entry['address'])
        except (TypeError, ValueError):
            continue
        if seg == paragraph:
            candidates.append(entry)
    unique = {(str(e.get('address')).lower(), int_value(e.get('size', 0))): e
              for e in candidates}
    if len(unique) > 1:
        raise ValueError(f'conflicting metadata for public {public}')
    return next(iter(unique.values()), None)


def inventory_modules():
    data = json.loads((ROOT / 'evidence/functions.json').read_text(encoding='utf-8'))
    modules = defaultdict(list)
    # Data rows count too: a module may begin with a table (e.g. EMU's
    # emu086_prefix_table at 1BAC:0000), which bounds the preceding module.
    for row in data.get('functions', []):
        try:
            paragraph = int_value(row['segment_paragraph'])
            linear = int_value(row.get('linear', paragraph * 16 + parse_address(row['addr'])[1]))
            end = int_value(row.get('end', linear + int_value(row.get('size', 0))))
        except (KeyError, TypeError, ValueError):
            continue
        modules[paragraph].append({'linear': linear, 'end': end, 'row': row})
    starts = {paragraph: min(item['linear'] for item in rows)
              for paragraph, rows in modules.items() if rows}
    ends = {paragraph: max(item['end'] for item in rows)
            for paragraph, rows in modules.items() if rows}
    return data, modules, starts, ends


def derive_start(paragraph, starts, ends, override):
    if override:
        seg, off = parse_address(override)
        return seg, off, 'explicit --start override'

    if paragraph == 0x066A:
        residue = json.loads((ROOT / 'evidence/seg0-residue.json').read_text(encoding='utf-8'))
        interval = residue['scope'].get('identified_RTL_contribution_interval')
        if not interval or len(interval) != 2:
            raise ValueError('seg0-residue.json lacks the identified RTL interval')
        linear = int(interval[1])
        off = linear - paragraph * 16
        if not 0 <= off <= 65535:
            raise ValueError('identified RTL end does not map into segment 066A')
        if linear != starts.get(paragraph):
            raise ValueError(f'066A start disagreement: RTL end {linear:#x}, '
                             f'inventory start {starts.get(paragraph)!r}')
        return paragraph, off, ('end of identified segment-0 RTL interval '
                                 f'({linear:#06x}; docs/runtime-drift.md gives 0000:66AA, '
                                 'then 066A:000B)')

    if paragraph not in starts:
        raise ValueError(f'no oracle start evidence for segment {paragraph:04X}; use --start')
    current = starts[paragraph]
    previous = [(p, start) for p, start in starts.items() if start < current]
    if previous:
        prev_para, _ = max(previous, key=lambda item: item[1])
        prev_end = ends.get(prev_para)
        if prev_end == current:
            return paragraph, current - paragraph * 16, (
                f'first byte after segment {prev_para:04X} last classified extent '
                f'(linear {prev_end:#06x})')
        if prev_end is not None and prev_end < current:
            gap = current - prev_end
            return paragraph, current - paragraph * 16, (
                f'first inventoried code address after segment {prev_para:04X}; '
                f'{gap} trailing/unclassified byte(s) follow its last classified extent')
    return paragraph, current - paragraph * 16, 'first inventoried code address for this paragraph'


def derive_end(paragraph, start_linear, starts, inventory):
    following = [(p, linear) for p, linear in starts.items() if linear > start_linear]
    if following:
        next_para, end_linear = min(following, key=lambda item: item[1])
        return end_linear, f'next module start {address_text(next_para, end_linear-next_para*16)}'
    end_linear = int_value(inventory.get('code_area_end', 0))
    if end_linear > start_linear:
        return end_linear, f'functions.json code_area_end upper bound ({end_linear:#x})'
    raise ValueError(f'cannot derive module end for {paragraph:04X}; no later module start')


def choose_code_segment(obj, selector, entries):
    code = [(i, seg) for i, seg in enumerate(obj.segments)
            if i and seg and seg['class'].upper() == 'CODE']
    try:
        paragraph = int(selector, 16)
        numeric = True
    except ValueError:
        paragraph = None
        numeric = False
    if numeric:
        matches = code
    else:
        matches = [(i, seg) for i, seg in code
                   if seg['name'].upper() == selector.upper()]
        if not matches:
            raise ValueError(f'no OMF CODE segment named {selector}')
        paragraphs = set()
        for entry in entries:
            if entry.get('address'):
                try:
                    paragraphs.add(parse_address(entry['address'])[0])
                except (TypeError, ValueError):
                    pass
        if len(paragraphs) == 1:
            paragraph = next(iter(paragraphs))
        elif len(paragraphs) > 1:
            raise ValueError('segment name maps to multiple target paragraphs; use a paragraph selector')
    if not matches:
        raise ValueError('object has no OMF CODE segment')
    if len(matches) != 1:
        names = ', '.join(f'{seg["name"]}[{i}]' for i, seg in matches)
        raise ValueError(f'object has multiple CODE segments ({names}); selector is ambiguous')
    if paragraph is None:
        raise ValueError('target paragraph is unknown; use a paragraph selector or --start')
    return paragraph, *matches[0]


def public_extents(obj, code_index, code_length):
    by_offset = defaultdict(list)
    for name, (segment, offset, _group) in obj.publics.items():
        if segment == code_index and 0 <= offset < code_length:
            by_offset[offset].append(name)
    if not by_offset:
        by_offset[0] = ['__segcmp_static_start__']
        synthetic = {0}
    else:
        synthetic = set()
        if 0 not in by_offset:
            by_offset[0] = ['__segcmp_static_start__']
            synthetic.add(0)
    offsets = sorted(by_offset)
    extents = []
    for index, start in enumerate(offsets):
        end = offsets[index + 1] if index + 1 < len(offsets) else code_length
        if end <= start:
            continue
        names = sorted(by_offset[start])
        extents.append({'start': start, 'end': end, 'names': names,
                        'synthetic': start in synthetic})
    return extents


def display_public(names):
    if not names or all(name.startswith('__segcmp_static_') for name in names):
        return 'static/unnamed'
    return '|'.join(name[1:] if name.startswith('_') else name for name in names)


def function_address_and_size(public, obj_start, obj_size, paragraph, start_off, entries):
    entry = metadata_for_public(public, entries, paragraph)
    if entry:
        address = str(entry['address']).upper()
        size = int_value(entry.get('size', obj_size))
        return address, size, entry
    return address_text(paragraph, start_off + obj_start), obj_size, None


def compare_one(obj, name, address, size, image, relocations, mapping):
    try:
        return objcmp.compare(obj, name, parse_address(address), size,
                              image, relocations, mapping)
    except (ValueError, KeyError, IndexError) as exc:
        return {'status': 'UNKNOWN', 'reason': str(exc)}, b''


def segment_owner(image_linear, object_offset, extents, entries, paragraph):
    public = None
    for extent in extents:
        if extent['start'] <= object_offset < extent['end']:
            public = display_public(extent['names'])
            if extent['synthetic']:
                public = None
            break
    metadata_names = []
    for entry in entries:
        if not entry.get('address') or not entry.get('size'):
            continue
        try:
            seg, off = parse_address(entry['address'])
            size = int_value(entry['size'])
        except (TypeError, ValueError):
            continue
        linear = seg * 16 + off
        if seg == paragraph and linear <= image_linear < linear + size:
            metadata_names.append(str(entry['function']))
    if public and metadata_names:
        normalized = {public.lower().split('|')[0],
                      public.lower().split('|')[0].lstrip('_')}
        matching = [name for name in metadata_names if name.lower() in normalized]
        if matching:
            return matching[0], public
    return 'static/unnamed', public


def data_summary(obj, code_index):
    contributions = []
    literals = []
    for index, seg in enumerate(obj.segments):
        if not index or not seg or index == code_index:
            continue
        present = seg['present']
        data = seg['data']
        emitted = sum(present)
        name = seg['name']
        cls = seg['class']
        entry = {'segment': name, 'class': cls, 'size': seg['length'],
                 'emitted_bytes': emitted,
                 'unemitted_bytes': seg['length'] - emitted,
                 'nonzero_emitted_bytes': sum(1 for i, value in enumerate(data)
                                              if present[i] and value)}
        contributions.append(entry)
        if 'LIT' in name.upper() or 'CONST' in name.upper() or \
                'LIT' in cls.upper() or 'CONST' in cls.upper():
            literals.append({'segment': name, 'bytes': emitted,
                             'basis': 'OMF segment name/class'})

    # Borland often places string literals in _DATA. Report printable
    # NUL-terminated runs as candidates, without claiming every run is a literal.
    string_candidates = []
    for index, seg in enumerate(obj.segments):
        if not index or not seg or index == code_index:
            continue
        data, present = seg['data'], seg['present']
        run_start = None
        for offset in range(seg['length'] + 1):
            byte = data[offset] if offset < seg['length'] and present[offset] else 0
            printable = 0x20 <= byte <= 0x7e
            if printable and run_start is None:
                run_start = offset
            elif byte == 0 and run_start is not None and offset - run_start >= 2:
                raw = bytes(data[run_start:offset]).decode('ascii', errors='replace')
                string_candidates.append({'segment': seg['name'], 'offset': run_start,
                                          'text': raw, 'bytes': offset-run_start+1})
                run_start = None
            elif not printable:
                run_start = None
    if string_candidates:
        literals.append({'segment': '_DATA candidates',
                         'bytes': sum(row['bytes'] for row in string_candidates),
                         'basis': 'printable NUL-terminated runs; candidates only'})
    return {'segments': contributions,
            'literal_pool': {'identified_segments': literals,
                             'string_candidates': string_candidates}}


def compare_data(obj, paragraph, image, relocations, mapping):
    """Check complete contributions against the independent ownership census.

    Census ownership is provisional; byte/fixup equality does not establish
    original source identifiers. Do not synthesize or mask unresolved storage.
    """
    census = json.loads((ROOT / 'evidence/dgroup.json').read_text(encoding='utf8'))
    rows = []
    for contribution in census['contributions']:
        if contribution['owner'] != f'{paragraph:04X}':
            continue
        name = contribution['object_segment']
        if contribution['space'] != 'DGROUP' or name not in ('_DATA', '_BSS'):
            continue
        candidates = [(i, s) for i, s in enumerate(obj.segments)
                      if s and s['name'] == name]
        start, end = contribution['start'], contribution['end_exclusive']
        if name == '_DATA' and paragraph in DATA_BOUNDS:
            start, end = DATA_BOUNDS[paragraph]
        row = {'segment': name, 'address': f'29BF:{start:04X}',
               'expected_extent': end-start, 'ownership': 'PROVISIONAL census'}
        if name == '_DATA' and paragraph in DATA_BOUNDS:
            row['ownership'] = 'menu literal boundary correction; docs/link.md'
        if len(candidates) != 1:
            row.update(status='MISMATCH', reason='missing or ambiguous object segment')
            rows.append(row)
            continue
        index, segment = candidates[0]
        length = segment['length']
        row['object_size'] = length
        padding = end-start-length
        # Word alignment between contributions is the linker's responsibility.
        aligned = (name == '_DATA' and padding == 1 and end % 2 == 0
                   and image[0x29BF0+end-1] == 0)
        row['link_alignment_bytes'] = 1 if aligned else 0
        if length != end-start and not aligned:
            row.update(status='MISMATCH', reason='full contribution size differs')
        elif name == '_BSS':
            if any(segment['present']):
                row.update(status='MISMATCH', reason='BSS unexpectedly emits initialized bytes')
            else:
                row.update(status='EXACT', reason='declared BSS size; not serialized-byte proof')
        elif not all(segment['present']):
            row.update(status='MISMATCH', reason='DATA has un-emitted bytes')
        else:
            try:
                result, _ = objcmp.compare(obj, name, (0x29BF, start), length,
                                          image, relocations, mapping,
                                          extent=(index, 0, length))
                row.update(result)
                row['status'] = row['status'].upper()
                row['proof'] = 'complete initialized contribution, including fixups and MZ sites'
            except (ValueError, KeyError, IndexError) as exc:
                row.update(status='UNKNOWN', reason=str(exc))
        if row['status'] in STRICT:
            # Size alone cannot reveal a reordered BSS. Validate every public
            # with an independently supplied source binding as well.
            for public, (si, offset, _group) in obj.publics.items():
                binding = mapping.get('symbols', {}).get(public)
                if si != index or binding is None:
                    continue
                target_seg, target_off = objcmp.address(binding)
                if target_seg * 16 + target_off != 0x29BF0 + start + offset:
                    row.update(status='MISMATCH', reason=f'public placement differs: {public}',
                               public=public, object_offset=offset, original_binding=binding)
                    break
        rows.append(row)
    far = [c for c in census['contributions']
           if c['owner'] == f'{paragraph:04X}' and c['space'] == '1E81']
    if far:
        start = min(c['start'] for c in far)
        end = max(c['end_exclusive'] for c in far)
        candidates = [(i, s) for i, s in enumerate(obj.segments)
                      if s and s['class'] in ('FAR_DATA', 'FAR_BSS') and s['length']]
        row = {'segment': 'far workspace', 'address': f'1E81:{start:04X}',
               'expected_extent': end-start, 'ownership': 'PROVISIONAL census',
               'proof': 'declared workspace and zero fill; natural link placement is separate'}
        if len(candidates) != 1:
            row.update(status='MISMATCH', reason='missing or ambiguous far contribution')
        else:
            index, segment = candidates[0]
            row.update(segment=segment['name'], object_size=segment['length'])
            if segment['length'] != end-start:
                row.update(status='MISMATCH', reason='far workspace size differs')
            elif any(segment['data']) or any(image[0x1E810+start:0x1E810+end]):
                row.update(status='MISMATCH', reason='far workspace is not all zero')
            elif any(f.segment == index for f in obj.fixups) or any(
                    0x1E810+start <= site < 0x1E810+end for site in relocations):
                row.update(status='MISMATCH', reason='unexpected far workspace fixup/relocation')
            else:
                row.update(status='EXACT', reason='zero workspace size and bound publics')
                for public, (si, offset, _group) in obj.publics.items():
                    binding = mapping.get('symbols', {}).get(public)
                    if si != index or binding is None:
                        continue
                    target_seg, target_off = objcmp.address(binding)
                    if target_seg*16+target_off != 0x1E810+start+offset:
                        row.update(status='MISMATCH', reason=f'far public placement differs: {public}')
                        break
        rows.append(row)
    return rows


def run(args):
    inputs = []
    for given in args.files:
        path = given if given.is_absolute() else (Path.cwd() / given)
        path = path.resolve()
        if not path.is_file():
            raise ValueError(f'input C file not found: {given}')
        inputs.append(path)

    metadata, mapping, metadata_conflicts = read_metadata(inputs)
    WORK.mkdir(parents=True, exist_ok=True)
    run_dir = Path(tempfile.mkdtemp(prefix='run_', dir=WORK))
    concatenated = run_dir / 'PROBE.C'
    payload = b''
    for path in inputs:
        source = path.read_bytes()
        payload += source
        if source and not source.endswith((b'\n', b'\r')):
            payload += b'\n'
    concatenated.write_bytes(payload)

    try:
        # Project headers (include/*.h) are staged next to the TU so sources can
        # use #include "dazzle.h"; they are part of the hashed compile input.
        headers = {h.name.upper(): h.read_bytes() for h in sorted((ROOT / 'include').glob('*.h'))}
        object_path = bcc.compile_source(concatenated, profile='dazzle', fresh=True, headers=headers)
    except RuntimeError as exc:
        selector_para = None
        try:
            selector_para = f'{int(args.segment, 16):04X}'
        except ValueError:
            pass
        failed = {
            'schema': 1,
            'kind': 'translation-unit-segment',
            'source': inputs[0].relative_to(ROOT).as_posix() if inputs[0].is_relative_to(ROOT) else str(inputs[0]),
            'sources': [p.relative_to(ROOT).as_posix() if p.is_relative_to(ROOT) else str(p) for p in inputs],
            'profile': 'dazzle',
            'segment_selector': args.segment,
            'segment_paragraph': selector_para,
            'status': 'COMPILE_ERROR',
            'reason': str(exc),
            'metadata_conflicts': metadata_conflicts,
            'results': [],
        }
        if args.out:
            output = args.out if args.out.is_absolute() else ROOT / args.out
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_text(json.dumps(failed, indent=2) + '\n',
                              encoding='utf-8', newline='\n')
        print(f'{args.segment} COMPILE_ERROR: {exc}')
        return failed
    obj = objcmp.Object(object_path.read_bytes())
    inventory, modules, starts, ends = inventory_modules()
    paragraph, code_index, code = choose_code_segment(obj, args.segment, metadata)
    start_seg, start_off, start_source = derive_start(paragraph, starts, ends, args.start)
    start_linear = start_seg * 16 + start_off
    end_linear, end_source = derive_end(paragraph, start_linear, starts, inventory)
    original_length = end_linear - start_linear
    if original_length <= 0:
        raise ValueError('derived original module extent is not positive')
    code_length = code['length']
    if start_off + code_length > 65536:
        raise ValueError('compiled code segment crosses its paragraph window')
    image, relocations = objcmp.load_oracle()
    if start_linear + max(code_length, original_length) > len(image):
        raise ValueError('derived segment extent exceeds the load image')

    objcmp.check_known_bindings(mapping, inventory)
    extents = public_extents(obj, code_index, code_length)
    original_publics = [(name, value) for name, value in obj.publics.items()
                        if value[0] == code_index]
    public_results = []
    for extent in extents:
        if extent['synthetic']:
            continue
        public = extent['names'][0]
        public_name = display_public(extent['names'])
        address, size, entry = function_address_and_size(
            public, extent['start'], extent['end']-extent['start'],
            paragraph, start_off, metadata)
        result, _resolved = compare_one(obj, public, address, size,
                                       image, relocations, mapping)
        function_status = str(result.get('status', 'UNKNOWN')).upper()
        function_reason = result.get('reason', '')
        if entry is None or not entry.get('size'):
            result = dict(result)
            result['diagnostic_status'] = function_status
            function_status = 'UNKNOWN'
            function_reason = 'no independent metadata address and size for this public'
        public_results.append({
            'function': (entry.get('function') if entry else public_name),
            'address': address,
            'size': size,
            'status': function_status,
            'reason': function_reason,
            'offset': result.get('offset'),
            'object_public': public,
            'object_offset': extent['start'],
            'object_size': extent['end']-extent['start'],
            **({'diagnostic_status': result['diagnostic_status']}
               if result.get('diagnostic_status') else {}),
        })

    resolved = bytearray(code['data'])
    segment_chunks = []
    problems = []
    first_hole = next((i for i, value in enumerate(code['present']) if not value), None)
    if first_hole is not None:
        problems.append({'offset': first_hole,
                         'status': 'MISMATCH',
                         'reason': f'object CODE segment has an un-emitted byte at +{first_hole:#x}'})
    else:
        # Static functions have no PUBDEF; register the synthetic extent name as a
        # local label so objcmp.compare can bound and verify it like any public.
        for extent in extents:
            if extent['synthetic']:
                obj.publics.setdefault(extent['names'][0], (code_index, extent['start'], None))
        for extent in extents:
            public = extent['names'][0]
            name = public
            target_off = start_off + extent['start']
            if target_off > 65535:
                problems.append({'offset': extent['start'], 'status': 'MISMATCH',
                                 'reason': 'target code offset exceeds 16 bits'})
                continue
            part_result, part_resolved = compare_one(
                obj, name, address_text(start_seg, target_off),
                extent['end']-extent['start'], image, relocations, mapping)
            if len(part_resolved) == extent['end']-extent['start']:
                resolved[extent['start']:extent['end']] = part_resolved
            chunk = {'start': extent['start'], 'end': extent['end'],
                     'public': display_public(extent['names']),
                     'synthetic': extent['synthetic'],
                     'status': str(part_result.get('status', 'UNKNOWN')).upper(),
                     'reason': part_result.get('reason', '')}
            segment_chunks.append(chunk)
            if part_result.get('status') not in STRICT:
                local_offset = part_result.get('offset')
                problems.append({'offset': (extent['start'] + int(local_offset)
                                             if local_offset is not None else extent['start']),
                                 'status': part_result.get('status', 'UNKNOWN'),
                                 'reason': part_result.get('reason', 'comparison did not prove this extent'),
                                 'public': chunk['public']})

    common = min(code_length, original_length)
    expected = image[start_linear:start_linear + common]
    byte_difference = next((i for i, (left, right) in enumerate(zip(resolved[:common], expected))
                            if left != right), None)
    if byte_difference is not None:
        problems.append({'offset': byte_difference, 'status': 'MISMATCH',
                         'reason': f'byte +{byte_difference:#x}: object {resolved[byte_difference]:02x}, '
                                   f'image {expected[byte_difference]:02x}'})
    # TLINK zero-fills between a module's end and the next module's aligned
    # start. Accept a shortfall only if the gap bytes are zero and the next
    # module start is word (gap 1) or paragraph (gap < 16) aligned; the gap is
    # link padding, reported separately and never counted as module code.
    alignment_padding = 0
    gap = original_length - code_length
    end_linear = start_linear + original_length
    if 0 < gap < 16 and not any(image[start_linear + code_length:end_linear]) and \
            (end_linear % 16 == 0 or (gap == 1 and end_linear % 2 == 0)):
        alignment_padding = gap
        original_length = code_length
    if code_length != original_length:
        problems.append({'offset': common, 'status': 'MISMATCH',
                         'reason': f'code segment length {code_length:#x} differs from '
                                   f'original module extent {original_length:#x}'})

    if byte_difference is not None:
        first_problem = next((row for row in problems
                             if row.get('reason', '').startswith('byte +')),
                            {'offset': byte_difference, 'status': 'MISMATCH',
                             'reason': 'first differing byte'})
    else:
        first_problem = min(problems, key=lambda row: row['offset']) if problems else None
    if first_problem:
        status = str(first_problem['status']).upper()
        if status in STRICT:
            status = 'MISMATCH'
        first_offset = first_problem['offset']
        target_linear = start_linear + first_offset
        owner, nearest_public = segment_owner(target_linear, first_offset,
                                             extents, metadata, start_seg)
        first_difference = {
            'offset': first_offset,
            'address': address_text(start_seg, start_off + first_offset)
            if start_off + first_offset <= 65535 else None,
            'function': owner,
            'nearest_public': nearest_public,
            'reason': first_problem['reason'],
            'object_byte': (f'{code["data"][first_offset]:02X}'
                            if first_offset < code_length else None),
            'image_byte': (f'{image[start_linear+first_offset]:02X}'
                           if first_offset < original_length and
                           start_linear + first_offset < len(image) else None),
        }
        reason = first_problem['reason']
    else:
        status = 'EXACT' if bytes(code['data']) == image[start_linear:start_linear+code_length] else 'RELOCATED'
        first_difference = None
        reason = ''

    data_info = data_summary(obj, code_index)
    data_mapping = {**mapping, 'segments': {**mapping.get('segments', {}),
                                          code['name']: address_text(start_seg, start_off)}}
    data_checks = compare_data(obj, paragraph, image, relocations, data_mapping)
    result = {
        'schema': 1,
        'kind': 'translation-unit-segment',
        'source': inputs[0].relative_to(ROOT).as_posix() if inputs[0].is_relative_to(ROOT) else str(inputs[0]),
        'sources': [p.relative_to(ROOT).as_posix() if p.is_relative_to(ROOT) else str(p) for p in inputs],
        'profile': 'dazzle',
        'object': str(object_path),
        'object_sha256': hashlib.sha256(object_path.read_bytes()).hexdigest(),
        'segment': {
            'name': code['name'],
            'paragraph': f'{paragraph:04X}',
            'start': address_text(start_seg, start_off),
            'start_linear': start_linear,
            'start_derivation': start_source,
            'object_length': code_length,
            'original_extent': {
                'end_linear': end_linear,
                'end': address_text(end_linear // 16, end_linear % 16),
                'length': original_length,
                'end_derivation': end_source,
                'length_matches': code_length == original_length,
                'alignment_padding_bytes': alignment_padding,
            },
        },
        'status': status,
        'reason': reason,
        'first_difference': first_difference,
        'results': public_results,
        'metadata_conflicts': metadata_conflicts,
        'segment_extents': segment_chunks,
        'data_contributions': data_info['segments'],
        'data_comparison': data_checks,
        'literal_pool': data_info['literal_pool'],
    }
    if args.out:
        output = args.out if args.out.is_absolute() else ROOT / args.out
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8', newline='\n')
        result['out'] = str(output)
    print_result(result)
    return result


def print_result(result):
    segment = result['segment']
    print(f'{segment["paragraph"]} {segment["name"]}: {result["status"]} '
          f'from {segment["start"]}; object {segment["object_length"]:#x} bytes, '
          f'original extent {segment["original_extent"]["length"]:#x} bytes '
          f'({"same length" if segment["original_extent"]["length_matches"] else "length differs"})'
          + (f' + {segment["original_extent"]["alignment_padding_bytes"]} zero byte(s) TLINK alignment padding'
             if segment["original_extent"].get("alignment_padding_bytes") else ''))
    if result['first_difference']:
        first = result['first_difference']
        owner = first['function']
        if first.get('nearest_public'):
            owner += f' (nearest public {first["nearest_public"]})'
        print(f'  first issue +{first["offset"]:#x} at {first["address"] or "?"} '
              f'in {owner}: {first["reason"]}')
    for item in result['data_contributions']:
        print(f'  {item["segment"]}/{item["class"]}: {item["size"]} bytes '
              f'({item["emitted_bytes"]} initialized, {item["unemitted_bytes"]} uninitialized)')
    for item in result.get('data_comparison', []):
        print(f'  data {item["segment"]}: {item["status"]} {item.get("reason", "")}')
    literals = result['literal_pool']['identified_segments']
    print('  literal pool: ' + (', '.join(f'{x["segment"]} {x["bytes"]} B' for x in literals)
                               if literals else 'no separate literal segment/string candidates found'))
    public_results = result['results']
    counts = defaultdict(int)
    for item in public_results:
        counts[item['status']] += 1
    print('  public extents: ' + ', '.join(f'{status} {count}' for status, count in sorted(counts.items())))
    for item in public_results:
        suffix = f'  {item["reason"]}' if item['reason'] else ''
        print(f'    {item["status"]:<9} {item["address"]} {item["function"]}{suffix}')


def main():
    # BC++ diagnostics can contain CP437 glyphs that are not representable by
    # the Windows console code page. Keep the captured diagnostic intact in
    # JSON and make the terminal rendering safe as well.
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, 'reconfigure'):
            stream.reconfigure(errors='backslashreplace')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('segment', help='OMF CODE segment name or target paragraph (hex)')
    parser.add_argument('files', nargs='+', type=Path, help='C sources concatenated in this order')
    parser.add_argument('--start', help='override target start as SSSS:OOOO')
    parser.add_argument('--out', type=Path, help='write full segment and per-public JSON result')
    parser.add_argument('--data', action='store_true', help='also require strict DATA and BSS checks')
    args = parser.parse_args()
    try:
        result = run(args)
    except (OSError, ValueError, RuntimeError, json.JSONDecodeError,
            objcmp.OMFError, TimeoutError) as exc:
        parser.exit(2, f'segcmp: {exc}\n')
    if result.get('status') not in STRICT:
        raise SystemExit(1)
    if args.data and any(row['status'] not in STRICT for row in result.get('data_comparison', [])):
        raise SystemExit(1)


if __name__ == '__main__':
    main()
