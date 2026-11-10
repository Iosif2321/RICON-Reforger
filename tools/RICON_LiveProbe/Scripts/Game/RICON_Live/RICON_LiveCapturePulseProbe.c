//! TOOL_ONLY coordinator. Execution permission and guard ACK live in the trusted external coordinator receipt.
enum RICON_ECapturePulseMode
{
	OFF = 0,
	REHEARSAL = 1,
	CAPTURE = 2
}

class RICON_CapturePulseGate
{
	static const int INTENDED_MS = 1000;
	static const int CUTOFF_MS = 1500;
	static const int VALIDATION_MS = 2000;
	static const int MIN_TRIGGER_REMAINING_MS = 1100;
	static const int LOCAL_START_SLOP_MS = MIN_TRIGGER_REMAINING_MS - INTENDED_MS;
	//! Результат предполагаемого InitEncryptionKey без изменения controller cache.
	static string EffectiveLocalKey(string cachedKey, bool factionKnown, string factionKey)
	{
		if (factionKnown)
			return factionKey;
		return cachedKey;
	}
	static string FactionPolicy(bool crossFaction, bool admin, string radioKey, string effectiveLocalKey)
	{
		if (crossFaction)
			return "CROSS_FACTION";
		if (admin)
			return "ADMIN";
		if (effectiveLocalKey.IsEmpty())
			return "EMPTY_LOCAL_KEY";
		if (radioKey == effectiveLocalKey)
			return "MATCHED_LOCAL_KEY";
		return "LOCAL_KEY_MISMATCH";
	}
	static RICON_ECapturePulseMode Mode(bool configured, string value)
	{
		if (!configured) return RICON_ECapturePulseMode.OFF;
		if (value == "rehearsal") return RICON_ECapturePulseMode.REHEARSAL;
		if (value == "capture") return RICON_ECapturePulseMode.CAPTURE;
		return RICON_ECapturePulseMode.OFF;
	}
}

//! Только фиксированный short400 pilot; обычный intended stop остаётся 1000 мс.
class RICON_CaptureShort400Gate
{
	static bool Config(bool configured, string value, bool serverA, bool separated, bool captureMode, bool guardedCapture, bool otherControl)
	{
		if (!configured)
			return true;
		return value == "1" && serverA && separated && captureMode && guardedCapture && !otherControl;
	}
}

//! Pure one-attempt state. Local timestamps are not the external QPC/guard origin.
class RICON_CapturePulseState
{
	protected bool m_bArmUsed;
	protected bool m_bStartUsed;
	protected bool m_bStopUsed;
	protected bool m_bInsideNative;
	protected bool m_bStopPending;
	protected bool m_bFinished;
	protected bool m_bInvalid;
	protected bool m_bStartAccepted;
	protected bool m_bStopAccepted;
	protected int m_iLast;
	protected int m_iArmAt;
	protected int m_iStartAt;
	protected int m_iIntendedMs = RICON_CapturePulseGate.INTENDED_MS;
	protected int m_iReturnAt = -1;
	protected string m_sReason;
	bool UseShort400()
	{
		if (m_bArmUsed || m_bFinished || m_bInvalid || m_iIntendedMs != RICON_CapturePulseGate.INTENDED_MS)
			return false;
		m_iIntendedMs = 400;
		return true;
	}
	int StartRequestTick() { return m_iStartAt; }
	int LastReturnTick() { return m_iReturnAt; }

	bool ArmUsed() { return m_bArmUsed; }
	bool StartUsed() { return m_bStartUsed; }
	bool StopUsed() { return m_bStopUsed; }
	bool InsideNative() { return m_bInsideNative; }
	bool Finished() { return m_bFinished; }
	bool Invalid() { return m_bInvalid; }
	string Reason() { return m_sReason; }
	void RejectBeforeRequest(string reason)
	{
		if (m_bStartUsed) { m_bInvalid = true; RequestStop(reason); return; }
		m_bInvalid = true; m_bFinished = true; m_sReason = reason;
	}
	bool Arm(int now)
	{
		if (m_bArmUsed || m_bFinished) return false;
		m_bArmUsed = true; m_iArmAt = now; m_iLast = now;
		if (now < 0)
		{
			m_bInvalid = true; m_bFinished = true; m_sReason = "ARM_CLOCK"; return false;
		}
		return true;
	}
	protected bool Clock(int now)
	{
		if (now < m_iLast)
		{
			m_bInvalid = true; m_bStopPending = true; m_sReason = "CLOCK_RESET"; return false;
		}
		m_iLast = now;
		return true;
	}
	bool CanBegin(int now, bool contextValid)
	{
		return m_bArmUsed && !m_bStartUsed && !m_bFinished && !m_bInvalid && contextValid
			&& now >= m_iLast && now - m_iArmAt <= RICON_CapturePulseGate.LOCAL_START_SLOP_MS;
	}
	bool BeginRequest(int now, bool contextValid)
	{
		if (!m_bArmUsed || m_bStartUsed || m_bFinished || m_bInvalid) return false;
		if (!Clock(now)) { m_bFinished = true; return false; }
		if (!contextValid || now - m_iArmAt > RICON_CapturePulseGate.LOCAL_START_SLOP_MS)
		{
			m_bInvalid = true; m_bFinished = true; m_sReason = "PRECALL_CONTEXT_OR_LATE"; return false;
		}
		m_bStartUsed = true; m_bInsideNative = true; m_iStartAt = now;
		return true;
	}
	void StartReturned(int now, bool accepted)
	{
		if (!m_bStartUsed || !m_bInsideNative || m_bStopUsed) return;
		m_bInsideNative = false; m_bStartAccepted = accepted;
		m_iReturnAt = now;
		Clock(now);
		if (!accepted) { m_bStopPending = true; m_sReason = "START_REQUEST_REJECTED"; }
		if (now - m_iArmAt >= RICON_CapturePulseGate.CUTOFF_MS) { m_bInvalid = true; m_bStopPending = true; m_sReason = "LOCAL_WINDOW_OVERRUN"; }
	}
	void RequestStop(string reason)
	{
		if (m_bFinished) return;
		m_bStopPending = true;
		if (m_sReason.IsEmpty()) m_sReason = reason;
	}
	bool NeedsStop(int now, bool contextValid)
	{
		if (!m_bStartUsed || m_bFinished || m_bStopUsed) return false;
		Clock(now);
		if (!contextValid) { m_bInvalid = true; m_bStopPending = true; m_sReason = "CONTEXT_CHANGED"; }
		if (now - m_iArmAt >= RICON_CapturePulseGate.CUTOFF_MS) { m_bInvalid = true; m_bStopPending = true; m_sReason = "LOCAL_WINDOW_OVERRUN"; }
		if (now - m_iStartAt >= m_iIntendedMs) RequestStop("INTENDED_STOP");
		return m_bStopPending && !m_bInsideNative;
	}
	bool BeginStop(int now, bool capturedBindingValid)
	{
		if (!m_bStartUsed || m_bStopUsed || m_bFinished) return false;
		RequestStop("CLEANUP");
		if (m_bInsideNative) return false;
		Clock(now);
		if (!capturedBindingValid)
		{
			m_bInvalid = true; m_bFinished = true; m_sReason = "STALE_BINDING_TEARDOWN_ONLY"; return false;
		}
		m_bStopUsed = true; m_bInsideNative = true;
		return true;
	}
	void StopReturned(int now, bool accepted)
	{
		if (!m_bStopUsed || !m_bInsideNative) return;
		m_iReturnAt = now;
		m_bInsideNative = false; m_bStopAccepted = accepted; Clock(now);
		if (now - m_iArmAt >= RICON_CapturePulseGate.CUTOFF_MS) { m_bInvalid = true; m_sReason = "LOCAL_WINDOW_OVERRUN"; }
		m_bFinished = true;
	}
	string Result(bool rehearsal)
	{
		if (m_bInvalid) return "INVALID";
		if (!m_bFinished) return "IN_PROGRESS";
		if (!m_bStartUsed) return "NOT_REQUESTED";
		if (rehearsal) return "REHEARSAL_ONLY";
		if (!m_bStartAccepted) return "START_REQUEST_REJECTED";
		if (!m_bStopAccepted) return "STOP_REQUEST_REJECTED";
		return "CAPTURE_REQUEST_CYCLE_OBSERVED";
	}
}

