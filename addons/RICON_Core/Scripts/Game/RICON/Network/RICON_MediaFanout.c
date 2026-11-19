//! One approved branch and its inclusive source sample boundary.
class RICON_MediaFanoutGrant
{
	ref RICON_MediaEgressRoute m_Route;
	int m_iFirstSampleIndex;
	int m_iGrantedAtMs = -1;
	int m_iRxAdmission;
	int m_iGeometryRevision;
	int m_iListenerIncarnation;
	// Separate from the media route ID. Zero is reserved for fanout configured without Temporal.
	int m_iTemporalLinkId;
}

//! Server-only projection of a current RX and its policy. No client address, route ID or clock claim.
class RICON_MediaReceiverApproval
{
	int m_iListenerPlayerId, m_iRxDeviceId, m_iRxGeneration, m_iPolicyRevision, m_iTemporalLinkId;
	int m_iRxTransceiverIndex = -1;
}

class RICON_MediaFanoutBranch
{
	ref RICON_MediaEgressRoute m_Route;
	ref RICON_MediaEgressQueue m_Queue;
	ref RICON_MediaRfHistory m_RfHistory;
	ref RICON_TemporalRfBudget m_PendingRfCoverage;
	RICON_OpenRadioStaticLosSource m_RfCoverageSource;
	ref RICON_GameRadioRfPoint m_PrecedingGameRfPoint;
	ref RICON_TemporalRfBudget m_PrecedingGameRfBudget;
	bool m_bGameRfCoverage;
	string m_sRfHistoryTimebaseId;
	int m_iFirstSampleIndex;
	int m_iGrantedAtMs;
	int m_iTemporalLinkId;
	bool m_bRetired;
	int m_iRxAdmission;
	int m_iGeometryRevision;
	int m_iListenerIncarnation;
}

//! Submit or refuse synchronously. Callbacks must not reenter or mutate fanout authority.
//! Do not retain an item for a later send.
class RICON_MediaFanoutTransport
{
	bool PrepareBranch(RICON_MediaFanout fanout, RICON_MediaEgressItem item, int nowMs) { return true; }
	void RetireBranch(RICON_MediaEgressRoute route, int nowMs) {}
	bool Send(RICON_MediaEgressItem item) { return false; }
}

//! Доверенный синхронный приёмник сохраняет свою копию кадра до возврата из Offer.
class RICON_MediaRelayFanoutSink
{
	bool Offer(RICON_MediaFanout fanout, RICON_QueuedMediaBlock block, int deadlineMs, int nowMs) { return false; }
}

enum RICON_EMediaDispatchDecision
{
	NOT_READY,
	EMPTY,
	REFUSED,
	ACCEPTED
}

//! One dispatcher per direct emission. Only authority-published rosters enter ReplaceRoutes.
class RICON_MediaFanout
{
	protected static const int RF_HISTORY_MAX_ENTRIES = 16;
	protected static const int RF_HISTORY_MAX_AGE_MS = 2000;
	protected static const int RF_COVERAGE_MAX_SPAN_MS = 1000;

	protected RICON_MediaStreamOwner m_SourceOwner;
	protected RICON_LifecycleStateMachine m_Lifecycle;
	protected RICON_TemporalPublicationOwner m_Publication;
	protected RICON_MediaReceiverAuthority m_ReceiverAuthority;
	protected RICON_MediaListenerAuthority m_ListenerAuthority;
	protected ref RICON_GameControllerLifecycle m_GameControllerLifecycle;
	protected ref RICON_MediaStreamBinding m_Source;
	protected RICON_MediaFanoutTransport m_DispatchTransport;
	protected ref RICON_MediaRelayFanoutSink m_RelaySink;
	protected string m_sRelayMappedTimebaseId;
	protected ref RICON_QueuedMediaBlock m_RelayOfferBlock;
	protected int m_iRelayOfferDeadlineMs, m_iRelayOfferAtMs;
	protected ref array<ref RICON_MediaFanoutBranch> m_Branches = {};
	protected int m_iMaxBranches, m_iMaxQueued, m_iMaxPayloadBytes;
	protected int m_iRevision, m_iNextBranch, m_iHighestRouteId;
	protected int m_iOpenRadioTxAdmission;
	protected int m_iLastNowMs = -1;
	protected bool m_bSending, m_bClosed;
	protected bool m_bPreparingBranch, m_bReadingOpenRadioRoutes, m_bObservingGameRf;
	protected bool m_bTemporal, m_bManagedReceivers, m_bLiveReceivers, m_bListenerRights;

	bool Configure(RICON_MediaStreamOwner sourceOwner, RICON_LifecycleStateMachine lifecycle,
		RICON_MediaStreamBinding source, int maxBranches, int maxQueuedPerBranch, int maxPayloadBytes,
		RICON_TemporalPublicationOwner publication = null, bool managedReceivers = false,
		RICON_MediaReceiverAuthority receiverAuthority = null, RICON_MediaListenerAuthority listenerAuthority = null,
		int openRadioNetworkId = 0, int nowMs = 0, RICON_GameControllerLifecycle controllerLifecycle = null)
	{
		if (m_bClosed || m_Source || !sourceOwner || !lifecycle || !source || !source.IsValid()
			|| source.m_iSessionEpoch != lifecycle.GetSessionEpoch() || maxBranches <= 0
			|| maxQueuedPerBranch <= 0 || maxPayloadBytes <= 0
			|| (managedReceivers && !publication)
			|| (publication && publication.GetSessionEpoch() != source.m_iSessionEpoch)) return false;
		if (receiverAuthority && (!managedReceivers || !receiverAuthority.IsScope(lifecycle))) return false;
		if (listenerAuthority && !listenerAuthority.IsScope(lifecycle, sourceOwner, receiverAuthority)) return false;
		if (controllerLifecycle && (!listenerAuthority || !controllerLifecycle.MatchesLifecycle(lifecycle))) return false;
		if (openRadioNetworkId < 0) return false;
		if (openRadioNetworkId > 0)
		{
			if (!listenerAuthority || source.m_iNetworkTypeId != openRadioNetworkId) return false;
			m_iOpenRadioTxAdmission = receiverAuthority.ReadOpenRadioSource(source, nowMs);
			if (m_iOpenRadioTxAdmission <= 0) return false;
		}
		m_SourceOwner = sourceOwner;
		m_Lifecycle = lifecycle;
		m_Publication = publication;
		m_ReceiverAuthority = receiverAuthority;
		m_ListenerAuthority = listenerAuthority;
		m_GameControllerLifecycle = controllerLifecycle;
		m_bListenerRights = listenerAuthority != null;
		m_bLiveReceivers = receiverAuthority != null;
		m_bTemporal = publication != null;
		m_bManagedReceivers = managedReceivers;
		m_Source = source.CloneBinding();
		m_iMaxBranches = maxBranches;
		m_iMaxQueued = maxQueuedPerBranch;
		m_iMaxPayloadBytes = maxPayloadBytes;
		return true;
	}

