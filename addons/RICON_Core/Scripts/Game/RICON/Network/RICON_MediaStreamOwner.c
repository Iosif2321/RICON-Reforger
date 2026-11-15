//! A detached block for the next stage. Taking it does not authorize any recipient.
class RICON_QueuedMediaBlock
{
	int m_iSequence, m_iFirstSampleIndex, m_iSampleCount, m_iCaptureLowerMs, m_iCaptureUpperMs;
	ref array<int> m_Bytes = {};
	ref RICON_MediaStreamBinding m_Binding;
	ref RICON_TemporalRfBudget m_RfBudget;
	ref RICON_MediaCaptureEnvelope m_CaptureEnvelope;
	ref RICON_MediaRfHistoryEntry m_RfHistory;
}

class RICON_MediaStreamScope
{
	ref RICON_MediaStreamBinding m_Binding;
	ref RICON_MediaIngress m_Ingress;
	ref RICON_MediaFanout m_DirectRoutes;
	ref RICON_MediaCaptureAnchor m_CaptureAnchor;
	int m_iFirstSequence;
	bool m_bRawCaptureAccepted;
	ref array<ref RICON_QueuedMediaBlock> m_Queue = {};

	void Retire()
	{
		if (m_Ingress) m_Ingress.Close();
		if (m_DirectRoutes) m_DirectRoutes.Close();
		m_Ingress = null;
		m_CaptureAnchor = null;
		m_DirectRoutes = null;
		m_Queue.Clear();
	}
}

//! One trusted owner per authority. Direct emissions only, without recipient routing or terminal audio drain.
class RICON_MediaStreamOwner
{
	protected RICON_LifecycleStateMachine m_Lifecycle;
	protected ref map<int, ref RICON_MediaStreamScope> m_Scopes = new map<int, ref RICON_MediaStreamScope>();
	protected int m_iEpoch, m_iMaxStreams, m_iMaxQueued, m_iWidth, m_iMaxPayloadBytes;
	protected int m_iLastNowMs = -1;
	protected int m_iHighestStreamId;
	protected int m_iLastPumpMs = -1;
	protected int m_iLastPumpedLease;
	protected bool m_bClosed;
	protected bool m_bPumping;
	protected bool m_bPumpCancelled;

	bool Configure(RICON_LifecycleStateMachine lifecycle, int maxStreams, int maxQueuedPerStream, int width, int maxPayloadBytes)
	{
		if (m_bClosed || m_iEpoch != 0 || !lifecycle || maxStreams <= 0 || maxQueuedPerStream <= 0 || width <= 0 || maxPayloadBytes <= 0)
			return false;
		int epoch = lifecycle.GetSessionEpoch();
		if (epoch <= 0 || epoch >= int.MAX) return false;
		m_Lifecycle = lifecycle;
		m_iEpoch = epoch;
		m_iMaxStreams = maxStreams;
		m_iMaxQueued = maxQueuedPerStream;
		m_iWidth = width;
		m_iMaxPayloadBytes = maxPayloadBytes;
		return true;
	}

	int ScopeCount() { return m_Scopes.Count(); }

	int LastObservedTimeMs() { return m_iLastNowMs; }

	bool IsScope(RICON_LifecycleStateMachine lifecycle)
	{
		return !m_bClosed && lifecycle && lifecycle == m_Lifecycle && lifecycle.GetSessionEpoch() == m_iEpoch;
	}

	bool IsSourceCurrent(RICON_MediaStreamBinding binding, RICON_LifecycleStateMachine lifecycle, int nowMs)
	{
		if (!binding || !lifecycle || lifecycle != m_Lifecycle || nowMs < m_iLastNowMs) return false;
		RICON_MediaStreamScope scope = FindScope(binding.m_iSessionEpoch, binding.m_iLeaseId);
		return scope && scope.m_Ingress && scope.m_Binding.Same(binding) && scope.m_Ingress.IsCurrent(nowMs);
	}

