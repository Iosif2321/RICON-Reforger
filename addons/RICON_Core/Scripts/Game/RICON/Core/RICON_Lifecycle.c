// Pure server-authority lifecycle state machine (REQ-lifecycle v0.3 @ 5993d9e).
// Status: STATIC_IMPLEMENTED; recompile required after post-parity review. No Rpl, GameMode callbacks or VoN actuator.

enum RICON_ELifecycleResult
{
	MALFORMED = 1,
	TICK_ANOMALY = 2,
	NOT_READY = 3,
	INVALID_SESSION = 4,
	STALE_CONNECTION = 5,
	STALE_GENERATION = 6,
	STALE_ENTITY = 7,
	DUPLICATE = 8,
	REQUEST_CONFLICT = 9,
	STALE_REQUEST = 10,
	LIMIT_REACHED = 11,
	RATE_LIMITED = 12,
	FORGED_OWNER = 13,
	INACTIVE_ENTITY = 14,
	UNKNOWN_DEVICE = 15,
	NOT_OWNER = 16,
	POWER_OFF = 17,
	INVALID_CATALOG = 18,
	DEVICE_BUSY = 19,
	UNKNOWN_LEASE = 20,
	STALE_LEASE = 21,
	OK = 22
}

enum RICON_EPlayerLifecycleState
{
	NONE = 0,
	CONNECTED_UNREGISTERED = 1,
	REGISTERED_UNBOUND = 2,
	BOUND_ACTIVE = 3,
	BOUND_INACTIVE = 4,
	DISCONNECTED = 5
}

enum RICON_ETransmitLeaseState
{
	NONE = 0,
	ACTIVE = 1,
	EXPIRED = 2,
	REVOKED = 3
}

enum RICON_ELeaseTerminalReason
{
	NONE = 0,
	TTL_EXPIRED = 1,
	CLIENT_STOP = 2,
	DISCONNECT = 3,
	RECONNECT = 4,
	ENTITY_CHANGED = 5,
	ENTITY_INACTIVE = 6,
	ACCESS_LOST = 7,
	POWER_LOST = 8,
	DEVICE_REMOVED = 9,
	CATALOG_INVALIDATED = 10,
	SESSION_ROTATED = 11,
	ADMIN_REVOKED = 12,
	TICK_ANOMALY = 13
}

enum RICON_ELifecycleEffectType
{
	PLAYER_STATE_CHANGED = 1,
	ENTITY_BOUND = 2,
	ENTITY_UNBOUND = 3,
	LEASE_GRANTED = 4,
	LEASE_TERMINATED = 5,
	VON_EXPIRY_INTENDED = 6,
	SUBSCRIPTIONS_CLEARED = 7,
	CARRIED_DEVICES_DETACHED = 8,
	TOMBSTONE_WRITTEN = 9,
	FULL_RESYNC_REQUIRED = 10
}

enum RICON_ELifecycleMethod
{
	PTT_START = 1,
	PTT_STOP = 2
}

enum RICON_EDeviceKind
{
	CARRIED = 1,
	WORLD = 2
}

enum RICON_EEntityUnbindReason
{
	KILLED = 1,
	DELETED = 2,
	CONTROL_LOST = 3
}

class RICON_LifecycleConfig
{
	int m_iMaxPlayers;
	int m_iMaxActiveLeases;
	int m_iMaxTerminalLeases;
	int m_iTerminalLeaseTtlMs;
	int m_iMaxTombstones;
	int m_iTombstoneTtlMs;
	int m_iMaxResponseCachePerPlayer;
	int m_iMaxRateEntries;
	int m_iRateWindowMs;
	int m_iRateMaxRequests;

	bool IsValid()
	{
		return m_iMaxPlayers > 0 && m_iMaxActiveLeases > 0 && m_iMaxTerminalLeases > 0
			&& m_iTerminalLeaseTtlMs > 0 && m_iMaxTombstones > 0 && m_iTombstoneTtlMs > 0
			&& m_iMaxResponseCachePerPlayer > 0 && m_iMaxRateEntries > 0
			&& m_iRateWindowMs > 0 && m_iRateMaxRequests > 0;
	}

	RICON_LifecycleConfig CloneConfig()
	{
		RICON_LifecycleConfig copy = new RICON_LifecycleConfig();
		copy.m_iMaxPlayers = m_iMaxPlayers;
		copy.m_iMaxActiveLeases = m_iMaxActiveLeases;
		copy.m_iMaxTerminalLeases = m_iMaxTerminalLeases;
		copy.m_iTerminalLeaseTtlMs = m_iTerminalLeaseTtlMs;
		copy.m_iMaxTombstones = m_iMaxTombstones;
		copy.m_iTombstoneTtlMs = m_iTombstoneTtlMs;
		copy.m_iMaxResponseCachePerPlayer = m_iMaxResponseCachePerPlayer;
		copy.m_iMaxRateEntries = m_iMaxRateEntries;
		copy.m_iRateWindowMs = m_iRateWindowMs;
		copy.m_iRateMaxRequests = m_iRateMaxRequests;
		return copy;
	}

	bool EqualsConfig(RICON_LifecycleConfig other)
	{
		return other && m_iMaxPlayers == other.m_iMaxPlayers
			&& m_iMaxActiveLeases == other.m_iMaxActiveLeases
			&& m_iMaxTerminalLeases == other.m_iMaxTerminalLeases
			&& m_iTerminalLeaseTtlMs == other.m_iTerminalLeaseTtlMs
			&& m_iMaxTombstones == other.m_iMaxTombstones
			&& m_iTombstoneTtlMs == other.m_iTombstoneTtlMs
			&& m_iMaxResponseCachePerPlayer == other.m_iMaxResponseCachePerPlayer
			&& m_iMaxRateEntries == other.m_iMaxRateEntries
			&& m_iRateWindowMs == other.m_iRateWindowMs
			&& m_iRateMaxRequests == other.m_iRateMaxRequests;
	}
}

class RICON_LifecycleCatalogPair
{
	int m_iNetworkTypeId;
	int m_iChannelId;

	void RICON_LifecycleCatalogPair(int networkTypeId, int channelId)
	{
		m_iNetworkTypeId = networkTypeId;
		m_iChannelId = channelId;
	}
}

class RICON_LifecycleDevice
{
	int m_iDeviceId;
	RICON_EDeviceKind m_eKind;
	int m_iOwnerPlayerId;
	bool m_bPowered;
	ref array<int> m_aAccessPlayerIds = {};
	ref array<ref RICON_LifecycleCatalogPair> m_aAllowed = {};

	bool IsValid()
	{
		if (m_iDeviceId <= 0 || (m_eKind != RICON_EDeviceKind.CARRIED && m_eKind != RICON_EDeviceKind.WORLD)
			|| m_iOwnerPlayerId < 0 || !m_aAccessPlayerIds || !m_aAllowed)
			return false;

		int previousAccess = 0;
		foreach (int playerId : m_aAccessPlayerIds)
		{
			if (playerId <= 0 || playerId <= previousAccess)
				return false;
			previousAccess = playerId;
		}

		int previousNetwork = 0;
		int previousChannel = 0;
		foreach (RICON_LifecycleCatalogPair pair : m_aAllowed)
		{
			if (!pair || pair.m_iNetworkTypeId <= 0 || pair.m_iChannelId <= 0)
				return false;
			if (pair.m_iNetworkTypeId < previousNetwork
				|| (pair.m_iNetworkTypeId == previousNetwork && pair.m_iChannelId <= previousChannel))
				return false;
			previousNetwork = pair.m_iNetworkTypeId;
			previousChannel = pair.m_iChannelId;
		}
		return true;
	}

