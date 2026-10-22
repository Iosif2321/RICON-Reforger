//! TOOL_ONLY: P1 исполняется без мира, сети, Play и сервера; границы путей — TEST_ONLY.
[WorkbenchPluginAttribute(name: "RICON: отсечка полезного вклада", description: "Корпус P1, неизменность входа и границы", wbModules: {"ScriptEditor"})]
class RICON_ChannelCutoffProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		Print("[RICON_CUTOFF] source=" + RICON_ChannelCutoffFixtures.SOURCE_SHA);
		RICON_ChannelCutoffRun runner = new RICON_ChannelCutoffRun();
		RICON_ChannelCutoffFixtures.Run(runner);
		runner.CheckBounds();
		PrintFormat("[RICON_CUTOFF] cases=%1 match=%2 immutable=%3 controls=%4 extras=%5 hostOnly=%6", runner.m_iCases,
			runner.m_iMatches, runner.m_iImmutable, runner.m_iControls, runner.m_iExtras, RICON_ChannelCutoffFixtures.HOST_ONLY);
		int cases = RICON_ChannelCutoffFixtures.CASES;
		if (runner.m_bOk && runner.m_iCases == cases && runner.m_iMatches == cases && runner.m_iImmutable == cases
			&& runner.m_iControls == 7 && runner.m_iExtras == 3)
		{
			Print("[RICON_CUTOFF] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_CUTOFF] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