	//! Complete detached set of admitted emissions for the specified channel. No queue is read or advanced.
	bool ReadActiveRadioSources(RICON_LifecycleStateMachine lifecycle, int networkId, int channelId, int nowMs,
		array<ref RICON_MediaStreamBinding> sources)
	{
		if (!sources) return false;
		sources.Clear();
		if (!IsScope(lifecycle) || networkId <= 0 || channelId <= 0 || nowMs < m_iLastNowMs || !lifecycle.CanQueryAt(nowMs)) return false;
		array<int> leases = {};
		for (int index = 0; index < m_Scopes.Count(); index++)
		{
			RICON_MediaStreamScope scope = m_Scopes.GetElement(index);
			RICON_MediaStreamBinding binding = scope.m_Binding;
			if (binding.m_iNetworkTypeId == networkId && binding.m_iChannelId == channelId && scope.m_Ingress
				&& scope.m_Ingress.IsCurrent(nowMs)) leases.Insert(m_Scopes.GetKey(index));
		}
		leases.Sort();
		foreach (int leaseId : leases) sources.Insert(m_Scopes.Get(leaseId).m_Binding.CloneBinding());
		return true;
	}

	bool CanFanOut(RICON_MediaStreamBinding binding, RICON_LifecycleStateMachine lifecycle, RICON_MediaFanout fanout,
		bool managed, int nowMs)
	{
		if (!IsSourceCurrent(binding, lifecycle, nowMs)) return false;
		RICON_MediaStreamScope scope = FindScope(binding.m_iSessionEpoch, binding.m_iLeaseId);
		if (managed) return scope.m_DirectRoutes == fanout;
		return !scope.m_DirectRoutes;
	}

	//! One managed dispatcher for an admitted source. Draining a managed scope requires its registered handle.
	RICON_MediaFanout OpenDirectRoutes(RICON_MediaStreamBinding source, RICON_LifecycleStateMachine lifecycle,
		RICON_TemporalPublicationOwner publication, int nowMs, int maxBranches, int maxQueuedPerBranch, int maxPayloadBytes,
		RICON_MediaReceiverAuthority receiverAuthority = null, RICON_MediaListenerAuthority listenerAuthority = null,
		int openRadioNetworkId = 0, RICON_GameControllerLifecycle controllerLifecycle = null)
	{
		if (!publication || !IsSourceCurrent(source, lifecycle, nowMs) || !publication.CanReadMediaLinks(lifecycle, nowMs)) return null;
		if (receiverAuthority && !receiverAuthority.CanRead(lifecycle, nowMs)) return null;
		if (listenerAuthority && !listenerAuthority.CanRead(nowMs)) return null;
		RICON_MediaStreamScope scope = FindScope(source.m_iSessionEpoch, source.m_iLeaseId);
		if (scope.m_DirectRoutes) return null;
		RICON_MediaFanout routes = new RICON_MediaFanout();
		if (!routes.Configure(this, lifecycle, source, maxBranches, maxQueuedPerBranch, maxPayloadBytes,
			publication, true, receiverAuthority, listenerAuthority, openRadioNetworkId, nowMs, controllerLifecycle)) return null;
		scope.m_DirectRoutes = routes;
		m_iLastNowMs = nowMs;
		return routes;
	}

	protected bool ObserveTime(int nowMs)
	{
		if (m_bClosed || !m_Lifecycle || nowMs < m_iLastNowMs || !m_Lifecycle.CanQueryAt(nowMs)) return false;
		m_iLastNowMs = nowMs;
		return true;
	}

	protected void ClearScopes()
	{
		for (int i = 0; i < m_Scopes.Count(); i++)
			m_Scopes.GetElement(i).Retire();
		m_Scopes.Clear();
	}

	protected bool Current(int leaseId, RICON_MediaStreamScope scope, int nowMs)
	{
		if (scope.m_Ingress && scope.m_Ingress.IsCurrent(nowMs)) return true;
		scope.Retire();
		// Keep the closed marker while this lease could still be used to open another window.
		if (!m_Lifecycle.QueryActiveLease(leaseId, nowMs)) m_Scopes.Remove(leaseId);
		return false;
	}

