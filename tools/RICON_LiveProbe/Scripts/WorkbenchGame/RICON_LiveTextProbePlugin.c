//! TOOL_ONLY: native-контроли %XX-записи RICON_LiveText без мира, Play и сервера (codex #4319).
//! Проверяет точный вывод для «%», пробела, запятой, «=», управляющих символов, кириллицы и строки вида текста меню.
//! Обратимость проверяет внешний разбор лога (docs/evidence/receipts/live-text-log-check-claude.py): он декодирует
//! напечатанные формы и сравнивает с исходными байтами. Сообщения — ASCII: кириллица в console.log печатается нечитаемо.
[WorkbenchPluginAttribute(name: "RICON: запись текста live-прибора", description: "%XX-форма без потерь", wbModules: {"ScriptEditor"})]
class RICON_LiveTextProbePlugin : WorkbenchPlugin
{
	static const int EXPECTED_CHECKS = 14;
	protected int m_iChecks;
	protected int m_iPassed;

	protected void Check(bool condition, string what)
	{
		m_iChecks++;
		if (condition)
			m_iPassed++;
		else
			Print("[RICON_LIVET] mismatch: " + what);
	}

	//! Точный вывод Escape и число wide-кодов; форма печатается всегда, чтобы внешний разбор видел фактическое.
	protected void Expect(string name, string value, string expected, int expectedWide)
	{
		int wide;
		string escaped = RICON_LiveText.Escape(value, wide);
		PrintFormat("[RICON_LIVET] case=%1 length=%2 wide=%3 escaped=%4", name, value.Length(), wide, escaped);
		Check(escaped == expected && wide == expectedWide, name);
	}

	override void RunCommandline()
	{
		Expect("plain", "abc_09.#-", "abc_09.#-", 0);
		Expect("empty", "", "", 0);
		Expect("space", "a b", "a%20b", 0);
		Expect("percent", "%", "%25", 0);
		Expect("comma", ",", "%2C", 0);
		Expect("equals", "=", "%3D", 0);
		Expect("controls", "\t\n", "%09%0A", 0);
		int del = 127;
		int bell = 7;
		Expect("tilde_del_bell", "~" + del.AsciiToString() + bell.AsciiToString(), "~%7F%07", 0);
		Expect("menu_like", "32.2 #AR-VON_FrequencyUnits_MHz", "32.2%20#AR-VON_FrequencyUnits_MHz", 0);
		Expect("cyrillic", "МГц", "%D0%9C%D0%93%D1%86", 0);
		Check(RICON_LiveText.Plain(33) && RICON_LiveText.Plain(126), "plain bounds 33 and 126");
		Check(!RICON_LiveText.Plain(32) && !RICON_LiveText.Plain(127), "space and DEL are escaped");
		Check(!RICON_LiveText.Plain(37) && !RICON_LiveText.Plain(44) && !RICON_LiveText.Plain(61), "percent comma equals are escaped");
		Check(RICON_LiveText.Esc("a b") == "a%20b", "Esc without counter");
		PrintFormat("[RICON_LIVET] checks=%1 passed=%2", m_iChecks, m_iPassed);
		if (m_iChecks == m_iPassed && m_iChecks == EXPECTED_CHECKS)
		{
			Print("[RICON_LIVET] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_LIVET] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
