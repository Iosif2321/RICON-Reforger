"""CT10.1: ручной golden, границы кадра, отсутствие алиасов и точная эквивалентность якорей."""
import copy
import json
from pathlib import Path
import random
import struct
import unittest

import channel_descriptor_oracle as cd
import channel_fading_oracle as cf
import build_channel_descriptor_vectors as vectors


GOLDEN = bytes.fromhex("""
52434644 01 0066
13 46494e4954455f4d554c5449504154485f5631
09 544553545f4f4e4c59
00000001
0000000a 00000000 00000014 00000000
43160000 46c35000
00000001 000003e8 000003e8 00001388
03 01
06 646972656374 3f800000 00000000 00000000
""")


def query(d, time, sub=0):
    return dict(tx=d["a"], rx=d["b"], frequencyMHz=d["frequencyMHz"], bandwidthHz=d["bandwidthHz"],
                profileId=d["profileId"], profileVersion=d["profileVersion"], environmentRevision=d["environmentRevision"],
                nowMs=time, subUs=sub)


class ChannelDescriptorTest(unittest.TestCase):
    def test_manual_golden_cd07(self):
        # Ручная разметка таблицы CD07: частота 150, полоса 25000, один direct, весь кадр 102 байта.
        result = cd.encode(cf.baseline()["descriptor"], 102)
        self.assertEqual(list(GOLDEN), result["bytes"])
        self.assertEqual(102, len(GOLDEN))
        self.assertEqual(GOLDEN, bytes.fromhex(vectors.build()["goldenHex"]))
        self.assertEqual(result, cd.decode(list(GOLDEN), 102))

    def test_exact_budget_and_structural_maximum(self):
        d = cf.baseline()["descriptor"]
        d.update(profileId="p" * 64, components=[dict(componentId=f"c{i:02}" + "x" * 61,
                 powerWeight=1, phaseQ=i, dopplerQ=0) for i in range(64)])
        result = cd.encode(d, 5066)
        self.assertEqual("OK", result["code"])
        self.assertEqual(cd.MAX_SUPPORTED_BYTES, result["sizeBytes"])
        self.assertEqual({"code": "RESOURCE_LIMIT"}, cd.encode(d, 5065))
        unknown = cd.normal(d)
        unknown["modelId"] = "m" * 64
        raw = cd.frame(unknown)
        self.assertEqual(cd.MAX_BYTES, len(raw))
        self.assertEqual({"code": "UNSUPPORTED_MODEL"}, cd.decode(list(raw), 5111))

    def test_zero_sign_is_canonical_but_positive_subnormal_survives(self):
        d = cf.baseline()["descriptor"]
        d["components"][0]["powerWeight"] = -0.0
        encoded = cd.encode(d, 5120)
        self.assertEqual(b"\0" * 4, struct.pack(">f", encoded["descriptor"]["components"][0]["powerWeight"]))
        d["components"][0]["powerWeight"] = 2**-149
        encoded = cd.encode(d, 5120)
        decoded = cd.decode(encoded["bytes"], 5120)
        self.assertEqual(bytes.fromhex("00000001"), struct.pack(">f", decoded["descriptor"]["components"][0]["powerWeight"]))

    def test_float32_extremes_and_int_min_are_lossless(self):
        d = vectors.dynamic()
        d.update(frequencyMHz=cf.f32(3.4028234663852886e38), bandwidthHz=2**-149)
        d["components"][0]["dopplerQ"] = -(1 << 31)
        result = cd.encode(d, 5120)
        self.assertEqual(result, cd.decode(result["bytes"], 5120))
        self.assertEqual({"code": "OUT_OF_MODEL_DOMAIN"}, cd.rebase(d, 1234))

    def test_deep_copy_and_input_immutability(self):
        d = vectors.dynamic()
        before = copy.deepcopy(d)
        result = cd.encode(d, 5120)
        decoded = cd.decode(result["bytes"], 5120)
        decoded["descriptor"]["a"]["deviceId"] = 999
        decoded["descriptor"]["components"][0]["phaseQ"] = 42
        decoded["bytes"][0] = 0
        self.assertEqual(before, d)
        self.assertEqual(list(cd.frame(cd.normal(d))), result["bytes"])
        rebased = cd.rebase(d, 1234)
        rebased["descriptor"]["components"].clear()
        self.assertEqual(before, d)

    def test_resource_priority_precedes_element_scan(self):
        self.assertEqual({"code": "RESOURCE_LIMIT"}, cd.decode([True, 999], 1))
        self.assertEqual({"code": "INVALID_FRAME"}, cd.decode([True, 999], 5120))
        self.assertEqual({"code": "INVALID_LIMITS"}, cd.decode([True, 999], False))

    def test_header_version_and_exact_consumption(self):
        for value in (GOLDEN[:-1], GOLDEN + b"\0"):
            self.assertEqual({"code": "INVALID_FRAME"}, cd.decode(list(value), 5120))
        raw = bytearray(GOLDEN)
        raw[4] = 2
        self.assertEqual({"code": "UNSUPPORTED_VERSION"}, cd.decode(list(raw), 5120))
        raw = bytearray(GOLDEN + b"\0")
        struct.pack_into(">H", raw, 5, len(raw))
        self.assertEqual({"code": "INVALID_FRAME"}, cd.decode(list(raw), 5120))

    def test_rebase_preserves_sequence_and_composition(self):
        d = vectors.dynamic()
        moved = cd.rebase(d, 1234)["descriptor"]
        for time, sub in ((1234, 0), (1234, 999), (2000, 123), (4999, 999)):
            q = query(d, time, sub)
            self.assertEqual(cf.evaluate(dict(descriptor=d, query=q)), cf.evaluate(dict(descriptor=moved, query=q)))
        twice = cd.rebase(moved, 4000)
        self.assertEqual(cd.rebase(d, 4000), twice)

    def test_rebase_near_int32_end_and_static_case(self):
        d = vectors.dynamic()
        d.update(anchorMs=0, validFromMs=0, validUntilMs=cf.MAX)
        moved = cd.rebase(d, cf.MAX-1)["descriptor"]
        for sub in (0, 999):
            q = query(d, cf.MAX-1, sub)
            self.assertEqual(cf.evaluate(dict(descriptor=d, query=q)), cf.evaluate(dict(descriptor=moved, query=q)))
        static = cf.baseline()["descriptor"]
        self.assertEqual(static["components"], cd.rebase(static, 4000)["descriptor"]["components"])

    def test_fixed_random_roundtrip_and_rebase(self):
        rng = random.Random(643102)
        for _ in range(128):
            d = vectors.dynamic()
            for c in d["components"]:
                c.update(powerWeight=cf.f32(rng.random()), phaseQ=rng.randrange(cf.M),
                         dopplerQ=rng.randrange(-cf.QUARTER, cf.QUARTER+1))
            encoded = cd.encode(d, 5120)
            self.assertEqual(encoded, cd.decode(encoded["bytes"], 5120))
            moved = cd.rebase(d, 2000)["descriptor"]
            q = query(d, rng.randrange(2000, 5000), rng.randrange(1000))
            self.assertEqual(cf.evaluate(dict(descriptor=d, query=q)), cf.evaluate(dict(descriptor=moved, query=q)))

    def test_corpus_and_failure_codes(self):
        corpus = vectors.build()
        codes = set()
        for c in corpus["cases"]:
            before = copy.deepcopy(c["input"])
            actual = cd.evaluate(c["input"])
            self.assertEqual(c["expect"], actual, c["id"])
            self.assertEqual(before, c["input"], c["id"])
            codes.add(actual["code"])
        self.assertEqual({"OK", "INVALID_LIMITS", "INVALID_DESCRIPTOR", "UNSUPPORTED_MODEL", "RESOURCE_LIMIT",
                          "INVALID_FRAME", "UNSUPPORTED_VERSION", "INVALID_ANCHOR", "OUT_OF_MODEL_DOMAIN"}, codes)
        stored = Path(__file__).with_name("vectors") / "channel_descriptor_v1.json"
        self.assertEqual(vectors.canonical().encode("utf-8"), stored.read_bytes())


if __name__ == "__main__":
    unittest.main()
