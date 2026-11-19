enum RICON_ESharedRxContributionKind
{
	INVALID,
	PCM,
	ENERGY_ONLY,
	MISSING_PCM
}

class RICON_SharedRxContribution
{
	RICON_ESharedRxContributionKind m_eKind;
	ref RICON_MediaEgressRoute m_Route;
	ref RICON_RelayWireGrant m_RelayGrant;
	int m_iFirstSampleIndex;
	float m_fPowerRatio;
	ref array<int> m_aSamples;
	ref RICON_MediaOutputPermit m_Permit;

	bool MatchesReceiver(int epoch, RICON_RelayDestination destination)
	{
		if (m_RelayGrant)
			return !m_Route && m_RelayGrant.IsValid() && m_RelayGrant.m_iEpoch == epoch && destination.Same(m_RelayGrant.m_Destination);
		return m_Route && m_Route.IsValid() && m_Route.m_Source.m_iSessionEpoch == epoch && destination.MatchesRoute(m_Route);
	}

	bool HasCurrentPermit()
	{
		if (m_RelayGrant)
		{
			RICON_RelayOutputPermit relay = RICON_RelayOutputPermit.Cast(m_Permit);
			return relay && relay.MatchesGrant(m_RelayGrant);
		}
		return m_Permit && m_Permit.MatchesRoute(m_Route);
	}

	int TxDeviceId()
	{
		if (m_RelayGrant) return m_RelayGrant.m_iTxDeviceId;
		return m_Route.m_Source.m_iDeviceId;
	}

	int TxIndex()
	{
		if (m_RelayGrant) return m_RelayGrant.m_iTxIndex;
		return m_Route.m_Source.m_iTxTransceiverIndex;
	}

	bool SameEmission(RICON_SharedRxContribution other)
	{
		if (m_RelayGrant) return m_RelayGrant.Same(other.m_RelayGrant);
		return !other.m_RelayGrant && m_Route.Same(other.m_Route);
	}
}

class RICON_SharedRxMixResult
{
	ref RICON_RelayPcmOrigin m_RelayInputOrigin; //!< Optional exact source-window binding of the qualified linear relay profile.
	ref RICON_MediaEgressRoute m_Rx;
	ref RICON_RelayDestination m_Destination;
	int m_iEpoch;
	int m_iFirstSampleIndex;
	int m_iWireDeadlineMs = -1;
	int m_iTargetMs = -1;
	ref array<int> m_aSamples = {};
	ref array<ref RICON_MediaOutputPermit> m_aPermits = {};
	float m_fTotalPowerRatio, m_fPcmPowerRatio, m_fEnergyOnlyPowerRatio, m_fMissingPcmPowerRatio, m_fNoisePowerRatio;
}

//! TEST_ONLY: one aligned 48 kHz PCM16 window with linear powers against a common reference.
class RICON_SharedRxTestMixer
{
	static const int WINDOW_SAMPLES = RICON_PcmResampler8To48.OUTPUT_SAMPLES;
	static const int MAX_INPUTS = 16;

	//! One explicit reference for the synthetic linear mix. Use a single receiver noise value per window.
	static bool TryNormalizeRfBudget(RICON_TemporalRfBudget budget, float referenceDbm, out float signalRatio, out float noiseRatio)
	{
		if (!budget)
		{
			signalRatio = 0.0;
			noiseRatio = 0.0;
			return false;
		}
		return TryNormalizePower(budget.m_fRxPowerDbm, budget.m_fNoiseDbm, referenceDbm, signalRatio, noiseRatio);
	}

	static bool TryNormalizePower(float signalDbm, float noiseDbm, float referenceDbm, out float signalRatio, out float noiseRatio)
	{
		signalRatio = 0.0;
		noiseRatio = 0.0;
		if (!RICON_Num.IsFinite(referenceDbm) || !RICON_Num.IsFinite(signalDbm) || !RICON_Num.IsFinite(noiseDbm)) return false;
		float signalDb = signalDbm - referenceDbm;
		float noiseDb = noiseDbm - referenceDbm;
		// Exclude non-finite differences and exponents outside the positive float32 mix domain.
		if (!(signalDb >= -460.0 && signalDb <= 20.0 && noiseDb >= -460.0 && noiseDb <= 20.0)) return false;
		float signal = Math.Pow(10, signalDb / 10.0);
		float noise = Math.Pow(10, noiseDb / 10.0);
		if (!IsPowerRatio(signal) || !IsPowerRatio(noise) || signal == 0.0 || noise == 0.0) return false;
		signalRatio = signal;
		noiseRatio = noise;
		return true;
	}

	static RICON_SharedRxMixResult Mix(RICON_MediaEgressRoute rx, int firstSampleIndex,
		array<ref RICON_SharedRxContribution> inputs, float noisePowerRatio, array<int> noiseSamples)
	{
		if (!rx || !rx.IsValid()) return null;
		return MixReceiver(rx.m_Source.m_iSessionEpoch, RICON_RelayDestination.FromRoute(rx), firstSampleIndex, inputs, noisePowerRatio, noiseSamples, rx);
	}

