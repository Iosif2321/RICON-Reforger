//! S3: чистая серверная служба пар по REQ-pair-service v0.2. Без мира, сети, таймера и снимков.
//! Подлинность входов обеспечивает серверный вызывающий код; DTO не является экспортом lifecycle.
enum RICON_EPairServiceCode
{
	NULL_INPUT = 0,
	INVALID_LIMITS = 1,
	LIMIT_EXCEEDED = 2,
	INVALID_ELEMENT = 3,
	DUPLICATE = 4,
	OK = 5
}

enum RICON_EPairCode
{
	BINDING_INACTIVE = 1,
	TX_PLAYER_INVALID = 2,
	TX_DEVICE_INVALID = 3,
	SELF_PAIR = 4,
	RX_PLAYER_INVALID = 5,
	RX_DEVICE_INVALID = 6,
	STATE_MISMATCH = 7,
	DEVICE_QUERY = 8,
	EVALUATED = 9
}

class RICON_TransmissionBinding
{
	int m_iLeaseId;
	int m_iSessionEpoch;
	int m_iTxPlayerId;
	int m_iTxGeneration;
	int m_iTxDeviceId;
	int m_iTxTransceiverIndex = -1;
	int m_iNetworkTypeId;
	int m_iChannelId;
	bool m_bActive;
	int m_iIssuedAtMs = -1;
	int m_iTtlMs;
}

class RICON_PairListener
{
	int m_iPlayerId;
	int m_iGeneration;
	int m_iSessionEpoch;
	int m_iDeviceId;
	int m_iTransceiverIndex = -1;
}

class RICON_PairPlayer
{
	int m_iPlayerId;
	int m_iGeneration;
	RICON_EPlayerLifecycleState m_eState;
}

class RICON_PairEndpoint
{
	ref RICON_RadioObservation m_Observation;
	ref RICON_RadioRfProfile m_Profile;
}

class RICON_PairServiceLimits
{
	int m_iMaxBindings;
	int m_iMaxListeners;
	int m_iMaxPlayers;
	int m_iMaxDevices;
	int m_iMaxEndpoints;
	int m_iMaxPairs;
}

class RICON_PairServiceInput
{
	int m_iNowMs = -1;
	int m_iSessionEpoch;
	ref RICON_PairServiceLimits m_Limits;
	ref RICON_RadioQueryContext m_Context;
	ref array<ref RICON_TransmissionBinding> m_aBindings = {};
	ref array<ref RICON_PairListener> m_aListeners = {};
	ref array<ref RICON_PairPlayer> m_aPlayers = {};
	ref array<ref RICON_LifecycleDevice> m_aDevices = {};
	ref array<ref RICON_PairEndpoint> m_aEndpoints = {};
}

//! Только значения: результат не держит ссылок на входы или бюджет evaluator.
class RICON_PairResult
{
	int m_iLeaseId;
	int m_iTxDeviceId;
	int m_iRxPlayerId;
	int m_iRxDeviceId;
	int m_iRxTransceiverIndex;
	RICON_EPairCode m_ePairCode = RICON_EPairCode.BINDING_INACTIVE;
	bool m_bHasDeviceCode;
	RICON_ERadioQueryCode m_eDeviceCode = RICON_ERadioQueryCode.NULL_INPUT;
	bool m_bHasBudget;
	bool m_bValid;
	RICON_EReason m_eReason = RICON_EReason.NUMERIC_FAILURE;
	RICON_ELinkState m_eState = RICON_ELinkState.LOST;
	float m_fIntelligibility;
	float m_fSinrDb;
}

class RICON_PairServiceResult
{
	RICON_EPairServiceCode m_eCallCode = RICON_EPairServiceCode.NULL_INPUT;
	ref array<ref RICON_PairResult> m_aPairs = {};
	int m_iS2Calls;
	int m_iEvaluatorCalls;
}

class RICON_PairService
{
	protected static bool EndpointIdentity(int deviceId, int index)
	{
		return deviceId > 0 && deviceId < int.MAX && index >= 0 && index < int.MAX;
	}

	protected static string EndpointKey(int deviceId, int index)
	{
		return deviceId.ToString() + ":" + index.ToString();
	}

	protected static bool ValidBinding(RICON_TransmissionBinding b)
	{
		if (!b || b.m_iLeaseId <= 0 || b.m_iSessionEpoch <= 0 || b.m_iTxPlayerId <= 0 || b.m_iTxGeneration <= 0)
			return false;
		return EndpointIdentity(b.m_iTxDeviceId, b.m_iTxTransceiverIndex) && b.m_iNetworkTypeId > 0
			&& b.m_iChannelId > 0 && b.m_iIssuedAtMs >= 0 && b.m_iTtlMs > 0;
	}

