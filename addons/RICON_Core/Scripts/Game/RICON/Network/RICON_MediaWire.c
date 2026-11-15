//! V1 scalar media header plus a bounded array of PCM/codec-independent payload bytes.
class RICON_MediaWireV1
{
	static const int MAX_PAYLOAD_BYTES = 64;
	static const int MAX_SAMPLE_COUNT = 48000;

	static array<int> EncodeBody(RICON_MediaEgressItem item)
	{
		if (!item || !item.m_Route || !item.m_Route.IsValid() || !item.m_Block) return null;
		RICON_QueuedMediaBlock block = item.m_Block;
		if (block.m_RfHistory || block.m_CaptureEnvelope) return null;
		if (!item.m_Route.m_Source.Same(block.m_Binding)) return null;
		if (!ValidRange(block.m_iSequence, block.m_iFirstSampleIndex, block.m_iSampleCount) || !block.m_Bytes) return null;
		int count = block.m_Bytes.Count();
		if (count < 1 || count > MAX_PAYLOAD_BYTES) return null;
		array<int> body = {};
		body.Insert(block.m_iSampleCount);
		foreach (int value : block.m_Bytes)
		{
			if (value < 0 || value > 255) return null;
			body.Insert(value);
		}
		return body;
	}

	static array<int> DecodeBody(int sequence, int firstSampleIndex, array<int> body, out int sampleCount)
	{
		sampleCount = 0;
		if (!body || body.Count() < 2 || body.Count() > MAX_PAYLOAD_BYTES + 1) return null;
		if (!ValidRange(sequence, firstSampleIndex, body[0])) return null;
		array<int> payload = {};
		for (int index = 1; index < body.Count(); index++)
		{
			int value = body[index];
			if (value < 0 || value > 255) return null;
			payload.Insert(value);
		}
		sampleCount = body[0];
		return payload;
	}

	static bool ValidRange(int sequence, int firstSampleIndex, int sampleCount)
	{
		return sequence >= 0 && sequence < int.MAX && firstSampleIndex >= 0
			&& sampleCount > 0 && sampleCount <= MAX_SAMPLE_COUNT && firstSampleIndex <= int.MAX - sampleCount;
	}
}

class RICON_MediaWirePending
{
	int m_iLocalDeadlineMs, m_iLocalArrivalMs;
	ref RICON_QueuedMediaBlock m_Block;
}

//! A decision retains this proof, but the installed grant remains its sole authority.
class RICON_MediaOutputPermit
{
	protected RICON_MediaWireReceiver m_Receiver;
	protected int m_iPolicyRevision;

	void RICON_MediaOutputPermit(RICON_MediaWireReceiver receiver, int policyRevision)
	{
		m_Receiver = receiver;
		m_iPolicyRevision = policyRevision;
	}

	bool IsCurrent()
	{
		return m_Receiver && m_Receiver.HasOutputPermit(m_iPolicyRevision, this);
	}

	bool MatchesRoute(RICON_MediaEgressRoute route)
	{
		return route && route.m_iPolicyRevision == m_iPolicyRevision && m_Receiver
			&& m_Receiver.HasOutputPermit(m_iPolicyRevision, this, route);
	}
}

class RICON_MediaWireGrant
{
	int m_iFirstSequence;
	int m_iFirstSampleIndex;
	bool m_bDirectTake;
	bool m_bArrivalAwarePlayout;
	ref RICON_MediaEgressRoute m_Route;
	ref RICON_MediaWindow m_Window;
	ref RICON_MediaPlayout m_Playout;
	ref RICON_MediaOutputPermit m_OutputPermit;
	ref array<ref RICON_MediaWirePending> m_Pending = {};

	void Close()
	{
		m_Window.Close();
		if (m_Playout) m_Playout.Close();
		m_Pending.Clear();
	}
}

//! One locally qualified recipient incarnation. Only trusted control installs or revokes routes.
class RICON_MediaWireReceiver
{
	protected int m_iEpoch, m_iPlayer, m_iConnection, m_iGeneration, m_iEntity;
	protected int m_iMaxRoutes, m_iWindowWidth, m_iMaxQueued, m_iMaxQueueAgeMs;
	protected int m_iLastControlSeq, m_iMaxUsedPolicies;
	protected int m_iLastNowMs = -1;
	protected int m_iExpiredCount;
	protected int m_iWireVersion = 1;
	protected string m_sRfTimebaseId;
	protected bool m_bClosed;
	protected bool m_bControllerBound;
	protected SCR_PlayerController m_Controller;
	protected ref map<int, ref RICON_MediaWireGrant> m_Grants = new map<int, ref RICON_MediaWireGrant>();
	protected ref set<int> m_UsedPolicies = new set<int>();

