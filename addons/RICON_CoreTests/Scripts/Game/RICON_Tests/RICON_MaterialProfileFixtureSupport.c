//! TEST_ONLY: корпус P3.2b — совпадение с frozen ожиданиями (MP07), неизменность входа, контроли сравнения,
//! цепь профиль → существующее ядро пластины (MS03) на той же и на другой частоте для каждого OK-случая с σ > 0,
//! повтор в обратном порядке, три контрпримера Power (codex #3980/#3981). Числа в логе — битовые образы binary32
//! (десятичная печать движка теряет субнормальные σ и не печатает 1e7), формат строк — codex #3982/#3986.
class RICON_MaterialProfileRun
{
	static const int BAD_ENUM = 99; //!< недопустимое значение provenance в native-корпусе
	int m_iCases;
	int m_iMatches;
	int m_iImmutable;
	int m_iControls;
	int m_iExtras;
	int m_iChains;
	bool m_bOk = true;
	float m_fMaxRelativeError;
	protected ref array<ref RICON_MaterialProfileInput> m_aInputs = {};
	protected ref array<ref RICON_MaterialProfileResult> m_aExpected = {};
	protected ref RICON_MaterialProfileInput m_Base;
	protected ref RICON_MaterialProfileResult m_BaseExpected;

	protected void Require(bool condition, string id, string what)
	{
		m_bOk = RICON_TestSupport.Expect(condition, id, what) && m_bOk;
	}

	//! Точный binary32 из битового образа (как в P3.2a): числа корпуса не зависят от разбора литералов.
	float F(int bits)
	{
		float value;
		RICON_DescriptorCodec.BitsToFloat(bits, value);
		return value;
	}

	RICON_MaterialProfileInput NewInput()
	{
		RICON_MaterialProfileInput input = new RICON_MaterialProfileInput();
		input.m_Profile = new RICON_MaterialFitProfile();
		return input;
	}

	RICON_MaterialProfileResult NewExpected(RICON_EMaterialProfileCode code)
	{
		RICON_MaterialProfileResult r = new RICON_MaterialProfileResult();
		r.m_eCode = code;
		if (code == RICON_EMaterialProfileCode.OK)
			r.m_Material = new RICON_MaterialProfile();
		return r;
	}

	//! MP07 native: относительная ошибка ≤ 2e-5 для положительного эталона, ноль — точно.
	static bool CloseParameter(float actual, float expected)
	{
		if (!RICON_Num.IsFinite(actual) || !RICON_Num.IsFinite(expected))
			return false;
		if (expected == 0)
			return actual == 0;
		if (expected < 0 || actual <= 0)
			return false;
		float error = Math.AbsFloat(actual - expected) / expected;
		return error <= 0.00002;
	}

	static bool SameResult(RICON_MaterialProfileResult a, RICON_MaterialProfileResult b)
	{
		if (!a || !b || a.m_eCode != b.m_eCode)
			return false;
		if (a.m_eCode != RICON_EMaterialProfileCode.OK)
			return true;
		RICON_MaterialProfile x = a.m_Material;
		RICON_MaterialProfile y = b.m_Material;
		if (!x || !y || x.m_eProvenance != y.m_eProvenance || x.m_fMinMHz != y.m_fMinMHz || x.m_fMaxMHz != y.m_fMaxMHz)
			return false;
		return CloseParameter(x.m_fEpsilonReal, y.m_fEpsilonReal) && CloseParameter(x.m_fConductivitySm, y.m_fConductivitySm);
	}

	//! Битовый образ binary32 десятичным int; все успешные выходы неотрицательны, знак печатается отдельно.
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

	static string Capture(RICON_MaterialProfileInput input)
	{
		if (!input)
			return "null";
		string text = Bits(input.m_fFrequencyMHz) + "#";
		RICON_MaterialFitProfile p = input.m_Profile;
		if (!p)
			return text + "null";
		int provenance = p.m_eProvenance;
		text = text + Bits(p.m_fA) + "|" + Bits(p.m_fB) + "|" + Bits(p.m_fC) + "|" + Bits(p.m_fD);
		text = text + "|" + Bits(p.m_fMinMHz) + "|" + Bits(p.m_fMaxMHz) + "|" + p.m_bMaxInclusive.ToString();
		text = text + "|" + provenance.ToString();
		return text;
	}

