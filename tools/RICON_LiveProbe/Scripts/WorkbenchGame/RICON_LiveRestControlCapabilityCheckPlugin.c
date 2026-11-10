//! Synthetic Game checks и отдельный optional WB context lookup, без HTTP запросов.
[WorkbenchPluginAttribute(name: "RICON: Check REST control capability", wbModules: {"ScriptEditor"})]
class RICON_LiveRestControlCapabilityCheckPlugin : WorkbenchPlugin
{
	protected void ContextProbe()
	{
		string requestedPort;
		if (!System.GetCLIParam("riconRestContextProbePort", requestedPort)) return;
		int port;
		if (!RICON_RestCapabilityGate.Port(requestedPort, port))
		{
			Print("[RICON_REST_CTX] scope=WB_ONLY status=INVALID_PORT apiPresent=NOT_CHECKED contextPresent=NOT_CHECKED noGET=1"); return;
		}
		Game game = GetGame();
		RestApi api;
		RestContext context;
		if (game) api = game.GetRestApi();
		if (api) context = api.GetContext(RICON_RestCapabilityGate.BaseUrl(port));
		PrintFormat("[RICON_REST_CTX] scope=WB_ONLY apiPresent=%1 contextPresent=%2 noGET=1 pure=0 capability=NOT_PROVEN", api != null, context != null);
	}
	override void RunCommandline()
	{
		RICON_RestCapabilityChecks checks = new RICON_RestCapabilityChecks();
		checks.Run();
		RICON_RestRehearsalChecks rehearsalChecks = new RICON_RestRehearsalChecks();
		rehearsalChecks.Run();
		RICON_RestCaptureChecks captureChecks = new RICON_RestCaptureChecks();
		captureChecks.Run();
		RICON_CaptureNoSendChecks noSendChecks = new RICON_CaptureNoSendChecks();
		noSendChecks.Run();
		RICON_CaptureWrongFrequencyChecks frequencyChecks = new RICON_CaptureWrongFrequencyChecks();
		frequencyChecks.Run();
		RICON_CaptureBlindChecks blindChecks = new RICON_CaptureBlindChecks();
		blindChecks.Run();
		ContextProbe();
	}
}
