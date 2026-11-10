"""Independent manual LINK_END/fresh-FULL expectations, frozen before source read.

LC01-LC30: CONTRACT_COUNTERS_RU.md initial SHA4c67d051... . Authority
fixtures below are complete trusted POST-state inputs, never copied from packets.
"""
import copy
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
if (HERE / 'CONTRACT_COUNTERS_RU.md').exists():
    sys.path[:0] = [str(HERE.parent / 'core'), str(HERE.parents[2].parent / 'RICON-Reforger' / 'oracle')]
else:
    sys.path.insert(0, str(HERE))
import temporal_lifecycle_oracle as lifecycle

MAX = 2147483647
LIMITS = dict(maxSnapshots=8, maxDescriptorBytes=102, maxTotalDescriptorBytes=816)


def binding(link=1, generation=1, lease=None):
    return dict(leaseId=10 + link if lease is None else lease, txGeneration=generation,
                rxGeneration=1, txDeviceId=101, txTransceiverIndex=0,
                rxDeviceId=201 + link, rxTransceiverIndex=0, networkTypeId=1, channelId=1)


def snapshot(link=1, seq=5, epoch=7, generation=1, lease=None, phase=0):
    return dict(schemaVersion=2, sessionEpoch=epoch, linkId=link, linkSeq=seq,
                ttlMs=1000, routeId=link, routeNodeCount=2, talkerPlayerId=1,
                listenerPlayerId=2, state='CLEAN', quality=.75, mode='RECEIVER',
                timebaseId='TEST_CLOCK', binding=binding(link, generation, lease),
                descriptorVersion=1,
                descriptor=dict(modelId='FINITE_MULTIPATH_V1', profileId='TEST_ONLY',
                                profileVersion=1, a=dict(deviceId=101, transceiverIndex=0),
                                b=dict(deviceId=201 + link, transceiverIndex=0),
                                frequencyMHz=150.0, bandwidthHz=25000.0,
                                environmentRevision=1, anchorMs=100, validFromMs=100,
                                validUntilMs=1200, staticChannel=True, coverageComplete=True,
                                resourceExhausted=False,
                                components=[dict(componentId='direct', powerWeight=1.0,
                                                 phaseQ=phase, dopplerQ=0)]))


def expected(link=1, epoch=7, generation=1, lease=None):
    return dict(sessionEpoch=epoch, linkId=link, talkerPlayerId=1, listenerPlayerId=2,
                mode='RECEIVER', timebaseId='TEST_CLOCK', binding=binding(link, generation, lease),
                channel=dict(tx=dict(deviceId=101, transceiverIndex=0),
                             rx=dict(deviceId=201 + link, transceiverIndex=0),
                             frequencyMHz=150.0, bandwidthHz=25000.0,
                             profileId='TEST_ONLY', profileVersion=1, environmentRevision=1))


def end(link=1, seq=6, epoch=7, generation=1, lease=None, reason='CLIENT_STOP'):
    return dict(sessionEpoch=epoch, linkId=link, linkSeq=seq, talkerPlayerId=1,
                listenerPlayerId=2, binding=binding(link, generation, lease), reason=reason)


def active_item(s):
    return dict(kind='ACTIVE', snapshot=copy.deepcopy(s))


def end_item(e):
    return dict(kind='END', end=copy.deepcopy(e))


def frame(delivery, items, kind='DELTA', high=1, epoch=7, owner=2):
    return dict(protocolVersion=1, kind=kind, ownerPlayerId=owner, sessionEpoch=epoch,
                deliverySeq=delivery, linkIdHighWater=high, items=copy.deepcopy(items))


def authority(delivery, post_ids=(), ended_ids=(), kind='DELTA', high=1, epoch=7):
    # Independent authoritative literals: caller chooses post_ids, not packet rows.
    return dict(ownerPlayerId=2, sessionEpoch=epoch, deliverySeq=delivery, kind=kind,
                linkIdHighWater=high,
                activeExpected={i: expected(i, epoch=epoch) for i in post_ids},
                endedTargets={i: end(i, epoch=epoch) for i in ended_ids})


def data(state):
    recovery = {'needsFull', 'seenHighWater', 'observedEpoch', 'observedDeliverySeq', 'observedHighWater'}
    return {k: v for k, v in state.items() if k not in recovery}


