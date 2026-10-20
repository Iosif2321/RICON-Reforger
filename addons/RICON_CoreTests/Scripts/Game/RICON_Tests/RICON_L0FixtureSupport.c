//! TEST_ONLY: сравнение принятого L0-корпуса с настоящими публичными методами Core.
class RICON_L0Run
{
	//! Vector.Distance в float32: 1 км дал +0.000061035 м; отдельный допуск Enforce, как в R21.
	static const float DISTANCE_TOLERANCE_M = 0.001;
	static float s_fMaxDeviationDb;
	protected string m_sId;
	protected bool m_bQuiet;
	protected bool m_bChecked;
	protected bool m_bFailed;

	void RICON_L0Run(string id, bool quiet = false)
	{
		m_sId = id;
		m_bQuiet = quiet;
	}

	bool IsPassed()
	{
		return m_bChecked && !m_bFailed;
	}

	bool IsFailed()
	{
		return m_bFailed;
	}

	void Check(bool condition, string field)
	{
		m_bChecked = true;
		if (!condition)
		{
			m_bFailed = true;
			if (!m_bQuiet)
				PrintFormat("[RICON_L0] FAIL %1: %2", m_sId, field);
		}
	}

	void CheckNumber(string field, float actual, float expected, float tolerance)
	{
		bool finite = RICON_Num.IsFinite(actual) && RICON_Num.IsFinite(expected);
		if (!m_bQuiet)
		{
			PrintFormat("[RICON_L0] %1 %2: actual=%3 expected=%4", m_sId, field,
				actual.ToString(-1, 9), expected.ToString(-1, 9));
			if (finite && tolerance == 0.01)
				s_fMaxDeviationDb = Math.Max(s_fMaxDeviationDb, Math.AbsFloat(actual - expected));
		}
		Check(finite && Math.AbsFloat(actual - expected) <= tolerance, field);
	}

	static float Special(int sign)
	{
		if (sign == 0)
			return RICON_L2Run.NaN();
		float largest = 3.402823e38;
		return sign * largest * 2.0;
	}

	void RequireSpecial(float value, int sign)
	{
		int expectedClass = 1;
		if (sign == 0)
			expectedClass = 1;
		else if (sign > 0)
			expectedClass = 2;
		else
			expectedClass = 3;
		Check(RICON_TestSupport.FloatClass(value) == expectedClass, "тип NaN / Infinity после присваивания");
	}

	static RICON_NoiseModel Noise(string environmentId)
	{
		if (environmentId == "rural")
			return new RICON_ManMadeNoiseModel("rural", 67.2, 27.7);
		if (environmentId == "city")
			return new RICON_ManMadeNoiseModel("city", 76.8, 27.7);
		return new RICON_NoiseModel();
	}

	//! Неизвестная категория намеренно не регистрируется: evaluator должен отвергнуть её.
	RICON_LinkBudget Evaluate(RICON_LinkQuery query, RICON_LinkQuery before)
	{
		RICON_FreeSpaceModel propagation = new RICON_FreeSpaceModel();
		RICON_NoiseModel noise = Noise(query.m_sNoiseEnvironmentId);
		RICON_LinearTestIntelligibility quality = RICON_TestSupport.NewTestIntelligibility();
		RICON_WirelessLinkEvaluator evaluator = new RICON_WirelessLinkEvaluator("test_vhf", propagation, noise, quality);
		RICON_LinkBudget result = evaluator.Evaluate(query);
		CheckSameQuery(query, before);
		return result;
	}

	protected static bool SameFloat(float a, float b)
	{
		return RICON_TestSupport.SameFloat(a, b);
	}

