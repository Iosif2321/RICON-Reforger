//! Pure stage 1: incoherent flat-PSD noise over selected disjoint bands. No world, media or access grants.
class RICON_RfNoiseBand
{
	float m_fFrequencyMHz;
	float m_fBandwidthHz;
}

class RICON_RfNoiseEmitter
{
	int m_iEmissionId;
	bool m_bActive;
	vector m_vAntennaPos;
	float m_fTotalPowerDbm;
	float m_fTxGainDbi;
	//! Total power is spread uniformly over the UNION bandwidth, not repeated for every target.
	ref array<ref RICON_RfNoiseBand> m_aBands = {};
}

class RICON_RxEnvironmentLimits
{
	int m_iMaxEmitters;
	int m_iMaxBandsPerEmitter;
}

class RICON_RxEnvironmentInput
{
	string m_sTimebaseId; //!< Optional model-clock label; interval-qualified relay input requires an explicit matching label.
	int m_iSessionEpoch;
	int m_iRevision;
	int m_iObservedAtMs = -1;
	int m_iTtlMs;
	int m_iRxGeneration; //!< Zero keeps the scalar-only profile without a physical registration witness.
	int m_iExcludedEmissionId; //!< 0: none; emission identity, not a player/device id.
	ref RICON_RadioObservation m_Receiver;
	ref RICON_RadioRfProfile m_Profile;
	ref RICON_RadioQueryContext m_Context;
	ref RICON_RxEnvironmentLimits m_Limits;
	ref array<ref RICON_RfNoiseEmitter> m_aEmitters = {};
}

//! Value snapshot for one receiver/frame. It is not a lifecycle export or permission to deliver PCM.
class RICON_RxEnvironment
{
	string m_sTimebaseId;
	bool m_bValid;
	RICON_EReason m_eReason = RICON_EReason.NUMERIC_FAILURE;
	int m_iSessionEpoch, m_iRevision, m_iObservedAtMs = -1, m_iTtlMs, m_iExcludedEmissionId;
	int m_iRxDeviceId, m_iRxTransceiverIndex = -1;
	int m_iRxGeneration;
	string m_sRxProfileId;
	vector m_vRxPos;
	float m_fFrequencyMHz, m_fBandwidthHz, m_fRxGainDbi, m_fRxNoiseFigureDb;
	bool m_bNoiseModelInDomain, m_bHasInterference;
	float m_fNoiseDbm, m_fInterferenceDbm, m_fNoisePlusInterferenceDbm;
	int m_iPowerCalls, m_iContributions;
	ref RICON_RadioQueryContext m_Context;

	bool IsCurrentFor(RICON_RadioObservation rx, RICON_RadioRfProfile profile, RICON_RadioQueryContext context, int epoch, int nowMs)
	{
		if (!m_bValid || m_eReason != RICON_EReason.OK || !rx || !profile || !context || !m_Context) return false;
		if (!RICON_RadioQueryBuilder.IsValidObservation(rx) || !RICON_RadioQueryBuilder.IsValidProfile(profile)
			|| !RICON_RadioQueryBuilder.IsValidContext(context)) return false;
		if (rx.m_iTuningValue != profile.m_iExpectedTuningValue) return false;
		if (m_iSessionEpoch != epoch || m_iRevision <= 0 || m_iRxGeneration < 0 || m_iObservedAtMs < 0 || m_iTtlMs <= 0
			|| nowMs < m_iObservedAtMs || nowMs - m_iObservedAtMs >= m_iTtlMs) return false;
		if (m_iRxDeviceId != rx.m_iDeviceId || m_iRxTransceiverIndex != rx.m_iTransceiverIndex
			|| m_iRxDeviceId != profile.m_iDeviceId || m_iRxTransceiverIndex != profile.m_iTransceiverIndex
			|| m_sRxProfileId != profile.m_sProfileId || m_vRxPos != rx.m_vAntennaPos
			|| m_fFrequencyMHz != profile.m_fFrequencyMHz || m_fBandwidthHz != profile.m_fRxBandwidthHz
			|| m_fRxGainDbi != profile.m_fRxGainDbi || m_fRxNoiseFigureDb != profile.m_fRxNoiseFigureDb) return false;
		if (context.m_bHasInterference || !RICON_RadioQueryBuilder.IsValidContext(m_Context)
			|| m_Context.m_sBandId != context.m_sBandId || m_Context.m_sModelId != context.m_sModelId
			|| m_Context.m_sNoiseEnvironmentId != context.m_sNoiseEnvironmentId
			|| m_Context.m_sIntelligibilityModelId != context.m_sIntelligibilityModelId) return false;
		return m_bNoiseModelInDomain && RICON_Num.IsFinite(m_fNoiseDbm) && RICON_Num.IsFinite(m_fNoisePlusInterferenceDbm)
			&& m_Context.m_bHasInterference == m_bHasInterference
			&& (!m_bHasInterference || (RICON_Num.IsFinite(m_fInterferenceDbm) && m_Context.m_fInterferenceDbm == m_fInterferenceDbm));
	}
}

