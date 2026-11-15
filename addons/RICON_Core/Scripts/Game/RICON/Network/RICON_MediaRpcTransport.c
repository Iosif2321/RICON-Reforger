//! Optional direct egress for items admitted by the synchronous fanout.
class RICON_MediaRpcTransport : RICON_MediaFanoutTransport
{
	protected ref RICON_GameControllerLifecycle m_Bridge;
	protected ref RICON_LifecycleStateMachine m_Lifecycle;
	protected ref RICON_MediaWirePublication m_Publication;
	protected RICON_MediaEgressRoute m_RevokeRoute;
	protected int m_iRevokeSeq;
	protected bool m_bClosed;
	protected string m_sRfTimebaseId;
	protected bool m_bHistoricalRf;
	protected RICON_MediaEgressItem m_SendingRfV3Item;

	bool Configure(RICON_GameControllerLifecycle bridge, RICON_LifecycleStateMachine lifecycle,
		int maxRoutes = 64, int ackTimeoutMs = 500, int maxUsedPolicies = 4096, string rfTimebaseId = "", bool historicalRf = false)
	{
		if (m_bClosed || m_Bridge || !bridge || !bridge.MatchesLifecycle(lifecycle)) return false;
		if (historicalRf && rfTimebaseId.IsEmpty()) return false;
		if (!rfTimebaseId.IsEmpty() && !RICON_TemporalSnapshots.ValidKey(rfTimebaseId)) return false;
		RplMode mode = RplSession.Mode();
		if (mode != RplMode.Dedicated && mode != RplMode.Listen) return false;
		RICON_MediaWirePublication publication = new RICON_MediaWirePublication();
		if (!publication.Configure(lifecycle.GetSessionEpoch(), maxRoutes, ackTimeoutMs, maxUsedPolicies)) return false;
		m_Publication = publication;
		m_Bridge = bridge;
		m_Lifecycle = lifecycle;
		m_sRfTimebaseId = rfTimebaseId;
		m_bHistoricalRf = historicalRf;
		return true;
	}

	string GetRfTimebaseId() { return m_sRfTimebaseId; }
	int GetWireVersion()
	{
		if (m_bHistoricalRf) return 3;
		if (m_sRfTimebaseId.IsEmpty()) return 1;
		return 2;
	}

	override bool PrepareBranch(RICON_MediaFanout fanout, RICON_MediaEgressItem item, int nowMs)
	{
		if (!IsCurrent() || !fanout || !item || !item.m_Route || !item.m_Block || nowMs >= item.m_iDeadlineMs) return false;
		RICON_MediaEgressRoute route = item.m_Route;
		RICON_QueuedMediaBlock block = item.m_Block;
		if (!m_bHistoricalRf && (block.m_RfHistory || block.m_CaptureEnvelope)) return false;
		if (item.m_iPayloadBytes < 1 || item.m_iPayloadBytes > RICON_MediaWireV1.MAX_PAYLOAD_BYTES
			|| block.m_iSampleCount < 1 || block.m_iSampleCount > RICON_MediaWireV1.MAX_SAMPLE_COUNT
			|| block.m_iFirstSampleIndex < 0 || block.m_iFirstSampleIndex > int.MAX - block.m_iSampleCount) return false;
		if (m_bHistoricalRf)
		{
			if (!RICON_MediaWireRfV3.Matches(item, m_sRfTimebaseId)) return false;
		}
		else if (!m_sRfTimebaseId.IsEmpty() && !RICON_MediaWireRfV2.Matches(item, block.m_RfBudget, m_sRfTimebaseId)) return false;
		int cutover;
		if (!fanout.ReadWireInstall(route, nowMs, cutover, m_Lifecycle, m_Bridge)
			|| !route.m_Source.Same(block.m_Binding) || block.m_iFirstSampleIndex < cutover) return false;
		SCR_PlayerController recipient = ResolveRecipient(route);
		if (!recipient) return false;
		// Branch and geometry are valid at the committed pump tick. Publication and ACK share the current authority clock.
		int authorityNowMs = m_Bridge.ReadWireAuthorityTime(m_Lifecycle);
		if (authorityNowMs < nowMs || authorityNowMs >= item.m_iDeadlineMs) return false;
		if (m_Publication.CanSend(route, block.m_iSequence, block.m_iFirstSampleIndex, authorityNowMs)) return true;
		if (m_Publication.IsInstallExpired(route, authorityNowMs))
		{
			RetireBranch(route, nowMs);
			return false;
		}

		int seq = m_Publication.Begin(route, block.m_iSequence, block.m_iFirstSampleIndex, authorityNowMs);
		if (seq > 0 && !recipient.RICON_SubmitMediaWireControlV1(this, fanout, route, seq, true, block.m_iSequence,
			block.m_iFirstSampleIndex, nowMs))
		{
			int refusedAtMs = m_Bridge.ReadWireAuthorityTime(m_Lifecycle);
			if (refusedAtMs >= authorityNowMs) m_Publication.Abandon(route, seq, refusedAtMs);
		}
		return false;
	}

	//! The trusted fanout calls this synchronously with its exact admitted queue item.
	override bool Send(RICON_MediaEgressItem item)
	{
		if (!item) return false;
		SCR_PlayerController recipient = ResolveRecipient(item.m_Route);
		if (!recipient) return false;
		if (m_bHistoricalRf)
		{
			if (m_SendingRfV3Item) return false;
			m_SendingRfV3Item = item;
			bool submitted = recipient.RICON_SubmitMediaWireRfV3(this, item);
			m_SendingRfV3Item = null;
			return submitted;
		}
		if (!m_sRfTimebaseId.IsEmpty()) return recipient.RICON_SubmitMediaWireRfV2(this, item);
		return recipient.RICON_SubmitMediaWireV1(this, item);
	}

	override void RetireBranch(RICON_MediaEgressRoute route, int nowMs)
	{
		if (!IsCurrent()) return;
		int authorityNowMs = m_Bridge.ReadWireAuthorityTime(m_Lifecycle);
		if (authorityNowMs >= nowMs && authorityNowMs >= 0) RevokeBranch(route, authorityNowMs);
	}

	//! Run once per active RADIO tick, including ticks without an eligible media head.
	void SweepExpiredInstalls(int nowMs)
	{
		if (!IsCurrent() || m_RevokeRoute) return;
		int authorityNowMs = m_Bridge.ReadWireAuthorityTime(m_Lifecycle);
		if (authorityNowMs < nowMs || authorityNowMs < 0) return;
		array<ref RICON_MediaEgressRoute> expired = {};
		if (!m_Publication.ReadExpired(authorityNowMs, expired)) return;
		foreach (RICON_MediaEgressRoute route : expired) RevokeBranch(route, authorityNowMs);
	}

