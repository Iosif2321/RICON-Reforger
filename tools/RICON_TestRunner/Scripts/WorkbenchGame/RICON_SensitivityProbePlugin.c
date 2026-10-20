//! TEST_ONLY: выбранные грунты и фазы профиля, без мира и без новой модели физики.
[WorkbenchPluginAttribute(name: "RICON: чувствительность L2", description: "16 опорных точек грунта и шага профиля", wbModules: {"ScriptEditor"})]
class RICON_SensitivityProbePlugin : RICON_LinkPhysicsProbePlugin
{
	static const float PEAK_TOLERANCE_M = 0.001;
	protected int m_iCases;
	protected int m_iQuietPositive;
	protected int m_iControls;
	protected bool m_bOk;

	protected static bool Compare(string id, RICON_L2Result actual, RICON_L2Result expected,
		int pointCount, float peak, int expectedCount, float expectedPeak, bool quiet = false)
	{
		if (!expected)
			return Check(false, id, "эталон существует", quiet);
		array<float> numbers = {expected.m_fLossDb, expected.m_fBullingtonActualDb,
			expected.m_fBullingtonSmoothDb, expected.m_fSphericalDb};
		foreach (float number : numbers)
		{
			if (!RICON_Num.IsFinite(number))
				return Check(false, id, "эталон конечен", quiet);
		}
		RICON_L2Run comparison = new RICON_L2Run(id, quiet);
		comparison.CheckOk(actual, expected.m_fLossDb, expected.m_fBullingtonActualDb,
			expected.m_fBullingtonSmoothDb, expected.m_fSphericalDb, expected.m_eBranchActual,
			expected.m_eBranchSmooth, expected.m_eSphericalCase, expected.m_eFirstTermAccuracy, DB_TOLERANCE);
		bool ok = comparison.IsPassed();
		ok = Check(pointCount == expectedCount, id, "число точек профиля", quiet) && ok;
		return Check(Within(peak, expectedPeak, PEAK_TOLERANCE_M), id, "высота вершины на сетке", quiet) && ok;
	}

	protected void Reject(string name, bool accepted)
	{
		if (!accepted)
			m_iControls++;
		else
		{
			m_bOk = false;
			RICON_TestSupport.Expect(false, "SENS_CONTROL", name);
		}
	}

	protected void Controls(string id, RICON_L2Result actual, RICON_L2Result expected, int count, float peak)
	{
		float saved = expected.m_fLossDb;
		expected.m_fLossDb += 2 * DB_TOLERANCE;
		Reject("L", Compare(id, actual, expected, count, peak, count, peak, true));
		expected.m_fLossDb = saved;
		saved = expected.m_fBullingtonActualDb;
		expected.m_fBullingtonActualDb += 2 * DB_TOLERANCE;
		Reject("Lba", Compare(id, actual, expected, count, peak, count, peak, true));
		expected.m_fBullingtonActualDb = saved;
		saved = expected.m_fBullingtonSmoothDb;
		expected.m_fBullingtonSmoothDb += 2 * DB_TOLERANCE;
		Reject("Lbs", Compare(id, actual, expected, count, peak, count, peak, true));
		expected.m_fBullingtonSmoothDb = saved;
		saved = expected.m_fSphericalDb;
		expected.m_fSphericalDb += 2 * DB_TOLERANCE;
		Reject("Lsph", Compare(id, actual, expected, count, peak, count, peak, true));
		expected.m_fSphericalDb = saved;

		RICON_EL2Branch branch = expected.m_eBranchActual;
		if (branch == RICON_EL2Branch.LOS)
			expected.m_eBranchActual = RICON_EL2Branch.TRANS;
		else
			expected.m_eBranchActual = RICON_EL2Branch.LOS;
		Reject("branchActual", Compare(id, actual, expected, count, peak, count, peak, true));
		expected.m_eBranchActual = branch;
		branch = expected.m_eBranchSmooth;
		if (branch == RICON_EL2Branch.LOS)
			expected.m_eBranchSmooth = RICON_EL2Branch.TRANS;
		else
			expected.m_eBranchSmooth = RICON_EL2Branch.LOS;
		Reject("branchSmooth", Compare(id, actual, expected, count, peak, count, peak, true));
		expected.m_eBranchSmooth = branch;
		RICON_EL2SphericalCase spherical = expected.m_eSphericalCase;
		if (spherical == RICON_EL2SphericalCase.CLEAR)
			expected.m_eSphericalCase = RICON_EL2SphericalCase.BEYOND;
		else
			expected.m_eSphericalCase = RICON_EL2SphericalCase.CLEAR;
		Reject("sphCase", Compare(id, actual, expected, count, peak, count, peak, true));
		expected.m_eSphericalCase = spherical;
		RICON_EFirstTermAccuracy accuracy = expected.m_eFirstTermAccuracy;
		if (accuracy == RICON_EFirstTermAccuracy.MET)
			expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.NOT_MET;
		else
			expected.m_eFirstTermAccuracy = RICON_EFirstTermAccuracy.MET;
		Reject("accuracy", Compare(id, actual, expected, count, peak, count, peak, true));
		expected.m_eFirstTermAccuracy = accuracy;

		RICON_EL2ResultCode code = actual.m_eCode;
		actual.m_eCode = RICON_EL2ResultCode.INVALID_PROFILE;
		Reject("code", Compare(id, actual, expected, count, peak, count, peak, true));
		actual.m_eCode = code;
		Reject("pointCount", Compare(id, actual, expected, count, peak, count + 1, peak, true));
		Reject("peak", Compare(id, actual, expected, count, peak, count, peak + 10 * PEAK_TOLERANCE_M, true));
		float nanValue = RICON_L2Run.NaN();
		m_bOk = RICON_TestSupport.Expect(RICON_TestSupport.FloatClass(nanValue) == 1, id, "сформировано NaN") && m_bOk;
		saved = actual.m_fLossDb;
		actual.m_fLossDb = nanValue;
		Reject("actual NaN", Compare(id, actual, expected, count, peak, count, peak, true));
		actual.m_fLossDb = saved;
		saved = expected.m_fLossDb;
		expected.m_fLossDb = nanValue;
		Reject("expected NaN", Compare(id, actual, expected, count, peak, count, peak, true));
		expected.m_fLossDb = saved;
		Reject("actual null", Compare(id, null, expected, count, peak, count, peak, true));
		Reject("expected null", Compare(id, actual, null, count, peak, count, peak, true));
	}

