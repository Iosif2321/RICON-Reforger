//! P3.2a: потери мощности при прохождении плоской однородной немагнитной пассивной пластины, воздух с обеих сторон
//! (REQ-material-slab v0.2 FROZEN, LF-SHA 9eeeb8c5…; источник P.2040-4 §2.2.1.5 (37), §2.2.2.2 (43)–(44), §3 (59)).
//! Чистая функция MS03–MS07: геометрия движка, каталог свойств, несколько слоёв, обходные пути, фаза и голос
//! сюда не входят (MS01, MS09). Параметры материала заданы для данной частоты; их статус ядро не удостоверяет.
enum RICON_EMaterialSlabCode
{
	INVALID_INPUT = 0,
	INVALID_PROFILE = 1,
	OUT_OF_MODEL_DOMAIN = 2,
	NUMERIC_DOMAIN_LIMIT = 3,
	NUMERIC_FAILURE = 4,
	OK = 5
}

//! Относительно плоскости падения (MS03), не мировые H/V.
enum RICON_ESlabPolarization
{
	TE = 0,
	TM = 1
}

//! Задаёт вызывающая сторона; сохраняется в успешном результате (MS03).
enum RICON_EMaterialProvenance
{
	SOURCE_DOMAIN = 0,
	ASSUMED_CONFIG = 1,
	TEST_ONLY = 2
}

class RICON_MaterialProfile
{
	float m_fEpsilonReal;
	float m_fConductivitySm;
	float m_fMinMHz;
	float m_fMaxMHz;
	RICON_EMaterialProvenance m_eProvenance;
}

class RICON_MaterialSlabInput
{
	float m_fFrequencyMHz;
	float m_fThicknessM;   //!< толщина по нормали к поверхности, м
	float m_fCosIncidence; //!< модуль косинуса угла к нормали
	RICON_ESlabPolarization m_ePolarization;
	ref RICON_MaterialProfile m_Material;
}

//! При отказе значим только m_eCode (MS04); при OK результат — ровно {code, lossDb, provenance}.
class RICON_MaterialSlabResult
{
	RICON_EMaterialSlabCode m_eCode;
	float m_fLossDb;
	RICON_EMaterialProvenance m_eProvenance;
}

//! Комплексное число в binary32. В срезе MS03 ни одна операция не переполняется; деление по Смиту и корень без
//! вычитания близких чисел — страховка на расширение области (MS05, деление SHOULD).
class RICON_SlabComplex
{
	float m_fRe;
	float m_fIm;

	static RICON_SlabComplex Make(float re, float im)
	{
		RICON_SlabComplex c = new RICON_SlabComplex();
		c.m_fRe = re;
		c.m_fIm = im;
		return c;
	}

	static RICON_SlabComplex CAdd(RICON_SlabComplex a, RICON_SlabComplex b)
	{
		return Make(a.m_fRe + b.m_fRe, a.m_fIm + b.m_fIm);
	}

	static RICON_SlabComplex CSub(RICON_SlabComplex a, RICON_SlabComplex b)
	{
		return Make(a.m_fRe - b.m_fRe, a.m_fIm - b.m_fIm);
	}

	static RICON_SlabComplex CMul(RICON_SlabComplex a, RICON_SlabComplex b)
	{
		float rr = a.m_fRe * b.m_fRe;
		float ii = a.m_fIm * b.m_fIm;
		float ri = a.m_fRe * b.m_fIm;
		float ir = a.m_fIm * b.m_fRe;
		return Make(rr - ii, ri + ir);
	}

	//! Деление по Смиту: без |b|² в знаменателе.
	static RICON_SlabComplex CDiv(RICON_SlabComplex a, RICON_SlabComplex b)
	{
		if (Math.AbsFloat(b.m_fRe) >= Math.AbsFloat(b.m_fIm))
		{
			float t = b.m_fIm / b.m_fRe;
			float den = b.m_fRe + b.m_fIm * t;
			float re = a.m_fRe + a.m_fIm * t;
			float im = a.m_fIm - a.m_fRe * t;
			return Make(re / den, im / den);
		}
		float s = b.m_fRe / b.m_fIm;
		float den2 = b.m_fIm + b.m_fRe * s;
		float re2 = a.m_fRe * s + a.m_fIm;
		float im2 = a.m_fIm * s - a.m_fRe;
		return Make(re2 / den2, im2 / den2);
	}

	//! Устойчивый модуль: big · sqrt(1 + ratio²).
	static float CAbs(RICON_SlabComplex a)
	{
		float ar = Math.AbsFloat(a.m_fRe);
		float ai = Math.AbsFloat(a.m_fIm);
		float big = Math.Max(ar, ai);
		if (big == 0)
			return 0;
		float small = Math.Min(ar, ai);
		float ratio = small / big;
		return big * Math.Sqrt(1 + ratio * ratio);
	}

