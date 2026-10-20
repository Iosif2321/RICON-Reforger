//! TEST_ONLY: выбранные точки физики через настоящий evaluator; без мира, Play и серверов.
//! Эталонные числа добавляются только из принятой ревизии link_probe_points_v1.json.
[WorkbenchPluginAttribute(name: "RICON: проверка физических точек", description: "Чистый расчёт Core в Workbench", wbModules: {"ScriptEditor"})]
class RICON_LinkPhysicsProbePlugin : WorkbenchPlugin
{
	static const string TAG = "[RICON_PHYSICS] ";
	static const float DB_TOLERANCE = 0.05;
	protected static float s_fMaxDeviationDb;

	protected static bool Within(float actual, float expected, float tolerance)
	{
		return RICON_Num.IsFinite(actual) && RICON_Num.IsFinite(expected)
			&& Math.AbsFloat(actual - expected) <= tolerance;
	}

	protected static bool Check(bool condition, string pointId, string field, bool quiet)
	{
		if (quiet)
			return condition;
		return RICON_TestSupport.Expect(condition, pointId, field);
	}

	protected static bool CheckDb(string pointId, string field, float actual, float expected, bool quiet)
	{
		if (quiet)
			return Within(actual, expected, DB_TOLERANCE);
		if (RICON_Num.IsFinite(actual) && RICON_Num.IsFinite(expected))
			s_fMaxDeviationDb = Math.Max(s_fMaxDeviationDb, Math.AbsFloat(actual - expected));
		PrintFormat("[RICON_PHYSICS] %1 %2: actual=%3 expected=%4", pointId, field,
			actual.ToString(-1, 9), expected.ToString(-1, 9));
		return RICON_TestSupport.Expect(Within(actual, expected, DB_TOLERANCE), pointId, field);
	}

	protected static float GroundHeight(float distanceKm, float hillHeightM)
	{
		return hillHeightM * Math.Max(0, 1.0 - Math.AbsFloat(distanceKm - 2.0) / 0.5);
	}

