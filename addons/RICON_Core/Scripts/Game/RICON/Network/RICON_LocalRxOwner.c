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
			|| route.m_Source.m_sProfileId != RICON_MediaPcmTestProfile.PROFILE_ID) return false;
		int firstSequence, firstSample;
		RICON_MediaOutputPermit permit = receiver.ReadUnclaimedPlayout(route, firstSequence, firstSample);
		RICON_MediaPlayout playout = new RICON_MediaPlayout();
		if (!permit || !playout.Configure(route, firstSequence, firstSample, RICON_MediaPcmTestProfile.FRAME_SAMPLES,
			RICON_MediaPcmTestProfile.FRAME_DURATION_MS, targetMs, 8, 8, leadMs)
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
		array<int> decoded = RICON_MediaPcmTestProfile.Decode(m_Route.m_Source, decision);
		if (!decoded || !decision.m_Block.m_RfHistory || !decision.m_Block.m_CaptureEnvelope) return "INVALID";
		float power, noise;
		if (!RICON_SharedRxTestMixer.TryNormalizeRfBudget(decision.m_Block.m_RfHistory.m_Budget, referenceDbm, power, noise))
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
		string status = slot.Offer(decision.m_iTargetMs, nowMs, expiryMs, input,
			decision.m_Block.m_CaptureEnvelope, referenceDbm, decision.m_Block.m_RfHistory);
		if (status == "ACCEPTED")
		{
			m_iNextSample = firstSample + RICON_MediaPcmTestProfile.FRAME_SAMPLES;
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
	protected ref RICON_TemporalRxEnvironment m_LastNoiseEnvironment;
	protected float m_fNoiseReferenceDbm;
	protected int m_iEpoch, m_iIssuedAtMs = -1, m_iLastOutputNowMs = -1;
	protected bool m_bClosed;
	protected bool m_bPreparing;
	protected bool m_bExactOutput, m_bFiniteOutput;

	bool Configure(int epoch, RICON_RelayDestination destination, int anchorMs, int sampleIndex, int outputCapacity, int nowMs)
	{
		if (m_bClosed || m_Address || !destination || anchorMs < nowMs || nowMs < 0
			|| sampleIndex < 0 || sampleIndex % RICON_SharedRxTestMixer.WINDOW_SAMPLES != 0) return false;
		RICON_SharedRxPresentationTimeline timeline = new RICON_SharedRxPresentationTimeline();
		RICON_RelayOutputQueue output = new RICON_RelayOutputQueue();
		if (!timeline.ConfigureReceiver(epoch, destination, anchorMs, sampleIndex)
			|| !output.Configure(epoch, destination, outputCapacity, nowMs)) return false;
		m_Address = destination.CloneDestination();
		m_Timeline = timeline;
		m_Output = output;
		m_iEpoch = epoch;
		m_iLastOutputNowMs = nowMs;
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
		if (!IsOpen() || m_bPreparing || !route || !route.IsValid() || !Matches(route.m_Source.m_iSessionEpoch, RICON_RelayDestination.FromRoute(route)))
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
		if (m_Direct.Count() >= RICON_SharedRxTestMixer.MAX_INPUTS) return null;
		RICON_LocalRxDirectTrack track = new RICON_LocalRxDirectTrack();
		if (!track.Configure(receiver, route, firstTargetMs, leadMs)) return null;
		if (!IsOpen()) { track.Close(); return null; }
		m_Direct.Insert(track);
		return track;
	}

	string StageDirect(RICON_LocalRxDirectTrack track, int nowMs, float referenceDbm)
	{
		if (!IsOpen()) return "CLOSED";
		if (m_bPreparing) return "BUSY";
		if (!track || !m_Direct.Contains(track)) return "WRONG_RX";
		return track.Stage(this, nowMs, referenceDbm);
	}

	string Offer(int targetMs, int nowMs, int wireExpiryMs, RICON_SharedRxContribution input,
		RICON_MediaCaptureEnvelope capture = null, float referenceDbm = 0.0, RICON_MediaRfHistoryEntry admittedHistory = null)
	{
		if (!IsOpen()) return "CLOSED";
		if (m_bPreparing) return "BUSY";
		if (!input || !input.MatchesReceiver(m_iEpoch, m_Address)) return "WRONG_RX";
		// Очередь вывода требует разрешение на всё окно; короткий direct-вклад не портит уже смешанный PCM.
		if (targetMs >= 0 && (wireExpiryMs <= targetMs || wireExpiryMs - targetMs < RICON_SharedRxPresentationTimeline.WINDOW_MS))
			return "EXPIRED";
		ref RICON_SharedRxPresentationTimeline timeline = m_Timeline;
		string status;
		if (capture || admittedHistory)
			status = timeline.OfferDirect(targetMs, nowMs, wireExpiryMs, input, capture, referenceDbm, admittedHistory);
		else status = timeline.Offer(targetMs, nowMs, wireExpiryMs, input);
		if (!IsOpen()) return "CLOSED";
		return status;
	}

	string OfferRelay(RICON_RelayPresentationWindow window, float referenceDbm, int nowMs)
	{
		if (!IsOpen()) return "CLOSED";
		if (m_bPreparing) return "BUSY";
		ref RICON_SharedRxPresentationTimeline timeline = m_Timeline;
		string status = timeline.OfferRelay(window, referenceDbm, nowMs);
		if (!IsOpen()) return "CLOSED";
		return status;
	}

	bool OpenRelay(RICON_RelayWireReceiver receiver)
	{
		if (!IsOpen() || m_bPreparing || !receiver || !receiver.MatchesBoundDestination(m_Address)) return false;
		return receiver.ClaimRawCursor();
	}

	string StageRelay(RICON_RelayWireReceiver receiver, int sequence, int targetMs, int nowMs, float referenceDbm)
	{
		if (!IsOpen() || !receiver) return "CLOSED";
		if (m_bPreparing) return "BUSY";
		ref RICON_SharedRxPresentationTimeline timeline = m_Timeline;
		string status = receiver.StagePresentation(timeline, sequence, targetMs, nowMs, referenceDbm);
		if (!IsOpen()) return "CLOSED";
		return status;
	}

	int SkipLate(int nowMs)
	{
		if (!IsOpen() || m_bPreparing) return 0;
		return m_Timeline.SkipLate(nowMs);
	}

	//! Commit закрывает цель. Если очередь её не принимает, повторно эта цель не выдаётся.
	string Prepare(int targetMs, int nowMs)
	{
		if (!IsOpen()) return "CLOSED";
		if (m_bPreparing) return "BUSY";
		ref RICON_LocalRxSlot held = this;
		m_bPreparing = true;
		ref RICON_SharedRxPresentationTimeline timeline = m_Timeline;
		int expiryMs;
		ref RICON_SharedRxMixResult result = timeline.Commit(targetMs, nowMs, 0, null, expiryMs);
		string status = OfferPrepared(result, nowMs);
		m_bPreparing = false;
		return status;
	}

	//! TEST_ONLY direct PCM. Model time comes from the authority that owns the completed history.
	//! The held V3 capture envelope maps that history to this one local target.
	string PrepareWithNoise(int targetMs, int nowMs, RICON_MediaRfHistory history, RICON_MediaEgressRoute route,
		int modelNowMs, float referenceDbm, array<int> noiseSamples)
	{
		if (!IsOpen()) return "CLOSED";
		if (m_bPreparing) return "BUSY";
		if (!route || !route.IsValid() || !Matches(route.m_Source.m_iSessionEpoch, RICON_RelayDestination.FromRoute(route)))
			return "WRONG_RX";
		ref RICON_SharedRxPresentationTimeline timeline = m_Timeline;
		if (nowMs < m_iLastOutputNowMs || !timeline.IsNextTarget(targetMs)
			|| timeline.CheckOfferTime(targetMs, nowMs) != "OK") return "INVALID_CURSOR";
		if (!history || !RICON_SharedRxTestMixer.IsPcmWindow(noiseSamples)) return "INVALID_NOISE";
		ref RICON_MediaRfHistoryEntry admittedHistory;
		RICON_MediaCaptureEnvelope capture = timeline.ReadDirectCapture(targetMs, route, referenceDbm, admittedHistory);
		if (!capture) return "INVALID_NOISE";
		ref RICON_MediaRfHistoryEntry selected = history.Select(route, capture, modelNowMs, false);
		if (!selected || !selected.m_Budget || !selected.m_Budget.m_RxEnvironment) return "INVALID_NOISE";
		RICON_TemporalRfBudget budget = selected.m_Budget;
		RICON_TemporalRxEnvironment environment = budget.m_RxEnvironment;
		if (!environment.IsValidFor(m_iEpoch, m_Address.m_iRxDeviceId, m_Address.m_iRxIndex,
		capture.m_sTimebaseId, budget.m_fNoiseDbm, m_Address.m_iRxGeneration)) return "INVALID_NOISE";
	if (!environment.Covers(selected.m_iValidFromMs, selected.m_iValidUntilMs))
			return "INVALID_NOISE";
		if (m_LastNoiseEnvironment)
		{
			if (environment.m_iRevision < m_LastNoiseEnvironment.m_iRevision) return "STALE_NOISE";
			if (environment.m_sTimebaseId != m_LastNoiseEnvironment.m_sTimebaseId
				|| referenceDbm != m_fNoiseReferenceDbm) return "INVALID_NOISE";
			if (environment.m_iRevision == m_LastNoiseEnvironment.m_iRevision && !environment.Same(m_LastNoiseEnvironment))
				return "INVALID_NOISE";
		}
		if (!RICON_SharedRxPresentationTimeline.MatchesV3History(admittedHistory, selected)) return "INVALID_NOISE";
		float signalPowerRatio, noisePowerRatio;
		if (!RICON_SharedRxTestMixer.TryNormalizePower(admittedHistory.m_Budget.m_fRxPowerDbm, environment.m_fNoisePlusInterferenceDbm,
			referenceDbm, signalPowerRatio, noisePowerRatio)) return "INVALID_NOISE";

		array<int> detachedNoise = {};
		detachedNoise.InsertAll(noiseSamples);
		ref RICON_MediaEgressRoute noiseRoute = route.CloneRoute();
		ref RICON_LocalRxSlot held = this;
		m_bPreparing = true;
		int expiryMs;
		ref RICON_SharedRxMixResult result = timeline.CommitQualifiedNoise(targetMs, nowMs, noisePowerRatio,
			detachedNoise, environment, noiseRoute, referenceDbm, expiryMs);
		if (IsOpen() && result)
		{
			m_LastNoiseEnvironment = environment;
			m_fNoiseReferenceDbm = referenceDbm;
		}
		string status = OfferPrepared(result, nowMs);
		m_bPreparing = false;
		return status;
	}

	protected string OfferPrepared(RICON_SharedRxMixResult result, int nowMs)
	{
		if (!IsOpen()) return "CLOSED";
		if (!result) return "INVALID_CURSOR";
		ref RICON_RelayOutputQueue output = m_Output;
		m_iLastOutputNowMs = Math.Max(m_iLastOutputNowMs, nowMs);
		string status = output.Offer(result, nowMs);
		if (!IsOpen()) return "CLOSED";
		return status;
	}

	//! Transfers a whole prepared prefix once into the finite WAV owner.
	string PrepareFiniteWave(RICON_PcmWavePlaybackOwner owner, int firstTargetMs, int firstSampleIndex,
		int windowCount, int nowMs, out int clipId)
	{
		clipId = 0;
		if (!IsOpen()) return "CLOSED";
		if (m_bPreparing) return "BUSY";
		if (m_bExactOutput || !owner || owner.GetEpoch() != m_iEpoch || nowMs < m_iLastOutputNowMs)
			return "WRONG_OUTPUT";

		ref RICON_LocalRxSlot held = this;
		ref RICON_PcmWavePlaybackOwner wave = owner;
		ref RICON_RelayOutputQueue output = m_Output;
		m_bPreparing = true;
		array<ref RICON_MediaOutputPermit> permits;
		ref RICON_PcmBatch batch = output.TakeFiniteWavePrefix(firstTargetMs, firstSampleIndex, windowCount, nowMs, permits);
		string status = "PREFIX_UNAVAILABLE";
		if (batch && IsOpen() && m_Output == output)
		{
			m_bFiniteOutput = true;
			m_iLastOutputNowMs = nowMs;
			status = wave.PrepareFiniteRxClip(batch, permits, this, nowMs, clipId);
		}
		m_bPreparing = false;
		if (IsOpen() && m_Output == output) return status;
		clipId = 0;
		return "CLOSED";
	}

	RICON_SharedRxMixResult Take(int nowMs)
	{
		if (!IsOpen() || m_bPreparing || m_bFiniteOutput) return null;
		ref RICON_RelayOutputQueue output = m_Output;
		m_iLastOutputNowMs = Math.Max(m_iLastOutputNowMs, nowMs);
		ref RICON_SharedRxMixResult result = output.Take(nowMs);
		if (!IsOpen()) return null;
		if (result)
		{
			m_bExactOutput = true;
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
		if (!IsOpen() || m_bPreparing || !window || window != m_Issued || m_iIssuedAtMs != nowMs || window.m_iTargetMs != nowMs
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
		m_LastNoiseEnvironment = null;
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

