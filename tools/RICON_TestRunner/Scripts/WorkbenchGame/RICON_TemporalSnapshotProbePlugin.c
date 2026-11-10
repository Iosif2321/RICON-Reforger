//! TOOL_ONLY: CT10 schema2 pure fixtures. No Play, world, server or voice actuator.
//! Source candidate: Enforce compile/execution NOT_RUN.
//! Future command: -wbModule=ScriptEditor -plugin=RICON_TemporalSnapshotProbePlugin
[WorkbenchPluginAttribute(name: "RICON: temporal snapshots", description: "Schema2 admission, copies, receiver and clock fixtures", wbModules: {"ScriptEditor"})]
class RICON_TemporalSnapshotProbePlugin : WorkbenchPlugin
{
    override void RunCommandline()
    {
        bool ok = RICON_TemporalSnapshotFixtures.RunAll();
        ok = ok && RICON_TemporalSnapshotFixtures.m_iChecks > 0;
        PrintFormat("[RICON_TEMPORAL] checks=%1 failures=%2 first_failure=%3",
            RICON_TemporalSnapshotFixtures.m_iChecks,
            RICON_TemporalSnapshotFixtures.m_iFailures,
            RICON_TemporalSnapshotFixtures.m_iFirstFailure);
        if (ok)
        {
            Print("[RICON_TEMPORAL] done: PASS");
            Workbench.Exit(0);
        }
        else
        {
            Print("[RICON_TEMPORAL] done: FAIL");
            Workbench.Exit(5);
        }
    }
}
