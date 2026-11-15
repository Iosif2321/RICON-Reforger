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

	//! Config check; the base class has no config and is never valid (REQ-LE-73).
	bool IsConfigValid()
	{
		return false;
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
