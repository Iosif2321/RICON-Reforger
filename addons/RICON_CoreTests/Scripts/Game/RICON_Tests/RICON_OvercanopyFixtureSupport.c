//! TEST_ONLY: frozen P4.2, все поля результата, неизменность входа и независимые контроли сравнения.
class RICON_OvercanopyRun
{
	int m_iCases;
	int m_iMatches;
	int m_iImmutable;
	int m_iControls;
	int m_iExtras;
	bool m_bOk = true;
	ref array<ref RICON_OvercanopyInput> m_aInputs = {};
	ref array<ref RICON_OvercanopyResult> m_aExpected = {};
	RICON_OvercanopyInput m_Base;
	RICON_OvercanopyResult m_BaseExpected;

	protected void Require(bool condition, string why)
	{
		m_bOk = RICON_TestSupport.Expect(condition, "overcanopy", why) && m_bOk;
	}

	static bool SameTerminal(RICON_OvercanopyTerminal a, RICON_OvercanopyTerminal b)
	{
		if (!a || !b || a.m_sEnd != b.m_sEnd || a.m_sMaterialId != b.m_sMaterialId || a.m_sCanopyProvenance != b.m_sCanopyProvenance)
			return false;
		if (a.m_fIntervalStartM != b.m_fIntervalStartM || a.m_fIntervalEndM != b.m_fIntervalEndM)
			return false;
		if (!RICON_VegetationRun.Close(a.m_fAntennaHeightM, b.m_fAntennaHeightM) || !RICON_VegetationRun.Close(a.m_fCanopyHeightM, b.m_fCanopyHeightM))
			return false;
		if (!RICON_VegetationRun.Close(a.m_fHdifM, b.m_fHdifM) || !RICON_VegetationRun.Close(a.m_fThetaDeg, b.m_fThetaDeg))
			return false;
		return RICON_VegetationRun.Close(a.m_fNu, b.m_fNu) && RICON_VegetationRun.Close(a.m_fJDb, b.m_fJDb)
			&& RICON_VegetationRun.Close(a.m_fAhDb, b.m_fAhDb);
	}

	static bool SameResult(RICON_OvercanopyResult a, RICON_OvercanopyResult b)
	{
		if (!a || !b || a.m_eCode != b.m_eCode || !a.m_aTerminals || !b.m_aTerminals)
			return false;
		if (a.m_sModel != b.m_sModel || a.m_sStatus != b.m_sStatus || a.m_sBudgetMode != b.m_sBudgetMode || a.m_bIntegrationClosed != b.m_bIntegrationClosed)
			return false;
		if (!RICON_VegetationRun.Close(a.m_fTotalAhDb, b.m_fTotalAhDb) || a.m_aTerminals.Count() != b.m_aTerminals.Count())
			return false;
		for (int i = 0; i < a.m_aTerminals.Count(); i++)
		{
			if (!SameTerminal(a.m_aTerminals[i], b.m_aTerminals[i]))
				return false;
		}
		return true;
	}

	protected static void Capture(RICON_OvercanopyInput input, out string text, notnull array<float> floats)
	{
		RICON_VegetationRun.Capture(input, text, floats);
		text += "|canopy:" + input.m_aCanopy.Count().ToString();
		foreach (RICON_CanopyEntry entry : input.m_aCanopy)
		{
			text += "|" + entry.m_sMaterialId + "|" + entry.m_sProvenance + "|" + entry.m_bHasHeight.ToString();
			floats.Insert(entry.m_fHeightM);
		}
	}

	protected void Reject(bool condition)
	{
		Require(!condition, "искажённый результат отвергнут");
		if (!condition)
			m_iControls++;
	}

