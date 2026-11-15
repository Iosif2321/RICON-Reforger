class RICON_MediaWirePublicationEntry
{
	ref RICON_MediaEgressRoute m_Route;
	int m_iControlSeq, m_iFirstSequence, m_iFirstSampleIndex, m_iAckDeadlineMs;
	bool m_bConfirmed;
}

//! One server publisher per epoch. Callers supply an authoritative server tick and current route.
class RICON_MediaWirePublication
{
	protected int m_iEpoch, m_iMaxRoutes, m_iMaxUsedPolicies, m_iAckTimeoutMs, m_iControlSeq;
	protected int m_iLastNowMs = -1;
	protected bool m_bClosed;
	protected ref map<int, ref RICON_MediaWirePublicationEntry> m_Routes = new map<int, ref RICON_MediaWirePublicationEntry>();
	protected ref set<int> m_UsedPolicies = new set<int>();

	bool Configure(int epoch, int maxRoutes, int ackTimeoutMs, int maxUsedPolicies = 4096)
	{
		if (m_bClosed || m_iEpoch != 0 || epoch <= 0 || epoch >= int.MAX) return false;
		if (maxRoutes < 1 || maxRoutes > 64 || ackTimeoutMs < 1 || ackTimeoutMs > 10000) return false;
		if (maxUsedPolicies < 1 || maxUsedPolicies > 4096) return false;
		m_iEpoch = epoch;
		m_iMaxRoutes = maxRoutes;
		m_iMaxUsedPolicies = maxUsedPolicies;
		m_iAckTimeoutMs = ackTimeoutMs;
		return true;
	}

	int Begin(RICON_MediaEgressRoute route, int firstSequence, int firstSampleIndex, int nowMs)
	{
		if (!ValidRoute(route) || !ValidCutover(firstSequence, firstSampleIndex) || !ValidTime(nowMs)) return 0;
		if (nowMs > int.MAX - m_iAckTimeoutMs || m_Routes.Count() >= m_iMaxRoutes) return 0;
		// Leave enough sequence space to revoke every outstanding publication.
		if (m_iControlSeq > int.MAX - 3 - m_Routes.Count()) return 0;
		int policyRevision = route.m_iPolicyRevision;
		// shortcut: used revisions consume epoch capacity until a synchronized control resync can retire them.
		if (m_UsedPolicies.Count() >= m_iMaxUsedPolicies || m_UsedPolicies.Contains(policyRevision)) return 0;
		if (ConflictsWithActiveRoute(route)) return 0;

		RICON_MediaWirePublicationEntry entry = new RICON_MediaWirePublicationEntry();
		entry.m_Route = route.CloneRoute();
		entry.m_iControlSeq = ++m_iControlSeq;
		entry.m_iFirstSequence = firstSequence;
		entry.m_iFirstSampleIndex = firstSampleIndex;
		entry.m_iAckDeadlineMs = nowMs + m_iAckTimeoutMs;
		m_Routes.Set(policyRevision, entry);
		m_UsedPolicies.Insert(policyRevision);
		m_iLastNowMs = nowMs;
		return entry.m_iControlSeq;
	}

	bool IsPendingInstall(RICON_MediaEgressRoute route, int controlSeq, int firstSequence, int firstSampleIndex, int nowMs)
	{
		RICON_MediaWirePublicationEntry entry = FindExact(route);
		if (!MatchesInstall(entry, controlSeq, firstSequence, firstSampleIndex) || !ObserveTime(nowMs)) return false;
		return !entry.m_bConfirmed && nowMs < entry.m_iAckDeadlineMs;
	}

	bool IsInstallExpired(RICON_MediaEgressRoute route, int nowMs)
	{
		RICON_MediaWirePublicationEntry entry = FindExact(route);
		if (!entry || entry.m_bConfirmed || !ObserveTime(nowMs)) return false;
		return nowMs >= entry.m_iAckDeadlineMs;
	}

	//! Expiration does not revoke a grant by itself. The caller publishes the exact revoke separately.
	bool ReadExpired(int nowMs, array<ref RICON_MediaEgressRoute> routes)
	{
		if (!routes) return false;
		routes.Clear();
		if (m_bClosed || m_iEpoch <= 0 || !ObserveTime(nowMs)) return false;
		foreach (int policy, RICON_MediaWirePublicationEntry entry : m_Routes)
		{
			if (!entry.m_bConfirmed && nowMs >= entry.m_iAckDeadlineMs) routes.Insert(entry.m_Route.CloneRoute());
		}
		return true;
	}

