//! TEST_ONLY: доступ к защищённой математике для численной калибровки, без нового production API.
class RICON_VegetationMathProbe : RICON_VegetationLayer
{
	static float SmallRatio()
	{
		RICON_VegetationAm am = new RICON_VegetationAm();
		am.m_fA1 = 340282346638528859811704183484516925440.0;
		am.m_fAlpha = 0;
		return WoodlandLoss(100000000000000000000.0, 0.00000000000000000001, 150, am);
	}
}

//! TEST_ONLY: P4 frozen corpus, глубокая неизменность входа/результатов и контроль сравнения.
class RICON_VegetationRun
{
	int m_iCases;
	int m_iMatches;
	int m_iImmutable;
	int m_iControls;
	int m_iExtras;
	bool m_bOk = true;
	ref array<ref RICON_VegetationInput> m_aInputs = {};
	ref array<ref RICON_VegetationResult> m_aExpected = {};
	RICON_VegetationInput m_Base;
	RICON_VegetationResult m_BaseExpected;

	protected void Require(bool condition, string id, string why)
	{
		m_bOk = RICON_TestSupport.Expect(condition, id, why) && m_bOk;
	}

	static bool Close(float a, float b)
	{
		return RICON_PathObservationRun.Close(a, b);
	}

	static bool SameInterval(RICON_VegetationInterval a, RICON_VegetationInterval b, bool uncovered = false)
	{
		if (!a || !b || a.m_fStartM != b.m_fStartM || a.m_fEndM != b.m_fEndM || a.m_sScenario != b.m_sScenario)
			return false;
		if (uncovered)
			return true;
		if (a.m_sMaterialId != b.m_sMaterialId || a.m_sPrimaryModel != b.m_sPrimaryModel || a.m_sPrimaryStatus != b.m_sPrimaryStatus)
			return false;
		if (a.m_bScenarioClosed != b.m_bScenarioClosed || a.m_sDepthTrend != b.m_sDepthTrend || a.m_sExcessBaseline != b.m_sExcessBaseline)
			return false;
		if (a.m_bHasGamma != b.m_bHasGamma || a.m_sGammaSource != b.m_sGammaSource || a.m_sGammaProvenance != b.m_sGammaProvenance)
			return false;
		if (a.m_sAmProvenance != b.m_sAmProvenance || a.m_bCapApplied != b.m_bCapApplied)
			return false;
		if (!Close(a.m_fPrimaryLossDb, b.m_fPrimaryLossDb) || !Close(a.m_fSpreadMinDb, b.m_fSpreadMinDb)
			|| !Close(a.m_fSpreadMaxDb, b.m_fSpreadMaxDb) || !Close(a.m_fExdDiagnosticDb, b.m_fExdDiagnosticDb)
			|| (a.m_bHasGamma && !Close(a.m_fGamma, b.m_fGamma)) || !a.m_aCandidates || !b.m_aCandidates
			|| a.m_aCandidates.Count() != b.m_aCandidates.Count())
			return false;
		for (int i = 0; i < a.m_aCandidates.Count(); i++)
		{
			RICON_VegetationCandidate ac = a.m_aCandidates[i];
			RICON_VegetationCandidate bc = b.m_aCandidates[i];
			if (!ac || !bc || ac.m_sModel != bc.m_sModel || ac.m_sStatus != bc.m_sStatus || !Close(ac.m_fLossDb, bc.m_fLossDb))
				return false;
		}
		return true;
	}

