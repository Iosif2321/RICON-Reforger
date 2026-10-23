//! TOOL_ONLY: P3.2a исполняется без мира, сети, Play и сервера; все материалы — TEST_ONLY.
[WorkbenchPluginAttribute(name: "RICON: пластина P3.2a", description: "Корпус kernel пластины, контроли, TE=TM, повтор", wbModules: {"ScriptEditor"})]
class RICON_MaterialSlabProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		Print("[RICON_SLAB] source=" + RICON_MaterialSlabFixtures.SOURCE_SHA);
		RICON_MaterialSlabRun runner = new RICON_MaterialSlabRun();
		RICON_MaterialSlabFixtures.Run(runner);
		runner.CheckExtras();
		float deviation = runner.m_fMaxDeviationDb;
		PrintFormat("[RICON_SLAB] cases=%1 match=%2 immutable=%3 controls=%4 extras=%5 hostOnly=%6 maxDeviationDb=%7", runner.m_iCases,
			runner.m_iMatches, runner.m_iImmutable, runner.m_iControls, runner.m_iExtras, RICON_MaterialSlabFixtures.HOST_ONLY,
			deviation.ToString(-1, 9));
		int cases = RICON_MaterialSlabFixtures.CASES;
		if (runner.m_bOk && runner.m_iCases == cases && runner.m_iMatches == cases && runner.m_iImmutable == cases
			&& runner.m_iControls == 5 && runner.m_iExtras == 3)
		{
			Print("[RICON_SLAB] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_SLAB] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