	bool Confirm(RICON_MediaEgressRoute route, int controlSeq, int firstSequence, int firstSampleIndex, int nowMs)
	{
		RICON_MediaWirePublicationEntry entry = FindExact(route);
		if (!MatchesInstall(entry, controlSeq, firstSequence, firstSampleIndex) || !ObserveTime(nowMs)) return false;
		if (entry.m_bConfirmed) return true;
		if (nowMs >= entry.m_iAckDeadlineMs) return false;
		entry.m_bConfirmed = true;
		return true;
	}

	bool CanSend(RICON_MediaEgressRoute route, int sequence, int firstSampleIndex, int nowMs)
	{
		RICON_MediaWirePublicationEntry entry = FindExact(route);
		if (!entry || !ValidCutover(sequence, firstSampleIndex) || !ValidTime(nowMs)) return false;
		return entry.m_bConfirmed && sequence >= entry.m_iFirstSequence && firstSampleIndex >= entry.m_iFirstSampleIndex;
	}

	int Revoke(RICON_MediaEgressRoute route, int nowMs)
	{
		if (!FindExact(route) || !ObserveTime(nowMs)) return 0;
		m_Routes.Remove(route.m_iPolicyRevision);
		return ++m_iControlSeq;
	}

	//! Use only when the transport refused before submitting the install RPC.
	bool Abandon(RICON_MediaEgressRoute route, int controlSeq, int nowMs)
	{
		RICON_MediaWirePublicationEntry entry = FindExact(route);
		if (!entry || entry.m_iControlSeq != controlSeq || entry.m_bConfirmed || !ObserveTime(nowMs)) return false;
		m_Routes.Remove(route.m_iPolicyRevision);
		return true;
	}

	void Close()
	{
		m_bClosed = true;
		m_Routes.Clear();
		m_UsedPolicies.Clear();
	}

	protected RICON_MediaWirePublicationEntry FindExact(RICON_MediaEgressRoute route)
	{
		if (!ValidRoute(route)) return null;
		RICON_MediaWirePublicationEntry entry = m_Routes.Get(route.m_iPolicyRevision);
		if (!entry || !entry.m_Route.Same(route)) return null;
		return entry;
	}

	protected bool ValidRoute(RICON_MediaEgressRoute route)
	{
		return !m_bClosed && m_iEpoch > 0 && route && route.IsValid() && route.m_Source.m_iSessionEpoch == m_iEpoch;
	}

	protected bool ConflictsWithActiveRoute(RICON_MediaEgressRoute route)
	{
		foreach (int policy, RICON_MediaWirePublicationEntry entry : m_Routes)
		{
			RICON_MediaEgressRoute prior = entry.m_Route;
			if (prior.m_iListenerPlayerId != route.m_iListenerPlayerId || prior.m_iListenerConnectionId != route.m_iListenerConnectionId) continue;
			if (prior.m_iListenerGeneration != route.m_iListenerGeneration || prior.m_iListenerEntityId != route.m_iListenerEntityId) continue;
			if (prior.m_Source.m_iStreamId != route.m_Source.m_iStreamId) continue;
			if (!prior.m_Source.Same(route.m_Source) || prior.m_iRouteId == route.m_iRouteId) return true;
			if (prior.m_iRxDeviceId == route.m_iRxDeviceId && prior.m_iRxTransceiverIndex == route.m_iRxTransceiverIndex) return true;
		}
		return false;
	}

	protected bool ValidTime(int nowMs)
	{
		return nowMs >= 0 && nowMs >= m_iLastNowMs;
	}

	protected bool ObserveTime(int nowMs)
	{
		if (!ValidTime(nowMs)) return false;
		m_iLastNowMs = nowMs;
		return true;
	}

	protected static bool ValidCutover(int sequence, int sampleIndex)
	{
		return sequence >= 0 && sequence < int.MAX && sampleIndex >= 0 && sampleIndex < int.MAX;
	}

	protected static bool MatchesInstall(RICON_MediaWirePublicationEntry entry, int controlSeq, int sequence, int sampleIndex)
	{
		return entry && controlSeq == entry.m_iControlSeq && sequence == entry.m_iFirstSequence
			&& sampleIndex == entry.m_iFirstSampleIndex;
	}
}
