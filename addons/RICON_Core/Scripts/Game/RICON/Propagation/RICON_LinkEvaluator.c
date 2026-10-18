// Link evaluator contract and the wireless implementation. Written from docs/req/REQ-link-eval.md and
// docs/req/REQ-core-contracts.md §3, §6 (clean-room). Status: STATIC_IMPLEMENTED (not compiled yet).

//! Common contract: query -> budget, with no knowledge of the network type (REQ-core §6, scenario EXT-5).
class RICON_LinkEvaluator
{
	//! Never returns null; an invalid query yields an invalid, fully finite budget.
	RICON_LinkBudget Evaluate(RICON_LinkQuery query)
	{
		RICON_LinkBudget budget = new RICON_LinkBudget();
		budget.MarkInvalid(RICON_EReason.UNKNOWN_MODEL);
		return budget;
	}
}

//! Radio link evaluation: losses -> received power -> noise (+ interference) -> SINR -> threshold -> Q -> state.
//! A pure evaluator: the authority manager resolves the models from its registries and passes them in; the
//! evaluator holds them strongly (REQ-core §3, finding API-01) and owns no registry (REQ-core §7).
class RICON_WirelessLinkEvaluator : RICON_LinkEvaluator
{
	//! Boltzmann constant k = BOLTZMANN_MANTISSA * 10^-23 J/K, exact by SI definition (REQ-LE-03).
	//! Kept as mantissa + power of ten: exponent literals are not confirmed by the offline Enforce docs.
	static const float BOLTZMANN_MANTISSA = 1.380649;
	static const float BOLTZMANN_EXPONENT = -23;
	//! Reference noise temperature T0 (REQ-LE-03).
	static const float REFERENCE_TEMPERATURE_K = 290.0;

	protected string m_sBandId;
	protected ref RICON_PropagationModel m_Propagation;
	protected ref RICON_NoiseModel m_Noise;
	protected ref RICON_IntelligibilityModel m_Intelligibility;

	//! Built by the manager for one band after it resolved band -> models in its registries.
	//! An empty band id or a null dependency makes every evaluation fail closed.
	void RICON_WirelessLinkEvaluator(string bandId, RICON_PropagationModel propagation, RICON_NoiseModel noise, RICON_IntelligibilityModel intelligibility)
	{
		m_sBandId = bandId;
		m_Propagation = propagation;
		m_Noise = noise;
		m_Intelligibility = intelligibility;
	}

