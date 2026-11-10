//! Pure Game checks и отдельное native readonly определение profile alias без открытия файла.
[WorkbenchPluginAttribute(name: "RICON: Check held file capability", wbModules: {"ScriptEditor"})]
class RICON_LiveFileHandleCapabilityCheckPlugin : WorkbenchPlugin
{
	protected void ReportAlias()
	{
		string absolute; string profile;
		if (!Workbench.GetAbsolutePath("$profile:RICON_FileHandleCapability_alias-probe.txt", absolute, false))
		{
			Print("[RICON_FHC_ALIAS] scope=NATIVE_READONLY outcome=UNKNOWN reason=RESOLVE_FAILED fileOpened=0"); return;
		}
		if (!System.GetCLIParam("profile", profile) || profile.IsEmpty())
		{
			Print("[RICON_FHC_ALIAS] scope=NATIVE_READONLY outcome=UNKNOWN reason=NO_CLI_PROFILE fileOpened=0"); return;
		}
		absolute.Replace("\\", "/"); profile.Replace("\\", "/"); absolute.ToLower(); profile.ToLower();
		if (profile.Length() < 4 || absolute.Length() < 4 || profile.Substring(1, 2) != ":/" || absolute.Substring(1, 2) != ":/"
			|| profile.IndexOf("..") >= 0 || absolute.IndexOf("..") >= 0 || profile.IndexOf("//") >= 0 || absolute.IndexOf("//") >= 0
			|| profile.IndexOf("/./") >= 0 || absolute.IndexOf("/./") >= 0)
		{
			Print("[RICON_FHC_ALIAS] scope=NATIVE_READONLY outcome=UNKNOWN reason=UNSUPPORTED_PATH_FORM fileOpened=0"); return;
		}
		if (profile.Substring(profile.Length() - 1, 1) == "/") profile = profile.Substring(0, profile.Length() - 1);
		string prefix = profile + "/";
		if (absolute.Length() <= prefix.Length() || absolute.Substring(0, prefix.Length()) != prefix)
		{
			Print("[RICON_FHC_ALIAS] scope=NATIVE_READONLY outcome=OUTSIDE fileOpened=0"); return;
		}
		string relative = absolute.Substring(prefix.Length(), absolute.Length() - prefix.Length());
		PrintFormat("[RICON_FHC_ALIAS] scope=NATIVE_READONLY outcome=INSIDE_ROOT relative=%1 fileOpened=0", RICON_LiveText.Esc(relative));
	}
	override void RunCommandline()
	{
		RICON_FileHandleCapabilityChecks checks = new RICON_FileHandleCapabilityChecks();
		checks.Run();
		ReportAlias();
	}
}
