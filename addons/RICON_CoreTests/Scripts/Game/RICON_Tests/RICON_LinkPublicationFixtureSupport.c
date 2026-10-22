//! TEST_ONLY: корпус S3b, тестовый источник номеров REQ-LP §6, неизменность входа и контроль самого сравнения.
class RICON_LinkFakeSource : RICON_LinkSequenceSource
{
	int m_iEpoch;
	ref map<int, int> m_mSeq = new map<int, int>();
	int m_iFailAt;
	int m_iRotateAt;
	int m_iInvalidAt;
	int m_iInvalidSeq = int.MAX;
	bool m_bHasRotatedEpoch;
	int m_iRotatedEpoch;
	int m_iCalls;
	int m_iReads;

	void RICON_LinkFakeSource(int epoch)
	{
		m_iEpoch = epoch;
	}

	void SetSequence(int linkId, int seq)
	{
		m_mSeq.Set(linkId, seq);
	}

	void SetRotatedEpoch(int epoch)
	{
		m_bHasRotatedEpoch = true;
		m_iRotatedEpoch = epoch;
	}

	override int GetSessionEpoch()
	{
		m_iReads++;
		return m_iEpoch;
	}

	override bool NextLinkSequence(int linkId, out int linkSeq, out bool rotated)
	{
		m_iCalls++;
		linkSeq = 0;
		rotated = false;
		if (m_iCalls == m_iFailAt)
			return false;
		int current;
		if (!m_mSeq.Find(linkId, current))
			current = 0;
		if (m_iCalls == m_iRotateAt || current >= int.MAX - 1)
		{
			m_mSeq.Clear();
			if (m_bHasRotatedEpoch)
				m_iEpoch = m_iRotatedEpoch;
			else if (m_iEpoch < int.MAX - 1)
				m_iEpoch++;
			else
				m_iEpoch = 1;
			current = 0;
			rotated = true;
		}
		linkSeq = current + 1;
		if (m_iCalls == m_iInvalidAt)
			linkSeq = m_iInvalidSeq;
		m_mSeq.Set(linkId, linkSeq);
		return true;
	}

	//! Тот же начальный сценарий без счётчиков: для повторного вызова на нетронутом source.
	RICON_LinkFakeSource CloneSource()
	{
		RICON_LinkFakeSource c = new RICON_LinkFakeSource(m_iEpoch);
		foreach (int link, int seq : m_mSeq)
			c.m_mSeq.Set(link, seq);
		c.m_iFailAt = m_iFailAt;
		c.m_iRotateAt = m_iRotateAt;
		c.m_iInvalidAt = m_iInvalidAt;
		c.m_iInvalidSeq = m_iInvalidSeq;
		c.m_bHasRotatedEpoch = m_bHasRotatedEpoch;
		c.m_iRotatedEpoch = m_iRotatedEpoch;
		return c;
	}

	//! sourceState REQ-LP §6: конечные epoch и карта, без счётчиков и параметров инъекций.
	bool SameState(RICON_LinkFakeSource other)
	{
		if (!other || m_iEpoch != other.m_iEpoch || m_mSeq.Count() != other.m_mSeq.Count())
			return false;
		foreach (int link, int seq : m_mSeq)
		{
			int value;
			if (!other.m_mSeq.Find(link, value) || value != seq)
				return false;
		}
		return true;
	}
}

//! Значения и порядок всех входов, включая старый реестр (REQ-LP-01).
class RICON_LinkPublicationInputSnapshot
{
	ref array<int> m_aInts = {};
	ref array<float> m_aFloats = {};

	void Ints(array<int> values)
	{
		foreach (int value : values)
			m_aInts.Insert(value);
	}

	void Floats(array<float> values)
	{
		foreach (float value : values)
			m_aFloats.Insert(value);
	}