	static bool SameResult(RICON_VegetationResult a, RICON_VegetationResult b)
	{
		if (!a || !b || a.m_eCode != b.m_eCode || a.m_bHasLoss != b.m_bHasLoss || !a.m_aIntervals || !b.m_aIntervals)
			return false;
		if (a.m_bHasLoss && !Close(a.m_fLossDb, b.m_fLossDb))
			return false;
		if (a.m_eCode == RICON_EVegetationCode.NO_MODEL_FOR_SCENARIO)
			return a.m_aIntervals.IsEmpty() && SameInterval(a.m_Uncovered, b.m_Uncovered, true);
		if (a.m_Uncovered || b.m_Uncovered)
			return false;
		if (a.m_eCode != RICON_EVegetationCode.OK)
			return a.m_aIntervals.IsEmpty();
		if (a.m_bAllScenariosClosed != b.m_bAllScenariosClosed || a.m_aIntervals.Count() != b.m_aIntervals.Count())
			return false;
		for (int i = 0; i < a.m_aIntervals.Count(); i++)
		{
			if (!SameInterval(a.m_aIntervals[i], b.m_aIntervals[i]))
				return false;
		}
		return true;
	}

	protected static void Capture(RICON_VegetationInput input, out string text, notnull array<float> floats)
	{
		RICON_PathObservationRun.Capture(input.m_Observation, null, text, floats);
		text += "|" + input.m_sPolarization + "|" + input.m_bHasTxHeight.ToString() + "|" + input.m_bHasRxHeight.ToString();
		floats.Insert(input.m_fTxHeightM);
		floats.Insert(input.m_fRxHeightM);
		text += "|catalog:" + input.m_aCatalog.Count().ToString();
		foreach (RICON_VegetationProfile p : input.m_aCatalog)
		{
			text += "|" + p.m_sMaterialId + "|" + p.m_bNegligible.ToString() + "|" + p.m_sClimate + "|" + p.m_sLeafState;
			text += "|" + p.m_sDensity + "|" + p.m_sMoisture + "|" + p.m_sGammaProvenance + "|" + p.m_aTables.Count().ToString();
			foreach (RICON_VegetationTable table : p.m_aTables)
			{
				text += "|" + table.m_sPolarization + "|" + table.m_aPoints.Count().ToString();
				foreach (RICON_VegetationPoint point : table.m_aPoints)
				{
					floats.Insert(point.m_fFrequencyMHz);
					floats.Insert(point.m_fGamma);
				}
			}
			if (!p.m_Am)
				text += "|noAm";
			else
			{
				text += "|" + p.m_Am.m_sProvenance;
				floats.Insert(p.m_Am.m_fA1);
				floats.Insert(p.m_Am.m_fAlpha);
				floats.Insert(p.m_Am.m_fMinMHz);
				floats.Insert(p.m_Am.m_fMaxMHz);
			}
		}
	}

	protected void Reject(bool condition)
	{
		Require(!condition, "vegetation controls", "искажённый результат отвергнут");
		if (!condition)
			m_iControls++;
	}

	protected void Controls(RICON_VegetationResult a, RICON_VegetationResult b)
	{
		Reject(SameResult(null, b));
		a.m_eCode = RICON_EVegetationCode.NO_VEGETATION;
		Reject(SameResult(a, b));
		a.m_eCode = b.m_eCode;
		a.m_fLossDb += 1;
		Reject(SameResult(a, b));
		a.m_fLossDb = b.m_fLossDb;
		a.m_bAllScenariosClosed = false;
		Reject(SameResult(a, b));
		a.m_bAllScenariosClosed = true;
		RICON_VegetationInterval r = a.m_aIntervals[0];
		RICON_VegetationInterval e = b.m_aIntervals[0];
		r.m_sPrimaryModel = "M5";
		Reject(SameResult(a, b));
		r.m_sPrimaryModel = e.m_sPrimaryModel;
		r.m_sPrimaryStatus = "ASSUMED_EXTENSION";
		Reject(SameResult(a, b));
		r.m_sPrimaryStatus = e.m_sPrimaryStatus;
		r.m_bScenarioClosed = false;
		Reject(SameResult(a, b));
		r.m_bScenarioClosed = true;
		r.m_bHasGamma = false;
		Reject(SameResult(a, b));
		r.m_bHasGamma = true;
		r.m_bCapApplied = true;
		Reject(SameResult(a, b));
		r.m_bCapApplied = false;
		RICON_VegetationCandidate candidate = r.m_aCandidates[0];
		string candidateStatus = candidate.m_sStatus;
		float candidateLoss = candidate.m_fLossDb;
		candidate.m_sStatus = "ASSUMED_EXTENSION";
		Reject(SameResult(a, b));
		candidate.m_sStatus = candidateStatus;
		candidate.m_fLossDb += 1;
		Reject(SameResult(a, b));
		candidate.m_fLossDb = candidateLoss;
		a.m_aIntervals.Clear();
		Reject(SameResult(a, b));
		a.m_aIntervals.Insert(r);
		Require(SameResult(a, b), "vegetation controls", "после порч результат восстановлен");
	}

