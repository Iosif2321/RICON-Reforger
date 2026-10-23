//! P4: чистые оценки на заданной прямой, REQ-vegetation v0.4. Нет мира, каталога по умолчанию, LOST и P1-границ.
enum RICON_EVegetationCode
{
	INVALID_OBSERVATION = 0,
	INVALID_INPUT = 1,
	VEGETATION_COVERAGE_INCOMPLETE = 2,
	VEGETATION_UNKNOWN = 3,
	NO_VEGETATION = 4,
	VEGETATION_MIXED_PROFILE = 5,
	NO_MODEL_FOR_SCENARIO = 6,
	NUMERIC_FAILURE = 7,
	OK = 8
}

class RICON_VegetationPoint
{
	float m_fFrequencyMHz;
	float m_fGamma;
}

class RICON_VegetationTable
{
	string m_sPolarization;
	ref array<ref RICON_VegetationPoint> m_aPoints = {};
}

class RICON_VegetationAm
{
	float m_fA1;
	float m_fAlpha;
	float m_fMinMHz;
	float m_fMaxMHz;
	string m_sProvenance;
}

class RICON_VegetationProfile
{
	string m_sMaterialId;
	bool m_bNegligible;
	string m_sClimate;
	string m_sLeafState;
	string m_sDensity;
	string m_sMoisture;
	string m_sGammaProvenance;
	ref array<ref RICON_VegetationTable> m_aTables = {};
	ref RICON_VegetationAm m_Am;
}

class RICON_VegetationInput
{
	ref RICON_PathObservation m_Observation;
	ref array<ref RICON_VegetationProfile> m_aCatalog = {};
	string m_sPolarization;
	bool m_bHasTxHeight;
	float m_fTxHeightM;
	bool m_bHasRxHeight;
	float m_fRxHeightM;
}

class RICON_VegetationCandidate
{
	string m_sModel;
	float m_fLossDb;
	string m_sStatus;
}

class RICON_VegetationInterval
{
	float m_fStartM;
	float m_fEndM;
	string m_sMaterialId;
	string m_sScenario;
	string m_sPrimaryModel;
	float m_fPrimaryLossDb;
	string m_sPrimaryStatus;
	bool m_bScenarioClosed;
	string m_sDepthTrend;
	string m_sExcessBaseline;
	ref array<ref RICON_VegetationCandidate> m_aCandidates = {};
	float m_fSpreadMinDb;
	float m_fSpreadMaxDb;
	float m_fExdDiagnosticDb;
	bool m_bHasGamma;
	float m_fGamma;
	string m_sGammaSource = "NONE";
	string m_sGammaProvenance;
	string m_sAmProvenance;
	bool m_bCapApplied;
}

class RICON_VegetationResult
{
	RICON_EVegetationCode m_eCode;
	bool m_bHasLoss;
	float m_fLossDb;
	bool m_bAllScenariosClosed;
	ref array<ref RICON_VegetationInterval> m_aIntervals = {};
	//! Только NO_MODEL: первый непокрытый интервал, без частичных потерь.
	ref RICON_VegetationInterval m_Uncovered;
}

class RICON_VegetationLayer
{
	protected static bool Positive(float x)
	{
		return RICON_Num.IsFinite(x) && x > 0;
	}

	protected static bool Nonnegative(float x)
	{
		return RICON_Num.IsFinite(x) && x >= 0;
	}

	protected static bool Provenance(string s)
	{
		return s == "MEASURED_TABLE" || s == "STANDARD_FIGURE" || s == "ASSUMED_CONFIG";
	}

