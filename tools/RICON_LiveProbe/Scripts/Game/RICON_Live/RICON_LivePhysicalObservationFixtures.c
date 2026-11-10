//! TEST_ONLY shared pure manual-probe guard/formatter tests. Never calls SampleNamedOwners/native/world.
//! W01-W12 frozen before helper/plugin implementation review; no fake scene or radio instances.
class RICON_LivePhysicalObservationFixtures
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
            PrintFormat("[RICON_LIVE_PHYSICAL_PURE] failed check=%1 case=%2", m_iChecks, label);
        }
        return ok;
    }
    protected static RICON_PhysicalWorldProbeRequest Request()
    {
        RICON_PhysicalWorldProbeRequest r = new RICON_PhysicalWorldProbeRequest();
        r.m_sEnable = "1";
        r.m_sRunId = "PhysicalObservation.1";
        r.m_sTxOwnerName = "EXISTING_RADIO_A";
        r.m_sRxOwnerName = "EXISTING_RADIO_B";
        return r;
    }
    protected static string LongText(int n)
    {
        string s;
        for (int i = 0; i < n; i++) { s += "A"; }
        return s;
    }
    protected static RICON_PhysicalObservationStamp Stamp()
    {
        RICON_PhysicalObservationStamp s = new RICON_PhysicalObservationStamp();
        s.m_sSourceId = "TEST_FORMATTER";
        s.m_sWorldIdentity = "TEST_WORLD_NOT_NATIVE";
        s.m_sTimebaseId = "TEST_CLOCK";
        s.m_sProvenance = "TEST_ONLY";
        s.m_iSourceRevision = 1;
        s.m_fRawWorldTimeMs = 100;
        return s;
    }
    protected static RICON_PhysicalEndpointWitness Endpoint(int device)
    {
        RICON_PhysicalEndpointWitness w = new RICON_PhysicalEndpointWitness();
        w.m_Observation = new RICON_RadioObservation();
        w.m_Observation.m_iDeviceId = device;
        w.m_Observation.m_iTransceiverIndex = 0;
        w.m_Observation.m_bPowered = true;
        w.m_Observation.m_iTuningValue = 50000;
        w.m_Observation.m_sNetworkKey = "FIXTURE_SECRET_NETWORK_KEY_DO_NOT_PRINT";
        w.m_Observation.m_vAntennaPos = Vector(0, 2, 0);
        if (device == 202) { w.m_Observation.m_vAntennaPos = Vector(10, 2, 0); }
        w.m_sIdentityToken = "test_radio_" + device.ToString();
        w.m_sAntennaSourceId = "CALLER_ENTITY_ORIGIN";
        w.m_sMotionProvenance = "MOTION_UNAVAILABLE";
        w.m_vOwnerAxis0 = Vector(1, 0, 0);
        w.m_vOwnerAxis1 = Vector(0, 1, 0);
        w.m_vOwnerAxis2 = Vector(0, 0, 1);
        return w;
    }
    //! Literal formatter input, not result produced by future code/native Sample.
    protected static RICON_PhysicalObservationResult Result()
    {
        RICON_PhysicalObservationResult r = new RICON_PhysicalObservationResult();
        r.m_sCode = "OK";
        r.m_iStampReads = 2;
        r.m_iEndpointReads = 4;
        r.m_Frame = new RICON_PhysicalObservationFrame();
        r.m_Frame.m_Start = Stamp();
        r.m_Frame.m_End = Stamp();
        r.m_Frame.m_Tx = Endpoint(101);
        r.m_Frame.m_Rx = Endpoint(202);
        r.m_Frame.m_sClockQualification = "TEST_ONLY_SPAN_OBSERVED_PHYSICAL_CLOCK_UNQUALIFIED";
        r.m_Frame.m_aMissingReasons.Insert("FULL_PATH_SET_NOT_OBSERVED");
        for (int i = 0; i < 5; i++) { r.m_Frame.m_aCoverage.Insert(RICON_EPathCoverage.NOT_OBSERVED); }
        return r;
    }
    protected static bool Contains(array<string> lines, string text)
    {
        foreach (string line : lines)
        {
            if (line.Contains(text))
            {
                return true;
            }
        }
        return false;
    }
    protected static void SafeLines(array<string> lines, string label)
    {
        if (!Check(lines != null, label + " array"))
        {
            return;
        }
        Check(lines.Count() > 0 && lines.Count() <= 12, label + " bounded lines");
        int terminal;
        foreach (string line : lines)
        {
            Check(!line.Contains("\n") && !line.Contains("\r"), label + " no raw line injection");
            Check(!line.Contains("FIXTURE_SECRET_NETWORK_KEY_DO_NOT_PRINT"), label + " no network key");
            if (line.Contains("terminal=1")) { terminal++; }
        }
        Check(terminal == 1, label + " exactly one terminal outcome");
    }
    protected static void RequestAdmission()
    {
        Check(RICON_LivePhysicalObservationProbe.ValidateRequest(Request()) == "OK", "W01 positive config only");
        Check(RICON_LivePhysicalObservationProbe.ValidateRequest(null) == "INVALID_REQUEST", "W02 null request");
        array<string> disabled = {"", "0", "true", "01", "yes", "1junk"};
        foreach (string enable : disabled)
        {
            RICON_PhysicalWorldProbeRequest r = Request();
            r.m_sEnable = enable;
            Check(RICON_LivePhysicalObservationProbe.ValidateRequest(r) == "PROBE_DISABLED", "W01 strict optin " + enable);
        }
        for (int k = 0; k < 12; k++)
        {
            RICON_PhysicalWorldProbeRequest r = Request();
            if (k == 0) { r.m_sRunId = ""; }
            if (k == 1) { r.m_sRunId = LongText(65); }
            if (k == 2) { r.m_sRunId = "run\nspoof"; }
            if (k == 3) { r.m_sTxOwnerName = ""; }
            if (k == 4) { r.m_sRxOwnerName = LongText(65); }
            if (k == 5) { r.m_sTxOwnerName = " spaced_name"; }
            if (k == 6) { r.m_sRxOwnerName = "name\rspoof"; }
            if (k == 7) { r.m_sTxOwnerName = "{GUID}Prefabs/Radio.et"; }
            if (k == 8) { r.m_sRxOwnerName = r.m_sTxOwnerName; }
            if (k == 9) { r.m_sRunId = "run=forged"; }
            if (k == 10) { r.m_sTxOwnerName = "A;Run()"; }
            if (k == 11) { r.m_sRxOwnerName = "name\\path"; }
            Check(RICON_LivePhysicalObservationProbe.ValidateRequest(r) == "INVALID_REQUEST", "W02 bounded exact key variant " + k);
        }
        RICON_PhysicalWorldProbeRequest boundary = Request();
        boundary.m_sRunId = LongText(64);
        boundary.m_sTxOwnerName = "A";
        boundary.m_sRxOwnerName = LongText(64);
        Check(RICON_LivePhysicalObservationProbe.ValidateRequest(boundary) == "OK", "W02 exact key boundaries");
        Check(RICON_LivePhysicalObservationProbe.MAX_HEIGHT_SAMPLES == 8, "W08 height cap8");
        Check(RICON_LivePhysicalObservationProbe.MAX_TRACES == 1 && RICON_LivePhysicalObservationProbe.MAX_SPAN_MS == 100, "W08 trace/span caps1/100");
        Check(RICON_LivePhysicalObservationProbe.MAX_RECEIPT_LINES == 12, "W11 receipt cap12");
    }
    protected static void ContextDistanceAndLatch()
    {
        for (int k = 0; k < 8; k++)
        {
            bool play = (k & 1) != 0;
            bool runtime = (k & 2) != 0;
            bool authority = (k & 4) != 0;
            string expected = "NOT_CURRENT_PLAY";
            if (play && runtime)
            {
                expected = "AUTHORITY_REQUIRED";
                if (authority) { expected = "OK"; }
            }
            Check(RICON_LivePhysicalObservationProbe.AdmitContext(play, runtime, authority) == expected, "W04 actual shared context gate " + k);
        }
        vector tx = Vector(0, 0, 0);
        Check(!RICON_LivePhysicalObservationProbe.InDistanceDomain(tx, Vector(1.99, 0, 0)), "W08 below2 horizontal");
        Check(RICON_LivePhysicalObservationProbe.InDistanceDomain(tx, Vector(2, 0, 0)), "W08 exact2 inclusive");
        Check(RICON_LivePhysicalObservationProbe.InDistanceDomain(tx, Vector(100, 0, 0)), "W08 exact100 inclusive");
        Check(!RICON_LivePhysicalObservationProbe.InDistanceDomain(tx, Vector(100.01, 0, 0)), "W08 above100 horizontal");
        Check(RICON_LivePhysicalObservationProbe.InDistanceDomain(tx, Vector(2, 1000, 0)), "W08 horizontal not3D domain");
        Check(!RICON_LivePhysicalObservationProbe.InDistanceDomain(tx, tx), "W08 coincident");
        Check(!RICON_LivePhysicalObservationProbe.InDistanceDomain(tx, Vector(RICON_L2Run.NaN(), 0, 0)), "W08 nonfinite point");
        RICON_PhysicalWorldProbeLatch latch = new RICON_PhysicalWorldProbeLatch();
        Check(latch.TakeAttempt(), "W09 first attempt consumed");
        Check(!latch.TakeAttempt() && !latch.TakeAttempt(), "W09 refusal/success cannot silently rearm");
    }
    protected static void ReceiptShapesAndNoSecrets()
    {
        array<string> lines = RICON_LivePhysicalObservationProbe.FormatReceipt("PhysicalObservation.1", Result());
        SafeLines(lines, "W11 declared TEST_ONLY result");
        Check(Contains(lines, "status=OBSERVATION_OK") && Contains(lines, "code=OK"), "W10 only valid result positive");
        Check(Contains(lines, "TEST_ONLY") && Contains(lines, "PHYSICAL_CLOCK_UNQUALIFIED"), "W11 source qualifiers not forced native");
        Check(Contains(lines, "test_radio_101") && Contains(lines, "test_radio_202"), "W13 both actual local radio identity witnesses present");
        RICON_PhysicalObservationResult header = Result();
        header.m_Frame.m_Start.m_sSourceId = "HEADER_SOURCE_SENTINEL";
        header.m_Frame.m_Start.m_iSourceRevision = 17;
        array<string> headerLines = RICON_LivePhysicalObservationProbe.FormatReceipt("different_run", header);
        Check(Contains(headerLines, "sourceId=HEADER_SOURCE_SENTINEL") && Contains(headerLines, "sourceRegistrationRevision=17"), "W13 actual source header not runId/default");
        lines = RICON_LivePhysicalObservationProbe.FormatReceipt("run", null);
        SafeLines(lines, "W10 null result");
        Check(lines.Count() == 1 && Contains(lines, "INVALID_RESULT") && Contains(lines, "status=REFUSED"), "W10 invalid result refusal");
        for (int k = 0; k < 10; k++)
        {
            RICON_PhysicalObservationResult r = Result();
            if (k == 0) { r.m_Frame = null; }
            if (k == 1) { r.m_Frame.m_Start = null; }
            if (k == 2) { r.m_Frame.m_End = null; }
            if (k == 3) { r.m_Frame.m_Tx = null; }
            if (k == 4) { r.m_Frame.m_Rx.m_Observation = null; }
            if (k == 5) { r.m_Frame.m_aCoverage.Remove(0); }
            if (k == 6) { for (int i = 0; i < 17; i++) { r.m_Frame.m_aMissingReasons.Insert("GAP"); } }
            if (k == 7) { r.m_Frame.m_sClockQualification = ""; }
            if (k == 8) { r.m_Frame.m_bAtomicSnapshot = true; }
            if (k == 9) { r.m_Frame.m_bCompletePhysicalPaths = true; }
            lines = RICON_LivePhysicalObservationProbe.FormatReceipt("run", r);
            SafeLines(lines, "W10 malformed positive " + k);
            Check(lines.Count() == 1 && Contains(lines, "INVALID_RESULT") && !Contains(lines, "OBSERVATION_OK"), "W10 no fake positive " + k);
        }
        RICON_PhysicalObservationResult refusal = new RICON_PhysicalObservationResult();
        refusal.m_sCode = "SOURCE_UNAVAILABLE";
        lines = RICON_LivePhysicalObservationProbe.FormatReceipt("run", refusal);
        SafeLines(lines, "W10 native refusal with noFrame");
        Check(lines.Count() == 1 && Contains(lines, "SOURCE_UNAVAILABLE") && Contains(lines, "status=REFUSED"), "W10 refusal code preserved");
        RICON_PhysicalObservationResult noisy = Result();
        noisy.m_Frame.m_Start.m_sWorldIdentity = LongText(300) + "\nworld_spoof";
        noisy.m_Frame.m_Tx.m_sIdentityToken = "entity\r\nspoof";
        lines = RICON_LivePhysicalObservationProbe.FormatReceipt("run\nspoof", noisy);
        SafeLines(lines, "W11 escaped clipped fields");
        Check(Contains(lines, "TRUNCATED"), "W11 clipping is explicit");
        Check(Contains(lines, "entity%0D%0Aspoof"), "W13 identity CRLF escaped rather than silently omitted");
        lines = RICON_LivePhysicalObservationProbe.Refusal("run\nspoof", "reason\r\nspoof");
        SafeLines(lines, "W11 refusal escaping");
        Check(lines.Count() == 1 && Contains(lines, "status=REFUSED"), "W11 one refusal terminal");
    }
    static bool RunAll()
    {
        m_iChecks = 0;
        m_iFailures = 0;
        m_iFirstFailure = 0;
        m_sFirstFailure = "";
        RequestAdmission();
        ContextDistanceAndLatch();
        ReceiptShapesAndNoSecrets();
        return m_iFailures == 0;
    }
}
