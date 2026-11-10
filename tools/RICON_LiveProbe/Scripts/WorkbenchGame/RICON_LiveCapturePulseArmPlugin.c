//! Explicit UI bridge only. Guard Ready/Armed ACK and held PID/window proof belong to the external coordinator.
[WorkbenchPluginAttribute(name: "RICON: Arm short capture pulse", wbModules: {"WorldEditor"})]
class RICON_LiveCapturePulseArmPlugin : WorkbenchPlugin
{
	override void Run()
	{
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		WorldEditorAPI api;
		if (editor) api = editor.GetApi();
		ArmaReforgerScripted game = ArmaReforgerScripted.Cast(GetGame());
		if (!api || !api.IsGameMode() || !game || !game.GetWorld() || game.GetWorld().IsEditMode())
		{
			Print("[RICON_PULSE_UI] result=REFUSED reason=NOT_CURRENT_PLAY"); return;
		}
		string run; string attempt; string mode;
		bool configured = System.GetCLIParam("riconPulseMode", mode);
		if (RICON_CapturePulseGate.Mode(configured, mode) == RICON_ECapturePulseMode.OFF
			|| !System.GetCLIParam("riconNativeRxRun", run) || !RICON_D0Gate.ValidRunId(run)
			|| !System.GetCLIParam("riconPulseAttempt", attempt) || !RICON_D0Gate.ValidRunId(attempt))
		{
			Print("[RICON_PULSE_UI] result=REFUSED reason=CONFIG"); return;
		}
		bool accepted = game.RICON_RequestCapturePulseArm(run, attempt, mode);
		PrintFormat("[RICON_PULSE_UI] run=%1 attempt=%2 mode=%3 accepted=%4 guardEvidenceScope=EXTERNAL_COORDINATOR_RECEIPT guardVerifiedByGame=0",
			run, attempt, mode, accepted);
	}
	//! Explicit refusal prevents any inherited command-line fallback from dispatching Run.
	override void RunCommandline()
	{
		Print("[RICON_PULSE_UI] result=REFUSED reason=CLI_ARM_FORBIDDEN");
	}
}