	bool BindRelaySink(RICON_MediaRelayFanoutSink sink, RICON_MediaStreamBinding source, int nowMs, string mappedTimebaseId = "")
	{
		if (!sink || m_RelaySink || m_bPreparingBranch || !m_Source || !m_Source.Same(source) || !CurrentSource(nowMs)) return false;
		if (!mappedTimebaseId.IsEmpty() && !RICON_TemporalSnapshots.ValidKey(mappedTimebaseId)) return false;
		m_RelaySink = sink;
		m_sRelayMappedTimebaseId = mappedTimebaseId;
		return true;
	}

	void UnbindRelaySink(RICON_MediaRelayFanoutSink sink)
	{
		if (!sink || m_RelaySink != sink) return;
		m_RelaySink = null;
		m_sRelayMappedTimebaseId = "";
	}

	bool HasRelaySink(RICON_MediaRelayFanoutSink sink)
	{
		return !m_bClosed && sink && m_RelaySink == sink;
	}

	//! Привязка действует только в рамках активного синхронного вызова.
	bool OwnsRelayOffer(RICON_MediaRelayFanoutSink sink, RICON_QueuedMediaBlock block, int deadlineMs, int nowMs)
	{
		return HasRelaySink(sink) && block && block == m_RelayOfferBlock
			&& deadlineMs == m_iRelayOfferDeadlineMs && nowMs == m_iRelayOfferAtMs
			&& m_SourceOwner && m_SourceOwner.CanFanOut(m_Source, m_Lifecycle, this, m_bManagedReceivers, nowMs);
	}

	protected bool CurrentSource(int nowMs)
	{
		if (m_bClosed || m_bSending || !m_Source) return false;
		if (!m_Lifecycle || !m_SourceOwner || m_Lifecycle.GetSessionEpoch() != m_Source.m_iSessionEpoch)
		{ Close(); return false; }
		if (nowMs < m_iLastNowMs || nowMs < m_SourceOwner.LastObservedTimeMs() || !m_Lifecycle.CanQueryAt(nowMs)) return false;
		m_iLastNowMs = nowMs;
		if (!m_SourceOwner.CanFanOut(m_Source, m_Lifecycle, this, m_bManagedReceivers, nowMs)) { Close(); return false; }
		return true;
	}

	protected static bool SameEndpoint(RICON_MediaEgressRoute a, RICON_MediaEgressRoute b)
	{
		return a.m_iListenerPlayerId == b.m_iListenerPlayerId && a.m_iRxDeviceId == b.m_iRxDeviceId
			&& a.m_iRxTransceiverIndex == b.m_iRxTransceiverIndex;
	}

	protected bool CanReadRoutes(int nowMs)
	{
		if (m_bLiveReceivers)
		{
			if (!m_ReceiverAuthority || !m_ReceiverAuthority.IsScope(m_Lifecycle)) { Close(); return false; }
			if (!m_ReceiverAuthority.CanRead(m_Lifecycle, nowMs)) return false;
			if (m_iOpenRadioTxAdmission > 0)
			{
				int admission = m_ReceiverAuthority.ReadOpenRadioSource(m_Source, nowMs);
				if (m_bClosed) return false;
				if (admission != m_iOpenRadioTxAdmission)
				{
					// A physical TX change ends the emission, including its undelivered ingress blocks.
					m_SourceOwner.Retire(m_Source.m_iSessionEpoch, m_Source.m_iLeaseId, nowMs);
					Close();
					return false;
				}
			}
		}
		if (m_bListenerRights)
		{
			if (!m_ListenerAuthority || !m_ListenerAuthority.IsScope(m_Lifecycle, m_SourceOwner, m_ReceiverAuthority))
			{ Close(); return false; }
			if (!m_ListenerAuthority.CanRead(nowMs)) return false;
		}
		if (!m_bTemporal) return true;
		if (!m_Publication) { Close(); return false; }
		// Unavailable proof pauses existing queues. Only a readable authority can retire a branch.
		return m_Publication.CanReadMediaLinks(m_Lifecycle, nowMs);
	}

	protected bool BranchCurrent(RICON_MediaFanoutBranch branch, int nowMs)
	{
		if (m_bClosed || branch.m_bRetired) return false;
		bool receiverCurrent = !m_bManagedReceivers || RICON_MediaEgressQueue.ReceiverCurrent(branch.m_Route, m_Lifecycle, nowMs);
		if (receiverCurrent && m_bTemporal)
			receiverCurrent = branch.m_iGeometryRevision == m_Publication.GetMediaGeometryRevision();
		if (receiverCurrent && m_bTemporal)
			receiverCurrent = branch.m_iListenerIncarnation == m_Publication.GetMediaListenerIncarnation(branch.m_Route);
		if (m_bClosed || branch.m_bRetired) return false;
		if (receiverCurrent && m_bLiveReceivers)
			receiverCurrent = m_ReceiverAuthority.ReadAdmission(branch.m_Route, nowMs) == branch.m_iRxAdmission;
		if (m_bClosed || branch.m_bRetired) return false;
		if (receiverCurrent && m_bListenerRights)
			receiverCurrent = m_ListenerAuthority.MatchesGrant(branch.m_Route, branch.m_iRxAdmission, nowMs);
		if (m_bClosed || branch.m_bRetired) return false;
		bool linkCurrent = receiverCurrent && (!m_bTemporal
			|| m_Publication.MatchesCurrentMediaLink(branch.m_iTemporalLinkId, branch.m_Route, m_Lifecycle, nowMs));
		if (m_bClosed || branch.m_bRetired) return false;
		if (linkCurrent) return true;
		if (m_iOpenRadioTxAdmission > 0) m_ListenerAuthority.Revoke(branch.m_Route.m_iPolicyRevision);
		RetireBranch(branch, nowMs);
		return false;
	}

