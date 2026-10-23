//! TOOL_ONLY: P4.3 с готовыми бюджетами, без мира/Play/сервера.
[WorkbenchPluginAttribute(name: "RICON: полный путь растительности P4.3", description: "Frozen бюджеты и контроль сравнения", wbModules: {"ScriptEditor"})]
class RICON_VegetationPathProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		Print("[RICON_VPATH] source=" + RICON_VegetationPathFixtures.SOURCE_SHA);
		RICON_VegetationPathRun t = new RICON_VegetationPathRun();
		RICON_VegetationPathFixtures.Run(t);
		t.CheckExtras();
		PrintFormat("[RICON_VPATH] cases=%1 match=%2 immutable=%3 controls=%4 extras=%5 hostOnly=%6", t.m_iCases,
			t.m_iMatches, t.m_iImmutable, t.m_iControls, t.m_iExtras, RICON_VegetationPathFixtures.HOST_ONLY);
		if (t.m_bOk && t.m_iCases == 29 && t.m_iMatches == 29 && t.m_iImmutable == 29 && t.m_iControls == 23 && t.m_iExtras == 3)
		{
			Print("[RICON_VPATH] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_VPATH] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
