//! Один ожидающий исходный кадр фиксированного маршрута. Владелец сервера передаёт блок из своей очереди и решение локального приёмника.
class RICON_RelayInputFrame
{
	protected ref RICON_MediaEgressRoute m_Route;
	protected ref RICON_QueuedMediaBlock m_Pending;
	protected int m_iLastSequence = -1;

	//! shortcut: дробные границы и неопределённость часов требуют отдельного профиля ретранслятора.
	static bool MatchesCapture(RICON_QueuedMediaBlock block, string mappedTimebaseId)
	{
		if (!block || block.m_RfBudget || block.m_RfHistory
			|| block.m_iSampleCount != RICON_MediaPcmProfile.FRAME_SAMPLES
			|| block.m_iCaptureLowerMs < 0 || block.m_iCaptureUpperMs <= block.m_iCaptureLowerMs
			|| block.m_iCaptureUpperMs - block.m_iCaptureLowerMs != RICON_MediaPcmProfile.FRAME_DURATION_MS) return false;
		RICON_MediaCaptureEnvelope envelope = block.m_CaptureEnvelope;
		if (mappedTimebaseId.IsEmpty()) return !envelope;
		return envelope && envelope.m_sTimebaseId == mappedTimebaseId && envelope.m_iMappingRevision > 0
			&& envelope.Matches(block.m_Binding, block.m_iFirstSampleIndex, block.m_iSampleCount)
			&& envelope.m_iLowerMs == block.m_iCaptureLowerMs && envelope.m_iUpperMs == block.m_iCaptureUpperMs
			&& envelope.m_iLowerSubUs == 0 && envelope.m_iUpperSubUs == 0;
	}

	bool Store(RICON_MediaEgressRoute route, RICON_QueuedMediaBlock block, string mappedTimebaseId = "")
	{
		if (m_Pending || !route || !route.IsValid() || !block || !route.m_Source.Same(block.m_Binding)
			|| route.m_Source.m_sProfileId != RICON_MediaPcmProfile.PROFILE_ID || (m_Route && !m_Route.Same(route))) return false;
		if (!MatchesCapture(block, mappedTimebaseId) || block.m_iSequence <= m_iLastSequence
			|| !RICON_MediaWireV1.ValidRange(block.m_iSequence, block.m_iFirstSampleIndex, block.m_iSampleCount)
			|| !block.m_Bytes || block.m_Bytes.Count() != RICON_MediaPcmProfile.PAYLOAD_BYTES) return false;

		RICON_QueuedMediaBlock snapshot = new RICON_QueuedMediaBlock();
		foreach (int value : block.m_Bytes)
		{
			if (value < 0 || value > 255) return false;
			snapshot.m_Bytes.Insert(value);
		}
		snapshot.m_Binding = block.m_Binding.CloneBinding();
		snapshot.m_iSequence = block.m_iSequence;
		snapshot.m_iFirstSampleIndex = block.m_iFirstSampleIndex;
		snapshot.m_iSampleCount = block.m_iSampleCount;
		snapshot.m_iCaptureLowerMs = block.m_iCaptureLowerMs;
		snapshot.m_iCaptureUpperMs = block.m_iCaptureUpperMs;
		if (block.m_CaptureEnvelope) snapshot.m_CaptureEnvelope = block.m_CaptureEnvelope.CloneEnvelope();
		if (!m_Route) m_Route = route.CloneRoute();
		m_Pending = snapshot;
		m_iLastSequence = snapshot.m_iSequence;
		return true;
	}

	//! Локальное решение V1 подтверждает допуск, байты и расписание, но метаданные берутся только из снимка сервера.
	RICON_QueuedMediaBlock Read(RICON_MediaEgressRoute route, RICON_MediaPlayoutDecision decision, int nowMs)
	{
		if (!m_Pending || !m_Route.Same(route) || !decision || decision.m_sKind != "PLAY" || decision.m_iSlotCount != 1
			|| decision.m_iDecidedAtMs < m_Pending.m_iCaptureUpperMs || decision.m_iDecidedAtMs > nowMs || decision.m_iWireExpiryMs <= nowMs
			|| !decision.m_OutputPermit || decision.m_OutputPermit.Type() != RICON_MediaOutputPermit
			|| !decision.m_OutputPermit.MatchesRoute(m_Route)) return null;
		if (decision.m_iSequence != m_Pending.m_iSequence || decision.m_iFirstSampleIndex != m_Pending.m_iFirstSampleIndex
			|| decision.m_iSampleCount != m_Pending.m_iSampleCount) return null;
		RICON_QueuedMediaBlock played = decision.m_Block;
		if (!played || played.m_CaptureEnvelope || played.m_RfBudget || played.m_RfHistory
			|| !m_Pending.m_Binding.Same(played.m_Binding) || played.m_iSequence != decision.m_iSequence
			|| played.m_iFirstSampleIndex != decision.m_iFirstSampleIndex || played.m_iSampleCount != decision.m_iSampleCount
			|| !played.m_Bytes || played.m_Bytes.Count() != m_Pending.m_Bytes.Count()) return null;
		for (int index = 0; index < played.m_Bytes.Count(); index++)
		{
			if (played.m_Bytes[index] != m_Pending.m_Bytes[index]) return null;
		}

		ref RICON_QueuedMediaBlock snapshot = m_Pending;
		Clear();
		return snapshot;
	}

	//! Освобождает слот без повторного допуска прежнего номера. Для другого маршрута нужен новый экземпляр.
	void Clear() { m_Pending = null; }
}