	//! Десятичная печать только для потерь пластины (дБ, |L| < 1e6); параметры материала печатаются битами.
	protected static string Text(float x)
	{
		if (RICON_Num.IsFinite(x) && Math.AbsFloat(x) < 1000000)
			return x.ToString(-1, 9);
		return "out_of_print_range";
	}

	protected static float RelativeError(float actual, float expected)
	{
		if (expected == 0)
			return 0;
		return Math.AbsFloat(actual - expected) / Math.AbsFloat(expected);
	}

	protected void Reject(bool accepted)
	{
		Require(!accepted, "material profile controls", "искажённое ожидание отвергнуто");
		if (!accepted)
			m_iControls++;
	}

	//! Пять контролей: null, код, provenance, ε′ вне допуска, σ ноль вместо положительного.
	protected void Controls(RICON_MaterialProfileResult actual, RICON_MaterialProfileResult expected)
	{
		Reject(SameResult(null, expected));
		actual.m_eCode = RICON_EMaterialProfileCode.NUMERIC_FAILURE;
		Reject(SameResult(actual, expected));
		actual.m_eCode = expected.m_eCode;
		RICON_MaterialProfile m = actual.m_Material;
		RICON_EMaterialProvenance provenance = m.m_eProvenance;
		m.m_eProvenance = RICON_EMaterialProvenance.ASSUMED_CONFIG;
		Reject(SameResult(actual, expected));
		m.m_eProvenance = provenance;
		float epsilon = m.m_fEpsilonReal;
		m.m_fEpsilonReal = epsilon * 1.0001;
		Reject(SameResult(actual, expected));
		m.m_fEpsilonReal = epsilon;
		float sigma = m.m_fConductivitySm;
		m.m_fConductivitySm = 0;
		Reject(SameResult(actual, expected));
		m.m_fConductivitySm = sigma;
	}

	void CheckCase(string id, RICON_MaterialProfileInput input, RICON_MaterialProfileResult expected)
	{
		m_iCases++;
		string before = Capture(input);
		RICON_MaterialProfileResult first = RICON_MaterialProfileKernel.Evaluate(input);
		bool matched = SameResult(first, expected);
		Require(matched, id, "код, material и provenance совпадают с эталоном");
		if (matched)
			m_iMatches++;
		if (matched && expected.m_eCode == RICON_EMaterialProfileCode.OK)
		{
			float e1 = RelativeError(first.m_Material.m_fEpsilonReal, expected.m_Material.m_fEpsilonReal);
			float e2 = RelativeError(first.m_Material.m_fConductivitySm, expected.m_Material.m_fConductivitySm);
			m_fMaxRelativeError = Math.Max(m_fMaxRelativeError, Math.Max(e1, e2));
		}
		first.m_eCode = RICON_EMaterialProfileCode.NUMERIC_FAILURE;
		if (first.m_Material)
			first.m_Material.m_fEpsilonReal = -1;
		RICON_MaterialProfileResult second = RICON_MaterialProfileKernel.Evaluate(input);
		bool immutable = Capture(input) == before && second != first && SameResult(second, expected);
		immutable = immutable && (!second.m_Material || second.m_Material != first.m_Material);
		Require(immutable, id, "вход не изменён, повторный результат и material новые");
		if (immutable)
			m_iImmutable++;
		if (expected.m_eCode == RICON_EMaterialProfileCode.OK && expected.m_Material.m_fConductivitySm > 0 && !m_Base && matched && immutable)
		{
			Controls(second, expected);
			Require(SameResult(second, expected), "material profile controls", "после контролей исходный результат восстановлен");
			m_Base = input;
			m_BaseExpected = expected;
		}
		m_aInputs.Insert(input);
		m_aExpected.Insert(expected);
		string eps = "NONE";
		string sig = "NONE";
		string lo = "NONE";
		string hi = "NONE";
		string prov = "NONE";
		if (second.m_eCode == RICON_EMaterialProfileCode.OK)
		{
			RICON_MaterialProfile m = second.m_Material;
			eps = Bits(m.m_fEpsilonReal);
			sig = Bits(m.m_fConductivitySm);
			lo = Bits(m.m_fMinMHz);
			hi = Bits(m.m_fMaxMHz);
			prov = typename.EnumToString(RICON_EMaterialProvenance, m.m_eProvenance);
		}
		PrintFormat("[RICON_MPROF] id=%1 code=%2 epsBits=%3 sigmaBits=%4 minBits=%5 maxBits=%6 provenance=%7 match=%8 immutable=%9", id,
			typename.EnumToString(RICON_EMaterialProfileCode, second.m_eCode), eps, sig, lo, hi, prov, matched, immutable);
	}

