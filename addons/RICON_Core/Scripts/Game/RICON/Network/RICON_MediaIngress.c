//! Direct emission only. The trusted control plane selects the TX index and representation at stream admission.
class RICON_MediaStreamBinding
{
	int m_iSessionEpoch, m_iPlayerId, m_iPlayerGeneration, m_iConnectionId, m_iEntityId;
	int m_iLeaseId, m_iDeviceId, m_iNetworkTypeId, m_iChannelId;
	int m_iTxTransceiverIndex = -1;
	int m_iStreamId, m_iStreamGeneration, m_iCaptureEpoch, m_iFormatEpoch;
	string m_sProfileId;

	bool IsValid()
	{
		return m_iSessionEpoch > 0 && m_iSessionEpoch < int.MAX && m_iPlayerId > 0 && m_iPlayerGeneration > 0
			&& m_iConnectionId > 0 && m_iEntityId > 0 && m_iLeaseId > 0 && m_iDeviceId > 0
			&& m_iNetworkTypeId > 0 && m_iChannelId > 0 && m_iTxTransceiverIndex >= 0
			&& m_iStreamId > 0 && m_iStreamGeneration > 0 && m_iCaptureEpoch > 0 && m_iFormatEpoch > 0
			&& RICON_TemporalSnapshots.ValidKey(m_sProfileId);
	}

	bool Same(RICON_MediaStreamBinding other)
	{
		return other && m_iSessionEpoch == other.m_iSessionEpoch && m_iPlayerId == other.m_iPlayerId
			&& m_iPlayerGeneration == other.m_iPlayerGeneration && m_iConnectionId == other.m_iConnectionId
			&& m_iEntityId == other.m_iEntityId && m_iLeaseId == other.m_iLeaseId && m_iDeviceId == other.m_iDeviceId
			&& m_iNetworkTypeId == other.m_iNetworkTypeId && m_iChannelId == other.m_iChannelId
			&& m_iTxTransceiverIndex == other.m_iTxTransceiverIndex && m_iStreamId == other.m_iStreamId
			&& m_iStreamGeneration == other.m_iStreamGeneration && m_iCaptureEpoch == other.m_iCaptureEpoch
			&& m_iFormatEpoch == other.m_iFormatEpoch && m_sProfileId == other.m_sProfileId;
	}

	RICON_MediaStreamBinding CloneBinding()
	{
		RICON_MediaStreamBinding copy = new RICON_MediaStreamBinding();
		copy.m_iSessionEpoch = m_iSessionEpoch;
		copy.m_iPlayerId = m_iPlayerId;
		copy.m_iPlayerGeneration = m_iPlayerGeneration;
		copy.m_iConnectionId = m_iConnectionId;
		copy.m_iEntityId = m_iEntityId;
		copy.m_iLeaseId = m_iLeaseId;
		copy.m_iDeviceId = m_iDeviceId;
		copy.m_iNetworkTypeId = m_iNetworkTypeId;
		copy.m_iChannelId = m_iChannelId;
		copy.m_iTxTransceiverIndex = m_iTxTransceiverIndex;
		copy.m_iStreamId = m_iStreamId;
		copy.m_iStreamGeneration = m_iStreamGeneration;
		copy.m_iCaptureEpoch = m_iCaptureEpoch;
		copy.m_iFormatEpoch = m_iFormatEpoch;
		copy.m_sProfileId = m_sProfileId;
		return copy;
	}
}

enum RICON_EMediaIngressDecision
{
	STREAM_NOT_ADMITTED,
	CLOSED,
	SENDER_MISMATCH,
	BINDING_MISMATCH,
	LEASE_INACTIVE,
	OUTSIDE_LEASE,
	WINDOW_REJECTED,
	ACCEPTED,
	QUEUE_FULL,
	CLOCK_UNVERIFIED,
	CAPTURE_EXPIRED
}

//! Pure direct media gate. The caller supplies transport identity, qualified time bounds and codec checks.
//! ACCEPTED consumes the sequence. An egress reservation and current recipient policy belong to the caller.
class RICON_MediaIngress
{
	protected ref RICON_MediaStreamBinding m_Binding;
	protected RICON_LifecycleStateMachine m_Lifecycle;
	protected ref RICON_MediaWindow m_Window;
	protected int m_iLastNowMs;
	protected bool m_bClosed;

