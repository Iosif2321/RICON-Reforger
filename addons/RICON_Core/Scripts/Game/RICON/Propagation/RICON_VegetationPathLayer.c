//! P4.3: выбор одного полного бюджета, REQ-vegetation-path v0.4. Не строгая граница P1 и не голосовой gate.
enum RICON_EVegetationPathCode
{
	INVALID_INPUT = 0,
	INVALID_OBSERVATION = 1,
	VEGETATION_COVERAGE_INCOMPLETE = 2,
	VEGETATION_UNKNOWN = 3,
	VEGETATION_MIXED_PROFILE = 4,
	NUMERIC_FAILURE = 5,
	NO_VEGETATION = 6,
	UNRESOLVED = 7,
	OK = 8
}

class RICON_VegetationPathBudget
{
	float m_fFsplDb;
	bool m_bHasL2;
	float m_fL2Db;
	float m_fFsplOverDb;
	bool m_bHasL2Over;
	float m_fL2OverDb;
}

class RICON_VegetationPathInput
{
	ref RICON_OvercanopyInput m_Input;
	ref RICON_VegetationPathBudget m_Budget;
}

class RICON_VegetationPathBranch
{
	bool m_bPresent;
	string m_sReason;
	float m_fBaseDb;
	float m_fAddedDb;
	float m_fTotalDb;
	bool m_bClosed;
	string m_sBudgetMode;
}

class RICON_VegetationPathResult
{
	RICON_EVegetationPathCode m_eCode;
	string m_sReason;
	bool m_bHasLoss;
	float m_fTotalLossDb;
	string m_sSelected;
	string m_sStatus;
	ref RICON_VegetationPathBranch m_Through;
	ref RICON_VegetationPathBranch m_Over;
}

class RICON_VegetationPathLayer
{
	protected static bool Nonnegative(float x)
	{
		return RICON_Num.IsFinite(x) && x >= 0;
	}

	protected static RICON_VegetationPathResult Make(RICON_EVegetationPathCode code)
	{
		RICON_VegetationPathResult r = new RICON_VegetationPathResult();
		r.m_eCode = code;
		return r;
	}

	//! Только общие причины OP/VP07; неприменимость модели остаётся причиной отсутствующей ветви.
	protected static RICON_VegetationPathResult CommonError(string name)
	{
		if (name == "INVALID_OBSERVATION")
			return Make(RICON_EVegetationPathCode.INVALID_OBSERVATION);
		if (name == "INVALID_INPUT")
			return Make(RICON_EVegetationPathCode.INVALID_INPUT);
		if (name == "VEGETATION_COVERAGE_INCOMPLETE")
			return Make(RICON_EVegetationPathCode.VEGETATION_COVERAGE_INCOMPLETE);
		if (name == "VEGETATION_UNKNOWN")
			return Make(RICON_EVegetationPathCode.VEGETATION_UNKNOWN);
		if (name == "VEGETATION_MIXED_PROFILE")
			return Make(RICON_EVegetationPathCode.VEGETATION_MIXED_PROFILE);
		return null;
	}

	protected static RICON_VegetationPathBranch Absent(string reason)
	{
		RICON_VegetationPathBranch branch = new RICON_VegetationPathBranch();
		branch.m_sReason = reason;
		return branch;
	}

	protected static bool ValidBudget(RICON_VegetationPathBudget b)
	{
		if (!b || !Nonnegative(b.m_fFsplDb) || b.m_fFsplDb == 0 || !Nonnegative(b.m_fFsplOverDb) || b.m_fFsplOverDb == 0)
			return false;
		return (!b.m_bHasL2 || Nonnegative(b.m_fL2Db)) && (!b.m_bHasL2Over || Nonnegative(b.m_fL2OverDb));
	}

	//! На уже проверенном входе: объединение только ненезначительной растительности, без новой RF-математики.
	protected static bool MidPath(RICON_OvercanopyInput input)
	{
		float length;
		array<ref RICON_WaterInterval> water;
		RICON_PathObservationModel.Validate(input.m_Observation, length, water);
		map<string, ref RICON_VegetationProfile> catalog = new map<string, ref RICON_VegetationProfile>();
		foreach (RICON_VegetationProfile p : input.m_aCatalog)
			catalog.Set(p.m_sMaterialId, p);
		bool have;
		float start;
		float stop;
		foreach (RICON_PathSegment segment : input.m_Observation.m_aSegments)
		{
			if (segment.m_eFamily != RICON_EPathFamily.VEGETATION || catalog.Get(segment.m_sMaterialId).m_bNegligible)
				continue;
			if (have && segment.m_fStartM <= stop)
			{
				stop = Math.Max(stop, segment.m_fEndM);
				continue;
			}
			if (have && start > 0 && stop < length)
				return true;
			start = segment.m_fStartM;
			stop = segment.m_fEndM;
			have = true;
		}
		return have && start > 0 && stop < length;
	}

