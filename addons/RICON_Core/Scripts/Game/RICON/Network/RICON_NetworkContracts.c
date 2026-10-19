// Network-level contracts: route validation, policy context, call-session and VoN seams (REQ-core §9).
// Status: WORKBENCH_COMPILE_PASS @ ad24d51; runtime behavior is not tested.

//! Stable route failure reasons in the global validation order of REQ-core §9.1.
enum RICON_ERouteReason
{
	OK = 0,
	INVALID_CONTEXT = 1,
	EMPTY_ROUTE = 2,
	TOO_MANY_HOPS = 3,
	NULL_LEG = 4,
	INVALID_ENDPOINT = 5,
	INVALID_BUDGET = 6,
	WRONG_START = 7,
	DISCONTINUITY = 8,
	CYCLE = 9,
	WRONG_DESTINATION = 10
}

//! Server-only access seam. The base policy fails closed.
class RICON_AccessPolicy
{
	bool CanUseEndpoint(int playerId, int endpointId)
	{
		return false;
	}
}

//! Immutable per-resolution context; no concrete manager or mutable registry is exposed (REQ-core §9.2).
class RICON_RoutingContext
{
	protected int m_iSessionEpoch;
	protected int m_iServerTimeMs;
	protected int m_iMaxHops;
	protected int m_iCorrelationId;
	protected ref RICON_AccessPolicy m_AccessPolicy;

	void RICON_RoutingContext(
		int sessionEpoch,
		int serverTimeMs,
		int maxHops,
		int correlationId,
		RICON_AccessPolicy accessPolicy)
	{
		m_iSessionEpoch = sessionEpoch;
		m_iServerTimeMs = serverTimeMs;
		m_iMaxHops = maxHops;
		m_iCorrelationId = correlationId;
		m_AccessPolicy = accessPolicy;
	}

	bool IsValid()
	{
		return m_iSessionEpoch > 0 && m_iSessionEpoch < int.MAX && m_iServerTimeMs >= 0
			&& m_iMaxHops > 0 && m_iCorrelationId > 0 && m_AccessPolicy != null;
	}

	int GetSessionEpoch() { return m_iSessionEpoch; }
	int GetServerTimeMs() { return m_iServerTimeMs; }
	int GetMaxHops() { return m_iMaxHops; }
	int GetCorrelationId() { return m_iCorrelationId; }
	RICON_AccessPolicy GetAccessPolicy() { return m_AccessPolicy; }
}

//! One ordered hop: from endpoint -> to endpoint, strongly owning its evaluated budget.
class RICON_RouteLeg
{
	int m_iFromEndpointId;
	int m_iToEndpointId;
	ref RICON_LinkBudget m_Budget;

	void RICON_RouteLeg(int fromEndpointId, int toEndpointId, RICON_LinkBudget budget)
	{
		m_iFromEndpointId = fromEndpointId;
		m_iToEndpointId = toEndpointId;
		m_Budget = budget;
	}
}

class RICON_Route
{
	int m_iTalkerEndpointId;
	int m_iListenerEndpointId;
	protected ref array<ref RICON_RouteLeg> m_aLegs = {};

	protected RICON_ERouteReason m_eReason = RICON_ERouteReason.INVALID_CONTEXT;
	protected float m_fQuality;
	protected RICON_ELinkState m_eState = RICON_ELinkState.LOST;
	protected bool m_bEvaluated;

	void RICON_Route(int talkerEndpointId, int listenerEndpointId)
	{
		m_iTalkerEndpointId = talkerEndpointId;
		m_iListenerEndpointId = listenerEndpointId;
	}

	//! Route is built once; after Evaluate its cached result cannot be invalidated by later mutations.
	bool AddLeg(RICON_RouteLeg leg)
	{
		if (m_bEvaluated || !leg)
			return false;
		m_aLegs.Insert(leg);
		return true;
	}

	int GetLegCount()
	{
		return m_aLegs.Count();
	}

	RICON_RouteLeg GetLeg(int index)
	{
		if (index < 0 || index >= m_aLegs.Count())
			return null;
		return m_aLegs[index];
	}