	//! MP05/MP07: профиль → существующее ядро пластины на той же частоте (OK, L ≥ 0) и на другой
	//! (OUT_OF_MODEL_DOMAIN, потому что minMHz = maxMHz = f). Толщина 0,1 м до 2000 МГц, выше 200/f, чтобы
	//! f·d ≤ 200 и 2f·d ≤ 400 оставались внутри MS06 (≤ 500) на обеих частотах; формулы пластины не копируются.
	protected bool ChainToSlab(RICON_MaterialProfileInput input)
	{
		RICON_MaterialProfileResult r = RICON_MaterialProfileKernel.Evaluate(input);
		if (r.m_eCode != RICON_EMaterialProfileCode.OK)
			return false;
		float f = input.m_fFrequencyMHz;
		float thickness = 0.1;
		if (f > 2000)
			thickness = 200.0 / f;
		RICON_MaterialSlabInput slab = new RICON_MaterialSlabInput();
		slab.m_fFrequencyMHz = f;
		slab.m_fThicknessM = thickness;
		slab.m_fCosIncidence = 1;
		slab.m_ePolarization = RICON_ESlabPolarization.TE;
		slab.m_Material = r.m_Material;
		RICON_MaterialSlabResult same = RICON_MaterialSlabKernel.Evaluate(slab);
		bool ok = same.m_eCode == RICON_EMaterialSlabCode.OK && same.m_fLossDb >= 0;
		float other = f * 2;
		if (other > 450000)
			other = f * 0.5;
		slab.m_fFrequencyMHz = other;
		RICON_MaterialSlabResult shifted = RICON_MaterialSlabKernel.Evaluate(slab);
		ok = ok && shifted.m_eCode == RICON_EMaterialSlabCode.OUT_OF_MODEL_DOMAIN;
		PrintFormat("[RICON_MPROF_CHAIN] fBits=%1 dBits=%2 epsBits=%3 sigmaBits=%4 slab=%5 loss=%6 shifted=%7 ok=%8", Bits(f), Bits(thickness),
			Bits(r.m_Material.m_fEpsilonReal), Bits(r.m_Material.m_fConductivitySm), typename.EnumToString(RICON_EMaterialSlabCode, same.m_eCode),
			Text(same.m_fLossDb), typename.EnumToString(RICON_EMaterialSlabCode, shifted.m_eCode), ok);
		return ok;
	}

