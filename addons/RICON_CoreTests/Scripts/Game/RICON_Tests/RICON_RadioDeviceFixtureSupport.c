//! TEST_ONLY: исполнение корпуса S2 (radio_device_v1.json rev2) через настоящий RICON_RadioQueryBuilder.
//! Настоящих раций, мира и сети здесь нет; native reader проверяется только на безопасной ветке.
class RICON_RadioDeviceRun
{
	int m_iCases;
	int m_iQuietPositive;
	int m_iImmutable;
	int m_iControls;
	int m_iChains;
	int m_iReaderChecks;
	bool m_bOk = true;
	// Копии входов R01 для сквозной цепочки; заполняются при исполнении корпуса.
	protected ref RICON_RadioObservation m_R01TxObservation;
	protected ref RICON_RadioRfProfile m_R01TxProfile;
	protected ref RICON_RadioObservation m_R01RxObservation;
	protected ref RICON_RadioRfProfile m_R01RxProfile;
	protected ref RICON_RadioQueryContext m_R01Context;

	protected void Require(bool condition, string id, string what)
	{
		m_bOk = RICON_TestSupport.Expect(condition, id, what) && m_bOk;
	}

	protected void Reject(string what, bool accepted)
	{
		Require(!accepted, "radio_device controls", what);
		if (!accepted)
			m_iControls++;
	}

	protected static bool SameVector(vector a, vector b)
	{
		return RICON_TestSupport.SameFloat(a[0], b[0]) && RICON_TestSupport.SameFloat(a[1], b[1])
			&& RICON_TestSupport.SameFloat(a[2], b[2]);
	}

	static RICON_RadioObservation CopyObservation(RICON_RadioObservation source)
	{
		if (!source)
			return null;
		RICON_RadioObservation copy = new RICON_RadioObservation();
		copy.m_iDeviceId = source.m_iDeviceId;
		copy.m_iTransceiverIndex = source.m_iTransceiverIndex;
		copy.m_bPowered = source.m_bPowered;
		copy.m_iTuningValue = source.m_iTuningValue;
		copy.m_sNetworkKey = source.m_sNetworkKey;
		copy.m_vAntennaPos = source.m_vAntennaPos;
		return copy;
	}

	static bool SameObservation(RICON_RadioObservation a, RICON_RadioObservation b)
	{
		if (!a || !b)
			return !a && !b;
		return a.m_iDeviceId == b.m_iDeviceId && a.m_iTransceiverIndex == b.m_iTransceiverIndex
			&& a.m_bPowered == b.m_bPowered && a.m_iTuningValue == b.m_iTuningValue
			&& a.m_sNetworkKey == b.m_sNetworkKey && SameVector(a.m_vAntennaPos, b.m_vAntennaPos);
	}

	static RICON_RadioRfProfile CopyProfile(RICON_RadioRfProfile source)
	{
		if (!source)
			return null;
		RICON_RadioRfProfile copy = new RICON_RadioRfProfile();
		copy.m_sProfileId = source.m_sProfileId;
		copy.m_iDeviceId = source.m_iDeviceId;
		copy.m_iTransceiverIndex = source.m_iTransceiverIndex;
		copy.m_sProvenance = source.m_sProvenance;
		copy.m_iExpectedTuningValue = source.m_iExpectedTuningValue;
		copy.m_fFrequencyMHz = source.m_fFrequencyMHz;
		copy.m_fRxBandwidthHz = source.m_fRxBandwidthHz;
		copy.m_fTxPowerDbm = source.m_fTxPowerDbm;
		copy.m_fTxGainDbi = source.m_fTxGainDbi;
		copy.m_fRxGainDbi = source.m_fRxGainDbi;
		copy.m_fRxNoiseFigureDb = source.m_fRxNoiseFigureDb;
		copy.m_bHasRxSensitivity = source.m_bHasRxSensitivity;
		copy.m_fRxSensitivityDbm = source.m_fRxSensitivityDbm;
		copy.m_bHasRequiredSinr = source.m_bHasRequiredSinr;
		copy.m_fRequiredSinrDb = source.m_fRequiredSinrDb;
		return copy;
	}