	//! Главный корень без вычитания: Re z > 0 в срезе MS03 (ε′ − 1 + C² ≥ C² > 0), Re p > 0, Im p ≤ 0.
	static RICON_SlabComplex CSqrt(RICON_SlabComplex z)
	{
		float modulus = CAbs(z);
		float half = (modulus + z.m_fRe) * 0.5;
		float re = Math.Sqrt(half);
		float im = z.m_fIm / (2 * re);
		return Make(re, im);
	}
}

class RICON_MaterialSlabKernel
{
	static const float SPEED_OF_LIGHT_MPS = 299792458.0;
	static const float TWO_PI = 6.283185307179586;
	static const float SIGMA_TO_EPSILON_IMAG = 17.98;     //!< (59): округлённое соглашение, не SI-константа (MS05)
	static const float DB_PER_NEPER = 8.685889638065037;  //!< 20 / ln 10
	static const float X_SATURATION = 12.0;               //!< MS06: при x ≥ 12 принимается D = 1 (ошибка < 4e-10 дБ)
	static const float SERIES_LIMIT = 0.0625;             //!< 1 − exp(−t) рядом при t ≤ 1/16
	static const float NEGATIVE_TOLERANCE_DB = 0.001;     //!< MS07
	static const float MIN_FREQUENCY_MHZ = 1.0;
	static const float MAX_FREQUENCY_MHZ = 450000.0;
	static const float MAX_FREQUENCY_THICKNESS = 500.0;   //!< binary32(fMHz · dM) ≤ 500 (MS03/MS06)
	static const float MIN_COSINE = 0.05;
	static const float MAX_EPSILON = 100.0;
	static const float MAX_SIGMA = 100000000.0;

	protected static RICON_MaterialSlabResult Make(RICON_EMaterialSlabCode code)
	{
		RICON_MaterialSlabResult r = new RICON_MaterialSlabResult();
		r.m_eCode = code;
		return r;
	}

	//! 1 − exp(−t) без вычитания около нуля (MS05): ряд до t⁷/5040 при t ≤ 1/16 (остаток ≤ t⁸/40320 < 6e-15,
	//! codex #3830), иначе напрямую.
	protected static float OneMinusExpNeg(float t)
	{
		if (t <= SERIES_LIMIT)
		{
			// t − t²/2 + t³/6 − t⁴/24 + t⁵/120 − t⁶/720 + t⁷/5040, схема Горнера по шагам (парсер Enforce
			// не принимает глубоко вложенную формулу).
			float h = 1.0 - t / 7.0;
			h = 1.0 - t / 6.0 * h;
			h = 1.0 - t / 5.0 * h;
			h = 1.0 - t / 4.0 * h;
			h = 1.0 - t / 3.0 * h;
			h = 1.0 - t / 2.0 * h;
			return t * h;
		}
		float decay = Math.Pow(Math.E, -t);
		return 1.0 - decay;
	}

	protected static bool Finite4(float a, float b, float c, float d)
	{
		return RICON_Num.IsFinite(a) && RICON_Num.IsFinite(b) && RICON_Num.IsFinite(c) && RICON_Num.IsFinite(d);
	}