	//! Контрпример Power (codex #3980/#3981): a = 4, b = 0, полоса 1..450000 включительно, TEST_ONLY; ожидание host —
	//! σ = f32(binary64 c·fGHz^d) битами, допуск MP07 (для субнормального σ bits 1 соседи отличаются на 100 %).
	protected void PowerExtra(int id, int fBits, int cBits, int dBits, RICON_EMaterialProfileCode code, int sigmaBits)
	{
		RICON_MaterialProfileInput input = NewInput();
		input.m_fFrequencyMHz = F(fBits);
		input.m_Profile.m_fA = 4;
		input.m_Profile.m_fB = 0;
		input.m_Profile.m_fC = F(cBits);
		input.m_Profile.m_fD = F(dBits);
		input.m_Profile.m_fMinMHz = 1;
		input.m_Profile.m_fMaxMHz = 450000;
		input.m_Profile.m_bMaxInclusive = true;
		input.m_Profile.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		RICON_MaterialProfileResult expected = NewExpected(code);
		if (code == RICON_EMaterialProfileCode.OK)
		{
			expected.m_Material.m_fEpsilonReal = 4;
			expected.m_Material.m_fConductivitySm = F(sigmaBits);
			expected.m_Material.m_fMinMHz = input.m_fFrequencyMHz;
			expected.m_Material.m_fMaxMHz = input.m_fFrequencyMHz;
			expected.m_Material.m_eProvenance = RICON_EMaterialProvenance.TEST_ONLY;
		}
		RICON_MaterialProfileResult actual = RICON_MaterialProfileKernel.Evaluate(input);
		bool ok = SameResult(actual, expected);
		string sig = "NONE";
		if (actual.m_eCode == RICON_EMaterialProfileCode.OK)
			sig = Bits(actual.m_Material.m_fConductivitySm);
		PrintFormat("[RICON_MPROF_POWER] id=%1 code=%2 sigmaBits=%3 ok=%4", id, typename.EnumToString(RICON_EMaterialProfileCode, actual.m_eCode), sig, ok);
		Require(ok, "material profile power " + id.ToString(), "контрпример Power совпадает с host-ожиданием");
		if (ok)
			m_iExtras++;
	}

	//! Шесть extras (codex #3986): null/восстановление, группа цепей, обратный повтор, три контрпримера Power.
	void CheckExtras()
	{
		bool ok = RICON_MaterialProfileKernel.Evaluate(null).m_eCode == RICON_EMaterialProfileCode.INVALID_INPUT && m_Base != null;
		if (m_Base)
		{
			RICON_MaterialFitProfile profile = m_Base.m_Profile;
			m_Base.m_Profile = null;
			ok = ok && RICON_MaterialProfileKernel.Evaluate(m_Base).m_eCode == RICON_EMaterialProfileCode.INVALID_PROFILE;
			m_Base.m_Profile = profile;
			ok = ok && SameResult(RICON_MaterialProfileKernel.Evaluate(m_Base), m_BaseExpected);
		}
		Require(ok, "material profile extras", "null вход и null профиль отвергнуты; базовый вход восстановлен");
		if (ok)
			m_iExtras++;
		ok = true;
		for (int i = 0; i < m_aInputs.Count(); i++)
		{
			RICON_MaterialProfileResult e = m_aExpected[i];
			if (e.m_eCode != RICON_EMaterialProfileCode.OK || e.m_Material.m_fConductivitySm <= 0)
				continue;
			bool chain = ChainToSlab(m_aInputs[i]);
			ok = ok && chain;
			m_iChains++;
		}
		Require(ok && m_iChains >= 3, "material profile extras", "цепи профиль → пластина для каждого OK-случая с σ > 0: OK на той же частоте, OUT_OF_MODEL_DOMAIN на другой");
		if (ok && m_iChains >= 3)
			m_iExtras++;
		ok = !m_aInputs.IsEmpty();
		for (int k = m_aInputs.Count() - 1; k >= 0; k--)
			ok = ok && SameResult(RICON_MaterialProfileKernel.Evaluate(m_aInputs[k]), m_aExpected[k]);
		Require(ok, "material profile extras", "повтор в обратном порядке сохраняет все результаты");
		if (ok)
			m_iExtras++;
		// f = 100, c = 9e-36, d = 10 → σ = 9e-46 → f32 1.4e-45 (bits 1), не ноль и не NUMERIC_FAILURE
		PowerExtra(1, 1120403456, 88041641, 1092616192, RICON_EMaterialProfileCode.OK, 1);
		// f = 1, c = 1e38, d = 15 → промежуточная степень 1e-45 субнормальна, произведение 9.999999406e-08 (bits 869711764)
		PowerExtra(2, 1065353216, 2123789977, 1097859072, RICON_EMaterialProfileCode.OK, 869711764);
		// f = 1000, c = 3.4e38, d = 1 → σ конечен (< FLT_MAX), значит NUMERIC_DOMAIN_LIMIT, а не NUMERIC_FAILURE
		PowerExtra(3, 1148846080, 2139081118, 1065353216, RICON_EMaterialProfileCode.NUMERIC_DOMAIN_LIMIT, 0);
	}
}
