//! P4.2: чистая поправка терминала P.2108-1. Полный бюджет над кронами и мир остаются задачей интеграции.
enum RICON_EOvercanopyCode
{
	INVALID_OBSERVATION = 0,
	INVALID_INPUT = 1,
	VEGETATION_COVERAGE_INCOMPLETE = 2,
	VEGETATION_UNKNOWN = 3,
	NO_VEGETATION = 4,
	VEGETATION_MIXED_PROFILE = 5,
	NO_TERMINAL_IN_VEGETATION = 6,
	OUT_OF_MODEL_DOMAIN = 7,
	NUMERIC_FAILURE = 8,
	OK = 9
}

class RICON_CanopyEntry
{
	string m_sMaterialId;
	bool m_bHasHeight;
	float m_fHeightM;
	string m_sProvenance;
}

class RICON_OvercanopyInput : RICON_VegetationInput
{
	ref array<ref RICON_CanopyEntry> m_aCanopy = {};
}

class RICON_OvercanopyTerminal
{
	string m_sEnd;
	float m_fIntervalStartM;
	float m_fIntervalEndM;
	string m_sMaterialId;
	float m_fAntennaHeightM;
	float m_fCanopyHeightM;
	string m_sCanopyProvenance;
	float m_fHdifM;
	float m_fThetaDeg;
	float m_fNu;
	float m_fJDb;
	float m_fAhDb;
}

class RICON_OvercanopyResult
{
	RICON_EOvercanopyCode m_eCode;
	string m_sModel;
	string m_sStatus;
	string m_sBudgetMode;
	bool m_bIntegrationClosed;
	float m_fTotalAhDb;
	ref array<ref RICON_OvercanopyTerminal> m_aTerminals = {};
}

//! Наследование переиспользует проверку профиля P4; ансамбль M1–M5 здесь не вызывается.
class RICON_OvercanopyLayer : RICON_VegetationLayer
{
	protected static RICON_OvercanopyResult NewResult(RICON_EOvercanopyCode code)
	{
		RICON_OvercanopyResult result = new RICON_OvercanopyResult();
		result.m_eCode = code;
		return result;
	}

	protected static bool CalculateTerminal(string end, float antenna, float frequency, RICON_VegetationInterval interval,
		RICON_CanopyEntry canopy, out RICON_OvercanopyTerminal result)
	{
		result = new RICON_OvercanopyTerminal();
		result.m_sEnd = end;
		result.m_fIntervalStartM = interval.m_fStartM;
		result.m_fIntervalEndM = interval.m_fEndM;
		result.m_sMaterialId = interval.m_sMaterialId;
		result.m_fAntennaHeightM = antenna;
		result.m_sCanopyProvenance = canopy.m_sProvenance;
		result.m_fCanopyHeightM = canopy.m_fHeightM;
		if (canopy.m_sProvenance == "STANDARD_DEFAULT")
			result.m_fCanopyHeightM = 15;
		// jDb=0 в этой ветви означает неприменение J, а не значение J(0).
		if (antenna >= result.m_fCanopyHeightM)
			return true;
		result.m_fHdifM = result.m_fCanopyHeightM - antenna;
		result.m_fThetaDeg = Math.Atan2(result.m_fHdifM, 27) * Math.RAD2DEG;
		float product = result.m_fHdifM * result.m_fThetaDeg;
		if (!Nonnegative(product))
			return false;
		result.m_fNu = 0.342 * Math.Sqrt(frequency / 1000.0) * Math.Sqrt(product);
		float x = result.m_fNu - 0.1;
		result.m_fJDb = 6.9 + 20 * Math.Log10(Math.Sqrt(x * x + 1) + x);
		result.m_fAhDb = result.m_fJDb - 6.03;
		return Nonnegative(result.m_fHdifM) && Nonnegative(result.m_fThetaDeg) && Nonnegative(result.m_fNu)
			&& Nonnegative(result.m_fJDb) && Nonnegative(result.m_fAhDb);
	}