	protected void RetireBranch(RICON_MediaFanoutBranch branch, int nowMs)
	{
		if (branch.m_bRetired) return;
		branch.m_bRetired = true;
		branch.m_Queue.Close();
		branch.m_PendingRfCoverage = null;
		branch.m_RfCoverageSource = null;
		branch.m_PrecedingGameRfPoint = null;
		branch.m_PrecedingGameRfBudget = null;
		if (branch.m_RfHistory) branch.m_RfHistory.Close();
		if (m_DispatchTransport) m_DispatchTransport.RetireBranch(branch.m_Route.CloneRoute(), nowMs);
	}

	//! The wire publisher may install only a branch still held by this exact managed authority.
	bool ReadWireInstall(RICON_MediaEgressRoute route, int nowMs, out int firstSampleIndex,
		RICON_LifecycleStateMachine lifecycle, RICON_GameControllerLifecycle bridge)
	{
		firstSampleIndex = 0;
		if (!route || !route.IsValid() || !bridge || m_GameControllerLifecycle != bridge || m_Lifecycle != lifecycle) return false;
		if (!m_bManagedReceivers || !m_bListenerRights || !bridge.MatchesLifecycle(lifecycle)) return false;
		if (!CurrentSource(nowMs) || !CanReadRoutes(nowMs)) return false;
		foreach (RICON_MediaFanoutBranch branch : m_Branches)
		{
			if (!branch.m_Route.Same(route)) continue;
			if (!BranchCurrent(branch, nowMs) || !RecipientControllerCurrent(route)) return false;
			firstSampleIndex = branch.m_iFirstSampleIndex;
			return true;
		}
		return false;
	}

	//! Local server RF data is readable only for an exact current branch at its committed tick.
	RICON_TemporalRfBudget ReadCurrentRfBudget(RICON_MediaEgressRoute route, int nowMs)
	{
		RICON_MediaFanoutBranch branch = FindRfBranch(route);
		return ReadBranchRfBudget(branch, nowMs);
	}

	//! An authority-side reader for a current managed receiver before its first media block.
	RICON_MediaEgressRoute ReadCurrentRoute(int listenerPlayerId, int rxDeviceId, int rxIndex, int nowMs)
	{
		if (m_bPreparingBranch || !HasCommittedRfTick(nowMs)) return null;
		if (!CurrentSource(nowMs) || !CanReadRoutes(nowMs)) return null;
		foreach (RICON_MediaFanoutBranch branch : m_Branches)
		{
			RICON_MediaEgressRoute route = branch.m_Route;
			if (route.m_iListenerPlayerId != listenerPlayerId || route.m_iRxDeviceId != rxDeviceId || route.m_iRxTransceiverIndex != rxIndex) continue;
			if (!BranchCurrent(branch, nowMs) || !RecipientControllerCurrent(route)) return null;
			return route.CloneRoute();
		}
		return null;
	}

	//! Refresh the server's current RADIO branches without consuming PCM or granting transport permission.
	bool ReadOpenRadioRoutes(int nowMs, array<ref RICON_MediaEgressRoute> routes)
	{
		if (!routes) return false;
		routes.Clear();
		if (m_bReadingOpenRadioRoutes || m_bPreparingBranch || m_bSending) return false;
		m_bReadingOpenRadioRoutes = true;
		bool accepted = CollectOpenRadioRoutes(nowMs, routes);
		m_bReadingOpenRadioRoutes = false;
		return accepted;
	}

	protected bool CollectOpenRadioRoutes(int nowMs, array<ref RICON_MediaEgressRoute> routes)
	{
		if (!HasCommittedRfTick(nowMs) || !RefreshOpenRadioRoster(nowMs)
			|| !HasCommittedRfTick(nowMs) || !CurrentSource(nowMs)) return false;
		// Finish without a ReadState callback that could invalidate an already checked branch.
		if (!m_ReceiverAuthority.MatchesRecordedOpenAdmission(m_Source, m_Source.m_iDeviceId, m_Source.m_iTxTransceiverIndex,
			0, m_iOpenRadioTxAdmission, nowMs)) return false;
		foreach (RICON_MediaFanoutBranch branch : m_Branches)
		{
			RICON_MediaEgressRoute route = branch.m_Route;
			if (branch.m_bRetired || branch.m_iGeometryRevision != m_Publication.GetMediaGeometryRevision()
				|| branch.m_iListenerIncarnation != m_Publication.GetMediaListenerIncarnation(route)
				|| !m_ReceiverAuthority.MatchesRecordedOpenAdmission(m_Source, route.m_iRxDeviceId,
					route.m_iRxTransceiverIndex, route.m_iRxGeneration, branch.m_iRxAdmission, nowMs)
				|| !m_ListenerAuthority.MatchesGrant(route, branch.m_iRxAdmission, nowMs)
				|| !RICON_MediaEgressQueue.ReceiverCurrent(route, m_Lifecycle, nowMs)
				|| !RecipientControllerCurrent(route)) return false;
		}
		foreach (RICON_MediaFanoutBranch current : m_Branches) routes.Insert(current.m_Route.CloneRoute());
		return true;
	}

	//! A trusted server continuity source starts an observation on an existing grant.
	bool BeginRfCoverage(RICON_MediaEgressRoute route, int nowMs, string timebaseId, RICON_OpenRadioStaticLosSource source)
	{
		if (m_bObservingGameRf) return false;
		RICON_MediaFanoutBranch branch = FindRfBranch(route);
		if (!branch || branch.m_bGameRfCoverage || branch.m_PendingRfCoverage || nowMs < branch.m_iGrantedAtMs) return false;
		if (!m_Publication.CanAttestRfCoverage(source, m_Lifecycle, nowMs)) return false;
		if (branch.m_RfHistory && branch.m_sRfHistoryTimebaseId != timebaseId) return false;
		RICON_TemporalRfBudget budget = ReadBranchRfBudget(branch, nowMs);
		if (!budget || budget.m_sTimebaseId != timebaseId) return false;
		if (!branch.m_RfHistory)
		{
			RICON_MediaRfHistory history = new RICON_MediaRfHistory();
			if (!history.Configure(branch.m_Route, timebaseId, RF_HISTORY_MAX_ENTRIES, RF_HISTORY_MAX_AGE_MS)) return false;
			branch.m_RfHistory = history;
			branch.m_sRfHistoryTimebaseId = timebaseId;
		}
		branch.m_PendingRfCoverage = budget;
		branch.m_RfCoverageSource = source;
		return true;
	}

