//! One open-radio bridge direction issued by the server for an exact destination.
class RICON_RelayStationDirection
{
	int m_iOriginTxGeneration, m_iInputFrequency;
	int m_iOutputNetworkId, m_iOutputChannelId, m_iOutputFrequency;
	int m_iDestinationIndex = -1;
	int m_iListenerPlayerId, m_iListenerConnectionId, m_iListenerGeneration, m_iListenerEntityId;
	bool m_bInputAllowed, m_bOutputAllowed;

	bool IsValid()
	{
		return m_iOriginTxGeneration > 0 && m_iInputFrequency > 0
			&& m_iOutputNetworkId > 0 && m_iOutputChannelId > 0 && m_iOutputFrequency > 0 && m_iDestinationIndex >= 0
			&& m_iListenerPlayerId > 0 && m_iListenerConnectionId > 0 && m_iListenerGeneration > 0 && m_iListenerEntityId > 0
			&& m_bInputAllowed && m_bOutputAllowed;
	}
}

//! Server-side address for a station emission, separate from its originating capture.
class RICON_RelayDestination
{
	int m_iPlayerId, m_iConnectionId, m_iPlayerGeneration, m_iEntityId;
	int m_iRxDeviceId, m_iRxIndex, m_iRxGeneration;

	static RICON_RelayDestination FromRoute(RICON_MediaEgressRoute route)
	{
		RICON_RelayDestination destination = new RICON_RelayDestination();
		destination.m_iPlayerId = route.m_iListenerPlayerId;
		destination.m_iConnectionId = route.m_iListenerConnectionId;
		destination.m_iPlayerGeneration = route.m_iListenerGeneration;
		destination.m_iEntityId = route.m_iListenerEntityId;
		destination.m_iRxDeviceId = route.m_iRxDeviceId;
		destination.m_iRxIndex = route.m_iRxTransceiverIndex;
		destination.m_iRxGeneration = route.m_iRxGeneration;
		return destination;
	}

	bool MatchesRoute(RICON_MediaEgressRoute route)
	{
		return route && m_iPlayerId == route.m_iListenerPlayerId && m_iConnectionId == route.m_iListenerConnectionId
			&& m_iPlayerGeneration == route.m_iListenerGeneration && m_iEntityId == route.m_iListenerEntityId
			&& m_iRxDeviceId == route.m_iRxDeviceId && m_iRxIndex == route.m_iRxTransceiverIndex
			&& m_iRxGeneration == route.m_iRxGeneration;
	}

	bool IsValid()
	{
		return m_iPlayerId > 0 && m_iConnectionId > 0 && m_iPlayerGeneration > 0 && m_iEntityId > 0
			&& m_iRxDeviceId > 0 && m_iRxIndex >= 0 && m_iRxGeneration > 0;
	}

	bool Same(RICON_RelayDestination other)
	{
		return other && m_iPlayerId == other.m_iPlayerId && m_iConnectionId == other.m_iConnectionId
			&& m_iPlayerGeneration == other.m_iPlayerGeneration && m_iEntityId == other.m_iEntityId
			&& m_iRxDeviceId == other.m_iRxDeviceId && m_iRxIndex == other.m_iRxIndex && m_iRxGeneration == other.m_iRxGeneration;
	}

	RICON_RelayDestination CloneDestination()
	{
		RICON_RelayDestination copy = new RICON_RelayDestination();
		copy.m_iPlayerId = m_iPlayerId;
		copy.m_iConnectionId = m_iConnectionId;
		copy.m_iPlayerGeneration = m_iPlayerGeneration;
		copy.m_iEntityId = m_iEntityId;
		copy.m_iRxDeviceId = m_iRxDeviceId;
		copy.m_iRxIndex = m_iRxIndex;
		copy.m_iRxGeneration = m_iRxGeneration;
		return copy;
	}
}

