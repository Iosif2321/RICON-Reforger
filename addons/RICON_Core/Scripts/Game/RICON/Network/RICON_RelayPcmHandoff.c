//! Fixed path identity. The direct upstream route identifies the RX proof for the test producer.
class RICON_RelayPcmPath
{
	ref RICON_MediaEgressRoute m_UpstreamRoute;
	int m_iRelayEndpointId;
	int m_iRelayTxDeviceId, m_iRelayTxIndex = -1, m_iRelayTxGeneration;
	int m_iDestinationEndpointId, m_iDestinationGeneration;
	int m_iBridgeGeneration, m_iRelayEmissionId, m_iTransformGeneration;
	int m_iProcessingDelayMs;

	bool IsValid()
	{
		if (!m_UpstreamRoute || !m_UpstreamRoute.IsValid()) return false;
		int origin = m_UpstreamRoute.m_Source.m_iDeviceId;
		if (m_iRelayEndpointId <= 0 || m_iDestinationEndpointId <= 0 || m_iRelayEndpointId == origin
			|| m_iDestinationEndpointId == origin || m_iDestinationEndpointId == m_iRelayEndpointId) return false;
		if (m_iRelayTxDeviceId <= 0 || m_iRelayTxDeviceId == origin || m_iRelayTxIndex < 0 || m_iRelayTxGeneration <= 0) return false;
		if (m_iRelayTxDeviceId == m_UpstreamRoute.m_iRxDeviceId && m_iRelayTxIndex == m_UpstreamRoute.m_iRxTransceiverIndex)
			return false;
		return m_iDestinationGeneration > 0 && m_iBridgeGeneration > 0 && m_iRelayEmissionId > 0
			&& m_iTransformGeneration > 0 && m_iProcessingDelayMs >= 0;
	}

	bool SamePath(RICON_RelayPcmPath other)
	{
		return other && m_UpstreamRoute && m_UpstreamRoute.Same(other.m_UpstreamRoute)
			&& m_iRelayEndpointId == other.m_iRelayEndpointId && m_iRelayTxDeviceId == other.m_iRelayTxDeviceId
			&& m_iRelayTxIndex == other.m_iRelayTxIndex && m_iRelayTxGeneration == other.m_iRelayTxGeneration
			&& m_iDestinationEndpointId == other.m_iDestinationEndpointId && m_iDestinationGeneration == other.m_iDestinationGeneration
			&& m_iBridgeGeneration == other.m_iBridgeGeneration && m_iRelayEmissionId == other.m_iRelayEmissionId
			&& m_iTransformGeneration == other.m_iTransformGeneration && m_iProcessingDelayMs == other.m_iProcessingDelayMs;
	}

	RICON_RelayPcmPath ClonePath()
	{
		RICON_RelayPcmPath copy = new RICON_RelayPcmPath();
		if (m_UpstreamRoute) copy.m_UpstreamRoute = m_UpstreamRoute.CloneRoute();
		copy.m_iRelayEndpointId = m_iRelayEndpointId;
		copy.m_iRelayTxDeviceId = m_iRelayTxDeviceId;
		copy.m_iRelayTxIndex = m_iRelayTxIndex;
		copy.m_iRelayTxGeneration = m_iRelayTxGeneration;
		copy.m_iDestinationEndpointId = m_iDestinationEndpointId;
		copy.m_iDestinationGeneration = m_iDestinationGeneration;
		copy.m_iBridgeGeneration = m_iBridgeGeneration;
		copy.m_iRelayEmissionId = m_iRelayEmissionId;
		copy.m_iTransformGeneration = m_iTransformGeneration;
		copy.m_iProcessingDelayMs = m_iProcessingDelayMs;
		return copy;
	}
}

//! Server-owned policy must check exact current route, bridge and physical generations on each call.
class RICON_RelayPcmPolicy
{
	bool CanUseInput(RICON_RelayPcmPath path, int nowMs) { return false; }
	bool CanUseOutput(RICON_RelayPcmPath path, int nowMs) { return false; }
}

//! Original capture sample units. The issuer supplies provenance and the earliest input deadline on one model clock.
class RICON_RelayPcmOrigin
{
	int m_iSequence, m_iFirstSampleIndex, m_iSampleCount;
	int m_iCaptureFromMs, m_iCaptureUntilMs, m_iRxReadyMs, m_iDeadlineMs;
	bool m_bContinuousInput;
	ref RICON_MediaCaptureEnvelope m_CaptureEnvelope;
	ref RICON_MediaCaptureAnchor m_TrustedCaptureAnchor;
	ref RICON_RelayPcmOrigin m_PreviousInput;

