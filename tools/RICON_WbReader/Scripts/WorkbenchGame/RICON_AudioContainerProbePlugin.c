//! TOOL_ONLY, кандидат R26. Читать только метаданные указанного ACP после взаимного ревью механизма.
[WorkbenchPluginAttribute(name: "RICON: метаданные ACP", description: "Ограниченное чтение корневых полей, без сохранения", wbModules: {"AudioEditor"})]
class RICON_AudioContainerProbePlugin : WorkbenchPlugin
{
	override void RunCommandline()
	{
		string enabled;
		if (!System.GetCLIParam("riconAcpInspect", enabled) || enabled != "yes")
		{
			Print("[RICON_ACP] BLOCKED: явный режим чтения не указан");
			Workbench.Exit(6);
			return;
		}
		WBModuleDef editor = Workbench.GetModule(AudioEditor);
		if (!editor || !editor.SetOpenedResource("$ArmaReforger:Sounds/VON/von.acp"))
		{
			Print("[RICON_ACP] INCOMPLETE: модуль или ресурс недоступен");
			Workbench.Exit(4);
			return;
		}
		int count = editor.GetNumContainers();
		BaseContainer root;
		if (count > 0)
			root = editor.GetContainer(0);
		if (!root)
		{
			Print("[RICON_ACP] INCOMPLETE: корневой контейнер не получен");
			Workbench.Exit(4);
			return;
		}
		PrintFormat("[RICON_ACP] containers=%1 class=%2 vars=%3", count, root.GetClassName(), root.GetNumVars());
		// shortcut: только 14 имён корневых полей, без значений и обхода графа; расширять после отдельного ревью.
		for (int i = 0; i < root.GetNumVars() && i < 14; i++)
			PrintFormat("[RICON_ACP] field=%1", root.GetVarName(i));
		Print("[RICON_ACP] METADATA_ONLY: граф и качество звука не проверены");
		Workbench.Exit(0);
	}
}
