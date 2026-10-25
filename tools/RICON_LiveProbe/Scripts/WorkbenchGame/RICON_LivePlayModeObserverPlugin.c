//! TOOL_ONLY B0b: штатное имя режима Play и самопроверка enum; режим и настройки не изменяются.
[WorkbenchPluginAttribute(name: "RICON: наблюдение режима Play", description: "Имя режима и проверка RplMode", wbModules: {"WorldEditor", "ScriptEditor"})]
class RICON_LivePlayModeObserverPlugin : WorldEditorPlugin
{
	override void OnGameModeStarted(string worldName, string gameMode, bool playFromCameraPos, vector cameraPosition, vector cameraAngles)
	{
		if (!RICON_LiveEndpointProbe.IsRequested())
			return;
		int fromCamera = 0;
		if (playFromCameraPos)
			fromCamera = 1;
		PrintFormat("[RICON_WBMODE] event=START mode=%1 world=%2 fromCamera=%3",
			RICON_LiveText.Esc(gameMode), RICON_LiveText.Esc(worldName), fromCamera);
	}

	override void OnGameModeEnded()
	{
		if (RICON_LiveEndpointProbe.IsRequested())
			Print("[RICON_WBMODE] event=END");
	}

	//! Явный CLI-запуск в ScriptEditor проверяет значения enum, не создавая мира или сетевой сессии.
	override void RunCommandline()
	{
		array<int> modes = {RplMode.None, RplMode.Client, RplMode.Listen, RplMode.Dedicated};
		array<string> names = {"None", "Client", "Listen", "Dedicated"};
		int passed;
		for (int i = 0; i < modes.Count(); i++)
		{
			string name = typename.EnumToString(RplMode, modes[i]);
			bool matches = modes[i] == i && name == names[i];
			if (matches)
				passed++;
			PrintFormat("[RICON_RPLMODE] value=%1 name=%2 match=%3", modes[i], RICON_LiveText.Esc(name), matches);
		}
		PrintFormat("[RICON_RPLMODE] checks=4 passed=%1", passed);
		int signalPassed = RICON_LiveBenchLauncherPlugin.CheckPluginSignal();
		PrintFormat("[RICON_PLUGIN_SIGNAL] checks=4 passed=%1", signalPassed);
		array<string> sampleValues = {"", "30", "120", "", "0", "121", "120junk", "0120"};
		array<int> sampleExpected = {30, 30, 120, 0, 0, 0, 0, 0};
		int samplePassed;
		for (int j = 0; j < sampleValues.Count(); j++)
		{
			if (RICON_LiveEndpointProbe.ResolveSampleLimit(j > 0, sampleValues[j]) == sampleExpected[j])
				samplePassed++;
		}
		PrintFormat("[RICON_PROBE_CONFIG] checks=8 passed=%1", samplePassed);
		if (passed == 4 && signalPassed == 4 && samplePassed == 8)
		{
			Print("[RICON_RPLMODE] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_RPLMODE] done: MISMATCH");
			Workbench.Exit(1);
		}
	}
}
