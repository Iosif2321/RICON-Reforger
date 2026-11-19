"""Build an opt-in TS3 Package Installer file from a pinned RICON test release."""

import argparse
import hashlib
import json
from pathlib import Path
import re
from zipfile import ZIP_DEFLATED, ZipFile


DLL_NAME = "ricon_pcm_probe.dll"


def build_package(release: Path, output: Path) -> None:
    if output.suffix != ".ts3_plugin":
        raise ValueError("Output must use the .ts3_plugin extension")
    manifest = json.loads((release / "plugin.json").read_text(encoding="utf-8-sig"))
    if not isinstance(manifest, dict):
        raise ValueError("Expected a RICON plugin manifest")
    version = manifest.get("version", "")
    if (manifest.get("api") != 26 or not isinstance(version, str)
            or not re.fullmatch(r"(?:0|[1-9][0-9]{0,4})(?:\.(?:0|[1-9][0-9]{0,4})){2}", version)):
        raise ValueError("Expected a RICON API 26 release with a numeric version")
    parts = tuple(map(int, version.split(".")))
    if max(parts) > 65535:
        raise ValueError("RICON version component out of range")

    dll = (release / DLL_NAME).read_bytes()
    if hashlib.sha256(dll).hexdigest() != manifest.get("sha256"):
        raise ValueError("RICON plugin checksum mismatch")
    helper = None
    if parts >= (0, 6, 0) and "updaterSha256" not in manifest:
        raise ValueError("An update-capable RICON release requires the verified updater")
    if "updaterSha256" in manifest:
        helper = (release / "RiconUpdate.exe").read_bytes()
        if hashlib.sha256(helper).hexdigest() != manifest["updaterSha256"]:
            raise ValueError("RICON updater checksum mismatch")

    package_info = (
        "Name = RICON PCM proof (TOOL_ONLY)\n"
        "Type = Plugin\n"
        "Author = RICON\n"
        f"Version = {version}\n"
        "Platforms = win64\n"
        "Description = RICON diagnostic plugin for TeamSpeak 3.6.2 x64 (API 26).\n"
    )
    with ZipFile(output, mode="x", compression=ZIP_DEFLATED) as package:
        package.writestr("package.ini", package_info)
        if helper is not None:
            package.writestr("plugins/", b"")
            package.writestr("plugins/ricon_pcm_probe/", b"")
            package.writestr("plugins/ricon_pcm_probe/RiconUpdate.exe", helper)
        package.writestr(f"plugins/{DLL_NAME}", dll)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("release", type=Path, help="Directory with plugin.json and the pinned DLL")
    parser.add_argument("output", type=Path, help="New .ts3_plugin file")
    options = parser.parse_args()
    build_package(options.release, options.output)
