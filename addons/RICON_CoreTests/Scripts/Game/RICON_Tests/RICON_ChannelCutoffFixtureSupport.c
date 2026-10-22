//! TEST_ONLY: корпус P1, неизменность входа, контроль самого сравнения и границы, которых нет в JSON.
class RICON_ChannelCutoffRun
{
	//! binary32-вычисление U против binary64 эталона; ветвление сравнивается строго (#3383, CC07).
	static const float UPPER_TOLERANCE_DB = 0.001;

	int m_iCases;
	int m_iMatches;
	int m_iImmutable;
	int m_iControls;
	int m_iExtras;
	bool m_bOk = true;

	protected void Require(bool condition, string id, string what)
	{
		m_bOk = RICON_TestSupport.Expect(condition, id, what) && m_bOk;
	}

	protected void Reject(bool accepted)
	{
		Require(!accepted, "cutoff controls", "искажённое ожидание отвергнуто");
		if (!accepted)
			m_iControls++;
	}

	static bool SameResult(RICON_CutoffResult a, RICON_CutoffResult b)
	{
		if (!a || !b)
			return false;
		if (a.m_eDecision != b.m_eDecision || a.m_eReason != b.m_eReason || a.m_bUsefulIsZero != b.m_bUsefulIsZero)
			return false;
		if (a.m_bHasUpper != b.m_bHasUpper)
			return false;
		if (!a.m_bHasUpper)
			return true;
		return RICON_Num.IsFinite(a.m_fUpperDbm) && RICON_Num.IsFinite(b.m_fUpperDbm)
			&& Math.AbsFloat(a.m_fUpperDbm - b.m_fUpperDbm) <= UPPER_TOLERANCE_DB;
	}

	//! Значения и порядок всех полей входа. float не форматируются в строку: ToString(-1, 9) на ~3.4e38
	//! переполняет стек VM Enforce (crash 2026-10-22, лог ricon-cutoff-claude-20261022-223428).
	protected static void Capture(RICON_CutoffInput input, out string text, notnull array<float> floats)
	{
		text = input.m_iNowMs.ToString() + "|" + input.m_iValidFromMs.ToString() + "|" + input.m_iValidUntilMs.ToString();
		text += "|" + input.m_iEnvironmentRevision.ToString() + "|" + input.m_iBoundRevision.ToString();
		text += "|" + input.m_bCoverageComplete.ToString() + "|" + input.m_bResourceExhausted.ToString();
		text += "|" + input.m_bHasTolerance.ToString() + "|" + input.m_bHasNumericalGuard.ToString();
		text += "|" + input.m_aPaths.Count().ToString();
		floats.Insert(input.m_fToleranceDbm);
		floats.Insert(input.m_fNumericalGuardDb);
		foreach (RICON_CutoffPathBound p : input.m_aPaths)
		{
			if (!p)
			{
				text += "|null";
				continue;
			}
			text += "|" + p.m_sPathId + "|" + p.m_bHasUpper.ToString();
			floats.Insert(p.m_fUpperDbm);
		}
	}

	//! Конечные — точным равенством; неконечные — по ToString() без числа знаков, который различает NaN, +Inf и −Inf
	//! (codex #3388); x != x в Enforce истинно для любого неконечного. Большой конечный сюда не попадает.
	static bool SameFloats(array<float> a, array<float> b)
	{
		if (a.Count() != b.Count())
			return false;
		for (int i = 0; i < a.Count(); i++)
		{
			bool af = RICON_Num.IsFinite(a[i]);
			bool bf = RICON_Num.IsFinite(b[i]);
			if (af != bf)
				return false;
			if (af && a[i] != b[i])
				return false;
			if (!af && a[i].ToString() != b[i].ToString())
				return false;
		}
		return true;
	}

	protected static bool SameInput(RICON_CutoffInput input, string text, array<float> floats)
	{
		string now;
		array<float> current = {};
		Capture(input, now, current);
		return now == text && SameFloats(current, floats);
	}

