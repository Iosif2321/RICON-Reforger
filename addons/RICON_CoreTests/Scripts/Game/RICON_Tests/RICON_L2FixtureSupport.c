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

	//! Calls the calculator on the vector's inputs and fails the case if the call changed them (pure function).
	RICON_L2Result Evaluate(RICON_TerrainProfile profile, float htgM, float hrgM, float frequencyMHz,
		RICON_TerrainDiffractionConfig config)
	{
		RICON_TerrainProfile profileBefore;
		if (profile)
			profileBefore = profile.CloneProfile();
		RICON_TerrainDiffractionConfig configBefore;
		if (config)
			configBefore = config.CloneConfig();
		RICON_L2Result result = RICON_TerrainDiffractionCalculator.Evaluate(profile, htgM, hrgM, frequencyMHz, config);
		if (!SameProfile(profileBefore, profile))
			Fail("profile changed by the call");
		if (!SameConfig(configBefore, config))
			Fail("config changed by the call");
		return result;
	}

	protected static bool SameProfile(RICON_TerrainProfile a, RICON_TerrainProfile b)
	{
		if (!a || !b)
			return !a && !b;
		if (a.m_aDistanceKm.Count() != b.m_aDistanceKm.Count() || a.m_aHeightM.Count() != b.m_aHeightM.Count())
			return false;
		for (int i = 0; i < a.m_aDistanceKm.Count(); i++)
		{
			if (a.m_aDistanceKm[i] != b.m_aDistanceKm[i] || a.m_aHeightM[i] != b.m_aHeightM[i])
				return false;
		}
		return true;
	}

	protected static bool SameConfig(RICON_TerrainDiffractionConfig a, RICON_TerrainDiffractionConfig b)
	{
		if (!a || !b)
			return !a && !b;
		if (a.m_fEarthRadiusKm != b.m_fEarthRadiusKm || a.m_fKFactor != b.m_fKFactor)
			return false;
		if (a.m_fPermittivity != b.m_fPermittivity || a.m_fConductivitySm != b.m_fConductivitySm)
			return false;
		if (a.m_ePolarization != b.m_ePolarization || a.m_fMaxProfileStepKm != b.m_fMaxProfileStepKm)
			return false;
		return a.m_iMaxProfilePoints == b.m_iMaxProfilePoints;
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

	//! REQ-L2 §6 adapter, hand-written: two layers with MODEL_ID, reason 16 through the evaluator, empty layers on
	//! failure, the < 2 m bypass without the provider, config and domain mapping (codex #1374).
	static bool CheckAdapter()
	{
		RICON_TerrainDiffractionConfig config = Config(6371.0, 1.3333334, 15.0, 0.001,
			RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_TerrainProfile flat = new RICON_TerrainProfile();
		for (int i = 0; i <= 10; i++)
			flat.AddPoint(0.1 * i, 100.0);
		RICON_TerrainProfile coarse = new RICON_TerrainProfile();
		coarse.AddPoint(0.0, 100.0);
		coarse.AddPoint(0.5, 100.0);
		coarse.AddPoint(1.0, 100.0);

		bool ok = Adapter("enum value", RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE == 16);

		// 0. the purity detector of Evaluate() is not vacuous: a changed point or config field is noticed.
		RICON_TerrainProfile changed = flat.CloneProfile();
		changed.m_aHeightM[5] = 101.0;
		ok = Adapter("purity detector: profile", SameProfile(flat, flat.CloneProfile()) && !SameProfile(flat, changed)) && ok;
		RICON_TerrainDiffractionConfig changedConfig = config.CloneConfig();
		changedConfig.m_iMaxProfilePoints = 999;
		ok = Adapter("purity detector: config", SameConfig(config, config.CloneConfig())
			&& !SameConfig(config, changedConfig)) && ok;

		// 1. valid model: OK, exactly two layers in order, both with MODEL_ID; values equal FSPL and the calculator.
		RICON_TerrainDiffractionModel model = new RICON_TerrainDiffractionModel(config,
			new RICON_SyntheticTerrainProfileProvider(flat, 10.0, 10.0));
		RICON_LinkQuery query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		array<ref RICON_LossLayer> layers = {};
		ok = Adapter("valid: OK", model.AppendLosses(query, 1000.0, layers) == RICON_EReason.OK) && ok;
		ok = Adapter("valid: two layers", layers.Count() == 2) && ok;
		if (layers.Count() == 2)
		{
			RICON_L2Result direct = RICON_TerrainDiffractionCalculator.Evaluate(flat, 10.0, 10.0, 50.0, config);
			ok = Adapter("valid: fspl layer", layers[0].m_sLayerId == RICON_TerrainDiffractionModel.FSPL_LAYER_ID
				&& layers[0].m_sModelId == RICON_TerrainDiffractionModel.MODEL_ID
				&& layers[0].m_fLossDb == RICON_FreeSpaceModel.LossDb(1000.0, 50.0)) && ok;
			ok = Adapter("valid: terrain layer", layers[1].m_sLayerId == RICON_TerrainDiffractionModel.TERRAIN_LAYER_ID
				&& layers[1].m_sModelId == RICON_TerrainDiffractionModel.MODEL_ID
				&& layers[1].m_fLossDb == direct.m_fLossDb) && ok;
		}
		// Evaluators live in locals: a temporary could be released during the call (sprint 17 finding).
		RICON_WirelessLinkEvaluator evaluator = NewEvaluator(model);
		RICON_LinkBudget budget = evaluator.Evaluate(query);
		ok = Adapter("valid: evaluator budget", budget.m_bValid && budget.m_eReason == RICON_EReason.OK
			&& budget.FindLayer(RICON_TerrainDiffractionModel.TERRAIN_LAYER_ID) != null) && ok;

		// 2. provider failure: reason 16, no layer, and the evaluator reports the same reason.
		RICON_TerrainDiffractionModel failing = new RICON_TerrainDiffractionModel(config,
			new RICON_SyntheticTerrainProfileProvider(flat, 10.0, 10.0, RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE));
		layers = {};
		ok = Adapter("provider failure: reason 16", failing.AppendLosses(query, 1000.0, layers)
			== RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE && layers.IsEmpty()) && ok;
		RICON_WirelessLinkEvaluator failingEvaluator = NewEvaluator(failing);
		budget = failingEvaluator.Evaluate(query);
		ok = Adapter("provider failure: evaluator", !budget.m_bValid
			&& budget.m_eReason == RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE) && ok;

		// 3. calculator rejects the provider's profile (step 0.5 km > 0.1 km): reason 16, no layer.
		RICON_TerrainDiffractionModel coarseModel = new RICON_TerrainDiffractionModel(config,
			new RICON_SyntheticTerrainProfileProvider(coarse, 10.0, 10.0));
		layers = {};
		ok = Adapter("bad profile: reason 16", coarseModel.AppendLosses(query, 1000.0, layers)
			== RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE && layers.IsEmpty()) && ok;

		// 4. < 2 m horizontal: terrain layer 0 without asking the (failing) provider.
		RICON_LinkQuery shortQuery = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50.0);
		shortQuery.m_vRxPos = "1 5 0";
		layers = {};
		ok = Adapter("< 2 m: OK without provider", failing.AppendLosses(shortQuery, 5.1, layers) == RICON_EReason.OK
			&& layers.Count() == 2) && ok;
		if (layers.Count() == 2)
			ok = Adapter("< 2 m: terrain layer 0", layers[1].m_fLossDb == 0
				&& layers[1].m_sLayerId == RICON_TerrainDiffractionModel.TERRAIN_LAYER_ID) && ok;

		// 5. invalid config and out-of-domain frequency.
		RICON_TerrainDiffractionConfig badConfig = Config(6371.0, 1.3333334, 1.0, 0.001,
			RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_TerrainDiffractionModel badModel = new RICON_TerrainDiffractionModel(badConfig,
			new RICON_SyntheticTerrainProfileProvider(flat, 10.0, 10.0));
		layers = {};
		ok = Adapter("invalid config", !badModel.IsConfigValid()
			&& badModel.AppendLosses(query, 1000.0, layers) == RICON_EReason.CATALOG_VIOLATION && layers.IsEmpty()) && ok;
		RICON_LinkQuery lowQuery = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 20.0);
		layers = {};
		ok = Adapter("frequency 20 MHz", !model.IsApplicable(20.0)
			&& model.AppendLosses(lowQuery, 1000.0, layers) == RICON_EReason.OUT_OF_MODEL_DOMAIN && layers.IsEmpty()) && ok;
		budget = evaluator.Evaluate(lowQuery);
		ok = Adapter("frequency 20 MHz: evaluator", !budget.m_bValid
			&& budget.m_eReason == RICON_EReason.OUT_OF_MODEL_DOMAIN) && ok;
		return ok;
	}

	protected static RICON_WirelessLinkEvaluator NewEvaluator(RICON_PropagationModel model)
	{
		return new RICON_WirelessLinkEvaluator(RICON_TestSupport.BAND_ID, model, new RICON_NoiseModel(),
			RICON_TestSupport.NewTestIntelligibility());
	}

	protected static bool Adapter(string what, bool condition)
	{
		if (!condition)
			PrintFormat("[RICON_TEST] FAIL l2 adapter: %1", what);
		return condition;
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
