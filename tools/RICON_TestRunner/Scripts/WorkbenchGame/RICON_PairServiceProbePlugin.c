//! TOOL_ONLY: S3 исполняется без мира, сети, Play и сервера; рации заданы фикстурами.
[WorkbenchPluginAttribute(name: "RICON: служба пар", description: "Корпус S3, фактические вызовы и неизменность", wbModules: {"ScriptEditor"})]
class RICON_PairServiceProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		Print("[RICON_PAIR] source=" + RICON_PairServiceFixtures.SOURCE_SHA);
		RICON_PairServiceRun runner = new RICON_PairServiceRun();
		RICON_PairServiceFixtures.Run(runner);
		runner.CheckEmpty();
		PrintFormat("[RICON_PAIR] cases=%1 match=%2 immutable=%3 controls=%4 extras=%5 pairs=%6", runner.m_iCases,
			runner.m_iMatches, runner.m_iImmutable, runner.m_iControls, runner.m_iExtras, runner.m_iPairs);
		int cases = RICON_PairServiceFixtures.CASES;
		if (runner.m_bOk && runner.m_iCases == cases && runner.m_iMatches == cases && runner.m_iImmutable == cases
			&& runner.m_iControls == 15 && runner.m_iExtras == 4)
		{
			Print("[RICON_PAIR] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_PAIR] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
