//! TEST_ONLY independent PC01-PC34 literals. Explicit RunAll only. SDK NOT_RUN by Reviewer.
//! Actual PairService/S2/Lifecycle; scalar-Q double is named. No world/manager-role/audio proof.
class RICON_TemporalProducerFixtureEvaluator : RICON_LinkEvaluator
{
    int m_iCalls;
    float m_fQuality = 0.5;
    RICON_ELinkState m_eState = RICON_ELinkState.CLEAN;
    override RICON_LinkBudget Evaluate(RICON_LinkQuery query)
    {
        m_iCalls++;
        RICON_LinkBudget b = new RICON_LinkBudget();
        b.m_bValid = true;
        b.m_eReason = RICON_EReason.OK;
        b.m_eState = m_eState;
        b.m_fIntelligibility = m_fQuality;
        b.m_fSinrDb = 20;
        return b;
    }
}

//! Read-only floor inspection. No protected state is seeded or changed by tests.
class RICON_TemporalProducerFixtureOwner : RICON_TemporalPublicationOwner
{
    void RICON_TemporalProducerFixtureOwner(int epoch, RICON_TemporalLimits limits, RICON_LinkPublicationLimits publicationLimits) {}
    int PrefixForTest()
    {
        return m_iGlobalPrefix;
    }
}

class RICON_TemporalProducerFixtureSource : RICON_LinkFakeSource
{
    ref array<int> m_aLinks = {};
    RICON_TemporalPublicationOwner m_ObservedOwner;
    int m_iExpectedActive = -1;
    int m_iExpectedJournalActive = -1;
    int m_iExpectedLinkOneSeq = -1;
    bool m_bSawPartial;
    bool m_bTryConsumeReentry;
    string m_sReentryCode;
    void RICON_TemporalProducerFixtureSource(int epoch) {}
    override bool NextLinkSequence(int linkId, out int linkSeq, out bool rotated)
    {
        m_aLinks.Insert(linkId);
        if (m_ObservedOwner)
        {
            RICON_TemporalProducerRegistry r = m_ObservedOwner.ReadProducerRegistry();
            if (m_iExpectedActive >= 0 && r.m_Entries.Count() != m_iExpectedActive)
            {
                m_bSawPartial = true;
            }
            if (m_iExpectedJournalActive >= 0 && m_ObservedOwner.ActiveCount() != m_iExpectedJournalActive)
            {
                m_bSawPartial = true;
            }
            if (m_iExpectedLinkOneSeq >= 0)
            {
                RICON_TemporalSnapshot s = m_ObservedOwner.ReadActive(1);
                if (!s || s.m_iLinkSeq != m_iExpectedLinkOneSeq)
                {
                    m_bSawPartial = true;
                }
            }
            if (m_bTryConsumeReentry)
            {
                m_bTryConsumeReentry = false;
                RICON_LifecycleResult empty = new RICON_LifecycleResult(RICON_ELifecycleResult.OK);
                RICON_TemporalCaptureResult reentry = m_ObservedOwner.ConsumeLifecycleResult(empty, 7);
                m_sReentryCode = reentry.m_sCode;
            }
        }
        return super.NextLinkSequence(linkId, linkSeq, rotated);
    }
}

class RICON_TemporalProducerFixtureFrame
{
    ref RICON_LifecycleStateMachine m_Lifecycle;
    ref RICON_TemporalProducerFixtureOwner m_Owner;
    ref RICON_TemporalProducerTick m_Tick;
    ref RICON_TemporalProducerFixtureSource m_Source;
    ref RICON_TemporalProducerFixtureEvaluator m_Evaluator;
}

//! Contract fault ONLY: normal candidate was validated, but private scratch capture reports a late failure.
class RICON_TemporalProducerFixtureFaultScratch : RICON_TemporalPublicationOwner
{
    void RICON_TemporalProducerFixtureFaultScratch(int epoch, RICON_TemporalLimits limits, RICON_LinkPublicationLimits publicationLimits) {}
    RICON_TemporalCaptureResult SeedActualBaseline(array<ref RICON_TemporalCaptureInput> rows, int reserved, int prefix)
    {
        m_iReservedHighWater = reserved;
        m_iGlobalPrefix = prefix;
        return super.ScratchCapture(rows);
    }
    override protected RICON_TemporalCaptureResult ScratchCapture(array<ref RICON_TemporalCaptureInput> rows)
    {
        return Result("TEST_ONLY_LATE_CAPTURE_FAILURE", true);
    }
}
class RICON_TemporalProducerFixtureFaultOwner : RICON_TemporalProducerFixtureOwner
{
    bool m_bInjectFault;
    bool m_bSeedWasOK;
    void RICON_TemporalProducerFixtureFaultOwner(int epoch, RICON_TemporalLimits limits, RICON_LinkPublicationLimits publicationLimits) {}
    override protected RICON_TemporalPublicationOwner ScratchOwner()
    {
        if (!m_bInjectFault)
        {
            return super.ScratchOwner();
        }
        RICON_TemporalProducerFixtureFaultScratch scratch = new RICON_TemporalProducerFixtureFaultScratch(m_iEpoch, m_Limits, m_PublicationLimits);
        RICON_TemporalProducerRegistry r = ReadProducerRegistry();
        array<ref RICON_TemporalCaptureInput> rows = {};
        for (int i = 0; i < r.m_Entries.Count(); i++)
        {
            RICON_TemporalProducerEntry held = r.m_Entries.GetElement(i);
            rows.Insert(new RICON_TemporalCaptureInput(held.m_Snapshot, held.m_Expected, held.m_iReceiverEntityId));
        }
        RICON_TemporalCaptureResult seeded = scratch.SeedActualBaseline(rows, GetReservedHighWater(), PrefixForTest());
        m_bSeedWasOK = seeded.m_sCode == "OK" || seeded.m_sCode == "DUPLICATE";
        return scratch;
    }
}