	void Registry(RICON_LinkRegistry r)
	{
		if (!r)
		{
			Ints({0});
			return;
		}
		Ints({1, r.m_iSessionEpoch, r.m_iHighWaterLinkId, r.m_iLastTickMs, r.m_bNeedsFullResync});
		if (!r.m_aEntries)
		{
			Ints({-1});
			return;
		}
		Ints({r.m_aEntries.Count()});
		foreach (RICON_LinkRegistryEntry e : r.m_aEntries)
		{
			if (!e)
			{
				Ints({0});
				continue;
			}
			Ints({1, e.m_iLeaseId, e.m_iRxDeviceId, e.m_iRxTransceiverIndex, e.m_iRxPlayerId, e.m_iTxPlayerId,
				e.m_iTxDeviceId, e.m_iTxTransceiverIndex, e.m_iLinkId, e.m_iLinkSeq, e.m_eState, e.m_iTtlMs, e.m_iPublishedAtMs});
			Floats({e.m_fQuality});
		}
	}

	static RICON_LinkPublicationInputSnapshot Capture(RICON_LinkPublicationInput input, RICON_LinkRegistry registry)
	{
		RICON_LinkPublicationInputSnapshot s = new RICON_LinkPublicationInputSnapshot();
		s.Registry(registry);
		if (!input)
		{
			s.Ints({0});
			return s;
		}
		s.Ints({1, input.m_iNowMs, input.m_iSessionEpoch, input.m_iTtlMs});
		s.Floats({input.m_fQualityDelta});
		if (input.m_Limits)
			s.Ints({1, input.m_Limits.m_iMaxBindings, input.m_Limits.m_iMaxPairs, input.m_Limits.m_iMaxActiveLinks});
		else
			s.Ints({0});
		if (!input.m_aBindings)
			s.Ints({-1});
		else
		{
			s.Ints({input.m_aBindings.Count()});
			foreach (RICON_TransmissionBinding b : input.m_aBindings)
			{
				if (!b)
					s.Ints({0});
				else
					s.Ints({1, b.m_iLeaseId, b.m_iSessionEpoch, b.m_iTxPlayerId, b.m_iTxGeneration, b.m_iTxDeviceId,
						b.m_iTxTransceiverIndex, b.m_iNetworkTypeId, b.m_iChannelId, b.m_bActive, b.m_iIssuedAtMs, b.m_iTtlMs});
			}
		}
		RICON_PairServiceResult pr = input.m_PairResult;
		if (!pr)
		{
			s.Ints({0});
			return s;
		}
		s.Ints({1, pr.m_eCallCode, pr.m_iS2Calls, pr.m_iEvaluatorCalls});
		if (!pr.m_aPairs)
		{
			s.Ints({-1});
			return s;
		}
		s.Ints({pr.m_aPairs.Count()});
		foreach (RICON_PairResult p : pr.m_aPairs)
		{
			if (!p)
			{
				s.Ints({0});
				continue;
			}
			s.Ints({1, p.m_iLeaseId, p.m_iTxDeviceId, p.m_iRxPlayerId, p.m_iRxDeviceId, p.m_iRxTransceiverIndex, p.m_ePairCode,
				p.m_bHasDeviceCode, p.m_eDeviceCode, p.m_bHasBudget, p.m_bValid, p.m_eReason, p.m_eState});
			s.Floats({p.m_fIntelligibility, p.m_fSinrDb});
		}
		return s;
	}

	//! Конечные — точным равенством; неконечные — по тексту (в Enforce x != x истинно для любого неконечного).
	static bool SameFloat(float a, float b)
	{
		bool af = RICON_Num.IsFinite(a);
		bool bf = RICON_Num.IsFinite(b);
		if (af && bf)
			return a == b;
		return af == bf && a.ToString() == b.ToString();
	}

	bool Same(RICON_LinkPublicationInputSnapshot other)
	{
		if (!other || m_aInts.Count() != other.m_aInts.Count() || m_aFloats.Count() != other.m_aFloats.Count())
			return false;
		for (int i = 0; i < m_aInts.Count(); i++)
		{
			if (m_aInts[i] != other.m_aInts[i])
				return false;
		}
		for (int j = 0; j < m_aFloats.Count(); j++)
		{
			if (!SameFloat(m_aFloats[j], other.m_aFloats[j]))
				return false;
		}
		return true;
	}
}

class RICON_LinkPublicationRun
{
	static const float QUALITY_TOLERANCE = 0.000001;

	int m_iCases;
	int m_iMatches;
	int m_iImmutable;
	int m_iControls;
	int m_iExtras;
	int m_iSnapshots;
	bool m_bOk = true;

