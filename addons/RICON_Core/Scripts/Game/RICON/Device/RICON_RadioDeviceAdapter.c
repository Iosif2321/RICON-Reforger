//! S2, REQ-radio-device-adapter v0.2: наблюдение рации, RF-профиль конца и сборка запроса одной направленной пары.
//! Сборщик чистый и исполняется без мира. Оболочка native API — только чтение, вызов с настоящей рацией не проверен.

//! Нормативный порядок проверок §3; первый применимый код. Значение 0 — безопасный отказ по умолчанию.
enum RICON_ERadioQueryCode
{
	NULL_INPUT = 0,
	INVALID_OBSERVATION = 1,
	INVALID_PROFILE = 2,
	INVALID_CONTEXT = 3,
	PROFILE_MISMATCH = 4,
	POWER_OFF = 5,
	TUNING_MISMATCH = 6,
	CHANNEL_MISMATCH = 7,
	KEY_MISMATCH = 8,
	READY = 9
}

//! Наблюдаемое состояние одного endpoint (§2.1). deviceId и точку антенны даёт серверный binding.
class RICON_RadioObservation
{
	int m_iDeviceId;
	int m_iTransceiverIndex = -1;
	bool m_bPowered;
	int m_iTuningValue;			//!< непрозрачное native значение; единица не установлена
	string m_sNetworkKey;		//!< не выводить в диагностические логи
	vector m_vAntennaPos;		//!< m, world space, точка антенны (REQ-WP-06)
}

//! RF-параметры одного конца (§2.2). Отдельный объект на каждое устройство; значений по умолчанию нет.
class RICON_RadioRfProfile
{
	static const string PROVENANCE_TEST_ONLY = "TEST_ONLY";
	static const string PROVENANCE_ASSUMED = "ASSUMED";

	string m_sProfileId;
	int m_iDeviceId;
	int m_iTransceiverIndex = -1;
	string m_sProvenance;
	int m_iExpectedTuningValue;
	float m_fFrequencyMHz;
	float m_fRxBandwidthHz;
	float m_fTxPowerDbm;
	float m_fTxGainDbi;
	float m_fRxGainDbi;
	float m_fRxNoiseFigureDb;
	bool m_bHasRxSensitivity;
	float m_fRxSensitivityDbm;
	bool m_bHasRequiredSinr;
	float m_fRequiredSinrDb;
}

//! Идентификаторы моделей и помеха для пары (§2.3). Разрешение ID проверяет существующий evaluator.
class RICON_RadioQueryContext
{
	string m_sBandId;
	string m_sModelId;
	string m_sNoiseEnvironmentId;
	string m_sIntelligibilityModelId;
	bool m_bHasInterference;
	float m_fInterferenceDbm;
}

class RICON_RadioQueryBuilder
{
	protected static bool IsIdentity(int value, int minimum)
	{
		return value >= minimum && value < int.MAX;
	}

	static bool IsValidObservation(RICON_RadioObservation observation)
	{
		return IsIdentity(observation.m_iDeviceId, 1) && IsIdentity(observation.m_iTransceiverIndex, 0)
			&& RICON_Num.IsFiniteVector(observation.m_vAntennaPos);
	}

	static bool IsValidProfile(RICON_RadioRfProfile profile)
	{
		if (profile.m_sProfileId.IsEmpty() || !IsIdentity(profile.m_iDeviceId, 1)
			|| !IsIdentity(profile.m_iTransceiverIndex, 0))
			return false;
		if (profile.m_sProvenance != RICON_RadioRfProfile.PROVENANCE_TEST_ONLY
			&& profile.m_sProvenance != RICON_RadioRfProfile.PROVENANCE_ASSUMED)
			return false;
		if (profile.m_bHasRxSensitivity == profile.m_bHasRequiredSinr)
			return false;
		if (!RICON_Num.IsFinite(profile.m_fFrequencyMHz) || !RICON_Num.IsFinite(profile.m_fRxBandwidthHz)
			|| !RICON_Num.IsFinite(profile.m_fTxPowerDbm) || !RICON_Num.IsFinite(profile.m_fTxGainDbi)
			|| !RICON_Num.IsFinite(profile.m_fRxGainDbi) || !RICON_Num.IsFinite(profile.m_fRxNoiseFigureDb))
			return false;
		if (profile.m_bHasRxSensitivity && !RICON_Num.IsFinite(profile.m_fRxSensitivityDbm))
			return false;
		if (profile.m_bHasRequiredSinr && !RICON_Num.IsFinite(profile.m_fRequiredSinrDb))
			return false;
		return profile.m_fFrequencyMHz > 0 && profile.m_fRxBandwidthHz > 0 && profile.m_fRxNoiseFigureDb >= 0;
	}

	static bool IsValidContext(RICON_RadioQueryContext context)
	{
		if (context.m_sBandId.IsEmpty() || context.m_sModelId.IsEmpty() || context.m_sNoiseEnvironmentId.IsEmpty()
			|| context.m_sIntelligibilityModelId.IsEmpty())
			return false;
		return !context.m_bHasInterference || RICON_Num.IsFinite(context.m_fInterferenceDbm);
	}

	protected static bool Matches(RICON_RadioObservation observation, RICON_RadioRfProfile profile)
	{
		return observation.m_iDeviceId == profile.m_iDeviceId
			&& observation.m_iTransceiverIndex == profile.m_iTransceiverIndex;
	}