	protected static void TraceNumber(string id, string field, float actual, float expected)
	{
		PrintFormat("[RICON_SENS] %1 %2 actual=%3 expected=%4", id, field,
			actual.ToString(-1, 9), expected.ToString(-1, 9));
	}

	//! Ожидания приходят из golden JSON; здесь строится только заданный синтетический вход.
	void CheckPoint(string id, float distanceM, float frequencyMHz, float antennaHeightM,
		float centerKm, float halfWidthKm, float hillHeightM, RICON_TerrainDiffractionConfig config,
		RICON_L2Result expected, int expectedCount, float expectedPeak)
	{
		m_iCases++;
		RICON_TerrainSamplingResult sampling = RICON_TerrainProfileSampling.BuildDistances(distanceM, config);
		if (!RICON_TestSupport.Expect(sampling && sampling.m_eCode == RICON_ETerrainSamplingCode.OK,
			id, "построение сетки"))
		{
			m_bOk = false;
			return;
		}
		RICON_TerrainProfile profile = new RICON_TerrainProfile();
		float peak = 0;
		foreach (float position : sampling.m_aDistanceKm)
		{
			float height = hillHeightM * Math.Max(0, 1.0 - Math.AbsFloat(position - centerKm) / halfWidthKm);
			profile.AddPoint(position, height);
			peak = Math.Max(peak, height);
		}
		RICON_L2Run purity = new RICON_L2Run(id);
		RICON_L2Result actual = purity.Evaluate(profile, antennaHeightM, antennaHeightM, frequencyMHz, config);
		m_bOk = RICON_TestSupport.Expect(!purity.IsFailed(), id, "неизменность входов") && m_bOk;
		int count = profile.m_aDistanceKm.Count();
		m_bOk = Compare(id, actual, expected, count, peak, expectedCount, expectedPeak) && m_bOk;
		if (Compare(id, actual, expected, count, peak, expectedCount, expectedPeak, true))
			m_iQuietPositive++;
		else
			m_bOk = false;
		if (!actual || !expected)
			return;
		TraceNumber(id, "L", actual.m_fLossDb, expected.m_fLossDb);
		TraceNumber(id, "Lba", actual.m_fBullingtonActualDb, expected.m_fBullingtonActualDb);
		TraceNumber(id, "Lbs", actual.m_fBullingtonSmoothDb, expected.m_fBullingtonSmoothDb);
		TraceNumber(id, "Lsph", actual.m_fSphericalDb, expected.m_fSphericalDb);
		PrintFormat("[RICON_SENS] %1 points=%2 expectedPoints=%3 peakM=%4 expectedPeakM=%5", id, count,
			expectedCount, peak.ToString(-1, 9), expectedPeak.ToString(-1, 9));
		PrintFormat("[RICON_SENS] %1 branchActual=%2 branchSmooth=%3 sphCase=%4 accuracy=%5", id,
			typename.EnumToString(RICON_EL2Branch, actual.m_eBranchActual),
			typename.EnumToString(RICON_EL2Branch, actual.m_eBranchSmooth),
			typename.EnumToString(RICON_EL2SphericalCase, actual.m_eSphericalCase),
			typename.EnumToString(RICON_EFirstTermAccuracy, actual.m_eFirstTermAccuracy));
		if (m_iCases == 1)
			Controls(id, actual, expected, count, peak);
	}

	override void RunCommandline()
	{
		m_iCases = 0;
		m_iQuietPositive = 0;
		m_iControls = 0;
		m_bOk = true;
		RICON_L2Run.s_fMaxDeviationDb = 0;
		Print("[RICON_SENS] source=" + RICON_SensitivityPoints.SOURCE_SHA);
		RICON_SensitivityPoints.Run(this);
		PrintFormat("[RICON_SENS] cases=%1 quietPositive=%2 controls=%3 maxDeviationDb=%4", m_iCases,
			m_iQuietPositive, m_iControls, RICON_L2Run.s_fMaxDeviationDb.ToString(-1, 9));
		if (m_bOk && m_iCases == 16 && m_iQuietPositive == 16 && m_iControls == 15)
		{
			Print("[RICON_SENS] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_SENS] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