	protected static bool ValidProfile(RICON_VegetationProfile p)
	{
		if (!p || !RICON_PathObservationModel.ValidKey(p.m_sMaterialId) || !p.m_aTables)
			return false;
		if (p.m_bNegligible)
			return p.m_aTables.IsEmpty() && !p.m_Am && p.m_sClimate.IsEmpty() && p.m_sLeafState.IsEmpty()
				&& p.m_sDensity.IsEmpty() && p.m_sMoisture.IsEmpty() && p.m_sGammaProvenance.IsEmpty();
		if (p.m_sClimate != "TEMPERATE" && p.m_sClimate != "TROPICAL" && p.m_sClimate != "UNSPECIFIED")
			return false;
		if (p.m_sLeafState != "IN_LEAF" && p.m_sLeafState != "LEAFLESS" && p.m_sLeafState != "UNKNOWN")
			return false;
		if (p.m_sDensity != "DENSE" && p.m_sDensity != "SPARSE" && p.m_sDensity != "UNKNOWN")
			return false;
		if (p.m_sMoisture != "DRY" && p.m_sMoisture != "WET" && p.m_sMoisture != "UNKNOWN")
			return false;
		if (!Provenance(p.m_sGammaProvenance) || p.m_aTables.Count() < 1 || p.m_aTables.Count() > 2)
			return false;
		set<string> seen = new set<string>();
		foreach (RICON_VegetationTable table : p.m_aTables)
		{
			if (!table || !table.m_aPoints || table.m_aPoints.Count() < 2 || table.m_aPoints.Count() > 32)
				return false;
			if ((table.m_sPolarization != "V" && table.m_sPolarization != "H" && table.m_sPolarization != "ANY")
				|| seen.Contains(table.m_sPolarization) || (table.m_sPolarization == "ANY" && p.m_aTables.Count() != 1))
				return false;
			seen.Insert(table.m_sPolarization);
			float previous = 0;
			foreach (RICON_VegetationPoint point : table.m_aPoints)
			{
				if (!point || !Positive(point.m_fFrequencyMHz) || !Positive(point.m_fGamma) || point.m_fFrequencyMHz <= previous)
					return false;
				previous = point.m_fFrequencyMHz;
			}
		}
		RICON_VegetationAm am = p.m_Am;
		return !am || (Positive(am.m_fA1) && RICON_Num.IsFinite(am.m_fAlpha) && Positive(am.m_fMinMHz)
			&& RICON_Num.IsFinite(am.m_fMaxMHz) && am.m_fMinMHz < am.m_fMaxMHz && Provenance(am.m_sProvenance));
	}

	protected static RICON_VegetationResult Make(RICON_EVegetationCode code)
	{
		RICON_VegetationResult r = new RICON_VegetationResult();
		r.m_eCode = code;
		r.m_bHasLoss = code == RICON_EVegetationCode.OK || code == RICON_EVegetationCode.NO_VEGETATION;
		return r;
	}

	protected static bool Gamma(RICON_VegetationProfile p, float frequency, string pol, out float gamma, out string source)
	{
		gamma = 0;
		source = "NONE";
		foreach (RICON_VegetationTable table : p.m_aTables)
		{
			if (table.m_sPolarization != pol && table.m_sPolarization != "ANY")
				continue;
			for (int i = 0; i < table.m_aPoints.Count(); i++)
			{
				RICON_VegetationPoint point = table.m_aPoints[i];
				if (frequency == point.m_fFrequencyMHz)
				{
					gamma = point.m_fGamma;
					source = "TABLE_POINT";
					return true;
				}
				if (i == 0 || frequency >= point.m_fFrequencyMHz)
					continue;
				RICON_VegetationPoint left = table.m_aPoints[i - 1];
				if (frequency <= left.m_fFrequencyMHz)
					continue;
				float t = (Math.Log(frequency) - Math.Log(left.m_fFrequencyMHz)) / (Math.Log(point.m_fFrequencyMHz) - Math.Log(left.m_fFrequencyMHz));
				gamma = Math.Pow(Math.E, Math.Log(left.m_fGamma) + t * (Math.Log(point.m_fGamma) - Math.Log(left.m_fGamma)));
				source = "INTERPOLATED";
				return true;
			}
		}
		return false;
	}

