//! TOOL_ONLY: метаданные ресурсов без чтения их содержимого (codex #4093). ResourceDatabase.SearchResources отдаёт в
//! обратный вызов ResourceName (с GUID самого ресурса) и точный путь; печатаются только эти два поля:
//! префабы раций (Prefabs/Items/Equipment/Radios, .et) — для Resource.Load точного аппарата стендом Play (GUID в логе
//! identity принадлежит родителю), и миры (worlds, .ent) — чтобы выбрать маленький мир для стенда.
//! Без мира, Play и сервера; файлы не открываются; вывод ограничен MAX_LINES строками на корень.
[WorkbenchPluginAttribute(name: "RICON: ResourceName раций и миров", description: "GUID и путь без чтения содержимого", wbModules: {"ScriptEditor"})]
class RICON_LiveResourceProbePlugin : WorkbenchPlugin
{
	static const int MAX_LINES = 200;
	protected string m_sTag;
	protected int m_iFound;
	protected int m_iPrinted;

	protected void OnResourceFound(ResourceName resourceName, string exactPath = "")
	{
		m_iFound++;
		if (m_iPrinted >= MAX_LINES)
			return;
		m_iPrinted++;
		// PrintFormat, а не Print(переменная): последний печатает отладочный вид «string x = '…'».
		PrintFormat("[RICON_LIVERES] kind=%1 resource=%2 path=%3", m_sTag, resourceName, exactPath);
	}

	protected bool List(string tag, string root, string ext)
	{
		m_sTag = tag;
		m_iFound = 0;
		m_iPrinted = 0;
		SearchResourcesFilter filter = new SearchResourcesFilter();
		filter.rootPath = root;
		filter.fileExtensions = {ext};
		filter.recursive = true;
		bool listed = ResourceDatabase.SearchResources(filter, OnResourceFound);
		PrintFormat("[RICON_LIVERES] kind=%1 root=%2 listed=%3 found=%4 printed=%5", tag, root, listed, m_iFound, m_iPrinted);
		return listed && m_iFound > 0;
	}

	override void RunCommandline()
	{
		bool radios = List("radio", "$ArmaReforger:Prefabs/Items/Equipment/Radios/", "et");
		bool worlds = List("world", "$ArmaReforger:worlds/", "ent");
		if (radios && worlds)
		{
			Print("[RICON_LIVERES] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_LIVERES] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
