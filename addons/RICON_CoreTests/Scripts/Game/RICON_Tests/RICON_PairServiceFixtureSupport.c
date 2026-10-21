//! TEST_ONLY: корпус S3, реальные вызовы S2/evaluator, неизменность входов и контроль самого сравнения.
class RICON_PairCountedService : RICON_PairService
{
	int m_iActualCalls;
	override protected RICON_ERadioQueryCode BuildQuery(RICON_PairEndpoint tx, RICON_PairEndpoint rx,
		RICON_RadioQueryContext context, out RICON_LinkQuery query)
	{
		m_iActualCalls++;
		return super.BuildQuery(tx, rx, context, query);
	}
}

class RICON_PairCountedEvaluator : RICON_LinkEvaluator
{
	int m_iActualCalls;
	protected ref RICON_LinkEvaluator m_Inner;
	void RICON_PairCountedEvaluator()
	{
		m_Inner = new RICON_WirelessLinkEvaluator("test_vhf", new RICON_FreeSpaceModel(),
			new RICON_NoiseModel(), RICON_TestSupport.NewTestIntelligibility());
	}
	override RICON_LinkBudget Evaluate(RICON_LinkQuery query)
	{
		m_iActualCalls++;
		return m_Inner.Evaluate(query);
	}
}

class RICON_PairNullBudgetEvaluator : RICON_LinkEvaluator
{
	int m_iActualCalls;
	override RICON_LinkBudget Evaluate(RICON_LinkQuery query)
	{
		m_iActualCalls++;
		return null;
	}
}

//! Фиксирует значения и порядок всех входов; RF-объекты копируются существующими помощниками S2.
class RICON_PairInputSnapshot
{
	ref array<int> m_aInts = {};
	ref array<ref RICON_RadioObservation> m_aObservations = {};
	ref array<ref RICON_RadioRfProfile> m_aProfiles = {};
	ref RICON_RadioQueryContext m_Context;

	void Ints(array<int> values)
	{
		foreach (int value : values)
			m_aInts.Insert(value);
	}

	static RICON_PairInputSnapshot Capture(RICON_PairServiceInput input)
	{
		RICON_PairInputSnapshot s = new RICON_PairInputSnapshot();
		if (!input)
		{
			s.Ints({0});
			return s;
		}
		s.Ints({1, input.m_iNowMs, input.m_iSessionEpoch});
		s.m_Context = RICON_RadioDeviceRun.CopyContext(input.m_Context);
		if (input.m_Limits)
		{
			RICON_PairServiceLimits lim = input.m_Limits;
			s.Ints({1, lim.m_iMaxBindings, lim.m_iMaxListeners, lim.m_iMaxPlayers,
				lim.m_iMaxDevices, lim.m_iMaxEndpoints, lim.m_iMaxPairs});
		}
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
		if (!input.m_aListeners)
			s.Ints({-1});
		else
		{
			s.Ints({input.m_aListeners.Count()});
			foreach (RICON_PairListener l : input.m_aListeners)
			{
				if (!l)
					s.Ints({0});
				else
					s.Ints({1, l.m_iPlayerId, l.m_iGeneration, l.m_iSessionEpoch, l.m_iDeviceId, l.m_iTransceiverIndex});
			}
		}
		if (!input.m_aPlayers)
			s.Ints({-1});
		else
		{
			s.Ints({input.m_aPlayers.Count()});
			foreach (RICON_PairPlayer p : input.m_aPlayers)
			{
				if (!p)
					s.Ints({0});
				else
					s.Ints({1, p.m_iPlayerId, p.m_iGeneration, p.m_eState});
			}
		}
		if (!input.m_aDevices)
			s.Ints({-1});
		else
		{
			s.Ints({input.m_aDevices.Count()});
			foreach (RICON_LifecycleDevice d : input.m_aDevices)
			{
				if (!d)
				{
					s.Ints({0});
					continue;
				}
				s.Ints({1, d.m_iDeviceId, d.m_eKind, d.m_iOwnerPlayerId, d.m_bPowered});
				if (!d.m_aAccessPlayerIds)
					s.Ints({-1});
				else
				{
					s.Ints({d.m_aAccessPlayerIds.Count()});
					s.Ints(d.m_aAccessPlayerIds);
				}
				if (!d.m_aAllowed)
					s.Ints({-1});
				else
				{
					s.Ints({d.m_aAllowed.Count()});
					foreach (RICON_LifecycleCatalogPair pair : d.m_aAllowed)
					{
						if (!pair)
							s.Ints({0});
						else
							s.Ints({1, pair.m_iNetworkTypeId, pair.m_iChannelId});
					}
				}
			}
		}
		if (!input.m_aEndpoints)
			s.Ints({-1});
		else
		{
			s.Ints({input.m_aEndpoints.Count()});
			foreach (RICON_PairEndpoint e : input.m_aEndpoints)
			{
				if (!e)
				{
					s.Ints({0});
					continue;
				}
				s.Ints({1});
				s.m_aObservations.Insert(RICON_RadioDeviceRun.CopyObservation(e.m_Observation));
				s.m_aProfiles.Insert(RICON_RadioDeviceRun.CopyProfile(e.m_Profile));
			}
		}
		return s;
	}