	//! ECAC табл. 13: alpha/A/B; индекс 0..4 соответствует 25/50/100/250/400 МГц.
	protected static void JbPoint(int index, string pol, out float alpha, out float a, out float b)
	{
		array<float> av = {0, 0, 0.045, 0.050, 0.055};
		array<float> aa = {0, 0, 0.615, 0.759, 1.02};
		array<float> ab = {0.00212, 0.00106, 0.000529, 0.000443, 0.000523};
		if (pol == "H")
		{
			av = {0, 0, 0.020, 0.025, 0.035};
			aa = {0, 0, 0.472, 0.774, 1.11};
			ab = {0.00424, 0.00424, 0.00551, 0.000588, 0.000598};
		}
		alpha = av[index];
		a = aa[index];
		b = ab[index];
	}

	protected static float JanskyBailey(float frequency, float depth, string pol)
	{
		array<float> frequencies = {25, 50, 100, 250, 400};
		int i = 0;
		while (i < 3 && frequency > frequencies[i + 1])
			i++;
		float alpha;
		float a;
		float b;
		JbPoint(i, pol, alpha, a, b);
		float alpha2;
		float a2;
		float b2;
		JbPoint(i + 1, pol, alpha2, a2, b2);
		float t = Math.Log(frequency / frequencies[i]) / Math.Log(frequencies[i + 1] / frequencies[i]);
		alpha = alpha + t * (alpha2 - alpha);
		a = a + t * (a2 - a);
		b = b + t * (b2 - b);
		float miles = depth / 1609.344;
		return -20 * Math.Log10(a * Math.Pow(Math.E, -1609 * alpha * miles) + b / miles);
	}

	protected static void Add(RICON_VegetationInterval r, string model, float loss)
	{
		RICON_VegetationCandidate c = new RICON_VegetationCandidate();
		c.m_sModel = model;
		c.m_fLossDb = loss;
		c.m_sStatus = "SOURCE_DOMAIN";
		if (model == "M2")
			c.m_sStatus = "UNCAPPED";
		else if (model == "M5")
			c.m_sStatus = "ASSUMED_EXTENSION";
		r.m_aCandidates.Insert(c);
	}

	//! (1-exp(-x))/x при x<0.1 без вычитания близких чисел.
	protected static float ShortPathFactor(float x)
	{
		return 1 - x * (0.5 - x * (1.0 / 6.0 - x * (1.0 / 24.0 - x * (1.0 / 120.0 - x / 720))));
	}

	protected static float WoodlandLoss(float depth, float gamma, float frequency, RICON_VegetationAm am)
	{
		float maximum = am.m_fA1 * Math.Pow(frequency, am.m_fAlpha);
		if (Positive(maximum))
		{
			float x = depth * (gamma / maximum);
			if (x < 0.1)
				return maximum * x * ShortPathFactor(x);
			return maximum * (1 - Math.Pow(Math.E, -x));
		}
		// Промежуточный Am может не помещаться во float32, хотя итоговая потеря конечна.
		float logAm = Math.Log(am.m_fA1) + am.m_fAlpha * Math.Log(frequency);
		if (!RICON_Num.IsFinite(logAm))
			return -1;
		float logX = Math.Log(depth) + Math.Log(gamma) - logAm;
		if (logX < Math.Log(0.1))
			return depth * gamma * ShortPathFactor(Math.Pow(Math.E, logX));
		float fraction = 1;
		if (logX < Math.Log(32))
			fraction = 1 - Math.Pow(Math.E, -Math.Pow(Math.E, logX));
		return Math.Pow(Math.E, logAm + Math.Log(fraction));
	}