	//! A trusted server caller attests unchanged RF inputs throughout [start, nowMs).
	bool CompleteRfCoverage(RICON_MediaEgressRoute route, int nowMs, RICON_OpenRadioStaticLosSource source)
	{
		if (m_bObservingGameRf) return false;
		RICON_MediaFanoutBranch branch = FindRfBranch(route);
		if (!branch || branch.m_bGameRfCoverage || !branch.m_PendingRfCoverage || !source || branch.m_RfCoverageSource != source) return false;
		ref RICON_TemporalRfBudget start = branch.m_PendingRfCoverage;
		branch.m_PendingRfCoverage = null;
		branch.m_RfCoverageSource = null;
		if (nowMs <= start.m_iTickMs || nowMs - start.m_iTickMs > RF_COVERAGE_MAX_SPAN_MS) return false;
		if (!m_Publication.CanAttestRfCoverage(source, m_Lifecycle, nowMs)) return false;
		RICON_TemporalRfBudget end = ReadBranchRfBudget(branch, nowMs);
		if (!end || end.m_iLinkId != start.m_iLinkId || end.m_sTimebaseId != start.m_sTimebaseId) return false;
		if (end.m_iModelMs < start.m_iModelMs || (end.m_iModelMs == start.m_iModelMs && end.m_iModelSubUs < start.m_iModelSubUs)) return false;
		if (end.m_fRxPowerDbm != start.m_fRxPowerDbm || end.m_fNoiseDbm != start.m_fNoiseDbm || end.m_fBandwidthHz != start.m_fBandwidthHz) return false;
		if (start.m_RxEnvironment)
		{
			if (!start.m_RxEnvironment.Same(end.m_RxEnvironment) || !start.m_RxEnvironment.Covers(start.m_iTickMs, nowMs)) return false;
		}
		else if (end.m_RxEnvironment) return false;
		return branch.m_RfHistory.RecordCompleted(branch.m_Route, start.m_iTickMs, nowMs, start, nowMs);
	}

	//! A successful result means one new completed interval. A first point only seeds the branch.
	bool ObserveGameRfCoverage(RICON_MediaEgressRoute route, int nowMs, RICON_GameRadioManagerComponent manager)
	{
		ref RICON_MediaFanout held = this;
		if (m_bClosed || m_bObservingGameRf || m_bReadingOpenRadioRoutes || m_iOpenRadioTxAdmission <= 0
			|| !manager || manager != RICON_CommsManagerComponent.GetInstance()) return false;
		ref RICON_MediaFanoutBranch branch = FindRfBranch(route);
		if (!branch || branch.m_bRetired || nowMs < branch.m_iGrantedAtMs || (branch.m_RfHistory && !branch.m_bGameRfCoverage)) return false;
		if (branch.m_PrecedingGameRfPoint && nowMs <= branch.m_PrecedingGameRfPoint.m_iTickMs) return false;
		m_bObservingGameRf = true;
		bool recorded = ObserveGameRfCoverageHeld(branch, nowMs, manager);
		m_bObservingGameRf = false;
		return recorded;
	}

	protected bool ObserveGameRfCoverageHeld(RICON_MediaFanoutBranch branch, int nowMs, RICON_GameRadioManagerComponent manager)
	{
		ref RICON_GameRadioRfPoint previous = branch.m_PrecedingGameRfPoint;
		ref RICON_TemporalRfBudget previousBudget = branch.m_PrecedingGameRfBudget;
		branch.m_PrecedingGameRfPoint = null;
		branch.m_PrecedingGameRfBudget = null;
		ref RICON_MediaEgressRoute route = branch.m_Route.CloneRoute();
		ref RICON_TemporalPublicationOwner publication = m_Publication;
		int revision = m_iRevision;
		int linkId = branch.m_iTemporalLinkId;
		ref RICON_TemporalRfBudget budget = ReadBranchRfBudget(branch, nowMs);
		if (!budget || budget.m_iTickMs != nowMs || budget.m_iLinkId != linkId || budget.m_RxEnvironment
			|| !GameRfBranchHeld(branch, route, publication, revision, linkId)) return false;
		ref RICON_GameRadioRfPoint current = manager.ReadGameRfCoveragePoint(route, linkId, nowMs, publication);
		if (!current || current.m_iTickMs != nowMs || current.m_iDeadlineMs <= nowMs || current.m_iAttempt <= 0
			|| (previous && current.m_iAttempt <= previous.m_iAttempt)
			|| current.m_iTxGeneration <= 0 || current.m_iTxAdmission != m_iOpenRadioTxAdmission
			|| current.m_iRxAdmission != branch.m_iRxAdmission || !GameRfBranchHeld(branch, route, publication, revision, linkId)) return false;
		ref RICON_MediaFreeSpaceMotionLimits limits;
		if (previous && previousBudget && previous.m_iAttempt == current.m_iAttempt - 1 && previous.m_iTickMs < nowMs
			&& previous.m_iDeadlineMs > nowMs && previous.SameScope(current))
			limits = manager.ReadGameRfCoverageLimits(route, previous, current);
		if (!GameRfBranchHeld(branch, route, publication, revision, linkId)
			|| manager != RICON_CommsManagerComponent.GetInstance()) return false;
		ref RICON_TemporalRfBudget confirmed = ReadBranchRfBudget(branch, nowMs);
		if (!budget.Same(confirmed) || !GameRfBranchHeld(branch, route, publication, revision, linkId)) return false;
		ref RICON_MediaReceiverAuthority receivers = m_ReceiverAuthority;
		ref RICON_MediaStreamBinding binding = m_Source;
		if (!receivers.MatchesRecordedOpenAdmission(binding, binding.m_iDeviceId, binding.m_iTxTransceiverIndex,
			current.m_iTxGeneration, current.m_iTxAdmission, nowMs)
			|| !receivers.MatchesRecordedOpenAdmission(binding, route.m_iRxDeviceId, route.m_iRxTransceiverIndex,
				route.m_iRxGeneration, current.m_iRxAdmission, nowMs)
			|| !GameRfBranchHeld(branch, route, publication, revision, linkId)) return false;
		ref RICON_GameRadioRfPoint finalPoint = manager.ReadGameRfCoveragePoint(route, linkId, nowMs, publication);
		if (!current.SamePoint(finalPoint) || m_iLastNowMs != nowMs || manager != RICON_CommsManagerComponent.GetInstance()
			|| !GameRfBranchHeld(branch, route, publication, revision, linkId)) return false;
		if (branch.m_RfHistory && branch.m_sRfHistoryTimebaseId != budget.m_sTimebaseId) return false;
		branch.m_bGameRfCoverage = true;
		branch.m_PrecedingGameRfPoint = current;
		branch.m_PrecedingGameRfBudget = confirmed;
		if (!limits) return false;
		ref RICON_MediaRfHistory history = branch.m_RfHistory;
		if (!history)
		{
			history = new RICON_MediaRfHistory();
			if (!history.Configure(route, budget.m_sTimebaseId, RF_HISTORY_MAX_ENTRIES, RF_HISTORY_MAX_AGE_MS)) return false;
		}
		if (!history.RecordCompletedFreeSpace(route, previousBudget, confirmed, previous.m_fDistanceM, current.m_fDistanceM, limits, nowMs)) return false;
		branch.m_RfHistory = history;
		branch.m_sRfHistoryTimebaseId = budget.m_sTimebaseId;
		return true;
	}

