//! TEST_ONLY observation sampler fixtures. No native world source is instantiated or invoked.
//! Manual O01-O18, supplemental to frozen PS36. Observation OK is not complete physical input.
class RICON_PhysicalObservationFixtureHeights : RICON_HeightSource
{
    int m_iCalls;
    int m_iFailAt;
    int m_iNanAt;
    ref array<string> m_Order;
    override bool TryGetHeight(float x, float z, out float heightM)
    {
        m_iCalls++;
        m_Order.Insert("HEIGHT");
        heightM = 0;
        if (m_iCalls == m_iFailAt)
        {
            return false;
        }
        if (m_iCalls == m_iNanAt)
        {
            heightM = RICON_L2Run.NaN();
        }
        return true;
    }
}

//! Explicit source DOUBLE: declared values/budget code are independent of raw float timestamps.
class RICON_PhysicalObservationFixtureSource : RICON_PhysicalObservationSource
{
    int m_iStampCalls, m_iEndpointCalls, m_iHeightGets, m_iTraceCalls, m_iSpanCalls;
    int m_iDriftKind, m_iEndpointFailAt;
    bool m_bStampUnavailable, m_bHeightUnavailable, m_bTraceUnavailable;
    bool m_bContact;
    float m_fFraction = 0.5;
    float m_fRawStart = 100;
    float m_fRawEnd = 101;
    float m_fElapsed = 1;
    string m_sSpanCode = "OK";
    bool m_bUseBaseSpan;
    int m_iLastSpanBudget = -1;
    bool m_bOriginalStamps;
    bool m_bMutateProviderRequest;
    bool m_bLongNetworkKey;
    bool m_bKnownZeroBody;
    ref RICON_PhysicalObservationRequest m_CallerToMutate;
    ref array<string> m_Order = {};
    ref array<ref RICON_PhysicalObservationStamp> m_Stamps = {};
    ref array<ref RICON_PhysicalEndpointWitness> m_Endpoints = {};
    ref array<ref RICON_PhysicalEndpointRequest> m_ProviderRequests = {};
    ref RICON_PhysicalTraceWitness m_Trace;
    ref RICON_PhysicalObservationFixtureHeights m_Heights;
    void RICON_PhysicalObservationFixtureSource()
    {
        m_Heights = new RICON_PhysicalObservationFixtureHeights();
        m_Heights.m_Order = m_Order;
    }
    override RICON_PhysicalObservationStamp ReadStamp()
    {
        m_iStampCalls++;
        m_Order.Insert("STAMP");
        if (m_bStampUnavailable)
        {
            return null;
        }
        RICON_PhysicalObservationStamp s = new RICON_PhysicalObservationStamp();
        s.m_sSourceId = "fixture_source";
        s.m_sTimebaseId = "TEST_CLOCK";
        s.m_sWorldIdentity = "fixture_world";
        s.m_sProvenance = "TEST_ONLY";
        s.m_iSourceRevision = 5;
        s.m_fRawWorldTimeMs = m_fRawStart;
        if (m_iStampCalls % 2 == 0)
        {
            s.m_fRawWorldTimeMs = m_fRawEnd;
            if (m_iDriftKind == 1) { s.m_sWorldIdentity = "other_world"; }
            if (m_iDriftKind == 2) { s.m_sSourceId = "other_source"; }
            if (m_iDriftKind == 3) { s.m_sTimebaseId = "OTHER_CLOCK"; }
            if (m_iDriftKind == 4) { s.m_iSourceRevision = 6; }
            if (m_iDriftKind == 5) { s.m_sProvenance = "ENGINE_GETTER"; }
        }
        m_Stamps.Insert(s);
        if (m_iStampCalls == 1 && m_CallerToMutate)
        {
            m_CallerToMutate.m_Tx.m_vAntennaPos = Vector(99, 99, 99);
            m_CallerToMutate.m_Rx.m_iDeviceId = 999;
            m_CallerToMutate.m_sExpectedSourceId = "changed_by_callback";
            m_CallerToMutate.m_bSampleTerrain = false;
        }
        return s;
    }
override RICON_PhysicalEndpointWitness ReadEndpoint(RICON_PhysicalEndpointRequest endpoint)
    {
        m_iEndpointCalls++;
        m_Order.Insert("ENDPOINT");
        m_ProviderRequests.Insert(endpoint);
        if (m_iEndpointCalls == m_iEndpointFailAt)
        {
            return null;
        }
        RICON_PhysicalEndpointWitness w = new RICON_PhysicalEndpointWitness();
        w.m_Observation = new RICON_RadioObservation();
        w.m_Observation.m_iDeviceId = endpoint.m_iDeviceId;
        w.m_Observation.m_iTransceiverIndex = endpoint.m_iTransceiverIndex;
        w.m_Observation.m_bPowered = true;
        w.m_Observation.m_iTuningValue = 50000;
        w.m_Observation.m_sNetworkKey = "fixture_key";
        w.m_Observation.m_vAntennaPos = endpoint.m_vAntennaPos;
        w.m_sIdentityToken = "radio_" + endpoint.m_iDeviceId.ToString();
        w.m_sAntennaSourceId = endpoint.m_sAntennaSourceId;
        w.m_sMotionProvenance = "MOTION_UNAVAILABLE";
        w.m_sBodyIdentityToken = "";
        w.m_bHasBodyVelocity = false;
        if (m_bKnownZeroBody)
        {
            w.m_bHasBodyVelocity = true;
            w.m_sBodyIdentityToken = "fixture_body_" + endpoint.m_iDeviceId.ToString();
            w.m_sMotionProvenance = "BODY_AT_CALLER_POINT";
            w.m_vBodyVelocityAtPoint = Vector(0, 0, 0);
        }
        w.m_vOwnerOrigin = Vector(1, 1, 1);
        w.m_vOwnerAxis0 = Vector(1, 0, 0);
        w.m_vOwnerAxis1 = Vector(0, 1, 0);
        w.m_vOwnerAxis2 = Vector(0, 0, 1);
        if (m_bLongNetworkKey)
        {
            string longKey;
            for (int i = 0; i < 2049; i++) { longKey += "a"; }
            w.m_Observation.m_sNetworkKey = longKey;
        }
        if (m_iEndpointCalls >= 3)
        {
            if (m_iDriftKind == 6) { w.m_sIdentityToken = "new_radio"; }
            if (m_iDriftKind == 7) { w.m_Observation.m_bPowered = false; }
            if (m_iDriftKind == 8) { w.m_Observation.m_iTuningValue = 50001; }
            if (m_iDriftKind == 9) { w.m_Observation.m_sNetworkKey = "new_fixture_key"; }
            if (m_iDriftKind == 10) { w.m_vOwnerOrigin = Vector(2, 1, 1); }
            if (m_iDriftKind == 11) { w.m_vOwnerAxis0 = Vector(0, 0, 1); }
            if (m_iDriftKind == 12) { w.m_vOwnerAxis1 = Vector(1, 0, 0); }
            if (m_iDriftKind == 13) { w.m_vOwnerAxis2 = Vector(0, 1, 0); }
            if (m_iDriftKind == 14)
            {
                w.m_sBodyIdentityToken = "body_fixture";
                w.m_bHasBodyVelocity = true;
                w.m_sMotionProvenance = "BODY_AT_CALLER_POINT";
                w.m_vBodyVelocityAtPoint = Vector(0, 0, 0);
            }
            if (m_iDriftKind == 15) { w.m_sAntennaSourceId = "other_caller_point"; }
            if (m_iDriftKind == 16) { w.m_Observation.m_iDeviceId = 999; }
            if (m_iDriftKind == 17) { w.m_Observation.m_iTransceiverIndex = 1; }
        }
        m_Endpoints.Insert(w);
        if (m_bMutateProviderRequest)
        {
            endpoint.m_iDeviceId = 999;
            endpoint.m_vAntennaPos = Vector(9, 9, 9);
        }
        return w;
    }
    override RICON_HeightSource GetHeightSource()
    {
        m_iHeightGets++;
        m_Order.Insert("HEIGHT_SOURCE");
        if (m_bHeightUnavailable)
        {
            return null;
        }
        return m_Heights;
    }
    override RICON_PhysicalTraceWitness ReadFirstContact(vector tx, vector rx, int flags, int layerMask)
    {
        m_iTraceCalls++;
        m_Order.Insert("TRACE");
        if (m_bTraceUnavailable)
        {
            return null;
        }
        RICON_PhysicalTraceWitness t = new RICON_PhysicalTraceWitness();
        t.m_bHasContact = m_bContact;
        t.m_fFraction = 1;
        if (m_bContact)
        {
            t.m_fFraction = m_fFraction;
            t.m_sEntityIdentity = "raw_entity";
            t.m_sMaterial = "raw_material_hint";
            t.m_iColliderIndex = 1;
            t.m_iNodeIndex = 2;
            t.m_vNormal = Vector(-1, 0, 0);
        }
        m_Trace = t;
        return t;
    }
    override string MeasureSpan(RICON_PhysicalObservationStamp start, RICON_PhysicalObservationStamp end, int maxSpanMs, out float elapsedMs)
    {
        m_iSpanCalls++;
        m_Order.Insert("SPAN");
        m_iLastSpanBudget = maxSpanMs;
        m_bOriginalStamps = m_Stamps.Count() >= 2 && start == m_Stamps[m_Stamps.Count() - 2] && end == m_Stamps[m_Stamps.Count() - 1];
        if (m_bUseBaseSpan)
        {
            return super.MeasureSpan(start, end, maxSpanMs, elapsedMs);
        }
        elapsedMs = m_fElapsed;
        return m_sSpanCode;
    }
    int TotalCallbacks()
    {
        return m_iStampCalls + m_iEndpointCalls + m_iHeightGets + m_iTraceCalls + m_iSpanCalls + m_Heights.m_iCalls;
    }
}