//! Private in-memory source identity. Encryption strings are never formatted or persisted.
class RICON_CapturePulseSource
{
	PlayerController m_Controller;
	IEntity m_Entity;
	BaseWorld m_World;
	SCR_VONController m_Control;
	SCR_VoNComponent m_Von;
	SCR_VONEntryRadio m_Entry;
	SCR_GadgetComponent m_Gadget;
	BaseRadioComponent m_Radio;
	BaseTransceiver m_Transmit;
	SCR_Faction m_Faction;
	string m_sPrivateKey;
	string m_sPrivateEffectiveLocalKey;
	string m_sEntityRpl;
	int m_iNativeIndex = -1;
	int m_iFrequency;
	ECommMethod m_PreviousMethod;
	BaseTransceiver m_PreviousTransmit;
	bool m_bCrossFaction;
	bool m_bAdmin;
	ref RICON_SeparatedPairGeometry m_SeparatedGeometry;
	void ClearPrivateKeys()
	{
		m_sPrivateKey = "";
		m_sPrivateEffectiveLocalKey = "";
	}
	bool SameCore(RICON_CapturePulseSource other)
	{
		if (!other || m_Controller != other.m_Controller || m_Entity != other.m_Entity || m_World != other.m_World) return false;
		if (m_Control != other.m_Control || m_Von != other.m_Von || m_Entry != other.m_Entry) return false;
		if (m_Gadget != other.m_Gadget || m_Radio != other.m_Radio || m_Transmit != other.m_Transmit) return false;
		if (m_Faction != other.m_Faction || m_sPrivateKey != other.m_sPrivateKey || m_sEntityRpl != other.m_sEntityRpl) return false;
		if (m_sPrivateEffectiveLocalKey != other.m_sPrivateEffectiveLocalKey) return false;
		if ((m_SeparatedGeometry != null) != (other.m_SeparatedGeometry != null)) return false;
		if (m_SeparatedGeometry && !m_SeparatedGeometry.SameIdentity(other.m_SeparatedGeometry)) return false;
		return m_iNativeIndex == other.m_iNativeIndex && m_iFrequency == other.m_iFrequency
			&& m_bCrossFaction == other.m_bCrossFaction && m_bAdmin == other.m_bAdmin;
	}
}

class RICON_LiveCapturePulseProbe
{
	protected ref RICON_CapturePulseState m_State;
	protected ref RICON_CapturePulseSource m_Source;
	protected RICON_LiveNativeRxProbe m_Nrx;
	protected BaseWorld m_World;
	protected RICON_ECapturePulseMode m_Mode;
	protected string m_sMode;
	protected string m_sRun;
	protected string m_sAttempt;
	protected bool m_bReady;
	protected bool m_bFinished;
	protected bool m_bInsideRouting;
	protected bool m_bPreparingStop;
	protected bool m_bCancelPending;
	protected bool m_bTrueInvoked;
	protected bool m_bTerminalWritten;
	protected bool m_bLogOverflow;
	protected int m_iLogs;
	protected int m_iStarted;
	protected int m_iPreparedGeneration;
	protected int m_iArmedGeneration;
	protected string m_sWaitReason;
	protected bool m_bRestArmUsed;
	protected bool m_bRestCaptureArmUsed;
	protected bool m_bNoSend;
	protected bool m_bWrongFrequency;
	protected bool m_bBPowerOff;
	protected bool m_bDirect;
	protected bool m_bShort400;
	protected bool m_bBlindTrial;
	protected bool m_bBlindChoiceUsed;
	protected bool m_bInsideBlindArm;
	protected int m_iBlindChoice = -1;

