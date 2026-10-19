// TEST_ONLY support for the generated L2 terrain parity fixture (RICON_L2Fixtures.c). Not for Workshop.
// One RICON_L2Run checks one vector: the result code exactly, losses within the vectors' Enforce tolerance, branches,
// spherical case and first-term accuracy exactly. CheckSampling() replays REQ-L2 §6 sampling cases written by hand.

class RICON_L2Run
{
	//! Largest |Enforce - oracle| over all compared losses of non-control cases, for the evidence log.
	static float s_fMaxDeviationDb;

	protected string m_sCaseId;
	protected bool m_bQuiet;
	protected bool m_bChecked;
	protected bool m_bFailed;

	void RICON_L2Run(string caseId, bool quiet = false)
	{
		m_sCaseId = caseId;
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

	void CheckCode(RICON_L2Result result, RICON_EL2ResultCode code)
	{
		m_bChecked = true;
		if (!result)
			Fail("null result");
		else if (result.m_eCode != code)
			Fail("code expected " + CodeName(code) + " actual " + CodeName(result.m_eCode));
	}

	void CheckOk(RICON_L2Result result, float loss, float bullingtonActual, float bullingtonSmooth, float spherical,
		RICON_EL2Branch branchActual, RICON_EL2Branch branchSmooth, RICON_EL2SphericalCase sphCase,
		RICON_EFirstTermAccuracy accuracy, float toleranceDb)
	{
		CheckCode(result, RICON_EL2ResultCode.OK);
		if (m_bFailed)
			return;
		CheckNumber("L", result.m_fLossDb, loss, toleranceDb);
		CheckNumber("Lba", result.m_fBullingtonActualDb, bullingtonActual, toleranceDb);
		CheckNumber("Lbs", result.m_fBullingtonSmoothDb, bullingtonSmooth, toleranceDb);
		CheckNumber("Lsph", result.m_fSphericalDb, spherical, toleranceDb);
		if (result.m_eBranchActual != branchActual)
			Fail("branchActual " + typename.EnumToString(RICON_EL2Branch, result.m_eBranchActual));
		if (result.m_eBranchSmooth != branchSmooth)
			Fail("branchSmooth " + typename.EnumToString(RICON_EL2Branch, result.m_eBranchSmooth));
		if (result.m_eSphericalCase != sphCase)
			Fail("sphCase " + typename.EnumToString(RICON_EL2SphericalCase, result.m_eSphericalCase));
		if (result.m_eFirstTermAccuracy != accuracy)
			Fail("firstTermAccuracy " + typename.EnumToString(RICON_EFirstTermAccuracy, result.m_eFirstTermAccuracy));
	}

	//! A special float is built at run time and must really be non-finite, or the case fails instead of passing.
	void RequireNonFinite(float value)
	{
		if (RICON_Num.IsFinite(value))
			Fail("special float was not constructed");
	}

	protected void CheckNumber(string what, float actual, float expected, float toleranceDb)
	{
		if (!m_bQuiet && RICON_Num.IsFinite(actual))
			s_fMaxDeviationDb = Math.Max(s_fMaxDeviationDb, Math.AbsFloat(actual - expected));
		if (!RICON_Num.IsFinite(actual) || Math.AbsFloat(actual - expected) > toleranceDb)
			Fail(what + " expected " + expected.ToString() + " actual " + actual.ToString());
	}

	protected void Fail(string problem)
	{
		if (m_bFailed)
			return;
		m_bFailed = true;
		if (!m_bQuiet)
			PrintFormat("[RICON_TEST] FAIL %1: %2", m_sCaseId, problem);
	}

	protected static string CodeName(RICON_EL2ResultCode code)
	{
		return typename.EnumToString(RICON_EL2ResultCode, code);
	}

	static RICON_TerrainDiffractionConfig Config(float earthRadiusKm, float kFactor, float permittivity,
		float conductivitySm, RICON_ETerrainPolarization polarization, float maxProfileStepKm, int maxProfilePoints)
	{
		RICON_TerrainDiffractionConfig config = new RICON_TerrainDiffractionConfig();
		config.m_fEarthRadiusKm = earthRadiusKm;
		config.m_fKFactor = kFactor;
		config.m_fPermittivity = permittivity;
		config.m_fConductivitySm = conductivitySm;
		config.m_ePolarization = polarization;
		config.m_fMaxProfileStepKm = maxProfileStepKm;
		config.m_iMaxProfilePoints = maxProfilePoints;
		return config;
	}

	static float NaN()
	{
		return Math.Sqrt(-1.0);
	}

	//! REQ-L2 §6 sampling rule, hand-written cases (expected values derived from the rule, not from the oracle run).
	static bool CheckSampling()
	{
		bool ok = SamplingCase("0 m", 0.0, 0.1, 1000, RICON_ETerrainSamplingCode.SKIP, 0);
		ok = SamplingCase("1.9 m", 1.9, 0.1, 1000, RICON_ETerrainSamplingCode.SKIP, 0) && ok;
		ok = SamplingCase("2.1 m, step 2 m", 2.1, 0.002, 1000, RICON_ETerrainSamplingCode.OK, 3) && ok;
		ok = SamplingCase("1 km, step 100 m", 1000.0, 0.1, 1000, RICON_ETerrainSamplingCode.OK, 11) && ok;
		ok = SamplingCase("1.05 km, step 100 m", 1050.0, 0.1, 1000, RICON_ETerrainSamplingCode.OK, 12) && ok;
		ok = SamplingCase("500 km, step 2 m, cap 1000", 500000.0, 0.002, 1000, RICON_ETerrainSamplingCode.UNAVAILABLE, 0) && ok;
		ok = SamplingCase("500.001 km", 500001.0, 10.0, 100000, RICON_ETerrainSamplingCode.UNAVAILABLE, 0) && ok;
		return ok;
	}

	protected static bool SamplingCase(string what, float horizontalDistanceM, float stepKm, int maxPoints,
		RICON_ETerrainSamplingCode expectedCode, int expectedPoints)
	{
		RICON_TerrainDiffractionConfig config = Config(6371.0, 1.3333334, 15.0, 0.001, RICON_ETerrainPolarization.VERTICAL,
			stepKm, maxPoints);
		RICON_TerrainSamplingResult result = RICON_TerrainProfileSampling.BuildDistances(horizontalDistanceM, config);
		bool ok = result && result.m_eCode == expectedCode && result.m_aDistanceKm.Count() == expectedPoints;
		if (ok && expectedPoints > 0)
		{
			int last = expectedPoints - 1;
			ok = result.m_aDistanceKm[0] == 0
				&& Math.AbsFloat(result.m_aDistanceKm[last] - horizontalDistanceM / 1000.0) <= 1e-6;
		}
		if (!ok)
			PrintFormat("[RICON_TEST] FAIL l2 sampling %1", what);
		return ok;
	}
}
