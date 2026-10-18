// Propagation model seam and the free-space model. Written from docs/req/REQ-link-eval.md §4-5 (clean-room).
// Status: STATIC_IMPLEMENTED (not compiled yet).

//! A propagation model appends its own loss layers to the budget (REQ-LE-30) and declares where it applies.
class RICON_PropagationModel
{
	//! Stable id used by the registry and oracle vectors.
	string GetModelId()
	{
		return string.Empty;
	}

	//! Frequency domain of the model (REQ-LE-42). Outside it the evaluator reports OUT_OF_MODEL_DOMAIN.
	bool IsApplicable(float frequencyMHz)
	{
		return false;
	}

	//! Append loss layers for the already validated query. Return false on numeric failure.
	bool AppendLosses(RICON_LinkQuery query, float distanceM, notnull array<ref RICON_LossLayer> layers)
	{
		return false;
	}
}

//! Free-space loss after ITU-R P.525: L = 20 lg(4 pi d / lambda), computed directly (REQ-LE-40).
class RICON_FreeSpaceModel : RICON_PropagationModel
{
	static const string MODEL_ID = "free_space";
	static const string LAYER_ID = "fspl";
	//! Speed of light, exact by SI definition (REQ-LE-02).
	static const float SPEED_OF_LIGHT_MPS = 299792458.0;

	override string GetModelId()
	{
		return MODEL_ID;
	}

	//! The free-space formula has no upper or lower frequency bound (REQ-LE-42).
	override bool IsApplicable(float frequencyMHz)
	{
		return frequencyMHz > 0;
	}

	override bool AppendLosses(RICON_LinkQuery query, float distanceM, notnull array<ref RICON_LossLayer> layers)
	{
		float lossDb = LossDb(distanceM, query.m_fFrequencyMHz);
		if (!RICON_Num.IsFinite(lossDb))
			return false;

		layers.Insert(new RICON_LossLayer(LAYER_ID, lossDb, MODEL_ID));
		return true;
	}

	//! Wavelength in metres for a frequency in MHz.
	static float WavelengthM(float frequencyMHz)
	{
		return SPEED_OF_LIGHT_MPS / (frequencyMHz * 1000000.0);
	}

	//! Inside one wavelength the far-field formula does not hold; the distance is raised to lambda,
	//! which keeps the loss continuous and finite at d = 0 (REQ-LE-41).
	static float LossDb(float distanceM, float frequencyMHz)
	{
		float wavelengthM = WavelengthM(frequencyMHz);
		float effectiveDistanceM = Math.Max(distanceM, wavelengthM);
		return 20.0 * Math.Log10(4.0 * Math.PI * effectiveDistanceM / wavelengthM);
	}
}