	static bool SameProfile(RICON_RadioRfProfile a, RICON_RadioRfProfile b)
	{
		if (!a || !b)
			return !a && !b;
		// Enforce отвергает длинные цепочки && («Formula too complex»): проверки по шагам.
		if (a.m_sProfileId != b.m_sProfileId || a.m_iDeviceId != b.m_iDeviceId)
			return false;
		if (a.m_iTransceiverIndex != b.m_iTransceiverIndex || a.m_sProvenance != b.m_sProvenance)
			return false;
		if (a.m_iExpectedTuningValue != b.m_iExpectedTuningValue)
			return false;
		if (!RICON_TestSupport.SameFloat(a.m_fFrequencyMHz, b.m_fFrequencyMHz)
			|| !RICON_TestSupport.SameFloat(a.m_fRxBandwidthHz, b.m_fRxBandwidthHz))
			return false;
		if (!RICON_TestSupport.SameFloat(a.m_fTxPowerDbm, b.m_fTxPowerDbm)
			|| !RICON_TestSupport.SameFloat(a.m_fTxGainDbi, b.m_fTxGainDbi))
			return false;
		if (!RICON_TestSupport.SameFloat(a.m_fRxGainDbi, b.m_fRxGainDbi)
			|| !RICON_TestSupport.SameFloat(a.m_fRxNoiseFigureDb, b.m_fRxNoiseFigureDb))
			return false;
		if (a.m_bHasRxSensitivity != b.m_bHasRxSensitivity
			|| !RICON_TestSupport.SameFloat(a.m_fRxSensitivityDbm, b.m_fRxSensitivityDbm))
			return false;
		return a.m_bHasRequiredSinr == b.m_bHasRequiredSinr
			&& RICON_TestSupport.SameFloat(a.m_fRequiredSinrDb, b.m_fRequiredSinrDb);
	}

	static RICON_RadioQueryContext CopyContext(RICON_RadioQueryContext source)
	{
		if (!source)
			return null;
		RICON_RadioQueryContext copy = new RICON_RadioQueryContext();
		copy.m_sBandId = source.m_sBandId;
		copy.m_sModelId = source.m_sModelId;
		copy.m_sNoiseEnvironmentId = source.m_sNoiseEnvironmentId;
		copy.m_sIntelligibilityModelId = source.m_sIntelligibilityModelId;
		copy.m_bHasInterference = source.m_bHasInterference;
		copy.m_fInterferenceDbm = source.m_fInterferenceDbm;
		return copy;
	}

	static bool SameContext(RICON_RadioQueryContext a, RICON_RadioQueryContext b)
	{
		if (!a || !b)
			return !a && !b;
		return a.m_sBandId == b.m_sBandId && a.m_sModelId == b.m_sModelId
			&& a.m_sNoiseEnvironmentId == b.m_sNoiseEnvironmentId
			&& a.m_sIntelligibilityModelId == b.m_sIntelligibilityModelId
			&& a.m_bHasInterference == b.m_bHasInterference
			&& RICON_TestSupport.SameFloat(a.m_fInterferenceDbm, b.m_fInterferenceDbm);
	}

	//! Точное сравнение: все поля запроса конечны и уже binary32 в обеих реализациях.
	static bool SameQuery(RICON_LinkQuery a, RICON_LinkQuery b)
	{
		if (!a || !b)
			return !a && !b;
		if (!SameVector(a.m_vTxPos, b.m_vTxPos) || !SameVector(a.m_vRxPos, b.m_vRxPos))
			return false;
		if (!RICON_TestSupport.SameFloat(a.m_fFrequencyMHz, b.m_fFrequencyMHz)
			|| !RICON_TestSupport.SameFloat(a.m_fBandwidthHz, b.m_fBandwidthHz))
			return false;
		if (!RICON_TestSupport.SameFloat(a.m_fTxPowerDbm, b.m_fTxPowerDbm)
			|| !RICON_TestSupport.SameFloat(a.m_fTxGainDbi, b.m_fTxGainDbi))
			return false;
		if (!RICON_TestSupport.SameFloat(a.m_fRxGainDbi, b.m_fRxGainDbi)
			|| !RICON_TestSupport.SameFloat(a.m_fRxNoiseFigureDb, b.m_fRxNoiseFigureDb))
			return false;
		if (a.m_bHasRxSensitivity != b.m_bHasRxSensitivity
			|| !RICON_TestSupport.SameFloat(a.m_fRxSensitivityDbm, b.m_fRxSensitivityDbm))
			return false;
		if (a.m_bHasRequiredSinr != b.m_bHasRequiredSinr
			|| !RICON_TestSupport.SameFloat(a.m_fRequiredSinrDb, b.m_fRequiredSinrDb))
			return false;
		if (a.m_bHasInterference != b.m_bHasInterference
			|| !RICON_TestSupport.SameFloat(a.m_fInterferenceDbm, b.m_fInterferenceDbm))
			return false;
		if (a.m_sBandId != b.m_sBandId || a.m_sModelId != b.m_sModelId)
			return false;
		return a.m_sNoiseEnvironmentId == b.m_sNoiseEnvironmentId
			&& a.m_sIntelligibilityModelId == b.m_sIntelligibilityModelId;
	}