	bool Same(RICON_PairInputSnapshot other)
	{
		if (!other || m_aInts.Count() != other.m_aInts.Count())
			return false;
		for (int i = 0; i < m_aInts.Count(); i++)
		{
			if (m_aInts[i] != other.m_aInts[i])
				return false;
		}
		if (!RICON_RadioDeviceRun.SameContext(m_Context, other.m_Context))
			return false;
		if (m_aObservations.Count() != other.m_aObservations.Count() || m_aProfiles.Count() != other.m_aProfiles.Count())
			return false;
		for (int j = 0; j < m_aObservations.Count(); j++)
		{
			if (!RICON_RadioDeviceRun.SameObservation(m_aObservations[j], other.m_aObservations[j]))
				return false;
			if (!RICON_RadioDeviceRun.SameProfile(m_aProfiles[j], other.m_aProfiles[j]))
				return false;
		}
		return true;
	}
}

class RICON_PairServiceRun
{
	int m_iCases;
	int m_iMatches;
	int m_iImmutable;
	int m_iControls;
	int m_iExtras;
	int m_iPairs;
	bool m_bOk = true;

	protected void Require(bool condition, string id, string what)
	{
		m_bOk = RICON_TestSupport.Expect(condition, id, what) && m_bOk;
	}

	protected void Reject(bool accepted)
	{
		Require(!accepted, "pair controls", "искажённое ожидание отвергнуто");
		if (!accepted)
			m_iControls++;
	}

	static bool SamePair(RICON_PairResult a, RICON_PairResult b)
	{
		if (!a || !b)
			return false;
		if (a.m_iLeaseId != b.m_iLeaseId || a.m_iTxDeviceId != b.m_iTxDeviceId || a.m_iRxPlayerId != b.m_iRxPlayerId)
			return false;
		if (a.m_iRxDeviceId != b.m_iRxDeviceId || a.m_iRxTransceiverIndex != b.m_iRxTransceiverIndex || a.m_ePairCode != b.m_ePairCode)
			return false;
		if (a.m_bHasDeviceCode != b.m_bHasDeviceCode || a.m_eDeviceCode != b.m_eDeviceCode || a.m_bHasBudget != b.m_bHasBudget)
			return false;
		if (a.m_bValid != b.m_bValid || a.m_eReason != b.m_eReason || a.m_eState != b.m_eState)
			return false;
		if (!RICON_Num.IsFinite(a.m_fSinrDb) || !RICON_Num.IsFinite(a.m_fIntelligibility))
			return false;
		if (!a.m_bHasBudget || !a.m_bValid)
			return a.m_fSinrDb == b.m_fSinrDb && a.m_fIntelligibility == b.m_fIntelligibility;
		return Math.AbsFloat(a.m_fSinrDb - b.m_fSinrDb) <= 0.05 && Math.AbsFloat(a.m_fIntelligibility - b.m_fIntelligibility) <= 0.002;
	}