	static RICON_SharedRxMixResult MixReceiver(int epoch, RICON_RelayDestination destination, int firstSampleIndex,
		array<ref RICON_SharedRxContribution> inputs, float noisePowerRatio, array<int> noiseSamples, RICON_MediaEgressRoute rx = null)
	{
		if (!destination || !destination.IsValid() || epoch <= 0 || epoch >= int.MAX
			|| firstSampleIndex < 0 || firstSampleIndex > int.MAX - WINDOW_SAMPLES
			|| !inputs || inputs.Count() > MAX_INPUTS || !IsPowerRatio(noisePowerRatio)) return null;
		if (noiseSamples && !IsPcmWindow(noiseSamples)) return null;
		if (!noiseSamples && noisePowerRatio != 0.0) return null;

		array<ref RICON_SharedRxContribution> ordered = {};
		foreach (RICON_SharedRxContribution input : inputs)
		{
			if (!input || !input.MatchesReceiver(epoch, destination)
				|| input.m_iFirstSampleIndex != firstSampleIndex || !IsPowerRatio(input.m_fPowerRatio)) return null;
			if (input.m_eKind == RICON_ESharedRxContributionKind.PCM)
			{
				if (!input.HasCurrentPermit() || !IsPcmWindow(input.m_aSamples)) return null;
			}
			else if ((input.m_eKind != RICON_ESharedRxContributionKind.ENERGY_ONLY
				&& input.m_eKind != RICON_ESharedRxContributionKind.MISSING_PCM) || input.m_aSamples || input.m_Permit) return null;

			int deviceId = input.TxDeviceId();
			int txIndex = input.TxIndex();
			int position = ordered.Count();
			for (int index = 0; index < ordered.Count(); ++index)
			{
				int otherDevice = ordered[index].TxDeviceId();
				int otherIndex = ordered[index].TxIndex();
				if (deviceId == otherDevice && txIndex == otherIndex) return null;
				if (deviceId < otherDevice || (deviceId == otherDevice && txIndex < otherIndex))
				{
					position = index;
					break;
				}
			}
			if (position == ordered.Count()) ordered.Insert(input);
			else ordered.InsertAt(input, position);
		}

		RICON_SharedRxMixResult result = new RICON_SharedRxMixResult();
		if (rx) result.m_Rx = rx.CloneRoute();
		result.m_Destination = destination.CloneDestination();
		result.m_iEpoch = epoch;
		result.m_iFirstSampleIndex = firstSampleIndex;
		result.m_fNoisePowerRatio = noisePowerRatio;
		array<float> total = {};
		total.Resize(WINDOW_SAMPLES);
		foreach (RICON_SharedRxContribution input : ordered)
		{
			if (input.m_eKind == RICON_ESharedRxContributionKind.ENERGY_ONLY)
				result.m_fEnergyOnlyPowerRatio += input.m_fPowerRatio;
			else if (input.m_eKind == RICON_ESharedRxContributionKind.MISSING_PCM)
				result.m_fMissingPcmPowerRatio += input.m_fPowerRatio;
			else
			{
				result.m_fPcmPowerRatio += input.m_fPowerRatio;
				if (!result.m_aPermits.Contains(input.m_Permit)) result.m_aPermits.Insert(input.m_Permit);
				float gain = Math.Sqrt(input.m_fPowerRatio);
				for (int index = 0; index < WINDOW_SAMPLES; ++index) total[index] = total[index] + input.m_aSamples[index] * gain;
			}
		}
		result.m_fTotalPowerRatio = result.m_fPcmPowerRatio + result.m_fEnergyOnlyPowerRatio
			+ result.m_fMissingPcmPowerRatio + noisePowerRatio;

		float noiseGain = Math.Sqrt(noisePowerRatio);
		for (int index = 0; index < WINDOW_SAMPLES; ++index)
		{
			float sample = total[index];
			if (noiseSamples) sample += noiseSamples[index] * noiseGain;
			result.m_aSamples.Insert(Math.Round(Math.Clamp(sample, -32768.0, 32767.0)));
		}
		return result;
	}

	static bool SameRx(RICON_MediaEgressRoute a, RICON_MediaEgressRoute b)
	{
		if (a.m_Source.m_iSessionEpoch != b.m_Source.m_iSessionEpoch) return false;
		if (a.m_iListenerPlayerId != b.m_iListenerPlayerId || a.m_iListenerConnectionId != b.m_iListenerConnectionId
			|| a.m_iListenerGeneration != b.m_iListenerGeneration || a.m_iListenerEntityId != b.m_iListenerEntityId) return false;
		return a.m_iRxDeviceId == b.m_iRxDeviceId && a.m_iRxTransceiverIndex == b.m_iRxTransceiverIndex
			&& a.m_iRxGeneration == b.m_iRxGeneration;
	}

	static bool IsPowerRatio(float ratio)
	{
		return RICON_Num.IsFinite(ratio) && ratio >= 0.0 && ratio <= 64.0;
	}

	static bool IsPcmWindow(array<int> samples)
	{
		if (!samples || samples.Count() != WINDOW_SAMPLES) return false;
		foreach (int sample : samples)
		{
			if (sample < -32768 || sample > 32767) return false;
		}
		return true;
	}
}