//! No output witness roots kept by the provider; only the sampler may retain them across calls.
class RICON_PhysicalObservationFixtureTransientSource : RICON_PhysicalObservationFixtureSource
{
    override RICON_PhysicalObservationStamp ReadStamp()
    {
        ref RICON_PhysicalObservationStamp stamp = super.ReadStamp();
        m_Stamps.Clear();
        return stamp;
    }
    override RICON_PhysicalEndpointWitness ReadEndpoint(RICON_PhysicalEndpointRequest endpoint)
    {
        ref RICON_PhysicalEndpointWitness witness = super.ReadEndpoint(endpoint);
        m_Endpoints.Clear();
        m_ProviderRequests.Clear();
        return witness;
    }
    override RICON_HeightSource GetHeightSource()
    {
        m_iHeightGets++;
        m_Order.Insert("HEIGHT_SOURCE");
        ref RICON_PhysicalObservationFixtureHeights heights = new RICON_PhysicalObservationFixtureHeights();
        heights.m_Order = m_Order;
        return heights;
    }
    override RICON_PhysicalTraceWitness ReadFirstContact(vector tx, vector rx, int flags, int layerMask)
    {
        ref RICON_PhysicalTraceWitness trace = super.ReadFirstContact(tx, rx, flags, layerMask);
        m_Trace = null;
        return trace;
    }
    override string MeasureSpan(RICON_PhysicalObservationStamp start, RICON_PhysicalObservationStamp end, int maxSpanMs, out float elapsedMs)
    {
        m_iSpanCalls++;
        m_iLastSpanBudget = maxSpanMs;
        elapsedMs = 1;
        // Independent trusted DOUBLE clock. No native timestamp or world clock is synthesized.
        return "OK";
    }
}

