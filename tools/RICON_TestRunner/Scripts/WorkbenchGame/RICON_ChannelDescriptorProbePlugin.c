//! TOOL_ONLY: CT10.1 исполняется без мира, сети, Play и сервера; дескрипторы — TEST_ONLY.
[WorkbenchPluginAttribute(name: "RICON: дескриптор канала CT10.1", description: "Корпус RCFD/1, Rebase, round trip и отказы", wbModules: {"ScriptEditor"})]
class RICON_ChannelDescriptorProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		Print("[RICON_DESCRIPTOR] source=" + RICON_ChannelDescriptorFixtures.SOURCE_SHA);
		RICON_ChannelDescriptorRun runner = new RICON_ChannelDescriptorRun();
		RICON_ChannelDescriptorFixtures.Run(runner);
		runner.CheckArithmetic();
		runner.CheckFrames();
		runner.CheckRebaseProperties();
		PrintFormat("[RICON_DESCRIPTOR] cases=%1 match=%2 immutable=%3 controls=%4 extras=%5 hostOnly=%6", runner.m_iCases,
			runner.m_iMatches, runner.m_iImmutable, runner.m_iControls, runner.m_iExtras, RICON_ChannelDescriptorFixtures.HOST_ONLY);
		int cases = RICON_ChannelDescriptorFixtures.CASES;
		if (runner.m_bOk && runner.m_iCases == cases && runner.m_iMatches == cases && runner.m_iImmutable == cases
			&& runner.m_iControls == 8 && runner.m_iExtras == 3)
		{
			Print("[RICON_DESCRIPTOR] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_DESCRIPTOR] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
