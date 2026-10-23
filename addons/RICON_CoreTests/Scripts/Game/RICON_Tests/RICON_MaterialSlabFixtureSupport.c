//! TEST_ONLY: корпус P3.2a — совпадение с frozen ожиданиями (MS08), неизменность входа, контроли сравнения,
//! TE = TM при нормальном падении и повтор в обратном порядке.
class RICON_MaterialSlabRun
{
	static const int BAD_ENUM = 99; //!< недопустимое значение polarization/provenance (MS049, MS053)
	int m_iCases;
	int m_iMatches;
	int m_iImmutable;
	int m_iControls;
	int m_iExtras;
	bool m_bOk = true;
	float m_fMaxDeviationDb;
	protected ref array<ref RICON_MaterialSlabInput> m_aInputs = {};
	protected ref array<ref RICON_MaterialSlabResult> m_aExpected = {};
	protected ref RICON_MaterialSlabInput m_Base;
	protected ref RICON_MaterialSlabResult m_BaseExpected;

	protected void Require(bool condition, string id, string what)
	{
		m_bOk = RICON_TestSupport.Expect(condition, id, what) && m_bOk;
	}

	//! Точный binary32 из битового образа: числа корпуса не зависят от разбора десятичных литералов (MS03).
	float F(int bits)
	{
		float value;
		RICON_DescriptorCodec.BitsToFloat(bits, value);
		return value;
	}

	RICON_MaterialSlabInput NewInput()
	{
		RICON_MaterialSlabInput input = new RICON_MaterialSlabInput();
		input.m_Material = new RICON_MaterialProfile();
		return input;
	}

	//! MS08: предварительный native допуск max(0.05 дБ, 2e-6·|L|).
	static float Tolerance(float expected)
	{
		return Math.Max(0.05, 0.000002 * Math.AbsFloat(expected));
	}

	static bool Close(float actual, float expected)
	{
		return RICON_Num.IsFinite(actual) && RICON_Num.IsFinite(expected) && Math.AbsFloat(actual - expected) <= Tolerance(expected);
	}

	//! Коды точно; при OK — provenance точно и lossDb в допуске; при отказе значим только код (MS04).
	static bool SameResult(RICON_MaterialSlabResult a, RICON_MaterialSlabResult b)
	{
		if (!a || !b || a.m_eCode != b.m_eCode)
			return false;
		if (a.m_eCode != RICON_EMaterialSlabCode.OK)
			return true;
		return a.m_eProvenance == b.m_eProvenance && Close(a.m_fLossDb, b.m_fLossDb);
	}

	protected static string Bits(float x)
	{
		if (!RICON_Num.IsFinite(x))
			return "nonfinite";
		if (x < 0)
		{
			int negative = RICON_DescriptorCodec.FloatToBits(-x);
			return "-" + negative.ToString();
		}
		int bits = RICON_DescriptorCodec.FloatToBits(x);
		return bits.ToString();
	}

	//! Снимок входа битовыми образами: любое изменение любого поля меняет текст.
	static string Capture(RICON_MaterialSlabInput input)
	{
		if (!input)
			return "null";
		int polarization = input.m_ePolarization;
		string text = Bits(input.m_fFrequencyMHz) + "|" + Bits(input.m_fThicknessM) + "|" + Bits(input.m_fCosIncidence) + "|"
			+ polarization.ToString() + "#";
		RICON_MaterialProfile m = input.m_Material;
		if (!m)
			return text + "null";
		int provenance = m.m_eProvenance;
		return text + Bits(m.m_fEpsilonReal) + "|" + Bits(m.m_fConductivitySm) + "|" + Bits(m.m_fMinMHz) + "|" + Bits(m.m_fMaxMHz) + "|"
			+ provenance.ToString();
	}

	protected static string Text(float x)
	{
		if (RICON_Num.IsFinite(x) && Math.AbsFloat(x) < 1000000)
			return x.ToString(-1, 9);
		return "out_of_print_range";
	}

	protected static string Provenance(RICON_MaterialSlabResult r)
	{
		if (r.m_eCode != RICON_EMaterialSlabCode.OK)
			return "NONE";
		return typename.EnumToString(RICON_EMaterialProvenance, r.m_eProvenance);
	}

	protected void Reject(bool accepted)
	{
		Require(!accepted, "material slab controls", "искажённое ожидание отвергнуто");
		if (!accepted)
			m_iControls++;
	}

	//! Пять контролей сравнения: null, код, provenance, lossDb выше и ниже допуска.
	protected void Controls(RICON_MaterialSlabResult actual, RICON_MaterialSlabResult expected)
	{
		Reject(SameResult(null, expected));
		actual.m_eCode = RICON_EMaterialSlabCode.NUMERIC_FAILURE;
		Reject(SameResult(actual, expected));
		actual.m_eCode = expected.m_eCode;
		actual.m_eProvenance = RICON_EMaterialProvenance.ASSUMED_CONFIG;
		Reject(SameResult(actual, expected));
		actual.m_eProvenance = expected.m_eProvenance;
		float loss = actual.m_fLossDb;
		float step = 2 * Tolerance(expected.m_fLossDb);
		actual.m_fLossDb = expected.m_fLossDb + step;
		Reject(SameResult(actual, expected));
		actual.m_fLossDb = expected.m_fLossDb - step;
		Reject(SameResult(actual, expected));
		actual.m_fLossDb = loss;
	}

