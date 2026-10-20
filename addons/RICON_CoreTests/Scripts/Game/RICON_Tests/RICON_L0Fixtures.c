//! GENERATED TEST_ONLY: gen_l0_fixture.py; ожидания скопированы, формулы не вычисляются.
//! Источник SHA-256: e7e1c7ef4026b03a7b7e666215ae01d8dcb34d94dbd716375dcd58b453e86a78
//! FSPL-01…04: effectiveDistanceM не экспортируется Core, сверяется только lossDb.
class RICON_L0Fixtures
{
	//! FSPL-01
	protected static void Case0(RICON_L0Run t)
	{
		t.CheckNumber("lossDb", RICON_FreeSpaceModel.LossDb(1000.0, 100.0), 72.447783, 0.01);
	}

	//! FSPL-02
	protected static void Case1(RICON_L0Run t)
	{
		t.CheckNumber("lossDb", RICON_FreeSpaceModel.LossDb(10000.0, 50.0), 86.427183, 0.01);
	}

	//! FSPL-03
	protected static void Case2(RICON_L0Run t)
	{
		t.CheckNumber("lossDb", RICON_FreeSpaceModel.LossDb(0.0, 100.0), 21.984197, 0.01);
	}

	//! FSPL-04
	protected static void Case3(RICON_L0Run t)
	{
		t.CheckNumber("lossDb", RICON_FreeSpaceModel.LossDb(2.99792458, 100.0), 21.984197, 0.01);
	}

	//! NTH-01
	protected static void Case4(RICON_L0Run t)
	{
		t.CheckNumber("thermalNoiseDbm", RICON_WirelessLinkEvaluator.ThermalNoiseDbm(12500.0), -133.006087, 0.01);
	}

	//! NTH-02
	protected static void Case5(RICON_L0Run t)
	{
		t.CheckNumber("thermalNoiseDbm", RICON_WirelessLinkEvaluator.ThermalNoiseDbm(25000.0), -129.995787, 0.01);
	}

	//! NTH-03
	protected static void Case6(RICON_L0Run t)
	{
		t.CheckNumber("thermalNoiseDbm", RICON_WirelessLinkEvaluator.ThermalNoiseDbm(1.0), -173.975187, 0.01);
	}

	//! NSYS-01
	protected static void Case7(RICON_L0Run t)
	{
		t.CheckSystemNoise(12500.0, 8.0, "none", 150.0, 0.0, -125.006087, true);
	}

	//! NSYS-02
	protected static void Case8(RICON_L0Run t)
	{
		t.CheckSystemNoise(12500.0, 8.0, "rural", 30.0, 26.283741, -106.668423, true);
	}

	//! NSYS-03
	protected static void Case9(RICON_L0Run t)
	{
		t.CheckSystemNoise(12500.0, 8.0, "rural", 50.0, 20.138531, -112.649757, true);
	}

	//! NSYS-04
	protected static void Case10(RICON_L0Run t)
	{
		t.CheckSystemNoise(12500.0, 8.0, "rural", 150.0, 6.922272, -122.906251, true);
	}

	//! NSYS-05
	protected static void Case11(RICON_L0Run t)
	{
		t.CheckSystemNoise(12500.0, 8.0, "rural", 250.0, 0.777062, -124.873277, true);
	}

	//! NSYS-06
	protected static void Case12(RICON_L0Run t)
	{
		t.CheckSystemNoise(12500.0, 8.0, "rural", 450.0, 0.0, -125.006087, false);
	}

	//! NSYS-07
	protected static void Case13(RICON_L0Run t)
	{
		t.CheckSystemNoise(12500.0, 8.0, "city", 50.0, 29.738531, -103.243135, true);
	}

	//! NSYS-08
	protected static void Case14(RICON_L0Run t)
	{
		t.CheckSystemNoise(12500.0, 8.0, "rural", 0.3, 81.683741, -51.322346, true);
	}