	//! Validate globally by reason class, then derive min quality and worst state (REQ-core §9.1).
	RICON_ERouteReason Evaluate(RICON_RoutingContext context)
	{
		m_bEvaluated = true;
		SetInvalid(RICON_ERouteReason.INVALID_CONTEXT);
		if (!context || !context.IsValid())
			return m_eReason;
		if (!m_aLegs || m_aLegs.IsEmpty())
			return SetInvalid(RICON_ERouteReason.EMPTY_ROUTE);
		if (m_aLegs.Count() > context.GetMaxHops())
			return SetInvalid(RICON_ERouteReason.TOO_MANY_HOPS);

		// Global pass 1: null legs anywhere beat every lower-priority reason.
		foreach (RICON_RouteLeg nullCheck : m_aLegs)
		{
			if (!nullCheck)
				return SetInvalid(RICON_ERouteReason.NULL_LEG);
		}

		// Global pass 2: route endpoints and every leg endpoint.
		if (m_iTalkerEndpointId <= 0 || m_iListenerEndpointId <= 0)
			return SetInvalid(RICON_ERouteReason.INVALID_ENDPOINT);
		foreach (RICON_RouteLeg endpointCheck : m_aLegs)
		{
			if (endpointCheck.m_iFromEndpointId <= 0 || endpointCheck.m_iToEndpointId <= 0
				|| endpointCheck.m_iFromEndpointId == endpointCheck.m_iToEndpointId)
				return SetInvalid(RICON_ERouteReason.INVALID_ENDPOINT);
		}

		// Global pass 3: budgets.
		foreach (RICON_RouteLeg budgetCheck : m_aLegs)
		{
			if (!IsBudgetValid(budgetCheck.m_Budget))
				return SetInvalid(RICON_ERouteReason.INVALID_BUDGET);
		}

		if (m_aLegs[0].m_iFromEndpointId != m_iTalkerEndpointId)
			return SetInvalid(RICON_ERouteReason.WRONG_START);

		// One ordered pass: continuity before cycle on each leg.
		set<int> seenEndpoints = new set<int>();
		seenEndpoints.Insert(m_iTalkerEndpointId);
		int previousTo = m_iTalkerEndpointId;
		foreach (RICON_RouteLeg pathLeg : m_aLegs)
		{
			if (pathLeg.m_iFromEndpointId != previousTo)
				return SetInvalid(RICON_ERouteReason.DISCONTINUITY);
			if (seenEndpoints.Contains(pathLeg.m_iToEndpointId))
				return SetInvalid(RICON_ERouteReason.CYCLE);
			seenEndpoints.Insert(pathLeg.m_iToEndpointId);
			previousTo = pathLeg.m_iToEndpointId;
		}

		if (m_aLegs[m_aLegs.Count() - 1].m_iToEndpointId != m_iListenerEndpointId)
			return SetInvalid(RICON_ERouteReason.WRONG_DESTINATION);

		m_fQuality = 1;
		m_eState = RICON_ELinkState.CLEAN;
		foreach (RICON_RouteLeg validLeg : m_aLegs)
		{
			m_fQuality = Math.Min(m_fQuality, validLeg.m_Budget.m_fIntelligibility);
			if (validLeg.m_Budget.m_eState < m_eState)
				m_eState = validLeg.m_Budget.m_eState;
		}
		m_eReason = RICON_ERouteReason.OK;
		return m_eReason;
	}

	RICON_ERouteReason GetReason()
	{
		return m_eReason;
	}

	bool WasEvaluated()
	{
		return m_bEvaluated;
	}

	float GetQuality()
	{
		return m_fQuality;
	}

	RICON_ELinkState GetState()
	{
		return m_eState;
	}

	protected RICON_ERouteReason SetInvalid(RICON_ERouteReason reason)
	{
		m_eReason = reason;
		m_fQuality = 0;
		m_eState = RICON_ELinkState.LOST;
		return m_eReason;
	}

	protected static bool IsBudgetValid(RICON_LinkBudget budget)
	{
		if (!budget || !budget.m_bValid || !RICON_Num.IsFinite(budget.m_fIntelligibility)
			|| budget.m_fIntelligibility < 0 || budget.m_fIntelligibility > 1)
			return false;
		if (budget.m_eState != RICON_ELinkState.LOST && budget.m_eState != RICON_ELinkState.WEAK
			&& budget.m_eState != RICON_ELinkState.DEGRADED && budget.m_eState != RICON_ELinkState.CLEAN)
			return false;
		return budget.m_eState != RICON_ELinkState.LOST || budget.m_fIntelligibility == 0;
	}
}

//! Null route is EMPTY_ROUTE, not an exception; outputs remain deterministic.
class RICON_RouteContract
{
	static RICON_ERouteReason Evaluate(
		RICON_Route route,
		RICON_RoutingContext context,
		out float quality,
		out RICON_ELinkState state)
	{
		quality = 0;
		state = RICON_ELinkState.LOST;
		if (!context || !context.IsValid())
			return RICON_ERouteReason.INVALID_CONTEXT;
		if (!route)
			return RICON_ERouteReason.EMPTY_ROUTE;
		RICON_ERouteReason reason = route.Evaluate(context);
		quality = route.GetQuality();
		state = route.GetState();
		return reason;
	}
}

//! Routing policy for one network type. Implementations receive all mutable-world inputs explicitly.
class RICON_RoutingPolicy
{
	string GetNetworkTypeId()
	{
		return string.Empty;
	}

	bool IsConfigValid()
	{
		return false;
	}

	bool RequiresCallSessionFactory()
	{
		return false;
	}

	RICON_Route FindRoute(
		RICON_RoutingContext context,
		int talkerEndpointId,
		int listenerEndpointId)
	{
		return null;
	}
}

//! Common session seam for cellular/satellite-like networks. State machines belong to their later stages.
class RICON_CallSession
{
	int GetSessionId()
	{
		return 0;
	}

	string GetNetworkTypeId()
	{
		return string.Empty;
	}
}

class RICON_CallSessionFactory
{
	string GetNetworkTypeId()
	{
		return string.Empty;
	}

	bool IsConfigValid()
	{
		return false;
	}

	RICON_CallSession Create(
		RICON_RoutingContext context,
		int sessionId,
		int talkerEndpointId,
		int listenerEndpointId)
	{
		return null;
	}
}

//! Narrow seam to engine VoN. Caller validates authority and snapshot receiver/order before this no-op scaffold.
class RICON_VoNIntegration
{
	void ApplySnapshot(RICON_LinkSnapshot snapshot)
	{
	}

	void OnTransmitLeaseExpired(int talkerPlayerId, int deviceId, int leaseId)
	{
	}
}
