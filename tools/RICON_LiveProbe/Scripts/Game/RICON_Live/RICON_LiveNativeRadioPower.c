//! Отдельная capability whole-radio power state. Capture/route/audio отсутствуют.
class RICON_RadioPowerGate
{
	static bool Config(bool configured, string value, bool nrx, bool conflict)
	{
		return configured && value == "1" && nrx && !conflict;
	}
	static int Restore(bool invoked, bool identity, bool protectedSame, bool currentPowered)
	{
		if (!invoked || !identity || !protectedSame)
			return -1;
		if (currentPowered)
			return 0;
		return 1;
	}
}

//! Pure observation одного ON call. Deadline никогда не переносится событием или getter.
class RICON_RadioRestoreObservation
{
	int CallAt = -1;
	int FirstOnAt = -1;
	int LastSampleAt = -1;
	bool Drift;
	bool LateSeen;
	bool Begin(int now)
	{
		if (CallAt >= 0 || now < 0) return false;
		CallAt = now;
		return true;
	}
	void Observe(int now, bool context, bool powered)
	{
		if (CallAt < 0) return;
		if (!context || now < CallAt || (LastSampleAt >= 0 && now < LastSampleAt)) { Drift = true; return; }
		LastSampleAt = now;
		if (FirstOnAt >= 0 && !powered) Drift = true;
		if (now - CallAt > 1000) { LateSeen = true; return; }
		if (!Drift && powered && FirstOnAt < 0) FirstOnAt = now;
	}
	bool Due(int now) { return CallAt >= 0 && now - CallAt >= 1000; }
	bool Qualified() { return FirstOnAt >= 0 && !Drift; }
}

//! Единственный radio и его bounded transceivers. Power намеренно исключён из identity/protected state.
class RICON_RadioPowerSnapshot
{
	IEntity Entity;
	SCR_GadgetComponent Gadget;
	BaseRadioComponent Radio;
	ref array<ref RICON_FrequencyOnlyAlignment> Endpoints = {};
	ref array<bool> Mutes = {};
	protected SCR_VoNComponent m_Von;
	protected ECommMethod m_Method;
	protected BaseTransceiver m_Transmit;
	bool Capture(IEntity entity)
	{
		if (!entity) return false;
		m_Von = SCR_VoNComponent.Cast(entity.FindComponent(SCR_VoNComponent));
		if (!m_Von) return false;
		m_Method = m_Von.GetCommMethod(); m_Transmit = m_Von.GetTransmitRadio();
		SCR_GadgetManagerComponent manager = SCR_GadgetManagerComponent.GetGadgetManager(entity);
		if (!manager) return false;
		array<SCR_GadgetComponent> gadgets = RICON_NativeRxGate.CombineCategories(manager.GetGadgetsByType(EGadgetType.RADIO), manager.GetGadgetsByType(EGadgetType.RADIO_BACKPACK));
		if (gadgets.Count() > 4) return false;
		foreach (SCR_GadgetComponent gadget : gadgets)
		{
			if (!gadget || !gadget.GetOwner()) return false;
			BaseRadioComponent radio = BaseRadioComponent.Cast(gadget.GetOwner().FindComponent(BaseRadioComponent));
			if (!radio) continue;
			if (Radio && Radio != radio) return false;
			if (!Radio) { Entity = entity; Gadget = gadget; Radio = radio; }
		}
		if (!Radio || Radio.TransceiversCount() < 1 || Radio.TransceiversCount() > 4) return false;
		for (int i = 0; i < Radio.TransceiversCount(); i++)
		{
			BaseTransceiver tr = Radio.GetTransceiver(i);
			if (!tr) return false;
			RICON_FrequencyOnlyAlignment endpoint = new RICON_FrequencyOnlyAlignment();
			if (!endpoint.Capture(entity, Gadget, Radio, tr, i, tr.GetFrequency())) { endpoint.ClearPrivate(); return false; }
			Endpoints.Insert(endpoint); Mutes.Insert(tr.IsMuted());
		}
		return RICON_PowerRefsCurrent(entity);
	}
	bool RICON_PowerRefsCurrent(IEntity entity)
	{
		if (!entity || entity != Entity || !Radio || Radio.TransceiversCount() != Endpoints.Count()) return false;
		SCR_GadgetManagerComponent manager = SCR_GadgetManagerComponent.GetGadgetManager(entity);
		if (!manager) return false;
		array<SCR_GadgetComponent> gadgets = RICON_NativeRxGate.CombineCategories(manager.GetGadgetsByType(EGadgetType.RADIO), manager.GetGadgetsByType(EGadgetType.RADIO_BACKPACK));
		if (gadgets.Count() > 4) return false;
		foreach (SCR_GadgetComponent gadget : gadgets)
		{
			if (!gadget || !gadget.GetOwner()) return false;
			BaseRadioComponent radio = BaseRadioComponent.Cast(gadget.GetOwner().FindComponent(BaseRadioComponent));
			if (radio && radio != Radio) return false;
		}
		foreach (RICON_FrequencyOnlyAlignment endpoint : Endpoints) if (!endpoint.Matches(entity)) return false;
		return true;
	}
	bool ProtectedSame()
	{
		if (!RICON_PowerRefsCurrent(Entity)) return false;
		if (!m_Von || Entity.FindComponent(SCR_VoNComponent) != m_Von || m_Von.GetCommMethod() != m_Method || m_Von.GetTransmitRadio() != m_Transmit) return false;
		for (int i = 0; i < Endpoints.Count(); i++)
		{
			if (!Endpoints[i].KeysSame() || Endpoints[i].Frequency() != Endpoints[i].Original() || Radio.GetTransceiver(i).IsMuted() != Mutes[i]) return false;
		}
		return true;
	}
	void Clear()
	{
		foreach (RICON_FrequencyOnlyAlignment endpoint : Endpoints) endpoint.ClearPrivate();
	}
}

