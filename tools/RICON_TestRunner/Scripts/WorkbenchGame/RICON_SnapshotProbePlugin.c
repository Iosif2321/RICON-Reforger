//! TOOL_ONLY: снимки Core, без Play, мира и authority-компонента.
[WorkbenchPluginAttribute(name: "RICON: снимки", description: "43 чистых случая, 3 allocator заблокированы", wbModules: {"ScriptEditor"})]
class RICON_SnapshotProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		if (RICON_SnapshotFixtures.RunAll())
		{
			Print("[RICON_SNAPSHOT] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_SNAPSHOT] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
