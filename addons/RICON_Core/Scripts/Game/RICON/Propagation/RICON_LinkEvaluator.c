// Link evaluator: model losses -> received power -> noise -> SNR -> intelligibility -> state.
// (ADR-002, PLAN v5 §3 LinkEvaluator). Status: STATIC_IMPLEMENTED/COMPILE_ONLY.

class RICON_LinkEvaluator
{
	//! Thermal noise density at 290 K, dBm/Hz.
	static const float THERMAL_NOISE_DBM_HZ = -174.0;
	//! SNR (dB) at which intelligibility is 0.5 (Hill curve half point, A3 HDSP semantics).
	static const float SNR_HALF_DB = 10.0;
	//! Sensitivity margin over the noise floor when the device does not define one.
	static const float DEFAULT_SENSITIVITY_MARGIN_DB = 8.0;
	static const float CLEAN_INTEL = 0.85;
	static const float DEGRADED_INTEL = 0.4;

	protected RICON_PropagationModel m_Model;

	void RICON_LinkEvaluator(RICON_PropagationModel model)
	{
		m_Model = model;
	}

	//! Returns a budget with m_bValid == false (and state LOST) on any non-finite input or model failure.
	RICON_LinkBudget Evaluate(RICON_LinkQuery query)
	{
		RICON_LinkBudget budget = new RICON_LinkBudget();
		budget.m_eState = RICON_ELinkState.LOST;
		budget.m_bValid = false;

		if (!m_Model || !IsQueryFinite(query) || query.m_fBandwidthHz <= 0)
			return budget;
		if (!m_Model.EvaluateLosses(query, budget))
			return budget;

		float totalLossDb = budget.m_fFsplDb + budget.m_fDiffractionDb + budget.m_fObstructionDb
			+ budget.m_fWeatherDb + budget.m_fShadowingDb + budget.m_fMultipathDb;
		budget.m_fRxPowerDbm = query.m_fTxPowerDbm + query.m_fTxGainDbi + query.m_fRxGainDbi - totalLossDb;

		float thermalDbm = THERMAL_NOISE_DBM_HZ + 10.0 * Math.Log10(query.m_fBandwidthHz) + query.m_fRxNoiseFigureDb;
		budget.m_fNoiseDbm = PowerSumDbm(thermalDbm, query.m_fInterferenceDbm);
		budget.m_fSnrDb = budget.m_fRxPowerDbm - budget.m_fNoiseDbm;
		budget.m_fIntelligibility = IntelligibilityFromSnr(budget.m_fSnrDb);

		float sensitivityDbm = query.m_fRxSensitivityDbm;
		if (sensitivityDbm <= -900)
			sensitivityDbm = thermalDbm + DEFAULT_SENSITIVITY_MARGIN_DB;

		budget.m_eState = StateFor(budget.m_fRxPowerDbm, sensitivityDbm, budget.m_fIntelligibility);
		if (budget.m_eState == RICON_ELinkState.LOST)
			budget.m_fIntelligibility = 0;

		budget.m_bValid = RICON_Num.IsFinite(budget.m_fRxPowerDbm) && RICON_Num.IsFinite(budget.m_fSnrDb);
		if (!budget.m_bValid)
		{
			budget.m_eState = RICON_ELinkState.LOST;
			budget.m_fIntelligibility = 0;
		}
		return budget;
	}

	//! intel = snrLin / (snrLin + halfLin); 0.5 at SNR_HALF_DB. Works in linear power, not dB.
	static float IntelligibilityFromSnr(float snrDb)
	{
		float snrLin = Math.Pow(10, snrDb / 10.0);
		float halfLin = Math.Pow(10, SNR_HALF_DB / 10.0);
		return Math.Clamp(snrLin / (snrLin + halfLin), 0, 1);
	}

	static RICON_ELinkState StateFor(float rxPowerDbm, float sensitivityDbm, float intel)
	{
		if (rxPowerDbm < sensitivityDbm)
			return RICON_ELinkState.LOST;
		if (intel >= CLEAN_INTEL)
			return RICON_ELinkState.CLEAN;
		if (intel >= DEGRADED_INTEL)
			return RICON_ELinkState.DEGRADED;
		return RICON_ELinkState.WEAK;
	}

	//! Sum of two powers given in dBm; values <= -900 are treated as absent.
	static float PowerSumDbm(float aDbm, float bDbm)
	{
		if (bDbm <= -900)
			return aDbm;
		if (aDbm <= -900)
			return bDbm;
		return 10.0 * Math.Log10(Math.Pow(10, aDbm / 10.0) + Math.Pow(10, bDbm / 10.0));
	}

	protected static bool IsQueryFinite(RICON_LinkQuery q)
	{
		if (!q)
			return false;
		return RICON_Num.IsFinite(q.m_fFrequencyMHz) && RICON_Num.IsFinite(q.m_fBandwidthHz)
			&& RICON_Num.IsFinite(q.m_fTxPowerDbm) && RICON_Num.IsFinite(q.m_fTxGainDbi)
			&& RICON_Num.IsFinite(q.m_fRxGainDbi) && RICON_Num.IsFinite(q.m_fRxNoiseFigureDb)
			&& RICON_Num.IsFinite(q.m_fRxSensitivityDbm) && RICON_Num.IsFinite(q.m_fInterferenceDbm);
	}
}
