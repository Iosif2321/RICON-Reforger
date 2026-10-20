//! TEST_ONLY: the route corpus oracle/vectors/route_v1.json against the real RICON_RouteContract / RICON_Route
//! (REQ-core §9). Expectations come from the generated RICON_RouteFixtures.c; nothing here recomputes a route.

//! A null leg cannot be added through the public AddLeg (it refuses null), but REQ-core §9.1 requires NULL_LEG for
//! it. This subclass inserts one through the protected array so the real Evaluate sees it (TEST_SUBCLASS).
class RICON_TestRoute : RICON_Route
{
	void InsertNullLeg()
	{
		m_aLegs.Insert(null);
	}
}

//! Copy of every leg field before the call, compared after it: Evaluate must not change its inputs.
class RICON_RouteInputSnapshot
{
	protected ref array<int> m_aInts = {};
	protected ref array<float> m_aQualities = {};
	protected ref array<bool> m_aFlags = {};

	void Take(RICON_Route route)
	{
		if (!route)
			return;
		m_aInts.Insert(route.m_iTalkerEndpointId);
		m_aInts.Insert(route.m_iListenerEndpointId);
		m_aInts.Insert(route.GetLegCount());
		for (int i = 0; i < route.GetLegCount(); i++)
			AddLeg(route.GetLeg(i));
	}

	protected void AddLeg(RICON_RouteLeg leg)
	{
		m_aFlags.Insert(leg != null);
		if (!leg)
			return;
		m_aInts.Insert(leg.m_iFromEndpointId);
		m_aInts.Insert(leg.m_iToEndpointId);
		m_aFlags.Insert(leg.m_Budget != null);
		if (!leg.m_Budget)
			return;
		m_aFlags.Insert(leg.m_Budget.m_bValid);
		m_aInts.Insert(leg.m_Budget.m_eState);
		m_aQualities.Insert(leg.m_Budget.m_fIntelligibility);
	}

	bool Equals(RICON_RouteInputSnapshot other)
	{
		if (m_aInts.Count() != other.m_aInts.Count() || m_aFlags.Count() != other.m_aFlags.Count()
			|| m_aQualities.Count() != other.m_aQualities.Count())
			return false;
		for (int i = 0; i < m_aInts.Count(); i++)
		{
			if (m_aInts[i] != other.m_aInts[i])
				return false;
		}
		for (int j = 0; j < m_aFlags.Count(); j++)
		{
			if (m_aFlags[j] != other.m_aFlags[j])
				return false;
		}
		for (int k = 0; k < m_aQualities.Count(); k++)
		{
			float a = m_aQualities[k];
			float b = other.m_aQualities[k];
			bool bothNaN = !RICON_Num.IsFinite(a) && !RICON_Num.IsFinite(b);
			if (!bothNaN && a != b)
				return false;
		}
		return true;
	}
}

class RICON_RouteRun
{
	static const float QUALITY_TOLERANCE = 0.000001; // route_v1.json tolerances.ratio
	int m_iCases;
	int m_iQuietPositive;
	int m_iControls;
	int m_iAdapterMapped;
	int m_iTestSubclass;
	bool m_bOk = true;

	static RICON_LinkBudget Budget(bool valid, float intelligibility, RICON_ELinkState state)
	{
		RICON_LinkBudget budget = new RICON_LinkBudget();
		budget.m_bValid = valid;
		budget.m_fIntelligibility = intelligibility;
		budget.m_eState = state;
		return budget;
	}

	static RICON_RoutingContext Context(int epoch, int timeMs, int maxHops, int correlationId, RICON_AccessPolicy policy)
	{
		return new RICON_RoutingContext(epoch, timeMs, maxHops, correlationId, policy);
	}

	//! One comparison of the real result with the expectation; quiet = no FAIL lines (used by the controls).
	protected static bool Compare(string id, RICON_ERouteReason actualReason, float actualQuality,
		RICON_ELinkState actualState, RICON_ERouteReason reason, float quality, RICON_ELinkState state, bool quiet)
	{
		bool ok = actualReason == reason;
		ok = ok && RICON_Num.IsFinite(actualQuality) && RICON_Num.IsFinite(quality)
			&& Math.AbsFloat(actualQuality - quality) <= QUALITY_TOLERANCE;
		ok = ok && actualState == state;
		if (!ok && !quiet)
			PrintFormat("[RICON_ROUTE] FAIL %1: reason %2/%3 quality %4/%5 state %6/%7", id,
				typename.EnumToString(RICON_ERouteReason, actualReason), typename.EnumToString(RICON_ERouteReason, reason),
				actualQuality.ToString(-1, 9), quality.ToString(-1, 9),
				typename.EnumToString(RICON_ELinkState, actualState), typename.EnumToString(RICON_ELinkState, state));
		return ok;
	}

	void Check(string id, RICON_Route route, RICON_RoutingContext context, RICON_ERouteReason reason, float quality,
		RICON_ELinkState state)
	{
		RICON_RouteInputSnapshot before = new RICON_RouteInputSnapshot();
		before.Take(route);
		float actualQuality;
		RICON_ELinkState actualState;
		RICON_ERouteReason actualReason = RICON_RouteContract.Evaluate(route, context, actualQuality, actualState);
		RICON_RouteInputSnapshot after = new RICON_RouteInputSnapshot();
		after.Take(route);
		m_iCases++;
		PrintFormat("[RICON_ROUTE] %1 reason=%2 quality=%3 state=%4", id,
			typename.EnumToString(RICON_ERouteReason, actualReason), actualQuality.ToString(-1, 9),
			typename.EnumToString(RICON_ELinkState, actualState));
		m_bOk = Compare(id, actualReason, actualQuality, actualState, reason, quality, state, false) && m_bOk;
		m_bOk = RICON_TestSupport.Expect(before.Equals(after), id, "входы не изменены") && m_bOk;
		if (Compare(id, actualReason, actualQuality, actualState, reason, quality, state, true))
			m_iQuietPositive++;
		else
			m_bOk = false;
		if (m_iCases == 1)
			Controls(id, actualReason, actualQuality, actualState, reason, quality, state);
	}

	//! Each control spoils one field of the expectation; the full comparison must reject every one.
	protected void Controls(string id, RICON_ERouteReason actualReason, float actualQuality, RICON_ELinkState actualState,
		RICON_ERouteReason reason, float quality, RICON_ELinkState state)
	{
		Reject(Compare(id, actualReason, actualQuality, actualState, RICON_ERouteReason.WRONG_DESTINATION, quality, state, true));
		Reject(Compare(id, actualReason, actualQuality, actualState, reason, quality + 2 * QUALITY_TOLERANCE, state, true));
		Reject(Compare(id, actualReason, actualQuality, actualState, reason, quality, RICON_ELinkState.WEAK, true));
		Reject(Compare(id, actualReason, RICON_L2Run.NaN(), actualState, reason, quality, state, true));
	}

	protected void Reject(bool accepted)
	{
		if (!accepted)
			m_iControls++;
		else
			m_bOk = false;
	}
}
