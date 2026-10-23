//! TEST_ONLY: корпус P1b, неизменность входа, контроль самого сравнения и проверки, которых нет в JSON.
class RICON_ChannelFadingRun
{
	//! real/imag и фазы — строго; gainDb native считает во float32, эталон — в binary64 по f32-модулю (CF10).
	static const float GAIN_TOLERANCE_DB = 0.001;

	int m_iCases;
	int m_iMatches;
	int m_iImmutable;
	int m_iControls;
	int m_iExtras;
	bool m_bOk = true;
	protected ref array<string> m_aIds = {};
	protected ref array<ref RICON_FadingDescriptor> m_aDescriptors = {};
	protected ref array<ref RICON_FadingQuery> m_aQueries = {};
	protected ref array<ref RICON_FadingResult> m_aExpected = {};

	protected void Require(bool condition, string id, string what)
	{
		m_bOk = RICON_TestSupport.Expect(condition, id, what) && m_bOk;
	}

	protected void Reject(bool accepted)
	{
		Require(!accepted, "fading controls", "искажённое ожидание отвергнуто");
		if (!accepted)
			m_iControls++;
	}

	void Add(RICON_FadingDescriptor d, string id, float weight, int phaseQ, int dopplerQ)
	{
		RICON_FadingComponent c = new RICON_FadingComponent();
		c.m_sComponentId = id;
		c.m_fPowerWeight = weight;
		c.m_iPhaseQ = phaseQ;
		c.m_iDopplerQ = dopplerQ;
		d.m_aComponents.Insert(c);
	}

	static bool SameResult(RICON_FadingResult a, RICON_FadingResult b)
	{
		if (!a || !b || a.m_eCode != b.m_eCode || a.m_bHasCoefficient != b.m_bHasCoefficient || a.m_bHasGain != b.m_bHasGain)
			return false;
		if (a.m_bHasCoefficient && (!RICON_Num.IsFinite(a.m_fReal) || !RICON_Num.IsFinite(a.m_fImag)
			|| a.m_fReal != b.m_fReal || a.m_fImag != b.m_fImag))
			return false;
		if (a.m_bHasGain && (!RICON_Num.IsFinite(a.m_fGainDb) || !RICON_Num.IsFinite(b.m_fGainDb)
			|| Math.AbsFloat(a.m_fGainDb - b.m_fGainDb) > GAIN_TOLERANCE_DB))
			return false;
		if (!a.m_aPhasesQ || !b.m_aPhasesQ || a.m_aPhasesQ.Count() != b.m_aPhasesQ.Count())
			return false;
		for (int i = 0; i < a.m_aPhasesQ.Count(); i++)
		{
			if (a.m_aPhasesQ[i] != b.m_aPhasesQ[i])
				return false;
		}
		return true;
	}

	protected static string Endpoint(RICON_FadingEndpoint e)
	{
		if (!e)
			return "null";
		return e.m_iDeviceId.ToString() + "/" + e.m_iTransceiverIndex.ToString();
	}

	//! Все поля входа; float — в массив без форматирования в строку (крах VM на ToString(-1, 9), P1).
	protected static void Capture(RICON_FadingDescriptor d, RICON_FadingQuery q, out string text, notnull array<float> floats)
	{
		text = d.m_sModelId + "|" + d.m_sProfileId + "|" + d.m_iProfileVersion.ToString() + "|" + Endpoint(d.m_A);
		text += "|" + Endpoint(d.m_B) + "|" + d.m_iEnvironmentRevision.ToString() + "|" + d.m_iAnchorMs.ToString();
		text += "|" + d.m_iValidFromMs.ToString() + "|" + d.m_iValidUntilMs.ToString() + "|" + d.m_bStaticChannel.ToString();
		text += "|" + d.m_bCoverageComplete.ToString() + "|" + d.m_bResourceExhausted.ToString();
		text += "|" + d.m_aComponents.Count().ToString();
		floats.Insert(d.m_fFrequencyMHz);
		floats.Insert(d.m_fBandwidthHz);
		foreach (RICON_FadingComponent c : d.m_aComponents)
		{
			if (!c)
			{
				text += "|null";
				continue;
			}
			text += "|" + c.m_sComponentId + "|" + c.m_iPhaseQ.ToString() + "|" + c.m_iDopplerQ.ToString();
			floats.Insert(c.m_fPowerWeight);
		}
		text += "|" + Endpoint(q.m_Tx) + "|" + Endpoint(q.m_Rx) + "|" + q.m_sProfileId + "|" + q.m_iProfileVersion.ToString();
		text += "|" + q.m_iEnvironmentRevision.ToString() + "|" + q.m_iNowMs.ToString() + "|" + q.m_iSubUs.ToString();
		floats.Insert(q.m_fFrequencyMHz);
		floats.Insert(q.m_fBandwidthHz);
	}

