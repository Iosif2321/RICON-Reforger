//! P2: вода вдоль заданного пути, REQ-path-observation v0.3 / ITU-R P.527-6 (8)–(27).
//! Один усреднённый профиль, без производственных значений по умолчанию и без строгих границ для P1.
enum RICON_EWaterCode
{
	INVALID_OBSERVATION = 0,
	INVALID_PROFILE = 1,
	WATER_COVERAGE_INCOMPLETE = 2,
	NO_WATER = 3,
	OUT_OF_MODEL_DOMAIN = 4,
	NUMERIC_FAILURE = 5,
	OK = 6
}

class RICON_WaterProfile
{
	float m_fTempC;
	float m_fSalinityPpt;
}

class RICON_WaterLayerResult
{
	RICON_EWaterCode m_eCode;
	bool m_bHasLoss;
	float m_fLossDb;
	//! Следующие значения присутствуют только при OK.
	float m_fAlphaDbPerM;
	float m_fInterfaceDb;
	float m_fWaterLengthM;
	int m_iCrossings;
	ref array<ref RICON_WaterInterval> m_aIntervals = {};
}

class RICON_WaterLayer
{
	protected static RICON_WaterLayerResult Make(RICON_EWaterCode code)
	{
		RICON_WaterLayerResult result = new RICON_WaterLayerResult();
		result.m_eCode = code;
		result.m_bHasLoss = code == RICON_EWaterCode.OK || code == RICON_EWaterCode.NO_WATER;
		return result;
	}

	protected static float Hypot(float x, float y)
	{
		float scale = Math.Max(Math.AbsFloat(x), Math.AbsFloat(y));
		if (scale == 0)
			return 0;
		float sx = x / scale;
		float sy = y / scale;
		return scale * Math.Sqrt(sx * sx + sy * sy);
	}

	//! Две релаксации Дебая, проводимость включена в ε″ ровно один раз.
	protected static bool Constants(float frequencyMHz, RICON_WaterProfile profile, out float alphaDbPerM, out float interfaceDb)
	{
		float t = profile.m_fTempC;
		float s = profile.m_fSalinityPpt;
		float t2 = t * t;
		float t3 = t2 * t;
		float t4 = t2 * t2;
		float s2 = s * s;
		float theta = 300.0 / (t + 273.15) - 1.0;
		float es = 77.66 + 103.3 * theta;
		float e1 = 0.0671 * es;
		float einf = 3.52 - 7.52 * theta;
		float f1 = 20.20 - 146.4 * theta + 316.0 * theta * theta;
		float f2 = 39.8 * f1;
		float ess = es * Math.Pow(Math.E, -3.33330e-3 * s + 4.74868e-6 * s2);
		float f1s = f1 * (1.0 + s * (2.3232e-3 - 7.9208e-5 * t + 3.6764e-6 * t2 + 3.5594e-7 * t3 + 8.9795e-9 * t4));
		float e1s = e1 * Math.Pow(Math.E, -6.28908e-3 * s + 1.76032e-4 * s2 - 9.22144e-5 * t * s);
		float f2s = f2 * (1.0 + s * (-1.99723e-2 + 1.81176e-4 * t));
		float einfs = einf * (1.0 + s * (-2.04265e-3 + 1.57883e-4 * t));
		float sigma35 = 2.903602 + 8.607e-2 * t + 4.738817e-4 * t2 - 2.991e-6 * t3 + 4.3047e-9 * t4;
		float r15OverS = (37.5109 + 5.45216 * s + 1.4409e-2 * s2) / (1004.75 + 182.283 * s + s2);
		float a0 = (6.9431 + 3.2841 * s - 9.9486e-2 * s2) / (84.850 + 69.024 * s + s2);
		float a1 = 49.843 - 0.2276 * s + 0.198e-2 * s2;
		float rt15 = 1.0 + a0 * (t - 15.0) / (a1 + t);
		//! Перенос масштаба 1000 в коэффициенты сохраняет ненулевую MHz при переводе очень малых частот.
		float ratio1 = frequencyMHz / (1000.0 * f1s);
		float ratio2 = frequencyMHz / (1000.0 * f2s);
		float debye1 = (ess - e1s) / (1.0 + ratio1 * ratio1);
		float debye2 = (e1s - einfs) / (1.0 + ratio2 * ratio2);
		float er = einfs + debye1 + debye2;
		float conduction = (18000.0 * sigma35 * r15OverS * rt15) * (s / frequencyMHz);
		float ei = ratio1 * debye1 + ratio2 * debye2 + conduction;
		if (!RICON_Num.IsFinite(er) || !RICON_Num.IsFinite(ei) || er <= 0 || ei < 0)
			return false;
		float magnitude = Hypot(er, ei);
		if (!RICON_Num.IsFinite(magnitude))
			return false;
		float nr = Math.Sqrt(magnitude / 2.0 + er / 2.0);
		//! |Im √ε| = ε″/(2·Re √ε), без потери малой мнимой части из-за вычитания |ε|−ε′.
		float ni = ei / (2.0 * nr);
		alphaDbPerM = (20.0 / Math.Log(10.0)) * (2.0 * Math.PI / 299.792458) * frequencyMHz * ni;
		//! −10·lg(4 Re n/|1+n|²); логарифмы избегают квадратного overflow и вычитания 1−|Γ|².
		float denominator = Hypot(1.0 + nr, ni);
		interfaceDb = 20.0 * Math.Log10(denominator) - 10.0 * Math.Log10(4.0 * nr);
		return RICON_Num.IsFinite(alphaDbPerM) && alphaDbPerM >= 0 && RICON_Num.IsFinite(interfaceDb) && interfaceDb >= 0;
	}

