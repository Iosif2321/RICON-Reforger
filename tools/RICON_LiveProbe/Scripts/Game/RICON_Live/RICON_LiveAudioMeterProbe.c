//! TOOL_ONLY local B getter/event telemetry. OutputVolume units, stage and stimulus energy remain UNKNOWN.
enum RICON_EAudioMeterPhase
{
	WAIT_BINDING,
	BASELINE,
	EVENT_WINDOW,
	RECOVERY,
	TERMINAL
}

class RICON_AudioMeterGate
{
	static const int LIMIT_MS = 60000;
	static const int BIND_MS = 40000;
	static const int BASELINE_MS = 4000;
	static const int EVENT_MS = 10000;
	static const int RECOVERY_MS = 4000;
	static const int SAMPLE_MS = 50;
	static const int EVENT_LIMIT = 512;
	static bool Requested(bool configured, string value, bool conflict) { return configured && value == "1" && !conflict; }
	static bool RoleAllowed(int role) { return role == 2; }
	static bool ClockGap(int previous, int now) { return previous < 0 || now < previous || now - previous > 5000; }
	static bool Expired(int elapsed, int limit) { return elapsed < 0 || elapsed >= limit; }
	static bool SampleDue(int previous, int now) { return now >= 0 && (previous < 0 || now - previous >= SAMPLE_MS); }
	static bool OrdinaryAllowed(int count) { return count >= 0 && count < EVENT_LIMIT; }
	static bool CanStart(int phase, bool requested, int started, bool context, bool eventMatch, AudioHandle handle)
	{
		return phase == RICON_EAudioMeterPhase.EVENT_WINDOW && requested && started == 0 && context && eventMatch && handle != AudioHandle.Invalid;
	}
	static bool CanFinish(int phase, int started, int finished, bool context, bool eventMatch, bool sameHandle, bool terminated)
	{
		return phase == RICON_EAudioMeterPhase.EVENT_WINDOW && started == 1 && finished == 0 && context && eventMatch && sameHandle && !terminated;
	}
}

//! Только exact local opt-in разрешает own player1/standalone0; прежний B gate не ослабляется.
class RICON_AudioMeterLocalGate
{
	static bool Requested(bool meter, bool configured, string value)
	{
		return meter && configured && value == "1";
	}
	static bool RoleAllowed(bool local, int player, int rplMode, bool server, bool client)
	{
		if (!local) return RICON_AudioMeterGate.RoleAllowed(player);
		return player == 1 && rplMode == 0 && server && !client;
	}
	static bool RplAllowed(bool local, bool known, bool same)
	{
		if (local) return true;
		return known && same;
	}
	static bool RefsCurrent(bool world, bool controller, bool entity, bool communication, bool events)
	{
		return world && controller && entity && communication && events;
	}
}

//! Pure event state, also used by Game-module boundary checks. No native callback or playback is synthesized here.
class RICON_AudioMeterEventState
{
	bool m_bRequested;
	int m_iStarted;
	int m_iFinished;
	int m_iRequestedAt = -1;
	int m_iFinishedAt = -1;
	AudioHandle m_Handle = AudioHandle.Invalid;
	bool m_bInNativeCall;
	bool m_bOverflowSeen;
	string m_sDeferredStop;
	void MarkOverflow() { m_bOverflowSeen = true; }
	bool NeedsGap(string reason) { return m_bOverflowSeen || reason == "CLOCK_OR_STALL"; }
	bool DeferStop(string reason)
	{
		if (!m_bInNativeCall) return false;
		if (m_sDeferredStop.IsEmpty()) m_sDeferredStop = reason;
		return true;
	}
	bool Begin(int now)
	{
		if (m_bRequested || now < 0) return false;
		m_bRequested = true; m_iRequestedAt = now;
		return true;
	}
	bool Started(int phase, bool context, bool eventMatch, AudioHandle handle)
	{
		if (!RICON_AudioMeterGate.CanStart(phase, m_bRequested, m_iStarted, context, eventMatch, handle)) return false;
		m_Handle = handle; m_iStarted++;
		return true;
	}
	bool Finished(int phase, int now, bool context, bool eventMatch, AudioHandle handle, bool terminated)
	{
		if (now < m_iRequestedAt || !RICON_AudioMeterGate.CanFinish(phase, m_iStarted, m_iFinished, context, eventMatch, handle == m_Handle, terminated)) return false;
		m_iFinished++; m_iFinishedAt = now;
		return true;
	}
	bool Outstanding() { return m_bRequested && m_iFinished != 1; }
}