	//! Печать только умеренных значений с 9 знаками; иначе метка out_of_print_range (см. Capture).
	protected static string Text(float v)
	{
		if (RICON_Num.IsFinite(v) && Math.AbsFloat(v) < 1000000)
			return v.ToString(-1, 9);
		return "out_of_print_range";
	}

	protected void Controls(RICON_CutoffResult actual, RICON_CutoffResult expected)
	{
		Reject(SameResult(null, expected));
		RICON_ECutoffDecision decision = actual.m_eDecision;
		actual.m_eDecision = RICON_ECutoffDecision.BOUNDED;
		Reject(SameResult(actual, expected));
		actual.m_eDecision = decision;
		RICON_ECutoffReason reason = actual.m_eReason;
		actual.m_eReason = RICON_ECutoffReason.BELOW_TOLERANCE;
		Reject(SameResult(actual, expected));
		actual.m_eReason = reason;
		actual.m_bUsefulIsZero = !actual.m_bUsefulIsZero;
		Reject(SameResult(actual, expected));
		actual.m_bUsefulIsZero = !actual.m_bUsefulIsZero;
		float upper = actual.m_fUpperDbm;
		actual.m_fUpperDbm = upper + 0.01;
		Reject(SameResult(actual, expected));
		actual.m_fUpperDbm = RICON_L2Run.NaN();
		Reject(SameResult(actual, expected));
		actual.m_fUpperDbm = upper;
		actual.m_bHasUpper = false;
		Reject(SameResult(actual, expected));
		actual.m_bHasUpper = true;
	}

	void CheckCase(string id, RICON_CutoffInput input, RICON_CutoffResult expected)
	{
		m_iCases++;
		string before;
		array<float> beforeFloats = {};
		Capture(input, before, beforeFloats);
		RICON_CutoffResult actual = RICON_ChannelCutoff.Assess(input);
		bool matched = SameResult(actual, expected);
		Require(matched, id, "решение, причина, признак нуля и граница совпадают с эталоном");
		if (matched)
			m_iMatches++;
		if (actual)
			actual.m_fUpperDbm = -9999;
		RICON_CutoffResult second = RICON_ChannelCutoff.Assess(input);
		bool immutable = SameInput(input, before, beforeFloats) && second != actual && SameResult(second, expected);
		Require(immutable, id, "вход не изменён, повторный результат новый");
		if (immutable)
			m_iImmutable++;
		if (id == "C04" && matched && immutable)
		{
			Controls(second, expected);
			Require(SameResult(second, expected), "cutoff controls", "после контролей исходный результат восстановлен");
			// Перестановка путей не меняет решение и границу.
			RICON_CutoffPathBound firstPath = input.m_aPaths[0];
			input.m_aPaths[0] = input.m_aPaths[1];
			input.m_aPaths[1] = firstPath;
			bool permuted = SameResult(RICON_ChannelCutoff.Assess(input), expected);
			Require(permuted, "cutoff permutation", "порядок путей не влияет на результат");
			if (permuted)
				m_iExtras++;
		}
		string upperText = "null";
		if (second && second.m_bHasUpper)
			upperText = Text(second.m_fUpperDbm);
		PrintFormat("[RICON_CUTOFF] %1 decision=%2 reason=%3 zero=%4 upper=%5 match=%6 immutable=%7", id,
			typename.EnumToString(RICON_ECutoffDecision, second.m_eDecision),
			typename.EnumToString(RICON_ECutoffReason, second.m_eReason), second.m_bUsefulIsZero, upperText, matched, immutable);
	}

