class RICON_MediaRfHistoryEntry
{
	int m_iValidFromMs, m_iValidUntilMs;
	ref RICON_TemporalRfBudget m_Budget;

	RICON_MediaRfHistoryEntry CloneEntry()
	{
		RICON_MediaRfHistoryEntry copy = new RICON_MediaRfHistoryEntry();
		copy.m_iValidFromMs = m_iValidFromMs;
		copy.m_iValidUntilMs = m_iValidUntilMs;
		if (m_Budget) copy.m_Budget = m_Budget.CloneBudget();
		return copy;
	}

	bool SameEntry(RICON_MediaRfHistoryEntry other)
	{
		return other && m_Budget && m_Budget.Same(other.m_Budget)
			&& m_iValidFromMs == other.m_iValidFromMs && m_iValidUntilMs == other.m_iValidUntilMs;
	}
}

class RICON_MediaFreeSpaceMotionLimits
{
	int m_iMaxSpanMs, m_iMaxSampleTimeErrorMs;
	float m_fFrequencyMHz, m_fMinDistanceM, m_fMaxDistanceM;
	float m_fDistanceErrorM, m_fMaxRelativeSpeedMps, m_fMaxLossVariationDb;
}

//! One exact route. Only the server authority may record completed, physically qualified intervals.
class RICON_MediaRfHistory
{
	protected ref RICON_MediaEgressRoute m_Route;
	protected ref array<ref RICON_MediaRfHistoryEntry> m_Entries = {};
	protected string m_sTimebaseId;
	protected int m_iMaxEntries, m_iMaxAgeMs, m_iLinkId;
	protected int m_iLastNowMs = -1;
	protected int m_iLastUntilMs, m_iLastModelMs, m_iLastModelSubUs;
	protected ref RICON_TemporalRxEnvironment m_LastRxEnvironment;
	protected bool m_bClosed;

	bool Configure(RICON_MediaEgressRoute route, string timebaseId, int maxEntries, int maxAgeMs)
	{
		if (m_bClosed || m_Route || !route || !route.IsValid() || !RICON_TemporalSnapshots.ValidKey(timebaseId)) return false;
		if (maxEntries < 1 || maxEntries > 64 || maxAgeMs < 1 || maxAgeMs > 60000) return false;
		m_Route = route.CloneRoute();
		m_sTimebaseId = timebaseId;
		m_iMaxEntries = maxEntries;
		m_iMaxAgeMs = maxAgeMs;
		m_Entries.Reserve(maxEntries);
		return true;
	}

	bool RecordCompleted(RICON_MediaEgressRoute route, int validFromMs, int validUntilMs, RICON_TemporalRfBudget budget, int nowMs)
	{
		if (!Matches(route, nowMs) || !budget || budget.m_sTimebaseId != m_sTimebaseId) return false;
		if (validFromMs < m_iLastUntilMs || validUntilMs <= validFromMs || validUntilMs > nowMs) return false;
		if (nowMs - validFromMs >= m_iMaxAgeMs || budget.m_iTickMs < validFromMs || budget.m_iTickMs >= validUntilMs) return false;
		if (budget.m_iLinkId <= 0 || budget.m_iLinkId >= int.MAX || (m_iLinkId && m_iLinkId != budget.m_iLinkId)) return false;
		if (budget.m_iModelMs < m_iLastModelMs || budget.m_iModelSubUs < 0 || budget.m_iModelSubUs >= 1000) return false;
		if (budget.m_iModelMs == m_iLastModelMs && budget.m_iModelSubUs < m_iLastModelSubUs) return false;
		if (!RICON_Num.IsFinite(budget.m_fRxPowerDbm) || !RICON_Num.IsFinite(budget.m_fNoiseDbm)) return false;
		if (!RICON_Num.IsFinite(budget.m_fBandwidthHz) || budget.m_fBandwidthHz <= 0) return false;
		RICON_TemporalRxEnvironment environment = budget.m_RxEnvironment;
		if (environment)
		{
			if (!environment.IsValidFor(route.m_Source.m_iSessionEpoch, route.m_iRxDeviceId, route.m_iRxTransceiverIndex,
				m_sTimebaseId, budget.m_fNoiseDbm, route.m_iRxGeneration) || !environment.Covers(validFromMs, validUntilMs)) return false;
			if (m_LastRxEnvironment && (environment.m_iRevision < m_LastRxEnvironment.m_iRevision
				|| (environment.m_iRevision == m_LastRxEnvironment.m_iRevision && !environment.Same(m_LastRxEnvironment)))) return false;
		}
		else if (m_LastRxEnvironment) return false;

		RICON_MediaRfHistoryEntry entry = new RICON_MediaRfHistoryEntry();
		entry.m_iValidFromMs = validFromMs;
		entry.m_iValidUntilMs = validUntilMs;
		entry.m_Budget = budget.CloneBudget();
		Advance(nowMs);
		if (m_Entries.Count() == m_iMaxEntries) m_Entries.RemoveOrdered(0);
		m_Entries.Insert(entry);
		m_iLastUntilMs = validUntilMs;
		m_iLastModelMs = entry.m_Budget.m_iModelMs;
		m_iLastModelSubUs = entry.m_Budget.m_iModelSubUs;
		m_iLinkId = entry.m_Budget.m_iLinkId;
		m_LastRxEnvironment = entry.m_Budget.m_RxEnvironment;
		return true;
	}

