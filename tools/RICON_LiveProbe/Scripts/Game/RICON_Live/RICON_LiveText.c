//! TOOL_ONLY: запись произвольной строки в поле text журнала без потерь (A1-E, codex #4315, #4319).
//! Поле text журнала — ASCII без пробелов, запятых и «=» (RICON_LiveJournal). Escape оставляет печатный ASCII 33…126,
//! кроме «%», «,» и «=», а каждый другой код пишет как %XX (две заглавные шестнадцатеричные цифры кода ToAscii, после
//! приведения отрицательного к 0…255). Код вне 0…255 пишется как %uXXXX и отдельно считается (поведение ToAscii для
//! не-ASCII проверяется пробником RICON_LiveTextProbePlugin, а не предполагается). Обратное декодирование — внешний
//! разбор лога. Engine-вызовов нет, кроме строковых методов.
class RICON_LiveText
{
	static const string HEX = "0123456789ABCDEF";

	protected static string Hex(int value, int digits)
	{
		string text = "";
		for (int i = digits - 1; i >= 0; i--)
		{
			int nibble = (value >> (4 * i)) & 15;
			text = text + HEX.Substring(nibble, 1);
		}
		return text;
	}

	//! true — код остаётся как есть (печатный ASCII без разделителей журнала и «%»).
	static bool Plain(int code)
	{
		return code >= 33 && code <= 126 && code != 37 && code != 44 && code != 61;
	}

	//! %XX-форма строки; wide — число кодов вне 0…255 (в выходе %uXXXX).
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
				text = text + "%" + Hex(code, 2);
			else
			{
				wide++;
				text = text + "%u" + Hex(code & 65535, 4);
			}
		}
		return text;
	}

	//! Escape без счётчика; пустое значение даёт пустую строку.
	static string Esc(string value)
	{
		int wide;
		return Escape(value, wide);
	}
}
