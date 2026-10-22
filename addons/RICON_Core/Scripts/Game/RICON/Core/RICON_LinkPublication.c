//! S3b: чистое формирование партии RICON_LinkSnapshot по REQ-link-publication v0.2 (корпус заморожен, SHA a60ed145…0be3).
//! Без RPC, отправки, таймеров, мира и звука. Состоянием владеет вызывающий код: вход и старый реестр не меняются.
enum RICON_ELinkPublicationCode
{
	NULL_INPUT = 0,
	INVALID_LIMITS = 1,
	LIMIT_EXCEEDED = 2,
	INVALID_ELEMENT = 3,
	INVALID_REGISTRY = 4,
	DUPLICATE = 5,
	UPSTREAM_REFUSED = 6,
	IDENTITY_MISMATCH = 7,
	CLOCK_ROLLBACK = 8,
	ACTIVE_LIMIT = 9,
	ID_EXHAUSTED = 10,
	NEED_FULL_RESYNC = 11,
	OK = 12
}

class RICON_LinkPublicationLimits
{
	int m_iMaxBindings;
	int m_iMaxPairs;
	int m_iMaxActiveLinks;
}

//! Данные одного серверного такта (REQ-LP-01).
class RICON_LinkPublicationInput
{
	int m_iNowMs = -1;
	int m_iSessionEpoch;
	int m_iTtlMs;
	float m_fQualityDelta;
	ref RICON_LinkPublicationLimits m_Limits;
	ref array<ref RICON_TransmissionBinding> m_aBindings = {};
	ref RICON_PairServiceResult m_PairResult;
}

//! Последние выданные значения одной незавершённой связи (REQ-LP-02).
class RICON_LinkRegistryEntry
{
	int m_iLeaseId;
	int m_iRxDeviceId;
	int m_iRxTransceiverIndex = -1;
	int m_iRxPlayerId;
	int m_iTxPlayerId;
	int m_iTxDeviceId;
	int m_iTxTransceiverIndex = -1;
	int m_iLinkId;
	int m_iLinkSeq;
	RICON_ELinkState m_eState = RICON_ELinkState.LOST;
	float m_fQuality;
	int m_iTtlMs;
	int m_iPublishedAtMs = -1;

	RICON_LinkRegistryEntry CloneEntry()
	{
		RICON_LinkRegistryEntry c = new RICON_LinkRegistryEntry();
		c.m_iLeaseId = m_iLeaseId;
		c.m_iRxDeviceId = m_iRxDeviceId;
		c.m_iRxTransceiverIndex = m_iRxTransceiverIndex;
		c.m_iRxPlayerId = m_iRxPlayerId;
		c.m_iTxPlayerId = m_iTxPlayerId;
		c.m_iTxDeviceId = m_iTxDeviceId;
		c.m_iTxTransceiverIndex = m_iTxTransceiverIndex;
		c.m_iLinkId = m_iLinkId;
		c.m_iLinkSeq = m_iLinkSeq;
		c.m_eState = m_eState;
		c.m_fQuality = m_fQuality;
		c.m_iTtlMs = m_iTtlMs;
		c.m_iPublishedAtMs = m_iPublishedAtMs;
		return c;
	}
}

//! Объект значений реестра. Начальное состояние: текущая epoch, highWater 0, lastTick −1, full resync, пусто.
class RICON_LinkRegistry
{
	int m_iSessionEpoch;
	int m_iHighWaterLinkId;
	int m_iLastTickMs = -1;
	bool m_bNeedsFullResync = true;
	ref array<ref RICON_LinkRegistryEntry> m_aEntries = {};

	//! Глубокая копия; null-массив и null-элементы сохраняются как есть (копия при preflight-отказе).
	RICON_LinkRegistry CloneRegistry()
	{
		RICON_LinkRegistry c = new RICON_LinkRegistry();
		c.m_iSessionEpoch = m_iSessionEpoch;
		c.m_iHighWaterLinkId = m_iHighWaterLinkId;
		c.m_iLastTickMs = m_iLastTickMs;
		c.m_bNeedsFullResync = m_bNeedsFullResync;
		if (!m_aEntries)
		{
			c.m_aEntries = null;
			return c;
		}
		foreach (RICON_LinkRegistryEntry e : m_aEntries)
		{
			if (e)
				c.m_aEntries.Insert(e.CloneEntry());
			else
				c.m_aEntries.Insert(null);
		}
		return c;
	}
}