	bool IsValid(int nowMs)
	{
		if (!ValidWindow(nowMs)) return false;
		if (m_bContinuousInput && !ValidSeriesWindow()) return false;
		if (!m_PreviousInput) return true;
		return m_bContinuousInput && !m_PreviousInput.m_PreviousInput && !m_PreviousInput.m_bContinuousInput
			&& m_PreviousInput.ValidWindow(nowMs) && m_PreviousInput.ValidSeriesWindow() && Follows(m_PreviousInput);
	}

	protected bool ValidWindow(int nowMs)
	{
		if (m_iSequence < 0 || m_iSequence >= int.MAX || m_iFirstSampleIndex < 0 || m_iSampleCount <= 0
			|| m_iFirstSampleIndex > int.MAX - m_iSampleCount || m_iCaptureFromMs < 0
			|| m_iCaptureFromMs >= m_iCaptureUntilMs || m_iCaptureUntilMs > m_iRxReadyMs || m_iRxReadyMs > nowMs) return false;
		if (m_TrustedCaptureAnchor) return HasValidTrustedCapture();
		if (m_CaptureEnvelope && (!m_CaptureEnvelope.m_Binding || m_CaptureEnvelope.m_iMappingRevision <= 0
			|| !RICON_TemporalSnapshots.ValidKey(m_CaptureEnvelope.m_sTimebaseId)
			|| m_CaptureEnvelope.m_iFirstSampleIndex != m_iFirstSampleIndex || m_CaptureEnvelope.m_iSampleCount != m_iSampleCount
			|| m_CaptureEnvelope.m_iLowerMs != m_iCaptureFromMs || m_CaptureEnvelope.m_iUpperMs != m_iCaptureUntilMs
			|| m_CaptureEnvelope.m_iLowerSubUs != 0 || m_CaptureEnvelope.m_iUpperSubUs != 0 || !IsFixedWindow())) return false;
		return true;
	}

	bool HasValidTrustedCapture()
	{
		return m_TrustedCaptureAnchor && m_CaptureEnvelope && m_CaptureEnvelope.m_Binding
			&& m_CaptureEnvelope.m_Binding.IsValid() && m_CaptureEnvelope.m_Binding.m_sProfileId == RICON_MediaPcmTestProfile.PROFILE_ID
			&& m_iSampleCount == RICON_MediaPcmTestProfile.FRAME_SAMPLES
			&& m_TrustedCaptureAnchor.m_iSampleRateHz == RICON_MediaPcmTestProfile.SAMPLE_RATE_HZ
			&& m_iFirstSampleIndex >= m_TrustedCaptureAnchor.m_iFirstSampleIndex
			&& (m_iFirstSampleIndex - m_TrustedCaptureAnchor.m_iFirstSampleIndex) % RICON_MediaPcmTestProfile.FRAME_SAMPLES == 0
			&& m_CaptureEnvelope.Matches(m_TrustedCaptureAnchor.m_Binding, m_iFirstSampleIndex, m_iSampleCount)
			&& RICON_MediaCaptureClock.MatchesMappedBounds(m_TrustedCaptureAnchor, m_CaptureEnvelope, m_iCaptureFromMs, m_iCaptureUntilMs);
	}

	protected bool ValidSeriesWindow()
	{
		if (m_TrustedCaptureAnchor) return HasValidTrustedCapture();
		return IsFixedWindow();
	}

	protected bool IsFixedWindow()
	{
		return m_iSampleCount == RICON_MediaPcmTestProfile.FRAME_SAMPLES
			&& m_iCaptureUntilMs - m_iCaptureFromMs == RICON_MediaPcmTestProfile.FRAME_DURATION_MS;
	}

	bool Follows(RICON_RelayPcmOrigin earlier)
	{
		if (!earlier || earlier.m_iSequence >= int.MAX - 1 || earlier.m_iSampleCount <= 0
			|| earlier.m_iFirstSampleIndex > int.MAX - earlier.m_iSampleCount || m_iSequence != earlier.m_iSequence + 1
			|| m_iFirstSampleIndex != earlier.m_iFirstSampleIndex + earlier.m_iSampleCount) return false;
		if (m_TrustedCaptureAnchor || earlier.m_TrustedCaptureAnchor)
			return m_TrustedCaptureAnchor && earlier.m_TrustedCaptureAnchor
				&& m_TrustedCaptureAnchor.SameAnchor(earlier.m_TrustedCaptureAnchor)
				&& HasValidTrustedCapture() && earlier.HasValidTrustedCapture();
		return m_iCaptureFromMs == earlier.m_iCaptureUntilMs && SameCaptureClock(earlier);
	}

