//! TOOL_ONLY, R27: только чтение одного узла графа von.acp через BaseContainerTools, без сохранения и правок.
//! Узел выбирается путём индексов детей (-riconAcpNode=2.0.1), поля — окном с -riconAcpVar=N.
//! Не больше 15 строк вывода за запуск, как у WbReader. Ресурс не сохраняется и не меняется.
[WorkbenchPluginAttribute(name: "RICON: узел графа ACP", description: "Ограниченное чтение одного узла von.acp", wbModules: {"ScriptEditor"})]
class RICON_AcpGraphReaderPlugin : WorkbenchPlugin
{
	static const ResourceName VON_ACP = "{B4C3941EC8B2AD20}Sounds/VON/von.acp";
	static const int MAX_VAR_LINES = 12;

	protected static string Value(BaseContainer node, int index)
	{
		string name = node.GetVarName(index);
		DataVarType type = node.GetDataVarType(index);
		string text = typename.EnumToString(DataVarType, type);
		if (!node.IsVariableSet(name))
			return text + " (не задано)";
		switch (type)
		{
			case DataVarType.SCALAR:
			{
				float f;
				if (node.Get(name, f))
					return text + " " + f.ToString(-1, 6);
				break;
			}
			case DataVarType.INTEGER:
			case DataVarType.FLAGS:
			{
				int i;
				if (node.Get(name, i))
					return text + " " + i.ToString();
				break;
			}
			case DataVarType.BOOLEAN:
			{
				bool b;
				if (node.Get(name, b))
					return text + " " + b.ToString();
				break;
			}
			case DataVarType.STRING:
			{
				string s;
				if (node.Get(name, s))
					return text + " \"" + s + "\"";
				break;
			}
			case DataVarType.RESOURCE_NAME:
			{
				ResourceName r;
				if (node.Get(name, r))
					return text + " " + r;
				break;
			}
			case DataVarType.VECTOR3:
			{
				vector v;
				if (node.Get(name, v))
					return text + " " + v.ToString();
				break;
			}
			case DataVarType.OBJECT:
			{
				BaseContainer child = node.GetObject(name);
				if (child)
					return text + " class=" + child.GetClassName();
				return text + " null";
			}
			case DataVarType.ARRAY:
			{
				BaseContainerList list = node.GetObjectArray(name);
				if (list)
					return text + " count=" + list.Count();
				break;
			}
		}
		return text;
	}

	override void RunCommandline()
	{
		string enabled;
		if (!System.GetCLIParam("riconAcpRead", enabled) || enabled != "yes")
		{
			Print("[RICON_ACP27] BLOCKED: явный режим чтения не указан");
			Workbench.Exit(6);
			return;
		}
		Resource resource = BaseContainerTools.LoadContainer(VON_ACP);
		if (!resource || !resource.IsValid() || !resource.GetResource())
		{
			Print("[RICON_ACP27] INCOMPLETE: ресурс не загружен");
			Workbench.Exit(4);
			return;
		}
		BaseContainer node = resource.GetResource().ToBaseContainer();
		if (!node)
		{
			Print("[RICON_ACP27] INCOMPLETE: корневой контейнер не получен");
			Workbench.Exit(4);
			return;
		}
		string path;
		System.GetCLIParam("riconAcpNode", path);
		array<string> steps = {};
		path.Split(".", steps, true);
		foreach (string step : steps)
		{
			int childIndex = step.ToInt(-1);
			if (childIndex < 0 || childIndex >= node.GetNumChildren())
			{
				PrintFormat("[RICON_ACP27] FAIL: нет ребёнка %1 у %2", step, node.GetClassName());
				Workbench.Exit(3);
				return;
			}
			node = node.GetChild(childIndex);
		}
		string firstText;
		System.GetCLIParam("riconAcpVar", firstText);
		int first = Math.Max(0, firstText.ToInt(0));
		int vars = node.GetNumVars();
		PrintFormat("[RICON_ACP27] node=\"%1\" class=%2 name=\"%3\" vars=%4 children=%5 varFrom=%6", path,
			node.GetClassName(), node.GetName(), vars, node.GetNumChildren(), first);
		string show;
		System.GetCLIParam("riconAcpShow", show);
		if (show == "children")
		{
			for (int c = first; c < node.GetNumChildren() && c < first + MAX_VAR_LINES; c++)
			{
				BaseContainer child = node.GetChild(c);
				PrintFormat("[RICON_ACP27] child %1 class=%2 name=\"%3\" vars=%4 children=%5", c, child.GetClassName(),
					child.GetName(), child.GetNumVars(), child.GetNumChildren());
			}
		}
		else
		{
			for (int k = first; k < vars && k < first + MAX_VAR_LINES; k++)
				PrintFormat("[RICON_ACP27] var %1 %2 = %3", k, node.GetVarName(k), Value(node, k));
		}
		Print("[RICON_ACP27] done: READ_ONLY, ресурс не сохранялся");
		Workbench.Exit(0);
	}
}
