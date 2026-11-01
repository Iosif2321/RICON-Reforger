//! TOOL_ONLY D0: readiness control plane only. RPCs contain run/challenge/readiness generations and lifetime;
//! never event names, callback outcomes, payload, audio, radio state or symbol timing.
modded class SCR_PlayerController
{
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RICON_D0ReadinessChallenge(string runId, int challengeId, int attempt)
	{
		if (GetPlayerId() != RICON_LiveCommunicationDiscovery.ROLE_B
			|| GetGame().GetPlayerController() != this)
			return;
		RICON_LiveCommunicationDiscovery probe = RICON_LiveCommunicationDiscovery.Active();
		if (!probe)
			return;
		int remainingMs;
		int playerId;
		int entityGeneration;
		int componentGeneration;
		int subscriptionGeneration;
		if (!probe.BuildReadinessAck(runId, challengeId, attempt, remainingMs, playerId, entityGeneration,
			componentGeneration, subscriptionGeneration))
			return;
		Rpc(RpcAsk_RICON_D0ReadinessAck, runId, challengeId, attempt, remainingMs, playerId,
			entityGeneration, componentGeneration, subscriptionGeneration);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_D0ReadinessAck(string runId, int challengeId, int attempt,
		int remainingMs, int playerId, int entityGeneration, int componentGeneration, int subscriptionGeneration)
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!Replication.IsServer() || !manager || GetPlayerId() != RICON_LiveCommunicationDiscovery.ROLE_B
			|| playerId != GetPlayerId() || manager.GetPlayerController(RICON_LiveCommunicationDiscovery.ROLE_B) != this)
			return;
		RICON_LiveCommunicationDiscovery probe = RICON_LiveCommunicationDiscovery.Active();
		if (!probe)
			return;
		probe.AcceptReadinessAck(this, runId, challengeId, attempt, remainingMs, playerId,
			entityGeneration, componentGeneration, subscriptionGeneration);
	}

	//! Server/host A addresses the already-owned B controller; this method carries no discovery result.
	bool RICON_D0SendReadinessChallenge(string runId, int challengeId, int attempt)
	{
		if (!Replication.IsServer() || GetPlayerId() != RICON_LiveCommunicationDiscovery.ROLE_B
			|| !RICON_D0Gate.ValidRunId(runId) || challengeId < 1 || challengeId > 2 || attempt < 1)
			return false;
		Rpc(RpcDo_RICON_D0ReadinessChallenge, runId, challengeId, attempt);
		return true;
	}
}