	protected bool SameCaptureClock(RICON_RelayPcmOrigin earlier)
	{
		if (!m_CaptureEnvelope) return !earlier.m_CaptureEnvelope;
		return earlier.m_CaptureEnvelope && m_CaptureEnvelope.m_Binding
			&& m_CaptureEnvelope.m_Binding.Same(earlier.m_CaptureEnvelope.m_Binding)
			&& m_CaptureEnvelope.m_sTimebaseId == earlier.m_CaptureEnvelope.m_sTimebaseId
			&& m_CaptureEnvelope.m_iMappingRevision == earlier.m_CaptureEnvelope.m_iMappingRevision;
	}

	bool SameWindow(RICON_RelayPcmOrigin other)
	{
		if (!other || m_iSequence != other.m_iSequence || m_iFirstSampleIndex != other.m_iFirstSampleIndex
			|| m_iSampleCount != other.m_iSampleCount || m_iCaptureFromMs != other.m_iCaptureFromMs
			|| m_iCaptureUntilMs != other.m_iCaptureUntilMs || m_iRxReadyMs != other.m_iRxReadyMs
			|| m_iDeadlineMs != other.m_iDeadlineMs) return false;
		if (m_TrustedCaptureAnchor)
		{
			if (!m_TrustedCaptureAnchor.SameAnchor(other.m_TrustedCaptureAnchor)) return false;
		}
		else if (other.m_TrustedCaptureAnchor) return false;
		if (m_CaptureEnvelope) return m_CaptureEnvelope.SameEnvelope(other.m_CaptureEnvelope);
		return !other.m_CaptureEnvelope;
	}

	int EffectiveDeadlineMs()
	{
		if (!m_PreviousInput) return m_iDeadlineMs;
		return Math.Min(m_iDeadlineMs, m_PreviousInput.m_iDeadlineMs);
	}

	RICON_RelayPcmOrigin CloneOrigin()
	{
		RICON_RelayPcmOrigin copy = CloneWindow();
		copy.m_bContinuousInput = m_bContinuousInput;
		if (m_PreviousInput) copy.m_PreviousInput = m_PreviousInput.CloneWindow();
		return copy;
	}

	RICON_RelayPcmOrigin CloneWindow()
	{
		RICON_RelayPcmOrigin copy = new RICON_RelayPcmOrigin();
		copy.m_iSequence = m_iSequence;
		copy.m_iFirstSampleIndex = m_iFirstSampleIndex;
		copy.m_iSampleCount = m_iSampleCount;
		copy.m_iCaptureFromMs = m_iCaptureFromMs;
		copy.m_iCaptureUntilMs = m_iCaptureUntilMs;
		copy.m_iRxReadyMs = m_iRxReadyMs;
		copy.m_iDeadlineMs = m_iDeadlineMs;
		if (m_CaptureEnvelope) copy.m_CaptureEnvelope = m_CaptureEnvelope.CloneEnvelope();
		if (m_TrustedCaptureAnchor) copy.m_TrustedCaptureAnchor = m_TrustedCaptureAnchor.CloneAnchor();
		return copy;
	}
}

class RICON_RelayPcmEmission
{
	ref RICON_RelayPcmPath m_Path;
	ref RICON_RelayPcmOrigin m_Origin;
	int m_iSequence, m_iRxFirstSampleIndex, m_iReadyAtMs, m_iEmitAtMs = -1;
	ref array<int> m_aSamples = {};
}

//! Internal TEST_ONLY contract. The trusted single-source RX producer supplies the mix and origin together.
class RICON_RelayPcmHandoff
{
	protected ref RICON_RelayPcmPath m_Path;
	protected ref RICON_RelayPcmPolicy m_Policy;
	protected ref RICON_MediaOutputPermit m_InputPermit;
	protected ref RICON_RelayPcmOrigin m_LastTrustedOrigin;
	protected ref array<ref RICON_RelayPcmEmission> m_Pending = {};
	protected int m_iLastNowMs = -1, m_iLastOriginSequence = -1;
	protected int m_iLastOriginSampleEnd, m_iLastRxSampleEnd, m_iLastCaptureEndMs, m_iNextEmissionSequence;
	protected bool m_bClosed, m_bCheckingRights, m_bSeriesQueue;
	protected int m_iCapacity = 1;

	bool Configure(RICON_RelayPcmPath path, RICON_RelayPcmPolicy policy)
	{
		if (m_bClosed || m_Path || !path || !path.IsValid() || !policy) return false;
		m_Path = path.ClonePath();
		m_Policy = policy;
		return true;
	}