	protected void Require(bool condition, string id, string what)
	{
		m_bOk = RICON_TestSupport.Expect(condition, id, what) && m_bOk;
	}

	protected void Reject(bool accepted)
	{
		Require(!accepted, "linkpub controls", "искажённое ожидание отвергнуто");
		if (!accepted)
			m_iControls++;
	}

	protected static bool SameQuality(float a, float b)
	{
		return RICON_Num.IsFinite(a) && RICON_Num.IsFinite(b) && Math.AbsFloat(a - b) <= QUALITY_TOLERANCE;
	}

	static bool SameSnapshot(RICON_LinkSnapshot a, RICON_LinkSnapshot b)
	{
		if (!a || !b)
			return false;
		if (a.m_iSchemaVersion != b.m_iSchemaVersion || a.m_iSessionEpoch != b.m_iSessionEpoch || a.m_iLinkId != b.m_iLinkId)
			return false;
		if (a.m_iLinkSeq != b.m_iLinkSeq || a.m_iTtlMs != b.m_iTtlMs || a.m_iRouteId != b.m_iRouteId)
			return false;
		if (a.m_iRouteNodeCount != b.m_iRouteNodeCount || a.m_iTalkerPlayerId != b.m_iTalkerPlayerId)
			return false;
		if (a.m_iListenerPlayerId != b.m_iListenerPlayerId || a.m_eState != b.m_eState)
			return false;
		return SameQuality(a.m_fQuality, b.m_fQuality);
	}

	static bool SameEntry(RICON_LinkRegistryEntry a, RICON_LinkRegistryEntry b)
	{
		if (!a || !b)
			return a == b;
		if (a.m_iLeaseId != b.m_iLeaseId || a.m_iRxDeviceId != b.m_iRxDeviceId || a.m_iRxTransceiverIndex != b.m_iRxTransceiverIndex)
			return false;
		if (a.m_iRxPlayerId != b.m_iRxPlayerId || a.m_iTxPlayerId != b.m_iTxPlayerId || a.m_iTxDeviceId != b.m_iTxDeviceId)
			return false;
		if (a.m_iTxTransceiverIndex != b.m_iTxTransceiverIndex || a.m_iLinkId != b.m_iLinkId || a.m_iLinkSeq != b.m_iLinkSeq)
			return false;
		if (a.m_eState != b.m_eState || a.m_iTtlMs != b.m_iTtlMs || a.m_iPublishedAtMs != b.m_iPublishedAtMs)
			return false;
		// Копия ошибочного реестра при отказе может содержать неконечное Q: неконечные сравниваются по тексту.
		if (!RICON_Num.IsFinite(a.m_fQuality) || !RICON_Num.IsFinite(b.m_fQuality))
			return RICON_LinkPublicationInputSnapshot.SameFloat(a.m_fQuality, b.m_fQuality);
		return SameQuality(a.m_fQuality, b.m_fQuality);
	}

	static bool SameRegistry(RICON_LinkRegistry a, RICON_LinkRegistry b)
	{
		if (!a || !b)
			return a == b;
		if (a.m_iSessionEpoch != b.m_iSessionEpoch || a.m_iHighWaterLinkId != b.m_iHighWaterLinkId)
			return false;
		if (a.m_iLastTickMs != b.m_iLastTickMs || a.m_bNeedsFullResync != b.m_bNeedsFullResync)
			return false;
		if (!a.m_aEntries || !b.m_aEntries)
			return a.m_aEntries == b.m_aEntries;
		if (a.m_aEntries.Count() != b.m_aEntries.Count())
			return false;
		for (int i = 0; i < a.m_aEntries.Count(); i++)
		{
			if (!SameEntry(a.m_aEntries[i], b.m_aEntries[i]))
				return false;
		}
		return true;
	}

	static bool SameResult(RICON_LinkPublicationResult a, RICON_LinkPublicationResult b)
	{
		if (!a || !b || !a.m_aSnapshots || !b.m_aSnapshots)
			return false;
		if (a.m_eCallCode != b.m_eCallCode || a.m_iSequenceCalls != b.m_iSequenceCalls || a.m_iEpochReads != b.m_iEpochReads)
			return false;
		if (a.m_bRequiresFullResync != b.m_bRequiresFullResync || a.m_aSnapshots.Count() != b.m_aSnapshots.Count())
			return false;
		for (int i = 0; i < a.m_aSnapshots.Count(); i++)
		{
			if (!SameSnapshot(a.m_aSnapshots[i], b.m_aSnapshots[i]))
				return false;
		}
		return SameRegistry(a.m_Registry, b.m_Registry);
	}