	static bool AllFinite(RICON_LinkQuery q)
	{
		if (!RICON_Num.IsFiniteVector(q.m_vTxPos) || !RICON_Num.IsFiniteVector(q.m_vRxPos))
			return false;
		if (!RICON_Num.IsFinite(q.m_fFrequencyMHz) || !RICON_Num.IsFinite(q.m_fBandwidthHz))
			return false;
		if (!RICON_Num.IsFinite(q.m_fTxPowerDbm) || !RICON_Num.IsFinite(q.m_fTxGainDbi))
			return false;
		if (!RICON_Num.IsFinite(q.m_fRxGainDbi) || !RICON_Num.IsFinite(q.m_fRxNoiseFigureDb))
			return false;
		if (!RICON_Num.IsFinite(q.m_fRxSensitivityDbm) || !RICON_Num.IsFinite(q.m_fRequiredSinrDb))
			return false;
		return RICON_Num.IsFinite(q.m_fInterferenceDbm);
	}

	//! Отрицательные контроли: сравнение обязано отвергнуть каждое искажение ожидания R01.
	protected void Controls(RICON_LinkQuery actual, RICON_LinkQuery expected)
	{
		RICON_LinkQuery probe = CopyQuery(expected);
		probe.m_vTxPos = Vector(probe.m_vTxPos[0], probe.m_vTxPos[1] + 0.001, probe.m_vTxPos[2]);
		Reject("высота TX", SameQuery(actual, probe));
		probe = CopyQuery(expected);
		probe.m_fBandwidthHz = 25000;
		Reject("полоса взята у отправителя", SameQuery(actual, probe));
		probe = CopyQuery(expected);
		probe.m_fTxPowerDbm = 30;
		Reject("мощность взята у приёмника", SameQuery(actual, probe));
		probe = CopyQuery(expected);
		probe.m_fRxGainDbi = 7;
		Reject("усиление RX взято у отправителя", SameQuery(actual, probe));
		probe = CopyQuery(expected);
		probe.m_fRxNoiseFigureDb = 12;
		Reject("NF взят у отправителя", SameQuery(actual, probe));
		probe = CopyQuery(expected);
		probe.m_fRequiredSinrDb = 14;
		Reject("порог взят у отправителя", SameQuery(actual, probe));
		probe = CopyQuery(expected);
		probe.m_sBandId = "other";
		Reject("bandId", SameQuery(actual, probe));
		probe = CopyQuery(expected);
		probe.m_fRequiredSinrDb = RICON_L2Run.NaN();
		Reject("NaN в ожидании", SameQuery(actual, probe));
		Reject("actual null", SameQuery(null, expected));
	}

	static RICON_LinkQuery CopyQuery(RICON_LinkQuery source)
	{
		RICON_LinkQuery copy = new RICON_LinkQuery();
		copy.m_vTxPos = source.m_vTxPos;
		copy.m_vRxPos = source.m_vRxPos;
		copy.m_fFrequencyMHz = source.m_fFrequencyMHz;
		copy.m_fBandwidthHz = source.m_fBandwidthHz;
		copy.m_fTxPowerDbm = source.m_fTxPowerDbm;
		copy.m_fTxGainDbi = source.m_fTxGainDbi;
		copy.m_fRxGainDbi = source.m_fRxGainDbi;
		copy.m_fRxNoiseFigureDb = source.m_fRxNoiseFigureDb;
		copy.m_bHasRxSensitivity = source.m_bHasRxSensitivity;
		copy.m_fRxSensitivityDbm = source.m_fRxSensitivityDbm;
		copy.m_bHasRequiredSinr = source.m_bHasRequiredSinr;
		copy.m_fRequiredSinrDb = source.m_fRequiredSinrDb;
		copy.m_bHasInterference = source.m_bHasInterference;
		copy.m_fInterferenceDbm = source.m_fInterferenceDbm;
		copy.m_sBandId = source.m_sBandId;
		copy.m_sModelId = source.m_sModelId;
		copy.m_sNoiseEnvironmentId = source.m_sNoiseEnvironmentId;
		copy.m_sIntelligibilityModelId = source.m_sIntelligibilityModelId;
		return copy;
	}

