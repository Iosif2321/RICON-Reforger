//! TEST_ONLY: P4.3, полный результат/ветви, неизменность и контроль сравнения.
class RICON_VegetationPathRun
{
	int m_iCases;
	int m_iMatches;
	int m_iImmutable;
	int m_iControls;
	int m_iExtras;
	bool m_bOk = true;
	ref array<ref RICON_VegetationPathInput> m_aInputs = {};
	ref array<ref RICON_VegetationPathResult> m_aExpected = {};
	RICON_VegetationPathInput m_Base;
	RICON_VegetationPathResult m_BaseExpected;

	protected void Require(bool condition, string why)
	{
		m_bOk = RICON_TestSupport.Expect(condition, "vegetation path", why) && m_bOk;
	}

	static bool SameBranch(RICON_VegetationPathBranch a, RICON_VegetationPathBranch b)
	{
		if (!a || !b)
			return a == b;
		if (a.m_bPresent != b.m_bPresent || a.m_sReason != b.m_sReason || a.m_bClosed != b.m_bClosed || a.m_sBudgetMode != b.m_sBudgetMode)
			return false;
		return RICON_VegetationRun.Close(a.m_fBaseDb, b.m_fBaseDb) && RICON_VegetationRun.Close(a.m_fAddedDb, b.m_fAddedDb)
			&& RICON_VegetationRun.Close(a.m_fTotalDb, b.m_fTotalDb);
	}

	static bool SameResult(RICON_VegetationPathResult a, RICON_VegetationPathResult b)
	{
		if (!a || !b || a.m_eCode != b.m_eCode || a.m_sReason != b.m_sReason || a.m_bHasLoss != b.m_bHasLoss)
			return false;
		if (a.m_sSelected != b.m_sSelected || a.m_sStatus != b.m_sStatus || !RICON_VegetationRun.Close(a.m_fTotalLossDb, b.m_fTotalLossDb))
			return false;
		return SameBranch(a.m_Through, b.m_Through) && SameBranch(a.m_Over, b.m_Over);
	}

	protected static void Capture(RICON_VegetationPathInput input, out string text, notnull array<float> floats)
	{
		RICON_OvercanopyRun.Capture(input.m_Input, text, floats);
		RICON_VegetationPathBudget b = input.m_Budget;
		text += "|budget:" + b.m_bHasL2.ToString() + "|" + b.m_bHasL2Over.ToString();
		floats.Insert(b.m_fFsplDb);
		floats.Insert(b.m_fL2Db);
		floats.Insert(b.m_fFsplOverDb);
		floats.Insert(b.m_fL2OverDb);
	}

	protected void Reject(bool accepted)
	{
		Require(!accepted, "контрольная порча отвергнута");
		if (!accepted)
			m_iControls++;
	}

	protected void Controls(RICON_VegetationPathResult a, RICON_VegetationPathResult b)
	{
		Reject(SameResult(null, b));
		a.m_eCode = RICON_EVegetationPathCode.UNRESOLVED;
		Reject(SameResult(a, b));
		a.m_eCode = b.m_eCode;
		a.m_sReason = "BAD";
		Reject(SameResult(a, b));
		a.m_sReason = b.m_sReason;
		a.m_bHasLoss = false;
		Reject(SameResult(a, b));
		a.m_bHasLoss = true;
		a.m_fTotalLossDb += 1;
		Reject(SameResult(a, b));
		a.m_fTotalLossDb = b.m_fTotalLossDb;
		a.m_sSelected = "OVER";
		Reject(SameResult(a, b));
		a.m_sSelected = b.m_sSelected;
		a.m_sStatus = "OPEN";
		Reject(SameResult(a, b));
		a.m_sStatus = b.m_sStatus;
		RICON_VegetationPathBranch through = a.m_Through;
		RICON_VegetationPathBranch over = a.m_Over;
		a.m_Through = null;
		Reject(SameResult(a, b));
		a.m_Through = through;
		a.m_Over = null;
		Reject(SameResult(a, b));
		a.m_Over = over;
		array<ref RICON_VegetationPathBranch> actuals = {through, over};
		array<ref RICON_VegetationPathBranch> expecteds = {b.m_Through, b.m_Over};
		for (int i = 0; i < 2; i++)
		{
			RICON_VegetationPathBranch r = actuals[i];
			RICON_VegetationPathBranch e = expecteds[i];
			r.m_bPresent = false;
			Reject(SameResult(a, b));
			r.m_bPresent = true;
			r.m_sReason = "BAD";
			Reject(SameResult(a, b));
			r.m_sReason = e.m_sReason;
			r.m_fBaseDb += 1;
			Reject(SameResult(a, b));
			r.m_fBaseDb = e.m_fBaseDb;
			r.m_fAddedDb += 1;
			Reject(SameResult(a, b));
			r.m_fAddedDb = e.m_fAddedDb;
			r.m_fTotalDb += 1;
			Reject(SameResult(a, b));
			r.m_fTotalDb = e.m_fTotalDb;
			r.m_bClosed = false;
			Reject(SameResult(a, b));
			r.m_bClosed = true;
			r.m_sBudgetMode = "BAD";
			Reject(SameResult(a, b));
			r.m_sBudgetMode = e.m_sBudgetMode;
		}
		Require(SameResult(a, b), "после порч результат восстановлен");
	}

	protected static string Text(float x)
	{
		if (RICON_Num.IsFinite(x) && Math.AbsFloat(x) < 1000000)
			return x.ToString(-1, 9);
		return "out_of_print_range";
	}

