//! Локальный шов общей сцены M1. Не сеть, не звук и не штатный DIRECT.
//! B заранее связывает два приёмника; покрытие A→B закрывается на опубликованном тике до корпуса.
class RICON_M1LocalScene
{
	protected static const int ACK_TIMEOUT_MS = 500;
	protected static const int RELAY_WIDTH = 4;

	protected ref RICON_LocalRxOwner m_Owner;
	protected ref RICON_LocalRxSlot m_DirectSlot;
	protected ref RICON_LocalRxSlot m_RelaySlot;
	protected ref RICON_MediaWireReceiver m_Direct;
	protected ref RICON_RelayWireReceiver m_Relay;
	protected ref RICON_MediaWirePublication m_Publication;
	protected ref RICON_MediaRfHistory m_History;
	protected ref RICON_MediaEgressRoute m_DirectRoute;
	protected ref RICON_RelayDestination m_DirectAddress;
	protected ref RICON_RelayDestination m_RelayAddress;
	protected ref RICON_TemporalRfBudget m_DirectBudget;
	protected ref RICON_TemporalRfBudget m_StationInput;
	protected ref RICON_TemporalRfBudget m_StationOutput;
	protected ref array<int> m_Body;
	protected ref array<int> m_RelayFields;
	protected string m_sTimebaseId;
	protected string m_sRelayTimebaseId;
	protected int m_iEpoch;
	protected int m_iBindNowMs = -1;
	protected int m_iPublishedTickMs = -1;
	protected int m_iCoverageFromMs = -1;
	protected int m_iDirectControlSeq;
	protected int m_iBodySequence = -1;
	protected int m_iBodySample = -1;
	protected int m_iStationInputDevice;
	protected int m_iStationOutputDevice;
	protected bool m_bBound;
	protected bool m_bCoverageClosed;
	protected bool m_bBodyAdmitted;
	protected bool m_bBodyBeforeInstall;
	protected bool m_bDirectInstall;
	protected bool m_bDirectConfirmed;
	protected bool m_bStationConfirmed;
	protected bool m_bStationRevoked;
	protected bool m_bClosed;

	//! Разные физические адреса, оба ещё без INSTALL. Чужое воплощение и тот же слот отклоняются.
	bool Bind(int epoch, RICON_RelayDestination incarnation, RICON_RelayDestination directAddress,
		RICON_RelayDestination relayAddress, string timebaseId, int anchorMs, int nowMs)
	{
		if (m_bClosed || m_bBound || epoch <= 0 || epoch >= int.MAX || nowMs < 0 || anchorMs < nowMs) return false;
		if (!RICON_TemporalSnapshots.ValidKey(timebaseId) || !SameIdentity(incarnation, directAddress)
			|| !SameIdentity(incarnation, relayAddress) || SamePhysical(directAddress, relayAddress)) return false;

		RICON_LocalRxOwner owner = new RICON_LocalRxOwner();
		if (!owner.Configure(epoch, incarnation, 2)) return false;
		RICON_LocalRxSlot directSlot = owner.Open(directAddress, anchorMs, 0, 2, nowMs);
		RICON_LocalRxSlot relaySlot = owner.Open(relayAddress, anchorMs, RICON_SharedRxTestMixer.WINDOW_SAMPLES, 2, nowMs);
		if (!directSlot || !relaySlot)
		{
			owner.Close();
			return false;
		}

		RICON_MediaWireReceiver direct = new RICON_MediaWireReceiver();
		if (!direct.Configure(epoch, directAddress.m_iPlayerId, directAddress.m_iConnectionId,
			directAddress.m_iPlayerGeneration, directAddress.m_iEntityId, 1, RELAY_WIDTH, 2, 30, 16, timebaseId, true))
		{
			owner.Close();
			return false;
		}
		RICON_RelayWireReceiver relay = new RICON_RelayWireReceiver();
		if (!relay.ExpectWireVersion(2))
		{
			owner.Close();
			direct.Close();
			return false;
		}
		RICON_MediaWirePublication publication = new RICON_MediaWirePublication();
		if (!publication.Configure(epoch, 1, ACK_TIMEOUT_MS, 8))
		{
			owner.Close();
			direct.Close();
			relay.Close();
			return false;
		}

		m_Owner = owner;
		m_DirectSlot = directSlot;
		m_RelaySlot = relaySlot;
		m_Direct = direct;
		m_Relay = relay;
		m_Publication = publication;
		m_DirectAddress = directAddress.CloneDestination();
		m_RelayAddress = relayAddress.CloneDestination();
		m_sTimebaseId = timebaseId;
		m_iEpoch = epoch;
		m_iBindNowMs = nowMs;
		m_bBound = true;
		return true;
	}