	bool CanAccess(int playerId)
	{
		if (m_iOwnerPlayerId == playerId)
			return true;
		return m_aAccessPlayerIds.Contains(playerId);
	}

	bool Allows(int networkTypeId, int channelId)
	{
		foreach (RICON_LifecycleCatalogPair pair : m_aAllowed)
		{
			if (pair.m_iNetworkTypeId == networkTypeId && pair.m_iChannelId == channelId)
				return true;
		}
		return false;
	}

	RICON_LifecycleDevice CloneDevice()
	{
		RICON_LifecycleDevice copy = new RICON_LifecycleDevice();
		copy.m_iDeviceId = m_iDeviceId;
		copy.m_eKind = m_eKind;
		copy.m_iOwnerPlayerId = m_iOwnerPlayerId;
		copy.m_bPowered = m_bPowered;
		foreach (int playerId : m_aAccessPlayerIds)
			copy.m_aAccessPlayerIds.Insert(playerId);
		foreach (RICON_LifecycleCatalogPair pair : m_aAllowed)
			copy.m_aAllowed.Insert(new RICON_LifecycleCatalogPair(pair.m_iNetworkTypeId, pair.m_iChannelId));
		return copy;
	}

	bool EqualsDevice(RICON_LifecycleDevice other)
	{
		if (!other || m_iDeviceId != other.m_iDeviceId || m_eKind != other.m_eKind
			|| m_iOwnerPlayerId != other.m_iOwnerPlayerId || m_bPowered != other.m_bPowered
			|| m_aAccessPlayerIds.Count() != other.m_aAccessPlayerIds.Count()
			|| m_aAllowed.Count() != other.m_aAllowed.Count())
			return false;
		for (int i = 0; i < m_aAccessPlayerIds.Count(); i++)
		{
			if (m_aAccessPlayerIds[i] != other.m_aAccessPlayerIds[i])
				return false;
		}
		for (int j = 0; j < m_aAllowed.Count(); j++)
		{
			if (m_aAllowed[j].m_iNetworkTypeId != other.m_aAllowed[j].m_iNetworkTypeId
				|| m_aAllowed[j].m_iChannelId != other.m_aAllowed[j].m_iChannelId)
				return false;
		}
		return true;
	}
}

class RICON_LifecycleEffect
{
	// Only fields listed for m_eType in REQ-lifecycle §1.1 are semantically meaningful; others stay at defaults.
	RICON_ELifecycleEffectType m_eType;
	int m_iPlayerId;
	int m_iGeneration;
	int m_iOldGeneration;
	int m_iNewGeneration;
	RICON_EPlayerLifecycleState m_eFromState;
	RICON_EPlayerLifecycleState m_eToState;
	int m_iEntityId;
	int m_iDeviceId;
	int m_iLeaseId;
	int m_iIssuedAtMs;
	int m_iTtlMs;
	RICON_ETransmitLeaseState m_eLeaseState;
	RICON_ELeaseTerminalReason m_eTerminalReason;
	int m_iConnectionId;
	int m_iExpiresAtMs;
	int m_iSessionEpoch;
}

class RICON_LifecycleResult
{
	RICON_ELifecycleResult m_eResultCode;
	bool m_bHasCachedResult;
	RICON_ELifecycleResult m_eCachedResultCode;
	bool m_bHasLease;
	int m_iLeaseId;
	RICON_ETransmitLeaseState m_eLeaseState;
	RICON_ELeaseTerminalReason m_eTerminalReason;
	ref array<ref RICON_LifecycleEffect> m_aEffects = {};

	void RICON_LifecycleResult(RICON_ELifecycleResult resultCode)
	{
		m_eResultCode = resultCode;
	}
}

class RICON_LifecycleFingerprint
{
	RICON_ELifecycleMethod m_eMethod;
	int m_iCorrelationId;
	int m_iPayloadPlayerId;
	int m_iSessionEpoch;
	int m_iGeneration;
	int m_iRequestSeq;
	int m_iDeviceId;
	int m_iNetworkTypeId;
	int m_iChannelId;
	int m_iLeaseId;

	bool EqualsFingerprint(RICON_LifecycleFingerprint other)
	{
		return other && m_eMethod == other.m_eMethod && m_iCorrelationId == other.m_iCorrelationId
			&& m_iPayloadPlayerId == other.m_iPayloadPlayerId && m_iSessionEpoch == other.m_iSessionEpoch
			&& m_iGeneration == other.m_iGeneration && m_iRequestSeq == other.m_iRequestSeq
			&& m_iDeviceId == other.m_iDeviceId && m_iNetworkTypeId == other.m_iNetworkTypeId
			&& m_iChannelId == other.m_iChannelId && m_iLeaseId == other.m_iLeaseId;
	}
}

class RICON_LifecyclePttRequest
{
	RICON_ELifecycleMethod m_eMethod;
	int m_iNowMs;
	int m_iSchemaVersion = 1;
	int m_iCorrelationId;
	int m_iAuthenticatedPlayerId;
	int m_iPayloadPlayerId;
	int m_iSessionEpoch;
	int m_iGeneration;
	int m_iRequestSeq;
	int m_iDeviceId;
	int m_iNetworkTypeId;
	int m_iChannelId;
	int m_iServerTtlMs;
	int m_iLeaseId;

	RICON_LifecycleFingerprint MakeFingerprint()
	{
		RICON_LifecycleFingerprint fingerprint = new RICON_LifecycleFingerprint();
		fingerprint.m_eMethod = m_eMethod;
		fingerprint.m_iCorrelationId = m_iCorrelationId;
		fingerprint.m_iPayloadPlayerId = m_iPayloadPlayerId;
		fingerprint.m_iSessionEpoch = m_iSessionEpoch;
		fingerprint.m_iGeneration = m_iGeneration;
		fingerprint.m_iRequestSeq = m_iRequestSeq;
		if (m_eMethod == RICON_ELifecycleMethod.PTT_START)
		{
			fingerprint.m_iDeviceId = m_iDeviceId;
			fingerprint.m_iNetworkTypeId = m_iNetworkTypeId;
			fingerprint.m_iChannelId = m_iChannelId;
		}
		else if (m_eMethod == RICON_ELifecycleMethod.PTT_STOP)
		{
			fingerprint.m_iLeaseId = m_iLeaseId;
		}
		return fingerprint;
	}

	bool IsStructurallyValid()
	{
		if (m_iSchemaVersion != 1 || m_iCorrelationId <= 0 || m_iAuthenticatedPlayerId <= 0
			|| m_iPayloadPlayerId <= 0 || m_iSessionEpoch <= 0 || m_iGeneration <= 0 || m_iRequestSeq <= 0)
			return false;
		if (m_eMethod == RICON_ELifecycleMethod.PTT_START)
			return m_iDeviceId > 0 && m_iNetworkTypeId > 0 && m_iChannelId > 0 && m_iServerTtlMs > 0;
		if (m_eMethod == RICON_ELifecycleMethod.PTT_STOP)
			return m_iLeaseId > 0;
		return false;
	}
}

class RICON_LifecycleCachedResponse
{
	int m_iRequestSeq;
	ref RICON_LifecycleFingerprint m_Fingerprint;
	RICON_ELifecycleResult m_eResultCode;
	bool m_bHasLease;
	int m_iLeaseId;
	RICON_ETransmitLeaseState m_eLeaseState;
	RICON_ELeaseTerminalReason m_eTerminalReason;
}

