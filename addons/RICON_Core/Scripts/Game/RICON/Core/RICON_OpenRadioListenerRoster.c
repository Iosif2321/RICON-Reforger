class RICON_OpenRadioListenerMembership
{
	ref RICON_OpenRadioPairListener m_Row;
	int m_iIncarnation;
}

//! Server-approved fixed radio slot for one current controlled entity.
class RICON_OpenRadioSlotAssignment
{
	int m_iPlayerId, m_iConnectionId, m_iEntityId, m_iDeviceId, m_iIndex, m_iGeneration;
	BaseRadioComponent m_Radio;
}

//! The trusted scenario replaces the complete listener set for one open RADIO channel.
class RICON_OpenRadioListenerRoster
{
	protected RICON_LifecycleStateMachine m_Lifecycle;
	protected RICON_MediaReceiverAuthority m_Receivers;
	protected ref RICON_GameControllerLifecycle m_GameControllerLifecycle;
	protected int m_iEpoch, m_iNetworkId, m_iChannelId, m_iMaxListeners, m_iRevision;
	protected int m_iLastWriteMs = -1;
	protected bool m_bClosed;
	protected ref map<string, ref RICON_OpenRadioListenerMembership> m_Rows = new map<string, ref RICON_OpenRadioListenerMembership>();

	bool Configure(RICON_LifecycleStateMachine lifecycle, RICON_MediaReceiverAuthority receivers,
		int networkId, int channelId, int maxListeners)
	{
		if (m_bClosed || m_iEpoch != 0 || !lifecycle || !receivers || !receivers.IsScope(lifecycle)
			|| networkId <= 0 || channelId <= 0 || maxListeners <= 0 || maxListeners > 4096) return false;
		m_Lifecycle = lifecycle;
		m_Receivers = receivers;
		m_iEpoch = lifecycle.GetSessionEpoch();
		m_iNetworkId = networkId;
		m_iChannelId = channelId;
		m_iMaxListeners = maxListeners;
		m_iRevision = 1;
		return true;
	}

	bool IsScope(RICON_LifecycleStateMachine lifecycle, RICON_MediaReceiverAuthority receivers, int networkId, int channelId)
	{
		return !m_bClosed && lifecycle && lifecycle == m_Lifecycle && lifecycle.GetSessionEpoch() == m_iEpoch
			&& receivers && receivers == m_Receivers && receivers.IsScope(lifecycle)
			&& networkId == m_iNetworkId && channelId == m_iChannelId;
	}

	bool UseGameControllerLifecycle(RICON_GameControllerLifecycle bridge)
	{
		if (m_GameControllerLifecycle || m_iLastWriteMs >= 0 || GetRevision() <= 0) return false;
		if (!bridge || !bridge.MatchesLifecycle(m_Lifecycle)) return false;
		m_GameControllerLifecycle = bridge;
		return true;
	}

	bool UsesGameAuthority(RICON_GameControllerLifecycle bridge, RICON_MediaReceiverAuthority receivers, int networkId)
	{
		return bridge && bridge == m_GameControllerLifecycle && bridge.MatchesLifecycle(m_Lifecycle)
			&& IsScope(m_Lifecycle, receivers, networkId, m_iChannelId);
	}

	int GetRevision()
	{
		if (!IsScope(m_Lifecycle, m_Receivers, m_iNetworkId, m_iChannelId)) return 0;
		if (m_GameControllerLifecycle && !m_GameControllerLifecycle.MatchesLifecycle(m_Lifecycle)) return 0;
		return m_iRevision;
	}

	bool Replace(int expectedRevision, array<ref RICON_OpenRadioPairListener> rows, int nowMs)
	{
		return !m_GameControllerLifecycle && ReplaceRows(expectedRevision, rows, nowMs);
	}

	bool ReplaceAssigned(int expectedRevision, array<ref RICON_OpenRadioSlotAssignment> assignments, int nowMs)
	{
		if (!m_GameControllerLifecycle || expectedRevision <= 0 || expectedRevision != GetRevision()) return false;
		if (!assignments || assignments.Count() > m_iMaxListeners || nowMs < m_iLastWriteMs) return false;
		if (!m_Receivers.CanRead(m_Lifecycle, nowMs)) return false;
		array<ref RICON_OpenRadioPairListener> rows = {};
		RICON_MediaEgressRoute rx = new RICON_MediaEgressRoute();
		rx.m_Source = new RICON_MediaStreamBinding();
		rx.m_Source.m_iSessionEpoch = m_iEpoch;
		rx.m_Source.m_iNetworkTypeId = m_iNetworkId;
		rx.m_Source.m_iChannelId = m_iChannelId;
		foreach (RICON_OpenRadioSlotAssignment assignment : assignments)
		{
			if (!assignment || !m_Receivers.MatchesRadio(assignment.m_iDeviceId, assignment.m_iIndex,
				assignment.m_iGeneration, assignment.m_Radio)) return false;
			RICON_LifecycleDevice device = m_Lifecycle.QueryDevice(assignment.m_iDeviceId, nowMs);
			if (!device || device.m_eKind != RICON_EDeviceKind.WORLD || !device.m_bPowered) return false;
			RICON_OpenRadioPairListener row = new RICON_OpenRadioPairListener();
			row.m_iPlayerId = assignment.m_iPlayerId;
			if (!m_GameControllerLifecycle.ReadBoundPlayer(row.m_iPlayerId, row.m_iConnectionId,
				row.m_iPlayerGeneration, row.m_iEntityId)) return false;
			if (row.m_iConnectionId != assignment.m_iConnectionId || row.m_iEntityId != assignment.m_iEntityId) return false;
			row.m_iDeviceId = assignment.m_iDeviceId;
			row.m_iIndex = assignment.m_iIndex;
			row.m_iGeneration = assignment.m_iGeneration;
			rx.m_iRxDeviceId = row.m_iDeviceId;
			rx.m_iRxTransceiverIndex = row.m_iIndex;
			rx.m_iRxGeneration = row.m_iGeneration;
			if (m_Receivers.ReadAdmission(rx, nowMs) <= 0) return false;
			rows.Insert(row);
		}
		return ReplaceRows(expectedRevision, rows, nowMs);
	}