	//! Чистый вычислитель: не меняет снимки, доставку, RX, голос или входное наблюдение.
	static RICON_WaterLayerResult Evaluate(RICON_PathObservation input, RICON_WaterProfile profile)
	{
		float lengthM;
		array<ref RICON_WaterInterval> water;
		if (!RICON_PathObservationModel.Validate(input, lengthM, water))
			return Make(RICON_EWaterCode.INVALID_OBSERVATION);
		if (!profile || !RICON_Num.IsFinite(profile.m_fTempC) || !RICON_Num.IsFinite(profile.m_fSalinityPpt))
			return Make(RICON_EWaterCode.INVALID_PROFILE);
		if (input.m_aCoverage[RICON_EPathFamily.WATER] != RICON_EPathCoverage.COMPLETE
			|| input.m_Tx.m_eMedium == RICON_EPathMedium.UNKNOWN || input.m_Rx.m_eMedium == RICON_EPathMedium.UNKNOWN)
			return Make(RICON_EWaterCode.WATER_COVERAGE_INCOMPLETE);
		if (water.IsEmpty())
			return Make(RICON_EWaterCode.NO_WATER);
		if (profile.m_fTempC < -4 || profile.m_fTempC > 40 || profile.m_fSalinityPpt < 0
			|| profile.m_fSalinityPpt > 40 || input.m_fFrequencyMHz > 1000000)
			return Make(RICON_EWaterCode.OUT_OF_MODEL_DOMAIN);
		float alphaDbPerM;
		float interfaceDb;
		if (!Constants(input.m_fFrequencyMHz, profile, alphaDbPerM, interfaceDb))
			return Make(RICON_EWaterCode.NUMERIC_FAILURE);
		float waterLengthM;
		int crossings;
		foreach (RICON_WaterInterval interval : water)
		{
			waterLengthM += interval.m_fEndM - interval.m_fStartM;
			if (interval.m_fStartM > 0)
				crossings++;
			if (interval.m_fEndM < lengthM)
				crossings++;
		}
		float lossDb = alphaDbPerM * waterLengthM + crossings * interfaceDb;
		if (!RICON_Num.IsFinite(waterLengthM) || !RICON_Num.IsFinite(lossDb) || waterLengthM < 0 || lossDb < 0)
			return Make(RICON_EWaterCode.NUMERIC_FAILURE);
		RICON_WaterLayerResult result = Make(RICON_EWaterCode.OK);
		result.m_fLossDb = lossDb;
		result.m_fAlphaDbPerM = alphaDbPerM;
		result.m_fInterfaceDb = interfaceDb;
		result.m_fWaterLengthM = waterLengthM;
		result.m_iCrossings = crossings;
		result.m_aIntervals = water;
		return result;
	}
}