	//! Выход не делит объекты со входом и с предыдущим результатом (REQ-LP-01).
	protected static bool NoAlias(RICON_LinkPublicationResult r, RICON_LinkRegistry input, RICON_LinkPublicationResult other)
	{
		if (!r.m_Registry)
			return true;
		if (r.m_Registry == input || (other && r.m_Registry == other.m_Registry))
			return false;
		if (input && r.m_Registry.m_aEntries && r.m_Registry.m_aEntries == input.m_aEntries)
			return false;
		if (r.m_Registry.m_aEntries)
		{
			foreach (RICON_LinkRegistryEntry e : r.m_Registry.m_aEntries)
			{
				if (!e)
					continue;
				if (input && input.m_aEntries && input.m_aEntries.Find(e) >= 0)
					return false;
				if (other && other.m_Registry && other.m_Registry.m_aEntries && other.m_Registry.m_aEntries.Find(e) >= 0)
					return false;
			}
		}
		if (other)
		{
			foreach (RICON_LinkSnapshot s : r.m_aSnapshots)
			{
				if (other.m_aSnapshots.Find(s) >= 0)
					return false;
			}
		}
		return true;
	}

	protected void Controls(RICON_LinkPublicationResult actual, RICON_LinkPublicationResult expected)
	{
		Reject(SameResult(null, expected));
		actual.m_eCallCode = RICON_ELinkPublicationCode.NEED_FULL_RESYNC;
		Reject(SameResult(actual, expected));
		actual.m_eCallCode = RICON_ELinkPublicationCode.OK;
		actual.m_iSequenceCalls++;
		Reject(SameResult(actual, expected));
		actual.m_iSequenceCalls--;
		actual.m_iEpochReads--;
		Reject(SameResult(actual, expected));
		actual.m_iEpochReads++;
		actual.m_bRequiresFullResync = !actual.m_bRequiresFullResync;
		Reject(SameResult(actual, expected));
		actual.m_bRequiresFullResync = !actual.m_bRequiresFullResync;
		RICON_LinkSnapshot first = actual.m_aSnapshots[0];
		actual.m_aSnapshots[0] = actual.m_aSnapshots[1];
		actual.m_aSnapshots[1] = first;
		Reject(SameResult(actual, expected));
		actual.m_aSnapshots[1] = actual.m_aSnapshots[0];
		actual.m_aSnapshots[0] = first;
		RICON_LinkSnapshot last = actual.m_aSnapshots[actual.m_aSnapshots.Count() - 1];
		actual.m_aSnapshots.Remove(actual.m_aSnapshots.Count() - 1);
		Reject(SameResult(actual, expected));
		actual.m_aSnapshots.Insert(last);
		first.m_iLinkSeq++;
		Reject(SameResult(actual, expected));
		first.m_iLinkSeq--;
		first.m_iTalkerPlayerId++;
		Reject(SameResult(actual, expected));
		first.m_iTalkerPlayerId--;
		float q = first.m_fQuality;
		first.m_fQuality = q + 0.00001;
		Reject(SameResult(actual, expected));
		first.m_fQuality = RICON_L2Run.NaN();
		Reject(SameResult(actual, expected));
		first.m_fQuality = q;
		first.m_iRouteNodeCount = 1;
		Reject(SameResult(actual, expected));
		first.m_iRouteNodeCount = 2;
		RICON_LinkRegistryEntry entry = actual.m_Registry.m_aEntries[0];
		entry.m_iPublishedAtMs++;
		Reject(SameResult(actual, expected));
		entry.m_iPublishedAtMs--;
		actual.m_Registry.m_iHighWaterLinkId++;
		Reject(SameResult(actual, expected));
		actual.m_Registry.m_iHighWaterLinkId--;
		actual.m_Registry.m_bNeedsFullResync = true;
		Reject(SameResult(actual, expected));
		actual.m_Registry.m_bNeedsFullResync = false;
	}

