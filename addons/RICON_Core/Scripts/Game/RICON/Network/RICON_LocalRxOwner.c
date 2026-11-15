//! Одна последовательность прямого излучения. FIR живёт ровно столько, сколько её допуск.
class RICON_LocalRxDirectTrack
{
	protected RICON_MediaWireReceiver m_Receiver;
	protected ref RICON_MediaEgressRoute m_Route;
	protected ref RICON_MediaOutputPermit m_Permit;
	protected ref RICON_MediaPlayout m_Playout;
	protected ref RICON_PcmResampler8To48 m_Resampler = new RICON_PcmResampler8To48();
	protected int m_iNextSample = -1;
	protected int m_iPreviousExpiryMs = -1;
	protected int m_iPreviousMapping;
	protected float m_fPreviousPower;

	bool Configure(RICON_MediaWireReceiver receiver, RICON_MediaEgressRoute route, int targetMs, int leadMs)
	{
		if (m_Receiver || !receiver || receiver.GetWireVersion() != 3 || !route || !route.IsValid()
			|| route.m_Source.m_sProfileId != RICON_MediaPcmProfile.PROFILE_ID) return false;
		int firstSequence, firstSample;
		RICON_MediaOutputPermit permit = receiver.ReadUnclaimedPlayout(route, firstSequence, firstSample);
		RICON_MediaPlayout playout = new RICON_MediaPlayout();
		if (!permit || !playout.Configure(route, firstSequence, firstSample, RICON_MediaPcmProfile.FRAME_SAMPLES,
			RICON_MediaPcmProfile.FRAME_DURATION_MS, targetMs, 8, 8, leadMs)
			|| !receiver.ClaimPlayout(route, playout, permit)) return false;
		m_Receiver = receiver;
		m_Route = route.CloneRoute();
		m_Permit = permit;
		m_Playout = playout;
		return true;
	}

	bool MatchesReceiver(RICON_MediaWireReceiver receiver) { return receiver && m_Receiver == receiver; }
	bool IsCurrent() { return m_Permit && m_Permit.MatchesRoute(m_Route); }

	string Stage(RICON_LocalRxSlot slot, int nowMs, float referenceDbm)
	{
		if (!m_Receiver || !IsCurrent()) return "REVOKED";
		if (!slot || !slot.Matches(m_Route.m_Source.m_iSessionEpoch, RICON_RelayDestination.FromRoute(m_Route))) return "WRONG_RX";
		if (nowMs < 0 || !RICON_Num.IsFinite(referenceDbm)) return "INVALID";
		m_Receiver.DrainToPlayout(m_Route, m_Playout, nowMs, 8);
		RICON_MediaPlayoutDecision decision = m_Receiver.PollPlayout(m_Route, m_Playout, nowMs);
		if (!decision)
		{
			if (!IsCurrent()) return "REVOKED";
			return "WAIT";
		}
		if (decision.m_sKind != "PLAY")
		{
			m_iNextSample = -1;
			return decision.m_sKind;
		}
		if (decision.m_OutputPermit != m_Permit) return "REVOKED";

		int firstSample = decision.m_iFirstSampleIndex;
		int priorNext = m_iNextSample;
		m_iNextSample = -1;
		array<int> decoded = RICON_MediaPcmProfile.Decode(m_Route.m_Source, decision);
		if (!decoded || !decision.m_Block.m_RfHistory || !decision.m_Block.m_CaptureEnvelope) return "INVALID";
		float power, noise;
		if (!RICON_SharedRxMixer.TryNormalizeRfBudget(decision.m_Block.m_RfHistory.m_Budget, referenceDbm, power, noise))
			return "INVALID_RF";
		int endMs = decision.m_iTargetMs + RICON_SharedRxPresentationTimeline.WINDOW_MS;
		int mappingRevision = decision.m_Block.m_CaptureEnvelope.m_iMappingRevision;
		// shortcut: сохраняем хвост только при неизменном усилении и полном сроке; переменный RF требует обработки до FIR.
		bool continuous = priorNext == firstSample && m_iPreviousExpiryMs >= endMs
			&& m_fPreviousPower == power && m_iPreviousMapping == mappingRevision;
		if (!continuous && !m_Resampler.Reset(firstSample)) return "INVALID";
		RICON_SharedRxContribution input = new RICON_SharedRxContribution();
		input.m_eKind = RICON_ESharedRxContributionKind.PCM;
		input.m_Route = m_Route;
		input.m_Permit = m_Permit;
		input.m_fPowerRatio = power;
		int firstOutput;
		input.m_aSamples = m_Resampler.Process(firstSample, decoded, firstOutput);
		if (!input.m_aSamples) return "INVALID";
		input.m_iFirstSampleIndex = firstOutput;
		int expiryMs = decision.m_iWireExpiryMs;
		if (continuous && m_iPreviousExpiryMs < expiryMs) expiryMs = m_iPreviousExpiryMs;
		string status = slot.Offer(decision.m_iTargetMs, nowMs, expiryMs, input);
		if (status == "ACCEPTED")
		{
			m_iNextSample = firstSample + RICON_MediaPcmProfile.FRAME_SAMPLES;
			m_iPreviousExpiryMs = decision.m_iWireExpiryMs;
			m_iPreviousMapping = mappingRevision;
			m_fPreviousPower = power;
		}
		return status;
	}