	void CheckCase(string id, RICON_RadioObservation txObservation, RICON_RadioRfProfile txProfile,
		RICON_RadioObservation rxObservation, RICON_RadioRfProfile rxProfile, RICON_RadioQueryContext context,
		RICON_ERadioQueryCode expectedCode, RICON_LinkQuery expected)
	{
		m_iCases++;
		if (id == "R01")
		{
			m_R01TxObservation = CopyObservation(txObservation);
			m_R01TxProfile = CopyProfile(txProfile);
			m_R01RxObservation = CopyObservation(rxObservation);
			m_R01RxProfile = CopyProfile(rxProfile);
			m_R01Context = CopyContext(context);
		}
		RICON_RadioObservation txBefore = CopyObservation(txObservation);
		RICON_RadioRfProfile txProfileBefore = CopyProfile(txProfile);
		RICON_RadioObservation rxBefore = CopyObservation(rxObservation);
		RICON_RadioRfProfile rxProfileBefore = CopyProfile(rxProfile);
		RICON_RadioQueryContext contextBefore = CopyContext(context);

		// Запрос заранее не null: отказ обязан его обнулить.
		RICON_LinkQuery actual = new RICON_LinkQuery();
		RICON_ERadioQueryCode code = RICON_RadioQueryBuilder.Build(txObservation, txProfile, rxObservation, rxProfile,
			context, actual);
		bool match = code == expectedCode && SameQuery(actual, expected) && (!actual || AllFinite(actual));
		Require(match, id, "совпадение с эталоном");
		if (match)
			m_iQuietPositive++;

		bool unchanged = SameObservation(txObservation, txBefore) && SameProfile(txProfile, txProfileBefore)
			&& SameObservation(rxObservation, rxBefore) && SameProfile(rxProfile, rxProfileBefore)
			&& SameContext(context, contextBefore);
		if (actual)
		{
			// Изменение выхода не должно дойти до входов; второй вызов даёт новый объект.
			actual.m_vTxPos = Vector(-1, -1, -1);
			actual.m_vRxPos = Vector(-2, -2, -2);
			actual.m_fTxPowerDbm = -999;
			actual.m_sBandId = "mutated";
			RICON_LinkQuery second;
			RICON_RadioQueryBuilder.Build(txObservation, txProfile, rxObservation, rxProfile, context, second);
			unchanged = unchanged && second && second != actual && SameQuery(second, expected)
				&& SameObservation(txObservation, txBefore) && SameObservation(rxObservation, rxBefore)
				&& SameContext(context, contextBefore);
			if (id == "R01" && second)
				Controls(second, expected);
		}
		Require(unchanged, id, "входы не изменены, запрос новый");
		if (unchanged)
			m_iImmutable++;
		PrintFormat("[RICON_RADIO] %1 code=%2 expected=%3 match=%4 unchanged=%5", id,
			typename.EnumToString(RICON_ERadioQueryCode, code), typename.EnumToString(RICON_ERadioQueryCode, expectedCode),
			match, unchanged);
	}

	//! Сквозная цепочка R01: S2 → S1 (ровный источник 100 м) → L2 → evaluator. Ожидания скопированы из
	//! oracle/test_radio_device_oracle.py:155–170 (codex), здесь не вычисляются.
	protected RICON_LinkBudget Evaluate(RICON_LinkQuery query, out int heightCalls)
	{
		RICON_AnalyticHeightSource source = new RICON_AnalyticHeightSource();
		source.m_fBase = 100;
		RICON_WorldProfileProvider provider = new RICON_WorldProfileProvider(source);
		RICON_TerrainDiffractionConfig config = RICON_L2Run.Config(6371, 1.3333334, 15, 0.001,
			RICON_ETerrainPolarization.VERTICAL, 0.1, 1000);
		RICON_TerrainDiffractionModel model = new RICON_TerrainDiffractionModel(config, provider);
		RICON_WirelessLinkEvaluator evaluator = new RICON_WirelessLinkEvaluator(query.m_sBandId, model,
			new RICON_NoiseModel(), RICON_TestSupport.NewTestIntelligibility());
		RICON_LinkBudget budget = evaluator.Evaluate(query);
		heightCalls = source.m_iCalls;
		return budget;
	}

