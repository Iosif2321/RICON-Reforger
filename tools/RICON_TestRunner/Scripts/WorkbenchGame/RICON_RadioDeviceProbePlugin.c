//! TOOL_ONLY: корпус S2 через настоящий RICON_RadioQueryBuilder и цепочка до SINR, без мира, Play и сервера.
[WorkbenchPluginAttribute(name: "RICON: адаптер рации", description: "Сборка запроса пары раций и отказы S2", wbModules: {"ScriptEditor"})]
class RICON_RadioDeviceProbePlugin : WorkbenchPlugin
{
	static const int CONTROLS = 9;
	static const int CHAINS = 2;

	override void RunCommandline()
	{
		Print("[RICON_RADIO] source=" + RICON_RadioDeviceFixtures.SOURCE_SHA);
		RICON_RadioDeviceRun runner = new RICON_RadioDeviceRun();
		RICON_RadioDeviceFixtures.Run(runner);
		runner.CheckChain();
		runner.CheckReader();
		PrintFormat("[RICON_RADIO] cases=%1 quietPositive=%2 immutable=%3 controls=%4 chains=%5 reader=%6",
			runner.m_iCases, runner.m_iQuietPositive, runner.m_iImmutable, runner.m_iControls, runner.m_iChains,
			runner.m_iReaderChecks);
		int cases = RICON_RadioDeviceFixtures.CASES;
		if (runner.m_bOk && runner.m_iCases == cases && runner.m_iQuietPositive == cases && runner.m_iImmutable == cases
			&& runner.m_iControls == CONTROLS && runner.m_iChains == CHAINS && runner.m_iReaderChecks == 1)
		{
			Print("[RICON_RADIO] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_RADIO] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
