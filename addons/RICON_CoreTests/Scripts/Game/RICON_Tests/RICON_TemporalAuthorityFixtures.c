//! TEST_ONLY integration candidates. Source fixtures NOT_RUN until explicitly allowed ScriptEditor execution.
class RICON_TemporalAuthorityFixtures : RICON_TemporalSnapshotFixtures
{
    static int s_iChecks, s_iFailures, s_iFirstFailure;
    protected static void Require(bool ok)
    {
        s_iChecks++;
        if (!ok) { s_iFailures++; if (s_iFirstFailure == 0) s_iFirstFailure = s_iChecks; }
    }

    protected static RICON_TemporalAuthorityJournal Journal()
    {
        RICON_TemporalLimits l = new RICON_TemporalLimits();
        l.m_iMaxSnapshots = 4; l.m_iMaxTotalDescriptorBytes = 408;
        return new RICON_TemporalAuthorityJournal(7, l);
    }

    protected static void Capture(RICON_TemporalAuthorityJournal j, int link, int listener, int rx, int lease = 11, int entityId = 0)
    {
        RICON_TemporalSnapshot s = Snapshot(link, rx, listener);
        RICON_TemporalExpected e = Expected(link, rx, listener);
        s.m_Binding.m_iLeaseId = lease; e.m_Binding.m_iLeaseId = lease;
        s.m_Binding.m_iRxGeneration = 8; e.m_Binding.m_iRxGeneration = 8;
        if (entityId == 0) entityId = 1000 + listener; // TEST_ONLY literal convention, not a production fallback.
        Require(j.CapturePublished(s, e, entityId).m_sCode == "OK");
    }

    protected static RICON_LifecycleResult Terminal(int epoch = 7, RICON_ELifecycleResult code = RICON_ELifecycleResult.OK)
    {
        RICON_LifecycleResult r = new RICON_LifecycleResult(code);
        RICON_LifecycleEffect t = new RICON_LifecycleEffect();
        t.m_eType = RICON_ELifecycleEffectType.LEASE_TERMINATED;
        t.m_iPlayerId = 1; t.m_iGeneration = 1; t.m_iDeviceId = 101; t.m_iLeaseId = 11;
        t.m_eLeaseState = RICON_ETransmitLeaseState.REVOKED; t.m_eTerminalReason = RICON_ELeaseTerminalReason.CLIENT_STOP;
        r.m_aEffects.Insert(t);
        RICON_LifecycleEffect v = new RICON_LifecycleEffect();
        v.m_eType = RICON_ELifecycleEffectType.VON_EXPIRY_INTENDED;
        v.m_iPlayerId = 1; v.m_iDeviceId = 101; v.m_iLeaseId = 11; v.m_iSessionEpoch = epoch;
        r.m_aEffects.Insert(v); return r;
    }

    protected static RICON_LifecycleEffect ClearRx(int player = 2, int generation = 8)
    {
        RICON_LifecycleEffect c = new RICON_LifecycleEffect();
        c.m_eType = RICON_ELifecycleEffectType.SUBSCRIPTIONS_CLEARED; c.m_iPlayerId = player; c.m_iGeneration = generation;
        return c;
    }