class RICON_LiveNativeRadioPower
{
	protected RICON_LiveNativeRxProbe m_Nrx;
	protected BaseWorld m_World;
	protected PlayerController m_Controller;
	protected ref RICON_RadioPowerSnapshot m_Snapshot;
	protected ref ScriptInvoker<bool> m_Event;
	protected string m_Run;
	protected string m_BRpl;
	protected int m_Player;
	protected int m_Started;
	protected int m_Last;
	protected int m_Poll;
	protected int m_BoundAt;
	protected int m_OffAt;
	protected ref RICON_RadioRestoreObservation m_RestoreObservation = new RICON_RadioRestoreObservation();
	protected bool m_RestoreNoop;
	protected bool m_InCallback;
	protected int m_MirrorOnAt = -1;
	protected int m_Rows;
	protected int m_Events;
	protected int m_Setters;
	protected bool m_Running;
	protected bool m_Bound;
	protected bool m_Invoked;
	protected bool m_RestoreUsed;
	protected bool m_OffObserved;
	protected bool m_InNative;
	protected bool m_Stopping;
	protected int m_RestoreReadback = -1;
	protected string m_Deferred;
	static bool Configured()
	{
		string value;
		return System.GetCLIParam("riconNativeRadioPower", value);
	}
	static bool Conflict()
	{
		array<string> names = {"riconPairPlacement", "riconCaptureSeparatedPair", "riconPulseMode", "riconPulseAttempt", "riconRestCapability", "riconRestCapturePulse", "riconRestRehearsalArm", "riconCaptureNoSend", "riconCaptureWrongFrequency", "riconCaptureShort400", "riconCaptureBlindTrial", "riconVoNSoundObserve", "riconBlindBObserve", "riconAudioMeter", "riconCommunicationDiscovery", "riconVoNInputShape", "riconVoNMapping"};
		foreach (string name : names) { string value; if (System.GetCLIParam(name, value)) return true; }
		return false;
	}
	protected static string Bit(bool value)
	{
		if (value)
			return "1";
		return "0";
	}
	protected static string Rpl(IEntity entity)
	{
		if (!entity) return "UNKNOWN";
		RplId id = Replication.FindItemId(entity);
		if (id == RplId.Invalid() || Replication.FindItem(id) != entity) return "UNKNOWN";
		return string.Format("%1", id);
	}
	protected void Emit(string kind, string fields)
	{
		if (m_Rows >= 160 && kind != "TERMINAL") { Stop("OVERFLOW"); return; }
		string view = "UNBOUND";
		if (m_Player == 1) view = "A_SERVER_MIRROR";
		else if (m_Player == 2) view = "B_LOCAL_OWNER";
		string line = "[RICON_RPOWER] seq=" + m_Rows.ToString() + " run=" + RICON_LiveText.Esc(m_Run);
		line = line + " attempt=RadioPower.1 view=" + view + " kind=" + kind;
		line = line + " rawTickMs=" + System.GetTickCount().ToString() + " text=" + fields;
		m_Rows++; PrintFormat("%1", line);
	}
	bool Start(BaseWorld world, RICON_LiveNativeRxProbe nrx)
	{
		string value; bool configured = System.GetCLIParam("riconNativeRadioPower", value);
		if (!RICON_RadioPowerGate.Config(configured, value, RICON_LiveNativeRxProbe.IsRequested(), Conflict())) return false;
		if (!world || world.IsEditMode() || !nrx || !RICON_PairPlacementGate.WorldAllowed(GetGame().GetWorldFile())) return false;
		if (!System.GetCLIParam("riconNativeRxRun", m_Run) || !RICON_D0Gate.ValidRunId(m_Run)) return false;
		m_World = world; m_Nrx = nrx; m_Started = System.GetTickCount(); m_Last = m_Started; m_Poll = m_Started;
		m_Running = true;
		Emit("WAITING", "captureCalls=0,powerScope=WHOLE_DEVICE,authority=UNQUALIFIED,DIRECT=UNKNOWN");
		return true;
	}
	protected bool OwnContext()
	{
		if (!m_World || GetGame().GetWorld() != m_World || m_World.IsEditMode() || !m_Controller || GetGame().GetPlayerController() != m_Controller) return false;
		if (m_Controller.GetPlayerId() != m_Player || !m_Snapshot || !m_Snapshot.Entity || Rpl(m_Snapshot.Entity) != m_BRpl) return false;
		ChimeraCharacter character = ChimeraCharacter.Cast(m_Snapshot.Entity);
		if (!character || !character.GetCharacterController() || character.GetCharacterController().GetLifeState() != ECharacterLifeState.ALIVE) return false;
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager || manager.GetPlayerControlledEntity(2) != m_Snapshot.Entity || !m_Snapshot.RICON_PowerRefsCurrent(m_Snapshot.Entity)) return false;
		if (m_Player == 2) return Replication.IsClient() && !Replication.IsServer() && m_Controller.GetControlledEntity() == m_Snapshot.Entity;
		return m_Player == 1 && Replication.IsServer();
	}
	protected bool Current()
	{
		return OwnContext() && m_Snapshot.ProtectedSame() && m_Snapshot.Radio.m_OnPowerChanged == m_Event;
	}
	protected bool ObservationCurrent(int now)
	{
		int remaining;
		return m_Running && !RICON_NativeRxGate.ClockGap(m_Last, now) && !RICON_NativeRxGate.Expired(now - m_Started)
			&& m_Nrx && m_Nrx.RICON_PulseHealth(now, m_Run, m_Player, remaining) && Current();
	}
	protected void State(string kind)
	{
		bool current = Current();
		string fields = "localPlayer=" + m_Player.ToString() + ",bindingGeneration=1,bEntityRpl=" + m_BRpl;
		fields = fields + ",identity=" + Bit(OwnContext()) + ",protectedSame=" + Bit(current);
		fields = fields + ",radioRefLocal=" + string.Format("%1", m_Snapshot.Radio) + ",transceivers=" + m_Snapshot.Endpoints.Count().ToString();
		string powered = "UNKNOWN";
		if (current)
		{
			bool observedPower = m_Snapshot.Radio.IsPowered();
			powered = Bit(observedPower);
			ObserveMirrorGetter(observedPower, System.GetTickCount());
			if (m_Player == 2 && m_RestoreObservation.CallAt >= 0)
			{
				int observedAt = System.GetTickCount();
				m_RestoreReadback = 0;
				if (observedPower) m_RestoreReadback = 1;
				m_RestoreObservation.Observe(observedAt, ObservationCurrent(observedAt), observedPower);
			}
		}
		fields = fields + ",powered=" + powered + ",powerEvents=" + m_Events.ToString();
		Emit(kind, fields + ",nativeOwnership=UNKNOWN,authority=UNQUALIFIED,replication=UNQUALIFIED,readbackScope=THIS_PROCESS");
	}
	//! Только getter A-view; payload события и B writer не являются источником mirror state.
	protected void ObserveMirrorGetter(bool powered, int now)
	{
		if (m_Player != 1) return;
		if (!powered) m_OffObserved = true;
		else if (m_OffObserved && m_MirrorOnAt < 0) m_MirrorOnAt = now;
	}
	protected void Bind(int now)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || controller.GetPlayerId() == 0) return;
		int player = controller.GetPlayerId(); int remaining;
		if ((player != 1 && player != 2) || (player == 1 && !Replication.IsServer()) || (player == 2 && (Replication.IsServer() || !Replication.IsClient()))) { Stop("ROLE"); return; }
		if (!m_Nrx.RICON_PulseHealth(now, m_Run, player, remaining)) return;
		PlayerManager manager = GetGame().GetPlayerManager();
		IEntity entity; if (manager) entity = manager.GetPlayerControlledEntity(2);
		if (!entity || entity.GetWorld() != m_World || Rpl(entity) == "UNKNOWN") return;
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character || !character.GetCharacterController() || character.GetCharacterController().GetLifeState() != ECharacterLifeState.ALIVE) return;
		RICON_RadioPowerSnapshot snapshot = new RICON_RadioPowerSnapshot();
		if (!snapshot.Capture(entity)) { snapshot.Clear(); return; }
		m_Snapshot = snapshot; m_Controller = controller; m_Player = player; m_BRpl = Rpl(entity);
		m_Event = snapshot.Radio.m_OnPowerChanged;
		if (!m_Event) { Stop("NO_POWER_EVENT_HOOK"); return; }
		m_Event.Insert(OnPowerChanged); m_Bound = true; m_BoundAt = System.GetTickCount();
		if (!ObservationCurrent(m_BoundAt)) { Stop("BINDING_OR_BUDGET"); return; }
		State("BOUND");
		if (player == 2 && !snapshot.Radio.IsPowered()) Stop("BASELINE_NOT_ON");
	}
	protected void OnPowerChanged(bool powered)
	{
		if (!m_Running) return;
		m_InCallback = true;
		m_Events++;
		Emit("POWER_EVENT", "payloadPowered=" + Bit(powered) + ",inNativeCall=" + Bit(m_InNative) + ",bEntityRpl=" + m_BRpl);
		int now = System.GetTickCount();
		if (RICON_NativeRxGate.ClockGap(m_Last, now) || RICON_NativeRxGate.Expired(now - m_Started) || !Current())
		{
			m_Deferred = "EVENT_CONTEXT_DRIFT"; m_InCallback = false; return;
		}
		m_Last = now;
		State("EVENT_READBACK");
		if (m_Running && m_Player == 2 && m_Setters > 0 && !m_RestoreUsed && !m_Snapshot.Radio.IsPowered()) m_OffObserved = true;
		m_InCallback = false;
	}
	protected void Off()
	{
		if (!m_Running || m_Invoked || m_Player != 2) return;
		m_Invoked = true;
		if (!Current() || !m_Snapshot.Radio.IsPowered()) { Stop("PRE_OFF_DRIFT"); return; }
		Emit("OFF_INTENT", "oneUse=1,expectedPowered=1,returnType=VOID");
		if (!ObservationCurrent(System.GetTickCount()) || !m_Snapshot.Radio.IsPowered()) { Stop("PRE_OFF_DRIFT"); return; }
		m_OffAt = System.GetTickCount(); m_InNative = true; m_Setters++;
		m_Snapshot.Radio.SetPower(false);
		m_InNative = false;
		int returnedAt = System.GetTickCount();
		Emit("OFF_RETURN", "callStartTickMs=" + m_OffAt.ToString() + ",callReturnTickMs=" + returnedAt.ToString() + ",returnType=VOID,effect=UNQUALIFIED");
		if (!m_Deferred.IsEmpty()) { Stop(m_Deferred); return; }
		if (!ObservationCurrent(System.GetTickCount())) { Stop("APPLIED_UNKNOWN"); return; }
		m_OffObserved = !m_Snapshot.Radio.IsPowered(); State("OFF_READBACK");
	}
	protected bool Restore()
	{
		if (m_RestoreUsed || m_Player != 2) return false;
		m_RestoreUsed = true;
		bool identity = OwnContext(); bool same = identity && m_Snapshot.ProtectedSame();
		bool powered; if (same) powered = m_Snapshot.Radio.IsPowered();
		int decision = RICON_RadioPowerGate.Restore(m_Setters > 0, identity, same, powered);
		if (decision < 0) { Emit("RESTORE_UNKNOWN", "setterInvoked=0,noClobber=1"); return false; }
		if (decision == 0) { m_RestoreNoop = true; m_RestoreReadback = 1; Emit("RESTORE_ALREADY_ON", "setterInvoked=0"); return true; }
		Emit("RESTORE_INTENT", "expectedPowered=0,returnType=VOID");
		if (!OwnContext() || !m_Snapshot.ProtectedSame() || m_Snapshot.Radio.IsPowered()) { Emit("RESTORE_UNKNOWN", "setterInvoked=0,noClobber=1"); return false; }
		m_InNative = true;
		int callAt = System.GetTickCount();
		if (!m_RestoreObservation.Begin(callAt)) { m_InNative = false; return false; }
		m_Setters++;
		m_Snapshot.Radio.SetPower(true);
		m_InNative = false;
		int returnedAt = System.GetTickCount();
		Emit("RESTORE_RETURN", "callStartTickMs=" + callAt.ToString() + ",callReturnTickMs=" + returnedAt.ToString() + ",returnType=VOID,effect=UNQUALIFIED");
		if (!OwnContext() || !m_Snapshot.ProtectedSame()) return false;
		m_RestoreReadback = 0;
		if (m_Snapshot.Radio.IsPowered()) m_RestoreReadback = 1;
		State("RESTORE_READBACK");
		return true;
	}
	void Tick(BaseWorld world)
	{
		if (!m_Running) return;
		int now = System.GetTickCount();
		if (!world || world != m_World || world.IsEditMode() || RICON_NativeRxGate.ClockGap(m_Last, now) || RICON_NativeRxGate.Expired(now - m_Started)) { Stop("WORLD_CLOCK_LIMIT"); return; }
		m_Last = now;
		if (!m_Deferred.IsEmpty()) { Stop(m_Deferred); return; }
		if (m_Player == 2 && m_RestoreObservation.Drift) { Stop("RESTORE_DRIFT"); return; }
		if (m_Player == 2 && m_RestoreObservation.Due(now))
		{
			if (!ObservationCurrent(now)) { Stop("APPLIED_UNKNOWN"); return; }
			State("RESTORE_DEADLINE");
			if (!m_Running) return;
			if (m_RestoreObservation.Qualified()) Stop("LOCAL_STATE_CYCLE_OBSERVED");
			else Stop("APPLIED_UNKNOWN_RESTORE_TIMEOUT_OR_DRIFT");
			return;
		}
		if (now - m_Poll < 500) return;
		m_Poll = now;
		if (!m_Bound) { Bind(now); return; }
		if (!ObservationCurrent(now)) { Stop("APPLIED_UNKNOWN"); return; }
		State("STATE");
		if (!m_Running) return;
		if (m_Player == 1)
		{
			if (m_MirrorOnAt >= 0 && now - m_MirrorOnAt >= 1000) Stop("MIRROR_SEQUENCE_OBSERVED");
			return;
		}
		if (m_Invoked && !m_RestoreUsed)
		{
			if (!m_Snapshot.Radio.IsPowered()) m_OffObserved = true;
			else if (m_OffObserved) { Stop("POWER_CHANGED_OUTSIDE_EXPECTED_WINDOW"); return; }
		}
		if (!m_Invoked && now - m_BoundAt >= 1000) { Off(); return; }
		if (m_Invoked && !m_RestoreUsed && now - m_OffAt >= 2000)
		{
			if (!m_OffObserved) { Stop("APPLIED_UNKNOWN_OFF_NOT_OBSERVED"); return; }
			if (!Restore()) { Stop("APPLIED_UNKNOWN"); return; }
			if (!m_Deferred.IsEmpty()) { Stop(m_Deferred); return; }
			if (m_RestoreNoop) { Stop("ALREADY_ORIGINAL_ON"); return; }
		}
	}
	void Stop(string reason)
	{
		if (!m_Running || m_Stopping) return;
		if (m_InNative || m_InCallback) { m_Deferred = reason; return; }
		m_Stopping = true;
		if (m_RestoreObservation.CallAt >= 0 && reason != "LOCAL_STATE_CYCLE_OBSERVED" && !m_RestoreObservation.Due(System.GetTickCount()))
			reason = "APPLIED_UNKNOWN_" + reason;
		if (m_Player == 2 && m_Setters > 0 && !m_RestoreUsed) Restore();
		if (m_RestoreObservation.CallAt >= 0 && reason != "LOCAL_STATE_CYCLE_OBSERVED")
			m_RestoreObservation.Observe(System.GetTickCount(), false, false);
		m_Running = false;
		if (m_Event) m_Event.Remove(OnPowerChanged);
		string fields = "reason=" + reason + ",setters=" + m_Setters.ToString() + ",powerEvents=" + m_Events.ToString();
		fields = fields + ",restoreReadback=" + m_RestoreReadback.ToString() + ",offReadback=" + Bit(m_OffObserved) + ",subscriptionsRemaining=0";
		fields = fields + ",restoreCallTickMs=" + m_RestoreObservation.CallAt.ToString() + ",firstQualifiedOnTickMs=" + m_RestoreObservation.FirstOnAt.ToString();
		int deadline = -1;
		if (m_RestoreObservation.CallAt >= 0) deadline = m_RestoreObservation.CallAt + 1000;
		fields = fields + ",restoreDeadlineTickMs=" + deadline.ToString();
		fields = fields + ",restoreQualified=" + Bit(m_RestoreObservation.Qualified()) + ",restoreLateGetter=" + Bit(m_RestoreObservation.LateSeen);
		Emit("TERMINAL", fields + ",captureCalls=0,authority=UNQUALIFIED,replication=UNQUALIFIED,DIRECT=UNKNOWN,powerScope=WHOLE_DEVICE");
		if (m_Snapshot) m_Snapshot.Clear();
		m_Event = null; m_Nrx = null; m_World = null; m_Controller = null;
	}
}