class RICON_LifecyclePlayer
{
	int m_iPlayerId;
	int m_iConnectionId;
	int m_iGeneration;
	RICON_EPlayerLifecycleState m_eState;
	int m_iEntityId;
	int m_iLastRequestSeq;
	bool m_bFullResyncRequired;
	ref array<ref RICON_LifecycleCachedResponse> m_aCache = {};
}

class RICON_LifecycleLease
{
	int m_iLeaseId;
	int m_iPlayerId;
	int m_iGeneration;
	int m_iDeviceId;
	int m_iIssuedAtMs;
	int m_iTtlMs;
	int m_iTerminalAtMs;
	RICON_ETransmitLeaseState m_eState;
	RICON_ELeaseTerminalReason m_eTerminalReason;
}

class RICON_LifecycleTombstone
{
	int m_iPlayerId;
	int m_iGeneration;
	int m_iConnectionId;
	int m_iLastRequestSeq;
	int m_iExpiresAtMs;
}

class RICON_LifecycleRateEntry
{
	int m_iPlayerId;
	RICON_ELifecycleMethod m_eMethod;
	int m_iWindowStartMs;
	int m_iCount;
}

class RICON_LifecycleStateMachine
{
	protected bool m_bReady;
	protected bool m_bTickAnomaly;
	protected int m_iSessionEpoch;
	protected int m_iLastObservedNowMs;
	protected int m_iGenerationAllocator;
	protected int m_iLeaseAllocator;
	protected ref RICON_LifecycleConfig m_Config;
	protected ref array<ref RICON_LifecycleDevice> m_aDevices = {};
	protected ref array<ref RICON_LifecyclePlayer> m_aPlayers = {};
	protected ref array<ref RICON_LifecycleLease> m_aLeases = {};
	protected ref array<ref RICON_LifecycleTombstone> m_aTombstones = {};
	protected ref array<ref RICON_LifecycleRateEntry> m_aRateEntries = {};

	RICON_LifecycleResult Initialize(
		int sessionEpoch,
		int nowMs,
		RICON_LifecycleConfig config,
		array<ref RICON_LifecycleDevice> devices)
	{
		if (m_bReady)
		{
			if (nowMs < 0)
				return NewResult(RICON_ELifecycleResult.MALFORMED);
			RICON_LifecycleResult observed = NewResult(RICON_ELifecycleResult.OK);
			if (!BeginEvent(nowMs, observed))
				return observed;
			if (sessionEpoch == m_iSessionEpoch && config && devices
				&& m_Config.EqualsConfig(config) && DevicesEqual(devices))
			{
				observed.m_eResultCode = RICON_ELifecycleResult.DUPLICATE;
				return observed;
			}
			observed.m_eResultCode = RICON_ELifecycleResult.REQUEST_CONFLICT;
			return observed;
		}

		if (nowMs < 0 || sessionEpoch <= 0 || !config || !devices || !AreDevicesCanonical(devices))
			return NewResult(RICON_ELifecycleResult.MALFORMED);
		if (!config.IsValid())
			return NewResult(RICON_ELifecycleResult.NOT_READY);

		m_Config = config.CloneConfig();
		m_aDevices.Clear();
		foreach (RICON_LifecycleDevice device : devices)
			m_aDevices.Insert(device.CloneDevice());
		m_iSessionEpoch = sessionEpoch;
		m_iLastObservedNowMs = nowMs;
		m_bReady = true;
		return NewResult(RICON_ELifecycleResult.OK);
	}

	RICON_LifecycleResult Connected(int nowMs, int playerId, int connectionId)
	{
		RICON_LifecycleResult result = NewResult(RICON_ELifecycleResult.OK);
		if (!BeginEvent(nowMs, result))
			return result;
		if (playerId <= 0 || connectionId <= 0)
			return SetCode(result, RICON_ELifecycleResult.MALFORMED);

		int playerIndex = FindPlayerIndex(playerId);
		RICON_LifecyclePlayer current;
		if (playerIndex >= 0)
			current = m_aPlayers[playerIndex];
		if (current && current.m_iConnectionId == connectionId)
			return SetCode(result, RICON_ELifecycleResult.DUPLICATE);
		if ((!current && m_aPlayers.Count() >= m_Config.m_iMaxPlayers) || m_iGenerationAllocator >= int.MAX)
			return SetCode(result, RICON_ELifecycleResult.LIMIT_REACHED);

		int oldGeneration = 0;
		RICON_EPlayerLifecycleState fromState = RICON_EPlayerLifecycleState.NONE;
		if (current)
		{
			oldGeneration = current.m_iGeneration;
			CleanupPlayer(playerIndex, RICON_ELeaseTerminalReason.RECONNECT, nowMs, result);
			fromState = RICON_EPlayerLifecycleState.DISCONNECTED;
		}

		m_iGenerationAllocator++;
		RICON_LifecyclePlayer player = new RICON_LifecyclePlayer();
		player.m_iPlayerId = playerId;
		player.m_iConnectionId = connectionId;
		player.m_iGeneration = m_iGenerationAllocator;
		player.m_eState = RICON_EPlayerLifecycleState.CONNECTED_UNREGISTERED;
		player.m_bFullResyncRequired = true;
		m_aPlayers.Insert(player);
		AddPlayerStateEffect(result, playerId, oldGeneration, player.m_iGeneration, fromState,
			RICON_EPlayerLifecycleState.CONNECTED_UNREGISTERED);
		AddFullResyncEffect(result, playerId, m_iSessionEpoch, player.m_iGeneration);
		return result;
	}

	RICON_LifecycleResult Registered(int nowMs, int playerId, int generation)
	{
		RICON_LifecycleResult result = NewResult(RICON_ELifecycleResult.OK);
		if (!BeginEvent(nowMs, result))
			return result;
		if (playerId <= 0 || generation <= 0)
			return SetCode(result, RICON_ELifecycleResult.MALFORMED);
		RICON_LifecyclePlayer player = ResolvePlayer(playerId, generation, result);
		if (!player)
			return result;
		if (generation != player.m_iGeneration)
			return SetCode(result, RICON_ELifecycleResult.STALE_GENERATION);
		if (player.m_eState != RICON_EPlayerLifecycleState.CONNECTED_UNREGISTERED)
			return SetCode(result, RICON_ELifecycleResult.DUPLICATE);
		RICON_EPlayerLifecycleState previous = player.m_eState;
		player.m_eState = RICON_EPlayerLifecycleState.REGISTERED_UNBOUND;
		AddPlayerStateEffect(result, playerId, generation, generation, previous, player.m_eState);
		return result;
	}

