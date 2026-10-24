"""Проверки идентичности и границ частичного каталога, без игры и RF-расчётов."""
import copy
import json
from pathlib import Path
import tempfile
import unittest

from radio_model_catalog import PREFIX, load_catalog, lookup, validate_catalog


class RadioModelCatalogTests(unittest.TestCase):
    def setUp(self):
        self.catalog = load_catalog()
        self.build = "1.8.0.13"

    def test_inventory_and_partial_status(self):
        expected = {"Radio_ANPRC68.et": "an_prc68", "Radio_ANPRC77.et": "an_prc77",
                    "Radio_ANPRC77_COK.et": "an_prc77", "Radio_R107M.et": "r107m",
                    "Radio_R107M_FIA.et": "r107m", "Radio_R148.et": "r148",
                    "Radio_R148_FIA.et": "r148e", "Radio_RF10/Radio_RF10.et": "rf10"}
        self.assertEqual({b["prefab"] for b in self.catalog["bindings"]}, {PREFIX+p for p in expected})
        for path, model_id in expected.items():
            with self.subTest(path=path):
                result = lookup(self.catalog, PREFIX+path, self.build)
                self.assertEqual(result["binding"]["modelId"], model_id)
                self.assertEqual(result["code"], "IDENTITY_ONLY" if model_id in ("r148", "r148e") else "PASSPORT_PARTIAL")
                self.assertFalse(result["readyForS2"])

    def test_identity_has_no_fuzzy_or_inheritance_fallback(self):
        for value in ("Radio_ANPRC68.et", "#AR-Item_ANPRC68_Name", PREFIX+"Radio_ANPRC68_Mod.et",
                      PREFIX.lower()+"Radio_ANPRC68.et", PREFIX+"../Radios/Radio_ANPRC68.et",
                      PREFIX.replace("ArmaReforger", "OtherMod")+"Radio_ANPRC68.et"):
            with self.subTest(value=value):
                self.assertEqual(lookup(self.catalog, value, self.build), {"code": "UNKNOWN_PREFAB", "readyForS2": False})

    def test_build_and_input_fail_without_profile(self):
        path = PREFIX+"Radio_ANPRC68.et"
        self.assertEqual(lookup(self.catalog, path, "1.8.0.14")["code"], "BUILD_MISMATCH")
        for value in (None, "", "  ", 4, True, [], {}):
            self.assertEqual(lookup(self.catalog, value, self.build)["code"], "INVALID_INPUT")
            self.assertEqual(lookup(self.catalog, path, value)["code"], "INVALID_INPUT")

    def test_base_prefabs_are_not_radio_items(self):
        for path in self.catalog["basePrefabs"]:
            self.assertEqual(lookup(self.catalog, path, self.build), {"code": "BASE_PREFAB", "readyForS2": False})

    def test_result_is_detached_and_inputs_stable(self):
        original = copy.deepcopy(self.catalog)
        result = lookup(self.catalog, PREFIX+"Radio_ANPRC68.et", self.build)
        result["model"]["facts"][0]["value"][0] = 999
        result["binding"]["reportedGame"]["transceivers"][0]["minRaw"] = 1
        self.assertEqual(self.catalog, original)
        self.assertEqual(lookup(self.catalog, PREFIX+"Radio_ANPRC68.et", self.build)["model"]["facts"][0]["value"], [30,79.95])

    def test_variant_and_native_values_are_not_passport_defaults(self):
        ordinary = lookup(self.catalog, PREFIX+"Radio_R148.et", self.build)
        fia = lookup(self.catalog, PREFIX+"Radio_R148_FIA.et", self.build)
        self.assertNotEqual(ordinary["binding"]["modelId"], fia["binding"]["modelId"])
        self.assertNotEqual(ordinary["binding"]["nameKey"], fia["binding"]["nameKey"])
        rf10 = lookup(self.catalog, PREFIX+"Radio_RF10/Radio_RF10.et", self.build)
        self.assertEqual(rf10["binding"]["reportedGame"]["transceivers"][0]["minRaw"], 30000)
        self.assertEqual(rf10["binding"]["reportedGame"]["tuningUnit"], "UNVERIFIED")
        self.assertEqual(rf10["model"]["facts"][0]["value"], [44,53.975])

    def test_measurement_conditions_survive_lookup(self):
        rf10 = lookup(self.catalog, PREFIX+"Radio_RF10/Radio_RF10.et", self.build)
        sensitivity = [f for f in rf10["model"]["facts"] if f["quantity"] == "rxSensitivityUv"]
        self.assertEqual([(f["value"],f["relation"]) for f in sensitivity], [(0.5,"less_than"),(1,"maximum"),(2,"maximum")])
        self.assertIn("97.5%", sensitivity[0]["conditions"])
        self.assertIn("восьми", sensitivity[1]["conditions"])
        self.assertIn("двух", sensitivity[2]["conditions"])
        prc68 = self.catalog["models"]["an_prc68"]["facts"]
        self.assertEqual([f["value"] for f in prc68 if f["quantity"] == "channelSpacingKHz"], [50])
        self.assertEqual([f["value"] for f in prc68 if f["quantity"] == "presetChannelSpacingKHz"], [200])

    def test_invalid_fact_numbers_are_rejected(self):
        for value in (True, "1", 0, -1, float("nan"), float("inf"), 10**400, None, [1]):
            changed = copy.deepcopy(self.catalog)
            changed["models"]["an_prc68"]["facts"][1]["value"] = value
            with self.subTest(value=repr(value)), self.assertRaises(ValueError):
                validate_catalog(changed)
        for value in ([40,30], [True,30], [30,float("inf")], [30], "30-80"):
            changed = copy.deepcopy(self.catalog)
            changed["models"]["an_prc68"]["facts"][0]["value"] = value
            with self.subTest(value=repr(value)), self.assertRaises(ValueError):
                validate_catalog(changed)

    def test_invalid_structure_and_references_are_rejected(self):
        changes = [lambda c: c.update(schemaVersion=True),
                   lambda c: c.update(readyForS2=True),
                   lambda c: c["bindings"].append(copy.deepcopy(c["bindings"][0])),
                   lambda c: c["bindings"][0].update(modelId="missing"),
                   lambda c: c["bindings"][0].update(nameSource="tm_prc68"),
                   lambda c: c["bindings"][0].update(prefab=c["basePrefabs"][0]),
                   lambda c: c["models"]["an_prc68"]["facts"][0].update(sourceId="wb_names"),
                   lambda c: c["models"]["an_prc68"]["facts"][0].update(conditions=""),
                   lambda c: c["models"]["an_prc68"].update(variant=""),
                   lambda c: c["sources"]["tm_prc68"].update(sha256="x"),
                   lambda c: c["bindings"][0]["reportedGame"].update(tuningUnit="kHz"),
                   lambda c: c["bindings"][0]["reportedGame"]["transceivers"][0].update(minRaw=None),
                   lambda c: c["bindings"][0]["reportedGame"]["transceivers"][0].update(stepRaw=True)]
        for i, change in enumerate(changes):
            changed = copy.deepcopy(self.catalog)
            change(changed)
            with self.subTest(mutation=i), self.assertRaises(ValueError):
                validate_catalog(changed)

    def test_strict_json_loader(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/"catalog.json"
            for content in ('{"schemaVersion":1,"schemaVersion":1}',
                            json.dumps(self.catalog).replace('79.95', 'NaN', 1),
                            json.dumps(self.catalog).replace('79.95', 'Infinity', 1)):
                path.write_text(content, encoding="utf-8")
                with self.assertRaises(ValueError):
                    load_catalog(path)


if __name__ == "__main__":
    unittest.main()
