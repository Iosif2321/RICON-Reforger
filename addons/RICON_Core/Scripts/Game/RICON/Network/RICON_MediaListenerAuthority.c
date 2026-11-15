class RICON_MediaListenerGrant
{
	ref RICON_MediaEgressRoute m_Scope;
	int m_iRxAdmission;
}

//! Trusted server decisions for one Lifecycle and source owner. Missing rights deny delivery.
class RICON_MediaListenerAuthority
{
	protected RICON_LifecycleStateMachine m_Lifecycle;
	protected RICON_MediaStreamOwner m_SourceOwner;
	protected RICON_MediaReceiverAuthority m_ReceiverAuthority;
	protected ref map<int, ref RICON_MediaListenerGrant> m_Grants = new map<int, ref RICON_MediaListenerGrant>();
	protected int m_iEpoch, m_iMaxGrants, m_iRevision;
	protected int m_iLastWriteMs = -1;
	protected bool m_bClosed;

	bool Configure(RICON_LifecycleStateMachine lifecycle, RICON_MediaStreamOwner sourceOwner,
		RICON_MediaReceiverAuthority receiverAuthority, int maxGrants)
	{
		if (m_bClosed || m_iEpoch != 0 || !sourceOwner || !receiverAuthority || maxGrants <= 0
			|| !sourceOwner.IsScope(lifecycle) || !receiverAuthority.IsScope(lifecycle)) return false;
		m_Lifecycle = lifecycle;
		m_SourceOwner = sourceOwner;
		m_ReceiverAuthority = receiverAuthority;
		m_iEpoch = lifecycle.GetSessionEpoch();
		m_iMaxGrants = maxGrants;
		return true;
	}

	bool IsScope(RICON_LifecycleStateMachine lifecycle, RICON_MediaStreamOwner sourceOwner, RICON_MediaReceiverAuthority receiverAuthority)
	{
		return !m_bClosed && lifecycle && lifecycle == m_Lifecycle && lifecycle.GetSessionEpoch() == m_iEpoch
			&& sourceOwner && sourceOwner == m_SourceOwner && sourceOwner.IsScope(lifecycle)
			&& receiverAuthority && receiverAuthority == m_ReceiverAuthority
			&& receiverAuthority.IsScope(lifecycle);
	}

	bool CanRead(int nowMs)
	{
		return IsScope(m_Lifecycle, m_SourceOwner, m_ReceiverAuthority) && nowMs >= m_iLastWriteMs
			&& nowMs >= m_SourceOwner.LastObservedTimeMs() && m_ReceiverAuthority.CanRead(m_Lifecycle, nowMs);
	}

	protected static bool SameScope(RICON_MediaEgressRoute a, RICON_MediaEgressRoute b)
	{
		return a.m_Source.Same(b.m_Source) && a.m_iListenerPlayerId == b.m_iListenerPlayerId
			&& a.m_iListenerConnectionId == b.m_iListenerConnectionId && a.m_iListenerGeneration == b.m_iListenerGeneration
			&& a.m_iListenerEntityId == b.m_iListenerEntityId && a.m_iRxDeviceId == b.m_iRxDeviceId
			&& a.m_iRxTransceiverIndex == b.m_iRxTransceiverIndex && a.m_iRxGeneration == b.m_iRxGeneration;
	}

	int Grant(RICON_MediaStreamBinding source, int listenerId, int rxDeviceId, int rxIndex, int rxGeneration, int nowMs)
	{
		bool created;
		return GrantTracked(source, listenerId, rxDeviceId, rxIndex, rxGeneration, nowMs, created);
	}

