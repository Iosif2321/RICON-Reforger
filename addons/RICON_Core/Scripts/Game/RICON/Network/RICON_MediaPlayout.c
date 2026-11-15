class RICON_MediaPlayoutDecision
{
	string m_sKind;
	int m_iSequence, m_iFirstSampleIndex, m_iSampleCount, m_iSlotCount;
	int m_iReleaseAtMs, m_iTargetMs, m_iDecidedAtMs;
	int m_iWireExpiryMs = -1;
	ref RICON_QueuedMediaBlock m_Block;
	ref RICON_MediaOutputPermit m_OutputPermit;
}

class RICON_MediaPlayoutEntry
{
	int m_iLocalExpiryMs;
	ref RICON_QueuedMediaBlock m_Block;
}

//! One V1 route with fixed-size frames and a caller-provided local playout anchor.
class RICON_MediaPlayout
{
	protected ref RICON_MediaEgressRoute m_Route;
	protected ref map<int, ref RICON_MediaPlayoutEntry> m_Pending = new map<int, ref RICON_MediaPlayoutEntry>();
	protected int m_iNextSequence, m_iNextSample, m_iNextDueMs;
	protected int m_iFrameSamples, m_iFrameDurationMs, m_iCapacity, m_iHorizonFrames;
	protected int m_iProcessingLeadMs;
	protected int m_iLastNowMs = -1;
	protected bool m_bClosed;

	bool Configure(RICON_MediaEgressRoute route, int firstSequence, int firstSampleIndex, int frameSamples,
		int frameDurationMs, int firstDueMs, int capacity, int horizonFrames, int processingLeadMs = 0)
	{
		if (m_bClosed || m_Route || !route || !route.IsValid()) return false;
		if (frameSamples < 1 || frameSamples > RICON_MediaWireV1.MAX_SAMPLE_COUNT || frameDurationMs < 1 || frameDurationMs > 1000) return false;
		if (capacity < 1 || capacity > 64 || horizonFrames < 1 || horizonFrames > 64) return false;
		if (firstSequence < 0 || firstSequence >= int.MAX || firstSampleIndex < 0 || firstDueMs < 0) return false;
		if (firstSampleIndex > int.MAX - frameSamples || firstDueMs > int.MAX - frameDurationMs) return false;
		if (processingLeadMs < 0 || processingLeadMs > 1000 || processingLeadMs > firstDueMs) return false;
		m_Route = route.CloneRoute();
		m_iNextSequence = firstSequence;
		m_iNextSample = firstSampleIndex;
		m_iNextDueMs = firstDueMs;
		m_iFrameSamples = frameSamples;
		m_iFrameDurationMs = frameDurationMs;
		m_iCapacity = capacity;
		m_iHorizonFrames = horizonFrames;
		m_iProcessingLeadMs = processingLeadMs;
		return true;
	}

	bool IsRoute(RICON_MediaEgressRoute route)
	{
		return !m_bClosed && m_Route && m_Route.Same(route);
	}

	bool IsAtCutover(int firstSequence, int firstSampleIndex)
	{
		return m_iNextSequence == firstSequence && m_iNextSample == firstSampleIndex;
	}

	bool NeedsCatchup(int nowMs)
	{
		return !m_bClosed && m_Route && nowMs >= m_iLastNowMs && nowMs >= LateBoundary();
	}