	protected void Controls(RICON_OvercanopyResult a, RICON_OvercanopyResult b)
	{
		Reject(SameResult(null, b));
		a.m_eCode = RICON_EOvercanopyCode.NO_VEGETATION;
		Reject(SameResult(a, b));
		a.m_eCode = b.m_eCode;
		a.m_sModel = "M5";
		Reject(SameResult(a, b));
		a.m_sModel = b.m_sModel;
		a.m_sStatus = "ASSUMED_EXTENSION";
		Reject(SameResult(a, b));
		a.m_sStatus = b.m_sStatus;
		a.m_sBudgetMode = "ADD_TO_THROUGH";
		Reject(SameResult(a, b));
		a.m_sBudgetMode = b.m_sBudgetMode;
		a.m_bIntegrationClosed = true;
		Reject(SameResult(a, b));
		a.m_bIntegrationClosed = false;
		a.m_fTotalAhDb += 1;
		Reject(SameResult(a, b));
		a.m_fTotalAhDb = b.m_fTotalAhDb;
		RICON_OvercanopyTerminal t = a.m_aTerminals[0];
		RICON_OvercanopyTerminal e = b.m_aTerminals[0];
		a.m_aTerminals.Clear();
		Reject(SameResult(a, b));
		a.m_aTerminals.Insert(t);
		t.m_sEnd = "RX";
		Reject(SameResult(a, b));
		t.m_sEnd = e.m_sEnd;
		t.m_sMaterialId = "BAD";
		Reject(SameResult(a, b));
		t.m_sMaterialId = e.m_sMaterialId;
		t.m_sCanopyProvenance = "BAD";
		Reject(SameResult(a, b));
		t.m_sCanopyProvenance = e.m_sCanopyProvenance;
		t.m_fIntervalStartM += 1;
		Reject(SameResult(a, b));
		t.m_fIntervalStartM = e.m_fIntervalStartM;
		t.m_fIntervalEndM += 1;
		Reject(SameResult(a, b));
		t.m_fIntervalEndM = e.m_fIntervalEndM;
		t.m_fAntennaHeightM += 1;
		Reject(SameResult(a, b));
		t.m_fAntennaHeightM = e.m_fAntennaHeightM;
		t.m_fCanopyHeightM += 1;
		Reject(SameResult(a, b));
		t.m_fCanopyHeightM = e.m_fCanopyHeightM;
		t.m_fHdifM += 1;
		Reject(SameResult(a, b));
		t.m_fHdifM = e.m_fHdifM;
		t.m_fThetaDeg += 1;
		Reject(SameResult(a, b));
		t.m_fThetaDeg = e.m_fThetaDeg;
		t.m_fNu += 1;
		Reject(SameResult(a, b));
		t.m_fNu = e.m_fNu;
		t.m_fJDb += 1;
		Reject(SameResult(a, b));
		t.m_fJDb = e.m_fJDb;
		t.m_fAhDb += 1;
		Reject(SameResult(a, b));
		t.m_fAhDb = e.m_fAhDb;
		Require(SameResult(a, b), "после контролей результат восстановлен");
	}

	protected static string Text(float x)
	{
		if (RICON_Num.IsFinite(x) && Math.AbsFloat(x) < 1000000)
			return x.ToString(-1, 9);
		return "out_of_print_range";
	}

	void CheckCase(string id, RICON_OvercanopyInput input, RICON_OvercanopyResult expected)
	{
		m_iCases++;
		string before;
		array<float> values = {};
		Capture(input, before, values);
		RICON_OvercanopyResult first = RICON_OvercanopyLayer.EvaluateCanopy(input);
		bool match = SameResult(first, expected);
		Require(match, id + ": все поля результата совпали");
		if (match)
			m_iMatches++;
		first.m_fTotalAhDb = -999;
		if (!first.m_aTerminals.IsEmpty())
			first.m_aTerminals[0].m_sMaterialId = "corrupt";
		RICON_OvercanopyResult actual = RICON_OvercanopyLayer.EvaluateCanopy(input);
		string after;
		array<float> afterValues = {};
		Capture(input, after, afterValues);
		bool immutable = before == after && RICON_ChannelCutoffRun.SameFloats(values, afterValues) && first != actual;
		immutable = immutable && first.m_aTerminals != actual.m_aTerminals && SameResult(actual, expected);
		Require(immutable, id + ": вход и вложенные результаты не разделяются между вызовами");
		if (immutable)
			m_iImmutable++;
		if (id == "O01" && match && immutable)
		{
			Controls(actual, expected);
			m_Base = input;
			m_BaseExpected = expected;
		}
		m_aInputs.Insert(input);
		m_aExpected.Insert(expected);
		PrintFormat("[RICON_CANOPY] id=%1 code=%2 total=%3 terminals=%4 match=%5 immutable=%6", id,
			typename.EnumToString(RICON_EOvercanopyCode, actual.m_eCode), Text(actual.m_fTotalAhDb), actual.m_aTerminals.Count(), match, immutable);
		for (int i = 0; i < actual.m_aTerminals.Count(); i++)
		{
			RICON_OvercanopyTerminal t = actual.m_aTerminals[i];
			PrintFormat("[RICON_CANOPY_GEOMETRY] id=%1 index=%2 end=%3 material=%4 provenance=%5 start=%6 stop=%7 antenna=%8 canopy=%9",
				id, i, t.m_sEnd, t.m_sMaterialId, t.m_sCanopyProvenance, Text(t.m_fIntervalStartM), Text(t.m_fIntervalEndM), Text(t.m_fAntennaHeightM), Text(t.m_fCanopyHeightM));
			PrintFormat("[RICON_CANOPY_M6] id=%1 index=%2 hdif=%3 theta=%4 nu=%5 j=%6 ah=%7", id, i,
				Text(t.m_fHdifM), Text(t.m_fThetaDeg), Text(t.m_fNu), Text(t.m_fJDb), Text(t.m_fAhDb));
		}
	}