	//! Перестановка всех трёх массивов не влияет на выход (REQ-LP-05, порядок ключей).
	protected void Permutation(RICON_LinkPublicationInput input, RICON_LinkRegistry registry, RICON_LinkFakeSource pristine,
		RICON_LinkPublicationResult expected)
	{
		for (int i = 0; i < input.m_aBindings.Count() / 2; i++)
		{
			int bi = input.m_aBindings.Count() - 1 - i;
			RICON_TransmissionBinding b = input.m_aBindings[i];
			input.m_aBindings[i] = input.m_aBindings[bi];
			input.m_aBindings[bi] = b;
		}
		array<ref RICON_PairResult> pairs = input.m_PairResult.m_aPairs;
		for (int j = 0; j < pairs.Count() / 2; j++)
		{
			int pj = pairs.Count() - 1 - j;
			RICON_PairResult p = pairs[j];
			pairs[j] = pairs[pj];
			pairs[pj] = p;
		}
		for (int k = 0; k < registry.m_aEntries.Count() / 2; k++)
		{
			int ek = registry.m_aEntries.Count() - 1 - k;
			RICON_LinkRegistryEntry e = registry.m_aEntries[k];
			registry.m_aEntries[k] = registry.m_aEntries[ek];
			registry.m_aEntries[ek] = e;
		}
		RICON_LinkFakeSource source = pristine.CloneSource();
		RICON_LinkPublicationService service = new RICON_LinkPublicationService();
		RICON_LinkPublicationResult result = service.Publish(input, registry, source);
		bool ok = SameResult(result, expected) && result.m_iSequenceCalls == source.m_iCalls && result.m_iEpochReads == source.m_iReads;
		Require(ok, "linkpub permutation", "порядок bindings, pairs и entries не влияет на выход");
		if (ok)
			m_iExtras++;
	}

	protected void PrintSnapshots(RICON_LinkPublicationResult result)
	{
		foreach (RICON_LinkSnapshot s : result.m_aSnapshots)
			PrintFormat("[RICON_LINKPUB_SNAP] link=%1 seq=%2 epoch=%3 talker=%4 listener=%5 state=%6 q=%7 ttl=%8", s.m_iLinkId,
				s.m_iLinkSeq, s.m_iSessionEpoch, s.m_iTalkerPlayerId, s.m_iListenerPlayerId,
				typename.EnumToString(RICON_ELinkState, s.m_eState), s.m_fQuality.ToString(-1, 9), s.m_iTtlMs);
	}

	void CheckCase(string id, RICON_LinkPublicationInput input, RICON_LinkRegistry registry, RICON_LinkFakeSource source,
		RICON_LinkPublicationResult expected, RICON_LinkFakeSource expectedSource)
	{
		m_iCases++;
		RICON_LinkPublicationInputSnapshot before = RICON_LinkPublicationInputSnapshot.Capture(input, registry);
		RICON_LinkFakeSource pristine;
		if (source)
			pristine = source.CloneSource();
		RICON_LinkPublicationService service = new RICON_LinkPublicationService();
		RICON_LinkSequenceSource used = source;
		RICON_LinkPublicationResult actual = service.Publish(input, registry, used);
		bool matched = SameResult(actual, expected);
		if (source)
			matched = matched && actual.m_iSequenceCalls == source.m_iCalls && actual.m_iEpochReads == source.m_iReads
				&& source.SameState(expectedSource);
		else
			matched = matched && !expectedSource;
		Require(matched, id, "полный результат, состояние источника и фактические вызовы совпадают с эталоном");
		if (matched)
			m_iMatches++;
		m_iSnapshots += actual.m_aSnapshots.Count();
		bool immutable = before.Same(RICON_LinkPublicationInputSnapshot.Capture(input, registry)) && NoAlias(actual, registry, null);
		// Искажение выхода не должно менять входы или следующий результат.
		if (actual.m_Registry)
			actual.m_Registry.m_iHighWaterLinkId = -77;
		if (actual.m_aSnapshots.Count() > 0)
			actual.m_aSnapshots[0].m_iLinkSeq = -5;
		RICON_LinkFakeSource secondSource;
		if (pristine)
			secondSource = pristine.CloneSource();
		RICON_LinkPublicationService secondService = new RICON_LinkPublicationService();
		RICON_LinkSequenceSource secondUsed = secondSource;
		RICON_LinkPublicationResult second = secondService.Publish(input, registry, secondUsed);
		immutable = immutable && before.Same(RICON_LinkPublicationInputSnapshot.Capture(input, registry)) && SameResult(second, expected);
		immutable = immutable && second != actual && NoAlias(second, registry, actual);
		if (secondSource)
			immutable = immutable && second.m_iSequenceCalls == secondSource.m_iCalls && second.m_iEpochReads == secondSource.m_iReads;
		Require(immutable, id, "входы и старый реестр сохранены, повторный результат новый и без общих объектов");
		if (immutable)
			m_iImmutable++;
		if (id == "P04" && matched && immutable)
		{
			Controls(second, expected);
			Require(SameResult(second, expected), "linkpub controls", "после контролей исходный результат восстановлен");
			PrintSnapshots(second);
			Permutation(input, registry, pristine, expected);
		}
		PrintFormat("[RICON_LINKPUB] %1 call=%2 snapshots=%3 seq=%4 reads=%5 full=%6 match=%7 immutable=%8", id,
			typename.EnumToString(RICON_ELinkPublicationCode, second.m_eCallCode), second.m_aSnapshots.Count(),
			second.m_iSequenceCalls, second.m_iEpochReads, second.m_bRequiresFullResync, matched, immutable);
	}