	protected int Now() { return System.GetTickCount(); }
	protected static string Bit(bool value)
	{
		if (value)
			return "1";
		return "0";
	}
	protected static string EntityRpl(IEntity entity)
	{
		if (!entity) return "UNKNOWN";
		RplId id = Replication.FindItemId(entity);
		if (id == RplId.Invalid() || Replication.FindItem(id) != entity) return "UNKNOWN";
		return string.Format("%1", id);
	}
	protected void Emit(string kind, string text)
	{
		if (m_bDirect)
		{
			if (m_bBPowerOff) text = "control=DIRECT_B_POWER_OFF," + text;
			else text = "control=DIRECT," + text;
			if (m_Source && m_Source.m_Von)
			{
				text = text + ",actualMethodDIRECT=" + Bit(m_Source.m_Von.GetCommMethod() == ECommMethod.DIRECT);
				text = text + ",actualTxNull=" + Bit(m_Source.m_Von.GetTransmitRadio() == null);
			}
		}
		if (m_bBlindTrial)
		{
			string blindTiming = "blindTrial=1,choiceState=UNSELECTED,plannedIntendedMs=UNSELECTED";
			if (m_bBlindChoiceUsed)
			{
				int intended = RICON_CapturePulseGate.INTENDED_MS;
				if (m_bShort400) intended = 400;
				blindTiming = "blindTrial=1,choiceState=SELECTED,blindChoice=" + m_iBlindChoice.ToString();
				blindTiming = blindTiming + ",plannedIntendedMs=" + intended.ToString();
				if (m_State && m_State.StartUsed())
				{
					int plannedStop = m_State.StartRequestTick() + intended;
					blindTiming = blindTiming + ",plannedStopTickMs=" + plannedStop.ToString();
					if (kind == "START_REQUEST_RETURN" || kind == "STOP_REQUEST_RETURN")
						blindTiming = blindTiming + ",returnTickMs=" + m_State.LastReturnTick().ToString();
				}
			}
			text = blindTiming + "," + text;
		}
		else if (m_bShort400)
		{
			string timing = "short400=1,plannedIntendedMs=400";
			if (m_State && m_State.StartUsed())
			{
				int plannedStopAt = m_State.StartRequestTick() + 400;
				timing = timing + ",plannedStopTickMs=" + plannedStopAt.ToString();
				if (kind == "START_REQUEST_RETURN" || kind == "STOP_REQUEST_RETURN")
					timing = timing + ",returnTickMs=" + m_State.LastReturnTick().ToString();
			}
			text = timing + "," + text;
		}
		if (m_bBPowerOff && !m_bDirect) text = "control=B_POWER_OFF," + text;
		else if (m_bNoSend) text = "control=NO_SEND," + text;
		else if (m_bWrongFrequency) text = "control=WRONG_FREQUENCY," + text;
		if (m_iLogs >= 64 && kind != "TERMINAL" && kind != "GAP")
		{
			m_bLogOverflow = true; m_bReady = false;
			if (m_State) m_State.RejectBeforeRequest("LOG_OVERFLOW");
			return;
		}
		string line = "[RICON_PULSE] seq=" + m_iLogs.ToString() + " t=" + Now().ToString();
		m_iLogs++;
		line = line + " run=" + RICON_LiveText.Esc(m_sRun) + " attempt=" + RICON_LiveText.Esc(m_sAttempt);
		line = line + " mode=" + m_sMode + " kind=" + kind;
		line = line + " guardEvidenceScope=EXTERNAL_COORDINATOR_RECEIPT guardVerifiedByGame=0 " + text;
		PrintFormat("%1", line);
	}
	bool Start(BaseWorld world, RICON_LiveNativeRxProbe observer)
	{
		string mode;
		bool configured = System.GetCLIParam("riconPulseMode", mode);
		m_Mode = RICON_CapturePulseGate.Mode(configured, mode);
		if (m_Mode == RICON_ECapturePulseMode.OFF || !world || world.IsEditMode() || !observer) return false;
		if (!RICON_CaptureControlGate.Valid()) return false;
		m_bNoSend = RICON_CaptureControlGate.NoSend();
		m_bWrongFrequency = RICON_CaptureControlGate.WrongFrequency();
		if (RICON_DirectCaptureGate.Configured())
		{
			if (!RICON_DirectCaptureGate.Requested() || m_Mode != RICON_ECapturePulseMode.CAPTURE || !RICON_RestCaptureGate.Requested()) return false;
			m_bDirect = true;
		}
		if (RICON_CaptureBPowerOffGate.Configured())
		{
			if (!RICON_CaptureBPowerOffGate.Requested() || m_Mode != RICON_ECapturePulseMode.CAPTURE || !RICON_RestCaptureGate.Requested()) return false;
			m_bBPowerOff = true;
		}
		if (m_bNoSend && (m_Mode != RICON_ECapturePulseMode.CAPTURE || !RICON_RestCaptureGate.Requested())) return false;
		if (m_bWrongFrequency && (m_Mode != RICON_ECapturePulseMode.CAPTURE || !RICON_RestCaptureGate.Requested())) return false;
		string shortValue;
		bool shortConfigured = System.GetCLIParam("riconCaptureShort400", shortValue);
		string blindValue;
		bool blindConfigured = System.GetCLIParam("riconCaptureBlindTrial", blindValue);
		bool otherControl = m_bNoSend || m_bWrongFrequency || blindConfigured;
		if (!RICON_CaptureShort400Gate.Config(shortConfigured, shortValue, Replication.IsServer(), RICON_CaptureSeparatedGate.Requested(),
			m_Mode == RICON_ECapturePulseMode.CAPTURE, RICON_RestCaptureGate.Requested(), otherControl)) return false;
		m_bShort400 = shortConfigured;
		if (!RICON_CaptureBlindGate.Config(blindConfigured, blindValue, Replication.IsServer(), RICON_CaptureSeparatedGate.Requested(),
			m_Mode == RICON_ECapturePulseMode.CAPTURE && RICON_RestCaptureGate.Requested(), shortConfigured || m_bNoSend || m_bWrongFrequency)) return false;
		m_bBlindTrial = blindConfigured;
		string run; string attempt;
		if (!System.GetCLIParam("riconNativeRxRun", run) || !RICON_D0Gate.ValidRunId(run)) return false;
		if (!System.GetCLIParam("riconPulseAttempt", attempt) || !RICON_D0Gate.ValidRunId(attempt)) return false;
		m_sRun = run; m_sAttempt = attempt; m_sMode = mode; m_World = world; m_Nrx = observer;
		m_State = new RICON_CapturePulseState(); m_iStarted = Now();
		if (m_bShort400 && !m_State.UseShort400()) return false;
		if (m_iStarted < 0) return false;
		if (m_bBlindTrial) Emit("WAITING", "autoArm=0,intendedMs=UNSELECTED,externalCutoffMs=1500,validationMs=2000,nativeInitialState=UNKNOWN,content=UNQUALIFIED");
		else if (m_bShort400) Emit("WAITING", "autoArm=0,intendedMs=400,externalCutoffMs=1500,validationMs=2000,nativeInitialState=UNKNOWN,content=UNQUALIFIED");
		else Emit("WAITING", "autoArm=0,intendedMs=1000,externalCutoffMs=1500,validationMs=2000,nativeInitialState=UNKNOWN,content=UNQUALIFIED");
		return true;
	}
	protected bool ResolveSource(int now, bool requirePulseBudget, out RICON_CapturePulseSource source, out string reason)
	{
		source = null; reason = "OBSERVER_NOT_READY";
		int remaining;
		if (!m_Nrx || !m_Nrx.RICON_PulseHealth(now, m_sRun, 1, remaining)) return false;
		if (requirePulseBudget && remaining < RICON_CapturePulseGate.VALIDATION_MS) return false;
		if (m_bBPowerOff && m_State.ArmUsed() && !RICON_LiveCaptureBPowerOff.ReadyForSource(m_sRun, m_World))
		{ reason = "B_POWER_OFF_NOT_CURRENT"; return false; }
		RICON_SeparatedPairGeometry separated;
		if (RICON_CaptureSeparatedGate.Configured())
		{
			if (!RICON_CaptureSeparatedGate.Requested() || !m_Nrx.RICON_ReadSeparatedGeometry(now, m_sRun, m_World, separated) || !separated.Eligible)
			{ reason = "LIVE_SEPARATED_GEOMETRY_NOT_READY"; return false; }
		}
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || controller.GetPlayerId() != 1) { reason = "NOT_LOCAL_A"; return false; }
		IEntity entity = controller.GetControlledEntity();
		if (!entity || entity.GetWorld() != m_World || GetGame().GetWorld() != m_World || m_World.IsEditMode())
		{ reason = "WORLD_OR_ENTITY"; return false; }
		string entityRpl = EntityRpl(entity);
		if (entityRpl == "UNKNOWN") { reason = "ENTITY_RPL_UNKNOWN"; return false; }
		SCR_VONController control = SCR_VONController.Cast(controller.FindComponent(SCR_VONController));
		SCR_VoNComponent von = SCR_VoNComponent.Cast(entity.FindComponent(SCR_VoNComponent));
		if (!control || !von || control.GetVONComponent() != von || control.IsVONDisabled()) { reason = "VON_COMPONENT"; return false; }
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character || !character.GetCharacterController() || character.GetCharacterController().GetLifeState() != ECharacterLifeState.ALIVE)
		{ reason = "NOT_ALIVE"; return false; }
		if (control.RICON_I0ScriptActive() || control.RICON_I0ToggledDirect() || !control.RICON_I0ActiveHoldAction().IsEmpty())
		{ reason = "SCRIPT_WRITER_OR_DIRECT"; return false; }
		if (m_bDirect)
		{
			RICON_SeparatedPairGeometry near;
			if (!m_Nrx.RICON_ReadDirectNearGeometry(now, m_sRun, m_World, near) || !near.Eligible)
			{ reason = "DIRECT_NEAR_NOT_CURRENT"; return false; }
			source = new RICON_CapturePulseSource();
			source.m_Controller = controller; source.m_Entity = entity; source.m_World = m_World; source.m_Control = control;
			source.m_Von = von; source.m_sEntityRpl = entityRpl; source.m_iFrequency = -1;
			source.m_PreviousMethod = von.GetCommMethod(); source.m_PreviousTransmit = von.GetTransmitRadio();
			source.m_SeparatedGeometry = near;
			if (!RICON_CaptureSeparatedGate.Fresh(near.ObservedAt, Now())) { source = null; reason = "DIRECT_NEAR_STALE"; return false; }
			reason = "READY_DIRECT";
			return true;
		}
		SCR_VONEntryRadio entry = SCR_VONEntryRadio.Cast(control.GetActiveEntry());
		if (!entry || !entry.IsActive() || !entry.IsUsable()) { reason = "ACTIVE_RADIO_ENTRY"; return false; }
		BaseTransceiver transmitter = entry.GetTransceiver(); SCR_GadgetComponent gadget = entry.GetGadget();
		SCR_GadgetManagerComponent manager = SCR_GadgetManagerComponent.GetGadgetManager(entity);
		if (!transmitter || !gadget || !gadget.GetOwner() || gadget.GetOwner().GetWorld() != m_World || !manager || !manager.IsGadgetOwned(gadget))
		{ reason = "OWNED_RADIO"; return false; }
		BaseRadioComponent radio = BaseRadioComponent.Cast(gadget.GetOwner().FindComponent(BaseRadioComponent));
		if (!radio || transmitter.GetRadio() != radio || !radio.IsPowered() || transmitter.IsMuted())
		{ reason = "RADIO_POWER_MUTE"; return false; }
		int count = radio.TransceiversCount(); int index = -1;
		if (count < 1 || count > 4) { reason = "RADIO_CATALOG"; return false; }
		for (int i = 0; i < count; i++) if (radio.GetTransceiver(i) == transmitter) index = i;
		if (index < 0 || entry.GetTransceiverNumber() != index + 1) { reason = "NATIVE_TRANSCEIVER_BINDING"; return false; }
		bool crossFaction = GetGame().GetVONCanTransmitCrossFaction(); bool admin = SCR_Global.IsAdmin();
		string key = radio.GetEncryptionKey(); string cachedKey = control.RICON_I0CachedLocalEncryptionKey();
		SCR_FactionManager factions = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		SCR_Faction faction;
		if (factions) faction = SCR_Faction.Cast(factions.GetLocalPlayerFaction());
		string factionKey;
		if (faction) factionKey = faction.GetFactionRadioEncryptionKey();
		string effectiveLocalKey = RICON_CapturePulseGate.EffectiveLocalKey(cachedKey, faction != null, factionKey);
		if (RICON_CapturePulseGate.FactionPolicy(crossFaction, admin, key, effectiveLocalKey) == "LOCAL_KEY_MISMATCH")
		{ reason = "LOCAL_FACTION_KEY_ELIGIBILITY"; return false; }
		source = new RICON_CapturePulseSource();
		source.m_Controller = controller; source.m_Entity = entity; source.m_World = m_World; source.m_Control = control;
		source.m_Von = von; source.m_Entry = entry; source.m_Gadget = gadget; source.m_Radio = radio;
		source.m_Transmit = transmitter; source.m_Faction = faction; source.m_sPrivateKey = key;
		source.m_sPrivateEffectiveLocalKey = effectiveLocalKey;
		source.m_sEntityRpl = entityRpl;
		source.m_iNativeIndex = index; source.m_iFrequency = transmitter.GetFrequency();
		source.m_PreviousMethod = von.GetCommMethod(); source.m_PreviousTransmit = von.GetTransmitRadio();
		source.m_bCrossFaction = crossFaction; source.m_bAdmin = admin;
		if (separated && !RICON_CaptureSeparatedGate.Fresh(separated.ObservedAt, Now()))
		{ source.ClearPrivateKeys(); source = null; reason = "LIVE_SEPARATED_GEOMETRY_STALE"; return false; }
		source.m_SeparatedGeometry = separated;
		reason = "READY";
		return true;
	}
	protected bool SourceAtRoute(int now, ECommMethod expectedMethod, BaseTransceiver expectedTransmit, bool requirePulseBudget = true)
	{
		RICON_CapturePulseSource current; string reason;
		if (!m_Source || !ResolveSource(now, requirePulseBudget, current, reason) || !m_Source.SameCore(current)) return false;
		return current.m_PreviousMethod == expectedMethod && current.m_PreviousTransmit == expectedTransmit;
	}
	protected bool SourceCurrent(int now, bool selectedRouting, bool requirePulseBudget = true)
	{
		if (!m_Source) return false;
		if (m_bDirect && selectedRouting) return SourceAtRoute(now, ECommMethod.DIRECT, null, requirePulseBudget);
		if (selectedRouting) return SourceAtRoute(now, ECommMethod.SQUAD_RADIO, m_Source.m_Transmit, requirePulseBudget);
		return SourceAtRoute(now, m_Source.m_PreviousMethod, m_Source.m_PreviousTransmit, requirePulseBudget);
	}
	//! Re-read the full source and exact expected route, then take a fresh clock at each mutation boundary.
	protected bool RoutingBoundary(ECommMethod expectedMethod, BaseTransceiver expectedTransmit)
	{
		bool current = SourceAtRoute(Now(), expectedMethod, expectedTransmit);
		return !m_bCancelPending && m_State.CanBegin(Now(), current);
	}
	//! Cleanup ownership is narrower than eligibility: a key/power/observer failure must not suppress a safe stop.
	protected bool CapturedBindingCurrent(bool selectedRouting)
	{
		if (!m_Source || !m_World || m_World.IsEditMode() || GetGame().GetWorld() != m_World) return false;
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || controller != m_Source.m_Controller || controller.GetPlayerId() != 1) return false;
		IEntity entity = controller.GetControlledEntity();
		if (!entity || entity != m_Source.m_Entity || entity.GetWorld() != m_World || EntityRpl(entity) != m_Source.m_sEntityRpl) return false;
		if (!m_Source.m_Von || entity.FindComponent(SCR_VoNComponent) != m_Source.m_Von || !m_Source.m_Control) return false;
		if (controller.FindComponent(SCR_VONController) != m_Source.m_Control || m_Source.m_Control.GetVONComponent() != m_Source.m_Von) return false;
		if (m_bDirect && selectedRouting)
		{
			if (m_Source.m_Control.RICON_I0ScriptActive() || m_Source.m_Control.RICON_I0ToggledDirect()
				|| !m_Source.m_Control.RICON_I0ActiveHoldAction().IsEmpty()) return false;
			return RICON_DirectCaptureGate.Route(m_Source.m_Von.GetCommMethod() == ECommMethod.DIRECT, m_Source.m_Von.GetTransmitRadio() == null);
		}
		if (!m_Source.m_Entry || m_Source.m_Control.GetActiveEntry() != m_Source.m_Entry || !m_Source.m_Transmit) return false;
		if (m_Source.m_Control.RICON_I0ScriptActive() || m_Source.m_Control.RICON_I0ToggledDirect()
			|| !m_Source.m_Control.RICON_I0ActiveHoldAction().IsEmpty()) return false;
		if (selectedRouting) return m_Source.m_Von.GetCommMethod() == ECommMethod.SQUAD_RADIO
			&& m_Source.m_Von.GetTransmitRadio() == m_Source.m_Transmit;
		return m_Source.m_Von.GetCommMethod() == m_Source.m_PreviousMethod
			&& m_Source.m_Von.GetTransmitRadio() == m_Source.m_PreviousTransmit;
	}
	//! Только readonly readiness для explicit REST rehearsal; native guard ACK остаётся внешним.
	int RestRehearsalPreparedGeneration(string run, string attempt, BaseWorld expectedWorld)
	{
		if (!RICON_RestRehearsalGate.Requested() || m_Mode != RICON_ECapturePulseMode.REHEARSAL || m_sMode != "rehearsal") return 0;
		if (m_bFinished || !m_bReady || !m_State || m_State.ArmUsed() || m_iPreparedGeneration < 1) return 0;
		if (run != m_sRun || attempt != m_sAttempt || !expectedWorld || expectedWorld != m_World) return 0;
		if (!SourceCurrent(Now(), false)) return 0;
		return m_iPreparedGeneration;
	}
	bool RequestRestRehearsalArm(string run, string attempt, int expectedPreparedGeneration, BaseWorld expectedWorld)
	{
		if (m_bRestArmUsed) return false;
		m_bRestArmUsed = true;
		int actual = RestRehearsalPreparedGeneration(run, attempt, expectedWorld);
		if (!RICON_RestRehearsalGate.PreparedMatches(expectedPreparedGeneration, actual)) return false;
		return RequestArm(run, attempt, "rehearsal");
	}
	//! Source-local A readiness; B/RR/external guard admission остаются у parent.
	int RestCapturePreparedGeneration(string run, string attempt, BaseWorld expectedWorld)
	{
		if (!RICON_RestCaptureGate.Requested() || m_Mode != RICON_ECapturePulseMode.CAPTURE || m_sMode != "capture") return 0;
		if (m_bFinished || !m_bReady || !m_State || m_State.ArmUsed() || m_iPreparedGeneration < 1) return 0;
		if (m_bCancelPending || m_bLogOverflow || m_State.Finished() || m_State.Invalid()) return 0;
		if (run != m_sRun || attempt != m_sAttempt || !expectedWorld || expectedWorld != m_World) return 0;
		if (!SourceCurrent(Now(), false)) return 0;
		return m_iPreparedGeneration;
	}
	bool RequestRestCaptureArm(string run, string attempt, int expectedPreparedGeneration, BaseWorld expectedWorld)
	{
		if (m_bRestCaptureArmUsed) return false;
		m_bRestCaptureArmUsed = true;
		if (m_bBlindTrial) return false;
		int actual = RestCapturePreparedGeneration(run, attempt, expectedWorld);
		if (!RICON_RestRehearsalGate.PreparedMatches(expectedPreparedGeneration, actual)) return false;
		if (m_bWrongFrequency && !RICON_CaptureFrequencyControl.Ready()) return false;
		return RequestArm(run, attempt, "capture");
	}
	//! Поздний выбор только после fresh source/N/world. Ни одного native вызова в этом ingress.
	protected bool ApplyBlindChoice(int choice)
	{
		if (!m_bBlindTrial || m_bBlindChoiceUsed || !m_State || m_State.ArmUsed() || m_State.Finished() || m_State.Invalid()) return false;
		if (choice != 0 && choice != 1) return false;
		if (choice == 0 && !m_State.UseShort400()) return false;
		m_bShort400 = choice == 0; m_iBlindChoice = choice; m_bBlindChoiceUsed = true;
		return true;
	}
	bool RequestRestBlindCaptureArm(string run, string attempt, int expectedPreparedGeneration, BaseWorld expectedWorld, int choice)
	{
		if (m_bRestCaptureArmUsed) return false;
		m_bRestCaptureArmUsed = true;
		if (!m_bBlindTrial) return false;
		int actual = RestCapturePreparedGeneration(run, attempt, expectedWorld);
		if (!RICON_RestRehearsalGate.PreparedMatches(expectedPreparedGeneration, actual)) return false;
		if (!ApplyBlindChoice(choice)) return false;
		m_bInsideBlindArm = true;
		bool accepted = RequestArm(run, attempt, "capture");
		m_bInsideBlindArm = false;
		return accepted;
	}
	//! Explicit UI caller сохранён; REST использует отдельные strict one-use wrappers.
	bool RequestArm(string run, string attempt, string mode)
	{
		if (m_bBPowerOff && !RICON_LiveCaptureBPowerOff.ReadyForSource(m_sRun, m_World)) return false;
		if (m_bBlindTrial && !m_bInsideBlindArm) return false;
		if (m_bFinished || !m_bReady || !m_State || m_State.ArmUsed()) return false;
		if (run != m_sRun || attempt != m_sAttempt || mode != m_sMode) return false;
		if (!m_State.Arm(Now())) return false;
		m_iArmedGeneration = m_iPreparedGeneration;
		Emit("ARM_REQUEST", "oneUse=1,externalAckIsNotValidatedByGame=1,preparedGeneration=" + m_iArmedGeneration.ToString());
		return true;
	}
	protected bool SelectRouting()
	{
		if (!m_Source || !m_Source.m_Von) return false;
		if (m_bDirect)
		{
			m_bInsideRouting = true;
			if (m_Source.m_PreviousMethod != ECommMethod.DIRECT)
			{
				if (!RoutingBoundary(m_Source.m_PreviousMethod, m_Source.m_PreviousTransmit)) { m_bInsideRouting = false; return false; }
				m_Source.m_Von.SetCommMethod(ECommMethod.DIRECT);
			}
			if (m_bCancelPending) { m_bInsideRouting = false; return false; }
			if (m_Source.m_PreviousTransmit != null)
			{
				if (!RoutingBoundary(ECommMethod.DIRECT, m_Source.m_PreviousTransmit)) { m_bInsideRouting = false; return false; }
				m_Source.m_Von.SetTransmitRadio(null);
			}
			m_bInsideRouting = false;
			if (m_bCancelPending || !SourceCurrent(Now(), true)) return false;
			Emit("DIRECT_ROUTE_READBACK", "plannedMethodDIRECT=1,plannedTxNull=1");
			return SourceCurrent(Now(), true);
		}
		if (m_Mode == RICON_ECapturePulseMode.REHEARSAL)
		{
			if (m_Source.m_PreviousMethod != ECommMethod.SQUAD_RADIO)
			{
				if (!RoutingBoundary(m_Source.m_PreviousMethod, m_Source.m_PreviousTransmit)) return false;
				Emit("WouldSetCommMethod", "value=SQUAD_RADIO,actualSetterCalls=0");
			}
			if (m_Source.m_PreviousTransmit != m_Source.m_Transmit)
			{
				if (!RoutingBoundary(m_Source.m_PreviousMethod, m_Source.m_PreviousTransmit)) return false;
				Emit("WouldSetTransmitRadio", "actualSetterCalls=0,actualRoute=PRIOR_UNCHANGED");
			}
			return !m_bCancelPending && SourceCurrent(Now(), false);
		}
		if (m_Mode != RICON_ECapturePulseMode.CAPTURE) return false;
		m_bInsideRouting = true;
		if (m_Source.m_PreviousMethod != ECommMethod.SQUAD_RADIO)
		{
			if (!RoutingBoundary(m_Source.m_PreviousMethod, m_Source.m_PreviousTransmit)) { m_bInsideRouting = false; return false; }
			m_Source.m_Von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		}
		if (m_bCancelPending || !m_Source || !m_Source.m_Von) { m_bInsideRouting = false; return false; }
		if (m_Source.m_PreviousTransmit != m_Source.m_Transmit)
		{
			if (!RoutingBoundary(ECommMethod.SQUAD_RADIO, m_Source.m_PreviousTransmit)) { m_bInsideRouting = false; return false; }
			m_Source.m_Von.SetTransmitRadio(m_Source.m_Transmit);
		}
		m_bInsideRouting = false;
		return !m_bCancelPending && SourceCurrent(Now(), true);
	}
	protected void BeginPulse()
	{
		if (m_bWrongFrequency && !RICON_CaptureFrequencyControl.Ready())
		{ m_State.RejectBeforeRequest("WRONG_FREQUENCY_NOT_READY"); Finish(); return; }
		int now = Now();
		if (m_iLogs > 54) { m_bLogOverflow = true; m_State.RejectBeforeRequest("LOG_BUDGET"); Finish(); return; }
		bool context = SourceCurrent(now, false);
		if (m_iPreparedGeneration != m_iArmedGeneration || !m_State.CanBegin(Now(), context))
		{ m_State.RejectBeforeRequest("PRECALL_BINDING_OR_WRITER"); Finish(); return; }
		Emit("PRIOR_ROUTE", "method=" + m_Source.m_PreviousMethod.ToString() + ",transmit=" + string.Format("%1", m_Source.m_PreviousTransmit));
		if (!m_State.CanBegin(Now(), true)) { m_State.RejectBeforeRequest("PRE_ROUTING_LATE"); Finish(); return; }
		if (!SelectRouting()) { m_State.RejectBeforeRequest("ROUTING_NOT_SELECTED"); Finish(); return; }
		now = Now();
		bool selected = m_Mode == RICON_ECapturePulseMode.CAPTURE;
		if (m_Mode == RICON_ECapturePulseMode.REHEARSAL) Emit("WouldRequestTrue", "actualCaptureCalls=0");
		else if (!m_bNoSend) Emit("START_INTENT", "armOneUseAlreadySet=1,microphoneContent=UNQUALIFIED");
		context = SourceCurrent(Now(), selected);
		int callAt = Now();
		if (m_bLogOverflow || !m_State.BeginRequest(callAt, context)) { Finish(); return; }
		bool accepted = true;
		if (RICON_CaptureControlGate.NativeAllowed(m_Mode == RICON_ECapturePulseMode.CAPTURE, m_bNoSend))
		{
			m_bTrueInvoked = true;
			accepted = m_Source.m_Von.SetCapture(true);
		}
		m_State.StartReturned(Now(), accepted);
		if (m_Mode == RICON_ECapturePulseMode.REHEARSAL) Emit("WouldReturnStart", "simulatedAccepted=1,actualCaptureCalls=0");
		else if (m_bNoSend) Emit("NO_SEND_START_SKIPPED", "nativeCalls=0,actualCaptureCalls=0,acceptedNative=NOT_REQUESTED,route=SQUAD_RADIO,exactTx=1,stateOnly=1");
		else Emit("START_REQUEST_RETURN", "accepted=" + Bit(accepted) + ",callStartTick=" + callAt.ToString() + ",requestOnly=1,nativeState=UNKNOWN");
		ServiceStop();
	}
	protected void ServiceStop()
	{
		if (!m_State || !m_State.StartUsed() || m_State.Finished()) { if (m_State && m_State.Finished()) Finish(); return; }
		int now = Now(); bool selected = m_Mode == RICON_ECapturePulseMode.CAPTURE;
		bool current = SourceCurrent(now, selected, false);
		now = Now();
		if (!m_State.NeedsStop(now, current) && !m_bCancelPending) return;
		m_bPreparingStop = true;
		if (m_Mode == RICON_ECapturePulseMode.REHEARSAL) Emit("WouldRequestFalse", "actualCaptureCalls=0");
		else if (!m_bNoSend) Emit("STOP_INTENT", "restorePriorRoute=0,capturedBindingRevalidatedAfterIntent=1");
		m_bPreparingStop = false;
		bool capturedOwned = CapturedBindingCurrent(selected);
		int callAt = Now();
		if (!m_State.BeginStop(callAt, capturedOwned)) { if (m_State.Finished()) Finish(); return; }
		bool accepted = true;
		if (RICON_CaptureControlGate.NativeAllowed(m_Mode == RICON_ECapturePulseMode.CAPTURE, m_bNoSend) && m_bTrueInvoked)
		{
			accepted = m_Source.m_Von.SetCapture(false);
		}
		m_State.StopReturned(Now(), accepted);
		if (m_Mode == RICON_ECapturePulseMode.REHEARSAL) Emit("WouldReturnStop", "simulatedAccepted=1,actualCaptureCalls=0");
		else if (m_bNoSend) Emit("NO_SEND_STOP_SKIPPED", "nativeCalls=0,actualCaptureCalls=0,acceptedNative=NOT_REQUESTED,restorePriorRoute=0,stateOnly=1");
		else Emit("STOP_REQUEST_RETURN", "accepted=" + Bit(accepted) + ",callStartTick=" + callAt.ToString() + ",nativeRelease=UNKNOWN,restorePriorRoute=0");
		Finish();
	}
	protected void Finish()
	{
		if (m_bTerminalWritten || m_bInsideRouting || m_bPreparingStop || (m_State && m_State.InsideNative())) return;
		m_bFinished = true; m_bTerminalWritten = true;
		string result = "NOT_REQUESTED"; string reason = "LIFECYCLE";
		if (m_State && (m_State.ArmUsed() || m_State.Invalid())) { result = m_State.Result(m_Mode == RICON_ECapturePulseMode.REHEARSAL); reason = m_State.Reason(); }
		result = RICON_CaptureControlGate.Result(m_bNoSend, result);
		if (m_bLogOverflow) Emit("GAP", "missingEvents=UNKNOWN,reason=LOG_OVERFLOW");
		string finalText = "result=" + result + ",reason=" + reason + ",nativeRelease=UNKNOWN";
		finalText = finalText + ",ownedAProcessTeardownRequired=" + Bit(m_State && m_State.ArmUsed()) + ",content=NOT_TESTED,audio=NOT_TESTED";
		if (m_bNoSend) finalText = finalText + ",nativeCalls=0,actualCaptureCalls=0";
		if (m_bWrongFrequency) RICON_CaptureFrequencyControl.RestoreOwned();
		Emit("TERMINAL", finalText);
		if (m_Source) m_Source.ClearPrivateKeys();
		m_Source = null; m_World = null; m_Nrx = null;
	}
	bool CanRelease() { return !m_bInsideRouting && !m_bPreparingStop && (!m_State || !m_State.InsideNative()); }
	void Cancel(string reason)
	{
		if (m_bFinished) return;
		m_bCancelPending = true;
		if (!m_State || !m_State.StartUsed()) { if (m_State && m_State.ArmUsed()) m_State.RejectBeforeRequest(reason); Finish(); return; }
		m_State.RequestStop(reason);
		if (CanRelease()) ServiceStop();
	}
	void Tick(BaseWorld world)
	{
		if (m_bFinished) return;
		if (m_bWrongFrequency && m_State && m_State.ArmUsed() && !RICON_CaptureFrequencyControl.Ready())
		{ Cancel("WRONG_FREQUENCY_DRIFT"); return; }
		int now = Now();
		if (m_bLogOverflow) { Cancel("LOG_OVERFLOW"); return; }
		if (!world || world != m_World || world.IsEditMode() || now < m_iStarted || now - m_iStarted >= RICON_NativeRxGate.LIMIT_MS)
		{ Cancel("WORLD_CLOCK_OR_LIFETIME"); return; }
		PlayerController controller = GetGame().GetPlayerController();
		if (controller && controller.GetPlayerId() == 2) { Cancel("B_ROLE_METADATA_ONLY"); return; }
		if (m_State.StartUsed()) { ServiceStop(); return; }
		if (m_State.ArmUsed()) { BeginPulse(); return; }
		RICON_CapturePulseSource source; string reason;
		if (!ResolveSource(now, true, source, reason))
		{
			m_bReady = false;
			if (reason != m_sWaitReason) { m_sWaitReason = reason; Emit("WAIT_ELIGIBILITY", "reason=" + reason); }
			return;
		}
		if (!m_bReady || !m_Source || !m_Source.SameCore(source) || m_Source.m_PreviousMethod != source.m_PreviousMethod
			|| m_Source.m_PreviousTransmit != source.m_PreviousTransmit)
		{
			if (m_Source) m_Source.ClearPrivateKeys();
			m_Source = source; m_bReady = true; m_iPreparedGeneration++;
			if (m_bDirect)
			{
				string directFields = "preparedGeneration=" + m_iPreparedGeneration.ToString() + ",directRequested=1,plannedMethodDIRECT=1";
				directFields = directFields + ",nativeIndex=-1,frequencyKHz=-1,radioEligibility=NOT_APPLICABLE_DIRECT";
				directFields = directFields + ",scriptIdleOnly=1,nativeCaptureIdle=UNKNOWN";
				Emit("PREPARED", directFields);
				return;
			}
			string factionText = "UNKNOWN"; string keySource = "CACHED_CONTROLLER";
			if (source.m_Faction) { factionText = source.m_Faction.GetFactionKey(); keySource = "LOCAL_FACTION"; }
			string policy = RICON_CapturePulseGate.FactionPolicy(source.m_bCrossFaction, source.m_bAdmin, source.m_sPrivateKey, source.m_sPrivateEffectiveLocalKey);
			string fields = "preparedGeneration=" + m_iPreparedGeneration.ToString() + ",faction=" + RICON_LiveText.Esc(factionText);
			fields = fields + ",factionKnown=" + Bit(source.m_Faction != null) + ",policy=" + policy + ",effectiveLocalKeySource=" + keySource;
			fields = fields + ",effectiveLocalKeyKnown=1,radioKeyKnown=1,effectiveLocalKeyEmpty=" + Bit(source.m_sPrivateEffectiveLocalKey.IsEmpty());
			fields = fields + ",effectiveLocalKeyMatch=" + Bit(source.m_sPrivateKey == source.m_sPrivateEffectiveLocalKey) + ",keyValuesLogged=0,nativeIndex=" + source.m_iNativeIndex.ToString();
			fields = fields + ",frequencyKHz=" + source.m_iFrequency.ToString() + ",crossFaction=" + Bit(source.m_bCrossFaction) + ",admin=" + Bit(source.m_bAdmin);
			fields = fields + ",scriptIdleOnly=1,nativeCaptureIdle=UNKNOWN";
			if (source.m_SeparatedGeometry)
			{
				fields = fields + ",geoActorARpl=" + source.m_SeparatedGeometry.ARpl + ",geoActorBRpl=" + source.m_SeparatedGeometry.BRpl;
				fields = fields + ",geoBinding=" + source.m_SeparatedGeometry.Binding.ToString();
			}
			Emit("PREPARED", fields);
		}
	}
}