	//! Модели удерживаются в локальных переменных и сильных ссылках evaluator (находка спринта 17).
	protected static RICON_LinkBudget EvaluatePoint(string modelId, string noiseId, float frequencyMHz,
		float distanceM, float antennaHeightM, float txPowerDbm, float hillHeightM)
	{
		RICON_PropagationModel propagation;
		RICON_TerrainDiffractionConfig config;
		RICON_TerrainProfile profile;
		RICON_SyntheticTerrainProfileProvider provider;
		if (modelId == RICON_TerrainDiffractionModel.MODEL_ID)
		{
			config = RICON_L2Run.Config(6371.0, 1.3333334, 15.0, 0.005,
				RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
			RICON_TerrainSamplingResult sampling = RICON_TerrainProfileSampling.BuildDistances(distanceM, config);
			if (sampling.m_eCode != RICON_ETerrainSamplingCode.OK)
				return null;
			profile = new RICON_TerrainProfile();
			foreach (float positionKm : sampling.m_aDistanceKm)
				profile.AddPoint(positionKm, GroundHeight(positionKm, hillHeightM));
			provider = new RICON_SyntheticTerrainProfileProvider(profile, antennaHeightM, antennaHeightM);
			propagation = new RICON_TerrainDiffractionModel(config, provider);
		}
		else if (modelId == RICON_FreeSpaceModel.MODEL_ID)
			propagation = new RICON_FreeSpaceModel();
		else
			return null;

		RICON_NoiseModel noise;
		if (noiseId == "none")
			noise = new RICON_NoiseModel();
		else if (noiseId == "rural")
			noise = new RICON_ManMadeNoiseModel("rural", 67.2, 27.7);
		else if (noiseId == "city")
			noise = new RICON_ManMadeNoiseModel("city", 76.8, 27.7);
		else
			return null;

		RICON_LinearTestIntelligibility quality = RICON_TestSupport.NewTestIntelligibility();
		RICON_WirelessLinkEvaluator evaluator = new RICON_WirelessLinkEvaluator(
			RICON_TestSupport.BAND_ID, propagation, noise, quality);
		RICON_LinkQuery query = RICON_TestSupport.NewQuery(modelId, frequencyMHz);
		query.m_vTxPos = Vector(0, antennaHeightM + GroundHeight(0, hillHeightM), 0);
		query.m_vRxPos = Vector(distanceM, antennaHeightM + GroundHeight(distanceM / 1000.0, hillHeightM), 0);
		query.m_fBandwidthHz = 25000.0;
		query.m_fRxNoiseFigureDb = 10.0;
		query.m_fTxPowerDbm = txPowerDbm;
		query.m_sNoiseEnvironmentId = noiseId;
		return evaluator.Evaluate(query);
	}

	protected static bool ComparePoint(string pointId, RICON_LinkBudget actual, RICON_LinkBudget expected,
		string modelId, bool quiet = false)
	{
		if (!Check(actual != null && expected != null, pointId, "бюджет существует", quiet))
			return false;
		if (!quiet)
			PrintFormat("[RICON_PHYSICS] %1 valid=%2 reason=%3 distanceM=%4", pointId, actual.m_bValid,
				typename.EnumToString(RICON_EReason, actual.m_eReason), actual.m_fDistanceM.ToString(-1, 9));
		bool ok = Check(actual.m_bValid && actual.m_eReason == RICON_EReason.OK,
			pointId, "valid / reason", quiet);
		ok = CheckDb(pointId, "totalLossDb", actual.m_fTotalLossDb, expected.m_fTotalLossDb, quiet) && ok;
		ok = CheckDb(pointId, "rxPowerDbm", actual.m_fRxPowerDbm, expected.m_fRxPowerDbm, quiet) && ok;
		ok = CheckDb(pointId, "thermalNoiseDbm", actual.m_fThermalNoiseDbm, expected.m_fThermalNoiseDbm, quiet) && ok;
		ok = CheckDb(pointId, "noiseDbm", actual.m_fNoiseDbm, expected.m_fNoiseDbm, quiet) && ok;
		ok = CheckDb(pointId, "sinrDb", actual.m_fSinrDb, expected.m_fSinrDb, quiet) && ok;
		ok = CheckDb(pointId, "sensitivityDbm", actual.m_fSensitivityDbm, expected.m_fSensitivityDbm, quiet) && ok;
		ok = Check(Within(actual.m_fDistanceM, expected.m_fDistanceM, 0.001),
			pointId, "3D distance, метры", quiet) && ok;
		ok = Check(actual.m_bNoiseModelInDomain == expected.m_bNoiseModelInDomain,
			pointId, "noiseModelInDomain", quiet) && ok;
		ok = Check(actual.m_eState == expected.m_eState,
			pointId, "TEST_ONLY state", quiet) && ok;
		ok = Check(Within(actual.m_fIntelligibility, expected.m_fIntelligibility, 0.000001),
			pointId, "TEST_ONLY Q", quiet) && ok;
		ok = Check((actual.m_fRxPowerDbm >= actual.m_fSensitivityDbm)
			== (expected.m_fRxPowerDbm >= expected.m_fSensitivityDbm),
			pointId, "aboveSensitivity", quiet) && ok;
		ok = Check((actual.m_fSinrDb >= 10.0) == (expected.m_fSinrDb >= 10.0),
			pointId, "meetsSnr", quiet) && ok;
		ok = Check(actual.m_aLayers.Count() == expected.m_aLayers.Count(),
			pointId, "число слоёв", quiet) && ok;
		if (actual.m_aLayers.Count() == expected.m_aLayers.Count())
		{
			for (int i = 0; i < expected.m_aLayers.Count(); i++)
			{
				RICON_LossLayer layer = actual.m_aLayers[i];
				RICON_LossLayer anchor = expected.m_aLayers[i];
				if (!Check(layer != null && anchor != null, pointId, "слой существует", quiet))
					return false;
				ok = Check(layer.m_sLayerId == anchor.m_sLayerId && layer.m_sModelId == modelId,
					pointId, "порядок / id слоёв", quiet) && ok;
				ok = CheckDb(pointId, layer.m_sLayerId, layer.m_fLossDb, anchor.m_fLossDb, quiet) && ok;
			}
		}
		if (!quiet)
			PrintFormat("[RICON_PHYSICS] %1 state=%2 Q=%3 inDomain=%4", pointId,
				typename.EnumToString(RICON_ELinkState, actual.m_eState), actual.m_fIntelligibility.ToString(-1, 9),
				actual.m_bNoiseModelInDomain);
		return ok;
	}

	override void RunCommandline()
	{
		s_fMaxDeviationDb = 0;
		Print(TAG + "source=35d7699 sha256=0789b8ebdb577c2a51a2eb8d897827ba7e7debaa4aad90dbebfe31d212f1adf6");
		bool ok = true;
		int cases = 0;
		int controls = 0;
		RICON_LinkBudget actual;
		RICON_LinkBudget expected;
		string modelId;

		modelId = RICON_TerrainDiffractionModel.MODEL_ID;
		expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 3000.0;
		expected.m_fTotalLossDb = 118.31774030310939;
		expected.m_fRxPowerDbm = -81.3280402597492;
		expected.m_fThermalNoiseDbm = -129.99578710750774;
		expected.m_fNoiseDbm = -115.1433064941856;
		expected.m_fSinrDb = 33.81526623443639;
		expected.m_fSensitivityDbm = -109.99578710750774;
		expected.m_fIntelligibility = 1.0;
		expected.m_bNoiseModelInDomain = true;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 80.87986175928, modelId));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 37.437878543829385, modelId));
		actual = EvaluatePoint(modelId, "rural", 88.0, 3000.0, 3.0, 36.98970004336019, 0.0);
		ok = ComparePoint("P01", actual, expected, modelId) && ok;
		ok = RICON_TestSupport.Expect(ComparePoint("P01", actual, expected, modelId, true), "P01", "quiet positive") && ok;
		cases++;
		float savedLoss = expected.m_fTotalLossDb;
		expected.m_fTotalLossDb += 2 * DB_TOLERANCE;
		if (!ComparePoint("X_loss", actual, expected, modelId, true))
			controls++;
		expected.m_fTotalLossDb = savedLoss;

		modelId = RICON_TerrainDiffractionModel.MODEL_ID;
		expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 10000.0;
		expected.m_fTotalLossDb = 140.81483862130216;
		expected.m_fRxPowerDbm = -103.82513857794197;
		expected.m_fThermalNoiseDbm = -129.99578710750774;
		expected.m_fNoiseDbm = -115.1433064941856;
		expected.m_fSinrDb = 11.318167916243624;
		expected.m_fSensitivityDbm = -109.99578710750774;
		expected.m_fIntelligibility = 0.8772722638747875;
		expected.m_bNoiseModelInDomain = true;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 91.33743666488675, modelId));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 49.477401956415406, modelId));
		actual = EvaluatePoint(modelId, "rural", 88.0, 10000.0, 3.0, 36.98970004336019, 0.0);
		ok = ComparePoint("P02", actual, expected, modelId) && ok;
		ok = RICON_TestSupport.Expect(ComparePoint("P02", actual, expected, modelId, true), "P02", "quiet positive") && ok;
		cases++;
		string savedLayerId = expected.m_aLayers[0].m_sLayerId;
		expected.m_aLayers[0].m_sLayerId = "wrong_layer";
		if (!ComparePoint("X_layer_id", actual, expected, modelId, true))
			controls++;
		expected.m_aLayers[0].m_sLayerId = savedLayerId;

		modelId = RICON_TerrainDiffractionModel.MODEL_ID;
		expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 1000.0;
		expected.m_fTotalLossDb = 87.33194719350867;
		expected.m_fRxPowerDbm = -57.33194719350867;
		expected.m_fThermalNoiseDbm = -129.99578710750774;
		expected.m_fNoiseDbm = -103.62103479827849;
		expected.m_fSinrDb = 46.28908760476982;
		expected.m_fSensitivityDbm = -109.99578710750774;
		expected.m_fIntelligibility = 1.0;
		expected.m_bNoiseModelInDomain = true;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 61.99020831627662, modelId));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 25.341738877232046, modelId));
		actual = EvaluatePoint(modelId, "rural", 30.0, 1000.0, 1.5, 30.0, 60.0);
		ok = ComparePoint("P03", actual, expected, modelId) && ok;
		ok = RICON_TestSupport.Expect(ComparePoint("P03", actual, expected, modelId, true), "P03", "quiet positive") && ok;
		cases++;
		ref RICON_LossLayer savedLayer = expected.m_aLayers[1];
		expected.m_aLayers.Remove(1);
		if (!ComparePoint("X_layer_count", actual, expected, modelId, true))
			controls++;
		expected.m_aLayers.Insert(savedLayer);

		modelId = RICON_TerrainDiffractionModel.MODEL_ID;
		expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 10000.0;
		expected.m_fTotalLossDb = 113.8194514369882;
		expected.m_fRxPowerDbm = -83.8194514369882;
		expected.m_fThermalNoiseDbm = -129.99578710750774;
		expected.m_fNoiseDbm = -103.62103479827849;
		expected.m_fSinrDb = 19.801583361290284;
		expected.m_fSensitivityDbm = -109.99578710750774;
		expected.m_fIntelligibility = 1.0;
		expected.m_bNoiseModelInDomain = true;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 81.99020831627662, modelId));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 31.82924312071158, modelId));
		actual = EvaluatePoint(modelId, "rural", 30.0, 10000.0, 1.5, 30.0, 60.0);
		ok = ComparePoint("P04", actual, expected, modelId) && ok;
		ok = RICON_TestSupport.Expect(ComparePoint("P04", actual, expected, modelId, true), "P04", "quiet positive") && ok;
		cases++;

		modelId = RICON_TerrainDiffractionModel.MODEL_ID;
		expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 2000.8997975910738;
		expected.m_fTotalLossDb = 81.16886027092181;
		expected.m_fRxPowerDbm = -44.179160227561624;
		expected.m_fThermalNoiseDbm = -129.99578710750774;
		expected.m_fNoiseDbm = -100.21594136130244;
		expected.m_fSinrDb = 56.03678113374082;
		expected.m_fSensitivityDbm = -109.99578710750774;
		expected.m_fIntelligibility = 1.0;
		expected.m_bNoiseModelInDomain = true;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 72.45169011438247, modelId));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 8.71717015653934, modelId));
		actual = EvaluatePoint(modelId, "city", 50.0, 2000.0, 3.0, 36.98970004336019, 60.0);
		ok = ComparePoint("P05", actual, expected, modelId) && ok;
		ok = RICON_TestSupport.Expect(ComparePoint("P05", actual, expected, modelId, true), "P05", "quiet positive") && ok;
		cases++;

		modelId = RICON_TerrainDiffractionModel.MODEL_ID;
		expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 10000.0;
		expected.m_fTotalLossDb = 120.02124660933212;
		expected.m_fRxPowerDbm = -83.03154656597194;
		expected.m_fThermalNoiseDbm = -129.99578710750774;
		expected.m_fNoiseDbm = -119.99578710750774;
		expected.m_fSinrDb = 36.9642405415358;
		expected.m_fSensitivityDbm = -109.99578710750774;
		expected.m_fIntelligibility = 1.0;
		expected.m_bNoiseModelInDomain = false;
		expected.m_eState = RICON_ELinkState.CLEAN;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 104.48898304844262, modelId));
		expected.m_aLayers.Insert(new RICON_LossLayer("terrain_diffraction", 15.532263560889504, modelId));
		actual = EvaluatePoint(modelId, "rural", 400.0, 10000.0, 10.0, 36.98970004336019, 0.0);
		ok = ComparePoint("P06", actual, expected, modelId) && ok;
		ok = RICON_TestSupport.Expect(ComparePoint("P06", actual, expected, modelId, true), "P06", "quiet positive") && ok;
		cases++;
		bool savedDomain = expected.m_bNoiseModelInDomain;
		expected.m_bNoiseModelInDomain = true;
		if (!ComparePoint("X_domain", actual, expected, modelId, true))
			controls++;
		expected.m_bNoiseModelInDomain = savedDomain;
		RICON_LinkBudget receiverOnly = EvaluatePoint(modelId, "none", 400.0, 10000.0, 10.0, 36.98970004336019, 0.0);
		ok = RICON_TestSupport.Expect(receiverOnly != null && receiverOnly.m_bNoiseModelInDomain, "P06", "fallback none") && ok;
		if (receiverOnly && actual)
		{
			ok = RICON_TestSupport.Expect(Within(actual.m_fNoiseDbm, receiverOnly.m_fNoiseDbm, DB_TOLERANCE)
				&& Within(actual.m_fSinrDb, receiverOnly.m_fSinrDb, DB_TOLERANCE), "P06", "fallback values") && ok;
		}

		modelId = RICON_FreeSpaceModel.MODEL_ID;
		expected = new RICON_LinkBudget();
		expected.m_fDistanceM = 14141.308091433231;
		expected.m_fTotalLossDb = 85.0;
		expected.m_fRxPowerDbm = -105.0;
		expected.m_fThermalNoiseDbm = -129.99578710750774;
		expected.m_fNoiseDbm = -94.10197309224583;
		expected.m_fSinrDb = -10.898026907754172;
		expected.m_fSensitivityDbm = -109.99578710750774;
		expected.m_fIntelligibility = 0.13673243640819427;
		expected.m_bNoiseModelInDomain = true;
		expected.m_eState = RICON_ELinkState.WEAK;
		expected.m_aLayers.Insert(new RICON_LossLayer("fspl", 85.0, modelId));
		actual = EvaluatePoint(modelId, "city", 30.0, 14141.308091433231, 1.5, -20.0, 0.0);
		ok = ComparePoint("P07", actual, expected, modelId) && ok;
		ok = RICON_TestSupport.Expect(ComparePoint("P07", actual, expected, modelId, true), "P07", "quiet positive") && ok;
		cases++;
		RICON_ELinkState savedState = expected.m_eState;
		expected.m_eState = RICON_ELinkState.CLEAN;
		if (!ComparePoint("X_state", actual, expected, modelId, true))
			controls++;
		expected.m_eState = savedState;

		float nonFinite = RICON_L2Run.NaN();
		ok = RICON_TestSupport.Expect(!Within(nonFinite, 0, DB_TOLERANCE)
			&& !Within(0, nonFinite, DB_TOLERANCE), "physics", "NaN actual / expected") && ok;
		ok = RICON_TestSupport.Expect(cases == 7 && controls == 5, "physics", "cases / controls") && ok;
		PrintFormat("[RICON_PHYSICS] cases=%1 controls=%2 maxDeviationDb=%3", cases, controls, s_fMaxDeviationDb.ToString(-1, 9));
		if (ok)
		{
			Print(TAG + "done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print(TAG + "done: FAIL");
			Workbench.Exit(5);
		}
	}
}
