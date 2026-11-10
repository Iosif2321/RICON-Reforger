"""Two fixed read-only MSVC ranges; no full binary dump or native execution."""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess

HERE = Path(__file__).resolve().parent
OLD = HERE.parents[1] / 'von-re-20261104/index'
manifest = json.loads((OLD / 'map.json').read_text())
image = Path(manifest['binary'])
tool = Path(r'C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\dumpbin.exe')
def sha(path):
    result = hashlib.sha256()
    with path.open('rb') as stream:
        while block := stream.read(1024 * 1024): result.update(block)
    return result.hexdigest()

before = image.stat()
image_hash = sha(image)
assert image_hash == manifest['sha256']
tool_hash = sha(tool)
assert tool_hash == '9a2bcd5343b2c1ab4dd4a906f673bc89d1dbe6812e1b3de7cc5c85223d9058eb'
rows = []
for name, start, end in (
    ('signal-scalar-getter', 0x140A1AC90, 0x140A1AD26),
    ('project-variable-getter', 0x140A1ADE0, 0x140A1AE22),
):
    with image.open('rb') as stream:
        section = next(s for s in manifest['sections']
            if 0x140000000 + s['rva'] <= start and end <= 0x140000000 + s['rva'] + s['raw_size'])
        stream.seek(section['raw_offset'] + start - 0x140000000 - section['rva'])
        raw = stream.read(end-start)
        pdata = next(s for s in manifest['sections'] if s['name'] == '.pdata')
        stream.seek(pdata['raw_offset'])
        pdata_bytes = stream.read(pdata['raw_size'])
        matches = [pdata_bytes[i:i+12] for i in range(0,len(pdata_bytes)-11,12)
            if struct.unpack_from('<II',pdata_bytes,i) == (start-0x140000000,end-0x140000000)]
        assert len(matches) == 1
    cmd = [str(tool), '/NOLOGO', '/DISASM:BYTES', f'/RANGE:0x{start:X},0x{end-1:X}', str(image)]
    run = subprocess.run(cmd, capture_output=True, check=True)
    assert not run.stderr.strip(), run.stderr
    output = HERE / (name+'.dumpbin.asm')
    output.write_bytes(run.stdout)
    text = run.stdout.decode('utf-8', errors='replace')
    instructions = [(int(m.group(1),16),m.group(2)) for m in re.finditer(r'^\s+([0-9A-F]{16}):\s+(.+)$',text,re.M)]
    assert instructions and instructions[0][0] == start
    assert all(start <= va < end for va,_ in instructions)
    rows.append({'name':name,'range':[hex(start),hex(end)],'bytes':len(raw),
        'raw_hex':raw.hex(),'raw_sha256':hashlib.sha256(raw).hexdigest(),
        'exact_pdata_bytes':matches[0].hex(),'command':cmd,'exit_code':run.returncode,
        'instructions':len(instructions),'last_va':hex(instructions[-1][0]),
        'last_instruction':instructions[-1][1].strip(),'asm_path':str(output),
        'asm_sha256':hashlib.sha256(run.stdout).hexdigest()})
after = image.stat()
assert (before.st_size,before.st_mtime_ns)==(after.st_size,after.st_mtime_ns)
(HERE/'getter_receipt.json').write_text(json.dumps({'image':str(image),'image_sha256':image_hash,
    'tool':str(tool),'tool_sha256':tool_hash,'source_stat_unchanged':True,
    'method':'Read-only DISASM:BYTES; RANGE uses inclusive end; exact raw .pdata and raw span verified; only2fixedfunctions',
    'records':rows},indent=2),encoding='utf-8')
print(json.dumps({'functions':len(rows),'total_requested_bytes':sum(x['bytes'] for x in rows),'source_unchanged':True}))
