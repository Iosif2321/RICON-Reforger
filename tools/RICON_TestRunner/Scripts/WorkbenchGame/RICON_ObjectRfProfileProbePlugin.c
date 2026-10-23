//! TOOL_ONLY: P3.1 исполняется без мира, сети, Play и сервера; профили и графы — TEST_ONLY.
[WorkbenchPluginAttribute(name: "RICON: RF-профиль объекта P3.1", description: "Корпус resolver, порядок записей и графы", wbModules: {"ScriptEditor"})]
class RICON_ObjectRfProfileProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		Print("[RICON_OBJPROFILE] source=" + RICON_ObjectRfProfileFixtures.SOURCE_SHA);
		RICON_ObjectRfProfileRun runner = new RICON_ObjectRfProfileRun();
		RICON_ObjectRfProfileFixtures.Run(runner);
		runner.CheckOrder();
		runner.CheckGraphs();
		PrintFormat("[RICON_OBJPROFILE] cases=%1 match=%2 immutable=%3 controls=%4 extras=%5 hostOnly=%6", runner.m_iCases,
			runner.m_iMatches, runner.m_iImmutable, runner.m_iControls, runner.m_iExtras, RICON_ObjectRfProfileFixtures.HOST_ONLY);
		int cases = RICON_ObjectRfProfileFixtures.CASES;
		if (runner.m_bOk && runner.m_iCases == cases && runner.m_iMatches == cases && runner.m_iImmutable == cases
			&& runner.m_iControls == 6 && runner.m_iExtras == 2)
		{
			Print("[RICON_OBJPROFILE] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_OBJPROFILE] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