	protected bool GameRfBranchHeld(RICON_MediaFanoutBranch branch, RICON_MediaEgressRoute route,
		RICON_TemporalPublicationOwner publication, int revision, int linkId)
	{
		return !m_bClosed && !branch.m_bRetired && m_Publication == publication && m_iRevision == revision
			&& branch.m_iTemporalLinkId == linkId && FindRfBranch(route) == branch;
	}

	protected RICON_MediaFanoutBranch FindRfBranch(RICON_MediaEgressRoute route)
	{
		if (!route || !route.IsValid() || m_bPreparingBranch || m_bSending || !m_Publication) return null;
		if (!m_bTemporal || !m_bManagedReceivers || !m_bLiveReceivers || !m_bListenerRights) return null;
		foreach (RICON_MediaFanoutBranch branch : m_Branches)
		{
			if (branch.m_Route.Same(route)) return branch;
		}
		return null;
	}

	protected RICON_TemporalRfBudget ReadBranchRfBudget(RICON_MediaFanoutBranch branch, int nowMs)
	{
		if (!branch || branch.m_bRetired || !HasCommittedRfTick(nowMs)) return null;
		if (!CurrentSource(nowMs) || !CanReadRoutes(nowMs)) return null;
		if (!BranchCurrent(branch, nowMs) || !RecipientControllerCurrent(branch.m_Route)) return null;
		if (m_bClosed || branch.m_bRetired || !m_Branches.Contains(branch)) return null;
		return m_Publication.ReadMediaRfBudget(branch.m_iTemporalLinkId, branch.m_Route, m_Lifecycle, nowMs);
	}

	protected bool HasCommittedRfTick(int nowMs)
	{
		if (!m_bTemporal || !m_bManagedReceivers || !m_bLiveReceivers || !m_bListenerRights || !m_Publication) return false;
		return m_Publication.HasCommittedMediaTick(m_Lifecycle, nowMs);
	}

	bool ReplaceRoutes(int revision, array<ref RICON_MediaFanoutGrant> grants, int nowMs)
	{
		if (m_bManagedReceivers || m_bPreparingBranch) return false;
		if (revision <= m_iRevision || !grants || grants.Count() > m_iMaxBranches || !CurrentSource(nowMs) || !CanReadRoutes(nowMs)) return false;
		return CommitRoutes(revision, grants, nowMs);
	}

	//! Full trusted server snapshot. A rejected newer snapshot cannot leave previous grants in force.
	bool ReplaceReceivers(int revision, array<ref RICON_MediaReceiverApproval> approvals, int nowMs)
	{
		if (m_iOpenRadioTxAdmission > 0 || m_bPreparingBranch) return false;
		if (!m_bManagedReceivers || revision <= m_iRevision || !CurrentSource(nowMs)) return false;
		if (CanReadRoutes(nowMs) && approvals && approvals.Count() <= m_iMaxBranches)
		{
			array<ref RICON_MediaFanoutGrant> grants = {};
			int nextRouteId = m_iHighestRouteId;
			foreach (RICON_MediaReceiverApproval approval : approvals)
			{
				RICON_MediaFanoutGrant grant = BuildReceiverGrant(approval, nowMs, nextRouteId);
				if (!grant) break;
				if (grant.m_Route.m_iRouteId > nextRouteId) nextRouteId = grant.m_Route.m_iRouteId;
				grants.Insert(grant);
			}
			if (grants.Count() == approvals.Count() && CommitRoutes(revision, grants, nowMs)) return true;
		}
		ClearBranches();
		m_iRevision = revision;
		return false;
	}

	//! No caller-provided recipient set. Each refresh reads the authority's committed direct pairs.
	bool CanPumpOpenRadio(RICON_TemporalPublicationOwner publication)
	{
		return !m_bClosed && m_iOpenRadioTxAdmission > 0 && publication && publication == m_Publication;
	}

	bool RefreshOpenRadio(int nowMs)
	{
		return !m_bReadingOpenRadioRoutes && RefreshOpenRadioRoster(nowMs);
	}

	protected bool RefreshOpenRadioRoster(int nowMs)
	{
		if (m_bPreparingBranch) return false;
		if (m_iOpenRadioTxAdmission <= 0 || !CurrentSource(nowMs) || !CanReadRoutes(nowMs)) return false;
		if (m_iRevision >= int.MAX - 1) { Close(); return false; }
		array<ref RICON_MediaDirectCandidate> candidates = {};
		if (!m_Publication.ReadDirectMediaCandidates(m_Source, m_Lifecycle, nowMs, candidates)) return false;
		array<ref RICON_MediaReceiverApproval> approvals = {};
		foreach (RICON_MediaDirectCandidate candidate : candidates)
		{
			int generation = m_ReceiverAuthority.ReadOpenRadioReceiver(m_Source, candidate.m_iRxDeviceId,
				candidate.m_iRxTransceiverIndex, m_iOpenRadioTxAdmission, nowMs);
			if (generation <= 0 || !RadioListenerEligible(candidate, nowMs)) continue;
			if (approvals.Count() >= m_iMaxBranches) { ClearBranches(); return false; }
			RICON_MediaReceiverApproval approval = new RICON_MediaReceiverApproval();
			approval.m_iListenerPlayerId = candidate.m_iListenerPlayerId;
			approval.m_iRxDeviceId = candidate.m_iRxDeviceId;
			approval.m_iRxTransceiverIndex = candidate.m_iRxTransceiverIndex;
			approval.m_iRxGeneration = generation;
			approval.m_iTemporalLinkId = candidate.m_iLinkId;
			approvals.Insert(approval);
		}
		return CommitOpenRadio(approvals, nowMs);
	}