	//! The server caller attests continuity, fixed free-space inputs and the motion bounds for the whole interval.
	bool RecordCompletedFreeSpace(RICON_MediaEgressRoute route, RICON_TemporalRfBudget start, RICON_TemporalRfBudget finish,
		float startDistanceM, float finishDistanceM, RICON_MediaFreeSpaceMotionLimits limits, int nowMs)
	{
		if (!limits || limits.m_iMaxSpanMs <= 0 || limits.m_iMaxSampleTimeErrorMs < 0) return false;
		if (!ValidFreeSpaceSample(start) || !ValidFreeSpaceSample(finish) || finish.m_iTickMs != nowMs
			|| start.m_sTimebaseId != finish.m_sTimebaseId || start.m_iLinkId != finish.m_iLinkId
			|| start.m_fNoiseDbm != finish.m_fNoiseDbm || start.m_fBandwidthHz != finish.m_fBandwidthHz) return false;
		int spanMs = finish.m_iTickMs - start.m_iTickMs;
		if (spanMs <= 0 || spanMs > limits.m_iMaxSpanMs) return false;

		float frequencyMHz = limits.m_fFrequencyMHz;
		float distanceErrorM = limits.m_fDistanceErrorM;
		float speedMps = limits.m_fMaxRelativeSpeedMps;
		if (!RICON_Num.IsFinite(frequencyMHz) || frequencyMHz <= 0
			|| !RICON_Num.IsFinite(limits.m_fMinDistanceM) || !RICON_Num.IsFinite(limits.m_fMaxDistanceM)
			|| !RICON_Num.IsFinite(startDistanceM) || !RICON_Num.IsFinite(finishDistanceM)
			|| !RICON_Num.IsFinite(distanceErrorM) || distanceErrorM < 0 || !RICON_Num.IsFinite(speedMps) || speedMps < 0
			|| !RICON_Num.IsFinite(limits.m_fMaxLossVariationDb) || limits.m_fMaxLossVariationDb < 0) return false;

		// Every instant in the interval is within half the span of one nominal sample time.
		float radiusM = distanceErrorM + speedMps * ((spanMs * 0.5 + limits.m_iMaxSampleTimeErrorMs) / 1000.0);
		if (!RICON_Num.IsFinite(radiusM * 2.0) || Math.AbsFloat(startDistanceM - finishDistanceM) > radiusM * 2.0) return false;
		float nearestM = Math.Min(startDistanceM, finishDistanceM) - radiusM;
		float farthestM = Math.Max(startDistanceM, finishDistanceM) + radiusM;
		float wavelengthM = RICON_FreeSpaceModel.WavelengthM(frequencyMHz);
		if (!RICON_Num.IsFinite(wavelengthM) || wavelengthM <= 0 || limits.m_fMinDistanceM <= 0
			|| nearestM < wavelengthM || nearestM < limits.m_fMinDistanceM || farthestM > limits.m_fMaxDistanceM) return false;

		float worstLossDb = RICON_FreeSpaceModel.LossDb(farthestM, frequencyMHz);
		float lossSpanDb = worstLossDb - RICON_FreeSpaceModel.LossDb(nearestM, frequencyMHz);
		if (!RICON_Num.IsFinite(lossSpanDb) || lossSpanDb < 0 || lossSpanDb > limits.m_fMaxLossVariationDb) return false;
		float startPowerDbm = start.m_fRxPowerDbm + RICON_FreeSpaceModel.LossDb(startDistanceM, frequencyMHz);
		float finishPowerDbm = finish.m_fRxPowerDbm + RICON_FreeSpaceModel.LossDb(finishDistanceM, frequencyMHz);
		if (!RICON_Num.IsFinite(startPowerDbm) || !RICON_Num.IsFinite(finishPowerDbm)
			|| Math.AbsFloat(startPowerDbm - finishPowerDbm) > 0.01) return false;

		float boundedPowerDbm = Math.Min(startPowerDbm, finishPowerDbm) - worstLossDb;
		if (!RICON_MediaWireRfV2.ValidPowerDbm(boundedPowerDbm)) return false;
		// Leave room for float arithmetic, then round down to the wire's 0.01 dB precision.
		boundedPowerDbm = Math.Floor((boundedPowerDbm - 0.001) * 100.0) / 100.0;
		if (!RICON_MediaWireRfV2.ValidPowerDbm(boundedPowerDbm)) return false;
		RICON_TemporalRfBudget budget = start.CloneBudget();
		budget.m_fRxPowerDbm = boundedPowerDbm;
		return RecordCompleted(route, start.m_iTickMs, finish.m_iTickMs, budget, nowMs);
	}

