// Authority lifecycle and registries for the common RICON core (REQ-core §3, §7, §8.1).
// Status: STATIC_IMPLEMENTED (not compiled yet). No production models are registered implicitly.

class RICON_CommsManagerComponentClass : SCR_BaseGameModeComponentClass
{
}

//! Immutable input used to build one band-scoped evaluator after all model ids are resolved.
class RICON_BandConfig
{
	string m_sBandId;
	string m_sPropagationModelId;
	string m_sNoiseEnvironmentId;
	string m_sIntelligibilityModelId;
	int m_iConfigVersion;

	void RICON_BandConfig(
		string bandId,
		string propagationModelId,
		string noiseEnvironmentId,
		string intelligibilityModelId,
		int configVersion)
	{
		m_sBandId = bandId;
		m_sPropagationModelId = propagationModelId;
		m_sNoiseEnvironmentId = noiseEnvironmentId;
		m_sIntelligibilityModelId = intelligibilityModelId;
		m_iConfigVersion = configVersion;
	}

	bool IsValid()
	{
		return !m_sBandId.IsEmpty() && !m_sPropagationModelId.IsEmpty() && !m_sNoiseEnvironmentId.IsEmpty()
			&& !m_sIntelligibilityModelId.IsEmpty() && m_iConfigVersion > 0;
	}
}

//! Noise model plus the provenance of its versioned coefficients (REQ-core §7).
class RICON_NoiseRegistration
{
	ref RICON_NoiseModel m_Model;
	string m_sSourceId;
	int m_iSourceRevision;

	void RICON_NoiseRegistration(RICON_NoiseModel model, string sourceId, int sourceRevision)
	{
		m_Model = model;
		m_sSourceId = sourceId;
		m_iSourceRevision = sourceRevision;
	}

	bool IsValid()
	{
		return m_Model && !m_Model.GetEnvironmentId().IsEmpty() && !m_sSourceId.IsEmpty() && m_iSourceRevision > 0;
	}
}

class RICON_CommsManagerComponent : SCR_BaseGameModeComponent
{
	// Process-local generation. It only distinguishes live world/session generations, not all historical servers.
	protected static int s_iProcessEpoch;

	protected int m_iSessionEpoch;
	protected bool m_bRegistriesFrozen;
	protected string m_sLastRegistryError;

	protected ref map<string, ref RICON_PropagationModel> m_mPropagationById = new map<string, ref RICON_PropagationModel>();
	protected ref map<string, ref RICON_NoiseRegistration> m_mNoiseByEnvironment = new map<string, ref RICON_NoiseRegistration>();
	protected ref map<string, ref RICON_IntelligibilityModel> m_mIntelligibilityById = new map<string, ref RICON_IntelligibilityModel>();
	protected ref map<string, ref RICON_BandConfig> m_mBandConfigById = new map<string, ref RICON_BandConfig>();
	protected ref map<string, ref RICON_WirelessLinkEvaluator> m_mEvaluatorByBand = new map<string, ref RICON_WirelessLinkEvaluator>();
	protected ref set<int> m_sConnectedPlayers = new set<int>();

	static RICON_CommsManagerComponent GetInstance()
	{
		ChimeraGame game = GetGame();
		if (!game)
			return null;

		BaseGameMode gameMode = game.GetGameMode();
		if (!gameMode)
			return null;
		return RICON_CommsManagerComponent.Cast(gameMode.FindComponent(RICON_CommsManagerComponent));
	}

	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		if (!IsAuthority())
			return;

