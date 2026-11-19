//! TEST_ONLY body with one exact-tick SERVER budget and a bounded V1 codec payload.
class RICON_MediaWireRfV2
{
	static const int HEADER_VALUES = 8;
	static const int MAX_BODY_VALUES = HEADER_VALUES + RICON_MediaWireV1.MAX_PAYLOAD_BYTES;
	protected static const int MIN_POWER_CDBM = -30000;
	protected static const int MAX_POWER_CDBM = 10000;
	static const int MAX_BANDWIDTH_HZ = 10000000;

	//! shortcut: only a point capture at the budget's authority tick; use qualified RF history for intervals.
	static bool Matches(RICON_MediaEgressItem item, RICON_TemporalRfBudget budget, string expectedTimebaseId)
	{
		if (!item || !item.m_Route || !item.m_Route.IsValid() || !item.m_Block || !budget) return false;
		if (!RICON_TemporalSnapshots.ValidKey(expectedTimebaseId) || budget.m_sTimebaseId != expectedTimebaseId) return false;
		if (!ValidStamp(budget.m_iLinkId, budget.m_iTickMs, budget.m_iModelMs, budget.m_iModelSubUs)) return false;
		RICON_QueuedMediaBlock block = item.m_Block;
		if (block.m_RfHistory || block.m_CaptureEnvelope) return false;
		if (budget.m_RxEnvironment || (block.m_RfBudget && block.m_RfBudget.m_RxEnvironment)) return false;
		if (!item.m_Route.m_Source.Same(block.m_Binding)) return false;
		if (!RICON_MediaWireV1.ValidRange(block.m_iSequence, block.m_iFirstSampleIndex, block.m_iSampleCount)) return false;
		if (block.m_iCaptureLowerMs != budget.m_iTickMs || block.m_iCaptureUpperMs != budget.m_iTickMs) return false;
		if (!ValidPowerDbm(budget.m_fRxPowerDbm) || !ValidPowerDbm(budget.m_fNoiseDbm)) return false;
		float bandwidth = budget.m_fBandwidthHz;
		return bandwidth >= 1.0 && bandwidth <= MAX_BANDWIDTH_HZ && bandwidth == Math.Floor(bandwidth);
	}

	static array<int> EncodeBody(RICON_MediaEgressItem item, RICON_TemporalRfBudget budget, string expectedTimebaseId)
	{
		if (!Matches(item, budget, expectedTimebaseId)) return null;
		RICON_QueuedMediaBlock block = item.m_Block;
		if (!block.m_Bytes) return null;
		int count = block.m_Bytes.Count();
		if (count < 1 || count > RICON_MediaWireV1.MAX_PAYLOAD_BYTES) return null;
		array<int> body = {};
		body.Reserve(HEADER_VALUES + count);
		body.Insert(block.m_iSampleCount);
		body.Insert(budget.m_iLinkId);
		body.Insert(budget.m_iTickMs);
		body.Insert(budget.m_iModelMs);
		body.Insert(budget.m_iModelSubUs);
		body.Insert(QuantizePower(budget.m_fRxPowerDbm));
		body.Insert(QuantizePower(budget.m_fNoiseDbm));
		int bandwidthHz = budget.m_fBandwidthHz;
		body.Insert(bandwidthHz);
		foreach (int value : block.m_Bytes)
		{
			if (value < 0 || value > 255) return null;
			body.Insert(value);
		}
		return body;
	}

	//! The caller supplies the pinned timebase after admitting the exact route.
	static array<int> DecodeBody(int sequence, int firstSampleIndex, array<int> body, out int sampleCount,
		out RICON_TemporalRfBudget budget)
	{
		sampleCount = 0;
		budget = null;
		if (!body || body.Count() <= HEADER_VALUES || body.Count() > MAX_BODY_VALUES) return null;
		if (!RICON_MediaWireV1.ValidRange(sequence, firstSampleIndex, body[0])) return null;
		if (!ValidStamp(body[1], body[2], body[3], body[4])) return null;
		if (!ValidWirePower(body[5]) || !ValidWirePower(body[6]) || body[7] < 1 || body[7] > MAX_BANDWIDTH_HZ) return null;
		array<int> payload = {};
		payload.Reserve(body.Count() - HEADER_VALUES);
		for (int index = HEADER_VALUES; index < body.Count(); index++)
		{
			int value = body[index];
			if (value < 0 || value > 255) return null;
			payload.Insert(value);
		}
		budget = new RICON_TemporalRfBudget();
		budget.m_iLinkId = body[1];
		budget.m_iTickMs = body[2];
		budget.m_iModelMs = body[3];
		budget.m_iModelSubUs = body[4];
		budget.m_fRxPowerDbm = body[5] / 100.0;
		budget.m_fNoiseDbm = body[6] / 100.0;
		budget.m_fBandwidthHz = body[7];
		sampleCount = body[0];
		return payload;
	}

	static bool ValidStamp(int linkId, int tickMs, int modelMs, int modelSubUs)
	{
		return linkId > 0 && linkId < int.MAX && tickMs >= 0 && tickMs < int.MAX
			&& modelMs >= 0 && modelMs < int.MAX && modelSubUs >= 0 && modelSubUs < 1000;
	}

	static bool ValidPowerDbm(float power)
	{
		return power >= MIN_POWER_CDBM / 100.0 && power <= MAX_POWER_CDBM / 100.0;
	}

	static bool ValidWirePower(int power)
	{
		return power >= MIN_POWER_CDBM && power <= MAX_POWER_CDBM;
	}

	static int QuantizePower(float power)
	{
		int magnitude = Math.Floor(Math.AbsFloat(power) * 100.0 + 0.5);
		if (power < 0.0) return -magnitude;
		return magnitude;
	}
}
