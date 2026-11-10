//! TEST_ONLY pure fixtures: explicit RunAll entrypoint, real journal/TemporalSnapshots validation, no autorun.
class RICON_TemporalCaptureBatchProbeJournal : RICON_TemporalAuthorityJournal
{
    void RICON_TemporalCaptureBatchProbeJournal(int epoch, RICON_TemporalLimits limits) {}
    int Prefix() { return m_iGlobalPrefix; }
    RICON_TemporalExpected ReadExpected(int link)
    {
        RICON_TemporalCapturedLink row = m_Active.Get(link);
        if (!row)
        {
            return null;
        }
        return row.m_Expected.CloneValidated();
    }
}

class RICON_TemporalCaptureBatchFixtures
{
    static int m_iChecks, m_iFailures, m_iFirstFailure;
    protected static bool Check(bool ok)
    {
        m_iChecks++;
        if (!ok)
        {
            m_iFailures++;
            if (m_iFirstFailure == 0)
            {
                m_iFirstFailure = m_iChecks;
            }
        }
        return ok;
    }
    static RICON_TemporalCaptureInput Input(int link = 1, int rx = 202, int seq = 1, bool wide = false, int listener = 2)
    {
        RICON_TemporalSnapshot s = RICON_TemporalLifecycleFixtures.Snapshot(link, rx);
        RICON_TemporalExpected e = RICON_TemporalLifecycleFixtures.Expected(link, rx);
        s.m_iLinkSeq = seq; s.m_iListenerPlayerId = listener; e.m_iListenerPlayerId = listener;
        if (wide)
        {
            // Manual CD07: extra one-character component costs 13+1 bytes, 102 -> 116.
            RICON_FadingComponent c = new RICON_FadingComponent(); c.m_sComponentId = "x"; c.m_fPowerWeight = 0;
            s.m_Descriptor.m_aComponents.Insert(c);
        }
        return new RICON_TemporalCaptureInput(s, e, 1000 + listener);
    }
    static RICON_TemporalCaptureBatchProbeJournal Journal(int cap = 4, int bytes = 408)
    {
        RICON_TemporalLimits limits = new RICON_TemporalLimits(); limits.m_iMaxSnapshots = cap;
        limits.m_iMaxDescriptorBytes = 5111; limits.m_iMaxTotalDescriptorBytes = bytes;
        return new RICON_TemporalCaptureBatchProbeJournal(7, limits);
    }
    protected static void Code(RICON_TemporalCaptureResult result, string code, bool full = false)
    {
        Check(result && result.m_sCode == code && result.m_bRequiresFullResync == full && result.m_aClosures.Count() == 0);
    }
    protected static bool PairIs(RICON_TemporalCaptureBatchProbeJournal j, int seqA, int seqB, int bytes)
    {
        RICON_TemporalSnapshot a = j.ReadActive(1); RICON_TemporalSnapshot b = j.ReadActive(2);
        return a && b && j.ActiveCount() == 2 && j.TotalDescriptorBytes() == bytes
            && a.m_iLinkSeq == seqA && b.m_iLinkSeq == seqB && j.PendingCount() == 0 && j.Prefix() == 0;
    }
    protected static bool SeedPair(RICON_TemporalCaptureBatchProbeJournal j, bool wideB)
    {
        array<ref RICON_TemporalCaptureInput> seed = {Input(), Input(2, 203, 1, wideB, 3)};
        return Check(j.CapturePublishedBatch(seed).m_sCode == "OK");
    }
    protected static void AtomicAndDuplicateCases()
    {
        RICON_TemporalCaptureBatchProbeJournal j = Journal();
        if (!SeedPair(j, true))
        {
            return;
        }
        RICON_TemporalCaptureInput a = Input(1, 202, 2); RICON_TemporalCaptureInput bad = Input(2, 203, 2, true, 3);
        a.m_Snapshot.m_Descriptor.m_aComponents[0].m_iPhaseQ = 1;
        bad.m_Snapshot.m_Descriptor.m_aComponents[0].m_iPhaseQ = 1073741824;
        array<ref RICON_TemporalCaptureInput> rows = {a, bad};
        Code(j.CapturePublishedBatch(rows), "INVALID_DESCRIPTOR"); Check(PairIs(j, 1, 1, 218));
        rows = {Input(), Input()}; Code(j.CapturePublishedBatch(rows), "DUPLICATE_LINK"); Check(PairIs(j, 1, 1, 218));
        rows = {Input(), Input(2, 203, 1, true, 3)};
        Code(j.CapturePublishedBatch(rows), "DUPLICATE"); Check(PairIs(j, 1, 1, 218));
        rows = {Input(), Input(2, 203, 2, true, 3)};
        Code(j.CapturePublishedBatch(rows), "OK"); Check(PairIs(j, 1, 2, 218));
        rows = {Input(1, 202, 2), Input(2, 203, 1, true, 3)};
        Code(j.CapturePublishedBatch(rows), "STALE"); Check(PairIs(j, 1, 2, 218));
        rows = {Input(1, 202, 2), Input(2, 203, 2, true, 3)};
        rows[1].m_Snapshot.m_Descriptor.m_aComponents[0].m_iPhaseQ = 2;
        Code(j.CapturePublishedBatch(rows), "LINK_SEQUENCE_CONFLICT", true); Check(PairIs(j, 1, 2, 218));
    }
    protected static void FinalBudgetCases()
    {
        RICON_TemporalCaptureBatchProbeJournal j = Journal(2, 218);
        if (!SeedPair(j, true))
        {
            return;
        }
        array<ref RICON_TemporalCaptureInput> swap = {Input(1, 202, 2, true), Input(2, 203, 2, false, 3)};
        Code(j.CapturePublishedBatch(swap), "OK"); Check(PairIs(j, 2, 2, 218));
        Check(RICON_DescriptorCodec.FrameSize(j.ReadActive(1).m_Descriptor) == 116
            && RICON_DescriptorCodec.FrameSize(j.ReadActive(2).m_Descriptor) == 102);
        j = Journal(2, 218);
        if (!SeedPair(j, true))
        {
            return;
        }
        swap = {Input(2, 203, 2, false, 3), Input(1, 202, 2, true)};
        Code(j.CapturePublishedBatch(swap), "OK"); Check(PairIs(j, 2, 2, 218));
        // Valid baseline204/cap217; never seed an already-overbudget journal.
        j = Journal(2, 217);
        if (!SeedPair(j, false))
        {
            return;
        }
        swap = {Input(1, 202, 2, true), Input(2, 203, 2, false, 3)};
        Code(j.CapturePublishedBatch(swap), "RESOURCE_LIMIT", true); Check(PairIs(j, 1, 1, 204));
        // Incoming204 fits cap421, but final retained218+204=422 does not.
        j = Journal(4, 421);
        if (!SeedPair(j, true))
        {
            return;
        }
        swap = {Input(21, 204), Input(22, 205)};
        Code(j.CapturePublishedBatch(swap), "RESOURCE_LIMIT", true); Check(PairIs(j, 1, 1, 218));
        j = Journal(1, 408); array<ref RICON_TemporalCaptureInput> first = {Input()};
        Code(j.CapturePublishedBatch(first), "OK"); first = {Input(2, 203)};
        Code(j.CapturePublishedBatch(first), "RESOURCE_LIMIT", true); Check(j.ActiveCount() == 1 && j.ReadActive(1));
    }
    protected static RICON_LifecycleResult Terminal25()
    {
        RICON_LifecycleResult r = new RICON_LifecycleResult(RICON_ELifecycleResult.OK);
        RICON_LifecycleEffect end = new RICON_LifecycleEffect(); end.m_eType = RICON_ELifecycleEffectType.LEASE_TERMINATED;
        end.m_iPlayerId = 1; end.m_iGeneration = 1; end.m_iDeviceId = 101; end.m_iLeaseId = 35;
        end.m_eLeaseState = RICON_ETransmitLeaseState.REVOKED; end.m_eTerminalReason = RICON_ELeaseTerminalReason.CLIENT_STOP;
        RICON_LifecycleEffect expiry = new RICON_LifecycleEffect(); expiry.m_eType = RICON_ELifecycleEffectType.VON_EXPIRY_INTENDED;
        expiry.m_iSessionEpoch = 7; expiry.m_iPlayerId = 1; expiry.m_iDeviceId = 101; expiry.m_iLeaseId = 35;
        r.m_aEffects.Insert(end); r.m_aEffects.Insert(expiry); return r;
    }
    protected static void FloorsAndIdentityCases()
    {
        RICON_TemporalCaptureBatchProbeJournal j = Journal(2, 408);
        RICON_TemporalCaptureInput held = Input(11, 202, 5);
        Code(j.CapturePublished(held.m_Snapshot, held.m_Expected, held.m_iReceiverEntityId), "OK");
        array<int> ids = {11}; Code(j.CommitSourceCheckpoint(7, 20, ids), "OK");
        RICON_TemporalCaptureInput z = Input(25, 204, 4);
        Code(j.CapturePublished(z.m_Snapshot, z.m_Expected, z.m_iReceiverEntityId), "OK");
        Check(j.ConsumeLifecycleResult(Terminal25(), 7).m_sCode == "OK" && j.PendingCount() == 1 && j.TotalDescriptorBytes() == 102);
        array<ref RICON_TemporalCaptureInput> rows = {Input(19, 205, 100)};
        Code(j.CapturePublishedBatch(rows), "CLOSED_ID", true);
        rows = {Input(25, 204, 100)}; Code(j.CapturePublishedBatch(rows), "CLOSED_ID", true);
        rows = {Input(11, 202, 6), Input(26, 205)}; Code(j.CapturePublishedBatch(rows), "RESOURCE_LIMIT", true);
        Check(j.Prefix() == 20 && j.PendingCount() == 1 && j.ActiveCount() == 1 && j.ReadActive(11).m_iLinkSeq == 5
            && j.ReadPending(25).m_iCapturedLinkSeq == 4 && j.TotalDescriptorBytes() == 102);
        RICON_TemporalCaptureInput rebind = Input(11, 202, 6); rebind.m_iReceiverEntityId = 9999;
        rows = {rebind}; Code(j.CapturePublishedBatch(rows), "ENTITY_REBIND_REQUIRES_NEW_LINK", true);
        rebind = Input(11, 202, 6); rebind.m_Snapshot.m_Binding.m_iLeaseId = 999; rebind.m_Expected.m_Binding.m_iLeaseId = 999;
        rows = {rebind}; Code(j.CapturePublishedBatch(rows), "BINDING_MISMATCH", true);
        Check(j.Prefix() == 20 && j.PendingCount() == 1 && j.ReadActive(11).m_iLinkSeq == 5);
    }
    protected static void InputsCopiesAndSingleCodes()
    {
        RICON_TemporalCaptureBatchProbeJournal j = Journal(2, int.MAX);
        Check(j.GetCaptureLimit() == 2);
        array<ref RICON_TemporalCaptureInput> rows = {}; Code(j.CapturePublishedBatch(rows), "OK");
        Code(j.CapturePublishedBatch(null), "INVALID_INPUT"); rows = {null}; Code(j.CapturePublishedBatch(rows), "INVALID_INPUT");
        rows = {Input(), Input(2, 203), null}; Code(j.CapturePublishedBatch(rows), "BATCH_LIMIT");
        Check(j.ActiveCount() == 0 && j.PendingCount() == 0 && j.TotalDescriptorBytes() == 0 && j.Prefix() == 0);
        RICON_TemporalCaptureInput input = Input(); rows = {input}; Code(j.CapturePublishedBatch(rows), "OK");
        input.m_Snapshot.m_Descriptor.m_aComponents[0].m_iPhaseQ = 99; input.m_Expected.m_Channel.m_fFrequencyMHz = 151;
        Check(j.ReadActive(1).m_Descriptor.m_aComponents[0].m_iPhaseQ == 0 && j.ReadExpected(1).m_Channel.m_fFrequencyMHz == 150);
        RICON_TemporalSnapshot read = j.ReadActive(1); read.m_Binding.m_iLeaseId = 999;
        Check(j.ReadActive(1).m_Binding.m_iLeaseId == 11);
        Code(j.CapturePublished(null, null, 0), "INVALID_RECEIVER_ENTITY");
        Code(j.CapturePublished(null, null, 1002), "INVALID_SNAPSHOT");
        input = Input(); Code(j.CapturePublished(input.m_Snapshot, null, 1002), "CONTEXT_MISMATCH");
        Code(j.CapturePublished(input.m_Snapshot, input.m_Expected, 1002), "DUPLICATE");
        input = Input(); input.m_Snapshot.m_iSessionEpoch = 8; input.m_Expected.m_iSessionEpoch = 8;
        Code(j.CapturePublished(input.m_Snapshot, input.m_Expected, 1002), "SCOPE_MISMATCH", true);
        input = Input(); input.m_Snapshot.m_iLinkSeq = int.MAX; rows = {input}; Code(j.CapturePublishedBatch(rows), "INVALID_SNAPSHOT");
        input = Input(); input.m_Snapshot.m_iLinkId = int.MAX; input.m_Snapshot.m_iRouteId = int.MAX; input.m_Expected.m_iLinkId = int.MAX;
        rows = {input}; Code(j.CapturePublishedBatch(rows), "OK"); Check(j.ReadActive(int.MAX) != null);
        RICON_TemporalLimits badLimits = new RICON_TemporalLimits(); badLimits.m_iMaxSnapshots = 0;
        j = new RICON_TemporalCaptureBatchProbeJournal(7, badLimits); rows = {};
        Code(j.CapturePublishedBatch(rows), "INVALID_LIMITS"); Check(j.GetCaptureLimit() == 0);
        j = Journal(); input = Input(); input.m_Snapshot.m_Descriptor.m_aComponents[0].m_fPowerWeight = 0;
        Code(j.CapturePublished(input.m_Snapshot, input.m_Expected, 1002), "OK");
        input = Input(); input.m_Snapshot.m_Descriptor.m_aComponents[0].m_fPowerWeight = -0.0;
        rows = {input}; Code(j.CapturePublishedBatch(rows), "DUPLICATE");
    }
    static bool RunAll()
    {
        m_iChecks = 0; m_iFailures = 0; m_iFirstFailure = 0;
        AtomicAndDuplicateCases(); FinalBudgetCases(); FloorsAndIdentityCases(); InputsCopiesAndSingleCodes();
        return m_iFailures == 0;
    }
}