	override RICON_LinkBudget Evaluate(RICON_LinkQuery query)
	{
		RICON_LinkBudget budget = new RICON_LinkBudget();
		if (!query)
		{
			budget.MarkInvalid(RICON_EReason.CATALOG_VIOLATION);
			return budget;
		}

		RICON_EReason inputReason = ValidateQuery(query);
		if (inputReason != RICON_EReason.OK)
		{
			budget.MarkInvalid(inputReason);
			return budget;
		}

		// The query must name exactly the models this evaluator was built with; a mismatch fails closed instead of
		// silently using another model (REQ-core §6). Order continues REQ-LE-80: bandId -> model -> noise -> Q model.
		if (m_sBandId.IsEmpty() || query.m_sBandId != m_sBandId)
		{
			budget.MarkInvalid(RICON_EReason.UNKNOWN_BAND);
			return budget;
		}
		if (!m_Propagation || query.m_sModelId != m_Propagation.GetModelId())
		{
			budget.MarkInvalid(RICON_EReason.UNKNOWN_MODEL);
			return budget;
		}
		if (!m_Noise || query.m_sNoiseEnvironmentId != m_Noise.GetEnvironmentId())
		{
			budget.MarkInvalid(RICON_EReason.UNKNOWN_NOISE_ENVIRONMENT);
			return budget;
		}
		if (!m_Intelligibility || query.m_sIntelligibilityModelId != m_Intelligibility.GetModelId())
		{
			budget.MarkInvalid(RICON_EReason.UNKNOWN_MODEL);
			return budget;
		}
		RICON_PropagationModel propagation = m_Propagation;
		RICON_NoiseModel noise = m_Noise;
		RICON_IntelligibilityModel intelligibility = m_Intelligibility;
		if (!intelligibility.IsConfigValid())
		{
			budget.MarkInvalid(RICON_EReason.CATALOG_VIOLATION);
			return budget;
		}
		if (!propagation.IsApplicable(query.m_fFrequencyMHz))
		{
			budget.MarkInvalid(RICON_EReason.OUT_OF_MODEL_DOMAIN);
			return budget;
		}

		// Losses (REQ-LE-30, REQ-LE-31).
		budget.m_fDistanceM = vector.Distance(query.m_vTxPos, query.m_vRxPos);
		if (!propagation.AppendLosses(query, budget.m_fDistanceM, budget.m_aLayers))
		{
			budget.MarkInvalid(RICON_EReason.NUMERIC_FAILURE);
			return budget;
		}
		if (!AreLayersValid(budget.m_aLayers, propagation.GetModelId()))
		{
			budget.MarkInvalid(RICON_EReason.MODEL_CONTRACT_VIOLATION);
			return budget;
		}
		budget.m_fTotalLossDb = 0;
		foreach (RICON_LossLayer layer : budget.m_aLayers)
		{
			budget.m_fTotalLossDb += layer.m_fLossDb;
		}

		// Received power (REQ-LE-50).
		budget.m_fRxPowerDbm = query.m_fTxPowerDbm + query.m_fTxGainDbi + query.m_fRxGainDbi - budget.m_fTotalLossDb;

		// Noise: N0 = kT0B, N = N0 + 10 lg(f_a + f_r - 1) (REQ-LE-60, REQ-LE-61).
		budget.m_fThermalNoiseDbm = ThermalNoiseDbm(query.m_fBandwidthHz);
		RICON_NoiseEstimate estimate = new RICON_NoiseEstimate();
		noise.Estimate(query.m_fFrequencyMHz, estimate);
		budget.m_bNoiseModelInDomain = estimate.m_bInDomain;
		float receiverFactor = Math.Pow(10, query.m_fRxNoiseFigureDb / 10.0);
		budget.m_fNoiseDbm = budget.m_fThermalNoiseDbm + 10.0 * Math.Log10(estimate.m_fFactorLinear + receiverFactor - 1);

		// Noise plus interference and SINR (REQ-LE-65, REQ-LE-70).
		budget.m_fNoisePlusInterferenceDbm = budget.m_fNoiseDbm;
		if (query.m_bHasInterference)
		{
			array<float> powers = {};
			powers.Insert(budget.m_fNoiseDbm);
			powers.Insert(query.m_fInterferenceDbm);
			float sumDbm;
			RICON_Num.TryPowerSumDbm(powers, sumDbm); // two terms: never empty
			budget.m_fNoisePlusInterferenceDbm = sumDbm;
		}
		budget.m_fSinrDb = budget.m_fRxPowerDbm - budget.m_fNoisePlusInterferenceDbm;

		// Receive threshold: explicit sensitivity, or the receiver's own noise plus required SINR (REQ-LE-71).
		if (query.m_bHasRxSensitivity)
			budget.m_fSensitivityDbm = query.m_fRxSensitivityDbm;
		else
			budget.m_fSensitivityDbm = budget.m_fThermalNoiseDbm + query.m_fRxNoiseFigureDb + query.m_fRequiredSinrDb;

		// State (REQ-LE-72, REQ-LE-75).
		if (budget.m_fRxPowerDbm < budget.m_fSensitivityDbm)
		{
			budget.m_fIntelligibility = 0;
			budget.m_eState = RICON_ELinkState.LOST;
		}
		else
		{
			budget.m_fIntelligibility = intelligibility.Intelligibility(budget.m_fSinrDb);
			if (!(budget.m_fIntelligibility >= 0 && budget.m_fIntelligibility <= 1))
			{
				budget.MarkInvalid(RICON_EReason.MODEL_CONTRACT_VIOLATION);
				return budget;
			}
			budget.m_eState = intelligibility.StateFor(budget.m_fIntelligibility);
			// Above the threshold only WEAK / DEGRADED / CLEAN are allowed; LOST is decided here (REQ-LE-75).
			if (budget.m_eState != RICON_ELinkState.WEAK && budget.m_eState != RICON_ELinkState.DEGRADED
				&& budget.m_eState != RICON_ELinkState.CLEAN)
			{
				budget.MarkInvalid(RICON_EReason.MODEL_CONTRACT_VIOLATION);
				return budget;
			}
		}

		// Every output finite or the whole budget is invalid (REQ-LE-82).
		if (!IsBudgetFinite(budget))
		{
			budget.MarkInvalid(RICON_EReason.NUMERIC_FAILURE);
			return budget;
		}
		budget.m_bValid = true;
		budget.m_eReason = RICON_EReason.OK;
		return budget;
	}