	protected static bool ValidListener(RICON_PairListener l)
	{
		return l && l.m_iPlayerId > 0 && l.m_iGeneration > 0 && l.m_iSessionEpoch > 0
			&& EndpointIdentity(l.m_iDeviceId, l.m_iTransceiverIndex);
	}

	protected static bool ValidLimits(RICON_PairServiceLimits l)
	{
		if (l.m_iMaxBindings < 1 || l.m_iMaxBindings > 4096 || l.m_iMaxListeners < 1 || l.m_iMaxListeners > 4096)
			return false;
		if (l.m_iMaxPlayers < 1 || l.m_iMaxPlayers > 4096 || l.m_iMaxDevices < 1 || l.m_iMaxDevices > 4096)
			return false;
		return l.m_iMaxEndpoints >= 1 && l.m_iMaxEndpoints <= 4096 && l.m_iMaxPairs >= 1 && l.m_iMaxPairs <= 65536;
	}

	protected static bool UniqueInt(map<int, bool> seen, int key)
	{
		if (seen.Contains(key))
			return false;
		seen.Set(key, true);
		return true;
	}

	protected static bool UniqueString(map<string, bool> seen, string key)
	{
		if (seen.Contains(key))
			return false;
		seen.Set(key, true);
		return true;
	}

	protected static RICON_EPairServiceCode Preflight(RICON_PairServiceInput input, RICON_LinkEvaluator evaluator)
	{
		if (!input || !evaluator || !input.m_Limits || !input.m_Context)
			return RICON_EPairServiceCode.NULL_INPUT;
		if (!input.m_aBindings || !input.m_aListeners || !input.m_aPlayers || !input.m_aDevices || !input.m_aEndpoints)
			return RICON_EPairServiceCode.NULL_INPUT;
		RICON_PairServiceLimits limits = input.m_Limits;
		if (!ValidLimits(limits))
			return RICON_EPairServiceCode.INVALID_LIMITS;
		if (input.m_aBindings.Count() > limits.m_iMaxBindings || input.m_aListeners.Count() > limits.m_iMaxListeners)
			return RICON_EPairServiceCode.LIMIT_EXCEEDED;
		if (input.m_aPlayers.Count() > limits.m_iMaxPlayers || input.m_aDevices.Count() > limits.m_iMaxDevices
			|| input.m_aEndpoints.Count() > limits.m_iMaxEndpoints)
			return RICON_EPairServiceCode.LIMIT_EXCEEDED;
		// Длины уже не больше 4096: произведение помещается в int32.
		if (input.m_aBindings.Count() * input.m_aListeners.Count() > limits.m_iMaxPairs)
			return RICON_EPairServiceCode.LIMIT_EXCEEDED;
		if (input.m_iNowMs < 0 || input.m_iSessionEpoch <= 0)
			return RICON_EPairServiceCode.INVALID_ELEMENT;
		foreach (RICON_TransmissionBinding binding : input.m_aBindings)
		{
			if (!ValidBinding(binding))
				return RICON_EPairServiceCode.INVALID_ELEMENT;
		}
		foreach (RICON_PairListener listener : input.m_aListeners)
		{
			if (!ValidListener(listener))
				return RICON_EPairServiceCode.INVALID_ELEMENT;
		}
		foreach (RICON_PairPlayer player : input.m_aPlayers)
		{
			if (!player || player.m_iPlayerId <= 0 || player.m_iGeneration <= 0)
				return RICON_EPairServiceCode.INVALID_ELEMENT;
			if (player.m_eState < RICON_EPlayerLifecycleState.NONE || player.m_eState > RICON_EPlayerLifecycleState.DISCONNECTED)
				return RICON_EPairServiceCode.INVALID_ELEMENT;
		}
		foreach (RICON_LifecycleDevice device : input.m_aDevices)
		{
			if (!device || !device.IsValid())
				return RICON_EPairServiceCode.INVALID_ELEMENT;
		}
		foreach (RICON_PairEndpoint endpoint : input.m_aEndpoints)
		{
			if (!endpoint || !endpoint.m_Observation || !endpoint.m_Profile)
				return RICON_EPairServiceCode.INVALID_ELEMENT;
			if (!EndpointIdentity(endpoint.m_Observation.m_iDeviceId, endpoint.m_Observation.m_iTransceiverIndex))
				return RICON_EPairServiceCode.INVALID_ELEMENT;
		}
		// Полная проверка элементов раньше любых дублей: REQ-PS-04.
		map<int, bool> ids = new map<int, bool>();
		foreach (RICON_TransmissionBinding b1 : input.m_aBindings)
		{
			if (!UniqueInt(ids, b1.m_iLeaseId))
				return RICON_EPairServiceCode.DUPLICATE;
		}
		ids.Clear();
		foreach (RICON_TransmissionBinding b2 : input.m_aBindings)
		{
			if (!UniqueInt(ids, b2.m_iTxDeviceId))
				return RICON_EPairServiceCode.DUPLICATE;
		}
		ids.Clear();
		foreach (RICON_PairPlayer p : input.m_aPlayers)
		{
			if (!UniqueInt(ids, p.m_iPlayerId))
				return RICON_EPairServiceCode.DUPLICATE;
		}
		ids.Clear();
		foreach (RICON_LifecycleDevice d : input.m_aDevices)
		{
			if (!UniqueInt(ids, d.m_iDeviceId))
				return RICON_EPairServiceCode.DUPLICATE;
		}
		map<string, bool> keys = new map<string, bool>();
		foreach (RICON_PairEndpoint e : input.m_aEndpoints)
		{
			if (!UniqueString(keys, EndpointKey(e.m_Observation.m_iDeviceId, e.m_Observation.m_iTransceiverIndex)))
				return RICON_EPairServiceCode.DUPLICATE;
		}
		keys.Clear();
		foreach (RICON_PairListener l : input.m_aListeners)
		{
			string key = EndpointKey(l.m_iDeviceId, l.m_iTransceiverIndex) + ":" + l.m_iPlayerId.ToString();
			if (!UniqueString(keys, key))
				return RICON_EPairServiceCode.DUPLICATE;
		}
		return RICON_EPairServiceCode.OK;
	}

