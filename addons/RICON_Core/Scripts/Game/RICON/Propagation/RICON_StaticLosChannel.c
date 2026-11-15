//! Trusted applicability for this geometry and interval, including static phase centres and narrowband single-path coverage.
//! A clear trace alone cannot qualify this profile. Distance error includes antenna and observation uncertainty.
class RICON_StaticLosProfile
{
	string m_sProfileId;
	int m_iProfileVersion, m_iEnvironmentRevision;
	int m_iValidFromMs = -1;
	int m_iValidUntilMs = -1;
	float m_fFrequencyMHz, m_fBandwidthHz;
	float m_fMinimumDistanceM, m_fMaximumDistanceM, m_fDistanceErrorM, m_fMaxPhaseErrorTurns;
	bool m_bSinglePathQualified;
}

//! Normalized coefficient of one static path. The scalar link budget owns mean free-space attenuation.
class RICON_StaticLosChannel
{
	protected static const float MAX_CYCLES = 4096.0;
	protected static const float MIN_WAVELENGTH_M = 0.000000000001;
	protected static const float RELATIVE_PHASE_MARGIN = 0.00000095367431640625;
	protected static const float ABSOLUTE_PHASE_MARGIN = 0.000000059604644775390625;

	static string Build(RICON_PairEndpoint tx, RICON_PairEndpoint rx, RICON_StaticLosProfile profile,
		int modelNowMs, int modelSubUs, out RICON_TemporalChannelInput channel)
	{
		channel = null;
		if (!ValidProfile(profile)) return "INVALID_PROFILE";
		if (modelNowMs < profile.m_iValidFromMs || modelNowMs >= profile.m_iValidUntilMs) return "STALE_DESCRIPTOR";
		if (modelSubUs < 0 || modelSubUs > 999) return "INVALID_TIME";
		if (!ValidEndpoint(tx) || !ValidEndpoint(rx)) return "INVALID_ENDPOINT";
		RICON_RadioObservation txObserved = tx.m_Observation;
		RICON_RadioObservation rxObserved = rx.m_Observation;
		if (txObserved.m_iDeviceId == rxObserved.m_iDeviceId && txObserved.m_iTransceiverIndex == rxObserved.m_iTransceiverIndex)
			return "INVALID_ENDPOINT";
		if (tx.m_Profile.m_fFrequencyMHz != profile.m_fFrequencyMHz || rx.m_Profile.m_fFrequencyMHz != profile.m_fFrequencyMHz
			|| rx.m_Profile.m_fRxBandwidthHz != profile.m_fBandwidthHz) return "CONTEXT_MISMATCH";

		float wavelengthM = RICON_FreeSpaceModel.WavelengthM(profile.m_fFrequencyMHz);
		float distanceM = vector.Distance(txObserved.m_vAntennaPos, rxObserved.m_vAntennaPos);
		if (!RICON_Num.IsFinite(wavelengthM) || wavelengthM < MIN_WAVELENGTH_M || !RICON_Num.IsFinite(distanceM))
			return "OUT_OF_MODEL_DOMAIN";
		float nearestM = distanceM - profile.m_fDistanceErrorM;
		float farthestM = distanceM + profile.m_fDistanceErrorM;
		if (nearestM < wavelengthM || nearestM < profile.m_fMinimumDistanceM || farthestM > profile.m_fMaximumDistanceM)
			return "OUT_OF_MODEL_DOMAIN";
		float cycles = distanceM / wavelengthM;
		if (!RICON_Num.IsFinite(cycles) || cycles > MAX_CYCLES) return "OUT_OF_MODEL_DOMAIN";
		// Reserve 16 binary32 unit roundoffs for the norm, wavelength and quotient, plus phase reduction/quantization.
		float phaseError = (profile.m_fDistanceErrorM / wavelengthM) * (1.0 + RELATIVE_PHASE_MARGIN)
			+ RELATIVE_PHASE_MARGIN * cycles + ABSOLUTE_PHASE_MARGIN;
		if (!RICON_Num.IsFinite(phaseError) || phaseError > profile.m_fMaxPhaseErrorTurns) return "PHASE_BUDGET_EXCEEDED";

		RICON_FadingEndpoint txKey = EndpointKey(txObserved);
		RICON_FadingEndpoint rxKey = EndpointKey(rxObserved);
		RICON_FadingDescriptor descriptor = new RICON_FadingDescriptor();
		descriptor.m_sModelId = RICON_FiniteMultipath.MODEL_ID;
		descriptor.m_sProfileId = profile.m_sProfileId;
		descriptor.m_iProfileVersion = profile.m_iProfileVersion;
		if (txKey.m_iDeviceId < rxKey.m_iDeviceId || (txKey.m_iDeviceId == rxKey.m_iDeviceId
			&& txKey.m_iTransceiverIndex < rxKey.m_iTransceiverIndex))
		{
			descriptor.m_A = txKey;
			descriptor.m_B = rxKey;
		}
		else
		{
			descriptor.m_A = rxKey;
			descriptor.m_B = txKey;
		}
		descriptor.m_fFrequencyMHz = profile.m_fFrequencyMHz;
		descriptor.m_fBandwidthHz = profile.m_fBandwidthHz;
		descriptor.m_iEnvironmentRevision = profile.m_iEnvironmentRevision;
		descriptor.m_iAnchorMs = profile.m_iValidFromMs;
		descriptor.m_iValidFromMs = profile.m_iValidFromMs;
		descriptor.m_iValidUntilMs = profile.m_iValidUntilMs;
		descriptor.m_bStaticChannel = true;
		descriptor.m_bCoverageComplete = true;
		RICON_FadingComponent direct = new RICON_FadingComponent();
		direct.m_sComponentId = "DIRECT_LOS";
		direct.m_fPowerWeight = 1.0;
		// With exp(+jωt), propagation delay contributes a negative phase. Wrap after integer quantization.
		int delayPhase = Math.Round((cycles - Math.Floor(cycles)) * RICON_FiniteMultipath.PHASE_M_F);
		direct.m_iPhaseQ = (-delayPhase) & RICON_FiniteMultipath.PHASE_MASK;
		direct.m_iDopplerQ = 0;
		descriptor.m_aComponents.Insert(direct);
		if (!RICON_FiniteMultipath.ValidDescriptor(descriptor)) return "INVALID_PROFILE";

		RICON_FadingQuery query = new RICON_FadingQuery();
		query.m_Tx = EndpointKey(txObserved);
		query.m_Rx = EndpointKey(rxObserved);
		query.m_sProfileId = profile.m_sProfileId;
		query.m_iProfileVersion = profile.m_iProfileVersion;
		query.m_iEnvironmentRevision = profile.m_iEnvironmentRevision;
		query.m_fFrequencyMHz = profile.m_fFrequencyMHz;
		query.m_fBandwidthHz = profile.m_fBandwidthHz;
		query.m_iNowMs = modelNowMs;
		query.m_iSubUs = modelSubUs;
		channel = new RICON_TemporalChannelInput();
		channel.m_Descriptor = descriptor;
		channel.m_Query = query;
		return "OK";
	}