	static RICON_OvercanopyResult EvaluateCanopy(RICON_OvercanopyInput input)
	{
		float length;
		array<ref RICON_WaterInterval> water;
		if (!input || !RICON_PathObservationModel.Validate(input.m_Observation, length, water))
			return NewResult(RICON_EOvercanopyCode.INVALID_OBSERVATION);
		if (!input.m_aCatalog || !input.m_aCanopy || (input.m_sPolarization != "V" && input.m_sPolarization != "H"))
			return NewResult(RICON_EOvercanopyCode.INVALID_INPUT);
		if ((input.m_bHasTxHeight && !Nonnegative(input.m_fTxHeightM)) || (input.m_bHasRxHeight && !Nonnegative(input.m_fRxHeightM)))
			return NewResult(RICON_EOvercanopyCode.INVALID_INPUT);
		map<string, ref RICON_VegetationProfile> catalog = new map<string, ref RICON_VegetationProfile>();
		foreach (RICON_VegetationProfile p : input.m_aCatalog)
		{
			if (!ValidProfile(p) || catalog.Contains(p.m_sMaterialId))
				return NewResult(RICON_EOvercanopyCode.INVALID_INPUT);
			catalog.Set(p.m_sMaterialId, p);
		}
		map<string, ref RICON_CanopyEntry> crowns = new map<string, ref RICON_CanopyEntry>();
		foreach (RICON_CanopyEntry entry : input.m_aCanopy)
		{
			if (!entry || !catalog.Contains(entry.m_sMaterialId) || crowns.Contains(entry.m_sMaterialId))
				return NewResult(RICON_EOvercanopyCode.INVALID_INPUT);
			if (catalog.Get(entry.m_sMaterialId).m_bNegligible)
				return NewResult(RICON_EOvercanopyCode.INVALID_INPUT);
			if (entry.m_sProvenance == "STANDARD_DEFAULT")
			{
				if (entry.m_bHasHeight)
					return NewResult(RICON_EOvercanopyCode.INVALID_INPUT);
			}
			else if (entry.m_sProvenance == "MEASURED" || entry.m_sProvenance == "ASSUMED_CONFIG")
			{
				if (!entry.m_bHasHeight || !Positive(entry.m_fHeightM))
					return NewResult(RICON_EOvercanopyCode.INVALID_INPUT);
			}
			else
				return NewResult(RICON_EOvercanopyCode.INVALID_INPUT);
			crowns.Set(entry.m_sMaterialId, entry);
		}
		if (input.m_Observation.m_aCoverage[RICON_EPathFamily.VEGETATION] != RICON_EPathCoverage.COMPLETE)
			return NewResult(RICON_EOvercanopyCode.VEGETATION_COVERAGE_INCOMPLETE);
		array<ref RICON_VegetationInterval> intervals = {};
		bool mixed;
		foreach (RICON_PathSegment segment : input.m_Observation.m_aSegments)
		{
			if (segment.m_eFamily != RICON_EPathFamily.VEGETATION)
				continue;
			if (segment.m_eKnowledge < RICON_EPathKnowledge.RULE || !catalog.Contains(segment.m_sMaterialId))
				return NewResult(RICON_EOvercanopyCode.VEGETATION_UNKNOWN);
			if (catalog.Get(segment.m_sMaterialId).m_bNegligible)
				continue;
			int count = intervals.Count();
			if (count > 0 && segment.m_fStartM <= intervals[count - 1].m_fEndM)
			{
				mixed = mixed || segment.m_sMaterialId != intervals[count - 1].m_sMaterialId;
				intervals[count - 1].m_fEndM = Math.Max(intervals[count - 1].m_fEndM, segment.m_fEndM);
				continue;
			}
			RICON_VegetationInterval interval = new RICON_VegetationInterval();
			interval.m_fStartM = segment.m_fStartM;
			interval.m_fEndM = segment.m_fEndM;
			interval.m_sMaterialId = segment.m_sMaterialId;
			intervals.Insert(interval);
		}
		if (intervals.IsEmpty())
			return NewResult(RICON_EOvercanopyCode.NO_VEGETATION);
		if (mixed)
			return NewResult(RICON_EOvercanopyCode.VEGETATION_MIXED_PROFILE);
		RICON_VegetationInterval first = intervals[0];
		RICON_VegetationInterval last = intervals[intervals.Count() - 1];
		bool tx = first.m_fStartM == 0;
		bool rx = last.m_fEndM == length;
		if (!tx && !rx)
			return NewResult(RICON_EOvercanopyCode.NO_TERMINAL_IN_VEGETATION);
		if ((tx && !crowns.Contains(first.m_sMaterialId)) || (rx && !crowns.Contains(last.m_sMaterialId)))
			return NewResult(RICON_EOvercanopyCode.INVALID_INPUT);
		float frequency = input.m_Observation.m_fFrequencyMHz;
		if (frequency < 30 || frequency > 3000 || (tx && !input.m_bHasTxHeight) || (rx && !input.m_bHasRxHeight))
			return NewResult(RICON_EOvercanopyCode.OUT_OF_MODEL_DOMAIN);
		RICON_OvercanopyResult result = NewResult(RICON_EOvercanopyCode.OK);
		result.m_sModel = "M6";
		result.m_sStatus = "SOURCE_DOMAIN";
		result.m_sBudgetMode = "BASE_AT_CANOPY_HEIGHT";
		RICON_OvercanopyTerminal terminal;
		if (tx)
		{
			if (!CalculateTerminal("TX", input.m_fTxHeightM, frequency, first, crowns.Get(first.m_sMaterialId), terminal))
				return NewResult(RICON_EOvercanopyCode.NUMERIC_FAILURE);
			result.m_aTerminals.Insert(terminal);
			result.m_fTotalAhDb += terminal.m_fAhDb;
		}
		if (rx)
		{
			if (!CalculateTerminal("RX", input.m_fRxHeightM, frequency, last, crowns.Get(last.m_sMaterialId), terminal))
				return NewResult(RICON_EOvercanopyCode.NUMERIC_FAILURE);
			result.m_aTerminals.Insert(terminal);
			result.m_fTotalAhDb += terminal.m_fAhDb;
		}
		if (!Nonnegative(result.m_fTotalAhDb))
			return NewResult(RICON_EOvercanopyCode.NUMERIC_FAILURE);
		return result;
	}
}
