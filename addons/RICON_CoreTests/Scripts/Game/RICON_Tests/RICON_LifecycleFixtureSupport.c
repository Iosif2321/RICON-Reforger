// TEST_ONLY support for the generated lifecycle parity fixture (RICON_LifecycleFixtures.c). Not for Workshop.
// One RICON_LifecycleTraceRun replays one trace of oracle/vectors/lifecycle_v1.json on a fresh
// RICON_LifecycleStateMachine and compares every step: result code, PTT response fields and the ordered effects.
// Effects are compared by projection: only the fields REQ-lifecycle §1.1 lists for the effect type.
// Like the host oracle runner, a trace stops at its first failed step.

class RICON_LifecycleTraceRun
{
	protected string m_sTraceId;
	protected int m_iExpectedSteps;
	protected int m_iStep;
	protected int m_iPassedSteps;
	protected bool m_bFailed;
	protected bool m_bQuiet;
	protected ref array<ref RICON_LifecycleEffect> m_aExpected = {};
	protected ref RICON_LifecycleStateMachine m_Machine = new RICON_LifecycleStateMachine();

	void RICON_LifecycleTraceRun(string traceId, int expectedSteps, bool quiet = false)
	{
		m_sTraceId = traceId;
		m_iExpectedSteps = expectedSteps;
		m_bQuiet = quiet;
	}

	RICON_LifecycleStateMachine Machine()
	{
		return m_Machine;
	}

	bool IsPassed()
	{
		return !m_bFailed && m_iPassedSteps == m_iExpectedSteps;
	}

	bool IsFailed()
	{
		return m_bFailed;
	}

	int GetPassedSteps()
	{
		return m_iPassedSteps;
	}

	string GetTraceId()
	{
		return m_sTraceId;
	}

	//! Expected effects of the current step, in order.
	void Psc(int playerId, int oldGeneration, int newGeneration, RICON_EPlayerLifecycleState fromState,
		RICON_EPlayerLifecycleState toState)
	{
		RICON_LifecycleEffect effect = NewEffect(RICON_ELifecycleEffectType.PLAYER_STATE_CHANGED, playerId);
		effect.m_iOldGeneration = oldGeneration;
		effect.m_iNewGeneration = newGeneration;
		effect.m_eFromState = fromState;
		effect.m_eToState = toState;
	}

	void Resync(int playerId, int sessionEpoch, int generation)
	{
		RICON_LifecycleEffect effect = NewEffect(RICON_ELifecycleEffectType.FULL_RESYNC_REQUIRED, playerId);
		effect.m_iSessionEpoch = sessionEpoch;
		effect.m_iGeneration = generation;
	}

	void Bound(int playerId, int generation, int entityId)
	{
		RICON_LifecycleEffect effect = NewEffect(RICON_ELifecycleEffectType.ENTITY_BOUND, playerId);
		effect.m_iGeneration = generation;
		effect.m_iEntityId = entityId;
	}

	void Unbound(int playerId, int generation, int entityId)
	{
		RICON_LifecycleEffect effect = NewEffect(RICON_ELifecycleEffectType.ENTITY_UNBOUND, playerId);
		effect.m_iGeneration = generation;
		effect.m_iEntityId = entityId;
	}

	void Granted(int playerId, int generation, int deviceId, int leaseId, int issuedAtMs, int ttlMs)
	{
		RICON_LifecycleEffect effect = NewEffect(RICON_ELifecycleEffectType.LEASE_GRANTED, playerId);
		effect.m_iGeneration = generation;
		effect.m_iDeviceId = deviceId;
		effect.m_iLeaseId = leaseId;
		effect.m_iIssuedAtMs = issuedAtMs;
		effect.m_iTtlMs = ttlMs;
	}

	void Terminated(int playerId, int generation, int deviceId, int leaseId, RICON_ETransmitLeaseState state,
		RICON_ELeaseTerminalReason reason)
	{
		RICON_LifecycleEffect effect = NewEffect(RICON_ELifecycleEffectType.LEASE_TERMINATED, playerId);
		effect.m_iGeneration = generation;
		effect.m_iDeviceId = deviceId;
		effect.m_iLeaseId = leaseId;
		effect.m_eLeaseState = state;
		effect.m_eTerminalReason = reason;
	}