    protected static void JournalCases()
    {
        RICON_TemporalAuthorityJournal j = Journal();
        Capture(j, 1, 2, 202); Capture(j, 2, 3, 203);
        RICON_LifecycleResult r = Terminal(7, RICON_ELifecycleResult.MALFORMED);
        r.m_aEffects.Insert(ClearRx());
        Require(r.m_aEffects[0].m_iSessionEpoch == 0);
        RICON_TemporalCaptureResult closureResult = j.ConsumeLifecycleResult(r, 7);
        Require(closureResult.m_sCode == "OK" && closureResult.m_aClosures.Count() == 2 && j.ActiveCount() == 0 && j.PendingCount() == 2);
        Require(closureResult.m_aClosures[0].m_iSessionEpoch == 7 && closureResult.m_aClosures[0].m_sReason == "CLIENT_STOP");
        Require(closureResult.m_aClosures[0].ToEnd(1) == null && closureResult.m_aClosures[0].ToEnd(int.MAX) == null);
        Require(closureResult.m_aClosures[0].ToEnd(2).m_iLinkSeq == 2);
        closureResult.m_aClosures[0].m_Binding.m_iRxGeneration = 99;
        Require(j.ReadPending(1).m_Binding.m_iRxGeneration == 8);
        Require(j.ConsumeLifecycleResult(r, 7).m_aClosures.Count() == 0);
        Require(j.CapturePublished(Snapshot(), Expected(), 1002).m_sCode == "CLOSED_ID");

        j = Journal(); Capture(j, 1, 2, 202); Capture(j, 2, 3, 203);
        r = Terminal(); r.m_aEffects.Insert(null);
        Require(j.ConsumeLifecycleResult(r, 7).m_sCode == "INVALID_EFFECTS" && j.ActiveCount() == 2 && j.PendingCount() == 0);
        r = Terminal(8);
        Require(j.ConsumeLifecycleResult(r, 7).m_sCode == "INVALID_EFFECTS" && j.ActiveCount() == 2);
        r = Terminal();
        Require(j.ConsumeLifecycleResult(r, 8).m_sCode == "SCOPE_MISMATCH" && j.ActiveCount() == 2);
        r.m_aEffects.RemoveOrdered(0);
        Require(j.ConsumeLifecycleResult(r, 7).m_sCode == "INVALID_EFFECTS" && j.ActiveCount() == 2);
        r = Terminal(); r.m_aEffects.RemoveOrdered(1);
        Require(j.ConsumeLifecycleResult(r, 7).m_sCode == "INVALID_EFFECTS" && j.ActiveCount() == 2 && j.PendingCount() == 0);

        j = Journal(); Capture(j, 1, 2, 202); Capture(j, 2, 3, 203, 12); Capture(j, 3, 2, 204, 13);
        r = new RICON_LifecycleResult(RICON_ELifecycleResult.DUPLICATE);
        r.m_aEffects.Insert(ClearRx());
        Require(j.ConsumeLifecycleResult(r, 7).m_aClosures.Count() == 2);
        Require(j.ReadActive(2) != null && j.ReadPending(1).m_sReason == "LINK_REMOVED" && j.ReadPending(3) != null);

        j = Journal();
        RICON_TemporalSnapshot s = Snapshot(); RICON_TemporalExpected e = Expected();
        Require(j.CapturePublished(s, e, 1002).m_sCode == "OK");
        s.m_Binding.m_iLeaseId = 999; s.m_Descriptor.m_aComponents[0].m_iPhaseQ = 99;
        e.m_Channel.m_fFrequencyMHz = 155;
        Require(j.ReadActive(1).m_Binding.m_iLeaseId == 11 && j.ReadActive(1).m_Descriptor.m_aComponents[0].m_iPhaseQ == 0);
        Require(j.CapturePublished(Snapshot(), Expected(), 1002).m_sCode == "DUPLICATE");
        s = Snapshot(); s.m_Descriptor.m_aComponents[0].m_iPhaseQ = 1;
        Require(j.CapturePublished(s, Expected(), 1002).m_sCode == "LINK_SEQUENCE_CONFLICT");
        RICON_TemporalClosureIntent empty = new RICON_TemporalClosureIntent();
        Require(empty.ToEnd(1) == null);
    }

    protected static void CheckpointCases()
    {
        RICON_TemporalAuthorityJournal j = Journal(); Capture(j, 1, 2, 202); Capture(j, 2, 3, 203, 12);
        Require(j.ConsumeLifecycleResult(Terminal(), 7).m_aClosures.Count() == 1);
        array<int> incomplete = {};
        Require(j.CommitSourceCheckpoint(7, 2, incomplete).m_sCode == "INVALID_CHECKPOINT" && j.PendingCount() == 1);
        array<int> allActive = {2};
        Require(j.CommitSourceCheckpoint(7, 2, allActive).m_sCode == "OK" && j.PendingCount() == 0 && j.ActiveCount() == 1);
        RICON_TemporalSnapshot s = Snapshot(1, 204, 4); RICON_TemporalExpected e = Expected(1, 204, 4);
        Require(j.CapturePublished(s, e, 1004).m_sCode == "CLOSED_ID");
        s = Snapshot(3, 204, 4); e = Expected(3, 204, 4);
        Require(j.CapturePublished(s, e, 1004).m_sCode == "OK");
        array<int> missingOtherOwner = {3};
        Require(j.CommitSourceCheckpoint(7, 3, missingOtherOwner).m_sCode == "INVALID_CHECKPOINT");
        array<int> complete = {2, 3};
        Require(j.CommitSourceCheckpoint(7, 3, complete).m_sCode == "OK");
        Require(j.CommitSourceCheckpoint(7, 2, complete).m_sCode == "HIGH_WATER_ROLLBACK");
    }