	//! Включается до первого вызова очереди. Однокадровый путь сохраняет один слот.
	bool EnableSeriesQueue(int capacity = 2)
	{
		if (m_bClosed || !m_Path || m_bSeriesQueue || m_iLastNowMs >= 0 || capacity < 2 || capacity > 16) return false;
		m_bSeriesQueue = true;
		m_iCapacity = capacity;
		return true;
	}

	bool HasSeriesRoom()
	{
		return !m_bClosed && !m_bCheckingRights && m_bSeriesQueue && m_Pending.Count() < m_iCapacity;
	}

	bool HasSeriesQueue() { return !m_bClosed && m_bSeriesQueue; }
	bool MatchesPath(RICON_RelayPcmPath path)
	{
		return !m_bClosed && m_Path && m_Path.SamePath(path);
	}

	bool MatchesAuthority(RICON_RelayPcmPath path, RICON_RelayPcmPolicy policy)
	{
		return policy && m_Policy == policy && MatchesPath(path);
	}

	bool CanTakeAt(int nowMs) { return nowMs >= 0 && nowMs >= m_iLastNowMs; }

	bool HasCurrentInputPermit()
	{
		return !m_bClosed && m_Path && m_InputPermit && m_InputPermit.MatchesRoute(m_Path.m_UpstreamRoute);
	}

	string Offer(RICON_SharedRxMixResult mix, RICON_RelayPcmOrigin origin, int nowMs)
	{
		if (m_bClosed || !m_Path || m_bCheckingRights) return "CLOSED";
		if (!CanTakeAt(nowMs)) return "BAD_TIME";
		if (m_InputPermit && !m_InputPermit.MatchesRoute(m_Path.m_UpstreamRoute))
		{
			Close();
			return "REVOKED";
		}
		if (m_bSeriesQueue && !m_Pending.IsEmpty() && nowMs >= m_Pending[0].m_Origin.EffectiveDeadlineMs())
		{
			Close();
			return "EXPIRED";
		}
		if (!mix || !m_Path.m_UpstreamRoute.Same(mix.m_Rx) || !RICON_SharedRxTestMixer.IsPcmWindow(mix.m_aSamples)
			|| !mix.m_aPermits || mix.m_aPermits.Count() != 1 || mix.m_iFirstSampleIndex < 0
			|| mix.m_iFirstSampleIndex > int.MAX - RICON_SharedRxTestMixer.WINDOW_SAMPLES
			|| !origin || !origin.IsValid(nowMs)) return "INVALID";
		if (m_bSeriesQueue && !origin.m_bContinuousInput) return "INVALID";
		bool anchored = origin.m_TrustedCaptureAnchor != null;
		if (mix.m_RelayInputOrigin)
		{
			if (!mix.m_RelayInputOrigin.SameWindow(origin) || mix.m_RelayInputOrigin.m_bContinuousInput != origin.m_bContinuousInput) return "INVALID";
			RICON_RelayPcmOrigin modeledPrevious = mix.m_RelayInputOrigin.m_PreviousInput;
			if (modeledPrevious && !modeledPrevious.SameWindow(origin.m_PreviousInput)) return "INVALID";
			if (!modeledPrevious && origin.m_PreviousInput) return "INVALID";
		}
		if (origin.m_CaptureEnvelope && (!origin.m_bContinuousInput
			|| !m_Path.m_UpstreamRoute.m_Source.Same(origin.m_CaptureEnvelope.m_Binding))) return "INVALID";
		int rateRatio = RICON_PcmResampler8To48.OUTPUT_RATE_HZ / RICON_PcmResampler8To48.INPUT_RATE_HZ;
		if (origin.m_bContinuousInput && (mix.m_iFirstSampleIndex % rateRatio != 0
			|| mix.m_iFirstSampleIndex / rateRatio != origin.m_iFirstSampleIndex)) return "INVALID";
		ref RICON_MediaOutputPermit permit = mix.m_aPermits[0];
		if (!permit || !permit.MatchesRoute(m_Path.m_UpstreamRoute) || (m_InputPermit && permit != m_InputPermit))
			return "INVALID";
		if (origin.m_iSequence <= m_iLastOriginSequence) return "DUPLICATE";
		if (m_iLastOriginSequence >= 0 && anchored != (m_LastTrustedOrigin != null)) return "INVALID";
		if (anchored)
		{
			if (!m_bSeriesQueue || !origin.m_bContinuousInput) return "INVALID";
			if (m_LastTrustedOrigin)
			{
				if (!origin.Follows(m_LastTrustedOrigin) || !origin.m_PreviousInput
					|| !origin.m_PreviousInput.SameWindow(m_LastTrustedOrigin)) return "INVALID";
			}
			else if (origin.m_PreviousInput) return "INVALID";
		}
		if (origin.m_iFirstSampleIndex < m_iLastOriginSampleEnd || (!anchored && origin.m_iCaptureFromMs < m_iLastCaptureEndMs)
			|| mix.m_iFirstSampleIndex < m_iLastRxSampleEnd)
			return "INVALID";
		int deadlineMs = origin.EffectiveDeadlineMs();
		if (deadlineMs <= nowMs || m_Path.m_iProcessingDelayMs >= deadlineMs - nowMs)
			return "EXPIRED";
		if (m_iNextEmissionSequence >= int.MAX) { Close(); return "CLOSED"; }
		if (!RefreshRights(nowMs)) return "REVOKED";
		if (!permit.MatchesRoute(m_Path.m_UpstreamRoute)) { Close(); return "REVOKED"; }
		if (!m_Pending.IsEmpty() && nowMs >= m_Pending[0].m_Origin.EffectiveDeadlineMs())
			m_Pending.Clear();
		if ((!m_bSeriesQueue && !m_Pending.IsEmpty()) || (m_bSeriesQueue && !HasSeriesRoom())) return "FULL";
		RICON_RelayPcmEmission next = new RICON_RelayPcmEmission();
		next.m_Path = m_Path.ClonePath();
		next.m_Origin = origin.CloneOrigin();
		next.m_iSequence = m_iNextEmissionSequence;
		next.m_iRxFirstSampleIndex = mix.m_iFirstSampleIndex;
		next.m_iReadyAtMs = nowMs + m_Path.m_iProcessingDelayMs;
		next.m_aSamples.InsertAll(mix.m_aSamples);
		m_InputPermit = permit;
		m_Pending.Insert(next);
		m_iNextEmissionSequence++;
		m_iLastOriginSequence = next.m_Origin.m_iSequence;
		m_iLastOriginSampleEnd = next.m_Origin.m_iFirstSampleIndex + next.m_Origin.m_iSampleCount;
		m_iLastRxSampleEnd = next.m_iRxFirstSampleIndex + RICON_SharedRxTestMixer.WINDOW_SAMPLES;
		m_iLastCaptureEndMs = next.m_Origin.m_iCaptureUntilMs;
		if (anchored) m_LastTrustedOrigin = next.m_Origin.CloneWindow();
		return "OK";
	}

