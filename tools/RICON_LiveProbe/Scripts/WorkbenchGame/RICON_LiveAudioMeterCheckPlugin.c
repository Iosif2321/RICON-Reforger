//! Thin Workbench bridge; pure checks execute in Game without a cross-module subclass.
[WorkbenchPluginAttribute(name: "RICON: Check local audio meter", wbModules: {"ScriptEditor"})]
class RICON_LiveAudioMeterCheckPlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		RICON_AudioMeterSelfChecks checks = new RICON_AudioMeterSelfChecks();
		checks.Run();
		RICON_AudioMeterLocalSelfChecks localChecks = new RICON_AudioMeterLocalSelfChecks();
		localChecks.Run();
	}
}
