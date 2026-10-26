//! TOOL_ONLY, только чтение: состав миров-кандидатов для стенда A0 (codex #4149, #4152).
//! Для каждого мира из параметров -riconListWorld1="…" … -riconListWorld4="…" World Editor открывает мир и печатает
//! opened, IsGameMode до/после, число сущностей, первые 15 сущностей (класс, имя, префаб предка), до 15 различных
//! классов со счётчиками и сущности, приводимые к BaseGameMode. Не вызывает SwitchToGameMode, CreateEntity, Save,
//! не меняет мир и не запускает автотесты. Документированные вызовы (Script API WorldEditorAPI, IEntitySource):
//! GetEditorEntityCount, GetEditorEntity, SourceToEntity, GetNumSubScenes, IsGameMode; BaseContainer.GetClassName,
//! GetName, GetAncestor; BaseResourceObject.GetResourceName.
//! B0d (codex #5061): при -riconListDetail=1 у компонента RespawnSystem GameMode печатаются все поля в мире и у предка
//! (GetNumVars, GetVarName, GetDataVarType, IsVariableSetDirectly, IsVariableSet, Get, GetObject), объекты — на уровень ниже.
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
		string detail;
		m_bDetail = System.GetCLIParam("riconListDetail", detail) && detail == "1";
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
	}

	static const int MAX_DETAIL = 40;        //!< классов компонентов GameMode
	static const int MAX_STRING_VARS = 8;    //!< строковых полей EditorCommentEntity (codex #4165)
	static const int MAX_VALUE_CHARS = 200;  //!< символов на значение (codex #4165)
	protected bool m_bDetail;

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
			// B0d (codex #5061): override мира или подмена при запуске — поля появления в мире и у префаба-предка
			if (component.GetClassName().Contains("RespawnSystem"))
			{
				PrintVars(w, entity, k, "world", component, 1);
				PrintVars(w, entity, k, "ancestor", component.GetAncestor(), 1);
			}
		}
	}

	static const int MAX_SPAWN_VARS = 24;    //!< полей на контейнер компонента появления (B0d)

	//! Все поля контейнера: тип, задано ли здесь (setDirect) или по цепочке предков (set), значение; у OBJECT — класс и,
	//! при depth > 0, его собственные поля на уровень ниже. Только getter-ы BaseContainer.
	protected void PrintVars(int w, int entity, int component, string scope, BaseContainer node, int depth)
	{
		if (!node)
		{
			PrintFormat("[RICON_LIVEW] w=%1 entity=%2 component=%3 scope=%4 container=NONE", w, entity, component, scope);
			return;
		}
		int n = node.GetNumVars();
		PrintFormat("[RICON_LIVEW] w=%1 entity=%2 component=%3 scope=%4 class=%5 vars=%6", w, entity, component, scope, node.GetClassName(), n);
		for (int v = 0; v < n && v < MAX_SPAWN_VARS; v++)
		{
			string name = node.GetVarName(v);
			DataVarType type = node.GetDataVarType(v);
			PrintFormat("[RICON_LIVEW] w=%1 entity=%2 component=%3 scope=%4 var=%5 type=%6 setDirect=%7 set=%8 value=%9", w, entity, component, scope,
				name, typename.EnumToString(DataVarType, type), node.IsVariableSetDirectly(name), node.IsVariableSet(name), VarValue(node, name, type));
			if (type == DataVarType.OBJECT && depth > 0)
				PrintVars(w, entity, component, scope + "." + name, node.GetObject(name), depth - 1);
		}
	}

	//! Значение поля одной строкой: числа, флаги, строки и ресурсы как есть (переводы строк заменяются пробелом), OBJECT —
	//! класс или null; иное — "-".
	protected static string VarValue(BaseContainer node, string name, DataVarType type)
	{
		switch (type)
		{
			case DataVarType.BOOLEAN:
			{
				bool b;
				if (node.Get(name, b))
					return b.ToString();
				break;
			}
			case DataVarType.INTEGER:
			case DataVarType.FLAGS:
			{
				int i;
				if (node.Get(name, i))
					return i.ToString();
				break;
			}
			case DataVarType.SCALAR:
			{
				// без явной точности: ToString(-1, 9) у значений около FLT_MAX переполнял стек VM
				float f;
				if (node.Get(name, f))
					return f.ToString();
				break;
			}
			case DataVarType.STRING:
			{
				string s;
				if (node.Get(name, s))
				{
					s.Replace("\r", " ");
					s.Replace("\n", " ");
					if (s.Length() > MAX_VALUE_CHARS)
						s = s.Substring(0, MAX_VALUE_CHARS);
					return "\"" + s + "\"";
				}
				break;
			}
			case DataVarType.RESOURCE_NAME:
			{
				ResourceName r;
				if (node.Get(name, r))
					return r;
				break;
			}
			case DataVarType.OBJECT:
			{
				BaseContainer child = node.GetObject(name);
				if (child)
					return "class=" + child.GetClassName();
				return "null";
			}
		}
		// ARRAY не раскрывается: GetObjectArray годится только для массивов объектов
		return "-";
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