	//! MS04: первый подходящий код определяет результат. Вход не изменяется.
	static RICON_MaterialSlabResult Evaluate(RICON_MaterialSlabInput input)
	{
		if (!input)
			return Make(RICON_EMaterialSlabCode.INVALID_INPUT);
		float f = input.m_fFrequencyMHz;
		float d = input.m_fThicknessM;
		float c = input.m_fCosIncidence;
		// Конечность первой: NaN в Workbench сравнивается как +Inf (RICON_TestSupport).
		if (!RICON_Num.IsFinite(f) || !RICON_Num.IsFinite(d) || !RICON_Num.IsFinite(c))
			return Make(RICON_EMaterialSlabCode.INVALID_INPUT);
		if (f <= 0 || d < 0 || c < 0 || c > 1)
			return Make(RICON_EMaterialSlabCode.INVALID_INPUT);
		if (input.m_ePolarization != RICON_ESlabPolarization.TE && input.m_ePolarization != RICON_ESlabPolarization.TM)
			return Make(RICON_EMaterialSlabCode.INVALID_INPUT);
		RICON_MaterialProfile m = input.m_Material;
		if (!m)
			return Make(RICON_EMaterialSlabCode.INVALID_PROFILE);
		float er = m.m_fEpsilonReal;
		float sg = m.m_fConductivitySm;
		float lo = m.m_fMinMHz;
		float hi = m.m_fMaxMHz;
		if (!Finite4(er, sg, lo, hi) || er < 1 || sg < 0 || lo <= 0 || lo > hi)
			return Make(RICON_EMaterialSlabCode.INVALID_PROFILE);
		if (m.m_eProvenance != RICON_EMaterialProvenance.SOURCE_DOMAIN && m.m_eProvenance != RICON_EMaterialProvenance.ASSUMED_CONFIG
			&& m.m_eProvenance != RICON_EMaterialProvenance.TEST_ONLY)
			return Make(RICON_EMaterialSlabCode.INVALID_PROFILE);
		if (f < MIN_FREQUENCY_MHZ || f > MAX_FREQUENCY_MHZ || f < lo || f > hi)
			return Make(RICON_EMaterialSlabCode.OUT_OF_MODEL_DOMAIN);
		// Guard на binary32-произведении самих входов, не на производной фазе (MS06).
		float product = f * d;
		if (product > MAX_FREQUENCY_THICKNESS || c < MIN_COSINE || er > MAX_EPSILON || sg > MAX_SIGMA)
			return Make(RICON_EMaterialSlabCode.NUMERIC_DOMAIN_LIMIT);
		RICON_MaterialSlabResult result = Make(RICON_EMaterialSlabCode.OK);
		result.m_eProvenance = m.m_eProvenance;
		// MS07: только d = 0 или воздух дают тождественную передачу, и только после всех guard.
		if (d == 0 || (er == 1 && sg == 0))
			return result;
		// MS05: e = ε′ − j·17.98·σ/f_GHz, p = sqrt(e − 1 + C²), q = k0·d·p = y − j·x.
		float frequencyGHz = f / 1000.0;
		float epsilonImag = -(SIGMA_TO_EPSILON_IMAG * sg / frequencyGHz);
		float zRe = (er - 1.0) + c * c;
		RICON_SlabComplex p = RICON_SlabComplex.CSqrt(RICON_SlabComplex.Make(zRe, epsilonImag));
		float frequencyHz = f * 1000000.0;
		float k0 = TWO_PI * frequencyHz / SPEED_OF_LIGHT_MPS;
		float kd = k0 * d;
		float y = kd * p.m_fRe;
		float x = -(kd * p.m_fIm);
		RICON_SlabComplex a;
		if (input.m_ePolarization == RICON_ESlabPolarization.TE)
			a = RICON_SlabComplex.Make(c, 0);
		else
			a = RICON_SlabComplex.Make(er * c, epsilonImag * c);
		RICON_SlabComplex s = RICON_SlabComplex.CAdd(a, p);
		RICON_SlabComplex u = RICON_SlabComplex.CDiv(a, s);
		RICON_SlabComplex v = RICON_SlabComplex.CDiv(p, s);
		RICON_SlabComplex r = RICON_SlabComplex.CSub(u, v);
		RICON_SlabComplex uv = RICON_SlabComplex.CMul(u, v);
		RICON_SlabComplex bigA = RICON_SlabComplex.Make(4 * uv.m_fRe, 4 * uv.m_fIm);
		RICON_SlabComplex bigD = RICON_SlabComplex.Make(1, 0);
		if (x < X_SATURATION)
		{
			// W = (1 − e^{−2x}) + 2e^{−2x}sin²y + j·e^{−2x}sin 2y; D = A + r²W (MS05).
			float t = 2 * x;
			float decay = Math.Pow(Math.E, -t);
			float siny = Math.Sin(y);
			float sin2y = Math.Sin(2 * y);
			float wRe = OneMinusExpNeg(t) + (2 * decay) * (siny * siny);
			float wIm = decay * sin2y;
			RICON_SlabComplex w = RICON_SlabComplex.Make(wRe, wIm);
			RICON_SlabComplex r2 = RICON_SlabComplex.CMul(r, r);
			RICON_SlabComplex r2w = RICON_SlabComplex.CMul(r2, w);
			bigD = RICON_SlabComplex.CAdd(bigA, r2w);
		}
		float absD = RICON_SlabComplex.CAbs(bigD);
		float absA = RICON_SlabComplex.CAbs(bigA);
		if (!RICON_Num.IsFinite(absD) || !RICON_Num.IsFinite(absA) || absD <= 0 || absA <= 0)
			return Make(RICON_EMaterialSlabCode.NUMERIC_FAILURE);
		// L = (20/ln10)·(x + ln|D| − ln|A|): потери в логарифмах, без exp(−x) для всего T (MS05, MS07).
		float logD = Math.Log10(absD);
		float logA = Math.Log10(absA);
		float loss = DB_PER_NEPER * x + 20.0 * (logD - logA);
		if (!RICON_Num.IsFinite(loss) || loss < -NEGATIVE_TOLERANCE_DB)
			return Make(RICON_EMaterialSlabCode.NUMERIC_FAILURE);
		if (loss < 0)
			loss = 0;
		result.m_fLossDb = loss;
		return result;
	}
}
