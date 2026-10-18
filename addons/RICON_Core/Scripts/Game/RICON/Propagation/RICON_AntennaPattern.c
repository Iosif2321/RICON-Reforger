// Antenna pattern seam (REQ-core §7 "Antenna", scenario EXT-4; review finding B6).
// Status: STATIC_IMPLEMENTED (not compiled yet).
//
// The evaluator never asks an antenna for gain: the server resolves the pattern for the current geometry and
// writes the result into RICON_LinkQuery.m_fTxGainDbi / m_fRxGainDbi (REQ-core §4). This keeps the evaluator
// free of antenna types, so a new directional antenna is a new pattern class, not a change of the core.

//! Gain of one antenna towards a direction. Implementations are pure and finite (REQ-CORE-003).
class RICON_AntennaPattern
{
	//! Stable id used by the antenna registry and configs.
	string GetPatternId()
	{
		return string.Empty;
	}

	//! Admission self-check for the registry (REQ-core §6). The base class is never admissible.
	bool IsConfigValid()
	{
		return false;
	}

	//! Gain in dBi towards directionWorld (any length; zero length is invalid) at frequencyMHz.
	//! Returns false and gainDbi = 0 when the input is outside the pattern's domain.
	bool GetGainDbi(vector directionWorld, float frequencyMHz, out float gainDbi)
	{
		gainDbi = 0;
		return false;
	}

	//! Shared input check for implementations: finite non-zero direction and positive finite frequency.
	protected static bool IsInputValid(vector directionWorld, float frequencyMHz)
	{
		if (!RICON_Num.IsFiniteVector(directionWorld) || directionWorld.Length() <= 0)
			return false;
		return RICON_Num.IsFinite(frequencyMHz) && frequencyMHz > 0;
	}
}

//! Isotropic reference antenna: 0 dBi in every direction at every frequency.
class RICON_IsotropicPattern : RICON_AntennaPattern
{
	static const string PATTERN_ID = "isotropic";

	override string GetPatternId()
	{
		return PATTERN_ID;
	}

	override bool IsConfigValid()
	{
		return true;
	}

	override bool GetGainDbi(vector directionWorld, float frequencyMHz, out float gainDbi)
	{
		gainDbi = 0;
		return RICON_AntennaPattern.IsInputValid(directionWorld, frequencyMHz);
	}
}