//! The station owns a fixed, unkeyed direction. The upstream listener tuple is an RX witness, not a station principal.
class RICON_RelayStationAuthority : RICON_RelayPcmPolicy
{
	protected ref RICON_LifecycleStateMachine m_Lifecycle;
	protected ref RICON_MediaStreamOwner m_SourceOwner;
	protected ref RICON_MediaReceiverAuthority m_Receivers;
	protected ref RICON_RelayPcmPath m_Path;
	protected RICON_RelayStationComponent m_Station;
	protected BaseRadioComponent m_StationRadio;
	protected int m_iOriginTxGeneration, m_iInputFrequency;
	protected int m_iOutputNetworkId, m_iOutputChannelId, m_iOutputFrequency, m_iDestinationIndex;
	protected int m_iListenerPlayerId, m_iListenerConnectionId, m_iListenerGeneration, m_iListenerEntityId;
	protected int m_iOriginAdmission, m_iRelayRxAdmission, m_iRelayTxAdmission, m_iDestinationAdmission;
	protected int m_iLastNowMs = -1;
	protected bool m_bInputLive, m_bOutputLive, m_bClosed, m_bRequiresNativeStation;

	bool Configure(RICON_RelayPcmPath path, RICON_RelayStationDirection direction, RICON_LifecycleStateMachine lifecycle,
		RICON_MediaStreamOwner sourceOwner, RICON_MediaReceiverAuthority receivers, int nowMs,
		RICON_RelayStationComponent station = null)
	{
		if (m_bClosed || m_Path || !path || !path.IsValid() || !direction || !direction.IsValid()) return false;
		if (!lifecycle || !sourceOwner || !receivers || !sourceOwner.IsScope(lifecycle) || !receivers.CanRead(lifecycle, nowMs)) return false;
		RICON_MediaEgressRoute upstream = path.m_UpstreamRoute;
		RICON_MediaStreamBinding origin = upstream.m_Source;
		if (origin.m_iSessionEpoch != lifecycle.GetSessionEpoch() || path.m_iRelayEndpointId != upstream.m_iRxDeviceId
			|| path.m_iDestinationEndpointId == path.m_iRelayTxDeviceId || direction.m_iListenerPlayerId == origin.m_iPlayerId
			|| direction.m_iListenerConnectionId == origin.m_iConnectionId) return false;

		m_Lifecycle = lifecycle;
		m_SourceOwner = sourceOwner;
		m_Receivers = receivers;
		m_Path = path.ClonePath();
		m_bRequiresNativeStation = station != null;
		m_Station = station;
		if (station)
		{
			IEntity owner = station.GetOwner();
			if (owner && !owner.IsDeleted()) m_StationRadio = BaseRadioComponent.Cast(owner.FindComponent(BaseRadioComponent));
			if (!NativeStationCurrent()) { Close(); return false; }
		}
		m_iOriginTxGeneration = direction.m_iOriginTxGeneration;
		m_iInputFrequency = direction.m_iInputFrequency;
		m_iOutputNetworkId = direction.m_iOutputNetworkId;
		m_iOutputChannelId = direction.m_iOutputChannelId;
		m_iOutputFrequency = direction.m_iOutputFrequency;
		m_iDestinationIndex = direction.m_iDestinationIndex;
		m_iListenerPlayerId = direction.m_iListenerPlayerId;
		m_iListenerConnectionId = direction.m_iListenerConnectionId;
		m_iListenerGeneration = direction.m_iListenerGeneration;
		m_iListenerEntityId = direction.m_iListenerEntityId;
		m_iLastNowMs = nowMs;

		m_iOriginAdmission = ReadOrigin(nowMs);
		m_iRelayRxAdmission = ReadRelayRx(nowMs);
		m_iRelayTxAdmission = ReadRelayTx(nowMs);
		m_iDestinationAdmission = ReadDestination(nowMs);
		m_bInputLive = m_iOriginAdmission > 0 && m_iRelayRxAdmission > 0;
		m_bOutputLive = m_iRelayTxAdmission > 0 && m_iDestinationAdmission > 0;
		if (m_bInputLive && m_bOutputLive && NativeStationCurrent()) return true;
		Close();
		return false;
	}

	override bool CanUseInput(RICON_RelayPcmPath path, int nowMs)
	{
		if (!m_bInputLive || !ObservePath(path, nowMs)) return false;
		m_bInputLive = ReadOrigin(nowMs) == m_iOriginAdmission && ReadRelayRx(nowMs) == m_iRelayRxAdmission;
		if (!NativeStationCurrent()) { Close(); return false; }
		return m_bInputLive;
	}

