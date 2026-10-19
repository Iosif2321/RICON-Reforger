// TEST_ONLY compile fixtures for the five extension scenarios of REQ-core §10. Each scenario uses only the public
// contracts of RICON_Core; none of them changes a core file. Numbers are test values, not physics.
// Status: SELF_CHECK_PASS (executed in Workbench by tools/RICON_TestRunner, 2026-10-19; no Play, no server).

//! EXT-1: a new VHF radio is configuration only. No new class: existing models + a BandConfig.
class RICON_Ext1NewVhfRadio
{
	static bool SelfCheck()
	{
		RICON_BandConfig config = new RICON_BandConfig(RICON_TestSupport.BAND_ID, RICON_FreeSpaceModel.MODEL_ID,
			RICON_TestSupport.NOISE_ID, RICON_LinearTestIntelligibility.MODEL_ID, 1);
		bool ok = RICON_TestSupport.Expect(config.IsValid(), "EXT-1", "band config must be valid");

		RICON_WirelessLinkEvaluator evaluator = new RICON_WirelessLinkEvaluator(config.m_sBandId,
			new RICON_FreeSpaceModel(), new RICON_NoiseModel(), RICON_TestSupport.NewTestIntelligibility());
		RICON_LinkBudget budget = evaluator.Evaluate(RICON_TestSupport.NewQuery(RICON_FreeSpaceModel.MODEL_ID, 150.0));
		ok = RICON_TestSupport.Expect(budget.m_bValid, "EXT-1", "budget must be valid") && ok;
		ok = RICON_TestSupport.Expect(budget.FindLayer(RICON_FreeSpaceModel.LAYER_ID) != null, "EXT-1", "fspl layer present") && ok;
		return ok;
	}
}

//! EXT-2: an HF sky-wave model is a new PropagationModel with its own layer id; query/budget do not change.
//! TEST_ONLY: the ionospheric loss is a fixed test value, not a sky-wave model.
class RICON_Ext2SkyWaveModel : RICON_PropagationModel
{
	static const string MODEL_ID = "hf_skywave_test";
	static const string LAYER_ID = "ionosphere_test";
	static const float FIXED_IONOSPHERE_LOSS_DB = 20.0;

	override string GetModelId()
	{
		return MODEL_ID;
	}

	override bool IsConfigValid()
	{
		return true;
	}

	override bool IsApplicable(float frequencyMHz)
	{
		return frequencyMHz >= 3.0 && frequencyMHz <= 30.0;
	}

	override RICON_EReason AppendLosses(RICON_LinkQuery query, float distanceM, notnull array<ref RICON_LossLayer> layers)
	{
		layers.Insert(new RICON_LossLayer(RICON_FreeSpaceModel.LAYER_ID, RICON_FreeSpaceModel.LossDb(distanceM, query.m_fFrequencyMHz), MODEL_ID));
		layers.Insert(new RICON_LossLayer(LAYER_ID, FIXED_IONOSPHERE_LOSS_DB, MODEL_ID));
		return RICON_EReason.OK;
	}

	static bool SelfCheck()
	{
		RICON_WirelessLinkEvaluator evaluator = new RICON_WirelessLinkEvaluator(RICON_TestSupport.BAND_ID,
			new RICON_Ext2SkyWaveModel(), new RICON_NoiseModel(), RICON_TestSupport.NewTestIntelligibility());

		RICON_LinkBudget inDomain = evaluator.Evaluate(RICON_TestSupport.NewQuery(MODEL_ID, 10.0));
		bool ok = RICON_TestSupport.Expect(inDomain.m_bValid, "EXT-2", "10 MHz must be valid");
		RICON_LossLayer ionosphere = inDomain.FindLayer(LAYER_ID);
		ok = RICON_TestSupport.Expect(ionosphere != null, "EXT-2", "own layer present") && ok;
		RICON_LossLayer freeSpace = inDomain.FindLayer(RICON_FreeSpaceModel.LAYER_ID);
		if (ionosphere && freeSpace)
		{
			float expectedTotal = freeSpace.m_fLossDb + ionosphere.m_fLossDb;
			ok = RICON_TestSupport.Expect(float.AlmostEqual(inDomain.m_fTotalLossDb, expectedTotal, 0.01), "EXT-2", "total = sum of layers") && ok;
		}

		RICON_LinkBudget outOfDomain = evaluator.Evaluate(RICON_TestSupport.NewQuery(MODEL_ID, 100.0));
		ok = RICON_TestSupport.Expect(!outOfDomain.m_bValid && outOfDomain.m_eReason == RICON_EReason.OUT_OF_MODEL_DOMAIN,
			"EXT-2", "100 MHz must be OUT_OF_MODEL_DOMAIN") && ok;
		return ok;
	}
}

