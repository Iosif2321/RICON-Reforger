// Network-level extension points (ADR-002, PLAN v5 §3.1 invariant 3). Status: STATIC_IMPLEMENTED/COMPILE_ONLY.
// Routing policies decide routes; only RICON_VoNIntegration may touch voice.

//! One hop of a route: transmitter endpoint -> receiver endpoint with its evaluated budget.
class RICON_RouteLeg
{
	int m_iFromEndpointId;
	int m_iToEndpointId;
	ref RICON_LinkBudget m_Budget;
}

//! A route from talker to listener. Quality is the weakest leg (min(legs), PLAN v5 §3).
class RICON_Route
{
	ref array<ref RICON_RouteLeg> m_aLegs = {};

	float GetQuality()
	{
		if (m_aLegs.IsEmpty())
			return 0;
		float q = 1;
		foreach (RICON_RouteLeg leg : m_aLegs)
		{
			if (!leg.m_Budget || !leg.m_Budget.m_bValid)
				return 0;
			q = Math.Min(q, leg.m_Budget.m_fIntelligibility);
		}
		return q;
	}

	RICON_ELinkState GetState()
	{
		if (m_aLegs.IsEmpty())
			return RICON_ELinkState.LOST;
		RICON_ELinkState worst = RICON_ELinkState.CLEAN;
		foreach (RICON_RouteLeg leg : m_aLegs)
		{
			if (!leg.m_Budget || !leg.m_Budget.m_bValid)
				return RICON_ELinkState.LOST;
			if (leg.m_Budget.m_eState < worst)
				worst = leg.m_Budget.m_eState;
		}
		return worst;
	}
}

//! Routing policy of one network type (radio-net, relay, cellular; wired is only a seam).
class RICON_RoutingPolicy
{
	string GetNetworkTypeId()
	{
		return "base";
	}

	//! Build the best route talker -> listener or return null if none is allowed.
	RICON_Route FindRoute(int talkerEndpointId, int listenerEndpointId)
	{
		return null;
	}
}

//! Narrow seam between server routing and the engine VoN (ADR-001 decides what it may actuate).
//! Implementations must be server-side only and must not trust client input.
class RICON_VoNIntegration
{
	//! Apply a new authoritative snapshot. Stale snapshots must be ignored by the caller (IsNewerThan).
	void ApplySnapshot(RICON_LinkSnapshot snapshot)
	{
	}

	//! Server PTT/transmit lease expired: stop transmission and restore catalog transceiver values.
	void OnTransmitLeaseExpired(int talkerPlayerId)
	{
	}
}