	static bool SameResult(RICON_PairServiceResult a, RICON_PairServiceResult b)
	{
		if (!a || !b || !a.m_aPairs || !b.m_aPairs)
			return false;
		if (a.m_eCallCode != b.m_eCallCode || a.m_iS2Calls != b.m_iS2Calls || a.m_iEvaluatorCalls != b.m_iEvaluatorCalls)
			return false;
		if (a.m_aPairs.Count() != b.m_aPairs.Count())
			return false;
		for (int i = 0; i < a.m_aPairs.Count(); i++)
		{
			if (!SamePair(a.m_aPairs[i], b.m_aPairs[i]))
				return false;
		}
		return true;
	}

	protected void Controls(RICON_PairServiceResult actual, RICON_PairServiceResult expected)
	{
		Reject(SameResult(null, expected));
		actual.m_aPairs.Insert(actual.m_aPairs[0]);
		Reject(SameResult(actual, expected));
		actual.m_aPairs.Remove(actual.m_aPairs.Count() - 1);
		RICON_PairResult last = actual.m_aPairs[actual.m_aPairs.Count() - 1];
		actual.m_aPairs.Remove(actual.m_aPairs.Count() - 1);
		Reject(SameResult(actual, expected));
		actual.m_aPairs.Insert(last);
		RICON_PairResult first = actual.m_aPairs[0];
		actual.m_aPairs[0] = actual.m_aPairs[1];
		actual.m_aPairs[1] = first;
		Reject(SameResult(actual, expected));
		actual.m_aPairs[1] = actual.m_aPairs[0];
		actual.m_aPairs[0] = first;
		actual.m_iS2Calls++;
		Reject(SameResult(actual, expected));
		actual.m_iS2Calls -= 2;
		Reject(SameResult(actual, expected));
		actual.m_iS2Calls++;
		actual.m_iEvaluatorCalls++;
		Reject(SameResult(actual, expected));
		actual.m_iEvaluatorCalls -= 2;
		Reject(SameResult(actual, expected));
		actual.m_iEvaluatorCalls++;
		RICON_EPairCode originalCode = first.m_ePairCode;
		first.m_ePairCode = RICON_EPairCode.SELF_PAIR;
		Reject(SameResult(actual, expected));
		first.m_ePairCode = originalCode;
		float sinr = first.m_fSinrDb;
		first.m_fSinrDb += 1;
		Reject(SameResult(actual, expected));
		first.m_fSinrDb = sinr;
		float q = first.m_fIntelligibility;
		first.m_fIntelligibility = 0.123;
		Reject(SameResult(actual, expected));
		first.m_fIntelligibility = RICON_L2Run.NaN();
		Reject(SameResult(actual, expected));
		first.m_fIntelligibility = q;
		first.m_iRxTransceiverIndex++;
		Reject(SameResult(actual, expected));
		first.m_iRxTransceiverIndex--;
		first.m_bHasBudget = false;
		Reject(SameResult(actual, expected));
		first.m_bHasBudget = true;
		actual.m_eCallCode = RICON_EPairServiceCode.NULL_INPUT;
		Reject(SameResult(actual, expected));
		actual.m_eCallCode = RICON_EPairServiceCode.OK;
	}