//! EXT-3: a satellite network is a new RoutingPolicy plus the common CallSession seam; no cellular code.
//! TEST_ONLY: the route goes through one fixed satellite endpoint with test budgets.
class RICON_Ext3SatelliteNetwork : RICON_RoutingPolicy
{
	static const string NETWORK_TYPE_ID = "satellite_test";
	static const int SATELLITE_ENDPOINT_ID = 9000;

	override string GetNetworkTypeId()
	{
		return NETWORK_TYPE_ID;
	}

	override bool IsConfigValid()
	{
		return true;
	}

	override bool RequiresCallSessionFactory()
	{
		return true;
	}

	override RICON_Route FindRoute(RICON_RoutingContext context, int talkerEndpointId, int listenerEndpointId)
	{
		RICON_Route route = new RICON_Route(talkerEndpointId, listenerEndpointId);
		route.AddLeg(new RICON_RouteLeg(talkerEndpointId, SATELLITE_ENDPOINT_ID,
			RICON_TestSupport.NewValidBudget(0.9, RICON_ELinkState.CLEAN)));
		route.AddLeg(new RICON_RouteLeg(SATELLITE_ENDPOINT_ID, listenerEndpointId,
			RICON_TestSupport.NewValidBudget(0.6, RICON_ELinkState.DEGRADED)));
		return route;
	}

	static bool SelfCheck()
	{
		RICON_Ext3SatelliteNetwork policy = new RICON_Ext3SatelliteNetwork();
		RICON_Ext3SatelliteCallFactory factory = new RICON_Ext3SatelliteCallFactory();
		bool ok = RICON_TestSupport.Expect(policy.IsConfigValid() && factory.IsConfigValid(), "EXT-3", "configs valid");
		ok = RICON_TestSupport.Expect(factory.GetNetworkTypeId() == policy.GetNetworkTypeId(), "EXT-3", "same network type id") && ok;

		RICON_RoutingContext context = new RICON_RoutingContext(7, 0, 2, 1, new RICON_TestAllowAllAccessPolicy());
		RICON_Route route = policy.FindRoute(context, 1, 2);
		ok = RICON_TestSupport.Expect(route != null, "EXT-3", "route found") && ok;
		if (route)
		{
			ok = RICON_TestSupport.Expect(route.Evaluate(context) == RICON_ERouteReason.OK, "EXT-3", "route valid") && ok;
			ok = RICON_TestSupport.Expect(float.AlmostEqual(route.GetQuality(), 0.6), "EXT-3", "quality = min(legs)") && ok;
			ok = RICON_TestSupport.Expect(route.GetState() == RICON_ELinkState.DEGRADED, "EXT-3", "state = worst leg") && ok;
		}

		RICON_CallSession session = factory.Create(context, 1, 1, 2);
		ok = RICON_TestSupport.Expect(session != null && session.GetNetworkTypeId() == NETWORK_TYPE_ID, "EXT-3", "call session created") && ok;
		return ok;
	}

	//! Registration through the real authority seam (REQ-core §9.3, CT-CORE-012). Compile coverage only:
	//! it needs a live manager on the server and is not called by SelfCheck.
	static bool RegisterInto(notnull RICON_CommsManagerComponent manager)
	{
		return manager.RegisterNetworkType(new RICON_Ext3SatelliteNetwork(), new RICON_Ext3SatelliteCallFactory());
	}
}

//! EXT-3 call-session seam for the satellite fixture. No call state machine is invented (REQ-core §9.3).
class RICON_Ext3SatelliteCallFactory : RICON_CallSessionFactory
{
	override string GetNetworkTypeId()
	{
		return RICON_Ext3SatelliteNetwork.NETWORK_TYPE_ID;
	}