class RICON_LiveAudioMeterProbe
{
	protected ref RICON_LiveJournal m_Journal;
	protected ref RICON_AudioMeterEventState m_Event;
	protected ref array<float> m_MasterBaseline = {};
	protected BaseWorld m_World;
	protected PlayerController m_Controller;
	protected IEntity m_Entity;
	protected SCR_CommunicationSoundComponent m_Communication;
	protected OnSoundEventStartedInvoker m_Started;
	protected OnSoundEventFinishedInvoker m_Finished;
	protected string m_sRun;
	protected string m_sRpl;
	protected string m_sLastMasters = "NOT_READ";
	protected int m_iT0;
	protected int m_iLastClock;
	protected int m_iLastBindPoll;
	protected int m_iPhaseSince;
	protected int m_iLastSample = -1;
	protected int m_iSamples;
	protected int m_iCallbacks;
	protected int m_iNativeCalls;
	protected int m_iBindingGeneration;
	protected int m_iSubscriptionGeneration;
	protected RICON_EAudioMeterPhase m_Phase;
	protected bool m_bRunning;
	protected bool m_bInvalid;
	protected bool m_bFinishMastersPending;
	protected bool m_bLocalCalibration;
	protected int m_iBoundPlayer = -1;
	protected string m_sLocalWaitReason;