	bool Synchronize(int nowMs)
	{
		if (!m_Lifecycle) return false;
		int epoch = m_Lifecycle.GetSessionEpoch();
		if (epoch < m_iEpoch || epoch >= int.MAX) return false;
		if (epoch != m_iEpoch)
		{
			// Lifecycle may reset its timebase on epoch rotation.
			if (!m_Lifecycle.CanQueryAt(nowMs)) return false;
			ClearScopes();
			m_iEpoch = epoch;
			m_iHighestStreamId = 0;
			m_iLastPumpMs = -1;
			m_iLastPumpedLease = 0;
			m_iLastNowMs = nowMs;
			return true;
		}
		if (!ObserveTime(nowMs)) return false;
		// shortcut: indexed scan at synchronization; replace with measured terminal-event dispatch if needed.
		for (int i = m_Scopes.Count() - 1; i >= 0; i--)
		{
			int leaseId = m_Scopes.GetKey(i);
			RICON_MediaStreamScope scope = m_Scopes.Get(leaseId);
			Current(leaseId, scope, nowMs);
		}
		return true;
	}

	//! Work once from a complete, freshly committed server tick. One visit releases at most maxFramesPerVisit already admitted frames; the default stays one.
	bool PumpOpenRadio(RICON_TemporalPublicationOwner publication, int nowMs, int maxVisits, int maxCaptureAgeMs,
		RICON_MediaFanoutTransport transport, int maxFramesPerVisit = 1)
	{
		if (m_bPumping || !transport || !publication || nowMs <= m_iLastPumpMs || maxVisits <= 0 || maxCaptureAgeMs <= 0
			|| maxFramesPerVisit <= 0 || maxFramesPerVisit > 16
			|| !publication.HasCommittedMediaTick(m_Lifecycle, nowMs) || !Synchronize(nowMs)) return false;
		m_bPumping = true;
		m_bPumpCancelled = false;
		m_iLastPumpMs = nowMs;
		array<int> leases = {};
		for (int index = 0; index < m_Scopes.Count(); index++)
		{
			RICON_MediaFanout routes = m_Scopes.GetElement(index).m_DirectRoutes;
			if (routes && routes.CanPumpOpenRadio(publication)) leases.Insert(m_Scopes.GetKey(index));
		}
		leases.Sort();
		RICON_MediaRpcTransport wire = RICON_MediaRpcTransport.Cast(transport);
		string rfTimebaseId;
		bool historicalRf = wire && wire.GetWireVersion() == 3;
		if (wire) rfTimebaseId = wire.GetRfTimebaseId();
		int start;
		while (start < leases.Count() && leases[start] <= m_iLastPumpedLease) start++;
		int visits = leases.Count();
		if (maxVisits < visits) visits = maxVisits;
		for (int offset = 0; offset < visits; offset++)
		{
			if (m_bClosed || m_bPumpCancelled || !publication.HasCommittedMediaTick(m_Lifecycle, nowMs)) break;
			int lease = leases[(start + offset) % leases.Count()];
			m_iLastPumpedLease = lease;
			RICON_MediaStreamScope scope = FindScope(m_iEpoch, lease);
			if (!scope || !scope.m_DirectRoutes || !scope.m_DirectRoutes.CanPumpOpenRadio(publication)) continue;
			ref RICON_MediaFanout routes = scope.m_DirectRoutes;
			if (!routes.RefreshOpenRadio(nowMs)) continue;
			bool sent = false;
			for (int frame = 0; frame < maxFramesPerVisit; frame++)
			{
				if (m_bClosed || m_bPumpCancelled || !publication.HasCommittedMediaTick(m_Lifecycle, nowMs)) break;
				int extracted = routes.FanOutOne(nowMs, int.MAX, maxCaptureAgeMs, rfTimebaseId, historicalRf);
				// Пустое извлечение по-прежнему отдаёт один уже стоящий прямой остаток и не повторяет его на следующих попытках.
				if (extracted < 0)
				{
					if (!sent) routes.DispatchOne(nowMs, transport);
					break;
				}
				routes.DispatchOne(nowMs, transport);
				sent = true;
			}
		}
		m_bPumping = false;
		return true;
	}

	void CancelOpenRadioPump() { m_bPumpCancelled = true; }