//! Узкий источник номеров (REQ-LP-08). Базовый класс fail-closed.
class RICON_LinkSequenceSource
{
	int GetSessionEpoch()
	{
		return 0;
	}

	bool NextLinkSequence(int linkId, out int linkSeq, out bool rotated)
	{
		linkSeq = 0;
		rotated = false;
		return false;
	}
}

//! Production-адаптер: только делегирует существующему менеджеру; authority проверяет сам менеджер.
class RICON_ManagerLinkSequenceSource : RICON_LinkSequenceSource
{
	protected RICON_CommsManagerComponent m_Manager;

	void RICON_ManagerLinkSequenceSource(RICON_CommsManagerComponent manager)
	{
		m_Manager = manager;
	}

	override int GetSessionEpoch()
	{
		if (!m_Manager)
			return 0;
		return m_Manager.GetSessionEpoch();
	}

	override bool NextLinkSequence(int linkId, out int linkSeq, out bool rotated)
	{
		linkSeq = 0;
		rotated = false;
		if (!m_Manager)
			return false;
		return m_Manager.NextLinkSequence(linkId, linkSeq, rotated);
	}
}

class RICON_LinkPublicationResult
{
	RICON_ELinkPublicationCode m_eCallCode = RICON_ELinkPublicationCode.NULL_INPUT;
	ref array<ref RICON_LinkSnapshot> m_aSnapshots = {};
	ref RICON_LinkRegistry m_Registry;
	int m_iSequenceCalls;
	int m_iEpochReads;
	bool m_bRequiresFullResync;
}

//! Внутренний элемент плана: один ключ объединения текущих пар и старых записей (REQ-LP-05).
class RICON_LinkPublicationItem
{
	static const int KEEP = 0;
	static const int CREATE = 1;
	static const int TERMINATE = 2;

	int m_iLeaseId;
	int m_iRxDeviceId;
	int m_iRxTransceiverIndex;
	int m_iRxPlayerId;
	RICON_PairResult m_Pair;
	RICON_TransmissionBinding m_Binding;
	RICON_LinkRegistryEntry m_Old;
	int m_iKind;
	RICON_ELinkState m_eState = RICON_ELinkState.LOST;
	float m_fQuality;
	int m_iLinkId;
	int m_iLinkSeq;
	bool m_bNeed;
}

class RICON_LinkPublicationService
{
	protected static bool ValidEndpoint(int deviceId, int index)
	{
		return deviceId > 0 && deviceId < int.MAX && index >= 0 && index < int.MAX;
	}

	//! REQ-PS-01 целиком.
	protected static bool ValidBinding(RICON_TransmissionBinding b)
	{
		if (!b || b.m_iLeaseId <= 0 || b.m_iSessionEpoch <= 0 || b.m_iTxPlayerId <= 0 || b.m_iTxGeneration <= 0)
			return false;
		return ValidEndpoint(b.m_iTxDeviceId, b.m_iTxTransceiverIndex) && b.m_iNetworkTypeId > 0
			&& b.m_iChannelId > 0 && b.m_iIssuedAtMs >= 0 && b.m_iTtlMs > 0;
	}

	protected static bool KnownState(RICON_ELinkState s)
	{
		return s >= RICON_ELinkState.LOST && s <= RICON_ELinkState.CLEAN;
	}

	protected static bool UnitQuality(float q)
	{
		return RICON_Num.IsFinite(q) && q >= 0 && q <= 1;
	}

