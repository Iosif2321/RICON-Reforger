// RICON link data types. Written from docs/req/REQ-link-eval.md and docs/req/REQ-core-contracts.md (clean-room).
// Status: STATIC_IMPLEMENTED (not compiled yet).

//! Link state (REQ-LE-72, REQ-LE-75).
enum RICON_ELinkState
{
	LOST = 0,
	WEAK = 1,
	DEGRADED = 2,
	CLEAN = 3
}

//! Why an evaluation is invalid (REQ-core §4, §11; REQ-LE-80). OK only when the budget is valid.
//! Values are stable: never renumber or reuse them (REQ-CORE-004).
enum RICON_EReason
{
	OK = 0,
	INVALID_COORDINATE = 1,
	INVALID_FREQUENCY = 2,
	INVALID_BANDWIDTH = 3,
	INVALID_GAIN = 4,
	INVALID_NOISE_FIGURE = 5,
	INVALID_SENSITIVITY = 6,
	INVALID_REQUIRED_SINR = 7,
	INVALID_INTERFERENCE = 8,
	CATALOG_VIOLATION = 9,
	UNKNOWN_BAND = 10,
	UNKNOWN_MODEL = 11,
	UNKNOWN_NOISE_ENVIRONMENT = 12,
	OUT_OF_MODEL_DOMAIN = 13,
	NUMERIC_FAILURE = 14,
	MODEL_CONTRACT_VIOLATION = 15
}

//! One transmitter -> receiver evaluation request, built by the server only (REQ-core §4).
//! Optional values carry an explicit flag; no numeric sentinels (REQ-LE-16, REQ-LE-18).
class RICON_LinkQuery
{
	vector m_vTxPos;					//!< m, world space (REQ-LE-10)
	vector m_vRxPos;					//!< m, world space
	float m_fFrequencyMHz;				//!< MHz, > 0 (REQ-LE-11)
	float m_fBandwidthHz;				//!< Hz, > 0 (REQ-LE-12)
	float m_fTxPowerDbm;				//!< dBm (REQ-LE-13)
	float m_fTxGainDbi;					//!< dBi (REQ-LE-14)
	float m_fRxGainDbi;					//!< dBi
	float m_fRxNoiseFigureDb;			//!< dB, >= 0 (REQ-LE-15)

	//! Exactly one of the two receive thresholds is set (REQ-LE-16).
	bool m_bHasRxSensitivity;
	float m_fRxSensitivityDbm;			//!< dBm, meaningful only with the flag
	bool m_bHasRequiredSinr;
	float m_fRequiredSinrDb;			//!< dB, meaningful only with the flag

	bool m_bHasInterference;
	float m_fInterferenceDbm;			//!< dBm aggregate from InterferenceModel (REQ-LE-18)

	string m_sBandId;
	string m_sModelId;
	string m_sNoiseEnvironmentId;		//!< REQ-LE-17
	string m_sIntelligibilityModelId;	//!< REQ-LE-73
}

//! One named loss contribution (REQ-LE-30). Models add layers without changing the budget class.
class RICON_LossLayer
{
	string m_sLayerId;
	float m_fLossDb;
	string m_sModelId;

	void RICON_LossLayer(string layerId, float lossDb, string modelId)
	{
		m_sLayerId = layerId;
		m_fLossDb = lossDb;
		m_sModelId = modelId;
	}
}

//! Result of one evaluation (REQ-core §5.2). All numbers are finite, valid or not (REQ-LE-81, REQ-LE-82).
class RICON_LinkBudget
{
	bool m_bValid;
	RICON_EReason m_eReason = RICON_EReason.NUMERIC_FAILURE;
	float m_fDistanceM;
	ref array<ref RICON_LossLayer> m_aLayers = {};
	float m_fTotalLossDb;
	float m_fRxPowerDbm;
	float m_fThermalNoiseDbm;			//!< N0 = kT0B (REQ-LE-60)
	float m_fNoiseDbm;					//!< N, receiver + external (REQ-LE-61)
	float m_fNoisePlusInterferenceDbm;	//!< N+I (REQ-LE-65)
	float m_fSinrDb;					//!< REQ-LE-70
	float m_fSensitivityDbm;			//!< threshold actually used (REQ-LE-71)
	bool m_bNoiseModelInDomain;			//!< REQ-LE-63
	float m_fIntelligibility;			//!< 0..1 (REQ-LE-73)
	RICON_ELinkState m_eState = RICON_ELinkState.LOST;

	//! Returns null when the layer was not computed (REQ-LE-32: absent is not zero).
	RICON_LossLayer FindLayer(string layerId)
	{
		foreach (RICON_LossLayer layer : m_aLayers)
		{
			if (layer.m_sLayerId == layerId)
				return layer;
		}
		return null;
	}

	//! Invalid result: LOST, zero intelligibility, every number finite (REQ-LE-81).
	void MarkInvalid(RICON_EReason reason)
	{
		m_bValid = false;
		m_eReason = reason;
		m_eState = RICON_ELinkState.LOST;
		m_fIntelligibility = 0;
		m_aLayers.Clear();
		m_fDistanceM = 0;
		m_fTotalLossDb = 0;
		m_fRxPowerDbm = 0;
		m_fThermalNoiseDbm = 0;
		m_fNoiseDbm = 0;
		m_fNoisePlusInterferenceDbm = 0;
		m_fSinrDb = 0;
		m_fSensitivityDbm = 0;
		m_bNoiseModelInDomain = false;
	}
}

//! Numeric helpers shared by the core.
class RICON_Num
{
	//! Finite: NaN fails every comparison, +-Inf lies beyond float.MAX (REQ-LE-83).
	static bool IsFinite(float v)
	{
		return v <= float.MAX && v >= -float.MAX;
	}

	static bool IsFiniteVector(vector v)
	{
		return IsFinite(v[0]) && IsFinite(v[1]) && IsFinite(v[2]);
	}

	//! Power sum in dBm without float32 overflow: factor out the largest term (REQ-LE-65, REQ-LE-66).
	//! Returns false for an empty list instead of inventing a value.
	static bool TryPowerSumDbm(notnull array<float> powersDbm, out float sumDbm)
	{
		sumDbm = 0;
		if (powersDbm.IsEmpty())
			return false;

		float peak = powersDbm[0];
		// Distinct loop variable names: Enforce rejects re-declaring a name in one function.
		foreach (float candidate : powersDbm)
		{
			peak = Math.Max(peak, candidate);
		}

		float linearSum = 0;
		foreach (float term : powersDbm)
		{
			linearSum += Math.Pow(10, (term - peak) / 10.0);
		}
		sumDbm = peak + 10.0 * Math.Log10(linearSum);
		return true;
	}
}