	protected static string Label(string s)
	{
		if (s.IsEmpty())
			return "NONE";
		return s;
	}

	protected static void PrintBranch(string id, string name, RICON_VegetationPathBranch branch)
	{
		if (!branch)
			return;
		PrintFormat("[RICON_VPATH_BRANCH] id=%1 branch=%2 present=%3 reason=%4 base=%5 added=%6 total=%7 closed=%8 mode=%9", id,
			name, branch.m_bPresent, Label(branch.m_sReason), Text(branch.m_fBaseDb), Text(branch.m_fAddedDb), Text(branch.m_fTotalDb), branch.m_bClosed, Label(branch.m_sBudgetMode));
	}

	void CheckCase(string id, RICON_VegetationPathInput input, RICON_VegetationPathResult expected)
	{
		m_iCases++;
		string before;
		array<float> values = {};
		Capture(input, before, values);
		RICON_VegetationPathResult first = RICON_VegetationPathLayer.Evaluate(input);
		bool match = SameResult(first, expected);
		Require(match, id + ": все поля результата совпали");
		if (match)
			m_iMatches++;
		first.m_fTotalLossDb = -999;
		if (first.m_Through)
			first.m_Through.m_sReason = "corrupt";
		if (first.m_Over)
			first.m_Over.m_sReason = "corrupt";
		RICON_VegetationPathResult actual = RICON_VegetationPathLayer.Evaluate(input);
		string after;
		array<float> afterValues = {};
		Capture(input, after, afterValues);
		bool immutable = before == after && RICON_ChannelCutoffRun.SameFloats(values, afterValues) && first != actual;
		immutable = immutable && (!first.m_Through || first.m_Through != actual.m_Through) && (!first.m_Over || first.m_Over != actual.m_Over);
		immutable = immutable && SameResult(actual, expected);
		Require(immutable, id + ": вход неизменен, результаты и ветви новые");
		if (immutable)
			m_iImmutable++;
		if (id == "P01" && match && immutable)
		{
			Controls(actual, expected);
			m_Base = input;
			m_BaseExpected = expected;
		}
		m_aInputs.Insert(input);
		m_aExpected.Insert(expected);
		PrintFormat("[RICON_VPATH] id=%1 code=%2 hasLoss=%3 loss=%4 selected=%5 status=%6 reason=%7 match=%8 immutable=%9", id,
			typename.EnumToString(RICON_EVegetationPathCode, actual.m_eCode), actual.m_bHasLoss, Text(actual.m_fTotalLossDb), Label(actual.m_sSelected),
			Label(actual.m_sStatus), Label(actual.m_sReason), match, immutable);
		PrintBranch(id, "THROUGH", actual.m_Through);
		PrintBranch(id, "OVER", actual.m_Over);
	}

	void CheckExtras()
	{
		bool ok = RICON_VegetationPathLayer.Evaluate(null).m_eCode == RICON_EVegetationPathCode.INVALID_INPUT;
		RICON_OvercanopyInput common = m_Base.m_Input;
		m_Base.m_Input = null;
		ok = ok && RICON_VegetationPathLayer.Evaluate(m_Base).m_eCode == RICON_EVegetationPathCode.INVALID_OBSERVATION;
		m_Base.m_Input = common;
		RICON_VegetationPathBudget budget = m_Base.m_Budget;
		m_Base.m_Budget = null;
		ok = ok && RICON_VegetationPathLayer.Evaluate(m_Base).m_eCode == RICON_EVegetationPathCode.INVALID_INPUT;
		m_Base.m_Budget = budget;
		Require(ok && SameResult(RICON_VegetationPathLayer.Evaluate(m_Base), m_BaseExpected), "null-ссылки отвергнуты; вход восстановлен");
		if (ok)
			m_iExtras++;
		float frequency = common.m_Observation.m_fFrequencyMHz;
		common.m_Observation.m_fFrequencyMHz = 0;
		m_Base.m_Budget = null;
		ok = RICON_VegetationPathLayer.Evaluate(m_Base).m_eCode == RICON_EVegetationPathCode.INVALID_OBSERVATION;
		common.m_Observation.m_fFrequencyMHz = frequency;
		m_Base.m_Budget = budget;
		RICON_PathSegment segment = common.m_Observation.m_aSegments[0];
		RICON_EPathKnowledge knowledge = segment.m_eKnowledge;
		string material = segment.m_sMaterialId;
		RICON_CanopyEntry canopy = common.m_aCanopy[0];
		string provenance = canopy.m_sProvenance;
		segment.m_eKnowledge = RICON_EPathKnowledge.UNKNOWN;
		segment.m_sMaterialId = "";
		canopy.m_sProvenance = "BAD";
		ok = ok && RICON_VegetationPathLayer.Evaluate(m_Base).m_eCode == RICON_EVegetationPathCode.VEGETATION_UNKNOWN;
		segment.m_eKnowledge = knowledge;
		segment.m_sMaterialId = material;
		canopy.m_sProvenance = provenance;
		Require(ok && SameResult(RICON_VegetationPathLayer.Evaluate(m_Base), m_BaseExpected), "порядок причин сохраняется, вход восстановлен");
		if (ok)
			m_iExtras++;
		ok = true;
		for (int i = m_aInputs.Count() - 1; i >= 0; i--)
			ok = ok && SameResult(RICON_VegetationPathLayer.Evaluate(m_aInputs[i]), m_aExpected[i]);
		Require(ok, "повтор в обратном порядке сохраняет все результаты");
		if (ok)
			m_iExtras++;
	}
}
