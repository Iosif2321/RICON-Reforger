#!/usr/bin/env python3
"""Только чтение локальной Script API: методы-кандидаты, номера HTML-строк и SHA, без запуска Workbench."""
import argparse
import hashlib
import html
import json
from pathlib import Path
import re

FILES = {
    "interfaceAudioEditor.html": {"SetOpenedResource", "GetContainer", "GetNumContainers"},
    "interfaceWBModuleDef.html": {"SetOpenedResource", "GetContainer", "GetNumContainers"},
    "interfaceWorkbench.html": {"GetModule"},
    "interfaceBaseContainer.html": {"GetClassName", "GetNumVars", "GetVarName"},
}


def methods(text):
    result = []
    for number, line in enumerate(text.splitlines(), 1):
        cell = re.search(r'<td class="memItemRight">(.*?)</td>', line)
        if cell is None:
            continue
        name = re.search(r'<a\b[^>]*>([A-Za-z_][A-Za-z_0-9]*)</a>\s*\(', cell[1])
        if name is not None:
            signature = " ".join(html.unescape(re.sub(r"<[^>]*>", " ", cell[1])).split())
            result.append({"name": name[1], "line": number, "signature": signature})
    return result


def audit(root):
    pages = []
    for filename, required in FILES.items():
        raw = (root / filename).read_bytes()
        found = [row for row in methods(raw.decode("utf-8")) if row["name"] in required]
        if {row["name"] for row in found} != required:
            raise ValueError(f"Неполная или изменившаяся API-страница: {filename}")
        pages.append({"file": filename, "sha256": hashlib.sha256(raw).hexdigest(), "methods": found})
    return {"evidence": "DOC_API_CANDIDATE", "acpReadTested": False, "pages": pages}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("html_root", type=Path)
    args = parser.parse_args()
    try:
        report = audit(args.html_root)
    except (OSError, UnicodeError, ValueError) as error:
        parser.exit(1, f"AUDIO_API_AUDIT_FAIL: {error}\n")
    print(json.dumps(report, ensure_ascii=True, indent=2))


if __name__ == "__main__":
    main()
