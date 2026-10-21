//! TOOL_ONLY, R27: только чтение одного узла графа von.acp через BaseContainerTools, без сохранения и правок.
//! Узел выбирается путём индексов детей (-riconAcpNode=2.0.1), окно полей или детей — с -riconAcpVar=N.
//! Каждая строка проходит Emit: управляющие символы экранируются, длина ограничена, строк не больше 15.
//! -riconAcpSelfTest=yes проверяет форматирование без загрузки ресурса.
[WorkbenchPluginAttribute(name: "RICON: узел графа ACP", description: "Ограниченное чтение одного узла von.acp", wbModules: {"ScriptEditor"})]
class RICON_AcpGraphReaderPlugin : WorkbenchPlugin
{
	static const ResourceName VON_ACP = "{B4C3941EC8B2AD20}Sounds/VON/von.acp";
	static const int MAX_ITEM_LINES = 12;
	static const int MAX_LINES = 15;
	static const int MAX_LINE_CHARS = 256;
	protected int m_iLines;

	//! Одна физическая строка: CR, LF, tab, обратная косая и прочие управляющие символы экранируются,
	//! после MAX_LINE_CHARS символов вывод обрезается с явной меткой.
	static string Escape(string text)
	{
		string result;
		int length = text.Length();
		for (int i = 0; i < length; i++)
		{
			if (result.Length() >= MAX_LINE_CHARS)
				return result + "...[cut " + (length - i).ToString() + "]";
			int code = text.ToAscii(i);
			if (code == 10)
				result += "\\n";
			else if (code == 13)
				result += "\\r";
			else if (code == 9)
				result += "\\t";
			else if (code == 92)
				result += "\\\\";
			else if (code >= 0 && code < 32)
				result += "\\x" + code.ToString();
			else
				result += text.Get(i);
		}
		return result;
	}

	protected bool Emit(string line)
	{
		if (m_iLines >= MAX_LINES)
			return false;
		m_iLines++;
		Print("[RICON_ACP27] " + Escape(line));
		return true;
	}

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
					return text + " count=" + list.Count().ToString();
				break;
			}
		}
		return text;
	}

	//! Проверка Escape без ресурса: многострочный, табулированный и длинный ввод.
	protected bool SelfTest()
	{
		string multi = Escape("a\nb\rc\td\\e");
		string longText;
		for (int i = 0; i < 300; i++)
			longText += "x";
		string cut = Escape(longText + "\n");
		bool ok = multi == "a\\nb\\rc\\td\\\\e" && !cut.Contains("\n")
			&& cut.Length() <= MAX_LINE_CHARS + 16 && cut.Contains("...[cut 45]");
		Emit("selftest multi=" + multi);
		Emit("selftest cutLength=" + cut.Length().ToString() + " ok=" + ok.ToString());
		return ok;
	}

	protected void Finish(string message, int code)
	{
		Emit(message);
		Workbench.Exit(code);
	}

	override void RunCommandline()
	{
		string mode;
		if (System.GetCLIParam("riconAcpSelfTest", mode) && mode == "yes")
		{
			if (SelfTest())
				Finish("done: SELFTEST, ресурс не загружался", 0);
			else
				Finish("FAIL: selftest", 5);
			return;
		}
		if (!System.GetCLIParam("riconAcpRead", mode) || mode != "yes")
		{
			Finish("BLOCKED: явный режим чтения не указан", 6);
			return;
		}
		Resource resource = BaseContainerTools.LoadContainer(VON_ACP);
		if (!resource || !resource.IsValid() || !resource.GetResource())
		{
			Finish("INCOMPLETE: ресурс не загружен", 4);
			return;
		}
		BaseContainer node = resource.GetResource().ToBaseContainer();
		if (!node)
		{
			Finish("INCOMPLETE: корневой контейнер не получен", 4);
			return;
		}
		string path;
		System.GetCLIParam("riconAcpNode", path);
		array<string> steps = {};
		path.Split(".", steps, true);
		foreach (string step : steps)
		{
			int childIndex = step.ToInt(-1);
			BaseContainer next;
			if (childIndex >= 0 && childIndex < node.GetNumChildren())
				next = node.GetChild(childIndex);
			if (!next)
			{
				Finish("FAIL: нет ребёнка " + step + " у " + node.GetClassName(), 3);
				return;
			}
			node = next;
		}
		string firstText;
		System.GetCLIParam("riconAcpVar", firstText);
		int first = Math.Max(0, firstText.ToInt(0));
		int vars = node.GetNumVars();
		int children = node.GetNumChildren();
		Emit("node=\"" + path + "\" class=" + node.GetClassName() + " name=\"" + node.GetName() + "\" vars=" + vars.ToString()
			+ " children=" + children.ToString() + " from=" + first.ToString());
		string show;
		System.GetCLIParam("riconAcpShow", show);
		if (show == "children")
		{
			for (int c = first; c < children && c < first + MAX_ITEM_LINES; c++)
			{
				BaseContainer child = node.GetChild(c);
				if (!child)
					Emit("child " + c.ToString() + " null");
				else
					Emit("child " + c.ToString() + " class=" + child.GetClassName() + " name=\"" + child.GetName()
						+ "\" vars=" + child.GetNumVars().ToString() + " children=" + child.GetNumChildren().ToString());
			}
		}
		else
		{
			for (int k = first; k < vars && k < first + MAX_ITEM_LINES; k++)
				Emit("var " + k.ToString() + " " + node.GetVarName(k) + " = " + Value(node, k));
		}
		Finish("done: READ_ONLY, ресурс не сохранялся", 0);
	}
}
