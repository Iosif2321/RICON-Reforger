//! TOOL_ONLY: executes the RICON_CoreTests fixtures inside Workbench (ScriptEditor), without Play, world or server.
//! Owner permission 2026-10-19: "run our tests in Workbench through a plugin, without Play and server".
//! It only calls the pure fixture code of RICON_CoreTests and prints the result to the Workbench log;
//! it reads and writes no files and touches no game resources.
//!
//! Command line: -wbModule=ScriptEditor -plugin=RICON_TestRunnerPlugin
//! Exit code: 0 = every suite passed, 5 = a suite failed. The final log line "[RICON_TestRunner] done: PASS|FAIL" is printed
//! only when every suite ran.
[WorkbenchPluginAttribute(name: "RICON Test Runner", description: "Run RICON_CoreTests fixtures (TOOL_ONLY)", wbModules: {"ScriptEditor"})]
class RICON_TestRunnerPlugin : WorkbenchPlugin
{
	static const string TAG = "[RICON_TestRunner] ";

	protected static string Verdict(bool passed)
	{
		if (passed)
			return "PASS";
		return "FAIL";
	}

	override void RunCommandline()
	{
		Print(TAG + "start");
		bool lifecycle = RICON_LifecycleFixtures.RunAll();
		Print(TAG + "suite lifecycle: " + Verdict(lifecycle));
		bool extension = RICON_ExtFixtures.RunAll();
		Print(TAG + "suite extension: " + Verdict(extension));

		bool ok = lifecycle && extension;
		Print(TAG + "done: " + Verdict(ok));
		if (ok)
			Workbench.Exit(0);
		else
			Workbench.Exit(5);
	}
}
