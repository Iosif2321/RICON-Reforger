//! Trusted provider's correlation of a capture frame to local and server monotonic clocks.
class RICON_MediaCaptureAnchor
{
	ref RICON_MediaStreamBinding m_Binding;
	string m_sTimebaseId;
	int m_iMappingRevision;
	int m_iFirstSampleIndex, m_iSampleRateHz;
	int m_iCaptureAnchorLocalMs, m_iCaptureAnchorSubUs;
	int m_iClockAnchorLocalMs, m_iClockAnchorServerMs;
	int m_iValidFromLocalMs, m_iValidUntilLocalMs;
	int m_iMaxMappingErrorUs, m_iMaxCaptureErrorUs, m_iMaxDriftPpm;

	bool SameAnchor(RICON_MediaCaptureAnchor other)
	{
		if (!other || !m_Binding || !m_Binding.Same(other.m_Binding)) return false;
		return m_sTimebaseId == other.m_sTimebaseId && m_iMappingRevision == other.m_iMappingRevision
			&& m_iFirstSampleIndex == other.m_iFirstSampleIndex && m_iSampleRateHz == other.m_iSampleRateHz
			&& m_iCaptureAnchorLocalMs == other.m_iCaptureAnchorLocalMs && m_iCaptureAnchorSubUs == other.m_iCaptureAnchorSubUs
			&& m_iClockAnchorLocalMs == other.m_iClockAnchorLocalMs && m_iClockAnchorServerMs == other.m_iClockAnchorServerMs
			&& m_iValidFromLocalMs == other.m_iValidFromLocalMs && m_iValidUntilLocalMs == other.m_iValidUntilLocalMs
			&& m_iMaxMappingErrorUs == other.m_iMaxMappingErrorUs && m_iMaxCaptureErrorUs == other.m_iMaxCaptureErrorUs
			&& m_iMaxDriftPpm == other.m_iMaxDriftPpm;
	}

	RICON_MediaCaptureAnchor CloneAnchor()
	{
		RICON_MediaCaptureAnchor copy = new RICON_MediaCaptureAnchor();
		if (m_Binding) copy.m_Binding = m_Binding.CloneBinding();
		copy.m_sTimebaseId = m_sTimebaseId;
		copy.m_iMappingRevision = m_iMappingRevision;
		copy.m_iFirstSampleIndex = m_iFirstSampleIndex;
		copy.m_iSampleRateHz = m_iSampleRateHz;
		copy.m_iCaptureAnchorLocalMs = m_iCaptureAnchorLocalMs;
		copy.m_iCaptureAnchorSubUs = m_iCaptureAnchorSubUs;
		copy.m_iClockAnchorLocalMs = m_iClockAnchorLocalMs;
		copy.m_iClockAnchorServerMs = m_iClockAnchorServerMs;
		copy.m_iValidFromLocalMs = m_iValidFromLocalMs;
		copy.m_iValidUntilLocalMs = m_iValidUntilLocalMs;
		copy.m_iMaxMappingErrorUs = m_iMaxMappingErrorUs;
		copy.m_iMaxCaptureErrorUs = m_iMaxCaptureErrorUs;
		copy.m_iMaxDriftPpm = m_iMaxDriftPpm;
		return copy;
	}
}

//! The closed server-time bound covers the first frame through the exclusive end of the block.
class RICON_MediaCaptureEnvelope
{
	ref RICON_MediaStreamBinding m_Binding;
	string m_sTimebaseId;
	int m_iMappingRevision;
	int m_iFirstSampleIndex, m_iSampleCount;
	int m_iLowerMs, m_iLowerSubUs, m_iUpperMs, m_iUpperSubUs;

	RICON_MediaCaptureEnvelope CloneEnvelope()
	{
		RICON_MediaCaptureEnvelope copy = new RICON_MediaCaptureEnvelope();
		if (m_Binding) copy.m_Binding = m_Binding.CloneBinding();
		copy.m_sTimebaseId = m_sTimebaseId;
		copy.m_iMappingRevision = m_iMappingRevision;
		copy.m_iFirstSampleIndex = m_iFirstSampleIndex;
		copy.m_iSampleCount = m_iSampleCount;
		copy.m_iLowerMs = m_iLowerMs;
		copy.m_iLowerSubUs = m_iLowerSubUs;
		copy.m_iUpperMs = m_iUpperMs;
		copy.m_iUpperSubUs = m_iUpperSubUs;
		return copy;
	}

	bool Matches(RICON_MediaStreamBinding binding, int firstSampleIndex, int sampleCount)
	{
		return m_Binding && m_Binding.Same(binding) && m_iFirstSampleIndex == firstSampleIndex && m_iSampleCount == sampleCount;
	}