	//! Домен и согласованность одной пары S3 (REQ-LP-03, абзац после таблицы).
	protected static bool ValidPair(RICON_PairResult p)
	{
		if (!p || p.m_iLeaseId <= 0 || p.m_iRxPlayerId <= 0)
			return false;
		if (p.m_iTxDeviceId <= 0 || p.m_iTxDeviceId >= int.MAX || !ValidEndpoint(p.m_iRxDeviceId, p.m_iRxTransceiverIndex))
			return false;
		if (p.m_ePairCode < RICON_EPairCode.BINDING_INACTIVE || p.m_ePairCode > RICON_EPairCode.EVALUATED)
			return false;
		if (p.m_eDeviceCode < RICON_ERadioQueryCode.NULL_INPUT || p.m_eDeviceCode > RICON_ERadioQueryCode.READY)
			return false;
		if (p.m_eReason < RICON_EReason.OK || p.m_eReason > RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE || !KnownState(p.m_eState))
			return false;
		if (!RICON_Num.IsFinite(p.m_fSinrDb) || !UnitQuality(p.m_fIntelligibility))
			return false;
		bool evaluated = p.m_ePairCode == RICON_EPairCode.EVALUATED;
		bool query = p.m_ePairCode == RICON_EPairCode.DEVICE_QUERY;
		if (p.m_bHasBudget != evaluated || p.m_bHasDeviceCode != (evaluated || query))
			return false;
		if (evaluated && p.m_eDeviceCode != RICON_ERadioQueryCode.READY)
			return false;
		if (query && p.m_eDeviceCode == RICON_ERadioQueryCode.READY)
			return false;
		if (!evaluated && !query && p.m_eDeviceCode != RICON_ERadioQueryCode.NULL_INPUT)
			return false;
		bool lostZero = p.m_eState == RICON_ELinkState.LOST && p.m_fIntelligibility == 0 && p.m_fSinrDb == 0;
		if (!p.m_bHasBudget)
		{
			if (p.m_bValid || p.m_eReason != RICON_EReason.NUMERIC_FAILURE || !lostZero)
				return false;
		}
		else
		{
			if (p.m_bValid != (p.m_eReason == RICON_EReason.OK))
				return false;
			if (!p.m_bValid && !lostZero)
				return false;
		}
		return p.m_eState != RICON_ELinkState.LOST || p.m_fIntelligibility == 0;
	}

	protected static bool ValidRegistry(RICON_LinkRegistry r)
	{
		if (r.m_iSessionEpoch <= 0 || r.m_iSessionEpoch >= int.MAX || r.m_iHighWaterLinkId < 0 || r.m_iLastTickMs < -1)
			return false;
		if (r.m_iLastTickMs == -1 && (r.m_aEntries.Count() > 0 || !r.m_bNeedsFullResync))
			return false;
		foreach (RICON_LinkRegistryEntry e : r.m_aEntries)
		{
			if (!e || e.m_iLeaseId <= 0 || e.m_iRxPlayerId <= 0 || e.m_iTxPlayerId <= 0)
				return false;
			if (!ValidEndpoint(e.m_iRxDeviceId, e.m_iRxTransceiverIndex) || !ValidEndpoint(e.m_iTxDeviceId, e.m_iTxTransceiverIndex))
				return false;
			if (e.m_iLinkId <= 0 || e.m_iLinkId > r.m_iHighWaterLinkId || e.m_iLinkSeq <= 0 || e.m_iLinkSeq >= int.MAX)
				return false;
			if (!KnownState(e.m_eState) || !UnitQuality(e.m_fQuality) || e.m_iTtlMs <= 0)
				return false;
			if (e.m_eState == RICON_ELinkState.LOST && e.m_fQuality != 0)
				return false;
			if (e.m_iPublishedAtMs < 0 || e.m_iPublishedAtMs > r.m_iLastTickMs)
				return false;
		}
		return true;
	}

	protected static string Key(int leaseId, int rxDeviceId, int rxIndex, int rxPlayerId)
	{
		return leaseId.ToString() + ":" + rxDeviceId.ToString() + ":" + rxIndex.ToString() + ":" + rxPlayerId.ToString();
	}

	protected static bool Unique(map<string, bool> seen, string key)
	{
		if (seen.Contains(key))
			return false;
		seen.Set(key, true);
		return true;
	}