	//! Старое поколение того же слота и чужое воплощение не открывают замену.
	bool ReplacementRejected(RICON_RelayDestination address, int anchorMs, int nowMs)
	{
		return m_bBound && !m_bClosed && m_Owner && !m_Owner.Open(address, anchorMs, 0, 2, nowMs);
	}

	bool PublishTick(int tickMs)
	{
		if (!m_bBound || m_bClosed || m_iPublishedTickMs >= 0 || m_bCoverageClosed || m_bBodyAdmitted) return false;
		if (tickMs <= m_iBindNowMs || tickMs >= int.MAX) return false;
		m_iPublishedTickMs = tickMs;
		return true;
	}

	//! Покрытие A→B закрывается только на точном опубликованном тике и только до первого корпуса.
	bool CloseDirectCoverage(RICON_MediaEgressRoute route, int claimedTickMs, int coverageFromMs, RICON_TemporalRfBudget budget)
	{
		if (!m_bBound || m_bClosed || m_bCoverageClosed || m_bBodyAdmitted || m_bDirectInstall) return false;
		if (m_iPublishedTickMs < 0 || claimedTickMs != m_iPublishedTickMs || !MatchesDirect(route) || !budget) return false;
		if (budget.m_sTimebaseId != m_sTimebaseId || budget.m_iLinkId <= 0) return false;
		if (coverageFromMs < 0 || coverageFromMs >= claimedTickMs) return false;
		if (budget.m_iTickMs < coverageFromMs || budget.m_iTickMs >= claimedTickMs) return false;

		RICON_MediaRfHistory history = new RICON_MediaRfHistory();
		if (!history.Configure(route, m_sTimebaseId, 4, 60000)) return false;
		if (!history.RecordCompleted(route, coverageFromMs, claimedTickMs, budget, claimedTickMs))
		{
			history.Close();
			return false;
		}
		m_History = history;
		m_DirectRoute = route.CloneRoute();
		m_DirectBudget = budget.CloneBudget();
		m_iCoverageFromMs = coverageFromMs;
		m_bCoverageClosed = true;
		return true;
	}

	bool SelectsDirectCoverage(RICON_MediaEgressRoute route, int lowerMs, int upperMs)
	{
		if (!m_bCoverageClosed || !m_History || !m_DirectRoute || !m_DirectRoute.Same(route)) return false;
		RICON_MediaCaptureEnvelope envelope = new RICON_MediaCaptureEnvelope();
		envelope.m_Binding = route.m_Source;
		envelope.m_sTimebaseId = m_sTimebaseId;
		envelope.m_iMappingRevision = 1;
		envelope.m_iFirstSampleIndex = 0;
		envelope.m_iSampleCount = RICON_MediaPcmTestProfile.FRAME_SAMPLES;
		envelope.m_iLowerMs = lowerMs;
		envelope.m_iUpperMs = upperMs;
		RICON_MediaRfHistoryEntry selected = m_History.Select(route, envelope, m_iPublishedTickMs);
		return selected && selected.m_Budget && selected.m_Budget.Same(m_DirectBudget);
	}

	//! Корпус A допускается без direct INSTALL и без его ACK: INSTALL ещё не существует.
	bool AdmitBody(array<int> pcm, int sequence, int firstSample)
	{
		if (!m_bCoverageClosed || m_bClosed || m_bBodyAdmitted || m_bDirectInstall || m_bDirectConfirmed) return false;
		if (!RICON_MediaWireV1.ValidRange(sequence, firstSample, RICON_MediaPcmTestProfile.FRAME_SAMPLES)) return false;
		if (!pcm || pcm.Count() != RICON_MediaPcmTestProfile.PAYLOAD_BYTES) return false;
		foreach (int octet : pcm)
		{
			if (octet < 0 || octet > 255) return false;
		}
		m_Body = CopyInts(pcm);
		m_iBodySequence = sequence;
		m_iBodySample = firstSample;
		m_bBodyAdmitted = true;
		m_bBodyBeforeInstall = !m_bDirectInstall;
		return true;
	}

