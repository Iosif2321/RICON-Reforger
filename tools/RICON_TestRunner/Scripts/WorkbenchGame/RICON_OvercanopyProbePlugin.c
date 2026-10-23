//! TOOL_ONLY: P4.2, без мира/Play/сервера и без production-каталога.
[WorkbenchPluginAttribute(name: "RICON: путь над кронами P4.2", description: "Frozen корпус P.2108-1 и контроли", wbModules: {"ScriptEditor"})]
class RICON_OvercanopyProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		Print("[RICON_CANOPY] source=" + RICON_OvercanopyFixtures.SOURCE_SHA);
		RICON_OvercanopyRun t = new RICON_OvercanopyRun();
		RICON_OvercanopyFixtures.Run(t);
		t.CheckExtras();
		PrintFormat("[RICON_CANOPY] cases=%1 match=%2 immutable=%3 controls=%4 extras=%5 hostOnly=%6", t.m_iCases,
			t.m_iMatches, t.m_iImmutable, t.m_iControls, t.m_iExtras, RICON_OvercanopyFixtures.HOST_ONLY);
		if (t.m_bOk && t.m_iCases == 30 && t.m_iMatches == 30 && t.m_iImmutable == 30 && t.m_iControls == 20 && t.m_iExtras == 5)
		{
			Print("[RICON_CANOPY] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_CANOPY] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
