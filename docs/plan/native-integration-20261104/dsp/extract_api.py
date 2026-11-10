"""Reuse the existing bounded exact-name reader; no engine, no broad source scan."""
from pathlib import Path
import importlib.util
import json
import hashlib

HERE = Path(__file__).resolve().parent
RESEARCH = HERE.parents[1] / 'von-re-20261104'
reader_path = RESEARCH / 'index/input_sources/extract_selected.py'
spec = importlib.util.spec_from_file_location('existing_exact_reader', reader_path)
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)

base = Path(r'C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger\addons')
for label, archive, names in (
    ('core', base / 'core/data.pak', {'AudioSystem.c', 'BaseSoundComponent.c', 'SndComponent.c', 'AudioHandle.c'}),
    ('game', base / 'data/data007.pak', {'VoNComponent.c', 'SignalsManagerComponent.c', 'SoundComponent.c'}),
):
    target = HERE / label
    target.mkdir(exist_ok=True)
    reader.OUT, reader.PAK, reader.NAMES = target, archive, names
    reader.main()

docs = Path(r'C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger Tools\Workbench\docs')
records = []
for rel in ('EnfusionScriptAPI/html/interfaceAudioSystem.html',
            'EnfusionScriptAPI/html/interfaceBaseSoundComponent.html',
            'ArmaReforgerScriptAPIPublic/interfaceVoNComponent.html',
            'ArmaReforgerScriptAPIPublic/interfaceSignalsManagerComponent.html'):
    path = docs / rel
    if not path.exists():
        path = docs / Path(rel).parent / 'html' / Path(rel).name
    if path.exists():
        records.append({'path': str(path), 'size': path.stat().st_size,
                        'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
(HERE / 'installed_docs_receipt.json').write_text(json.dumps(records, indent=2), encoding='utf-8')