    protected static void ActualLifecyclePrelude()
    {
        // Execute the existing pure Lifecycle, not invented effects, once execution is allowed.
        RICON_LifecycleStateMachine machine = new RICON_LifecycleStateMachine();
        RICON_LifecycleDevice device = RICON_LifecycleTraceRun.Device(101, RICON_EDeviceKind.CARRIED, 1, true);
        device.m_aAllowed.Insert(new RICON_LifecycleCatalogPair(1, 1));
        array<ref RICON_LifecycleDevice> devices = {device};
        RICON_LifecycleDevice receiverDevice = RICON_LifecycleTraceRun.Device(202, RICON_EDeviceKind.CARRIED, 2, true);
        receiverDevice.m_aAllowed.Insert(new RICON_LifecycleCatalogPair(1, 1)); devices.Insert(receiverDevice);
        RICON_LifecycleConfig config = RICON_LifecycleTraceRun.Config(4, 4, 8, 60000, 4, 30000, 4, 8, 1000, 5);
        Require(machine.Initialize(7, 0, config, devices).m_eResultCode == RICON_ELifecycleResult.OK);
        machine.Connected(1, 1, 501); machine.Registered(2, 1, 1); machine.EntityBound(3, 1, 1, 1001);
        machine.Connected(3, 2, 502); machine.Registered(3, 2, 2); machine.EntityBound(3, 2, 2, 1002);
        Require(machine.GetPlayerGeneration(1) == 1 && machine.GetPlayerGeneration(2) == 2);
        RICON_LifecyclePttRequest start = RICON_LifecycleTraceRun.Start(100, 1, 1, 1, 1, 7, 1, 1, 101, 1, 1, 10);
        RICON_LifecycleResult granted = machine.ProcessPtt(start);
        Require(granted.m_eResultCode == RICON_ELifecycleResult.OK && granted.m_iLeaseId == 1);
        RICON_TemporalAuthorityJournal journal = Journal();
        RICON_TemporalSnapshot first = Snapshot(); RICON_TemporalExpected firstExpected = Expected();
        first.m_Binding.m_iLeaseId = 1; firstExpected.m_Binding.m_iLeaseId = 1;
        first.m_Binding.m_iRxGeneration = 2; firstExpected.m_Binding.m_iRxGeneration = 2;
        Require(journal.CapturePublished(first, firstExpected, 1002).m_sCode == "OK");
        int beforeEpoch = machine.GetSessionEpoch();
        // Schema=0 is rejected AFTER BeginEvent sweeps the expired lease.
        RICON_LifecyclePttRequest malformed = RICON_LifecycleTraceRun.Stop(120, 0, 2, 1, 1, 7, 1, 2, 1);
        RICON_LifecycleResult expired = machine.ProcessPtt(malformed);
        Require(expired.m_eResultCode == RICON_ELifecycleResult.MALFORMED);
        RICON_TemporalCaptureResult capture = journal.ConsumeLifecycleResult(expired, beforeEpoch);
        Require(capture.m_sCode == "OK" && capture.m_aClosures.Count() == 1);
        Require(capture.m_aClosures[0].m_sReason == "TTL_EXPIRED" && capture.m_aClosures[0].m_iSessionEpoch == 7);

        // The same real result can contain old-epoch terminations and new-epoch FULL effects.
        start = RICON_LifecycleTraceRun.Start(121, 1, 3, 1, 1, 7, 1, 2, 101, 1, 1, 1000);
        granted = machine.ProcessPtt(start);
        Require(granted.m_eResultCode == RICON_ELifecycleResult.OK && granted.m_iLeaseId == 2);
        journal = Journal(); first = Snapshot(2, 202, 2); firstExpected = Expected(2, 202, 2);
        first.m_Binding.m_iLeaseId = 2; firstExpected.m_Binding.m_iLeaseId = 2;
        first.m_Binding.m_iRxGeneration = 2; firstExpected.m_Binding.m_iRxGeneration = 2;
        Require(journal.CapturePublished(first, firstExpected, 1002).m_sCode == "OK");
        beforeEpoch = machine.GetSessionEpoch();
        RICON_LifecycleResult changed = machine.EntityChanged(122, 2, 2, 1002, 1003);
        Require(machine.GetPlayerGeneration(2) == 2 && machine.GetPlayerEntityId(2) == 1003);
        RICON_TemporalSnapshot rebound = Snapshot(3, 202, 2); RICON_TemporalExpected reboundExpected = Expected(3, 202, 2);
        rebound.m_Binding.m_iLeaseId = 2; reboundExpected.m_Binding.m_iLeaseId = 2;
        rebound.m_Binding.m_iRxGeneration = 2; reboundExpected.m_Binding.m_iRxGeneration = 2;
        Require(journal.CapturePublished(rebound, reboundExpected, machine.GetPlayerEntityId(2)).m_sCode == "OK");
        capture = journal.ConsumeLifecycleResult(changed, beforeEpoch);
        Require(capture.m_aClosures.Count() == 1 && capture.m_aClosures[0].m_iLinkId == 2 && journal.ReadActive(3) != null);
        Require(journal.ConsumeLifecycleResult(changed, beforeEpoch).m_aClosures.Count() == 0 && journal.ReadActive(3) != null);
        RICON_LifecycleResult rotated = machine.RotateEpoch(123, 8);
        capture = journal.ConsumeLifecycleResult(rotated, beforeEpoch);
        Require(machine.GetSessionEpoch() == 8 && capture.m_sCode == "OK" && capture.m_bRequiresFullResync);
        Require(capture.m_aClosures.Count() == 1 && capture.m_aClosures[0].m_iSessionEpoch == 7
            && capture.m_aClosures[0].m_sReason == "SESSION_ROTATED");
    }

