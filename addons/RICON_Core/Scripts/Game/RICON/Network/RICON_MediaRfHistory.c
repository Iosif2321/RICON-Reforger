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

//! One exact route. Only the server authority may record completed, physically qualified intervals.
class RICON_MediaRfHistory
{
	protected ref RICON_MediaEgressRoute m_Route;
	protected ref array<ref RICON_MediaRfHistoryEntry> m_Entries = {};
	protected string m_sTimebaseId;
	protected int m_iMaxEntries, m_iMaxAgeMs, m_iLinkId;
	protected int m_iLastNowMs = -1;
	protected int m_iLastUntilMs, m_iLastModelMs, m_iLastModelSubUs;
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
		return true;
	}

	//! The caller independently checks current branch rights before applying the selected budget.
	RICON_MediaRfHistoryEntry Select(RICON_MediaEgressRoute route, RICON_MediaCaptureEnvelope envelope, int nowMs)
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

		Advance(nowMs);
		foreach (RICON_MediaRfHistoryEntry entry : m_Entries)
		{
			if (entry.m_iValidFromMs > envelope.m_iLowerMs) break;
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
