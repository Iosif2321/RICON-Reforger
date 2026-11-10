//! TOOL_ONLY: реальные getter observations + TEST_ONLY входы S3. Никаких engine effects.
class RICON_PairShadowEndpoint
{
	IEntity Actor;
	string ActorRpl;
	SCR_GadgetComponent Gadget;
	BaseRadioComponent Radio;
	BaseTransceiver Transceiver;
	int TransceiverCount;
	ref RICON_RadioObservation Observation;
}

class RICON_LivePairShadow
{
	static const int MAX_EVALUATIONS = 8;
	static const int POLL_MS = 1000;
	protected int m_LastClock = -1;
	protected int m_LastPoll = -1;
	protected int m_Evaluations;
	protected bool m_Done;
	protected string m_LastStatus;
	protected string m_Run;
	protected ref RICON_PairShadowEndpoint m_A;
	protected ref RICON_PairShadowEndpoint m_B;

	static bool Requested()
	{
		string value;
		return System.GetCLIParam("riconPairShadow", value) && value == "1" && RICON_LiveNativeRxProbe.IsRequested();
	}
	protected static string Bit(bool value)
	{
		if (value)
		{
			return "1";
		}
		return "0";
	}
	protected static string RplText(IEntity entity)
	{
		if (!entity) return "UNKNOWN";
		RplId id = Replication.FindItemId(entity);
		if (id == RplId.Invalid() || Replication.FindItem(id) != entity) return "UNKNOWN";
		return string.Format("%1", id);
	}
	protected static string Scope()
	{
		return ",scope=TEST_ONLY_SHADOW,authority=NONE,nativeEffect=NONE,perpairActuator=NONE,ApplySnapshot=BLOCKED_NO_ACTUATOR,streamJointToken=ABSENT";
	}
	protected void Status(int now, string status)
	{
		if (status == m_LastStatus) return;
		m_LastStatus = status;
		PrintFormat("[RICON_PAIR_SHADOW] run=%1 rawTickMs=%2 status=%3 callCode=NOT_CALLED pairCode=NOT_CALLED deviceCode=NOT_CALLED hasBudget=0 valid=0 reason=NOT_EVALUATED state=NOT_EVALUATED q=UNAVAILABLE%4", m_Run, now, status, Scope());
	}
	protected static bool ReadEndpoint(IEntity actor, BaseWorld world, int deviceId, out RICON_PairShadowEndpoint endpoint)
	{
		endpoint = null;
		if (!actor || actor.GetWorld() != world || RplText(actor) == "UNKNOWN") return false;
		ChimeraCharacter character = ChimeraCharacter.Cast(actor);
		if (!character || !character.GetCharacterController() || character.GetCharacterController().GetLifeState() != ECharacterLifeState.ALIVE) return false;
		SCR_GadgetManagerComponent gadgets = SCR_GadgetManagerComponent.GetGadgetManager(actor);
		if (!gadgets) return false;
		array<SCR_GadgetComponent> list = RICON_NativeRxGate.CombineCategories(gadgets.GetGadgetsByType(EGadgetType.RADIO), gadgets.GetGadgetsByType(EGadgetType.RADIO_BACKPACK));
		if (list.IsEmpty() || list.Count() > 4) return false;
		RICON_PairShadowEndpoint found = new RICON_PairShadowEndpoint();
		foreach (SCR_GadgetComponent gadget : list)
		{
			if (!gadget || !gadget.GetOwner() || !gadgets.IsGadgetOwned(gadget)) return false;
			BaseRadioComponent radio = BaseRadioComponent.Cast(gadget.GetOwner().FindComponent(BaseRadioComponent));
			if (!radio || (found.Radio && found.Radio != radio)) return false;
			found.Gadget = gadget; found.Radio = radio;
		}
		if (!found.Radio || !found.Radio.GetOwner() || found.Radio.GetOwner().GetWorld() != world) return false;
		found.TransceiverCount = found.Radio.TransceiversCount();
		if (found.TransceiverCount < 1 || found.TransceiverCount > 4) return false;
		for (int i = 0; i < found.TransceiverCount; i++)
		{
			BaseTransceiver transceiver = found.Radio.GetTransceiver(i);
			if (!transceiver || transceiver.GetRadio() != found.Radio) return false;
		}
		found.Transceiver = found.Radio.GetTransceiver(0);
		vector transform[4]; found.Radio.GetOwner().GetWorldTransform(transform);
		if (!RICON_BaseRadioObservationReader.TryRead(found.Radio, deviceId, 0, transform[3], found.Observation)) return false;
		found.Actor = actor; found.ActorRpl = RplText(actor);
		endpoint = found;
		return true;
	}
	protected static bool SameIdentity(RICON_PairShadowEndpoint before, RICON_PairShadowEndpoint after)
	{
		return before && after && before.Actor == after.Actor && before.ActorRpl == after.ActorRpl && before.Gadget == after.Gadget
			&& before.Radio == after.Radio && before.Transceiver == after.Transceiver && before.TransceiverCount == after.TransceiverCount;
	}
	//! Индекс0 выбран только стендом; это не binding активного VoN TX. RF значения не native getters.
	static RICON_RadioRfProfile TestProfile(RICON_RadioObservation observation)
	{
		RICON_RadioRfProfile p = new RICON_RadioRfProfile();
		p.m_sProfileId = "pair_shadow_test_only"; p.m_sProvenance = RICON_RadioRfProfile.PROVENANCE_TEST_ONLY;
		p.m_iDeviceId = observation.m_iDeviceId; p.m_iTransceiverIndex = observation.m_iTransceiverIndex;
		p.m_iExpectedTuningValue = observation.m_iTuningValue;
		p.m_fFrequencyMHz = observation.m_iTuningValue / 1000.0;
		p.m_fRxBandwidthHz = 12500; p.m_fTxPowerDbm = 37; p.m_fTxGainDbi = 0; p.m_fRxGainDbi = 0;
		p.m_fRxNoiseFigureDb = 8; p.m_bHasRequiredSinr = true; p.m_fRequiredSinrDb = 10;
		return p;
	}
	static RICON_PairServiceInput CandidateInput(int now, RICON_RadioObservation a, RICON_RadioObservation b)
	{
		RICON_PairServiceInput input = new RICON_PairServiceInput();
		input.m_iNowMs = now; input.m_iSessionEpoch = 1;
		input.m_Limits = new RICON_PairServiceLimits();
		input.m_Limits.m_iMaxBindings = 1; input.m_Limits.m_iMaxListeners = 1; input.m_Limits.m_iMaxPlayers = 2;
		input.m_Limits.m_iMaxDevices = 2; input.m_Limits.m_iMaxEndpoints = 2; input.m_Limits.m_iMaxPairs = 1;
		input.m_Context = new RICON_RadioQueryContext();
		input.m_Context.m_sBandId = RICON_TestSupport.BAND_ID; input.m_Context.m_sModelId = RICON_FreeSpaceModel.MODEL_ID;
		input.m_Context.m_sNoiseEnvironmentId = RICON_TestSupport.NOISE_ID;
		input.m_Context.m_sIntelligibilityModelId = RICON_LinearTestIntelligibility.MODEL_ID;
		RICON_TransmissionBinding binding = new RICON_TransmissionBinding();
		binding.m_iLeaseId = 1; binding.m_iSessionEpoch = 1; binding.m_iTxPlayerId = 1; binding.m_iTxGeneration = 1;
		binding.m_iTxDeviceId = a.m_iDeviceId; binding.m_iTxTransceiverIndex = a.m_iTransceiverIndex;
		binding.m_iNetworkTypeId = 1; binding.m_iChannelId = 1; binding.m_bActive = true;
		binding.m_iIssuedAtMs = now; binding.m_iTtlMs = 1000; input.m_aBindings.Insert(binding);
		RICON_PairListener listener = new RICON_PairListener();
		listener.m_iPlayerId = 2; listener.m_iGeneration = 1; listener.m_iSessionEpoch = 1;
		listener.m_iDeviceId = b.m_iDeviceId; listener.m_iTransceiverIndex = b.m_iTransceiverIndex; input.m_aListeners.Insert(listener);
		for (int i = 1; i <= 2; i++)
		{
			RICON_RadioObservation observed = a; if (i == 2) observed = b;
			RICON_PairPlayer player = new RICON_PairPlayer();
			player.m_iPlayerId = i; player.m_iGeneration = 1; player.m_eState = RICON_EPlayerLifecycleState.BOUND_ACTIVE;
			input.m_aPlayers.Insert(player);
			RICON_LifecycleDevice device = new RICON_LifecycleDevice();
			device.m_iDeviceId = observed.m_iDeviceId; device.m_eKind = RICON_EDeviceKind.CARRIED;
			device.m_iOwnerPlayerId = i; device.m_bPowered = observed.m_bPowered;
			device.m_aAllowed.Insert(new RICON_LifecycleCatalogPair(1, 1)); input.m_aDevices.Insert(device);
			RICON_PairEndpoint endpoint = new RICON_PairEndpoint();
			endpoint.m_Observation = observed; endpoint.m_Profile = TestProfile(observed); input.m_aEndpoints.Insert(endpoint);
		}
		return input;
	}
	protected void Result(int now, int completed, int bindingGeneration, RICON_PairShadowEndpoint a, RICON_PairShadowEndpoint b, RICON_PairServiceResult result, bool current)
	{
		string facts = string.Format(",aRpl=%1,bRpl=%2,aRadioRefLocal=%3,bRadioRefLocal=%4,aTransceiverRefLocal=%5,bTransceiverRefLocal=%6", a.ActorRpl, b.ActorRpl, a.Radio, b.Radio, a.Transceiver, b.Transceiver);
		facts = facts + ",aCatalog=" + a.TransceiverCount.ToString() + ",bCatalog=" + b.TransceiverCount.ToString() + ",aPowered=" + Bit(a.Observation.m_bPowered) + ",bPowered=" + Bit(b.Observation.m_bPowered);
		facts = facts + ",aTuning=" + a.Observation.m_iTuningValue.ToString() + ",bTuning=" + b.Observation.m_iTuningValue.ToString() + ",keysMatch=" + Bit(a.Observation.m_sNetworkKey == b.Observation.m_sNetworkKey);
		facts = facts + ",aRadioPos=" + a.Observation.m_vAntennaPos.ToString() + ",bRadioPos=" + b.Observation.m_vAntennaPos.ToString();
		facts = facts + ",observedBindingGeneration=" + bindingGeneration.ToString() + ",contextCurrent=" + Bit(current) + ",completedTickMs=" + completed.ToString();
		facts = facts + ",fixtureEpoch=1,fixtureLease=1,fixtureGeneration=1,bLifecycleAuthority=UNKNOWN,activeTxBinding=TEST_ONLY_CANDIDATE,endpointSelection=TEST_ONLY_NATIVE_INDEX0,antennaPoint=RADIO_ENTITY_ORIGIN,atomic=0";
		facts = facts + ",rfProfile=TEST_ONLY,txPowerDbm=37,bandwidthHz=12500,noiseFigureDb=8,requiredSinrDb=10,rfGainsDbi=0,nativeRfParameters=NOT_READ,propagation=free_space,intelligibility=linear_test,radioRange=NOT_MODEL_INPUT";
		string line = "[RICON_PAIR_SHADOW] run=" + m_Run + " rawTickMs=" + now.ToString() + " status=PAIR_RESULT callCode=" + typename.EnumToString(RICON_EPairServiceCode, result.m_eCallCode);
		line = line + " pairCount=" + result.m_aPairs.Count().ToString() + " s2Calls=" + result.m_iS2Calls.ToString() + " evaluatorCalls=" + result.m_iEvaluatorCalls.ToString();
		if (result.m_aPairs.Count() == 1)
		{
			RICON_PairResult row = result.m_aPairs[0];
			line = line + " pairCode=" + typename.EnumToString(RICON_EPairCode, row.m_ePairCode) + " hasDeviceCode=" + Bit(row.m_bHasDeviceCode);
			line = line + " deviceCode=" + typename.EnumToString(RICON_ERadioQueryCode, row.m_eDeviceCode) + " hasBudget=" + Bit(row.m_bHasBudget) + " valid=" + Bit(row.m_bValid);
			line = line + " reason=" + typename.EnumToString(RICON_EReason, row.m_eReason) + " state=" + typename.EnumToString(RICON_ELinkState, row.m_eState) + " q=" + row.m_fIntelligibility.ToString();
		}
		else line = line + " pairCode=NOT_RETURNED deviceCode=NOT_EVALUATED hasBudget=0 valid=0 reason=NOT_EVALUATED state=NOT_EVALUATED q=UNAVAILABLE";
		PrintFormat("%1", line + facts + Scope());
	}
	void Tick(BaseWorld world, RICON_LiveNativeRxProbe probe)
	{
		if (m_Done || !Requested() || !world || world.IsEditMode() || !probe) return;
		int now = System.GetTickCount();
		if (now < 0 || (m_LastClock >= 0 && now < m_LastClock)) { m_Done = true; Status(now, "CLOCK_INVALID"); return; }
		m_LastClock = now;
		if (m_LastPoll >= 0 && now - m_LastPoll < POLL_MS) return;
		m_LastPoll = now;
		if (!System.GetCLIParam("riconNativeRxRun", m_Run) || !RICON_D0Gate.ValidRunId(m_Run)) { m_Done = true; Status(now, "RUN_INVALID"); return; }
		if (GetGame().GetWorld() != world || !RICON_PairPlacementGate.WorldAllowed(GetGame().GetWorldFile())) { m_Done = true; Status(now, "WORLD_REFUSED"); return; }
		if (!Replication.IsServer()) { m_Done = true; Status(now, "REQUIRES_LOCAL_A_SERVER"); return; }
		int remaining;
		bool nrxCurrent = probe.RICON_PulseHealth(now, m_Run, 1, remaining);
		if (remaining <= 0) { m_Done = true; Status(now, "NRX_ENDED"); m_A = null; m_B = null; return; }
		if (!nrxCurrent)
		{
			Status(now, "WAIT_NRX_CURRENT");
			if (m_A) { m_Done = true; m_A = null; m_B = null; }
			return;
		}
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || controller.GetPlayerId() != 1) { m_Done = true; Status(now, "REQUIRES_LOCAL_A_SERVER"); return; }
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players) { Status(now, "WAIT_PLAYER_MANAGER"); return; }
		IEntity actorA = players.GetPlayerControlledEntity(1); IEntity actorB = players.GetPlayerControlledEntity(2);
		if (!actorA || !actorB || actorA == actorB || controller.GetControlledEntity() != actorA) { Status(now, "WAIT_DISTINCT_ACTORS"); return; }
		int generation = probe.RICON_PairPlacementBinding(now, m_Run, world, controller, actorA);
		if (generation != 1) { Status(now, "WAIT_CURRENT_BINDING"); return; }
		RICON_PairShadowEndpoint a; RICON_PairShadowEndpoint b;
		if (!ReadEndpoint(actorA, world, 1, a) || !ReadEndpoint(actorB, world, 2, b) || a.Radio == b.Radio || a.ActorRpl == b.ActorRpl) { Status(now, "WAIT_OWNED_RADIO_OBSERVATIONS"); return; }
		if (m_A && (!SameIdentity(m_A, a) || !SameIdentity(m_B, b))) { m_Done = true; Status(now, "BINDING_CHANGED"); return; }
		m_A = a; m_B = b;
		RICON_PairServiceInput input = CandidateInput(now, a.Observation, b.Observation);
		RICON_WirelessLinkEvaluator evaluator = new RICON_WirelessLinkEvaluator(RICON_TestSupport.BAND_ID, new RICON_FreeSpaceModel(), new RICON_NoiseModel(), RICON_TestSupport.NewTestIntelligibility());
		RICON_PairService service = new RICON_PairService();
		RICON_PairServiceResult result = service.Evaluate(input, evaluator);
		RICON_PairShadowEndpoint afterA; RICON_PairShadowEndpoint afterB;
		bool current = players.GetPlayerControlledEntity(1) == actorA && players.GetPlayerControlledEntity(2) == actorB
			&& ReadEndpoint(actorA, world, 1, afterA) && ReadEndpoint(actorB, world, 2, afterB) && SameIdentity(a, afterA) && SameIdentity(b, afterB);
		if (current) current = a.Observation.m_bPowered == afterA.Observation.m_bPowered && b.Observation.m_bPowered == afterB.Observation.m_bPowered
			&& a.Observation.m_iTuningValue == afterA.Observation.m_iTuningValue && b.Observation.m_iTuningValue == afterB.Observation.m_iTuningValue
			&& a.Observation.m_sNetworkKey == afterA.Observation.m_sNetworkKey && b.Observation.m_sNetworkKey == afterB.Observation.m_sNetworkKey;
		int checkedAt = System.GetTickCount();
		if (current) current = probe.RICON_PairPlacementBinding(checkedAt, m_Run, world, controller, actorA) == generation;
		int completed = System.GetTickCount();
		current = current && completed >= now && completed - now <= 500 && completed - now < remaining;
		Result(now, completed, generation, a, b, result, current);
		m_Evaluations++;
		if (!current || m_Evaluations >= MAX_EVALUATIONS) { m_Done = true; Status(completed, "SHADOW_ENDED"); m_A = null; m_B = null; }
	}
}
