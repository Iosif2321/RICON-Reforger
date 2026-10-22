//! TOOL_ONLY: P2 без мира, Play, сети и сервера.
[WorkbenchPluginAttribute(name: "RICON: путь и вода", description: "Frozen P2, неизменность, контроли и границы", wbModules: {"ScriptEditor"})]
class RICON_PathObservationProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		Print("[RICON_PATH] source=" + RICON_PathObservationFixtures.SOURCE_SHA);
		RICON_PathObservationRun runner = new RICON_PathObservationRun();
		RICON_PathObservationFixtures.Run(runner);
		runner.CheckBounds();
		PrintFormat("[RICON_PATH] cases=%1 match=%2 immutable=%3 controls=%4 extras=%5 hostOnly=%6", runner.m_iCases,
			runner.m_iMatches, runner.m_iImmutable, runner.m_iControls, runner.m_iExtras, RICON_PathObservationFixtures.HOST_ONLY);
		int cases = RICON_PathObservationFixtures.CASES;
		if (runner.m_bOk && runner.m_iCases == cases && runner.m_iMatches == cases && runner.m_iImmutable == cases
			&& runner.m_iControls == 11 && runner.m_iExtras == 8)
		{
			Print("[RICON_PATH] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_PATH] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