	protected static bool ValidFreeSpaceSample(RICON_TemporalRfBudget sample)
	{
		return sample && !sample.m_RxEnvironment && sample.m_iTickMs == sample.m_iModelMs && sample.m_iModelSubUs == 0
			&& RICON_MediaWireRfV2.ValidStamp(sample.m_iLinkId, sample.m_iTickMs, sample.m_iModelMs, sample.m_iModelSubUs)
			&& RICON_MediaWireRfV2.ValidPowerDbm(sample.m_fRxPowerDbm) && RICON_MediaWireRfV2.ValidPowerDbm(sample.m_fNoiseDbm)
			&& sample.m_fBandwidthHz >= 1.0 && sample.m_fBandwidthHz <= RICON_MediaWireRfV2.MAX_BANDWIDTH_HZ
			&& sample.m_fBandwidthHz == Math.Floor(sample.m_fBandwidthHz);
	}

	//! The caller independently checks current branch rights before applying the selected budget.
	RICON_MediaRfHistoryEntry Select(RICON_MediaEgressRoute route, RICON_MediaCaptureEnvelope envelope, int nowMs, bool observe = true)
	{
		if (!Matches(route, nowMs) || !envelope || envelope.m_sTimebaseId != m_sTimebaseId) return null;
		if (!m_Route.m_Source.Same(envelope.m_Binding) || envelope.m_iMappingRevision <= 0) return null;
		if (!RICON_MediaWireV1.ValidRange(0, envelope.m_iFirstSampleIndex, envelope.m_iSampleCount)) return null;
		if (envelope.m_iLowerMs < 0 || envelope.m_iUpperMs > nowMs) return null;
		if (envelope.m_iLowerSubUs < 0 || envelope.m_iLowerSubUs >= 1000) return null;
		if (envelope.m_iUpperSubUs < 0 || envelope.m_iUpperSubUs >= 1000) return null;
		if (envelope.m_iUpperMs == nowMs && envelope.m_iUpperSubUs > 0) return null;
		if (envelope.m_iLowerMs > envelope.m_iUpperMs) return null;
		if (envelope.m_iLowerMs == envelope.m_iUpperMs && envelope.m_iLowerSubUs >= envelope.m_iUpperSubUs) return null;

		if (observe) Advance(nowMs);
		foreach (RICON_MediaRfHistoryEntry entry : m_Entries)
		{
			if (entry.m_iValidFromMs > envelope.m_iLowerMs) break;
			if (nowMs - entry.m_iValidFromMs >= m_iMaxAgeMs) continue;
			if (!RICON_TemporalClock.ContainsInterval(envelope.m_iLowerMs, envelope.m_iLowerSubUs,
				envelope.m_iUpperMs, envelope.m_iUpperSubUs, entry.m_iValidFromMs, entry.m_iValidUntilMs)) continue;
			return entry.CloneEntry();
		}
		return null;
	}

	void Close()
	{
		m_bClosed = true;
		m_Route = null;
		m_Entries.Clear();
		m_LastRxEnvironment = null;
	}

	protected bool Matches(RICON_MediaEgressRoute route, int nowMs)
	{
		return !m_bClosed && m_Route && m_Route.Same(route) && nowMs >= 0 && nowMs >= m_iLastNowMs;
	}

	protected void Advance(int nowMs)
	{
		m_iLastNowMs = nowMs;
		while (m_Entries.Count() > 0 && nowMs - m_Entries[0].m_iValidFromMs >= m_iMaxAgeMs)
			m_Entries.RemoveOrdered(0);
	}
}