	void Von(int playerId, int deviceId, int leaseId)
	{
		RICON_LifecycleEffect effect = NewEffect(RICON_ELifecycleEffectType.VON_EXPIRY_INTENDED, playerId);
		effect.m_iDeviceId = deviceId;
		effect.m_iLeaseId = leaseId;
	}

	void Subscriptions(int playerId, int generation)
	{
		RICON_LifecycleEffect effect = NewEffect(RICON_ELifecycleEffectType.SUBSCRIPTIONS_CLEARED, playerId);
		effect.m_iGeneration = generation;
	}

	void Detached(int playerId, int generation)
	{
		RICON_LifecycleEffect effect = NewEffect(RICON_ELifecycleEffectType.CARRIED_DEVICES_DETACHED, playerId);
		effect.m_iGeneration = generation;
	}

	void Tombstone(int playerId, int generation, int connectionId, int expiresAtMs)
	{
		RICON_LifecycleEffect effect = NewEffect(RICON_ELifecycleEffectType.TOMBSTONE_WRITTEN, playerId);
		effect.m_iGeneration = generation;
		effect.m_iConnectionId = connectionId;
		effect.m_iExpiresAtMs = expiresAtMs;
	}

	//! Compares one step and moves to the next. hasLease/leaseId/leaseState/reason describe the PTT response
	//! fields; NONE stands for a field the oracle response does not carry.
	void Check(RICON_LifecycleResult actual, RICON_ELifecycleResult code, bool hasCached,
		RICON_ELifecycleResult cachedCode, bool hasLease, int leaseId, RICON_ETransmitLeaseState leaseState,
		RICON_ELeaseTerminalReason reason)
	{
		int step = m_iStep;
		m_iStep++;
		if (m_bFailed)
		{
			m_aExpected.Clear();
			return;
		}

		string problem = Compare(actual, code, hasCached, cachedCode, hasLease, leaseId, leaseState, reason, m_aExpected);
		m_aExpected.Clear();
		if (problem.IsEmpty())
		{
			m_iPassedSteps++;
			return;
		}

		m_bFailed = true;
		if (!m_bQuiet)
			PrintFormat("[RICON_TEST] FAIL %1 step %2: %3", m_sTraceId, step, problem);
	}

	protected string Compare(RICON_LifecycleResult actual, RICON_ELifecycleResult code, bool hasCached,
		RICON_ELifecycleResult cachedCode, bool hasLease, int leaseId, RICON_ETransmitLeaseState leaseState,
		RICON_ELeaseTerminalReason reason, array<ref RICON_LifecycleEffect> expected)
	{
		if (!actual)
			return "null result";
		if (actual.m_eResultCode != code)
			return "resultCode expected " + ResultName(code) + " actual " + ResultName(actual.m_eResultCode);
		if (actual.m_bHasCachedResult != hasCached
			|| (hasCached && actual.m_eCachedResultCode != cachedCode))
			return "cachedResultCode expected " + ResultName(cachedCode) + " actual "
				+ ResultName(actual.m_eCachedResultCode) + " has " + actual.m_bHasCachedResult.ToString();
		if (actual.m_bHasLease != hasLease)
			return "lease fields presence expected " + hasLease.ToString() + " actual " + actual.m_bHasLease.ToString();
		if (hasLease && (actual.m_iLeaseId != leaseId || actual.m_eLeaseState != leaseState
			|| actual.m_eTerminalReason != reason))
			return "lease fields expected " + leaseId.ToString() + "/"
				+ typename.EnumToString(RICON_ETransmitLeaseState, leaseState) + "/"
				+ typename.EnumToString(RICON_ELeaseTerminalReason, reason) + " actual " + actual.m_iLeaseId.ToString()
				+ "/" + typename.EnumToString(RICON_ETransmitLeaseState, actual.m_eLeaseState) + "/"
				+ typename.EnumToString(RICON_ELeaseTerminalReason, actual.m_eTerminalReason);
		if (!actual.m_aEffects)
			return "null effects";
		if (actual.m_aEffects.Count() != expected.Count())
			return "effects count expected " + expected.Count().ToString() + " actual "
				+ actual.m_aEffects.Count().ToString() + EffectTypes(actual.m_aEffects);
		for (int i = 0; i < expected.Count(); i++)
		{
			string field = DiffEffect(expected[i], actual.m_aEffects[i]);
			if (!field.IsEmpty())
				return "effect " + i.ToString() + " (" + EffectName(expected[i].m_eType) + ") differs in " + field
					+ EffectTypes(actual.m_aEffects);
		}
		return "";
	}