	RICON_LifecycleResult EntityBound(int nowMs, int playerId, int generation, int entityId)
	{
		RICON_LifecycleResult result = NewResult(RICON_ELifecycleResult.OK);
		if (!BeginEvent(nowMs, result))
			return result;
		if (playerId <= 0 || generation <= 0 || entityId <= 0)
			return SetCode(result, RICON_ELifecycleResult.MALFORMED);
		RICON_LifecyclePlayer player = ResolvePlayer(playerId, generation, result);
		if (!player)
			return result;
		if (player.m_eState == RICON_EPlayerLifecycleState.CONNECTED_UNREGISTERED)
			return SetCode(result, RICON_ELifecycleResult.NOT_READY);
		if (generation != player.m_iGeneration)
			return SetCode(result, RICON_ELifecycleResult.STALE_GENERATION);
		if (player.m_eState == RICON_EPlayerLifecycleState.BOUND_ACTIVE)
		{
			if (player.m_iEntityId == entityId)
				return SetCode(result, RICON_ELifecycleResult.DUPLICATE);
			return SetCode(result, RICON_ELifecycleResult.STALE_ENTITY);
		}
		if (player.m_eState != RICON_EPlayerLifecycleState.REGISTERED_UNBOUND
			&& player.m_eState != RICON_EPlayerLifecycleState.BOUND_INACTIVE)
			return SetCode(result, RICON_ELifecycleResult.NOT_READY);

		RICON_EPlayerLifecycleState previous = player.m_eState;
		player.m_iEntityId = entityId;
		player.m_eState = RICON_EPlayerLifecycleState.BOUND_ACTIVE;
		AddEntityEffect(result, RICON_ELifecycleEffectType.ENTITY_BOUND, playerId, generation, entityId);
		AddPlayerStateEffect(result, playerId, generation, generation, previous, player.m_eState);
		return result;
	}

	RICON_LifecycleResult EntityChanged(int nowMs, int playerId, int generation, int fromEntityId, int toEntityId)
	{
		RICON_LifecycleResult result = NewResult(RICON_ELifecycleResult.OK);
		if (!BeginEvent(nowMs, result))
			return result;
		if (playerId <= 0 || generation <= 0 || fromEntityId <= 0 || toEntityId <= 0)
			return SetCode(result, RICON_ELifecycleResult.MALFORMED);
		RICON_LifecyclePlayer player = ResolvePlayer(playerId, generation, result);
		if (!player)
			return result;
		if (generation != player.m_iGeneration)
			return SetCode(result, RICON_ELifecycleResult.STALE_GENERATION);
		if (player.m_eState != RICON_EPlayerLifecycleState.BOUND_ACTIVE || player.m_iEntityId != fromEntityId)
			return SetCode(result, RICON_ELifecycleResult.STALE_ENTITY);
		if (fromEntityId == toEntityId)
			return SetCode(result, RICON_ELifecycleResult.DUPLICATE);

		TerminatePlayerLeases(playerId, RICON_ELeaseTerminalReason.ENTITY_CHANGED, nowMs, result);
		AddEntityEffect(result, RICON_ELifecycleEffectType.ENTITY_UNBOUND, playerId, generation, fromEntityId);
		AddSimplePlayerEffect(result, RICON_ELifecycleEffectType.CARRIED_DEVICES_DETACHED, playerId, generation);
		AddEntityEffect(result, RICON_ELifecycleEffectType.ENTITY_BOUND, playerId, generation, toEntityId);
		player.m_iEntityId = toEntityId;
		return result;
	}

	RICON_LifecycleResult EntityUnbound(
		int nowMs,
		int playerId,
		int generation,
		int entityId,
		RICON_EEntityUnbindReason reason)
	{
		RICON_LifecycleResult result = NewResult(RICON_ELifecycleResult.OK);
		if (!BeginEvent(nowMs, result))
			return result;
		if (playerId <= 0 || generation <= 0 || entityId <= 0
			|| (reason != RICON_EEntityUnbindReason.KILLED && reason != RICON_EEntityUnbindReason.DELETED
				&& reason != RICON_EEntityUnbindReason.CONTROL_LOST))
			return SetCode(result, RICON_ELifecycleResult.MALFORMED);
		RICON_LifecyclePlayer player = ResolvePlayer(playerId, generation, result);
		if (!player)
			return result;
		if (generation != player.m_iGeneration)
			return SetCode(result, RICON_ELifecycleResult.STALE_GENERATION);
		if (player.m_iEntityId != entityId)
			return SetCode(result, RICON_ELifecycleResult.STALE_ENTITY);

		TerminatePlayerLeases(playerId, RICON_ELeaseTerminalReason.ENTITY_INACTIVE, nowMs, result);
		AddEntityEffect(result, RICON_ELifecycleEffectType.ENTITY_UNBOUND, playerId, generation, entityId);
		AddSimplePlayerEffect(result, RICON_ELifecycleEffectType.CARRIED_DEVICES_DETACHED, playerId, generation);
		RICON_EPlayerLifecycleState previous = player.m_eState;
		player.m_iEntityId = 0;
		player.m_eState = RICON_EPlayerLifecycleState.BOUND_INACTIVE;
		AddPlayerStateEffect(result, playerId, generation, generation, previous, player.m_eState);
		return result;
	}

	RICON_LifecycleResult Disconnected(int nowMs, int playerId, int generation, int connectionId)
	{
		RICON_LifecycleResult result = NewResult(RICON_ELifecycleResult.OK);
		if (!BeginEvent(nowMs, result))
			return result;
		if (playerId <= 0 || generation <= 0 || connectionId <= 0)
			return SetCode(result, RICON_ELifecycleResult.MALFORMED);
		int playerIndex = FindPlayerIndex(playerId);
		if (playerIndex < 0)
		{
			RICON_LifecycleTombstone tombstone = FindLatestTombstone(playerId);
			if (tombstone && tombstone.m_iGeneration == generation && tombstone.m_iConnectionId == connectionId)
				return SetCode(result, RICON_ELifecycleResult.DUPLICATE);
			if (tombstone && generation <= tombstone.m_iGeneration)
				return SetCode(result, RICON_ELifecycleResult.STALE_GENERATION);
			return SetCode(result, RICON_ELifecycleResult.NOT_READY);
		}

		RICON_LifecyclePlayer player = m_aPlayers[playerIndex];
		if (connectionId != player.m_iConnectionId)
			return SetCode(result, RICON_ELifecycleResult.STALE_CONNECTION);
		if (generation != player.m_iGeneration)
			return SetCode(result, RICON_ELifecycleResult.STALE_GENERATION);
		CleanupPlayer(playerIndex, RICON_ELeaseTerminalReason.DISCONNECT, nowMs, result);
		return result;
	}

	RICON_LifecycleResult ProcessPtt(RICON_LifecyclePttRequest request)
	{
		RICON_LifecycleResult result = NewResult(RICON_ELifecycleResult.OK);
		if (!m_bReady)
			return SetCode(result, RICON_ELifecycleResult.NOT_READY);
		if (!request)
			return SetCode(result, RICON_ELifecycleResult.MALFORMED);
		if (!BeginEvent(request.m_iNowMs, result))
			return result;
		if (!request.IsStructurallyValid())
			return SetCode(result, RICON_ELifecycleResult.MALFORMED);

		RICON_LifecyclePlayer player = FindPlayer(request.m_iAuthenticatedPlayerId);
		if (!player)
		{
			RICON_LifecycleTombstone tombstone = FindLatestTombstone(request.m_iAuthenticatedPlayerId);
			if (!tombstone || request.m_iGeneration > tombstone.m_iGeneration)
				return SetCode(result, RICON_ELifecycleResult.NOT_READY);
			if (request.m_iSessionEpoch != m_iSessionEpoch)
				return SetCode(result, RICON_ELifecycleResult.INVALID_SESSION);
			return SetCode(result, RICON_ELifecycleResult.STALE_GENERATION);
		}
		if (request.m_iSessionEpoch != m_iSessionEpoch)
			return SetCode(result, RICON_ELifecycleResult.INVALID_SESSION);
		if (request.m_iGeneration != player.m_iGeneration)
			return SetCode(result, RICON_ELifecycleResult.STALE_GENERATION);

		RICON_LifecycleFingerprint fingerprint = request.MakeFingerprint();
		RICON_LifecycleCachedResponse cached = FindCached(player, request.m_iRequestSeq);
		if (cached)
		{
			if (!cached.m_Fingerprint.EqualsFingerprint(fingerprint))
				return SetCode(result, RICON_ELifecycleResult.REQUEST_CONFLICT);
			result.m_eResultCode = RICON_ELifecycleResult.DUPLICATE;
			result.m_bHasCachedResult = true;
			result.m_eCachedResultCode = cached.m_eResultCode;
			CopyCachedLease(cached, result);
			return result;
		}
		if (request.m_iRequestSeq <= player.m_iLastRequestSeq)
			return SetCode(result, RICON_ELifecycleResult.STALE_REQUEST);

		player.m_iLastRequestSeq = request.m_iRequestSeq;
		RICON_LifecycleResult response = ProcessNewPtt(request, player, result);
		CacheResponse(player, fingerprint, response);
		return response;
	}