	protected bool ReplaceRows(int expectedRevision, array<ref RICON_OpenRadioPairListener> rows, int nowMs)
	{
		int revision = GetRevision();
		if (revision <= 0 || revision != expectedRevision || !rows || rows.Count() > m_iMaxListeners
			|| nowMs < m_iLastWriteMs || !m_Lifecycle.CanQueryAt(nowMs)) return false;
		map<string, ref RICON_OpenRadioListenerMembership> staged = new map<string, ref RICON_OpenRadioListenerMembership>();
		bool changed = rows.Count() != m_Rows.Count();
		foreach (RICON_OpenRadioPairListener row : rows)
		{
			if (!ListenerCurrent(row)) return false;
			RICON_LifecycleDevice device = m_Lifecycle.QueryDevice(row.m_iDeviceId, nowMs);
			if (!device || !device.CanAccess(row.m_iPlayerId) || !device.Allows(m_iNetworkId, m_iChannelId)) return false;
			string key = ListenerKey(row.m_iPlayerId, row.m_iDeviceId, row.m_iIndex);
			if (staged.Contains(key)) return false;
			RICON_OpenRadioListenerMembership existing = m_Rows.Get(key);
			if (existing && SameBinding(existing.m_Row, row))
			{
				staged.Set(key, existing);
				continue;
			}
			if (revision == int.MAX) return false;
			RICON_OpenRadioListenerMembership added = new RICON_OpenRadioListenerMembership();
			added.m_Row = CopyRow(row);
			added.m_iIncarnation = revision + 1;
			staged.Set(key, added);
			changed = true;
		}
		if (!changed)
		{
			m_iLastWriteMs = nowMs;
			return true;
		}
		if (revision == int.MAX) return false;
		m_Rows = staged;
		m_iRevision++;
		m_iLastWriteMs = nowMs;
		return true;
	}

	bool Read(array<ref RICON_OpenRadioPairListener> rows)
	{
		if (!rows) return false;
		rows.Clear();
		if (GetRevision() == 0) return false;
		if (m_GameControllerLifecycle)
		{
			for (int index = 0; index < m_Rows.Count(); index++)
			{
				if (!ListenerCurrent(m_Rows.GetElement(index).m_Row)) return false;
			}
		}
		for (int index = 0; index < m_Rows.Count(); index++)
			rows.Insert(CopyRow(m_Rows.GetElement(index).m_Row));
		return true;
	}

	bool MatchesInput(RICON_PairServiceInput input, RICON_LifecycleStateMachine lifecycle)
	{
		if (!IsScope(lifecycle, m_Receivers, m_iNetworkId, m_iChannelId) || !input || input.m_iSessionEpoch != m_iEpoch
			|| !input.m_aBindings || !input.m_aListeners || input.m_aListeners.Count() != m_Rows.Count()
			|| !lifecycle.CanQueryAt(input.m_iNowMs)) return false;
		foreach (RICON_TransmissionBinding binding : input.m_aBindings)
		{
			if (!binding || binding.m_iSessionEpoch != m_iEpoch || binding.m_iNetworkTypeId != m_iNetworkId
				|| binding.m_iChannelId != m_iChannelId) return false;
		}
		set<string> seen = new set<string>();
		foreach (RICON_PairListener projected : input.m_aListeners)
		{
			if (!projected || projected.m_iSessionEpoch != m_iEpoch) return false;
			string key = ListenerKey(projected.m_iPlayerId, projected.m_iDeviceId, projected.m_iTransceiverIndex);
			RICON_OpenRadioListenerMembership entry = m_Rows.Get(key);
			if (seen.Contains(key) || !entry || entry.m_Row.m_iPlayerGeneration != projected.m_iGeneration
				|| !ListenerCurrent(entry.m_Row)) return false;
			seen.Insert(key);
		}
		return true;
	}