	bool Configure(int epoch, int listenerPlayerId, int listenerConnectionId, int listenerGeneration,
		int listenerEntityId, int maxRoutes, int windowWidth, int maxQueuedPerRoute, int maxQueueAgeMs = 250,
		int maxUsedPolicies = 4096, string rfTimebaseId = "", bool historicalRf = false)
	{
		if (m_bClosed || m_iEpoch != 0 || epoch <= 0 || epoch >= int.MAX || listenerPlayerId <= 0
			|| listenerConnectionId <= 0 || listenerGeneration <= 0 || listenerEntityId <= 0) return false;
		if (maxRoutes < 1 || maxRoutes > 16 || windowWidth < 1 || windowWidth > 64
			|| maxQueuedPerRoute < 1 || maxQueuedPerRoute > 16 || maxQueueAgeMs < 1 || maxQueueAgeMs > 2000) return false;
		if (maxUsedPolicies < 1 || maxUsedPolicies > 4096) return false;
		if ((historicalRf || !rfTimebaseId.IsEmpty()) && !RICON_TemporalSnapshots.ValidKey(rfTimebaseId)) return false;
		m_iEpoch = epoch;
		m_iPlayer = listenerPlayerId;
		m_iConnection = listenerConnectionId;
		m_iGeneration = listenerGeneration;
		m_iEntity = listenerEntityId;
		m_iMaxRoutes = maxRoutes;
		m_iWindowWidth = windowWidth;
		m_iMaxQueued = maxQueuedPerRoute;
		m_iMaxQueueAgeMs = maxQueueAgeMs;
		m_iMaxUsedPolicies = maxUsedPolicies;
		m_sRfTimebaseId = rfTimebaseId;
		if (historicalRf) m_iWireVersion = 3;
		else if (!rfTimebaseId.IsEmpty()) m_iWireVersion = 2;
		return true;
	}

	int GetWireVersion() { return m_iWireVersion; }
	string GetRfTimebaseId() { return m_sRfTimebaseId; }

	bool IsEndpoint(int epoch, int playerId, int conn, int gen, int entity)
	{
		return !m_bClosed && m_iEpoch > 0 && epoch == m_iEpoch && playerId == m_iPlayer
			&& conn == m_iConnection && gen == m_iGeneration && entity == m_iEntity;
	}

	//! Control is ordered. Policy identities may arrive out of issuance order and cannot be reused.
	bool Install(int controlSeq, RICON_MediaEgressRoute route, int firstSequence, int firstSampleIndex = 0)
	{
		if (!ValidControl(controlSeq, route) || m_UsedPolicies.Count() >= m_iMaxUsedPolicies
			|| m_UsedPolicies.Contains(route.m_iPolicyRevision)
			|| m_Grants.Count() >= m_iMaxRoutes || firstSampleIndex < 0 || firstSampleIndex >= int.MAX) return false;
		foreach (int policy, RICON_MediaWireGrant grant : m_Grants)
		{
			RICON_MediaEgressRoute prior = grant.m_Route;
			if (prior.m_Source.m_iStreamId != route.m_Source.m_iStreamId) continue;
			if (!prior.m_Source.Same(route.m_Source) || prior.m_iRouteId == route.m_iRouteId) return false;
			if (prior.m_iRxDeviceId == route.m_iRxDeviceId && prior.m_iRxTransceiverIndex == route.m_iRxTransceiverIndex)
				return false;
		}
		RICON_MediaWindow window = new RICON_MediaWindow();
		if (!window.Initialize(route.m_Source.m_iStreamGeneration, firstSequence, m_iWindowWidth,
			RICON_MediaWireV1.MAX_PAYLOAD_BYTES)) return false;
		RICON_MediaWireGrant installed = new RICON_MediaWireGrant();
		installed.m_iFirstSequence = firstSequence;
		installed.m_iFirstSampleIndex = firstSampleIndex;
		installed.m_Route = route.CloneRoute();
		installed.m_Window = window;
		int policyRevision = installed.m_Route.m_iPolicyRevision;
		installed.m_OutputPermit = new RICON_MediaOutputPermit(this, policyRevision);
		m_Grants.Set(policyRevision, installed);
		m_UsedPolicies.Insert(policyRevision);
		m_iLastControlSeq = controlSeq;
		return true;
	}