	RICON_LifecycleResult Tick(int nowMs)
	{
		RICON_LifecycleResult result = NewResult(RICON_ELifecycleResult.OK);
		BeginEvent(nowMs, result);
		return result;
	}

	RICON_LifecycleResult RotateEpoch(int nowMs, int newEpoch)
	{
		RICON_LifecycleResult result = NewResult(RICON_ELifecycleResult.OK);
		if (!m_bReady)
			return SetCode(result, RICON_ELifecycleResult.NOT_READY);
		if (nowMs < 0)
			return SetCode(result, RICON_ELifecycleResult.MALFORMED);
		if (m_bTickAnomaly)
		{
			if (newEpoch <= m_iSessionEpoch)
				return SetCode(result, RICON_ELifecycleResult.MALFORMED);
		}
		else
		{
			if (!BeginEvent(nowMs, result))
				return result;
			if (newEpoch <= m_iSessionEpoch)
				return SetCode(result, RICON_ELifecycleResult.MALFORMED);
		}

		array<int> activeIds = ActiveLeaseIds();
		foreach (int leaseId : activeIds)
			TerminateLease(leaseId, RICON_ETransmitLeaseState.REVOKED,
				RICON_ELeaseTerminalReason.SESSION_ROTATED, nowMs, result);

		array<int> playerIds = {};
		foreach (RICON_LifecyclePlayer player : m_aPlayers)
			playerIds.Insert(player.m_iPlayerId);
		playerIds.Sort();
		m_iGenerationAllocator = 0;
		foreach (int playerId : playerIds)
		{
			RICON_LifecyclePlayer player = FindPlayer(playerId);
			int oldGeneration = player.m_iGeneration;
			AddSimplePlayerEffect(result, RICON_ELifecycleEffectType.SUBSCRIPTIONS_CLEARED, playerId, oldGeneration);
			if (player.m_iEntityId > 0)
				AddEntityEffect(result, RICON_ELifecycleEffectType.ENTITY_UNBOUND, playerId, oldGeneration, player.m_iEntityId);
			AddSimplePlayerEffect(result, RICON_ELifecycleEffectType.CARRIED_DEVICES_DETACHED, playerId, oldGeneration);
			m_iGenerationAllocator++;
			AddPlayerStateEffect(result, playerId, oldGeneration, m_iGenerationAllocator, player.m_eState,
				RICON_EPlayerLifecycleState.CONNECTED_UNREGISTERED);
			AddFullResyncEffect(result, playerId, newEpoch, m_iGenerationAllocator);
			player.m_iGeneration = m_iGenerationAllocator;
			player.m_eState = RICON_EPlayerLifecycleState.CONNECTED_UNREGISTERED;
			player.m_iEntityId = 0;
			player.m_iLastRequestSeq = 0;
			player.m_bFullResyncRequired = true;
			player.m_aCache.Clear();
		}

		m_aTombstones.Clear();
		m_aLeases.Clear();
		m_aRateEntries.Clear();
		m_iLeaseAllocator = 0;
		m_iSessionEpoch = newEpoch;
		m_iLastObservedNowMs = nowMs;
		m_bTickAnomaly = false;
		return result;
	}

	int GetSessionEpoch() { return m_iSessionEpoch; }
	int GetPlayerCount() { return m_aPlayers.Count(); }
	int GetLeaseCount() { return m_aLeases.Count(); }
	int GetTombstoneCount() { return m_aTombstones.Count(); }
	bool IsTickAnomalyLatched() { return m_bTickAnomaly; }

	RICON_EPlayerLifecycleState GetPlayerState(int playerId)
	{
		RICON_LifecyclePlayer player = FindPlayer(playerId);
		if (!player)
			return RICON_EPlayerLifecycleState.NONE;
		return player.m_eState;
	}

	int GetPlayerGeneration(int playerId)
	{
		RICON_LifecyclePlayer player = FindPlayer(playerId);
		if (!player)
			return 0;
		return player.m_iGeneration;
	}

	int GetPlayerEntityId(int playerId)
	{
		RICON_LifecyclePlayer player = FindPlayer(playerId);
		if (!player)
			return 0;
		return player.m_iEntityId;
	}

	protected RICON_LifecycleResult ProcessNewPtt(
		RICON_LifecyclePttRequest request,
		RICON_LifecyclePlayer player,
		RICON_LifecycleResult result)
	{
		int rateIndex = FindRateIndex(request.m_iAuthenticatedPlayerId, request.m_eMethod);
		if (rateIndex < 0 && m_aRateEntries.Count() >= m_Config.m_iMaxRateEntries)
			return SetCode(result, RICON_ELifecycleResult.LIMIT_REACHED);
		if (request.m_eMethod == RICON_ELifecycleMethod.PTT_START
			&& (CountActiveLeases() >= m_Config.m_iMaxActiveLeases || m_iLeaseAllocator >= int.MAX))
			return SetCode(result, RICON_ELifecycleResult.LIMIT_REACHED);

		RICON_LifecycleRateEntry rate;
		if (rateIndex < 0)
		{
			rate = new RICON_LifecycleRateEntry();
			rate.m_iPlayerId = request.m_iAuthenticatedPlayerId;
			rate.m_eMethod = request.m_eMethod;
			rate.m_iWindowStartMs = request.m_iNowMs;
			m_aRateEntries.Insert(rate);
		}
		else
		{
			rate = m_aRateEntries[rateIndex];
			if (request.m_iNowMs - rate.m_iWindowStartMs >= m_Config.m_iRateWindowMs)
			{
				rate.m_iWindowStartMs = request.m_iNowMs;
				rate.m_iCount = 0;
			}
		}

		if (rate.m_iCount >= m_Config.m_iRateMaxRequests)
			return SetCode(result, RICON_ELifecycleResult.RATE_LIMITED);
		rate.m_iCount++;
		if (request.m_iPayloadPlayerId != request.m_iAuthenticatedPlayerId)
			return SetCode(result, RICON_ELifecycleResult.FORGED_OWNER);

		if (request.m_eMethod == RICON_ELifecycleMethod.PTT_START)
			return StartTransmit(request, player, result);
		return StopTransmit(request, player, result);
	}