	void Close()
	{
		// Закрытый playout остаётся закреплённым за старым grant: его нельзя вновь вычитать сырым путём.
		if (m_Playout) m_Playout.Close();
		m_Receiver = null;
		m_Route = null;
		m_Permit = null;
		m_Playout = null;
	}
}

//! Локальное состояние физического RX. Сетевые разрешения остаются у получателей транспорта.
class RICON_LocalRxSlot
{
	protected ref RICON_RelayDestination m_Address;
	protected ref RICON_SharedRxPresentationTimeline m_Timeline;
	protected ref RICON_RelayOutputQueue m_Output;
	protected ref RICON_SharedRxMixResult m_Issued;
	protected ref array<ref RICON_MediaOutputPermit> m_IssuedPermits = {};
	protected ref array<ref RICON_LocalRxDirectTrack> m_Direct = {};
	protected int m_iEpoch, m_iIssuedAtMs = -1;
	protected bool m_bClosed;

	bool Configure(int epoch, RICON_RelayDestination destination, int anchorMs, int sampleIndex, int outputCapacity, int nowMs)
	{
		if (m_bClosed || m_Address || !destination || anchorMs < nowMs || nowMs < 0
			|| sampleIndex < 0 || sampleIndex % RICON_SharedRxMixer.WINDOW_SAMPLES != 0) return false;
		RICON_SharedRxPresentationTimeline timeline = new RICON_SharedRxPresentationTimeline();
		RICON_RelayOutputQueue output = new RICON_RelayOutputQueue();
		if (!timeline.ConfigureReceiver(epoch, destination, anchorMs, sampleIndex)
			|| !output.Configure(epoch, destination, outputCapacity, nowMs)) return false;
		m_Address = destination.CloneDestination();
		m_Timeline = timeline;
		m_Output = output;
		m_iEpoch = epoch;
		return true;
	}

	bool IsOpen() { return !m_bClosed && m_Timeline && m_Output; }
	string SlotKey()
	{
		if (!m_Address) return "";
		return string.Format("%1:%2", m_Address.m_iRxDeviceId, m_Address.m_iRxIndex);
	}
	int Generation()
	{
		if (!m_Address) return 0;
		return m_Address.m_iRxGeneration;
	}
	bool Matches(int epoch, RICON_RelayDestination destination)
	{
		return IsOpen() && m_iEpoch == epoch && m_Address.Same(destination);
	}

	RICON_LocalRxDirectTrack OpenDirect(RICON_MediaWireReceiver receiver, RICON_MediaEgressRoute route, int firstTargetMs, int leadMs)
	{
		if (!IsOpen() || !route || !route.IsValid() || !Matches(route.m_Source.m_iSessionEpoch, RICON_RelayDestination.FromRoute(route)))
			return null;
		if (!m_Timeline.IsNextTarget(firstTargetMs)) return null;
		for (int index = m_Direct.Count() - 1; index >= 0; index--)
		{
			ref RICON_LocalRxDirectTrack prior = m_Direct[index];
			bool current = prior.IsCurrent();
			if (!IsOpen()) return null;
			if (current) continue;
			prior.Close();
			m_Direct.RemoveOrdered(index);
		}
		if (m_Direct.Count() >= RICON_SharedRxMixer.MAX_INPUTS) return null;
		RICON_LocalRxDirectTrack track = new RICON_LocalRxDirectTrack();
		if (!track.Configure(receiver, route, firstTargetMs, leadMs)) return null;
		if (!IsOpen()) { track.Close(); return null; }
		m_Direct.Insert(track);
		return track;
	}