class RICON_TemporalProducerFixtures
{
    static int m_iChecks, m_iFailures, m_iFirstFailure;
    static string m_sFirstFailure;
    protected static bool Check(bool ok, string label)
    {
        m_iChecks++;
        if (!ok)
        {
            m_iFailures++;
            if (m_iFirstFailure == 0)
            {
                m_iFirstFailure = m_iChecks;
                m_sFirstFailure = label;
            }
            PrintFormat("[RICON_TEMPORAL_PRODUCER] failed check=%1 case=%2", m_iChecks, label);
        }
        return ok;
    }
    protected static RICON_FadingEndpoint Endpoint(int device)
    {
        RICON_FadingEndpoint e = new RICON_FadingEndpoint();
        e.m_iDeviceId = device;
        e.m_iTransceiverIndex = 0;
        return e;
    }
    protected static RICON_FadingDescriptor Descriptor(int rx)
    {
        RICON_FadingDescriptor d = new RICON_FadingDescriptor();
        d.m_sModelId = "FINITE_MULTIPATH_V1";
        d.m_sProfileId = "TEST_ONLY";
        d.m_iProfileVersion = 1;
        d.m_A = Endpoint(101);
        d.m_B = Endpoint(rx);
        d.m_fFrequencyMHz = 150;
        d.m_fBandwidthHz = 25000;
        d.m_iEnvironmentRevision = 4;
        d.m_iAnchorMs = 90;
        d.m_iValidFromMs = 100;
        d.m_iValidUntilMs = 1200;
        d.m_bStaticChannel = false;
        d.m_bCoverageComplete = true;
        RICON_FadingComponent c = new RICON_FadingComponent();
        c.m_sComponentId = "direct";
        c.m_fPowerWeight = 1;
        c.m_iPhaseQ = 0;
        c.m_iDopplerQ = 1;
        d.m_aComponents.Insert(c);
        return d;
    }
    //! Separate literal query factory, deliberately does not copy fields from Descriptor.
    protected static RICON_FadingQuery ChannelQuery(int rx, int nowMs)
    {
        RICON_FadingQuery q = new RICON_FadingQuery();
        q.m_Tx = Endpoint(101);
        q.m_Rx = Endpoint(rx);
        q.m_fFrequencyMHz = 150;
        q.m_fBandwidthHz = 25000;
        q.m_sProfileId = "TEST_ONLY";
        q.m_iProfileVersion = 1;
        q.m_iEnvironmentRevision = 4;
        q.m_iNowMs = nowMs;
        q.m_iSubUs = 0;
        return q;
    }
    protected static RICON_LifecycleDevice Device(int device, int player)
    {
        RICON_LifecycleDevice d = RICON_LifecycleTraceRun.Device(device, RICON_EDeviceKind.CARRIED, player, true);
        d.m_aAllowed.Insert(new RICON_LifecycleCatalogPair(1, 1));
        return d;
    }
    protected static RICON_PairEndpoint RadioEndpoint(int device, int distance)
    {
        RICON_PairEndpoint e = new RICON_PairEndpoint();
        e.m_Observation = new RICON_RadioObservation();
        e.m_Observation.m_iDeviceId = device;
        e.m_Observation.m_iTransceiverIndex = 0;
        e.m_Observation.m_bPowered = true;
        e.m_Observation.m_iTuningValue = 50000;
        e.m_Observation.m_sNetworkKey = "fixture_net";
        e.m_Observation.m_vAntennaPos = Vector(distance, 2, 0);
        e.m_Profile = new RICON_RadioRfProfile();
        e.m_Profile.m_sProfileId = "TEST_ONLY";
        e.m_Profile.m_sProvenance = "TEST_ONLY";
        e.m_Profile.m_iDeviceId = device;
        e.m_Profile.m_iTransceiverIndex = 0;
        e.m_Profile.m_iExpectedTuningValue = 50000;
        e.m_Profile.m_fFrequencyMHz = 150;
        e.m_Profile.m_fRxBandwidthHz = 25000;
        e.m_Profile.m_fTxPowerDbm = 37;
        e.m_Profile.m_fRxNoiseFigureDb = 5;
        e.m_Profile.m_bHasRxSensitivity = true;
        e.m_Profile.m_fRxSensitivityDbm = -120;
        return e;
    }
    protected static RICON_LifecycleStateMachine Lifecycle(bool receiverOwnedByTx = false)
    {
        RICON_LifecycleStateMachine m = new RICON_LifecycleStateMachine();
        int rxOwner = 2;
        if (receiverOwnedByTx) { rxOwner = 1; }
        array<ref RICON_LifecycleDevice> devices = {Device(101, 1), Device(202, rxOwner), Device(203, 3)};
        RICON_LifecycleConfig config = RICON_LifecycleTraceRun.Config(8, 8, 16, 60000, 8, 30000, 16, 16, 1000, 50);
        if (!Check(m.Initialize(7, 0, config, devices).m_eResultCode == RICON_ELifecycleResult.OK, "setup Initialize"))
        {
            return null;
        }
        for (int p = 1; p <= 3; p++)
        {
            if (!Check(m.Connected(1, p, 500 + p).m_eResultCode == RICON_ELifecycleResult.OK, "setup Connected"))
            {
                return null;
            }
        }
        for (int q = 1; q <= 3; q++)
        {
            if (!Check(m.Registered(2, q, q).m_eResultCode == RICON_ELifecycleResult.OK, "setup Registered"))
            {
                return null;
            }
        }
        for (int r = 1; r <= 3; r++)
        {
            if (!Check(m.EntityBound(3, r, r, 1000 + r).m_eResultCode == RICON_ELifecycleResult.OK, "setup EntityBound"))
            {
                return null;
            }
        }
        RICON_LifecyclePttRequest start = RICON_LifecycleTraceRun.Start(100, 1, 1, 1, 1, 7, 1, 1, 101, 1, 1, 2000);
        RICON_LifecycleResult grant = m.ProcessPtt(start);
        if (!Check(grant.m_eResultCode == RICON_ELifecycleResult.OK && grant.m_iLeaseId == 1, "setup actual lease1"))
        {
            return null;
        }
        return m;
    }
    protected static RICON_PairServiceInput PairInput(int nowMs, bool two, bool selfPlayer = false)
    {
        RICON_PairServiceInput input = new RICON_PairServiceInput();
        input.m_iNowMs = nowMs;
        input.m_iSessionEpoch = 7;
        input.m_Limits = new RICON_PairServiceLimits();
        input.m_Limits.m_iMaxBindings = 8;
        input.m_Limits.m_iMaxListeners = 8;
        input.m_Limits.m_iMaxPlayers = 8;
        input.m_Limits.m_iMaxDevices = 8;
        input.m_Limits.m_iMaxEndpoints = 8;
        input.m_Limits.m_iMaxPairs = 64;
        input.m_Context = new RICON_RadioQueryContext();
        input.m_Context.m_sBandId = "test_vhf";
        input.m_Context.m_sModelId = "free_space";
        input.m_Context.m_sNoiseEnvironmentId = "none";
        input.m_Context.m_sIntelligibilityModelId = "linear_test";
        RICON_TransmissionBinding b = new RICON_TransmissionBinding();
        b.m_iLeaseId = 1;
        b.m_iSessionEpoch = 7;
        b.m_iTxPlayerId = 1;
        b.m_iTxGeneration = 1;
        b.m_iTxDeviceId = 101;
        b.m_iTxTransceiverIndex = 0;
        b.m_iNetworkTypeId = 1;
        b.m_iChannelId = 1;
        b.m_bActive = true;
        b.m_iIssuedAtMs = 100;
        b.m_iTtlMs = 2000;
        input.m_aBindings.Insert(b);
        for (int p = 1; p <= 3; p++)
        {
            RICON_PairPlayer player = new RICON_PairPlayer();
            player.m_iPlayerId = p;
            player.m_iGeneration = p;
            player.m_eState = RICON_EPlayerLifecycleState.BOUND_ACTIVE;
            input.m_aPlayers.Insert(player);
        }
        int rxOwner = 2;
        if (selfPlayer) { rxOwner = 1; }
        input.m_aDevices = {Device(101, 1), Device(202, rxOwner), Device(203, 3)};
        input.m_aEndpoints = {RadioEndpoint(101, 0), RadioEndpoint(202, 100), RadioEndpoint(203, 200)};
        int last = 2;
        if (two)
        {
            last = 3;
        }
        for (int rx = 2; rx <= last; rx++)
        {
            RICON_PairListener l = new RICON_PairListener();
            l.m_iPlayerId = rx;
            l.m_iGeneration = rx;
            l.m_iSessionEpoch = 7;
            l.m_iDeviceId = 200 + rx;
            l.m_iTransceiverIndex = 0;
            if (selfPlayer)
            {
                l.m_iPlayerId = 1;
                l.m_iGeneration = 1;
            }
            input.m_aListeners.Insert(l);
        }
        return input;
    }
    protected static RICON_TemporalProducerTick Tick(int nowMs, bool two = true, bool selfPlayer = false)
    {
        RICON_TemporalProducerTick t = new RICON_TemporalProducerTick();
        t.m_iSchemaVersion = 2;
        t.m_PairInput = PairInput(nowMs, two, selfPlayer);
        t.m_iTtlMs = 101;
        t.m_fQualityDelta = 0.125;
        t.m_sMode = "SERVER";
        t.m_sTimebaseId = "TB7";
        foreach (RICON_PairListener l : t.m_PairInput.m_aListeners)
        {
            RICON_TemporalChannelInput c = new RICON_TemporalChannelInput();
            c.m_Query = ChannelQuery(l.m_iDeviceId, nowMs);
            c.m_Descriptor = Descriptor(l.m_iDeviceId);
            t.m_ChannelByPair.Set(Key(l.m_iDeviceId, l.m_iPlayerId), c);
        }
        return t;
    }
    protected static string Key(int rx = 202, int player = 2)
    {
        return RICON_TemporalPublicationService.PairKey(1, rx, 0, player);
    }
    protected static RICON_LinkPublicationLimits PublicationLimits()
    {
        RICON_LinkPublicationLimits p = new RICON_LinkPublicationLimits();
        p.m_iMaxBindings = 8;
        p.m_iMaxPairs = 64;
        p.m_iMaxActiveLinks = 8;
        return p;
    }
    protected static RICON_TemporalProducerFixtureFrame Frame(bool two = true, int total = 500, int tracked = 8, bool selfPlayer = false)
    {
        RICON_TemporalProducerFixtureFrame f = new RICON_TemporalProducerFixtureFrame();
        f.m_Lifecycle = Lifecycle(selfPlayer);
        if (!f.m_Lifecycle)
        {
            return null;
        }
        RICON_TemporalLimits lim = new RICON_TemporalLimits();
        lim.m_iMaxSnapshots = tracked;
        lim.m_iMaxDescriptorBytes = 5111;
        lim.m_iMaxTotalDescriptorBytes = total;
        f.m_Owner = new RICON_TemporalProducerFixtureOwner(7, lim, PublicationLimits());
        f.m_Tick = Tick(100, two, selfPlayer);
        f.m_Source = new RICON_TemporalProducerFixtureSource(7);
        f.m_Evaluator = new RICON_TemporalProducerFixtureEvaluator();
        return f;
    }
    protected static RICON_TemporalPublicationResult Publish(RICON_TemporalProducerFixtureFrame f, RICON_TemporalProducerTick tick)
    {
        int nowMs = 100;
        if (tick && tick.m_PairInput)
        {
            nowMs = tick.m_PairInput.m_iNowMs;
        }
        return f.m_Owner.Publish(tick, f.m_Evaluator, f.m_Lifecycle, nowMs, f.m_Source);
    }
    protected static bool Warm(RICON_TemporalProducerFixtureFrame f)
    {
        if (!Check(f != null, "setup Frame"))
        {
            return false;
        }
        RICON_TemporalPublicationResult r = Publish(f, f.m_Tick);
        if (!Check(r && r.m_sCode == "OK", "setup first Publish"))
        {
            return false;
        }
        int count = f.m_Tick.m_PairInput.m_aListeners.Count();
        if (!Check(r.m_aSnapshots.Count() == count && f.m_Owner.ActiveCount() == count, "setup complete active baseline"))
        {
            return false;
        }
        return Check(f.m_Owner.ReadProducerRegistry().m_Entries.Count() == count, "setup complete registry baseline");
    }
    protected static bool SameExpected(RICON_TemporalExpected a, RICON_TemporalExpected b)
    {
        if (!a || !b || !a.m_Binding.Same(b.m_Binding))
        {
            return false;
        }
        if (a.m_iSessionEpoch != b.m_iSessionEpoch || a.m_iLinkId != b.m_iLinkId)
        {
            return false;
        }
        if (a.m_iTalkerPlayerId != b.m_iTalkerPlayerId || a.m_iListenerPlayerId != b.m_iListenerPlayerId)
        {
            return false;
        }
        if (a.m_sMode != b.m_sMode || a.m_sTimebaseId != b.m_sTimebaseId)
        {
            return false;
        }
        RICON_TemporalChannelContext x = a.m_Channel;
        RICON_TemporalChannelContext y = b.m_Channel;
        if (!x || !y)
        {
            return false;
        }
        if (!RICON_TemporalSnapshots.SameEndpoint(x.m_Tx, y.m_Tx) || !RICON_TemporalSnapshots.SameEndpoint(x.m_Rx, y.m_Rx))
        {
            return false;
        }
        if (x.m_fFrequencyMHz != y.m_fFrequencyMHz || x.m_fBandwidthHz != y.m_fBandwidthHz)
        {
            return false;
        }
        return x.m_sProfileId == y.m_sProfileId && x.m_iProfileVersion == y.m_iProfileVersion && x.m_iEnvironmentRevision == y.m_iEnvironmentRevision;
    }
    protected static bool SameRegistry(RICON_TemporalProducerRegistry a, RICON_TemporalProducerRegistry b)
    {
        if (!a || !b || a.m_Entries.Count() != b.m_Entries.Count())
        {
            return false;
        }
        if (a.m_iSessionEpoch != b.m_iSessionEpoch || a.m_iHighWaterLinkId != b.m_iHighWaterLinkId || a.m_iLastTickMs != b.m_iLastTickMs)
        {
            return false;
        }
        if (a.m_bNeedsFullResync != b.m_bNeedsFullResync)
        {
            return false;
        }
        for (int i = 0; i < a.m_Entries.Count(); i++)
        {
            string key = a.m_Entries.GetKey(i);
            RICON_TemporalProducerEntry x = a.m_Entries.Get(key);
            RICON_TemporalProducerEntry y = b.m_Entries.Get(key);
            if (!x || !y || !RICON_TemporalProtocolRules.SameSnapshot(x.m_Snapshot, y.m_Snapshot))
            {
                return false;
            }
            if (!SameExpected(x.m_Expected, y.m_Expected))
            {
                return false;
            }
            if (x.m_iPublishedAtMs != y.m_iPublishedAtMs || x.m_iReceiverEntityId != y.m_iReceiverEntityId)
            {
                return false;
            }
        }
        return true;
    }
    protected static void RejectZeroNext(RICON_TemporalProducerFixtureFrame f, RICON_TemporalProducerTick tick, string label)
    {
        RICON_TemporalProducerRegistry before = f.m_Owner.ReadProducerRegistry();
        int calls = f.m_Source.m_iCalls;
        int active = f.m_Owner.ActiveCount();
        int pending = f.m_Owner.PendingCount();
        int bytes = f.m_Owner.TotalDescriptorBytes();
        int prefix = f.m_Owner.PrefixForTest();
        int reserved = f.m_Owner.GetReservedHighWater();
        array<ref RICON_TemporalSnapshot> journalBefore = {};
        for (int j = 0; j < before.m_Entries.Count(); j++)
        {
            journalBefore.Insert(f.m_Owner.ReadActive(before.m_Entries.GetElement(j).m_Snapshot.m_iLinkId));
        }
        RICON_TemporalPublicationResult r = Publish(f, tick);
        if (!Check(r != null, label + " result"))
        {
            return;
        }
        Check(r.m_sCode != "OK" && r.m_iSequenceCalls == 0, label + " reject/zeroNext");
        Check(f.m_Source.m_iCalls == calls && f.m_Owner.GetReservedHighWater() == reserved, label + " source/reservation inert");
        Check(r.m_aSnapshots.Count() == 0 && r.m_aEnds.Count() == 0, label + " no output");
        Check(SameRegistry(before, f.m_Owner.ReadProducerRegistry()), label + " registry unchanged");
        Check(f.m_Owner.ActiveCount() == active && f.m_Owner.PendingCount() == pending, label + " journal counts");
        Check(f.m_Owner.TotalDescriptorBytes() == bytes && f.m_Owner.PrefixForTest() == prefix, label + " journal bytes/floor");
        for (int i = 0; i < before.m_Entries.Count(); i++)
        {
            RICON_TemporalSnapshot s = before.m_Entries.GetElement(i).m_Snapshot;
            RICON_TemporalSnapshot prior = journalBefore[i];
            RICON_TemporalSnapshot after = f.m_Owner.ReadActive(s.m_iLinkId);
            bool same = !prior && !after;
            if (prior && after)
            {
                same = RICON_TemporalProtocolRules.SameSnapshot(prior, after);
            }
            Check(same, label + " journal payload");
        }
    }
    protected static void Wide(RICON_FadingDescriptor d)
    {
        RICON_FadingComponent x = new RICON_FadingComponent();
        x.m_sComponentId = "x";
        x.m_fPowerWeight = 0;
        d.m_aComponents.Insert(x);
    }
    protected static void BasicAndCopies()
    {
        RICON_TemporalProducerFixtureFrame f = Frame();
        if (!Check(f != null, "PC01 Frame"))
        {
            return;
        }
        f.m_Source.m_ObservedOwner = f.m_Owner;
        f.m_Source.m_iExpectedActive = 0;
        f.m_Source.m_iExpectedJournalActive = 0;
        RICON_TemporalPublicationResult r = Publish(f, f.m_Tick);
        if (!Check(r && r.m_sCode == "OK", "PC01 initial OK"))
        {
            return;
        }
        if (!Check(r.m_aSnapshots.Count() == 2 && r.m_aEnds.Count() == 0, "PC01 two ACTIVE/no END"))
        {
            return;
        }
        Check(r.m_iSequenceCalls == 2 && r.m_iEpochReads == 3, "PC01 real fake-source counts");
        Check(!f.m_Source.m_bSawPartial, "PC27 no publication prefix observed in Next");
        Check(f.m_Owner.ActiveCount() == 2 && f.m_Owner.TotalDescriptorBytes() == 204, "PC01 journal baseline204");
        RICON_TemporalProducerRegistry stored = f.m_Owner.ReadProducerRegistry();
        Check(stored.m_Entries.Count() == 2 && stored.m_iHighWaterLinkId == 2, "PC01 registry adopted");
        Check(stored.m_Entries.Get(Key()).m_iReceiverEntityId == 1002, "PC01 actual RX entity");
        RICON_TemporalSnapshot a = f.m_Owner.ReadActive(1);
        if (!Check(a != null, "PC01 journal read"))
        {
            return;
        }
        Check(a.m_iSchemaVersion == 2 && a.m_iLinkSeq == 1, "PC01 schema2 seq1");
        Check(a.m_Binding.m_iLeaseId == 1 && a.m_Binding.m_iRxGeneration == 2, "PC01 actual lease/gen");
        Check(a.m_iListenerPlayerId == 2 && a.m_sMode == "SERVER", "PC01 directed context");
        Check(r.m_aSnapshots[0].m_iLinkId == 1 && r.m_aSnapshots[1].m_iLinkId == 2, "PC29 sorted assigned IDs");
        r.m_aSnapshots[0].m_Descriptor.m_aComponents[0].m_iPhaseQ = 99;
        f.m_Tick.m_ChannelByPair.Get(Key()).m_Descriptor.m_aComponents[0].m_iPhaseQ = 88;
        stored.m_Entries.Get(Key()).m_Expected.m_Channel.m_fFrequencyMHz = 151;
        stored.m_Entries.Get(Key()).m_Snapshot.m_Binding.m_iChannelId = 99;
        a.m_Descriptor.m_aComponents[0].m_iPhaseQ = 77;
        Check(f.m_Owner.ReadActive(1).m_Descriptor.m_aComponents[0].m_iPhaseQ == 0, "PC04 journal detached");
        stored = f.m_Owner.ReadProducerRegistry();
        Check(stored.m_Entries.Get(Key()).m_Snapshot.m_Descriptor.m_aComponents[0].m_iPhaseQ == 0, "PC04 registry detached");
        Check(stored.m_Entries.Get(Key()).m_Expected.m_Channel.m_fFrequencyMHz == 150, "PC04 Expected detached");
        Check(stored.m_Entries.Get(Key()).m_Snapshot.m_Binding.m_iChannelId == 1, "PC04 binding detached");
    }
    protected static void ActualEvaluatorSmoke()
    {
        RICON_TemporalProducerFixtureFrame f = Frame(false);
        if (!Check(f != null, "actual evaluator Frame"))
        {
            return;
        }
        RICON_PairCountedEvaluator actual = new RICON_PairCountedEvaluator();
        RICON_TemporalPublicationResult r = f.m_Owner.Publish(f.m_Tick, actual, f.m_Lifecycle, 100, f.m_Source);
        Check(r && r.m_sCode == "OK", "actual Pair/S2/FreeSpace evaluator OK");
        Check(actual.m_iActualCalls == 1, "actual evaluator called once");
        Check(f.m_Owner.ActiveCount() == 1, "actual evaluator adopted active");
    }
    protected static void DescriptorCanonicalAndTiming()
    {
        RICON_TemporalProducerFixtureFrame f = Frame();
        if (!Warm(f))
        {
            return;
        }
        RICON_TemporalProducerTick t = Tick(110);
        t.m_ChannelByPair.Get(Key()).m_Descriptor.m_aComponents[0].m_iPhaseQ = 1;
        RICON_TemporalPublicationResult r = Publish(f, t);
        if (!Check(r && r.m_sCode == "OK", "PC02 descriptor only OK"))
        {
            return;
        }
        if (!Check(r.m_iSequenceCalls == 1 && r.m_aSnapshots.Count() == 1, "PC02 only changed pair issues seq"))
        {
            return;
        }
        Check(r.m_aSnapshots[0].m_iLinkId == 1 && r.m_aSnapshots[0].m_iLinkSeq == 2, "PC02 A seq2");
        Check(f.m_Owner.ReadActive(1).m_Descriptor.m_aComponents[0].m_iPhaseQ == 1, "PC02 phase1 journal");
        Check(f.m_Owner.ReadActive(2).m_iLinkSeq == 1, "PC02 B unchanged");
        f = Frame(false);
        if (!Check(f != null, "PC03 Frame"))
        {
            return;
        }
        Wide(f.m_Tick.m_ChannelByPair.Get(Key()).m_Descriptor);
        if (!Warm(f))
        {
            return;
        }
        t = Tick(110, false);
        Wide(t.m_ChannelByPair.Get(Key()).m_Descriptor);
        t.m_ChannelByPair.Get(Key()).m_Descriptor.m_aComponents[1].m_fPowerWeight = -0.0;
        r = Publish(f, t);
        Check(r && r.m_sCode == "OK" && r.m_iSequenceCalls == 0, "PC03 canonical negative zero noop");
        Check(r.m_aSnapshots.Count() == 0, "PC03 no emitted raw-sign change");
        Check(f.m_Owner.ReadProducerRegistry().m_Entries.Get(Key()).m_iPublishedAtMs == 100, "PC03 receipt unchanged");
        f = Frame();
        if (!Warm(f))
        {
            return;
        }
        r = Publish(f, Tick(150));
        Check(r && r.m_sCode == "OK" && r.m_iSequenceCalls == 0, "PC05 150 before ceil halfTTL");
        r = Publish(f, Tick(151));
        Check(r && r.m_sCode == "OK" && r.m_iSequenceCalls == 2, "PC05 151 exact keepalive boundary");
        RICON_TemporalSnapshot keep = f.m_Owner.ReadActive(1);
        Check(keep.m_iTtlMs == 101 && keep.m_Descriptor.m_iAnchorMs == 90, "PC05 TTL is not model anchor");
        Check(keep.m_Descriptor.m_iValidFromMs == 100 && keep.m_Descriptor.m_iValidUntilMs == 1200, "PC05 original physical interval");
        RICON_FadingResult expired = RICON_FiniteMultipath.Evaluate(keep.m_Descriptor, ChannelQuery(202, 1200));
        Check(expired.m_eCode == RICON_EFadingCode.STALE_DESCRIPTOR, "PC05 keepalive cannot extend physical validity");
        f = Frame(false);
        if (!Warm(f))
        {
            return;
        }
        t = Tick(110, false);
        t.m_ChannelByPair.Get(Key()).m_Descriptor.m_iAnchorMs = 91;
        t.m_ChannelByPair.Get(Key()).m_Descriptor.m_aComponents[0].m_iPhaseQ = 1000;
        r = Publish(f, t);
        Check(r && r.m_sCode == "OK" && r.m_iSequenceCalls == 1, "PC06 exact continuous rebase publishes bytes");
        keep = f.m_Owner.ReadActive(1);
        Check(keep.m_Descriptor.m_iAnchorMs == 91 && keep.m_Descriptor.m_aComponents[0].m_iPhaseQ == 1000, "PC06 no sequence-seeded phase reset");
        RICON_FadingResult oldSample = RICON_FiniteMultipath.Evaluate(Descriptor(202), ChannelQuery(202, 110));
        RICON_FadingResult rebased = RICON_FiniteMultipath.Evaluate(keep.m_Descriptor, ChannelQuery(202, 110));
        Check(oldSample.m_eCode == RICON_EFadingCode.OK && rebased.m_eCode == RICON_EFadingCode.OK, "PC06 both physical samples eligible");
        Check(oldSample.m_aPhasesQ[0] == 20000 && rebased.m_aPhasesQ[0] == 20000, "PC06 literal phase20000 at110ms");
        Check(oldSample.m_fReal == rebased.m_fReal && oldSample.m_fImag == rebased.m_fImag, "PC06 phase20000 literal continuity");
        f = Frame(false);
        if (!Warm(f))
        {
            return;
        }
        t = Tick(110, false);
        f.m_Evaluator.m_fQuality = 0.5625;
        r = Publish(f, t);
        Check(r && r.m_sCode == "OK" && r.m_iSequenceCalls == 0, "PC07 below Q delta no publish");
        f.m_Evaluator.m_fQuality = 0.625;
        r = Publish(f, Tick(111, false));
        Check(r && r.m_sCode == "OK" && r.m_iSequenceCalls == 1, "PC07 exact Q delta publish");
        Check(f.m_Owner.ReadActive(1).m_fQuality == 0.625, "PC07 last published Q comparison");
        f = Frame(false);
        if (!Warm(f))
        {
            return;
        }
        t = Tick(110, false);
        t.m_fQualityDelta = 0.0625;
        r = Publish(f, t);
        Check(r && r.m_sCode == "OK" && r.m_iSequenceCalls == 0, "PC07 threshold alone no publish");
    }
    protected static void PreflightMatrix()
    {
        for (int k = 0; k < 28; k++)
        {
            RICON_TemporalProducerFixtureFrame f = Frame();
            if (!Warm(f))
            {
                return;
            }
            RICON_TemporalProducerTick t = Tick(110);
            RICON_TemporalChannelInput bad = t.m_ChannelByPair.Get(Key(203, 3));
            t.m_ChannelByPair.Get(Key()).m_Descriptor.m_aComponents[0].m_iPhaseQ = 1;
            if (k == 0) { t.m_ChannelByPair.Remove(Key(203, 3)); }
            if (k == 1) { bad.m_Query = null; }
            if (k == 2) { bad.m_Descriptor.m_aComponents[0].m_iPhaseQ = 1073741824; }
            if (k == 3) { bad.m_Descriptor.m_sModelId = "UNSUPPORTED"; }
            if (k == 4) { t.m_iSchemaVersion = 1; }
            if (k == 5) { t.m_sMode = "ALIEN"; }
            if (k == 6) { t.m_sTimebaseId = ""; }
            if (k == 7) { bad.m_Descriptor.m_fFrequencyMHz = 151; }
            if (k == 8)
            {
                bad.m_Descriptor.m_fFrequencyMHz = 151;
                bad.m_Query.m_fFrequencyMHz = 151;
            }
            if (k == 9) { bad.m_Query.m_Rx.m_iDeviceId = 999; }
            if (k == 10) { bad.m_Descriptor = null; }
            if (k == 11) { t.m_PairInput.m_aBindings = null; }
            if (k == 12) { t.m_PairInput.m_aPlayers[1].m_iGeneration = 99; }
            if (k == 13) { t.m_PairInput.m_aListeners.Insert(t.m_PairInput.m_aListeners[0]); }
            if (k == 14) { t.m_iTtlMs = 0; }
            if (k == 15) { t.m_PairInput.m_iNowMs = 99; }
            if (k == 16) { t.m_ChannelByPair.Get(Key()).m_Descriptor.m_aComponents[0].m_iPhaseQ = 1073741824; }
            if (k == 17) { t.m_PairInput.m_Limits.m_iMaxPairs = 1; }
            if (k == 18) { t.m_PairInput.m_aEndpoints[1].m_Observation.m_iTuningValue = 50001; }
            if (k == 19) { t.m_PairInput.m_aDevices[1].m_bPowered = false; }
            if (k == 20) { t.m_PairInput.m_aDevices[1].m_iOwnerPlayerId = 99; }
            if (k == 21) { t.m_PairInput.m_aDevices[1].m_aAllowed[0].m_iChannelId = 2; }
            if (k == 22) { t.m_PairInput.m_aDevices[1].m_aAccessPlayerIds.Insert(99); }
            if (k == 23) { t.m_PairInput.m_aBindings[0].m_iIssuedAtMs = 99; }
            if (k == 24) { t.m_PairInput.m_aBindings[0].m_iChannelId = 2; }
            if (k == 25) { t.m_PairInput.m_aListeners[0].m_iGeneration = 99; }
            if (k == 26) { t.m_PairInput.m_aDevices.Remove(1); }
            if (k == 27)
            {
                t.m_PairInput.m_aBindings[0].m_iChannelId = 2;
                t.m_ChannelByPair.Clear();
            }
            RejectZeroNext(f, t, "PC08/09/21/22/24 preflight variant " + k);
        }
        RICON_TemporalProducerFixtureFrame empty = Frame();
        if (!Warm(empty))
        {
            return;
        }
        RejectZeroNext(empty, null, "PC09 null tick");
        RICON_TemporalProducerTick missing = Tick(110);
        missing.m_PairInput = null;
        RejectZeroNext(empty, missing, "PC09 null pair input");
    }
    protected static void SourceFailuresAndReservations()
    {
        RICON_TemporalProducerFixtureFrame f = Frame();
        if (!Check(f != null, "PC12 initial Frame"))
        {
            return;
        }
        f.m_Source.m_iFailAt = 2;
        RICON_TemporalPublicationResult r = Publish(f, f.m_Tick);
        if (!Check(r && r.m_sCode == "NEED_FULL_RESYNC", "PC12 initial second Next fails"))
        {
            return;
        }
        Check(r.m_iSequenceCalls == 2 && r.m_iEpochReads == 3, "PC12 failed attempts counted");
        Check(r.m_aSnapshots.Count() == 0 && r.m_aEnds.Count() == 0, "PC12 no partial output");
        Check(f.m_Owner.GetReservedHighWater() == 2 && f.m_Owner.ProducerNeedsFull(), "PC12 reserved1/2 retained");
        Check(f.m_Owner.ActiveCount() == 0 && f.m_Owner.ReadProducerRegistry().m_Entries.Count() == 0, "PC12 no fake partial adoption");
        Check(f.m_Owner.ReadProducerRegistry().m_iHighWaterLinkId == 0, "PC12 committed H distinct from reservation H");
        Check(f.m_Source.m_mSeq.Get(1) == 1, "PC12 issued seq1 cannot rollback");
        f.m_Source.m_iFailAt = 0;
        r = Publish(f, Tick(101));
        if (!Check(r && r.m_sCode == "OK", "PC12 same epoch retry OK"))
        {
            return;
        }
        if (!Check(r.m_aSnapshots.Count() == 2 && r.m_iSequenceCalls == 2, "PC12 complete retry"))
        {
            return;
        }
        Check(r.m_aSnapshots[0].m_iLinkId == 3 && r.m_aSnapshots[1].m_iLinkId == 4, "PC12 retry assigns3/4 not1/2");
        Check(!f.m_Owner.ReadActive(1) && !f.m_Owner.ReadActive(2), "PC12 abandoned IDs never active");
        Check(f.m_Owner.GetReservedHighWater() == 4 && f.m_Owner.ActiveCount() == 2, "PC12 successful retry adopts both");
        f = Frame();
        if (!Warm(f))
        {
            return;
        }
        RICON_TemporalProducerRegistry before = f.m_Owner.ReadProducerRegistry();
        f.m_Source.m_ObservedOwner = f.m_Owner;
        f.m_Source.m_iExpectedActive = 2;
        f.m_Source.m_iExpectedJournalActive = 2;
        f.m_Source.m_iExpectedLinkOneSeq = 1;
        f.m_Source.m_iFailAt = f.m_Source.m_iCalls + 2;
        RICON_TemporalProducerTick t = Tick(110);
        t.m_ChannelByPair.Get(Key()).m_Descriptor.m_aComponents[0].m_iPhaseQ = 1;
        t.m_ChannelByPair.Get(Key(203, 3)).m_Descriptor.m_aComponents[0].m_iPhaseQ = 1;
        r = Publish(f, t);
        Check(r && r.m_sCode == "NEED_FULL_RESYNC", "PC10 later Next failure");
        Check(r.m_iSequenceCalls == 2 && r.m_iEpochReads == 3, "PC10 all attempts transparent");
        Check(r.m_aSnapshots.Count() == 0 && r.m_aEnds.Count() == 0, "PC10 no prefix output");
        Check(SameRegistry(before, f.m_Owner.ReadProducerRegistry()), "PC10 complete old registry remains");
        Check(f.m_Owner.ReadActive(1).m_iLinkSeq == 1 && f.m_Owner.ReadActive(2).m_iLinkSeq == 1, "PC10 journal remains seq1/1");
        Check(f.m_Source.m_mSeq.Get(1) == 2 && !f.m_Source.m_bSawPartial, "PC10 consumed external seq not commit proof");
        for (int k = 0; k < 5; k++)
        {
            f = Frame(false);
            if (!Warm(f))
            {
                return;
            }
            before = f.m_Owner.ReadProducerRegistry();
            if (k < 3)
            {
                f.m_Source.m_iInvalidAt = f.m_Source.m_iCalls + 1;
                f.m_Source.m_iInvalidSeq = 0;
                if (k == 1) { f.m_Source.m_iInvalidSeq = int.MAX; }
                if (k == 2) { f.m_Source.m_iInvalidSeq = 1; }
            }
            else if (k == 3)
            {
                f.m_Source.m_iRotateAt = f.m_Source.m_iCalls + 1;
                f.m_Source.SetRotatedEpoch(8);
            }
            else
            {
                f.m_Source.SetSequence(1, int.MAX - 1);
            }
            t = Tick(110, false);
            t.m_ChannelByPair.Get(Key()).m_Descriptor.m_aComponents[0].m_iPhaseQ = 1;
            r = Publish(f, t);
            Check(r && r.m_sCode == "NEED_FULL_RESYNC", "PC11 invalid/rotated source " + k);
            Check(r.m_iSequenceCalls == 1 && r.m_iEpochReads == 2, "PC11 no further Next " + k);
            Check(SameRegistry(before, f.m_Owner.ReadProducerRegistry()), "PC11 old registry preserved " + k);
            Check(f.m_Owner.ReadActive(1).m_iSessionEpoch == 7, "PC14 old journal epoch preserved " + k);
        }
        f = Frame(false);
        if (!Check(f != null, "PC15 Frame"))
        {
            return;
        }
        f.m_Source.m_iEpoch = 8;
        r = Publish(f, f.m_Tick);
        Check(r && r.m_sCode == "NEED_FULL_RESYNC" && r.m_iSequenceCalls == 0, "PC15 first epoch mismatch no Next");
        Check(r.m_iEpochReads == 1 && f.m_Owner.ActiveCount() == 0, "PC15 no new epoch fake FULL");
    }
    protected static void BudgetsAndOrdering()
    {
        RICON_TemporalProducerFixtureFrame f = Frame(true, 217);
        if (!Warm(f))
        {
            return;
        }
        RICON_TemporalProducerTick t = Tick(110);
        Wide(t.m_ChannelByPair.Get(Key()).m_Descriptor);
        RejectZeroNext(f, t, "PC28 valid204 proposed218 cap217");
        for (int order = 0; order < 2; order++)
        {
            f = Frame(true, 218);
            if (!Check(f != null, "PC28 swap Frame"))
            {
                return;
            }
            Wide(f.m_Tick.m_ChannelByPair.Get(Key(203, 3)).m_Descriptor);
            if (!Warm(f))
            {
                return;
            }
            Check(f.m_Owner.TotalDescriptorBytes() == 218, "PC28 valid seed218");
            t = Tick(110);
            Wide(t.m_ChannelByPair.Get(Key()).m_Descriptor);
            if (order == 1)
            {
                RICON_PairListener a = t.m_PairInput.m_aListeners[0];
                t.m_PairInput.m_aListeners[0] = t.m_PairInput.m_aListeners[1];
                t.m_PairInput.m_aListeners[1] = a;
            }
            RICON_TemporalPublicationResult r = Publish(f, t);
            Check(r && r.m_sCode == "OK" && r.m_iSequenceCalls == 2, "PC28 swap no transient cap " + order);
            Check(f.m_Owner.TotalDescriptorBytes() == 218, "PC28 final bytes218 " + order);
            Check(RICON_DescriptorCodec.FrameSize(f.m_Owner.ReadActive(1).m_Descriptor) == 116, "PC28 A116 " + order);
            Check(RICON_DescriptorCodec.FrameSize(f.m_Owner.ReadActive(2).m_Descriptor) == 102, "PC28 B102 " + order);
        }
        for (int permutation = 0; permutation < 2; permutation++)
        {
            f = Frame();
            if (!Check(f != null, "PC29 permutation Frame"))
            {
                return;
            }
            if (permutation == 1)
            {
                RICON_PairListener first = f.m_Tick.m_PairInput.m_aListeners[0];
                f.m_Tick.m_PairInput.m_aListeners[0] = f.m_Tick.m_PairInput.m_aListeners[1];
                f.m_Tick.m_PairInput.m_aListeners[1] = first;
            }
            RICON_TemporalPublicationResult sorted = Publish(f, f.m_Tick);
            if (!Check(sorted && sorted.m_sCode == "OK", "PC29 permutation OK"))
            {
                return;
            }
            Check(f.m_Source.m_aLinks.Count() == 2 && f.m_Source.m_aLinks[0] == 1 && f.m_Source.m_aLinks[1] == 2, "PC29 actual Next order");
            Check(sorted.m_aSnapshots[0].m_iListenerPlayerId == 2 && sorted.m_aSnapshots[1].m_iListenerPlayerId == 3, "PC29 canonical key order");
        }
        f = Frame(false, 500, 1);
        if (!Warm(f))
        {
            return;
        }
        RejectZeroNext(f, Tick(110), "PC25 retained journal cap preflight");
    }
    protected static RICON_TemporalProducerTick EmptyTick(int nowMs)
    {
        RICON_TemporalProducerTick t = Tick(nowMs);
        t.m_PairInput.m_aBindings.Clear();
        t.m_PairInput.m_aListeners.Clear();
        t.m_ChannelByPair.Clear();
        return t;
    }
    protected static void EmptyWithdrawAndLost()
    {
        RICON_TemporalProducerFixtureFrame f = Frame();
        if (!Warm(f))
        {
            return;
        }
        RICON_TemporalPublicationResult r = Publish(f, EmptyTick(110));
        if (!Check(r && r.m_sCode == "OK", "PC23 complete empty withdrawal OK"))
        {
            return;
        }
        Check(r.m_aSnapshots.Count() == 0 && r.m_aEnds.Count() == 2, "PC23 typed END not LOST snapshot");
        Check(r.m_iSequenceCalls == 2 && f.m_Owner.ActiveCount() == 0, "PC23 removal co-commits journal");
        Check(f.m_Owner.ReadProducerRegistry().m_Entries.Count() == 0, "PC23 removal co-commits registry");
        Check(r.m_aEnds[0].m_iLinkId == 1 && r.m_aEnds[0].m_iLinkSeq == 2, "PC23 old first identity ENDseq2");
        Check(r.m_aEnds[0].m_sReason == "LINK_REMOVED" && r.m_aEnds[0].m_Binding.m_iRxDeviceId == 202, "PC23 old binding/reason");
        r = Publish(f, EmptyTick(111));
        Check(r && r.m_sCode == "OK" && r.m_iSequenceCalls == 0, "PC23 repeated empty no END replay");
        Check(r.m_aEnds.Count() == 0 && r.m_aSnapshots.Count() == 0, "PC23 repeated empty no output");
        f = Frame(false);
        if (!Warm(f))
        {
            return;
        }
        f.m_Evaluator.m_eState = RICON_ELinkState.LOST;
        f.m_Evaluator.m_fQuality = 0;
        r = Publish(f, Tick(110, false));
        if (!Check(r && r.m_sCode == "OK", "PC24 EVALUATED LOST OK"))
        {
            return;
        }
        Check(r.m_aEnds.Count() == 0 && r.m_aSnapshots.Count() == 1, "PC24 LOST ACTIVE not END");
        Check(f.m_Owner.ActiveCount() == 1 && f.m_Owner.ReadActive(1).m_fQuality == 0, "PC24 LOST retained");
        Check(f.m_Owner.ReadActive(1).m_Descriptor.m_iValidUntilMs == 1200, "PC24 LOST temporal data retained");
        f = Frame(false);
        if (!Warm(f))
        {
            return;
        }
        RICON_TemporalProducerTick emptyMedium = Tick(110, false);
        emptyMedium.m_ChannelByPair.Get(Key()).m_Descriptor.m_aComponents.Clear();
        r = Publish(f, emptyMedium);
        Check(r && r.m_sCode == "OK" && r.m_iSequenceCalls == 1, "PC34 valid empty medium is not missing descriptor");
        Check(f.m_Owner.ReadActive(1).m_Descriptor.m_aComponents.Count() == 0, "PC34 zero components retained explicitly");
    }
    protected static void LifecycleStopAndEntityRebind()
    {
        RICON_TemporalProducerFixtureFrame f = Frame();
        if (!Warm(f))
        {
            return;
        }
        RICON_LifecyclePttRequest stop = RICON_LifecycleTraceRun.Stop(110, 1, 2, 1, 1, 7, 1, 2, 1);
        RICON_LifecycleResult stopped = f.m_Lifecycle.ProcessPtt(stop);
        if (!Check(stopped.m_eResultCode == RICON_ELifecycleResult.OK, "PC15 actual Stop"))
        {
            return;
        }
        RICON_TemporalCaptureResult consumed = f.m_Owner.ConsumeLifecycleResult(stopped, 7);
        if (!Check(consumed && consumed.m_sCode == "OK", "PC15 consume real terminal effects"))
        {
            return;
        }
        Check(f.m_Owner.PendingCount() == 2 && f.m_Owner.ActiveCount() == 0, "PC15 old captures precede event");
        Check(f.m_Owner.ReadProducerRegistry().m_Entries.Count() == 2, "PC15 last published registry pending next tick");
        Check(f.m_Owner.ReadPending(1).m_sReason == "CLIENT_STOP", "PC15 original terminal reason");
        RICON_TemporalPublicationResult r = Publish(f, EmptyTick(111));
        if (!Check(r && r.m_sCode == "OK", "PC15 pending endings Publish"))
        {
            return;
        }
        Check(r.m_aEnds.Count() == 2 && r.m_aSnapshots.Count() == 0, "PC15 typed terminal outputs");
        Check(r.m_aEnds[0].m_sReason == "CLIENT_STOP" && r.m_aEnds[1].m_sReason == "CLIENT_STOP", "PC15 no loss of pending reason");
        Check(r.m_aEnds[0].m_Binding.m_iLeaseId == 1 && r.m_aEnds[0].m_iSessionEpoch == 7, "PC15 old lease/epoch in END");
        Check(f.m_Owner.ReadProducerRegistry().m_Entries.Count() == 0, "PC15 pending producer rows withdrawn");
        f = Frame();
        if (!Warm(f))
        {
            return;
        }
        RICON_LifecycleResult changed = f.m_Lifecycle.EntityChanged(110, 2, 2, 1002, 1003);
        if (!Check(changed.m_eResultCode == RICON_ELifecycleResult.OK, "PC18 actual RX EntityChanged"))
        {
            return;
        }
        consumed = f.m_Owner.ConsumeLifecycleResult(changed, 7);
        Check(consumed && consumed.m_sCode == "OK" && f.m_Owner.PendingCount() == 1, "PC18 consume old RX entity receipt");
        r = Publish(f, Tick(111));
        if (!Check(r && r.m_sCode == "OK", "PC18 rebind transaction OK"))
        {
            return;
        }
        Check(r.m_aEnds.Count() == 1 && r.m_aEnds[0].m_iLinkId == 1, "PC18 END old link1");
        Check(r.m_aSnapshots.Count() == 1 && r.m_aSnapshots[0].m_iLinkId == 3, "PC18 new identity link3");
        Check(f.m_Owner.ReadProducerRegistry().m_Entries.Get(Key()).m_iReceiverEntityId == 1003, "PC18 current actual entity1003");
        Check(!f.m_Owner.ReadActive(1) && f.m_Owner.ReadActive(3) != null, "PC18 no in-place rebind");
        f.m_Owner.ConsumeLifecycleResult(changed, 7);
        Check(f.m_Owner.ReadActive(3) != null, "PC18 old event replay keeps new entity link3");
    }
    protected static void DirectBypassAndReentry()
    {
        RICON_TemporalProducerFixtureFrame f = Frame(false);
        if (!Warm(f))
        {
            return;
        }
        RICON_TemporalProducerRegistry before = f.m_Owner.ReadProducerRegistry();
        RICON_TemporalCaptureResult denied = f.m_Owner.CapturePublished(null, null, 0);
        Check(denied && denied.m_sCode == "PRODUCER_OWNER_REQUIRED", "PC32 direct single bypass denied");
        array<ref RICON_TemporalCaptureInput> empty = {};
        denied = f.m_Owner.CapturePublishedBatch(empty);
        Check(denied && denied.m_sCode == "PRODUCER_OWNER_REQUIRED", "PC32 direct batch bypass denied");
        array<int> ids = {1};
        denied = f.m_Owner.CommitSourceCheckpoint(7, 1, ids);
        Check(denied && denied.m_sCode == "PRODUCER_OWNER_REQUIRED", "PC31 caller supplied checkpoint denied");
        Check(SameRegistry(before, f.m_Owner.ReadProducerRegistry()) && f.m_Owner.ActiveCount() == 1, "PC32 bypass inert");
        f = Frame();
        if (!Check(f != null, "PC27 reentry Frame"))
        {
            return;
        }
        f.m_Source.m_ObservedOwner = f.m_Owner;
        f.m_Source.m_iExpectedActive = 0;
        f.m_Source.m_iExpectedJournalActive = 0;
        f.m_Source.m_bTryConsumeReentry = true;
        RICON_TemporalPublicationResult r = Publish(f, f.m_Tick);
        Check(r && r.m_sCode == "NEED_FULL_RESYNC", "PC27 mutating reentry aborts publication");
        Check(f.m_Source.m_sReentryCode == "PRODUCER_BUSY", "PC27 busy guard observable");
        Check(f.m_Owner.ActiveCount() == 0 && f.m_Owner.ReadProducerRegistry().m_Entries.Count() == 0, "PC27 no adoption after interference");
        Check(!f.m_Source.m_bSawPartial, "PC27 old state visible during callbacks");
    }
    protected static void ExplicitTimeAndContext()
    {
        RICON_TemporalProducerFixtureFrame f = Frame(false);
        if (!Warm(f))
        {
            return;
        }
        RICON_TemporalProducerRegistry before = f.m_Owner.ReadProducerRegistry();
        RICON_TemporalProducerTick t = Tick(110, false);
        RICON_TemporalPublicationResult r = f.m_Owner.Publish(t, f.m_Evaluator, f.m_Lifecycle, 109, f.m_Source);
        Check(r && r.m_sCode != "OK" && r.m_iSequenceCalls == 0, "PC33 mixed authoritative tick rejected");
        Check(SameRegistry(before, f.m_Owner.ReadProducerRegistry()), "PC33 mixed time no adoption");
        t = Tick(110, false);
        t.m_ChannelByPair.Get(Key()).m_Query.m_iEnvironmentRevision = 5;
        t.m_ChannelByPair.Get(Key()).m_Descriptor.m_iEnvironmentRevision = 5;
        r = Publish(f, t);
        Check(r && r.m_sCode == "OK" && r.m_iSequenceCalls == 1, "PC20 independent current revision update");
        Check(f.m_Owner.ReadProducerRegistry().m_Entries.Get(Key()).m_Expected.m_Channel.m_iEnvironmentRevision == 5, "PC20 Expected revision5 adopted");
        t = Tick(111, false);
        t.m_ChannelByPair.Get(Key()).m_Query.m_iEnvironmentRevision = 5;
        t.m_ChannelByPair.Get(Key()).m_Descriptor.m_iEnvironmentRevision = 5;
        t.m_sMode = "RECEIVER";
        t.m_sTimebaseId = "TB8";
        r = Publish(f, t);
        Check(r && r.m_sCode == "OK" && r.m_iSequenceCalls == 1, "PC20 mode/timebase update despite same Q");
        Check(f.m_Owner.ReadActive(1).m_sMode == "RECEIVER" && f.m_Owner.ReadActive(1).m_sTimebaseId == "TB8", "PC20 directed current context retained");
    }
    protected static void ReadOnlyDeviceQuery()
    {
        RICON_LifecycleStateMachine m = Lifecycle();
        if (!Check(m != null, "DQ actual lifecycle setup"))
        {
            return;
        }
        Check(m.CanQueryAt(100) && !m.CanQueryAt(99), "DQ ready timefloor domains");
        RICON_LifecycleDevice first = m.QueryDevice(202, 105);
        if (!Check(first != null, "DQ current device clone"))
        {
            return;
        }
        Check(first.m_iOwnerPlayerId == 2 && first.m_bPowered, "DQ actual owner/power");
        first.m_iOwnerPlayerId = 99;
        first.m_bPowered = false;
        first.m_aAllowed[0].m_iChannelId = 99;
        first.m_aAccessPlayerIds.Insert(99);
        RICON_LifecycleDevice second = m.QueryDevice(202, 105);
        Check(second && second.m_iOwnerPlayerId == 2 && second.m_bPowered, "DQ detached scalar fields");
        Check(second.m_aAllowed.Count() == 1 && second.m_aAllowed[0].m_iChannelId == 1, "DQ deep allowed clone");
        Check(second.m_aAccessPlayerIds.Count() == 0, "DQ deep ACL clone");
        Check(!m.QueryDevice(0, 105) && !m.QueryDevice(999, 105), "DQ absent/invalid device IDs");
        Check(!m.QueryDevice(202, 99), "DQ rollback query failclosed");
        Check(m.QueryDevice(202, 2100) != null, "DQ device lifetime independent of TX lease");
        Check(m.QueryActiveLease(1, 105) != null, "DQ future device query did not advance timefloor or Sweep");
        RICON_LifecycleResult tick = m.Tick(106);
        Check(tick.m_eResultCode == RICON_ELifecycleResult.OK, "DQ rollback did not latch anomaly");
        Check(m.QueryActiveLease(1, 106) != null, "DQ genuine next Tick retains active lease");
        RICON_LifecycleStateMachine unready = new RICON_LifecycleStateMachine();
        Check(!unready.CanQueryAt(100) && !unready.QueryDevice(101, 100), "DQ unready failclosed");
    }
    protected static void FullSourceHandoffAndPendingRotation()
    {
        RICON_TemporalProducerFixtureFrame f = Frame(true, 500, 2);
        if (!Warm(f))
        {
            return;
        }
        RICON_LifecycleResult stopped = f.m_Lifecycle.ProcessPtt(RICON_LifecycleTraceRun.Stop(110, 1, 2, 1, 1, 7, 1, 2, 1));
        if (!Check(stopped.m_eResultCode == RICON_ELifecycleResult.OK, "PC25 cap recovery actual Stop"))
        {
            return;
        }
        f.m_Owner.ConsumeLifecycleResult(stopped, 7);
        Check(f.m_Owner.PendingCount() == 2 && f.m_Owner.ActiveCount() == 0, "PC25 cap includes two pending closures");
        int calls = f.m_Source.m_iCalls;
        RICON_TemporalPublicationResult full = f.m_Owner.BuildFullSourceHandoff();
        if (!Check(full && full.m_sCode == "OK", "PC31 private source FULL from own state"))
        {
            return;
        }
        Check(full.m_aActiveIds.Count() == 0 && full.m_ExpectedByLink.Count() == 0, "PC31 projected closed rows withdrawn");
        Check(full.m_iSessionEpoch == 7, "PC31 empty handoff has explicit owner epoch");
        Check(full.m_iReservedHighWater == 2 && f.m_Owner.PrefixForTest() == 2, "PC31 own H2 closes floor");
        Check(f.m_Source.m_iCalls == calls && full.m_iSequenceCalls == 0, "PC31 source handoff is not fresh END issuance");
        Check(f.m_Owner.PendingCount() == 0 && f.m_Owner.ReadProducerRegistry().m_Entries.Count() == 0, "PC25 certified cap recovery no payload resurrection");
        Check(f.m_Owner.ProducerNeedsFull(), "PC16 handoff forces next sequenced full");
        RICON_LifecycleResult grant = f.m_Lifecycle.ProcessPtt(RICON_LifecycleTraceRun.Start(111, 1, 3, 1, 1, 7, 1, 3, 101, 1, 1, 2000));
        if (!Check(grant.m_eResultCode == RICON_ELifecycleResult.OK && grant.m_iLeaseId == 2, "PC19 genuine lease rekey2"))
        {
            return;
        }
        RICON_TemporalProducerTick t = Tick(112);
        t.m_PairInput.m_aBindings[0].m_iLeaseId = 2;
        t.m_PairInput.m_aBindings[0].m_iIssuedAtMs = 111;
        RICON_TemporalChannelInput a = t.m_ChannelByPair.Get(Key());
        RICON_TemporalChannelInput b = t.m_ChannelByPair.Get(Key(203, 3));
        t.m_ChannelByPair.Clear();
        t.m_ChannelByPair.Set(RICON_TemporalPublicationService.PairKey(2, 202, 0, 2), a);
        t.m_ChannelByPair.Set(RICON_TemporalPublicationService.PairKey(2, 203, 0, 3), b);
        RICON_TemporalPublicationResult r = Publish(f, t);
        if (!Check(r && r.m_sCode == "OK", "PC25 new links fit after certified recovery"))
        {
            return;
        }
        Check(r.m_bFull && r.m_aSnapshots.Count() == 2, "PC16 next complete active set sequenced");
        Check(r.m_aSnapshots[0].m_iLinkId == 3 && r.m_aSnapshots[1].m_iLinkId == 4, "PC19 new lease never reuses closed1/2");
        Check(r.m_aSnapshots[0].m_Binding.m_iLeaseId == 2, "PC19 current lease2 binding");
        Check(r.m_aSnapshots[0].m_Descriptor.m_iAnchorMs == 90, "PC19 lease rekey not fading reset");
        Check(f.m_Owner.ActiveCount() == 2 && f.m_Owner.PendingCount() == 0, "PC25 both journals live within cap2");
        f = Frame();
        if (!Warm(f))
        {
            return;
        }
        RICON_LifecycleResult changed = f.m_Lifecycle.EntityChanged(110, 2, 2, 1002, 1003);
        f.m_Owner.ConsumeLifecycleResult(changed, 7);
        RICON_TemporalProducerRegistry prior = f.m_Owner.ReadProducerRegistry();
        RICON_TemporalClosureIntent old = f.m_Owner.ReadPending(1);
        if (!Check(old != null, "PC14 old pending before rotated source"))
        {
            return;
        }
        f.m_Source.m_iRotateAt = f.m_Source.m_iCalls + 1;
        f.m_Source.SetRotatedEpoch(8);
        r = Publish(f, Tick(111));
        Check(r && r.m_sCode == "NEED_FULL_RESYNC", "PC14 rotation aborts rebind" );
        Check(SameRegistry(prior, f.m_Owner.ReadProducerRegistry()), "PC14 old registry not renamed/cleared");
        RICON_TemporalClosureIntent retained = f.m_Owner.ReadPending(1);
        Check(retained && retained.m_iSessionEpoch == 7 && retained.m_iCapturedLinkSeq == old.m_iCapturedLinkSeq, "PC14 old pending epoch/seq retained");
        Check(retained.m_sReason == old.m_sReason && retained.m_Binding.Same(old.m_Binding), "PC14 original pending reason/binding retained");
    }
    protected static void RxGenerationAndReverse()
    {
        RICON_TemporalProducerFixtureFrame f = Frame();
        if (!Warm(f))
        {
            return;
        }
        RICON_LifecycleResult disconnected = f.m_Lifecycle.Disconnected(110, 2, 2, 502);
        if (!Check(disconnected.m_eResultCode == RICON_ELifecycleResult.OK, "PC17 actual old RX disconnect"))
        {
            return;
        }
        f.m_Owner.ConsumeLifecycleResult(disconnected, 7);
        Check(f.m_Lifecycle.Connected(111, 2, 602).m_eResultCode == RICON_ELifecycleResult.OK, "PC17 reconnect");
        if (!Check(f.m_Lifecycle.GetPlayerGeneration(2) == 4, "PC17 actual global RXgeneration4"))
        {
            return;
        }
        Check(f.m_Lifecycle.Registered(112, 2, 4).m_eResultCode == RICON_ELifecycleResult.OK, "PC17 register generation4");
        Check(f.m_Lifecycle.EntityBound(113, 2, 4, 2002).m_eResultCode == RICON_ELifecycleResult.OK, "PC17 bind new RX entity");
        RICON_TemporalProducerTick t = Tick(114);
        t.m_PairInput.m_aPlayers[1].m_iGeneration = 4;
        t.m_PairInput.m_aListeners[0].m_iGeneration = 4;
        RICON_TemporalPublicationResult r = Publish(f, t);
        if (!Check(r && r.m_sCode == "OK", "PC17 actual generation rebind transaction"))
        {
            return;
        }
        Check(r.m_aEnds.Count() == 1 && r.m_aEnds[0].m_iLinkId == 1, "PC17 old link1 ended");
        Check(r.m_aEnds[0].m_Binding.m_iRxGeneration == 2, "PC17 END retains old generation2");
        Check(r.m_aSnapshots.Count() == 1 && r.m_aSnapshots[0].m_iLinkId == 3, "PC17 new identity ID3");
        Check(r.m_aSnapshots[0].m_Binding.m_iRxGeneration == 4, "PC17 ACTIVE current generation4");
        Check(f.m_Owner.ReadActive(2).m_iLinkSeq == 1, "PC22 unchanged B not blanket republished");
        f = Frame(false);
        if (!Check(f != null, "PC30 reverse Frame"))
        {
            return;
        }
        RICON_LifecycleResult secondTx = f.m_Lifecycle.ProcessPtt(RICON_LifecycleTraceRun.Start(100, 1, 10, 2, 2, 7, 2, 1, 202, 1, 1, 2000));
        if (!Check(secondTx.m_eResultCode == RICON_ELifecycleResult.OK && secondTx.m_iLeaseId == 2, "PC30 actual reverse TX grant2"))
        {
            return;
        }
        RICON_TransmissionBinding reverse = new RICON_TransmissionBinding();
        reverse.m_iLeaseId = 2;
        reverse.m_iSessionEpoch = 7;
        reverse.m_iTxPlayerId = 2;
        reverse.m_iTxGeneration = 2;
        reverse.m_iTxDeviceId = 202;
        reverse.m_iTxTransceiverIndex = 0;
        reverse.m_iNetworkTypeId = 1;
        reverse.m_iChannelId = 1;
        reverse.m_bActive = true;
        reverse.m_iIssuedAtMs = 100;
        reverse.m_iTtlMs = 2000;
        f.m_Tick.m_PairInput.m_aBindings.Insert(reverse);
        RICON_PairListener toOne = new RICON_PairListener();
        toOne.m_iPlayerId = 1;
        toOne.m_iGeneration = 1;
        toOne.m_iSessionEpoch = 7;
        toOne.m_iDeviceId = 101;
        toOne.m_iTransceiverIndex = 0;
        f.m_Tick.m_PairInput.m_aListeners.Insert(toOne);
        RICON_TemporalChannelInput back = new RICON_TemporalChannelInput();
        back.m_Query = ChannelQuery(202, 100);
        back.m_Query.m_Tx = Endpoint(202);
        back.m_Query.m_Rx = Endpoint(101);
        back.m_Descriptor = Descriptor(202);
        f.m_Tick.m_ChannelByPair.Set(RICON_TemporalPublicationService.PairKey(2, 101, 0, 1), back);
        r = Publish(f, f.m_Tick);
        if (!Check(r && r.m_sCode == "OK", "PC30 directed reverse valid with canonical descriptor"))
        {
            return;
        }
        Check(r.m_aSnapshots.Count() == 2 && r.m_iSequenceCalls == 2, "PC30 two directed links, self-pairs not new links");
        Check(r.m_aSnapshots[0].m_iTalkerPlayerId == 1 && r.m_aSnapshots[0].m_iListenerPlayerId == 2, "PC30 forward directed header");
        Check(r.m_aSnapshots[1].m_iTalkerPlayerId == 2 && r.m_aSnapshots[1].m_iListenerPlayerId == 1, "PC30 reverse directed header");
        Check(r.m_aSnapshots[1].m_Descriptor.m_A.m_iDeviceId == 101 && r.m_aSnapshots[1].m_Descriptor.m_B.m_iDeviceId == 202, "PC30 canonical min/max shared");
        Check(r.m_aSnapshots[1].m_Descriptor.m_aComponents[0].m_iPhaseQ == 0, "PC30 no reverse-direction phase seed");
    }
    //! PC35 source-review counter: same player may own distinct devices; Snapshot2 still forbids same players.
    protected static void SamePlayerTwoDevicesPreflight()
    {
        RICON_TemporalProducerFixtureFrame f = Frame(false, 500, 8, true);
        if (!Check(f != null, "PC35 same player two devices Frame"))
        {
            return;
        }
        RICON_PairService pair = new RICON_PairService();
        RICON_PairServiceResult evaluated = pair.Evaluate(f.m_Tick.m_PairInput, f.m_Evaluator);
        if (!Check(evaluated.m_eCallCode == RICON_EPairServiceCode.OK && evaluated.m_aPairs.Count() == 1, "PC35 actual PairService source input valid"))
        {
            return;
        }
        Check(evaluated.m_aPairs[0].m_ePairCode == RICON_EPairCode.EVALUATED && evaluated.m_aPairs[0].m_bValid, "PC35 same-player/different-device EVALUATED");
        RejectZeroNext(f, f.m_Tick, "PC35 Snapshot2 same-player structural failure before Next");
    }
    //! New independent coverage after a927 freeze: live B must survive pruning closed A.
    protected static void RemainingActiveFullHandoff()
    {
        RICON_TemporalProducerFixtureFrame f = Frame();
        if (!Warm(f))
        {
            return;
        }
        RICON_LifecycleResult changed = f.m_Lifecycle.EntityChanged(110, 2, 2, 1002, 1003);
        if (!Check(changed.m_eResultCode == RICON_ELifecycleResult.OK, "PC36 actual old RX closure"))
        {
            return;
        }
        f.m_Owner.ConsumeLifecycleResult(changed, 7);
        Check(f.m_Owner.ActiveCount() == 1 && f.m_Owner.PendingCount() == 1, "PC36 reachable liveB/closedA baseline");
        int calls = f.m_Source.m_iCalls;
        RICON_TemporalPublicationResult full = f.m_Owner.BuildFullSourceHandoff();
        if (!Check(full && full.m_sCode == "OK", "PC36 full handoff with remaining active"))
        {
            return;
        }
        if (!Check(full.m_aSnapshots.Count() == 1 && full.m_aActiveIds.Count() == 1, "PC36 exactly complete live set"))
        {
            return;
        }
        Check(full.m_iSessionEpoch == 7 && full.m_iReservedHighWater == 2, "PC36 empty/nonempty header scope7/H2");
        Check(full.m_aActiveIds[0] == 2 && full.m_aSnapshots[0].m_iLinkId == 2, "PC36 only live B2");
        Check(full.m_ExpectedByLink.Count() == 1 && full.m_ExpectedByLink.Contains(2), "PC36 all-owner Expected keyed2");
        Check(f.m_Source.m_iCalls == calls && full.m_iSequenceCalls == 0, "PC36 handoff no fake new issuance");
        Check(f.m_Owner.ActiveCount() == 1 && f.m_Owner.PendingCount() == 0, "PC36 prune closure retain live capture");
        Check(f.m_Owner.PrefixForTest() == 2 && f.m_Owner.ReadProducerRegistry().m_Entries.Count() == 1, "PC36 registry/journal co-adopted");
        Check(f.m_Owner.ReadActive(2).m_iLinkSeq == 1, "PC36 handoff not TTL/seq refresh");
        full.m_aActiveIds[0] = 999;
        full.m_aSnapshots[0].m_Descriptor.m_aComponents[0].m_iPhaseQ = 99;
        full.m_ExpectedByLink.Get(2).m_Channel.m_fFrequencyMHz = 151;
        Check(f.m_Owner.ReadActive(2).m_Descriptor.m_aComponents[0].m_iPhaseQ == 0, "PC36 handoff output detached");
        Check(f.m_Owner.ReadProducerRegistry().m_Entries.Get(Key(203, 3)).m_Expected.m_Channel.m_fFrequencyMHz == 150, "PC36 Expected output detached");
        RICON_TemporalPublicationResult next = Publish(f, Tick(111));
        if (!Check(next && next.m_sCode == "OK", "PC36 next full creates replacement A"))
        {
            return;
        }
        Check(next.m_bFull && next.m_aSnapshots.Count() == 2, "PC36 next sequenced full covers A+B");
        Check(f.m_Owner.ReadActive(2).m_iLinkSeq == 2 && f.m_Owner.ReadActive(3) != null, "PC36 Bseq2/newA3 no closedID reuse");
    }
    //! Controlled unreachable-after-valid-preflight fault, not a live resource/race reproducer.
    protected static void LateScratchCaptureFault()
    {
        RICON_TemporalProducerFixtureFrame f = Frame();
        if (!Check(f != null, "PC37 late scratch fault Frame"))
        {
            return;
        }
        RICON_TemporalLimits limits = new RICON_TemporalLimits();
        limits.m_iMaxSnapshots = 8;
        limits.m_iMaxDescriptorBytes = 5111;
        limits.m_iMaxTotalDescriptorBytes = 500;
        RICON_TemporalProducerFixtureFaultOwner fault = new RICON_TemporalProducerFixtureFaultOwner(7, limits, PublicationLimits());
        f.m_Owner = fault;
        if (!Warm(f))
        {
            return;
        }
        RICON_TemporalProducerRegistry before = fault.ReadProducerRegistry();
        Check(fault.PendingCount() == 0 && fault.PrefixForTest() == 0, "PC37 fault baseline has no omitted closed/floor copy");
        fault.m_bInjectFault = true;
        RICON_TemporalProducerTick t = Tick(110);
        t.m_ChannelByPair.Get(Key()).m_Descriptor.m_aComponents[0].m_iPhaseQ = 1;
        t.m_ChannelByPair.Get(Key(203, 3)).m_Descriptor.m_aComponents[0].m_iPhaseQ = 1;
        RICON_TemporalPublicationResult r = Publish(f, t);
        if (!Check(r && r.m_sCode == "NEED_FULL_RESYNC", "PC37 actual publish handles late scratch error"))
        {
            return;
        }
        Check(r.m_iSequenceCalls == 2 && r.m_iEpochReads == 3, "PC37 fault occurs AFTER two successful Next calls");
        Check(fault.m_bSeedWasOK, "PC37 scratch baseline validated through actual super capture");
        Check(r.m_aSnapshots.Count() == 0 && r.m_aEnds.Count() == 0, "PC37 ready prefix discarded");
        Check(SameRegistry(before, fault.ReadProducerRegistry()), "PC37 committed registry unchanged");
        Check(fault.ReadActive(1).m_iLinkSeq == 1 && fault.ReadActive(2).m_iLinkSeq == 1, "PC37 committed journal unchanged");
        Check(fault.TotalDescriptorBytes() == 204 && fault.PendingCount() == 0 && fault.PrefixForTest() == 0, "PC37 all journal budgets/floors unchanged");
        Check(f.m_Source.m_mSeq.Get(1) == 2 && f.m_Source.m_mSeq.Get(2) == 2, "PC37 consumed external issuer not rolled back");
        Check(fault.ProducerNeedsFull(), "PC37 recovery latched");
    }
    static bool RunAll()
    {
        m_iChecks = 0;
        m_iFailures = 0;
        m_iFirstFailure = 0;
        m_sFirstFailure = "";
        BasicAndCopies();
        ActualEvaluatorSmoke();
        DescriptorCanonicalAndTiming();
        PreflightMatrix();
        SourceFailuresAndReservations();
        BudgetsAndOrdering();
        EmptyWithdrawAndLost();
        LifecycleStopAndEntityRebind();
        DirectBypassAndReentry();
        ExplicitTimeAndContext();
        ReadOnlyDeviceQuery();
        FullSourceHandoffAndPendingRotation();
        RxGenerationAndReverse();
        SamePlayerTwoDevicesPreflight();
        RemainingActiveFullHandoff();
        LateScratchCaptureFault();
        return m_iFailures == 0;
    }
}