	protected RICON_LifecycleResult StartTransmit(
		RICON_LifecyclePttRequest request,
		RICON_LifecyclePlayer player,
		RICON_LifecycleResult result)
	{
		if (player.m_eState != RICON_EPlayerLifecycleState.BOUND_ACTIVE)
			return SetCode(result, RICON_ELifecycleResult.INACTIVE_ENTITY);
		RICON_LifecycleDevice device = FindDevice(request.m_iDeviceId);
		if (!device)
			return SetCode(result, RICON_ELifecycleResult.UNKNOWN_DEVICE);
		if (!device.CanAccess(request.m_iAuthenticatedPlayerId))
			return SetCode(result, RICON_ELifecycleResult.NOT_OWNER);
		if (!device.m_bPowered)
			return SetCode(result, RICON_ELifecycleResult.POWER_OFF);
		if (!device.Allows(request.m_iNetworkTypeId, request.m_iChannelId))
			return SetCode(result, RICON_ELifecycleResult.INVALID_CATALOG);
		if (FindActiveLeaseByDevice(request.m_iDeviceId))
			return SetCode(result, RICON_ELifecycleResult.DEVICE_BUSY);

		m_iLeaseAllocator++;
		RICON_LifecycleLease lease = new RICON_LifecycleLease();
		lease.m_iLeaseId = m_iLeaseAllocator;
		lease.m_iPlayerId = request.m_iAuthenticatedPlayerId;
		lease.m_iGeneration = player.m_iGeneration;
		lease.m_iDeviceId = request.m_iDeviceId;
		lease.m_iIssuedAtMs = request.m_iNowMs;
		lease.m_iTtlMs = request.m_iServerTtlMs;
		lease.m_eState = RICON_ETransmitLeaseState.ACTIVE;
		m_aLeases.Insert(lease);
		AddLeaseEffect(result, RICON_ELifecycleEffectType.LEASE_GRANTED, lease);
		result.m_bHasLease = true;
		result.m_iLeaseId = lease.m_iLeaseId;
		result.m_eLeaseState = lease.m_eState;
		return result;
	}

	protected RICON_LifecycleResult StopTransmit(
		RICON_LifecyclePttRequest request,
		RICON_LifecyclePlayer player,
		RICON_LifecycleResult result)
	{
		RICON_LifecycleLease lease = FindLease(request.m_iLeaseId);
		if (!lease)
			return SetCode(result, RICON_ELifecycleResult.UNKNOWN_LEASE);
		if (lease.m_iPlayerId != request.m_iAuthenticatedPlayerId || lease.m_iGeneration != player.m_iGeneration)
			return SetCode(result, RICON_ELifecycleResult.STALE_LEASE);
		if (lease.m_eState == RICON_ETransmitLeaseState.ACTIVE)
			TerminateLease(lease.m_iLeaseId, RICON_ETransmitLeaseState.REVOKED,
				RICON_ELeaseTerminalReason.CLIENT_STOP, request.m_iNowMs, result);
		result.m_bHasLease = true;
		result.m_iLeaseId = lease.m_iLeaseId;
		result.m_eLeaseState = lease.m_eState;
		result.m_eTerminalReason = lease.m_eTerminalReason;
		return result;
	}

	protected bool BeginEvent(int nowMs, RICON_LifecycleResult result)
	{
		if (!m_bReady)
		{
			result.m_eResultCode = RICON_ELifecycleResult.NOT_READY;
			return false;
		}
		if (nowMs < 0)
		{
			result.m_eResultCode = RICON_ELifecycleResult.MALFORMED;
			return false;
		}
		if (m_bTickAnomaly)
		{
			result.m_eResultCode = RICON_ELifecycleResult.TICK_ANOMALY;
			return false;
		}
		if (nowMs < m_iLastObservedNowMs)
		{
			m_bTickAnomaly = true;
			array<int> activeIds = ActiveLeaseIds();
			foreach (int leaseId : activeIds)
				TerminateLease(leaseId, RICON_ETransmitLeaseState.REVOKED,
					RICON_ELeaseTerminalReason.TICK_ANOMALY, nowMs, result);
			result.m_eResultCode = RICON_ELifecycleResult.TICK_ANOMALY;
			return false;
		}

		ObserveAndSweep(nowMs, result);
		return true;
	}

	protected void ObserveAndSweep(int nowMs, RICON_LifecycleResult result)
	{
		m_iLastObservedNowMs = nowMs;
		array<int> due = {};
		foreach (RICON_LifecycleLease lease : m_aLeases)
		{
			if (lease.m_eState == RICON_ETransmitLeaseState.ACTIVE
				&& nowMs - lease.m_iIssuedAtMs >= lease.m_iTtlMs)
				due.Insert(lease.m_iLeaseId);
		}
		due.Sort();
		foreach (int leaseId : due)
			TerminateLease(leaseId, RICON_ETransmitLeaseState.EXPIRED,
				RICON_ELeaseTerminalReason.TTL_EXPIRED, nowMs, result);

		for (int tombstoneIndex = m_aTombstones.Count() - 1; tombstoneIndex >= 0; tombstoneIndex--)
		{
			if (nowMs >= m_aTombstones[tombstoneIndex].m_iExpiresAtMs)
				m_aTombstones.RemoveOrdered(tombstoneIndex);
		}
		for (int leaseIndex = m_aLeases.Count() - 1; leaseIndex >= 0; leaseIndex--)
		{
			RICON_LifecycleLease terminal = m_aLeases[leaseIndex];
			if (terminal.m_eState != RICON_ETransmitLeaseState.ACTIVE
				&& nowMs - terminal.m_iTerminalAtMs >= m_Config.m_iTerminalLeaseTtlMs)
				m_aLeases.RemoveOrdered(leaseIndex);
		}
	}

	protected void CleanupPlayer(
		int playerIndex,
		RICON_ELeaseTerminalReason reason,
		int nowMs,
		RICON_LifecycleResult result)
	{
		RICON_LifecyclePlayer player = m_aPlayers[playerIndex];
		TerminatePlayerLeases(player.m_iPlayerId, reason, nowMs, result);
		AddSimplePlayerEffect(result, RICON_ELifecycleEffectType.SUBSCRIPTIONS_CLEARED,
			player.m_iPlayerId, player.m_iGeneration);
		if (player.m_iEntityId > 0)
			AddEntityEffect(result, RICON_ELifecycleEffectType.ENTITY_UNBOUND,
				player.m_iPlayerId, player.m_iGeneration, player.m_iEntityId);
		AddSimplePlayerEffect(result, RICON_ELifecycleEffectType.CARRIED_DEVICES_DETACHED,
			player.m_iPlayerId, player.m_iGeneration);
		AddPlayerStateEffect(result, player.m_iPlayerId, player.m_iGeneration, player.m_iGeneration,
			player.m_eState, RICON_EPlayerLifecycleState.DISCONNECTED);

		while (m_aTombstones.Count() >= m_Config.m_iMaxTombstones)
			RemoveOldestTombstone();
		RICON_LifecycleTombstone tombstone = new RICON_LifecycleTombstone();
		tombstone.m_iPlayerId = player.m_iPlayerId;
		tombstone.m_iGeneration = player.m_iGeneration;
		tombstone.m_iConnectionId = player.m_iConnectionId;
		tombstone.m_iLastRequestSeq = player.m_iLastRequestSeq;
		tombstone.m_iExpiresAtMs = SafeExpiresAt(nowMs, m_Config.m_iTombstoneTtlMs);
		m_aTombstones.Insert(tombstone);
		AddTombstoneEffect(result, tombstone);
		RemoveRateEntries(player.m_iPlayerId);
		m_aPlayers.RemoveOrdered(playerIndex);
	}

