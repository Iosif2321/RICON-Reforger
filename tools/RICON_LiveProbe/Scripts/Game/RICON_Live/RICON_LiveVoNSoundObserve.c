//! B-passive public callback capability. Ни одного source/audio/routing setter.
class RICON_VoNSoundObserveGate
{
	static const int EVENT_LIMIT = 256;
	static bool Config(bool configured, string value, bool nrx, bool separated, bool server, bool client)
	{
		return configured && value == "1" && nrx && separated && !server && client;
	}
}

//! Только один наблюдаемый active lifetime на компонент. Overlap не превращается в stream token.
class RICON_VoNSoundLifetime
{
	AudioHandle Handle = AudioHandle.Invalid;
	string EventName;
	int Priority;
	int StartTick;
	int Ordinal;
	int Starts;
	int Finishes;
	int Ambiguous;
	bool Active;
	int Started(string eventName, AudioHandle handle, int priority, int now)
	{
		Starts++;
		if (handle == AudioHandle.Invalid || Active || now < 0)
		{
			Ambiguous++;
			return 0;
		}
		Active = true; Handle = handle; EventName = eventName; Priority = priority; StartTick = now; Ordinal++;
		return Ordinal;
	}
	int Finished(string eventName, AudioHandle handle, int priority, int now, out int duration)
	{
		duration = -1; Finishes++;
		if (!Active || handle == AudioHandle.Invalid || handle != Handle || eventName != EventName || priority != Priority || now < StartTick)
		{
			Ambiguous++;
			return 0;
		}
		duration = now - StartTick; Active = false;
		return Ordinal;
	}
}

class RICON_VoNSoundTap
{
	RICON_LiveVoNSoundObserve Probe;
	IEntity Actor;
	SCR_CommunicationSoundComponent Component;
	string Subject;
	string ActorRpl;
	string Availability;
	bool Scripted;
	protected ref OnSoundEventStartedInvoker m_Started;
	protected ref OnSoundEventFinishedInvoker m_Finished;
	protected bool m_Attached;
	int Generation;
	ref RICON_VoNSoundLifetime Life = new RICON_VoNSoundLifetime();
	bool Prepare(RICON_LiveVoNSoundObserve probe, string subject, IEntity actor, string rpl)
	{
		Probe = probe; Subject = subject; Actor = actor; ActorRpl = rpl;
		Component = SCR_CommunicationSoundComponent.Cast(actor.FindComponent(SCR_CommunicationSoundComponent));
		Availability = "COMPONENT_MISSING";
		if (!Component) return false;
		Scripted = Component.IsScriptedMethodsCallEnabled(); Availability = "SCRIPTED_DISABLED";
		if (!Scripted) return false;
		m_Started = Component.GetOnSoundEventStarted(); m_Finished = Component.GetOnSoundEventFinished();
		Availability = "INVOKER_MISSING";
		if (!m_Started || !m_Finished) return false;
		Availability = "AVAILABLE";
		return true;
	}
	bool SharesHooks(RICON_VoNSoundTap other)
	{
		return other && (Component == other.Component || m_Started == other.m_Started || m_Finished == other.m_Finished);
	}
	void Attach()
	{
		m_Started.Insert(OnStarted); m_Finished.Insert(OnFinished); m_Attached = true; Generation = 1;
	}
	int Subscriptions()
	{
		if (m_Attached) return 2;
		return 0;
	}
	bool Current()
	{
		return m_Attached && Actor && Component && Actor.FindComponent(SCR_CommunicationSoundComponent) == Component
			&& Component.IsScriptedMethodsCallEnabled() && Component.GetOnSoundEventStarted() == m_Started
			&& Component.GetOnSoundEventFinished() == m_Finished;
	}
	void Detach()
	{
		bool attached = m_Attached; m_Attached = false;
		if (attached && m_Started) m_Started.Remove(OnStarted);
		if (attached && m_Finished) m_Finished.Remove(OnFinished);
		m_Started = null; m_Finished = null; Probe = null;
		if (attached) Generation++;
	}
	protected void OnStarted(SCR_CommunicationSoundComponent component, string eventName, AudioHandle handle, int priority)
	{
		if (m_Attached && Probe) Probe.Record(this, component, true, eventName, handle, priority, false);
	}
	protected void OnFinished(SCR_CommunicationSoundComponent component, string eventName, AudioHandle handle, int priority, bool terminated)
	{
		if (m_Attached && Probe) Probe.Record(this, component, false, eventName, handle, priority, terminated);
	}
}

