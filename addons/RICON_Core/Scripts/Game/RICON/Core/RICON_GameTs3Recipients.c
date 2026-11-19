//! Detached TS3 metadata for a caller-attested current RF selection. It grants no media permission.
class RICON_GameTs3RecipientSnapshot
{
	ref RICON_GameTs3MatchedBinding m_Source;
	ref array<ref RICON_GameTs3MatchedBinding> m_Recipients = {};
	int m_iSampleMs, m_iDeadlineMs, m_iLeaseId, m_iTxDeviceId, m_iTxTransceiverIndex;
	int m_iTxGeneration, m_iNetworkId, m_iChannelId, m_iIssuedAtMs;

	static RICON_GameTs3RecipientSnapshot Build(RICON_GameTs3BindingStore store, RICON_GameRadioPttEmission emission,
		array<ref RICON_GameRadioPttContext> admitted, int nowMs)
	{
		if (!store || !emission || !admitted || admitted.IsEmpty() || !emission.m_Context || !emission.m_Context.IsValid()) return null;
		if (emission.m_iLeaseId <= 0 || emission.m_iTxGeneration <= 0 || emission.m_iDeviceId <= 0
			|| emission.m_iTxTransceiverIndex < 0 || emission.m_iTxTransceiverIndex >= int.MAX
			|| emission.m_iNetworkId <= 0 || emission.m_iChannelId <= 0) return null;
		if (emission.m_iIssuedAtMs < 0 || emission.m_iServerNowMs < emission.m_iIssuedAtMs || nowMs < emission.m_iServerNowMs
			|| nowMs >= emission.m_iDeadlineMs || emission.m_iDeadlineMs >= int.MAX) return null;

		RICON_GameRadioPttContext source = emission.m_Context;
		map<int, ref RICON_GameRadioPttContext> byPlayer = new map<int, ref RICON_GameRadioPttContext>();
		foreach (RICON_GameRadioPttContext candidate : admitted)
		{
			if (!candidate || !candidate.IsValid() || candidate.m_sDomain != source.m_sDomain || candidate.m_iEpoch != source.m_iEpoch
				|| candidate.m_iPolicyRevision != source.m_iPolicyRevision || candidate.m_iPlayerId == source.m_iPlayerId
				|| candidate.m_iConnectionId == source.m_iConnectionId || candidate.m_iEntityId == source.m_iEntityId) return null;
			RICON_GameRadioPttContext previous = byPlayer.Get(candidate.m_iPlayerId);
			if (previous && !previous.Same(candidate)) return null;
			if (!previous) byPlayer.Set(candidate.m_iPlayerId, candidate);
		}

		ref RICON_GameTs3MatchedBinding sourceMatch = store.ReadMatched(source, nowMs);
		if (!ValidMatch(sourceMatch, source, nowMs)) return null;
		ref RICON_GameTs3RecipientSnapshot snapshot = new RICON_GameTs3RecipientSnapshot();
		snapshot.m_Source = sourceMatch;
		snapshot.m_iDeadlineMs = emission.m_iDeadlineMs;
		if (sourceMatch.m_iDeadlineMs < snapshot.m_iDeadlineMs) snapshot.m_iDeadlineMs = sourceMatch.m_iDeadlineMs;
		set<string> clientUids = new set<string>();
		set<int> clientIds = new set<int>();
		clientUids.Insert(sourceMatch.m_Claim.m_sClientUid);
		clientIds.Insert(sourceMatch.m_iObservedClientId);
		foreach (int playerId, RICON_GameRadioPttContext target : byPlayer)
		{
			ref RICON_GameTs3MatchedBinding recipient = store.ReadMatched(target, nowMs);
			if (!recipient) continue;
			if (!ValidMatch(recipient, target, nowMs)) return null;
			if (recipient.m_Claim.m_sServerUid != sourceMatch.m_Claim.m_sServerUid) continue;
			string clientUid = recipient.m_Claim.m_sClientUid;
			int clientId = recipient.m_iObservedClientId;
			if (clientUids.Contains(clientUid) || clientIds.Contains(clientId)) return null;
			clientUids.Insert(clientUid);
			clientIds.Insert(clientId);
			snapshot.m_Recipients.Insert(recipient);
			if (recipient.m_iDeadlineMs < snapshot.m_iDeadlineMs) snapshot.m_iDeadlineMs = recipient.m_iDeadlineMs;
		}
		if (snapshot.m_Recipients.IsEmpty()) return null;
		snapshot.m_iSampleMs = nowMs;
		snapshot.m_iLeaseId = emission.m_iLeaseId;
		snapshot.m_iTxDeviceId = emission.m_iDeviceId;
		snapshot.m_iTxTransceiverIndex = emission.m_iTxTransceiverIndex;
		snapshot.m_iTxGeneration = emission.m_iTxGeneration;
		snapshot.m_iNetworkId = emission.m_iNetworkId;
		snapshot.m_iChannelId = emission.m_iChannelId;
		snapshot.m_iIssuedAtMs = emission.m_iIssuedAtMs;
		return snapshot;
	}

	protected static bool ValidMatch(RICON_GameTs3MatchedBinding proof, RICON_GameRadioPttContext context, int nowMs)
	{
		return proof && proof.m_Claim && proof.m_Claim.IsValid() && proof.m_Claim.m_Context.Same(context)
			&& RICON_GameTs3PendingBinding.IsValidId(proof.m_sBindingId) && proof.m_iDeadlineMs > nowMs && proof.m_iDeadlineMs < int.MAX
			&& proof.m_iObservedClientId > 0 && proof.m_iObservedClientId <= 65535;
	}
}