	static RICON_VegetationPathResult Evaluate(RICON_VegetationPathInput input)
	{
		if (!input)
			return Make(RICON_EVegetationPathCode.INVALID_INPUT);
		RICON_VegetationResult throughResult = RICON_VegetationLayer.Evaluate(input.m_Input);
		RICON_OvercanopyResult overResult = RICON_OvercanopyLayer.EvaluateCanopy(input.m_Input);
		string throughCode = typename.EnumToString(RICON_EVegetationCode, throughResult.m_eCode);
		string overCode = typename.EnumToString(RICON_EOvercanopyCode, overResult.m_eCode);
		RICON_VegetationPathResult error = CommonError(throughCode);
		if (error)
			return error;
		error = CommonError(overCode);
		if (error)
			return error;
		RICON_VegetationPathBudget b = input.m_Budget;
		if (!ValidBudget(b))
			return Make(RICON_EVegetationPathCode.INVALID_INPUT);
		if (throughResult.m_eCode == RICON_EVegetationCode.NUMERIC_FAILURE || overResult.m_eCode == RICON_EOvercanopyCode.NUMERIC_FAILURE)
			return Make(RICON_EVegetationPathCode.NUMERIC_FAILURE);
		RICON_VegetationPathResult r;
		if (throughResult.m_eCode == RICON_EVegetationCode.NO_VEGETATION)
		{
			if (!b.m_bHasL2)
			{
				r = Make(RICON_EVegetationPathCode.UNRESOLVED);
				r.m_sReason = "BASE_BUDGET_MISSING";
				return r;
			}
			float plain = b.m_fFsplDb + b.m_fL2Db;
			if (!Nonnegative(plain))
				return Make(RICON_EVegetationPathCode.NUMERIC_FAILURE);
			r = Make(RICON_EVegetationPathCode.NO_VEGETATION);
			r.m_bHasLoss = true;
			r.m_fTotalLossDb = plain;
			return r;
		}
		RICON_VegetationPathBranch through = Absent(throughCode);
		if (throughResult.m_eCode == RICON_EVegetationCode.OK)
		{
			bool jb;
			foreach (RICON_VegetationInterval interval : throughResult.m_aIntervals)
				jb = jb || interval.m_sExcessBaseline == "FREE_SPACE";
			if (!jb && !b.m_bHasL2)
				through = Absent("BASE_BUDGET_MISSING");
			else
			{
				through = new RICON_VegetationPathBranch();
				through.m_bPresent = true;
				through.m_fBaseDb = b.m_fFsplDb;
				through.m_sBudgetMode = "FREE_SPACE_PLUS_JB";
				if (!jb)
				{
					through.m_fBaseDb = b.m_fFsplDb + b.m_fL2Db;
					through.m_sBudgetMode = "BASE_PLUS_EXCESS";
				}
				through.m_fAddedDb = throughResult.m_fLossDb;
				through.m_fTotalDb = through.m_fBaseDb + through.m_fAddedDb;
				through.m_bClosed = throughResult.m_bAllScenariosClosed;
			}
		}
		RICON_VegetationPathBranch over = Absent(overCode);
		if (MidPath(input.m_Input))
			over = Absent("MID_PATH_VEGETATION");
		else if (overResult.m_eCode == RICON_EOvercanopyCode.OK)
		{
			if (!b.m_bHasL2Over)
				over = Absent("BASE_BUDGET_MISSING");
			else
			{
				over = new RICON_VegetationPathBranch();
				over.m_bPresent = true;
				over.m_fBaseDb = b.m_fFsplOverDb + b.m_fL2OverDb;
				over.m_fAddedDb = overResult.m_fTotalAhDb;
				over.m_fTotalDb = over.m_fBaseDb + over.m_fAddedDb;
				over.m_bClosed = overResult.m_sStatus == "SOURCE_DOMAIN";
			}
		}
		if ((through.m_bPresent && (!Nonnegative(through.m_fBaseDb) || !Nonnegative(through.m_fTotalDb)))
			|| (over.m_bPresent && (!Nonnegative(over.m_fBaseDb) || !Nonnegative(over.m_fTotalDb))))
			return Make(RICON_EVegetationPathCode.NUMERIC_FAILURE);
		r = Make(RICON_EVegetationPathCode.OK);
		r.m_Through = through;
		r.m_Over = over;
		if (!through.m_bPresent && !over.m_bPresent)
		{
			r.m_eCode = RICON_EVegetationPathCode.UNRESOLVED;
			r.m_sReason = "NO_BRANCH";
			return r;
		}
		r.m_bHasLoss = true;
		float difference = through.m_fTotalDb - over.m_fTotalDb;
		if (through.m_bPresent && (!over.m_bPresent || difference <= 0.001))
		{
			r.m_sSelected = "THROUGH";
			r.m_fTotalLossDb = through.m_fTotalDb;
		}
		else
		{
			r.m_sSelected = "OVER";
			r.m_fTotalLossDb = over.m_fTotalDb;
		}
		r.m_sStatus = "OPEN";
		if (through.m_bClosed && over.m_bClosed)
			r.m_sStatus = "CLOSED";
		else if (over.m_bClosed)
			r.m_sStatus = "OVER_REFERENCE_ONLY";
		return r;
	}
}