	//! Порядок ключа (leaseId, rxDeviceId, rxTransceiverIndex, rxPlayerId), все по возрастанию.
	protected static bool Before(RICON_LinkPublicationItem a, RICON_LinkPublicationItem b)
	{
		if (a.m_iLeaseId != b.m_iLeaseId)
			return a.m_iLeaseId < b.m_iLeaseId;
		if (a.m_iRxDeviceId != b.m_iRxDeviceId)
			return a.m_iRxDeviceId < b.m_iRxDeviceId;
		if (a.m_iRxTransceiverIndex != b.m_iRxTransceiverIndex)
			return a.m_iRxTransceiverIndex < b.m_iRxTransceiverIndex;
		return a.m_iRxPlayerId < b.m_iRxPlayerId;
	}

	//! Устойчивая сортировка слиянием: до 2·65536 ключей, O(n log n). Ключи уникальны, поэтому порядок полный.
	protected static void SortItems(array<ref RICON_LinkPublicationItem> items)
	{
		int n = items.Count();
		array<ref RICON_LinkPublicationItem> buffer = {};
		buffer.Resize(n);
		for (int width = 1; width < n; width *= 2)
		{
			for (int lo = 0; lo < n; lo += 2 * width)
			{
				int mid = lo + width;
				if (mid > n)
					mid = n;
				int hi = lo + 2 * width;
				if (hi > n)
					hi = n;
				int i = lo;
				int j = mid;
				int k = lo;
				while (i < mid && j < hi)
				{
					if (Before(items[j], items[i]))
					{
						buffer[k] = items[j];
						j++;
					}
					else
					{
						buffer[k] = items[i];
						i++;
					}
					k++;
				}
				while (i < mid)
				{
					buffer[k] = items[i];
					i++;
					k++;
				}
				while (j < hi)
				{
					buffer[k] = items[j];
					j++;
					k++;
				}
			}
			for (int c = 0; c < n; c++)
				items[c] = buffer[c];
		}
	}

	//! REQ-LP-05: объединение ключей текущих пар и старых записей той же epoch, в порядке ключа.
	protected static void BuildPlan(RICON_LinkPublicationInput input, map<int, RICON_TransmissionBinding> bindings,
		array<ref RICON_LinkRegistryEntry> oldEntries, array<ref RICON_LinkPublicationItem> plan)
	{
		map<string, RICON_LinkRegistryEntry> old = new map<string, RICON_LinkRegistryEntry>();
		foreach (RICON_LinkRegistryEntry e : oldEntries)
			old.Set(Key(e.m_iLeaseId, e.m_iRxDeviceId, e.m_iRxTransceiverIndex, e.m_iRxPlayerId), e);
		map<string, bool> seen = new map<string, bool>();
		array<ref RICON_LinkPublicationItem> items = {};
		foreach (RICON_PairResult p : input.m_PairResult.m_aPairs)
		{
			string key = Key(p.m_iLeaseId, p.m_iRxDeviceId, p.m_iRxTransceiverIndex, p.m_iRxPlayerId);
			seen.Set(key, true);
			RICON_LinkPublicationItem item = new RICON_LinkPublicationItem();
			item.m_iLeaseId = p.m_iLeaseId;
			item.m_iRxDeviceId = p.m_iRxDeviceId;
			item.m_iRxTransceiverIndex = p.m_iRxTransceiverIndex;
			item.m_iRxPlayerId = p.m_iRxPlayerId;
			item.m_Pair = p;
			item.m_Binding = bindings.Get(p.m_iLeaseId);
			item.m_Old = old.Get(key);
			items.Insert(item);
		}
		foreach (RICON_LinkRegistryEntry e2 : oldEntries)
		{
			if (seen.Contains(Key(e2.m_iLeaseId, e2.m_iRxDeviceId, e2.m_iRxTransceiverIndex, e2.m_iRxPlayerId)))
				continue;
			RICON_LinkPublicationItem gone = new RICON_LinkPublicationItem();
			gone.m_iLeaseId = e2.m_iLeaseId;
			gone.m_iRxDeviceId = e2.m_iRxDeviceId;
			gone.m_iRxTransceiverIndex = e2.m_iRxTransceiverIndex;
			gone.m_iRxPlayerId = e2.m_iRxPlayerId;
			gone.m_Old = e2;
			items.Insert(gone);
		}
		SortItems(items);
		foreach (RICON_LinkPublicationItem it : items)
		{
			RICON_PairResult pair = it.m_Pair;
			bool hasOld = it.m_Old != null;
			if (pair && pair.m_ePairCode == RICON_EPairCode.EVALUATED && pair.m_bValid)
			{
				it.m_iKind = RICON_LinkPublicationItem.KEEP;
				if (!hasOld)
					it.m_iKind = RICON_LinkPublicationItem.CREATE;
				it.m_eState = pair.m_eState;
				it.m_fQuality = pair.m_fIntelligibility;
				if (pair.m_eState == RICON_ELinkState.LOST)
					it.m_fQuality = 0;
				plan.Insert(it);
				continue;
			}
			if (!hasOld)
				continue;
			bool keep = pair && (pair.m_ePairCode == RICON_EPairCode.EVALUATED || pair.m_ePairCode == RICON_EPairCode.STATE_MISMATCH
				|| pair.m_ePairCode == RICON_EPairCode.DEVICE_QUERY);
			it.m_iKind = RICON_LinkPublicationItem.TERMINATE;
			if (keep)
				it.m_iKind = RICON_LinkPublicationItem.KEEP;
			it.m_eState = RICON_ELinkState.LOST;
			it.m_fQuality = 0;
			plan.Insert(it);
		}
	}