	override bool CanUseOutput(RICON_RelayPcmPath path, int nowMs)
	{
		if (!m_bOutputLive || !ObservePath(path, nowMs)) return false;
		m_bOutputLive = ReadRelayTx(nowMs) == m_iRelayTxAdmission && ReadDestination(nowMs) == m_iDestinationAdmission;
		if (!NativeStationCurrent()) { Close(); return false; }
		return m_bOutputLive;
	}

	//! Exact native station radio bound to this inlet; a legacy policy cannot witness a game sample.
	bool MatchesNativeInputRadio(RICON_RelayPcmPath path, BaseRadioComponent radio)
	{
		return !m_bClosed && m_bInputLive && m_bRequiresNativeStation && m_Path && m_Path.SamePath(path)
			&& radio && radio == m_StationRadio && NativeStationCurrent();
	}

	//! Перед чтением источник и вход станции должны совпасть со снимком допуска.
	bool MatchesInputEndpoints(RICON_RelayPcmPath path, RICON_RadioObservation tx, int txGeneration,
		RICON_RadioObservation rx, int rxGeneration)
	{
		if (!m_bInputLive || !m_Path || !m_Path.SamePath(path) || !tx || !rx || !tx.m_bPowered || !rx.m_bPowered) return false;
		RICON_MediaEgressRoute upstream = m_Path.m_UpstreamRoute;
		RICON_MediaStreamBinding origin = upstream.m_Source;
		return tx.m_iDeviceId == origin.m_iDeviceId && tx.m_iTransceiverIndex == origin.m_iTxTransceiverIndex
			&& txGeneration == m_iOriginTxGeneration && rx.m_iDeviceId == upstream.m_iRxDeviceId
			&& rx.m_iTransceiverIndex == upstream.m_iRxTransceiverIndex && rxGeneration == upstream.m_iRxGeneration
			&& tx.m_iTuningValue == m_iInputFrequency && rx.m_iTuningValue == m_iInputFrequency
			&& tx.m_sNetworkKey.IsEmpty() && rx.m_sNetworkKey.IsEmpty();
	}

	//! Фиксированная станция вычитывает отдельную очередь. Take отклоняет очередь с владельцем fanout.
	RICON_QueuedMediaBlock TakeInputBlock(RICON_RelayPcmPath path, int nowMs)
	{
		if (!CanObserveTime(nowMs) || !CanUseInput(path, nowMs)) return null;
		RICON_MediaStreamBinding origin = m_Path.m_UpstreamRoute.m_Source;
		return m_SourceOwner.Take(origin.m_iSessionEpoch, origin.m_iLeaseId, nowMs);
	}

	bool CanUseSharedInputFanout(RICON_RelayPcmPath path, RICON_MediaFanout fanout, int nowMs)
	{
		if (!fanout || !CanObserveTime(nowMs) || !CanUseInput(path, nowMs)) return false;
		return m_SourceOwner.CanFanOut(m_Path.m_UpstreamRoute.m_Source, m_Lifecycle, fanout, true, nowMs);
	}

	//! The caller also rechecks current authority at evaluation time.
	bool MatchesOutputEndpoints(RICON_RelayPcmPath path, RICON_RadioObservation tx, RICON_RadioObservation rx)
	{
		if (!m_Path || !m_Path.SamePath(path) || !tx || !rx || !tx.m_bPowered || !rx.m_bPowered) return false;
		return tx.m_iDeviceId == m_Path.m_iRelayTxDeviceId && tx.m_iTransceiverIndex == m_Path.m_iRelayTxIndex
			&& rx.m_iDeviceId == m_Path.m_iDestinationEndpointId && rx.m_iTransceiverIndex == m_iDestinationIndex
			&& tx.m_iTuningValue == m_iOutputFrequency && rx.m_iTuningValue == m_iOutputFrequency
			&& tx.m_sNetworkKey.IsEmpty() && rx.m_sNetworkKey.IsEmpty();
	}

	//! Classify a stale timestamp without observing it or invalidating a station grant.
	bool CanObserveTime(int nowMs)
	{
		return nowMs >= 0 && nowMs >= m_iLastNowMs
			&& (!m_SourceOwner || nowMs >= m_SourceOwner.LastObservedTimeMs())
			&& (!m_Receivers || nowMs >= m_Receivers.LastWriteTimeMs())
			&& (!m_Lifecycle || nowMs >= m_Lifecycle.LastObservedTimeMs());
	}