	protected static string Text(float x)
	{
		if (RICON_Num.IsFinite(x) && Math.AbsFloat(x) < 1000000)
			return x.ToString(-1, 9);
		return "out_of_print_range";
	}

	void CheckCase(string id, RICON_VegetationInput input, RICON_VegetationResult expected)
	{
		m_iCases++;
		string before;
		array<float> values = {};
		Capture(input, before, values);
		RICON_VegetationResult first = RICON_VegetationLayer.Evaluate(input);
		bool match = SameResult(first, expected);
		Require(match, id, "код, интервалы, кандидаты, статусы и все числа совпали");
		if (match)
			m_iMatches++;
		first.m_fLossDb = -999;
		if (!first.m_aIntervals.IsEmpty())
			first.m_aIntervals[0].m_aCandidates[0].m_sModel = "corrupt";
		if (first.m_Uncovered)
			first.m_Uncovered.m_sScenario = "corrupt";
		RICON_VegetationResult actual = RICON_VegetationLayer.Evaluate(input);
		string after;
		array<float> afterValues = {};
		Capture(input, after, afterValues);
		bool immutable = before == after && RICON_ChannelCutoffRun.SameFloats(values, afterValues) && first != actual
			&& first.m_aIntervals != actual.m_aIntervals && SameResult(actual, expected);
		Require(immutable, id, "вход неизменен; результат и вложенные записи не разделяются с предыдущим вызовом");
		if (immutable)
			m_iImmutable++;
		if (id == "V03" && match && immutable)
		{
			Controls(actual, expected);
			m_Base = input;
			m_BaseExpected = expected;
		}
		if (id == "V18" && match)
		{
			RICON_VegetationInterval keep = actual.m_Uncovered;
			actual.m_Uncovered = null;
			Reject(SameResult(actual, expected));
			actual.m_Uncovered = keep;
		}
		m_aInputs.Insert(input);
		m_aExpected.Insert(expected);
		PrintFormat("[RICON_VEG] id=%1 code=%2 hasLoss=%3 loss=%4 intervals=%5 closed=%6 match=%7 immutable=%8", id,
			typename.EnumToString(RICON_EVegetationCode, actual.m_eCode), actual.m_bHasLoss, Text(actual.m_fLossDb),
			actual.m_aIntervals.Count(), actual.m_bAllScenariosClosed, match, immutable);
		if (actual.m_Uncovered)
			PrintFormat("[RICON_VEG_OPEN] id=%1 start=%2 end=%3 scenario=%4", id, Text(actual.m_Uncovered.m_fStartM), Text(actual.m_Uncovered.m_fEndM), actual.m_Uncovered.m_sScenario);
		for (int i = 0; i < actual.m_aIntervals.Count(); i++)
		{
			RICON_VegetationInterval r = actual.m_aIntervals[i];
			PrintFormat("[RICON_VEG_INTERVAL] id=%1 index=%2 start=%3 end=%4 primary=%5 loss=%6 status=%7 closed=%8", id, i,
				Text(r.m_fStartM), Text(r.m_fEndM), r.m_sPrimaryModel, Text(r.m_fPrimaryLossDb), r.m_sPrimaryStatus, r.m_bScenarioClosed);
			for (int j = 0; j < r.m_aCandidates.Count(); j++)
				PrintFormat("[RICON_VEG_CANDIDATE] id=%1 interval=%2 index=%3 model=%4 loss=%5 status=%6", id, i, j,
					r.m_aCandidates[j].m_sModel, Text(r.m_aCandidates[j].m_fLossDb), r.m_aCandidates[j].m_sStatus);
		}
	}