	//! Name of the first differing field among those the effect type defines, or "" when equal.
	protected string DiffEffect(RICON_LifecycleEffect expected, RICON_LifecycleEffect actual)
	{
		if (!actual)
			return "null effect";
		if (actual.m_eType != expected.m_eType)
			return "type (actual " + EffectName(actual.m_eType) + ")";
		if (actual.m_iPlayerId != expected.m_iPlayerId)
			return "playerId";

		switch (expected.m_eType)
		{
			case RICON_ELifecycleEffectType.PLAYER_STATE_CHANGED:
			{
				if (actual.m_iOldGeneration != expected.m_iOldGeneration)
					return "oldGeneration";
				if (actual.m_iNewGeneration != expected.m_iNewGeneration)
					return "newGeneration";
				if (actual.m_eFromState != expected.m_eFromState)
					return "fromState";
				if (actual.m_eToState != expected.m_eToState)
					return "toState";
				return "";
			}
			case RICON_ELifecycleEffectType.FULL_RESYNC_REQUIRED:
			{
				if (actual.m_iSessionEpoch != expected.m_iSessionEpoch)
					return "sessionEpoch";
				if (actual.m_iGeneration != expected.m_iGeneration)
					return "generation";
				return "";
			}
			case RICON_ELifecycleEffectType.ENTITY_BOUND:
			case RICON_ELifecycleEffectType.ENTITY_UNBOUND:
			{
				if (actual.m_iGeneration != expected.m_iGeneration)
					return "generation";
				if (actual.m_iEntityId != expected.m_iEntityId)
					return "entityId";
				return "";
			}
			case RICON_ELifecycleEffectType.LEASE_GRANTED:
			{
				if (actual.m_iGeneration != expected.m_iGeneration)
					return "generation";
				if (actual.m_iDeviceId != expected.m_iDeviceId)
					return "deviceId";
				if (actual.m_iLeaseId != expected.m_iLeaseId)
					return "leaseId";
				if (actual.m_iIssuedAtMs != expected.m_iIssuedAtMs)
					return "issuedAtMs";
				if (actual.m_iTtlMs != expected.m_iTtlMs)
					return "ttlMs";
				return "";
			}
			case RICON_ELifecycleEffectType.LEASE_TERMINATED:
			{
				if (actual.m_iGeneration != expected.m_iGeneration)
					return "generation";
				if (actual.m_iDeviceId != expected.m_iDeviceId)
					return "deviceId";
				if (actual.m_iLeaseId != expected.m_iLeaseId)
					return "leaseId";
				if (actual.m_eLeaseState != expected.m_eLeaseState)
					return "leaseState";
				if (actual.m_eTerminalReason != expected.m_eTerminalReason)
					return "terminalReason";
				return "";
			}
			case RICON_ELifecycleEffectType.VON_EXPIRY_INTENDED:
			{
				if (actual.m_iDeviceId != expected.m_iDeviceId)
					return "deviceId";
				if (actual.m_iLeaseId != expected.m_iLeaseId)
					return "leaseId";
				return "";
			}
			case RICON_ELifecycleEffectType.SUBSCRIPTIONS_CLEARED:
			case RICON_ELifecycleEffectType.CARRIED_DEVICES_DETACHED:
			{
				if (actual.m_iGeneration != expected.m_iGeneration)
					return "generation";
				return "";
			}
			case RICON_ELifecycleEffectType.TOMBSTONE_WRITTEN:
			{
				if (actual.m_iGeneration != expected.m_iGeneration)
					return "generation";
				if (actual.m_iConnectionId != expected.m_iConnectionId)
					return "connectionId";
				if (actual.m_iExpiresAtMs != expected.m_iExpiresAtMs)
					return "expiresAtMs";
				return "";
			}
		}
		return "unknown type";
	}

	protected RICON_LifecycleEffect NewEffect(RICON_ELifecycleEffectType type, int playerId)
	{
		RICON_LifecycleEffect effect = new RICON_LifecycleEffect();
		effect.m_eType = type;
		effect.m_iPlayerId = playerId;
		m_aExpected.Insert(effect);
		return effect;
	}

	protected static string ResultName(RICON_ELifecycleResult code)
	{
		return typename.EnumToString(RICON_ELifecycleResult, code);
	}

