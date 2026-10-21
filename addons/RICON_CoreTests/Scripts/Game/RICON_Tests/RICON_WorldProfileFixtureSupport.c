//! TEST_ONLY: независимые векторы геометрии профиля; настоящего мира здесь нет.
class RICON_TestHeightHill
{
	float m_fX;
	float m_fZ;
	float m_fHeight;
	float m_fSigma;

	void RICON_TestHeightHill(float x, float z, float height, float sigma)
	{
		m_fX = x;
		m_fZ = z;
		m_fHeight = height;
		m_fSigma = sigma;
	}
}

class RICON_AnalyticHeightSource : RICON_HeightSource
{
	float m_fBase;
	float m_fSlopeX;
	float m_fSlopeZ;
	int m_iFailAtCall = -1;
	int m_iNanAtCall = -1;
	int m_iCalls;
	ref array<ref RICON_TestHeightHill> m_aHills = {};
	ref array<vector> m_aPoints = {};

	override bool TryGetHeight(float x, float z, out float heightM)
	{
		int index = m_iCalls++;
		m_aPoints.Insert(Vector(x, 0, z));
		heightM = 0;
		if (index == m_iFailAtCall)
			return false;
		if (index == m_iNanAtCall)
		{
			heightM = RICON_L2Run.NaN();
			return true;
		}
		heightM = m_fBase + m_fSlopeX * x + m_fSlopeZ * z;
		foreach (RICON_TestHeightHill hill : m_aHills)
		{
			float dx = x - hill.m_fX;
			float dz = z - hill.m_fZ;
			heightM += hill.m_fHeight * Math.Pow(Math.E, -(dx * dx + dz * dz) / (2.0 * hill.m_fSigma * hill.m_fSigma));
		}
		return true;
	}
}

class RICON_WorldProfileObservation
{
	bool m_bProviderCalled;
	RICON_EReason m_eProviderReason;
	ref RICON_TerrainProfile m_Profile;
	float m_fTxAgl;
	float m_fRxAgl;
	int m_iHeightCalls;
	bool m_bModel;
	RICON_EReason m_eModelReason;
	bool m_bHasL2;
	RICON_EL2ResultCode m_eL2Code;
	ref array<ref RICON_LossLayer> m_aLayers = {};
}

//! Обёртка наблюдает реальный GetProfile; его решение не подменяется.
class RICON_ObservedWorldProfileProvider : RICON_WorldProfileProvider
{
	ref RICON_WorldProfileObservation m_Observed = new RICON_WorldProfileObservation();

	override RICON_EReason GetProfile(RICON_LinkQuery query, RICON_TerrainDiffractionConfig config,
		out RICON_TerrainProfile profile, out float transmitterHeightAglM, out float receiverHeightAglM)
	{
		m_Observed.m_bProviderCalled = true;
		RICON_EReason reason = super.GetProfile(query, config, profile, transmitterHeightAglM, receiverHeightAglM);
		m_Observed.m_eProviderReason = reason;
		m_Observed.m_Profile = profile;
		m_Observed.m_fTxAgl = transmitterHeightAglM;
		m_Observed.m_fRxAgl = receiverHeightAglM;
		return reason;
	}
}

class RICON_WorldProfileRun
{
	int m_iCases;
	int m_iQuietPositive;
	int m_iControls;
	int m_iEndpointChecks;
	int m_iBudgetChecks;
	bool m_bOk = true;

	protected static bool ExactEnds(RICON_AnalyticHeightSource source, RICON_LinkQuery query)
	{
		if (!source || !query || source.m_aPoints.IsEmpty())
			return false;
		vector first = source.m_aPoints[0];
		vector last = source.m_aPoints[source.m_aPoints.Count() - 1];
		return first[0] == query.m_vTxPos[0] && first[2] == query.m_vTxPos[2]
			&& last[0] == query.m_vRxPos[0] && last[2] == query.m_vRxPos[2];
	}

	protected static bool Within(float actual, float expected, float tolerance)
	{
		return RICON_Num.IsFinite(actual) && RICON_Num.IsFinite(expected) && Math.AbsFloat(actual - expected) <= tolerance;
	}