	//! Первый direct INSTALL появляется только у готового корпуса и только на опубликованном тике.
	int BeginDirectInstall(RICON_MediaEgressRoute route, int claimedTickMs)
	{
		if (!ReadyForDirectInstall(route, claimedTickMs)) return 0;
		int seq = m_Publication.Begin(route, m_iBodySequence, m_iBodySample, claimedTickMs);
		if (seq <= 0) return 0;
		if (!m_Direct.Install(seq, route, m_iBodySequence, m_iBodySample))
		{
			m_Publication.Abandon(route, seq, claimedTickMs);
			return 0;
		}
		m_iDirectControlSeq = seq;
		m_bDirectInstall = true;
		return seq;
	}

	bool ConfirmDirect(RICON_MediaEgressRoute route, int controlSeq, int nowMs)
	{
		if (!m_bDirectInstall || m_bDirectConfirmed || m_bClosed || controlSeq != m_iDirectControlSeq) return false;
		if (!m_DirectRoute || !m_DirectRoute.Same(route) || nowMs < m_iPublishedTickMs) return false;
		if (!m_Publication.Confirm(route, controlSeq, m_iBodySequence, m_iBodySample, nowMs)) return false;
		m_bDirectConfirmed = true;
		return true;
	}

	//! Подтверждение станции не подтверждает прямой маршрут и не требуется для корпуса.
	bool ConfirmRelay(array<int> fields, string timebaseId, int width)
	{
		if (!m_bBound || m_bClosed || m_bStationConfirmed || m_bStationRevoked || width != RELAY_WIDTH) return false;
		if (timebaseId != m_sTimebaseId || !fields) return false;
		RICON_RelayWireGrant grant = RICON_RelayWireControl.Decode(fields, timebaseId);
		if (!grant || grant.m_iWireVersion != 2 || grant.m_iEpoch != m_iEpoch || !grant.m_Destination.Same(m_RelayAddress))
			return false;
		if (!m_Relay.ApplyControl(RICON_RelayWireControl.INSTALL_SEQ, true, fields, timebaseId, m_RelayAddress, m_iEpoch, width))
			return false;
		m_RelayFields = CopyInts(fields);
		m_sRelayTimebaseId = timebaseId;
		m_bStationConfirmed = true;
		return true;
	}

	//! A→R и R→B остаются разными моделями и не подменяют покрытие прямого A→B.
	bool KeepStationModels(RICON_TemporalRfBudget inputAr, int inputDevice, RICON_TemporalRfBudget outputRb, int outputDevice)
	{
		if (!m_bCoverageClosed || m_bClosed || m_StationInput || !inputAr || !outputRb) return false;
		if (inputAr.Same(outputRb) || inputAr.m_iLinkId == outputRb.m_iLinkId) return false;
		if (inputAr.m_iLinkId == m_DirectBudget.m_iLinkId || outputRb.m_iLinkId == m_DirectBudget.m_iLinkId) return false;
		if (inputAr.m_sTimebaseId != m_sTimebaseId || outputRb.m_sTimebaseId != m_sTimebaseId) return false;
		if (inputDevice <= 0 || inputDevice == m_DirectAddress.m_iRxDeviceId || outputDevice != m_RelayAddress.m_iRxDeviceId)
			return false;
		m_StationInput = inputAr.CloneBudget();
		m_StationOutput = outputRb.CloneBudget();
		m_iStationInputDevice = inputDevice;
		m_iStationOutputDevice = outputDevice;
		return true;
	}

	bool RevokeStation(array<int> fields, string timebaseId, int width)
	{
		if (!m_bStationConfirmed || m_bStationRevoked || m_bClosed || width != RELAY_WIDTH) return false;
		if (timebaseId != m_sRelayTimebaseId || !SameInts(fields, m_RelayFields)) return false;
		if (!m_Relay.ApplyControl(RICON_RelayWireControl.REVOKE_SEQ, false, fields, timebaseId, m_RelayAddress, m_iEpoch, width))
			return false;
		m_bStationConfirmed = false;
		m_bStationRevoked = true;
		return true;
	}