	override bool IsConfigValid()
	{
		return true;
	}

	override RICON_CallSession Create(RICON_RoutingContext context, int sessionId, int talkerEndpointId, int listenerEndpointId)
	{
		if (!context || !context.IsValid() || sessionId <= 0)
			return null;
		return new RICON_Ext3SatelliteCallSession(sessionId);
	}
}

class RICON_Ext3SatelliteCallSession : RICON_CallSession
{
	protected int m_iSessionId;

	void RICON_Ext3SatelliteCallSession(int sessionId)
	{
		m_iSessionId = sessionId;
	}

	override int GetSessionId()
	{
		return m_iSessionId;
	}

	override string GetNetworkTypeId()
	{
		return RICON_Ext3SatelliteNetwork.NETWORK_TYPE_ID;
	}
}

//! EXT-4: a directional antenna is a new AntennaPattern; the evaluator only sees the resulting gain.
//! TEST_ONLY table: within 30 deg of boresight +6 dBi, within 90 deg 0 dBi, otherwise -10 dBi.
class RICON_Ext4DirectionalAntenna : RICON_AntennaPattern
{
	static const string PATTERN_ID = "directional_test";

	protected vector m_vBoresight;

	void RICON_Ext4DirectionalAntenna(vector boresight)
	{
		m_vBoresight = boresight;
	}

	override string GetPatternId()
	{
		return PATTERN_ID;
	}

	override bool IsConfigValid()
	{
		return RICON_Num.IsFiniteVector(m_vBoresight) && m_vBoresight.Length() > 0;
	}

	override bool GetGainDbi(vector directionWorld, float frequencyMHz, out float gainDbi)
	{
		gainDbi = 0;
		if (!IsConfigValid() || !RICON_AntennaPattern.IsInputValid(directionWorld, frequencyMHz))
			return false;

		float cosAngle = vector.Dot(directionWorld.Normalized(), m_vBoresight.Normalized());
		if (cosAngle >= Math.Cos(30.0 * Math.DEG2RAD))
			gainDbi = 6.0;
		else if (cosAngle >= 0)
			gainDbi = 0.0;
		else
			gainDbi = -10.0;
		return true;
	}

	static bool SelfCheck()
	{
		RICON_Ext4DirectionalAntenna antenna = new RICON_Ext4DirectionalAntenna("1 0 0");
		float onAxis;
		float side;
		float back;
		bool ok = RICON_TestSupport.Expect(antenna.GetGainDbi("1 0 0", 150.0, onAxis) && onAxis == 6.0, "EXT-4", "on axis +6 dBi");
		ok = RICON_TestSupport.Expect(antenna.GetGainDbi("0 0 1", 150.0, side) && side == 0.0, "EXT-4", "side 0 dBi") && ok;
		ok = RICON_TestSupport.Expect(antenna.GetGainDbi("-1 0 0", 150.0, back) && back == -10.0, "EXT-4", "back -10 dBi") && ok;
		ok = RICON_TestSupport.Expect(!antenna.GetGainDbi("0 0 0", 150.0, back), "EXT-4", "zero direction rejected") && ok;

		// The evaluator does not know about antennas: the gain only enters the query.
		RICON_WirelessLinkEvaluator evaluator = new RICON_WirelessLinkEvaluator(RICON_TestSupport.BAND_ID,
			new RICON_FreeSpaceModel(), new RICON_NoiseModel(), RICON_TestSupport.NewTestIntelligibility());
		RICON_LinkQuery query = RICON_TestSupport.NewQuery(RICON_FreeSpaceModel.MODEL_ID, 150.0);
		RICON_LinkBudget baseline = evaluator.Evaluate(query);
		query.m_fTxGainDbi = onAxis;
		RICON_LinkBudget withGain = evaluator.Evaluate(query);
		ok = RICON_TestSupport.Expect(baseline.m_bValid && withGain.m_bValid
			&& float.AlmostEqual(withGain.m_fRxPowerDbm - baseline.m_fRxPowerDbm, onAxis, 0.01), "EXT-4", "gain adds to Rx power") && ok;
		return ok;
	}
}

