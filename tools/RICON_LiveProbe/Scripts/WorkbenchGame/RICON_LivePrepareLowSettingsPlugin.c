//! Opt-in fresh-own-config preparation only. Root must verify canonical paths, reparse and freshness before execution.
[WorkbenchPluginAttribute(name: "RICON: Prepare fresh Low settings", wbModules: {"ScriptEditor"})]
class RICON_LivePrepareLowSettingsPlugin : WorkbenchPlugin
{
	protected void Refuse(string reason)
	{
		PrintFormat("[RICON_LOWPREP] failure=%1 disk=UNVERIFIED", reason);
		Print("[RICON_LOWPREP] done: FAILED");
	}
	protected bool OwnCfg(string profile, string cfg)
	{
		if (profile.IsEmpty() || cfg.IsEmpty() || profile.IndexOf("..") >= 0 || cfg.IndexOf("..") >= 0) return false;
		profile.Replace("\\", "/"); cfg.Replace("\\", "/"); profile.ToLower(); cfg.ToLower();
		if (profile.Length() < 4 || cfg.Length() < 4 || profile.Substring(1, 2) != ":/" || cfg.Substring(1, 2) != ":/") return false;
		if (profile.IndexOf("//") >= 0 || cfg.IndexOf("//") >= 0 || profile.IndexOf("/./") >= 0 || cfg.IndexOf("/./") >= 0) return false;
		if (profile.Substring(profile.Length() - 1, 1) == "/") profile = profile.Substring(0, profile.Length() - 1);
		string prefix = profile + "/";
		return cfg.Length() > prefix.Length() && cfg.Substring(0, prefix.Length()) == prefix;
	}
	protected bool Integral(float value)
	{
		return value >= -2147483648.0 && value < 2147483648.0 && Math.Floor(value) == value;
	}
	override void RunCommandline()
	{
		string opt; string profile; string cfg;
		if (!System.GetCLIParam("riconPrepareLow", opt) || opt != "1") { Print("[RICON_LOWPREP] done: DISABLED"); return; }
		if (!System.GetCLIParam("profile", profile) || !System.GetCLIParam("cfg", cfg) || !OwnCfg(profile, cfg)) { Refuse("OWN_CFG_GUARD"); return; }
		Game game = GetGame();
		if (!game) { Refuse("NO_GAME_CONTEXT"); return; }
		UserSettings settings = game.GetEngineUserSettings();
		if (!settings) { Refuse("NO_ENGINE_USER_SETTINGS"); return; }
		BaseContainer display = settings.GetModule("DisplayUserSettings");
		if (!display) { Refuse("NO_DISPLAY_MODULE"); return; }
		int index = display.GetVarIndex("OverallQuality");
		if (index < 0 || index >= display.GetNumVars()) { Refuse("NO_OVERALL_QUALITY"); return; }
		int before;
		if (!display.Get("OverallQuality", before)) { Refuse("CURRENT_UNREADABLE"); return; }
		array<string> names = {}; array<int> values = {};
		display.GetEnumValues(index, names, values);
		if (!names || !values || names.Count() != values.Count() || names.Count() > 64) { Refuse("MALFORMED_ENUM"); return; }
		for (int i = 0; i < names.Count(); i++) if (names[i].IsEmpty()) { Refuse("EMPTY_ENUM_LABEL"); return; }
		float minimum; float maximum; float step;
		int count = display.GetLimits(index, minimum, maximum, step);
		if (count < 0 || count > 3) { Refuse("MALFORMED_LIMITS_COUNT"); return; }
		if ((count >= 1 && !Integral(minimum)) || (count >= 2 && (!Integral(maximum) || maximum < minimum)) || (count == 3 && (!Integral(step) || step <= 0))) { Refuse("MALFORMED_OR_NONINTEGRAL_LIMITS"); return; }
		int target = 0; string mapping = "UI_INDEX";
		if (!values.IsEmpty()) { target = values[0]; mapping = "ENUM_INDEX_0"; }
		else if (count == 3) { target = minimum; mapping = "LIMIT_MIN_INDEX_0"; }
		PrintFormat("[RICON_LOWPREP] intent=LOW_UI_INDEX_0 before=%1 target=%2 mapping=%3 enumCount=%4 limitsCount=%5 pathGuard=LEXICAL_ONLY", before, target, mapping, values.Count(), count);
		if (!display.Set("OverallQuality", target)) { Refuse("SET_REJECTED_EFFECT_UNKNOWN"); return; }
		int readback;
		if (!display.Get("OverallQuality", readback)) { Refuse("AFTER_SET_UNREADABLE"); return; }
		PrintFormat("[RICON_LOWPREP] readback=AFTER_SET actual=%1 expected=%2", readback, target);
		if (readback != target) { Refuse("AFTER_SET_MISMATCH"); return; }
		Print("[RICON_LOWPREP] applyPreset=REQUEST_BEGIN");
		game.ApplySettingsPreset();
		game.UserSettingsChanged();
		Print("[RICON_LOWPREP] applyPreset=RETURNED userSettingsChanged=RETURNED");
		settings = game.GetEngineUserSettings();
		if (!settings) { Refuse("AFTER_APPLY_SETTINGS_MISSING"); return; }
		display = settings.GetModule("DisplayUserSettings");
		if (!display || !display.Get("OverallQuality", readback)) { Refuse("AFTER_APPLY_UNREADABLE"); return; }
		PrintFormat("[RICON_LOWPREP] readback=AFTER_APPLY actual=%1 expected=%2", readback, target);
		if (readback != target) { Refuse("AFTER_APPLY_MISMATCH"); return; }
		Print("[RICON_LOWPREP] save=REQUEST_ONLY disk=UNVERIFIED");
		game.SaveUserSettings();
		Print("[RICON_LOWPREP] saveCall=RETURNED disk=UNVERIFIED");
		Print("[RICON_LOWPREP] done: SAVE_REQUEST_ONLY");
	}
}