	protected bool RadioListenerEligible(RICON_MediaDirectCandidate candidate, int nowMs)
	{
		int listener = candidate.m_iListenerPlayerId;
		int connection = m_Lifecycle.GetPlayerConnectionId(listener);
		if (connection <= 0 || connection == m_Source.m_iConnectionId || candidate.m_iRxDeviceId == m_Source.m_iDeviceId) return false;
		RICON_LifecycleDevice device = m_Lifecycle.QueryDevice(candidate.m_iRxDeviceId, nowMs);
		return device && device.m_bPowered && device.CanAccess(listener) && device.Allows(m_Source.m_iNetworkTypeId, m_Source.m_iChannelId);
	}

	protected bool CommitOpenRadio(array<ref RICON_MediaReceiverApproval> approvals, int nowMs)
	{
		// An obsolete branch must release its policy before a replacement reserves a fresh revision.
		foreach (RICON_MediaFanoutBranch branch : m_Branches)
		{
			bool stillCandidate;
			foreach (RICON_MediaReceiverApproval approval : approvals)
			{
				if (approval.m_iTemporalLinkId == branch.m_iTemporalLinkId && approval.m_iRxGeneration == branch.m_Route.m_iRxGeneration)
				{ stillCandidate = true; break; }
			}
			if (stillCandidate && BranchCurrent(branch, nowMs)) continue;
			m_ListenerAuthority.Revoke(branch.m_Route.m_iPolicyRevision);
			RetireBranch(branch, nowMs);
		}
		array<int> created = {};
		array<ref RICON_MediaFanoutGrant> grants = {};
		int nextRouteId = m_iHighestRouteId;
		foreach (RICON_MediaReceiverApproval approval : approvals)
		{
			bool issued;
			approval.m_iPolicyRevision = m_ListenerAuthority.GrantTracked(m_Source, approval.m_iListenerPlayerId,
				approval.m_iRxDeviceId, approval.m_iRxTransceiverIndex, approval.m_iRxGeneration, nowMs, issued);
			if (approval.m_iPolicyRevision <= 0) break;
			if (issued) created.Insert(approval.m_iPolicyRevision);
			RICON_MediaFanoutGrant grant = BuildReceiverGrant(approval, nowMs, nextRouteId);
			if (!grant) break;
			if (grant.m_Route.m_iRouteId > nextRouteId) nextRouteId = grant.m_Route.m_iRouteId;
			grants.Insert(grant);
		}
		if (grants.Count() == approvals.Count() && CommitRoutes(m_iRevision + 1, grants, nowMs)) return true;
		foreach (int revision : created) m_ListenerAuthority.Revoke(revision);
		ClearBranches();
		return false;
	}

	protected RICON_MediaFanoutGrant BuildReceiverGrant(RICON_MediaReceiverApproval approval, int nowMs, int nextRouteId)
	{
		if (!approval || approval.m_iTemporalLinkId <= 0) return null;
		RICON_MediaEgressRoute route = new RICON_MediaEgressRoute();
		route.m_Source = m_Source;
		route.m_iListenerPlayerId = approval.m_iListenerPlayerId;
		route.m_iListenerConnectionId = m_Lifecycle.GetPlayerConnectionId(approval.m_iListenerPlayerId);
		route.m_iListenerGeneration = m_Lifecycle.GetPlayerGeneration(approval.m_iListenerPlayerId);
		route.m_iListenerEntityId = m_Lifecycle.GetPlayerEntityId(approval.m_iListenerPlayerId);
		route.m_iRxDeviceId = approval.m_iRxDeviceId;
		route.m_iRxTransceiverIndex = approval.m_iRxTransceiverIndex;
		route.m_iRxGeneration = approval.m_iRxGeneration;
		route.m_iPolicyRevision = approval.m_iPolicyRevision;
		route.m_iRouteGeneration = 1;
		RICON_MediaFanoutGrant grant = new RICON_MediaFanoutGrant();
		grant.m_Route = route;
		grant.m_iTemporalLinkId = approval.m_iTemporalLinkId;
		if (m_bTemporal) grant.m_iGeometryRevision = m_Publication.GetMediaGeometryRevision();
		if (m_bTemporal) grant.m_iListenerIncarnation = m_Publication.GetMediaListenerIncarnation(route);
		if (grant.m_iListenerIncarnation < 0) return null;
		grant.m_iGrantedAtMs = nowMs;
		if (m_bLiveReceivers)
		{
			grant.m_iRxAdmission = m_ReceiverAuthority.ReadAdmission(route, nowMs);
			if (!grant.m_iRxAdmission) return null;
		}
		if (m_bListenerRights && !m_ListenerAuthority.MatchesGrant(route, grant.m_iRxAdmission, nowMs)) return null;
		foreach (RICON_MediaFanoutBranch existing : m_Branches)
		{
			if (!SameEndpoint(existing.m_Route, route)) continue;
			if (route.m_iRxGeneration < existing.m_Route.m_iRxGeneration
				|| route.m_iPolicyRevision < existing.m_Route.m_iPolicyRevision) return null;
			route.m_iRouteId = existing.m_Route.m_iRouteId;
			if (!existing.m_bRetired && existing.m_iRxAdmission == grant.m_iRxAdmission
				&& existing.m_iGeometryRevision == grant.m_iGeometryRevision
				&& existing.m_iListenerIncarnation == grant.m_iListenerIncarnation
				&& existing.m_iTemporalLinkId == grant.m_iTemporalLinkId && existing.m_Route.Same(route))
			{
				grant.m_iGrantedAtMs = existing.m_iGrantedAtMs;
				return grant;
			}
			break;
		}
		if (nextRouteId >= int.MAX - 1) return null;
		route.m_iRouteId = nextRouteId + 1;
		return grant;
	}

