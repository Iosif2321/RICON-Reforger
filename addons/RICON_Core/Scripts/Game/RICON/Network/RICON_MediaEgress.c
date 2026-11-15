//! Trusted route record from the authority. Direct radio branch with a single RX and one revision.
class RICON_MediaEgressRoute
{
	ref RICON_MediaStreamBinding m_Source;
	int m_iListenerPlayerId, m_iListenerConnectionId, m_iListenerGeneration, m_iListenerEntityId;
	int m_iRxDeviceId, m_iRxGeneration, m_iRouteId, m_iRouteGeneration, m_iPolicyRevision;
	int m_iRxTransceiverIndex = -1;

	bool IsValid()
	{
		return m_Source && m_Source.IsValid() && m_iListenerPlayerId > 0 && m_iListenerPlayerId != m_Source.m_iPlayerId
			&& m_iListenerConnectionId > 0 && m_iListenerConnectionId != m_Source.m_iConnectionId
			&& m_iListenerGeneration > 0 && m_iListenerEntityId > 0 && m_iRxDeviceId > 0 && m_iRxDeviceId != m_Source.m_iDeviceId
			&& m_iRxTransceiverIndex >= 0 && m_iRxGeneration > 0 && m_iRouteId > 0 && m_iRouteGeneration > 0 && m_iPolicyRevision > 0;
	}

	bool Same(RICON_MediaEgressRoute other)
	{
		return other && m_Source && m_Source.Same(other.m_Source) && m_iListenerPlayerId == other.m_iListenerPlayerId
			&& m_iListenerConnectionId == other.m_iListenerConnectionId && m_iListenerGeneration == other.m_iListenerGeneration
			&& m_iListenerEntityId == other.m_iListenerEntityId && m_iRxDeviceId == other.m_iRxDeviceId
			&& m_iRxTransceiverIndex == other.m_iRxTransceiverIndex && m_iRxGeneration == other.m_iRxGeneration
			&& m_iRouteId == other.m_iRouteId && m_iRouteGeneration == other.m_iRouteGeneration && m_iPolicyRevision == other.m_iPolicyRevision;
	}

	RICON_MediaEgressRoute CloneRoute()
	{
		RICON_MediaEgressRoute copy = new RICON_MediaEgressRoute();
		if (m_Source) copy.m_Source = m_Source.CloneBinding();
		copy.m_iListenerPlayerId = m_iListenerPlayerId;
		copy.m_iListenerConnectionId = m_iListenerConnectionId;
		copy.m_iListenerGeneration = m_iListenerGeneration;
		copy.m_iListenerEntityId = m_iListenerEntityId;
		copy.m_iRxDeviceId = m_iRxDeviceId;
		copy.m_iRxTransceiverIndex = m_iRxTransceiverIndex;
		copy.m_iRxGeneration = m_iRxGeneration;
		copy.m_iRouteId = m_iRouteId;
		copy.m_iRouteGeneration = m_iRouteGeneration;
		copy.m_iPolicyRevision = m_iPolicyRevision;
		return copy;
	}
}

class RICON_MediaEgressItem
{
	ref RICON_MediaEgressRoute m_Route;
	ref RICON_QueuedMediaBlock m_Block;
	int m_iDeadlineMs;
	int m_iPayloadBytes;
}

//! Pure direct route queue. Call Take at the synchronous send boundary with the current trusted route or null on revoke.
//! The authority supplies RX tuning/generation, social/cryptographic eligibility and the deadline.
//! Only the trusted dispatcher may Enqueue an unchanged Owner.Take block once per route.
class RICON_MediaEgressQueue
{
	protected RICON_MediaStreamOwner m_SourceOwner;
	protected RICON_LifecycleStateMachine m_Lifecycle;
	protected ref RICON_MediaEgressRoute m_Route;
	protected ref array<ref RICON_MediaEgressItem> m_Queue = {};
	protected int m_iMaxQueued, m_iMaxPayloadBytes;
	protected int m_iLastNowMs = -1;
	protected bool m_bClosed;

