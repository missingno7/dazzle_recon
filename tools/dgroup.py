"""Read-only data ownership census; writes only evidence/dgroup.json.

python -I tools/dgroup.py
Locations and names are identification evidence, never strict match acceptance.
Compiler probe receipts live in build/workers/s-data/.
"""
import hashlib
import json
import re
import sys
from collections import defaultdict, Counter
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from objcmp import Object
from rtldiff import objects

DG, FAR = 0x29bf0, 0x1e810
LIB = Path('C:/tools/borland-c++-3.0/files/LIB')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read(path):
    return json.loads((ROOT / path).read_text(encoding='utf-8'))


def main():
    image = (ROOT / 'reference/load_image.bin').read_bytes()
    oracle = read('evidence/oracle.json')
    assert sha(image) == read('evidence/segments.json')['load_image']['sha256']
    relocs = {16*r['segment']+r['offset'] for r in oracle['relocations']['ordered_segment_offset']}
    inventory = read('evidence/seg0-residue.json')
    library_paths = [LIB / p for p in ('LLIB/CL.LIB', 'LLIB/MATHL.LIB', 'XLIB/EMU.LIB')]
    obs, names, hashes = {}, {}, {}
    for path in library_paths:
        for name, page, obj, digest in objects(path):
            key = (str(path), page)
            obs[key], names[key], hashes[key] = obj, name, digest
    c0 = (str(LIB / 'LLIB/C0L.OBJ'), 0)
    raw = Path(c0[0]).read_bytes()
    obs[c0], names[c0], hashes[c0] = Object(raw), 'C0L', sha(raw)
    placements, reasons = defaultdict(dict), defaultdict(list)
    for r in inventory['members']:
        key = (r.get('library', c0[0]), r['page'])
        assert hashes[key] == r['object_sha256']
        placements[key][r['segment_index']] = r['base']
    for name, base in [('EMU086', 0x1bac0), ('EMU087', 0x1e240), ('E87TRAP', 0x1e630)]:
        key = next(k for k in obs if names[k] == name and k[0].endswith('EMU.LIB'))
        si = next(i for i,s in enumerate(obs[key].segments) if s and s['class']=='CODE' and s['length'])
        placements[key][si] = base
    dg_names = {n for o in obs.values() for n,(_,_,g) in o.publics.items()
                if g and o.groups[g][0] == 'DGROUP'}
    exports, constraints = defaultdict(set), defaultdict(set)
    for iteration in range(8):
        for key, ps in list(placements.items()):
            o = obs[key]
            for n,(si,off,g) in o.publics.items():
                if si in ps:
                    exports[n].add(ps[si]+off)
            for si,base in list(ps.items()):
                s = o.segments[si]
                for f in o.fixups:
                    if f.segment != si or f.relative:
                        continue
                    at = base+f.offset
                    raw = int.from_bytes(s['data'][f.offset:f.offset+2], 'little')
                    actual = None
                    if f.location == 3 and (not f.offset or s['data'][f.offset-1] not in (0x9a,0xea)
                                            or image[at-1] != 0x90):
                        actual = int.from_bytes(image[at:at+2], 'little') + 16*int.from_bytes(image[at+2:at+4], 'little')
                    elif f.location == 1 and (
                        (f.frame[0]==1 and o.groups[f.frame[1]][0]=='DGROUP') or
                        (f.frame[0]==5 and f.target[0]==2 and o.externals[f.target[1]] in dg_names) or
                        (f.frame[0]==0 and any(f.frame[1] in g[1] for g in o.groups[1:] if g[0]=='DGROUP'))):
                        actual = DG+int.from_bytes(image[at:at+2], 'little')
                    if actual is None:
                        continue
                    val = actual-raw-f.displacement
                    if f.location == 1:
                        val = DG+((val-DG)&65535)
                    ev = {'site_linear':at, 'source_member':names[key], 'source_segment':s['name'],
                          'fixup_offset':f.offset, 'location':f.location,
                          'raw_addend':raw, 'displacement':f.displacement}
                    if f.target[0] == 0:
                        target = (key, f.target[1])
                        constraints[target].add(val)
                        assert len(constraints[target]) == 1, (target, constraints[target])
                        ps[f.target[1]] = val
                        if ev not in reasons[target]:
                            reasons[target].append(ev)
                    elif f.target[0] == 2:
                        n = o.externals[f.target[1]]
                        exports[n].add(val)
                        if ev not in reasons[n]:
                            reasons[n].append(ev)
            # Bring in data-only members through independently bound PUBDEFs.
        for key,o in obs.items():
            for n,(si,off,g) in o.publics.items():
                if len(exports[n]) == 1 and o.segments[si]['class'] != 'CODE':
                    placements[key].setdefault(si, next(iter(exports[n]))-off)
    assert not [n for n,v in exports.items() if len(v)>1]

    # Combined auxiliary segments, independently identified by their code targets.
    fixed = [('C0L','_STACK',DG+0x78d0), ('EMUVARS','_STACK',DG+0x78d0),
             ('FPSTKLEN','_DATA',DG+0x45b2), ('DEFLT87','_DATA',DG+0x4114),
             ('REALCVT','_CVTSEG',DG+0x47ec), ('CVTFAK','_CVTSEG',DG+0x47ee),
             ('SCANTOD','_SCNSEG',DG+0x47f0), ('CVTFAK','_SCNSEG',DG+0x47f6),
             ('FPINIT','_EXIT_',DG+0x4826)]
    fixed += [(n,'_INIT_',DG+0x47fc+6*i) for i,n in enumerate(
        ['FPINIT','SETUPIO','CRTINIT','DELAY','SETARGV','SETENVP','TZSET'])]
    for name,seg,base in fixed:
        keys = [k for k,o in obs.items() if names[k]==name and
                any(s['name']==seg and s['length'] for s in o.segments[1:])]
        # CL and MATHL both have SCANTOD; only MATHL owns _SCNSEG.
        assert len(keys)==1, (name,seg,keys)
        key=keys[0]; si=next(i for i,s in enumerate(obs[key].segments) if s and s['name']==seg)
        placements[key][si]=base
        reasons[key,si].append({'basis':'auxiliary segment signature / code-target record; see docs/data-layout.md'})
    # STKLEN's stock value is 0x1000. The application overrides it with 0x3000.
    rejected = []
    for key in list(placements):
        if names[key]=='STKLEN':
            rejected.append({'member':'CL.LIB/STKLEN','proposed_ds':0x94,
                             'object_hex':'00 10','oracle_hex':image[DG+0x94:DG+0x96].hex(' '),
                             'outcome':'rejected; application __stklen override'})
            del placements[key]
    for key,ps in placements.items():
        for n,(si,off,g) in obs[key].publics.items():
            if si in ps:
                exports[n].add(ps[si]+off)

    def target(o,ps,method,index):
        if method==0:
            return ps.get(index)
        if method==1 and o.groups[index][0]=='DGROUP':
            return DG
        if method==2 and len(exports[o.externals[index]])==1:
            return next(iter(exports[o.externals[index]]))
        return None

    def data_check(key,si,base):
        o,ps=obs[key],placements[key]; s=o.segments[si]
        raw=bytearray(s['data']); mask=bytearray(s['present']); results=[]
        for f in o.fixups:
            if f.segment!=si:
                continue
            width={0:1,1:2,2:2,3:4,4:1}[f.location]
            mask[f.offset:f.offset+width]=bytes(width)
            t=target(o,ps,*f.target); fm,fi=f.frame; frame=None
            if fm==1 and o.groups[fi][0]=='DGROUP': frame=DG
            elif fm==0:
                frame=DG if any(fi in g[1] for g in o.groups[1:] if g[0]=='DGROUP') else 0 if o.segments[fi]['name']=='_TEXT' else (ps.get(fi,0)//16)*16
            elif fm==5 and t is not None:
                frame=DG if DG<=t<DG+0x78d0 else 0 if t<0x66ab else t//16*16
            actual=image[base+f.offset:base+f.offset+width]
            resolved=None
            if t is not None and frame is not None and f.location in (1,2,3):
                add=int.from_bytes(raw[f.offset:f.offset+2],'little')
                off=((t-frame)+f.displacement+add)&65535
                seg=frame//16
                if f.location==1: resolved=off.to_bytes(2,'little')
                if f.location==2: resolved=((seg+add)&65535).to_bytes(2,'little')
                if f.location==3:
                    segadd=int.from_bytes(raw[f.offset+2:f.offset+4],'little')
                    resolved=off.to_bytes(2,'little')+((seg+segadd)&65535).to_bytes(2,'little')
            relocation_ok = (base+f.offset+(2 if f.location==3 else 0) in relocs) if f.location in (2,3) else True
            results.append({'offset':f.offset,'location':f.location,'target_linear':t,
                            'result':'unresolved' if resolved is None else 'pass' if resolved==actual and relocation_ok else 'FAIL',
                            'mz_relocation_ok':relocation_ok})
        diffs=[i for i,v in enumerate(mask) if v and raw[i]!=image[base+i]]
        return {'emitted_invariant_bytes':sum(mask),'invariant_differences':diffs,
                'unemitted_bytes':s['present'].count(0),'fixups':results,
                'note':'Identification cross-check, not an acceptance certificate; source code locations seed bindings.'}

    contrib=[]
    def add(space,start,end,owner,seg,kind,name,confidence='medium',evidence=None,**more):
        if start==end:return
        ident=f'{space}:{start:04X}:{owner}:{seg}'
        contrib.append(dict(id=ident,space=space,start=start,end_exclusive=end,size=end-start,
                            owner=owner,object_segment=seg,kind=kind,proposed_name=name,
                            confidence=confidence,proof_level='PROVISIONAL' if owner!='UNKNOWN' else 'UNKNOWN',
                            evidence=evidence or ['docs/data-layout.md; source DS-reference bands and contiguous link order'],**more))

    for key,ps in placements.items():
        o=obs[key]
        for si,base in ps.items():
            s=o.segments[si]
            if s['class']=='CODE' or not s['length'] or names[key]=='EMUVARS':continue
            if not DG<=base<DG+0x79e0:continue
            owner='C0L' if key==c0 else Path(key[0]).name+'/'+names[key]
            pubs=[{'name':n,'offset':off,'ds_offset':base-DG+off} for n,(idx,off,g) in o.publics.items() if idx==si]
            ev=reasons[key,si]+[e for n,(idx,_,_) in o.publics.items() if idx==si for e in reasons[n]]
            if names[key]=='FPSTKLEN':
                ev=[{'basis':'CC 00 cell at DS:45B2 between independently located FMODE and IOERROR; stock library extraction adjacency reproduced in TLINK probe. Symbol not independently bound.'}]
            add('DGROUP',base-DG,base-DG+s['length'],owner,s['name'],
                'bss' if s['class']=='BSS' else 'stack' if s['class']=='STACK' else 'table' if s['name']!='_DATA' else 'initialized_storage',
                names[key]+'_'+s['name'].strip('_'), 'medium' if names[key]=='FPSTKLEN' else 'high', ev,
                library=key[0],library_page=key[1],object_sha256=hashes[key],
                publics=pubs,comparison=data_check(key,si,base) if s['class']!='STACK' else None)
    for owner,a,b in [('066A',0x94,0xa38),('0B84',0xa38,0xa50),('0CCD',0xa50,0xbfe),
                      ('11C1',0xbfe,0x249d),('158D',0x249e,0x39a4),('17F5',0x39a4,0x3bc6),
                      ('1899',0x3bc6,0x3c66),('19F0',0x3c66,0x3d66),('1A72',0x3d66,0x40f5)]:
        add('DGROUP',a,b,owner,'_DATA','initialized_storage','module_'+owner+'_initialized_data_and_pool')
    for owner,a,b in [('066A',0x482c,0x6031),('0B84',0x6032,0x60f8),('0CCD',0x60f8,0x7438),
                      ('11C1',0x7438,0x7452),('17F5',0x7456,0x7480),('1899',0x7480,0x7756),
                      ('19F0',0x7756,0x7766)]:
        add('DGROUP',a,b,owner,'_BSS','bss','module_'+owner+'_uninitialized_globals')
    add('DGROUP',0x7452,0x7456,'UNKNOWN','_BSS','bss','unresolved_DOSI_DLOGO_boundary', 'low',
        ['REGPACK begins 743E and needs 20 bytes through 7451; first proved DLOGO reference is 7456.'])
    add('DGROUP',0x7950,0x79e0,'MATHL.LIB/EMUVARS','_STACK','stack','emulator_common_stack_extension','high',
        ['EMUVARS SEGDEF 78 10 01: paragraph, COMMON, length 0110; __emu segment relocation at 0000:02AD resolves to 314C.',
         'TLINK 5.0 floating-point link probe: C0L STACK 0080 + EMUVARS COMMON 0110 gives one 0110-byte stack, __emu at offset zero.'])
    # All remaining physical gaps are explicit linker padding, not hidden owners.
    for space,limit in [('DGROUP',0x79e0)]:
        cursor=0
        for r in sorted([r for r in contrib if r['space']==space],key=lambda r:r['start']):
            assert cursor<=r['start'], ('overlap',cursor,r)
            if cursor<r['start']:
                add(space,cursor,r['start'],'TLINK','alignment','padding',f'padding_{cursor:04x}',
                    'medium',['OMF contribution extent and next contribution alignment; oracle bytes zero where serialized'])
            cursor=r['end_exclusive']
        assert cursor==limit
    # The relocation frame is shared; original far SEGDEF names/combine flags remain open.
    far_items=[(0,0x1d4,'158D','menu_error_screen_buffer_3x78_words','medium'),
               (0x1d4,0x1d8,'158D','directory_list_tail','high'),
               (0x1d8,0x203,'158D','directory_find_block','high'),
               (0x203,0x207,'158D','edited_image_record_base','high'),
               (0x207,0x257,'158D','directory_path_80','high'),
               (0x257,0x259,'158D','edited_image_count','high'),
               (0x259,0x25a,'UNKNOWN','far_contribution_alignment_or_storage','low'),
               (0x25a,0x740,'1A72','huff_son_627_words','high'),
               (0x740,0xe9a,'1A72','huff_parent_941_words','high'),
               (0xe9a,0x1382,'1A72','huff_frequency_628_words','high'),
               (0x1382,0x2383,'1A72','prefix_length_4097_bytes','high'),
               (0x2383,0x6385,'1A72','right_child_8193_words','high'),
               (0x6385,0x8387,'1A72','parent_node_4097_words','high'),
               (0x8387,0xa389,'1A72','left_child_4097_words','high'),
               (0xa389,0xa38b,'1A72','match_length','high'),
               (0xa38b,0xa38d,'1A72','match_position','high'),
               (0xa38d,0xb3c8,'1A72','dictionary_4155_bytes','high'),
               (0xb3c8,0xb3cc,'1A72','progress_step','high'),
               (0xb3cc,0xb3d0,'1A72','progress_next','high'),
               (0xb3d0,0xb3d4,'1A72','compressed_size','high'),
               (0xb3d4,0xb3d8,'1A72','expanded_size','high'),
               (0xb3d8,0xb3dc,'1A72','lzh_output','high'),
               (0xb3dc,0xb3e0,'1A72','lzh_input','high')]
    for a,b,owner,name,confidence in far_items:
        ev=['src/seg1a72.c objcmp symbol bindings and array dimensions; DHUF indexed access extents'] if owner=='1A72' else [
            '158D:005D/0074 tail; 0113 findfirst buffer; 0D86 image table; 174C/1F0C getcwd; 22CD count; 2505/265E screen buffer.']
        add('1E81',a,b,owner,'FAR_DATA/FAR_BSS unresolved','bss' if owner!='UNKNOWN' else 'unknown',name,confidence,ev)
    assert not any(image[FAR:DG])

    # Point symbols do not imply an object size. Cross-module references never establish definition ownership.
    symbols=[]
    for path in ('evidence/names-startup.json','evidence/names-graphics.json'):
        for row in read(path):
            if row['addr'].lower().startswith(('29bf:','ds:')):
                symbols.append({'space':'DGROUP','offset':int(row['addr'].split(':')[1],16),
                                'name':row['proposed_name'],'kind':row['kind'],'confidence':row['confidence'],
                                'evidence':path+': '+row['evidence'], 'type':row.get('type')})
    report_status={}
    for path in (ROOT/'evidence/match').glob('*.json'):
        for r in json.loads(path.read_text(encoding='utf-8')).get('results',[]):
            report_status[r['address'].lower(),r['function']]=r['status']
    bindings=[]
    for path in sorted([*(ROOT/'src').rglob('*.c'),*(ROOT/'asm').glob('*.asm')]):
        if path.parent.name=='probes':continue
        source=path.read_text(encoding='utf-8')
        for match in re.finditer(r'/\*\s*objcmp:\s*(\{.*?\})\s*\*/',source,re.S):
            meta=json.loads(match.group(1)); address=meta.get('address','').lower()
            site={'source':path.relative_to(ROOT).as_posix(),'line':source[:match.start()].count('\n')+1,
                  'function':meta.get('function'),'address':address,
                  'function_status':report_status.get((address,meta.get('function')),'PROVISIONAL')}
            if meta.get('segments'):
                bindings.append(dict(site,segments=meta['segments'],caveat='Candidate object contribution binding, not original TU start.'))
            for n,v in meta.get('symbols',{}).items():
                seg,off=v.lower().split(':')
                if seg in ('ds','29bf','1e81'):
                    symbols.append(dict(site,space='1E81' if seg=='1e81' else 'DGROUP',offset=int(off,16),
                                        name=n,kind='source_binding',confidence='medium',extent='unknown'))

    # Explicit initialized subobjects and uninterrupted C-string spans.
    items=defaultdict(list)
    def item(a,b,name,kind='table',confidence='high',evidence='Static bytes plus named source bindings'):
        items['DGROUP'].append(dict(start=a,end_exclusive=b,proposed_name=name,kind=kind,confidence=confidence,evidence=[evidence]))
    for a,b,n in [(0x94,0x96,'application_stklen_0x3000'),(0x96,0x9a,'executable_name_pointer'),
                  (0x9a,0x9e,'program_name_pointer'),(0x9e,0xa2,'revision_pointer'),(0xa2,0xa6,'registration_notice_pointer')]:
        item(a,b,n,'initialized_var')
    for a,b,n in [(0x1fe,0x41a,'alt_image_records_36x15'),(0x41a,0x474,'ctrl_image_records_6x15'),
                  (0x474,0x51c,'fade_records_24x7'),(0x24b8,0x24e5,'help_pages_5x9'),
                  (0x24e5,0x2512,'configuration_pages_5x9'),(0x3d66,0x3e66,'position_code_256'),
                  (0x3e66,0x3f66,'position_length_256'),(0x3f69,0x3f8b,'bit_mask_17_words'),
                  (0x3f8b,0x3fad,'bit_shift_17_words')]:item(a,b,n)
    for a,b,n in [(0xa6,0xe6,'DZLE_direction_table'),(0xe6,0x1e6,'DZLE_byte_table'),
                  (0xa38,0xa40,'DZL2_direction_table_0'),(0xa40,0xa48,'DZL2_direction_table_1'),
                  (0xa50,0xa8c,'DZFD_color_index_tables'),(0xa8c,0xacc,'DZFD_coordinate_offsets'),
                  (0xacc,0xbbe,'DZFD_direction_tables')]:
        item(a,b,n,'table','medium','Indexed DS references in the corresponding image/fade module; source bindings preserve original offsets')
    for a,b,n in [(0x3bca,0x3be7,'mono_ega_error'),(0x3be7,0x3c04,'mono_vga_error'),
                  (0x3c04,0x3c28,'color_adapter_error'),(0x3c28,0x3c36,'disk_one_message'),
                  (0x3c36,0x3c3d,'disk_two_message'),(0x3c3d,0x3c5f,'disk_park_message')]:
        assert image[DG+b-1]==ord('$')
        item(a,b,n,'string_literal','high','DVDT INT 21h/AH=09h dollar-terminated string')
        items['DGROUP'][-1]['text_cp437']=image[DG+a:DG+b-1].decode('cp437')
        items['DGROUP'][-1]['terminator']='$'
    for a,b,n in [(0x3bc6,0x3bc8,'video_mode_cookie'),(0x3c5f,0x3c60,'disk_count'),
                  (0x3c60,0x3c62,'first_disk_cylinders'),(0x3c62,0x3c64,'second_disk_cylinders'),
                  (0x3c64,0x3c66,'input_poll_divider'),(0x3c66,0x3c68,'pcx_number')]:item(a,b,n,'initialized_var')
    for a,b,n in [(0x6af,0x70b,'DZLE_numeric_literal_pool'),(0xbbe,0xbd4,'DZFD_numeric_literal_pool'),
                  (0x2007,0x200f,'DOSI_numeric_literal'),(0x201c,0x2087,'DOSI_numeric_literals')]:
        item(a,b,n,'numeric_literal','medium','x87 operands and docs/fp-idioms.md; exact cell types require instruction widths')
    string_spans=[(0x51c,0x6af),(0x70b,0xa38),(0xbd4,0xbfe),(0xc18,0x2007),(0x200f,0x201c),
                  (0x2087,0x249d),(0x2535,0x39a4),(0x39ae,0x3bc6),(0x3c68,0x3d66),(0x3fb4,0x40f5)]
    for a,b in string_spans:
        p=a
        while p<b:
            end=image.index(0,DG+p,DG+b)+1-DG
            raw=image[DG+p:DG+end-1]
            # Numeric cells can occur between strings in the common literal pool.
            if raw and all(v in (7,8,9,10,13,27) or 32<=v<=255 for v in raw):
                item(p,end,'literal_'+format(p,'04x'),'string_literal','medium',
                     'NUL-terminated source-order pool span; CP437 text, including duplicate and short literals')
                items['DGROUP'][-1]['text_cp437']=raw.decode('cp437')
            p=end
    # Named globals: lengths below are independent structure/operand evidence, not distances to next name.
    sizes={0xc02:4,0xc06:4,0x461a:4,0x482c:10,0x4836:700,0x4af2:700,0x4db8:36,0x4ddc:2,
           0x5c5f:80,0x5cc7:80,0x5d17:80,0x5d68:4,0x5d94:4,0x5ff2:4,0x5ff6:4,
           0x5ffa:4,0x5ffe:4,0x6002:4,0x6016:11,0x6021:4,0x6027:4,
           0x743e:20,0x749a:700,0x7756:4,0x775a:4,0x775e:4,0x7762:4}
    for a in range(0x5c33,0x5c4f,4):sizes[a]=4
    for s in symbols:
        a=s['offset']
        if s['space']=='DGROUP' and s['kind']=='global' and a not in (0x5d96,):
            size=sizes.get(a,2)
            if not any(max(a,r['start'])<min(a+size,r['end_exclusive']) for r in items['DGROUP']):
                item(a,a+size,s['name'],'bss' if a>=0x482c else 'initialized_var',s['confidence'],s['evidence'])
    for a,b,n in [(0x3f66,0x3f68,'get_buffer'),(0x3f68,0x3f69,'get_count'),(0x3fad,0x3faf,'put_buffer'),
                  (0x3faf,0x3fb0,'put_count'),(0x3fb0,0x3fb4,'manual_filename_pointer')]:item(a,b,n,'initialized_var')

    ranges=[]
    for c in sorted(contrib,key=lambda c:(c['space'],c['start'])):
        candidates=[r for r in items[c['space']] if c['start']<=r['start']<r['end_exclusive']<=c['end_exclusive']]
        cuts=sorted({c['start'],c['end_exclusive'],*[v for r in candidates for v in (r['start'],r['end_exclusive'])],
                     *[v for v in (len(image)-DG,0x78c3) if c['space']=='DGROUP' and c['start']<v<c['end_exclusive']]})
        origin=DG if c['space']=='DGROUP' else FAR
        for a,b in zip(cuts,cuts[1:]):
            sub=next((r for r in candidates if r['start']<=a and b<=r['end_exclusive']),{})
            r={k:c[k] for k in ('space','owner','object_segment','proof_level')}
            r.update(start=a,end_exclusive=b,size=b-a,linear=origin+a,linear_end_exclusive=origin+b,
                     contribution=c['id'],kind=sub.get('kind',c['kind']),proposed_name=sub.get('proposed_name',c['proposed_name']+f'_{a:04x}'),
                     confidence=sub.get('confidence',c['confidence']),evidence=sub.get('evidence',c['evidence']))
            if 'text_cp437' in sub:r['text_cp437']=sub['text_cp437']
            if 'terminator' in sub:r['terminator']=sub['terminator']
            serialized=max(0,min(b,len(image)-origin)-a)
            r['serialized_bytes']=serialized
            r['serialized_sha256']=sha(image[origin+a:origin+a+serialized]) if serialized else None
            ranges.append(r)
    for space,end in [('DGROUP',0x79e0),('1E81',0xb3e0)]:
        selected=sorted([r for r in ranges if r['space']==space],key=lambda r:r['start'])
        assert selected[0]['start']==0 and selected[-1]['end_exclusive']==end
        assert all(a['end_exclusive']==b['start'] for a,b in zip(selected,selected[1:]))
        assert sum(r['size'] for r in selected)==end
    failures=[(c['owner'],c['object_segment'],c['comparison']) for c in contrib if c.get('comparison') and
              (c['comparison']['invariant_differences'] or any(f['result']=='FAIL' for f in c['comparison']['fixups']))]
    assert not failures, failures
    rtl_members=[]
    for key,ps in placements.items():
        if not ps:continue
        o=obs[key]
        rtl_members.append({'member':names[key],'library':key[0],'page':key[1],
                            'object_sha256':hashes[key],
                            'noncode_segments':[{'name':s['name'],'class':s['class'],'length':s['length'],
                                                 'linear':ps.get(si)} for si,s in enumerate(o.segments[1:],1)
                                                if s['class']!='CODE'],
                            'commons':o.commons})
    literals={r['start']:r for r in ranges if r['space']=='DGROUP' and r['kind']=='string_literal'}
    literal_refs=defaultdict(list)
    modules=[('066A',0x66ab,0xb843),('0B84',0xb843,0xccda),('0CCD',0xccda,0x11c1a),
             ('11C1',0x11c1a,0x158d4),('158D',0x158d4,0x17f58),('17F5',0x17f58,0x18996),
             ('1899',0x18996,0x19f04),('19F0',0x19f04,0x1a727),('1A72',0x1a727,0x1bac0)]
    # Recognizable far-pointer argument construction. These are not a complete xref census.
    for module,a,b in modules:
        for m in re.finditer(rb'\x1e\xb8(..)\x50',image[a:b],re.S):
            off=int.from_bytes(m.group(1),'little')
            if off in literals:
                literal_refs[off].append({'code_module':module,'linear':a+m.start(),
                                          'encoding':'PUSH DS; MOV AX,offset; PUSH AX',
                                          'confidence':'medium','basis':'static instruction-pattern reference'})
    for site in sorted(relocs):
        if DG<=site<DG+0x482c and int.from_bytes(image[site:site+2],'little')==0x29bf:
            off=int.from_bytes(image[site-2:site],'little')
            if off in literals:
                literal_refs[off].append({'data_pointer_linear':site-2,'segment_relocation_linear':site})
    probe_path=ROOT/'build/workers/s-data/probes.json'
    probe_rows=json.loads(probe_path.read_text(encoding='utf-8')) if probe_path.exists() else []
    for p in probe_rows:
        if re.fullmatch(r'bss[012]\.c',p['name']):
            definitions=re.findall(r'int (\w+);',p['source'])
            emitted=[n[1:] for n,v in sorted(p['publics'].items(),key=lambda nv:nv[1][1])]
            assert emitted==definitions[::-1] and not p['commons']
    invariant_total=sum(c['comparison']['emitted_invariant_bytes'] for c in contrib if c.get('comparison'))
    input_paths=['evidence/seg0-residue.json','evidence/names-startup.json','evidence/names-graphics.json',
                 *[p.relative_to(ROOT).as_posix() for p in (ROOT/'src').rglob('*.c') if p.parent.name!='probes'],
                 *[p.relative_to(ROOT).as_posix() for p in (ROOT/'asm').glob('*.asm')]]
    doc={'schema':1,'status':'PROVISIONAL ownership census; original app SEGDEFs and source definition order remain incomplete',
         'address_convention':'All intervals half-open. DGROUP means 29BF:offset; 1E81 means 1E81:offset. No normalized/diagnostic result is an accepted match.',
         'provenance':{'load_image_sha256':sha(image),'oracle_sha256':sha((ROOT/'evidence/oracle.json').read_bytes()),
                       'libraries':{str(p):sha(p.read_bytes()) for p in library_paths},'C0L_sha256':hashes[c0],
                       'input_sha256':{p:sha((ROOT/p).read_bytes()) for p in input_paths}},
         'spaces':[{'id':'DGROUP','paragraph':0x29bf,'linear':DG,'mapped_size':0x79e0,
                    'DATA_end_exclusive':0x47ec,'auxiliary_initialized_end_exclusive':0x482c,
                    'BSS_start':0x482c,'BSS_end_exclusive':0x78c3,'load_image_suffix_size':len(image)-DG,
                    'stack_start':0x78d0,'stack_extent':0x110,'stack_entry_sp':0x80},
                   {'id':'1E81','paragraph':0x1e81,'linear':FAR,'mapped_size':DG-FAR,'serialized_zero_bytes':DG-FAR,
                    'relocation_sites_by_code_module':{'158D':34,'1A72':177},
                    'class':'zero-initialized storage; original FAR_DATA vs FAR_BSS unproved'}],
         'contributions':sorted(contrib,key=lambda c:(c['space'],c['start'])), 'ranges':ranges,
         'rtl_members':rtl_members,
         'literal_order':[{'ds_offset':a,'owner':r['owner'],'text_cp437':r['text_cp437'],
                           'references':literal_refs[a]} for a,r in sorted(literals.items())],
         'symbols':symbols,'candidate_segment_bindings':bindings,'rejected_hypotheses':rejected,
         'stack_overlay':{'owner':'MATHL.LIB/EMUVARS','space':'DGROUP','start':0x78d0,'end_exclusive':0x79e0,
                          'symbol':'__emu','overlaps_C0L_dummy_stack_bytes':128,'omf_combine':'COMMON',
                          'note':'One physical stack, not concatenated 0080+0110 allocations. C0 relocates SS:SP before emulator initialization.'},
         'compiler_probes':{'receipt':'build/workers/s-data/probes.json','flags':['-c','-ml','-O1'],
                            'compiler_sha256':sha((LIB.parent/'BIN/BCC.EXE').read_bytes()),
                            'runner_sha256':sha(Path('C:/tools/nmlgcdos/msdos.exe').read_bytes()),
                            'BSS_order':'reverse declaration order in three shuffled 27-symbol TUs; no COMDEF',
                            'DATA_order':'definitions and local aggregate images at source encounter, then pooled literals; duplicate strings retained, numeric constants shared',
                            'alignment':'byte-packed objects by default; -a word-aligns non-char objects',
                            'far':'uninitialized explicit far globals reverse order, emitted zero-filled FAR_DATA; unique default segment names differ from shared 1E81 frame'},
         'stack_link_probe':{'command':'TLINK.EXE /m C0L.OBJ PROBE.OBJ,PROBE.EXE,PROBE.MAP,EMU.LIB MATHL.LIB CL.LIB',
                             'linker_sha256':sha((LIB.parent/'BIN/TLINK.EXE').read_bytes()),
                             'map_sha256':sha((ROOT/'build/workers/s-data/link/PROBE.MAP').read_bytes()) if (ROOT/'build/workers/s-data/link/PROBE.MAP').exists() else None,
                             'observation':'one _STACK length 0110, __emu offset 0000; C0L STACK 0080, EMUVARS COMMON 0110'},
         'probe_observations':probe_rows,
         'summary':{'ranges':len(ranges),'physical_bytes':sum(r['size'] for r in ranges),
                    'bytes_by_owner':dict(Counter({o:sum(r['size'] for r in ranges if r['owner']==o) for o in sorted({r['owner'] for r in ranges})})),
                    'RTL_data_fixup_results':dict(Counter(f['result'] for c in contrib if c.get('comparison') for f in c['comparison']['fixups'])),
                    'RTL_emitted_invariant_bytes_checked':invariant_total,
                    'RTL_member_objects_including_alias_alternatives':len(rtl_members),
                    'string_literal_entries':len(literals),
                    'literal_entries_with_references':sum(bool(v) for v in literal_refs.values()),
                    'strict_accepted_bytes':0},
         'unresolved':['App TU definition ownership is inferred from placement and use, not original PUBDEFs; shared globals may be defined in 066A even when chiefly used in 11C1.',
                       'DGROUP 7452..7456 definition owner/objects unresolved; far byte 0259 unresolved.',
                       'Exact application object names, all object sizes/source identifiers, and far segment names/combine flags require reconstruction probes.',
                       'Literal pool starts are not original TU starts. Empty/unreferenced objects and literals remain unidentified within attributed storage.',
                       '158D error-screen buffer references use absolute row 22..24 times 009Ch, exceeding the inferred 01D4-byte allocation; no runtime experiment performed.']}
    (ROOT/'evidence/dgroup.json').write_text(json.dumps(doc,indent=2,ensure_ascii=True)+'\n',encoding='utf-8')
    print(json.dumps(doc['summary'],indent=2))


if __name__=='__main__':main()
