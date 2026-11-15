//! A qualified capture envelope and one completed RF interval, on the route's negotiated timebase.
class RICON_MediaWireRfV3
{
	static const int HEADER_VALUES = RICON_MediaWireRfV2.HEADER_VALUES + 7;
	static const int MAX_BODY_VALUES = HEADER_VALUES + RICON_MediaWireV1.MAX_PAYLOAD_BYTES;

	//! Validate detached server metadata before touching the payload.
	static bool Matches(RICON_MediaEgressItem item, string timebaseId)
	{
		if (!item || !item.m_Route || !item.m_Route.IsValid() || !item.m_Block) return false;
		if (!RICON_TemporalSnapshots.ValidKey(timebaseId)) return false;
		RICON_QueuedMediaBlock block = item.m_Block;
		RICON_MediaCaptureEnvelope envelope = block.m_CaptureEnvelope;
		RICON_MediaRfHistoryEntry history = block.m_RfHistory;
		if (!envelope || !history || !history.m_Budget || !item.m_Route.m_Source.Same(block.m_Binding)) return false;
		if (!RICON_MediaWireV1.ValidRange(block.m_iSequence, block.m_iFirstSampleIndex, block.m_iSampleCount)) return false;
		if (!envelope.Matches(block.m_Binding, block.m_iFirstSampleIndex, block.m_iSampleCount)) return false;

		RICON_TemporalRfBudget budget = history.m_Budget;
		if (envelope.m_sTimebaseId != timebaseId || budget.m_sTimebaseId != timebaseId) return false;
		if (block.m_RfBudget && !budget.Same(block.m_RfBudget)) return false;
		if (!ValidCoverage(envelope, history.m_iValidFromMs, history.m_iValidUntilMs, budget.m_iTickMs)) return false;
		int upperCeilingMs = envelope.m_iUpperMs;
		if (envelope.m_iUpperSubUs > 0) upperCeilingMs++;
		if (block.m_iCaptureLowerMs != envelope.m_iLowerMs || block.m_iCaptureUpperMs != upperCeilingMs) return false;
		if (!RICON_MediaWireRfV2.ValidStamp(budget.m_iLinkId, budget.m_iTickMs, budget.m_iModelMs, budget.m_iModelSubUs)) return false;
		if (!RICON_MediaWireRfV2.ValidPowerDbm(budget.m_fRxPowerDbm) || !RICON_MediaWireRfV2.ValidPowerDbm(budget.m_fNoiseDbm)) return false;
		float bandwidth = budget.m_fBandwidthHz;
		return bandwidth >= 1.0 && bandwidth <= RICON_MediaWireRfV2.MAX_BANDWIDTH_HZ && bandwidth == Math.Floor(bandwidth);
	}

	static array<int> EncodeBody(RICON_MediaEgressItem item, string timebaseId)
	{
		if (!Matches(item, timebaseId)) return null;
		RICON_QueuedMediaBlock block = item.m_Block;
		if (!block.m_Bytes) return null;
		int count = block.m_Bytes.Count();
		if (count < 1 || count > RICON_MediaWireV1.MAX_PAYLOAD_BYTES) return null;

		RICON_MediaCaptureEnvelope envelope = block.m_CaptureEnvelope;
		RICON_MediaRfHistoryEntry history = block.m_RfHistory;
		RICON_TemporalRfBudget budget = history.m_Budget;
		array<int> body = {};
		body.Reserve(HEADER_VALUES + count);
		body.Insert(block.m_iSampleCount);
		body.Insert(budget.m_iLinkId);
		body.Insert(budget.m_iTickMs);
		body.Insert(budget.m_iModelMs);
		body.Insert(budget.m_iModelSubUs);
		body.Insert(RICON_MediaWireRfV2.QuantizePower(budget.m_fRxPowerDbm));
		body.Insert(RICON_MediaWireRfV2.QuantizePower(budget.m_fNoiseDbm));
		int bandwidthHz = budget.m_fBandwidthHz;
		body.Insert(bandwidthHz);
		body.Insert(envelope.m_iLowerMs);
		body.Insert(envelope.m_iLowerSubUs);
		body.Insert(envelope.m_iUpperMs);
		body.Insert(envelope.m_iUpperSubUs);
		body.Insert(history.m_iValidFromMs);
		body.Insert(history.m_iValidUntilMs);
		body.Insert(envelope.m_iMappingRevision);
		foreach (int value : block.m_Bytes)
		{
			if (value < 0 || value > 255) return null;
			body.Insert(value);
		}
		return body;
	}