	bool Revoke(int controlSeq, RICON_MediaEgressRoute route)
	{
		if (!ValidControl(controlSeq, route)) return false;
		RICON_MediaWireGrant grant = m_Grants.Get(route.m_iPolicyRevision);
		if (!grant || !grant.m_Route.Same(route)) return false;
		grant.Close();
		m_Grants.Remove(route.m_iPolicyRevision);
		m_iLastControlSeq = controlSeq;
		return true;
	}

	string Receive(int epoch, int streamId, int routeId, int routeGeneration, int policyRevision,
		int sequence, int firstSampleIndex, array<int> body, int nowMs)
	{
		return ReceiveBody(1, epoch, streamId, routeId, routeGeneration, policyRevision, sequence, firstSampleIndex, body, nowMs);
	}

	string ReceiveRfV2(int epoch, int streamId, int routeId, int routeGeneration, int policyRevision,
		int sequence, int firstSampleIndex, array<int> body, int nowMs)
	{
		return ReceiveBody(2, epoch, streamId, routeId, routeGeneration, policyRevision, sequence, firstSampleIndex, body, nowMs);
	}

	string ReceiveRfV3(int epoch, int streamId, int routeId, int routeGeneration, int policyRevision,
		int sequence, int firstSampleIndex, array<int> body, int nowMs)
	{
		return ReceiveBody(3, epoch, streamId, routeId, routeGeneration, policyRevision, sequence, firstSampleIndex, body, nowMs);
	}

	protected string ReceiveBody(int version, int epoch, int streamId, int routeId, int routeGeneration, int policyRevision,
		int sequence, int firstSampleIndex, array<int> body, int nowMs)
	{
		if (m_bClosed) return "CLOSED";
		if (m_iEpoch == 0 || epoch != m_iEpoch) return "STALE_EPOCH";
		RICON_MediaWireGrant grant = m_Grants.Get(policyRevision);
		if (!grant || grant.m_Route.m_Source.m_iStreamId != streamId || grant.m_Route.m_iRouteId != routeId
			|| grant.m_Route.m_iRouteGeneration != routeGeneration) return "NO_GRANT";
		if (version != m_iWireVersion) return "MODE_MISMATCH";
		int sampleCount;
		array<int> payload;
		RICON_TemporalRfBudget budget;
		RICON_MediaCaptureEnvelope envelope;
		RICON_MediaRfHistoryEntry history;
		int headerValues;
		if (version == 3)
		{
			payload = RICON_MediaWireRfV3.DecodeBody(sequence, firstSampleIndex, body, sampleCount, envelope, history);
			if (!payload || !envelope || !history || !history.m_Budget) return "INVALID_BODY";
			budget = history.m_Budget;
			envelope.m_Binding = grant.m_Route.m_Source.CloneBinding();
			envelope.m_sTimebaseId = m_sRfTimebaseId;
			headerValues = RICON_MediaWireRfV3.HEADER_VALUES;
		}
		else if (version == 2)
		{
			payload = RICON_MediaWireRfV2.DecodeBody(sequence, firstSampleIndex, body, sampleCount, budget);
			if (!payload || !budget) return "INVALID_BODY";
			headerValues = RICON_MediaWireRfV2.HEADER_VALUES;
		}
		else payload = RICON_MediaWireV1.DecodeBody(sequence, firstSampleIndex, body, sampleCount);
		if (!payload) return "INVALID_BODY";
		if (budget) budget.m_sTimebaseId = m_sRfTimebaseId;
		if (firstSampleIndex < grant.m_iFirstSampleIndex) return "BEFORE_CUTOVER";
		if (nowMs > int.MAX - m_iMaxQueueAgeMs || !ObserveTime(nowMs)) return "BAD_TIME";
		PurgeExpired(grant, nowMs);
		if (grant.m_Pending.Count() >= m_iMaxQueued) return "FULL";

		// Use the validated wire integers so distinct headers cannot round to the same float key.
		string contextKey;
		for (int index = 1; index < headerValues; index++)
		{
			int value = body[index];
			contextKey += "/" + value.ToString();
		}
		RICON_EMediaWindowDecision decision = grant.m_Window.Accept(grant.m_Route.m_Source.m_iStreamGeneration,
			sequence, payload, firstSampleIndex, sampleCount, contextKey);
		if (decision != RICON_EMediaWindowDecision.ACCEPTED) return typename.EnumToString(RICON_EMediaWindowDecision, decision);

		RICON_QueuedMediaBlock block = new RICON_QueuedMediaBlock();
		block.m_iSequence = sequence;
		block.m_iFirstSampleIndex = firstSampleIndex;
		block.m_iSampleCount = sampleCount;
		// The V3 envelope preserves remote RF provenance. -1 forbids reuse as server egress input.
		block.m_iCaptureLowerMs = -1;
		block.m_iCaptureUpperMs = -1;
		block.m_Binding = grant.m_Route.m_Source.CloneBinding();
		block.m_Bytes = payload;
		block.m_RfBudget = budget;
		block.m_CaptureEnvelope = envelope;
		block.m_RfHistory = history;
		RICON_MediaWirePending pending = new RICON_MediaWirePending();
		pending.m_Block = block;
		pending.m_iLocalArrivalMs = nowMs;
		pending.m_iLocalDeadlineMs = nowMs + m_iMaxQueueAgeMs;
		grant.m_Pending.Insert(pending);
		return "ACCEPTED";
	}