	void CheckExtras()
	{
		bool ok = RICON_OvercanopyLayer.EvaluateCanopy(null).m_eCode == RICON_EOvercanopyCode.INVALID_OBSERVATION;
		RICON_PathObservation observation = m_Base.m_Observation;
		m_Base.m_Observation = null;
		ok = ok && RICON_OvercanopyLayer.EvaluateCanopy(m_Base).m_eCode == RICON_EOvercanopyCode.INVALID_OBSERVATION;
		m_Base.m_Observation = observation;
		array<ref RICON_VegetationProfile> catalog = m_Base.m_aCatalog;
		m_Base.m_aCatalog = null;
		ok = ok && RICON_OvercanopyLayer.EvaluateCanopy(m_Base).m_eCode == RICON_EOvercanopyCode.INVALID_INPUT;
		m_Base.m_aCatalog = catalog;
		array<ref RICON_CanopyEntry> crowns = m_Base.m_aCanopy;
		m_Base.m_aCanopy = null;
		ok = ok && RICON_OvercanopyLayer.EvaluateCanopy(m_Base).m_eCode == RICON_EOvercanopyCode.INVALID_INPUT;
		m_Base.m_aCanopy = crowns;
		crowns.Insert(null);
		ok = ok && RICON_OvercanopyLayer.EvaluateCanopy(m_Base).m_eCode == RICON_EOvercanopyCode.INVALID_INPUT;
		crowns.Remove(crowns.Count() - 1);
		Require(ok, "null-вход/наблюдение/каталог/кроны/запись отвергнуты");
		if (ok)
			m_iExtras++;
		RICON_VegetationProfile unused = catalog[2];
		string density = unused.m_sDensity;
		unused.m_sDensity = "BAD";
		ok = RICON_OvercanopyLayer.EvaluateCanopy(m_Base).m_eCode == RICON_EOvercanopyCode.INVALID_INPUT;
		unused.m_sDensity = density;
		RICON_CanopyEntry unusedCrown = crowns[1];
		float unusedHeight = unusedCrown.m_fHeightM;
		unusedCrown.m_fHeightM = -1;
		ok = ok && RICON_OvercanopyLayer.EvaluateCanopy(m_Base).m_eCode == RICON_EOvercanopyCode.INVALID_INPUT;
		unusedCrown.m_fHeightM = unusedHeight;
		crowns.Insert(crowns[0]);
		ok = ok && RICON_OvercanopyLayer.EvaluateCanopy(m_Base).m_eCode == RICON_EOvercanopyCode.INVALID_INPUT;
		crowns.Remove(crowns.Count() - 1);
		Require(ok && SameResult(RICON_OvercanopyLayer.EvaluateCanopy(m_Base), m_BaseExpected), "весь каталог проверен, дубликаты отвергнуты, вход восстановлен");
		if (ok)
			m_iExtras++;
		RICON_CanopyEntry canopy = crowns[0];
		float height = canopy.m_fHeightM;
		string provenance = canopy.m_sProvenance;
		bool hasHeight = canopy.m_bHasHeight;
		RICON_PathSegment segment = observation.m_aSegments[0];
		float stop = segment.m_fEndM;
		segment.m_fEndM = 2000;
		canopy.m_bHasHeight = true;
		canopy.m_fHeightM = 300000000000000000000000000000000000000.0;
		canopy.m_sProvenance = "MEASURED";
		m_Base.m_bHasRxHeight = false;
		ok = RICON_OvercanopyLayer.EvaluateCanopy(m_Base).m_eCode == RICON_EOvercanopyCode.OUT_OF_MODEL_DOMAIN;
		m_Base.m_aCanopy = {};
		ok = ok && RICON_OvercanopyLayer.EvaluateCanopy(m_Base).m_eCode == RICON_EOvercanopyCode.INVALID_INPUT;
		m_Base.m_aCanopy = crowns;
		m_Base.m_bHasRxHeight = true;
		segment.m_fEndM = stop;
		canopy.m_bHasHeight = hasHeight;
		canopy.m_fHeightM = height;
		canopy.m_sProvenance = provenance;
		Require(ok && SameResult(RICON_OvercanopyLayer.EvaluateCanopy(m_Base), m_BaseExpected), "missing canopy перед OUT_OF_DOMAIN, домен перед NUMERIC, вход восстановлен");
		if (ok)
			m_iExtras++;
		ok = true;
		float originalHeight = m_Base.m_fTxHeightM;
		float previous = 1000;
		array<float> heights = {0, 2, 10, 14.9, 15, 20};
		foreach (float antenna : heights)
		{
			m_Base.m_fTxHeightM = antenna;
			RICON_OvercanopyResult result = RICON_OvercanopyLayer.EvaluateCanopy(m_Base);
			ok = ok && result.m_eCode == RICON_EOvercanopyCode.OK && result.m_fTotalAhDb <= previous && result.m_fTotalAhDb >= 0;
			previous = result.m_fTotalAhDb;
		}
		m_Base.m_fTxHeightM = originalHeight;
		Require(ok && SameResult(RICON_OvercanopyLayer.EvaluateCanopy(m_Base), m_BaseExpected), "подъём антенны не повышает поправку; выше крон ноль; вход восстановлен");
		if (ok)
			m_iExtras++;
		ok = true;
		for (int i = m_aInputs.Count() - 1; i >= 0; i--)
			ok = ok && SameResult(RICON_OvercanopyLayer.EvaluateCanopy(m_aInputs[i]), m_aExpected[i]);
		Require(ok, "обратный порядок всех случаев сохраняет результаты");
		if (ok)
			m_iExtras++;
	}
}