	//! Проверяет именно guard и сравнение целого query, которые ранее смешивали NaN и Infinity.
	static bool SpecialSelfCheck()
	{
		array<float> values = {Special(0), Special(1), Special(-1)};
		array<int> signs = {0, 1, -1};
		int positive;
		int rejected;
		for (int i = 0; i < 3; i++)
		{
			RICON_L0Run guard = new RICON_L0Run("special-positive", true);
			guard.RequireSpecial(values[i], signs[i]);
			if (guard.IsPassed()) positive++;
			RICON_L0Run wrongGuard = new RICON_L0Run("special-negative", true);
			wrongGuard.RequireSpecial(values[(i + 1) % 3], signs[i]);
			if (wrongGuard.IsFailed()) rejected++;
			RICON_LinkQuery a = new RICON_LinkQuery();
			RICON_LinkQuery b = new RICON_LinkQuery();
			a.m_fFrequencyMHz = values[i];
			b.m_fFrequencyMHz = values[i];
			RICON_L0Run same = new RICON_L0Run("query-positive", true);
			same.CheckSameQuery(a, b);
			if (same.IsPassed()) positive++;
			b.m_fFrequencyMHz = values[(i + 1) % 3];
			RICON_L0Run changed = new RICON_L0Run("query-negative", true);
			changed.CheckSameQuery(a, b);
			if (changed.IsFailed()) rejected++;
		}
		PrintFormat("[RICON_L0] specialPositive=%1 specialControls=%2", positive, rejected);
		return positive == 6 && rejected == 6;
	}

	protected void CheckSameQuery(RICON_LinkQuery a, RICON_LinkQuery b)
	{
		for (int i = 0; i < 3; i++)
			Check(SameFloat(a.m_vTxPos[i], b.m_vTxPos[i]) && SameFloat(a.m_vRxPos[i], b.m_vRxPos[i]), "позиции не изменены");
		Check(SameFloat(a.m_fFrequencyMHz, b.m_fFrequencyMHz), "frequencyMHz не изменена");
		Check(SameFloat(a.m_fBandwidthHz, b.m_fBandwidthHz), "bandwidthHz не изменена");
		Check(SameFloat(a.m_fTxPowerDbm, b.m_fTxPowerDbm), "txPowerDbm не изменена");
		Check(SameFloat(a.m_fTxGainDbi, b.m_fTxGainDbi), "txGainDbi не изменён");
		Check(SameFloat(a.m_fRxGainDbi, b.m_fRxGainDbi), "rxGainDbi не изменён");
		Check(SameFloat(a.m_fRxNoiseFigureDb, b.m_fRxNoiseFigureDb), "NF не изменён");
		Check(SameFloat(a.m_fRxSensitivityDbm, b.m_fRxSensitivityDbm), "sensitivity не изменена");
		Check(SameFloat(a.m_fRequiredSinrDb, b.m_fRequiredSinrDb), "requiredSinr не изменён");
		Check(SameFloat(a.m_fInterferenceDbm, b.m_fInterferenceDbm), "interference не изменена");
		Check(a.m_bHasRxSensitivity == b.m_bHasRxSensitivity && a.m_bHasRequiredSinr == b.m_bHasRequiredSinr
			&& a.m_bHasInterference == b.m_bHasInterference, "флаги не изменены");
		Check(a.m_sBandId == b.m_sBandId && a.m_sModelId == b.m_sModelId
			&& a.m_sNoiseEnvironmentId == b.m_sNoiseEnvironmentId
			&& a.m_sIntelligibilityModelId == b.m_sIntelligibilityModelId, "id не изменены");
	}

