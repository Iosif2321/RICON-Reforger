"""One exact Input RTTI lookup in .data/.rdata; no code or source corpus scan."""
from pathlib import Path
import hashlib
import json
import struct

HERE = Path(__file__).resolve().parent
INDEX = HERE.parents[1] / 'von-re-20261104/index'
m = json.loads((INDEX / 'map.json').read_text())
image = Path(m['binary'])
base = 0x140000000
pdata = [tuple(int(v,16) for v in line.split('\t')[:2])
    for line in (INDEX/'pdata.tsv').read_text().splitlines()[1:]]
before = image.stat()
with image.open('rb') as stream:
    def read(va, length):
        section = next(s for s in m['sections'] if base+s['rva'] <= va and va+length <= base+s['rva']+s['raw_size'])
        stream.seek(section['raw_offset']+va-base-section['rva'])
        result = stream.read(length)
        assert len(result) == length
        return result
    data_section = next(s for s in m['sections'] if s['name']=='.data')
    rdata_section = next(s for s in m['sections'] if s['name']=='.rdata')
    data = read(base+data_section['rva'], data_section['raw_size'])
    rdata = read(base+rdata_section['rva'], rdata_section['raw_size'])
    records=[]
    # Only these four exact RTTI class/name variants are permitted; no broader type scan.
    names=[f'.?A{kind}{name}@iop@enf@@'.encode()+b'\0'
        for name in ('IOPItemInput','IOPItemInputClass') for kind in ('U','V')]
    searches=[]
    for name in names:
        positions=[]
        pos=0
        while (pos:=data.find(name,pos)) >= 0:
            positions.append(pos);pos+=len(name)
        searches.append({'exact_name':name[:-1].decode(),'matches':len(positions)})
        for pos in positions:
            td=base+data_section['rva']+pos-16
            needle=struct.pack('<I',td-base)
            cp=0
            while (cp:=rdata.find(needle,cp))>=0:
                candidate=cp-12;cp+=4
                if candidate<0 or candidate+24>len(rdata):continue
                col=base+rdata_section['rva']+candidate
                rawcol=rdata[candidate:candidate+24]
                fields=struct.unpack('<6I',rawcol)
                if fields[0]!=1 or base+fields[5]!=col:continue
                vp=0
                while (vp:=rdata.find(struct.pack('<Q',col),vp))>=0:
                    vtable=base+rdata_section['rva']+vp+8;vp+=8
                    record={'name':name[:-1].decode(),'td':hex(td),'COL':hex(col),'COL_bytes':rawcol.hex(),
                        'this_offset':fields[1],'vtable':hex(vtable)}
                    # The runtime primary interface is the only relevant update interface.
                    # Do not treat secondary/descriptor table+90 as the same slot contract.
                    if fields[1]==0 and b'IOPItemInput@' in name:
                        table_prefix=read(vtable,19*8)
                        slots=struct.unpack('<19Q',table_prefix)
                        assert all(any(s['characteristics']&0x20000000 and base+s['rva']<=t<base+s['rva']+s['virtual_size'] for s in m['sections']) for t in slots)
                        slotbytes=table_prefix[-8:]
                        target=slots[18]
                        boundary=next(([hex(a),hex(b)] for a,b in pdata if a<=target<b),None)
                        record.update({'primary_table_prefix19slots':table_prefix.hex(),'slot90_pointer_bytes':slotbytes.hex(),
                            'slot90_target':hex(target),'target_pdata':boundary,'target_first16bytes':read(target,16).hex()})
                    else:
                        record['slot90_status']='NOT_APPLIED_TO_SECONDARY_OR_CLASS_DESCRIPTOR'
                    records.append(record)
after=image.stat()
assert (before.st_size,before.st_mtime_ns)==(after.st_size,after.st_mtime_ns)
result={'image_sha256_from_getter_receipt':json.loads((HERE/'getter_receipt.json').read_text())['image_sha256'],
    'method':'One exact four-variant Input RTTI lookup: only .data names, .rdata COL/pointer refs,known vtable+90 and16targetbytes; no code-xref scan, source scan or generic RTTI inventory',
    'source_stat_unchanged':True,'data_bytes':len(data),'data_sha256':hashlib.sha256(data).hexdigest(),
    'rdata_bytes':len(rdata),'rdata_sha256':hashlib.sha256(rdata).hexdigest(),'searches':searches,'records':records}
(HERE/'selected-input-rtti.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps(result,indent=2))
