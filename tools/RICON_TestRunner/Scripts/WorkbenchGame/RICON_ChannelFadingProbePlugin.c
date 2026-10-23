//! TOOL_ONLY: P1b исполняется без мира, сети, Play и сервера; составляющие среды — TEST_ONLY.
[WorkbenchPluginAttribute(name: "RICON: коэффициент среды P1b", description: "Корпус P1b, неизменность входа, порядок и отказы", wbModules: {"ScriptEditor"})]
class RICON_ChannelFadingProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		Print("[RICON_FADING] source=" + RICON_ChannelFadingFixtures.SOURCE_SHA);
		RICON_ChannelFadingRun runner = new RICON_ChannelFadingRun();
		RICON_ChannelFadingFixtures.Run(runner);
		runner.CheckArithmetic();
		runner.CheckFaults();
		runner.CheckOrder();
		PrintFormat("[RICON_FADING] cases=%1 match=%2 immutable=%3 controls=%4 extras=%5 hostOnly=%6", runner.m_iCases,
			runner.m_iMatches, runner.m_iImmutable, runner.m_iControls, runner.m_iExtras, RICON_ChannelFadingFixtures.HOST_ONLY);
		int cases = RICON_ChannelFadingFixtures.CASES;
		if (runner.m_bOk && runner.m_iCases == cases && runner.m_iMatches == cases && runner.m_iImmutable == cases
			&& runner.m_iControls == 9 && runner.m_iExtras == 3)
		{
			Print("[RICON_FADING] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_FADING] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