	//! Transfer fresh admission FIFO ownership. Residence is measured only on the local clock.
	RICON_QueuedMediaBlock Take(RICON_MediaEgressRoute route, int nowMs)
	{
		if (m_bClosed || !route) return null;
		RICON_MediaWireGrant grant = m_Grants.Get(route.m_iPolicyRevision);
		if (!grant || grant.m_Playout || !grant.m_Route.Same(route) || !ObserveTime(nowMs)) return null;
		PurgeExpired(grant, nowMs);
		if (grant.m_Pending.Count() == 0) return null;
		RICON_QueuedMediaBlock block = grant.m_Pending[0].m_Block;
		grant.m_Pending.RemoveOrdered(0);
		grant.m_bDirectTake = true;
		return block;
	}

	//! Снимок начальной позиции для уже выданного разрешения, пока у него нет потребителя.
	RICON_MediaOutputPermit ReadUnclaimedPlayout(RICON_MediaEgressRoute route, out int firstSequence, out int firstSampleIndex)
	{
		firstSequence = -1;
		firstSampleIndex = -1;
		if (m_bClosed || !route) return null;
		ref RICON_MediaWireGrant grant = m_Grants.Get(route.m_iPolicyRevision);
		if (!grant || grant.m_bDirectTake || grant.m_Playout || !grant.m_Route.Same(route)
			|| !HasOutputPermit(route.m_iPolicyRevision, grant.m_OutputPermit, route)
			|| m_bClosed || m_Grants.Get(route.m_iPolicyRevision) != grant) return null;
		firstSequence = grant.m_iFirstSequence;
		firstSampleIndex = grant.m_iFirstSampleIndex;
		return grant.m_OutputPermit;
	}

	bool ClaimPlayout(RICON_MediaEgressRoute route, RICON_MediaPlayout playout, RICON_MediaOutputPermit permit)
	{
		int firstSequence, firstSampleIndex;
		if (!playout || !permit || ReadUnclaimedPlayout(route, firstSequence, firstSampleIndex) != permit
			|| !playout.IsRoute(route) || !playout.IsAtCutover(firstSequence, firstSampleIndex)) return false;
		RICON_MediaWireGrant grant = m_Grants.Get(route.m_iPolicyRevision);
		grant.m_Playout = playout;
		grant.m_bArrivalAwarePlayout = true;
		return true;
	}

