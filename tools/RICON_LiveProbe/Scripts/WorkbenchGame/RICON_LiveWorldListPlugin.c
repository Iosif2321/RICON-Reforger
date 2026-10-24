//! TOOL_ONLY, только чтение: состав миров-кандидатов для стенда A0 (codex #4149, #4152).
//! Для каждого мира из параметров -riconListWorld1="…" … -riconListWorld4="…" World Editor открывает мир и печатает
//! opened, IsGameMode до/после, число сущностей, первые 15 сущностей (класс, имя, префаб предка), до 15 различных
//! классов со счётчиками и сущности, приводимые к BaseGameMode. Не вызывает SwitchToGameMode, CreateEntity, Save,
//! не меняет мир и не запускает автотесты. Документированные вызовы (Script API WorldEditorAPI, IEntitySource):
//! GetEditorEntityCount, GetEditorEntity, SourceToEntity, GetNumSubScenes, IsGameMode; BaseContainer.GetClassName,
//! GetName, GetAncestor; BaseResourceObject.GetResourceName.
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
		WorldEditor editor = Workbench.GetModule(WorldEditor);
		if (!editor)
		{
			Print("[RICON_LIVEW] state=NO_WORLD_EDITOR");
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
			Print("[RICON_LIVEW] state=NO_WORLD_ARGUMENT tried=System.GetCLIParam");
		PrintFormat("[RICON_LIVEW] state=DONE worlds=%1 saved=0 play=0", listed);
	}

	protected void ListWorld(WorldEditor editor, int w, string world)
	{
		bool opened = editor.SetOpenedResource(world);
		PrintFormat("[RICON_LIVEW] w=%1 state=OPEN world=%2 opened=%3", w, world, opened);
		if (!opened)
			return;
		WorldEditorAPI api = editor.GetApi();
		if (!api)
		{
			PrintFormat("[RICON_LIVEW] w=%1 state=NO_API", w);
			return;
		}
		bool gameBefore = api.IsGameMode();
		int count = api.GetEditorEntityCount();
		PrintFormat("[RICON_LIVEW] w=%1 entities=%2 subScenes=%3 isGameModeBefore=%4", w, count, api.GetNumSubScenes(), gameBefore);
		map<string, int> classes = new map<string, int>();
		array<string> order = {};
		int gameModes = 0;
		int nulls = 0;
		int noInstance = 0;
		int noInstanceGameModeClass = 0;
		for (int i = 0; i < count; i++)
		{
			IEntitySource src = api.GetEditorEntity(i);
			if (!src)
			{
				nulls++;
				continue;
			}
			string cls = src.GetClassName();
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
			if (gameMode)
			{
				gameModes++;
				if (gameModes <= MAX_GAME_MODES)
					PrintFormat("[RICON_LIVEW] w=%1 gameMode=%2 entity=%3 class=%4 prefab=%5", w, gameModes, i, cls, prefab);
			}
		}
		for (int c = 0; c < order.Count() && c < MAX_CLASSES; c++)
			PrintFormat("[RICON_LIVEW] w=%1 class=%2 count=%3", w, order[c], classes.Get(order[c]));
		PrintFormat("[RICON_LIVEW] w=%1 summary entities=%2 nullSources=%3 noInstance=%4 noInstanceGameModeClass=%5 distinctClasses=%6 gameModes=%7 isGameModeAfter=%8",
			w, count, nulls, noInstance, noInstanceGameModeClass, order.Count(), gameModes, api.IsGameMode());
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