	void CheckCase(string id, RICON_MaterialSlabInput input, RICON_MaterialSlabResult expected)
	{
		m_iCases++;
		string before = Capture(input);
		RICON_MaterialSlabResult first = RICON_MaterialSlabKernel.Evaluate(input);
		bool matched = SameResult(first, expected);
		Require(matched, id, "код, lossDb и provenance совпадают с эталоном");
		if (matched)
			m_iMatches++;
		if (matched && expected.m_eCode == RICON_EMaterialSlabCode.OK)
			m_fMaxDeviationDb = Math.Max(m_fMaxDeviationDb, Math.AbsFloat(first.m_fLossDb - expected.m_fLossDb));
		first.m_fLossDb = -999;
		first.m_eCode = RICON_EMaterialSlabCode.NUMERIC_FAILURE;
		RICON_MaterialSlabResult second = RICON_MaterialSlabKernel.Evaluate(input);
		bool immutable = Capture(input) == before && second != first && SameResult(second, expected);
		Require(immutable, id, "вход не изменён, повторный результат новый");
		if (immutable)
			m_iImmutable++;
		if (id == "MS003" && matched && immutable)
		{
			Controls(second, expected);
			Require(SameResult(second, expected), "material slab controls", "после контролей исходный результат восстановлен");
			m_Base = input;
			m_BaseExpected = expected;
		}
		m_aInputs.Insert(input);
		m_aExpected.Insert(expected);
		PrintFormat("[RICON_SLAB] id=%1 code=%2 loss=%3 provenance=%4 match=%5 immutable=%6", id,
			typename.EnumToString(RICON_EMaterialSlabCode, second.m_eCode), Text(second.m_fLossDb), Provenance(second), matched, immutable);
	}

	//! TE и TM при нормальном падении дают одно и то же |r| (a = C против a = eC при p² = e), MS08.
	protected bool TeEqualsTm(float epsilon, float sigma, float thickness)
	{
		RICON_MaterialSlabInput probe = NewInput();
		probe.m_fFrequencyMHz = 900;
		probe.m_fThicknessM = thickness;
		probe.m_fCosIncidence = 1;
		probe.m_ePolarization = RICON_ESlabPolarization.TE;
		probe.m_Material.m_fEpsilonReal = epsilon;
		probe.m_Material.m_fConductivitySm = sigma;
		probe.m_Material.m_fMinMHz = 1;
		probe.m_Material.m_fMaxMHz = 450000;
		probe.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialSlabResult te = RICON_MaterialSlabKernel.Evaluate(probe);
		probe.m_ePolarization = RICON_ESlabPolarization.TM;
		RICON_MaterialSlabResult tm = RICON_MaterialSlabKernel.Evaluate(probe);
		bool ok = te.m_eCode == RICON_EMaterialSlabCode.OK && tm.m_eCode == RICON_EMaterialSlabCode.OK && te.m_fLossDb > 0
			&& Math.AbsFloat(te.m_fLossDb - tm.m_fLossDb) <= 0.001;
		PrintFormat("[RICON_SLAB_TETM] eps=%1 sigma=%2 d=%3 te=%4 tm=%5 ok=%6", Text(epsilon), Text(sigma), Text(thickness),
			Text(te.m_fLossDb), Text(tm.m_fLossDb), ok);
		return ok;
	}

	void CheckExtras()
	{
		bool ok = RICON_MaterialSlabKernel.Evaluate(null).m_eCode == RICON_EMaterialSlabCode.INVALID_INPUT && m_Base != null;
		if (m_Base)
		{
			RICON_MaterialProfile material = m_Base.m_Material;
			m_Base.m_Material = null;
			ok = ok && RICON_MaterialSlabKernel.Evaluate(m_Base).m_eCode == RICON_EMaterialSlabCode.INVALID_PROFILE;
			m_Base.m_Material = material;
			ok = ok && SameResult(RICON_MaterialSlabKernel.Evaluate(m_Base), m_BaseExpected);
		}
		Require(ok, "material slab extras", "null вход и null профиль отвергнуты; базовый вход восстановлен");
		if (ok)
			m_iExtras++;
		ok = TeEqualsTm(5.3, 0.03, 0.2) && TeEqualsTm(4.0, 1.0, 0.05) && TeEqualsTm(100.0, 0.5, 0.1);
		Require(ok, "material slab extras", "TE и TM при нормальном падении совпадают в пределах 0.001 дБ");
		if (ok)
			m_iExtras++;
		ok = !m_aInputs.IsEmpty();
		for (int k = m_aInputs.Count() - 1; k >= 0; k--)
			ok = ok && SameResult(RICON_MaterialSlabKernel.Evaluate(m_aInputs[k]), m_aExpected[k]);
		Require(ok, "material slab extras", "повтор в обратном порядке сохраняет все результаты");
		if (ok)
			m_iExtras++;
	}
}