	static bool IsRequested()
	{
		string value; bool configured = System.GetCLIParam("riconAudioMeter", value);
		string other; bool conflict;
		if (System.GetCLIParam("riconCommunicationDiscovery", other) && other == "1") conflict = true;
		if (System.GetCLIParam("riconVoNInputShape", other) && other == "1") conflict = true;
		if (System.GetCLIParam("riconVoNMapping", other) && other == "1") conflict = true;
		if (System.GetCLIParam("riconNativeRx", other) && other == "1") conflict = true;
		return RICON_AudioMeterGate.Requested(configured, value, conflict);
	}
	protected static string Bit(bool value)
	{
		if (value) return "1";
		return "0";
	}
	protected static string Scalar(float value)
	{
		if (RICON_Num.IsFinite(value)) return value.ToString(-1, 9);
		return RICON_LiveText.Esc(string.Format("%1", value));
	}
	protected static string RplText(IEntity entity)
	{
		if (!entity) return "UNKNOWN";
		RplId id = Replication.FindItemId(entity);
		if (id == RplId.Invalid() || Replication.FindItem(id) != entity) return "UNKNOWN";
		return string.Format("%1", id);
	}
	protected static bool Alive(IEntity entity)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		return character && character.GetCharacterController() && character.GetCharacterController().GetLifeState() == ECharacterLifeState.ALIVE;
	}
	protected void Append(int now, string kind, string text)
	{
		if (m_bLocalCalibration) { AppendLocal(now, kind, text); return; }
		string fields = "run=" + RICON_LiveText.Esc(m_sRun) + ",phase=" + m_Phase.ToString();
		fields = fields + ",rawTickMs=" + now.ToString() + ",startTickMs=" + m_iT0.ToString();
		fields = fields + ",bindingGeneration=" + m_iBindingGeneration.ToString() + ",subscriptionGeneration=" + m_iSubscriptionGeneration.ToString() + "," + text;
		m_Journal.Add(now - m_iT0, "ownB", kind, 2, -1, -1, -1, fields);
		string line = RICON_LiveJournal.Format(m_Journal.At(m_Journal.Count() - 1));
		line.Replace("[RICON_LIVE]", "[RICON_METER]");
		PrintFormat("%1", line);
	}
	protected void AppendLocal(int now, string kind, string text)
	{
		PlayerController current = GetGame().GetPlayerController();
		int player = -1;
		if (current) player = current.GetPlayerId();
		int rplMode = RplSession.Mode();
		string fields = "run=" + RICON_LiveText.Esc(m_sRun) + ",scope=LOCAL_CALIBRATION,phase=" + m_Phase.ToString();
		fields = fields + ",rawTickMs=" + now.ToString() + ",startTickMs=" + m_iT0.ToString();
		fields = fields + ",bindingGeneration=" + m_iBindingGeneration.ToString() + ",subscriptionGeneration=" + m_iSubscriptionGeneration.ToString();
		fields = fields + ",localPlayer=" + player.ToString() + ",boundPlayer=" + m_iBoundPlayer.ToString();
		fields = fields + ",rplMode=" + rplMode.ToString() + ",isServer=" + Bit(Replication.IsServer());
		fields = fields + ",isClient=" + Bit(Replication.IsClient()) + ",rplProof=LOCAL_REFS," + text;
		m_Journal.Add(now - m_iT0, "LOCAL_CALIBRATION", kind, player, -1, -1, -1, fields);
		string line = RICON_LiveJournal.Format(m_Journal.At(m_Journal.Count() - 1));
		line.Replace("[RICON_LIVE]", "[RICON_METER]");
		PrintFormat("%1", line);
	}
	protected bool Emit(int now, string kind, string text)
	{
		if (!m_bRunning) return false;
		if (!RICON_AudioMeterGate.OrdinaryAllowed(m_Journal.Count()))
		{
			m_Event.MarkOverflow(); Abort("OVERFLOW"); return false;
		}
		Append(now, kind, text);
		return true;
	}
	protected void Abort(string reason)
	{
		m_bInvalid = true;
		Stop(reason);
	}
	protected bool ClockReady(int now)
	{
		if (RICON_AudioMeterGate.ClockGap(m_iLastClock, now)) { Abort("CLOCK_OR_STALL"); return false; }
		m_iLastClock = now;
		if (RICON_AudioMeterGate.Expired(now - m_iT0, RICON_AudioMeterGate.LIMIT_MS)) { Abort("LIMIT"); return false; }
		return true;
	}
	protected bool ContextCurrent()
	{
		if (m_bLocalCalibration) return LocalContextCurrent();
		PlayerController controller = GetGame().GetPlayerController();
		if (m_iBindingGeneration != 1 || m_iSubscriptionGeneration != 1) return false;
		if (!controller || controller != m_Controller) return false;
		if (!RICON_AudioMeterGate.RoleAllowed(controller.GetPlayerId())) return false;
		if (!m_Entity || controller.GetControlledEntity() != m_Entity) return false;
		if (m_Entity.GetWorld() != m_World || RplText(m_Entity) != m_sRpl) return false;
		if (!Alive(m_Entity) || !m_Communication) return false;
		if (m_Entity.FindComponent(SCR_CommunicationSoundComponent) != m_Communication) return false;
		if (!m_Communication.IsScriptedMethodsCallEnabled() || !m_Started || !m_Finished) return false;
		if (m_Communication.GetOnSoundEventStarted() != m_Started) return false;
		return m_Communication.GetOnSoundEventFinished() == m_Finished;
	}
	protected bool LocalContextCurrent()
	{
		if (m_iBindingGeneration != 1 || m_iSubscriptionGeneration != 1) return false;
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || !m_Entity || !m_Communication) return false;
		if (!RICON_AudioMeterLocalGate.RoleAllowed(true, controller.GetPlayerId(), RplSession.Mode(), Replication.IsServer(), Replication.IsClient())) return false;
		bool worldCurrent = m_World && GetGame().GetWorld() == m_World && !m_World.IsEditMode();
		bool controllerCurrent = controller == m_Controller;
		if (!worldCurrent || !controllerCurrent) return false;
		bool entityCurrent = controller.GetControlledEntity() == m_Entity && m_Entity.GetWorld() == m_World && Alive(m_Entity);
		if (!entityCurrent) return false;
		bool communicationCurrent = m_Entity.FindComponent(SCR_CommunicationSoundComponent) == m_Communication && m_Communication.IsScriptedMethodsCallEnabled();
		if (!communicationCurrent) return false;
		bool eventsCurrent = m_Started && m_Finished && m_Communication.GetOnSoundEventStarted() == m_Started;
		eventsCurrent = eventsCurrent && m_Communication.GetOnSoundEventFinished() == m_Finished;
		return RICON_AudioMeterLocalGate.RefsCurrent(worldCurrent, controllerCurrent, entityCurrent, communicationCurrent, eventsCurrent);
	}
	protected float Master(int index)
	{
		if (index == 0) return AudioSystem.GetMasterVolume(AudioSystem.SFX);
		if (index == 1) return AudioSystem.GetMasterVolume(AudioSystem.Music);
		if (index == 2) return AudioSystem.GetMasterVolume(AudioSystem.VoiceChat);
		if (index == 3) return AudioSystem.GetMasterVolume(AudioSystem.Dialog);
		return AudioSystem.GetMasterVolume(AudioSystem.UI);
	}
	protected bool ReadMasters(int now, string where)
	{
		if (!ContextCurrent()) { Abort("MASTERS_CONTEXT"); return false; }
		bool first = m_MasterBaseline.IsEmpty(); bool finite = true; bool same = true;
		string fields = "where=" + where + ",order=SFX_Music_VoiceChat_Dialog_UI,atomic=0";
		for (int i = 0; i < 5; i++)
		{
			float value = Master(i); bool valid = RICON_Num.IsFinite(value);
			fields = fields + ",master" + i.ToString() + "=" + Scalar(value) + ",finite" + i.ToString() + "=" + Bit(valid);
			finite = finite && valid;
			if (first) m_MasterBaseline.Insert(value);
			else if (m_MasterBaseline[i] != value) same = false;
		}
		m_sLastMasters = where;
		fields = fields + ",sameAsBaseline=" + Bit(same) + ",otherEffects=UNKNOWN,playback=NOT_TESTED";
		if (!Emit(now, "MASTER_READBACK", fields)) return false;
		if (!finite || !same) { Abort("MASTER_NONFINITE_OR_CHANGED"); return false; }
		return true;
	}
	bool Start(BaseWorld world)
	{
		if (m_bRunning || m_Journal || !IsRequested() || !world || world.IsEditMode()) return false;
		string run;
		if (!System.GetCLIParam("riconAudioMeterRun", run) || !RICON_D0Gate.ValidRunId(run)) return false;
		string localValue;
		bool localConfigured = System.GetCLIParam("riconAudioMeterLocal", localValue);
		m_bLocalCalibration = RICON_AudioMeterLocalGate.Requested(true, localConfigured, localValue);
		int now = System.GetTickCount(); if (now < 0) return false;
		m_sRun = run; m_World = world; m_iT0 = now; m_iLastClock = now; m_iLastBindPoll = now - 500;
		m_Phase = RICON_EAudioMeterPhase.WAIT_BINDING; m_iPhaseSince = now;
		m_Event = new RICON_AudioMeterEventState();
		m_Journal = new RICON_LiveJournal(516); m_Journal.SetStream(false);
		m_bRunning = true;
		if (m_bLocalCalibration) return Emit(now, "START", "mode=LOCAL_CALIBRATION,units=UNKNOWN,stage=UNKNOWN,energy=UNKNOWN,playback=NOT_TESTED");
		return Emit(now, "START", "mode=LOCAL_B,units=UNKNOWN,stage=UNKNOWN,energy=UNKNOWN,playback=NOT_TESTED");
	}
	protected void Bind(int now)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || controller.GetPlayerId() <= 0) return;
		if (m_bLocalCalibration)
		{
			if (!RICON_AudioMeterLocalGate.RoleAllowed(true, controller.GetPlayerId(), RplSession.Mode(), Replication.IsServer(), Replication.IsClient()))
			{ Abort("ROLE_NOT_LOCAL_STANDALONE"); return; }
			if (GetGame().GetWorld() != m_World || m_World.IsEditMode()) { Abort("LOCAL_WORLD_CHANGED"); return; }
		}
		else if (!RICON_AudioMeterGate.RoleAllowed(controller.GetPlayerId())) { Abort("ROLE_NOT_B"); return; }
		IEntity entity = controller.GetControlledEntity();
		if (!entity || entity.GetWorld() != m_World || !Alive(entity)) return;
		SCR_CommunicationSoundComponent communication = SCR_CommunicationSoundComponent.Cast(entity.FindComponent(SCR_CommunicationSoundComponent));
		if (!communication || !communication.IsScriptedMethodsCallEnabled())
		{
			if (m_bLocalCalibration)
			{
				string reason = "COMMUNICATION_MISSING";
				if (communication) reason = "SCRIPTED_METHODS_DISABLED";
				if (reason != m_sLocalWaitReason)
				{
					m_sLocalWaitReason = reason;
					Emit(now, "WAIT_BINDING", "reason=" + reason + ",scriptedFlagForced=0");
				}
			}
			return;
		}
		string rpl = RplText(entity);
		if (!RICON_AudioMeterLocalGate.RplAllowed(m_bLocalCalibration, rpl != "UNKNOWN", true)) return;
		OnSoundEventStartedInvoker started = communication.GetOnSoundEventStarted();
		OnSoundEventFinishedInvoker finished = communication.GetOnSoundEventFinished();
		if (!started || !finished) return;
		m_Controller = controller; m_Entity = entity; m_Communication = communication; m_sRpl = rpl;
		if (m_bLocalCalibration) m_iBoundPlayer = controller.GetPlayerId();
		m_Started = started; m_Finished = finished; m_iBindingGeneration = 1; m_iSubscriptionGeneration = 1;
		m_Started.Insert(OnSoundStarted); m_Finished.Insert(OnSoundFinished);
		m_Phase = RICON_EAudioMeterPhase.BASELINE; m_iPhaseSince = now;
		if (m_bLocalCalibration)
		{
			string bound = string.Format("entityRpl=%1,componentRefLocal=%2,subscriptions=2,baseline=NO_REQUEST_NOT_PROVEN_SILENCE", rpl, communication);
			bound = bound + ",scriptedMethodsEnabled=" + Bit(communication.IsScriptedMethodsCallEnabled()) + ",alive=" + Bit(Alive(entity));
			bool allowed = RICON_AudioMeterLocalGate.RoleAllowed(true, controller.GetPlayerId(), RplSession.Mode(), Replication.IsServer(), Replication.IsClient());
			bound = bound + ",roleAllowed=" + Bit(allowed) + ",boundReadbacksAtomic=0";
			if (!Emit(now, "BOUND", bound)) return;
		}
		else if (!Emit(now, "BOUND", string.Format("entityRpl=%1,componentRefLocal=%2,subscriptions=2,baseline=NO_REQUEST_NOT_PROVEN_SILENCE", rpl, communication))) return;
		ReadMasters(now, "BIND");
	}
	protected bool Sample(int now)
	{
		if (!RICON_AudioMeterGate.SampleDue(m_iLastSample, now)) return true;
		int previous = m_iLastSample; m_iLastSample = now;
		float value = AudioSystem.OutputVolume();
		int returned = System.GetTickCount(); m_iSamples++;
		bool current = ContextCurrent(); bool finite = RICON_Num.IsFinite(value);
		string fields = "sample=" + m_iSamples.ToString() + ",valueRaw=" + Scalar(value) + ",finite=" + Bit(finite);
		int dt = -1; if (previous >= 0) dt = now - previous;
		fields = fields + ",previousReadTickMs=" + previous.ToString() + ",dtMs=" + dt.ToString();
		fields = fields + ",returnTickMs=" + returned.ToString() + ",contextValid=" + Bit(current);
		fields = fields + ",units=UNKNOWN,stage=UNKNOWN,stimulusEnergy=UNKNOWN";
		if (!Emit(now, "SAMPLE", fields)) return false;
		if (!finite || !current) { Abort("SAMPLE_NONFINITE_OR_CONTEXT"); return false; }
		return ClockReady(returned);
	}
	protected void Request(int now)
	{
		if (m_Event.m_bRequested || m_Journal.Count() > RICON_AudioMeterGate.EVENT_LIMIT - 16) { Abort("REQUEST_BUDGET_OR_DUPLICATE"); return; }
		if (!ReadMasters(now, "PRE_REQUEST")) return;
		now = System.GetTickCount();
		if (!ClockReady(now) || !ContextCurrent()) { Abort("REQUEST_CONTEXT"); return; }
		if (!m_Event.Begin(now)) { Abort("REQUEST_DUPLICATE"); return; }
		m_Phase = RICON_EAudioMeterPhase.EVENT_WINDOW; m_iPhaseSince = now;
		if (!Emit(now, "REQUEST_BEGIN", "event=SOUND_VOICE_PAIN_RELIEVE,priority=80,ignoreQueue=0,localInputEnergy=UNQUALIFIED")) return;
		int finalNow = System.GetTickCount();
		if (!m_bRunning || m_bInvalid || !ClockReady(finalNow) || !ContextCurrent()) { Abort("FINAL_CALL_CONTEXT"); return; }
		m_Event.m_bInNativeCall = true;
		m_iNativeCalls++;
		m_Communication.SoundEventPriority(SCR_SoundEvent.SOUND_VOICE_PAIN_RELIEVE, SCR_ECommunicationSoundEventPriority.SOUND_PAIN_RELIEVE, false);
		int returned = System.GetTickCount(); m_Event.m_bInNativeCall = false;
		if (!m_Event.m_sDeferredStop.IsEmpty())
		{
			Append(returned, "REQUEST_RETURN", "durationMs=" + (returned - finalNow).ToString() + ",deferredStop=1,postMasters=NOT_READ,playback=NOT_TESTED");
			Stop(m_Event.m_sDeferredStop); return;
		}
		if (!Emit(returned, "REQUEST_RETURN", "durationMs=" + (returned - finalNow).ToString() + ",accepted=UNKNOWN,playback=NOT_TESTED")) return;
		if (!ClockReady(returned) || !ContextCurrent()) { Abort("RETURN_CONTEXT"); return; }
		ReadMasters(returned, "POST_REQUEST");
	}
	protected bool EventMatch(string eventName, int priority)
	{
		return eventName == SCR_SoundEvent.SOUND_VOICE_PAIN_RELIEVE && priority == SCR_ECommunicationSoundEventPriority.SOUND_PAIN_RELIEVE;
	}
	protected bool CallbackReady(int now, SCR_CommunicationSoundComponent component)
	{
		if (!m_bRunning || !m_Event.m_sDeferredStop.IsEmpty()) return false;
		if (!ClockReady(now)) return false;
		if (component != m_Communication || !ContextCurrent()) { Abort("CALLBACK_CONTEXT"); return false; }
		if (m_Phase == RICON_EAudioMeterPhase.EVENT_WINDOW && RICON_AudioMeterGate.Expired(now - m_Event.m_iRequestedAt, RICON_AudioMeterGate.EVENT_MS)) { Abort("EVENT_TIMEOUT"); return false; }
		return true;
	}
	protected void OnSoundStarted(SCR_CommunicationSoundComponent component, string eventName, AudioHandle handle, int priority)
	{
		if (!m_bRunning) return;
		int now = System.GetTickCount(); m_iCallbacks++;
		if (!Emit(now, "LOCAL_START", string.Format("event=%1,priority=%2,handle=%3,componentMatch=%4,inNativeCall=%5,energy=UNKNOWN", RICON_LiveText.Esc(eventName), priority, handle, Bit(component == m_Communication), Bit(m_Event.m_bInNativeCall)))) return;
		if (!CallbackReady(now, component)) return;
		if (!m_Event.Started(m_Phase, true, EventMatch(eventName, priority), handle)) Abort("FOREIGN_OR_DUPLICATE_START");
	}
	protected void OnSoundFinished(SCR_CommunicationSoundComponent component, string eventName, AudioHandle handle, int priority, bool terminated)
	{
		if (!m_bRunning) return;
		int now = System.GetTickCount(); m_iCallbacks++;
		if (!Emit(now, "LOCAL_FINISH", string.Format("event=%1,priority=%2,handle=%3,componentMatch=%4,terminated=%5,inNativeCall=%6,energy=UNKNOWN", RICON_LiveText.Esc(eventName), priority, handle, Bit(component == m_Communication), Bit(terminated), Bit(m_Event.m_bInNativeCall)))) return;
		if (!CallbackReady(now, component)) return;
		if (!m_Event.Finished(m_Phase, now, true, EventMatch(eventName, priority), handle, terminated)) { Abort("FOREIGN_OR_DUPLICATE_FINISH"); return; }
		m_Phase = RICON_EAudioMeterPhase.RECOVERY; m_iPhaseSince = now; m_bFinishMastersPending = true;
	}
	void Tick(BaseWorld world, float timeSlice)
	{
		if (!m_bRunning) return;
		int now = System.GetTickCount();
		if (!world || world != m_World || world.IsEditMode()) { Abort("WORLD_CHANGED"); return; }
		if (!ClockReady(now)) return;
		if (m_Phase == RICON_EAudioMeterPhase.WAIT_BINDING)
		{
			if (RICON_AudioMeterGate.Expired(now - m_iT0, RICON_AudioMeterGate.BIND_MS)) { Abort("BIND_TIMEOUT"); return; }
			if (now - m_iLastBindPoll >= 500) { m_iLastBindPoll = now; Bind(now); }
			return;
		}
		if (!ContextCurrent()) { Abort("BINDING_CHANGED"); return; }
		if (m_bFinishMastersPending)
		{
			m_bFinishMastersPending = false;
			if (!ReadMasters(now, "FINISH")) return;
		}
		if (!Sample(now)) return;
		if (m_Phase == RICON_EAudioMeterPhase.BASELINE && now - m_iPhaseSince >= RICON_AudioMeterGate.BASELINE_MS) Request(now);
		else if (m_Phase == RICON_EAudioMeterPhase.EVENT_WINDOW && RICON_AudioMeterGate.Expired(now - m_Event.m_iRequestedAt, RICON_AudioMeterGate.EVENT_MS)) Abort("EVENT_TIMEOUT");
		else if (m_Phase == RICON_EAudioMeterPhase.RECOVERY && now - m_iPhaseSince >= RICON_AudioMeterGate.RECOVERY_MS)
		{
			if (ReadMasters(now, "RECOVERY_END")) Stop("SEQUENCE_COMPLETE");
		}
	}
	bool IsRunning() { return m_bRunning; }
	void Stop(string reason)
	{
		if (!m_bRunning) return;
		if (m_Event.DeferStop(reason)) return;
		m_bRunning = false;
		if (m_Started) m_Started.Remove(OnSoundStarted);
		if (m_Finished) m_Finished.Remove(OnSoundFinished);
		m_Started = null; m_Finished = null; m_iSubscriptionGeneration++;
		int now = System.GetTickCount();
		if (m_Event.NeedsGap(reason)) Append(now, "GAP", "reason=" + reason + ",overflowSeen=" + Bit(m_Event.m_bOverflowSeen) + ",missing=UNKNOWN");
		string fields = "reason=" + reason + ",invalid=" + Bit(m_bInvalid) + ",samples=" + m_iSamples.ToString();
		fields = fields + ",callbacks=" + m_iCallbacks.ToString() + ",requested=" + Bit(m_Event.m_bRequested) + ",nativeCalls=" + m_iNativeCalls.ToString();
		fields = fields + ",overflowSeen=" + Bit(m_Event.m_bOverflowSeen);
		fields = fields + ",started=" + m_Event.m_iStarted.ToString() + ",finished=" + m_Event.m_iFinished.ToString();
		fields = fields + ",eventOutstanding=" + Bit(m_Event.Outstanding()) + ",lastMasters=" + m_sLastMasters;
		fields = fields + ",subscriptionsRemaining=0,semantics=UNKNOWN,inputEnergy=UNQUALIFIED,playback=NOT_TESTED,drain=UNPROVEN";
		m_Phase = RICON_EAudioMeterPhase.TERMINAL;
		Append(now, "TERMINAL", fields);
		m_World = null; m_Controller = null; m_Entity = null; m_Communication = null;
	}
}