	//! PSUM-01
	protected static void Case15(RICON_L0Run t)
	{
		array<float> powers = {-120.0, -120.0};
		array<float> before = {-120.0, -120.0};
		float actual;
		t.Check(RICON_Num.TryPowerSumDbm(powers, actual), "сумма определена");
		t.CheckNumber("sumDbm", actual, -116.9897, 0.01);
		t.Check(powers.Count() == before.Count(), "размер входа не изменён");
		for (int i = 0; i < before.Count() && i < powers.Count(); i++)
			t.Check(powers[i] == before[i], "мощности не изменены");
	}

	//! PSUM-02
	protected static void Case16(RICON_L0Run t)
	{
		array<float> powers = {-125.006, -45.0};
		array<float> before = {-125.006, -45.0};
		float actual;
		t.Check(RICON_Num.TryPowerSumDbm(powers, actual), "сумма определена");
		t.CheckNumber("sumDbm", actual, -45.0, 0.01);
		t.Check(powers.Count() == before.Count(), "размер входа не изменён");
		for (int i = 0; i < before.Count() && i < powers.Count(); i++)
			t.Check(powers[i] == before[i], "мощности не изменены");
	}

	//! PSUM-03
	protected static void Case17(RICON_L0Run t)
	{
		array<float> powers = {-400.0, -400.0};
		array<float> before = {-400.0, -400.0};
		float actual;
		t.Check(RICON_Num.TryPowerSumDbm(powers, actual), "сумма определена");
		t.CheckNumber("sumDbm", actual, -396.9897, 0.01);
		t.Check(powers.Count() == before.Count(), "размер входа не изменён");
		for (int i = 0; i < before.Count() && i < powers.Count(); i++)
			t.Check(powers[i] == before[i], "мощности не изменены");
	}

	//! PSUM-04
	protected static void Case18(RICON_L0Run t)
	{
		array<float> powers = {400.0, 400.0};
		array<float> before = {400.0, 400.0};
		float actual;
		t.Check(RICON_Num.TryPowerSumDbm(powers, actual), "сумма определена");
		t.CheckNumber("sumDbm", actual, 403.0103, 0.01);
		t.Check(powers.Count() == before.Count(), "размер входа не изменён");
		for (int i = 0; i < before.Count() && i < powers.Count(); i++)
			t.Check(powers[i] == before[i], "мощности не изменены");
	}

	//! PSUM-05
	protected static void Case19(RICON_L0Run t)
	{
		array<float> powers = {400.0, -400.0};
		array<float> before = {400.0, -400.0};
		float actual;
		t.Check(RICON_Num.TryPowerSumDbm(powers, actual), "сумма определена");
		t.CheckNumber("sumDbm", actual, 400.0, 0.01);
		t.Check(powers.Count() == before.Count(), "размер входа не изменён");
		for (int i = 0; i < before.Count() && i < powers.Count(); i++)
			t.Check(powers[i] == before[i], "мощности не изменены");
	}

	//! INTL-01
	protected static void Case20(RICON_L0Run t)
	{
		RICON_LinearTestIntelligibility quality = new RICON_LinearTestIntelligibility(-15.0, 15.0, 0.8, 0.5);
		t.Check(quality.IsConfigValid(), "TEST_ONLY конфиг");
		t.CheckNumber("intelligibility", quality.Intelligibility(-15.0), 0.0, 0.000001);
	}

	//! INTL-02
	protected static void Case21(RICON_L0Run t)
	{
		RICON_LinearTestIntelligibility quality = new RICON_LinearTestIntelligibility(-15.0, 15.0, 0.8, 0.5);
		t.Check(quality.IsConfigValid(), "TEST_ONLY конфиг");
		t.CheckNumber("intelligibility", quality.Intelligibility(0.0), 0.5, 0.000001);
	}

	//! INTL-03
	protected static void Case22(RICON_L0Run t)
	{
		RICON_LinearTestIntelligibility quality = new RICON_LinearTestIntelligibility(-15.0, 15.0, 0.8, 0.5);
		t.Check(quality.IsConfigValid(), "TEST_ONLY конфиг");
		t.CheckNumber("intelligibility", quality.Intelligibility(9.0), 0.8, 0.000001);
	}