	string Offer(RICON_MediaEgressRoute route, RICON_QueuedMediaBlock block, int localExpiryMs, int nowMs, int arrivalMs = -1)
	{
		if (m_bClosed) return "CLOSED";
		if (!IsRoute(route)) return "WRONG_ROUTE";
		if (!block || !m_Route.m_Source.Same(block.m_Binding)) return "INVALID_BLOCK";
		int sequence = block.m_iSequence;
		if (sequence < 0 || sequence >= int.MAX || block.m_iSampleCount != m_iFrameSamples) return "INVALID_BLOCK";
		if (sequence < m_iNextSequence) return "BEFORE_CURSOR";
		int offset = sequence - m_iNextSequence;
		if (offset >= m_iHorizonFrames) return "OUTSIDE_HORIZON";

		// The horizon and frame limits bound both products before the addition guards.
		int sampleDelta = offset * m_iFrameSamples;
		int timeDelta = offset * m_iFrameDurationMs;
		if (m_iNextSample > int.MAX - m_iFrameSamples - sampleDelta || m_iNextDueMs > int.MAX - m_iFrameDurationMs - timeDelta)
			return "INVALID_BLOCK";
		if (block.m_iFirstSampleIndex != m_iNextSample + sampleDelta || !block.m_Bytes) return "INVALID_BLOCK";
		if (block.m_Bytes.Count() < 1 || block.m_Bytes.Count() > RICON_MediaWireV1.MAX_PAYLOAD_BYTES) return "INVALID_BLOCK";
		foreach (int value : block.m_Bytes)
		{
			if (value < 0 || value > 255) return "INVALID_BLOCK";
		}
		if (arrivalMs < -1 || arrivalMs > nowMs || !ObserveTime(nowMs)) return "BAD_TIME";
		int dueMs = m_iNextDueMs + timeDelta;
		int acceptedAt = nowMs;
		if (arrivalMs >= 0) acceptedAt = arrivalMs;
		// Время приёма доказывает своевременность пакета, но не возвращает уже упущенный игровой слот.
		if (nowMs >= dueMs || acceptedAt >= dueMs - m_iProcessingLeadMs) return "LATE";
		if (localExpiryMs <= dueMs) return "EXPIRED";
		RICON_MediaPlayoutEntry held = m_Pending.Get(sequence);
		if (held)
		{
			RICON_MediaCaptureEnvelope priorEnvelope = held.m_Block.m_CaptureEnvelope;
			if (priorEnvelope && !priorEnvelope.SameEnvelope(block.m_CaptureEnvelope)) return "IDENTITY_CONFLICT";
			if (!priorEnvelope && block.m_CaptureEnvelope) return "IDENTITY_CONFLICT";
			RICON_MediaRfHistoryEntry priorHistory = held.m_Block.m_RfHistory;
			if (priorHistory && !priorHistory.SameEntry(block.m_RfHistory)) return "IDENTITY_CONFLICT";
			if (!priorHistory && block.m_RfHistory) return "IDENTITY_CONFLICT";
			RICON_TemporalRfBudget priorBudget = held.m_Block.m_RfBudget;
			if (priorBudget && !priorBudget.Same(block.m_RfBudget)) return "IDENTITY_CONFLICT";
			if (!priorBudget && block.m_RfBudget) return "IDENTITY_CONFLICT";
			array<int> prior = held.m_Block.m_Bytes;
			if (prior.Count() != block.m_Bytes.Count()) return "IDENTITY_CONFLICT";
			for (int index = 0; index < prior.Count(); index++)
			{
				if (prior[index] != block.m_Bytes[index]) return "IDENTITY_CONFLICT";
			}
			return "DUPLICATE";
		}
		if (m_Pending.Count() >= m_iCapacity) return "FULL";

		RICON_QueuedMediaBlock copy = new RICON_QueuedMediaBlock();
		copy.m_iSequence = sequence;
		copy.m_iFirstSampleIndex = block.m_iFirstSampleIndex;
		copy.m_iSampleCount = m_iFrameSamples;
		copy.m_iCaptureLowerMs = block.m_iCaptureLowerMs;
		copy.m_iCaptureUpperMs = block.m_iCaptureUpperMs;
		copy.m_Binding = m_Route.m_Source.CloneBinding();
		if (block.m_RfBudget) copy.m_RfBudget = block.m_RfBudget.CloneBudget();
		if (block.m_CaptureEnvelope) copy.m_CaptureEnvelope = block.m_CaptureEnvelope.CloneEnvelope();
		if (block.m_RfHistory) copy.m_RfHistory = block.m_RfHistory.CloneEntry();
		foreach (int value : block.m_Bytes) copy.m_Bytes.Insert(value);
		RICON_MediaPlayoutEntry entry = new RICON_MediaPlayoutEntry();
		entry.m_iLocalExpiryMs = localExpiryMs;
		entry.m_Block = copy;
		m_Pending.Set(sequence, entry);
		return "ACCEPTED";
	}

