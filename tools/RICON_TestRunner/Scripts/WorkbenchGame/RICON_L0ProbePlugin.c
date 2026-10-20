//! TOOL_ONLY: полный корпус L0, без Play, мира и сервера.
[WorkbenchPluginAttribute(name: "RICON: корпус L0", description: "Чистый расчёт Core против 57 векторов", wbModules: {"ScriptEditor"})]
class RICON_L0ProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		if (RICON_L0Fixtures.RunAll())
		{
			Print("[RICON_L0] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_L0] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
