//! TOOL_ONLY: S3b исполняется без мира, сети, Play и сервера; источник номеров — тестовый double REQ-LP §6.
[WorkbenchPluginAttribute(name: "RICON: снимки связи", description: "Корпус S3b, фактические вызовы source и неизменность", wbModules: {"ScriptEditor"})]
class RICON_LinkPublicationProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		Print("[RICON_LINKPUB] source=" + RICON_LinkPublicationFixtures.SOURCE_SHA);
		RICON_LinkPublicationRun runner = new RICON_LinkPublicationRun();
		RICON_LinkPublicationFixtures.Run(runner);
		runner.CheckSources();
		PrintFormat("[RICON_LINKPUB] cases=%1 match=%2 immutable=%3 controls=%4 extras=%5 snapshots=%6", runner.m_iCases,
			runner.m_iMatches, runner.m_iImmutable, runner.m_iControls, runner.m_iExtras, runner.m_iSnapshots);
		int cases = RICON_LinkPublicationFixtures.CASES;
		if (runner.m_bOk && runner.m_iCases == cases && runner.m_iMatches == cases && runner.m_iImmutable == cases
			&& runner.m_iControls == 15 && runner.m_iExtras == 4)
		{
			Print("[RICON_LINKPUB] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_LINKPUB] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
