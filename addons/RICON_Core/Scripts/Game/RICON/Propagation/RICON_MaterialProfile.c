//! P3.2b: частотные параметры материала по P.2040-4 Annex §3 (57)–(58): ε′ = a·f^b, σ = c·f^d, f в ГГц
//! (REQ-material-profile v0.2). Чистая функция MP02–MP05: заполняет RICON_MaterialProfile контракта MS03 для одной
//! частоты (minMHz = maxMHz = f), ядро пластины не меняет. Каталог Table 3, выбор ID и геометрия — вне ядра.
enum RICON_EMaterialProfileCode
{
	INVALID_INPUT = 0,
	INVALID_PROFILE = 1,
	OUT_OF_MODEL_DOMAIN = 2,
	NUMERIC_FAILURE = 3,
	NUMERIC_DOMAIN_LIMIT = 4,
	OK = 5
}

//! Степенной профиль одной строки каталога; числа — уже binary32 (MP02), происхождение задаёт вызывающая сторона.
class RICON_MaterialFitProfile
{
	float m_fA;
	float m_fB;
	float m_fC;
	float m_fD;
	float m_fMinMHz;
	float m_fMaxMHz;
	bool m_bMaxInclusive;
	RICON_EMaterialProvenance m_eProvenance;
}

class RICON_MaterialProfileInput
{
	float m_fFrequencyMHz;
	ref RICON_MaterialFitProfile m_Profile;
}

//! При отказе значим только m_eCode (MP03); при OK результат — ровно {code, material} (MP05).
class RICON_MaterialProfileResult
{
	RICON_EMaterialProfileCode m_eCode;
	ref RICON_MaterialProfile m_Material;
}

class RICON_MaterialProfileKernel
{
	static const float MIN_FREQUENCY_MHZ = 1.0;
	static const float MAX_FREQUENCY_MHZ = 450000.0;
	static const float MIN_EPSILON = 1.0;
	static const float MAX_EPSILON = 100.0;
	static const float MAX_SIGMA = 100000000.0;
	static const float SCALE_SHIFT = 20.0;     //!< при t < 0: 10^t = 10^(t+20) · 1e-20, см. Power
	static const float SCALE_DOWN = 1.0e-20;   //!< binary32(1e-20), относительная погрешность −3.2e-8

	protected static RICON_MaterialProfileResult Make(RICON_EMaterialProfileCode code)
	{
		RICON_MaterialProfileResult r = new RICON_MaterialProfileResult();
		r.m_eCode = code;
		return r;
	}

	protected static bool Finite4(float a, float b, float c, float d)
	{
		return RICON_Num.IsFinite(a) && RICON_Num.IsFinite(b) && RICON_Num.IsFinite(c) && RICON_Num.IsFinite(d);
	}

	//! MP04: coef·f^exp. Точные случаи — coef = 0 и exp = 0. Иначе только устойчивая форма 10^t,
	//! t = log10 coef + exp·log10 f: отдельная степень f^exp не вычисляется, поэтому её переполнение или
	//! субнормальность не портят конечное произведение. При t < 0 (результат < 1, переполнение невозможно) степень
	//! берётся как 10^(t+20)·1e-20: округление к субнормальному или к нулю выполняет умножение IEEE, а не Math.Pow,
	//! который ниже FLT_TRUE_MIN отдаёт ноль вместо округления вверх (прогон 2026-10-23 21:08:40, контрпример 1
	//! codex #3980: 9e-46 → NUMERIC_FAILURE при ожидании bits 1), тогда как субнормальный 1e-40 он считает верно.
	//! Порогов классификации нет — классифицируется фактический результат binary32: не конечный → переполнение
	//! (false); ноль → underflow положительного значения (решает вызывающий по c > 0).
	protected static bool Power(float coef, float exponent, float fghz, out float value)
	{
		value = 0;
		if (coef == 0)
			return true;
		if (exponent == 0)
		{
			value = coef;
			return true;
		}
		float logCoef = Math.Log10(coef);
		float logF = Math.Log10(fghz);
		float t = logCoef + exponent * logF;
		if (!RICON_Num.IsFinite(t))
			return false;
		float grown;
		if (t < 0)
		{
			float shifted = Math.Pow(10.0, t + SCALE_SHIFT);
			grown = shifted * SCALE_DOWN;
		}
		else
		{
			grown = Math.Pow(10.0, t);
		}
		if (!RICON_Num.IsFinite(grown))
			return false;
		value = grown;
		return true;
	}