class RICON_LiveVoNSoundObserve
{
	protected RICON_LiveNativeRxProbe m_Nrx;
	protected BaseWorld m_World;
	protected PlayerController m_Controller;
	protected ref RICON_SeparatedPairGeometry m_Identity;
	protected ref RICON_VoNSoundTap m_Own;
	protected ref RICON_VoNSoundTap m_Proxy;
	protected string m_Run;
	protected int m_Started;
	protected int m_Last;
	protected int m_LastReady;
	protected int m_NrxStart = -1;
	protected int m_Rows;
	protected int m_RawStarts;
	protected int m_RawFinishes;
	protected bool m_Running;
	protected bool m_Bound;
	protected bool m_Invalid;
	static bool Configured()
	{
		string value;
		return System.GetCLIParam("riconVoNSoundObserve", value);
	}
	protected static string Bit(bool value)
	{
		if (value)
			return "1";
		return "0";
	}
	protected string IdentityText(bool current)
	{
		string own = "UNKNOWN"; string proxy = "UNKNOWN"; int generation;
		if (m_Identity) { own = m_Identity.BRpl; proxy = m_Identity.ARpl; generation = m_Identity.Binding; }
		string text = "localPlayer=2,bindingGeneration=" + generation.ToString();
		text = text + ",ownBRpl=" + own + ",proxyARpl=" + proxy;
		return text + ",current=" + Bit(current) + ",observerStartTickMs=" + m_Started.ToString();
	}
	protected void Append(int now, string kind, string fields)
	{
		string line = "[RICON_VSOUND] seq=" + m_Rows.ToString() + " run=" + RICON_LiveText.Esc(m_Run);
		line = line + " kind=" + kind + " rawTickMs=" + now.ToString();
		line = line + " startTickMs=" + m_NrxStart.ToString() + " text=" + fields;
		m_Rows++; PrintFormat("%1", line);
	}
	protected bool Emit(int now, string kind, string fields)
	{
		if (m_Rows >= RICON_VoNSoundObserveGate.EVENT_LIMIT)
		{
			Stop("UNQUALIFIED", "OVERFLOW", true);
			return false;
		}
		Append(now, kind, fields);
		return true;
	}
	bool Start(BaseWorld world, RICON_LiveNativeRxProbe nrx)
	{
		string value; bool configured = System.GetCLIParam("riconVoNSoundObserve", value);
		if (!RICON_VoNSoundObserveGate.Config(configured, value, RICON_LiveNativeRxProbe.IsRequested(),
			RICON_CaptureSeparatedGate.Requested(), Replication.IsServer(), Replication.IsClient())) return false;
		if (!world || world.IsEditMode() || !nrx || !RICON_PairPlacementGate.WorldAllowed(GetGame().GetWorldFile())) return false;
		if (!System.GetCLIParam("riconNativeRxRun", m_Run) || !RICON_D0Gate.ValidRunId(m_Run)) return false;
		m_Started = System.GetTickCount(); m_Last = m_Started; m_LastReady = m_Started;
		if (m_Started < 0) return false;
		m_World = world; m_Nrx = nrx; m_Running = true;
		Emit(m_Started, "WAITING", "availability=UNQUALIFIED,subscriptions=0,source=ORDINARY_PLAYER_ONLY,stimulusRequests=0");
		return true;
	}
	protected bool Current(int now)
	{
		if (!m_Bound || !m_Identity || !m_Controller || GetGame().GetPlayerController() != m_Controller || m_Controller.GetPlayerId() != 2) return false;
		int remaining;
		if (!m_Nrx || !m_Nrx.RICON_PulseHealth(now, m_Run, 2, remaining) || now - RICON_NativeRxGate.LIMIT_MS + remaining != m_NrxStart) return false;
		RICON_SeparatedPairGeometry actual;
		if (!m_Nrx.RICON_ReadSeparatedGeometry(now, m_Run, m_World, actual) || !actual.Eligible || !m_Identity.SameIdentity(actual)) return false;
		return m_Own.Current() && m_Proxy.Current();
	}
	protected string HookText(RICON_VoNSoundTap tap)
	{
		string text = "subject=" + tap.Subject + ",ownerActorRpl=" + tap.ActorRpl;
		text = text + ",componentRefLocal=" + string.Format("%1", tap.Component) + ",scriptedEnabled=" + Bit(tap.Scripted);
		return text + ",availability=" + tap.Availability + ",subscriptions=" + tap.Subscriptions().ToString() + ",subjectScope=COMPONENT_OWNER_ONLY";
	}
	protected void Bind(int now)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || controller.GetPlayerId() == 0) return;
		if (controller.GetPlayerId() != 2) { Stop("UNQUALIFIED", "NOT_B_ROLE", true); return; }
		int remaining;
		if (!m_Nrx.RICON_PulseHealth(now, m_Run, 2, remaining)) return;
		RICON_SeparatedPairGeometry identity;
		if (!m_Nrx.RICON_ReadSeparatedGeometry(now, m_Run, m_World, identity) || !identity.Eligible) return;
		m_Identity = identity; m_Controller = controller; m_NrxStart = now - RICON_NativeRxGate.LIMIT_MS + remaining;
		m_Own = new RICON_VoNSoundTap(); m_Proxy = new RICON_VoNSoundTap();
		bool own = m_Own.Prepare(this, "ownB", identity.ActorB, identity.BRpl);
		bool proxy = m_Proxy.Prepare(this, "sourceAproxy", identity.ActorA, identity.ARpl);
		now = System.GetTickCount();
		if (!ClockValid(now)) { Stop("UNQUALIFIED", "CLOCK_OR_LIFETIME", true); return; }
		if (!Emit(now, "AVAILABILITY", IdentityText(false) + "," + HookText(m_Own))) return;
		if (!Emit(now, "AVAILABILITY", IdentityText(false) + "," + HookText(m_Proxy))) return;
		if (!own || !proxy || m_Own.SharesHooks(m_Proxy))
		{
			string reason = "ALIASED_HOOK";
			if (!own) reason = m_Own.Availability;
			else if (!proxy) reason = m_Proxy.Availability;
			Emit(now, "NOT_AVAILABLE", IdentityText(false) + ",ownBReady=" + Bit(own) + ",aProxyReady=" + Bit(proxy) + ",reason=" + reason);
			Stop("NOT_AVAILABLE", reason, false);
			return;
		}
		m_Own.Attach(); m_Proxy.Attach(); m_Bound = true;
		PublishReady();
	}
	protected void PublishReady()
	{
		int now = System.GetTickCount();
		if (!Current(now)) { Stop("UNQUALIFIED", "BINDING_CHANGED", true); return; }
		int observed = System.GetTickCount();
		if (!ClockValid(observed) || observed - now > 500) { Stop("UNQUALIFIED", "CLOCK_OR_LIFETIME", true); return; }
		m_Last = observed; m_LastReady = observed;
		string fields = IdentityText(true) + ",ownBReady=1,aProxyReady=1,ownBSubscriptions=2,aProxySubscriptions=2,subscriptionGeneration=1";
		fields = fields + ",ownBComponentRefLocal=" + string.Format("%1", m_Own.Component);
		fields = fields + ",aProxyComponentRefLocal=" + string.Format("%1", m_Proxy.Component);
		Emit(observed, "READY", fields + ",nativeStreamBinding=UNQUALIFIED,rxJointToken=ABSENT");
	}
	protected bool ClockValid(int now)
	{
		if (RICON_NativeRxGate.ClockGap(m_Last, now) || RICON_NativeRxGate.Expired(now - m_Started)) return false;
		return m_NrxStart < 0 || !RICON_NativeRxGate.Expired(now - m_NrxStart);
	}
	void Record(RICON_VoNSoundTap tap, SCR_CommunicationSoundComponent component, bool started, string eventName, AudioHandle handle, int priority, bool terminated)
	{
		if (!m_Running || (tap != m_Own && tap != m_Proxy)) return;
		if (started) m_RawStarts++;
		else m_RawFinishes++;
		int callbackAt = System.GetTickCount();
		bool valid = ClockValid(callbackAt) && component == tap.Component && Current(callbackAt);
		int observed = System.GetTickCount();
		valid = valid && ClockValid(observed) && observed >= callbackAt && observed - callbackAt <= 500;
		int ordinal; int duration = -1;
		if (valid)
		{
			m_Last = observed;
			if (started) ordinal = tap.Life.Started(eventName, handle, priority, callbackAt);
			else ordinal = tap.Life.Finished(eventName, handle, priority, callbackAt, duration);
		}
		string kind = "SOUND_FINISH";
		if (started) kind = "SOUND_START";
		string fields = IdentityText(valid) + "," + HookText(tap);
		fields = fields + ",eventName=" + RICON_LiveText.Esc(eventName) + ",handle=" + string.Format("%1", handle);
		fields = fields + ",handleKnown=" + Bit(handle != AudioHandle.Invalid) + ",priority=" + priority.ToString();
		fields = fields + ",terminated=" + Bit(terminated) + ",callbackTickMs=" + callbackAt.ToString();
		fields = fields + ",componentMatches=" + Bit(component == tap.Component) + ",subscriptionGeneration=" + tap.Generation.ToString();
		fields = fields + ",callbackComponentRefLocal=" + string.Format("%1", component);
		fields = fields + ",lifetimeOrdinal=" + ordinal.ToString() + ",lifetimeDurationMs=" + duration.ToString();
		if (!Emit(observed, kind, fields + ",nativeStreamBinding=UNQUALIFIED,rxJointToken=ABSENT,route=UNQUALIFIED")) return;
		if (!valid) Stop("UNQUALIFIED", "CALLBACK_CONTEXT_OR_CLOCK", true);
	}
	void Tick(BaseWorld world)
	{
		if (!m_Running) return;
		int now = System.GetTickCount();
		if (!world || world != m_World || world.IsEditMode() || !ClockValid(now)) { Stop("UNQUALIFIED", "WORLD_CLOCK_OR_LIFETIME", true); return; }
		m_Last = now;
		if (!m_Bound) { Bind(now); return; }
		if (now - m_LastReady >= 500) PublishReady();
	}
	void Stop(string result, string reason, bool invalid)
	{
		if (!m_Running) return;
		m_Running = false; m_Invalid = m_Invalid || invalid;
		int pending; int starts; int finishes; int ambiguous;
		if (m_Own)
		{
			m_Own.Detach();
			if (m_Own.Life.Active) pending++;
			starts += m_Own.Life.Starts; finishes += m_Own.Life.Finishes; ambiguous += m_Own.Life.Ambiguous;
		}
		if (m_Proxy)
		{
			m_Proxy.Detach();
			if (m_Proxy.Life.Active) pending++;
			starts += m_Proxy.Life.Starts; finishes += m_Proxy.Life.Finishes; ambiguous += m_Proxy.Life.Ambiguous;
		}
		int now = System.GetTickCount();
		if (reason == "OVERFLOW") Append(now, "GAP", "reason=OVERFLOW,missingEvents=UNKNOWN");
		string fields = IdentityText(false) + ",result=" + result + ",reason=" + reason;
		fields = fields + ",invalid=" + Bit(m_Invalid) + ",subscriptionsRemaining=0,rawStarts=" + m_RawStarts.ToString();
		fields = fields + ",rawFinishes=" + m_RawFinishes.ToString() + ",trackedStarts=" + starts.ToString();
		fields = fields + ",trackedFinishes=" + finishes.ToString() + ",pendingKnown=" + pending.ToString() + ",ambiguous=" + ambiguous.ToString();
		fields = fields + ",attribution=UNQUALIFIED_PUBLIC_STREAM_BINDING,nativeStreamBinding=UNQUALIFIED,rxJointToken=ABSENT";
		Append(now, "TERMINAL", fields + ",DIRECT=NOT_TESTED,drain=UNKNOWN,observerCaptureCalls=0");
		m_World = null; m_Nrx = null; m_Controller = null; m_Identity = null;
	}
}