	bool Configure(RICON_MediaStreamOwner sourceOwner, RICON_LifecycleStateMachine lifecycle, RICON_MediaEgressRoute granted,
		int maxQueued, int maxPayloadBytes)
	{
		if (m_bClosed || m_Route || !sourceOwner || !lifecycle || !granted || !granted.IsValid()
			|| maxQueued <= 0 || maxPayloadBytes <= 0 || lifecycle.GetSessionEpoch() != granted.m_Source.m_iSessionEpoch) return false;
		m_SourceOwner = sourceOwner;
		m_Lifecycle = lifecycle;
		m_Route = granted.CloneRoute();
		m_iMaxQueued = maxQueued;
		m_iMaxPayloadBytes = maxPayloadBytes;
		return true;
	}

	int PendingCount() { return m_Queue.Count(); }

	protected bool ObserveTime(int nowMs)
	{
		if (m_bClosed || !m_Route) return false;
		if (!m_Lifecycle || !m_SourceOwner) { Close(); return false; }
		if (!m_Lifecycle.CanQueryAt(nowMs)) return false;
		if (m_Lifecycle.GetSessionEpoch() == m_Route.m_Source.m_iSessionEpoch
			&& (nowMs < m_iLastNowMs || nowMs < m_SourceOwner.LastObservedTimeMs())) return false;
		m_iLastNowMs = nowMs;
		return true;
	}

	protected bool EndpointsCurrent(int nowMs)
	{
		if (!m_Lifecycle || !m_SourceOwner || !m_Lifecycle.CanQueryAt(nowMs)
			|| !m_SourceOwner.IsSourceCurrent(m_Route.m_Source, m_Lifecycle, nowMs)) return false;
		return ReceiverCurrent(m_Route, m_Lifecycle, nowMs);
	}

	static bool ReceiverCurrent(RICON_MediaEgressRoute route, RICON_LifecycleStateMachine lifecycle, int nowMs)
	{
		int listener = route.m_iListenerPlayerId;
		if (lifecycle.GetPlayerState(listener) != RICON_EPlayerLifecycleState.BOUND_ACTIVE
			|| lifecycle.GetPlayerConnectionId(listener) != route.m_iListenerConnectionId
			|| lifecycle.GetPlayerGeneration(listener) != route.m_iListenerGeneration
			|| lifecycle.GetPlayerEntityId(listener) != route.m_iListenerEntityId) return false;
		RICON_LifecycleDevice rx = lifecycle.QueryDevice(route.m_iRxDeviceId, nowMs);
		return rx && rx.m_bPowered && rx.CanAccess(listener)
			&& rx.Allows(route.m_Source.m_iNetworkTypeId, route.m_Source.m_iChannelId);
	}

	bool Enqueue(RICON_QueuedMediaBlock block, int deadlineMs, int nowMs, RICON_TemporalRfBudget rfBudget = null,
		RICON_MediaRfHistoryEntry rfHistory = null)
	{
		if (!m_Route || !block || !m_Route.m_Source.Same(block.m_Binding) || !ObserveTime(nowMs)) return false;
		if (!EndpointsCurrent(nowMs)) { Close(); return false; }
		if ((block.m_CaptureEnvelope != null) != (rfHistory != null)) return false;
		if (rfHistory && !CoveredCapture(block, rfHistory, rfBudget, nowMs)) return false;
		if (nowMs >= deadlineMs || m_Queue.Count() >= m_iMaxQueued || !block.m_Bytes || block.m_Bytes.IsEmpty()
			|| block.m_Bytes.Count() > m_iMaxPayloadBytes || block.m_iSequence < 0 || block.m_iSequence >= int.MAX
			|| block.m_iFirstSampleIndex < 0 || block.m_iSampleCount <= 0 || block.m_iFirstSampleIndex > int.MAX - block.m_iSampleCount
			|| block.m_iCaptureLowerMs < 0 || block.m_iCaptureUpperMs < block.m_iCaptureLowerMs || block.m_iCaptureUpperMs > nowMs)
			return false;
		foreach (int value : block.m_Bytes)
		{
			if (value < 0 || value > 255) return false;
		}
		RICON_QueuedMediaBlock media = new RICON_QueuedMediaBlock();
		media.m_Binding = m_Route.m_Source.CloneBinding();
		media.m_iSequence = block.m_iSequence;
		media.m_iFirstSampleIndex = block.m_iFirstSampleIndex;
		media.m_iSampleCount = block.m_iSampleCount;
		media.m_iCaptureLowerMs = block.m_iCaptureLowerMs;
		media.m_iCaptureUpperMs = block.m_iCaptureUpperMs;
		if (block.m_CaptureEnvelope) media.m_CaptureEnvelope = block.m_CaptureEnvelope.CloneEnvelope();
		if (rfBudget) media.m_RfBudget = rfBudget.CloneBudget();
		if (rfHistory) media.m_RfHistory = rfHistory.CloneEntry();
		foreach (int byteValue : block.m_Bytes) media.m_Bytes.Insert(byteValue);
		RICON_MediaEgressItem item = new RICON_MediaEgressItem();
		item.m_Block = media;
		item.m_iDeadlineMs = deadlineMs;
		m_Queue.Insert(item);
		return true;
	}