class RICON_PhysicalObservationFixtures
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
            PrintFormat("[RICON_PHYSICAL_OBSERVATION] failed check=%1 case=%2", m_iChecks, label);
        }
        return ok;
    }
    protected static RICON_PhysicalObservationLimits Limits()
    {
        RICON_PhysicalObservationLimits l = new RICON_PhysicalObservationLimits();
        l.m_iMaxHeightSamples = 3;
        l.m_iMaxTraces = 1;
        l.m_iMaxSpanMs = 2;
        return l;
    }
    protected static RICON_PhysicalEndpointRequest Endpoint(int device, vector pos, string source)
    {
        RICON_PhysicalEndpointRequest e = new RICON_PhysicalEndpointRequest();
        e.m_iDeviceId = device;
        e.m_iTransceiverIndex = 0;
        e.m_vAntennaPos = pos;
        e.m_sAntennaSourceId = source;
        return e;
    }
    protected static RICON_PhysicalObservationRequest Request(bool terrain = true, bool trace = true)
    {
        RICON_PhysicalObservationRequest r = new RICON_PhysicalObservationRequest();
        r.m_Tx = Endpoint(101, Vector(0, 2, 0), "caller_tx");
        r.m_Rx = Endpoint(202, Vector(10, 2, 0), "caller_rx");
        r.m_sExpectedSourceId = "fixture_source";
        r.m_sTimebaseId = "TEST_CLOCK";
        r.m_iExpectedSourceRevision = 5;
        r.m_bSampleTerrain = terrain;
        r.m_bSampleFirstContact = trace;
        r.m_iTraceFlags = TraceFlags.ENTS | TraceFlags.WORLD | TraceFlags.OCEAN;
        r.m_iTraceLayerMask = 1;
        if (terrain)
        {
            r.m_TerrainConfig = new RICON_TerrainDiffractionConfig();
            r.m_TerrainConfig.m_fEarthRadiusKm = 6371;
            r.m_TerrainConfig.m_fKFactor = 4.0 / 3.0;
            r.m_TerrainConfig.m_fPermittivity = 15;
            r.m_TerrainConfig.m_fConductivitySm = 0.005;
            r.m_TerrainConfig.m_ePolarization = RICON_ETerrainPolarization.VERTICAL;
            r.m_TerrainConfig.m_fMaxProfileStepKm = 0.005;
            r.m_TerrainConfig.m_iMaxProfilePoints = 32;
        }
        return r;
    }
    protected static bool OK(RICON_PhysicalObservationResult r, string label)
    {
        return Check(r && r.m_sCode == "OK" && r.m_Frame != null, label);
    }
    protected static void Failure(RICON_PhysicalObservationResult r, string code, string label)
    {
        if (!Check(r != null, label + " result"))
        {
            return;
        }
        Check(r.m_sCode == code, label + " code");
        Check(!r.m_Frame, label + " no frame prefix");
    }
    protected static void NoPhysicalOverclaim(RICON_PhysicalObservationFrame f, string label)
    {
        Check(!f.m_bAtomicSnapshot && !f.m_bCompletePhysicalPaths, label + " not atomic/full multipath");
        Check(f.m_sClockQualification == "TEST_ONLY_SPAN_OBSERVED_PHYSICAL_CLOCK_UNQUALIFIED", label + " clock qualification");
        Check(f.m_aMissingReasons.Count() > 0, label + " missing physical inputs explicit");
        for (int i = 1; i < 5; i++)
        {
            Check(f.m_aCoverage[i] != RICON_EPathCoverage.COMPLETE, label + " family not complete " + i);
        }
    }
    protected static void FunctionalPositive()
    {
        RICON_PhysicalObservationRequest req = Request();
        RICON_PhysicalObservationFixtureSource s = new RICON_PhysicalObservationFixtureSource();
        RICON_PhysicalObservationResult r = RICON_PhysicalObservationSampler.Sample(req, s, Limits());
        if (!OK(r, "O01 functional sampler positive"))
        {
            return;
        }
        Check(r.m_iStampReads == 2 && r.m_iEndpointReads == 4, "O01 stamp/endpoint counters");
        Check(r.m_iHeightCalls == 3 && r.m_iTraceCalls == 1, "O01 height/trace counters");
        Check(s.m_iStampCalls == 2 && s.m_iEndpointCalls == 4, "O01 actual DOUBLE reads");
        Check(s.m_Heights.m_iCalls == 3 && s.m_iTraceCalls == 1, "O01 actual requested callbacks");
        Check(s.m_iSpanCalls == 1 && s.m_iLastSpanBudget == 2 && s.m_bOriginalStamps, "O01 issuing stamp refs/budget");
        RICON_PhysicalObservationFrame f = r.m_Frame;
        Check(f.m_fElapsedMs == 1, "O01 elapsed diagnostic1ms");
        Check(f.m_Tx.m_Observation.m_iDeviceId == 101 && f.m_Rx.m_Observation.m_iDeviceId == 202, "O01 source identities");
        Check(f.m_Tx.m_Observation.m_iTuningValue == 50000, "O01 tuning remains opaque integer");
        Check(f.m_Tx.m_Observation.m_vAntennaPos == Vector(0, 2, 0), "O10 caller point value");
        Check(f.m_Tx.m_sAntennaSourceId == "caller_tx", "O10 caller position provenance");
        Check(!f.m_Tx.m_bHasBodyVelocity && f.m_Tx.m_sMotionProvenance == "MOTION_UNAVAILABLE", "O09 missing motion not zero");
        Check(f.m_Tx.m_sBodyIdentityToken.IsEmpty(), "O09 missing body explicit");
        if (Check(f.m_Terrain != null, "O01 terrain profile"))
        {
            Check(f.m_Terrain.m_aDistanceKm.Count() == 3 && f.m_Terrain.m_aHeightM.Count() == 3, "O01 exact height grid3");
            Check(f.m_Terrain.m_aDistanceKm[0] == 0 && f.m_Terrain.m_aDistanceKm[2] == 0.01, "O01 profile kilometres");
            Check(f.m_fTxHeightAglM == 2 && f.m_fRxHeightAglM == 2, "O01 antenna AGL arithmetic metres");
        }
        Check(f.m_aCoverage[0] == RICON_EPathCoverage.COMPLETE, "O01 only sampled terrain complete");
        Check(f.m_FirstContact && !f.m_FirstContact.m_bHasContact && f.m_FirstContact.m_fFraction == 1, "O11 direct no-hit fact");
        NoPhysicalOverclaim(f, "O01/O11");
        string order;
        foreach (string call : s.m_Order) { order += call + "|"; }
        Check(order == "STAMP|ENDPOINT|ENDPOINT|HEIGHT_SOURCE|HEIGHT|HEIGHT|HEIGHT|TRACE|ENDPOINT|ENDPOINT|STAMP|SPAN|", "O01 callback order");
        for (int contact = 0; contact < 2; contact++)
        {
            s = new RICON_PhysicalObservationFixtureSource();
            s.m_bContact = true;
            if (contact == 1) { s.m_fFraction = 1; }
            r = RICON_PhysicalObservationSampler.Sample(Request(false), s, Limits());
            if (!OK(r, "O12 contact observed"))
            {
                return;
            }
            Check(r.m_Frame.m_FirstContact.m_bHasContact, "O12 fraction1 may still contact");
            Check(r.m_Frame.m_FirstContact.m_sMaterial == "raw_material_hint", "O12 raw material is a hint");
            NoPhysicalOverclaim(r.m_Frame, "O12 contact not wall/multipath coverage");
        }
        s = new RICON_PhysicalObservationFixtureSource();
        r = RICON_PhysicalObservationSampler.Sample(Request(false, false), s, Limits());
        if (OK(r, "O01 endpoint-only positive"))
        {
            Check(!r.m_Frame.m_Terrain && !r.m_Frame.m_FirstContact, "O01 no invented optional readings");
            Check(s.m_iHeightGets == 0 && s.m_iTraceCalls == 0, "O01 optional callbacks skipped");
            NoPhysicalOverclaim(r.m_Frame, "O01 endpoint-only");
        }
    }
    protected static void PreflightZeroCallbacks()
    {
        for (int k = 0; k < 19; k++)
        {
            RICON_PhysicalObservationLimits l = Limits();
            RICON_PhysicalObservationRequest q = Request();
            RICON_PhysicalObservationFixtureSource s = new RICON_PhysicalObservationFixtureSource();
            string code = "INVALID_REQUEST";
            if (k == 0) { l.m_iMaxHeightSamples = 0; code = "INVALID_LIMITS"; }
            if (k == 1) { l.m_iMaxTraces = 2; code = "INVALID_LIMITS"; }
            if (k == 2) { l.m_iMaxSpanMs = 60001; code = "INVALID_LIMITS"; }
            if (k == 3) { q = null; }
            if (k == 4) { q.m_Tx = null; }
            if (k == 5) { q.m_Rx.m_iDeviceId = 101; }
            if (k == 6) { q.m_Rx.m_vAntennaPos = q.m_Tx.m_vAntennaPos; }
            if (k == 7) { q.m_Tx.m_iDeviceId = int.MAX; }
            if (k == 8) { q.m_Rx.m_iTransceiverIndex = -1; }
            if (k == 9) { q.m_Tx.m_sAntennaSourceId = ""; }
            if (k == 10) { q.m_iExpectedSourceRevision = 0; }
            if (k == 11) { q.m_sExpectedSourceId = ""; }
            if (k == 12) { q.m_sTimebaseId = ""; }
            if (k == 13) { q.m_iTraceFlags = 0; }
            if (k == 14) { q.m_iTraceFlags = 1073741824; }
            if (k == 15) { q.m_iTraceLayerMask = 0; }
            if (k == 16) { q.m_TerrainConfig.m_fMaxProfileStepKm = 0; }
            if (k == 17) { l.m_iMaxTraces = 0; code = "RESOURCE_LIMIT"; }
            if (k == 18) { q.m_TerrainConfig.m_fMaxProfileStepKm = 0.002; code = "RESOURCE_LIMIT"; }
            RICON_PhysicalObservationResult r = RICON_PhysicalObservationSampler.Sample(q, s, l);
            Failure(r, code, "O02/O03 preflight " + k);
            Check(s.TotalCallbacks() == 0, "O03 zero provider calls " + k);
        }
        RICON_PhysicalObservationFixtureSource near = new RICON_PhysicalObservationFixtureSource();
        RICON_PhysicalObservationRequest shortRay = Request();
        shortRay.m_Rx.m_vAntennaPos = Vector(1, 2, 0);
        Failure(RICON_PhysicalObservationSampler.Sample(shortRay, near, Limits()), "TERRAIN_UNAVAILABLE", "O13 short terrain ray");
        Check(near.TotalCallbacks() == 0, "O13 short ray no getters");
    }
    protected static void DriftFailuresAndAtomicResults()
    {
        for (int drift = 1; drift <= 17; drift++)
        {
            RICON_PhysicalObservationFixtureSource s = new RICON_PhysicalObservationFixtureSource();
            s.m_iDriftKind = drift;
            RICON_PhysicalObservationResult r = RICON_PhysicalObservationSampler.Sample(Request(), s, Limits());
            Check(r && r.m_sCode != "OK" && !r.m_Frame, "O06/O07/O08 drift refuses mixed frame " + drift);
        }
        for (int late = 0; late < 6; late++)
        {
            RICON_PhysicalObservationFixtureSource s = new RICON_PhysicalObservationFixtureSource();
            string code;
            if (late == 0) { s.m_bStampUnavailable = true; code = "SOURCE_UNAVAILABLE"; }
            if (late == 1) { s.m_iEndpointFailAt = 3; code = "ENDPOINT_UNAVAILABLE"; }
            if (late == 2) { s.m_bHeightUnavailable = true; code = "TERRAIN_UNAVAILABLE"; }
            if (late == 3) { s.m_Heights.m_iFailAt = 2; code = "TERRAIN_UNAVAILABLE"; }
            if (late == 4) { s.m_bTraceUnavailable = true; code = "TRACE_UNAVAILABLE"; }
            if (late == 5) { s.m_bLongNetworkKey = true; code = "RESOURCE_LIMIT"; }
            RICON_PhysicalObservationResult r = RICON_PhysicalObservationSampler.Sample(Request(), s, Limits());
            Failure(r, code, "O13 late/source failure " + late);
        }
        RICON_PhysicalObservationFixtureSource nan = new RICON_PhysicalObservationFixtureSource();
        nan.m_Heights.m_iNanAt = 2;
        Failure(RICON_PhysicalObservationSampler.Sample(Request(), nan, Limits()), "TERRAIN_UNAVAILABLE", "O13 nonfinite height");
    }
    protected static void SpanAndLargeClock()
    {
        for (int k = 0; k < 9; k++)
        {
            RICON_PhysicalObservationFixtureSource s = new RICON_PhysicalObservationFixtureSource();
            RICON_PhysicalObservationLimits l = Limits();
            string expected = "OK";
            if (k == 0) { l.m_iMaxSpanMs = 0; s.m_fElapsed = 0; }
            if (k == 1) { l.m_iMaxSpanMs = 2; s.m_fElapsed = 2; }
            if (k == 2) { s.m_fElapsed = 2.5; s.m_sSpanCode = "SPAN_EXCEEDED"; expected = "SPAN_EXCEEDED"; }
            if (k == 3) { l.m_iMaxSpanMs = 60000; s.m_fElapsed = 60000; }
            if (k == 4) { l.m_iMaxSpanMs = 60000; s.m_fElapsed = 60000.5; s.m_sSpanCode = "SPAN_EXCEEDED"; expected = "SPAN_EXCEEDED"; }
            if (k == 5) { s.m_sSpanCode = "CLOCK_ROLLBACK"; expected = "CLOCK_ROLLBACK"; }
            if (k == 6) { s.m_bUseBaseSpan = true; expected = "TIME_UNQUALIFIED"; }
            if (k == 7) { s.m_fElapsed = 1000; expected = "INVALID_SOURCE_RESULT"; }
            if (k == 8) { s.m_fElapsed = -1; expected = "INVALID_SOURCE_RESULT"; }
            RICON_PhysicalObservationResult r = RICON_PhysicalObservationSampler.Sample(Request(false, false), s, l);
            if (expected == "OK")
            {
                if (OK(r, "O04/O17 exact source span " + k))
                {
                    Check(r.m_Frame.m_fElapsedMs == s.m_fElapsed, "O17 short diagnostic retained " + k);
                    NoPhysicalOverclaim(r.m_Frame, "O17 timestamp bound not atomic/CF proof");
                }
            }
            else
            {
                Failure(r, expected, "O05/O17/O18 span refusal " + k);
            }
            Check(s.m_iLastSpanBudget == l.m_iMaxSpanMs, "O17 exact integer budget passed " + k);
        }
        for (int max = 0; max <= 1; max++)
        {
            RICON_PhysicalObservationFixtureSource s = new RICON_PhysicalObservationFixtureSource();
            s.m_fRawStart = 16777216;
            s.m_fRawEnd = 16777216;
            s.m_fElapsed = 1;
            RICON_PhysicalObservationLimits l = Limits();
            l.m_iMaxSpanMs = max;
            if (max == 0) { s.m_sSpanCode = "SPAN_EXCEEDED"; }
            RICON_PhysicalObservationResult r = RICON_PhysicalObservationSampler.Sample(Request(false, false), s, l);
            if (max == 0)
            {
                Failure(r, "SPAN_EXCEEDED", "O17 raw large-clock float zero cannot pass");
            }
            else if (OK(r, "O17 positive opaque/source span at large clock"))
            {
                Check(r.m_Frame.m_fElapsedMs == 1, "O17 actual declared1ms not raw subtraction0");
                NoPhysicalOverclaim(r.m_Frame, "O17 large timestamp");
            }
        }
    }
    protected static void SnapshotCopiesAndRetention()
    {
        RICON_PhysicalObservationRequest q = Request();
        RICON_PhysicalObservationFixtureSource s = new RICON_PhysicalObservationFixtureSource();
        s.m_CallerToMutate = q;
        s.m_bMutateProviderRequest = true;
        RICON_PhysicalObservationResult r = RICON_PhysicalObservationSampler.Sample(q, s, Limits());
        if (!OK(r, "O14 private request snapshot before callbacks"))
        {
            return;
        }
        Check(r.m_Frame.m_Tx.m_Observation.m_iDeviceId == 101 && r.m_Frame.m_Rx.m_Observation.m_iDeviceId == 202, "O14 original endpoint values retained");
        Check(r.m_Frame.m_Terrain != null && r.m_iHeightCalls == 3, "O14 original options retained");
        Check(s.m_ProviderRequests[0] != q.m_Tx && s.m_ProviderRequests[0] != s.m_ProviderRequests[2], "O14 provider gets private per-call request");
        s.m_Stamps[0].m_sSourceId = "changed_after_return";
        s.m_Endpoints[0].m_Observation.m_iTuningValue = 999;
        s.m_Endpoints[2].m_sIdentityToken = "changed_after_return";
        s.m_Trace.m_fFraction = 0.1;
        Check(r.m_Frame.m_Start.m_sSourceId == "fixture_source", "O14 detached stamp header");
        Check(r.m_Frame.m_Tx.m_Observation.m_iTuningValue == 50000, "O14 detached radio witness");
        Check(r.m_Frame.m_Tx.m_sIdentityToken == "radio_101", "O14 detached before/after source data");
        Check(r.m_Frame.m_FirstContact.m_fFraction == 1, "O14 detached trace witness");
        r.m_Frame.m_Tx.m_Observation.m_iTuningValue = 123;
        s = new RICON_PhysicalObservationFixtureSource();
        RICON_PhysicalObservationResult again = RICON_PhysicalObservationSampler.Sample(Request(false, false), s, Limits());
        if (OK(again, "O15 repeated independent source sample"))
        {
            Check(again.m_Frame.m_Tx.m_Observation.m_iTuningValue == 50000, "O15 no retained mutable result/cache seed");
            NoPhysicalOverclaim(again.m_Frame, "O15 no process synthesis");
        }
        q = Request(false, false);
        q.m_Rx.m_iDeviceId = 101;
        q.m_Rx.m_iTransceiverIndex = 1;
        s = new RICON_PhysicalObservationFixtureSource();
        r = RICON_PhysicalObservationSampler.Sample(q, s, Limits());
        Check(r && r.m_sCode == "OK", "O01 distinct physical TRX keys may share device, no delivery claim");
    }
    protected static void KnownZeroBodyAndTransientOwnership()
    {
        RICON_PhysicalObservationFixtureSource known = new RICON_PhysicalObservationFixtureSource();
        known.m_bKnownZeroBody = true;
        RICON_PhysicalObservationResult r = RICON_PhysicalObservationSampler.Sample(Request(), known, Limits());
        if (OK(r, "O19 stable known-zero body velocity"))
        {
            Check(r.m_Frame.m_Tx.m_bHasBodyVelocity && r.m_Frame.m_Rx.m_bHasBodyVelocity, "O19 known differs from unavailable");
            Check(r.m_Frame.m_Tx.m_vBodyVelocityAtPoint == Vector(0, 0, 0), "O19 measured zero value retained");
            Check(r.m_Frame.m_Tx.m_sMotionProvenance == "BODY_AT_CALLER_POINT", "O19 not certified antenna motion");
            Check(!r.m_Frame.m_Tx.m_sBodyIdentityToken.IsEmpty(), "O19 body witness supplied");
            NoPhysicalOverclaim(r.m_Frame, "O19 known body zero not full static medium");
        }
        RICON_PhysicalObservationFixtureTransientSource transient = new RICON_PhysicalObservationFixtureTransientSource();
        r = RICON_PhysicalObservationSampler.Sample(Request(), transient, Limits());
        if (!OK(r, "O20 transient unretained provider witnesses"))
        {
            return;
        }
        Check(transient.m_Stamps.Count() == 0 && transient.m_Endpoints.Count() == 0, "O20 provider keeps no stamp/endpoint roots");
        Check(transient.m_ProviderRequests.Count() == 0 && !transient.m_Trace, "O20 provider keeps no request/trace roots");
        Check(r.m_iStampReads == 2 && r.m_iEndpointReads == 4, "O20 lifetime through all callbacks");
        Check(r.m_iHeightCalls == 3 && r.m_iTraceCalls == 1, "O20 transient height provider alive");
        Check(r.m_Frame.m_Tx.m_Observation.m_iDeviceId == 101 && r.m_Frame.m_Rx.m_Observation.m_iDeviceId == 202, "O20 detached frame witnesses survive");
        Check(r.m_Frame.m_Terrain != null && r.m_Frame.m_FirstContact != null, "O20 copied profile/contact survive");
        NoPhysicalOverclaim(r.m_Frame, "O20 not native lifetime/world proof");
    }
    static bool RunAll()
    {
        m_iChecks = 0;
        m_iFailures = 0;
        m_iFirstFailure = 0;
        m_sFirstFailure = "";
        FunctionalPositive();
        PreflightZeroCallbacks();
        DriftFailuresAndAtomicResults();
        SpanAndLargeClock();
        SnapshotCopiesAndRetention();
        KnownZeroBodyAndTransientOwnership();
        return m_iFailures == 0;
    }
}