	int ReadIncarnation(RICON_MediaEgressRoute route)
	{
		if (!route || !route.m_Source || GetRevision() == 0 || route.m_Source.m_iSessionEpoch != m_iEpoch
			|| route.m_Source.m_iNetworkTypeId != m_iNetworkId || route.m_Source.m_iChannelId != m_iChannelId) return 0;
		string key = ListenerKey(route.m_iListenerPlayerId, route.m_iRxDeviceId, route.m_iRxTransceiverIndex);
		RICON_OpenRadioListenerMembership entry = m_Rows.Get(key);
		if (!entry) return 0;
		RICON_OpenRadioPairListener row = entry.m_Row;
		if (route.m_iListenerGeneration != row.m_iPlayerGeneration || route.m_iListenerConnectionId != row.m_iConnectionId
			|| route.m_iListenerEntityId != row.m_iEntityId || route.m_iRxGeneration != row.m_iGeneration
			|| !ListenerCurrent(row)) return 0;
		return entry.m_iIncarnation;
	}

	void ConsumeLifecycle(RICON_LifecycleResult result, int preEventEpoch)
	{
		if (m_bClosed || !result || !result.m_aEffects || preEventEpoch != m_iEpoch) return;
		bool changed;
		foreach (RICON_LifecycleEffect effect : result.m_aEffects)
		{
			if (!effect || (effect.m_eType != RICON_ELifecycleEffectType.ENTITY_UNBOUND
				&& effect.m_eType != RICON_ELifecycleEffectType.SUBSCRIPTIONS_CLEARED)) continue;
			for (int index = m_Rows.Count() - 1; index >= 0; index--)
			{
				RICON_OpenRadioPairListener row = m_Rows.GetElement(index).m_Row;
				if (row.m_iPlayerId != effect.m_iPlayerId || row.m_iPlayerGeneration != effect.m_iGeneration) continue;
				if (effect.m_eType == RICON_ELifecycleEffectType.ENTITY_UNBOUND && row.m_iEntityId != effect.m_iEntityId) continue;
				if (m_iRevision == int.MAX)
				{
					Close();
					return;
				}
				m_Rows.Remove(m_Rows.GetKey(index));
				changed = true;
			}
		}
		if (changed) m_iRevision++;
	}

	void Close()
	{
		m_bClosed = true;
		m_iRevision = 0;
		m_Rows.Clear();
		m_Lifecycle = null;
		m_Receivers = null;
		m_GameControllerLifecycle = null;
	}

	protected bool ListenerCurrent(RICON_OpenRadioPairListener row)
	{
		if (!row || row.m_iPlayerId <= 0 || row.m_iPlayerGeneration <= 0 || row.m_iConnectionId <= 0 || row.m_iEntityId <= 0)
			return false;
		if (row.m_iDeviceId <= 0 || row.m_iDeviceId >= int.MAX || row.m_iIndex < 0 || row.m_iIndex >= int.MAX || row.m_iGeneration <= 0)
			return false;
		if (m_GameControllerLifecycle)
		{
			int connectionId, generation, entityId;
			if (!m_GameControllerLifecycle.ReadBoundPlayer(row.m_iPlayerId, connectionId, generation, entityId)) return false;
			if (connectionId != row.m_iConnectionId || generation != row.m_iPlayerGeneration || entityId != row.m_iEntityId) return false;
			return m_Receivers.HasRegistration(row.m_iDeviceId, row.m_iIndex, row.m_iGeneration);
		}
		return m_Lifecycle.GetPlayerState(row.m_iPlayerId) == RICON_EPlayerLifecycleState.BOUND_ACTIVE
			&& m_Lifecycle.GetPlayerGeneration(row.m_iPlayerId) == row.m_iPlayerGeneration
			&& m_Lifecycle.GetPlayerConnectionId(row.m_iPlayerId) == row.m_iConnectionId
			&& m_Lifecycle.GetPlayerEntityId(row.m_iPlayerId) == row.m_iEntityId
			&& m_Receivers.HasRegistration(row.m_iDeviceId, row.m_iIndex, row.m_iGeneration);
	}

	protected static string ListenerKey(int playerId, int deviceId, int index) { return string.Format("%1:%2:%3", playerId, deviceId, index); }

	protected static bool SameBinding(RICON_OpenRadioPairListener a, RICON_OpenRadioPairListener b)
	{
		return a.m_iPlayerGeneration == b.m_iPlayerGeneration && a.m_iConnectionId == b.m_iConnectionId
			&& a.m_iEntityId == b.m_iEntityId && a.m_iGeneration == b.m_iGeneration;
	}

	protected static RICON_OpenRadioPairListener CopyRow(RICON_OpenRadioPairListener row)
	{
		RICON_OpenRadioPairListener copy = new RICON_OpenRadioPairListener();
		copy.m_iPlayerId = row.m_iPlayerId;
		copy.m_iPlayerGeneration = row.m_iPlayerGeneration;
		copy.m_iConnectionId = row.m_iConnectionId;
		copy.m_iEntityId = row.m_iEntityId;
		copy.m_iDeviceId = row.m_iDeviceId;
		copy.m_iIndex = row.m_iIndex;
		copy.m_iGeneration = row.m_iGeneration;
		return copy;
	}
}
