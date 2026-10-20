//! TOOL_ONLY: the route corpus (48 vectors) against the real route contract; no Play, world or server.
[WorkbenchPluginAttribute(name: "RICON: корпус маршрутов", description: "RICON_RouteContract против 48 векторов", wbModules: {"ScriptEditor"})]
class RICON_RouteProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		Print("[RICON_ROUTE] source=" + RICON_RouteFixtures.SOURCE_SHA);
		RICON_RouteRun t = new RICON_RouteRun();
		RICON_RouteFixtures.Run(t);
		PrintFormat("[RICON_ROUTE] cases=%1 quietPositive=%2 controls=%3 adapterMapped=%4 testSubclass=%5", t.m_iCases,
			t.m_iQuietPositive, t.m_iControls, RICON_RouteFixtures.ADAPTER_MAPPED, RICON_RouteFixtures.TEST_SUBCLASS);
		bool ok = t.m_bOk && t.m_iCases == 48 && t.m_iQuietPositive == 48 && t.m_iControls == 4;
		if (ok)
		{
			Print("[RICON_ROUTE] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_ROUTE] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