class RICON_RxEnvironmentService
{
	//! First scalar transfer profile: evaluate a narrow overlap slice, not an entire wide jammer at its centre.
	static const float MAX_TRANSFER_FRACTION = 0.01;
	//! Numerical domain of this float32 MHz profile; not a hardware receiver selectivity limit.
	static RICON_EReason BandReason(float frequencyMHz, float bandwidthHz)
	{
		if (!RICON_Num.IsFinite(frequencyMHz) || frequencyMHz <= 0) return RICON_EReason.INVALID_FREQUENCY;
		if (!RICON_Num.IsFinite(bandwidthHz) || bandwidthHz <= 0 || frequencyMHz - bandwidthHz / 2000000.0 <= 0)
			return RICON_EReason.INVALID_BANDWIDTH;
		if (!RICON_Num.IsFinite(frequencyMHz * 1000000.0) || bandwidthHz < Math.Max(1.0, frequencyMHz * 0.5))
			return RICON_EReason.OUT_OF_MODEL_DOMAIN;
		return RICON_EReason.OK;
	}

	static bool TryOverlapHz(float aMHz, float aHz, float bMHz, float bHz, out float overlapHz, out float centreMHz)
	{
		overlapHz = 0;
		centreMHz = 0;
		// Relative MHz offsets preserve narrow bands better than subtracting large absolute Hz edges.
		float offset = bMHz - aMHz;
		float aHalf = aHz / 2000000.0, bHalf = bHz / 2000000.0;
		float lower = Math.Max(-aHalf, offset - bHalf), upper = Math.Min(aHalf, offset + bHalf);
		float intersection = upper - lower;
		if (!RICON_Num.IsFinite(intersection)) return false;
		if (intersection <= 0) return true;
		overlapHz = Math.Min(intersection * 1000000.0, Math.Min(aHz, bHz));
		centreMHz = aMHz + (lower + upper) * 0.5;
		return RICON_Num.IsFinite(overlapHz) && overlapHz > 0 && RICON_Num.IsFinite(centreMHz) && centreMHz > 0;
	}

	static RICON_RadioQueryContext CopyContext(RICON_RadioQueryContext source)
	{
		RICON_RadioQueryContext result = new RICON_RadioQueryContext();
		result.m_sBandId = source.m_sBandId;
		result.m_sModelId = source.m_sModelId;
		result.m_sNoiseEnvironmentId = source.m_sNoiseEnvironmentId;
		result.m_sIntelligibilityModelId = source.m_sIntelligibilityModelId;
		result.m_bHasInterference = source.m_bHasInterference;
		if (source.m_bHasInterference) result.m_fInterferenceDbm = source.m_fInterferenceDbm;
		return result;
	}

