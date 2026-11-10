//! Thin bridge: all pure/synthetic self-checks remain inside the Game module before sealing.
[WorkbenchPluginAttribute(name: "RICON: Check capture pulse coordinator", wbModules: {"ScriptEditor"})]
class RICON_LiveCapturePulseCheckPlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		RICON_CapturePulseSelfChecks checks = new RICON_CapturePulseSelfChecks();
		checks.Run();
		RICON_CaptureShort400SelfChecks shortChecks = new RICON_CaptureShort400SelfChecks();
		shortChecks.Run();
		RICON_DirectPulseChecks directChecks = new RICON_DirectPulseChecks();
		directChecks.Run();
	}
}
