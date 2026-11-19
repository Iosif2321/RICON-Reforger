//! Build a complete SERVER tick from a detached single-channel projection.
//! The caller qualifies the same static model and model clock for every non-self pair in this scope.
//! The tick borrows the projection unchanged until synchronous Publish returns.
class RICON_StaticLosTick
{
	static string Build(RICON_PairServiceInput input, RICON_StaticLosProfile profile, int modelNowMs, int modelSubUs,
		string timebaseId, int ttlMs, float qualityDelta, out RICON_TemporalProducerTick tick)
	{
		tick = null;
		if (!RICON_TemporalSnapshots.ValidKey(timebaseId) || ttlMs <= 0 || !RICON_Num.IsFinite(qualityDelta)
			|| qualityDelta <= 0 || qualityDelta > 1 || modelNowMs < 0 || modelSubUs < 0 || modelSubUs > 999)
			return "INVALID_TICK";
		RICON_EPairServiceCode structure = RICON_PairService.ValidateInputStructure(input);
		if (structure != RICON_EPairServiceCode.OK) return typename.EnumToString(RICON_EPairServiceCode, structure);
		foreach (RICON_PairListener listenerScope : input.m_aListeners)
		{
			if (listenerScope.m_iSessionEpoch != input.m_iSessionEpoch) return "STALE_LISTENER";
		}

		map<string, ref RICON_PairEndpoint> endpoints = new map<string, ref RICON_PairEndpoint>();
		foreach (RICON_PairEndpoint endpoint : input.m_aEndpoints)
		{
			RICON_RadioObservation observation = endpoint.m_Observation;
			endpoints.Set(EndpointKey(observation.m_iDeviceId, observation.m_iTransceiverIndex), endpoint);
		}
		RICON_TemporalProducerTick staged = new RICON_TemporalProducerTick();
		staged.m_PairInput = input;
		staged.m_iTtlMs = ttlMs;
		staged.m_fQualityDelta = qualityDelta;
		staged.m_sMode = "SERVER";
		staged.m_sTimebaseId = timebaseId;
		foreach (RICON_TransmissionBinding binding : input.m_aBindings)
		{
			if (binding.m_iSessionEpoch != input.m_iSessionEpoch || !binding.IsAuthorityLiveAt(input.m_iNowMs))
				return "STALE_BINDING";
			RICON_PairEndpoint tx = endpoints.Get(EndpointKey(binding.m_iTxDeviceId, binding.m_iTxTransceiverIndex));
			if (!tx) return "ENDPOINT_UNAVAILABLE";
			foreach (RICON_PairListener listener : input.m_aListeners)
			{
				if (RICON_PairService.IsSelfPair(input, binding, listener)) continue;
				RICON_PairEndpoint rx = endpoints.Get(EndpointKey(listener.m_iDeviceId, listener.m_iTransceiverIndex));
				if (!rx) return "ENDPOINT_UNAVAILABLE";
				RICON_TemporalChannelInput channel;
				string code = RICON_StaticLosChannel.Build(tx, rx, profile, modelNowMs, modelSubUs, channel);
				if (code != "OK") return code;
				string key = RICON_TemporalPublicationService.PairKey(binding.m_iLeaseId, listener.m_iDeviceId,
					listener.m_iTransceiverIndex, listener.m_iPlayerId);
				staged.m_ChannelByPair.Set(key, channel);
			}
		}
		tick = staged;
		return "OK";
	}

	protected static string EndpointKey(int deviceId, int index) { return string.Format("%1:%2", deviceId, index); }
}