	protected static bool ListenerBefore(RICON_PairListener a, RICON_PairListener b)
	{
		if (a.m_iDeviceId != b.m_iDeviceId)
			return a.m_iDeviceId < b.m_iDeviceId;
		if (a.m_iTransceiverIndex != b.m_iTransceiverIndex)
			return a.m_iTransceiverIndex < b.m_iTransceiverIndex;
		return a.m_iPlayerId < b.m_iPlayerId;
	}

	protected static bool HasAccess(RICON_LifecycleDevice device, int playerId, RICON_TransmissionBinding binding)
	{
		return device && device.CanAccess(playerId) && device.Allows(binding.m_iNetworkTypeId, binding.m_iChannelId);
	}

	//! Стенд переопределяет только счётчик; сама сборка остаётся существующим S2.
	protected RICON_ERadioQueryCode BuildQuery(RICON_PairEndpoint tx, RICON_PairEndpoint rx,
		RICON_RadioQueryContext context, out RICON_LinkQuery query)
	{
		return RICON_RadioQueryBuilder.Build(tx.m_Observation, tx.m_Profile, rx.m_Observation, rx.m_Profile, context, query);
	}

	protected RICON_PairResult EvaluatePair(RICON_PairServiceInput input, RICON_TransmissionBinding b,
		RICON_PairListener l, map<int, ref RICON_PairPlayer> players, map<int, ref RICON_LifecycleDevice> devices,
		map<string, ref RICON_PairEndpoint> endpoints, RICON_LinkEvaluator evaluator, RICON_PairServiceResult result)
	{
		RICON_PairResult row = new RICON_PairResult();
		row.m_iLeaseId = b.m_iLeaseId;
		row.m_iTxDeviceId = b.m_iTxDeviceId;
		row.m_iRxPlayerId = l.m_iPlayerId;
		row.m_iRxDeviceId = l.m_iDeviceId;
		row.m_iRxTransceiverIndex = l.m_iTransceiverIndex;
		if (!b.m_bActive || b.m_iSessionEpoch != input.m_iSessionEpoch || input.m_iNowMs < b.m_iIssuedAtMs)
			return row;
		if (input.m_iNowMs - b.m_iIssuedAtMs >= b.m_iTtlMs)
			return row;
		row.m_ePairCode = RICON_EPairCode.TX_PLAYER_INVALID;
		RICON_PairPlayer txPlayer = players.Get(b.m_iTxPlayerId);
		if (!txPlayer || txPlayer.m_eState != RICON_EPlayerLifecycleState.BOUND_ACTIVE || txPlayer.m_iGeneration != b.m_iTxGeneration)
			return row;
		row.m_ePairCode = RICON_EPairCode.TX_DEVICE_INVALID;
		RICON_LifecycleDevice txDevice = devices.Get(b.m_iTxDeviceId);
		RICON_PairEndpoint txEndpoint = endpoints.Get(EndpointKey(b.m_iTxDeviceId, b.m_iTxTransceiverIndex));
		if (!txEndpoint || !HasAccess(txDevice, b.m_iTxPlayerId, b))
			return row;
		row.m_ePairCode = RICON_EPairCode.SELF_PAIR;
		if (l.m_iDeviceId == b.m_iTxDeviceId)
			return row;
		row.m_ePairCode = RICON_EPairCode.RX_PLAYER_INVALID;
		RICON_PairPlayer rxPlayer = players.Get(l.m_iPlayerId);
		if (l.m_iSessionEpoch != input.m_iSessionEpoch || !rxPlayer)
			return row;
		if (rxPlayer.m_eState != RICON_EPlayerLifecycleState.BOUND_ACTIVE || rxPlayer.m_iGeneration != l.m_iGeneration)
			return row;
		row.m_ePairCode = RICON_EPairCode.RX_DEVICE_INVALID;
		RICON_LifecycleDevice rxDevice = devices.Get(l.m_iDeviceId);
		RICON_PairEndpoint rxEndpoint = endpoints.Get(EndpointKey(l.m_iDeviceId, l.m_iTransceiverIndex));
		if (!rxEndpoint || !HasAccess(rxDevice, l.m_iPlayerId, b))
			return row;
		row.m_ePairCode = RICON_EPairCode.STATE_MISMATCH;
		if (txDevice.m_bPowered != txEndpoint.m_Observation.m_bPowered || rxDevice.m_bPowered != rxEndpoint.m_Observation.m_bPowered)
			return row;
		row.m_ePairCode = RICON_EPairCode.DEVICE_QUERY;
		row.m_bHasDeviceCode = true;
		result.m_iS2Calls++;
		RICON_LinkQuery query;
		row.m_eDeviceCode = BuildQuery(txEndpoint, rxEndpoint, input.m_Context, query);
		if (row.m_eDeviceCode != RICON_ERadioQueryCode.READY)
			return row;
		result.m_iEvaluatorCalls++;
		RICON_LinkBudget budget = evaluator.Evaluate(query);
		// Нарушение обещания evaluator «никогда null» не должно уронить вызывающий код.
		if (!budget)
			budget = new RICON_LinkBudget();
		row.m_ePairCode = RICON_EPairCode.EVALUATED;
		row.m_bHasBudget = true;
		row.m_bValid = budget.m_bValid;
		row.m_eReason = budget.m_eReason;
		row.m_eState = budget.m_eState;
		row.m_fIntelligibility = budget.m_fIntelligibility;
		row.m_fSinrDb = budget.m_fSinrDb;
		return row;
	}