	//! Все проходы REQ-LP-03 до любого обращения к source. При OK заполняет план.
	protected static RICON_ELinkPublicationCode Preflight(RICON_LinkPublicationInput input, RICON_LinkRegistry registry,
		RICON_LinkSequenceSource source, array<ref RICON_LinkPublicationItem> plan)
	{
		if (!input || !registry || !source || !input.m_Limits || !input.m_aBindings || !input.m_PairResult)
			return RICON_ELinkPublicationCode.NULL_INPUT;
		if (!input.m_PairResult.m_aPairs || !registry.m_aEntries)
			return RICON_ELinkPublicationCode.NULL_INPUT;
		RICON_LinkPublicationLimits lim = input.m_Limits;
		if (lim.m_iMaxBindings < 1 || lim.m_iMaxBindings > 4096 || lim.m_iMaxPairs < 1 || lim.m_iMaxPairs > 65536
			|| lim.m_iMaxActiveLinks < 1 || lim.m_iMaxActiveLinks > 65536)
			return RICON_ELinkPublicationCode.INVALID_LIMITS;
		array<ref RICON_PairResult> pairs = input.m_PairResult.m_aPairs;
		if (input.m_aBindings.Count() > lim.m_iMaxBindings || pairs.Count() > lim.m_iMaxPairs
			|| registry.m_aEntries.Count() > lim.m_iMaxActiveLinks)
			return RICON_ELinkPublicationCode.LIMIT_EXCEEDED;
		float qd = input.m_fQualityDelta;
		if (input.m_iNowMs < 0 || input.m_iSessionEpoch <= 0 || input.m_iSessionEpoch >= int.MAX || input.m_iTtlMs <= 0
			|| !RICON_Num.IsFinite(qd) || qd <= 0 || qd > 1)
			return RICON_ELinkPublicationCode.INVALID_ELEMENT;
		RICON_PairServiceResult pr = input.m_PairResult;
		if (pr.m_eCallCode < RICON_EPairServiceCode.NULL_INPUT || pr.m_eCallCode > RICON_EPairServiceCode.OK
			|| pr.m_iS2Calls < 0 || pr.m_iEvaluatorCalls < 0)
			return RICON_ELinkPublicationCode.INVALID_ELEMENT;
		foreach (RICON_TransmissionBinding b : input.m_aBindings)
		{
			if (!ValidBinding(b))
				return RICON_ELinkPublicationCode.INVALID_ELEMENT;
		}
		foreach (RICON_PairResult p : pairs)
		{
			if (!ValidPair(p))
				return RICON_ELinkPublicationCode.INVALID_ELEMENT;
		}
		if (!ValidRegistry(registry))
			return RICON_ELinkPublicationCode.INVALID_REGISTRY;
		map<string, bool> seen = new map<string, bool>();
		foreach (RICON_TransmissionBinding b1 : input.m_aBindings)
		{
			if (!Unique(seen, b1.m_iLeaseId.ToString()))
				return RICON_ELinkPublicationCode.DUPLICATE;
		}
		seen.Clear();
		foreach (RICON_TransmissionBinding b2 : input.m_aBindings)
		{
			if (!Unique(seen, b2.m_iTxDeviceId.ToString()))
				return RICON_ELinkPublicationCode.DUPLICATE;
		}
		seen.Clear();
		foreach (RICON_PairResult p1 : pairs)
		{
			if (!Unique(seen, Key(p1.m_iLeaseId, p1.m_iRxDeviceId, p1.m_iRxTransceiverIndex, p1.m_iRxPlayerId)))
				return RICON_ELinkPublicationCode.DUPLICATE;
		}
		seen.Clear();
		foreach (RICON_LinkRegistryEntry e1 : registry.m_aEntries)
		{
			if (!Unique(seen, Key(e1.m_iLeaseId, e1.m_iRxDeviceId, e1.m_iRxTransceiverIndex, e1.m_iRxPlayerId)))
				return RICON_ELinkPublicationCode.DUPLICATE;
		}
		seen.Clear();
		foreach (RICON_LinkRegistryEntry e2 : registry.m_aEntries)
		{
			if (!Unique(seen, e2.m_iLinkId.ToString()))
				return RICON_ELinkPublicationCode.DUPLICATE;
		}
		if (pr.m_eCallCode != RICON_EPairServiceCode.OK)
			return RICON_ELinkPublicationCode.UPSTREAM_REFUSED;
		int epoch = input.m_iSessionEpoch;
		bool sameEpoch = registry.m_iSessionEpoch == epoch;
		map<int, RICON_TransmissionBinding> bindings = new map<int, RICON_TransmissionBinding>();
		foreach (RICON_TransmissionBinding b3 : input.m_aBindings)
			bindings.Set(b3.m_iLeaseId, b3);
		// При другой epoch старый реестр для правил §3 считается пустым (REQ-LP-03, последний абзац).
		array<ref RICON_LinkRegistryEntry> oldEntries = {};
		map<string, RICON_LinkRegistryEntry> oldByKey = new map<string, RICON_LinkRegistryEntry>();
		if (sameEpoch)
		{
			foreach (RICON_LinkRegistryEntry e3 : registry.m_aEntries)
			{
				oldEntries.Insert(e3);
				oldByKey.Set(Key(e3.m_iLeaseId, e3.m_iRxDeviceId, e3.m_iRxTransceiverIndex, e3.m_iRxPlayerId), e3);
			}
		}
		foreach (RICON_PairResult p2 : pairs)
		{
			RICON_TransmissionBinding b4 = bindings.Get(p2.m_iLeaseId);
			if (!b4 || b4.m_iTxDeviceId != p2.m_iTxDeviceId)
				return RICON_ELinkPublicationCode.IDENTITY_MISMATCH;
			if (p2.m_ePairCode != RICON_EPairCode.BINDING_INACTIVE && b4.m_iSessionEpoch != epoch)
				return RICON_ELinkPublicationCode.IDENTITY_MISMATCH;
			RICON_LinkRegistryEntry prev = oldByKey.Get(Key(p2.m_iLeaseId, p2.m_iRxDeviceId, p2.m_iRxTransceiverIndex, p2.m_iRxPlayerId));
			if (prev && (prev.m_iTxPlayerId != b4.m_iTxPlayerId || prev.m_iTxDeviceId != b4.m_iTxDeviceId
				|| prev.m_iTxTransceiverIndex != b4.m_iTxTransceiverIndex))
				return RICON_ELinkPublicationCode.IDENTITY_MISMATCH;
		}
		if (sameEpoch && registry.m_iLastTickMs >= 0 && input.m_iNowMs < registry.m_iLastTickMs)
			return RICON_ELinkPublicationCode.CLOCK_ROLLBACK;
		BuildPlan(input, bindings, oldEntries, plan);
		int active = 0;
		int created = 0;
		foreach (RICON_LinkPublicationItem item : plan)
		{
			if (item.m_iKind != RICON_LinkPublicationItem.TERMINATE)
				active++;
			if (item.m_iKind == RICON_LinkPublicationItem.CREATE)
				created++;
		}
		if (active > lim.m_iMaxActiveLinks)
			return RICON_ELinkPublicationCode.ACTIVE_LIMIT;
		int highWater = 0;
		if (sameEpoch)
			highWater = registry.m_iHighWaterLinkId;
		if (created > int.MAX - highWater)
			return RICON_ELinkPublicationCode.ID_EXHAUSTED;
		return RICON_ELinkPublicationCode.OK;
	}