	protected static string EffectName(RICON_ELifecycleEffectType type)
	{
		return typename.EnumToString(RICON_ELifecycleEffectType, type);
	}

	protected static string EffectTypes(array<ref RICON_LifecycleEffect> effects)
	{
		string text = "; actual types:";
		foreach (RICON_LifecycleEffect effect : effects)
		{
			if (effect)
				text += " " + EffectName(effect.m_eType);
			else
				text += " null";
		}
		return text;
	}

	//! Typed request builders. Arguments follow the payload schema order of REQ-lifecycle §1.2.
	static RICON_LifecyclePttRequest Start(int nowMs, int schemaVersion, int correlationId, int authenticatedPlayerId,
		int payloadPlayerId, int sessionEpoch, int generation, int requestSeq, int deviceId, int networkTypeId,
		int channelId, int ttlMs)
	{
		RICON_LifecyclePttRequest request = NewRequest(RICON_ELifecycleMethod.PTT_START, nowMs, schemaVersion,
			correlationId, authenticatedPlayerId, payloadPlayerId, sessionEpoch, generation, requestSeq);
		request.m_iDeviceId = deviceId;
		request.m_iNetworkTypeId = networkTypeId;
		request.m_iChannelId = channelId;
		request.m_iServerTtlMs = ttlMs;
		return request;
	}

	static RICON_LifecyclePttRequest Stop(int nowMs, int schemaVersion, int correlationId, int authenticatedPlayerId,
		int payloadPlayerId, int sessionEpoch, int generation, int requestSeq, int leaseId)
	{
		RICON_LifecyclePttRequest request = NewRequest(RICON_ELifecycleMethod.PTT_STOP, nowMs, schemaVersion,
			correlationId, authenticatedPlayerId, payloadPlayerId, sessionEpoch, generation, requestSeq);
		request.m_iLeaseId = leaseId;
		return request;
	}

	protected static RICON_LifecyclePttRequest NewRequest(RICON_ELifecycleMethod method, int nowMs, int schemaVersion,
		int correlationId, int authenticatedPlayerId, int payloadPlayerId, int sessionEpoch, int generation,
		int requestSeq)
	{
		RICON_LifecyclePttRequest request = new RICON_LifecyclePttRequest();
		request.m_eMethod = method;
		request.m_iNowMs = nowMs;
		request.m_iSchemaVersion = schemaVersion;
		request.m_iCorrelationId = correlationId;
		request.m_iAuthenticatedPlayerId = authenticatedPlayerId;
		request.m_iPayloadPlayerId = payloadPlayerId;
		request.m_iSessionEpoch = sessionEpoch;
		request.m_iGeneration = generation;
		request.m_iRequestSeq = requestSeq;
		return request;
	}

	static RICON_LifecycleConfig Config(int maxPlayers, int maxActiveLeases, int maxTerminalLeases,
		int terminalLeaseTtlMs, int maxTombstones, int tombstoneTtlMs, int maxResponseCachePerPlayer,
		int maxRateEntries, int rateWindowMs, int rateMaxRequests)
	{
		RICON_LifecycleConfig config = new RICON_LifecycleConfig();
		config.m_iMaxPlayers = maxPlayers;
		config.m_iMaxActiveLeases = maxActiveLeases;
		config.m_iMaxTerminalLeases = maxTerminalLeases;
		config.m_iTerminalLeaseTtlMs = terminalLeaseTtlMs;
		config.m_iMaxTombstones = maxTombstones;
		config.m_iTombstoneTtlMs = tombstoneTtlMs;
		config.m_iMaxResponseCachePerPlayer = maxResponseCachePerPlayer;
		config.m_iMaxRateEntries = maxRateEntries;
		config.m_iRateWindowMs = rateWindowMs;
		config.m_iRateMaxRequests = rateMaxRequests;
		return config;
	}

	//! ownerPlayerId 0 = no owner. Access ids and allowed pairs are added by the caller in canonical order.
	static RICON_LifecycleDevice Device(int deviceId, RICON_EDeviceKind kind, int ownerPlayerId, bool powered)
	{
		RICON_LifecycleDevice device = new RICON_LifecycleDevice();
		device.m_iDeviceId = deviceId;
		device.m_eKind = kind;
		device.m_iOwnerPlayerId = ownerPlayerId;
		device.m_bPowered = powered;
		return device;
	}
}
