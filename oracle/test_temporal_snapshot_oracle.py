"""Independent CT10 host assertions; expected values precede root implementation.

Case IDs refer to frozen independent-cases.json (293485e8...). No universal
event adapter and no expected-value generation by the implementation under test.
"""
import copy
import json
import math
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
LOCAL_FIXTURE = HERE / 'independent-cases.json'
ORACLE = HERE.parents[2].parent / 'RICON-Reforger' / 'oracle' if LOCAL_FIXTURE.exists() else HERE
FIXTURE = LOCAL_FIXTURE if LOCAL_FIXTURE.exists() else HERE / 'vectors' / 'temporal_snapshot_v2.review.json'
sys.path.insert(0, str(ORACLE))
import channel_descriptor_oracle as cd
import channel_fading_oracle as cf
import temporal_snapshot_oracle as temporal

MANUAL = json.loads(FIXTURE.read_text(encoding='utf-8'))
LIMITS = dict(maxSnapshots=4, maxDescriptorBytes=102, maxTotalDescriptorBytes=408)
# Manual CD07 frame: D0, fixed-width endpoint IDs 101/202 and times 100/100/1200.
GOLDEN_D0 = list(bytes.fromhex('''
52434644 01 0066 13 46494e4954455f4d554c5449504154485f5631
09 544553545f4f4e4c59 00000001 00000065 00000000 000000ca 00000000
43160000 46c35000 00000001 00000064 00000064 000004b0 03 01
06 646972656374 3f800000 00000000 00000000
'''))


def snapshot():
    s = copy.deepcopy(MANUAL['fixtures']['S0'])
    s.pop('temporalDescriptorRef')
    s.update(mode='RECEIVER', timebaseId='TEST_CLOCK', descriptorVersion=1,
             binding=dict(leaseId=11, txGeneration=1, rxGeneration=1,
                          txDeviceId=101, txTransceiverIndex=0, rxDeviceId=202,
                          rxTransceiverIndex=0, networkTypeId=1, channelId=1),
             descriptor=copy.deepcopy(MANUAL['fixtures']['D0']))
    return s


def expected():
    # Independent trusted fixture; never inferred from a returned actual snapshot.
    return dict(sessionEpoch=7, linkId=1, talkerPlayerId=1, listenerPlayerId=2,
                mode='RECEIVER', timebaseId='TEST_CLOCK',
                binding=dict(leaseId=11, txGeneration=1, rxGeneration=1,
                             txDeviceId=101, txTransceiverIndex=0, rxDeviceId=202,
                             rxTransceiverIndex=0, networkTypeId=1, channelId=1),
                channel=dict(tx=dict(deviceId=101, transceiverIndex=0),
                             rx=dict(deviceId=202, transceiverIndex=0),
                             frequencyMHz=150.0, bandwidthHz=25000.0,
                             profileId='TEST_ONLY', profileVersion=1, environmentRevision=1))


def mapping(local=100, server=100, error=0):
    return dict(mappingId='SYNTHETIC_TEST_ONLY', timebaseId='TEST_CLOCK',
                localAnchorMs=local, serverAnchorMs=server, validFromLocalMs=local,
                validUntilLocalMs=local + 2000, maxErrorUs=error)


def sequence(values):
    values = iter(values)
    calls = []

    def take(link):
        calls.append(link)
        return next(values)

    return take, calls


def query(now=110, sub=0):
    return dict(tx=dict(deviceId=101, transceiverIndex=0),
                rx=dict(deviceId=202, transceiverIndex=0), frequencyMHz=150.0,
                bandwidthHz=25000.0, profileId='TEST_ONLY', profileVersion=1,
                environmentRevision=1, nowMs=now, subUs=sub)


def second_link():
    s, e = snapshot(), expected()
    s.update(linkId=2, routeId=2, listenerPlayerId=3)
    s['binding']['rxDeviceId'] = s['descriptor']['b']['deviceId'] = 203
    e.update(linkId=2, listenerPlayerId=3)
    e['binding']['rxDeviceId'] = e['channel']['rx']['deviceId'] = 203
    return s, e