	static bool ValidProfile(RICON_StaticLosProfile profile)
	{
		if (!profile || !profile.m_bSinglePathQualified || !RICON_TemporalSnapshots.ValidKey(profile.m_sProfileId)
			|| profile.m_iProfileVersion < 1 || profile.m_iEnvironmentRevision < 1
			|| profile.m_iValidFromMs < 0 || profile.m_iValidUntilMs <= profile.m_iValidFromMs) return false;
		if (!RICON_Num.IsFinite(profile.m_fFrequencyMHz) || !RICON_Num.IsFinite(profile.m_fBandwidthHz)
			|| profile.m_fFrequencyMHz <= 0 || profile.m_fBandwidthHz <= 0) return false;
		if (!RICON_Num.IsFinite(profile.m_fMinimumDistanceM) || !RICON_Num.IsFinite(profile.m_fMaximumDistanceM)
			|| !RICON_Num.IsFinite(profile.m_fDistanceErrorM) || !RICON_Num.IsFinite(profile.m_fMaxPhaseErrorTurns)) return false;
		return profile.m_fMinimumDistanceM > 0 && profile.m_fMaximumDistanceM >= profile.m_fMinimumDistanceM
			&& profile.m_fDistanceErrorM >= 0 && profile.m_fMaxPhaseErrorTurns > 0 && profile.m_fMaxPhaseErrorTurns < 0.5;
	}

	protected static bool ValidEndpoint(RICON_PairEndpoint endpoint)
	{
		if (!endpoint || !endpoint.m_Observation || !endpoint.m_Profile) return false;
		RICON_RadioObservation observation = endpoint.m_Observation;
		RICON_RadioRfProfile profile = endpoint.m_Profile;
		return RICON_RadioQueryBuilder.IsValidObservation(observation) && RICON_RadioQueryBuilder.IsValidProfile(profile)
			&& observation.m_iDeviceId == profile.m_iDeviceId && observation.m_iTransceiverIndex == profile.m_iTransceiverIndex;
	}

	protected static RICON_FadingEndpoint EndpointKey(RICON_RadioObservation observation)
	{
		RICON_FadingEndpoint key = new RICON_FadingEndpoint();
		key.m_iDeviceId = observation.m_iDeviceId;
		key.m_iTransceiverIndex = observation.m_iTransceiverIndex;
		return key;
	}
}