	//! MP03: первый подходящий код определяет результат. Вход не изменяется, material создаётся заново.
	static RICON_MaterialProfileResult Evaluate(RICON_MaterialProfileInput input)
	{
		if (!input)
			return Make(RICON_EMaterialProfileCode.INVALID_INPUT);
		float f = input.m_fFrequencyMHz;
		if (!RICON_Num.IsFinite(f) || f <= 0)
			return Make(RICON_EMaterialProfileCode.INVALID_INPUT);
		RICON_MaterialFitProfile p = input.m_Profile;
		if (!p)
			return Make(RICON_EMaterialProfileCode.INVALID_PROFILE);
		float a = p.m_fA;
		float b = p.m_fB;
		float c = p.m_fC;
		float d = p.m_fD;
		float lo = p.m_fMinMHz;
		float hi = p.m_fMaxMHz;
		if (!Finite4(a, b, c, d) || !RICON_Num.IsFinite(lo) || !RICON_Num.IsFinite(hi))
			return Make(RICON_EMaterialProfileCode.INVALID_PROFILE);
		if (a <= 0 || c < 0 || lo <= 0 || lo > hi)
			return Make(RICON_EMaterialProfileCode.INVALID_PROFILE);
		if (lo == hi && !p.m_bMaxInclusive)
			return Make(RICON_EMaterialProfileCode.INVALID_PROFILE);
		if (p.m_eProvenance != RICON_EMaterialProvenance.SOURCE_DOMAIN && p.m_eProvenance != RICON_EMaterialProvenance.ASSUMED_CONFIG
			&& p.m_eProvenance != RICON_EMaterialProvenance.TEST_ONLY)
			return Make(RICON_EMaterialProfileCode.INVALID_PROFILE);
		if (f < MIN_FREQUENCY_MHZ || f > MAX_FREQUENCY_MHZ || f < lo)
			return Make(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		if (p.m_bMaxInclusive)
		{
			if (f > hi)
				return Make(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		}
		else
		{
			if (f >= hi)
				return Make(RICON_EMaterialProfileCode.OUT_OF_MODEL_DOMAIN);
		}
		float fghz = f / 1000.0;
		float epsilon;
		float sigma;
		if (!Power(a, b, fghz, epsilon) || !Power(c, d, fghz, sigma))
			return Make(RICON_EMaterialProfileCode.NUMERIC_FAILURE);
		// MP03.4: положительность — по коэффициентам (a > 0 всегда, c > 0), не по вычисленному значению;
		// underflow в ноль (в том числе FTZ движка) — потеря, не нулевая проводимость.
		if (!RICON_Num.IsFinite(epsilon) || epsilon <= 0)
			return Make(RICON_EMaterialProfileCode.NUMERIC_FAILURE);
		if (!RICON_Num.IsFinite(sigma) || (c > 0 && sigma <= 0))
			return Make(RICON_EMaterialProfileCode.NUMERIC_FAILURE);
		if (epsilon < MIN_EPSILON || epsilon > MAX_EPSILON || sigma > MAX_SIGMA)
			return Make(RICON_EMaterialProfileCode.NUMERIC_DOMAIN_LIMIT);
		RICON_MaterialProfileResult result = Make(RICON_EMaterialProfileCode.OK);
		RICON_MaterialProfile material = new RICON_MaterialProfile();
		material.m_fEpsilonReal = epsilon;
		material.m_fConductivitySm = sigma;
		material.m_fMinMHz = f;
		material.m_fMaxMHz = f;
		material.m_eProvenance = p.m_eProvenance;
		result.m_Material = material;
		return result;
	}
}