	protected static bool SameInput(RICON_FadingDescriptor d, RICON_FadingQuery q, string text, array<float> floats)
	{
		string now;
		array<float> current = {};
		Capture(d, q, now, current);
		return now == text && RICON_ChannelCutoffRun.SameFloats(current, floats);
	}

	protected static string Text(float v)
	{
		if (RICON_Num.IsFinite(v) && Math.AbsFloat(v) < 1000000)
			return v.ToString(-1, 9);
		return "out_of_print_range";
	}

	protected void Controls(RICON_FadingResult actual, RICON_FadingResult expected)
	{
		Reject(SameResult(null, expected));
		RICON_EFadingCode code = actual.m_eCode;
		actual.m_eCode = RICON_EFadingCode.ZERO_AT_SAMPLE;
		Reject(SameResult(actual, expected));
		actual.m_eCode = code;
		float real = actual.m_fReal;
		actual.m_fReal = real + 0.000001;
		Reject(SameResult(actual, expected));
		actual.m_fReal = real;
		float imag = actual.m_fImag;
		actual.m_fImag = imag - 0.000001;
		Reject(SameResult(actual, expected));
		actual.m_fImag = imag;
		float gain = actual.m_fGainDb;
		actual.m_fGainDb = gain + 0.002;
		Reject(SameResult(actual, expected));
		actual.m_fGainDb = RICON_L2Run.NaN();
		Reject(SameResult(actual, expected));
		actual.m_fGainDb = gain;
		actual.m_bHasGain = false;
		Reject(SameResult(actual, expected));
		actual.m_bHasGain = true;
		int phase = actual.m_aPhasesQ[1];
		actual.m_aPhasesQ[1] = phase + 1;
		Reject(SameResult(actual, expected));
		actual.m_aPhasesQ[1] = phase;
		actual.m_aPhasesQ.Insert(0);
		Reject(SameResult(actual, expected));
		actual.m_aPhasesQ.Remove(actual.m_aPhasesQ.Count() - 1);
	}

	void CheckCase(string id, RICON_FadingDescriptor d, RICON_FadingQuery q, RICON_FadingResult expected)
	{
		m_iCases++;
		string before;
		array<float> beforeFloats = {};
		Capture(d, q, before, beforeFloats);
		RICON_FadingResult actual = RICON_FiniteMultipath.Evaluate(d, q);
		bool matched = SameResult(actual, expected);
		Require(matched, id, "код, коэффициент, gainDb и фазы совпадают с эталоном");
		if (matched)
			m_iMatches++;
		if (actual)
		{
			actual.m_fReal = -9999;
			actual.m_aPhasesQ.Insert(-1);
		}
		RICON_FadingResult second = RICON_FiniteMultipath.Evaluate(d, q);
		bool immutable = SameInput(d, q, before, beforeFloats) && second != actual && SameResult(second, expected)
			&& (!actual || second.m_aPhasesQ != actual.m_aPhasesQ);
		Require(immutable, id, "вход не изменён, повторный результат и его фазы — новые объекты");
		if (immutable)
			m_iImmutable++;
		if (id == "F05" && matched && immutable)
		{
			Controls(second, expected);
			Require(SameResult(second, expected), "fading controls", "после контролей исходный результат восстановлен");
		}
		m_aIds.Insert(id);
		m_aDescriptors.Insert(d);
		m_aQueries.Insert(q);
		m_aExpected.Insert(expected);
		string realText = "null";
		string imagText = "null";
		string gainText = "null";
		if (second && second.m_bHasCoefficient)
		{
			realText = Text(second.m_fReal);
			imagText = Text(second.m_fImag);
		}
		if (second && second.m_bHasGain)
			gainText = Text(second.m_fGainDb);
		PrintFormat("[RICON_FADING] %1 code=%2 real=%3 imag=%4 gain=%5 phases=%6 match=%7 immutable=%8", id,
			typename.EnumToString(RICON_EFadingCode, second.m_eCode), realText, imagText, gainText,
			second.m_aPhasesQ.Count(), matched, immutable);
	}