//! EXT-5: a wired link is another LinkEvaluator with the same contract and no radio noise model.
//! TEST_ONLY: cable loss is a fixed 0.1 dB per metre; intelligibility is 1 above sensitivity, else LOST.
class RICON_Ext5WiredEvaluator : RICON_LinkEvaluator
{
	static const string MODEL_ID = "wired_test";
	static const string LAYER_ID = "cable_test";
	static const float LOSS_DB_PER_M = 0.1;

	override RICON_LinkBudget Evaluate(RICON_LinkQuery query)
	{
		RICON_LinkBudget budget = new RICON_LinkBudget();
		if (!query || !RICON_Num.IsFiniteVector(query.m_vTxPos) || !RICON_Num.IsFiniteVector(query.m_vRxPos)
			|| !RICON_Num.IsFinite(query.m_fTxPowerDbm) || !query.m_bHasRxSensitivity || !RICON_Num.IsFinite(query.m_fRxSensitivityDbm))
		{
			budget.MarkInvalid(RICON_EReason.CATALOG_VIOLATION);
			return budget;
		}

		budget.m_fDistanceM = vector.Distance(query.m_vTxPos, query.m_vRxPos);
		float lossDb = LOSS_DB_PER_M * budget.m_fDistanceM;
		budget.m_aLayers.Insert(new RICON_LossLayer(LAYER_ID, lossDb, MODEL_ID));
		budget.m_fTotalLossDb = lossDb;
		budget.m_fRxPowerDbm = query.m_fTxPowerDbm - lossDb;
		budget.m_fSensitivityDbm = query.m_fRxSensitivityDbm;
		// A cable has no radio noise model: the noise fields are explicit finite zeros, not computed values.
		budget.m_fThermalNoiseDbm = 0;
		budget.m_fNoiseDbm = 0;
		budget.m_fNoisePlusInterferenceDbm = 0;
		budget.m_fSinrDb = 0;
		budget.m_bNoiseModelInDomain = true;
		if (budget.m_fRxPowerDbm < budget.m_fSensitivityDbm)
		{
			budget.m_fIntelligibility = 0;
			budget.m_eState = RICON_ELinkState.LOST;
		}
		else
		{
			budget.m_fIntelligibility = 1;
			budget.m_eState = RICON_ELinkState.CLEAN;
		}
		// Same contract as every evaluator: a valid budget carries only finite numbers (REQ-LE-82).
		if (!RICON_Num.IsFinite(budget.m_fDistanceM) || !RICON_Num.IsFinite(lossDb) || !RICON_Num.IsFinite(budget.m_fRxPowerDbm))
		{
			budget.MarkInvalid(RICON_EReason.NUMERIC_FAILURE);
			return budget;
		}
		budget.m_bValid = true;
		budget.m_eReason = RICON_EReason.OK;
		return budget;
	}

	static bool SelfCheck()
	{
		RICON_LinkEvaluator evaluator = new RICON_Ext5WiredEvaluator();
		RICON_LinkQuery query = new RICON_LinkQuery();
		query.m_vTxPos = "0 0 0";
		query.m_vRxPos = "100 0 0";
		query.m_fTxPowerDbm = 0;
		query.m_bHasRxSensitivity = true;
		query.m_fRxSensitivityDbm = -20;

		RICON_LinkBudget nearBudget = evaluator.Evaluate(query);
		bool ok = RICON_TestSupport.Expect(nearBudget.m_bValid && nearBudget.m_eState == RICON_ELinkState.CLEAN, "EXT-5", "100 m cable CLEAN");
		query.m_vRxPos = "500 0 0";
		RICON_LinkBudget farBudget = evaluator.Evaluate(query);
		ok = RICON_TestSupport.Expect(farBudget.m_bValid && farBudget.m_eState == RICON_ELinkState.LOST, "EXT-5", "500 m cable LOST") && ok;
		ok = RICON_TestSupport.Expect(farBudget.FindLayer(LAYER_ID) != null, "EXT-5", "own layer present") && ok;
		return ok;
	}
}