	//! Перестановку выполняет сам стенд после проверки исходного ввода; каждый Case создаёт свои объекты.
	protected void Permutation(RICON_PairServiceInput input, RICON_PairServiceResult expected)
	{
		for (int i = 0; i < input.m_aBindings.Count() / 2; i++)
		{
			int bi = input.m_aBindings.Count() - 1 - i;
			RICON_TransmissionBinding b = input.m_aBindings[i];
			input.m_aBindings[i] = input.m_aBindings[bi];
			input.m_aBindings[bi] = b;
		}
		for (int j = 0; j < input.m_aListeners.Count() / 2; j++)
		{
			int li = input.m_aListeners.Count() - 1 - j;
			RICON_PairListener l = input.m_aListeners[j];
			input.m_aListeners[j] = input.m_aListeners[li];
			input.m_aListeners[li] = l;
		}
		for (int k = 0; k < input.m_aPlayers.Count() / 2; k++)
		{
			int pi = input.m_aPlayers.Count() - 1 - k;
			RICON_PairPlayer p = input.m_aPlayers[k];
			input.m_aPlayers[k] = input.m_aPlayers[pi];
			input.m_aPlayers[pi] = p;
		}
		for (int n = 0; n < input.m_aDevices.Count() / 2; n++)
		{
			int di = input.m_aDevices.Count() - 1 - n;
			RICON_LifecycleDevice d = input.m_aDevices[n];
			input.m_aDevices[n] = input.m_aDevices[di];
			input.m_aDevices[di] = d;
		}
		for (int m = 0; m < input.m_aEndpoints.Count() / 2; m++)
		{
			int ei = input.m_aEndpoints.Count() - 1 - m;
			RICON_PairEndpoint e = input.m_aEndpoints[m];
			input.m_aEndpoints[m] = input.m_aEndpoints[ei];
			input.m_aEndpoints[ei] = e;
		}
		RICON_PairInputSnapshot before = RICON_PairInputSnapshot.Capture(input);
		RICON_PairCountedService service = new RICON_PairCountedService();
		RICON_PairCountedEvaluator evaluator = new RICON_PairCountedEvaluator();
		RICON_PairServiceResult result = service.Evaluate(input, evaluator);
		bool ok = SameResult(result, expected) && before.Same(RICON_PairInputSnapshot.Capture(input));
		ok = ok && result.m_iS2Calls == service.m_iActualCalls && result.m_iEvaluatorCalls == evaluator.m_iActualCalls;
		Require(ok, "pair permutation", "порядок всех пяти массивов не влияет на выход");
		if (ok)
			m_iExtras++;
		RICON_PairNullBudgetEvaluator broken = new RICON_PairNullBudgetEvaluator();
		RICON_PairServiceResult guarded = service.Evaluate(input, broken);
		int invalidBudgets = 0;
		foreach (RICON_PairResult pair : guarded.m_aPairs)
		{
			if (pair.m_ePairCode == RICON_EPairCode.EVALUATED && pair.m_bHasBudget && !pair.m_bValid
				&& pair.m_eReason == RICON_EReason.NUMERIC_FAILURE && pair.m_eState == RICON_ELinkState.LOST
				&& pair.m_fIntelligibility == 0 && pair.m_fSinrDb == 0)
				invalidBudgets++;
		}
		bool safe = invalidBudgets == expected.m_iEvaluatorCalls && broken.m_iActualCalls == expected.m_iEvaluatorCalls;
		Require(safe, "pair null budget", "нарушение интерфейса evaluator не приводит к падению");
		if (safe)
			m_iExtras++;
	}

	protected void PrintPairs(RICON_PairServiceResult result)
	{
		foreach (RICON_PairResult pair : result.m_aPairs)
		{
			string key = pair.m_iLeaseId.ToString() + ":" + pair.m_iRxDeviceId.ToString()
				+ ":" + pair.m_iRxTransceiverIndex.ToString() + ":" + pair.m_iRxPlayerId.ToString();
			PrintFormat("[RICON_PAIR_ROW] key=%1 pair=%2 hasDevice=%3 device=%4", key,
				typename.EnumToString(RICON_EPairCode, pair.m_ePairCode), pair.m_bHasDeviceCode,
				typename.EnumToString(RICON_ERadioQueryCode, pair.m_eDeviceCode));
			PrintFormat("[RICON_PAIR_BUDGET] key=%1 has=%2 valid=%3 reason=%4 state=%5 q=%6 sinr=%7", key,
				pair.m_bHasBudget, pair.m_bValid, typename.EnumToString(RICON_EReason, pair.m_eReason),
				typename.EnumToString(RICON_ELinkState, pair.m_eState), pair.m_fIntelligibility.ToString(-1, 9),
				pair.m_fSinrDb.ToString(-1, 9));
		}
	}