class IndependentLifecycleTests(unittest.TestCase):
    def base(self, limits=None, delivery=10, high=1, ids=(1,), epoch=7):
        rx = lifecycle.OrderedReceiver(2, dict(LIMITS if limits is None else limits))
        packet = frame(delivery, [active_item(snapshot(i, epoch=epoch)) for i in ids],
                       kind='FULL', high=high, epoch=epoch)
        proof = authority(delivery, ids, kind='FULL', high=high, epoch=epoch)
        self.assertEqual(rx.apply(100, packet, proof)['code'], 'ACCEPT_FULL')
        return rx

    def closed(self):
        rx = self.base()
        out = rx.apply(110, frame(11, [end_item(end())]), authority(11, (), (1,)))
        self.assertEqual(out['code'], 'ACCEPT_DELTA')
        self.assertIsNone(rx.read(1))
        self.assertEqual(rx.state()['closed'][1]['linkSeq'], 6)
        return rx

    def test_LC01_active_lost_is_not_end(self):
        rx = self.base()
        s = snapshot(seq=6)
        s.update(state='LOST', quality=0.0)
        self.assertEqual(rx.apply(110, frame(11, [active_item(s)]), authority(11, (1,)))['code'], 'ACCEPT_DELTA')
        self.assertEqual(rx.read(1)['descriptor']['anchorMs'], 100)
        self.assertEqual(rx.read(1)['state'], 'LOST')
        self.assertEqual(rx.state()['closed'], {})
        self.assertTrue(rx.visible(110, 1))

    def test_LC02_end_removes_slot_and_retains_floor(self):
        rx = self.closed()
        st = rx.state()
        self.assertEqual((st['sessionEpoch'], st['deliverySeq']), (7, 11))
        self.assertEqual(st['active'], {})
        self.assertEqual(st['closed'][1], end())

    def test_LC03_LC04_closed_id_rejects_older_and_even_newer_active(self):
        for seq in (5, 7):
            with self.subTest(seq=seq):
                rx = self.closed()
                before = rx.state()
                out = rx.apply(120, frame(12, [active_item(snapshot(seq=seq))]), authority(12, (1,)))
                self.assertEqual(out['code'], 'CLOSED_ID')
                self.assertEqual(data(rx.state()), data(before))
                self.assertIsNone(rx.read(1))

    def test_LC05_replayed_end_has_no_effect(self):
        rx = self.closed()
        before = rx.state()
        self.assertEqual(rx.apply(900, frame(11, [end_item(end())]), authority(11, (), (1,)))['code'], 'STALE')
        self.assertEqual(rx.state(), before)

    def test_end_same_body_in_new_delivery_is_idempotent(self):
        rx = self.closed()
        out = rx.apply(120, frame(12, [end_item(end())]), authority(12, (), (1,)))
        self.assertEqual(out['code'], 'ACCEPT_DELTA')
        self.assertEqual(rx.state()['closed'][1], end())
        self.assertEqual(rx.state()['active'], {})

    def test_equal_end_sequence_changed_reason_is_conflict(self):
        rx = self.closed()
        changed = end(reason='ADMIN_REVOKED')
        proof = authority(12, (), (1,))
        proof['endedTargets'][1] = end(reason='ADMIN_REVOKED')
        before = rx.state()
        out = rx.apply(120, frame(12, [end_item(changed)]), proof)
        self.assertEqual(out['code'], 'LINK_SEQUENCE_CONFLICT')
        self.assertEqual(data(rx.state()), data(before))

    def test_LC06_old_binding_end_cannot_close_current_lease(self):
        rx = self.base(high=2, ids=(2,))
        wrong = end(2, generation=1, lease=11)
        # Even an internally consistent receipt claiming closure cannot override old-binding pin.
        proof = authority(11, (), (2,), high=2)
        proof['endedTargets'][2] = end(2, generation=1, lease=11)
        before = rx.state()
        out = rx.apply(110, frame(11, [end_item(wrong)], high=2), proof)
        self.assertEqual(out['code'], 'BINDING_MISMATCH')
        self.assertEqual(data(rx.state()), data(before))
        self.assertEqual(rx.read(2)['binding']['leaseId'], 12)

    def test_LC07_late_bad_delta_is_atomic(self):
        rx = self.base()
        bad = end(2)
        bad['linkSeq'] = 0
        packet = frame(11, [active_item(snapshot(seq=6, phase=268435456)), end_item(bad)], high=2)
        before = rx.state()
        out = rx.apply(110, packet, authority(11, (1,), (2,), high=2))
        self.assertNotIn(out['code'], ('ACCEPT_FULL', 'ACCEPT_DELTA'))
        self.assertEqual(data(rx.state()), data(before))
        self.assertEqual(rx.read(1)['linkSeq'], 5)

    def test_LC08_late_bad_full_does_not_destroy_prior_identity(self):
        rx = self.base()
        bad = snapshot(2)
        bad['descriptor']['components'][0]['phaseQ'] = 1073741824
        before = rx.state()
        out = rx.apply(200, frame(11, [active_item(snapshot(seq=6)), active_item(bad)],
                                  kind='FULL', high=2), authority(11, (1, 2), kind='FULL', high=2))
        self.assertEqual(out['code'], 'INVALID_DESCRIPTOR')
        self.assertTrue(out['needsFull'])
        self.assertEqual(data(rx.state()), data(before))
        self.assertEqual(rx.read(1)['linkSeq'], 5)
        self.assertFalse(rx.visible(201, 1))

    def test_LC09_LC10_stale_full_does_not_clear_or_refresh(self):
        for delivery, items, ids in ((9, [], ()), (10, [active_item(snapshot())], (1,))):
            with self.subTest(delivery=delivery):
                rx = self.base()
                before = rx.state()
                out = rx.apply(900, frame(delivery, items, kind='FULL'), authority(delivery, ids, kind='FULL'))
                self.assertEqual(out['code'], 'STALE')
                self.assertEqual(rx.state(), before)
                self.assertFalse(rx.visible(1100, 1))

    def test_stale_full_preserves_already_latched_need_full(self):
        rx = self.base()
        self.assertEqual(rx.apply(110, frame(12, [active_item(snapshot(seq=6))]), authority(12, (1,)))['code'], 'GAP')
        before = rx.state()
        out = rx.apply(900, frame(10, [], kind='FULL'), authority(10, (), kind='FULL'))
        self.assertEqual(out['code'], 'STALE')
        self.assertEqual(rx.state(), before)

    def test_LC11_fresh_full_cannot_roll_back_per_link_sequence(self):
        rx = self.base()
        self.assertEqual(rx.apply(110, frame(11, [active_item(snapshot(seq=6, phase=268435456))]),
                                  authority(11, (1,)))['code'], 'ACCEPT_DELTA')
        before = rx.state()
        out = rx.apply(200, frame(12, [active_item(snapshot(seq=5))], kind='FULL'), authority(12, (1,), kind='FULL'))
        self.assertEqual(out['code'], 'LINK_SEQUENCE_ROLLBACK')
        self.assertEqual(data(rx.state()), data(before))

    def test_LC12_equal_sequence_changed_body_in_fresh_full_rejects(self):
        rx = self.base()
        changed = snapshot(seq=5, phase=268435456)
        before = rx.state()
        out = rx.apply(200, frame(11, [active_item(changed)], kind='FULL'), authority(11, (1,), kind='FULL'))
        self.assertEqual(out['code'], 'LINK_SEQUENCE_CONFLICT')
        self.assertEqual(data(rx.state()), data(before))

    def test_LC13_fresh_checkpoint_equal_body_preserves_receipt(self):
        rx = self.base()
        out = rx.apply(900, frame(11, [active_item(snapshot())], kind='FULL'), authority(11, (1,), kind='FULL'))
        self.assertEqual(out['code'], 'ACCEPT_FULL')
        self.assertEqual(rx.state()['active'][1]['receivedAtMs'], 100)
        self.assertEqual(rx.state()['deliverySeq'], 11)
        self.assertFalse(rx.visible(1100, 1))

    def test_LC14_LC15_gap_requires_full_and_preserves_cached_data(self):
        rx = self.base()
        before = rx.state()
        out = rx.apply(110, frame(12, [active_item(snapshot(seq=6))]), authority(12, (1,)))
        self.assertEqual((out['code'], out['needsFull']), ('GAP', True))
        self.assertEqual(data(rx.state()), data(before))
        self.assertFalse(rx.visible(110, 1))
        for delivery, code in ((11, 'STALE'), (13, 'NEED_FULL')):
            self.assertEqual(rx.apply(120, frame(delivery, [active_item(snapshot(seq=6))]),
                                      authority(delivery, (1,)))['code'], code)
            self.assertEqual(data(rx.state()), data(before))

    def test_LC16_fresh_full_recovers_gap_then_next_delta_works(self):
        rx = self.base()
        rx.apply(110, frame(12, [active_item(snapshot(seq=6))]), authority(12, (1,)))
        out = rx.apply(200, frame(13, [active_item(snapshot(seq=6))], kind='FULL'), authority(13, (1,), kind='FULL'))
        self.assertEqual((out['code'], out['needsFull']), ('ACCEPT_FULL', False))
        self.assertEqual(rx.apply(210, frame(14, [active_item(snapshot(seq=7))]), authority(14, (1,)))['code'], 'ACCEPT_DELTA')
        self.assertEqual(rx.read(1)['linkSeq'], 7)

    def test_LC17_delivery_sequence_does_not_wrap_or_accept_max(self):
        rx = self.base(delivery=MAX - 1)
        before = rx.state()
        out = rx.apply(110, frame(MAX, [active_item(snapshot(seq=6))]), authority(MAX, (1,)))
        self.assertNotIn(out['code'], ('ACCEPT_FULL', 'ACCEPT_DELTA'))
        self.assertEqual(data(rx.state()), data(before))

    def test_LC18_link_sequence_max_rejected_without_close(self):
        rx = lifecycle.OrderedReceiver(2, dict(LIMITS))
        self.assertEqual(rx.apply(100, frame(10, [active_item(snapshot(seq=MAX - 1))], kind='FULL'),
                                  authority(10, (1,), kind='FULL'))['code'], 'ACCEPT_FULL')
        before = rx.state()
        proof = authority(11, (), (1,))
        proof['endedTargets'][1] = end(seq=MAX)
        out = rx.apply(110, frame(11, [end_item(end(seq=MAX))]), proof)
        self.assertNotEqual(out['code'], 'ACCEPT_DELTA')
        self.assertEqual(data(rx.state()), data(before))
        self.assertIsNotNone(rx.read(1))

    def capped(self):
        rx = self.base(limits=dict(LIMITS, maxSnapshots=2))
        self.assertEqual(rx.apply(110, frame(11, [end_item(end(2))], high=2),
                                  authority(11, (1,), (2,), high=2))['code'], 'ACCEPT_DELTA')
        self.assertEqual(rx.state()['closed'][2]['linkSeq'], 6)
        return rx

    def test_LC19_tombstone_cap_never_evicts_existing_floor(self):
        rx = self.capped()
        before = rx.state()
        out = rx.apply(120, frame(12, [end_item(end(3))], high=3), authority(12, (1,), (3,), high=3))
        self.assertEqual((out['code'], out['needsFull']), ('RESOURCE_LIMIT', True))
        self.assertEqual(data(rx.state()), data(before))
        self.assertEqual(rx.state()['closed'][2]['linkSeq'], 6)
        self.assertNotIn(3, rx.state()['closed'])

    def test_LC20_checkpoint_pruning_still_prevents_reanimation(self):
        rx = self.capped()
        rx.apply(120, frame(12, [end_item(end(3))], high=3), authority(12, (1,), (3,), high=3))
        out = rx.apply(200, frame(13, [active_item(snapshot())], kind='FULL', high=3), authority(13, (1,), kind='FULL', high=3))
        self.assertEqual((out['code'], out['needsFull']), ('ACCEPT_FULL', False))
        self.assertEqual((rx.state()['checkpointHighWater'], rx.state()['closed']), (3, {}))
        before = rx.state()
        out = rx.apply(210, frame(14, [active_item(snapshot(2, seq=7))], high=3), authority(14, (1, 2), high=3))
        self.assertEqual(out['code'], 'CLOSED_ID')
        self.assertEqual(data(rx.state()), data(before))

    def test_LC21_checkpoint_cannot_shrink_or_miss_active_frontier(self):
        rx = self.base(high=3)
        before = rx.state()
        out = rx.apply(110, frame(11, [active_item(snapshot())], kind='FULL', high=2), authority(11, (1,), kind='FULL', high=2))
        self.assertEqual(out['code'], 'HIGH_WATER_ROLLBACK')
        self.assertEqual(data(rx.state()), data(before))
        rx = self.base()
        before = rx.state()
        out = rx.apply(110, frame(11, [active_item(snapshot(2))], kind='FULL', high=1), authority(11, (2,), kind='FULL', high=1))
        self.assertNotEqual(out['code'], 'ACCEPT_FULL')
        self.assertEqual(data(rx.state()), data(before))

    def test_LC22_fresh_full_cannot_reopen_closed_id(self):
        rx = self.closed()
        before = rx.state()
        out = rx.apply(120, frame(12, [active_item(snapshot(seq=100))], kind='FULL'), authority(12, (1,), kind='FULL'))
        self.assertEqual(out['code'], 'CLOSED_ID')
        self.assertEqual(data(rx.state()), data(before))

    def test_LC23_missing_or_stale_trusted_ended_receipt(self):
        for receipt_delivery, ended_ids in ((11, ()), (10, (1,))):
            with self.subTest(receipt_delivery=receipt_delivery, ended_ids=ended_ids):
                rx = self.base()
                before = rx.state()
                out = rx.apply(110, frame(11, [end_item(end())]), authority(receipt_delivery, (), ended_ids))
                self.assertEqual(out['code'], 'INVALID_AUTHORITY')
                self.assertEqual(rx.state(), before)

    def test_LC24_wrong_receipt_target_does_not_authorize_end(self):
        rx = self.base()
        proof = authority(11, (), (1,))
        proof['endedTargets'][1]['binding']['txGeneration'] = 2
        before = rx.state()
        out = rx.apply(110, frame(11, [end_item(end())]), proof)
        self.assertEqual(out['code'], 'INVALID_AUTHORITY')
        self.assertEqual(rx.state(), before)

    def test_LC25_caller_and_result_copies_are_detached(self):
        rx = lifecycle.OrderedReceiver(2, dict(LIMITS))
        packet = frame(10, [active_item(snapshot())], kind='FULL')
        proof = authority(10, (1,), kind='FULL')
        rx.apply(100, packet, proof)
        packet['items'][0]['snapshot']['descriptor']['components'][0]['phaseQ'] = 123
        proof['activeExpected'][1]['binding']['txGeneration'] = 99
        exposed = rx.read(1)
        exposed['binding']['leaseId'] = 999
        st = rx.state()
        st['active'][1]['snapshot']['descriptor']['components'].clear()
        st['active'][1]['expected']['channel']['frequencyMHz'] = 151.0
        self.assertEqual(rx.read(1)['descriptor']['components'][0]['phaseQ'], 0)
        self.assertEqual(rx.read(1)['binding']['leaseId'], 11)
        self.assertEqual(rx.state()['active'][1]['expected']['channel']['frequencyMHz'], 150.0)
        closed = self.closed()
        state = closed.state()
        state['closed'][1]['binding']['leaseId'] = 999
        self.assertEqual(closed.state()['closed'][1]['binding']['leaseId'], 11)

    def test_LC26_active_and_event_limits_reject_without_partial_apply(self):
        limits = dict(LIMITS, maxSnapshots=1)
        rx = self.base(limits=limits)
        before = rx.state()
        out = rx.apply(110, frame(11, [active_item(snapshot(2))], high=2), authority(11, (1, 2), high=2))
        self.assertEqual(out['code'], 'RESOURCE_LIMIT')
        self.assertEqual(data(rx.state()), data(before))
        rx = self.base(limits=limits)
        before = rx.state()
        out = rx.apply(110, frame(11, [active_item(snapshot(seq=6)), end_item(end(2))], high=2),
                       authority(11, (1,), (2,), high=2))
        self.assertNotEqual(out['code'], 'ACCEPT_DELTA')
        self.assertEqual(data(rx.state()), data(before))

    def test_LC27_exact_packet_types_and_fields_do_not_coerce(self):
        variants = [('deliverySeq', True), ('deliverySeq', 11.0), ('deliverySeq', 0),
                    ('deliverySeq', MAX), ('linkIdHighWater', -1), ('linkIdHighWater', False),
                    ('linkIdHighWater', MAX + 1), ('protocolVersion', 1.0), ('kind', 'BOGUS')]
        for key, value in variants:
            with self.subTest(key=key, value=value):
                rx = self.base()
                packet = frame(11, [active_item(snapshot(seq=6))])
                packet[key] = value
                before = rx.state()
                out = rx.apply(110, packet, authority(11, (1,)))
                self.assertNotIn(out['code'], ('ACCEPT_DELTA', 'ACCEPT_FULL'))
                self.assertEqual(data(rx.state()), data(before))
        for extra in (False, True):
            rx = self.base()
            packet = frame(11, [active_item(snapshot(seq=6))])
            if extra:
                packet['extra'] = 1
            else:
                packet.pop('items')
            before = rx.state()
            out = rx.apply(110, packet, authority(11, (1,)))
            self.assertNotEqual(out['code'], 'ACCEPT_DELTA')
            self.assertEqual(data(rx.state()), data(before))

    def test_LC28_foreign_scope_never_clears_current_identity(self):
        rx = self.base()
        before = rx.state()
        packet = frame(11, [], kind='FULL', owner=3)
        self.assertEqual(rx.apply(110, packet, authority(11, (), kind='FULL'))['code'], 'INVALID_AUTHORITY')
        self.assertEqual(rx.state(), before)
        # Epoch identity comes from independent current authority, not numeric ordering.
        out = rx.apply(120, frame(1, [active_item(snapshot(epoch=3))], kind='FULL', epoch=3),
                       authority(1, (1,), kind='FULL', epoch=3))
        self.assertEqual(out['code'], 'ACCEPT_FULL')
        self.assertEqual((rx.state()['sessionEpoch'], rx.state()['deliverySeq']), (3, 1))
        before = rx.state()
        out = rx.apply(130, frame(12, [], kind='FULL', epoch=7), authority(12, (), kind='FULL', epoch=3))
        self.assertEqual(out['code'], 'INVALID_AUTHORITY')
        self.assertEqual(rx.state(), before)

    def test_LC29_old_generation_active_rejects_but_separate_ended_receipt_closes(self):
        rx = self.base()
        proof = authority(11, (1,))
        proof['activeExpected'][1] = expected(generation=2)
        before = rx.state()
        out = rx.apply(110, frame(11, [active_item(snapshot(seq=6))]), proof)
        self.assertEqual(out['code'], 'CONTEXT_MISMATCH')
        self.assertEqual(data(rx.state()), data(before))
        rx = self.base()
        # No active Expected for the ended lease; independent old-binding receipt suffices.
        out = rx.apply(110, frame(11, [end_item(end())]), authority(11, (), (1,)))
        self.assertEqual(out['code'], 'ACCEPT_DELTA')
        self.assertIsNone(rx.read(1))

    def test_LC30_duplicate_and_mixed_same_id_items_reject_atomically(self):
        for items in ([active_item(snapshot(seq=6)), active_item(snapshot(seq=7))],
                      [active_item(snapshot(seq=6)), end_item(end())]):
            rx = self.base()
            before = rx.state()
            out = rx.apply(110, frame(11, items), authority(11, (1,), (1,) if items[1]['kind'] == 'END' else ()))
            self.assertNotEqual(out['code'], 'ACCEPT_DELTA')
            self.assertEqual(data(rx.state()), data(before))

    def test_LC31_full_authority_is_complete_not_filtered_from_packet(self):
        rx = self.base()
        before = rx.state()
        out = rx.apply(110, frame(11, [active_item(snapshot())], kind='FULL', high=2),
                       authority(11, (1, 2), kind='FULL', high=2))
        self.assertNotEqual(out['code'], 'ACCEPT_FULL')
        self.assertEqual(data(rx.state()), data(before))

    def test_LC32_delta_missing_end_cannot_retain_revoked_other_link(self):
        rx = self.base(high=2, ids=(1, 2))
        before = rx.state()
        out = rx.apply(110, frame(11, [active_item(snapshot(2, seq=6))], high=2), authority(11, (2,), high=2))
        self.assertEqual(out['code'], 'COMPLETENESS_MISMATCH')
        self.assertTrue(out['needsFull'])
        self.assertEqual(data(rx.state()), data(before))
        self.assertFalse(rx.visible(110, 1))

    def test_LC33_delta_revalidates_retained_generation_not_only_items(self):
        rx = self.base(high=2, ids=(1, 2))
        proof = authority(11, (1, 2), high=2)
        proof['activeExpected'][1] = expected(generation=2)
        before = rx.state()
        out = rx.apply(110, frame(11, [active_item(snapshot(2, seq=6))], high=2), proof)
        self.assertEqual(out['code'], 'CONTEXT_MISMATCH')
        self.assertEqual(data(rx.state()), data(before))
        self.assertTrue(rx.state()['needsFull'])

    def test_invalid_end_reasons_and_unhashable_ids_never_apply(self):
        for reason in ('NONE', 'LOST', 'UNKNOWN'):
            rx = self.base()
            bad = end(reason=reason)
            proof = authority(11, (), (1,))
            proof['endedTargets'][1] = end(reason=reason)
            before = rx.state()
            out = rx.apply(110, frame(11, [end_item(bad)]), proof)
            self.assertNotEqual(out['code'], 'ACCEPT_DELTA')
            self.assertEqual(data(rx.state()), data(before))
        rx = self.base()
        bad = snapshot(seq=6)
        bad['linkId'] = []
        before = rx.state()
        out = rx.apply(110, frame(11, [active_item(bad)]), authority(11, (1,)))
        self.assertNotEqual(out['code'], 'ACCEPT_DELTA')
        self.assertEqual(data(rx.state()), data(before))

    def test_authority_map_bool_keys_do_not_alias_link_one(self):
        for kind in ('ACTIVE', 'END'):
            with self.subTest(kind=kind):
                rx = self.base()
                if kind == 'ACTIVE':
                    packet = frame(11, [active_item(snapshot(seq=6))])
                    proof = authority(11, (1,))
                    proof['activeExpected'] = {True: expected()}
                else:
                    packet = frame(11, [end_item(end())])
                    proof = authority(11, (), (1,))
                    proof['endedTargets'] = {True: end()}
                before = rx.state()
                out = rx.apply(110, packet, proof)
                self.assertNotEqual(out['code'], 'ACCEPT_DELTA')
                self.assertEqual(rx.state(), before)

    def test_LC34_rejected_trusted_end_frontier_cannot_be_forgotten(self):
        rx = self.capped()
        out = rx.apply(120, frame(12, [end_item(end(3))], high=3), authority(12, (1,), (3,), high=3))
        self.assertEqual(out['code'], 'RESOURCE_LIMIT')
        self.assertEqual((rx.state()['observedDeliverySeq'], rx.state()['observedHighWater']), (12, 3))
        self.assertEqual(rx.state()['seenHighWater'], 3)
        before = rx.state()
        out = rx.apply(200, frame(13, [active_item(snapshot())], kind='FULL', high=2),
                       authority(13, (1,), kind='FULL', high=2))
        self.assertEqual(out['code'], 'HIGH_WATER_ROLLBACK')
        self.assertEqual(data(rx.state()), data(before))
        self.assertEqual(rx.state()['closed'][2]['linkSeq'], 6)
        self.assertEqual(rx.apply(210, frame(14, [active_item(snapshot())], kind='FULL', high=3),
                                  authority(14, (1,), kind='FULL', high=3))['code'], 'ACCEPT_FULL')
        self.assertEqual(rx.state()['checkpointHighWater'], 3)
        self.assertEqual(rx.apply(220, frame(15, [active_item(snapshot(3, seq=7))], high=3),
                                  authority(15, (1, 3), high=3))['code'], 'CLOSED_ID')

    def test_LC35_gap_observation_rejects_intermediate_older_full(self):
        rx = self.base(delivery=3)
        self.assertEqual(rx.apply(110, frame(5, [end_item(end(2))], high=2),
                                  authority(5, (1,), (2,), high=2))['code'], 'GAP')
        self.assertEqual((rx.state()['deliverySeq'], rx.state()['observedDeliverySeq']), (3, 5))
        before = rx.state()
        out = rx.apply(120, frame(4, [active_item(snapshot())], kind='FULL', high=2),
                       authority(4, (1,), kind='FULL', high=2))
        self.assertEqual(out['code'], 'STALE')
        self.assertEqual(rx.state(), before)
        self.assertFalse(rx.visible(120, 1))
        self.assertEqual(rx.apply(130, frame(6, [active_item(snapshot())], kind='FULL', high=2),
                                  authority(6, (1,), kind='FULL', high=2))['code'], 'ACCEPT_FULL')

    def test_LC35_initial_and_new_epoch_failed_full_keep_pending_frontier(self):
        for prior_epoch in (None, 7):
            with self.subTest(prior_epoch=prior_epoch):
                rx = lifecycle.OrderedReceiver(2, dict(LIMITS)) if prior_epoch is None else self.base()
                before = rx.state()
                epoch = 7 if prior_epoch is None else 3
                bad = snapshot(2, epoch=epoch)
                bad['descriptor']['components'][0]['phaseQ'] = 1073741824
                out = rx.apply(110, frame(10, [active_item(snapshot(epoch=epoch)), active_item(bad)],
                                          kind='FULL', high=2, epoch=epoch),
                               authority(10, (1, 2), kind='FULL', high=2, epoch=epoch))
                self.assertEqual(out['code'], 'INVALID_DESCRIPTOR')
                self.assertEqual(data(rx.state()), data(before))
                self.assertEqual(rx.state()['seenHighWater'], before['seenHighWater'])
                self.assertEqual((rx.state()['observedEpoch'], rx.state()['observedDeliverySeq'],
                                  rx.state()['observedHighWater']), (epoch, 10, 2))
                pending = rx.state()
                out = rx.apply(120, frame(9, [active_item(snapshot(epoch=epoch))], kind='FULL', high=1, epoch=epoch),
                               authority(9, (1,), kind='FULL', high=1, epoch=epoch))
                self.assertEqual(out['code'], 'STALE')
                self.assertEqual(rx.state(), pending)
                out = rx.apply(130, frame(11, [active_item(snapshot(epoch=epoch))], kind='FULL', high=2, epoch=epoch),
                               authority(11, (1,), kind='FULL', high=2, epoch=epoch))
                self.assertEqual(out['code'], 'ACCEPT_FULL')
                self.assertEqual((rx.state()['sessionEpoch'], rx.state()['checkpointHighWater']), (epoch, 2))

    def test_LC36_deep_malformed_graph_is_rejected_before_copy(self):
        deep = {'leaf': 'unchanged'}
        for _ in range(1500):
            deep = {'nested': deep}
        for target in ('quality', 'descriptor', 'authority'):
            with self.subTest(target=target):
                rx = self.base()
                packet = frame(11, [active_item(snapshot(seq=6))])
                proof = authority(11, (1,))
                if target == 'authority':
                    proof['activeExpected'][1]['channel']['frequencyMHz'] = deep
                else:
                    packet['items'][0]['snapshot'][target] = deep
                before = rx.state()
                out = rx.apply(110, packet, proof)
                self.assertNotEqual(out['code'], 'ACCEPT_DELTA')
                self.assertEqual(data(rx.state()), data(before))
                retained_input = (proof['activeExpected'][1]['channel']['frequencyMHz'] if target == 'authority'
                                  else packet['items'][0]['snapshot'][target])
                self.assertIs(retained_input, deep)
                cursor = deep
                for _ in range(1500):
                    self.assertEqual(set(cursor), {'nested'})
                    cursor = cursor['nested']
                self.assertEqual(cursor, {'leaf': 'unchanged'})

    def test_LC37_oversize_batch_precedes_deep_graph_scan_or_copy(self):
        rx = self.base(limits=dict(LIMITS, maxSnapshots=1))
        deep = {'leaf': 1}
        for _ in range(1500):
            deep = {'nested': deep}
        packet = frame(11, [active_item(snapshot(seq=6)), active_item(snapshot(2))], high=2)
        packet['items'][0]['snapshot']['quality'] = deep
        before = rx.state()
        out = rx.apply(110, packet, authority(11, (1, 2), high=2))
        self.assertEqual(out['code'], 'BATCH_LIMIT')
        self.assertEqual(data(rx.state()), data(before))
        self.assertIs(packet['items'][0]['snapshot']['quality'], deep)

    def test_LC38_invalid_deep_configuration_rejects_before_copy(self):
        deep = []
        for _ in range(1500):
            deep = [deep]
        bad_limits = dict(LIMITS, maxSnapshots=deep)
        rx = lifecycle.OrderedReceiver(2, bad_limits)
        before = rx.state()
        out = rx.apply(100, frame(10, [active_item(snapshot())], kind='FULL'),
                       authority(10, (1,), kind='FULL'))
        self.assertEqual(out['code'], 'INVALID_LIMITS')
        self.assertEqual(rx.state(), before)
        self.assertIs(bad_limits['maxSnapshots'], deep)

    def test_LC39_internal_authority_pins_reject_inertly_before_observation(self):
        for target, field in (('ACTIVE', 'linkId'), ('ACTIVE', 'sessionEpoch'),
                              ('ACTIVE', 'listenerPlayerId'), ('ACTIVE', 'tx'), ('ACTIVE', 'rx'),
                              ('END', 'linkId'), ('END', 'sessionEpoch'), ('END', 'listenerPlayerId')):
            with self.subTest(target=target, field=field):
                rx = self.base()
                if target == 'ACTIVE':
                    packet = frame(11, [active_item(snapshot(seq=6))])
                    proof = authority(11, (1,))
                    record = proof['activeExpected'][1]
                    if field in ('tx', 'rx'):
                        record['channel'][field] = dict(deviceId=999, transceiverIndex=0)
                    else:
                        record[field] = {'linkId': 2, 'sessionEpoch': 8, 'listenerPlayerId': 3}[field]
                else:
                    foreign = end()
                    foreign[field] = {'linkId': 2, 'sessionEpoch': 8, 'listenerPlayerId': 3}[field]
                    packet = frame(11, [end_item(foreign)], high=2)
                    proof = authority(11, (1,), (1,), high=2)
                    proof['endedTargets'][1] = copy.deepcopy(foreign)
                before = rx.state()
                out = rx.apply(110, packet, proof)
                self.assertEqual(out['code'], 'INVALID_AUTHORITY')
                self.assertEqual(rx.state(), before)

    def test_real_negative_controls_detect_thirteen_unsafe_paths(self):
        apply = lifecycle.OrderedReceiver.apply
        active_row = lifecycle.OrderedReceiver._active_row
        postflight = lifecycle.OrderedReceiver._postflight

        def end_as_lost(rx, now, envelope, proof):
            if envelope['kind'] == 'DELTA' and envelope['items'] and envelope['items'][0]['kind'] == 'END':
                wrong = snapshot(seq=6)
                wrong.update(state='LOST', quality=0.0)
                return apply(rx, now, frame(11, [active_item(wrong)]), authority(11, (1,)))
            return apply(rx, now, envelope, proof)

        def forget_closed(rx, now, envelope, proof):
            if rx._closed and envelope['kind'] == 'DELTA' and envelope['items'][0]['kind'] == 'ACTIVE':
                rx._closed = {}
                rx._checkpoint = 0
            return apply(rx, now, envelope, proof)

        def commit_prefix_before_bad_full(rx, now, envelope, proof):
            if envelope['kind'] == 'FULL' and len(envelope['items']) == 2:
                rx._active[1] = dict(snapshot=copy.deepcopy(envelope['items'][0]['snapshot']),
                                     receivedAtMs=now, expected=expected())
            return apply(rx, now, envelope, proof)

        def stale_full_destroys_cache(rx, now, envelope, proof):
            if (envelope['kind'] == 'FULL' and rx._epoch == envelope['sessionEpoch']
                    and envelope['deliverySeq'] <= rx._delivery_seq):
                rx._active = {}
            return apply(rx, now, envelope, proof)

        def equal_body_refreshes_time(rx, s, old, now, trusted):
            code, row = active_row(rx, s, old, now, trusted)
            if code == 'OK' and old and s['linkSeq'] == old['snapshot']['linkSeq']:
                row['receivedAtMs'] = now
            return code, row

        def accepts_rollback(rx, s, old, now, trusted):
            if old and s['linkSeq'] < old['snapshot']['linkSeq']:
                return 'OK', dict(snapshot=copy.deepcopy(s), receivedAtMs=now, expected=copy.deepcopy(trusted))
            return active_row(rx, s, old, now, trusted)

        def ignores_gap(rx, now, envelope, proof):
            if (envelope['kind'] == 'DELTA' and rx._epoch == envelope['sessionEpoch']
                    and envelope['deliverySeq'] > rx._delivery_seq + 1):
                rx._delivery_seq = envelope['deliverySeq'] - 1
            return apply(rx, now, envelope, proof)

        def evicts_floor_on_cap(rx, active, closed, trusted):
            while len(active) + len(closed) > rx._limits['maxSnapshots'] and closed:
                closed.pop(next(iter(closed)))
            return postflight(rx, active, closed, trusted)

        def forgets_checkpoint_after_pruning(rx, now, envelope, proof):
            if (envelope['kind'] == 'DELTA' and not rx._closed and rx._checkpoint == 3
                    and envelope['items'] and envelope['items'][0]['kind'] == 'ACTIVE'):
                rx._checkpoint = 0
            return apply(rx, now, envelope, proof)

        def trusts_end_packet_as_receipt(rx, now, envelope, proof):
            forged = copy.deepcopy(proof)
            for item in envelope['items']:
                if item['kind'] == 'END':
                    forged['endedTargets'][item['end']['linkId']] = copy.deepcopy(item['end'])
            return apply(rx, now, envelope, forged)

        def shallow_read(rx, link):
            row = rx._active.get(link)
            return row['snapshot'] if row else None

        controls = [
            ('NC01', lifecycle.OrderedReceiver, 'apply', end_as_lost, 'test_LC02_end_removes_slot_and_retains_floor'),
            ('NC02', lifecycle.OrderedReceiver, 'apply', forget_closed, 'test_LC03_LC04_closed_id_rejects_older_and_even_newer_active'),
            ('NC03', lifecycle, '_same_binding', lambda left, right: True, 'test_LC06_old_binding_end_cannot_close_current_lease'),
            ('NC04', lifecycle.OrderedReceiver, 'apply', commit_prefix_before_bad_full, 'test_LC08_late_bad_full_does_not_destroy_prior_identity'),
            ('NC05', lifecycle.OrderedReceiver, 'apply', stale_full_destroys_cache, 'test_LC09_LC10_stale_full_does_not_clear_or_refresh'),
            ('NC06', lifecycle.OrderedReceiver, '_active_row', equal_body_refreshes_time, 'test_LC13_fresh_checkpoint_equal_body_preserves_receipt'),
            ('NC07', lifecycle.OrderedReceiver, '_active_row', accepts_rollback, 'test_LC11_fresh_full_cannot_roll_back_per_link_sequence'),
            ('NC08', lifecycle.OrderedReceiver, 'apply', ignores_gap, 'test_LC14_LC15_gap_requires_full_and_preserves_cached_data'),
            ('NC09', lifecycle.OrderedReceiver, '_postflight', evicts_floor_on_cap, 'test_LC19_tombstone_cap_never_evicts_existing_floor'),
            ('NC10', lifecycle.OrderedReceiver, 'apply', forgets_checkpoint_after_pruning, 'test_LC20_checkpoint_pruning_still_prevents_reanimation'),
            ('NC11', lifecycle.OrderedReceiver, 'apply', trusts_end_packet_as_receipt, 'test_LC23_missing_or_stale_trusted_ended_receipt'),
            ('NC12', lifecycle.OrderedReceiver, 'read', shallow_read, 'test_LC25_caller_and_result_copies_are_detached'),
            ('NC13', lifecycle.OrderedReceiver, '_postflight', lambda rx, a, c, e: 'OK', 'test_LC32_delta_missing_end_cannot_retain_revoked_other_link'),
        ]
        for name, target, attribute, broken, method in controls:
            with self.subTest(control=name):
                result = unittest.TestResult()
                with patch.object(target, attribute, broken):
                    IndependentLifecycleTests(method).run(result)
                self.assertEqual(result.errors, [], name + ': an incidental error is not proof of semantic detection')
                self.assertTrue(result.failures, name + ': intentionally unsafe path escaped independent assertions')


if __name__ == '__main__':
    unittest.main()
