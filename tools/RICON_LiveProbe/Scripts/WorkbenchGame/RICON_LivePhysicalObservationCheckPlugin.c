//! TOOL_ONLY: explicit ScriptEditor pure guard/receipt fixtures. Never dispatches the native menu probe.
[WorkbenchPluginAttribute(name: "RICON: check physical observation guards", wbModules: {"ScriptEditor"})]
class RICON_LivePhysicalObservationCheckPlugin : WorkbenchPlugin
{
    override void RunCommandline()
    {
        bool passed = RICON_LivePhysicalObservationFixtures.RunAll();
        PrintFormat("[RICON_PHYSICAL_PROBE_CHECK] checks=%1 failures=%2 first=%3 label=%4",
            RICON_LivePhysicalObservationFixtures.m_iChecks, RICON_LivePhysicalObservationFixtures.m_iFailures,
            RICON_LivePhysicalObservationFixtures.m_iFirstFailure, RICON_LivePhysicalObservationFixtures.m_sFirstFailure);
        if (passed && RICON_LivePhysicalObservationFixtures.m_iChecks > 0 && RICON_LivePhysicalObservationFixtures.m_iFailures == 0)
        {
            Print("[RICON_PHYSICAL_PROBE_CHECK] done: PASS");
            Workbench.Exit(0);
        }
        else
        {
            Print("[RICON_PHYSICAL_PROBE_CHECK] done: FAIL");
            Workbench.Exit(5);
        }
    }
}