	protected int Find(string id)
	{
		return m_aIds.Find(id);
	}

	//! Целочисленная фаза и константы полинома: значения проверены точной арифметикой Python (MulMod) и
	//! правильным округлением одиночного деления binary32 (коэффициенты CF07).
	void CheckArithmetic()
	{
		int m = RICON_FiniteMultipath.PHASE_M;
		bool ok = RICON_FiniteMultipath.MulMod(m - 1, m - 1) == 1 && RICON_FiniteMultipath.MulMod(m - 1, 1000) == 1073740824;
		ok = ok && RICON_FiniteMultipath.MulMod(32768, 32768) == 0 && RICON_FiniteMultipath.MulMod(32767, 32769) == m - 1;
		ok = ok && RICON_FiniteMultipath.MulMod(123456789, 1000) == 1050221064;
		ok = ok && RICON_FiniteMultipath.MulMod(m - 1, RICON_FiniteMultipath.QUARTER) == 805306368;
		ok = ok && RICON_FiniteMultipath.MulMod(536870911, 999999) == 535870913;
		ok = ok && RICON_FiniteMultipath.PhaseAt(0, -RICON_FiniteMultipath.QUARTER, 1) == 805306368;
		ok = ok && RICON_FiniteMultipath.PhaseAt(m - 1, RICON_FiniteMultipath.QUARTER, 1) == RICON_FiniteMultipath.QUARTER - 1;
		float cosA;
		float sinA;
		RICON_FiniteMultipath.CosSin(0, cosA, sinA);
		float cosB;
		float sinB;
		RICON_FiniteMultipath.CosSin(RICON_FiniteMultipath.QUARTER, cosB, sinB);
		float cosC;
		float sinC;
		RICON_FiniteMultipath.CosSin(2 * RICON_FiniteMultipath.QUARTER, cosC, sinC);
		float cosD;
		float sinD;
		RICON_FiniteMultipath.CosSin(3 * RICON_FiniteMultipath.QUARTER, cosD, sinD);
		ok = ok && cosA == 1 && sinA == 0 && cosB == 0 && sinB == 1 && cosC == -1 && sinC == 0 && cosD == 0 && sinD == -1;
		ok = ok && RICON_FiniteMultipath.S3 == -1.0 / 6.0 && RICON_FiniteMultipath.S5 == 1.0 / 120.0;
		ok = ok && RICON_FiniteMultipath.S7 == -1.0 / 5040.0 && RICON_FiniteMultipath.S9 == 1.0 / 362880.0;
		ok = ok && RICON_FiniteMultipath.S11 == -1.0 / 39916800.0 && RICON_FiniteMultipath.C4 == 1.0 / 24.0;
		ok = ok && RICON_FiniteMultipath.C6 == -1.0 / 720.0 && RICON_FiniteMultipath.C8 == 1.0 / 40320.0;
		ok = ok && RICON_FiniteMultipath.C10 == -1.0 / 3628800.0 && RICON_FiniteMultipath.C12 == 1.0 / 479001600.0;
		Require(ok, "fading arithmetic", "MulMod/PhaseAt на крайних парах, кардинальные фазы и литералы полинома");
		if (ok)
			m_iExtras++;
	}