	//! Close server admission even when the former recipient is no longer reachable.
	bool RevokeBranch(RICON_MediaEgressRoute route, int nowMs)
	{
		if (!IsCurrent() || m_RevokeRoute) return false;
		int seq = m_Publication.Revoke(route, nowMs);
		if (seq <= 0) return false;
		SCR_PlayerController recipient = ResolveRecipient(route);
		if (!recipient) return false;
		m_RevokeRoute = route;
		m_iRevokeSeq = seq;
		bool submitted = recipient.RICON_SubmitMediaWireControlV1(this, null, route, seq, false, 0, 0, nowMs);
		m_RevokeRoute = null;
		m_iRevokeSeq = 0;
		return submitted;
	}

	bool AuthorizeMedia(SCR_PlayerController recipient, RICON_MediaEgressItem item)
	{
		if (!recipient || !item || !item.m_Block || ResolveRecipient(item.m_Route) != recipient) return false;
		if (m_bHistoricalRf && m_SendingRfV3Item != item) return false;
		int nowMs = m_Bridge.ReadWireAuthorityTime(m_Lifecycle);
		return nowMs >= 0 && nowMs < item.m_iDeadlineMs && m_Publication.CanSend(item.m_Route,
			item.m_Block.m_iSequence, item.m_Block.m_iFirstSampleIndex, nowMs);
	}

	bool AuthorizeControl(SCR_PlayerController recipient, RICON_MediaFanout fanout, RICON_MediaEgressRoute route,
		int controlSeq, bool install, int firstSequence, int firstSampleIndex, int nowMs)
	{
		if (!recipient || controlSeq <= 0 || controlSeq >= int.MAX || ResolveRecipient(route) != recipient) return false;
		if (!install)
			return m_RevokeRoute && m_RevokeRoute.Same(route) && m_iRevokeSeq == controlSeq
				&& firstSequence == 0 && firstSampleIndex == 0;
		int cutover;
		if (!fanout || !fanout.ReadWireInstall(route, nowMs, cutover, m_Lifecycle, m_Bridge) || firstSampleIndex < cutover) return false;
		int authorityNowMs = m_Bridge.ReadWireAuthorityTime(m_Lifecycle);
		return authorityNowMs >= nowMs && m_Publication.IsPendingInstall(route, controlSeq, firstSequence, firstSampleIndex, authorityNowMs);
	}

	bool ConfirmInstall(SCR_PlayerController sender, int controlSeq, array<int> routeFields, string profileId,
		out RICON_MediaEgressRoute route, out int firstSampleIndex, string rfTimebaseId = "", int wireVersion = 0)
	{
		route = null;
		firstSampleIndex = 0;
		if (!sender || !IsCurrent() || rfTimebaseId != m_sRfTimebaseId) return false;
		if (wireVersion == 0)
		{
			wireVersion = 1;
			if (!rfTimebaseId.IsEmpty()) wireVersion = 2;
		}
		if (wireVersion != GetWireVersion()) return false;
		int firstSequence;
		route = RICON_MediaWireControlV1.DecodeRoute(routeFields, profileId, firstSequence, firstSampleIndex);
		if (ResolveRecipient(route) != sender) return false;
		int nowMs = m_Bridge.ReadWireAuthorityTime(m_Lifecycle);
		return nowMs >= 0 && m_Publication.Confirm(route, controlSeq, firstSequence, firstSampleIndex, nowMs);
	}

	bool IsCurrent()
	{
		return !m_bClosed && m_Publication && m_Bridge && m_Bridge.MatchesLifecycle(m_Lifecycle);
	}

	protected SCR_PlayerController ResolveRecipient(RICON_MediaEgressRoute route)
	{
		if (!IsCurrent() || !route || !route.IsValid() || route.m_Source.m_iSessionEpoch != m_Lifecycle.GetSessionEpoch()) return null;
		return m_Bridge.ResolveRemoteRecipient(route.m_iListenerPlayerId, route.m_iListenerConnectionId,
			route.m_iListenerGeneration, route.m_iListenerEntityId);
	}

	void Close()
	{
		m_bClosed = true;
		if (m_Publication) m_Publication.Close();
		m_Publication = null;
		m_RevokeRoute = null;
		m_SendingRfV3Item = null;
		m_iRevokeSeq = 0;
		m_Bridge = null;
		m_Lifecycle = null;
	}
}

