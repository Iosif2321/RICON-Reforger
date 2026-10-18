// Server-authoritative comms manager attached to the GameMode entity (ADR-002, PLAN v5 Э3).
// Status: STATIC_IMPLEMENTED/COMPILE_ONLY. Lifecycle hooks only; registries grow in Э3.

class RICON_CommsManagerComponentClass : SCR_BaseGameModeComponentClass
{
}

class RICON_CommsManagerComponent : SCR_BaseGameModeComponent
{
	//! Bumped on every mission start; snapshots from an older epoch are rejected (R-01).
	protected int m_iSessionEpoch;
	//! Band id -> propagation model (PLAN v5 §3: Band -> PropagationModel registry).
	protected ref map<string, ref RICON_PropagationModel> m_mModelsByBand = new map<string, ref RICON_PropagationModel>();
	//! Player state is keyed by PlayerId, never by entity (lesson R-02).
	protected ref set<int> m_sConnectedPlayers = new set<int>();

	static RICON_CommsManagerComponent GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;
		return RICON_CommsManagerComponent.Cast(gameMode.FindComponent(RICON_CommsManagerComponent));
	}

	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		if (!Replication.IsServer())
			return;

		m_iSessionEpoch++;
		RegisterModel("default", new RICON_FreeSpaceModel());
	}

	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		if (Replication.IsServer())
			m_sConnectedPlayers.Insert(playerId);
	}

	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		if (Replication.IsServer())
			m_sConnectedPlayers.RemoveItem(playerId);
	}

	//! Server-only registration of a propagation model for a band id.
	void RegisterModel(string bandId, RICON_PropagationModel model)
	{
		if (!Replication.IsServer() || !model)
			return;
		m_mModelsByBand.Set(bandId, model);
	}

	RICON_PropagationModel GetModel(string bandId)
	{
		RICON_PropagationModel model = m_mModelsByBand.Get(bandId);
		if (!model)
			model = m_mModelsByBand.Get("default");
		return model;
	}

	int GetSessionEpoch()
	{
		return m_iSessionEpoch;
	}
}