	void CheckBudget(RICON_LinkBudget actual, RICON_LinkBudget expected)
	{
		Check(actual != null, "бюджет существует");
		if (!actual)
			return;
		Check(actual.m_bValid == expected.m_bValid, "valid");
		Check(actual.m_eReason == expected.m_eReason, "reason");
		Check(actual.m_eState == expected.m_eState, "state");
		CheckNumber("intelligibility", actual.m_fIntelligibility, expected.m_fIntelligibility, 0.000001);
		array<float> numbers = {actual.m_fDistanceM, actual.m_fTotalLossDb, actual.m_fRxPowerDbm,
			actual.m_fThermalNoiseDbm, actual.m_fNoiseDbm, actual.m_fNoisePlusInterferenceDbm,
			actual.m_fSinrDb, actual.m_fSensitivityDbm, actual.m_fIntelligibility};
		foreach (float number : numbers)
			Check(RICON_Num.IsFinite(number), "allFinite");
		foreach (RICON_LossLayer finiteLayer : actual.m_aLayers)
			Check(finiteLayer != null && RICON_Num.IsFinite(finiteLayer.m_fLossDb), "allFinite слоёв");
		if (!m_bQuiet)
			PrintFormat("[RICON_L0] %1 valid=%2 reason=%3 state=%4 inDomain=%5", m_sId, actual.m_bValid,
				typename.EnumToString(RICON_EReason, actual.m_eReason),
				typename.EnumToString(RICON_ELinkState, actual.m_eState), actual.m_bNoiseModelInDomain);
		if (!expected.m_bValid)
			return;
		CheckNumber("distanceM", actual.m_fDistanceM, expected.m_fDistanceM, DISTANCE_TOLERANCE_M);
		CheckNumber("totalLossDb", actual.m_fTotalLossDb, expected.m_fTotalLossDb, 0.01);
		CheckNumber("rxPowerDbm", actual.m_fRxPowerDbm, expected.m_fRxPowerDbm, 0.01);
		CheckNumber("thermalNoiseDbm", actual.m_fThermalNoiseDbm, expected.m_fThermalNoiseDbm, 0.01);
		CheckNumber("noiseDbm", actual.m_fNoiseDbm, expected.m_fNoiseDbm, 0.01);
		CheckNumber("noisePlusInterferenceDbm", actual.m_fNoisePlusInterferenceDbm, expected.m_fNoisePlusInterferenceDbm, 0.01);
		CheckNumber("sinrDb", actual.m_fSinrDb, expected.m_fSinrDb, 0.01);
		CheckNumber("sensitivityDbm", actual.m_fSensitivityDbm, expected.m_fSensitivityDbm, 0.01);
		Check(actual.m_bNoiseModelInDomain == expected.m_bNoiseModelInDomain, "noiseModelInDomain");
		Check(actual.m_aLayers.Count() == expected.m_aLayers.Count(), "число слоёв");
		if (actual.m_aLayers.Count() != expected.m_aLayers.Count())
			return;
		for (int layerIndex = 0; layerIndex < expected.m_aLayers.Count(); layerIndex++)
		{
			RICON_LossLayer layer = actual.m_aLayers[layerIndex];
			RICON_LossLayer anchor = expected.m_aLayers[layerIndex];
			Check(layer != null, "слой существует");
			if (!layer)
				return;
			Check(layer.m_sLayerId == anchor.m_sLayerId && layer.m_sModelId == RICON_FreeSpaceModel.MODEL_ID, "id / порядок слоёв");
			CheckNumber("layer_" + anchor.m_sLayerId, layer.m_fLossDb, anchor.m_fLossDb, 0.01);
		}
	}

	void CheckSystemNoise(float bandwidthHz, float noiseFigureDb, string environmentId, float frequencyMHz,
		float expectedFaDb, float expectedNoiseDbm, bool expectedDomain)
	{
		RICON_NoiseModel noise = Noise(environmentId);
		RICON_NoiseEstimate estimate = new RICON_NoiseEstimate();
		noise.Estimate(frequencyMHz, estimate);
		CheckNumber("faDb", 10.0 * Math.Log10(estimate.m_fFactorLinear), expectedFaDb, 0.01);
		Check(estimate.m_bInDomain == expectedDomain, "noiseModelInDomain");
		RICON_LinkQuery query = RICON_TestSupport.NewQuery(RICON_FreeSpaceModel.MODEL_ID, frequencyMHz);
		query.m_sBandId = "test_vhf";
		query.m_sNoiseEnvironmentId = environmentId;
		query.m_fBandwidthHz = bandwidthHz;
		query.m_fRxNoiseFigureDb = noiseFigureDb;
		RICON_FreeSpaceModel propagation = new RICON_FreeSpaceModel();
		RICON_LinearTestIntelligibility quality = RICON_TestSupport.NewTestIntelligibility();
		RICON_WirelessLinkEvaluator evaluator = new RICON_WirelessLinkEvaluator("test_vhf", propagation, noise, quality);
		RICON_LinkBudget actual = evaluator.Evaluate(query);
		Check(actual != null && actual.m_bValid, "бюджет system_noise");
		if (actual)
		{
			CheckNumber("noiseDbm", actual.m_fNoiseDbm, expectedNoiseDbm, 0.01);
			Check(actual.m_bNoiseModelInDomain == expectedDomain, "область шума в бюджете");
		}
		if (!m_bQuiet)
			PrintFormat("[RICON_L0] %1 inDomain=%2", m_sId, estimate.m_bInDomain);
	}
}
