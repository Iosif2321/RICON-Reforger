"""Exact registration lookup of two named methods, not a broad audio/API scan."""
from pathlib import Path
import bisect
import hashlib
import json
import re
import struct

HERE=Path(__file__).resolve().parent
INDEX=HERE.parents[1]/'von-re-20261104/index'
m=json.loads((INDEX/'map.json').read_text())
image=Path(m['binary']);data=image.read_bytes()
assert hashlib.sha256(data).hexdigest()==m['sha256']
base=0x140000000
def va(offset):
    s=next(s for s in m['sections'] if s['raw_offset']<=offset<s['raw_offset']+s['raw_size'])
    return base+s['rva']+offset-s['raw_offset']
functions=[tuple(int(x,16) for x in row.split('\t')[:2]) for row in (INDEX/'pdata.tsv').read_text().splitlines()[1:]]
starts=[x[0] for x in functions]
def boundary(addr):
    i=bisect.bisect_right(starts,addr)-1
    return [hex(x) for x in functions[i]] if i>=0 and addr<functions[i][1] else None
names={}
for name in ('SetSignalValue','SetSignalValueStr'):
    needle=name.encode()+b'\0'
    for s in m['sections']:
        if s['name'] not in ('.rdata','.data'):continue
        p=s['raw_offset'];limit=p+s['raw_size']
        while (p:=data.find(needle,p,limit))>=0:
            names[va(p)]=name;p+=len(needle)
refs=[]
pattern=re.compile(rb'[\x40-\x4f]?[\x8b\x8d\x89][\x05\x0d\x15\x1d\x25\x2d\x35\x3d]....',re.S)
for s in m['sections']:
    if not s['characteristics']&0x20000000:continue
    code=data[s['raw_offset']:s['raw_offset']+s['raw_size']]
    for match in pattern.finditer(code):
        address=base+s['rva']+match.start()
        target=address+len(match[0])+struct.unpack('<i',match[0][-4:])[0]
        if target in names:
            refs.append({'method':names[target],'source':hex(address),'target_string':hex(target),
                'bytes':match[0].hex(),'pdata':boundary(address),'boundary_verified':False})
pointer_refs=[]
for address,name in names.items():
    needle=struct.pack('<Q',address)
    for s in m['sections']:
        if s['name'] not in ('.rdata','.data'):continue
        p=s['raw_offset'];limit=p+s['raw_size']
        while (p:=data.find(needle,p,limit))>=0:
            pointer_refs.append({'method':name,'pointer_va':hex(va(p)),'target':hex(address)});p+=8
result={'image':str(image),'image_sha256':m['sha256'],
    'method':'Only2 exact null-terminated method names in data; code RIP-form refs and data pointers restricted to those exact targets. References are byte candidates until bounded instruction verification; no broad audio/callgraph scan.',
    'strings':[{'va':hex(v),'name':name} for v,name in names.items()],'code_candidates':refs,'data_pointers':pointer_refs}
(HERE/'two-method-registration-candidates.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps(result,indent=2))