	bool Open(RICON_MediaStreamBinding admitted, int nowMs, int firstSequence)
	{
		if (!admitted || !m_Lifecycle || admitted.m_iSessionEpoch != m_Lifecycle.GetSessionEpoch() || !Synchronize(nowMs))
			return false;
		if (admitted.m_iStreamId <= m_iHighestStreamId || m_Scopes.Contains(admitted.m_iLeaseId)
			|| m_Scopes.Count() >= m_iMaxStreams) return false;
		RICON_MediaIngress ingress = new RICON_MediaIngress();
		if (!ingress.Open(admitted, m_Lifecycle, nowMs, firstSequence, m_iWidth, m_iMaxPayloadBytes)) return false;
		RICON_MediaStreamScope scope = new RICON_MediaStreamScope();
		scope.m_Binding = admitted.CloneBinding();
		scope.m_Ingress = ingress;
		scope.m_iFirstSequence = firstSequence;
		m_Scopes.Set(admitted.m_iLeaseId, scope);
		m_iHighestStreamId = admitted.m_iStreamId;
		return true;
	}

	protected RICON_MediaStreamScope FindScope(int sessionEpoch, int leaseId)
	{
		if (!m_Lifecycle || sessionEpoch != m_iEpoch || m_Lifecycle.GetSessionEpoch() != m_iEpoch) return null;
		return m_Scopes.Get(leaseId);
	}

	bool UsesMappedCapture(RICON_MediaStreamBinding source)
	{
		if (!source) return false;
		RICON_MediaStreamScope scope = FindScope(source.m_iSessionEpoch, source.m_iLeaseId);
		return scope && scope.m_CaptureAnchor && scope.m_Binding.Same(source);
	}

	RICON_EMediaIngressDecision Accept(RICON_MediaStreamBinding claimed, int authenticatedPlayerId,
		int authenticatedConnectionId, int nowMs, int captureLowerMs, int captureUpperMs,
		int sequence, int firstSampleIndex, int sampleCount, array<int> payload, out RICON_EMediaWindowDecision windowDecision)
	{
		return AcceptBlock(claimed, authenticatedPlayerId, authenticatedConnectionId, nowMs, captureLowerMs, captureUpperMs,
			sequence, firstSampleIndex, sampleCount, payload, windowDecision, false);
	}

	//! Trusted server control installs one measured mapping before admitting media for this stream.
	bool InstallCaptureAnchor(RICON_MediaStreamBinding source, RICON_MediaCaptureAnchor anchor, int nowMs)
	{
		if (!source || !RICON_MediaCaptureClock.ValidAnchor(anchor, source)) return false;
		RICON_MediaStreamScope scope = FindScope(source.m_iSessionEpoch, source.m_iLeaseId);
		if (!scope || scope.m_CaptureAnchor || scope.m_bRawCaptureAccepted || !IsSourceCurrent(source, m_Lifecycle, nowMs)) return false;
		if (!ObserveTime(nowMs)) return false;
		scope.m_CaptureAnchor = anchor.CloneAnchor();
		return true;
	}

	RICON_EMediaIngressDecision AcceptMapped(RICON_MediaStreamBinding claimed, int authenticatedPlayerId,
		int authenticatedConnectionId, int nowMs, int sequence, int firstSampleIndex, int sampleCount,
		array<int> payload, out RICON_EMediaWindowDecision windowDecision)
	{
		return AcceptBlock(claimed, authenticatedPlayerId, authenticatedConnectionId, nowMs, -1, -1,
			sequence, firstSampleIndex, sampleCount, payload, windowDecision, true);
	}