	protected static RICON_CutoffInput Base()
	{
		RICON_CutoffInput input = new RICON_CutoffInput();
		input.m_iNowMs = 100;
		input.m_iValidFromMs = 0;
		input.m_iValidUntilMs = 200;
		input.m_iEnvironmentRevision = 1;
		input.m_iBoundRevision = 1;
		input.m_bCoverageComplete = true;
		input.m_bHasTolerance = true;
		input.m_fToleranceDbm = -120;
		input.m_bHasNumericalGuard = true;
		input.m_fNumericalGuardDb = 0;
		return input;
	}

	//! Null-вход, null-путь, пустой pathId и граница 4096/4097 путей — состояния, которых нет в JSON-корпусе.
	void CheckBounds()
	{
		RICON_CutoffResult none = RICON_ChannelCutoff.Assess(null);
		bool ok = none && none.m_eDecision == RICON_ECutoffDecision.UNRESOLVED && none.m_eReason == RICON_ECutoffReason.INVALID_INPUT
			&& !none.m_bUsefulIsZero && !none.m_bHasUpper;
		RICON_CutoffInput nullPath = Base();
		nullPath.m_aPaths.Insert(null);
		ok = ok && RICON_ChannelCutoff.Assess(nullPath).m_eReason == RICON_ECutoffReason.INVALID_INPUT;
		RICON_CutoffInput emptyId = Base();
		RICON_CutoffPathBound unnamed = new RICON_CutoffPathBound();
		unnamed.m_bHasUpper = true;
		unnamed.m_fUpperDbm = -150;
		emptyId.m_aPaths.Insert(unnamed);
		ok = ok && RICON_ChannelCutoff.Assess(emptyId).m_eReason == RICON_ECutoffReason.INVALID_INPUT;
		// Контроль самого сравнения входа: NaN, +Inf и −Inf различаются между собой и с конечным значением.
		float nan = RICON_L2Run.NaN();
		float plus = RICON_L0Run.Special(1);
		float minus = RICON_L0Run.Special(-1);
		ok = ok && !SameFloats({nan}, {minus}) && !SameFloats({nan}, {plus}) && !SameFloats({plus}, {minus});
		ok = ok && !SameFloats({nan}, {0.0}) && SameFloats({nan}, {nan}) && SameFloats({minus}, {minus});
		Require(ok, "cutoff null", "null-вход, null-путь и пустой pathId: INVALID_INPUT; NaN/±Inf различаются");
		if (ok)
			m_iExtras++;
		RICON_CutoffInput many = Base();
		for (int i = 0; i < RICON_ChannelCutoff.MAX_PATHS; i++)
		{
			RICON_CutoffPathBound p = new RICON_CutoffPathBound();
			p.m_sPathId = i.ToString();
			p.m_bHasUpper = true;
			p.m_fUpperDbm = -200;
			many.m_aPaths.Insert(p);
		}
		RICON_CutoffResult full = RICON_ChannelCutoff.Assess(many);
		// 4096 путей по −200 дБм: граница −200 + 20·lg 4096 ≈ −127.75 < −120, отсечка допустима.
		bool limit = full.m_eDecision == RICON_ECutoffDecision.BOUNDED && full.m_eReason == RICON_ECutoffReason.BELOW_TOLERANCE
			&& full.m_bHasUpper && Math.AbsFloat(full.m_fUpperDbm - (-127.75)) <= 0.01;
		RICON_CutoffPathBound extra = new RICON_CutoffPathBound();
		extra.m_sPathId = "extra";
		extra.m_bHasUpper = true;
		extra.m_fUpperDbm = -200;
		many.m_aPaths.Insert(extra);
		limit = limit && RICON_ChannelCutoff.Assess(many).m_eReason == RICON_ECutoffReason.INVALID_INPUT;
		Require(limit, "cutoff path limit", "4096 путей суммируются, 4097-й путь — INVALID_INPUT");
		if (limit)
			m_iExtras++;
		PrintFormat("[RICON_CUTOFF_LIMIT] paths=%1 upper=%2", RICON_ChannelCutoff.MAX_PATHS, Text(full.m_fUpperDbm));
	}
}