	//! Раздельные признаки применимости и арифметического отказа сохраняют приоритет NO_MODEL всего пути.
	protected static bool Calculate(RICON_VegetationInterval r, RICON_VegetationInput input, RICON_VegetationProfile p)
	{
		float d = r.m_fEndM - r.m_fStartM;
		float f = input.m_Observation.m_fFrequencyMHz;
		r.m_bHasGamma = Gamma(p, f, input.m_sPolarization, r.m_fGamma, r.m_sGammaSource);
		if (r.m_bHasGamma)
			r.m_sGammaProvenance = p.m_sGammaProvenance;
		if (p.m_Am)
			r.m_sAmProvenance = p.m_Am.m_sProvenance;
		if (r.m_sScenario == "S2" && p.m_sClimate == "TROPICAL" && f >= 25 && f <= 400 && d >= 8 && d <= 1600
			&& input.m_bHasTxHeight && input.m_bHasRxHeight && input.m_fTxHeightM >= 2 && input.m_fTxHeightM <= 7
			&& input.m_fRxHeightM >= 2 && input.m_fRxHeightM <= 7)
		{
			float aj = JanskyBailey(f, d, input.m_sPolarization);
			if (aj >= 0)
				Add(r, "M4", aj);
		}
		if (r.m_sScenario != "S3+" && f >= 230 && f <= 95000 && d <= 400 && p.m_sClimate == "TEMPERATE"
			&& p.m_sLeafState == "IN_LEAF" && p.m_sDensity == "DENSE" && p.m_sMoisture == "DRY")
		{
			float med = 0.45 * d;
			if (d >= 14)
				med = 1.33 * Math.Pow(d, 0.588);
			Add(r, "M3", med * Math.Pow(f / 1000, 0.284));
		}
		if (r.m_sScenario == "S1" && r.m_bHasGamma && p.m_Am && f >= p.m_Am.m_fMinMHz && f <= p.m_Am.m_fMaxMHz)
		{
			Add(r, "M1", WoodlandLoss(d, r.m_fGamma, f, p.m_Am));
		}
		if (r.m_sScenario == "S3" && f <= 1000 && r.m_bHasGamma)
			Add(r, "M2", d * r.m_fGamma);
		if ((r.m_sScenario == "S1" || r.m_sScenario == "S2") && d <= 400 && r.m_bHasGamma)
			Add(r, "M5", d * r.m_fGamma);
		if (r.m_aCandidates.IsEmpty())
			return false;
		RICON_VegetationCandidate primary = r.m_aCandidates[0];
		r.m_sPrimaryModel = primary.m_sModel;
		r.m_fPrimaryLossDb = primary.m_fLossDb;
		r.m_sPrimaryStatus = primary.m_sStatus;
		r.m_bScenarioClosed = primary.m_sStatus == "SOURCE_DOMAIN" && (r.m_sScenario == "S1" || r.m_sScenario == "S2");
		r.m_sExcessBaseline = "OTHER_MECHANISMS";
		r.m_sDepthTrend = "LINEAR";
		if (primary.m_sModel == "M1")
			r.m_sDepthTrend = "SATURATING";
		else if (primary.m_sModel == "M3")
			r.m_sDepthTrend = "SUBLINEAR";
		else if (primary.m_sModel == "M4")
		{
			r.m_sDepthTrend = "EMPIRICAL";
			r.m_sExcessBaseline = "FREE_SPACE";
		}
		r.m_fSpreadMinDb = primary.m_fLossDb;
		r.m_fSpreadMaxDb = primary.m_fLossDb;
		foreach (RICON_VegetationCandidate candidate : r.m_aCandidates)
		{
			r.m_fSpreadMinDb = Math.Min(r.m_fSpreadMinDb, candidate.m_fLossDb);
			r.m_fSpreadMaxDb = Math.Max(r.m_fSpreadMaxDb, candidate.m_fLossDb);
		}
		r.m_fExdDiagnosticDb = 0.26 * Math.Pow(f / 1000, 0.77) * d;
		return true;
	}

	protected static bool Finite(RICON_VegetationInterval r)
	{
		if (!Nonnegative(r.m_fPrimaryLossDb) || !Nonnegative(r.m_fSpreadMinDb) || !Nonnegative(r.m_fSpreadMaxDb)
			|| !Nonnegative(r.m_fExdDiagnosticDb) || (r.m_bHasGamma && !Nonnegative(r.m_fGamma)))
			return false;
		foreach (RICON_VegetationCandidate c : r.m_aCandidates)
		{
			if (!Nonnegative(c.m_fLossDb))
				return false;
		}
		return true;
	}