	protected static RICON_EReason Validate(RICON_RxEnvironmentInput input)
	{
		if (!input || !input.m_Receiver || !input.m_Profile || !input.m_Context || !input.m_Limits || !input.m_aEmitters)
			return RICON_EReason.CATALOG_VIOLATION;
		if (input.m_iSessionEpoch <= 0 || input.m_iRevision <= 0 || input.m_iRxGeneration < 0 || input.m_iObservedAtMs < 0 || input.m_iTtlMs <= 0
			|| input.m_iExcludedEmissionId < 0 || input.m_iExcludedEmissionId >= int.MAX) return RICON_EReason.CATALOG_VIOLATION;
		if (input.m_Limits.m_iMaxEmitters < 1 || input.m_Limits.m_iMaxEmitters > 4096
			|| input.m_Limits.m_iMaxBandsPerEmitter < 1 || input.m_Limits.m_iMaxBandsPerEmitter > 64
			|| input.m_aEmitters.Count() > input.m_Limits.m_iMaxEmitters) return RICON_EReason.CATALOG_VIOLATION;
		if (!RICON_RadioQueryBuilder.IsValidObservation(input.m_Receiver) || !RICON_RadioQueryBuilder.IsValidProfile(input.m_Profile)
			|| input.m_Receiver.m_iDeviceId != input.m_Profile.m_iDeviceId
			|| input.m_Receiver.m_iTransceiverIndex != input.m_Profile.m_iTransceiverIndex) return RICON_EReason.CATALOG_VIOLATION;
		if (input.m_Receiver.m_iTuningValue != input.m_Profile.m_iExpectedTuningValue) return RICON_EReason.INVALID_FREQUENCY;
		if (!RICON_RadioQueryBuilder.IsValidContext(input.m_Context) || input.m_Context.m_bHasInterference)
			return RICON_EReason.INVALID_INTERFERENCE;
		RICON_EReason rxBand = BandReason(input.m_Profile.m_fFrequencyMHz, input.m_Profile.m_fRxBandwidthHz);
		if (rxBand != RICON_EReason.OK) return rxBand;
		map<int, bool> ids = new map<int, bool>();
		foreach (RICON_RfNoiseEmitter emitter : input.m_aEmitters)
		{
			if (!emitter || emitter.m_iEmissionId <= 0 || emitter.m_iEmissionId >= int.MAX || ids.Contains(emitter.m_iEmissionId)
				|| !RICON_Num.IsFiniteVector(emitter.m_vAntennaPos) || !RICON_Num.IsFinite(emitter.m_fTotalPowerDbm)
				|| !RICON_Num.IsFinite(emitter.m_fTxGainDbi) || !emitter.m_aBands || emitter.m_aBands.IsEmpty()
				|| emitter.m_aBands.Count() > input.m_Limits.m_iMaxBandsPerEmitter) return RICON_EReason.INVALID_INTERFERENCE;
			ids.Set(emitter.m_iEmissionId, true);
			for (int bandIndex = 0; bandIndex < emitter.m_aBands.Count(); bandIndex++)
			{
				RICON_RfNoiseBand band = emitter.m_aBands[bandIndex];
				if (!band) return RICON_EReason.INVALID_INTERFERENCE;
				RICON_EReason bandReason = BandReason(band.m_fFrequencyMHz, band.m_fBandwidthHz);
				if (bandReason != RICON_EReason.OK) return bandReason;
				for (int earlier = 0; earlier < bandIndex; earlier++)
				{
					RICON_RfNoiseBand previous = emitter.m_aBands[earlier];
					float intersection, intersectionCentre;
					if (!TryOverlapHz(band.m_fFrequencyMHz, band.m_fBandwidthHz, previous.m_fFrequencyMHz, previous.m_fBandwidthHz, intersection, intersectionCentre))
						return RICON_EReason.NUMERIC_FAILURE;
					if (intersection > 0) return RICON_EReason.INVALID_INTERFERENCE;
				}
			}
		}
		return RICON_EReason.OK;
	}