    protected static void EntityAddressCases()
    {
        RICON_TemporalAuthorityJournal j = Journal();
        Capture(j, 1, 2, 202, 11, 1002);
        Capture(j, 3, 2, 202, 11, 1003);
        RICON_LifecycleResult r = new RICON_LifecycleResult(RICON_ELifecycleResult.OK);
        RICON_LifecycleEffect unbound = new RICON_LifecycleEffect();
        unbound.m_eType = RICON_ELifecycleEffectType.ENTITY_UNBOUND;
        unbound.m_iPlayerId = 2; unbound.m_iGeneration = 8; unbound.m_iEntityId = 1002;
        r.m_aEffects.Insert(unbound);
        Require(j.ConsumeLifecycleResult(r, 7).m_aClosures.Count() == 1 && j.ReadActive(3) != null);
        Require(j.ConsumeLifecycleResult(r, 7).m_aClosures.Count() == 0 && j.ReadActive(3) != null);
        j = Journal(); Capture(j, 1, 2, 202, 11, 1002);
        RICON_TemporalSnapshot s = Snapshot(); RICON_TemporalExpected e = Expected();
        s.m_Binding.m_iRxGeneration = 8; e.m_Binding.m_iRxGeneration = 8; s.m_iLinkSeq = 2;
        Require(j.CapturePublished(s, e, 1003).m_sCode == "ENTITY_REBIND_REQUIRES_NEW_LINK" && j.ActiveCount() == 1);
        Require(j.CapturePublished(s, e, 0).m_sCode == "INVALID_RECEIVER_ENTITY" && j.ActiveCount() == 1);
    }

    static bool RunCaptureChecks()
    {
        s_iChecks = 0; s_iFailures = 0; s_iFirstFailure = 0;
        JournalCases(); CheckpointCases(); ActualLifecyclePrelude(); EntityAddressCases();
        return s_iFailures == 0 && s_iChecks > 0;
    }
}
