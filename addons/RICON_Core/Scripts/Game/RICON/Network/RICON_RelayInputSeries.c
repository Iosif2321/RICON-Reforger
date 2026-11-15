//! Локальное окно PCM на 48 кГц и его зависимости на исходной шкале 8 кГц.
class RICON_RelayInputSeriesWindow
{
	ref array<int> m_aSamples;
	ref RICON_RelayPcmOrigin m_Origin;
	int m_iFirstSampleIndex;
	int m_iOldestCaptureMs;
	int m_iDeadlineMs;
}

//! Один интерполятор фиксированного маршрута. Допуск кадра и оценка RF остаются у серверного владельца.
class RICON_RelayInputSeries
{
	protected ref RICON_MediaEgressRoute m_Route;
	protected ref RICON_PcmResampler8To48 m_Resampler;
	protected ref RICON_RelayPcmOrigin m_Previous;
	protected int m_iNextSampleIndex;
	protected int m_iLastNowMs = -1;
	protected string m_sMappedTimebaseId;
	protected bool m_bClosed;

	bool Configure(RICON_MediaEgressRoute route, int firstSourceSampleIndex, string mappedTimebaseId = "")
	{
		if (m_bClosed || m_Route || !route || !route.IsValid()
			|| route.m_Source.m_sProfileId != RICON_MediaPcmProfile.PROFILE_ID) return false;
		if (!mappedTimebaseId.IsEmpty() && !RICON_TemporalSnapshots.ValidKey(mappedTimebaseId)) return false;
		RICON_PcmResampler8To48 resampler = new RICON_PcmResampler8To48();
		if (!resampler.Reset(firstSourceSampleIndex)) return false;
		m_Route = route.CloneRoute();
		m_Resampler = resampler;
		m_iNextSampleIndex = firstSourceSampleIndex;
		m_sMappedTimebaseId = mappedTimebaseId;
		return true;
	}

	//! После разрыва или истечения срока для продолжения требуется новое поколение маршрута.
	string Process(RICON_MediaEgressRoute route, RICON_QueuedMediaBlock block, int nowMs, int deadlineMs,
		out RICON_RelayInputSeriesWindow window)
	{
		window = null;
		if (!m_Route) return "CLOSED";
		if (nowMs < 0 || nowMs < m_iLastNowMs) return "BAD_TIME";
		if (!m_Route.Same(route) || (block && !m_Route.m_Source.Same(block.m_Binding))) return "WRONG_ROUTE";
		if (!RICON_RelayInputFrame.MatchesCapture(block, m_sMappedTimebaseId)
			|| !RICON_MediaWireV1.ValidRange(block.m_iSequence, block.m_iFirstSampleIndex, block.m_iSampleCount)
			|| block.m_iCaptureUpperMs > nowMs) return "INVALID_INPUT_FRAME";
		if (m_Previous && block.m_iSequence <= m_Previous.m_iSequence) return "RETIRED";
		array<int> samples = RICON_MediaPcmProfile.DecodeBytes(block.m_Bytes);
		if (!samples) return "INVALID_INPUT_FRAME";
		if (block.m_iFirstSampleIndex != m_iNextSampleIndex || (m_Previous
			&& (block.m_iSequence != m_Previous.m_iSequence + 1 || block.m_iCaptureLowerMs != m_Previous.m_iCaptureUntilMs
				|| (block.m_CaptureEnvelope && block.m_CaptureEnvelope.m_iMappingRevision != m_Previous.m_CaptureEnvelope.m_iMappingRevision))))
		{
			Close();
			return "GAP";
		}
		int effectiveDeadlineMs = deadlineMs;
		if (m_Previous) effectiveDeadlineMs = Math.Min(effectiveDeadlineMs, m_Previous.m_iDeadlineMs);
		if (nowMs >= effectiveDeadlineMs)
		{
			Close();
			return "EXPIRED";
		}

		int firstOutputIndex;
		array<int> output = m_Resampler.Process(block.m_iFirstSampleIndex, samples, firstOutputIndex);
		if (!output)
		{
			Close();
			return "CLOSED";
		}
		RICON_RelayPcmOrigin origin = new RICON_RelayPcmOrigin();
		origin.m_iSequence = block.m_iSequence;
		origin.m_iFirstSampleIndex = block.m_iFirstSampleIndex;
		origin.m_iSampleCount = block.m_iSampleCount;
		origin.m_iCaptureFromMs = block.m_iCaptureLowerMs;
		origin.m_iCaptureUntilMs = block.m_iCaptureUpperMs;
		origin.m_iRxReadyMs = nowMs;
		origin.m_iDeadlineMs = deadlineMs;
		if (block.m_CaptureEnvelope) origin.m_CaptureEnvelope = block.m_CaptureEnvelope.CloneEnvelope();
		ref RICON_RelayPcmOrigin previous = m_Previous;
		m_Previous = origin.CloneWindow();
		origin.m_bContinuousInput = true;

		window = new RICON_RelayInputSeriesWindow();
		window.m_aSamples = output;
		window.m_Origin = origin;
		window.m_iFirstSampleIndex = firstOutputIndex;
		window.m_iOldestCaptureMs = origin.m_iCaptureFromMs;
		window.m_iDeadlineMs = effectiveDeadlineMs;
		if (previous)
		{
			// История интерполятора помещается в один предыдущий 32-отсчётный блок.
			window.m_iOldestCaptureMs = previous.m_iCaptureFromMs;
			origin.m_PreviousInput = previous;
		}
		m_iLastNowMs = nowMs;
		m_iNextSampleIndex += block.m_iSampleCount;
		return "OK";
	}