	//! Вход фиксированного профиля. Полный binding и опора отсчётов уже установлены сервером.
	RICON_EMediaIngressDecision AcceptRemotePcm(int epoch, int leaseId, int streamId, int authenticatedPlayerId,
		int authenticatedConnectionId, int playerGeneration, int entityId, int nowMs, int sequence, int firstSampleIndex,
		array<int> payload, int maxCaptureAgeMs, out RICON_EMediaWindowDecision windowDecision, string requiredTimebaseId = "")
	{
		windowDecision = RICON_EMediaWindowDecision.INVALID_WINDOW;
		if (m_bClosed) return RICON_EMediaIngressDecision.CLOSED;
		RICON_MediaStreamScope scope = FindScope(epoch, leaseId);
		if (!scope) return RICON_EMediaIngressDecision.STREAM_NOT_ADMITTED;
		RICON_MediaStreamBinding binding = scope.m_Binding;
		if (binding.m_iPlayerId != authenticatedPlayerId || binding.m_iConnectionId != authenticatedConnectionId)
			return RICON_EMediaIngressDecision.SENDER_MISMATCH;
		if (binding.m_iStreamId != streamId || binding.m_iPlayerGeneration != playerGeneration || binding.m_iEntityId != entityId
			|| binding.m_sProfileId != RICON_MediaPcmProfile.PROFILE_ID) return RICON_EMediaIngressDecision.BINDING_MISMATCH;
		if (!scope.m_Ingress) return RICON_EMediaIngressDecision.CLOSED;
		RICON_MediaCaptureAnchor anchor = scope.m_CaptureAnchor;
		if (!anchor || anchor.m_iSampleRateHz != RICON_MediaPcmProfile.SAMPLE_RATE_HZ
			|| (!requiredTimebaseId.IsEmpty() && anchor.m_sTimebaseId != requiredTimebaseId))
			return RICON_EMediaIngressDecision.CLOCK_UNVERIFIED;
		if (nowMs < 0 || nowMs < m_iLastNowMs || maxCaptureAgeMs <= 0) return RICON_EMediaIngressDecision.LEASE_INACTIVE;
		int frameSamples = RICON_MediaPcmProfile.FRAME_SAMPLES;
		if (sequence < scope.m_iFirstSequence || sequence >= int.MAX)
			windowDecision = RICON_EMediaWindowDecision.INVALID_SEQUENCE;
		else if (firstSampleIndex < anchor.m_iFirstSampleIndex || firstSampleIndex > int.MAX - frameSamples)
			windowDecision = RICON_EMediaWindowDecision.INVALID_SAMPLE_RANGE;
		else
		{
			int offset = firstSampleIndex - anchor.m_iFirstSampleIndex;
			if (offset % frameSamples != 0 || offset / frameSamples != sequence - scope.m_iFirstSequence)
				windowDecision = RICON_EMediaWindowDecision.INVALID_SAMPLE_RANGE;
		}
		if (windowDecision != RICON_EMediaWindowDecision.INVALID_WINDOW) return RICON_EMediaIngressDecision.WINDOW_REJECTED;
		if (!payload || payload.Count() != RICON_MediaPcmProfile.PAYLOAD_BYTES)
		{
			windowDecision = RICON_EMediaWindowDecision.INVALID_PAYLOAD;
			return RICON_EMediaIngressDecision.WINDOW_REJECTED;
		}
		foreach (int octet : payload)
		{
			if (octet >= 0 && octet <= 255) continue;
			windowDecision = RICON_EMediaWindowDecision.INVALID_PAYLOAD;
			return RICON_EMediaIngressDecision.WINDOW_REJECTED;
		}
		return AcceptBlock(binding, authenticatedPlayerId, authenticatedConnectionId, nowMs, -1, -1,
			sequence, firstSampleIndex, frameSamples, payload, windowDecision, true, maxCaptureAgeMs);
	}