	bool SameEnvelope(RICON_MediaCaptureEnvelope other)
	{
		if (!other || !Matches(other.m_Binding, other.m_iFirstSampleIndex, other.m_iSampleCount)) return false;
		if (m_sTimebaseId != other.m_sTimebaseId || m_iMappingRevision != other.m_iMappingRevision) return false;
		return m_iLowerMs == other.m_iLowerMs && m_iLowerSubUs == other.m_iLowerSubUs
			&& m_iUpperMs == other.m_iUpperMs && m_iUpperSubUs == other.m_iUpperSubUs;
	}
}

class RICON_MediaCaptureClock
{
	static bool ValidAnchor(RICON_MediaCaptureAnchor anchor, RICON_MediaStreamBinding source)
	{
		if (!anchor || !anchor.m_Binding || !source || !source.IsValid() || !anchor.m_Binding.Same(source)) return false;
		if (!RICON_TemporalSnapshots.ValidKey(anchor.m_sTimebaseId) || anchor.m_iMappingRevision <= 0) return false;
		if (anchor.m_iFirstSampleIndex < 0 || anchor.m_iFirstSampleIndex >= int.MAX) return false;
		if (anchor.m_iSampleRateHz < 8000 || anchor.m_iSampleRateHz > 192000) return false;
		if (anchor.m_iMaxDriftPpm < 0 || anchor.m_iMaxDriftPpm > 1000) return false;
		if (anchor.m_iMaxCaptureErrorUs < 0 || anchor.m_iMaxMappingErrorUs < 0) return false;
		if (anchor.m_iMaxCaptureErrorUs > int.MAX - anchor.m_iMaxMappingErrorUs) return false;
		int captureMs = anchor.m_iCaptureAnchorLocalMs;
		int captureSubUs = anchor.m_iCaptureAnchorSubUs;
		return LocalCovered(anchor, captureMs, captureSubUs, captureMs, captureSubUs, anchor.m_iMaxCaptureErrorUs);
	}

	//! Match the exact envelope and the conservative whole-millisecond bounds derived from that envelope.
	static bool MatchesMappedBounds(RICON_MediaCaptureAnchor anchor, RICON_MediaCaptureEnvelope envelope,
		int lowerMs, int upperMs)
	{
		if (!envelope || !envelope.m_Binding || lowerMs < 0 || upperMs <= lowerMs || lowerMs != envelope.m_iLowerMs) return false;
		ref RICON_MediaCaptureEnvelope expected = Map(anchor, envelope.m_Binding, envelope.m_iFirstSampleIndex, envelope.m_iSampleCount);
		if (!expected || !expected.SameEnvelope(envelope)) return false;
		int ceiling = expected.m_iUpperMs;
		if (expected.m_iUpperSubUs > 0)
		{
			if (ceiling == int.MAX) return false;
			ceiling++;
		}
		return upperMs == ceiling;
	}