	void Close()
	{
		m_bClosed = true;
		m_Route = null;
		m_Resampler = null;
		m_Previous = null;
	}
}

//! Очередь кадров станции отдельно от прямых копий. Голова хранится у владельца, хвост ждёт её завершения.
class RICON_RelayFrameReservation
{
	protected ref array<ref RICON_QueuedMediaBlock> m_aBlocks = {};
	protected ref array<int> m_aDeadlines = {};
	protected int m_iCapacity;
	protected int m_iNextSequence = -1;
	protected bool m_bClosed;

	bool Open(int capacity, int firstSequence)
	{
		if (m_iCapacity != 0 || m_bClosed || capacity < 1 || capacity > 16 || firstSequence < 0) return false;
		m_iCapacity = capacity;
		m_iNextSequence = firstSequence;
		return true;
	}

	int Count()
	{
		return m_aBlocks.Count();
	}

	protected int Occupied(bool headBusy)
	{
		int count = Count();
		if (headBusy) count++;
		return count;
	}

	int Room(bool headBusy)
	{
		if (m_bClosed || m_iCapacity <= 0) return 0;
		int room = m_iCapacity - Occupied(headBusy);
		if (room < 0) return 0;
		return room;
	}

	//! STORE — кадр должен занять свободную голову. HOLD — он ждёт в хвосте. Иначе серию продолжать нельзя.
	string Admit(RICON_QueuedMediaBlock block, int deadlineMs, bool headBusy)
	{
		if (m_bClosed || m_iCapacity <= 0 || m_iNextSequence < 0) return "CLOSED";
		if (!block || block.m_iSequence != m_iNextSequence + Occupied(headBusy)) return "GAP";
		if (Occupied(headBusy) >= m_iCapacity) return "OVERFLOW";
		if (!headBusy && Count() == 0) return "STORE";
		m_aBlocks.Insert(block);
		m_aDeadlines.Insert(deadlineMs);
		return "HOLD";
	}

	void CompleteHead()
	{
		if (m_iNextSequence >= 0) m_iNextSequence++;
	}

	RICON_QueuedMediaBlock TakeHeld(out int deadlineMs)
	{
		deadlineMs = 0;
		if (m_aBlocks.IsEmpty()) return null;
		ref RICON_QueuedMediaBlock block = m_aBlocks[0];
		deadlineMs = m_aDeadlines[0];
		m_aBlocks.RemoveOrdered(0);
		m_aDeadlines.RemoveOrdered(0);
		return block;
	}

	void Close()
	{
		m_bClosed = true;
		m_aBlocks.Clear();
		m_aDeadlines.Clear();
	}
}