	protected void TerminatePlayerLeases(
		int playerId,
		RICON_ELeaseTerminalReason reason,
		int nowMs,
		RICON_LifecycleResult result)
	{
		array<int> leaseIds = {};
		foreach (RICON_LifecycleLease lease : m_aLeases)
		{
			if (lease.m_eState == RICON_ETransmitLeaseState.ACTIVE && lease.m_iPlayerId == playerId)
				leaseIds.Insert(lease.m_iLeaseId);
		}
		leaseIds.Sort();
		foreach (int leaseId : leaseIds)
			TerminateLease(leaseId, RICON_ETransmitLeaseState.REVOKED, reason, nowMs, result);
	}

	protected void TerminateLease(
		int leaseId,
		RICON_ETransmitLeaseState terminalState,
		RICON_ELeaseTerminalReason reason,
		int nowMs,
		RICON_LifecycleResult result)
	{
		RICON_LifecycleLease lease = FindLease(leaseId);
		if (!lease || lease.m_eState != RICON_ETransmitLeaseState.ACTIVE)
			return;
		lease.m_eState = terminalState;
		lease.m_eTerminalReason = reason;
		lease.m_iTerminalAtMs = nowMs;
		AddLeaseEffect(result, RICON_ELifecycleEffectType.LEASE_TERMINATED, lease);
		AddVonExpiryEffect(result, lease);
		UpdateCachedLease(lease);
		EvictTerminalLeases();
	}

	protected void EvictTerminalLeases()
	{
		while (CountTerminalLeases() > m_Config.m_iMaxTerminalLeases)
		{
			int oldestIndex = -1;
			for (int i = 0; i < m_aLeases.Count(); i++)
			{
				RICON_LifecycleLease candidate = m_aLeases[i];
				if (candidate.m_eState == RICON_ETransmitLeaseState.ACTIVE)
					continue;
				if (oldestIndex < 0 || candidate.m_iTerminalAtMs < m_aLeases[oldestIndex].m_iTerminalAtMs
					|| (candidate.m_iTerminalAtMs == m_aLeases[oldestIndex].m_iTerminalAtMs
						&& candidate.m_iLeaseId < m_aLeases[oldestIndex].m_iLeaseId))
					oldestIndex = i;
			}
			if (oldestIndex < 0)
				return;
			m_aLeases.RemoveOrdered(oldestIndex);
		}
	}

	protected void UpdateCachedLease(RICON_LifecycleLease lease)
	{
		RICON_LifecyclePlayer player = FindPlayer(lease.m_iPlayerId);
		if (!player || player.m_iGeneration != lease.m_iGeneration)
			return;
		foreach (RICON_LifecycleCachedResponse cached : player.m_aCache)
		{
			if (cached.m_bHasLease && cached.m_iLeaseId == lease.m_iLeaseId)
			{
				cached.m_eLeaseState = lease.m_eState;
				cached.m_eTerminalReason = lease.m_eTerminalReason;
			}
		}
	}

	protected void CacheResponse(
		RICON_LifecyclePlayer player,
		RICON_LifecycleFingerprint fingerprint,
		RICON_LifecycleResult response)
	{
		while (player.m_aCache.Count() >= m_Config.m_iMaxResponseCachePerPlayer)
		{
			int smallestIndex = 0;
			for (int i = 1; i < player.m_aCache.Count(); i++)
			{
				if (player.m_aCache[i].m_iRequestSeq < player.m_aCache[smallestIndex].m_iRequestSeq)
					smallestIndex = i;
			}
			player.m_aCache.RemoveOrdered(smallestIndex);
		}

		RICON_LifecycleCachedResponse cached = new RICON_LifecycleCachedResponse();
		cached.m_iRequestSeq = fingerprint.m_iRequestSeq;
		cached.m_Fingerprint = fingerprint;
		cached.m_eResultCode = response.m_eResultCode;
		cached.m_bHasLease = response.m_bHasLease;
		cached.m_iLeaseId = response.m_iLeaseId;
		cached.m_eLeaseState = response.m_eLeaseState;
		cached.m_eTerminalReason = response.m_eTerminalReason;
		player.m_aCache.Insert(cached);
	}

	protected bool AreDevicesCanonical(array<ref RICON_LifecycleDevice> devices)
	{
		int previousId = 0;
		foreach (RICON_LifecycleDevice device : devices)
		{
			if (!device || !device.IsValid() || device.m_iDeviceId <= previousId)
				return false;
			previousId = device.m_iDeviceId;
		}
		return true;
	}

	protected bool DevicesEqual(array<ref RICON_LifecycleDevice> devices)
	{
		if (devices.Count() != m_aDevices.Count())
			return false;
		for (int i = 0; i < devices.Count(); i++)
		{
			if (!m_aDevices[i].EqualsDevice(devices[i]))
				return false;
		}
		return true;
	}

	protected RICON_LifecyclePlayer ResolvePlayer(
		int playerId,
		int generation,
		RICON_LifecycleResult result)
	{
		RICON_LifecyclePlayer player = FindPlayer(playerId);
		if (player)
			return player;
		RICON_LifecycleTombstone tombstone = FindLatestTombstone(playerId);
		if (tombstone && generation <= tombstone.m_iGeneration)
			result.m_eResultCode = RICON_ELifecycleResult.STALE_GENERATION;
		else
			result.m_eResultCode = RICON_ELifecycleResult.NOT_READY;
		return null;
	}

	protected RICON_LifecyclePlayer FindPlayer(int playerId)
	{
		int index = FindPlayerIndex(playerId);
		if (index < 0)
			return null;
		return m_aPlayers[index];
	}

	protected int FindPlayerIndex(int playerId)
	{
		for (int i = 0; i < m_aPlayers.Count(); i++)
		{
			if (m_aPlayers[i].m_iPlayerId == playerId)
				return i;
		}
		return -1;
	}

	protected RICON_LifecycleTombstone FindLatestTombstone(int playerId)
	{
		RICON_LifecycleTombstone latest;
		foreach (RICON_LifecycleTombstone tombstone : m_aTombstones)
		{
			if (tombstone.m_iPlayerId == playerId && (!latest || tombstone.m_iGeneration > latest.m_iGeneration))
				latest = tombstone;
		}
		return latest;
	}

	protected RICON_LifecycleDevice FindDevice(int deviceId)
	{
		foreach (RICON_LifecycleDevice device : m_aDevices)
		{
			if (device.m_iDeviceId == deviceId)
				return device;
		}
		return null;
	}

	protected RICON_LifecycleLease FindLease(int leaseId)
	{
		foreach (RICON_LifecycleLease lease : m_aLeases)
		{
			if (lease.m_iLeaseId == leaseId)
				return lease;
		}
		return null;
	}

	protected RICON_LifecycleLease FindActiveLeaseByDevice(int deviceId)
	{
		foreach (RICON_LifecycleLease lease : m_aLeases)
		{
			if (lease.m_iDeviceId == deviceId && lease.m_eState == RICON_ETransmitLeaseState.ACTIVE)
				return lease;
		}
		return null;
	}

	protected RICON_LifecycleCachedResponse FindCached(RICON_LifecyclePlayer player, int requestSeq)
	{
		foreach (RICON_LifecycleCachedResponse cached : player.m_aCache)
		{
			if (cached.m_iRequestSeq == requestSeq)
				return cached;
		}
		return null;
	}