	//! The receiver attaches its admitted stream and timebase after successful decoding.
	static array<int> DecodeBody(int sequence, int firstSampleIndex, array<int> body, out int sampleCount,
		out RICON_MediaCaptureEnvelope envelope, out RICON_MediaRfHistoryEntry history)
	{
		sampleCount = 0;
		envelope = null;
		history = null;
		if (!body || body.Count() <= HEADER_VALUES || body.Count() > MAX_BODY_VALUES) return null;
		if (!RICON_MediaWireV1.ValidRange(sequence, firstSampleIndex, body[0])) return null;
		if (!RICON_MediaWireRfV2.ValidStamp(body[1], body[2], body[3], body[4])) return null;
		if (!RICON_MediaWireRfV2.ValidWirePower(body[5]) || !RICON_MediaWireRfV2.ValidWirePower(body[6])) return null;
		if (body[7] < 1 || body[7] > RICON_MediaWireRfV2.MAX_BANDWIDTH_HZ) return null;

		RICON_MediaCaptureEnvelope parsedEnvelope = new RICON_MediaCaptureEnvelope();
		parsedEnvelope.m_iFirstSampleIndex = firstSampleIndex;
		parsedEnvelope.m_iSampleCount = body[0];
		parsedEnvelope.m_iLowerMs = body[8];
		parsedEnvelope.m_iLowerSubUs = body[9];
		parsedEnvelope.m_iUpperMs = body[10];
		parsedEnvelope.m_iUpperSubUs = body[11];
		parsedEnvelope.m_iMappingRevision = body[14];
		if (!ValidCoverage(parsedEnvelope, body[12], body[13], body[2])) return null;

		array<int> payload = {};
		payload.Reserve(body.Count() - HEADER_VALUES);
		for (int index = HEADER_VALUES; index < body.Count(); index++)
		{
			int value = body[index];
			if (value < 0 || value > 255) return null;
			payload.Insert(value);
		}

		RICON_TemporalRfBudget budget = new RICON_TemporalRfBudget();
		budget.m_iLinkId = body[1];
		budget.m_iTickMs = body[2];
		budget.m_iModelMs = body[3];
		budget.m_iModelSubUs = body[4];
		budget.m_fRxPowerDbm = body[5] / 100.0;
		budget.m_fNoiseDbm = body[6] / 100.0;
		budget.m_fBandwidthHz = body[7];
		history = new RICON_MediaRfHistoryEntry();
		history.m_iValidFromMs = body[12];
		history.m_iValidUntilMs = body[13];
		history.m_Budget = budget;
		envelope = parsedEnvelope;
		sampleCount = parsedEnvelope.m_iSampleCount;
		return payload;
	}

	protected static bool ValidCoverage(RICON_MediaCaptureEnvelope envelope, int fromMs, int untilMs, int budgetTickMs)
	{
		if (envelope.m_iMappingRevision <= 0 || budgetTickMs < fromMs || budgetTickMs >= untilMs) return false;
		if (envelope.m_iLowerMs == envelope.m_iUpperMs && envelope.m_iLowerSubUs >= envelope.m_iUpperSubUs) return false;
		return RICON_TemporalClock.ContainsInterval(envelope.m_iLowerMs, envelope.m_iLowerSubUs, envelope.m_iUpperMs,
			envelope.m_iUpperSubUs, fromMs, untilMs);
	}
}
