// RICON core link data types (ADR-002). Status: STATIC_IMPLEMENTED/COMPILE_ONLY.
// Pure data: no engine calls, so the same semantics are mirrored by oracle/ for vector tests.

//! Link state derived from received power and intelligibility (A3 HDSP semantics, PLAN v5 Э4).
enum RICON_ELinkState
{
	LOST = 0,
	WEAK = 1,
	DEGRADED = 2,
	CLEAN = 3
}

//! Input of one link evaluation (transmitter -> receiver). All units SI unless suffixed.
class RICON_LinkQuery
{
	vector m_vTxPos;
	vector m_vRxPos;
	float m_fFrequencyMHz;
	float m_fBandwidthHz;
	float m_fTxPowerDbm;
	float m_fTxGainDbi;
	float m_fRxGainDbi;
	float m_fRxNoiseFigureDb;
	float m_fRxSensitivityDbm;		//!< NaN-free; <= -900 means "derive from noise floor"
	float m_fInterferenceDbm = -999;	//!< InterferenceModel seam; -999 = none
}

//! Output of one link evaluation, per layer so oracle parity can be checked layer by layer.
class RICON_LinkBudget
{
	float m_fDistanceM;
	float m_fFsplDb;
	float m_fDiffractionDb;
	float m_fObstructionDb;
	float m_fWeatherDb;
	float m_fShadowingDb;
	float m_fMultipathDb;
	float m_fRxPowerDbm;
	float m_fNoiseDbm;
	float m_fSnrDb;
	float m_fIntelligibility;		//!< 0..1
	float m_fFadingM;				//!< Nakagami m for client presentation
	RICON_ELinkState m_eState;
	bool m_bValid;					//!< false if any input was non-finite
}

//! Server-authoritative, owner-scoped description of one talker->listener route (PLAN v5 §3).
class RICON_LinkSnapshot
{
	int m_iSessionEpoch;
	int m_iLinkId;
	int m_iLinkSeq;		//!< monotonic per link; integers only (R-21)
	float m_fTtlS;
	int m_iTalkerPlayerId;
	int m_iListenerPlayerId;
	int m_iRouteNodeCount;	//!< 1 = direct radio, 2+ = via relays
	float m_fQuality;		//!< min(legs) intelligibility, 0..1
	RICON_ELinkState m_eState;

	//! Newer snapshot wins; stale or replayed ones are rejected (lessons R-01, R-06).
	bool IsNewerThan(RICON_LinkSnapshot other)
	{
		if (!other)
			return true;
		if (m_iSessionEpoch != other.m_iSessionEpoch)
			return m_iSessionEpoch > other.m_iSessionEpoch;
		return m_iLinkSeq > other.m_iLinkSeq;
	}
}

//! Shared numeric helpers.
class RICON_Num
{
	//! Enforce Math has no IsNaN/IsInf; NaN != NaN, Inf exceeds any finite bound.
	static bool IsFinite(float v)
	{
		if (v != v)
			return false;
		return v < 1e30 && v > -1e30;
	}
}