	//! Базовый источник и адаптер без менеджера fail-closed (REQ-LP-08); настоящий manager authority — NOT_RUN.
	void CheckSources()
	{
		RICON_LinkSequenceSource fallback = new RICON_LinkSequenceSource();
		int seq = 99;
		bool rotated = true;
		bool ok = fallback.GetSessionEpoch() == 0 && !fallback.NextLinkSequence(1, seq, rotated) && seq == 0 && !rotated;
		RICON_ManagerLinkSequenceSource adapter = new RICON_ManagerLinkSequenceSource(null);
		seq = 99;
		rotated = true;
		ok = ok && adapter.GetSessionEpoch() == 0 && !adapter.NextLinkSequence(1, seq, rotated) && seq == 0 && !rotated;
		Require(ok, "linkpub sources", "базовый источник и адаптер без менеджера: epoch 0, Next false");
		if (ok)
			m_iExtras++;
		RICON_LinkPublicationInput input = new RICON_LinkPublicationInput();
		input.m_iNowMs = 0;
		input.m_iSessionEpoch = 3;
		input.m_iTtlMs = 1000;
		input.m_fQualityDelta = 0.5;
		input.m_Limits = new RICON_LinkPublicationLimits();
		input.m_Limits.m_iMaxBindings = 1;
		input.m_Limits.m_iMaxPairs = 1;
		input.m_Limits.m_iMaxActiveLinks = 1;
		input.m_PairResult = new RICON_PairServiceResult();
		input.m_PairResult.m_eCallCode = RICON_EPairServiceCode.OK;
		RICON_LinkRegistry registry = new RICON_LinkRegistry();
		registry.m_iSessionEpoch = 3;
		RICON_LinkPublicationService service = new RICON_LinkPublicationService();
		RICON_LinkPublicationResult r = service.Publish(input, registry, adapter);
		bool resync = r.m_eCallCode == RICON_ELinkPublicationCode.NEED_FULL_RESYNC && r.m_iEpochReads == 1
			&& r.m_iSequenceCalls == 0 && r.m_bRequiresFullResync && r.m_aSnapshots.Count() == 0
			&& r.m_Registry && r.m_Registry.m_iSessionEpoch == 3 && r.m_Registry.m_bNeedsFullResync;
		Require(resync, "linkpub adapter", "адаптер без менеджера: NEED_FULL_RESYNC, реестр с входной epoch");
		if (resync)
			m_iExtras++;
		CheckOtherEpoch();
	}

