//! TOOL_ONLY: explicit pure integration fixture runner. No Play/world/server or voice capture.
//! Runs only through an explicit ScriptEditor invocation; no autorun.
[WorkbenchPluginAttribute(name: "RICON: temporal integration", description: "Ordered lifecycle and authority capture fixtures", wbModules: {"ScriptEditor"})]
class RICON_TemporalIntegrationProbePlugin : WorkbenchPlugin
{
    override void RunCommandline()
    {
        bool snapshot = RICON_TemporalSnapshotFixtures.RunAll();
        bool ordered = RICON_TemporalLifecycleFixtures.RunAll();
        bool capture = RICON_TemporalAuthorityFixtures.RunCaptureChecks();
        bool leaseQuery = RICON_LifecycleLeaseQueryFixtures.RunAll();
        bool batch = RICON_TemporalCaptureBatchFixtures.RunAll();
        bool producer = RICON_TemporalProducerFixtures.RunAll();
        bool observation = RICON_PhysicalObservationFixtures.RunAll();
        PrintFormat("[RICON_TEMPORAL_INTEGRATION] snapshot checks=%1 failures=%2 first=%3",
            RICON_TemporalSnapshotFixtures.m_iChecks, RICON_TemporalSnapshotFixtures.m_iFailures,
            RICON_TemporalSnapshotFixtures.m_iFirstFailure);
        PrintFormat("[RICON_TEMPORAL_INTEGRATION] ordered checks=%1 failures=%2 first=%3",
            RICON_TemporalLifecycleFixtures.m_iChecks, RICON_TemporalLifecycleFixtures.m_iFailures,
            RICON_TemporalLifecycleFixtures.m_iFirstFailure);
        PrintFormat("[RICON_TEMPORAL_INTEGRATION] capture checks=%1 failures=%2 first=%3",
            RICON_TemporalAuthorityFixtures.s_iChecks, RICON_TemporalAuthorityFixtures.s_iFailures,
            RICON_TemporalAuthorityFixtures.s_iFirstFailure);
        PrintFormat("[RICON_TEMPORAL_INTEGRATION] lease_query checks=%1 failures=%2 first=%3",
            RICON_LifecycleLeaseQueryFixtures.m_iChecks, RICON_LifecycleLeaseQueryFixtures.m_iFailures,
            RICON_LifecycleLeaseQueryFixtures.m_iFirstFailure);
        PrintFormat("[RICON_TEMPORAL_INTEGRATION] capture_batch checks=%1 failures=%2 first=%3",
            RICON_TemporalCaptureBatchFixtures.m_iChecks, RICON_TemporalCaptureBatchFixtures.m_iFailures,
            RICON_TemporalCaptureBatchFixtures.m_iFirstFailure);
        PrintFormat("[RICON_TEMPORAL_INTEGRATION] producer checks=%1 failures=%2 first=%3 label=%4",
            RICON_TemporalProducerFixtures.m_iChecks, RICON_TemporalProducerFixtures.m_iFailures,
            RICON_TemporalProducerFixtures.m_iFirstFailure, RICON_TemporalProducerFixtures.m_sFirstFailure);
        PrintFormat("[RICON_TEMPORAL_INTEGRATION] observation checks=%1 failures=%2 first=%3 label=%4",
            RICON_PhysicalObservationFixtures.m_iChecks, RICON_PhysicalObservationFixtures.m_iFailures,
            RICON_PhysicalObservationFixtures.m_iFirstFailure, RICON_PhysicalObservationFixtures.m_sFirstFailure);
        bool passed = snapshot && ordered && capture && leaseQuery && batch && producer && observation;
        if (RICON_PhysicalObservationFixtures.m_iChecks <= 0) passed = false;
        if (RICON_TemporalProducerFixtures.m_iChecks <= 0) passed = false;
        if (RICON_TemporalSnapshotFixtures.m_iChecks <= 0) passed = false;
        if (RICON_TemporalLifecycleFixtures.m_iChecks <= 0 || RICON_TemporalAuthorityFixtures.s_iChecks <= 0) passed = false;
        if (RICON_LifecycleLeaseQueryFixtures.m_iChecks <= 0 || RICON_TemporalCaptureBatchFixtures.m_iChecks <= 0) passed = false;
        if (passed)
        {
            Print("[RICON_TEMPORAL_INTEGRATION] done: PASS");
            Workbench.Exit(0);
        }
        else
        {
            Print("[RICON_TEMPORAL_INTEGRATION] done: FAIL");
            Workbench.Exit(5);
        }
    }
}