	protected static bool CoveredCapture(RICON_QueuedMediaBlock block, RICON_MediaRfHistoryEntry history,
		RICON_TemporalRfBudget budget, int nowMs)
	{
		RICON_MediaCaptureEnvelope envelope = block.m_CaptureEnvelope;
		if (!history.m_Budget || (budget && !budget.Same(history.m_Budget))) return false;
		if (history.m_iValidUntilMs > nowMs || history.m_Budget.m_sTimebaseId != envelope.m_sTimebaseId) return false;
		if (!envelope.Matches(block.m_Binding, block.m_iFirstSampleIndex, block.m_iSampleCount)) return false;
		return RICON_TemporalClock.ContainsInterval(envelope.m_iLowerMs, envelope.m_iLowerSubUs,
			envelope.m_iUpperMs, envelope.m_iUpperSubUs, history.m_iValidFromMs, history.m_iValidUntilMs);
	}

	RICON_MediaEgressItem Take(RICON_MediaEgressRoute current, int nowMs)
	{
		if (!PrepareHead(current, nowMs)) return null;
		RICON_MediaEgressItem item = m_Queue[0];
		m_Queue.RemoveOrdered(0);
		item.m_Route = m_Route.CloneRoute();
		return item;
	}

	//! Read metadata of the first unexpired block without taking its payload.
	RICON_MediaEgressItem ReadHead(RICON_MediaEgressRoute current, int nowMs)
	{
		if (!PrepareHead(current, nowMs)) return null;
		RICON_MediaEgressItem head = m_Queue[0];
		RICON_MediaEgressItem preview = new RICON_MediaEgressItem();
		preview.m_iDeadlineMs = head.m_iDeadlineMs;
		preview.m_iPayloadBytes = head.m_Block.m_Bytes.Count();
		preview.m_Route = m_Route.CloneRoute();
		preview.m_Block = new RICON_QueuedMediaBlock();
		preview.m_Block.m_Binding = preview.m_Route.m_Source;
		preview.m_Block.m_iSequence = head.m_Block.m_iSequence;
		preview.m_Block.m_iFirstSampleIndex = head.m_Block.m_iFirstSampleIndex;
		preview.m_Block.m_iSampleCount = head.m_Block.m_iSampleCount;
		preview.m_Block.m_iCaptureLowerMs = head.m_Block.m_iCaptureLowerMs;
		preview.m_Block.m_iCaptureUpperMs = head.m_Block.m_iCaptureUpperMs;
		if (head.m_Block.m_CaptureEnvelope) preview.m_Block.m_CaptureEnvelope = head.m_Block.m_CaptureEnvelope.CloneEnvelope();
		if (head.m_Block.m_RfBudget) preview.m_Block.m_RfBudget = head.m_Block.m_RfBudget.CloneBudget();
		if (head.m_Block.m_RfHistory) preview.m_Block.m_RfHistory = head.m_Block.m_RfHistory.CloneEntry();
		return preview;
	}

	protected bool PrepareHead(RICON_MediaEgressRoute current, int nowMs)
	{
		if (!ObserveTime(nowMs)) return false;
		if (!m_Route.Same(current) || !EndpointsCurrent(nowMs)) { Close(); return false; }
		while (!m_Queue.IsEmpty() && nowMs >= m_Queue[0].m_iDeadlineMs) m_Queue.RemoveOrdered(0);
		return !m_Queue.IsEmpty();
	}

	void Close()
	{
		m_bClosed = true;
		m_Queue.Clear();
		m_Route = null;
		m_Lifecycle = null;
		m_SourceOwner = null;
	}
}
