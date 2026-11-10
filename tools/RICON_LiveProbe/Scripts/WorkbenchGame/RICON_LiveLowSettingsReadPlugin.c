//! ScriptEditor-only owning settings readback. No world, setters or persistence.
[WorkbenchPluginAttribute(name: "RICON: Read graphics quality enum", wbModules: {"ScriptEditor"})]
class RICON_LiveLowSettingsReadPlugin : WorkbenchPlugin
{
	protected void Refuse(string reason)
	{
		PrintFormat("[RICON_LOWREAD] failure=%1", reason);
		Print("[RICON_LOWREAD] done: FAILED");
	}
	override void RunCommandline()
	{
		Print("[RICON_LOWREAD] begin: READ_ONLY");
		Game game = GetGame();
		if (!game) { Refuse("NO_GAME_CONTEXT"); return; }
		UserSettings settings = game.GetEngineUserSettings();
		if (!settings) { Refuse("NO_ENGINE_USER_SETTINGS"); return; }
		BaseContainer display = settings.GetModule("DisplayUserSettings");
		if (!display) { Refuse("NO_DISPLAY_MODULE"); return; }
		int index = display.GetVarIndex("OverallQuality");
		if (index < 0 || index >= display.GetNumVars()) { Refuse("NO_OVERALL_QUALITY_VARIABLE"); return; }
		int current;
		if (!display.Get("OverallQuality", current)) { Refuse("CURRENT_VALUE_UNREADABLE"); return; }
		array<string> names = {};
		array<int> values = {};
		display.GetEnumValues(index, names, values);
		float minimum; float maximum; float step;
		int limitsCount = display.GetLimits(index, minimum, maximum, step);
		int namesCount = -1; int valuesCount = -1;
		if (names) namesCount = names.Count();
		if (values) valuesCount = values.Count();
		PrintFormat("[RICON_LOWREAD] module=DisplayUserSettings variable=OverallQuality index=%1 current=%2 namesCount=%3 valuesCount=%4 limitsCount=%5", index, current, namesCount, valuesCount, limitsCount);
		string minText = "UNAVAILABLE"; string maxText = "UNAVAILABLE"; string stepText = "UNAVAILABLE";
		if (limitsCount >= 1) minText = minimum.ToString(-1, 9);
		if (limitsCount >= 2) maxText = maximum.ToString(-1, 9);
		if (limitsCount >= 3) stepText = step.ToString(-1, 9);
		PrintFormat("[RICON_LOWREAD] limitMin=%1 limitMax=%2 limitStep=%3 readonly=1", minText, maxText, stepText);
		if (limitsCount < 0 || limitsCount > 3) { Refuse("LIMITS_COUNT_OUT_OF_SCOPE"); return; }
		if (!names || !values) { Refuse("ENUM_ARRAY_MISSING"); return; }
		if (names.Count() != values.Count()) { Refuse("ENUM_ARRAY_MISMATCH"); return; }
		if (names.IsEmpty()) { Print("[RICON_LOWREAD] done: READ_OK_WITHOUT_ENUM"); return; }
		if (names.Count() > 64) { Refuse("COUNT_OVER64"); return; }
		bool currentInEnum;
		for (int i = 0; i < names.Count(); i++)
		{
			string name = names[i];
			if (name.IsEmpty()) { Refuse("ENUM_NAME_EMPTY"); return; }
			// Keep an owning enum label on one log line without dumping settings or paths.
			name.Replace("\r", "\\r"); name.Replace("\n", "\\n");
			PrintFormat("[RICON_LOWREAD] enumIndex=%1 name=%2 value=%3", i, name, values[i]);
			if (values[i] == current) currentInEnum = true;
		}
		PrintFormat("[RICON_LOWREAD] currentInEnum=%1 readonly=1", currentInEnum);
		Print("[RICON_LOWREAD] done: READ_OK");
	}
}
