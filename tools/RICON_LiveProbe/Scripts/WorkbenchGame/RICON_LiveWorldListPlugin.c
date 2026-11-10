//! TOOL_ONLY, только чтение: состав миров-кандидатов для стенда A0 (codex #4149, #4152).
//! Для каждого мира из параметров -riconListWorld1="…" … -riconListWorld4="…" World Editor открывает мир и печатает
//! opened, IsGameMode до/после, число сущностей, первые 15 сущностей (класс, имя, префаб предка), до 15 различных
//! классов со счётчиками и сущности, приводимые к BaseGameMode. Не вызывает SwitchToGameMode, CreateEntity, Save,
//! не меняет мир и не запускает автотесты. Документированные вызовы (Script API WorldEditorAPI, IEntitySource):
//! GetEditorEntityCount, GetEditorEntity, SourceToEntity, GetNumSubScenes, IsGameMode; BaseContainer.GetClassName,
//! GetName, GetAncestor; BaseResourceObject.GetResourceName.
//! B0d (codex #5061, #5065): при -riconListDetail=1 у SCR_RespawnSystemComponent GameMode — m_bEnableRespawn и m_SpawnLogic
//! в мире и у предка, плюс имена полей, заданных в самом контейнере (Get, GetObject, IsVariableSetDirectly, GetNumVars, GetVarName).
//! Запуск — только после START → ACK → run → END на шине.
[WorkbenchPluginAttribute(name: "RICON: состав миров (только чтение)", description: "Список сущностей и GameMode миров", wbModules: {"WorldEditor"})]
class RICON_LiveWorldListPlugin : WorkbenchPlugin
{
	static const int MAX_WORLDS = 4;
	static const int MAX_ENTITIES = 15;
	static const int MAX_CLASSES = 15;
	static const int MAX_GAME_MODES = 5;

