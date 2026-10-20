//! TOOL_ONLY: снимки Core, без Play, мира и authority-компонента.
[WorkbenchPluginAttribute(name: "RICON: снимки", description: "43 чистых случая, 3 allocator заблокированы", wbModules: {"ScriptEditor"})]
class RICON_SnapshotProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		bool ok = RICON_SnapshotRun.SpecialSelfCheck();
		ok = RICON_SnapshotFixtures.RunAll() && ok;
		if (ok)
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