	string StageDirect(RICON_LocalRxDirectTrack track, int nowMs, float referenceDbm)
	{
		if (!IsOpen()) return "CLOSED";
		if (!track || !m_Direct.Contains(track)) return "WRONG_RX";
		return track.Stage(this, nowMs, referenceDbm);
	}

	string Offer(int targetMs, int nowMs, int wireExpiryMs, RICON_SharedRxContribution input)
	{
		if (!IsOpen()) return "CLOSED";
		if (!input || !input.MatchesReceiver(m_iEpoch, m_Address)) return "WRONG_RX";
		// Очередь вывода требует разрешение на всё окно; короткий direct-вклад не портит уже смешанный PCM.
		if (targetMs >= 0 && (wireExpiryMs <= targetMs || wireExpiryMs - targetMs < RICON_SharedRxPresentationTimeline.WINDOW_MS))
			return "EXPIRED";
		ref RICON_SharedRxPresentationTimeline timeline = m_Timeline;
		string status = timeline.Offer(targetMs, nowMs, wireExpiryMs, input);
		if (!IsOpen()) return "CLOSED";
		return status;
	}

	string OfferRelay(RICON_RelayPresentationWindow window, float referenceDbm, int nowMs)
	{
		if (!IsOpen()) return "CLOSED";
		ref RICON_SharedRxPresentationTimeline timeline = m_Timeline;
		string status = timeline.OfferRelay(window, referenceDbm, nowMs);
		if (!IsOpen()) return "CLOSED";
		return status;
	}

	bool OpenRelay(RICON_RelayWireReceiver receiver)
	{
		if (!IsOpen() || !receiver || !receiver.MatchesBoundDestination(m_Address)) return false;
		return receiver.ClaimRawCursor();
	}

	string StageRelay(RICON_RelayWireReceiver receiver, int sequence, int targetMs, int nowMs, float referenceDbm)
	{
		if (!IsOpen() || !receiver) return "CLOSED";
		ref RICON_SharedRxPresentationTimeline timeline = m_Timeline;
		string status = receiver.StagePresentation(timeline, sequence, targetMs, nowMs, referenceDbm);
		if (!IsOpen()) return "CLOSED";
		return status;
	}

	int SkipLate(int nowMs)
	{
		if (!IsOpen()) return 0;
		return m_Timeline.SkipLate(nowMs);
	}

	//! Commit закрывает цель. Если очередь её не принимает, повторно эта цель не выдаётся.
	string Prepare(int targetMs, int nowMs)
	{
		if (!IsOpen()) return "CLOSED";
		ref RICON_SharedRxPresentationTimeline timeline = m_Timeline;
		int expiryMs;
		RICON_SharedRxMixResult result = timeline.Commit(targetMs, nowMs, 0, null, expiryMs);
		if (!IsOpen()) return "CLOSED";
		if (!result) return "INVALID_CURSOR";
		ref RICON_RelayOutputQueue output = m_Output;
		string status = output.Offer(result, nowMs);
		if (!IsOpen()) return "CLOSED";
		return status;
	}

	RICON_SharedRxMixResult Take(int nowMs)
	{
		if (!IsOpen()) return null;
		ref RICON_RelayOutputQueue output = m_Output;
		ref RICON_SharedRxMixResult result = output.Take(nowMs);
		if (!IsOpen()) return null;
		if (result)
		{
			m_Issued = result;
			m_iIssuedAtMs = nowMs;
			m_IssuedPermits.Clear();
			foreach (RICON_MediaOutputPermit permit : result.m_aPermits) m_IssuedPermits.Insert(permit);
		}
		return result;
	}