	static bool Compare(RICON_WorldProfileObservation actual, RICON_WorldProfileObservation expected)
	{
		if (!actual || !expected || actual.m_bProviderCalled != expected.m_bProviderCalled
			|| actual.m_iHeightCalls != expected.m_iHeightCalls || actual.m_bModel != expected.m_bModel
			|| actual.m_bHasL2 != expected.m_bHasL2)
			return false;
		if (actual.m_bProviderCalled && actual.m_eProviderReason != expected.m_eProviderReason)
			return false;
		if (actual.m_bModel && actual.m_eModelReason != expected.m_eModelReason)
			return false;
		if (actual.m_bHasL2 && actual.m_eL2Code != expected.m_eL2Code)
			return false;
		if (!Within(actual.m_fTxAgl, expected.m_fTxAgl, 0.01) || !Within(actual.m_fRxAgl, expected.m_fRxAgl, 0.01))
			return false;
		if ((actual.m_Profile != null) != (expected.m_Profile != null))
			return false;
		if (!actual.m_Profile)
		{
			if (actual.m_fTxAgl != 0 || actual.m_fRxAgl != 0)
				return false;
		}
		else
		{
			int count = expected.m_Profile.Count();
			if (count == 0 || actual.m_Profile.Count() != count || actual.m_Profile.m_aHeightM.Count() != count)
				return false;
			float distanceTolerance = 0.000001 * (1.0 + expected.m_Profile.m_aDistanceKm[count - 1]);
			for (int i = 0; i < count; i++)
			{
				if (!Within(actual.m_Profile.m_aDistanceKm[i], expected.m_Profile.m_aDistanceKm[i], distanceTolerance)
					|| !Within(actual.m_Profile.m_aHeightM[i], expected.m_Profile.m_aHeightM[i], 0.01))
					return false;
			}
		}
		if (actual.m_aLayers.Count() != expected.m_aLayers.Count())
			return false;
		for (int j = 0; j < expected.m_aLayers.Count(); j++)
		{
			RICON_LossLayer a = actual.m_aLayers[j];
			RICON_LossLayer e = expected.m_aLayers[j];
			if (!a || !e || a.m_sLayerId != e.m_sLayerId || a.m_sModelId != e.m_sModelId
				|| !Within(a.m_fLossDb, e.m_fLossDb, 0.05))
				return false;
		}
		return true;
	}

	protected void Require(bool condition, string id, string what)
	{
		m_bOk = RICON_TestSupport.Expect(condition, id, what) && m_bOk;
	}

	protected void Reject(string what, bool accepted)
	{
		Require(!accepted, "world_profile controls", what);
		if (!accepted)
			m_iControls++;
	}

	protected void Controls(RICON_WorldProfileObservation actual, RICON_WorldProfileObservation expected)
	{
		expected.m_iHeightCalls++;
		Reject("число обращений", Compare(actual, expected));
		expected.m_iHeightCalls--;
		expected.m_eProviderReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		Reject("причина поставщика", Compare(actual, expected));
		expected.m_eProviderReason = RICON_EReason.OK;
		expected.m_eModelReason = RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		Reject("причина модели", Compare(actual, expected));
		expected.m_eModelReason = RICON_EReason.OK;
		expected.m_fTxAgl += 0.1;
		Reject("высота TX", Compare(actual, expected));
		expected.m_fTxAgl -= 0.1;
		expected.m_fRxAgl += 0.1;
		Reject("высота RX", Compare(actual, expected));
		expected.m_fRxAgl -= 0.1;
		float saved = expected.m_Profile.m_aHeightM[0];
		expected.m_Profile.m_aHeightM[0] = saved + 0.1;
		Reject("высота точки", Compare(actual, expected));
		expected.m_Profile.m_aHeightM[0] = saved;
		saved = expected.m_Profile.m_aDistanceKm[1];
		expected.m_Profile.m_aDistanceKm[1] = saved + 0.01;
		Reject("дистанция точки", Compare(actual, expected));
		expected.m_Profile.m_aDistanceKm[1] = saved;
		actual.m_Profile.m_aHeightM.Insert(0);
		Reject("размер профиля", Compare(actual, expected));
		actual.m_Profile.m_aHeightM.Remove(actual.m_Profile.m_aHeightM.Count() - 1);
		saved = expected.m_aLayers[1].m_fLossDb;
		expected.m_aLayers[1].m_fLossDb += 0.1;
		Reject("потери рельефа", Compare(actual, expected));
		expected.m_aLayers[1].m_fLossDb = saved;
		expected.m_bProviderCalled = false;
		Reject("факт вызова поставщика", Compare(actual, expected));
		expected.m_bProviderCalled = true;
		Reject("actual null", Compare(null, expected));
		Reject("expected null", Compare(actual, null));
		saved = actual.m_Profile.m_aHeightM[0];
		actual.m_Profile.m_aHeightM[0] = RICON_L2Run.NaN();
		Reject("нечисловая высота", Compare(actual, expected));
		actual.m_Profile.m_aHeightM[0] = saved;
	}