	//! Decide one source slot for processing, or report a late range. Sink commit is separate.
	RICON_MediaPlayoutDecision Poll(int nowMs)
	{
		int releaseAt = m_iNextDueMs - m_iProcessingLeadMs;
		if (m_bClosed || !m_Route || !ObserveTime(nowMs) || nowMs < releaseAt) return null;
		RICON_MediaPlayoutDecision decision = new RICON_MediaPlayoutDecision();
		decision.m_iSequence = m_iNextSequence;
		decision.m_iFirstSampleIndex = m_iNextSample;
		decision.m_iReleaseAtMs = releaseAt;
		decision.m_iTargetMs = m_iNextDueMs;
		decision.m_iDecidedAtMs = nowMs;
		int lateBoundary = LateBoundary();
		if (nowMs >= lateBoundary)
		{
			int skippedSlots = (nowMs - lateBoundary) / m_iFrameDurationMs + 1;
			int sampleLimit = (int.MAX - m_iNextSample) / m_iFrameSamples;
			int sequenceLimit = int.MAX - m_iNextSequence;
			int timeLimit = (int.MAX - m_iNextDueMs) / m_iFrameDurationMs;
			if (skippedSlots > sampleLimit) skippedSlots = sampleLimit;
			if (skippedSlots > sequenceLimit) skippedSlots = sequenceLimit;
			if (skippedSlots > timeLimit) skippedSlots = timeLimit;
			decision.m_sKind = "SCHEDULER_LATE";
			decision.m_iSlotCount = skippedSlots;
			DiscardBefore(m_iNextSequence + skippedSlots);
		}
		else
		{
			decision.m_iSlotCount = 1;
			RICON_MediaPlayoutEntry entry = m_Pending.Get(m_iNextSequence);
			if (entry) decision.m_iWireExpiryMs = entry.m_iLocalExpiryMs;
			if (!entry) decision.m_sKind = "GAP";
			else if (nowMs >= entry.m_iLocalExpiryMs) decision.m_sKind = "EXPIRED";
			else
			{
				decision.m_sKind = "PLAY";
				decision.m_Block = entry.m_Block;
			}
			if (entry) m_Pending.Remove(m_iNextSequence);
		}
		decision.m_iSampleCount = decision.m_iSlotCount * m_iFrameSamples;
		m_iNextSequence += decision.m_iSlotCount;
		m_iNextSample += decision.m_iSampleCount;
		m_iNextDueMs += decision.m_iSlotCount * m_iFrameDurationMs;
		if (m_iNextSequence >= int.MAX || m_iNextSample > int.MAX - m_iFrameSamples || m_iNextDueMs > int.MAX - m_iFrameDurationMs) Close();
		return decision;
	}

	protected int LateBoundary()
	{
		if (m_iProcessingLeadMs > 0) return m_iNextDueMs;
		return m_iNextDueMs + m_iFrameDurationMs;
	}

	void Close()
	{
		m_bClosed = true;
		m_Route = null;
		m_Pending.Clear();
	}

	protected void DiscardBefore(int sequenceFloor)
	{
		array<int> staleSequences = {};
		foreach (int sequence, RICON_MediaPlayoutEntry entry : m_Pending)
		{
			if (sequence < sequenceFloor) staleSequences.Insert(sequence);
		}
		foreach (int staleSequence : staleSequences) m_Pending.Remove(staleSequence);
	}

	protected bool ObserveTime(int nowMs)
	{
		if (nowMs < 0 || nowMs < m_iLastNowMs) return false;
		m_iLastNowMs = nowMs;
		return true;
	}
}
