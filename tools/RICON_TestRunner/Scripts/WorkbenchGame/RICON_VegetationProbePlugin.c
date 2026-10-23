//! TOOL_ONLY: чистый P4, без мира/Play/сервера и без production-каталога.
[WorkbenchPluginAttribute(name: "RICON: растительность P4", description: "Frozen корпус и независимые контроли", wbModules: {"ScriptEditor"})]
class RICON_VegetationProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		Print("[RICON_VEG] source=" + RICON_VegetationFixtures.SOURCE_SHA);
		RICON_VegetationRun t = new RICON_VegetationRun();
		RICON_VegetationFixtures.Run(t);
		t.CheckExtras();
		PrintFormat("[RICON_VEG] cases=%1 match=%2 immutable=%3 controls=%4 extras=%5 hostOnly=%6", t.m_iCases,
			t.m_iMatches, t.m_iImmutable, t.m_iControls, t.m_iExtras, RICON_VegetationFixtures.HOST_ONLY);
		if (t.m_bOk && t.m_iCases == 58 && t.m_iMatches == 58 && t.m_iImmutable == 58 && t.m_iControls == 13 && t.m_iExtras == 3)
		{
			Print("[RICON_VEG] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_VEG] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