	//! INTL-04
	protected static void Case23(RICON_L0Run t)
	{
		RICON_LinearTestIntelligibility quality = new RICON_LinearTestIntelligibility(-15.0, 15.0, 0.8, 0.5);
		t.Check(quality.IsConfigValid(), "TEST_ONLY конфиг");
		t.CheckNumber("intelligibility", quality.Intelligibility(15.0), 1.0, 0.000001);
	}

	//! INTL-05
	protected static void Case24(RICON_L0Run t)
	{
		RICON_LinearTestIntelligibility quality = new RICON_LinearTestIntelligibility(-15.0, 15.0, 0.8, 0.5);
		t.Check(quality.IsConfigValid(), "TEST_ONLY конфиг");
		t.CheckNumber("intelligibility", quality.Intelligibility(400.0), 1.0, 0.000001);
	}

	//! INTL-06
	protected static void Case25(RICON_L0Run t)
	{
		RICON_LinearTestIntelligibility quality = new RICON_LinearTestIntelligibility(-15.0, 15.0, 0.8, 0.5);
		t.Check(quality.IsConfigValid(), "TEST_ONLY конфиг");
		t.CheckNumber("intelligibility", quality.Intelligibility(-400.0), 0.0, 0.000001);
	}

	//! INTL-07
	protected static void Case26(RICON_L0Run t)
	{
		RICON_LinearTestIntelligibility quality = new RICON_LinearTestIntelligibility(-15.0, 15.0, 0.8, 0.5);
		t.Check(quality.IsConfigValid(), "TEST_ONLY конфиг");
		t.CheckNumber("intelligibility", quality.Intelligibility(-20.0), 0.0, 0.000001);
	}