	//! Последняя проверка перед синхронным выводом; результат Take не является разрешением.
	bool AuthorizeOutput(RICON_SharedRxMixResult window, int nowMs)
	{
		if (!IsOpen() || !window || window != m_Issued || m_iIssuedAtMs != nowMs || window.m_iTargetMs != nowMs
			|| !Matches(window.m_iEpoch, window.m_Destination) || !window.m_aPermits) return false;
		m_Issued = null;
		ref array<ref RICON_MediaOutputPermit> issued = m_IssuedPermits;
		m_IssuedPermits = {};
		if (issued.Count() != window.m_aPermits.Count()) return false;
		for (int index = 0; index < issued.Count(); index++)
		{
			RICON_MediaOutputPermit permit = issued[index];
			if (!permit || !permit.IsCurrent() || !IsOpen()) return false;
		}
		// Чтение permit может вызвать закрытие или каскад отзыва. Сверяем исходный набор заново.
		if (!window.m_aPermits || issued.Count() != window.m_aPermits.Count()) return false;
		for (int check = 0; check < issued.Count(); check++)
			if (window.m_aPermits[check] != issued[check]) return false;
		return IsOpen();
	}

	void Close()
	{
		m_bClosed = true;
		foreach (RICON_LocalRxDirectTrack track : m_Direct) track.Close();
		m_Direct.Clear();
		m_Issued = null;
		m_IssuedPermits.Clear();
		if (m_Timeline) m_Timeline.Close();
		if (m_Output) m_Output.Close();
		m_Timeline = null;
		m_Output = null;
	}
}

//! Один реестр на поколение локального подключения; закрытые слоты учитываются в лимите.
class RICON_LocalRxOwner
{
	protected ref RICON_RelayDestination m_Incarnation;
	protected ref map<string, ref RICON_LocalRxSlot> m_Slots = new map<string, ref RICON_LocalRxSlot>();
	protected int m_iEpoch, m_iSlotLimit;
	protected bool m_bClosed;

	bool Configure(int epoch, RICON_RelayDestination incarnation, int slotLimit)
	{
		if (m_bClosed || m_Incarnation || epoch <= 0 || epoch >= int.MAX
			|| !incarnation || !incarnation.IsValid() || slotLimit < 1 || slotLimit > 16) return false;
		m_Incarnation = incarnation.CloneDestination();
		m_iEpoch = epoch;
		m_iSlotLimit = slotLimit;
		return true;
	}

	RICON_LocalRxSlot Open(RICON_RelayDestination destination, int anchorMs, int sampleIndex, int outputCapacity, int nowMs)
	{
		if (!MatchesConnection(m_iEpoch, destination)) return null;
		string key = string.Format("%1:%2", destination.m_iRxDeviceId, destination.m_iRxIndex);
		RICON_LocalRxSlot previous = m_Slots.Get(key);
		if (previous)
		{
			if (previous.IsOpen() || destination.m_iRxGeneration <= previous.Generation()) return null;
		}
		else if (m_Slots.Count() >= m_iSlotLimit) return null;
		RICON_LocalRxSlot slot = new RICON_LocalRxSlot();
		if (!slot.Configure(m_iEpoch, destination, anchorMs, sampleIndex, outputCapacity, nowMs)) return null;
		m_Slots.Set(key, slot);
		return slot;
	}

	bool MatchesConnection(int epoch, RICON_RelayDestination destination)
	{
		return destination && destination.IsValid() && MatchesIncarnation(epoch, destination.m_iPlayerId,
			destination.m_iConnectionId, destination.m_iPlayerGeneration, destination.m_iEntityId);
	}

	bool MatchesIncarnation(int epoch, int player, int connection, int generation, int entity)
	{
		return !m_bClosed && m_Incarnation && epoch == m_iEpoch && player == m_Incarnation.m_iPlayerId
			&& connection == m_Incarnation.m_iConnectionId && generation == m_Incarnation.m_iPlayerGeneration
			&& entity == m_Incarnation.m_iEntityId;
	}

	int Epoch() { return m_iEpoch; }

	bool IsCurrent(RICON_LocalRxSlot slot)
	{
		return !m_bClosed && slot && slot.IsOpen() && m_Slots.Get(slot.SlotKey()) == slot;
	}

	bool Revoke(RICON_LocalRxSlot slot)
	{
		if (!IsCurrent(slot)) return false;
		slot.Close();
		return true;
	}

	void Close()
	{
		m_bClosed = true;
		foreach (string key, RICON_LocalRxSlot slot : m_Slots) slot.Close();
		m_Slots.Clear();
		m_Incarnation = null;
	}
}