	//! Null-объекты, которых нет в JSON, и защитный NUMERIC_FAILURE через fault-injection на границе Classify.
	void CheckFaults()
	{
		int base = Find("F01");
		RICON_FadingDescriptor d = m_aDescriptors[base];
		RICON_FadingQuery q = m_aQueries[base];
		bool ok = RICON_FiniteMultipath.Evaluate(null, q).m_eCode == RICON_EFadingCode.INVALID_INPUT;
		ok = ok && RICON_FiniteMultipath.Evaluate(d, null).m_eCode == RICON_EFadingCode.INVALID_INPUT;
		RICON_FadingEndpoint keepTx = q.m_Tx;
		q.m_Tx = null;
		ok = ok && RICON_FiniteMultipath.Evaluate(d, q).m_eCode == RICON_EFadingCode.INVALID_INPUT;
		q.m_Tx = keepTx;
		d.m_aComponents.Insert(null);
		ok = ok && RICON_FiniteMultipath.Evaluate(d, q).m_eCode == RICON_EFadingCode.INVALID_INPUT;
		d.m_aComponents.Remove(d.m_aComponents.Count() - 1);
		ok = ok && SameResult(RICON_FiniteMultipath.Evaluate(d, q), m_aExpected[base]);
		array<int> phases = {7};
		RICON_FadingResult nan = RICON_FiniteMultipath.Classify(RICON_L2Run.NaN(), 0, phases);
		RICON_FadingResult inf = RICON_FiniteMultipath.Classify(0, RICON_L0Run.Special(-1), phases);
		RICON_FadingResult zero = RICON_FiniteMultipath.Classify(0, 0, phases);
		RICON_FadingResult unit = RICON_FiniteMultipath.Classify(0, -1, phases);
		ok = ok && nan.m_eCode == RICON_EFadingCode.NUMERIC_FAILURE && !nan.m_bHasCoefficient && nan.m_aPhasesQ.Count() == 0;
		ok = ok && inf.m_eCode == RICON_EFadingCode.NUMERIC_FAILURE && !inf.m_bHasGain;
		ok = ok && zero.m_eCode == RICON_EFadingCode.ZERO_AT_SAMPLE && zero.m_bHasCoefficient && !zero.m_bHasGain
			&& zero.m_aPhasesQ.Count() == 1 && zero.m_aPhasesQ[0] == 7 && zero.m_aPhasesQ != phases;
		ok = ok && unit.m_eCode == RICON_EFadingCode.OK && unit.m_bHasGain && unit.m_fGainDb == 0;
		// Наименьший субнормальный real: модуль либо положителен (OK, конечный gain), либо платформа сбросила его
		// в 0 (FTZ) — тогда NUMERIC_FAILURE. ZERO_AT_SAMPLE или −Inf недопустимы в обоих случаях.
		float tiny = 0.000000000000000000000000000000000000000000001401298464324817;
		RICON_FadingResult sub = RICON_FiniteMultipath.Classify(tiny, 0, phases);
		bool subOk = (sub.m_eCode == RICON_EFadingCode.OK && RICON_Num.IsFinite(sub.m_fGainDb))
			|| sub.m_eCode == RICON_EFadingCode.NUMERIC_FAILURE;
		ok = ok && subOk && tiny != 0;
		Require(ok, "fading faults", "null-объекты — INVALID_INPUT; NaN/Inf — NUMERIC_FAILURE; ZERO до hypot; субнормаль не ZERO");
		if (ok)
			m_iExtras++;
		PrintFormat("[RICON_FADING_FAULTS] subnormal code=%1 gain=%2", typename.EnumToString(RICON_EFadingCode, sub.m_eCode),
			Text(sub.m_fGainDb));
	}

	//! CF10: порядок и частота вызовов. Серии F40–F50, F65–F69 и пара F04→F05 повторяются в обратном порядке,
	//! вперемешку и дважды; чистая функция времени не зависит от предыдущих вызовов и пауз (PTT-перерыв без перезапуска).
	void CheckOrder()
	{
		array<string> series = {"F04", "F05", "F40", "F41", "F42", "F43", "F44", "F45", "F46", "F47", "F48", "F49", "F50",
			"F65", "F66", "F67", "F68", "F69"};
		bool ok = true;
		for (int pass = 0; pass < 2; pass++)
		{
			for (int i = series.Count() - 1; i >= 0; i--)
			{
				int at = Find(series[i]);
				ok = ok && at >= 0 && SameResult(RICON_FiniteMultipath.Evaluate(m_aDescriptors[at], m_aQueries[at]), m_aExpected[at]);
				int other = Find(series[(i * 7 + pass) % series.Count()]);
				RICON_FiniteMultipath.Evaluate(m_aDescriptors[other], m_aQueries[other]);
			}
		}
		// ZERO_AT_SAMPLE не удаляет descriptor: тот же объект после нуля даёт ненулевой отсчёт позже.
		int zeroAt = Find("F04");
		int laterAt = Find("F05");
		RICON_FadingResult zero = RICON_FiniteMultipath.Evaluate(m_aDescriptors[zeroAt], m_aQueries[zeroAt]);
		RICON_FadingResult later = RICON_FiniteMultipath.Evaluate(m_aDescriptors[zeroAt], m_aQueries[laterAt]);
		ok = ok && zero.m_eCode == RICON_EFadingCode.ZERO_AT_SAMPLE && SameResult(later, m_aExpected[laterAt]);
		Require(ok, "fading order", "обратный и перемешанный порядок, повтор и ZERO→OK на том же descriptor");
		if (ok)
			m_iExtras++;
	}
}