	//! REQ-LP-09/10: пустой реестр сброса. Невалидная epoch source заменяется входной.
	protected static RICON_LinkRegistry ResetRegistry(int sourceEpoch, int inputEpoch, int previousEpoch, int highWaterIfSame)
	{
		RICON_LinkRegistry r = new RICON_LinkRegistry();
		r.m_iSessionEpoch = sourceEpoch;
		if (sourceEpoch <= 0 || sourceEpoch >= int.MAX)
			r.m_iSessionEpoch = inputEpoch;
		if (r.m_iSessionEpoch == previousEpoch)
			r.m_iHighWaterLinkId = highWaterIfSame;
		r.m_iLastTickMs = -1;
		r.m_bNeedsFullResync = true;
		return r;
	}

	//! REQ-LP-06: нужен ли снимок сохраняемой связи.
	protected static bool NeedSnapshot(RICON_LinkPublicationItem it, RICON_LinkPublicationInput input, bool fullResync)
	{
		if (it.m_iKind != RICON_LinkPublicationItem.KEEP || fullResync)
			return true;
		RICON_LinkRegistryEntry e = it.m_Old;
		if (it.m_eState != e.m_eState || input.m_iTtlMs != e.m_iTtlMs)
			return true;
		// Разность двух binary32 с одним округлением к float32 до abs (промежуточная float-переменная).
		float diff = it.m_fQuality - e.m_fQuality;
		if (Math.AbsFloat(diff) >= input.m_fQualityDelta)
			return true;
		int half = e.m_iTtlMs / 2 + e.m_iTtlMs % 2;
		if (half < 1)
			half = 1;
		return input.m_iNowMs - e.m_iPublishedAtMs >= half;
	}

