// Intelligibility seam (REQ-LE-73..76). Written from docs/req/REQ-link-eval.md §8 (clean-room).
// Status: STATIC_IMPLEMENTED (not compiled yet). No production curve exists: BLOCKED_CALIBRATION (REQ-LE-74).

//! Maps SINR to intelligibility Q in [0, 1] and Q to a link state. Parameters come only from a versioned
//! config; this base class has no built-in values (REQ-LE-73).
class RICON_IntelligibilityModel
{
	string GetModelId()
	{
		return string.Empty;
	}

	//! Must be non-decreasing in SINR and return a finite value in [0, 1] for every finite input.
	float Intelligibility(float sinrDb)
	{
		return 0;
	}

	//! State for a receiver that is above its threshold (REQ-LE-75). LOST is decided by the evaluator.
	RICON_ELinkState StateFor(float intelligibility)
	{
		return RICON_ELinkState.WEAK;
	}
}

//! TEST_ONLY fixture: Q = clamp((SINR - a) / (b - a), 0, 1). Not a speech model (REQ-LE-74).
//! Overflow-free for any finite SINR because it never exponentiates (REQ-LE-84).
class RICON_LinearTestIntelligibility : RICON_IntelligibilityModel
{
	static const string MODEL_ID = "linear_test";

	protected float m_fLowDb;
	protected float m_fHighDb;
	protected float m_fCleanQ;
	protected float m_fDegradedQ;

	void RICON_LinearTestIntelligibility(float lowDb, float highDb, float cleanQ, float degradedQ)
	{
		m_fLowDb = lowDb;
		m_fHighDb = highDb;
		m_fCleanQ = cleanQ;
		m_fDegradedQ = degradedQ;
	}

	//! Config check of REQ-LE-76: a < b and 0 <= Q_deg <= Q_clean <= 1.
	bool IsConfigValid()
	{
		return m_fLowDb < m_fHighDb && m_fDegradedQ >= 0 && m_fDegradedQ <= m_fCleanQ && m_fCleanQ <= 1;
	}

	override string GetModelId()
	{
		return MODEL_ID;
	}

	override float Intelligibility(float sinrDb)
	{
		return Math.Clamp((sinrDb - m_fLowDb) / (m_fHighDb - m_fLowDb), 0, 1);
	}

	override RICON_ELinkState StateFor(float intelligibility)
	{
		if (intelligibility >= m_fCleanQ)
			return RICON_ELinkState.CLEAN;
		if (intelligibility >= m_fDegradedQ)
			return RICON_ELinkState.DEGRADED;
		return RICON_ELinkState.WEAK;
	}
}
