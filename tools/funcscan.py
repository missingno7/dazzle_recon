#!/usr/bin/env python3
"""Build a call/prologue/library-seeded inventory for DAZZLE's 16-bit code."""
from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

import capstone
from capstone.x86_const import (X86_INS_CALL, X86_INS_JMP, X86_INS_LCALL,
                                X86_INS_LJMP, X86_OP_IMM, X86_OP_MEM,
                                X86_OP_REG, X86_REG_BH, X86_REG_BP, X86_REG_BX,
                                X86_REG_CS, X86_REG_CX, X86_REG_DI, X86_REG_DX,
                                X86_REG_SI)

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
import libmatch

IMAGE = ROOT / 'reference' / 'load_image.bin'
SEGMENTS = ROOT / 'evidence' / 'segments.json'
DEFAULT_RUNTIME = ROOT / 'build' / 'workers' / 'rtlid' / 'bc30_large.json'
DEFAULT_STARTUP = pathlib.Path(r'C:\tools\borland-c++-3.0\files\LIB\LLIB\C0L.OBJ')
DEFAULT_OUTPUT = ROOT / 'evidence' / 'functions.json'
CODE_END = 0x1e810


def parse_relocations():
    oracle = json.loads((ROOT / 'evidence' / 'oracle.json').read_text(encoding='utf-8'))
    sites = set()
    for row in oracle['relocations']['ordered_segment_offset']:
        if isinstance(row, str):
            seg, off = (int(x, 16) for x in row.split(':'))
        elif isinstance(row, dict):
            seg, off = row['segment'], row['offset']
        else:
            seg, off = row
        sites.add(seg * 16 + off)
    return sites


def addr_text(linear, paragraph):
    return f'{paragraph:04X}:{(linear - paragraph * 16) & 0xffff:04X}'


def _ranges_from_evidence(segments):
    ranges = []
    for s in segments:
        start = s['byte_offset']
        size = s.get('size_estimate')
        if size is None or start >= CODE_END:
            continue
        end = min(CODE_END, start + size)
        if s['kind_guess'] == 'code':
            ranges.append([start, end, s['paragraph_base'], s['kind_guess']])
        elif start == 0xccd0:
            ranges.append([0xccda, end, s['paragraph_base'], 'mixed-code'])
        elif start == 0x1bac0:
            ranges.append([0x1bad2, end, s['paragraph_base'], 'mixed-code'])
    # The E87 program has a short 80-bit constant prefix, then handler code.
    ranges.append([0x1e27f, CODE_END, 0x1e24, 'mixed-code'])
    return ranges


def _contains(ranges, addr):
    return any(a <= addr < b for a, b, _, _ in ranges)


def _segment_paragraph(segments, ranges, addr):
    candidates = [s for s in segments
                  if s['byte_offset'] <= addr < s['byte_offset'] + (s.get('size_estimate') or 0)]
    if candidates:
        return max(candidates, key=lambda s: s['byte_offset'])['paragraph_base']
    candidates = [r for r in ranges if r[0] <= addr < r[1]]
    return candidates[-1][2] if candidates else (addr >> 4)


def _flow_target(ins, segment_base):
    b = bytes(ins.bytes)
    if ins.id in (X86_INS_LCALL, X86_INS_LJMP) and len(b) >= 5:
        off = int.from_bytes(b[-4:-2], 'little')
        seg = int.from_bytes(b[-2:], 'little')
        return seg * 16 + off
    if ins.id in (X86_INS_CALL, X86_INS_JMP) or ins.group(capstone.CS_GRP_JUMP):
        if not ins.operands or ins.operands[0].type != X86_OP_IMM:
            return None
        # 16-bit relative targets wrap within the current CS segment.
        if b and b[0] in (0xe8, 0xe9) and len(b) == 3:
            disp = int.from_bytes(b[1:3], 'little', signed=True)
            off = ((ins.address - segment_base) + len(b) + disp) & 0xffff
            return segment_base + off
        if b and (b[0] == 0xeb or 0x70 <= b[0] <= 0x7f or 0xe0 <= b[0] <= 0xe3) and len(b) == 2:
            disp = int.from_bytes(b[1:2], 'little', signed=True)
            off = ((ins.address - segment_base) + len(b) + disp) & 0xffff
            return segment_base + off
        return int(ins.operands[0].imm)
    return None


