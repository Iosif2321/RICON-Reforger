enum RICON_EMediaWindowDecision
{
	INVALID_WINDOW,
	CLOSED,
	WRONG_GENERATION,
	INVALID_SEQUENCE,
	RETIRED,
	SEQUENCE_GAP,
	INVALID_PAYLOAD,
	INVALID_SAMPLE_RANGE,
	DUPLICATE,
	IDENTITY_CONFLICT,
	ACCEPTED
}

class RICON_MediaWindowBlock
{
	int m_iFirstSampleIndex;
	int m_iSampleCount;
	string m_sContextKey;
	ref array<int> m_Bytes = {};
}

//! One window per trusted emitting stream. Before Accept, the owner checks binding, representation and deadline.
class RICON_MediaWindow
{
	protected int m_iGeneration;
	protected int m_iWidth;
	protected int m_iMaxPayloadBytes;
	protected int m_iFloor;
	protected int m_iHigh;
	protected bool m_bClosed;
	protected ref map<int, ref RICON_MediaWindowBlock> m_Payloads = new map<int, ref RICON_MediaWindowBlock>();

	bool Initialize(int generation, int firstSequence, int width, int maxPayloadBytes)
	{
		if (m_bClosed || m_iGeneration != 0 || generation <= 0 || firstSequence < 0
			|| firstSequence >= int.MAX || width <= 0 || maxPayloadBytes <= 0)
			return false;
		m_iGeneration = generation;
		m_iWidth = width;
		m_iMaxPayloadBytes = maxPayloadBytes;
		m_iFloor = firstSequence;
		m_iHigh = firstSequence - 1;
		return true;
	}

	RICON_EMediaWindowDecision Accept(int generation, int sequence, array<int> payload, int firstSampleIndex, int sampleCount,
		string contextKey = "")
	{
		if (m_bClosed) return RICON_EMediaWindowDecision.CLOSED;
		if (m_iGeneration == 0) return RICON_EMediaWindowDecision.INVALID_WINDOW;
		if (generation != m_iGeneration) return RICON_EMediaWindowDecision.WRONG_GENERATION;
		if (sequence < 0 || sequence >= int.MAX) return RICON_EMediaWindowDecision.INVALID_SEQUENCE;
		if (sequence < m_iFloor) return RICON_EMediaWindowDecision.RETIRED;
		if (sequence > m_iHigh && sequence - m_iHigh > m_iWidth) return RICON_EMediaWindowDecision.SEQUENCE_GAP;
		if (firstSampleIndex < 0 || sampleCount <= 0 || firstSampleIndex > int.MAX - sampleCount)
			return RICON_EMediaWindowDecision.INVALID_SAMPLE_RANGE;
		// V3 has at most 14 signed int32 metadata fields, each with a separator.
		if (!payload || payload.Count() == 0 || payload.Count() > m_iMaxPayloadBytes || contextKey.Length() > 168)
			return RICON_EMediaWindowDecision.INVALID_PAYLOAD;
		foreach (int value : payload)
		{
			if (value < 0 || value > 255) return RICON_EMediaWindowDecision.INVALID_PAYLOAD;
		}

		RICON_MediaWindowBlock held = m_Payloads.Get(sequence);
		if (held)
		{
			if (held.m_iFirstSampleIndex != firstSampleIndex || held.m_iSampleCount != sampleCount
				|| held.m_Bytes.Count() != payload.Count() || held.m_sContextKey != contextKey) return RICON_EMediaWindowDecision.IDENTITY_CONFLICT;
			for (int i = 0; i < held.m_Bytes.Count(); i++)
			{
				if (held.m_Bytes[i] != payload[i]) return RICON_EMediaWindowDecision.IDENTITY_CONFLICT;
			}
			return RICON_EMediaWindowDecision.DUPLICATE;
		}

		if (sequence > m_iHigh)
		{
			m_iHigh = sequence;
			AdvanceFloor(m_iHigh - m_iWidth + 1);
		}

		RICON_MediaWindowBlock block = new RICON_MediaWindowBlock();
		block.m_iFirstSampleIndex = firstSampleIndex;
		block.m_iSampleCount = sampleCount;
		block.m_sContextKey = contextKey;
		foreach (int byteValue : payload) block.m_Bytes.Insert(byteValue);
		m_Payloads.Set(sequence, block);
		return RICON_EMediaWindowDecision.ACCEPTED;
	}

	//! A trusted presentation owner has closed these positions, including any missing packets.
	void RetireThrough(int generation, int sequence)
	{
		if (m_bClosed || m_iGeneration <= 0 || m_iGeneration != generation || sequence < m_iFloor || sequence >= int.MAX)
			return;
		if (m_iHigh < sequence) m_iHigh = sequence;
		AdvanceFloor(sequence + 1);
	}

	protected void AdvanceFloor(int nextFloor)
	{
		if (nextFloor <= m_iFloor) return;
		m_iFloor = nextFloor;
		// shortcut: indexed map scans can be quadratic; measure small profiles before selecting a ring.
		for (int index = m_Payloads.Count() - 1; index >= 0; index--)
		{
			int key = m_Payloads.GetKey(index);
			if (key < m_iFloor) m_Payloads.Remove(key);
		}
	}

	void Close()
	{
		m_bClosed = true;
		m_Payloads.Clear();
	}
}
