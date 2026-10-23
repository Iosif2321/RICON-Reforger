//! TOOL_ONLY: P3.2b исполняется без мира, сети, Play и сервера; профили — примеры Table 3 и TEST_ONLY.
[WorkbenchPluginAttribute(name: "RICON: профиль материала P3.2b", description: "Корпус kernel профиля, контроли, цепь к пластине, повтор", wbModules: {"ScriptEditor"})]
class RICON_MaterialProfileProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		Print("[RICON_MPROF] source=" + RICON_MaterialProfileFixtures.SOURCE_SHA);
		RICON_MaterialProfileRun runner = new RICON_MaterialProfileRun();
		RICON_MaterialProfileFixtures.Run(runner);
		runner.CheckExtras();
		float worst = runner.m_fMaxRelativeError;
		PrintFormat("[RICON_MPROF] cases=%1 match=%2 immutable=%3 controls=%4 extras=%5 hostOnly=%6 chains=%7 maxRelativeError=%8", runner.m_iCases,
			runner.m_iMatches, runner.m_iImmutable, runner.m_iControls, runner.m_iExtras, RICON_MaterialProfileFixtures.HOST_ONLY,
			runner.m_iChains, worst.ToString(-1, 9));
		int cases = RICON_MaterialProfileFixtures.CASES;
		// extras = 6: null/восстановление, группа цепей (все OK-случаи с σ > 0), обратный повтор, 3 контрпримера Power.
		if (runner.m_bOk && runner.m_iCases == cases && runner.m_iMatches == cases && runner.m_iImmutable == cases
			&& runner.m_iControls == 5 && runner.m_iExtras == 6)
		{
			Print("[RICON_MPROF] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_MPROF] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