def _byte_ranges(values):
    values = sorted(set(values))
    if not values:
        return []
    out = []
    a = prev = values[0]
    for x in values[1:]:
        if x > prev + 1:
            out.append((a, prev + 1))
            a = x
        prev = x
    out.append((a, prev + 1))
    return out


def _union_size(ranges):
    spans = sorted((a, b) for a, b in ranges if b > a)
    total = 0
    end = -1
    for a, b in spans:
        if b <= end:
            continue
        total += b - max(a, end)
        end = b
    return total


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--runtime-report', type=pathlib.Path, default=DEFAULT_RUNTIME,
                    help='detailed JSON output from libmatch.py for the selected release/model')
    ap.add_argument('--startup', type=pathlib.Path, default=DEFAULT_STARTUP,
                    help='candidate model startup object (C0L.OBJ)')
    ap.add_argument('--output', type=pathlib.Path, default=DEFAULT_OUTPUT)
    args = ap.parse_args()

    image = IMAGE.read_bytes()
    segment_doc = json.loads(SEGMENTS.read_text(encoding='utf-8'))
    segments = segment_doc['segments']
    ranges = _ranges_from_evidence(segments)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    md.detail = True

    # Confirmed data islands in executable segment contributions. Entries are
    # also emitted as class=data records so they are excluded from code totals.
    islands = [
        {'start': 0xccc2, 'end': 0xccda, 'name': 'switch_offsets_0ccc2',
         'kind': '12-entry 16-bit near-offset table', 'paragraph': 0xb84,
         'targets': []},
        {'start': 0x1bac0, 'end': 0x1bad2, 'name': 'emu086_prefix_table',
         'kind': '18-byte runtime constant/index table', 'paragraph': 0x1bac,
         'targets': []},
        {'start': 0x1e240, 'end': 0x1e27f, 'name': 'e87_extended_constants',
         'kind': 'E87 80-bit constant prefix', 'paragraph': 0x1e24,
         'targets': []},
        {'start': 0x1e5f2, 'end': 0x1e606, 'name': 'e87_opcode_metadata',
         'kind': '20-byte E87 opcode metadata table', 'paragraph': 0x1e24,
         'targets': []},
    ]
    # EMU086's dispatch is `jmp cs:[bx+209Ch]` after zeroing BH and doubling BX.
    emu_table = {'start': 0x1bac0 + 0x209c, 'end': 0x1bac0 + 0x209c + 512,
                 'name': 'emu086_dispatch_offsets',
                 'kind': '256-entry 16-bit CS-relative dispatch table',
                 'paragraph': 0x1bac, 'targets': []}
    islands.append(emu_table)

    # The 8087 emulator also has two compact code-offset dispatch tables and a
    # one-byte opcode dispatch into E87 code. The latter is computed rather
    # than stored as a table, so its entries are added below as starts only.
    islands.extend([
        {'start': 0x1bac0 + 0x2520, 'end': 0x1bac0 + 0x2520 + 64,
         'name': 'emu087_call_offsets',
         'kind': '32-entry 16-bit CS-relative call-offset table',
         'paragraph': 0x1bac, 'targets': []},
        {'start': 0x1bac0 + 0x257b, 'end': 0x1bac0 + 0x257b + 14,
         'name': 'emu087_jump_offsets',
         'kind': '7-entry 16-bit CS-relative jump-offset table',
         'paragraph': 0x1bac, 'targets': []},
        {'start': 0x1bac0 + 0x2435, 'end': 0x1bac0 + 0x2435 + 32,
         'name': 'emu086_opcode_family_offsets',
         'kind': '16-entry 16-bit CS-relative opcode-family offset table',
         'paragraph': 0x1bac, 'targets': []},
        {'start': 0xad8, 'end': 0xae0, 'name': 'rtl_dispatch_offsets_00ad8',
         'kind': '4-entry 16-bit CS-relative dispatch table',
         'paragraph': 0, 'targets': []},
    ])
    known_island_ranges = [(x['start'], x['end']) for x in islands]

    # Recover ordinary compiler switch tables from the common sequence
    # `cmp index,limit; ja default; shl index,1; jmp/call cs:[index+table]`.
    # Only accept tables whose complete target vector lands in executable
    # ranges; dynamic vtable/search-loop references are deliberately excluded.
    indexed_regs = (X86_REG_BX, X86_REG_BP, X86_REG_DI, X86_REG_SI)
    discovered = {}
    for lo, hi, paragraph, _ in ranges:
        decoded = []
        p = lo
        while p < hi:
            skip_to = next((b for a, b in known_island_ranges if a <= p < b), None)
            if skip_to is not None:
                p = skip_to
                continue
            row = next(iter(md.disasm(image[p:min(hi, p + 15)], p, count=1)), None)
            if row is None:
                p += 1
                continue
            decoded.append(row)
            p += row.size
        for i, ins in enumerate(decoded):
            if ins.id not in (X86_INS_CALL, X86_INS_JMP, X86_INS_LCALL, X86_INS_LJMP) or \
                    not ins.operands or ins.operands[0].type != X86_OP_MEM:
                continue
            mem = ins.operands[0].mem
            if mem.segment != X86_REG_CS or mem.base not in indexed_regs:
                continue

            # Borland also emits linear key searches followed by an indirect
            # jump through the parallel target array: CX is the key count,
            # BX starts at the selector array, and the operand displacement
            # equals twice the key count.
            if mem.base == X86_REG_BX and (mem.disp & 0xffff) % 2 == 0:
                count = (mem.disp & 0xffff) // 2
                lo_i = max(0, i - 40)
                cx_i = next((j for j in range(i - 1, lo_i - 1, -1)
                             if decoded[j].mnemonic == 'mov' and len(decoded[j].operands) == 2 and
                             decoded[j].operands[0].type == X86_OP_REG and
                             decoded[j].operands[0].reg == X86_REG_CX and
                             decoded[j].operands[1].type == X86_OP_IMM and
                             decoded[j].operands[1].imm == count), None)
                bx_i = next((j for j in range(i - 1, cx_i, -1)
                             if decoded[j].mnemonic == 'mov' and len(decoded[j].operands) == 2 and
                             decoded[j].operands[0].type == X86_OP_REG and
                             decoded[j].operands[0].reg == X86_REG_BX and
                             decoded[j].operands[1].type == X86_OP_IMM), None) if cx_i is not None else None
                seq = decoded[bx_i + 1:i] if bx_i is not None else []
                if count and count <= 128 and bx_i is not None and \
                        any(x.mnemonic == 'loop' for x in seq) and \
                        any(x.mnemonic == 'add' and len(x.operands) == 2 and
                            x.operands[0].type == X86_OP_REG and
                            x.operands[0].reg == X86_REG_BX and
                            x.operands[1].type == X86_OP_IMM and x.operands[1].imm == 2
                            for x in seq):
                    table_start = paragraph * 16 + decoded[bx_i].operands[1].imm
                    table_end = table_start + count * 4
                    target_start = table_start + count * 2
                    target_addrs = [paragraph * 16 + int.from_bytes(
                        image[target_start + 2*n:target_start + 2*n + 2], 'little')
                        for n in range(count)] if table_end <= len(image) else []
                    if target_addrs and not any(table_start < b and a < table_end
                                                for a, b in known_island_ranges) and \
                            all(_contains(ranges, t) and not any(a <= t < b
                                for a, b in known_island_ranges) for t in target_addrs):
                        key = (table_start, table_end, paragraph)
                        discovered.setdefault(key, (target_addrs, ins.address, count * 2))

            shl_at = next((j for j in range(i - 1, max(-1, i - 6), -1)
                           if decoded[j].mnemonic in ('shl', 'sal') and
                           len(decoded[j].operands) >= 2 and
                           decoded[j].operands[0].type == X86_OP_REG and
                           decoded[j].operands[0].reg == mem.base and
                           decoded[j].operands[1].type == X86_OP_IMM and
                           decoded[j].operands[1].imm == 1), None)
            if shl_at is None:
                continue
            prior = decoded[max(0, shl_at - 8):shl_at]
            bound = None
            for j in range(len(prior) - 1, -1, -1):
                cmp_ins = prior[j]
                if cmp_ins.mnemonic != 'cmp' or len(cmp_ins.operands) != 2 or \
                        cmp_ins.operands[1].type != X86_OP_IMM:
                    continue
                lhs = cmp_ins.operands[0]
                if lhs.type == X86_OP_REG and lhs.reg == mem.base:
                    bound = cmp_ins.operands[1].imm
                    break
                # Some compiler output compares AX, then copies AX into BX.
                if lhs.type == X86_OP_REG and any(
                        x.mnemonic == 'mov' and len(x.operands) == 2 and
                        x.operands[0].type == X86_OP_REG and x.operands[0].reg == mem.base and
                        x.operands[1].type == X86_OP_REG and x.operands[1].reg == lhs.reg
                        for x in prior[j + 1:]):
                    bound = cmp_ins.operands[1].imm
                    break
            if bound is None or not 0 <= bound < 256:
                continue
            count = bound + 1
            table_start = paragraph * 16 + (mem.disp & 0xffff)
            table_end = table_start + count * 2
            if table_end > len(image) or any(table_start < b and a < table_end
                                             for a, b in known_island_ranges):
                continue
            target_addrs = []
            for n in range(count):
                off = int.from_bytes(image[table_start + 2*n:table_start + 2*n + 2], 'little')
                target = paragraph * 16 + off
                if not _contains(ranges, target) or any(a <= target < b
                                                        for a, b in known_island_ranges):
                    target_addrs = []
                    break
                target_addrs.append(target)
            if not target_addrs:
                continue
            discovered.setdefault((table_start, table_end, paragraph),
                                  (target_addrs, ins.address, 0))
    for (start, end, paragraph), (targets, ref, target_offset) in sorted(discovered.items()):
        row = {'start': start, 'end': end,
               'name': f'cs_keyed_dispatch_{start:05x}' if target_offset else
                       f'cs_switch_{start:05x}',
               'kind': f'{len(targets)}-entry CS-relative selector/offset table' if target_offset else
                       f'{len(targets)}-entry 16-bit CS-relative jump table',
               'paragraph': paragraph, 'targets': [], 'reference': ref,
               'target_offset': target_offset}
        islands.append(row)

    island_ranges = [(x['start'], x['end']) for x in islands]
    for island in islands:
        width = 2 if any(marker in island['kind'] for marker in
                          ('offset', 'dispatch table', 'jump table')) else 0
        if width:
            first = island.get('target_offset', 0)
            for pos in range(island['start'] + first, island['end'], width):
                off = int.from_bytes(image[pos:pos + width], 'little')
                target = island['paragraph'] * 16 + off
                if 0 <= target < CODE_END and _contains(ranges, target) and \
                        not any(a <= target < b for a, b in island_ranges):
                    island['targets'].append(target)

    # Full-extent invariant-byte hits seed known RTL names and code extents;
    # relocation fields are wildcards and are reported separately by libmatch.
    runtime_doc = json.loads(args.runtime_report.read_text(encoding='utf-8'))
    candidates = {}

    def add(addr, evidence, name=None, forced_class=None, confidence=None, extent=None):
        if not (0 <= addr < CODE_END):
            return
        if any(a <= addr < b for a, b in island_ranges):
            return
        row = candidates.setdefault(addr, {'evidence': set(), 'names': set(),
                                           'forced_class': None, 'confidence': None,
                                           'extents': []})
        row['evidence'].add(evidence)
        if name:
            row['names'].add(name)
        if forced_class:
            row['forced_class'] = forced_class
        if confidence:
            row['confidence'] = confidence
        if extent:
            row['extents'].append(extent)

    runtime_ranges = []
    runtime_function_spans = {}
    for lib in runtime_doc['libraries']:
        for member in lib['extent_hits']:
            for seg in member['segments']:
                hits = seg['full_extent_offsets']
                if not hits:
                    continue
                lo, hi = seg['emitted']
                for hit in hits:
                    base = hit - lo
                    runtime_ranges.append((base + lo, base + hi))
                    pubs = sorted((p['offset'], p['name']) for p in seg['pubdefs'])
                    unique_offsets = sorted(set(off for off, _ in pubs))
                    for off, name in pubs:
                        addr = base + off
                        if not (lo <= off < hi) or name in ('__MMODEL', 'DGROUP@'):
                            continue
                        next_off = next((q for q in unique_offsets if q > off), hi)
                        add(addr, f"rtl_member:{member['member']}", name,
                            'runtime', 'high', (addr, base + next_off))
                        runtime_function_spans.setdefault(addr, []).append((addr, base + next_off))

    # The startup object is not a library member, but it is Borland RTL code.
    startup_records = list(libmatch.omf_records(args.startup.read_bytes()))
    startup_name, startup_segments, _ = libmatch.parse_module(startup_records)
    for seg in startup_segments:
        if seg.cls != 'CODE' or not sum(seg.mask):
            continue
        emitted = [i for i, bit in enumerate(seg.mask) if bit]
        lo, hi = min(emitted), max(emitted) + 1
        runtime_ranges.append((lo, hi))
        pubs = sorted((o, n) for n, o in seg.pubdefs)
        offsets = sorted(set(o for o, _ in pubs))
        for off, name in pubs:
            if name in ('__MMODEL', 'DGROUP@') or not (lo <= off < hi):
                continue
            next_off = next((q for q in offsets if q > off), hi)
            add(off, 'c0_startup_pubdef', name, 'runtime', 'high', (off, next_off))
            runtime_function_spans.setdefault(off, []).append((off, next_off))
    add(0, 'mz_entry', 'C0_startup', 'runtime', 'high', (0, len(startup_segments[0].data)))

    # Jump-table targets are valid code labels even when they have no BP frame.
    jump_table_starts = []
    for idx, off in enumerate([int.from_bytes(image[0xccd0+i:0xccd2+i], 'little')
                               for i in range(0, 10, 2)]):
        target = 0xccd0 + off
        jump_table_starts.append(target)
        add(target, f'jump_table:0ccd0[{idx}]', None, None, 'medium')
    for idx, target_off in enumerate(emu_table['targets']):
        add(target_off, f'jump_table:emu086[{idx}]', None, 'runtime', 'medium')
    for island in islands:
        if island['name'].startswith(('emu087_', 'emu086_opcode_', 'rtl_dispatch_', 'cs_switch_',
                                      'cs_keyed_dispatch_')):
            for idx, target_off in enumerate(island['targets']):
                add(target_off, f"jump_table:{island['name']}[{idx}]", None, None, 'medium')
    for opcode in range(0xe6, 0x100):
        target = 0x1bac0 + 0x26e0 + opcode
        add(target, f'computed_dispatch:emu087_opcode[{opcode:02x}]', None, None, 'medium')

    def in_island(addr):
        return any(a <= addr < b for a, b in island_ranges)

    def linearsweep(lo, hi, paragraph):
        p = lo
        segbase = paragraph * 16
        while p < hi:
            if in_island(p):
                p = next(b for a, b in island_ranges if a <= p < b)
                continue
            insns = list(md.disasm(image[p:min(hi, p + 15)], p, count=1))
            if not insns:
                p += 1
                continue
            ins = insns[0]
            if ins.id in (X86_INS_CALL, X86_INS_LCALL):
                target = _flow_target(ins, segbase)
                if target is not None and _contains(ranges, target) and not in_island(target):
                    before = image[p - 1] == 0x0e if p > lo else False
                    evidence = 'push_cs_call_near' if before and ins.id == X86_INS_CALL else (
                        'far_call' if ins.id == X86_INS_LCALL else 'near_call')
                    add(target, f'{evidence}:{p:05x}', None, None, 'high')
            p += max(ins.size, 1)

    for lo, hi, paragraph, _ in ranges:
        linearsweep(lo, hi, paragraph)

    # Source-level frame prologues, plus ENTER, are high-quality starts.
    for lo, hi, _, _ in ranges:
        for pos in range(lo, hi - 2):
            if in_island(pos):
                continue
            if image[pos:pos + 3] == b'\x55\x8b\xec':
                add(pos, 'prologue:push_bp_mov_bp_sp', None, None, 'high')
            elif image[pos] == 0xc8 and pos + 4 <= hi:
                add(pos, 'prologue:enter', None, None, 'medium')

    # Decode candidate functions and discover nested direct call targets.
    analyses = {}
    pending = list(candidates)
    processed = set()
    while pending:
        start = pending.pop()
        if start in processed or not _contains(ranges, start) or in_island(start):
            continue
        processed.add(start)
        paragraph = _segment_paragraph(segments, ranges, start)
        segbase = paragraph * 16
        queue = [start]
        seen = set()
        insns = []
        callrefs = []
        has_return = False
        high = start
        budget = 0
        while queue and budget < 4096:
            pc = queue.pop()
            while _contains(ranges, pc) and not in_island(pc) and pc not in seen and budget < 4096:
                row = list(md.disasm(image[pc:min(CODE_END, pc + 15)], pc, count=1))
                if not row:
                    break
                ins = row[0]
                seen.add(pc)
                insns.append(ins)
                budget += 1
                high = max(high, pc + ins.size)
                if ins.id in (X86_INS_CALL, X86_INS_LCALL):
                    target = _flow_target(ins, segbase)
                    if target is not None and _contains(ranges, target) and not in_island(target):
                        callrefs.append((target, pc, 'far_call' if ins.id == X86_INS_LCALL else 'near_call'))
                        if target not in candidates:
                            add(target, f"{callrefs[-1][2]}:{pc:05x}", None, None, 'high')
                            pending.append(target)
                    pc += ins.size
                    continue
                if ins.mnemonic in ('ret', 'retf', 'iret'):
                    has_return = True
                    break
                branch = ins.group(capstone.CS_GRP_JUMP)
                if branch:
                    target = _flow_target(ins, segbase)
                    if target is not None and _contains(ranges, target) and not in_island(target):
                        # Keep intra-function branches; an already known start is a tail edge.
                        if ins.id == X86_INS_JMP and target in candidates and target != start:
                            break
                        queue.append(target)
                    if ins.id in (X86_INS_JMP, X86_INS_LJMP):
                        break
                pc += ins.size
        analyses[start] = {'end': high, 'has_return': has_return,
                           'instructions': insns, 'calls': callrefs,
                           'visited': seen, 'paragraph': paragraph}

    # Recheck entry points discovered during CFG walks and analyze any new ones.
    new = [a for a in candidates if a not in processed and _contains(ranges, a)]
    for addr in new:
        # The outer work list can gain candidates late; a small recursive pass is
        # sufficient because linear sweep already supplied all ordinary calls.
        paragraph = _segment_paragraph(segments, ranges, addr)
        segbase = paragraph * 16
        row = next(iter(md.disasm(image[addr:min(CODE_END, addr + 15)], addr, count=1)), None)
        if row:
            analyses[addr] = {'end': addr + row.size, 'has_return': row.mnemonic in ('ret','retf','iret'),
                              'instructions': [row], 'calls': [], 'visited': {addr},
                              'paragraph': paragraph}

    # Gather code features for assembly triage and choose confidence/class.
    entries = []
    runtime_starts = set(runtime_function_spans)
    for addr, c in candidates.items():
        ana = analyses.get(addr, {'end': addr + 1, 'has_return': False,
                                  'instructions': [], 'calls': [], 'visited': {addr},
                                  'paragraph': _segment_paragraph(segments, ranges, addr)})
        instructions = ana['instructions']
        names = sorted(c['names'])
        paragraph = ana['paragraph']
        next_strong = min((x for x, other in candidates.items()
                           if x > addr and _segment_paragraph(segments, ranges, x) == paragraph
                           and (other['names'] or any(e.startswith('prologue:') for e in other['evidence']))),
                          default=CODE_END)
        end = min(max(addr + 1, ana['end']), next_strong)
        for a, b in c['extents']:
            if a == addr and b > a:
                end = min(max(end, b), next_strong) if ana['has_return'] else min(b, next_strong)
        if end <= addr:
            end = addr + 1

        has_bp = any(i.mnemonic == 'push' and i.op_str == 'bp' for i in instructions[:5]) and \
                 any(i.mnemonic == 'mov' and i.op_str.replace(' ', '') in ('bp,sp','ebp,esp')
                     for i in instructions[:6])
        has_enter = any(i.mnemonic == 'enter' for i in instructions[:3])
        strings = sorted({i.mnemonic for i in instructions
                          if i.mnemonic in ('movsb','movsw','cmpsb','cmpsw','stosb','stosw',
                                            'lodsb','lodsw','scasb','scasw','insb','insw','outsb','outsw')})
        port_imm = []
        dx_port = None
        for i in instructions:
            if i.mnemonic == 'mov' and i.operands and i.operands[0].type != X86_OP_IMM:
                if i.operands[0].reg == X86_REG_DX and len(i.operands) > 1 and i.operands[1].type == X86_OP_IMM:
                    dx_port = i.operands[1].imm & 0xffff
            if i.mnemonic in ('in','out') and dx_port in (0x3c4,0x3ce,0x3d4,0x3c8,0x3c9):
                port_imm.append(dx_port)
        evidence = sorted(c['evidence'])
        runtime_hit = addr in runtime_starts or any(a <= addr < b for a, b in runtime_ranges)
        if c['forced_class'] == 'runtime' or runtime_hit:
            cls = 'runtime'
            confidence = c['confidence'] or ('high' if names else 'medium')
        elif c['forced_class'] == 'glue':
            cls, confidence = 'glue', c['confidence'] or 'medium'
        elif not ana['has_return']:
            cls, confidence = 'uncertain', 'low'
        elif strings or port_imm or (not has_bp and not has_enter and len(instructions) > 8
                                     and any(i.mnemonic in ('rep','cld','std','xlatb','in','out')
                                             for i in instructions)):
            cls, confidence = 'asm_candidate', 'medium'
        elif not has_bp and not has_enter and end - addr <= 16 and any('call' in e for e in evidence):
            cls, confidence = 'glue', 'medium'
        elif any('near_call' in e or 'far_call' in e or 'push_cs_call_near' in e for e in evidence):
            cls, confidence = 'application', 'high'
        elif any(e.startswith('prologue:') for e in evidence):
            cls, confidence = 'application', 'medium'
        elif any('jump_table:' in e for e in evidence):
            cls, confidence = 'uncertain', 'low'
        else:
            cls, confidence = 'uncertain', 'low'
        name = '|'.join(names) if names else f'f_{paragraph:04X}_{(addr - paragraph * 16) & 0xffff:04X}'
        ev = list(evidence)
        if strings:
            ev.append('string_instruction:' + ','.join(strings))
        if port_imm:
            ev.append('vga_port_io:' + ','.join(f'{p:04x}' for p in sorted(set(port_imm))))
        if not has_bp and not has_enter:
            ev.append('no_bp_frame_in_decoded_prefix')
        ev.append('ret_reachable' if ana['has_return'] else 'no_return_found')
        if any('jump_table:' in e for e in evidence):
            ev.append('dispatch_table_target')
        overlaps = [(max(addr, d['start']), min(end, d['end'])) for d in islands
                    if addr < d['end'] and d['start'] < end]
        data_overlap_bytes = _union_size(overlaps)
        if data_overlap_bytes:
            ev.extend(f"overlaps_data_island:{d['name']}" for d in islands
                      if addr < d['end'] and d['start'] < end)
        entries.append({'addr': addr_text(addr, paragraph), 'linear': addr,
                        'size': end - addr, 'end': end,
                        'segment_paragraph': paragraph, 'name': name,
                        'class': cls, 'confidence': confidence,
                        'data_overlap_bytes': data_overlap_bytes,
                        'evidence': sorted(set(ev))})

    # Add explicit data islands to the same canonical list and a dedicated index.
    data_entries = []
    for island in islands:
        a, b, paragraph = island['start'], island['end'], island['paragraph']
        row = {'addr': addr_text(a, paragraph), 'linear': a, 'size': b-a, 'end': b,
               'segment_paragraph': paragraph, 'name': island['name'], 'class': 'data',
               'confidence': 'high',
               'evidence': [island['kind'], 'excluded_from_function_bytes']}
        entries.append(row)
        data_entries.append({**row, 'targets': [f'{x:05x}' for x in island['targets']]})
    data_entries.sort(key=lambda x: x['linear'])

    # Collapse duplicate starts from overlapping runtime libraries or aliases.
    merged = {}
    for row in entries:
        merged.setdefault(row['linear'], row)
        prev = merged[row['linear']]
        if row['class'] == 'data':
            merged[row['linear']] = row
        elif prev['class'] != 'data':
            if prev['name'].startswith('f_') and not row['name'].startswith('f_'):
                prev['name'] = row['name']
            elif row['name'] not in prev['name'].split('|'):
                prev['name'] += '|' + row['name']
            prev['evidence'] = sorted(set(prev['evidence']) | set(row['evidence']))
            if row['class'] == 'runtime' or prev['class'] in ('uncertain','application'):
                prev['class'] = row['class']
                prev['confidence'] = row['confidence']
            prev['end'] = max(prev['end'], row['end'])
            prev['size'] = prev['end'] - prev['linear']
    entries = sorted(merged.values(), key=lambda x: x['linear'])

    function_entries = [x for x in entries if x['class'] != 'data']
    runtime_rows = [x for x in function_entries if x['class'] == 'runtime']
    app_rows = [x for x in function_entries if x['class'] == 'application']
    asm_rows = [x for x in function_entries if x['class'] == 'asm_candidate']
    uncertain_rows = [x for x in function_entries if x['class'] == 'uncertain']
    glue_rows = [x for x in function_entries if x['class'] == 'glue']
    data_bytes = _union_size([(x['linear'],x['end']) for x in data_entries])
    def code_bytes_without_islands(spans):
        spans = list(spans)
        total = _union_size(spans)
        cuts = [(max(a, island['linear']), min(b, island['end']))
                for a, b in spans for island in data_entries
                if a < island['end'] and island['linear'] < b]
        return max(0, total - _union_size(cuts))

    # Count identified RTL code by full-extent candidate spans and C0 startup,
    # not by summing public-symbol spans. Aliases, duplicate emulator members,
    # and embedded tables are collapsed or excluded.
    runtime_bytes = code_bytes_without_islands(runtime_ranges)
    runtime_entry_span_bytes = code_bytes_without_islands(
        (x['linear'], x['end']) for x in runtime_rows)
    application_bytes = code_bytes_without_islands(
        (x['linear'], x['end']) for x in app_rows)
    summary = {'code_area_end': f'0x{CODE_END:05x}',
               'runtime_functions': len(runtime_rows), 'runtime_bytes': runtime_bytes,
               'runtime_entry_span_bytes': runtime_entry_span_bytes,
               'runtime_public_symbols': sum(not x['name'].startswith('f_')
                                             for x in runtime_rows),
               'runtime_dispatch_targets': sum(any(e.startswith('jump_table:emu086[')
                                                   for e in x['evidence'])
                                               for x in runtime_rows),
               'application_functions': len(app_rows), 'application_bytes': application_bytes,
               'asm_candidates': len(asm_rows),
               'asm_candidate_bytes': code_bytes_without_islands(
                   (x['linear'], x['end']) for x in asm_rows),
               'uncertain': len(uncertain_rows),
               'uncertain_bytes': code_bytes_without_islands(
                   (x['linear'], x['end']) for x in uncertain_rows),
               'glue_functions': len(glue_rows),
               'glue_bytes': code_bytes_without_islands(
                   (x['linear'], x['end']) for x in glue_rows),
               'data_in_code_bytes': data_bytes,
               'runtime_member_full_extent_hits': runtime_doc['totals']['full_extent_hit_members'],
               'runtime_target_checks_passed': runtime_doc['totals']['resolved_external_target_checks'],
               'runtime_target_mismatches': runtime_doc['totals']['target_mismatches']}
    output = {'schema': 1, 'image_size': len(image), 'code_area_end': f'0x{CODE_END:05x}',
              'memory_model': 'large', 'runtime_candidate': 'Borland C++ 3.0 / Turbo C++ 3.0 large-model RTL',
              'summary': summary, 'functions': entries, 'data_islands': data_entries}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(summary, indent=2))


if __name__ == '__main__':
    main()
