//! TOOL_ONLY: запись произвольной строки в поле text журнала без потерь (A1-E, codex #4315, #4319, #4339).
//! Поле text журнала — ASCII без пробелов, запятых и «=» (RICON_LiveJournal). Escape оставляет печатный ASCII 33…126,
//! кроме «%», «,» и «=», а каждый другой байт пишет как %XX (две заглавные шестнадцатеричные цифры). Length() и
//! ToAscii(i) работают с байтами UTF-8 (пробник RICON_LiveTextProbePlugin, лог ricon-livetext-claude-20261024-181308:
//! кириллица — 6 байт, wide 0). Отрицательный код приводится к 0…255; код вне 0…255 второго протокола не получает:
//! пишется маркер отказа %ZZ, который строгий декодер (docs/evidence/receipts/live_text_codec_claude.py) отвергает,
//! и считается в wide. Обратное декодирование — только внешний разбор лога. Engine-вызовов нет, кроме строковых методов.
class RICON_LiveText
{
	static const string HEX = "0123456789ABCDEF";
	static const string REFUSED = "%ZZ";

	protected static string Hex2(int value)
	{
		return HEX.Substring((value >> 4) & 15, 1) + HEX.Substring(value & 15, 1);
	}

	//! true — код остаётся как есть (печатный ASCII без разделителей журнала и «%»).
	static bool Plain(int code)
	{
		return code >= 33 && code <= 126 && code != 37 && code != 44 && code != 61;
	}

	//! %XX-форма строки; wide — число кодов вне 0…255 (в выходе маркер отказа %ZZ).
	static string Escape(string value, out int wide)
	{
		wide = 0;
		string text = "";
		for (int i = 0; i < value.Length(); i++)
		{
			int code = value.ToAscii(i);
			if (code < 0 && code >= -128)
				code = code + 256;
			if (Plain(code))
				text = text + value.Substring(i, 1);
			else if (code >= 0 && code <= 255)
				text = text + "%" + Hex2(code);
			else
			{
				wide++;
				text = text + REFUSED;
			}
		}
		return text;
	}

	//! Escape без счётчика; пустое значение даёт пустую строку, отказ виден в выходе как %ZZ.
	static string Esc(string value)
	{
		int wide;
		return Escape(value, wide);
	}
}