	RICON_LinkPublicationResult Publish(RICON_LinkPublicationInput input, RICON_LinkRegistry registry, RICON_LinkSequenceSource source)
	{
		RICON_LinkPublicationResult result = new RICON_LinkPublicationResult();
		array<ref RICON_LinkPublicationItem> plan = {};
		result.m_eCallCode = Preflight(input, registry, source, plan);
		if (result.m_eCallCode != RICON_ELinkPublicationCode.OK)
		{
			if (registry)
				result.m_Registry = registry.CloneRegistry();
			return result;
		}
		int epoch = input.m_iSessionEpoch;
		int firstEpoch = source.GetSessionEpoch();
		result.m_iEpochReads = 1;
		if (firstEpoch != epoch || registry.m_iSessionEpoch != epoch)
		{
			result.m_eCallCode = RICON_ELinkPublicationCode.NEED_FULL_RESYNC;
			result.m_bRequiresFullResync = true;
			result.m_Registry = ResetRegistry(firstEpoch, epoch, registry.m_iSessionEpoch, registry.m_iHighWaterLinkId);
			return result;
		}
		// REQ-LP-10: все новые id выделяются до первого Next, только в локальной новой версии.
		int highWater = registry.m_iHighWaterLinkId;
		foreach (RICON_LinkPublicationItem item : plan)
		{
			if (item.m_iKind == RICON_LinkPublicationItem.CREATE)
			{
				highWater++;
				item.m_iLinkId = highWater;
			}
			else
				item.m_iLinkId = item.m_Old.m_iLinkId;
		}
		bool full = registry.m_bNeedsFullResync;
		foreach (RICON_LinkPublicationItem step : plan)
		{
			step.m_bNeed = NeedSnapshot(step, input, full);
			if (!step.m_bNeed)
				continue;
			int seq;
			bool rotated;
			bool ok = source.NextLinkSequence(step.m_iLinkId, seq, rotated);
			result.m_iSequenceCalls++;
			int readEpoch = source.GetSessionEpoch();
			result.m_iEpochReads++;
			bool stale = step.m_Old && seq <= step.m_Old.m_iLinkSeq;
			if (!ok || rotated || seq <= 0 || seq >= int.MAX || stale || readEpoch != epoch)
			{
				result.m_eCallCode = RICON_ELinkPublicationCode.NEED_FULL_RESYNC;
				result.m_bRequiresFullResync = true;
				result.m_Registry = ResetRegistry(readEpoch, epoch, registry.m_iSessionEpoch, highWater);
				return result;
			}
			step.m_iLinkSeq = seq;
		}
		RICON_LinkRegistry fresh = new RICON_LinkRegistry();
		fresh.m_iSessionEpoch = epoch;
		fresh.m_iHighWaterLinkId = highWater;
		fresh.m_iLastTickMs = input.m_iNowMs;
		fresh.m_bNeedsFullResync = false;
		foreach (RICON_LinkPublicationItem it : plan)
		{
			int talker;
			if (it.m_Binding)
				talker = it.m_Binding.m_iTxPlayerId;
			else
				talker = it.m_Old.m_iTxPlayerId;
			if (it.m_bNeed)
			{
				RICON_LinkSnapshot s = new RICON_LinkSnapshot();
				s.m_iSchemaVersion = RICON_LinkSnapshot.CURRENT_SCHEMA_VERSION;
				s.m_iSessionEpoch = epoch;
				s.m_iLinkId = it.m_iLinkId;
				s.m_iLinkSeq = it.m_iLinkSeq;
				s.m_iTtlMs = input.m_iTtlMs;
				s.m_iRouteId = it.m_iLinkId;
				s.m_iRouteNodeCount = 2;
				s.m_iTalkerPlayerId = talker;
				s.m_iListenerPlayerId = it.m_iRxPlayerId;
				s.m_eState = it.m_eState;
				s.m_fQuality = it.m_fQuality;
				result.m_aSnapshots.Insert(s);
			}
			if (it.m_iKind == RICON_LinkPublicationItem.TERMINATE)
				continue;
			if (!it.m_bNeed)
			{
				fresh.m_aEntries.Insert(it.m_Old.CloneEntry());
				continue;
			}
			RICON_LinkRegistryEntry e = new RICON_LinkRegistryEntry();
			e.m_iLeaseId = it.m_iLeaseId;
			e.m_iRxDeviceId = it.m_iRxDeviceId;
			e.m_iRxTransceiverIndex = it.m_iRxTransceiverIndex;
			e.m_iRxPlayerId = it.m_iRxPlayerId;
			e.m_iTxPlayerId = it.m_Binding.m_iTxPlayerId;
			e.m_iTxDeviceId = it.m_Binding.m_iTxDeviceId;
			e.m_iTxTransceiverIndex = it.m_Binding.m_iTxTransceiverIndex;
			e.m_iLinkId = it.m_iLinkId;
			e.m_iLinkSeq = it.m_iLinkSeq;
			e.m_eState = it.m_eState;
			e.m_fQuality = it.m_fQuality;
			e.m_iTtlMs = input.m_iTtlMs;
			e.m_iPublishedAtMs = input.m_iNowMs;
			fresh.m_aEntries.Insert(e);
		}
		result.m_eCallCode = RICON_ELinkPublicationCode.OK;
		result.m_bRequiresFullResync = full;
		result.m_Registry = fresh;
		return result;
	}
}