modded class SCR_PlayerController
{
	protected RICON_MediaRpcTransport m_RICON_WireAckTransport;
	protected ref RICON_MediaWireReceiver m_RICON_WireReceiver;
	protected ref RICON_LocalRxOwner m_RICON_LocalRx;
	protected BaseWorld m_RICON_LocalRxWorld;
	protected IEntity m_RICON_LocalRxEntity;
	protected BaseWorld m_RICON_WireWorld;
	protected IEntity m_RICON_WireEntity;
	protected int m_iRICON_WireEpoch, m_iRICON_WirePlayer, m_iRICON_WireConnection, m_iRICON_WireGeneration, m_iRICON_WireEntityId;
	protected RICON_CommsManagerComponent m_RICON_RelayAckManager;
	protected ref RICON_RelayWireReceiver m_RICON_RelayReceiver;
	protected ref RICON_RelayLocalPlayout m_RICON_RelayPlayout;
	protected ref RICON_RelayOutputQueue m_RICON_RelayOutputQueue;
	protected int m_iRICON_RelayWakeSerial;
	protected bool m_bRICON_RelayPlayoutUsed;
	protected float m_fRICON_RelayReferenceDbm;
	protected ref RICON_RelayDestination m_RICON_RelayDestination;
	protected BaseWorld m_RICON_RelayWorld;
	protected IEntity m_RICON_RelayEntity;
	protected int m_iRICON_RelayEpoch, m_iRICON_RelayWidth;

	//! Возвращает успешность вызова RPC. Допуск и доставка определяются сервером.
	bool RICON_SubmitOpenRadioPcm(int epoch, int leaseId, int streamId, int sequence, int firstSampleIndex, array<int> payload)
	{
		if (!RICON_IsLocalWireOwner() || epoch <= 0 || leaseId <= 0 || streamId <= 0 || sequence < 0
			|| firstSampleIndex < 0 || !payload || payload.Count() != RICON_MediaPcmProfile.PAYLOAD_BYTES) return false;
		Rpc(RpcAsk_RICON_OpenRadioPcm, epoch, leaseId, streamId, sequence, firstSampleIndex, payload);
		return true;
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Server)]
	protected void RpcAsk_RICON_OpenRadioPcm(int epoch, int leaseId, int streamId, int sequence, int firstSampleIndex, array<int> payload)
	{
		if (!Replication.IsServer() || !payload || payload.Count() != RICON_MediaPcmProfile.PAYLOAD_BYTES) return;
		RICON_CommsManagerComponent manager = RICON_CommsManagerComponent.GetInstance();
		if (manager) manager.ReceiveOpenRadioPcm(this, epoch, leaseId, streamId, sequence, firstSampleIndex, payload);
	}

	bool RICON_SubmitRelayReadyRequest(RICON_CommsManagerComponent manager, int epoch)
	{
		if (!Replication.IsServer() || !manager || manager != RICON_CommsManagerComponent.GetInstance()
			|| !manager.AuthorizeRelayReadyRequest(this, epoch)) return false;
		Rpc(RpcDo_RICON_RelayReadyRequest, epoch);
		return true;
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RICON_RelayReadyRequest(int epoch)
	{
		if (epoch == m_iRICON_RelayEpoch) RICON_AnnounceRelayWireReady();
	}

	protected void RICON_AnnounceRelayWireReady()
	{
		if (!RICON_IsCurrentRelayWireReceiver()) return;
		array<int> body = RICON_RelayWireReady.Encode(m_iRICON_RelayEpoch, m_RICON_RelayDestination);
		if (body) Rpc(RpcAsk_RICON_RelayReady, body);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_RelayReady(array<int> body)
	{
		if (!Replication.IsServer()) return;
		RICON_CommsManagerComponent manager = RICON_CommsManagerComponent.GetInstance();
		if (manager) manager.ConfirmRelayWireReady(this, body);
	}

	bool RICON_SubmitRelayWireInstall(RICON_CommsManagerComponent manager, RICON_RelayWireGrant grant)
	{
		if (!Replication.IsServer() || !manager || manager != RICON_CommsManagerComponent.GetInstance()) return false;
		array<int> fields = RICON_RelayWireControl.Encode(grant);
		if (!fields || !manager.AuthorizeRelayWireInstall(this, grant)) return false;
		m_RICON_RelayAckManager = manager;
		Rpc(RpcDo_RICON_RelayControl, RICON_RelayWireControl.INSTALL_SEQ, true, fields, grant.m_sTimebaseId);
		return true;
	}

	bool RICON_SubmitRelayWireRevoke(RICON_CommsManagerComponent manager, RICON_RelayWireGrant grant)
	{
		if (!Replication.IsServer() || !manager || manager != m_RICON_RelayAckManager) return false;
		array<int> fields = RICON_RelayWireControl.Encode(grant);
		if (!fields || !manager.AuthorizeRelayWireRevoke(this, grant)) return false;
		m_RICON_RelayAckManager = null;
		Rpc(RpcDo_RICON_RelayControl, RICON_RelayWireControl.REVOKE_SEQ, false, fields, grant.m_sTimebaseId);
		return true;
	}

	bool RICON_SubmitRelayWireMedia(RICON_CommsManagerComponent manager, array<int> body, string timebaseId)
	{
		if (!Replication.IsServer() || !manager || manager != m_RICON_RelayAckManager
			|| !body || body.Count() != RICON_RelayWire.BODY_VALUES || !manager.AuthorizeRelayWireMedia(this, body, timebaseId)) return false;
		Rpc(RpcDo_RICON_RelayMedia, body, timebaseId);
		return true;
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Owner)]
	protected void RpcDo_RICON_RelayMedia(array<int> body, string timebaseId)
	{
		RICON_OnRelayWireMediaObservation("RAW", body && body.Count() == RICON_RelayWire.BODY_VALUES);
		bool accepted = RICON_IsCurrentRelayWireReceiver()
			&& m_RICON_RelayReceiver.Enqueue(body, timebaseId, m_RICON_RelayDestination, m_iRICON_RelayEpoch, System.GetTickCount());
		if (accepted) RICON_OnRelayWireMediaObservation("ACCEPTED", true);
	}

	//! Observation only. Kept separate from direct wire probes and never includes PCM.
	protected void RICON_OnRelayWireMediaObservation(string stage, bool expectedBodySize)
	{
	}

	//! Trusted presentation owner drains only the live attached receiver.
	RICON_RelayWireFrame RICON_TakeRelayWireFrame()
	{
		if (m_bRICON_RelayPlayoutUsed || !RICON_IsCurrentRelayWireReceiver()) return null;
		return m_RICON_RelayReceiver.Take(System.GetTickCount());
	}

	//! Opt-in owner-side pump for a bound and installed relay stream. No implicit audio sink.
	bool RICON_StartRelayPlayout(int delayMs, int prepareLeadMs, int maxPerWake, float referenceDbm)
	{
		if (m_bRICON_RelayPlayoutUsed || !RICON_IsCurrentRelayWireReceiver() || !m_RICON_RelayReceiver.IsPresentationActive()
			|| !RICON_Num.IsFinite(referenceDbm) || m_iRICON_RelayWakeSerial >= int.MAX) return false;
		ScriptCallQueue queue = GetGame().GetCallqueue();
		if (!queue) return false;
		RICON_RelayLocalPlayout playout = new RICON_RelayLocalPlayout();
		if (!playout.Configure(m_RICON_RelayReceiver, delayMs, prepareLeadMs, maxPerWake)) return false;
		RICON_RelayOutputQueue output = new RICON_RelayOutputQueue();
		if (!output.Configure(m_iRICON_RelayEpoch, m_RICON_RelayDestination, 16, System.GetTickCount())) return false;
		m_RICON_RelayPlayout = playout;
		m_RICON_RelayOutputQueue = output;
		m_bRICON_RelayPlayoutUsed = true;
		m_fRICON_RelayReferenceDbm = referenceDbm;
		m_iRICON_RelayWakeSerial++;
		queue.CallLater(RICON_PumpRelayPlayout, RICON_MediaPcmProfile.FRAME_DURATION_MS, true, m_iRICON_RelayWakeSerial);
		return true;
	}

	protected void RICON_PumpRelayPlayout(int serial)
	{
		if (!m_RICON_RelayPlayout || serial != m_iRICON_RelayWakeSerial || !RICON_IsCurrentRelayWireReceiver()) return;
		array<ref RICON_SharedRxMixResult> windows = {};
		int skipped;
		string status = m_RICON_RelayPlayout.Pump(System.GetTickCount(), m_fRICON_RelayReferenceDbm, windows, skipped);
		if (status != "OK" && status != "WAIT")
		{
			RICON_StopRelayPlayout();
			return;
		}
		foreach (RICON_SharedRxMixResult window : windows)
		{
			if (!m_RICON_RelayPlayout || serial != m_iRICON_RelayWakeSerial || !RICON_IsCurrentRelayWireReceiver()) return;
			ref RICON_RelayOutputQueue output = m_RICON_RelayOutputQueue;
			if (!output) return;
			output.Offer(window, System.GetTickCount());
		}
	}

	//! The timed sink must commit this exact window or discard it.
	RICON_SharedRxMixResult RICON_TakeRelayOutputWindow()
	{
		if (!m_RICON_RelayOutputQueue || !RICON_IsCurrentRelayWireReceiver()) return null;
		ref RICON_RelayOutputQueue output = m_RICON_RelayOutputQueue;
		return output.Take(System.GetTickCount());
	}

	protected void RICON_StopRelayPlayout()
	{
		if (!m_RICON_RelayPlayout) return;
		if (GetGame() && GetGame().GetCallqueue()) GetGame().GetCallqueue().Remove(RICON_PumpRelayPlayout);
		m_RICON_RelayPlayout.Close();
		m_RICON_RelayPlayout = null;
		if (m_RICON_RelayOutputQueue) m_RICON_RelayOutputQueue.Close();
		m_RICON_RelayOutputQueue = null;
		if (m_RICON_RelayReceiver) m_RICON_RelayReceiver.Close();
	}

	string RICON_PrepareRelayPresentation(int targetMs)
	{
		if (m_bRICON_RelayPlayoutUsed || !RICON_IsCurrentRelayWireReceiver()) return "CLOSED";
		return m_RICON_RelayReceiver.PreparePresentation(targetMs, System.GetTickCount());
	}

	string RICON_StageRelayPresentation(RICON_SharedRxPresentationTimeline timeline, int sequence, int targetMs, float referenceDbm)
	{
		if (m_bRICON_RelayPlayoutUsed || !RICON_IsCurrentRelayWireReceiver()) return "CLOSED";
		return m_RICON_RelayReceiver.StagePresentation(timeline, sequence, targetMs, System.GetTickCount(), referenceDbm);
	}

	bool RICON_AdvanceRelayBeforeSlot(RICON_SharedRxPresentationTimeline timeline, int sequence, int targetMs)
	{
		return !m_bRICON_RelayPlayoutUsed && RICON_IsCurrentRelayWireReceiver()
			&& m_RICON_RelayReceiver.AdvanceBeforeSlot(timeline, sequence, targetMs, System.GetTickCount());
	}

	//! Call synchronously at the RX commit boundary; queued playback needs its own live permit.
	RICON_RelayPresentationWindow RICON_CommitRelayPresentation()
	{
		if (m_bRICON_RelayPlayoutUsed || !RICON_IsCurrentRelayWireReceiver()) return null;
		return m_RICON_RelayReceiver.CommitPresentation(System.GetTickCount());
	}

	//! The trusted client control/bootstrap supplies B's current endpoint independently of the install RPC.
	bool RICON_BindRelayWireReceiver(RICON_RelayDestination destination, int epoch, int width, int wireVersion = 1)
	{
		if (!destination || !destination.IsValid() || epoch <= 0 || epoch >= int.MAX || width < 1 || width > 64
			|| (wireVersion != 1 && wireVersion != 2)
			|| !RICON_IsLocalWireOwner() || GetPlayerId() != destination.m_iPlayerId) return false;
		BaseWorld world = GetGame().GetWorld();
		IEntity entity = GetControlledEntity();
		if (!world || world.IsEditMode() || !entity || entity.IsDeleted() || entity.GetWorld() != world) return false;
		// This transport profile permits one publication per epoch. Keep the fence after detaching.
		if (world == m_RICON_RelayWorld && epoch <= m_iRICON_RelayEpoch) return false;
		RICON_CloseRelayWireReceiver();
		m_RICON_RelayReceiver = new RICON_RelayWireReceiver();
		m_bRICON_RelayPlayoutUsed = false;
		if (!m_RICON_RelayReceiver.ExpectWireVersion(wireVersion) || !m_RICON_RelayReceiver.BindOutputController(this))
		{
			RICON_CloseRelayWireReceiver();
			return false;
		}
		m_RICON_RelayDestination = destination.CloneDestination();
		m_RICON_RelayWorld = world;
		m_RICON_RelayEntity = entity;
		m_iRICON_RelayEpoch = epoch;
		m_iRICON_RelayWidth = width;
		RICON_AnnounceRelayWireReady();
		return true;
	}

	protected bool RICON_IsCurrentRelayWireReceiver()
	{
		if (!m_RICON_RelayReceiver) return false;
		if (!RICON_IsLocalWireOwner() || !m_RICON_RelayWorld || GetGame().GetWorld() != m_RICON_RelayWorld
			|| m_RICON_RelayWorld.IsEditMode() || !m_RICON_RelayEntity || m_RICON_RelayEntity.IsDeleted()
			|| m_RICON_RelayEntity.GetWorld() != m_RICON_RelayWorld || GetControlledEntity() != m_RICON_RelayEntity
			|| GetPlayerId() != m_RICON_RelayDestination.m_iPlayerId)
		{
			RICON_CloseRelayWireReceiver();
			return false;
		}
		return true;
	}

	bool RICON_IsRelayOutputReceiverCurrent(RICON_RelayWireReceiver receiver)
	{
		return receiver && receiver == m_RICON_RelayReceiver && RICON_IsCurrentRelayWireReceiver();
	}

	RICON_SharedRxMixResult RICON_CommitRelaySharedRx(RICON_SharedRxPresentationTimeline timeline, float referenceDbm,
		float noiseRatio, array<int> noiseSamples, out int expiryMs)
	{
		expiryMs = -1;
		if (m_bRICON_RelayPlayoutUsed || !timeline || !RICON_IsCurrentRelayWireReceiver()
			|| !timeline.MatchesReceiver(m_iRICON_RelayEpoch, m_RICON_RelayDestination)) return null;
		int nowMs = System.GetTickCount();
		RICON_RelayPresentationWindow window = m_RICON_RelayReceiver.CommitPresentation(nowMs);
		if (window && timeline.OfferRelay(window, referenceDbm, nowMs) != "ACCEPTED") return null;
		return timeline.Commit(nowMs, nowMs, noiseRatio, noiseSamples, expiryMs);
	}

	protected void RICON_CloseRelayWireReceiver()
	{
		RICON_StopRelayPlayout();
		if (m_RICON_RelayReceiver) m_RICON_RelayReceiver.Close();
		m_RICON_RelayReceiver = null;
		m_RICON_RelayDestination = null;
		m_RICON_RelayEntity = null;
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RICON_RelayControl(int controlSeq, bool install, array<int> fields, string timebaseId)
	{
		if (!RICON_IsCurrentRelayWireReceiver()
			|| !m_RICON_RelayReceiver.ApplyControl(controlSeq, install, fields, timebaseId,
				m_RICON_RelayDestination, m_iRICON_RelayEpoch, m_iRICON_RelayWidth)) return;
		if (install) Rpc(RpcAsk_RICON_RelayInstallAck, controlSeq, fields, timebaseId);
		else RICON_CloseRelayWireReceiver();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_RelayInstallAck(int controlSeq, array<int> fields, string timebaseId)
	{
		if (!Replication.IsServer() || !m_RICON_RelayAckManager
			|| m_RICON_RelayAckManager != RICON_CommsManagerComponent.GetInstance()) return;
		m_RICON_RelayAckManager.ConfirmRelayWireInstall(this, controlSeq, fields, timebaseId);
	}

	//! True means handed to Rpc; the engine does not acknowledge receipt here.
	bool RICON_SubmitMediaWireV1(RICON_MediaRpcTransport transport, RICON_MediaEgressItem item)
	{
		if (!Replication.IsServer() || !transport || transport != m_RICON_WireAckTransport) return false;
		if (transport.GetWireVersion() != 1) return false;
		array<int> body = RICON_MediaWireV1.EncodeBody(item);
		if (!body || !transport.AuthorizeMedia(this, item)) return false;
		RICON_MediaEgressRoute route = item.m_Route;
		RICON_MediaStreamBinding source = route.m_Source;

		Rpc(RpcDo_RICON_MediaV1, source.m_iSessionEpoch, source.m_iStreamId, route.m_iRouteId, route.m_iRouteGeneration,
			route.m_iPolicyRevision, item.m_Block.m_iSequence, item.m_Block.m_iFirstSampleIndex, body);
		RICON_ObserveMediaWireRoute("MEDIA_SUBMITTED", route, item.m_Block.m_iSequence, item.m_Block.m_iFirstSampleIndex, body.Count());
		return true;
	}

	bool RICON_SubmitMediaWireRfV2(RICON_MediaRpcTransport transport, RICON_MediaEgressItem item)
	{
		if (!Replication.IsServer() || !transport || transport != m_RICON_WireAckTransport || !item || !item.m_Block) return false;
		string timebaseId = transport.GetRfTimebaseId();
		if (transport.GetWireVersion() != 2) return false;
		array<int> body = RICON_MediaWireRfV2.EncodeBody(item, item.m_Block.m_RfBudget, timebaseId);
		if (!body || !transport.AuthorizeMedia(this, item)) return false;
		RICON_MediaEgressRoute route = item.m_Route;
		RICON_MediaStreamBinding source = route.m_Source;
		Rpc(RpcDo_RICON_MediaRfV2, source.m_iSessionEpoch, source.m_iStreamId, route.m_iRouteId, route.m_iRouteGeneration,
			route.m_iPolicyRevision, item.m_Block.m_iSequence, item.m_Block.m_iFirstSampleIndex, body);
		RICON_ObserveMediaWireRoute("MEDIA_RF_SUBMITTED", route, item.m_Block.m_iSequence, item.m_Block.m_iFirstSampleIndex, body.Count());
		return true;
	}

	bool RICON_SubmitMediaWireRfV3(RICON_MediaRpcTransport transport, RICON_MediaEgressItem item)
	{
		if (!Replication.IsServer() || !transport || transport != m_RICON_WireAckTransport) return false;
		if (transport.GetWireVersion() != 3) return false;
		array<int> body = RICON_MediaWireRfV3.EncodeBody(item, transport.GetRfTimebaseId());
		if (!body || !transport.AuthorizeMedia(this, item)) return false;
		RICON_MediaEgressRoute route = item.m_Route;
		RICON_MediaStreamBinding source = route.m_Source;
		Rpc(RpcDo_RICON_MediaRfV3, source.m_iSessionEpoch, source.m_iStreamId, route.m_iRouteId, route.m_iRouteGeneration,
			route.m_iPolicyRevision, item.m_Block.m_iSequence, item.m_Block.m_iFirstSampleIndex, body);
		RICON_ObserveMediaWireRoute("MEDIA_RF_V3_SUBMITTED", route, item.m_Block.m_iSequence, item.m_Block.m_iFirstSampleIndex, body.Count());
		return true;
	}

	//! Install reads the active authority branch. Revocation may outlive the branch itself.
	bool RICON_SubmitMediaWireControlV1(RICON_MediaRpcTransport transport, RICON_MediaFanout fanout,
		RICON_MediaEgressRoute route, int controlSeq, bool install, int firstSequence, int firstSampleIndex, int nowMs)
	{
		if (!Replication.IsServer() || !transport) return false;
		if (!install && transport != m_RICON_WireAckTransport) return false;
		if (m_RICON_WireAckTransport && m_RICON_WireAckTransport != transport && m_RICON_WireAckTransport.IsCurrent()) return false;
		array<int> fields = RICON_MediaWireControlV1.EncodeRoute(route, firstSequence, firstSampleIndex);
		if (!fields || !transport.AuthorizeControl(this, fanout, route, controlSeq, install, firstSequence, firstSampleIndex, nowMs)) return false;
		if (install) m_RICON_WireAckTransport = transport;
		string rfTimebaseId = transport.GetRfTimebaseId();
		int wireVersion = transport.GetWireVersion();
		if (wireVersion == 3) Rpc(RpcDo_RICON_MediaControlRfV3, controlSeq, install, fields, route.m_Source.m_sProfileId, rfTimebaseId);
		else if (wireVersion == 2) Rpc(RpcDo_RICON_MediaControlRfV2, controlSeq, install, fields, route.m_Source.m_sProfileId, rfTimebaseId);
		else Rpc(RpcDo_RICON_MediaControlV1, controlSeq, install, fields, route.m_Source.m_sProfileId);
		string stage = "REVOKE_SUBMITTED";
		if (install) stage = "INSTALL_SUBMITTED";
		if (wireVersion == 3) stage = "RF_V3_" + stage;
		RICON_ObserveMediaWireRoute(stage, route, controlSeq, firstSampleIndex, fields.Count());
		return true;
	}

	//! Only trusted control may attach its already configured endpoint.
	bool RICON_BindMediaWireReceiver(RICON_MediaWireReceiver receiver, int epoch, int connectionId, int generation, int entityId)
	{
		if (m_RICON_WireReceiver || !receiver || !RICON_IsLocalWireOwner()) return false;
		IEntity entity = GetControlledEntity();
		BaseWorld world = GetGame().GetWorld();
		if (!entity || entity.IsDeleted() || !world || world.IsEditMode() || entity.GetWorld() != world) return false;
		int playerId = GetPlayerId();
		if (!receiver.IsEndpoint(epoch, playerId, connectionId, generation, entityId)) return false;
		// Keep a high-water mark for this controller and world so a retired grant cannot be installed again.
		if (world == m_RICON_WireWorld)
		{
			if (epoch < m_iRICON_WireEpoch || (epoch == m_iRICON_WireEpoch && entityId <= m_iRICON_WireEntityId)) return false;
		}
		if (!receiver.BindOutputController(this)) return false;

		if (m_RICON_LocalRx && !m_RICON_LocalRx.MatchesIncarnation(epoch, playerId, connectionId, generation, entityId))
			RICON_CloseLocalRx();
		m_RICON_WireReceiver = receiver;
		m_RICON_WireWorld = world;
		m_RICON_WireEntity = entity;
		m_iRICON_WireEpoch = epoch;
		m_iRICON_WirePlayer = playerId;
		m_iRICON_WireConnection = connectionId;
		m_iRICON_WireGeneration = generation;
		m_iRICON_WireEntityId = entityId;
		return true;
	}

	bool RICON_UnbindMediaWireReceiver(RICON_MediaWireReceiver receiver)
	{
		if (!receiver || receiver != m_RICON_WireReceiver) return false;
		RICON_CloseMediaWireReceiver();
		return true;
	}

	bool RICON_IsOutputReceiverCurrent(RICON_MediaWireReceiver receiver)
	{
		return receiver && receiver == m_RICON_WireReceiver && RICON_IsCurrentWireReceiver();
	}

	//! Trusted drain uses the currently attached receiver, not a cached controller incarnation.
	RICON_QueuedMediaBlock RICON_TakeMediaWire(RICON_MediaWireReceiver receiver, RICON_MediaEgressRoute route)
	{
		if (!receiver || receiver != m_RICON_WireReceiver || !RICON_IsCurrentWireReceiver()) return null;
		RICON_QueuedMediaBlock block = receiver.Take(route, System.GetTickCount());
		if (block)
		{
			string stage = "MEDIA_TAKEN";
			if (receiver.GetWireVersion() == 3) stage = "MEDIA_RF_V3_TAKEN";
			RICON_ObserveMediaWireRoute(stage, route, block.m_iSequence, block.m_iFirstSampleIndex, block.m_Bytes.Count());
		}
		return block;
	}

	//! The active local controller is the only gameplay drain into an attached playout timeline.
	int RICON_DrainMediaWireToPlayout(RICON_MediaWireReceiver receiver, RICON_MediaEgressRoute route,
		RICON_MediaPlayout playout, int maxBlocks)
	{
		if (!receiver || receiver != m_RICON_WireReceiver || !RICON_IsCurrentWireReceiver()) return 0;
		return receiver.DrainToPlayout(route, playout, System.GetTickCount(), maxBlocks);
	}

	RICON_MediaPlayoutDecision RICON_PollMediaWirePlayout(RICON_MediaWireReceiver receiver, RICON_MediaEgressRoute route,
		RICON_MediaPlayout playout)
	{
		if (!receiver || receiver != m_RICON_WireReceiver || !RICON_IsCurrentWireReceiver()) return null;
		return receiver.PollPlayout(route, playout, System.GetTickCount());
	}

	//! Локальный bootstrap открывает физический RX только для уже привязанного сетевого получателя.
	RICON_LocalRxSlot RICON_OpenLocalRx(int epoch, RICON_RelayDestination destination, int anchorMs,
		int sampleIndex, int outputCapacity)
	{
		if (!destination || !destination.IsValid() || !RICON_IsLocalWireOwner()) return null;
		bool directBound = RICON_IsCurrentWireReceiver() && m_RICON_WireReceiver.IsEndpoint(epoch, destination.m_iPlayerId,
			destination.m_iConnectionId, destination.m_iPlayerGeneration, destination.m_iEntityId);
		bool relayBound = RICON_IsCurrentRelayWireReceiver() && m_iRICON_RelayEpoch == epoch && m_RICON_RelayDestination.Same(destination);
		if (!directBound && !relayBound) return null;
		if (m_RICON_LocalRx)
		{
			RICON_IsLocalRxCurrent();
			if (m_RICON_LocalRx && epoch > m_RICON_LocalRx.Epoch()) RICON_CloseLocalRx();
		}
		if (!m_RICON_LocalRx)
		{
			RICON_LocalRxOwner owner = new RICON_LocalRxOwner();
			if (!owner.Configure(epoch, destination, 16)) return null;
			m_RICON_LocalRx = owner;
			m_RICON_LocalRxWorld = GetGame().GetWorld();
			m_RICON_LocalRxEntity = GetControlledEntity();
		}
		if (!m_RICON_LocalRx.MatchesConnection(epoch, destination)) return null;
		return m_RICON_LocalRx.Open(destination, anchorMs, sampleIndex, outputCapacity, System.GetTickCount());
	}

	bool RICON_RevokeLocalRx(RICON_LocalRxSlot slot)
	{
		return RICON_IsLocalRxCurrent() && m_RICON_LocalRx.Revoke(slot);
	}

	RICON_LocalRxDirectTrack RICON_OpenLocalRxDirect(RICON_LocalRxSlot slot, RICON_MediaEgressRoute route, int firstTargetMs, int leadMs)
	{
		if (!RICON_IsLocalRxCurrent() || !m_RICON_LocalRx.IsCurrent(slot) || !RICON_IsCurrentWireReceiver()) return null;
		return slot.OpenDirect(m_RICON_WireReceiver, route, firstTargetMs, leadMs);
	}

	string RICON_StageLocalRxDirect(RICON_LocalRxSlot slot, RICON_LocalRxDirectTrack track, float referenceDbm)
	{
		if (!RICON_IsLocalRxCurrent() || !m_RICON_LocalRx.IsCurrent(slot) || !track || !RICON_IsCurrentWireReceiver()
			|| !track.MatchesReceiver(m_RICON_WireReceiver)) return "CLOSED";
		return slot.StageDirect(track, System.GetTickCount(), referenceDbm);
	}

	string RICON_StageLocalRxRelay(RICON_LocalRxSlot slot, int sequence, int targetMs, float referenceDbm)
	{
		if (!RICON_IsLocalRxCurrent() || !m_RICON_LocalRx.IsCurrent(slot) || m_bRICON_RelayPlayoutUsed
			|| !RICON_IsCurrentRelayWireReceiver()) return "CLOSED";
		if (!slot.Matches(m_iRICON_RelayEpoch, m_RICON_RelayDestination)) return "WRONG_RX";
		return slot.StageRelay(m_RICON_RelayReceiver, sequence, targetMs, System.GetTickCount(), referenceDbm);
	}

	bool RICON_AuthorizeLocalRxOutput(RICON_LocalRxSlot slot, RICON_SharedRxMixResult window)
	{
		return RICON_IsLocalRxCurrent() && m_RICON_LocalRx.IsCurrent(slot)
			&& slot.AuthorizeOutput(window, System.GetTickCount());
	}

	protected bool RICON_IsLocalRxCurrent()
	{
		if (!m_RICON_LocalRx) return false;
		if (RICON_IsLocalWireOwner() && m_RICON_LocalRxWorld && GetGame().GetWorld() == m_RICON_LocalRxWorld
			&& !m_RICON_LocalRxWorld.IsEditMode() && m_RICON_LocalRxEntity && !m_RICON_LocalRxEntity.IsDeleted()
			&& GetControlledEntity() == m_RICON_LocalRxEntity && m_RICON_LocalRxEntity.GetWorld() == m_RICON_LocalRxWorld)
		{
			// Отзыв одной Wire-ветви не удаляет историю поколений физических слотов.
			if (RICON_IsCurrentWireReceiver() && m_RICON_LocalRx.MatchesIncarnation(m_iRICON_WireEpoch, m_iRICON_WirePlayer,
				m_iRICON_WireConnection, m_iRICON_WireGeneration, m_iRICON_WireEntityId)) return true;
			if (RICON_IsCurrentRelayWireReceiver() && m_RICON_LocalRx.MatchesConnection(m_iRICON_RelayEpoch, m_RICON_RelayDestination))
				return true;
			return false;
		}
		RICON_CloseLocalRx();
		return false;
	}

	protected void RICON_CloseLocalRx()
	{
		if (m_RICON_LocalRx) m_RICON_LocalRx.Close();
		m_RICON_LocalRx = null;
		m_RICON_LocalRxWorld = null;
		m_RICON_LocalRxEntity = null;
	}

	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		if (from != to)
		{
			RICON_CloseLocalRx();
			m_RICON_RelayAckManager = null;
			RICON_CloseRelayWireReceiver();
		}
		if (from != to) m_RICON_WireAckTransport = null;
		if (from != to && m_RICON_WireReceiver) RICON_CloseMediaWireReceiver();
		super.OnControlledEntityChanged(from, to);
	}

	override protected void OnOwnershipChanged(bool changing, bool becameOwner)
	{
		RICON_CloseLocalRx();
		m_RICON_RelayAckManager = null;
		RICON_CloseRelayWireReceiver();
		m_RICON_WireAckTransport = null;
		if (m_RICON_WireReceiver) RICON_CloseMediaWireReceiver();
		super.OnOwnershipChanged(changing, becameOwner);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Owner)]
	protected void RpcDo_RICON_MediaV1(int sessionEpoch, int streamId, int routeId, int routeGeneration,
		int policyRevision, int sequence, int firstSampleIndex, array<int> body)
	{
		RICON_ReceiveMediaWire(1, sessionEpoch, streamId, routeId, routeGeneration, policyRevision, sequence, firstSampleIndex, body);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Owner)]
	protected void RpcDo_RICON_MediaRfV2(int sessionEpoch, int streamId, int routeId, int routeGeneration,
		int policyRevision, int sequence, int firstSampleIndex, array<int> body)
	{
		RICON_ReceiveMediaWire(2, sessionEpoch, streamId, routeId, routeGeneration, policyRevision, sequence, firstSampleIndex, body);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Owner)]
	protected void RpcDo_RICON_MediaRfV3(int sessionEpoch, int streamId, int routeId, int routeGeneration,
		int policyRevision, int sequence, int firstSampleIndex, array<int> body)
	{
		RICON_ReceiveMediaWire(3, sessionEpoch, streamId, routeId, routeGeneration, policyRevision, sequence, firstSampleIndex, body);
	}

	protected void RICON_ReceiveMediaWire(int wireVersion, int sessionEpoch, int streamId, int routeId, int routeGeneration,
		int policyRevision, int sequence, int firstSampleIndex, array<int> body)
	{
		int length = -1;
		int lengthCap = RICON_MediaWireV1.MAX_PAYLOAD_BYTES + 2;
		string prefix = "MEDIA_";
		if (wireVersion == 3)
		{
			lengthCap = RICON_MediaWireRfV3.MAX_BODY_VALUES + 1;
			prefix = "MEDIA_RF_V3_";
		}
		else if (wireVersion == 2)
		{
			lengthCap = RICON_MediaWireRfV2.MAX_BODY_VALUES + 1;
			prefix = "MEDIA_RF_";
		}
		if (body) length = Math.Min(body.Count(), lengthCap);
		RICON_OnMediaWireObservation(prefix + "RAW", sessionEpoch, streamId, routeId, routeGeneration, policyRevision,
			sequence, firstSampleIndex, length);
		string stage = prefix + "REJECTED";
		if (RICON_IsCurrentWireReceiver() && sessionEpoch == m_iRICON_WireEpoch && m_RICON_WireReceiver.GetWireVersion() == wireVersion)
		{
			int nowMs = System.GetTickCount();
			string result;
			if (wireVersion == 3) result = m_RICON_WireReceiver.ReceiveRfV3(sessionEpoch, streamId, routeId, routeGeneration, policyRevision,
				sequence, firstSampleIndex, body, nowMs);
			else if (wireVersion == 2) result = m_RICON_WireReceiver.ReceiveRfV2(sessionEpoch, streamId, routeId, routeGeneration, policyRevision,
				sequence, firstSampleIndex, body, nowMs);
			else result = m_RICON_WireReceiver.Receive(sessionEpoch, streamId, routeId, routeGeneration, policyRevision,
				sequence, firstSampleIndex, body, nowMs);
			if (result == "ACCEPTED") stage = prefix + "ACCEPTED";
			else if (result == "DUPLICATE" || result == "INVALID_BODY" || result == "FULL")
				RICON_OnMediaWireObservation(prefix + result, sessionEpoch, streamId, routeId, routeGeneration,
					policyRevision, sequence, firstSampleIndex, length);
		}
		RICON_OnMediaWireObservation(stage, sessionEpoch, streamId, routeId, routeGeneration, policyRevision,
			sequence, firstSampleIndex, length);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RICON_MediaControlV1(int controlSeq, bool install, array<int> routeFields, string profileId)
	{
		RICON_ReceiveMediaWireControl(controlSeq, install, routeFields, profileId, "", 1);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RICON_MediaControlRfV2(int controlSeq, bool install, array<int> routeFields, string profileId, string rfTimebaseId)
	{
		if (rfTimebaseId.IsEmpty()) return;
		RICON_ReceiveMediaWireControl(controlSeq, install, routeFields, profileId, rfTimebaseId, 2);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_RICON_MediaControlRfV3(int controlSeq, bool install, array<int> routeFields, string profileId, string rfTimebaseId)
	{
		if (rfTimebaseId.IsEmpty()) return;
		RICON_ReceiveMediaWireControl(controlSeq, install, routeFields, profileId, rfTimebaseId, 3);
	}

	protected void RICON_ReceiveMediaWireControl(int controlSeq, bool install, array<int> routeFields, string profileId,
		string rfTimebaseId, int wireVersion)
	{
		string prefix = "";
		if (wireVersion == 3) prefix = "RF_V3_";
		string stage = "REVOKE_REJECTED";
		if (install) stage = "INSTALL_REJECTED";
		int length = RICON_WireRouteFieldLength(routeFields);
		if (controlSeq <= 0 || controlSeq >= int.MAX || !RICON_IsCurrentWireReceiver()
			|| m_RICON_WireReceiver.GetWireVersion() != wireVersion || m_RICON_WireReceiver.GetRfTimebaseId() != rfTimebaseId)
		{
			RICON_ObserveMediaWireRoute(prefix + stage, null, controlSeq, 0, length);
			return;
		}
		int firstSequence, firstSampleIndex;
		RICON_MediaEgressRoute route = RICON_MediaWireControlV1.DecodeRoute(routeFields, profileId, firstSequence, firstSampleIndex);
		if (!route || !m_RICON_WireReceiver.IsEndpoint(route.m_Source.m_iSessionEpoch, route.m_iListenerPlayerId,
			route.m_iListenerConnectionId, route.m_iListenerGeneration, route.m_iListenerEntityId))
		{
			RICON_ObserveMediaWireRoute(prefix + stage, route, controlSeq, firstSampleIndex, length);
			return;
		}
		if (install && m_RICON_WireReceiver.Install(controlSeq, route, firstSequence, firstSampleIndex))
		{
			if (wireVersion == 3) Rpc(RpcAsk_RICON_MediaInstallAckRfV3, controlSeq, routeFields, profileId, rfTimebaseId);
			else if (wireVersion == 2) Rpc(RpcAsk_RICON_MediaInstallAckRfV2, controlSeq, routeFields, profileId, rfTimebaseId);
			else Rpc(RpcAsk_RICON_MediaInstallAckV1, controlSeq, routeFields, profileId);
			RICON_ObserveMediaWireRoute(prefix + "INSTALL_ACCEPTED", route, controlSeq, firstSampleIndex, length);
			RICON_ObserveMediaWireRoute(prefix + "ACK_SUBMITTED", route, controlSeq, firstSampleIndex, length);
			return;
		}
		if (!install && firstSequence == 0 && firstSampleIndex == 0 && m_RICON_WireReceiver.Revoke(controlSeq, route))
			stage = "REVOKE_ACCEPTED";
		RICON_ObserveMediaWireRoute(prefix + stage, route, controlSeq, firstSampleIndex, length);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_MediaInstallAckV1(int controlSeq, array<int> routeFields, string profileId)
	{
		RICON_ReceiveMediaWireAck(controlSeq, routeFields, profileId, "", 1);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_MediaInstallAckRfV2(int controlSeq, array<int> routeFields, string profileId, string rfTimebaseId)
	{
		if (rfTimebaseId.IsEmpty()) return;
		RICON_ReceiveMediaWireAck(controlSeq, routeFields, profileId, rfTimebaseId, 2);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RICON_MediaInstallAckRfV3(int controlSeq, array<int> routeFields, string profileId, string rfTimebaseId)
	{
		if (rfTimebaseId.IsEmpty()) return;
		RICON_ReceiveMediaWireAck(controlSeq, routeFields, profileId, rfTimebaseId, 3);
	}

	protected void RICON_ReceiveMediaWireAck(int controlSeq, array<int> routeFields, string profileId, string rfTimebaseId, int wireVersion)
	{
		string stage = "ACK_REJECTED";
		RICON_MediaEgressRoute route;
		int firstSampleIndex = 0;
		if (Replication.IsServer() && m_RICON_WireAckTransport && controlSeq > 0 && controlSeq < int.MAX
			&& m_RICON_WireAckTransport.ConfirmInstall(this, controlSeq, routeFields, profileId, route, firstSampleIndex, rfTimebaseId, wireVersion))
			stage = "ACK_CONFIRMED";
		if (wireVersion == 3) stage = "RF_V3_" + stage;
		RICON_ObserveMediaWireRoute(stage, route, controlSeq, firstSampleIndex, RICON_WireRouteFieldLength(routeFields));
	}

	protected int RICON_WireRouteFieldLength(array<int> routeFields)
	{
		if (!routeFields) return -1;
		return Math.Min(routeFields.Count(), RICON_MediaWireControlV1.ROUTE_FIELD_COUNT + 1);
	}

	protected void RICON_ObserveMediaWireRoute(string stage, RICON_MediaEgressRoute route, int sequence, int firstSampleIndex, int length)
	{
		if (!route || !route.m_Source)
		{
			RICON_OnMediaWireObservation(stage, 0, 0, 0, 0, 0, sequence, firstSampleIndex, length);
			return;
		}
		RICON_OnMediaWireObservation(stage, route.m_Source.m_iSessionEpoch, route.m_Source.m_iStreamId, route.m_iRouteId,
			route.m_iRouteGeneration, route.m_iPolicyRevision, sequence, firstSampleIndex, length);
	}

	//! Test addons may observe only bounded scalar metadata. Keep overrides read-only.
	protected void RICON_OnMediaWireObservation(string stage, int epoch, int streamId, int routeId,
		int routeGeneration, int policyRevision, int sequence, int firstSampleIndex, int length)
	{
	}

	protected bool RICON_IsCurrentWireReceiver()
	{
		if (!m_RICON_WireReceiver) return false;
		if (!RICON_IsLocalWireOwner() || !m_RICON_WireWorld || GetGame().GetWorld() != m_RICON_WireWorld
			|| m_RICON_WireWorld.IsEditMode() || !m_RICON_WireEntity || m_RICON_WireEntity.IsDeleted()
			|| m_RICON_WireEntity.GetWorld() != m_RICON_WireWorld
			|| GetControlledEntity() != m_RICON_WireEntity || GetPlayerId() != m_iRICON_WirePlayer)
		{
			RICON_CloseMediaWireReceiver();
			return false;
		}
		return m_RICON_WireReceiver.IsEndpoint(m_iRICON_WireEpoch, m_iRICON_WirePlayer, m_iRICON_WireConnection,
			m_iRICON_WireGeneration, m_iRICON_WireEntityId);
	}

	protected bool RICON_IsLocalWireOwner()
	{
		if (Replication.IsServer() || !Replication.IsClient() || RplSession.Mode() != RplMode.Client
			|| !GetGame() || GetGame().GetPlayerController() != this) return false;
		RplId item = Replication.FindItemId(this);
		if (!item.IsValid()) return false;
		RplNode node = Replication.FindNode(item);
		return node && node.IsOwner();
	}

	protected void RICON_CloseMediaWireReceiver()
	{
		m_RICON_WireReceiver.Close();
		m_RICON_WireReceiver = null;
		m_RICON_WireEntity = null;
	}
}
