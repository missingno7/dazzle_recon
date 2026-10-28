"""Generate docs/STATUS.md from the function and match evidence."""
from collections import Counter, defaultdict
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
META_RE = re.compile(r'/\*\s*objcmp:\s*(\{.*?\})\s*\*/', re.S)
STRICT = {'EXACT', 'RELOCATED'}
APPLICATION_CLASSES = {'application', 'asm_candidate', 'uncertain', 'glue'}
STATUS_ORDER = ('EXACT', 'RELOCATED', 'SEMANTIC', 'PROVISIONAL',
                'UNKNOWN/asm-origin', 'blocked', 'not yet attempted')
SEGMENTS = [('0000', 'segment-0 application residue'),
            ('066A', '066A'), ('0B84', '0B84'), ('0CCD', '0CCD'),
            ('11C1', '11C1'), ('158D', '158D'), ('17F5', '17F5'),
            ('1899', '1899'), ('19F0', '19F0'), ('1A72', '1A72')]


def load_json(path):
    return json.loads(path.read_text(encoding='utf-8'))


def int_value(value):
    return int(value, 0) if isinstance(value, str) else int(value)


def address_key(value):
    if not value or ':' not in str(value):
        return None
    seg, off = str(value).strip().split(':', 1)
    if seg.upper() == 'DS':
        seg = '29BF'
    try:
        return f'{int(seg, 16) & 65535:04X}:{int(off, 16) & 65535:04X}'
    except ValueError:
        return str(value).strip().upper()


def address_segment(row):
    if row.get('segment_paragraph') is not None:
        return f'{int(row["segment_paragraph"]):04X}'
    key = address_key(row.get('addr') or row.get('address'))
    return key.split(':', 1)[0] if key else '????'


def metadata_for_source(source, cache, warnings):
    if not source:
        return []
    if source in cache:
        return cache[source]
    path = Path(source)
    if not path.is_absolute():
        path = ROOT / path
    try:
        resolved = path.resolve()
        resolved.relative_to(ROOT.resolve())
        contents = resolved.read_text(encoding='utf-8', errors='replace')
    except (OSError, ValueError):
        warnings.add(f'source unavailable: {source}')
        cache[source] = []
        return []
    entries = []
    for raw in META_RE.findall(contents):
        try:
            entries.append(json.loads(raw))
        except json.JSONDecodeError:
            warnings.add(f'invalid objcmp metadata: {source}')
    cache[source] = entries
    return entries


def choose_metadata(entries, result):
    if not entries:
        return {}
    addr = address_key(result.get('address'))
    name = str(result.get('function', '')).lower()
    for entry in entries:
        if addr and address_key(entry.get('address')) == addr:
            return entry
    for entry in entries:
        if name and str(entry.get('function', '')).lower() == name:
            return entry
    return entries[0] if len(entries) == 1 else {}


def blocker_label(value):
    if not value:
        return None
    if isinstance(value, dict):
        value = value.get('class') or value.get('kind') or value.get('reason') or value
    if isinstance(value, (list, tuple, set)):
        value = '; '.join(str(item) for item in value)
    label = re.sub(r'\s+', ' ', str(value)).strip()
    return label or None


def blocker_class(value):
    label = blocker_label(value)
    if not label:
        return None
    label = label.split(';', 1)[0].strip()
    lowered = label.lower()
    if 'not converged' in lowered or 'not-converged' in lowered:
        return 'source not converged'
    explicit = re.match(r'^([a-z][a-z0-9]*(?:-[a-z0-9]+)+)(?=\s*:)', lowered)
    if explicit:
        return explicit.group(1)
    leading = re.match(r'^([a-z][a-z0-9]*(?:-[a-z0-9]+)+)\b', lowered)
    return leading.group(1) if leading else label


def interval_mask(end, rows):
    mask = bytearray(end)
    for row in rows:
        try:
            start = int_value(row.get('linear', 0))
            size = int_value(row.get('size', 0))
        except (TypeError, ValueError):
            continue
        lo, hi = max(0, start), min(end, start + max(0, size))
        if hi > lo:
            mask[lo:hi] = b'\x01' * (hi - lo)
    return mask