	bool Bound() { return m_bBound && !m_bClosed; }
	bool CoverageClosed() { return m_bCoverageClosed && !m_bClosed; }
	bool BodyAdmitted() { return m_bBodyAdmitted && !m_bClosed; }
	bool BodyBeforeInstall() { return m_bBodyBeforeInstall; }
	bool DirectInstallBegun() { return m_bDirectInstall && !m_bClosed; }
	bool DirectConfirmed() { return m_bDirectConfirmed && !m_bClosed; }
	bool StationConfirmed() { return m_bStationConfirmed && !m_bStationRevoked && !m_bClosed; }
	bool StationRevoked() { return m_bStationRevoked; }
	int PublishedTick() { return m_iPublishedTickMs; }
	int DirectCoverageLink()
	{
		if (!m_DirectBudget) return 0;
		return m_DirectBudget.m_iLinkId;
	}
	bool DirectSlotCurrent() { return m_Owner && m_Owner.IsCurrent(m_DirectSlot); }
	bool RelaySlotCurrent() { return m_Owner && m_Owner.IsCurrent(m_RelaySlot); }
	bool DirectLive()
	{
		return DirectConfirmed() && DirectInstallBegun() && BodyAdmitted() && DirectSlotCurrent();
	}
	bool StationModelsDistinct()
	{
		return m_StationInput && m_StationOutput && !m_StationInput.Same(m_StationOutput)
			&& m_DirectBudget && m_StationInput.m_iLinkId != m_DirectBudget.m_iLinkId
			&& m_StationOutput.m_iLinkId != m_DirectBudget.m_iLinkId
			&& m_iStationInputDevice != m_DirectAddress.m_iRxDeviceId
			&& m_iStationOutputDevice == m_RelayAddress.m_iRxDeviceId;
	}

	array<int> ReadBody()
	{
		if (!m_Body) return null;
		return CopyInts(m_Body);
	}

	protected bool ReadyForDirectInstall(RICON_MediaEgressRoute route, int claimedTickMs)
	{
		return m_bBodyAdmitted && m_bBodyBeforeInstall && !m_bDirectInstall && !m_bClosed
			&& m_bCoverageClosed && claimedTickMs == m_iPublishedTickMs
			&& m_DirectRoute && m_DirectRoute.Same(route) && m_Publication && m_Direct;
	}

	protected bool MatchesDirect(RICON_MediaEgressRoute route)
	{
		return route && route.IsValid() && m_Direct && m_Direct.IsEndpoint(m_iEpoch, route.m_iListenerPlayerId,
			route.m_iListenerConnectionId, route.m_iListenerGeneration, route.m_iListenerEntityId)
			&& route.m_iRxDeviceId == m_DirectAddress.m_iRxDeviceId
			&& route.m_iRxTransceiverIndex == m_DirectAddress.m_iRxIndex
			&& route.m_iRxGeneration == m_DirectAddress.m_iRxGeneration
			&& route.m_Source && route.m_Source.m_iSessionEpoch == m_iEpoch;
	}

	protected static bool SameIdentity(RICON_RelayDestination incarnation, RICON_RelayDestination address)
	{
		return incarnation && address && incarnation.IsValid() && address.IsValid()
			&& incarnation.m_iPlayerId == address.m_iPlayerId
			&& incarnation.m_iConnectionId == address.m_iConnectionId
			&& incarnation.m_iPlayerGeneration == address.m_iPlayerGeneration
			&& incarnation.m_iEntityId == address.m_iEntityId;
	}

	protected static bool SamePhysical(RICON_RelayDestination left, RICON_RelayDestination right)
	{
		return left && right && left.m_iRxDeviceId == right.m_iRxDeviceId && left.m_iRxIndex == right.m_iRxIndex;
	}

	protected static array<int> CopyInts(array<int> source)
	{
		if (!source) return null;
		array<int> copy = {};
		copy.Reserve(source.Count());
		foreach (int value : source) copy.Insert(value);
		return copy;
	}

	protected static bool SameInts(array<int> left, array<int> right)
	{
		if (!left || !right || left.Count() != right.Count()) return false;
		for (int index = 0; index < left.Count(); index++)
		{
			if (left[index] != right[index]) return false;
		}
		return true;
	}

	void Close()
	{
		m_bClosed = true;
		if (m_History) m_History.Close();
		if (m_Publication) m_Publication.Close();
		if (m_Direct) m_Direct.Close();
		if (m_Relay) m_Relay.Close();
		if (m_Owner) m_Owner.Close();
		m_History = null;
		m_Publication = null;
		m_Direct = null;
		m_Relay = null;
		m_Owner = null;
		m_DirectSlot = null;
		m_RelaySlot = null;
		m_Body = null;
	}
}
