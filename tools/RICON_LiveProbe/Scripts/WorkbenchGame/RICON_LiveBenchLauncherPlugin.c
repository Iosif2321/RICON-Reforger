//! TOOL_ONLY: запуск стенда (а′) — World Editor открывает мир из параметра командной строки -riconBenchWorld,
//! в памяти создаёт одну GenericEntity «RICON_LiveBench» с RICON_LiveRadioBenchComponent и переходит в Play.
//! Мир НЕ сохраняется: Save() не вызывается, файлы мира на диске не меняются. Документированные вызовы:
//! WorldEditor.GetCmdLine/SetOpenedResource/GetApi/SwitchToGameMode, WorldEditorAPI.BeginEntityAction/CreateEntity/
//! CreateComponent/EndEntityAction (Script API). Завершение процесса — внешним таймаутом (журнал стенда потоковый).
//! Запуск — только после START → ACK → run → END на шине и source review codex (#4091).
[WorkbenchPluginAttribute(name: "RICON: стенд раций в Play", description: "Открыть мир, добавить стенд в памяти, Play", wbModules: {"WorldEditor"})]
class RICON_LiveBenchLauncherPlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor)
		{
			Print("[RICON_LIVEB] state=NO_WORLD_EDITOR");
			return;
		}
		// Прогон 07:04:49: WorldEditor.GetCmdLine("-riconBenchWorld") при «-riconBenchWorld значение» вернул false.
		// Проверенный в RICON_WbReaderPlugin способ — System.GetCLIParam("имя") при форме «-имя="значение"».
		string world;
		string source = "System.GetCLIParam";
		if (!System.GetCLIParam("riconBenchWorld", world) || world.IsEmpty())
		{
			source = "WorldEditor.GetCmdLine";
			if (!editor.GetCmdLine("riconBenchWorld", world) || world.IsEmpty())
			{
				Print("[RICON_LIVEB] state=NO_WORLD_ARGUMENT tried=System.GetCLIParam,WorldEditor.GetCmdLine");
				return;
			}
		}
		PrintFormat("[RICON_LIVEB] state=WORLD_ARGUMENT via=%1", source);
		bool opened = editor.SetOpenedResource(world);
		PrintFormat("[RICON_LIVEB] state=OPEN world=%1 opened=%2", world, opened);
		if (!opened)
			return;
		WorldEditorAPI api = editor.GetApi();
		if (!api)
		{
			Print("[RICON_LIVEB] state=NO_API");
			return;
		}
		api.BeginEntityAction("RICON live bench");
		IEntitySource bench = api.CreateEntity("GenericEntity", "RICON_LiveBench", api.GetCurrentEntityLayerId(), null, vector.Zero, vector.Zero);
		IEntityComponentSource component;
		if (bench)
			component = api.CreateComponent(bench, "RICON_LiveRadioBenchComponent");
		api.EndEntityAction("RICON live bench");
		PrintFormat("[RICON_LIVEB] state=BENCH_CREATED entity=%1 component=%2 saved=0", bench != null, component != null);
		if (!bench || !component)
			return;
		editor.SwitchToGameMode(false, false);
		// Переход асинхронный: ванильный SCR_FPSDiagnosticPlugin.c:360–369 ждёт WorldEditorAPI.IsGameMode() в цикле
		// с Sleep(100) и считает потерю фокуса окна (GetFPS ≈ FPS_UNFOCUSED) провалом перехода. Здесь — только
		// наблюдение в отдельном потоке, без возврата в edit mode (codex #4139: одна выборка ничего не доказывает).
		PrintFormat("[RICON_LIVEB] state=PLAY_REQUESTED isGameModeNow=%1", api.IsGameMode());
		m_Api = api;
		thread WatchGameMode();
	}

	protected WorldEditorAPI m_Api;
	static const int WATCH_STEPS = 300;      //!< 300 × 100 мс = 30 с
	static const int WATCH_PRINT_EVERY = 10; //!< печать раз в секунду и при каждой смене режима

	protected void WatchGameMode()
	{
		bool last = m_Api.IsGameMode();
		for (int i = 0; i < WATCH_STEPS; i++)
		{
			bool now = m_Api.IsGameMode();
			if (now != last || i % WATCH_PRINT_EVERY == 0)
				PrintFormat("[RICON_LIVEB] watch t=%1ms isGameMode=%2 fps=%3", i * 100, now, System.GetFPS());
			last = now;
			Sleep(100);
		}
		PrintFormat("[RICON_LIVEB] watch=END isGameMode=%1", m_Api.IsGameMode());
	}
}