	static RICON_VegetationResult Evaluate(RICON_VegetationInput input)
	{
		float length;
		array<ref RICON_WaterInterval> water;
		if (!input || !RICON_PathObservationModel.Validate(input.m_Observation, length, water))
			return Make(RICON_EVegetationCode.INVALID_OBSERVATION);
		if (!input.m_aCatalog || (input.m_sPolarization != "V" && input.m_sPolarization != "H")
			|| (input.m_bHasTxHeight && !Nonnegative(input.m_fTxHeightM)) || (input.m_bHasRxHeight && !Nonnegative(input.m_fRxHeightM)))
			return Make(RICON_EVegetationCode.INVALID_INPUT);
		map<string, ref RICON_VegetationProfile> catalog = new map<string, ref RICON_VegetationProfile>();
		foreach (RICON_VegetationProfile p : input.m_aCatalog)
		{
			if (!ValidProfile(p) || catalog.Contains(p.m_sMaterialId))
				return Make(RICON_EVegetationCode.INVALID_INPUT);
			catalog.Set(p.m_sMaterialId, p);
		}
		if (input.m_Observation.m_aCoverage[RICON_EPathFamily.VEGETATION] != RICON_EPathCoverage.COMPLETE)
			return Make(RICON_EVegetationCode.VEGETATION_COVERAGE_INCOMPLETE);
		array<ref RICON_VegetationInterval> intervals = {};
		bool mixed;
		foreach (RICON_PathSegment segment : input.m_Observation.m_aSegments)
		{
			if (segment.m_eFamily != RICON_EPathFamily.VEGETATION)
				continue;
			if (segment.m_eKnowledge < RICON_EPathKnowledge.RULE || !catalog.Contains(segment.m_sMaterialId))
				return Make(RICON_EVegetationCode.VEGETATION_UNKNOWN);
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
			return Make(RICON_EVegetationCode.NO_VEGETATION);
		if (mixed)
			return Make(RICON_EVegetationCode.VEGETATION_MIXED_PROFILE);
		RICON_VegetationResult result = Make(RICON_EVegetationCode.OK);
		result.m_bAllScenariosClosed = true;
		foreach (RICON_VegetationInterval r : intervals)
		{
			int touches = 0;
			if (r.m_fStartM == 0)
				touches++;
			if (r.m_fEndM == length)
				touches++;
			r.m_sScenario = "S3";
			if (touches == 2)
				r.m_sScenario = "S2";
			else if (touches == 1)
				r.m_sScenario = "S1";
			else if (r.m_fEndM - r.m_fStartM > 400)
				r.m_sScenario = "S3+";
			if (!Calculate(r, input, catalog.Get(r.m_sMaterialId)))
			{
				RICON_VegetationResult uncovered = Make(RICON_EVegetationCode.NO_MODEL_FOR_SCENARIO);
				uncovered.m_Uncovered = new RICON_VegetationInterval();
				uncovered.m_Uncovered.m_fStartM = r.m_fStartM;
				uncovered.m_Uncovered.m_fEndM = r.m_fEndM;
				uncovered.m_Uncovered.m_sScenario = r.m_sScenario;
				return uncovered;
			}
		}
		foreach (RICON_VegetationInterval finished : intervals)
		{
			if (!Finite(finished))
				return Make(RICON_EVegetationCode.NUMERIC_FAILURE);
			result.m_fLossDb += finished.m_fPrimaryLossDb;
			result.m_bAllScenariosClosed = result.m_bAllScenariosClosed && finished.m_bScenarioClosed;
			result.m_aIntervals.Insert(finished);
		}
		if (!Nonnegative(result.m_fLossDb))
			return Make(RICON_EVegetationCode.NUMERIC_FAILURE);
		return result;
	}
}