	protected static RICON_LifecycleLease CurrentLease(RICON_MediaStreamBinding binding, RICON_LifecycleStateMachine lifecycle, int nowMs)
	{
		if (!lifecycle || lifecycle.GetSessionEpoch() != binding.m_iSessionEpoch
			|| lifecycle.GetPlayerState(binding.m_iPlayerId) != RICON_EPlayerLifecycleState.BOUND_ACTIVE
			|| lifecycle.GetPlayerConnectionId(binding.m_iPlayerId) != binding.m_iConnectionId
			|| lifecycle.GetPlayerGeneration(binding.m_iPlayerId) != binding.m_iPlayerGeneration
			|| lifecycle.GetPlayerEntityId(binding.m_iPlayerId) != binding.m_iEntityId)
			return null;
		RICON_LifecycleLease lease = lifecycle.QueryActiveLease(binding.m_iLeaseId, nowMs);
		if (!lease || !lease.MatchesSource(binding.m_iPlayerId, binding.m_iPlayerGeneration, binding.m_iDeviceId,
			binding.m_iNetworkTypeId, binding.m_iChannelId))
			return null;
		RICON_LifecycleDevice device = lifecycle.QueryDevice(binding.m_iDeviceId, nowMs);
		if (!device || !device.m_bPowered || !device.CanAccess(binding.m_iPlayerId)
			|| !device.Allows(binding.m_iNetworkTypeId, binding.m_iChannelId))
			return null;
		return lease;
	}

	bool Open(RICON_MediaStreamBinding admitted, RICON_LifecycleStateMachine lifecycle, int nowMs,
		int firstSequence, int width, int maxPayloadBytes)
	{
		if (m_bClosed || m_Binding || !admitted || !admitted.IsValid() || !CurrentLease(admitted, lifecycle, nowMs))
			return false;
		RICON_MediaWindow window = new RICON_MediaWindow();
		if (!window.Initialize(admitted.m_iStreamGeneration, firstSequence, width, maxPayloadBytes))
			return false;
		m_Binding = admitted.CloneBinding();
		m_Lifecycle = lifecycle;
		m_Window = window;
		m_iLastNowMs = nowMs;
		return true;
	}

	RICON_EMediaIngressDecision Accept(RICON_MediaStreamBinding claimed, int authenticatedPlayerId,
		int authenticatedConnectionId, int nowMs, int captureLowerMs, int captureUpperMs,
		int sequence, int firstSampleIndex, int sampleCount, array<int> payload, out RICON_EMediaWindowDecision windowDecision)
	{
		windowDecision = RICON_EMediaWindowDecision.INVALID_WINDOW;
		if (m_bClosed) return RICON_EMediaIngressDecision.CLOSED;
		if (!m_Binding) return RICON_EMediaIngressDecision.STREAM_NOT_ADMITTED;
		if (authenticatedPlayerId != m_Binding.m_iPlayerId || authenticatedConnectionId != m_Binding.m_iConnectionId)
			return RICON_EMediaIngressDecision.SENDER_MISMATCH;
		if (!m_Binding.Same(claimed)) return RICON_EMediaIngressDecision.BINDING_MISMATCH;
		// QueryActiveLease is read-only, so a stale timestamp must not reopen this window.
		if (nowMs < m_iLastNowMs) return RICON_EMediaIngressDecision.LEASE_INACTIVE;
		m_iLastNowMs = nowMs;
		RICON_LifecycleLease lease = CurrentLease(m_Binding, m_Lifecycle, nowMs);
		if (!lease) return RICON_EMediaIngressDecision.LEASE_INACTIVE;
		if (captureLowerMs < lease.m_iIssuedAtMs || captureUpperMs < captureLowerMs || captureUpperMs > nowMs
			|| captureUpperMs - lease.m_iIssuedAtMs >= lease.m_iTtlMs)
			return RICON_EMediaIngressDecision.OUTSIDE_LEASE;
		windowDecision = m_Window.Accept(m_Binding.m_iStreamGeneration, sequence, payload, firstSampleIndex, sampleCount);
		if (windowDecision != RICON_EMediaWindowDecision.ACCEPTED) return RICON_EMediaIngressDecision.WINDOW_REJECTED;
		return RICON_EMediaIngressDecision.ACCEPTED;
	}

	bool IsCurrent(int nowMs)
	{
		return m_Binding && !m_bClosed && nowMs >= m_iLastNowMs && CurrentLease(m_Binding, m_Lifecycle, nowMs) != null;
	}

	void Close()
	{
		m_bClosed = true;
		if (m_Window) m_Window.Close();
		m_Window = null;
		m_Binding = null;
		m_Lifecycle = null;
	}
}