	void CheckExtras()
	{
		bool ok = RICON_VegetationLayer.Evaluate(null).m_eCode == RICON_EVegetationCode.INVALID_OBSERVATION;
		RICON_PathObservation observation = m_Base.m_Observation;
		m_Base.m_Observation = null;
		ok = ok && RICON_VegetationLayer.Evaluate(m_Base).m_eCode == RICON_EVegetationCode.INVALID_OBSERVATION;
		m_Base.m_Observation = observation;
		array<ref RICON_VegetationProfile> catalog = m_Base.m_aCatalog;
		m_Base.m_aCatalog = null;
		ok = ok && RICON_VegetationLayer.Evaluate(m_Base).m_eCode == RICON_EVegetationCode.INVALID_INPUT;
		m_Base.m_aCatalog = catalog;
		Require(ok, "vegetation nulls", "null-вход/наблюдение/каталог не принимаются");
		if (ok)
			m_iExtras++;
		RICON_VegetationProfile unused = catalog[2];
		string density = unused.m_sDensity;
		unused.m_sDensity = "invalid";
		ok = RICON_VegetationLayer.Evaluate(m_Base).m_eCode == RICON_EVegetationCode.INVALID_INPUT;
		unused.m_sDensity = density;
		float maximum = unused.m_Am.m_fMaxMHz;
		unused.m_Am.m_fMaxMHz = unused.m_Am.m_fMinMHz;
		ok = ok && RICON_VegetationLayer.Evaluate(m_Base).m_eCode == RICON_EVegetationCode.INVALID_INPUT;
		unused.m_Am.m_fMaxMHz = maximum;
		catalog.Insert(null);
		ok = ok && RICON_VegetationLayer.Evaluate(m_Base).m_eCode == RICON_EVegetationCode.INVALID_INPUT;
		catalog.Remove(catalog.Count() - 1);
		Require(ok && SameResult(RICON_VegetationLayer.Evaluate(m_Base), m_BaseExpected), "vegetation global catalog", "неиспользуемая запись тоже проверена; вход восстановлен");
		if (ok)
			m_iExtras++;
		float frequency = observation.m_fFrequencyMHz;
		RICON_VegetationAm am = catalog[0].m_Am;
		float a1 = am.m_fA1;
		float alpha = am.m_fAlpha;
		observation.m_fFrequencyMHz = 150;
		am.m_fA1 = 340282346638528859811704183484516925440.0;
		am.m_fAlpha = 1;
		RICON_VegetationResult largeAm = RICON_VegetationLayer.Evaluate(m_Base);
		ok = largeAm.m_eCode == RICON_EVegetationCode.OK && Close(largeAm.m_fLossDb, 2.588558764075543)
			&& largeAm.m_aIntervals.Count() == 1 && largeAm.m_aIntervals[0].m_sPrimaryModel == "M1";
		ok = ok && Close(RICON_VegetationMathProbe.SmallRatio(), 1);
		observation.m_fFrequencyMHz = frequency;
		am.m_fA1 = a1;
		am.m_fAlpha = alpha;
		Require(ok && SameResult(RICON_VegetationLayer.Evaluate(m_Base), m_BaseExpected), "vegetation large Am", "Am больше FLT_MAX, но конечная потеря сохраняется; вход восстановлен");
		if (ok)
			m_iExtras++;
		ok = true;
		for (int i = m_aInputs.Count() - 1; i >= 0; i--)
			ok = ok && SameResult(RICON_VegetationLayer.Evaluate(m_aInputs[i]), m_aExpected[i]);
		Require(ok, "vegetation reverse", "повтор всех случаев в обратном порядке не меняет результаты");
		if (ok)
			m_iExtras++;
	}
}