	override void RunCommandline()
	{
		string geometry;
		m_bGeometry = System.GetCLIParam("riconListGeometry", geometry) && geometry == "1";
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor)
		{
			Print("[RICON_LIVEW] state=NO_WORLD_EDITOR");
			if (m_bGeometry)
			{
				GeometryResult(0, "INCOMPLETE", "NO_WORLD_EDITOR", 0);
			}
			return;
		}
		string detail;
		m_bDetail = System.GetCLIParam("riconListDetail", detail) && detail == "1";
		if (m_bGeometry && m_bDetail)
		{
			GeometryResult(0, "INCOMPLETE", "DETAIL_NOT_ALLOWED", 0);
			return;
		}
		int listed = 0;
		for (int w = 1; w <= MAX_WORLDS; w++)
		{
			string world;
			if (!System.GetCLIParam("riconListWorld" + w.ToString(), world) || world.IsEmpty())
				continue;
			listed++;
			ListWorld(editor, w, world);
		}
		if (listed == 0)
		{
			Print("[RICON_LIVEW] state=NO_WORLD_ARGUMENT tried=System.GetCLIParam");
			if (m_bGeometry)
			{
				GeometryResult(0, "INCOMPLETE", "NO_WORLD_ARGUMENT", 0);
			}
		}
		PrintFormat("[RICON_LIVEW] state=DONE worlds=%1 saved=0 play=0", listed);
	}

	protected void ListWorld(WorldEditor editor, int w, string world)
	{
		if (m_bGeometry && world != "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent")
		{
			GeometryResult(w, "INCOMPLETE", "WORLD_NOT_FIXED_MPTEST", 0);
			return;
		}
		bool opened = editor.SetOpenedResource(world);
		PrintFormat("[RICON_LIVEW] w=%1 state=OPEN world=%2 opened=%3", w, world, opened);
		if (!opened)
		{
			if (m_bGeometry)
			{
				GeometryResult(w, "INCOMPLETE", "WORLD_NOT_OPENED", 0);
			}
			return;
		}
		WorldEditorAPI api = editor.GetApi();
		if (!api)
		{
			PrintFormat("[RICON_LIVEW] w=%1 state=NO_API", w);
			if (m_bGeometry)
			{
				GeometryResult(w, "INCOMPLETE", "NO_API", 0);
			}
			return;
		}
		bool gameBefore = api.IsGameMode();
		if (m_bGeometry && gameBefore)
		{
			GeometryResult(w, "INCOMPLETE", "NOT_EDITOR_CONTEXT", 0);
			return;
		}
		int count = api.GetEditorEntityCount();
		PrintFormat("[RICON_LIVEW] w=%1 entities=%2 subScenes=%3 isGameModeBefore=%4", w, count, api.GetNumSubScenes(), gameBefore);
		map<string, int> classes = new map<string, int>();
		array<string> order = {};
		int gameModes = 0;
		int nulls = 0;
		int noInstance = 0;
		int noInstanceGameModeClass = 0;
		int geometryTerrainCount = 0;
		IEntity geometryTerrain;
		for (int i = 0; i < count; i++)
		{
			IEntitySource src = api.GetEditorEntity(i);
			if (!src)
			{
				nulls++;
				continue;
			}
			string cls = src.GetClassName();
			if (m_bGeometry && cls == "GenericTerrainEntity")
			{
				geometryTerrainCount++;
			}
			string prefab = PrefabOf(src);
			if (i < MAX_ENTITIES)
				PrintFormat("[RICON_LIVEW] w=%1 entity=%2 class=%3 name=%4 prefab=%5", w, i, cls, src.GetName(), prefab);
			if (classes.Contains(cls))
				classes.Set(cls, classes.Get(cls) + 1);
			else
			{
				classes.Insert(cls, 1);
				order.Insert(cls);
			}
			// codex #4157: «нет BaseGameMode» отличаем от source без экземпляра в мире редактора
			IEntity instance = api.SourceToEntity(src);
			if (!instance)
			{
				noInstance++;
				if (cls.Contains("GameMode"))
					noInstanceGameModeClass++;
				continue;
			}
			BaseGameMode gameMode = BaseGameMode.Cast(instance);
			if (m_bGeometry && cls == "GenericTerrainEntity")
			{
				geometryTerrain = instance;
			}
			if (gameMode)
			{
				gameModes++;
				if (gameModes <= MAX_GAME_MODES)
				{
					PrintFormat("[RICON_LIVEW] w=%1 gameMode=%2 entity=%3 class=%4 prefab=%5", w, gameModes, i, cls, prefab);
					if (m_bDetail)
						PrintComponents(w, i, src);
				}
			}
			if (m_bDetail && cls == "EditorCommentEntity")
				PrintStringVars(w, i, src);
		}
		for (int c = 0; c < order.Count() && c < MAX_CLASSES; c++)
			PrintFormat("[RICON_LIVEW] w=%1 class=%2 count=%3", w, order[c], classes.Get(order[c]));
		PrintFormat("[RICON_LIVEW] w=%1 summary entities=%2 nullSources=%3 noInstance=%4 noInstanceGameModeClass=%5 distinctClasses=%6 gameModes=%7 isGameModeAfter=%8",
			w, count, nulls, noInstance, noInstanceGameModeClass, order.Count(), gameModes, api.IsGameMode());
		if (m_bGeometry)
		{
			ReadGeometry(api, w, geometryTerrain, geometryTerrainCount);
		}
	}

	//! Только пять фиксированных XZ кандидатов. Это не voice radius и не проверка безопасной постановки персонажа.
	protected bool GeometryFinite(float value)
	{
		return value <= float.MAX && value >= -float.MAX;
	}

	protected void GeometryResult(int w, string result, string reason, int samples)
	{
		PrintFormat("[RICON_LIVEG] terminal=1 w=%1 result=%2 reason=%3 samples=%4 scope=EDITOR_SOURCE voiceRange=UNKNOWN standingSafe=NOT_TESTED physics=NOT_TESTED", w, result, reason, samples);
	}

	protected void ReadGeometry(WorldEditorAPI api, int w, IEntity terrain, int terrainCount)
	{
		if (api.IsGameMode() || terrainCount != 1 || !terrain)
		{
			GeometryResult(w, "INCOMPLETE", "EDITOR_OR_TERRAIN_NOT_UNIQUE", 0);
			return;
		}
		BaseWorld geometryWorld = terrain.GetWorld();
		if (!geometryWorld)
		{
			GeometryResult(w, "INCOMPLETE", "NO_BASE_WORLD", 0);
			return;
		}
		vector boundsMin;
		vector boundsMax;
		geometryWorld.GetBoundBox(boundsMin, boundsMax);
		for (int axis = 0; axis < 3; axis++)
		{
			if (!GeometryFinite(boundsMin[axis]) || !GeometryFinite(boundsMax[axis]) || boundsMin[axis] > boundsMax[axis])
			{
				GeometryResult(w, "INCOMPLETE", "NONFINITE_OR_UNORDERED_BOUNDS", 0);
				return;
			}
		}
		if (boundsMin[0] >= boundsMax[0] || boundsMin[2] >= boundsMax[2])
		{
			GeometryResult(w, "INCOMPLETE", "EMPTY_XZ_BOUNDS", 0);
			return;
		}
		PrintFormat("[RICON_LIVEG] w=%1 bounds=1 minX=%2 minY=%3 minZ=%4 maxX=%5 maxY=%6 maxZ=%7", w, boundsMin[0], boundsMin[1], boundsMin[2], boundsMax[0], boundsMax[1], boundsMax[2]);
		array<vector> points = {"128 0 128", "228 0 128", "28 0 128", "128 0 228", "128 0 28"};
		// Весь fixed набор проверяется до первого height getter; невалидная точка не читается.
		for (int checkIndex = 0; checkIndex < 5; checkIndex++)
		{
			vector checkPoint = points[checkIndex];
			if (checkPoint[0] < boundsMin[0] || checkPoint[0] > boundsMax[0] || checkPoint[2] < boundsMin[2] || checkPoint[2] > boundsMax[2])
			{
				GeometryResult(w, "INCOMPLETE", "POINT_OUTSIDE_WORLD_XZ", 0);
				return;
			}
		}
		for (int pointIndex = 0; pointIndex < 5; pointIndex++)
		{
			if (api.IsGameMode())
			{
				GeometryResult(w, "INCOMPLETE", "EDITOR_CONTEXT_CHANGED", pointIndex);
				return;
			}
			vector samplePoint = points[pointIndex];
			float heightM = geometryWorld.GetSurfaceY(samplePoint[0], samplePoint[2]);
			if (!GeometryFinite(heightM))
			{
				GeometryResult(w, "INCOMPLETE", "NONFINITE_HEIGHT", pointIndex);
				return;
			}
			PrintFormat("[RICON_LIVEG] w=%1 point=%2 x=%3 z=%4 surfaceY=%5", w, pointIndex, samplePoint[0], samplePoint[2], heightM);
		}
		GeometryResult(w, "EDITOR_GEOMETRY_OBSERVED", "FIVE_FIXED_POINTS", 5);
	}

	static const int MAX_DETAIL = 40;        //!< классов компонентов GameMode
	static const int MAX_STRING_VARS = 8;    //!< строковых полей EditorCommentEntity (codex #4165)
	static const int MAX_VALUE_CHARS = 200;  //!< символов на значение (codex #4165)
	protected bool m_bDetail;
	protected bool m_bGeometry;

	//! Классы компонентов сущности (только чтение IEntitySource.GetComponentCount/GetComponent), не более MAX_DETAIL.
	protected void PrintComponents(int w, int entity, IEntitySource src)
	{
		int n = src.GetComponentCount();
		PrintFormat("[RICON_LIVEW] w=%1 entity=%2 components=%3", w, entity, n);
		for (int k = 0; k < n && k < MAX_DETAIL; k++)
		{
			IEntityComponentSource component = src.GetComponent(k);
			if (!component)
				continue;
			PrintFormat("[RICON_LIVEW] w=%1 entity=%2 component=%3 class=%4", w, entity, k, component.GetClassName());
			// B0d (codex #5061, #5065): override мира или подмена при запуске — два поля появления в мире и у префаба-предка
			if (component.GetClassName() == "SCR_RespawnSystemComponent")
			{
				PrintRespawnConfig(w, entity, k, "world", component);
				PrintRespawnConfig(w, entity, k, "ancestor", component.GetAncestor());
			}
		}
	}

	static const int MAX_OVERRIDE_NAMES = 16;  //!< имён полей в строке overrides (codex #5070)

	//! Поля появления контейнера компонента: m_bEnableRespawn (readOk, значение или UNKNOWN, задано ли здесь) и объект
	//! m_SpawnLogic (есть ли, класс, задан ли здесь). overrides — только имена полей, заданных именно в этом контейнере:
	//! так видно, что меняет мир, без угадывания прочих имён. Только getter-ы BaseContainer.
	protected void PrintRespawnConfig(int w, int entity, int component, string scope, BaseContainer node)
	{
		if (!node)
		{
			PrintFormat("[RICON_LIVEW] w=%1 entity=%2 component=%3 scope=%4 container=0", w, entity, component, scope);
			return;
		}
		bool enabled;
		bool readOk = node.Get("m_bEnableRespawn", enabled);
		string value = "UNKNOWN";
		if (readOk)
			value = enabled.ToString();
		BaseContainer logic = node.GetObject("m_SpawnLogic");
		string logicClass = "NONE";
		if (logic)
			logicClass = logic.GetClassName();
		PrintFormat("[RICON_LIVEW] w=%1 entity=%2 component=%3 scope=%4 container=1 enableReadOk=%5 enable=%6 enableSetDirect=%7 logic=%8 logicSetDirect=%9",
			w, entity, component, scope, readOk, value, node.IsVariableSetDirectly("m_bEnableRespawn"), logicClass, node.IsVariableSetDirectly("m_SpawnLogic"));
		// codex #5070: не более MAX_OVERRIDE_NAMES имён и MAX_VALUE_CHARS символов, остальное — числом more
		string overrides;
		int listed = 0;
		int more = 0;
		int n = node.GetNumVars();
		for (int v = 0; v < n; v++)
		{
			string name = node.GetVarName(v);
			if (!node.IsVariableSetDirectly(name))
				continue;
			if (listed >= MAX_OVERRIDE_NAMES || overrides.Length() + name.Length() + 1 > MAX_VALUE_CHARS)
			{
				more++;
				continue;
			}
			if (listed > 0)
				overrides += "|";
			overrides += name;
			listed++;
		}
		if (listed == 0)
			overrides = "-";
		PrintFormat("[RICON_LIVEW] w=%1 entity=%2 component=%3 scope=%4 vars=%5 overrides=%6 more=%7", w, entity, component, scope, n, overrides, more);
	}

	//! Строковые переменные сущности (GetDataVarType == STRING, тот же приём, что в RICON_AcpViewPlugin), не более MAX_STRING_VARS;
	//! переводы строк заменяются пробелом, значение обрезается до MAX_VALUE_CHARS, length — длина до обрезки.
	protected void PrintStringVars(int w, int entity, IEntitySource src)
	{
		int n = src.GetNumVars();
		int printed = 0;
		for (int v = 0; v < n && printed < MAX_STRING_VARS; v++)
		{
			string name = src.GetVarName(v);
			if (src.GetDataVarType(v) != DataVarType.STRING)
				continue;
			string value;
			if (!src.Get(name, value))
				continue;
			printed++;
			value.Replace("\r", " ");
			value.Replace("\n", " ");
			int length = value.Length();
			if (length > MAX_VALUE_CHARS)
				value = value.Substring(0, MAX_VALUE_CHARS);
			PrintFormat("[RICON_LIVEW] w=%1 entity=%2 var=%3 set=%4 length=%5 value=%6", w, entity, name, src.IsVariableSetDirectly(name), length, value);
		}
		PrintFormat("[RICON_LIVEW] w=%1 entity=%2 vars=%3 stringVarsPrinted=%4", w, entity, n, printed);
	}

	//! Префаб, от которого унаследована сущность мира (пусто, если сущность создана без префаба).
	protected string PrefabOf(IEntitySource src)
	{
		BaseContainer ancestor = src.GetAncestor();
		if (!ancestor)
			return "-";
		string name = ancestor.GetResourceName();
		if (name.IsEmpty())
			return "-";
		return name;
	}
}