	static RICON_RxEnvironment Evaluate(RICON_RxEnvironmentInput input, RICON_LinkEvaluator evaluator)
	{
		RICON_RxEnvironment result = new RICON_RxEnvironment();
		if (input && input.m_Receiver && input.m_Profile)
		{
			result.m_iRxDeviceId = input.m_Receiver.m_iDeviceId;
			result.m_iRxTransceiverIndex = input.m_Receiver.m_iTransceiverIndex;
		result.m_iRxGeneration = input.m_iRxGeneration;
			result.m_sRxProfileId = input.m_Profile.m_sProfileId;
			result.m_iSessionEpoch = input.m_iSessionEpoch;
			result.m_sTimebaseId = input.m_sTimebaseId;
			result.m_iRevision = input.m_iRevision;
			result.m_iObservedAtMs = input.m_iObservedAtMs;
			result.m_iTtlMs = input.m_iTtlMs;
			result.m_iExcludedEmissionId = input.m_iExcludedEmissionId;
		}
		result.m_eReason = Validate(input);
		if (result.m_eReason != RICON_EReason.OK) return result;
		result.m_vRxPos = input.m_Receiver.m_vAntennaPos;
		result.m_fFrequencyMHz = input.m_Profile.m_fFrequencyMHz;
		result.m_fBandwidthHz = input.m_Profile.m_fRxBandwidthHz;
		result.m_fRxGainDbi = input.m_Profile.m_fRxGainDbi;
		result.m_fRxNoiseFigureDb = input.m_Profile.m_fRxNoiseFigureDb;
		if (!evaluator) { result.m_eReason = RICON_EReason.UNKNOWN_MODEL; return result; }
		RICON_LinkQuery query = new RICON_LinkQuery();
		query.m_vRxPos = input.m_Receiver.m_vAntennaPos;
		query.m_fRxGainDbi = input.m_Profile.m_fRxGainDbi;
		query.m_fRxNoiseFigureDb = input.m_Profile.m_fRxNoiseFigureDb;
		query.m_fFrequencyMHz = input.m_Profile.m_fFrequencyMHz;
		query.m_fBandwidthHz = input.m_Profile.m_fRxBandwidthHz;
		query.m_sBandId = input.m_Context.m_sBandId;
		query.m_sModelId = input.m_Context.m_sModelId;
		query.m_sNoiseEnvironmentId = input.m_Context.m_sNoiseEnvironmentId;
		RICON_LinkBudget noise = new RICON_LinkBudget();
		result.m_eReason = evaluator.FillReceiverNoise(query, noise);
		if (result.m_eReason != RICON_EReason.OK) return result;
		if (!noise.m_bNoiseModelInDomain) { result.m_eReason = RICON_EReason.OUT_OF_MODEL_DOMAIN; return result; }
		array<float> powers = {};
		foreach (RICON_RfNoiseEmitter emission : input.m_aEmitters)
		{
			if (!emission.m_bActive || emission.m_iEmissionId == input.m_iExcludedEmissionId) continue;
			array<float> widths = {};
			foreach (RICON_RfNoiseBand part : emission.m_aBands) widths.Insert(part.m_fBandwidthHz);
			widths.Sort();
			float totalBandwidth = 0;
			foreach (float width : widths) totalBandwidth += width;
			if (!RICON_Num.IsFinite(totalBandwidth) || totalBandwidth <= 0) { result.m_eReason = RICON_EReason.NUMERIC_FAILURE; return result; }
			foreach (RICON_RfNoiseBand target : emission.m_aBands)
			{
				float overlap, overlapCentre;
				if (!TryOverlapHz(input.m_Profile.m_fFrequencyMHz, input.m_Profile.m_fRxBandwidthHz,
					target.m_fFrequencyMHz, target.m_fBandwidthHz, overlap, overlapCentre)) { result.m_eReason = RICON_EReason.NUMERIC_FAILURE; return result; }
				if (overlap <= 0) continue;
				if (overlap > overlapCentre * 1000000.0 * MAX_TRANSFER_FRACTION)
				{ result.m_eReason = RICON_EReason.OUT_OF_MODEL_DOMAIN; return result; }
				query.m_vTxPos = emission.m_vAntennaPos;
				query.m_fTxPowerDbm = emission.m_fTotalPowerDbm;
				query.m_fTxGainDbi = emission.m_fTxGainDbi;
				query.m_fFrequencyMHz = overlapCentre;
				RICON_LinkBudget power = new RICON_LinkBudget();
				result.m_iPowerCalls++;
				result.m_eReason = evaluator.FillReceivedPower(query, power, overlap);
				if (result.m_eReason != RICON_EReason.OK) return result;
				float received = power.m_fRxPowerDbm + 10.0 * (Math.Log10(overlap) - Math.Log10(totalBandwidth));
				if (!RICON_Num.IsFinite(received)) { result.m_eReason = RICON_EReason.NUMERIC_FAILURE; return result; }
				powers.Insert(received);
			}
		}
		powers.Sort();
		float interference;
		bool hasInterference = !powers.IsEmpty();
		if (hasInterference && (!RICON_Num.TryPowerSumDbm(powers, interference) || !RICON_Num.IsFinite(interference)))
		{ result.m_eReason = RICON_EReason.NUMERIC_FAILURE; return result; }
		array<float> combined = {};
		combined.Insert(noise.m_fNoiseDbm);
		if (hasInterference) combined.Insert(interference);
		float total;
		if (!RICON_Num.TryPowerSumDbm(combined, total) || !RICON_Num.IsFinite(total)) { result.m_eReason = RICON_EReason.NUMERIC_FAILURE; return result; }
		result.m_Context = CopyContext(input.m_Context);
		result.m_Context.m_bHasInterference = hasInterference;
		if (hasInterference) result.m_Context.m_fInterferenceDbm = interference; //!< ONLY J; the existing evaluator adds N.
		result.m_fNoiseDbm = noise.m_fNoiseDbm;
		result.m_bNoiseModelInDomain = true;
		result.m_bHasInterference = hasInterference;
		if (hasInterference) result.m_fInterferenceDbm = interference;
		result.m_fNoisePlusInterferenceDbm = total;
		result.m_iContributions = powers.Count();
		result.m_bValid = true;
		result.m_eReason = RICON_EReason.OK;
		return result;
	}
}