	bool OwnsGeometryScope(RICON_StaticLosGeometryAuthority geometry)
	{
		return geometry && geometry.IsLifecycleScope(m_Lifecycle, m_Receivers);
	}

	RICON_RelayDestination ReadDestinationAddress(RICON_RelayPcmPath path, int nowMs)
	{
		if (!CanObserveTime(nowMs) || !CanUseOutput(path, nowMs)) return null;
		RICON_RelayDestination destination = new RICON_RelayDestination();
		destination.m_iPlayerId = m_iListenerPlayerId;
		destination.m_iConnectionId = m_iListenerConnectionId;
		destination.m_iPlayerGeneration = m_iListenerGeneration;
		destination.m_iEntityId = m_iListenerEntityId;
		destination.m_iRxDeviceId = m_Path.m_iDestinationEndpointId;
		destination.m_iRxIndex = m_iDestinationIndex;
		destination.m_iRxGeneration = m_Path.m_iDestinationGeneration;
		return destination;
	}

	void RevokeInput() { m_bInputLive = false; }
	void RevokeOutput() { m_bOutputLive = false; }

	//! The owner forwards committed effects so an entity round trip cannot restore the old destination grant.
	bool ConsumeLifecycle(RICON_LifecycleResult result, int preEventEpoch)
	{
		if (!m_Path || !result || !result.m_aEffects) return false;
		RICON_MediaStreamBinding origin = m_Path.m_UpstreamRoute.m_Source;
		if (preEventEpoch != origin.m_iSessionEpoch) return false;
		bool revoked = false;
		foreach (RICON_LifecycleEffect effect : result.m_aEffects)
		{
			if (!effect) continue;
			if (m_bInputLive && EndsOrigin(effect, origin))
			{
				RevokeInput();
				revoked = true;
			}
			if (m_bOutputLive && EndsDestination(effect))
			{
				RevokeOutput();
				revoked = true;
			}
		}
		return revoked;
	}

	void Close()
	{
		m_bClosed = true;
		RevokeInput();
		RevokeOutput();
		m_Path = null;
		m_Lifecycle = null;
		m_SourceOwner = null;
		m_Receivers = null;
		m_Station = null;
		m_StationRadio = null;
	}

	protected bool NativeStationCurrent()
	{
		if (!m_bRequiresNativeStation) return true;
		if (!m_Station || !m_StationRadio || !m_Path || !m_Receivers || !m_Station.MatchesRadio(m_StationRadio)) return false;
		IEntity owner = m_Station.GetOwner();
		if (!owner || owner.IsDeleted() || BaseRadioComponent.Cast(owner.FindComponent(BaseRadioComponent)) != m_StationRadio) return false;
		RICON_MediaEgressRoute upstream = m_Path.m_UpstreamRoute;
		return m_Receivers.MatchesRadio(upstream.m_iRxDeviceId, upstream.m_iRxTransceiverIndex, upstream.m_iRxGeneration, m_StationRadio)
			&& m_Receivers.MatchesRadio(m_Path.m_iRelayTxDeviceId, m_Path.m_iRelayTxIndex, m_Path.m_iRelayTxGeneration, m_StationRadio);
	}

	protected bool ObservePath(RICON_RelayPcmPath path, int nowMs)
	{
		if (!m_Path || !m_Path.SamePath(path) || nowMs < m_iLastNowMs) return false;
		if (!m_SourceOwner.IsScope(m_Lifecycle) || !m_Receivers.IsScope(m_Lifecycle) || !NativeStationCurrent())
		{
			Close();
			return false;
		}
		if (nowMs < m_SourceOwner.LastObservedTimeMs() || !m_Receivers.CanRead(m_Lifecycle, nowMs)) return false;
		m_iLastNowMs = nowMs;
		return true;
	}