	string Take(int nowMs, out RICON_RelayPcmEmission emission, int plannedEmitMs = -1)
	{
		emission = null;
		if (m_bClosed || !m_Path || m_bCheckingRights) return "CLOSED";
		if (!CanTakeAt(nowMs)) return "BAD_TIME";
		if (plannedEmitMs < -1 || plannedEmitMs > nowMs || (plannedEmitMs >= 0 && !m_bSeriesQueue)) return "BAD_TIME";
		if (!RefreshRights(nowMs)) return "REVOKED";
		if (m_Pending.IsEmpty()) return "EMPTY";
		ref RICON_RelayPcmEmission head = m_Pending[0];
		if (nowMs >= head.m_Origin.EffectiveDeadlineMs())
		{
			if (m_bSeriesQueue) Close();
			else m_Pending.Clear();
			return "EXPIRED";
		}
		if (nowMs < head.m_iReadyAtMs) return "WAIT";
		if (plannedEmitMs >= 0 && head.m_iReadyAtMs > plannedEmitMs) return "GAP";
		emission = head;
		m_Pending.RemoveOrdered(0);
		emission.m_iEmitAtMs = nowMs;
		if (plannedEmitMs >= 0) emission.m_iEmitAtMs = plannedEmitMs;
		return "OK";
	}

	protected bool RefreshRights(int nowMs)
	{
		m_iLastNowMs = nowMs;
		m_bCheckingRights = true;
		ref RICON_RelayPcmPolicy policy = m_Policy;
		bool allowed = policy.CanUseInput(m_Path.ClonePath(), nowMs);
		if (allowed && !m_bClosed) allowed = policy.CanUseOutput(m_Path.ClonePath(), nowMs);
		m_bCheckingRights = false;
		if (m_bClosed) return false;
		if (!allowed || (m_InputPermit && !m_InputPermit.MatchesRoute(m_Path.m_UpstreamRoute)))
		{
			Close();
			return false;
		}
		return true;
	}

	void Close()
	{
		m_bClosed = true;
		m_Pending.Clear();
		m_InputPermit = null;
		m_LastTrustedOrigin = null;
		m_Path = null;
		m_Policy = null;
	}
}