	//! TX-поля берутся из профиля отправителя, RX-поля — из профиля приёмника (§4). Отказ: query = null.
	//! Одно устройство на обоих концах не исключается: отбор пар — S3.
	static RICON_ERadioQueryCode Build(RICON_RadioObservation txObservation, RICON_RadioRfProfile txProfile,
		RICON_RadioObservation rxObservation, RICON_RadioRfProfile rxProfile, RICON_RadioQueryContext context,
		out RICON_LinkQuery query)
	{
		query = null;
		if (!txObservation || !txProfile || !rxObservation || !rxProfile || !context)
			return RICON_ERadioQueryCode.NULL_INPUT;
		if (!IsValidObservation(txObservation) || !IsValidObservation(rxObservation))
			return RICON_ERadioQueryCode.INVALID_OBSERVATION;
		if (!IsValidProfile(txProfile) || !IsValidProfile(rxProfile))
			return RICON_ERadioQueryCode.INVALID_PROFILE;
		if (!IsValidContext(context))
			return RICON_ERadioQueryCode.INVALID_CONTEXT;
		if (!Matches(txObservation, txProfile) || !Matches(rxObservation, rxProfile))
			return RICON_ERadioQueryCode.PROFILE_MISMATCH;
		if (!txObservation.m_bPowered || !rxObservation.m_bPowered)
			return RICON_ERadioQueryCode.POWER_OFF;
		if (txObservation.m_iTuningValue != txProfile.m_iExpectedTuningValue
			|| rxObservation.m_iTuningValue != rxProfile.m_iExpectedTuningValue)
			return RICON_ERadioQueryCode.TUNING_MISMATCH;
		if (txProfile.m_fFrequencyMHz != rxProfile.m_fFrequencyMHz)
			return RICON_ERadioQueryCode.CHANNEL_MISMATCH;
		// Политика сборщика: точное равенство непрозрачных строк, две пустые строки равны. Не доказательство шифрования.
		if (txObservation.m_sNetworkKey != rxObservation.m_sNetworkKey)
			return RICON_ERadioQueryCode.KEY_MISMATCH;

		RICON_LinkQuery result = new RICON_LinkQuery();
		result.m_vTxPos = txObservation.m_vAntennaPos;
		result.m_vRxPos = rxObservation.m_vAntennaPos;
		result.m_fFrequencyMHz = txProfile.m_fFrequencyMHz;
		result.m_fBandwidthHz = rxProfile.m_fRxBandwidthHz;
		result.m_fTxPowerDbm = txProfile.m_fTxPowerDbm;
		result.m_fTxGainDbi = txProfile.m_fTxGainDbi;
		result.m_fRxGainDbi = rxProfile.m_fRxGainDbi;
		result.m_fRxNoiseFigureDb = rxProfile.m_fRxNoiseFigureDb;
		result.m_bHasRxSensitivity = rxProfile.m_bHasRxSensitivity;
		result.m_fRxSensitivityDbm = 0;
		if (rxProfile.m_bHasRxSensitivity)
			result.m_fRxSensitivityDbm = rxProfile.m_fRxSensitivityDbm;
		result.m_bHasRequiredSinr = rxProfile.m_bHasRequiredSinr;
		result.m_fRequiredSinrDb = 0;
		if (rxProfile.m_bHasRequiredSinr)
			result.m_fRequiredSinrDb = rxProfile.m_fRequiredSinrDb;
		result.m_bHasInterference = context.m_bHasInterference;
		result.m_fInterferenceDbm = 0;
		if (context.m_bHasInterference)
			result.m_fInterferenceDbm = context.m_fInterferenceDbm;
		result.m_sBandId = context.m_sBandId;
		result.m_sModelId = context.m_sModelId;
		result.m_sNoiseEnvironmentId = context.m_sNoiseEnvironmentId;
		result.m_sIntelligibilityModelId = context.m_sIntelligibilityModelId;
		query = result;
		return RICON_ERadioQueryCode.READY;
	}
}

//! Тонкая read-only оболочка над BaseRadioComponent (§5). Не назначает ID и не регистрируется на рациях.
//! shortcut: getters читаются по очереди, атомарного снимка нет; актуальность binding проверяют lifecycle и S3.
class RICON_BaseRadioObservationReader
{
	static bool TryRead(BaseRadioComponent radio, int deviceId, int transceiverIndex, vector antennaPos,
		out RICON_RadioObservation observation)
	{
		observation = null;
		if (!RICON_CommsManagerComponent.IsAuthority())
			return false;
		if (!radio || deviceId < 1 || deviceId >= int.MAX || transceiverIndex < 0 || transceiverIndex >= int.MAX
			|| !RICON_Num.IsFiniteVector(antennaPos))
			return false;
		if (!radio.GetOwner() || transceiverIndex >= radio.TransceiversCount())
			return false;
		BaseTransceiver transceiver = radio.GetTransceiver(transceiverIndex);
		if (!transceiver || transceiver.GetRadio() != radio)
			return false;
		RICON_RadioObservation result = new RICON_RadioObservation();
		result.m_iDeviceId = deviceId;
		result.m_iTransceiverIndex = transceiverIndex;
		result.m_bPowered = radio.IsPowered();
		result.m_iTuningValue = transceiver.GetFrequency();
		result.m_sNetworkKey = radio.GetEncryptionKey();
		result.m_vAntennaPos = antennaPos;
		observation = result;
		return true;
	}
}
