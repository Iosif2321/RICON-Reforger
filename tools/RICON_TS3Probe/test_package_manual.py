import hashlib
import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from zipfile import ZipFile

from package_manual import build_package


class ManualPackageTest(unittest.TestCase):
    def test_legacy_and_update_capable_packages(self):
        with TemporaryDirectory() as temp:
            root = Path(temp)
            dll, helper = b"test plugin", b"test updater"
            (root / "ricon_pcm_probe.dll").write_bytes(dll)
            (root / "RiconUpdate.exe").write_bytes(helper)
            manifest = {"api": 26, "version": "0.5.9", "sha256": hashlib.sha256(dll).hexdigest()}
            meta = root / "plugin.json"
            meta.write_text(json.dumps(manifest))
            build_package(root, root / "old.ts3_plugin")
            with ZipFile(root / "old.ts3_plugin") as archive:
                self.assertEqual(archive.namelist(), ["package.ini", "plugins/ricon_pcm_probe.dll"])

            output = root / "new.ts3_plugin"
            for version, extra in (("0.6.3", {}), ("0.6.3", {"updaterSha256": "0" * 64}),
                                   ("0.65536.1", {}), ("0.06.1", {})):
                with self.subTest(version=version, extra=extra):
                    meta.write_text(json.dumps(dict(manifest, version=version, **extra)))
                    with self.assertRaises(ValueError):
                        build_package(root, output)
                    self.assertFalse(output.exists())
            manifest.update(version="0.6.3", updaterSha256=hashlib.sha256(helper).hexdigest())
            meta.write_text(json.dumps(manifest))
            build_package(root, output)
            with ZipFile(output) as archive:
                self.assertEqual(archive.read("plugins/ricon_pcm_probe/RiconUpdate.exe"), helper)
                self.assertEqual(archive.read("plugins/ricon_pcm_probe.dll"), dll)
                self.assertIn(b"Version = 0.6.3", archive.read("package.ini"))


if __name__ == "__main__":
    unittest.main()