	//! The grant owns its attached playout lifetime. Neither FIFO residence nor its deadline is refreshed here.
	int DrainToPlayout(RICON_MediaEgressRoute route, RICON_MediaPlayout playout, int nowMs, int maxBlocks)
	{
		if (m_bClosed || !route || !playout || maxBlocks < 1 || maxBlocks > 16) return 0;
		RICON_MediaWireGrant grant = m_Grants.Get(route.m_iPolicyRevision);
		if (!grant || grant.m_bDirectTake || (grant.m_Playout && grant.m_Playout != playout)) return 0;
		if (!grant.m_Route.Same(route) || !playout.IsRoute(route)) return 0;
		if (!grant.m_Playout && !playout.IsAtCutover(grant.m_iFirstSequence, grant.m_iFirstSampleIndex)) return 0;
		if (!ObserveTime(nowMs)) return 0;
		grant.m_Playout = playout;
		PurgeExpired(grant, nowMs);
		// Commit overdue intervals before deciding which pending records fit the new horizon.
		if (playout.NeedsCatchup(nowMs)) return 0;

		int accepted = 0;
		for (int attempt = 0; attempt < maxBlocks && grant.m_Pending.Count() > 0; attempt++)
		{
			RICON_MediaWirePending pending = grant.m_Pending[0];
			int arrivalMs = -1;
			if (grant.m_bArrivalAwarePlayout) arrivalMs = pending.m_iLocalArrivalMs;
			string result = playout.Offer(route, pending.m_Block, pending.m_iLocalDeadlineMs, nowMs, arrivalMs);
			if (result == "FULL" || result == "BAD_TIME" || result == "WRONG_ROUTE" || result == "CLOSED") break;
			// Drop even OUTSIDE_HORIZON so a far-future block cannot hide useful FIFO entries behind it.
			grant.m_Pending.RemoveOrdered(0);
			if (result == "ACCEPTED") accepted++;
			if (result == "EXPIRED" && m_iExpiredCount < int.MAX) m_iExpiredCount++;
		}
		return accepted;
	}

	int GetExpiredCount() { return m_iExpiredCount; }

	//! Decide one slot only while the exact installed grant still owns this timeline.
	RICON_MediaPlayoutDecision PollPlayout(RICON_MediaEgressRoute route, RICON_MediaPlayout playout, int nowMs)
	{
		if (m_bClosed || !route || !playout) return null;
		RICON_MediaWireGrant grant = m_Grants.Get(route.m_iPolicyRevision);
		if (!grant || grant.m_Playout != playout || !grant.m_Route.Same(route) || !playout.IsRoute(route)) return null;
		if (!ObserveTime(nowMs)) return null;
		RICON_MediaPlayoutDecision decision = playout.Poll(nowMs);
		if (decision && decision.m_sKind == "PLAY") decision.m_OutputPermit = grant.m_OutputPermit;
		return decision;
	}

	bool HasOutputPermit(int policyRevision, RICON_MediaOutputPermit permit, RICON_MediaEgressRoute route = null)
	{
		if (m_bClosed || !permit) return false;
		if (m_bControllerBound && (!m_Controller || !m_Controller.RICON_IsOutputReceiverCurrent(this))) return false;
		RICON_MediaWireGrant grant = m_Grants.Get(policyRevision);
		return grant && grant.m_OutputPermit == permit && (!route || grant.m_Route.Same(route));
	}

	bool BindOutputController(SCR_PlayerController controller)
	{
		if (m_bControllerBound || m_bClosed || !controller || m_Grants.Count() > 0) return false;
		m_Controller = controller;
		m_bControllerBound = true;
		return true;
	}

	void Close()
	{
		m_bClosed = true;
		foreach (int policy, RICON_MediaWireGrant grant : m_Grants) grant.Close();
		m_Grants.Clear();
		m_UsedPolicies.Clear();
	}

	protected bool ObserveTime(int nowMs)
	{
		if (nowMs < 0 || nowMs < m_iLastNowMs) return false;
		m_iLastNowMs = nowMs;
		return true;
	}

	protected void PurgeExpired(RICON_MediaWireGrant grant, int nowMs)
	{
		// Fixed residence and a monotonic arrival clock put all expired entries at the head.
		while (grant.m_Pending.Count() > 0 && nowMs >= grant.m_Pending[0].m_iLocalDeadlineMs)
		{
			grant.m_Pending.RemoveOrdered(0);
			if (m_iExpiredCount < int.MAX) m_iExpiredCount++;
		}
	}

	protected bool ValidControl(int controlSeq, RICON_MediaEgressRoute route)
	{
		return controlSeq > m_iLastControlSeq && controlSeq < int.MAX && route && route.IsValid()
			&& IsEndpoint(route.m_Source.m_iSessionEpoch, route.m_iListenerPlayerId, route.m_iListenerConnectionId,
				route.m_iListenerGeneration, route.m_iListenerEntityId);
	}
}