	RICON_PairServiceResult Evaluate(RICON_PairServiceInput input, RICON_LinkEvaluator evaluator)
	{
		RICON_PairServiceResult result = new RICON_PairServiceResult();
		result.m_eCallCode = Preflight(input, evaluator);
		if (result.m_eCallCode != RICON_EPairServiceCode.OK)
			return result;
		map<int, ref RICON_PairPlayer> players = new map<int, ref RICON_PairPlayer>();
		map<int, ref RICON_LifecycleDevice> devices = new map<int, ref RICON_LifecycleDevice>();
		map<string, ref RICON_PairEndpoint> endpoints = new map<string, ref RICON_PairEndpoint>();
		map<int, ref RICON_TransmissionBinding> bindings = new map<int, ref RICON_TransmissionBinding>();
		array<int> leaseIds = {};
		foreach (RICON_PairPlayer player : input.m_aPlayers)
			players.Set(player.m_iPlayerId, player);
		foreach (RICON_LifecycleDevice device : input.m_aDevices)
			devices.Set(device.m_iDeviceId, device);
		foreach (RICON_PairEndpoint endpoint : input.m_aEndpoints)
			endpoints.Set(EndpointKey(endpoint.m_Observation.m_iDeviceId, endpoint.m_Observation.m_iTransceiverIndex), endpoint);
		foreach (RICON_TransmissionBinding binding : input.m_aBindings)
		{
			bindings.Set(binding.m_iLeaseId, binding);
			leaseIds.Insert(binding.m_iLeaseId);
		}
		leaseIds.Sort();
		array<ref RICON_PairListener> ordered = {};
		foreach (RICON_PairListener listener : input.m_aListeners)
			ordered.Insert(listener);
		// shortcut: вставки для не более 4096 слушателей; менять после измерения нагрузки Э10.
		for (int i = 1; i < ordered.Count(); i++)
		{
			RICON_PairListener item = ordered[i];
			int j = i - 1;
			while (j >= 0 && ListenerBefore(item, ordered[j]))
			{
				ordered[j + 1] = ordered[j];
				j--;
			}
			ordered[j + 1] = item;
		}
		foreach (int leaseId : leaseIds)
		{
			foreach (RICON_PairListener target : ordered)
				result.m_aPairs.Insert(EvaluatePair(input, bindings.Get(leaseId), target, players, devices, endpoints, evaluator, result));
		}
		return result;
	}
}
