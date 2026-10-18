// Propagation model extension point (ADR-002, PLAN v5 §3.1 invariant 2). Status: STATIC_IMPLEMENTED/COMPILE_ONLY.
// Models are registered per band by the server and must keep outputs finite.

//! Base class: a model fills the loss layers it knows; RICON_LinkEvaluator does the rest.
class RICON_PropagationModel
{
	//! Stable id used in configs and oracle vectors, e.g. "free_space".
	string GetModelId()
	{
		return "base";
	}

	//! Fill loss layers of outBudget for the query. Return false if the model cannot evaluate it.
	bool EvaluateLosses(RICON_LinkQuery query, RICON_LinkBudget outBudget)
	{
		return false;
	}
}

//! Free-space reference model (ITU-R P.525 free-space path loss). No terrain, objects or weather:
//! it is layer L0 of HDSP and the first oracle parity target.
class RICON_FreeSpaceModel : RICON_PropagationModel
{
	static const float SPEED_OF_LIGHT_MPS = 299792458.0;

	override string GetModelId()
	{
		return "free_space";
	}

	override bool EvaluateLosses(RICON_LinkQuery query, RICON_LinkBudget outBudget)
	{
		float freqMHz = query.m_fFrequencyMHz;
		if (!RICON_Num.IsFinite(freqMHz) || freqMHz <= 0)
			return false;

		float distanceM = vector.Distance(query.m_vTxPos, query.m_vRxPos);
		outBudget.m_fDistanceM = distanceM;
		outBudget.m_fFsplDb = FreeSpaceLossDb(distanceM, freqMHz);
		outBudget.m_fDiffractionDb = 0;
		outBudget.m_fObstructionDb = 0;
		outBudget.m_fWeatherDb = 0;
		outBudget.m_fShadowingDb = 0;
		outBudget.m_fMultipathDb = 0;
		return true;
	}

	//! FSPL(dB) = 20*log10(d_km) + 20*log10(f_MHz) + 32.44.
	//! Distances below one wavelength are clamped to one wavelength (far-field formula is invalid there).
	static float FreeSpaceLossDb(float distanceM, float freqMHz)
	{
		float wavelengthM = SPEED_OF_LIGHT_MPS / (freqMHz * 1000000.0);
		float d = Math.Max(distanceM, wavelengthM);
		return 20.0 * Math.Log10(d / 1000.0) + 20.0 * Math.Log10(freqMHz) + 32.44;
	}
}