	protected static RICON_LinkQuery Query27()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! BUD-01
	protected static void Case27(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query27();
		RICON_LinkQuery before = Query27();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 1000.0;
		expected.m_fTotalLossDb = 75.969608;
		expected.m_fRxPowerDbm = -38.969608;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -122.906251;
		expected.m_fNoisePlusInterferenceDbm = -122.906251;
		expected.m_fSinrDb = 83.936643;
		expected.m_fSensitivityDbm = -115.006087;
		expected.m_bNoiseModelInDomain = true;
		expected.m_fIntelligibility = 1.0;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 75.969608, RICON_FreeSpaceModel.MODEL_ID));
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query28()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = true;
		q.m_fInterferenceDbm = -45.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! BUD-02
	protected static void Case28(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query28();
		RICON_LinkQuery before = Query28();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 1000.0;
		expected.m_fTotalLossDb = 75.969608;
		expected.m_fRxPowerDbm = -38.969608;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -122.906251;
		expected.m_fNoisePlusInterferenceDbm = -45.0;
		expected.m_fSinrDb = 6.030392;
		expected.m_fSensitivityDbm = -115.006087;
		expected.m_bNoiseModelInDomain = true;
		expected.m_fIntelligibility = 0.701013;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.DEGRADED;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 75.969608, RICON_FreeSpaceModel.MODEL_ID));
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query29()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = true;
		q.m_fInterferenceDbm = -35.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! BUD-03
	protected static void Case29(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query29();
		RICON_LinkQuery before = Query29();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 1000.0;
		expected.m_fTotalLossDb = 75.969608;
		expected.m_fRxPowerDbm = -38.969608;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -122.906251;
		expected.m_fNoisePlusInterferenceDbm = -35.0;
		expected.m_fSinrDb = -3.969608;
		expected.m_fSensitivityDbm = -115.006087;
		expected.m_bNoiseModelInDomain = true;
		expected.m_fIntelligibility = 0.36768;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.WEAK;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 75.969608, RICON_FreeSpaceModel.MODEL_ID));
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query30()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(100000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 0.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! BUD-04
	protected static void Case30(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query30();
		RICON_LinkQuery before = Query30();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 100000.0;
		expected.m_fTotalLossDb = 115.969608;
		expected.m_fRxPowerDbm = -115.969608;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -122.906251;
		expected.m_fNoisePlusInterferenceDbm = -122.906251;
		expected.m_fSinrDb = 6.936643;
		expected.m_fSensitivityDbm = -115.006087;
		expected.m_bNoiseModelInDomain = true;
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.LOST;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 115.969608, RICON_FreeSpaceModel.MODEL_ID));
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query31()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(100000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 0.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = true;
		q.m_fRxSensitivityDbm = -119.0;
		q.m_bHasRequiredSinr = false;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! BUD-05
	protected static void Case31(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query31();
		RICON_LinkQuery before = Query31();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 100000.0;
		expected.m_fTotalLossDb = 115.969608;
		expected.m_fRxPowerDbm = -115.969608;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -122.906251;
		expected.m_fNoisePlusInterferenceDbm = -122.906251;
		expected.m_fSinrDb = 6.936643;
		expected.m_fSensitivityDbm = -119.0;
		expected.m_bNoiseModelInDomain = true;
		expected.m_fIntelligibility = 0.731221;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.DEGRADED;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 115.969608, RICON_FreeSpaceModel.MODEL_ID));
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query32()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 450.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! BUD-06
	protected static void Case32(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query32();
		RICON_LinkQuery before = Query32();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 1000.0;
		expected.m_fTotalLossDb = 85.512033;
		expected.m_fRxPowerDbm = -48.512033;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -125.006087;
		expected.m_fNoisePlusInterferenceDbm = -125.006087;
		expected.m_fSinrDb = 76.494054;
		expected.m_fSensitivityDbm = -115.006087;
		expected.m_bNoiseModelInDomain = false;
		expected.m_fIntelligibility = 1.0;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 85.512033, RICON_FreeSpaceModel.MODEL_ID));
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query33()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(0.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! BUD-07
	protected static void Case33(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query33();
		RICON_LinkQuery before = Query33();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 0.0;
		expected.m_fTotalLossDb = 21.984197;
		expected.m_fRxPowerDbm = 15.015803;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -122.906251;
		expected.m_fNoisePlusInterferenceDbm = -122.906251;
		expected.m_fSinrDb = 137.922054;
		expected.m_fSensitivityDbm = -115.006087;
		expected.m_bNoiseModelInDomain = true;
		expected.m_fIntelligibility = 1.0;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 21.984197, RICON_FreeSpaceModel.MODEL_ID));
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query34()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(3000.0, 4000.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 3.0;
		q.m_fRxGainDbi = -2.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! BUD-08
	protected static void Case34(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query34();
		RICON_LinkQuery before = Query34();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 5000.0;
		expected.m_fTotalLossDb = 89.949008;
		expected.m_fRxPowerDbm = -51.949008;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -122.906251;
		expected.m_fNoisePlusInterferenceDbm = -122.906251;
		expected.m_fSinrDb = 70.957242;
		expected.m_fSensitivityDbm = -115.006087;
		expected.m_bNoiseModelInDomain = true;
		expected.m_fIntelligibility = 1.0;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 89.949008, RICON_FreeSpaceModel.MODEL_ID));
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query35()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(RICON_L0Run.Special(0), 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-01
	protected static void Case35(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query35();
		RICON_LinkQuery before = Query35();
		t.RequireSpecial(q.m_vTxPos[0], 0);
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.INVALID_COORDINATE;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query36()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(0.0, RICON_L0Run.Special(1), 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-02
	protected static void Case36(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query36();
		RICON_LinkQuery before = Query36();
		t.RequireSpecial(q.m_vRxPos[1], 1);
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.INVALID_COORDINATE;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query37()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 0.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-03
	protected static void Case37(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query37();
		RICON_LinkQuery before = Query37();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.INVALID_FREQUENCY;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query38()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = -1.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-04
	protected static void Case38(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query38();
		RICON_LinkQuery before = Query38();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.INVALID_FREQUENCY;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query39()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = -1.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-05
	protected static void Case39(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query39();
		RICON_LinkQuery before = Query39();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.INVALID_BANDWIDTH;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query40()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 0.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-06
	protected static void Case40(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query40();
		RICON_LinkQuery before = Query40();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.INVALID_BANDWIDTH;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query41()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = -1.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-07
	protected static void Case41(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query41();
		RICON_LinkQuery before = Query41();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.INVALID_NOISE_FIGURE;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query42()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = true;
		q.m_fRxSensitivityDbm = -119.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-08
	protected static void Case42(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query42();
		RICON_LinkQuery before = Query42();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.CATALOG_VIOLATION;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query43()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = false;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-09
	protected static void Case43(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query43();
		RICON_LinkQuery before = Query43();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.CATALOG_VIOLATION;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query44()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = RICON_L0Run.Special(0);
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-10
	protected static void Case44(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query44();
		RICON_LinkQuery before = Query44();
		t.RequireSpecial(q.m_fTxPowerDbm, 0);
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.CATALOG_VIOLATION;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query45()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "swamp";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-11
	protected static void Case45(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query45();
		RICON_LinkQuery before = Query45();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.UNKNOWN_NOISE_ENVIRONMENT;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query46()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = RICON_L0Run.Special(0);
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-12
	protected static void Case46(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query46();
		RICON_LinkQuery before = Query46();
		t.RequireSpecial(q.m_fTxGainDbi, 0);
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.INVALID_GAIN;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query47()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = RICON_L0Run.Special(1);
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-13
	protected static void Case47(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query47();
		RICON_LinkQuery before = Query47();
		t.RequireSpecial(q.m_fRxGainDbi, 1);
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.INVALID_GAIN;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query48()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "speech_v0";
		return q;
	}

	//! INV-14
	protected static void Case48(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query48();
		RICON_LinkQuery before = Query48();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.UNKNOWN_MODEL;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query49()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "itm";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-15
	protected static void Case49(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query49();
		RICON_LinkQuery before = Query49();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.UNKNOWN_MODEL;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query50()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-16
	protected static void Case50(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query50();
		RICON_LinkQuery before = Query50();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.UNKNOWN_BAND;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query51()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 0.0;
		q.m_fBandwidthHz = -1.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-17
	protected static void Case51(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query51();
		RICON_LinkQuery before = Query51();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.INVALID_FREQUENCY;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query52()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = RICON_L0Run.Special(0);
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = -1.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-18
	protected static void Case52(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query52();
		RICON_LinkQuery before = Query52();
		t.RequireSpecial(q.m_fTxPowerDbm, 0);
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.CATALOG_VIOLATION;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query53()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = RICON_L0Run.Special(0);
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = -1.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-19
	protected static void Case53(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query53();
		RICON_LinkQuery before = Query53();
		t.RequireSpecial(q.m_fTxGainDbi, 0);
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.INVALID_GAIN;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query54()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = -1.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = false;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "rural";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-20
	protected static void Case54(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query54();
		RICON_LinkQuery before = Query54();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.INVALID_NOISE_FIGURE;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query55()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "swamp";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "itm";
		q.m_sIntelligibilityModelId = "linear_test";
		return q;
	}

	//! INV-21
	protected static void Case55(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query55();
		RICON_LinkQuery before = Query55();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.UNKNOWN_MODEL;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	protected static RICON_LinkQuery Query56()
	{
		RICON_LinkQuery q = new RICON_LinkQuery();
		q.m_vTxPos = Vector(0.0, 0.0, 0.0);
		q.m_vRxPos = Vector(1000.0, 0.0, 0.0);
		q.m_fFrequencyMHz = 150.0;
		q.m_fBandwidthHz = 12500.0;
		q.m_fTxPowerDbm = 37.0;
		q.m_fTxGainDbi = 0.0;
		q.m_fRxGainDbi = 0.0;
		q.m_fRxNoiseFigureDb = 8.0;
		q.m_bHasRxSensitivity = false;
		q.m_fRxSensitivityDbm = 0.0;
		q.m_bHasRequiredSinr = true;
		q.m_fRequiredSinrDb = 10.0;
		q.m_bHasInterference = false;
		q.m_fInterferenceDbm = 0.0;
		q.m_sNoiseEnvironmentId = "swamp";
		q.m_sBandId = "test_vhf";
		q.m_sModelId = "free_space";
		q.m_sIntelligibilityModelId = "speech_v0";
		return q;
	}

	//! INV-22
	protected static void Case56(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query56();
		RICON_LinkQuery before = Query56();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.UNKNOWN_NOISE_ENVIRONMENT;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	//! X_loss
	protected static void Case57(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query27();
		RICON_LinkQuery before = Query27();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 1000.0;
		expected.m_fTotalLossDb = 75.989608;
		expected.m_fRxPowerDbm = -38.969608;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -122.906251;
		expected.m_fNoisePlusInterferenceDbm = -122.906251;
		expected.m_fSinrDb = 83.936643;
		expected.m_fSensitivityDbm = -115.006087;
		expected.m_bNoiseModelInDomain = true;
		expected.m_fIntelligibility = 1.0;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 75.969608, RICON_FreeSpaceModel.MODEL_ID));
		t.CheckBudget(actual, expected);
	}

	//! X_distance
	protected static void Case58(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query27();
		RICON_LinkQuery before = Query27();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 1000.002;
		expected.m_fTotalLossDb = 75.969608;
		expected.m_fRxPowerDbm = -38.969608;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -122.906251;
		expected.m_fNoisePlusInterferenceDbm = -122.906251;
		expected.m_fSinrDb = 83.936643;
		expected.m_fSensitivityDbm = -115.006087;
		expected.m_bNoiseModelInDomain = true;
		expected.m_fIntelligibility = 1.0;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 75.969608, RICON_FreeSpaceModel.MODEL_ID));
		t.CheckBudget(actual, expected);
	}

	//! X_state
	protected static void Case59(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query28();
		RICON_LinkQuery before = Query28();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 1000.0;
		expected.m_fTotalLossDb = 75.969608;
		expected.m_fRxPowerDbm = -38.969608;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -122.906251;
		expected.m_fNoisePlusInterferenceDbm = -45.0;
		expected.m_fSinrDb = 6.030392;
		expected.m_fSensitivityDbm = -115.006087;
		expected.m_bNoiseModelInDomain = true;
		expected.m_fIntelligibility = 0.701013;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 75.969608, RICON_FreeSpaceModel.MODEL_ID));
		t.CheckBudget(actual, expected);
	}

	//! X_domain
	protected static void Case60(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query32();
		RICON_LinkQuery before = Query32();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 1000.0;
		expected.m_fTotalLossDb = 85.512033;
		expected.m_fRxPowerDbm = -48.512033;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -125.006087;
		expected.m_fNoisePlusInterferenceDbm = -125.006087;
		expected.m_fSinrDb = 76.494054;
		expected.m_fSensitivityDbm = -115.006087;
		expected.m_bNoiseModelInDomain = true;
		expected.m_fIntelligibility = 1.0;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 85.512033, RICON_FreeSpaceModel.MODEL_ID));
		t.CheckBudget(actual, expected);
	}

	//! X_layer_id
	protected static void Case61(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query27();
		RICON_LinkQuery before = Query27();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 1000.0;
		expected.m_fTotalLossDb = 75.969608;
		expected.m_fRxPowerDbm = -38.969608;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -122.906251;
		expected.m_fNoisePlusInterferenceDbm = -122.906251;
		expected.m_fSinrDb = 83.936643;
		expected.m_fSensitivityDbm = -115.006087;
		expected.m_bNoiseModelInDomain = true;
		expected.m_fIntelligibility = 1.0;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("wrong_layer", 75.969608, RICON_FreeSpaceModel.MODEL_ID));
		t.CheckBudget(actual, expected);
	}

	//! X_layer_count
	protected static void Case62(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query27();
		RICON_LinkQuery before = Query27();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 1000.0;
		expected.m_fTotalLossDb = 75.969608;
		expected.m_fRxPowerDbm = -38.969608;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -122.906251;
		expected.m_fNoisePlusInterferenceDbm = -122.906251;
		expected.m_fSinrDb = 83.936643;
		expected.m_fSensitivityDbm = -115.006087;
		expected.m_bNoiseModelInDomain = true;
		expected.m_fIntelligibility = 1.0;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.CLEAN;
		t.CheckBudget(actual, expected);
	}

	//! X_reason
	protected static void Case63(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query35();
		RICON_LinkQuery before = Query35();
		t.RequireSpecial(q.m_vTxPos[0], 0);
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fIntelligibility = 0.0;
		expected.m_bValid = false;
		expected.m_eReason = RICON_EReason.INVALID_GAIN;
		expected.m_eState = RICON_ELinkState.LOST;
		t.CheckBudget(actual, expected);
	}

	//! X_nonfinite_expected
	protected static void Case64(RICON_L0Run t)
	{
		RICON_LinkQuery q = Query27();
		RICON_LinkQuery before = Query27();
		RICON_LinkBudget actual = t.Evaluate(q, before);
		RICON_LinkBudget expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 1000.0;
		expected.m_fTotalLossDb = RICON_L0Run.Special(0);
		expected.m_fRxPowerDbm = -38.969608;
		expected.m_fThermalNoiseDbm = -133.006087;
		expected.m_fNoiseDbm = -122.906251;
		expected.m_fNoisePlusInterferenceDbm = -122.906251;
		expected.m_fSinrDb = 83.936643;
		expected.m_fSensitivityDbm = -115.006087;
		expected.m_bNoiseModelInDomain = true;
		expected.m_fIntelligibility = 1.0;
		expected.m_bValid = true;
		expected.m_eReason = RICON_EReason.OK;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 75.969608, RICON_FreeSpaceModel.MODEL_ID));
		t.CheckBudget(actual, expected);
	}

	static bool RunAll()
	{
		RICON_L0Run.s_fMaxDeviationDb = 0;
		Print("[RICON_L0] source=e7e1c7ef4026b03a7b7e666215ae01d8dcb34d94dbd716375dcd58b453e86a78");
		bool ok = true;
		int cases = 0;
		int quietPositive = 0;
		int rejected = 0;
		RICON_L0Run t;
		t = new RICON_L0Run("FSPL-01");
		Case0(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("FSPL-01", true);
		Case0(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("FSPL-02");
		Case1(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("FSPL-02", true);
		Case1(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("FSPL-03");
		Case2(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("FSPL-03", true);
		Case2(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("FSPL-04");
		Case3(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("FSPL-04", true);
		Case3(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("NTH-01");
		Case4(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("NTH-01", true);
		Case4(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("NTH-02");
		Case5(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("NTH-02", true);
		Case5(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("NTH-03");
		Case6(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("NTH-03", true);
		Case6(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-01");
		Case7(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-01", true);
		Case7(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-02");
		Case8(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-02", true);
		Case8(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-03");
		Case9(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-03", true);
		Case9(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-04");
		Case10(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-04", true);
		Case10(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-05");
		Case11(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-05", true);
		Case11(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-06");
		Case12(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-06", true);
		Case12(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-07");
		Case13(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-07", true);
		Case13(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-08");
		Case14(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("NSYS-08", true);
		Case14(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("PSUM-01");
		Case15(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("PSUM-01", true);
		Case15(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("PSUM-02");
		Case16(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("PSUM-02", true);
		Case16(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("PSUM-03");
		Case17(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("PSUM-03", true);
		Case17(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("PSUM-04");
		Case18(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("PSUM-04", true);
		Case18(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("PSUM-05");
		Case19(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("PSUM-05", true);
		Case19(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INTL-01");
		Case20(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INTL-01", true);
		Case20(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INTL-02");
		Case21(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INTL-02", true);
		Case21(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INTL-03");
		Case22(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INTL-03", true);
		Case22(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INTL-04");
		Case23(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INTL-04", true);
		Case23(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INTL-05");
		Case24(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INTL-05", true);
		Case24(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INTL-06");
		Case25(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INTL-06", true);
		Case25(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INTL-07");
		Case26(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INTL-07", true);
		Case26(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-01");
		Case27(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-01", true);
		Case27(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-02");
		Case28(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-02", true);
		Case28(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-03");
		Case29(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-03", true);
		Case29(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-04");
		Case30(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-04", true);
		Case30(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-05");
		Case31(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-05", true);
		Case31(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-06");
		Case32(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-06", true);
		Case32(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-07");
		Case33(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-07", true);
		Case33(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-08");
		Case34(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("BUD-08", true);
		Case34(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-01");
		Case35(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-01", true);
		Case35(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-02");
		Case36(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-02", true);
		Case36(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-03");
		Case37(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-03", true);
		Case37(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-04");
		Case38(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-04", true);
		Case38(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-05");
		Case39(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-05", true);
		Case39(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-06");
		Case40(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-06", true);
		Case40(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-07");
		Case41(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-07", true);
		Case41(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-08");
		Case42(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-08", true);
		Case42(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-09");
		Case43(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-09", true);
		Case43(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-10");
		Case44(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-10", true);
		Case44(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-11");
		Case45(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-11", true);
		Case45(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-12");
		Case46(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-12", true);
		Case46(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-13");
		Case47(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-13", true);
		Case47(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-14");
		Case48(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-14", true);
		Case48(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-15");
		Case49(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-15", true);
		Case49(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-16");
		Case50(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-16", true);
		Case50(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-17");
		Case51(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-17", true);
		Case51(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-18");
		Case52(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-18", true);
		Case52(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-19");
		Case53(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-19", true);
		Case53(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-20");
		Case54(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-20", true);
		Case54(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-21");
		Case55(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-21", true);
		Case55(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("INV-22");
		Case56(t);
		if (t.IsPassed())
			cases++;
		else
			ok = false;
		t = new RICON_L0Run("INV-22", true);
		Case56(t);
		if (t.IsPassed())
			quietPositive++;
		else
			ok = false;
		t = new RICON_L0Run("X_loss", true);
		Case57(t);
		if (t.IsFailed())
			rejected++;
		else
			ok = false;
		t = new RICON_L0Run("X_distance", true);
		Case58(t);
		if (t.IsFailed())
			rejected++;
		else
			ok = false;
		t = new RICON_L0Run("X_state", true);
		Case59(t);
		if (t.IsFailed())
			rejected++;
		else
			ok = false;
		t = new RICON_L0Run("X_domain", true);
		Case60(t);
		if (t.IsFailed())
			rejected++;
		else
			ok = false;
		t = new RICON_L0Run("X_layer_id", true);
		Case61(t);
		if (t.IsFailed())
			rejected++;
		else
			ok = false;
		t = new RICON_L0Run("X_layer_count", true);
		Case62(t);
		if (t.IsFailed())
			rejected++;
		else
			ok = false;
		t = new RICON_L0Run("X_reason", true);
		Case63(t);
		if (t.IsFailed())
			rejected++;
		else
			ok = false;
		t = new RICON_L0Run("X_nonfinite_expected", true);
		Case64(t);
		if (t.IsFailed())
			rejected++;
		else
			ok = false;
		t = new RICON_L0Run("X_nonfinite_actual", true);
		t.CheckNumber("finite", RICON_L0Run.Special(0), 0, 0.01);
		ok = t.IsFailed() && ok;
		PrintFormat("[RICON_L0] cases=%1 quietPositive=%2 controls=%3 unobservableFields=4 maxDeviationDb=%4",
			cases, quietPositive, rejected, RICON_L0Run.s_fMaxDeviationDb.ToString(-1, 9));
		return ok && cases == 57 && quietPositive == 57 && rejected == 8;
	}
}