	void CheckChain()
	{
		RICON_RadioObservation txObservation = m_R01TxObservation;
		RICON_RadioRfProfile txProfile = m_R01TxProfile;
		RICON_RadioObservation rxObservation = m_R01RxObservation;
		RICON_RadioRfProfile rxProfile = m_R01RxProfile;
		RICON_RadioQueryContext context = m_R01Context;
		if (!txObservation)
		{
			Require(false, "radio_chain", "корпус не дал входы R01");
			return;
		}
		RICON_LinkQuery query;
		bool ready = RICON_RadioQueryBuilder.Build(txObservation, txProfile, rxObservation, rxProfile, context, query)
			== RICON_ERadioQueryCode.READY;
		int calls;
		RICON_LinkBudget budget;
		if (ready)
			budget = Evaluate(query, calls);
		bool ok = ready && budget && budget.m_bValid && budget.m_eReason == RICON_EReason.OK && calls == 21
			&& Math.AbsFloat(budget.m_fTotalLossDb - 108.1797261261363) <= 0.05
			&& Math.AbsFloat(budget.m_fRxPowerDbm - (-66.1797261261363)) <= 0.05
			&& Math.AbsFloat(budget.m_fSinrDb - 58.82636093801125) <= 0.05
			&& budget.m_eState == RICON_ELinkState.CLEAN;
		Require(ok, "radio_chain", "R01 → профиль → потери → мощность → SINR");
		if (ok)
			m_iChains++;
		if (budget)
			PrintFormat("[RICON_RADIO] chain loss=%1 rx=%2 sinr=%3 reason=%4 state=%5 calls=%6",
				budget.m_fTotalLossDb.ToString(-1, 9), budget.m_fRxPowerDbm.ToString(-1, 9),
				budget.m_fSinrDb.ToString(-1, 9), budget.m_eReason, budget.m_eState, calls);

		// Ноги на поверхности: сборщик не поднимает антенну, ядро отказывает штатно.
		RICON_RadioObservation feet = CopyObservation(txObservation);
		feet.m_vAntennaPos = Vector(feet.m_vAntennaPos[0], 100, feet.m_vAntennaPos[2]);
		RICON_LinkQuery feetQuery;
		bool feetReady = RICON_RadioQueryBuilder.Build(feet, txProfile, rxObservation, rxProfile, context, feetQuery)
			== RICON_ERadioQueryCode.READY;
		int feetCalls;
		RICON_LinkBudget feetBudget;
		if (feetReady)
			feetBudget = Evaluate(feetQuery, feetCalls);
		bool feetOk = feetReady && feetQuery.m_vTxPos[1] == 100 && feetBudget && !feetBudget.m_bValid
			&& feetBudget.m_eReason == RICON_EReason.TERRAIN_PROFILE_UNAVAILABLE;
		Require(feetOk, "radio_chain", "антенна на уровне земли: READY, затем отказ L2 без подъёма");
		if (feetOk)
			m_iChains++;
		if (feetBudget)
			PrintFormat("[RICON_RADIO] feet ready=%1 txY=%2 valid=%3 reason=%4", feetReady,
				feetQuery.m_vTxPos[1].ToString(-1, 3), feetBudget.m_bValid, feetBudget.m_eReason);
	}

	//! Безопасная ветвь native reader без настоящей рации: отказ, observation = null.
	void CheckReader()
	{
		RICON_RadioObservation observation = new RICON_RadioObservation();
		bool read = RICON_BaseRadioObservationReader.TryRead(null, 1, 0, Vector(0, 2, 0), observation);
		bool ok = !read && !observation;
		Require(ok, "radio_reader", "нет рации: отказ без observation");
		if (ok)
			m_iReaderChecks++;
		PrintFormat("[RICON_RADIO] reader null radio read=%1 observationNull=%2 authority=%3", read, !observation,
			RICON_CommsManagerComponent.IsAuthority());
	}
}