	int GrantTracked(RICON_MediaStreamBinding source, int listenerId, int rxDeviceId, int rxIndex, int rxGeneration, int nowMs, out bool created)
	{
		created = false;
		if (!CanRead(nowMs) || !source || !m_SourceOwner.IsSourceCurrent(source, m_Lifecycle, nowMs)) return 0;
		RICON_MediaEgressRoute scope = new RICON_MediaEgressRoute();
		scope.m_Source = source.CloneBinding();
		scope.m_iListenerPlayerId = listenerId;
		scope.m_iListenerConnectionId = m_Lifecycle.GetPlayerConnectionId(listenerId);
		scope.m_iListenerGeneration = m_Lifecycle.GetPlayerGeneration(listenerId);
		scope.m_iListenerEntityId = m_Lifecycle.GetPlayerEntityId(listenerId);
		scope.m_iRxDeviceId = rxDeviceId;
		scope.m_iRxTransceiverIndex = rxIndex;
		scope.m_iRxGeneration = rxGeneration;
		if (listenerId <= 0 || listenerId == source.m_iPlayerId || scope.m_iListenerConnectionId <= 0
			|| scope.m_iListenerConnectionId == source.m_iConnectionId || scope.m_iListenerGeneration <= 0
			|| scope.m_iListenerEntityId <= 0 || rxDeviceId == source.m_iDeviceId) return 0;
		if (!RICON_MediaEgressQueue.ReceiverCurrent(scope, m_Lifecycle, nowMs)) return 0;
		int admission = m_ReceiverAuthority.ReadAdmission(scope, nowMs);
		if (admission <= 0) return 0;

		// Retired emissions and listeners do not reserve capacity across later PTTs.
		for (int index = m_Grants.Count() - 1; index >= 0; index--)
		{
			int revision = m_Grants.GetKey(index);
			RICON_MediaListenerGrant held = m_Grants.GetElement(index);
			RICON_MediaEgressRoute prior = held.m_Scope;
			if (SameScope(prior, scope))
			{
				if (held.m_iRxAdmission == admission)
				{
					m_iLastWriteMs = nowMs;
					return revision;
				}
				m_Grants.Remove(revision);
			}
			else if (!m_SourceOwner.IsSourceCurrent(prior.m_Source, m_Lifecycle, nowMs)
				|| !RICON_MediaEgressQueue.ReceiverCurrent(prior, m_Lifecycle, nowMs)
				|| m_ReceiverAuthority.ReadAdmission(prior, nowMs) != held.m_iRxAdmission)
				m_Grants.Remove(revision);
		}
		m_iLastWriteMs = nowMs;
		if (m_Grants.Count() >= m_iMaxGrants || m_iRevision >= int.MAX - 1) return 0;
		RICON_MediaListenerGrant grant = new RICON_MediaListenerGrant();
		grant.m_Scope = scope;
		grant.m_iRxAdmission = admission;
		m_iRevision++;
		m_Grants.Set(m_iRevision, grant);
		created = true;
		return m_iRevision;
	}

	bool Revoke(int revision)
	{
		// A still-issued exact revision can be removed without trusting the event clock.
		if (!m_Grants.Contains(revision)) return false;
		m_Grants.Remove(revision);
		return true;
	}

	bool MatchesGrant(RICON_MediaEgressRoute route, int rxAdmission, int nowMs)
	{
		if (!route || !route.m_Source || rxAdmission <= 0 || !CanRead(nowMs)) return false;
		RICON_MediaListenerGrant held = m_Grants.Get(route.m_iPolicyRevision);
		return held && rxAdmission == held.m_iRxAdmission && SameScope(held.m_Scope, route);
	}

	//! Consume committed Lifecycle effects so a change and return between sends cannot restore an old right.
	void ConsumeLifecycle(RICON_LifecycleResult result, int preEventEpoch)
	{
		if (!result || !result.m_aEffects || preEventEpoch != m_iEpoch) return;
		foreach (RICON_LifecycleEffect effect : result.m_aEffects)
		{
			if (!effect) continue;
			if (effect.m_eType != RICON_ELifecycleEffectType.LEASE_TERMINATED
				&& effect.m_eType != RICON_ELifecycleEffectType.ENTITY_UNBOUND
				&& effect.m_eType != RICON_ELifecycleEffectType.SUBSCRIPTIONS_CLEARED) continue;
			for (int index = m_Grants.Count() - 1; index >= 0; index--)
			{
				RICON_MediaEgressRoute scope = m_Grants.GetElement(index).m_Scope;
				if (EndsGrant(effect, scope)) m_Grants.Remove(m_Grants.GetKey(index));
			}
		}
	}

	protected static bool EndsGrant(RICON_LifecycleEffect effect, RICON_MediaEgressRoute scope)
	{
		RICON_MediaStreamBinding source = scope.m_Source;
		bool tx = source.m_iPlayerId == effect.m_iPlayerId && source.m_iPlayerGeneration == effect.m_iGeneration;
		if (effect.m_eType == RICON_ELifecycleEffectType.LEASE_TERMINATED)
			return tx && source.m_iLeaseId == effect.m_iLeaseId && source.m_iDeviceId == effect.m_iDeviceId;
		bool rx = scope.m_iListenerPlayerId == effect.m_iPlayerId && scope.m_iListenerGeneration == effect.m_iGeneration;
		if (effect.m_eType == RICON_ELifecycleEffectType.ENTITY_UNBOUND)
			return (tx && source.m_iEntityId == effect.m_iEntityId) || (rx && scope.m_iListenerEntityId == effect.m_iEntityId);
		return rx;
	}

	void ForgetPlayer(int playerId)
	{
		for (int index = m_Grants.Count() - 1; index >= 0; index--)
		{
			RICON_MediaEgressRoute scope = m_Grants.GetElement(index).m_Scope;
			if (scope.m_iListenerPlayerId == playerId || scope.m_Source.m_iPlayerId == playerId)
				m_Grants.Remove(m_Grants.GetKey(index));
		}
	}

	void Close()
	{
		m_bClosed = true;
		m_Grants.Clear();
		m_Lifecycle = null;
		m_SourceOwner = null;
		m_ReceiverAuthority = null;
	}
}