	void CheckCase(string id, bool modelCase, RICON_LinkQuery query, RICON_TerrainDiffractionConfig config,
		RICON_AnalyticHeightSource source, RICON_WorldProfileObservation expected)
	{
		m_iCases++;
		RICON_ObservedWorldProfileProvider provider = new RICON_ObservedWorldProfileProvider(source);
		RICON_WorldProfileObservation actual = provider.m_Observed;
		actual.m_bModel = modelCase;
		if (modelCase)
		{
			RICON_TerrainDiffractionModel model = new RICON_TerrainDiffractionModel(config, provider);
			actual.m_eModelReason = model.AppendLosses(query, vector.Distance(query.m_vTxPos, query.m_vRxPos), actual.m_aLayers);
			if (actual.m_Profile)
			{
				RICON_L2Result l2 = RICON_TerrainDiffractionCalculator.Evaluate(actual.m_Profile,
					actual.m_fTxAgl, actual.m_fRxAgl, query.m_fFrequencyMHz, config);
				actual.m_bHasL2 = true;
				actual.m_eL2Code = l2.m_eCode;
			}
		}
		else
		{
			RICON_TerrainProfile profile = new RICON_TerrainProfile();
			float txHeight = 123;
			float rxHeight = 456;
			provider.GetProfile(query, config, profile, txHeight, rxHeight);
		}
		if (source)
			actual.m_iHeightCalls = source.m_iCalls;
		Require(Compare(actual, expected), id, "совпадение с эталоном");
		if (Compare(actual, expected))
			m_iQuietPositive++;
		int count = 0;
		if (actual.m_Profile)
		{
			count = actual.m_Profile.Count();
			if (source && count > 0)
			{
				Require(source.m_aPoints.Count() == count && ExactEnds(source, query), id, "точные TX и RX");
			}
			for (int k = 0; k < count; k++)
				PrintFormat("[RICON_WORLD] %1 point=%2 d=%3 h=%4", id, k,
					actual.m_Profile.m_aDistanceKm[k].ToString(-1, 9), actual.m_Profile.m_aHeightM[k].ToString(-1, 9));
		}
		PrintFormat("[RICON_WORLD] %1 called=%2 providerReason=%3 model=%4 modelReason=%5 heightCalls=%6 n=%7 l2=%8",
			id, actual.m_bProviderCalled, actual.m_eProviderReason, actual.m_bModel, actual.m_eModelReason,
			actual.m_iHeightCalls, count, actual.m_eL2Code);
		PrintFormat("[RICON_WORLD] %1 htg=%2 hrg=%3", id, actual.m_fTxAgl.ToString(-1, 9), actual.m_fRxAgl.ToString(-1, 9));
		foreach (RICON_LossLayer layer : actual.m_aLayers)
			PrintFormat("[RICON_WORLD] %1 layer=%2 loss=%3", id, layer.m_sLayerId, layer.m_fLossDb.ToString(-1, 9));
		if (id == "W01" && Compare(actual, expected))
			Controls(actual, expected);
	}