		m_iSessionEpoch = AllocateSessionEpoch();
		m_sLastRegistryError = string.Empty;
	}

	//! OnPlayerConnected is documented server-only.
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		m_sConnectedPlayers.Insert(playerId);
	}

	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		if (IsAuthority())
			m_sConnectedPlayers.RemoveItem(playerId);
	}

	static bool IsAuthority()
	{
		return Replication.IsServer();
	}

	//! Register before FreezeRegistries. Duplicate/empty/null ids fail closed; no default fallback.
	bool RegisterPropagation(RICON_PropagationModel model)
	{
		if (!CanMutateRegistries("RegisterPropagation"))
			return false;
		if (!model || model.GetModelId().IsEmpty())
			return RegistryFailure("RegisterPropagation: null model or empty model id");
		if (m_mPropagationById.Contains(model.GetModelId()))
			return RegistryFailure("RegisterPropagation: duplicate model id");
		m_mPropagationById.Set(model.GetModelId(), model);
		return true;
	}

	bool RegisterNoise(RICON_NoiseModel model, string sourceId, int sourceRevision)
	{
		if (!CanMutateRegistries("RegisterNoise"))
			return false;
		RICON_NoiseRegistration registration = new RICON_NoiseRegistration(model, sourceId, sourceRevision);
		if (!registration.IsValid())
			return RegistryFailure("RegisterNoise: invalid model or source metadata");
		string environmentId = model.GetEnvironmentId();
		if (m_mNoiseByEnvironment.Contains(environmentId))
			return RegistryFailure("RegisterNoise: duplicate environment id");
		m_mNoiseByEnvironment.Set(environmentId, registration);
		return true;
	}

	bool RegisterIntelligibility(RICON_IntelligibilityModel model)
	{
		if (!CanMutateRegistries("RegisterIntelligibility"))
			return false;
		if (!model || model.GetModelId().IsEmpty() || !model.IsConfigValid())
			return RegistryFailure("RegisterIntelligibility: invalid model or config");
		if (m_mIntelligibilityById.Contains(model.GetModelId()))
			return RegistryFailure("RegisterIntelligibility: duplicate model id");
		m_mIntelligibilityById.Set(model.GetModelId(), model);
		return true;
	}

	//! Resolve one immutable Band -> dependencies mapping and create the pure evaluator.
	bool RegisterBand(RICON_BandConfig config)
	{
		if (!CanMutateRegistries("RegisterBand"))
			return false;
		if (!config || !config.IsValid())
			return RegistryFailure("RegisterBand: invalid config");
		if (m_mBandConfigById.Contains(config.m_sBandId))
			return RegistryFailure("RegisterBand: duplicate band id");

		RICON_PropagationModel propagation = m_mPropagationById.Get(config.m_sPropagationModelId);
		RICON_NoiseRegistration noiseRegistration = m_mNoiseByEnvironment.Get(config.m_sNoiseEnvironmentId);
		RICON_IntelligibilityModel intelligibility = m_mIntelligibilityById.Get(config.m_sIntelligibilityModelId);
		if (!propagation || !noiseRegistration || !intelligibility)
			return RegistryFailure("RegisterBand: one or more dependency ids are unknown");

		// Store an internal copy so the caller cannot mutate the registered mapping after validation.
		RICON_BandConfig stored = new RICON_BandConfig(
			config.m_sBandId,
			config.m_sPropagationModelId,
			config.m_sNoiseEnvironmentId,
			config.m_sIntelligibilityModelId,
			config.m_iConfigVersion);
		RICON_WirelessLinkEvaluator evaluator = new RICON_WirelessLinkEvaluator(
			stored.m_sBandId,
			propagation,
			noiseRegistration.m_Model,
			intelligibility);
		m_mBandConfigById.Set(stored.m_sBandId, stored);
		m_mEvaluatorByBand.Set(stored.m_sBandId, evaluator);
		return true;
	}

	//! No registration is allowed after freeze. At least one fully resolved band is required.
	bool FreezeRegistries()
	{
		if (!CanMutateRegistries("FreezeRegistries"))
			return false;
		if (m_mEvaluatorByBand.IsEmpty())
			return RegistryFailure("FreezeRegistries: no resolved bands");
		m_bRegistriesFrozen = true;
		return true;
	}

	RICON_WirelessLinkEvaluator GetBandEvaluator(string bandId)
	{
		if (!m_bRegistriesFrozen || bandId.IsEmpty())
			return null;
		return m_mEvaluatorByBand.Get(bandId);
	}

	string GetNoiseSourceId(string environmentId)
	{
		RICON_NoiseRegistration registration = m_mNoiseByEnvironment.Get(environmentId);
		if (!registration)
			return string.Empty;
		return registration.m_sSourceId;
	}

	int GetNoiseSourceRevision(string environmentId)
	{
		RICON_NoiseRegistration registration = m_mNoiseByEnvironment.Get(environmentId);
		if (!registration)
			return 0;
		return registration.m_iSourceRevision;
	}

	bool AreRegistriesFrozen()
	{
		return m_bRegistriesFrozen;
	}

	string GetLastRegistryError()
	{
		return m_sLastRegistryError;
	}

	int GetSessionEpoch()
	{
		return m_iSessionEpoch;
	}

	//! Used before linkSeq would reach int.MAX. Full-resync effects are handled by the snapshot service.
	bool RotateSessionEpoch()
	{
		if (!IsAuthority())
			return false;
		int previous = m_iSessionEpoch;
		m_iSessionEpoch = AllocateSessionEpoch();
		return m_iSessionEpoch != 0 && m_iSessionEpoch != previous;
	}

	protected bool CanMutateRegistries(string operation)
	{
		if (!IsAuthority())
			return RegistryFailure(operation + ": authority required");
		if (m_bRegistriesFrozen)
			return RegistryFailure(operation + ": registries are frozen");
		return true;
	}

	protected bool RegistryFailure(string message)
	{
		m_sLastRegistryError = message;
		PrintFormat("[RICON] registry rejected: %1", message);
		return false;
	}

	//! Starts with Unix seconds and then increments process-locally across world/session generations.
	//! If script state is rebuilt, clients must establish a fresh session; global historical uniqueness is not claimed.
	protected static int AllocateSessionEpoch()
	{
		if (s_iProcessEpoch <= 0 || s_iProcessEpoch >= int.MAX - 1)
		{
			s_iProcessEpoch = System.GetUnixTime();
			if (s_iProcessEpoch <= 0 || s_iProcessEpoch >= int.MAX)
				s_iProcessEpoch = 1;
			return s_iProcessEpoch;
		}

		s_iProcessEpoch++;
		return s_iProcessEpoch;
	}
}