def exclusive_bytes(rows, islands, end):
    """Assign each non-island byte to at most one function start."""
    occupied = bytearray(end)
    for row in islands:
        try:
            lo = max(0, int_value(row.get('linear', 0)))
            hi = min(end, lo + max(0, int_value(row.get('size', 0))))
        except (TypeError, ValueError):
            continue
        if hi > lo:
            occupied[lo:hi] = b'\x01' * (hi - lo)
    credits = {}
    for row in sorted(rows, key=lambda item: (int_value(item.get('linear', 0)),
                                               str(item.get('addr', '')))):
        try:
            start = int_value(row.get('linear', 0))
            size = max(0, int_value(row.get('size', 0)))
        except (TypeError, ValueError):
            credits[row['_key']] = 0
            continue
        lo, hi = max(0, start), min(end, start + size)
        credit = 0
        for pos in range(lo, hi):
            if not occupied[pos]:
                occupied[pos] = 1
                credit += 1
        credits[row['_key']] = credit
    return credits


def format_num(value):
    return f'{value:,}'


def main():
    inventory = load_json(ROOT / 'evidence/functions.json')
    patterns = load_json(ROOT / 'evidence/patterns.json')
    summary = inventory.get('summary', {})
    warnings = set()
    source_cache = {}
    results_by_addr = defaultdict(list)
    metadata_by_addr = defaultdict(list)
    match_files = sorted((ROOT / 'evidence/match').glob('*.json'))

    for match_path in match_files:
        payload = load_json(match_path)
        source = payload.get('source')
        source_meta = metadata_for_source(source, source_cache, warnings)
        stamp = match_path.stat().st_mtime_ns
        for entry in source_meta:
            key = address_key(entry.get('address'))
            if key and ('level' in entry or 'blocker' in entry):
                metadata_by_addr[key].append({'metadata': entry, 'mtime': stamp,
                                              'source': source or str(match_path)})
        for result in payload.get('results', []):
            key = address_key(result.get('address'))
            if not key:
                warnings.add(f'result missing address: {match_path.name}')
                continue
            meta = choose_metadata(source_meta, result)
            record = {'result': result, 'metadata': meta, 'mtime': stamp,
                      'source': source or str(match_path),
                      'profile': payload.get('profile', '')}
            results_by_addr[key].append(record)
            if meta and ('level' in meta or 'blocker' in meta):
                metadata_by_addr[key].append({'metadata': meta, 'mtime': stamp,
                                              'source': source or str(match_path)})

    # Inventory is address keyed: aliases and duplicate reports never add a function twice.
    inventory_by_addr = {}
    inventory_conflicts = []
    for row in inventory.get('functions', []):
        if row.get('class') == 'data':
            continue
        key = address_key(row.get('addr'))
        if not key:
            continue
        if key in inventory_by_addr:
            previous = inventory_by_addr[key]
            if (previous.get('class'), previous.get('size'), previous.get('name')) != (
                    row.get('class'), row.get('size'), row.get('name')):
                inventory_conflicts.append((key, previous, row))
            continue
        item = dict(row)
        item['_key'] = key
        item['_segment'] = address_segment(row)
        inventory_by_addr[key] = item

    latest_by_addr = {}
    conflicts = []
    for key, records in results_by_addr.items():
        values = {(str(r['result'].get('status', '')).upper(),
                   int_value(r['result'].get('size', 0)),
                   str(r['result'].get('function', ''))) for r in records}
        newest = max(r['mtime'] for r in records)
        latest = [r for r in records if r['mtime'] == newest]
        latest_values = {(str(r['result'].get('status', '')).upper(),
                          int_value(r['result'].get('size', 0))) for r in latest}
        history_conflict = len(values) > 1
        latest_conflict = len(latest_values) > 1
        chosen = sorted(latest, key=lambda r: (r['source'], r['profile']))[0]
        latest_by_addr[key] = {'record': chosen, 'conflict': latest_conflict}
        if history_conflict or latest_conflict:
            labels = sorted({f"{r['result'].get('status', 'UNKNOWN')}"
                             f"/{r['result'].get('size', '?')}B"
                             f" ({Path(r['source']).name})" for r in records})
            conflicts.append((key, labels))

    def current_meta(key):
        records = metadata_by_addr.get(key, [])
        if not records:
            return {}
        newest = max(r['mtime'] for r in records)
        options = [r['metadata'] for r in records if r['mtime'] == newest]
        # Preserve a blocker if simultaneous source references carry one.
        result = dict(options[0])
        blockers = sorted({label for item in options
                           if (label := blocker_label(item.get('blocker')) )})
        if blockers:
            result['blocker'] = blockers
        return result

    def result_status(key):
        latest = latest_by_addr.get(key)
        if not latest:
            return None
        if latest['conflict']:
            return 'CONFLICT'
        return str(latest['record']['result'].get('status', 'UNKNOWN')).upper()

    def classify(row):
        key = row['_key']
        status = result_status(key)
        meta = current_meta(key)
        level = str(meta.get('level', '')).strip().upper().replace('-', '_').replace(' ', '_')
        if status in STRICT:
            return status
        if status in ('SEMANTIC', 'PROVISIONAL'):
            return status
        if level in ('SEMANTIC', 'PROVISIONAL'):
            return level
        if status == 'BLOCKED' or level == 'BLOCKED':
            return 'blocked'
        if (level in ('UNKNOWN', 'ASM_ORIGIN', 'ASM') or 'ASM' in level or 'ASSEMB' in level or
                row.get('class') in ('asm_candidate', 'uncertain') or
                status is not None):
            return 'UNKNOWN/asm-origin'
        return 'not yet attempted'

    app_rows = [row for row in inventory_by_addr.values()
                if row.get('class') in APPLICATION_CLASSES]
    code_end = int_value(inventory.get('code_area_end', '0'))
    islands = inventory.get('data_islands', [])
    credits = exclusive_bytes(app_rows, islands, code_end)
    categories = Counter()
    category_bytes = Counter()
    for row in app_rows:
        category = classify(row)
        categories[category] += 1
        category_bytes[category] += credits.get(row['_key'], 0)
    class_counts = Counter(row.get('class', 'unclassified') for row in app_rows)

    strict_latest = {key for key, latest in latest_by_addr.items()
                     if not latest['conflict'] and result_status(key) in STRICT}
    matched_app = [row for row in app_rows if row['_key'] in strict_latest]
    runtime_rows = [row for row in inventory_by_addr.values()
                    if row.get('class') == 'runtime']
    matched_runtime = [row for row in runtime_rows if row['_key'] in strict_latest]

    def matched_result_bytes(rows):
        total = 0
        for row in rows:
            record = latest_by_addr[row['_key']]['record']['result']
            try:
                total += int_value(record.get('size', row.get('size', 0)))
            except (TypeError, ValueError):
                total += int_value(row.get('size', 0))
        return total

    def strict_at(addr):
        key = address_key(addr)
        return key in strict_latest

    images = patterns.get('images', [])
    fades = patterns.get('fades', [])
    image_matched = sum(strict_at(item.get('entry')) for item in images)
    fades_matched = sum(strict_at(item.get('entry')) for item in fades)

    segment_rows = []
    for seg, label in SEGMENTS:
        rows = [row for row in app_rows if row['_segment'] == seg]
        segment_rows.append((label, rows,
                             sum(credits.get(row['_key'], 0) for row in rows),
                             [row for row in rows if row['_key'] in strict_latest]))

    blocker_counts = Counter()
    for row in app_rows:
        if result_status(row['_key']) in STRICT:
            continue
        blocker = blocker_class(current_meta(row['_key']).get('blocker'))
        if blocker:
            blocker_counts[blocker] += 1

    # Coverage mask spans the stated code area; data islands count as classified data.
    code_rows = list(inventory_by_addr.values())
    classified = interval_mask(code_end, code_rows)
    data_mask = interval_mask(code_end, islands)
    classified = bytearray(a or b for a, b in zip(classified, data_mask))
    gaps = []
    pos = 0
    while pos < code_end:
        if classified[pos]:
            pos += 1
            continue
        start = pos
        while pos < code_end and not classified[pos]:
            pos += 1
        gaps.append((start, pos))
    gap_bytes = sum(end - start for start, end in gaps)

    out = [
        '# DAZZLE reconstruction status',
        '',
        '<!-- Auto-generated by tools/status.py from evidence/functions.json, '
        'evidence/patterns.json, and evidence/match/*.json. -->',
        '',
        f'Latest strict results: EXACT/RELOCATED only. Read {len(match_files)} match file(s); '
        f'{len(results_by_addr)} distinct result address(es).',
        '',
        '## Inventory and strict matches',
        '',
        '| Scope | Functions | Bytes | Strictly matched | Matched result bytes |',
        '|---|---:|---:|---:|---:|',
        f'| Runtime library | {format_num(len(runtime_rows))} | '
        f'{format_num(int_value(summary.get("runtime_bytes", 0)))} | '
        f'{format_num(len(matched_runtime))} | {format_num(matched_result_bytes(matched_runtime))} |',
        f'| DAZZLE application code, including uncertain/assembly starts | '
        f'{format_num(len(app_rows))} | {format_num(sum(credits.values()))} | '
        f'{format_num(len(matched_app))} | {format_num(matched_result_bytes(matched_app))} |',
        '',
        'Application denominator: ' + ', '.join(
            f'{format_num(class_counts.get(kind, 0))} {label}' for kind, label in (
                ('application', 'identified application starts'),
                ('asm_candidate', 'assembly candidates'),
                ('uncertain', 'uncertain starts'), ('glue', 'glue starts'))) +
        ', address-deduplicated. Application bytes are '
        'the union of these extents with data islands excluded. Runtime bytes use '
        'functions.json summary totals. Matched result bytes use each address latest result '

        'size and count only EXACT/RELOCATED.',
        '',
        '## Application status',
        '',
        '| Latest classification | Functions | Unique inventory bytes |',
        '|---|---:|---:|',
    ]
    for category in STATUS_ORDER:
        out.append(f'| {category} | {format_num(categories[category])} | '
                   f'{format_num(category_bytes[category])} |')
    out.extend(['', '## Code segments', '',
                '| Segment | Functions | Unique inventory bytes | Strictly matched | Matched result bytes |',
                '|---|---:|---:|---:|---:|'])
    for label, rows, byte_count, matched in segment_rows:
        out.append(f'| {label} | {format_num(len(rows))} | {format_num(byte_count)} | '
                   f'{format_num(len(matched))} | {format_num(matched_result_bytes(matched))} |')
    out.extend(['', '## Image algorithms and fades', '',
                f'- Image algorithms: **{image_matched}/{len(images)}** strict matches.',
                f'- Fades: **{fades_matched}/{len(fades)}** strict matches.',
                '', '## Top blocker classes', ''])
    if blocker_counts:
        out.extend(['| Blocker | Functions |', '|---|---:|'])
        for label, count in blocker_counts.most_common(10):
            safe_label = label.replace('|', '&#124;')
            out.append(f'| {safe_label} | {format_num(count)} |')
    else:
        out.append('No blocker metadata found.')
    out.extend(['', '## Unresolved code/data regions', '',
                f'Uncovered within load-image code area `[0, 0x{code_end:X})`: '
                f'**{format_num(gap_bytes)} bytes in {len(gaps)} region(s)**. '
                'Ranges are half-open load-image offsets; classified function extents and data '
                'islands are covered.', ''])
    if gaps:
        out.append('| Load-image range | Bytes |')
        out.append('|---|---:|')
        for start, end in gaps:
            out.append(f'| `0x{start:05X}..0x{end:05X}` | {format_num(end - start)} |')
    else:
        out.append('No uncovered ranges.')

    out.extend(['', '## Result conflicts', ''])
    if conflicts or inventory_conflicts:
        for key, labels in conflicts:
            out.append(f'- `{key}`: ' + '; '.join(labels))
        for key, left, right in inventory_conflicts:
            out.append(f'- Inventory `{key}`: conflicting class/size/name '
                       f'({left.get("class")}/{left.get("size")}/{left.get("name")} vs '
                       f'{right.get("class")}/{right.get("size")}/{right.get("name")}).')
    else:
        out.append('None.')
    if warnings:
        out.extend(['', '## Input notes', ''])
        out.extend(f'- {warning}' for warning in sorted(warnings))

    output = ROOT / 'docs/STATUS.md'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n'.join(out).rstrip() + '\n', encoding='utf-8', newline='\n')
    print(f'Wrote {output.relative_to(ROOT)}: {len(match_files)} match files, '
          f'{len(results_by_addr)} result addresses, {gap_bytes} uncovered bytes.')


if __name__ == '__main__':
    main()