	void CheckCase(string id, RICON_PairServiceInput input, bool hasEvaluator, RICON_PairServiceResult expected)
	{
		m_iCases++;
		RICON_PairInputSnapshot before = RICON_PairInputSnapshot.Capture(input);
		RICON_PairCountedService service = new RICON_PairCountedService();
		RICON_PairCountedEvaluator counter = new RICON_PairCountedEvaluator();
		RICON_LinkEvaluator evaluator;
		if (hasEvaluator)
			evaluator = counter;
		RICON_PairServiceResult actual = service.Evaluate(input, evaluator);
		bool matched = SameResult(actual, expected);
		matched = matched && actual.m_iS2Calls == service.m_iActualCalls && actual.m_iEvaluatorCalls == counter.m_iActualCalls;
		Require(matched, id, "полный результат и фактические вызовы совпадают с эталоном");
		if (matched)
			m_iMatches++;
		m_iPairs += actual.m_aPairs.Count();
		bool immutable = before.Same(RICON_PairInputSnapshot.Capture(input));
		// Искажение выхода не должно менять входы или следующий результат.
		actual.m_iS2Calls = -99;
		if (actual.m_aPairs.Count() > 0)
		{
			actual.m_aPairs[0].m_iLeaseId = -123;
			actual.m_aPairs[0].m_fSinrDb = 999;
		}
		RICON_PairCountedService secondService = new RICON_PairCountedService();
		RICON_PairCountedEvaluator secondCounter = new RICON_PairCountedEvaluator();
		RICON_LinkEvaluator secondEvaluator;
		if (hasEvaluator)
			secondEvaluator = secondCounter;
		RICON_PairServiceResult second = secondService.Evaluate(input, secondEvaluator);
		immutable = immutable && before.Same(RICON_PairInputSnapshot.Capture(input)) && SameResult(second, expected);
		immutable = immutable && second != actual && second.m_aPairs != actual.m_aPairs;
		for (int pi = 0; pi < second.m_aPairs.Count() && pi < actual.m_aPairs.Count(); pi++)
		{
			if (second.m_aPairs[pi] == actual.m_aPairs[pi])
				immutable = false;
		}
		immutable = immutable && second.m_iS2Calls == secondService.m_iActualCalls && second.m_iEvaluatorCalls == secondCounter.m_iActualCalls;
		Require(immutable, id, "все входы и порядок сохранены, повторный результат новый");
		if (immutable)
			m_iImmutable++;
		if (id == "P01" && matched && immutable)
		{
			Controls(second, expected);
			Require(SameResult(second, expected), "pair controls", "после контролей исходный результат восстановлен");
			PrintPairs(second);
			Permutation(input, expected);
		}
		PrintFormat("[RICON_PAIR] %1 call=%2 pairs=%3 s2=%4 evaluator=%5 match=%6 immutable=%7", id,
			typename.EnumToString(RICON_EPairServiceCode, second.m_eCallCode), second.m_aPairs.Count(),
			second.m_iS2Calls, second.m_iEvaluatorCalls, matched, immutable);
	}

	void CheckEmpty()
	{
		RICON_PairServiceInput input = new RICON_PairServiceInput();
		input.m_iNowMs = 0;
		input.m_iSessionEpoch = 1;
		input.m_Context = new RICON_RadioQueryContext();
		input.m_Limits = new RICON_PairServiceLimits();
		input.m_Limits.m_iMaxBindings = 1;
		input.m_Limits.m_iMaxListeners = 1;
		input.m_Limits.m_iMaxPlayers = 1;
		input.m_Limits.m_iMaxDevices = 1;
		input.m_Limits.m_iMaxEndpoints = 1;
		input.m_Limits.m_iMaxPairs = 1;
		RICON_PairCountedService service = new RICON_PairCountedService();
		RICON_PairCountedEvaluator evaluator = new RICON_PairCountedEvaluator();
		RICON_PairServiceResult result = service.Evaluate(input, evaluator);
		bool ok = result.m_eCallCode == RICON_EPairServiceCode.OK && result.m_aPairs.Count() == 0;
		ok = ok && result.m_iS2Calls == 0 && result.m_iEvaluatorCalls == 0 && service.m_iActualCalls == 0 && evaluator.m_iActualCalls == 0;
		Require(ok, "pair empty", "пустая матрица: OK, пар и фактических вызовов нет");
		if (ok)
			m_iExtras++;
		RICON_PairServiceResult absent = service.Evaluate(null, evaluator);
		bool refused = absent.m_eCallCode == RICON_EPairServiceCode.NULL_INPUT && absent.m_aPairs.Count() == 0;
		refused = refused && service.m_iActualCalls == 0 && evaluator.m_iActualCalls == 0;
		Require(refused, "pair null", "null входа: отказ до фактических вызовов");
		if (refused)
			m_iExtras++;
	}
}
