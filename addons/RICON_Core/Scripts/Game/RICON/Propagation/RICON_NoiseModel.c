// External radio noise seam (REQ-LE-61..64). Written from docs/req/REQ-link-eval.md §7 (clean-room).
// Status: STATIC_IMPLEMENTED (not compiled yet).

//! Answer of a noise model: external noise factor f_a (linear, relative to kT0b) and whether
//! the model covers the frequency. Out of domain the factor is 1, never 0 (REQ-LE-63).
class RICON_NoiseEstimate
{
	float m_fFactorLinear = 1;
	bool m_bInDomain;
}

//! Base: no external noise above the kT0 reference (f_a = 1), valid at every frequency (REQ-LE-64).
class RICON_NoiseModel
{
	string GetEnvironmentId()
	{
		return "none";
	}

	void Estimate(float frequencyMHz, notnull RICON_NoiseEstimate outEstimate)
	{
		outEstimate.m_fFactorLinear = 1;
		outEstimate.m_bInDomain = true;
	}
}

//! Median man-made noise F_am = c - d lg f (dB above kT0b) for one environment category (REQ-LE-62).
//! Valid 0.3..250 MHz, no extrapolation (REQ-LE-63). The c/d constants are not hardcoded here: they come from
//! the versioned environment config of the authority manager (REQ-core §7), because the P.372 revision is
//! still unconfirmed (only a secondary source).
class RICON_ManMadeNoiseModel : RICON_NoiseModel
{
	static const float MIN_FREQUENCY_MHZ = 0.3;
	static const float MAX_FREQUENCY_MHZ = 250.0;

	protected string m_sEnvironmentId;
	protected float m_fC;
	protected float m_fD;

	void RICON_ManMadeNoiseModel(string environmentId, float c, float d)
	{
		m_sEnvironmentId = environmentId;
		m_fC = c;
		m_fD = d;
	}

	override string GetEnvironmentId()
	{
		return m_sEnvironmentId;
	}

	override void Estimate(float frequencyMHz, notnull RICON_NoiseEstimate outEstimate)
	{
		if (frequencyMHz < MIN_FREQUENCY_MHZ || frequencyMHz > MAX_FREQUENCY_MHZ)
		{
			outEstimate.m_fFactorLinear = 1;
			outEstimate.m_bInDomain = false;
			return;
		}

		float faDb = m_fC - m_fD * Math.Log10(frequencyMHz);
		outEstimate.m_fFactorLinear = Math.Pow(10, faDb / 10.0);
		outEstimate.m_bInDomain = true;
	}
}