	protected bool CommitRoutes(int revision, array<ref RICON_MediaFanoutGrant> grants, int nowMs)
	{
		array<ref RICON_MediaFanoutBranch> next = {};
		int highest = m_iHighestRouteId;
		foreach (RICON_MediaFanoutGrant grant : grants)
		{
			if (!grant || !grant.m_Route || !grant.m_Route.IsValid() || !m_Source.Same(grant.m_Route.m_Source)
				|| grant.m_iFirstSampleIndex < 0) return false;
			if (m_bTemporal && !m_Publication.MatchesCurrentMediaLink(grant.m_iTemporalLinkId, grant.m_Route, m_Lifecycle, nowMs)) return false;
			if (!m_bTemporal && grant.m_iTemporalLinkId != 0) return false;
			int geometryRevision;
			int listenerIncarnation;
			if (m_bTemporal) geometryRevision = m_Publication.GetMediaGeometryRevision();
			if (m_bTemporal) listenerIncarnation = m_Publication.GetMediaListenerIncarnation(grant.m_Route);
			if (listenerIncarnation < 0) return false;
			if (m_bManagedReceivers && !RICON_MediaEgressQueue.ReceiverCurrent(grant.m_Route, m_Lifecycle, nowMs)) return false;
			foreach (RICON_MediaFanoutBranch candidate : next)
			{
				if (candidate.m_Route.m_iRouteId == grant.m_Route.m_iRouteId || SameEndpoint(candidate.m_Route, grant.m_Route)) return false;
			}
			RICON_MediaFanoutBranch retained;
			bool knownId;
			foreach (RICON_MediaFanoutBranch existing : m_Branches)
			{
				if (existing.m_Route.m_iRouteId != grant.m_Route.m_iRouteId) continue;
				knownId = true;
				if (existing.m_bRetired || existing.m_iRxAdmission != grant.m_iRxAdmission || existing.m_iTemporalLinkId != grant.m_iTemporalLinkId
					|| existing.m_iGeometryRevision != geometryRevision
					|| existing.m_iListenerIncarnation != listenerIncarnation
					|| !SameEndpoint(existing.m_Route, grant.m_Route) || grant.m_iFirstSampleIndex < existing.m_iFirstSampleIndex
					|| grant.m_iGrantedAtMs < existing.m_iGrantedAtMs
					|| grant.m_Route.m_iRouteGeneration < existing.m_Route.m_iRouteGeneration
					|| grant.m_Route.m_iPolicyRevision < existing.m_Route.m_iPolicyRevision) return false;
				if (existing.m_Route.Same(grant.m_Route) && existing.m_iFirstSampleIndex == grant.m_iFirstSampleIndex
					&& existing.m_iGrantedAtMs == grant.m_iGrantedAtMs)
					retained = existing;
				else if (grant.m_Route.m_iRouteGeneration == existing.m_Route.m_iRouteGeneration
					&& grant.m_Route.m_iPolicyRevision == existing.m_Route.m_iPolicyRevision) return false;
				break;
			}
			// A removed ID stays retired without retaining an unbounded route history.
			if (!knownId && grant.m_Route.m_iRouteId <= m_iHighestRouteId) return false;
			if (grant.m_Route.m_iRouteId > highest) highest = grant.m_Route.m_iRouteId;
			if (retained)
			{
				next.Insert(retained);
				continue;
			}
			RICON_MediaFanoutBranch branch = new RICON_MediaFanoutBranch();
			branch.m_Route = grant.m_Route.CloneRoute();
			branch.m_iFirstSampleIndex = grant.m_iFirstSampleIndex;
			branch.m_iGrantedAtMs = grant.m_iGrantedAtMs;
			branch.m_iTemporalLinkId = grant.m_iTemporalLinkId;
			branch.m_iRxAdmission = grant.m_iRxAdmission;
			branch.m_iGeometryRevision = geometryRevision;
			branch.m_iListenerIncarnation = listenerIncarnation;
			branch.m_Queue = new RICON_MediaEgressQueue();
			if (!branch.m_Queue.Configure(m_SourceOwner, m_Lifecycle, branch.m_Route, m_iMaxQueued, m_iMaxPayloadBytes)) return false;
			next.Insert(branch);
		}
		int nextBranch;
		if (m_iOpenRadioTxAdmission > 0 && !m_Branches.IsEmpty())
		{
			for (int offset = 0; offset < m_Branches.Count(); offset++)
			{
				int priorIndex = (m_iNextBranch + offset) % m_Branches.Count();
				int retainedIndex = next.Find(m_Branches[priorIndex]);
				if (retainedIndex < 0) continue;
				nextBranch = retainedIndex;
				break;
			}
		}
		foreach (RICON_MediaFanoutBranch oldBranch : m_Branches)
		{
			if (!next.Contains(oldBranch)) RetireBranch(oldBranch, nowMs);
		}
		m_Branches = next;
		m_iNextBranch = nextBranch;
		m_iRevision = revision;
		m_iHighestRouteId = highest;
		return true;
	}

	int FanOutOne(int nowMs, int deadlineMs, int maxCaptureAgeMs = 0, string rfTimebaseId = "", bool historicalRf = false)
	{
		if (m_bPreparingBranch) return -1;
		if (m_Source && m_SourceOwner && m_SourceOwner.UsesMappedCapture(m_Source) != historicalRf) return -1;
		if (historicalRf && (rfTimebaseId.IsEmpty() || !HasCommittedRfTick(nowMs))) return -1;
		if (m_iRevision == 0 || !CurrentSource(nowMs) || !CanReadRoutes(nowMs) || deadlineMs <= nowMs || maxCaptureAgeMs < 0) return -1;
		ref RICON_QueuedMediaBlock block = m_SourceOwner.Take(m_Source.m_iSessionEpoch, m_Source.m_iLeaseId, nowMs, this);
		if (!block) return -1;
		RICON_MediaCaptureEnvelope envelope = block.m_CaptureEnvelope;
		if ((envelope != null) != historicalRf) return 0;
		if (historicalRf && (envelope.m_sTimebaseId != rfTimebaseId || !envelope.Matches(block.m_Binding, block.m_iFirstSampleIndex, block.m_iSampleCount))) return 0;
		if (maxCaptureAgeMs > 0)
		{
			if (nowMs - block.m_iCaptureLowerMs >= maxCaptureAgeMs) return 0;
			if (block.m_iCaptureLowerMs <= int.MAX - maxCaptureAgeMs)
		{
			int captureDeadlineMs = block.m_iCaptureLowerMs + maxCaptureAgeMs;
			if (captureDeadlineMs < deadlineMs) deadlineMs = captureDeadlineMs;
		}
		}
		int queued = 0;
		foreach (RICON_MediaFanoutBranch branch : m_Branches)
		{
			if (!BranchCurrent(branch, nowMs)) continue;
			// Require a strictly later capture bound in the shared time domain to drop same-ms ambiguity.
			if (block.m_iCaptureLowerMs <= branch.m_iGrantedAtMs || block.m_iFirstSampleIndex < branch.m_iFirstSampleIndex) continue;
			RICON_TemporalRfBudget rfBudget;
			RICON_MediaRfHistoryEntry historicalEntry;
			if (historicalRf)
			{
				if (!branch.m_RfHistory || !RecipientControllerCurrent(branch.m_Route)) continue;
				historicalEntry = branch.m_RfHistory.Select(branch.m_Route, envelope, nowMs);
				if (!historicalEntry) continue;
				rfBudget = historicalEntry.m_Budget;
			}
			else if (!rfTimebaseId.IsEmpty())
			{
				// An explicit point sample can name this tick. Older capture needs historical RF evidence.
				if (block.m_iCaptureLowerMs != nowMs || block.m_iCaptureUpperMs != nowMs) continue;
				rfBudget = ReadCurrentRfBudget(branch.m_Route, nowMs);
				if (!rfBudget || rfBudget.m_sTimebaseId != rfTimebaseId) continue;
			}
			if (branch.m_Queue.Enqueue(block, deadlineMs, nowMs, rfBudget, historicalEntry)) queued++;
		}
		if (m_RelaySink && rfTimebaseId == m_sRelayMappedTimebaseId && historicalRf == !m_sRelayMappedTimebaseId.IsEmpty()
			&& CurrentSource(nowMs))
		{
			// Прямые очереди уже получили копии, поэтому результат Offer не влияет на их постановку.
			ref RICON_MediaRelayFanoutSink sink = m_RelaySink;
			m_bSending = true;
			m_RelayOfferBlock = block;
			m_iRelayOfferDeadlineMs = deadlineMs;
			m_iRelayOfferAtMs = nowMs;
			sink.Offer(this, block, deadlineMs, nowMs);
			m_RelayOfferBlock = null;
			m_bSending = false;
		}
		return queued;
	}