	//! Unit codex #3343: реестр другой epoch с highWater=MAX и полным maxActive считается пустым для ACTIVE_LIMIT и
	//! ID_EXHAUSTED; итог — NEED_FULL_RESYNC после одного чтения epoch, highWater сбрасывается в 0.
	protected void CheckOtherEpoch()
	{
		RICON_LinkPublicationInput input = new RICON_LinkPublicationInput();
		input.m_iNowMs = 5;
		input.m_iSessionEpoch = 5;
		input.m_iTtlMs = 1000;
		input.m_fQualityDelta = 0.125;
		input.m_Limits = new RICON_LinkPublicationLimits();
		input.m_Limits.m_iMaxBindings = 1;
		input.m_Limits.m_iMaxPairs = 1;
		input.m_Limits.m_iMaxActiveLinks = 1;
		RICON_TransmissionBinding b = new RICON_TransmissionBinding();
		b.m_iLeaseId = 21;
		b.m_iSessionEpoch = 5;
		b.m_iTxPlayerId = 1;
		b.m_iTxGeneration = 1;
		b.m_iTxDeviceId = 101;
		b.m_iTxTransceiverIndex = 0;
		b.m_iNetworkTypeId = 1;
		b.m_iChannelId = 1;
		b.m_bActive = true;
		b.m_iIssuedAtMs = 0;
		b.m_iTtlMs = 10000;
		input.m_aBindings.Insert(b);
		input.m_PairResult = new RICON_PairServiceResult();
		input.m_PairResult.m_eCallCode = RICON_EPairServiceCode.OK;
		RICON_PairResult p = new RICON_PairResult();
		p.m_iLeaseId = 21;
		p.m_iTxDeviceId = 101;
		p.m_iRxPlayerId = 2;
		p.m_iRxDeviceId = 202;
		p.m_iRxTransceiverIndex = 0;
		p.m_ePairCode = RICON_EPairCode.EVALUATED;
		p.m_bHasDeviceCode = true;
		p.m_eDeviceCode = RICON_ERadioQueryCode.READY;
		p.m_bHasBudget = true;
		p.m_bValid = true;
		p.m_eReason = RICON_EReason.OK;
		p.m_eState = RICON_ELinkState.CLEAN;
		p.m_fIntelligibility = 0.75;
		p.m_fSinrDb = 16;
		input.m_PairResult.m_aPairs.Insert(p);
		RICON_LinkRegistry registry = new RICON_LinkRegistry();
		registry.m_iSessionEpoch = 4;
		registry.m_iHighWaterLinkId = int.MAX;
		registry.m_iLastTickMs = 10;
		registry.m_bNeedsFullResync = false;
		RICON_LinkRegistryEntry e = new RICON_LinkRegistryEntry();
		e.m_iLeaseId = 11;
		e.m_iRxDeviceId = 202;
		e.m_iRxTransceiverIndex = 0;
		e.m_iRxPlayerId = 2;
		e.m_iTxPlayerId = 1;
		e.m_iTxDeviceId = 101;
		e.m_iTxTransceiverIndex = 0;
		e.m_iLinkId = int.MAX;
		e.m_iLinkSeq = 5;
		e.m_eState = RICON_ELinkState.CLEAN;
		e.m_fQuality = 0.5;
		e.m_iTtlMs = 1000;
		e.m_iPublishedAtMs = 10;
		registry.m_aEntries.Insert(e);
		RICON_LinkFakeSource source = new RICON_LinkFakeSource(5);
		RICON_LinkPublicationService service = new RICON_LinkPublicationService();
		RICON_LinkPublicationResult r = service.Publish(input, registry, source);
		bool ok = r.m_eCallCode == RICON_ELinkPublicationCode.NEED_FULL_RESYNC && r.m_iEpochReads == 1 && source.m_iReads == 1;
		ok = ok && r.m_iSequenceCalls == 0 && source.m_iCalls == 0 && r.m_bRequiresFullResync && r.m_aSnapshots.Count() == 0;
		ok = ok && r.m_Registry && r.m_Registry.m_iSessionEpoch == 5 && r.m_Registry.m_iHighWaterLinkId == 0;
		ok = ok && r.m_Registry.m_iLastTickMs == -1 && r.m_Registry.m_bNeedsFullResync && r.m_Registry.m_aEntries.Count() == 0;
		ok = ok && registry.m_iHighWaterLinkId == int.MAX && registry.m_aEntries.Count() == 1;
		Require(ok, "linkpub other epoch", "другая epoch: NEED_FULL_RESYNC без ложных ACTIVE_LIMIT/ID_EXHAUSTED, highWater 0");
		if (ok)
			m_iExtras++;
	}
}