	//! The anchor comes from trusted capture qualification. This arithmetic cannot establish its provenance.
	static RICON_MediaCaptureEnvelope Map(RICON_MediaCaptureAnchor anchor, RICON_MediaStreamBinding source,
		int firstSample, int sampleCount)
	{
		if (!ValidAnchor(anchor, source) || firstSample < anchor.m_iFirstSampleIndex) return null;
		if (sampleCount <= 0 || sampleCount > 48000 || firstSample > int.MAX - sampleCount) return null;

		int firstElapsedMs, firstElapsedUs, endElapsedMs, endElapsedUs;
		int firstOffset = firstSample - anchor.m_iFirstSampleIndex;
		int endOffset = firstOffset + sampleCount;
		Elapsed(firstOffset, anchor.m_iSampleRateHz, false, firstElapsedMs, firstElapsedUs);
		Elapsed(endOffset, anchor.m_iSampleRateHz, true, endElapsedMs, endElapsedUs);
		int driftUs = DriftBoundUs(endElapsedMs, endElapsedUs, anchor.m_iMaxDriftPpm);
		if (driftUs > int.MAX - anchor.m_iMaxCaptureErrorUs - anchor.m_iMaxMappingErrorUs) return null;
		int captureErrorUs = anchor.m_iMaxCaptureErrorUs + driftUs;

		int firstMs, firstUs, endMs, endUs;
		if (!LocalPoint(anchor, firstElapsedMs, firstElapsedUs, firstMs, firstUs)) return null;
		if (!LocalPoint(anchor, endElapsedMs, endElapsedUs, endMs, endUs)) return null;
		if (!LocalCovered(anchor, firstMs, firstUs, endMs, endUs, captureErrorUs)) return null;

		int estimateMs, estimateUs, lowerMs, lowerUs, upperMs, upperUs, unusedMs, unusedUs;
		int mappingErrorUs = captureErrorUs + anchor.m_iMaxMappingErrorUs;
		if (RICON_TemporalClock.MapInterval(firstMs, firstUs, anchor.m_iClockAnchorLocalMs, anchor.m_iClockAnchorServerMs,
			anchor.m_iValidFromLocalMs, anchor.m_iValidUntilLocalMs, mappingErrorUs,
			estimateMs, estimateUs, lowerMs, lowerUs, unusedMs, unusedUs) != 0) return null;
		if (RICON_TemporalClock.MapInterval(endMs, endUs, anchor.m_iClockAnchorLocalMs, anchor.m_iClockAnchorServerMs,
			anchor.m_iValidFromLocalMs, anchor.m_iValidUntilLocalMs, mappingErrorUs,
			estimateMs, estimateUs, unusedMs, unusedUs, upperMs, upperUs) != 0) return null;

		RICON_MediaCaptureEnvelope envelope = new RICON_MediaCaptureEnvelope();
		envelope.m_Binding = source.CloneBinding();
		envelope.m_sTimebaseId = anchor.m_sTimebaseId;
		envelope.m_iMappingRevision = anchor.m_iMappingRevision;
		envelope.m_iFirstSampleIndex = firstSample;
		envelope.m_iSampleCount = sampleCount;
		envelope.m_iLowerMs = lowerMs;
		envelope.m_iLowerSubUs = lowerUs;
		envelope.m_iUpperMs = upperMs;
		envelope.m_iUpperSubUs = upperUs;
		return envelope;
	}

	protected static void Elapsed(int offset, int rate, bool roundUp, out int ms, out int subUs)
	{
		// Divide each remainder first so no product needs more than signed int32.
		int scaledMs = (offset % rate) * 1000;
		int scaledUs = (scaledMs % rate) * 1000;
		ms = (offset / rate) * 1000 + scaledMs / rate;
		subUs = scaledUs / rate;
		if (roundUp && scaledUs % rate != 0) subUs++;
		if (subUs == 1000)
		{
			ms++;
			subUs = 0;
		}
	}

	//! Ppm bounds the time error as a fraction of nominal elapsed time.
	protected static int DriftBoundUs(int elapsedMs, int elapsedSubUs, int ppm)
	{
		int remainderUs = (elapsedMs % 1000) * 1000 + elapsedSubUs;
		int scaledRemainder = remainderUs * ppm;
		int driftUs = (elapsedMs / 1000) * ppm + scaledRemainder / 1000000;
		if (scaledRemainder % 1000000 != 0) driftUs++;
		return driftUs;
	}

	protected static bool LocalPoint(RICON_MediaCaptureAnchor anchor, int elapsedMs, int elapsedUs,
		out int pointMs, out int pointUs)
	{
		int sumUs = anchor.m_iCaptureAnchorSubUs + elapsedUs;
		int offsetMs = elapsedMs + sumUs / 1000;
		if (anchor.m_iCaptureAnchorLocalMs > int.MAX - offsetMs) return false;
		pointMs = anchor.m_iCaptureAnchorLocalMs + offsetMs;
		pointUs = sumUs % 1000;
		return true;
	}

	protected static bool LocalCovered(RICON_MediaCaptureAnchor anchor, int firstMs, int firstUs,
		int endMs, int endUs, int captureErrorUs)
	{
		if (anchor.m_iValidFromLocalMs < 0 || anchor.m_iValidUntilLocalMs <= anchor.m_iValidFromLocalMs) return false;
		if (anchor.m_iClockAnchorLocalMs < anchor.m_iValidFromLocalMs || anchor.m_iClockAnchorLocalMs >= anchor.m_iValidUntilLocalMs
			|| anchor.m_iClockAnchorServerMs < 0) return false;
		if (!RICON_TemporalClock.ContainsInterval(firstMs, firstUs, endMs, endUs,
			anchor.m_iValidFromLocalMs, anchor.m_iValidUntilLocalMs)) return false;
		int errorMs = captureErrorUs / 1000;
		int errorUs = captureErrorUs % 1000;
		int leftMs = firstMs - anchor.m_iValidFromLocalMs;
		int rightMs = anchor.m_iValidUntilLocalMs - endMs;
		if (leftMs < errorMs || (leftMs == errorMs && firstUs < errorUs)) return false;
		return rightMs > errorMs && (rightMs - errorMs > 1 || endUs + errorUs < 1000);
	}
}
