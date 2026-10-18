// TEST_ONLY helpers for the extension fixtures of REQ-core §10. Not part of RICON_Core, not for Workshop.
// Status: COMPILE_FIXTURE / SELF_CHECK_NOT_RUN.

//! Builds queries and fixture models with explicit, documented test values (no physics is claimed).
class RICON_TestSupport
{
	static const string BAND_ID = "test_band";
	static const string NOISE_ID = "none";

	//! Linear intelligibility fixture of REQ-LE-74 (TEST_ONLY).
	static RICON_LinearTestIntelligibility NewTestIntelligibility()
	{
		return new RICON_LinearTestIntelligibility(-15.0, 15.0, 0.8, 0.5);
	}

	//! A valid query 1 km apart at the given frequency; threshold from required SINR, no interference.
	static RICON_LinkQuery NewQuery(string modelId, float frequencyMHz)
	{
		RICON_LinkQuery query = new RICON_LinkQuery();
		query.m_vTxPos = "0 0 0";
		query.m_vRxPos = "1000 0 0";
		query.m_fFrequencyMHz = frequencyMHz;
		query.m_fBandwidthHz = 12500.0;
		query.m_fTxPowerDbm = 37.0;
		query.m_fTxGainDbi = 0;
		query.m_fRxGainDbi = 0;
		query.m_fRxNoiseFigureDb = 8.0;
		query.m_bHasRequiredSinr = true;
		query.m_fRequiredSinrDb = 10.0;
		query.m_sBandId = BAND_ID;
		query.m_sModelId = modelId;
		query.m_sNoiseEnvironmentId = NOISE_ID;
		query.m_sIntelligibilityModelId = RICON_LinearTestIntelligibility.MODEL_ID;
		return query;
	}

	//! A valid budget for route fixtures (TEST_ONLY numbers).
	static RICON_LinkBudget NewValidBudget(float intelligibility, RICON_ELinkState state)
	{
		RICON_LinkBudget budget = new RICON_LinkBudget();
		budget.m_bValid = true;
		budget.m_eReason = RICON_EReason.OK;
		budget.m_fIntelligibility = intelligibility;
		budget.m_eState = state;
		return budget;
	}

	static bool Expect(bool condition, string fixture, string what)
	{
		if (!condition)
			PrintFormat("[RICON_TEST] FAIL %1: %2", fixture, what);
		return condition;
	}
}

//! TEST_ONLY access policy that allows every endpoint, so route fixtures can build a valid RoutingContext.
class RICON_TestAllowAllAccessPolicy : RICON_AccessPolicy
{
	override bool CanUseEndpoint(int playerId, int endpointId)
	{
		return true;
	}
}

//! Runs every fixture. Returns true only if all self-checks pass. Nothing calls this automatically.
class RICON_ExtFixtures
{
	static bool RunAll()
	{
		bool ok = RICON_Ext1NewVhfRadio.SelfCheck();
		ok = RICON_Ext2SkyWaveModel.SelfCheck() && ok;
		ok = RICON_Ext3SatelliteNetwork.SelfCheck() && ok;
		ok = RICON_Ext4DirectionalAntenna.SelfCheck() && ok;
		ok = RICON_Ext5WiredEvaluator.SelfCheck() && ok;
		PrintFormat("[RICON_TEST] extension fixtures: %1", ok);
		return ok;
	}
}