	protected int FindRateIndex(int playerId, RICON_ELifecycleMethod method)
	{
		for (int i = 0; i < m_aRateEntries.Count(); i++)
		{
			if (m_aRateEntries[i].m_iPlayerId == playerId && m_aRateEntries[i].m_eMethod == method)
				return i;
		}
		return -1;
	}

	protected int CountActiveLeases()
	{
		int count = 0;
		foreach (RICON_LifecycleLease lease : m_aLeases)
		{
			if (lease.m_eState == RICON_ETransmitLeaseState.ACTIVE)
				count++;
		}
		return count;
	}

	protected int CountTerminalLeases()
	{
		int count = 0;
		foreach (RICON_LifecycleLease lease : m_aLeases)
		{
			if (lease.m_eState != RICON_ETransmitLeaseState.ACTIVE)
				count++;
		}
		return count;
	}

	protected array<int> ActiveLeaseIds()
	{
		array<int> ids = {};
		foreach (RICON_LifecycleLease lease : m_aLeases)
		{
			if (lease.m_eState == RICON_ETransmitLeaseState.ACTIVE)
				ids.Insert(lease.m_iLeaseId);
		}
		ids.Sort();
		return ids;
	}

	protected void RemoveRateEntries(int playerId)
	{
		for (int i = m_aRateEntries.Count() - 1; i >= 0; i--)
		{
			if (m_aRateEntries[i].m_iPlayerId == playerId)
				m_aRateEntries.RemoveOrdered(i);
		}
	}

	protected void RemoveOldestTombstone()
	{
		if (m_aTombstones.IsEmpty())
			return;
		int oldestIndex = 0;
		for (int i = 1; i < m_aTombstones.Count(); i++)
		{
			RICON_LifecycleTombstone candidate = m_aTombstones[i];
			RICON_LifecycleTombstone oldest = m_aTombstones[oldestIndex];
			if (candidate.m_iExpiresAtMs < oldest.m_iExpiresAtMs
				|| (candidate.m_iExpiresAtMs == oldest.m_iExpiresAtMs && candidate.m_iPlayerId < oldest.m_iPlayerId)
				|| (candidate.m_iExpiresAtMs == oldest.m_iExpiresAtMs && candidate.m_iPlayerId == oldest.m_iPlayerId
					&& candidate.m_iGeneration < oldest.m_iGeneration))
				oldestIndex = i;
		}
		m_aTombstones.RemoveOrdered(oldestIndex);
	}

	protected int SafeExpiresAt(int nowMs, int ttlMs)
	{
		if (nowMs > int.MAX - ttlMs)
			return int.MAX;
		return nowMs + ttlMs;
	}

	protected RICON_LifecycleResult NewResult(RICON_ELifecycleResult code)
	{
		return new RICON_LifecycleResult(code);
	}

	protected RICON_LifecycleResult SetCode(RICON_LifecycleResult result, RICON_ELifecycleResult code)
	{
		result.m_eResultCode = code;
		return result;
	}

	protected void CopyCachedLease(RICON_LifecycleCachedResponse cached, RICON_LifecycleResult result)
	{
		result.m_bHasLease = cached.m_bHasLease;
		result.m_iLeaseId = cached.m_iLeaseId;
		result.m_eLeaseState = cached.m_eLeaseState;
		result.m_eTerminalReason = cached.m_eTerminalReason;
	}

	protected void AddPlayerStateEffect(
		RICON_LifecycleResult result,
		int playerId,
		int oldGeneration,
		int newGeneration,
		RICON_EPlayerLifecycleState fromState,
		RICON_EPlayerLifecycleState toState)
	{
		RICON_LifecycleEffect effect = new RICON_LifecycleEffect();
		effect.m_eType = RICON_ELifecycleEffectType.PLAYER_STATE_CHANGED;
		effect.m_iPlayerId = playerId;
		effect.m_iOldGeneration = oldGeneration;
		effect.m_iNewGeneration = newGeneration;
		effect.m_eFromState = fromState;
		effect.m_eToState = toState;
		result.m_aEffects.Insert(effect);
	}

	protected void AddEntityEffect(
		RICON_LifecycleResult result,
		RICON_ELifecycleEffectType type,
		int playerId,
		int generation,
		int entityId)
	{
		RICON_LifecycleEffect effect = new RICON_LifecycleEffect();
		effect.m_eType = type;
		effect.m_iPlayerId = playerId;
		effect.m_iGeneration = generation;
		effect.m_iEntityId = entityId;
		result.m_aEffects.Insert(effect);
	}

	protected void AddSimplePlayerEffect(
		RICON_LifecycleResult result,
		RICON_ELifecycleEffectType type,
		int playerId,
		int generation)
	{
		RICON_LifecycleEffect effect = new RICON_LifecycleEffect();
		effect.m_eType = type;
		effect.m_iPlayerId = playerId;
		effect.m_iGeneration = generation;
		result.m_aEffects.Insert(effect);
	}

	protected void AddLeaseEffect(
		RICON_LifecycleResult result,
		RICON_ELifecycleEffectType type,
		RICON_LifecycleLease lease)
	{
		RICON_LifecycleEffect effect = new RICON_LifecycleEffect();
		effect.m_eType = type;
		effect.m_iPlayerId = lease.m_iPlayerId;
		effect.m_iGeneration = lease.m_iGeneration;
		effect.m_iDeviceId = lease.m_iDeviceId;
		effect.m_iLeaseId = lease.m_iLeaseId;
		effect.m_iIssuedAtMs = lease.m_iIssuedAtMs;
		effect.m_iTtlMs = lease.m_iTtlMs;
		effect.m_eLeaseState = lease.m_eState;
		effect.m_eTerminalReason = lease.m_eTerminalReason;
		result.m_aEffects.Insert(effect);
	}

	protected void AddVonExpiryEffect(RICON_LifecycleResult result, RICON_LifecycleLease lease)
	{
		RICON_LifecycleEffect effect = new RICON_LifecycleEffect();
		effect.m_eType = RICON_ELifecycleEffectType.VON_EXPIRY_INTENDED;
		effect.m_iPlayerId = lease.m_iPlayerId;
		effect.m_iDeviceId = lease.m_iDeviceId;
		effect.m_iLeaseId = lease.m_iLeaseId;
		result.m_aEffects.Insert(effect);
	}

	protected void AddFullResyncEffect(RICON_LifecycleResult result, int playerId, int sessionEpoch, int generation)
	{
		RICON_LifecycleEffect effect = new RICON_LifecycleEffect();
		effect.m_eType = RICON_ELifecycleEffectType.FULL_RESYNC_REQUIRED;
		effect.m_iPlayerId = playerId;
		effect.m_iSessionEpoch = sessionEpoch;
		effect.m_iGeneration = generation;
		effect.m_iNewGeneration = generation;
		result.m_aEffects.Insert(effect);
	}

	protected void AddTombstoneEffect(RICON_LifecycleResult result, RICON_LifecycleTombstone tombstone)
	{
		RICON_LifecycleEffect effect = new RICON_LifecycleEffect();
		effect.m_eType = RICON_ELifecycleEffectType.TOMBSTONE_WRITTEN;
		effect.m_iPlayerId = tombstone.m_iPlayerId;
		effect.m_iGeneration = tombstone.m_iGeneration;
		effect.m_iConnectionId = tombstone.m_iConnectionId;
		effect.m_iExpiresAtMs = tombstone.m_iExpiresAtMs;
		result.m_aEffects.Insert(effect);
	}
}