	protected int ReadOrigin(int nowMs)
	{
		RICON_MediaStreamBinding origin = m_Path.m_UpstreamRoute.m_Source;
		if (!m_SourceOwner.IsSourceCurrent(origin, m_Lifecycle, nowMs)) return 0;
		return m_Receivers.ReadOpenEndpointAdmission(origin.m_iDeviceId, origin.m_iTxTransceiverIndex, m_iOriginTxGeneration,
			origin.m_iNetworkTypeId, origin.m_iChannelId, m_iInputFrequency, nowMs);
	}

	protected int ReadRelayRx(int nowMs)
	{
		RICON_MediaEgressRoute upstream = m_Path.m_UpstreamRoute;
		RICON_MediaStreamBinding origin = upstream.m_Source;
		if (!StationDeviceAllows(upstream.m_iRxDeviceId, origin.m_iNetworkTypeId, origin.m_iChannelId, nowMs)) return 0;
		return m_Receivers.ReadOpenEndpointAdmission(upstream.m_iRxDeviceId, upstream.m_iRxTransceiverIndex, upstream.m_iRxGeneration,
			origin.m_iNetworkTypeId, origin.m_iChannelId, m_iInputFrequency, nowMs);
	}

	protected int ReadRelayTx(int nowMs)
	{
		if (!StationDeviceAllows(m_Path.m_iRelayTxDeviceId, m_iOutputNetworkId, m_iOutputChannelId, nowMs)) return 0;
		return m_Receivers.ReadOpenEndpointAdmission(m_Path.m_iRelayTxDeviceId, m_Path.m_iRelayTxIndex, m_Path.m_iRelayTxGeneration,
			m_iOutputNetworkId, m_iOutputChannelId, m_iOutputFrequency, nowMs);
	}

	protected int ReadDestination(int nowMs)
	{
		int playerId = m_iListenerPlayerId;
		if (m_Lifecycle.GetPlayerState(playerId) != RICON_EPlayerLifecycleState.BOUND_ACTIVE
			|| m_Lifecycle.GetPlayerConnectionId(playerId) != m_iListenerConnectionId
			|| m_Lifecycle.GetPlayerGeneration(playerId) != m_iListenerGeneration
			|| m_Lifecycle.GetPlayerEntityId(playerId) != m_iListenerEntityId) return 0;
		RICON_LifecycleDevice device = m_Lifecycle.QueryDevice(m_Path.m_iDestinationEndpointId, nowMs);
		if (!device || !device.m_bPowered || !device.CanAccess(playerId) || !device.Allows(m_iOutputNetworkId, m_iOutputChannelId)) return 0;
		return m_Receivers.ReadOpenEndpointAdmission(m_Path.m_iDestinationEndpointId, m_iDestinationIndex, m_Path.m_iDestinationGeneration,
			m_iOutputNetworkId, m_iOutputChannelId, m_iOutputFrequency, nowMs);
	}

	protected bool StationDeviceAllows(int deviceId, int networkId, int channelId, int nowMs)
	{
		RICON_LifecycleDevice device = m_Lifecycle.QueryDevice(deviceId, nowMs);
		return device && device.m_eKind == RICON_EDeviceKind.WORLD && device.m_iOwnerPlayerId == 0
			&& device.m_bPowered && device.Allows(networkId, channelId);
	}

	protected static bool EndsOrigin(RICON_LifecycleEffect effect, RICON_MediaStreamBinding origin)
	{
		if (effect.m_iPlayerId != origin.m_iPlayerId || effect.m_iGeneration != origin.m_iPlayerGeneration) return false;
		if (effect.m_eType == RICON_ELifecycleEffectType.LEASE_TERMINATED)
			return effect.m_iLeaseId == origin.m_iLeaseId && effect.m_iDeviceId == origin.m_iDeviceId;
		return effect.m_eType == RICON_ELifecycleEffectType.ENTITY_UNBOUND && effect.m_iEntityId == origin.m_iEntityId;
	}

	protected bool EndsDestination(RICON_LifecycleEffect effect)
	{
		if (effect.m_iPlayerId != m_iListenerPlayerId || effect.m_iGeneration != m_iListenerGeneration) return false;
		return effect.m_eType == RICON_ELifecycleEffectType.SUBSCRIPTIONS_CLEARED
			|| (effect.m_eType == RICON_ELifecycleEffectType.ENTITY_UNBOUND && effect.m_iEntityId == m_iListenerEntityId);
	}
}