class IndependentTemporalTests(unittest.TestCase):
    def receiver(self, s=None, at=100):
        rx = temporal.Receiver({1: expected()}, dict(LIMITS))
        self.assertEqual(rx.full_resync(at, 7, 2, [snapshot() if s is None else s])['decision'], 'ACCEPT_RESYNC')
        return rx

    def test_I01_descriptor_only_publication(self):
        pub = temporal.Publisher({1: expected()}, dict(LIMITS))
        take, calls = sequence([1, 2])
        self.assertEqual(len(pub.publish(100, [snapshot()], take)['snapshots']), 1)
        changed = snapshot()
        changed['descriptor']['components'][0]['phaseQ'] = 268435456
        out = pub.publish(101, [changed], take)
        self.assertEqual((out['code'], len(out['snapshots']), out['sequenceCalls']), ('OK', 1, 1))
        self.assertEqual(out['snapshots'][0]['linkSeq'], 2)
        self.assertEqual(out['snapshots'][0]['quality'], .75)
        self.assertEqual(out['snapshots'][0]['descriptor']['components'][0]['phaseQ'], 268435456)
        repeat = pub.publish(102, [changed], take)
        self.assertEqual((repeat['code'], repeat['snapshots'], repeat['sequenceCalls']), ('OK', [], 0))
        self.assertEqual(calls, [1, 1])

    def test_I02_signed_zero_canonical_noop(self):
        pub = temporal.Publisher({1: expected()}, dict(LIMITS))
        take, calls = sequence([1])
        s = snapshot()
        s['descriptor']['components'][0]['powerWeight'] = 0.0
        self.assertEqual(pub.publish(100, [s], take)['code'], 'OK')
        s['descriptor']['components'][0]['powerWeight'] = -0.0
        out = pub.publish(101, [s], take)
        self.assertEqual((out['code'], out['snapshots'], calls), ('OK', [], [1]))

    def test_I03_keepalive_preserves_physical_window(self):
        pub = temporal.Publisher({1: expected()}, dict(LIMITS))
        take, calls = sequence([1, 2])
        first = pub.publish(100, [snapshot()], take)['snapshots'][0]
        rx = self.receiver(first)
        second = pub.publish(600, [snapshot()], take)['snapshots'][0]
        self.assertEqual(rx.receive(600, second)['decision'], 'ACCEPT_NEWER')
        d = rx.read(1)['descriptor']
        self.assertEqual((d['anchorMs'], d['validFromMs'], d['validUntilMs']), (100, 100, 1200))
        self.assertEqual(rx.visible(1200, 1), dict(visible=True, effects=[]))
        self.assertEqual(rx.view(1200, 1, mapping())['code'], 'STALE_DESCRIPTOR')
        self.assertEqual(rx.visible(1600, 1), dict(visible=False, effects=[]))
        self.assertEqual(rx.view(1600, 1, mapping())['code'], 'PRESENTATION_EXPIRED')
        self.assertEqual(calls, [1, 1])

    def test_I04_rebase_manual_phase_and_composition(self):
        s = snapshot()
        d = s['descriptor']
        d['staticChannel'] = False
        d['components'][0]['dopplerQ'] = 1
        rebased = cd.rebase(d, 101)
        self.assertEqual(rebased['code'], 'OK')
        moved = rebased['descriptor']
        self.assertEqual((moved['anchorMs'], moved['validFromMs'], moved['validUntilMs']), (101, 101, 1200))
        self.assertEqual(moved['components'][0]['phaseQ'], 1000)
        rx = self.receiver(s)
        self.assertEqual(rx.view(110, 1, mapping(), sub_us=123)['result']['phasesQ'], [10123])
        newer = copy.deepcopy(s)
        newer.update(linkSeq=2, descriptor=moved)
        self.assertEqual(rx.receive(110, newer)['decision'], 'ACCEPT_NEWER')
        self.assertEqual(rx.view(110, 1, mapping(), sub_us=123)['result']['phasesQ'], [10123])
        self.assertEqual(cd.rebase(moved, 110)['descriptor']['components'][0]['phaseQ'], 10000)
        self.assertEqual(cd.rebase(d, 1200), dict(code='INVALID_ANCHOR'))

    def test_I05_rekey_does_not_reset_manual_phase(self):
        s = snapshot()
        s['descriptor']['staticChannel'] = False
        s['descriptor']['components'][0]['dopplerQ'] = 1
        s.update(sessionEpoch=8, linkId=20, routeId=20)
        s['binding']['leaseId'] = 99
        e = expected()
        e.update(sessionEpoch=8, linkId=20)
        e['binding']['leaseId'] = 99
        rx = temporal.Receiver({20: e}, dict(LIMITS))
        self.assertEqual(rx.full_resync(100, 8, 2, [s])['decision'], 'ACCEPT_RESYNC')
        self.assertEqual(rx.read(20)['descriptor']['anchorMs'], 100)
        self.assertEqual(rx.view(110, 20, mapping())['result']['phasesQ'], [10000])

    def test_I06_duplicate_and_old_update_do_not_refresh_ttl(self):
        rx = self.receiver(at=0)
        newer = snapshot()
        newer['linkSeq'] = 2
        newer['descriptor']['components'][0]['phaseQ'] = 268435456
        self.assertEqual(rx.receive(10, newer)['decision'], 'ACCEPT_NEWER')
        self.assertEqual(rx.receive(20, newer)['decision'], 'STALE')
        self.assertEqual(rx.receive(30, snapshot())['decision'], 'STALE')
        self.assertEqual(rx.read(1)['descriptor']['components'][0]['phaseQ'], 268435456)
        self.assertEqual(rx.legacy.slots[1][1], 10)
        self.assertFalse(rx.visible(1010, 1)['visible'])

    def test_I07_nested_input_output_and_registry_are_detached(self):
        pub = temporal.Publisher({1: expected()}, dict(LIMITS))
        take, _ = sequence([1])
        s = snapshot()
        out = pub.publish(100, [s], take)
        rx = self.receiver(out['snapshots'][0])
        s['descriptor']['a']['deviceId'] = 999
        s['descriptor']['components'][0]['phaseQ'] = 42
        out['snapshots'][0]['descriptor']['components'].clear()
        readback = rx.read(1)
        readback['descriptor']['b']['transceiverIndex'] = 7
        pub_copy = pub.read(1)
        pub_copy['descriptor']['components'].clear()
        for retained in (pub.read(1), rx.read(1)):
            self.assertEqual(retained['descriptor']['a']['deviceId'], 101)
            self.assertEqual(retained['descriptor']['b']['transceiverIndex'], 0)
            self.assertEqual(retained['descriptor']['components'][0]['phaseQ'], 0)

    def test_I08_late_bad_full_resync_clears_old_and_new_state(self):
        rx = self.receiver()
        first = snapshot()
        late = snapshot()
        late.update(linkId=2, routeId=2, listenerPlayerId=3)
        late['binding']['rxDeviceId'] = late['descriptor']['b']['deviceId'] = 203
        late['descriptor']['components'][0]['phaseQ'] = 1073741824
        e2 = expected()
        e2.update(linkId=2, listenerPlayerId=3)
        e2['binding']['rxDeviceId'] = e2['channel']['rx']['deviceId'] = 203
        rx = temporal.Receiver({1: expected(), 2: e2}, dict(LIMITS))
        self.assertEqual(rx.full_resync(0, 7, 2, [first])['decision'], 'ACCEPT_RESYNC')
        out = rx.full_resync(20, 7, 2, [first, late])
        self.assertEqual(out['decision'], 'INVALID_RESYNC')
        self.assertEqual(set(out['effects']), {'REQUEST_RESYNC', 'DIAGNOSTIC'})
        self.assertEqual(out['slots'], {})
        self.assertIsNone(rx.read(1))
        self.assertIsNone(rx.read(2))
        self.assertIsNone(rx.legacy.epoch)
        self.assertEqual(rx.receive(30, first)['decision'], 'INVALID_IDENTITY')

    def test_I09_late_publication_error_has_no_sequence_calls(self):
        first = snapshot()
        late = snapshot()
        late.update(linkId=2, routeId=2, listenerPlayerId=3)
        late['binding']['rxDeviceId'] = late['descriptor']['b']['deviceId'] = 203
        late['descriptor']['components'][0]['phaseQ'] = 1073741824
        e2 = expected()
        e2.update(linkId=2, listenerPlayerId=3)
        e2['binding']['rxDeviceId'] = e2['channel']['rx']['deviceId'] = 203
        pub = temporal.Publisher({1: expected(), 2: e2}, dict(LIMITS))
        take, calls = sequence([])
        before = copy.deepcopy([first, late])
        out = pub.publish(100, [first, late], take)
        self.assertEqual((out['code'], out['snapshots'], out['sequenceCalls']), ('INVALID_DESCRIPTOR', [], 0))
        self.assertEqual(calls, [])
        self.assertEqual(pub.entries, {})
        self.assertEqual([first, late], before)

    def test_I10_unsupported_snapshot_schema_does_not_downgrade(self):
        rx = self.receiver()
        for version in (1, 3):
            with self.subTest(version=version):
                s = snapshot()
                s.update(schemaVersion=version, linkSeq=2)
                self.assertEqual(temporal.validate(s, expected(), dict(LIMITS))['code'], 'UNSUPPORTED_SCHEMA')
                out = rx.receive(110, s)
                self.assertEqual((out['decision'], out['reason']), ('INVALID_SNAPSHOT', 'UNSUPPORTED_SCHEMA'))
                self.assertIn('REQUEST_RESYNC', out['effects'])
                self.assertEqual(rx.read(1)['linkSeq'], 1)

    def test_I11_unsupported_model_has_no_descriptor_or_bytes(self):
        s = snapshot()
        s['descriptor']['modelId'] = 'OTHER_MODEL'
        out = temporal.validate(s, expected(), dict(LIMITS))
        self.assertEqual(out['code'], 'UNSUPPORTED_MODEL')
        self.assertNotIn('snapshot', out)

    def test_I12_descriptor_version_is_independent_from_snapshot_schema(self):
        s = snapshot()
        s['descriptorVersion'] = 2
        self.assertEqual(temporal.validate(s, expected(), dict(LIMITS))['code'], 'UNSUPPORTED_DESCRIPTOR_VERSION')
        raw = list(GOLDEN_D0)
        raw[4] = 2
        self.assertEqual(cd.decode(raw, 5120), dict(code='UNSUPPORTED_VERSION'))

    def test_I13_manual_golden_and_exact_budget(self):
        self.assertEqual(len(GOLDEN_D0), 102)
        self.assertEqual(cd.encode(snapshot()['descriptor'], 102)['bytes'], GOLDEN_D0)
        self.assertEqual(temporal.validate(snapshot(), expected(), dict(LIMITS))['code'], 'OK')
        tight = dict(LIMITS, maxDescriptorBytes=101)
        self.assertEqual(temporal.validate(snapshot(), expected(), tight)['code'], 'RESOURCE_LIMIT')

    def test_I14_codec_size_priority_and_trailing_data(self):
        self.assertEqual(cd.decode([True, 999], 1), dict(code='RESOURCE_LIMIT'))
        self.assertEqual(cd.decode([True, 999], 5120), dict(code='INVALID_FRAME'))
        self.assertEqual(cd.decode(GOLDEN_D0 + [0], 5120), dict(code='INVALID_FRAME'))

    def test_I15_missing_and_ill_typed_descriptor_never_fall_back(self):
        for value in (None, {}, []):
            with self.subTest(value=value):
                s = snapshot()
                s['descriptor'] = value
                self.assertEqual(temporal.validate(s, expected(), dict(LIMITS))['code'], 'INVALID_DESCRIPTOR')
        s = snapshot()
        s.pop('descriptor')
        # Frozen I15 includes the removed descriptor field under invalid descriptor.
        self.assertEqual(temporal.validate(s, expected(), dict(LIMITS))['code'], 'INVALID_DESCRIPTOR')

    def test_I16_empty_components_remain_valid_empty_medium(self):
        s = snapshot()
        s['descriptor']['components'] = []
        self.assertEqual(temporal.validate(s, expected(), dict(LIMITS))['code'], 'OK')
        self.assertEqual(cd.encode(s['descriptor'], 5120)['sizeBytes'], 83)
        out = self.receiver(s).view(110, 1, mapping())
        self.assertEqual(out['code'], 'EMPTY_MEDIUM')
        self.assertEqual((out['result']['real'], out['result']['imag'], out['result']['gainDb']), (0.0, 0.0, None))

    def test_I17_strict_descriptor_domains(self):
        for key, value in (('anchorMs', 100.0), ('validUntilMs', 2147483648),
                           ('frequencyMHz', float('nan')), ('unexpected', 1)):
            with self.subTest(key=key, value=value):
                s = snapshot()
                s['descriptor'][key] = value
                self.assertEqual(temporal.validate(s, expected(), dict(LIMITS))['code'], 'INVALID_DESCRIPTOR')
        s = snapshot()
        s['descriptor']['a']['deviceId'] = True
        self.assertEqual(temporal.validate(s, expected(), dict(LIMITS))['code'], 'INVALID_DESCRIPTOR')

    def test_I18_coverage_resource_and_order_remain_distinct(self):
        for key, value, code in (('coverageComplete', False, 'INCOMPLETE_COVERAGE'),
                                 ('resourceExhausted', True, 'RESOURCE_EXHAUSTED')):
            with self.subTest(code=code):
                s = snapshot()
                s['descriptor'][key] = value
                self.assertEqual(temporal.validate(s, expected(), dict(LIMITS))['code'], 'OK')
                self.assertEqual(self.receiver(s).view(110, 1, mapping())['code'], code)
        s = snapshot()
        s['descriptor']['components'] = [dict(componentId=name, powerWeight=1.0, phaseQ=0, dopplerQ=0)
                                          for name in ('z', 'a')]
        self.assertEqual(temporal.validate(s, expected(), dict(LIMITS))['code'], 'INVALID_DESCRIPTOR')

    def test_I19_storage_does_not_invent_clock_mapping(self):
        rx = self.receiver(at=10000)
        self.assertTrue(rx.visible(10010, 1)['visible'])
        out = rx.view(10010, 1, None)
        self.assertEqual((out['code'], out['visible']), ('CLOCK_UNVERIFIED', True))
        self.assertNotIn('result', out)
        self.assertEqual(rx.read(1)['descriptor']['anchorMs'], 100)

    def test_I20_half_open_and_mapping_uncertainty_boundaries(self):
        s = snapshot()
        s['ttlMs'] = 2000
        rx = self.receiver(s, at=10000)
        m = mapping(local=10000, server=100)
        self.assertEqual(rx.view(10000, 1, m)['code'], 'OK')
        self.assertEqual(rx.view(11099, 1, m, sub_us=999)['code'], 'OK')
        self.assertEqual(rx.view(11100, 1, m)['code'], 'STALE_DESCRIPTOR')
        uncertain = dict(m, maxErrorUs=1)
        self.assertEqual(rx.view(11099, 1, uncertain, sub_us=999)['code'], 'STALE_DESCRIPTOR')
        self.assertEqual(rx.view(10000, 1, uncertain)['code'], 'STALE_DESCRIPTOR')

    def test_I21_and_I26_two_directed_links_share_canonical_pair(self):
        reverse = snapshot()
        reverse.update(linkId=2, routeId=2, talkerPlayerId=2, listenerPlayerId=1, quality=.5)
        reverse['binding'].update(leaseId=12, txDeviceId=202, rxDeviceId=101)
        e2 = expected()
        e2.update(linkId=2, talkerPlayerId=2, listenerPlayerId=1)
        e2['binding'].update(leaseId=12, txDeviceId=202, rxDeviceId=101)
        e2['channel'].update(tx=dict(deviceId=202, transceiverIndex=0), rx=dict(deviceId=101, transceiverIndex=0))
        self.assertEqual(temporal.validate(reverse, e2, dict(LIMITS))['code'], 'OK')
        pub = temporal.Publisher({1: expected(), 2: e2}, dict(LIMITS))
        take, calls = sequence([1, 1])
        out = pub.publish(100, [snapshot(), reverse], take)
        self.assertEqual((out['code'], len(out['snapshots']), out['sequenceCalls']), ('OK', 2, 2))
        self.assertEqual(calls, [1, 2])
        self.assertEqual([s['quality'] for s in out['snapshots']], [.75, .5])
        for s in out['snapshots']:
            self.assertEqual(s['descriptor']['a'], dict(deviceId=101, transceiverIndex=0))
            self.assertEqual(s['descriptor']['b'], dict(deviceId=202, transceiverIndex=0))
        self.assertEqual(cf.evaluate(dict(descriptor=reverse['descriptor'], query=e2['channel'] | dict(nowMs=110, subUs=0)))['phasesQ'], [0])

    def test_I22_zero_at_sample_keeps_future_nonzero(self):
        s = snapshot()
        s['descriptor']['staticChannel'] = False
        s['descriptor']['components'] = [dict(componentId='a', powerWeight=1.0, phaseQ=0, dopplerQ=0),
                                          dict(componentId='b', powerWeight=1.0, phaseQ=536870912, dopplerQ=268435456)]
        wide = dict(LIMITS, maxDescriptorBytes=5120, maxTotalDescriptorBytes=20480)
        rx = temporal.Receiver({1: expected()}, wide)
        self.assertEqual(rx.full_resync(100, 7, 2, [s])['decision'], 'ACCEPT_RESYNC')
        zero = rx.view(100, 1, mapping())
        self.assertEqual((zero['code'], zero['result']['phasesQ']), ('ZERO_AT_SAMPLE', [0, 536870912]))
        nonzero = rx.view(100, 1, mapping(), sub_us=1)
        self.assertEqual((nonzero['code'], nonzero['result']['phasesQ']), ('OK', [0, 805306368]))
        self.assertIsNotNone(rx.read(1))

    def test_I24_rf_context_is_not_replaced_by_link_identity(self):
        for key, value in (('frequencyMHz', 151.0), ('bandwidthHz', 12500.0),
                           ('profileVersion', 2), ('environmentRevision', 2)):
            with self.subTest(field=key):
                s = snapshot()
                s['descriptor'][key] = value
                self.assertEqual(temporal.validate(s, expected(), dict(LIMITS))['code'], 'CONTEXT_MISMATCH')

    def test_I25_codec_integrity_precedes_version(self):
        raw = list(GOLDEN_D0)
        raw[4] = 2
        self.assertEqual(cd.decode(raw + [0], 5120), dict(code='INVALID_FRAME'))
        self.assertEqual(cd.decode(raw, 101), dict(code='RESOURCE_LIMIT'))

    def test_I27_lost_quality_zero_preserves_temporal_data(self):
        s = snapshot()
        s.update(state='LOST', quality=0.0)
        self.assertEqual(temporal.validate(s, expected(), dict(LIMITS))['code'], 'OK')
        rx = self.receiver(s)
        self.assertEqual(rx.read(1)['descriptor']['anchorMs'], 100)
        out = rx.view(110, 1, mapping())
        self.assertEqual((out['code'], out['result']['phasesQ']), ('OK', [0]))

    def test_I28_cf_accepts_same_device_different_transceivers(self):
        d = copy.deepcopy(MANUAL['fixtures']['D0'])
        d['b'].update(deviceId=101, transceiverIndex=1)
        self.assertEqual(cd.encode(d, 102)['code'], 'OK')
        q = query()
        q['rx'] = dict(deviceId=101, transceiverIndex=1)
        out = cf.evaluate(dict(descriptor=d, query=q))
        self.assertEqual((out['code'], out['phasesQ']), ('OK', [0]))

    def test_strict_full_resync_headers_cannot_coerce_float_or_bool(self):
        for epoch, schema in ((7, 2.0), (True, 2), (7, True)):
            with self.subTest(epoch=epoch, schema=schema):
                rx = self.receiver()
                out = rx.full_resync(110, epoch, schema, [snapshot()])
                self.assertEqual(out['decision'], 'INVALID_RESYNC')
                self.assertEqual(out['slots'], {})
                self.assertIsNone(rx.legacy.epoch)
        rx = self.receiver()
        out = rx.full_resync(110, 7, 3, [])
        self.assertEqual((out['decision'], out['reason']), ('INVALID_RESYNC', 'UNSUPPORTED_SCHEMA'))
        self.assertEqual(out['slots'], {})
        self.assertIsNone(rx.legacy.epoch)

    def test_unhashable_link_id_rejected_without_exception(self):
        s = snapshot()
        s['linkId'] = []
        self.assertEqual(temporal.validate(s, expected(), dict(LIMITS))['code'], 'INVALID_SNAPSHOT')
        rx = self.receiver()
        self.assertEqual(rx.receive(110, s)['decision'], 'INVALID_SNAPSHOT')
        self.assertEqual(rx.read(1)['linkSeq'], 1)

    def test_expected_channel_must_not_supply_hidden_time_or_extra_keys(self):
        for field, value in (('nowMs', 110), ('subUs', 123), ('unexpected', 1)):
            with self.subTest(field=field):
                e = expected()
                e['channel'][field] = value
                self.assertEqual(temporal.validate(snapshot(), e, dict(LIMITS))['code'], 'CONTEXT_MISMATCH')

    def test_retained_state_count_and_total_byte_caps_before_single_commit(self):
        second, e2 = second_link()
        for limits in (dict(LIMITS, maxSnapshots=1), dict(LIMITS, maxTotalDescriptorBytes=153)):
            with self.subTest(limits=limits):
                rx = temporal.Receiver({1: expected(), 2: e2}, limits)
                self.assertEqual(rx.full_resync(100, 7, 2, [snapshot()])['decision'], 'ACCEPT_RESYNC')
                out = rx.receive(110, second)
                self.assertEqual(out['decision'], 'INVALID_SNAPSHOT')
                self.assertEqual(rx.read(1)['linkSeq'], 1)
                self.assertIsNone(rx.read(2))
                self.assertEqual(rx.legacy.slots[1][1], 100)

    def test_retained_state_caps_precede_publication_callback(self):
        second, e2 = second_link()
        for limits in (dict(LIMITS, maxSnapshots=1), dict(LIMITS, maxTotalDescriptorBytes=153)):
            with self.subTest(limits=limits):
                pub = temporal.Publisher({1: expected(), 2: e2}, limits)
                take, calls = sequence([1])
                self.assertEqual(pub.publish(100, [snapshot()], take)['code'], 'OK')
                out = pub.publish(110, [second], take)
                self.assertNotEqual(out['code'], 'OK')
                self.assertEqual((out['snapshots'], out['sequenceCalls'], calls), ([], 0, [1]))
                self.assertEqual(pub.read(1)['linkSeq'], 1)
                self.assertIsNone(pub.read(2))

    def test_expected_map_keys_are_strict_positive_integers(self):
        for key in (True, 0, '1', None):
            with self.subTest(key=key):
                rx = temporal.Receiver({key: expected()}, dict(LIMITS))
                out = rx.full_resync(100, 7, 2, [snapshot()])
                self.assertEqual(out['decision'], 'INVALID_RESYNC')
                self.assertEqual(out['reason'], 'INVALID_INPUT')
                self.assertEqual(out['slots'], {})
                pub = temporal.Publisher({key: expected()}, dict(LIMITS))
                take, calls = sequence([])
                out = pub.publish(100, [snapshot()], take)
                self.assertEqual((out['code'], out['snapshots'], calls), ('INVALID_INPUT', [], []))

    def test_callback_ordinary_exceptions_do_not_commit_partial_outputs(self):
        second, e2 = second_link()
        for exception in (KeyError('external sequence'), OSError('external sequence')):
            with self.subTest(exception=type(exception).__name__):
                pub = temporal.Publisher({1: expected(), 2: e2}, dict(LIMITS))
                take, _ = sequence([1])
                self.assertEqual(pub.publish(100, [snapshot()], take)['code'], 'OK')
                changed = snapshot()
                changed['descriptor']['components'][0]['phaseQ'] = 268435456
                calls = []

                def failing(link):
                    calls.append(link)
                    if len(calls) == 1:
                        return 2
                    raise exception

                out = pub.publish(101, [changed, second], failing)
                self.assertEqual((out['code'], out['snapshots'], out['sequenceCalls']), ('NEED_FULL_RESYNC', [], 2))
                self.assertEqual(calls, [1, 2])
                self.assertEqual(pub.read(1)['linkSeq'], 1)
                self.assertEqual(pub.read(1)['descriptor']['components'][0]['phaseQ'], 0)
                self.assertIsNone(pub.read(2))

    def test_real_negative_controls_detect_twelve_broken_paths(self):
        publish = temporal.Publisher.publish
        receive = temporal.Receiver.receive
        read = temporal.Receiver.read
        full = temporal.Receiver.full_resync
        view = temporal.Receiver.view
        validate = temporal.validate
        rebase = cd.rebase

        def descriptor_trigger_disabled(pub, now, candidates, next_seq, **kwargs):
            rewritten = copy.deepcopy(candidates)
            for s in rewritten:
                old = pub.read(s['linkId'])
                if old is not None:
                    s['descriptor'] = old['descriptor']
            return publish(pub, now, rewritten, next_seq, **kwargs)

        def raw_negative_zero_publishes(pub, now, candidates, next_seq, **kwargs):
            if any(math.copysign(1.0, c['descriptor']['components'][0]['powerWeight']) < 0 for c in candidates):
                return dict(code='OK', snapshots=copy.deepcopy(candidates), sequenceCalls=1)
            return publish(pub, now, candidates, next_seq, **kwargs)

        def keepalive_extends_physical_window(pub, now, candidates, next_seq, **kwargs):
            had_previous = bool(pub.entries)
            out = publish(pub, now, candidates, next_seq, **kwargs)
            if had_previous and out['code'] == 'OK':
                for s in out['snapshots']:
                    s['descriptor']['validUntilMs'] += s['ttlMs']
                    pub.entries[s['linkId']][0]['descriptor']['validUntilMs'] += s['ttlMs']
            return out

        def stale_refreshes_ttl(rx, now, s):
            out = receive(rx, now, s)
            if out['decision'] == 'STALE':
                retained = rx.legacy.slots[s['linkId']][0]
                rx.legacy.slots[s['linkId']] = retained, now
            return out

        def shallow_read(rx, link):
            slot = rx.legacy.slots.get(link)
            return slot[0] if slot else None

        def invalid_full_keeps_old(rx, now, epoch, schema, snapshots):
            old_epoch, old_slots = rx.legacy.epoch, copy.deepcopy(rx.legacy.slots)
            out = full(rx, now, epoch, schema, snapshots)
            if out['decision'] == 'INVALID_RESYNC':
                rx.legacy.epoch, rx.legacy.slots = old_epoch, old_slots
            return out

        def callback_before_preflight(pub, now, candidates, next_seq, **kwargs):
            if candidates:
                try:
                    next_seq(candidates[0]['linkId'])
                except StopIteration:
                    pass
            return publish(pub, now, candidates, next_seq, **kwargs)

        def rejects_empty(s, e, limits):
            if isinstance(s, dict) and isinstance(s.get('descriptor'), dict) and s['descriptor'].get('components') == []:
                return dict(code='INVALID_DESCRIPTOR')
            return validate(s, e, limits)

        def rejects_reverse_direction(s, e, limits):
            if s['binding']['txDeviceId'] > s['binding']['rxDeviceId']:
                return dict(code='CONTEXT_MISMATCH')
            return validate(s, e, limits)

        def drops_lost_temporal(s, e, limits):
            if s['state'] == 'LOST' or s['quality'] == 0:
                return dict(code='INVALID_DESCRIPTOR')
            return validate(s, e, limits)

        def invents_clock(rx, now, link, m, sub_us=0):
            return view(rx, now, link, mapping(local=10000, server=100) if m is None else m, sub_us=sub_us)

        def rebase_resets_phase(d, anchor):
            out = rebase(d, anchor)
            if out['code'] == 'OK':
                out['descriptor']['components'][0]['phaseQ'] = 0
            return out

        controls = [
            ('NC01', temporal.Publisher, 'publish', descriptor_trigger_disabled, 'test_I01_descriptor_only_publication'),
            ('NC02', temporal.Publisher, 'publish', raw_negative_zero_publishes, 'test_I02_signed_zero_canonical_noop'),
            ('NC03', temporal.Publisher, 'publish', keepalive_extends_physical_window, 'test_I03_keepalive_preserves_physical_window'),
            ('NC04', cd, 'rebase', rebase_resets_phase, 'test_I04_rebase_manual_phase_and_composition'),
            ('NC05', temporal.Receiver, 'receive', stale_refreshes_ttl, 'test_I06_duplicate_and_old_update_do_not_refresh_ttl'),
            ('NC06', temporal.Receiver, 'read', shallow_read, 'test_I07_nested_input_output_and_registry_are_detached'),
            ('NC07', temporal.Receiver, 'full_resync', invalid_full_keeps_old, 'test_I08_late_bad_full_resync_clears_old_and_new_state'),
            ('NC08', temporal.Publisher, 'publish', callback_before_preflight, 'test_I09_late_publication_error_has_no_sequence_calls'),
            ('NC09', temporal, 'validate', rejects_empty, 'test_I16_empty_components_remain_valid_empty_medium'),
            ('NC10', temporal.Receiver, 'view', invents_clock, 'test_I19_storage_does_not_invent_clock_mapping'),
            ('NC11', temporal, 'validate', rejects_reverse_direction, 'test_I21_and_I26_two_directed_links_share_canonical_pair'),
            ('NC12', temporal, 'validate', drops_lost_temporal, 'test_I27_lost_quality_zero_preserves_temporal_data'),
        ]
        for name, target, attribute, broken, method in controls:
            with self.subTest(control=name):
                result = unittest.TestResult()
                with patch.object(target, attribute, broken):
                    IndependentTemporalTests(method).run(result)
                self.assertEqual(result.errors, [], name + ': an incidental exception is not a detected semantic failure')
                self.assertTrue(result.failures, name + ': broken path escaped the independent assertion')


if __name__ == '__main__':
    unittest.main()
