//! TOOL_ONLY: синтетические высоты через реальный provider и L2, без мира, Play и сервера.
[WorkbenchPluginAttribute(name: "RICON: профиль мира", description: "Геометрия и отказы поставщика профиля", wbModules: {"ScriptEditor"})]
class RICON_WorldProfileProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		Print("[RICON_WORLD] source=" + RICON_WorldProfileFixtures.SOURCE_SHA);
		RICON_WorldProfileRun runner = new RICON_WorldProfileRun();
		runner.CheckNullWorld();
		runner.CheckBudgetChain();
		RICON_WorldProfileFixtures.Run(runner);
		PrintFormat("[RICON_WORLD] cases=%1 quietPositive=%2 controls=%3 endpoints=%4 budget=%5", runner.m_iCases,
			runner.m_iQuietPositive, runner.m_iControls, runner.m_iEndpointChecks, runner.m_iBudgetChecks);
		if (runner.m_bOk && runner.m_iCases == RICON_WorldProfileFixtures.CASES
			&& runner.m_iQuietPositive == RICON_WorldProfileFixtures.CASES && runner.m_iControls == 14
			&& runner.m_iEndpointChecks == 1 && runner.m_iBudgetChecks == 1)
		{
			Print("[RICON_WORLD] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_WORLD] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
