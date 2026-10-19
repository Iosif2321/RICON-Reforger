//! TOOL_ONLY: read-only lookup of game resources through Workbench for Workbench orders (docs/evidence/WB-order-*.md).
//! Lists resources with ResourceDatabase.SearchResources, opens them with FileMode.READ only and prints matching
//! lines with context to the Workbench log.
//! It never copies, writes or unpacks resources; whole files are not printed.
//!
//! Command line (with -wbModule=ScriptEditor -plugin=RICON_WbReaderPlugin):
//!   -riconRoot=<exact path, e.g. "$ArmaReforger:Prefabs">   search root (optional, "-" = whole database)
//!   -riconExt=<extension without dot, e.g. "c">             file extension (required)
//! Values cannot contain spaces on the command line; write a space as %20.
//!   -riconPath=<substring>                                 keep files whose path contains it (optional)
//!   -riconGrep=<substring>                                 print lines containing it (required)
//!   -riconContext=<N>                                      lines of context on each side (default 3, clamped to 0..7)
//!   -riconMaxFiles=<N>                                     stop after N matching files (default 10, at least 1)
//! Output is hard-capped at MAX_LINES_TOTAL source lines per run (agora #767), so a frequent needle cannot print
//! a whole file.
[WorkbenchPluginAttribute(name: "RICON WB Reader", description: "Read-only resource lookup (TOOL_ONLY)", wbModules: {"ScriptEditor", "ResourceManager"})]
class RICON_WbReaderPlugin : WorkbenchPlugin
{
	static const int MAX_CONTEXT = 7;
	static const int MAX_LINES_TOTAL = 15;
	static const string TAG = "[RICON_WbReader] ";

	protected ref array<string> m_aFiles = {};
	protected string m_sPathFilter;
	protected int m_iPrintedTotal;

	//! SearchResourcesCallback: resource name and its exact path.
	protected void OnResourceFound(ResourceName resourceName, string exactPath = "")
	{
		// Scripts have no resource GUID, so GetPath() is empty for them; the exact path is always set.
		string path = exactPath;
		if (path.IsEmpty())
			path = resourceName.GetPath();

		if (!m_sPathFilter.IsEmpty() && !path.Contains(m_sPathFilter))
			return;

		m_aFiles.Insert(path);
	}

	protected static string ReadParam(string name, string fallback)
	{
		string value;
		if (System.GetCLIParam(name, value) && !value.IsEmpty())
		{
			value.Replace("%20", " ");
			return value;
		}

		return fallback;
	}

	protected void PrintMatches(string path, string needle, int context, out int matchCount)
	{
		matchCount = 0;
		FileHandle handle = FileIO.OpenFile(path, FileMode.READ);
		if (!handle)
		{
			Print(TAG + "cannot open " + path, LogLevel.WARNING);
			return;
		}

		array<string> lines = {};
		string line;
		while (handle.ReadLine(line) >= 0)
		{
			lines.Insert(line);
		}

		handle.Close();

		int lastPrinted = -1;
		for (int lineIndex = 0; lineIndex < lines.Count(); lineIndex++)
		{
			if (!lines[lineIndex].Contains(needle))
				continue;

			matchCount++;
			int first = Math.Max(lineIndex - context, lastPrinted + 1);
			int last = Math.Min(lineIndex + context, lines.Count() - 1);
			if (first > lastPrinted + 1)
				Print(TAG + "---- " + path);

			for (int printIndex = first; printIndex <= last; printIndex++)
			{
				if (m_iPrintedTotal >= MAX_LINES_TOTAL)
				{
					Print(TAG + "line cap reached in " + path + ", further matches not printed");
					return;
				}

				Print(TAG + path + ":" + (printIndex + 1).ToString() + ": " + lines[printIndex]);
				m_iPrintedTotal++;
			}

			lastPrinted = last;
		}
	}

	override void RunCommandline()
	{
		string root = ReadParam("riconRoot", "");
		string ext = ReadParam("riconExt", "");
		string needle = ReadParam("riconGrep", "");
		m_sPathFilter = ReadParam("riconPath", "");
		int context = Math.ClampInt(ReadParam("riconContext", "3").ToInt(), 0, MAX_CONTEXT);
		int maxFiles = Math.Max(ReadParam("riconMaxFiles", "10").ToInt(), 1);

		if (root == "-")
			root = "";

		if (ext.IsEmpty() || needle.IsEmpty())
		{
			Print(TAG + "missing -riconExt or -riconGrep", LogLevel.ERROR);
			Workbench.Exit(2);
			return;
		}

		Print(TAG + "root=" + root + " ext=" + ext + " path~" + m_sPathFilter + " grep=" + needle);
		SearchResourcesFilter filter = new SearchResourcesFilter();
		filter.rootPath = root;
		filter.fileExtensions = {ext};
		filter.recursive = true;
		bool listed = ResourceDatabase.SearchResources(filter, OnResourceFound);
		Print(TAG + "SearchResources=" + listed.ToString() + ", candidate files: " + m_aFiles.Count().ToString());

		int matchedFiles = 0;
		foreach (string path : m_aFiles)
		{
			int matchCount;
			PrintMatches(path, needle, context, matchCount);
			if (matchCount > 0)
				matchedFiles++;

			if (matchedFiles >= maxFiles || m_iPrintedTotal >= MAX_LINES_TOTAL)
				break;
		}

		Print(TAG + "done, files with matches: " + matchedFiles.ToString() + ", lines printed: " + m_iPrintedTotal.ToString());
		Workbench.Exit(0);
	}
}