	void CheckNullWorld()
	{
		RICON_BaseWorldHeightSource source = new RICON_BaseWorldHeightSource(null);
		float height = 123;
		Require(!source.TryGetHeight(1, 2, height) && height == 0, "world_profile", "мир отсутствует: отказ без API");
		Require(RICON_TestSupport.FloatClass(RICON_L0Run.Special(0)) == 1
			&& RICON_TestSupport.FloatClass(RICON_L0Run.Special(1)) == 2
			&& RICON_TestSupport.FloatClass(RICON_L0Run.Special(-1)) == 3,
			"world_profile", "классы NaN и бесконечностей");

		RICON_LinkQuery query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50);
		query.m_vTxPos = Vector(13000, 2, 10000);
		query.m_vRxPos = Vector(0.01, 2, 0.1);
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371, 1.3333334, 15, 0.001,
			RICON_ETerrainPolarization.VERTICAL, 10, 1000);
		RICON_AnalyticHeightSource analytic = new RICON_AnalyticHeightSource();
		RICON_WorldProfileProvider provider = new RICON_WorldProfileProvider(analytic);
		RICON_TerrainProfile profile;
		float txAgl;
		float rxAgl;
		bool exact = provider.GetProfile(query, config, profile, txAgl, rxAgl) == RICON_EReason.OK
			&& analytic.m_iCalls == 3 && ExactEnds(analytic, query);
		Require(exact, "world_profile", "малые координаты RX после большого TX без потери разрядов");
		if (exact)
		{
			m_iEndpointChecks++;
			vector saved = analytic.m_aPoints[2];
			analytic.m_aPoints[2] = Vector(0.009765625, 0, 0.099609375);
			Reject("потеря младших разрядов RX", ExactEnds(analytic, query));
			analytic.m_aPoints[2] = saved;
		}
	}

	//! W01 плюс явные TEST_ONLY параметры приёмника: полный путь до SINR и состояния.
	void CheckBudgetChain()
	{
		RICON_AnalyticHeightSource source = new RICON_AnalyticHeightSource();
		source.m_fBase = 100;
		RICON_WorldProfileProvider provider = new RICON_WorldProfileProvider(source);
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371, 1.3333334, 15, 0.001,
			RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_TerrainDiffractionModel model = new RICON_TerrainDiffractionModel(config, provider);
		RICON_NoiseModel noise = new RICON_NoiseModel();
		RICON_LinearTestIntelligibility intelligibility = RICON_TestSupport.NewTestIntelligibility();
		RICON_WirelessLinkEvaluator evaluator = new RICON_WirelessLinkEvaluator(RICON_TestSupport.BAND_ID,
			model, noise, intelligibility);
		RICON_LinkQuery query = RICON_TestSupport.NewQuery(RICON_TerrainDiffractionModel.MODEL_ID, 50);
		query.m_vTxPos = Vector(0, 101.5, 0);
		query.m_vRxPos = Vector(2000, 101.5, 0);
		RICON_LinkBudget budget = evaluator.Evaluate(query);
		if (!budget)
		{
			Require(false, "world_budget", "бюджет существует");
			return;
		}
		bool ok = budget.m_bValid && budget.m_eReason == RICON_EReason.OK && source.m_iCalls == 21
			&& budget.m_aLayers.Count() == 2 && budget.m_bNoiseModelInDomain
			&& Within(budget.m_fTotalLossDb, 108.1797261261363, 0.05)
			&& Within(budget.m_fRxPowerDbm, -71.1797261261363, 0.05)
			&& Within(budget.m_fThermalNoiseDbm, -133.00608706414755, 0.05)
			&& Within(budget.m_fNoiseDbm, -125.00608706414755, 0.05)
			&& Within(budget.m_fNoisePlusInterferenceDbm, -125.00608706414755, 0.05)
			&& Within(budget.m_fSinrDb, 53.82636093801125, 0.05)
			&& Within(budget.m_fSensitivityDbm, -115.00608706414755, 0.05)
			&& budget.m_fIntelligibility == 1 && budget.m_eState == RICON_ELinkState.CLEAN;
		Require(ok, "world_budget", "источник → профиль → потери → мощность → SINR → TEST_ONLY Q");
		if (ok)
			m_iBudgetChecks++;
		PrintFormat("[RICON_WORLD] budget loss=%1 rx=%2 thermal=%3 noise=%4 noisePlusI=%5 sinr=%6 sensitivity=%7 Q=%8",
			budget.m_fTotalLossDb.ToString(-1, 9), budget.m_fRxPowerDbm.ToString(-1, 9),
			budget.m_fThermalNoiseDbm.ToString(-1, 9), budget.m_fNoiseDbm.ToString(-1, 9),
			budget.m_fNoisePlusInterferenceDbm.ToString(-1, 9), budget.m_fSinrDb.ToString(-1, 9),
			budget.m_fSensitivityDbm.ToString(-1, 9), budget.m_fIntelligibility.ToString(-1, 9));
	}
}
