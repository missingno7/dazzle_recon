"""Relocation-aware 16-bit disassembler for the DAZZLE load image.

Usage:
  python tools/disasm.py SEG:OFF [-n BYTES | --func]
  python tools/disasm.py 0x1b94f -n 64          (linear load-image offset)

Addresses are load-image paragraphs (load segment 0). Words carrying an MZ
relocation are annotated as `<seg XXXX>` (their value is a target paragraph).
--func stops after the first RET/RETF/IRET that is not jumped over
(a heuristic; check the output).
"""
import argparse
import json
import pathlib
import struct

import capstone

ROOT = pathlib.Path(__file__).resolve().parent.parent
IMAGE = ROOT / 'reference' / 'load_image.bin'
ORACLE = ROOT / 'evidence' / 'oracle.json'


def relocation_sites():
    rel = json.loads(ORACLE.read_text())['relocations']['ordered_segment_offset']
    sites = set()
    for r in rel:
        if isinstance(r, str):
            seg, off = (int(x, 16) for x in r.split(':'))
        elif isinstance(r, dict):
            seg, off = r['segment'], r['offset']
        else:
            seg, off = r
        sites.add(seg * 16 + off)
    return sites


def parse_addr(text):
    if ':' in text:
        seg, off = (int(x, 16) for x in text.split(':'))
        return seg, off
    lin = int(text, 0)
    return lin >> 4, lin & 15


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('addr')
    ap.add_argument('-n', type=lambda s: int(s, 0), default=256)
    ap.add_argument('--func', action='store_true')
    a = ap.parse_args()
    image = IMAGE.read_bytes()
    sites = relocation_sites()
    seg, off = parse_addr(a.addr)
    base = seg * 16
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    limit = 0x10000 if a.func else a.n
    code = image[base + off: base + off + limit]
    furthest = off
    for ins in md.disasm(code, off):
        lin = base + ins.address
        notes = [f'<seg {struct.unpack_from("<H", image, s)[0]:04x}>'
                 for s in range(lin, lin + ins.size - 1) if s in sites]
        text = f'{seg:04x}:{ins.address:04x}  {lin:05x}  {ins.bytes.hex(" "):<24} {ins.mnemonic} {ins.op_str}'
        print(text + ('   ; ' + ' '.join(notes) if notes else ''))
        if a.func:
            if ins.mnemonic.startswith('j') and ins.op_str.startswith('0x'):
                furthest = max(furthest, int(ins.op_str, 16))
            if ins.mnemonic in ('ret', 'retf', 'iret') and ins.address >= furthest:
                break


if __name__ == '__main__':
    main()