	protected RICON_EMediaIngressDecision AcceptBlock(RICON_MediaStreamBinding claimed, int authenticatedPlayerId,
		int authenticatedConnectionId, int nowMs, int captureLowerMs, int captureUpperMs, int sequence,
		int firstSampleIndex, int sampleCount, array<int> payload, out RICON_EMediaWindowDecision windowDecision,
		bool mapped, int maxCaptureAgeMs = 0)
	{
		windowDecision = RICON_EMediaWindowDecision.INVALID_WINDOW;
		if (m_bClosed) return RICON_EMediaIngressDecision.CLOSED;
		if (!claimed) return RICON_EMediaIngressDecision.STREAM_NOT_ADMITTED;
		RICON_MediaStreamScope scope = FindScope(claimed.m_iSessionEpoch, claimed.m_iLeaseId);
		if (!scope) return RICON_EMediaIngressDecision.STREAM_NOT_ADMITTED;
		if (authenticatedPlayerId != scope.m_Binding.m_iPlayerId || authenticatedConnectionId != scope.m_Binding.m_iConnectionId)
			return RICON_EMediaIngressDecision.SENDER_MISMATCH;
		if (!scope.m_Binding.Same(claimed)) return RICON_EMediaIngressDecision.BINDING_MISMATCH;
		if (!scope.m_Ingress) return RICON_EMediaIngressDecision.CLOSED;
		if (mapped && !scope.m_CaptureAnchor) return RICON_EMediaIngressDecision.CLOCK_UNVERIFIED;
		if (!mapped && scope.m_CaptureAnchor) return RICON_EMediaIngressDecision.CLOCK_UNVERIFIED;
		RICON_MediaCaptureEnvelope envelope;
		if (mapped)
		{
			envelope = RICON_MediaCaptureClock.Map(scope.m_CaptureAnchor, scope.m_Binding, firstSampleIndex, sampleCount);
			if (!envelope) return RICON_EMediaIngressDecision.CLOCK_UNVERIFIED;
			captureLowerMs = envelope.m_iLowerMs;
			captureUpperMs = envelope.m_iUpperMs;
			if (envelope.m_iUpperSubUs > 0)
			{
				if (captureUpperMs == int.MAX) return RICON_EMediaIngressDecision.CLOCK_UNVERIFIED;
				captureUpperMs++;
			}
			if (captureUpperMs > nowMs) return RICON_EMediaIngressDecision.OUTSIDE_LEASE;
			if (maxCaptureAgeMs > 0 && (captureLowerMs < 0 || nowMs - captureLowerMs >= maxCaptureAgeMs))
				return RICON_EMediaIngressDecision.CAPTURE_EXPIRED;
		}
		if (!ObserveTime(nowMs) || !Current(claimed.m_iLeaseId, scope, nowMs)) return RICON_EMediaIngressDecision.LEASE_INACTIVE;
		if (scope.m_Queue.Count() >= m_iMaxQueued) return RICON_EMediaIngressDecision.QUEUE_FULL;
		RICON_EMediaIngressDecision decision = scope.m_Ingress.Accept(claimed, authenticatedPlayerId, authenticatedConnectionId,
			nowMs, captureLowerMs, captureUpperMs, sequence, firstSampleIndex, sampleCount, payload, windowDecision);
		if (decision != RICON_EMediaIngressDecision.ACCEPTED) return decision;
		RICON_QueuedMediaBlock block = new RICON_QueuedMediaBlock();
		block.m_Binding = scope.m_Binding.CloneBinding();
		block.m_iSequence = sequence;
		block.m_iFirstSampleIndex = firstSampleIndex;
		block.m_iSampleCount = sampleCount;
		block.m_iCaptureLowerMs = captureLowerMs;
		block.m_iCaptureUpperMs = captureUpperMs;
		block.m_CaptureEnvelope = envelope;
		if (!mapped) scope.m_bRawCaptureAccepted = true;
		foreach (int value : payload) block.m_Bytes.Insert(value);
		scope.m_Queue.Insert(block);
		return decision;
	}

	RICON_QueuedMediaBlock Take(int sessionEpoch, int leaseId, int nowMs, RICON_MediaFanout fanout = null)
	{
		RICON_MediaStreamScope scope = FindScope(sessionEpoch, leaseId);
		if (!scope || (scope.m_DirectRoutes && scope.m_DirectRoutes != fanout)) return null;
		if (!ObserveTime(nowMs) || !Current(leaseId, scope, nowMs) || scope.m_Queue.IsEmpty()) return null;
		RICON_QueuedMediaBlock block = scope.m_Queue[0];
		scope.m_Queue.RemoveOrdered(0);
		return block;
	}

	bool Retire(int sessionEpoch, int leaseId, int nowMs)
	{
		RICON_MediaStreamScope scope = FindScope(sessionEpoch, leaseId);
		if (!scope || !ObserveTime(nowMs)) return false;
		scope.Retire();
		return true;
	}

	void Close()
	{
		ClearScopes();
		m_Lifecycle = null;
		m_bClosed = true;
	}
}
