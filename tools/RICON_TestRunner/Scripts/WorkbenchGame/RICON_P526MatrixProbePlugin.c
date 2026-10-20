//! TEST_ONLY: P526-точки принятой матрицы; старый построитель запросов R21 не изменяется.
[WorkbenchPluginAttribute(name: "RICON: матрица P526", description: "Потери, мощность и SINR в 1008 расчётах Core", wbModules: {"ScriptEditor"})]
class RICON_P526MatrixProbePlugin : RICON_LinkPhysicsProbePlugin
{
	protected int m_iCases;
	protected int m_iQuietPositive;
	protected int m_iControls;
	protected int m_iPhysicalSnr;
	protected int m_iFallback;
	protected bool m_bOk;

	protected static bool CompareSample(string id, RICON_LinkBudget actual, float loss, float rx, float sinr,
		bool domain, bool above, bool meets, bool checkMeets, bool quiet = false)
	{
		if (!Check(actual != null, id, "бюджет существует", quiet))
			return false;
		bool ok = Check(actual.m_bValid && actual.m_eReason == RICON_EReason.OK, id, "valid / reason", quiet);
		ok = CheckDb(id, "totalLossDb", actual.m_fTotalLossDb, loss, quiet) && ok;
		ok = CheckDb(id, "rxPowerDbm", actual.m_fRxPowerDbm, rx, quiet) && ok;
		ok = CheckDb(id, "sinrDb", actual.m_fSinrDb, sinr, quiet) && ok;
		array<float> numbers = {actual.m_fDistanceM, actual.m_fThermalNoiseDbm, actual.m_fNoiseDbm,
			actual.m_fNoisePlusInterferenceDbm, actual.m_fSensitivityDbm, actual.m_fIntelligibility};
		foreach (float number : numbers)
			ok = Check(RICON_Num.IsFinite(number), id, "конечность бюджета", quiet) && ok;
		bool actualAbove = actual.m_fRxPowerDbm >= actual.m_fSensitivityDbm;
		bool actualMeets = actual.m_fSinrDb >= 10.0;
		ok = Check(actual.m_bNoiseModelInDomain == domain, id, "noiseModelInDomain", quiet) && ok;
		ok = Check(actualAbove == above, id, "aboveSensitivity", quiet) && ok;
		if (checkMeets)
			ok = Check(actualMeets == meets, id, "meetsSnr", quiet) && ok;
		if (!quiet)
			PrintFormat("[RICON_P526] %1 domain=%2 above=%3 meets=%4 physicalSnr=%5",
				id, actual.m_bNoiseModelInDomain, actualAbove, actualMeets, checkMeets);
		return ok;
	}

	protected void Reject(bool accepted)
	{
		if (!accepted)
			m_iControls++;
		else
			m_bOk = false;
	}

	//! Ожидания только копируются из JSON; для внеобластного шума sinr — опорный none, а не физический эталон.
	void CheckPoint(string id, float frequency, float distance, float height, float txPower, float peak,
		string environment, float loss, float rx, float sinr, bool domain, bool above, bool meets, bool checkMeets)
	{
		RICON_LinkBudget actual = EvaluatePoint(RICON_TerrainDiffractionModel.MODEL_ID, environment,
			frequency, distance, height, txPower, peak);
		m_iCases++;
		m_bOk = CompareSample(id, actual, loss, rx, sinr, domain, above, meets, checkMeets) && m_bOk;
		m_bOk = RICON_TestSupport.Expect(domain == checkMeets, id, "область сравнения SNR") && m_bOk;
		if (CompareSample(id, actual, loss, rx, sinr, domain, above, meets, checkMeets, true))
			m_iQuietPositive++;
		else
			m_bOk = false;
		if (checkMeets)
			m_iPhysicalSnr++;
		else
			m_iFallback++;
		if (!checkMeets && m_iFallback == 1 && actual)
			Reject(CompareSample(id, actual, loss, rx, sinr + 2 * DB_TOLERANCE, domain, above, meets, false, true));

		if (m_iCases == 1 && actual)
		{
			Reject(CompareSample(id, actual, loss + 2 * DB_TOLERANCE, rx, sinr, domain, above, meets, checkMeets, true));
			Reject(CompareSample(id, actual, loss, rx + 2 * DB_TOLERANCE, sinr, domain, above, meets, checkMeets, true));
			Reject(CompareSample(id, actual, loss, rx, sinr + 2 * DB_TOLERANCE, domain, above, meets, checkMeets, true));
			Reject(CompareSample(id, actual, loss, rx, sinr, !domain, above, meets, checkMeets, true));
			Reject(CompareSample(id, actual, loss, rx, sinr, domain, !above, meets, checkMeets, true));
			Reject(CompareSample(id, actual, loss, rx, sinr, domain, above, !meets, checkMeets, true));
			Reject(CompareSample(id, actual, RICON_L2Run.NaN(), rx, sinr, domain, above, meets, checkMeets, true));
			float savedLoss = actual.m_fTotalLossDb;
			actual.m_fTotalLossDb = RICON_L2Run.NaN();
			Reject(CompareSample(id, actual, loss, rx, sinr, domain, above, meets, checkMeets, true));
			actual.m_fTotalLossDb = savedLoss;
		}
	}

	override void RunCommandline()
	{
		m_iCases = 0;
		m_iQuietPositive = 0;
		m_iControls = 0;
		m_iPhysicalSnr = 0;
		m_iFallback = 0;
		m_bOk = true;
		s_fMaxDeviationDb = 0;
		Print("[RICON_P526] source=" + RICON_P526MatrixPoints.SOURCE_SHA);
		RICON_P526MatrixPoints.Run(this);
		PrintFormat("[RICON_P526] cases=%1 quietPositive=%2 controls=%3 physicalSnr=%4 fallback=%5 maxDeviationDb=%6",
			m_iCases, m_iQuietPositive, m_iControls, m_iPhysicalSnr, m_iFallback, s_fMaxDeviationDb.ToString(-1, 9));
		bool ok = m_bOk && m_iCases == 1008 && m_iQuietPositive == 1008 && m_iControls == 9
			&& m_iPhysicalSnr == 840 && m_iFallback == 168;
		if (ok)
		{
			Print("[RICON_P526] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_P526] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