	//! N0 = 10 lg(k T0 B * 1000) dBm (REQ-LE-60), summed in dB so no tiny intermediate value is formed.
	static float ThermalNoiseDbm(float bandwidthHz)
	{
		float kDbWattPerHzK = 10.0 * Math.Log10(BOLTZMANN_MANTISSA) + 10.0 * BOLTZMANN_EXPONENT;
		return kDbWattPerHzK + 10.0 * Math.Log10(REFERENCE_TEMPERATURE_K) + 10.0 * Math.Log10(bandwidthHz) + 30.0;
	}

	//! Domain checks of REQ-LE-10..18 in the fixed order of REQ-LE-80; the first failing boundary is reported.
	protected static RICON_EReason ValidateQuery(RICON_LinkQuery q)
	{
		if (!RICON_Num.IsFiniteVector(q.m_vTxPos) || !RICON_Num.IsFiniteVector(q.m_vRxPos))
			return RICON_EReason.INVALID_COORDINATE;
		if (!RICON_Num.IsFinite(q.m_fFrequencyMHz) || q.m_fFrequencyMHz <= 0)
			return RICON_EReason.INVALID_FREQUENCY;
		if (!RICON_Num.IsFinite(q.m_fBandwidthHz) || q.m_fBandwidthHz <= 0)
			return RICON_EReason.INVALID_BANDWIDTH;
		if (!RICON_Num.IsFinite(q.m_fTxPowerDbm))
			return RICON_EReason.CATALOG_VIOLATION;
		if (!RICON_Num.IsFinite(q.m_fTxGainDbi) || !RICON_Num.IsFinite(q.m_fRxGainDbi))
			return RICON_EReason.INVALID_GAIN;
		if (!RICON_Num.IsFinite(q.m_fRxNoiseFigureDb) || q.m_fRxNoiseFigureDb < 0)
			return RICON_EReason.INVALID_NOISE_FIGURE;
		if (q.m_bHasRxSensitivity == q.m_bHasRequiredSinr)
			return RICON_EReason.CATALOG_VIOLATION;
		if (q.m_bHasRxSensitivity && !RICON_Num.IsFinite(q.m_fRxSensitivityDbm))
			return RICON_EReason.INVALID_SENSITIVITY;
		if (q.m_bHasRequiredSinr && !RICON_Num.IsFinite(q.m_fRequiredSinrDb))
			return RICON_EReason.INVALID_REQUIRED_SINR;
		if (q.m_bHasInterference && !RICON_Num.IsFinite(q.m_fInterferenceDbm))
			return RICON_EReason.INVALID_INTERFERENCE;
		return RICON_EReason.OK;
	}

	//! Contract of a propagation model's layers (REQ-LE-31): not empty, no null entry, non-empty unique ids,
	//! produced by this model, finite values.
	protected static bool AreLayersValid(notnull array<ref RICON_LossLayer> layers, string modelId)
	{
		if (layers.IsEmpty())
			return false;

		for (int i = 0; i < layers.Count(); i++)
		{
			RICON_LossLayer current = layers[i];
			if (!current || current.m_sLayerId.IsEmpty() || current.m_sModelId != modelId || !RICON_Num.IsFinite(current.m_fLossDb))
				return false;

			for (int j = 0; j < i; j++)
			{
				if (layers[j].m_sLayerId == current.m_sLayerId)
					return false;
			}
		}
		return true;
	}

	protected static bool IsBudgetFinite(RICON_LinkBudget b)
	{
		foreach (RICON_LossLayer layer : b.m_aLayers)
		{
			if (!RICON_Num.IsFinite(layer.m_fLossDb))
				return false;
		}
		return RICON_Num.IsFinite(b.m_fDistanceM) && RICON_Num.IsFinite(b.m_fTotalLossDb)
			&& RICON_Num.IsFinite(b.m_fRxPowerDbm) && RICON_Num.IsFinite(b.m_fThermalNoiseDbm)
			&& RICON_Num.IsFinite(b.m_fNoiseDbm) && RICON_Num.IsFinite(b.m_fNoisePlusInterferenceDbm)
			&& RICON_Num.IsFinite(b.m_fSinrDb) && RICON_Num.IsFinite(b.m_fSensitivityDbm)
			&& RICON_Num.IsFinite(b.m_fIntelligibility);
	}
}
