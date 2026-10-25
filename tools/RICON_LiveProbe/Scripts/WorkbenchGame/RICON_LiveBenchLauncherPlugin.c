//! TOOL_ONLY: запуск стенда (а′) — World Editor открывает мир из параметра командной строки -riconBenchWorld,
//! в памяти создаёт одну GenericEntity «RICON_LiveBench» с RICON_LiveRadioBenchComponent и переходит в Play.
//! Мир НЕ сохраняется: Save() не вызывается, файлы мира на диске не меняются. Документированные вызовы:
//! WorldEditor.GetCmdLine/SetOpenedResource/GetApi/SwitchToGameMode, WorldEditorAPI.BeginEntityAction/CreateEntity/
//! CreateComponent/EndEntityAction/GetWorld/TryGetTerrainSurfaceY, BaseWorld.GetBoundBox (Script API).
//! Точка стенда проверяется внутри границ мира (ChoosePoint), иначе явный отказ без создания сущности.
//! A0b (codex #4233): необязательно в том же действии — одна сущность SCR_BaseGameMode (-riconBenchGameMode, только
//! это значение) и одна RadioManagerEntity (-riconBenchRadioManager=1), обе по имени класса и тоже только в памяти.
//! Завершение процесса — внешним таймаутом (журнал стенда потоковый).
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
		string benchMode;
		System.GetCLIParam("riconBenchMode", benchMode);
		string startFlagValue;
		bool startFlag = System.GetCLIParam("riconBenchStartFlag", startFlagValue);
		if (startFlag && (startFlagValue != "1" || benchMode != "inspect" || !RICON_LiveEndpointProbe.IsRequested()))
		{
			Print("[RICON_LIVEB] inspect=REFUSED reason=FLAG_REQUIRES_INSPECT_PROBE");
			return;
		}
		if (RICON_LiveEndpointProbe.IsRequested())
		{
			WorkbenchPlugin modeObserver = editor.GetPlugin(RICON_LivePlayModeObserverPlugin);
			PrintFormat("[RICON_WBMODE] registered=%1", modeObserver != null);
			if (!modeObserver)
				return;
		}
		if (benchMode == "inspect")
		{
			// B0b: доступ к меню открытого edit-мира, без создания стенда и без перехода в Play.
			PrintFormat("[RICON_LIVEB] state=OPEN_ONLY saved=0 isGameMode=%1", api.IsGameMode());
			if (RICON_LiveEndpointProbe.IsRequested())
			{
				m_Api = api;
				thread ReturnFromInspectedPlay(editor, startFlag);
			}
			return;
		}
		vector point;
		if (!ChoosePoint(api, point))
			return;
		bool wantGameMode, wantManager;
		if (!ReadSetup(api, wantGameMode, wantManager))
			return;
		bool endpointOnly = benchMode == "endpoint";
		string restartValue;
		bool restartEndpoint = System.GetCLIParam("riconEndpointRestart", restartValue) && restartValue == "1";
		if (restartEndpoint && (!endpointOnly || !RICON_LiveEndpointProbe.IsRequested()))
		{
			Print("[RICON_LIVEB] state=SETUP_REFUSED reason=RESTART_REQUIRES_ENDPOINT_PROBE");
			return;
		}
		IEntitySource bench;
		IEntityComponentSource component;
		bool createEntities = !endpointOnly || wantGameMode || wantManager;
		if (createEntities)
			api.BeginEntityAction("RICON live bench");
		if (!endpointOnly)
		{
			bench = api.CreateEntity("GenericEntity", "RICON_LiveBench", api.GetCurrentEntityLayerId(), null, point, vector.Zero);
			if (bench)
				component = api.CreateComponent(bench, "RICON_LiveRadioBenchComponent");
		}
		// A0b (codex #4233): по имени класса (документированный CreateEntity(className, …)), без префаба и без fallback
		IEntitySource gameMode;
		if (wantGameMode)
			gameMode = api.CreateEntity(BARE_GAME_MODE, "RICON_BenchGameMode", api.GetCurrentEntityLayerId(), null, point, vector.Zero);
		IEntitySource manager;
		if (wantManager)
			manager = api.CreateEntity(MANAGER_CLASS, "RICON_BenchRadioManager", api.GetCurrentEntityLayerId(), null, point, vector.Zero);
		if (createEntities)
			api.EndEntityAction("RICON live bench");
		if (endpointOnly)
			Print("[RICON_LIVEB] state=ENDPOINT_ONLY bench=0 saved=0");
		else
			PrintFormat("[RICON_LIVEB] state=BENCH_CREATED entity=%1 component=%2 saved=0", bench != null, component != null);
		if (!endpointOnly && (!bench || !component))
			return;
		PrintFormat("[RICON_LIVEB] setup created gameMode=%1 gameModeOk=%2 radioManager=%3 via=class radioManagerOk=%4",
			wantGameMode, gameMode != null, wantManager, manager != null);
		if ((wantGameMode && !gameMode) || (wantManager && !manager))
		{
			Print("[RICON_LIVEB] state=SETUP_REFUSED reason=CREATE_FAILED");
			return;
		}
		editor.SwitchToGameMode(false, false);
		// Переход асинхронный: ванильный SCR_FPSDiagnosticPlugin.c:360–369 ждёт WorldEditorAPI.IsGameMode() в цикле
		// с Sleep(100) и считает потерю фокуса окна (GetFPS ≈ FPS_UNFOCUSED) провалом перехода. Здесь — только
		// наблюдение в отдельном потоке, без возврата в edit mode (codex #4139: одна выборка ничего не доказывает).
		PrintFormat("[RICON_LIVEB] state=PLAY_REQUESTED isGameModeNow=%1", api.IsGameMode());
		m_Api = api;
		thread WatchGameMode();
		if (restartEndpoint)
			thread RestartEndpoint(editor);
	}

	//! B0b: Play запускает оператор через меню; после наблюдения возвращаемся в edit для восстановления настройки.
	protected void ReturnFromInspectedPlay(WorldEditor editor, bool startFlag)
	{
		if (startFlag && !StartFromFlag(editor))
			return;
		Print("[RICON_LIVEB] inspect=WAIT_PLAY limitMs=120000");
		int waitSteps;
		while (!m_Api.IsGameMode() && waitSteps < 1200)
		{
			Sleep(100);
			waitSteps++;
		}
		if (!m_Api.IsGameMode())
		{
			Print("[RICON_LIVEB] inspect=REFUSED reason=PLAY_NOT_ENTERED");
			return;
		}
		Print("[RICON_LIVEB] inspect=PLAY_ENTERED");
		int samples;
		for (int i = 0; i < 450 && m_Api.IsGameMode(); i++)
		{
			ArmaReforgerScripted game = ArmaReforgerScripted.Cast(GetGame());
			if (game)
				samples = game.RICON_EndpointSampleCount();
			if (samples >= RICON_LiveEndpointProbe.SAMPLE_LIMIT)
				break;
			Sleep(100);
		}
		PrintFormat("[RICON_LIVEB] inspect=OBSERVATION_END samples=%1 complete=%2 isGameMode=%3", samples,
			samples >= RICON_LiveEndpointProbe.SAMPLE_LIMIT, m_Api.IsGameMode());
		if (m_Api.IsGameMode())
			editor.SwitchToEditMode();
		Print("[RICON_LIVEB] inspect=EDIT_REQUESTED");
		if (WaitForStableEdit())
			Print("[RICON_LIVEB] inspect=EDIT_STABLE checks=51");
		else
			Print("[RICON_LIVEB] inspect=REFUSED reason=EDIT_NOT_REACHED");
	}

	//! Единственный оператор создаёт отсутствующий сигнал только после проверки настройки сервера.
	protected bool StartFromFlag(WorldEditor editor)
	{
		string flag = "$RICON_LiveProbe:ops/RICON_B0b_start.flag";
		if (!FileIO.FileExists("$logs:console.log") || FileIO.FileExists(flag))
		{
			Print("[RICON_LIVEB] inspect=REFUSED reason=FLAG_PATH_OR_STALE");
			return false;
		}
		Print("[RICON_LIVEB] inspect=WAIT_FLAG limitMs=120000");
		for (int i = 0; i < 1200; i++)
		{
			if (m_Api.IsGameMode())
			{
				Print("[RICON_LIVEB] inspect=REFUSED reason=PLAY_BEFORE_FLAG");
				return false;
			}
			if (FileIO.FileExists(flag))
			{
				Print("[RICON_LIVEB] inspect=FLAG_SEEN");
				editor.SwitchToGameMode(false, false);
				Print("[RICON_LIVEB] inspect=FLAG_PLAY_REQUESTED");
				return true;
			}
			Sleep(100);
		}
		Print("[RICON_LIVEB] inspect=REFUSED reason=FLAG_NOT_SEEN");
		return false;
	}

	//! Тест жизненного цикла: прерываем активное наблюдение, затем запускаем новый Play, не сохраняя мир.
	protected void RestartEndpoint(WorldEditor editor)
	{
		bool sampled;
		for (int i = 0; i < 600; i++)
		{
			ArmaReforgerScripted game = ArmaReforgerScripted.Cast(GetGame());
			if (m_Api.IsGameMode() && game && game.RICON_EndpointSampleCount() >= 3)
			{
				sampled = true;
				break;
			}
			Sleep(100);
		}
		if (!sampled)
		{
			Print("[RICON_LIVEB] restart=REFUSED reason=NO_ACTIVE_SAMPLES");
			return;
		}
		editor.SwitchToEditMode();
		Print("[RICON_LIVEB] restart=EDIT_REQUESTED");
		if (!WaitForStableEdit())
		{
			Print("[RICON_LIVEB] restart=REFUSED reason=EDIT_NOT_REACHED");
			return;
		}
		Print("[RICON_LIVEB] restart=EDIT_STABLE checks=51");
		editor.SwitchToGameMode(false, false);
		Print("[RICON_LIVEB] restart=SECOND_PLAY_REQUESTED");
		for (int k = 0; k < 300; k++)
		{
			if (m_Api.IsGameMode())
			{
				Print("[RICON_LIVEB] restart=SECOND_PLAY_ENTERED");
				return;
			}
			Sleep(100);
		}
		Print("[RICON_LIVEB] restart=REFUSED reason=SECOND_PLAY_NOT_ENTERED");
	}

	protected bool WaitForStableEdit()
	{
		BaseWorld editWorld;
		int stableChecks;
		for (int j = 0; j < 600; j++)
		{
			BaseWorld currentWorld = m_Api.GetWorld();
			bool validEdit = !m_Api.IsGameMode() && currentWorld && currentWorld.IsEditMode();
			if (validEdit && m_Api.GetEditorEntityCount() > 0)
			{
				if (currentWorld == editWorld)
					stableChecks++;
				else
				{
					editWorld = currentWorld;
					stableChecks = 1;
				}
				// 51 проверка с 50 промежутками: не менее 5 с после появления одного и того же edit-мира.
				if (stableChecks >= 51)
					return true;
			}
			else
			{
				editWorld = null;
				stableChecks = 0;
			}
			Sleep(100);
		}
		return false;
	}

	static const float EDGE_MARGIN_M = 10.0;  //!< запас от края границ по x/z; рации ставятся в +0…+6 м по x от стенда
	static const float HEIGHT_ABOVE_M = 1.0;  //!< высота стенда над рельефом

	//! Попытки 5/5b: стенд в <0,0,0> «out world bounds» в Plain-мире (codex #4176). Точка — центр BaseWorld.GetBoundBox
	//! по x/z, высота — WorldEditorAPI.TryGetTerrainSurfaceY + HEIGHT_ABOVE_M. Отказ (без создания стенда), если границы
	//! уже 2·EDGE_MARGIN_M, в точке нет рельефа или точка с запасом не внутри границ по всем трём осям.
	protected bool ChoosePoint(WorldEditorAPI api, out vector point)
	{
		BaseWorld world = api.GetWorld();
		if (!world)
		{
			Print("[RICON_LIVEB] state=PLACEMENT_REFUSED reason=NO_WORLD");
			return false;
		}
		vector mins, maxs;
		world.GetBoundBox(mins, maxs);
		float x = (mins[0] + maxs[0]) * 0.5;
		float z = (mins[2] + maxs[2]) * 0.5;
		float terrainY;
		bool onTerrain = api.TryGetTerrainSurfaceY(x, z, terrainY);
		point = Vector(x, terrainY + HEIGHT_ABOVE_M, z);
		bool spanOk = maxs[0] - mins[0] > 2 * EDGE_MARGIN_M && maxs[2] - mins[2] > 2 * EDGE_MARGIN_M;
		bool insideX = point[0] >= mins[0] + EDGE_MARGIN_M && point[0] <= maxs[0] - EDGE_MARGIN_M;
		bool insideZ = point[2] >= mins[2] + EDGE_MARGIN_M && point[2] <= maxs[2] - EDGE_MARGIN_M;
		bool insideY = point[1] >= mins[1] && point[1] <= maxs[1];
		bool inside = insideX && insideZ && insideY;
		PrintFormat("[RICON_LIVEB] placement boundsMin=%1 boundsMax=%2 point=%3 onTerrain=%4 terrainY=%5 spanOk=%6 inside=%7",
			mins, maxs, point, onTerrain, terrainY, spanOk, inside);
		if (!spanOk || !onTerrain || !inside)
		{
			PrintFormat("[RICON_LIVEB] state=PLACEMENT_REFUSED reason=spanOk:%1,onTerrain:%2,inside:%3", spanOk, onTerrain, inside);
			return false;
		}
		return true;
	}

	//! A0b (codex #4233): единственное допустимое значение -riconBenchGameMode и класс менеджера радио.
	static const string BARE_GAME_MODE = "SCR_BaseGameMode";
	static const string MANAGER_CLASS = "RadioManagerEntity";

	//! Необязательные -riconBenchGameMode=SCR_BaseGameMode и -riconBenchRadioManager=0|1. Другое значение, уже
	//! существующий game mode при запросе bare, уже существующий менеджер при запросе создания или сущность мира без
	//! экземпляра (проверить нельзя) — явный отказ без создания чего-либо (SETUP_REFUSED). Без параметров — как в bench9.
	protected bool ReadSetup(WorldEditorAPI api, out bool wantGameMode, out bool wantManager)
	{
		string gameModeValue, managerValue;
		// codex #4237: явно заданное пустое значение — не «нет параметра», а значение вне списка
		bool gameModeGiven = System.GetCLIParam("riconBenchGameMode", gameModeValue);
		bool managerGiven = System.GetCLIParam("riconBenchRadioManager", managerValue);
		int gameModes = 0;
		int managers = 0;
		int noInstance = 0;
		int count = api.GetEditorEntityCount();
		for (int i = 0; i < count; i++)
		{
			IEntitySource src = api.GetEditorEntity(i);
			IEntity instance = null;
			if (src)
				instance = api.SourceToEntity(src);
			if (!instance)
			{
				noInstance++;
				continue;
			}
			if (BaseGameMode.Cast(instance))
				gameModes++;
			if (RadioManagerEntity.Cast(instance))
				managers++;
		}
		PrintFormat("[RICON_LIVEB] setup existing entities=%1 gameModes=%2 radioManagers=%3 noInstance=%4 gameModeParam=%5:%6 radioManagerParam=%7:%8",
			count, gameModes, managers, noInstance, gameModeGiven, gameModeValue, managerGiven, managerValue);
		string reason;
		if (gameModeGiven && gameModeValue != BARE_GAME_MODE)
			reason = "GAMEMODE_PARAM";
		else if (managerGiven && managerValue != "0" && managerValue != "1")
			reason = "RADIOMANAGER_PARAM";
		wantGameMode = gameModeGiven;
		wantManager = managerGiven && managerValue == "1";
		if (reason.IsEmpty() && (wantGameMode || wantManager) && noInstance > 0)
			reason = "UNVERIFIED_SOURCES";
		else if (reason.IsEmpty() && wantGameMode && gameModes > 0)
			reason = "GAMEMODE_EXISTS";
		else if (reason.IsEmpty() && wantManager && managers > 0)
			reason = "RADIOMANAGER_EXISTS";
		if (!reason.IsEmpty())
		{
			PrintFormat("[RICON_LIVEB] state=SETUP_REFUSED reason=%1", reason);
			return false;
		}
		return true;
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
