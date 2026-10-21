//! TOOL_ONLY: открыть von.acp или Von.sig в GUI Audio Editor для просмотра (разрешение владельца в чате claude
//! 2026-10-21: «если можешь сам используй audio editor»). Только OpenResource/OpenModule/SetOpenedResource: без Save,
//! Set, ExecuteAction и Exit. Формы имени перебираются до первой удачной; каждая попытка печатается одной строкой.
//! -riconAcpView=acp|sig выбирает ресурс; без него Workbench сразу завершается с кодом 6.
//! Окно закрывает запускающий скрипт по PID, без сохранения.
[WorkbenchPluginAttribute(name: "RICON: открыть von.acp", description: "Открыть von.acp или Von.sig в Audio Editor только для просмотра", wbModules: {"ScriptEditor"})]
class RICON_AcpViewPlugin : WorkbenchPlugin
{
	static const ref array<string> ACP_NAMES = {"Sounds/VON/von.acp", "{B4C3941EC8B2AD20}Sounds/VON/von.acp",
		"$ArmaReforger:Sounds/VON/von.acp"};
	static const ref array<string> SIG_NAMES = {"Sounds/VON/Von.sig", "$ArmaReforger:Sounds/VON/Von.sig"};

	//! Только чтение открытого ресурса: класс и имя корня, затем по строке на узел с заданными числовыми полями.
	//! Не больше 14 строк данных; каждая строка проходит RICON_AcpGraphReaderPlugin.Escape.
	protected void ReadOpened()
	{
		WBModuleDef editor = Workbench.GetModule(AudioEditor);
		if (!editor)
		{
			Print("[RICON_ACPVIEW] INCOMPLETE: модуль не получен");
			return;
		}
		int count = editor.GetNumContainers();
		BaseContainer root = editor.GetContainer(0);
		if (!root)
		{
			Print("[RICON_ACPVIEW] INCOMPLETE: containers=" + count.ToString() + ", корень не получен");
			return;
		}
		int children = root.GetNumChildren();
		Print("[RICON_ACPVIEW] " + RICON_AcpGraphReaderPlugin.Escape("containers=" + count.ToString() + " root=" + root.GetClassName()
			+ " vars=" + root.GetNumVars().ToString() + " children=" + children.ToString()));
		for (int c = 0; c < children && c < 13; c++)
		{
			BaseContainer node = root.GetChild(c);
			if (!node)
				continue;
			string line = c.ToString() + " " + node.GetClassName() + " \"" + node.GetName() + "\"";
			for (int v = 0; v < node.GetNumVars(); v++)
			{
				string varName = node.GetVarName(v);
				if (!node.IsVariableSet(varName))
					continue;
				DataVarType type = node.GetDataVarType(v);
				float f;
				int i;
				string s;
				if (type == DataVarType.SCALAR && node.Get(varName, f))
					line += " " + varName + "=" + f.ToString(-1, 6);
				else if (type == DataVarType.INTEGER && node.Get(varName, i))
					line += " " + varName + "=" + i.ToString();
				else if (type == DataVarType.STRING && node.Get(varName, s))
					line += " " + varName + "=\"" + s + "\"";
			}
			Print("[RICON_ACPVIEW] " + RICON_AcpGraphReaderPlugin.Escape(line));
		}
	}

	override void RunCommandline()
	{
		string mode;
		System.GetCLIParam("riconAcpView", mode);
		array<string> names;
		if (mode == "acp")
			names = ACP_NAMES;
		else if (mode == "sig")
			names = SIG_NAMES;
		if (!names)
		{
			Print("[RICON_ACPVIEW] BLOCKED: явный режим просмотра не указан");
			Workbench.Exit(6);
			return;
		}
		foreach (string name : names)
		{
			bool opened = Workbench.OpenResource(name);
			Print("[RICON_ACPVIEW] OpenResource=" + opened.ToString() + " name=" + name);
			if (opened)
			{
				string read;
				if (System.GetCLIParam("riconAcpViewRead", read) && read == "yes")
				{
					ReadOpened();
					if (GetGame() && GetGame().GetCallqueue())
						GetGame().GetCallqueue().CallLater(ReadOpened, 15000, false);
					else
						Print("[RICON_ACPVIEW] INCOMPLETE: нет очереди вызовов для повторного чтения");
				}
				return;
			}
		}
		bool module = Workbench.OpenModule(AudioEditor);
		WBModuleDef editor = Workbench.GetModule(AudioEditor);
		bool hasEditor = editor != null;
		Print("[RICON_ACPVIEW] OpenModule=" + module.ToString() + " module=" + hasEditor.ToString());
		if (!editor)
			return;
		foreach (string name : names)
		{
			bool assigned = editor.SetOpenedResource(name);
			Print("[RICON_ACPVIEW] SetOpenedResource=" + assigned.ToString() + " name=" + name);
			if (assigned)
				return;
		}
	}
}