	RICON_EMediaDispatchDecision DispatchOne(int nowMs, RICON_MediaFanoutTransport transport)
	{
		if (!transport || m_bPreparingBranch || m_iRevision == 0) return RICON_EMediaDispatchDecision.NOT_READY;
		if (m_Source && m_SourceOwner && m_SourceOwner.UsesMappedCapture(m_Source) && !HasCommittedRfTick(nowMs))
			return RICON_EMediaDispatchDecision.NOT_READY;
		if (!CurrentSource(nowMs) || !CanReadRoutes(nowMs)) return RICON_EMediaDispatchDecision.NOT_READY;
		if (m_DispatchTransport && m_DispatchTransport != transport && RICON_MediaRpcTransport.Cast(m_DispatchTransport))
			return RICON_EMediaDispatchDecision.NOT_READY;
		m_DispatchTransport = transport;
		bool waiting;
		for (int i = 0; i < m_Branches.Count(); i++)
		{
			RICON_MediaFanoutBranch branch = m_Branches[m_iNextBranch];
			m_iNextBranch++;
			if (m_iNextBranch == m_Branches.Count()) m_iNextBranch = 0;
			if (!BranchCurrent(branch, nowMs)) continue;
			RICON_MediaEgressItem head = branch.m_Queue.ReadHead(branch.m_Route, nowMs);
			if (!head) continue;
			bool historicalRf = head.m_Block.m_RfHistory != null;
			if (historicalRf && !HasCommittedRfTick(nowMs)) return RICON_EMediaDispatchDecision.NOT_READY;
			if (!RecipientControllerCurrent(head.m_Route)) return RICON_EMediaDispatchDecision.REFUSED;
			m_bPreparingBranch = true;
			bool prepared = transport.PrepareBranch(this, head, nowMs);
			m_bPreparingBranch = false;
			if (!CurrentSource(nowMs) || !CanReadRoutes(nowMs)) return RICON_EMediaDispatchDecision.NOT_READY;
			if (historicalRf && !HasCommittedRfTick(nowMs)) return RICON_EMediaDispatchDecision.NOT_READY;
			if (!BranchCurrent(branch, nowMs)) continue;
			if (!prepared) { waiting = true; continue; }
			RICON_MediaEgressItem item = branch.m_Queue.Take(branch.m_Route, nowMs);
			if (!item) continue;
			if (!RecipientControllerCurrent(item.m_Route)) return RICON_EMediaDispatchDecision.REFUSED;
			m_bSending = true;
			bool sent = transport.Send(item);
			m_bSending = false;
			if (sent) return RICON_EMediaDispatchDecision.ACCEPTED;
			return RICON_EMediaDispatchDecision.REFUSED;
		}
		if (waiting) return RICON_EMediaDispatchDecision.NOT_READY;
		return RICON_EMediaDispatchDecision.EMPTY;
	}

	protected bool RecipientControllerCurrent(RICON_MediaEgressRoute route)
	{
		if (!m_GameControllerLifecycle) return true;
		if (!m_GameControllerLifecycle.MatchesLifecycle(m_Lifecycle)) return false;
		if (RplSession.Mode() != RplMode.None)
			return m_GameControllerLifecycle.ResolveRemoteRecipient(route.m_iListenerPlayerId, route.m_iListenerConnectionId,
				route.m_iListenerGeneration, route.m_iListenerEntityId) != null;
		int connectionId, generation, entityId;
		return m_GameControllerLifecycle.ReadBoundPlayer(route.m_iListenerPlayerId, connectionId, generation, entityId)
			&& connectionId == route.m_iListenerConnectionId && generation == route.m_iListenerGeneration
			&& entityId == route.m_iListenerEntityId;
	}

	void Close()
	{
		m_bClosed = true;
		m_RelaySink = null;
		m_RelayOfferBlock = null;
		ClearBranches();
		m_Source = null;
		m_SourceOwner = null;
		m_Lifecycle = null;
		m_Publication = null;
		m_ReceiverAuthority = null;
		m_ListenerAuthority = null;
		m_GameControllerLifecycle = null;
		m_DispatchTransport = null;
	}

	protected void ClearBranches()
	{
		foreach (RICON_MediaFanoutBranch branch : m_Branches)
		{
			if (m_iOpenRadioTxAdmission > 0 && m_ListenerAuthority) m_ListenerAuthority.Revoke(branch.m_Route.m_iPolicyRevision);
			RetireBranch(branch, m_iLastNowMs);
		}
		m_Branches.Clear();
		m_iNextBranch = 0;
	}
}
