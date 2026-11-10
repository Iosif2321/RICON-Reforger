//! TOOL_ONLY: metadata; opt-in вызывает local source AddOrFindSignal(name,1). Без capture/radio setters и DSP verdict.
class RICON_NativeRxGate
{
	static const int LIMIT_MS = 60000;
	static const int STALL_MS = 5000;
	static const int EVENT_LIMIT = 512;
	static bool Requested(bool configured, string value) { return configured && value == "1"; }
	static bool RoleAllowed(int role) { return role == 1 || role == 2; }
	static bool Expired(int elapsed) { return elapsed < 0 || elapsed >= LIMIT_MS; }
	static bool ClockGap(int previous, int now) { return now < previous || now - previous > STALL_MS; }
	static bool QualityValid(float value) { return RICON_Num.IsFinite(value) && value >= 0 && value <= 1; }
	static bool CatalogAllowed(int gadgets, int transceivers) { return gadgets >= 0 && gadgets <= 4 && transceivers >= 0 && transceivers <= 16; }
	static array<SCR_GadgetComponent> CombineCategories(array<SCR_GadgetComponent> handheld, array<SCR_GadgetComponent> backpack)
	{
		array<SCR_GadgetComponent> found = {};
		if (handheld) found.InsertAll(handheld);
		if (backpack) found.InsertAll(backpack);
		return found;
	}
	static bool OrdinaryAllowed(int count) { return count >= 0 && count < EVENT_LIMIT; }
	static string Route(bool present, bool matched)
	{
		if (!present) return "NULL_OR_UNKNOWN";
		if (matched) return "CATALOG_REF_MATCH";
		return "UNBOUND_RECEIVER";
	}
}

//! Одно нейтральное окно наблюдения только от собственного qualifying RX на B.
class RICON_BlindBObserveWindow
{
	protected int m_First = -1;
	protected bool m_Closed;
	static bool Config(bool configured, string value, bool nativeRx, bool separated, bool server, bool client)
	{
		if (!configured)
			return true;
		return value == "1" && nativeRx && separated && !server && client;
	}
	bool FirstRx(int now, bool qualifying)
	{
		if (!qualifying || now < 0 || m_First >= 0 || m_Closed)
			return false;
		m_First = now;
		return true;
	}
	bool Active() { return m_First >= 0 && !m_Closed; }
	int PollMs()
	{
		if (Active())
			return 100;
		return 500;
	}
	int HeartbeatMs()
	{
		if (Active())
			return 100;
		return 1000;
	}
	bool HeartbeatWritten(int now)
	{
		if (!Active() || now < m_First || now - m_First < 1100)
			return false;
		m_Closed = true;
		return true;
	}
}

//! Pure rules: known означает наблюдённые local getter values, не media/native permission.
class RICON_RadioReadinessGate
{
	static string View(int player, bool server, bool client)
	{
		if (player == 1 && server) return "A_SERVER_VIEW";
		if (player == 2 && client && !server) return "B_OWN_A_PROXY_VIEW";
		return "UNQUALIFIED_VIEW";
	}
	static bool PairKnown(bool identities, bool catalogs, int uniqueA, int uniqueB, bool refs)
	{
		return identities && catalogs && uniqueA == 1 && uniqueB == 1 && refs;
	}
	static bool BindingAllowed(bool known, bool radio, int index, bool active, bool refs)
	{
		return known && radio && index >= 0 && active && refs;
	}
	static int Bit(bool known, bool value)
	{
		if (!known) return -1;
		if (value) return 1;
		return 0;
	}
	static int Equality(bool known, string a, string b) { return Bit(known, a == b); }
}

//! Только публичные metadata fields; ключи никогда не хранятся в snapshot.
class RICON_RadioReadinessSnapshot
{
	string View = "UNQUALIFIED_VIEW";
	int OwnIdentityCurrent;
	int EntryKnown;
	int EntryRadio = -1;
	int EntryIndex = -1;
	int EntryNativeIndex = -1;
	int EntryActive = -1;
	int EntryUsable = -1;
	int EntryBindingAllowed = -1;
	int EntryFrequency = -1;
	int PairKnown;
	int UniqueA = -1;
	int UniqueB = -1;
	int AEmpty = -1;
	int BEmpty = -1;
	int KeysEqual = -1;
	string AFrequencies = "UNKNOWN";
	string BFrequencies = "UNKNOWN";
	string AOwnerRpl = "UNKNOWN";
	string BOwnerRpl = "UNKNOWN";
	string Text()
	{
		string text = "rrView=" + View + ",rrOwnIdentityCurrent=" + OwnIdentityCurrent.ToString();
		text = text + ",rrEntryKnown=" + EntryKnown.ToString() + ",rrEntryRadio=" + EntryRadio.ToString();
		text = text + ",rrEntryIndex=" + EntryIndex.ToString() + ",rrEntryNativeIndex=" + EntryNativeIndex.ToString();
		text = text + ",rrEntryActive=" + EntryActive.ToString() + ",rrEntryUsable=" + EntryUsable.ToString();
		text = text + ",rrEntryBindingAllowed=" + EntryBindingAllowed.ToString() + ",rrEntryFrequencyKHz=" + EntryFrequency.ToString();
		text = text + ",rrPairKnown=" + PairKnown.ToString() + ",rrAUnique=" + UniqueA.ToString();
		text = text + ",rrBUnique=" + UniqueB.ToString() + ",rrAKeyEmpty=" + AEmpty.ToString();
		text = text + ",rrBKeyEmpty=" + BEmpty.ToString() + ",rrKeysEqual=" + KeysEqual.ToString();
		text = text + ",rrAFrequenciesKHz=" + AFrequencies + ",rrBFrequenciesKHz=" + BFrequencies;
		text = text + ",rrAOwnerRpl=" + AOwnerRpl + ",rrBOwnerRpl=" + BOwnerRpl;
		return text + ",rrReplication=UNQUALIFIED,rrNativeOwnership=UNQUALIFIED,rrPermission=SCRIPT_STATE_ONLY,rrAtomic=0";
	}
}

//! Только DIRECT opt-in и near fixture, не native SpeakingRange.
class RICON_DirectCaptureGate
{
	static bool Config(bool configured, string value, bool nrx, bool conflict)
	{
		return configured && value == "1" && nrx && !conflict;
	}
	static bool OptionalPower(bool configured, string value) { return !configured || value == "1"; }
	static bool Configured()
	{
		string value;
		return System.GetCLIParam("riconCaptureDirect", value);
	}
	static bool Requested()
	{
		string value; bool configured = System.GetCLIParam("riconCaptureDirect", value);
		array<string> forbidden = {"riconCaptureSeparatedPair", "riconPairPlacement", "riconNativeRadioPower", "riconCaptureNoSend", "riconCaptureWrongFrequency", "riconCaptureShort400", "riconCaptureBlindTrial", "riconVoNSoundObserve", "riconBlindBObserve", "riconRestRehearsalArm", "riconAudioMeter", "riconCommunicationDiscovery", "riconVoNInputShape"};
		foreach (string name : forbidden) { string other; if (System.GetCLIParam(name, other)) return false; }
		string powerValue; bool powerConfigured = System.GetCLIParam("riconCaptureBPowerOff", powerValue);
		if (!OptionalPower(powerConfigured, powerValue)) return false;
		return Config(configured, value, RICON_LiveNativeRxProbe.IsRequested(), false);
	}
	static bool Near(vector a, vector b)
	{
		return RICON_PairPlacementGate.Finite(a) && RICON_PairPlacementGate.Finite(b) && vector.Distance(a, b) <= 3;
	}
	static bool Route(bool methodDirect, bool txNull) { return methodDirect && txNull; }
	static string Text(RICON_SeparatedPairGeometry geometry)
	{
		int eligible;
		if (geometry.Eligible) eligible = 1;
		string text = "geoView=" + geometry.View + ",geoKnown=1,geoEligible=" + eligible.ToString();
		text = text + ",geoActorARpl=" + geometry.ARpl + ",geoActorBRpl=" + geometry.BRpl;
		text = text + ",geoAX=" + geometry.APosition[0].ToString() + ",geoAY=" + geometry.APosition[1].ToString();
		text = text + ",geoAZ=" + geometry.APosition[2].ToString() + ",geoBX=" + geometry.BPosition[0].ToString();
		text = text + ",geoBY=" + geometry.BPosition[1].ToString() + ",geoBZ=" + geometry.BPosition[2].ToString();
		text = text + ",geoDistance3D=" + vector.Distance(geometry.APosition, geometry.BPosition).ToString();
		return text + ",geoObservedTickMs=" + geometry.ObservedAt.ToString() + ",geoScope=LIVE_REFERENCES";
	}
}

//! One real invoker owner per tap; synthetic invokers are confined to the check plugin.
class RICON_NativeRxTap
{
	protected RICON_LiveNativeRxProbe m_Probe;
	protected ScriptInvoker m_Receive;
	protected ScriptInvoker m_Capture;
	protected ScriptInvokerInt m_Used;
	protected int m_iOwner;
	protected int m_iGeneration;

	int OwnerPlayer() { return m_iOwner; }
	int Generation() { return m_iGeneration; }
	bool MatchesReceive(ScriptInvoker receive) { return receive && m_Receive == receive && Subscriptions() == 1; }
	bool MatchesSource(ScriptInvoker capture, ScriptInvokerInt used)
	{
		return capture && used && m_Capture == capture && m_Used == used && Subscriptions() == 2;
	}
	int Subscriptions()
	{
		int count;
		if (m_Receive) count++;
		if (m_Capture) count++;
		if (m_Used) count++;
		return count;
	}
	bool AttachReceive(RICON_LiveNativeRxProbe probe, ScriptInvoker receive, int ownerPlayer)
	{
		if (!probe || !receive || Subscriptions() != 0 || !RICON_NativeRxGate.RoleAllowed(ownerPlayer)) return false;
		m_Probe = probe; m_iOwner = ownerPlayer; m_iGeneration++;
		m_Receive = receive; m_Receive.Insert(OnReceive);
		return true;
	}
	bool AttachSource(RICON_LiveNativeRxProbe probe, ScriptInvoker capture, ScriptInvokerInt used)
	{
		if (!probe || !capture || !used || Subscriptions() != 0) return false;
		m_Probe = probe; m_iOwner = 1; m_iGeneration++;
		m_Capture = capture; m_Used = used;
		m_Capture.Insert(OnCapture); m_Used.Insert(OnUsed);
		return true;
	}
	void Detach()
	{
		if (m_Receive) m_Receive.Remove(OnReceive);
		if (m_Capture) m_Capture.Remove(OnCapture);
		if (m_Used) m_Used.Remove(OnUsed);
		m_Receive = null; m_Capture = null; m_Used = null; m_Probe = null;
		m_iGeneration++;
	}
	protected void OnReceive(int sender, BaseTransceiver receiver, int frequency, float quality)
	{
		if (m_Probe) m_Probe.Receive(this, sender, receiver, frequency, quality);
	}
	protected void OnCapture(BaseTransceiver transmitter)
	{
		if (m_Probe) m_Probe.Capture(this, transmitter);
	}
	protected void OnUsed(int sender)
	{
		if (m_Probe) m_Probe.Used(this, sender);
	}
}

class RICON_NativeRxEndpoint
{
	SCR_GadgetComponent m_Gadget;
	BaseRadioComponent m_Radio;
	BaseTransceiver m_Transceiver;
	int m_iNativeIndex;
}

class RICON_LiveNativeRxProbe
{
	protected ref RICON_LiveJournal m_Journal;
	protected ref RICON_NativeRxTap m_OwnTap;
	protected ref RICON_NativeRxTap m_ProxyTap;
	protected ref array<ref RICON_NativeRxEndpoint> m_Catalog = {};
	protected BaseWorld m_World;
	protected PlayerController m_Controller;
	protected IEntity m_Entity;
	protected IEntity m_ProxyA;
	protected string m_sOwnRpl;
	protected string m_sProxyRpl;
	protected SCR_VoNComponent m_OwnVon;
	protected SCR_VoNComponent m_ProxyVon;
	protected int m_iPlayer = -1;
	protected int m_iT0;
	protected int m_iLastClock;
	protected int m_iLastPoll;
	protected int m_iLastHeartbeat;
	protected int m_iCallbacks;
	protected bool m_bRunning;
	protected bool m_bBound;
	protected bool m_bInvalid;
	protected bool m_bSndIndexObserved;
	protected bool m_bSourceSignalPreregister;
	protected bool m_bSourceSignalAttempted;
	protected SignalsManagerComponent m_SourceSignalManager;
	protected int m_iSourceSignalIndex = -1;
	protected string m_sRun;
	protected ref RICON_BlindBObserveWindow m_BlindBWindow;

	static bool IsRequested()
	{
		string value;
		bool configured = System.GetCLIParam("riconNativeRx", value);
		if (!RICON_NativeRxGate.Requested(configured, value)) return false;
		string d0; string v0;
		if (System.GetCLIParam("riconCommunicationDiscovery", d0) && d0 == "1") return false;
		if (System.GetCLIParam("riconVoNInputShape", v0) && v0 == "1") return false;
		return true;
	}
	protected static string Bit(bool value)
	{
		if (value)
			return "1";
		return "0";
	}
	protected static string RplText(IEntity entity)
	{
		if (!entity) return "UNKNOWN";
		RplId id = Replication.FindItemId(entity);
		if (id == RplId.Invalid() || Replication.FindItem(id) != entity) return "UNKNOWN";
		return string.Format("%1", id);
	}
	protected void Append(int now, string owner, string kind, int sender, int index, int frequency, string text)
	{
		string fields = "run=" + RICON_LiveText.Esc(m_sRun) + ",localPlayer=" + m_iPlayer.ToString();
		fields = fields + ",bindingGeneration=" + Bit(m_bBound) + ",rawTickMs=" + now.ToString() + ",startTickMs=" + m_iT0.ToString() + "," + text;
		m_Journal.Add(now - m_iT0, owner, kind, sender, -1, index, frequency, fields);
		string line = RICON_LiveJournal.Format(m_Journal.At(m_Journal.Count() - 1));
		line.Replace("[RICON_LIVE]", "[RICON_NRX]");
		PrintFormat("%1", line);
	}
	protected bool Emit(int now, string owner, string kind, int sender, int index, int frequency, string text)
	{
		if (!m_bRunning) return false;
		if (!RICON_NativeRxGate.OrdinaryAllowed(m_Journal.Count()))
		{
			m_bInvalid = true; Stop("OVERFLOW"); return false;
		}
		Append(now, owner, kind, sender, index, frequency, text);
		return true;
	}
	bool Start(BaseWorld world)
	{
		if (m_bRunning || !world || world.IsEditMode()) return false;
		string run;
		if (!System.GetCLIParam("riconNativeRxRun", run) || !RICON_D0Gate.ValidRunId(run)) return false;
		string preregValue;
		bool preregConfigured = System.GetCLIParam("riconSourceSignalPreregister", preregValue);
		if (preregConfigured && (preregValue != "1" || !IsRequested() || Replication.IsServer() || !Replication.IsClient()
			|| !RICON_PairPlacementGate.WorldAllowed(GetGame().GetWorldFile()))) return false;
		string observeValue;
		bool observeConfigured = System.GetCLIParam("riconBlindBObserve", observeValue);
		if (!RICON_BlindBObserveWindow.Config(observeConfigured, observeValue, IsRequested(), RICON_CaptureSeparatedGate.Requested(),
			Replication.IsServer(), Replication.IsClient())) return false;
		if (observeConfigured && !RICON_PairPlacementGate.WorldAllowed(GetGame().GetWorldFile())) return false;
		if (observeConfigured) m_BlindBWindow = new RICON_BlindBObserveWindow();
		int now = System.GetTickCount();
		if (now < 0) return false;
		m_sRun = run; m_World = world; m_iT0 = now; m_iLastClock = now; m_iLastPoll = now - 500; m_iLastHeartbeat = now;
		m_bBound = false; m_bInvalid = false; m_iCallbacks = 0; m_iPlayer = -1; m_Catalog.Clear();
		m_bSndIndexObserved = false;
		m_bSourceSignalPreregister = preregConfigured; m_bSourceSignalAttempted = false;
		m_SourceSignalManager = null; m_iSourceSignalIndex = -1;
		m_Journal = new RICON_LiveJournal(516); m_Journal.SetStream(false);
		m_OwnTap = new RICON_NativeRxTap(); m_ProxyTap = new RICON_NativeRxTap();
		m_bRunning = true;
		string startMode = "passive=1";
		if (m_bSourceSignalPreregister) startMode = "passive=0,localSignalPreregister=1";
		Emit(now, "process", "START", -1, -1, -1, startMode + ",clock=RUN_LOCAL_ELAPSED_MS,origin=START,bulkAllocation=UNKNOWN,delivery=NOT_TESTED");
		return true;
	}
	protected bool BuildCatalog(IEntity entity, out array<ref RICON_NativeRxEndpoint> found)
	{
		found = {};
		SCR_GadgetManagerComponent manager = SCR_GadgetManagerComponent.GetGadgetManager(entity);
		if (!manager) return false;
		array<SCR_GadgetComponent> gadgets = RICON_NativeRxGate.CombineCategories(
			manager.GetGadgetsByType(EGadgetType.RADIO), manager.GetGadgetsByType(EGadgetType.RADIO_BACKPACK));
		if (gadgets.Count() > 4) return false;
		foreach (SCR_GadgetComponent gadget : gadgets)
		{
			if (!gadget || !gadget.GetOwner() || !manager.IsGadgetOwned(gadget)) return false;
			BaseRadioComponent radio = BaseRadioComponent.Cast(gadget.GetOwner().FindComponent(BaseRadioComponent));
			if (!radio) return false;
			int count = radio.TransceiversCount();
			if (count < 0 || count > 4) return false;
			for (int i = 0; i < count; i++)
			{
				BaseTransceiver tr = radio.GetTransceiver(i);
				if (!tr) return false;
				foreach (RICON_NativeRxEndpoint previous : found) if (previous.m_Transceiver == tr) return false;
				RICON_NativeRxEndpoint endpoint = new RICON_NativeRxEndpoint();
				endpoint.m_Gadget = gadget; endpoint.m_Radio = radio; endpoint.m_Transceiver = tr; endpoint.m_iNativeIndex = i;
				found.Insert(endpoint);
			}
		}
		return RICON_NativeRxGate.CatalogAllowed(gadgets.Count(), found.Count());
	}
	protected bool IdentityCurrent()
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || controller != m_Controller || controller.GetPlayerId() != m_iPlayer) return false;
		if (!m_Entity || controller.GetControlledEntity() != m_Entity || m_Entity.GetWorld() != m_World) return false;
		if (RplText(m_Entity) != m_sOwnRpl) return false;
		if (!m_OwnVon || m_Entity.FindComponent(SCR_VoNComponent) != m_OwnVon) return false;
		if (m_iPlayer == 1) return m_OwnTap.MatchesSource(m_OwnVon.m_OnCaptureVON, SCR_VoNComponent.GetOnVoNUsed());
		PlayerManager manager = GetGame().GetPlayerManager();
		return manager && m_ProxyA && manager.GetPlayerControlledEntity(1) == m_ProxyA
			&& RplText(m_ProxyA) == m_sProxyRpl
			&& m_ProxyA.GetWorld() == m_World && m_ProxyVon && m_ProxyA.FindComponent(SCR_VoNComponent) == m_ProxyVon
			&& m_OwnTap.MatchesReceive(m_OwnVon.m_OnReceivedVON) && m_ProxyTap.MatchesReceive(m_ProxyVon.m_OnReceivedVON);
	}
	//! Read-only health for the opt-in pulse; does not advance clocks, bind, log or claim native idle.
	bool RICON_PulseHealth(int now, string expectedRun, int requiredRole, out int remainingMs)
	{
		remainingMs = RICON_NativeRxGate.LIMIT_MS - (now - m_iT0);
		return m_bRunning && m_bBound && !m_bInvalid && m_sRun == expectedRun && m_iPlayer == requiredRole
			&& now >= m_iLastClock && now - m_iLastClock <= 500 && now >= m_iLastPoll && now - m_iLastPoll <= 500
			&& remainingMs > 0 && m_Journal && RICON_NativeRxGate.OrdinaryAllowed(m_Journal.Count()) && IdentityCurrent();
	}
	protected bool Bind(int now)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller) return false;
		int player = controller.GetPlayerId();
		if (!RICON_NativeRxGate.RoleAllowed(player)) return false;
		if (m_BlindBWindow && player != 2) { m_bInvalid = true; Stop("B_OBSERVE_REQUIRES_LOCAL_B"); return false; }
		IEntity entity = controller.GetControlledEntity();
		if (!entity || entity.GetWorld() != m_World) return false;
		SCR_VoNComponent ownVon = SCR_VoNComponent.Cast(entity.FindComponent(SCR_VoNComponent));
		if (!ownVon) return false;
		IEntity proxy; SCR_VoNComponent proxyVon;
		if (player == 2)
		{
			PlayerManager manager = GetGame().GetPlayerManager();
			if (manager) proxy = manager.GetPlayerControlledEntity(1);
			if (!proxy || proxy.GetWorld() != m_World) return false;
			proxyVon = SCR_VoNComponent.Cast(proxy.FindComponent(SCR_VoNComponent));
			if (!proxyVon || proxyVon == ownVon || proxyVon.m_OnReceivedVON == ownVon.m_OnReceivedVON) return false;
		}
		array<ref RICON_NativeRxEndpoint> catalog;
		if (!BuildCatalog(entity, catalog) || catalog.IsEmpty()) return false;
		string ownRpl = RplText(entity); string proxyRpl = RplText(proxy);
		if (ownRpl == "UNKNOWN" || (player == 2 && proxyRpl == "UNKNOWN")) return false;
		m_Controller = controller; m_Entity = entity; m_OwnVon = ownVon; m_iPlayer = player;
		m_sOwnRpl = ownRpl; m_sProxyRpl = proxyRpl;
		m_ProxyA = proxy; m_ProxyVon = proxyVon; m_Catalog = catalog;
		bool attached;
		if (player == 1) attached = m_OwnTap.AttachSource(this, ownVon.m_OnCaptureVON, SCR_VoNComponent.GetOnVoNUsed());
		else attached = m_OwnTap.AttachReceive(this, ownVon.m_OnReceivedVON, 2)
			&& m_ProxyTap.AttachReceive(this, proxyVon.m_OnReceivedVON, 1);
		if (!attached) { m_OwnTap.Detach(); m_ProxyTap.Detach(); return false; }
		m_bBound = true;
		Emit(now, "process", "BOUND", -1, -1, -1, "subscriptions=2,catalog=" + catalog.Count().ToString()
			+ ",ownRpl=" + RplText(entity) + ",proxyARpl=" + RplText(proxy) + ",callbackOwner=UNQUALIFIED");
		for (int c = 0; c < catalog.Count(); c++)
		{
			if (!Emit(now, "process", "BOUND_CATALOG", -1, c, catalog[c].m_Transceiver.GetFrequency(), EndpointText(catalog[c].m_Transceiver, c))) return false;
		}
		string vonRefProbe;
		if (System.GetCLIParam("riconNativeVoNRefProbe", vonRefProbe) && vonRefProbe == "1" && m_bRunning && IdentityCurrent())
		{
			string proxyToken = "NONE";
			if (m_ProxyVon) proxyToken = m_ProxyVon.ToString();
			PrintFormat("[RICON_VONREF] run=%1 localPlayer=%2 bindingGeneration=%3 ownRpl=%4 proxyARpl=%5 ownVoN=%6 proxyVoN=%7 scope=OPAQUE_MANAGED_TOKEN_ONLY",
				RICON_LiveText.Esc(m_sRun), m_iPlayer, Bit(m_bBound), m_sOwnRpl, m_sProxyRpl, m_OwnVon.ToString(), proxyToken);
		}
		return m_bRunning;
	}
	//! Только readonly reuse bound identity для passive placement; observer state не меняется.
	int RICON_PairPlacementBinding(int now, string run, BaseWorld world, PlayerController controller, IEntity entity)
	{
		if (world != m_World || controller != m_Controller || entity != m_Entity) return 0;
		int remaining;
		if (!RICON_PulseHealth(now, run, m_iPlayer, remaining)) return 0;
		return 1;
	}
	bool RICON_PairPlacementOwnRadioPosition(out vector position)
	{
		position = vector.Zero;
		if (!m_bBound || !IdentityCurrent() || m_Catalog.IsEmpty()) return false;
		BaseRadioComponent radio = m_Catalog[0].m_Radio;
		if (!radio || !radio.GetOwner() || radio.GetOwner().GetWorld() != m_World) return false;
		foreach (RICON_NativeRxEndpoint endpoint : m_Catalog) if (!endpoint || endpoint.m_Radio != radio) return false;
		vector matrix[4]; radio.GetOwner().GetWorldTransform(matrix); position = matrix[3];
		return RICON_Num.IsFinite(position[0]) && RICON_Num.IsFinite(position[1]) && RICON_Num.IsFinite(position[2]);
	}
	protected string EndpointText(BaseTransceiver transceiver, int index)
	{
		BaseRadioComponent radio; IEntity radioEntity;
		if (transceiver) radio = transceiver.GetRadio();
		if (radio) radioEntity = radio.GetOwner();
		int nativeIndex = -1;
		if (index >= 0 && index < m_Catalog.Count()) nativeIndex = m_Catalog[index].m_iNativeIndex;
		return string.Format("nativeRefLocal=%1,radioRefLocal=%2,radioOwnerRefLocal=%3,nativeIndex=%4,radioOwnerRpl=%5,identityScope=PROCESS_AND_BINDING_GENERATION", transceiver, radio, radioEntity, nativeIndex, RplText(radioEntity));
	}
	//! Live readonly, независимый от завершённого placement helper. Никаких callbacks/setters/clock advance.
	bool RICON_ReadSeparatedGeometry(int now, string run, BaseWorld world, out RICON_SeparatedPairGeometry geometry)
	{
		geometry = null;
		if (!RICON_CaptureSeparatedGate.Requested() || world != m_World || !world || GetGame().GetWorld() != world || world.IsEditMode()) return false;
		if (!RICON_PairPlacementGate.WorldAllowed(GetGame().GetWorldFile())) return false;
		int remaining;
		if (!RICON_PulseHealth(now, run, m_iPlayer, remaining)) return false;
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager) return false;
		IEntity a = manager.GetPlayerControlledEntity(1); IEntity b = manager.GetPlayerControlledEntity(2);
		if (!a || !b || a == b || a.GetWorld() != world || b.GetWorld() != world) return false;
		string aRpl = RplText(a); string bRpl = RplText(b);
		if (aRpl == "UNKNOWN" || bRpl == "UNKNOWN") return false;
		if ((m_iPlayer == 1 && a != m_Entity) || (m_iPlayer == 2 && (b != m_Entity || a != m_ProxyA))) return false;
		ChimeraCharacter characterA = ChimeraCharacter.Cast(a); ChimeraCharacter characterB = ChimeraCharacter.Cast(b);
		if (!characterA || !characterB || !characterA.GetCharacterController() || !characterB.GetCharacterController()) return false;
		if (characterA.GetCharacterController().GetLifeState() != ECharacterLifeState.ALIVE || characterB.GetCharacterController().GetLifeState() != ECharacterLifeState.ALIVE) return false;
		if (a.GetRootParent() != a || b.GetRootParent() != b) return false;
		SCR_CompartmentAccessComponent ca = SCR_CompartmentAccessComponent.Cast(a.FindComponent(SCR_CompartmentAccessComponent));
		SCR_CompartmentAccessComponent cb = SCR_CompartmentAccessComponent.Cast(b.FindComponent(SCR_CompartmentAccessComponent));
		if ((ca && ca.GetVehicle()) || (cb && cb.GetVehicle())) return false;
		RICON_SeparatedPairGeometry observed = new RICON_SeparatedPairGeometry();
		vector aMatrix[4]; vector bMatrix[4]; a.GetWorldTransform(aMatrix); b.GetWorldTransform(bMatrix);
		observed.APosition = aMatrix[3]; observed.BPosition = bMatrix[3];
		if (!RICON_PairPlacementGate.Finite(observed.APosition) || !RICON_PairPlacementGate.Finite(observed.BPosition)) return false;
		if (!RICON_PairPlacementOwnRadioPosition(observed.OwnRadioPosition)) return false;
		observed.CatalogCount = m_Catalog.Count();
		foreach (RICON_NativeRxEndpoint endpoint : m_Catalog)
		{
			if (!endpoint || Match(endpoint.m_Transceiver) < 0) return false;
			if (!observed.NativeIndices.IsEmpty()) observed.NativeIndices = observed.NativeIndices + "|";
			observed.NativeIndices = observed.NativeIndices + endpoint.m_iNativeIndex.ToString();
		}
		float surfaceA = world.GetSurfaceY(observed.APosition[0], observed.APosition[2]);
		float surfaceB = world.GetSurfaceY(observed.BPosition[0], observed.BPosition[2]);
		if (!RICON_Num.IsFinite(surfaceA) || !RICON_Num.IsFinite(surfaceB)) return false;
		if (m_iPlayer == 1 && Replication.IsServer()) observed.View = "A_SERVER";
		else if (m_iPlayer == 2 && Replication.IsClient() && !Replication.IsServer()) observed.View = "B_LOCAL";
		else return false;
		if (manager.GetPlayerControlledEntity(1) != a || manager.GetPlayerControlledEntity(2) != b) return false;
		if (RplText(a) != aRpl || RplText(b) != bRpl || !IdentityCurrent()) return false;
		foreach (RICON_NativeRxEndpoint current : m_Catalog) if (!current || Match(current.m_Transceiver) < 0) return false;
		observed.ActorA = a; observed.ActorB = b; observed.ARpl = aRpl; observed.BRpl = bRpl; observed.Binding = 1;
		observed.ObservedAt = now;
		int completedAt = System.GetTickCount();
		if (completedAt < now || completedAt - now > 500) return false;
		observed.Eligible = RICON_CaptureSeparatedGate.Eligible(observed.APosition, observed.BPosition, observed.OwnRadioPosition, m_iPlayer, surfaceA, surfaceB);
		if (m_iPlayer == 1) observed.OwnRadioOffset = vector.Distance(observed.OwnRadioPosition, observed.APosition);
		else observed.OwnRadioOffset = vector.Distance(observed.OwnRadioPosition, observed.BPosition);
		geometry = observed;
		return true;
	}
	//! Reuse current player refs/GetOrigin; без I0 режима, radio permission или движения actors.
	bool RICON_ReadDirectNearGeometry(int now, string run, BaseWorld world, out RICON_SeparatedPairGeometry geometry)
	{
		geometry = null;
		if (!RICON_DirectCaptureGate.Requested() || !world || world != m_World || GetGame().GetWorld() != world || world.IsEditMode()) return false;
		if (!RICON_PairPlacementGate.WorldAllowed(GetGame().GetWorldFile())) return false;
		int remaining;
		if (!RICON_PulseHealth(now, run, m_iPlayer, remaining)) return false;
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager) return false;
		IEntity a = manager.GetPlayerControlledEntity(1); IEntity b = manager.GetPlayerControlledEntity(2);
		if (!a || !b || a == b || a.GetWorld() != world || b.GetWorld() != world) return false;
		string aRpl = RplText(a); string bRpl = RplText(b);
		if (aRpl == "UNKNOWN" || bRpl == "UNKNOWN") return false;
		if ((m_iPlayer == 1 && a != m_Entity) || (m_iPlayer == 2 && (b != m_Entity || a != m_ProxyA))) return false;
		ChimeraCharacter ca = ChimeraCharacter.Cast(a); ChimeraCharacter cb = ChimeraCharacter.Cast(b);
		if (!ca || !cb || !ca.GetCharacterController() || !cb.GetCharacterController()) return false;
		if (ca.GetCharacterController().GetLifeState() != ECharacterLifeState.ALIVE || cb.GetCharacterController().GetLifeState() != ECharacterLifeState.ALIVE) return false;
		if (a.GetRootParent() != a || b.GetRootParent() != b) return false;
		SCR_CompartmentAccessComponent va = SCR_CompartmentAccessComponent.Cast(a.FindComponent(SCR_CompartmentAccessComponent));
		SCR_CompartmentAccessComponent vb = SCR_CompartmentAccessComponent.Cast(b.FindComponent(SCR_CompartmentAccessComponent));
		if ((va && va.GetVehicle()) || (vb && vb.GetVehicle())) return false;
		RICON_SeparatedPairGeometry observed = new RICON_SeparatedPairGeometry();
		observed.APosition = a.GetOrigin(); observed.BPosition = b.GetOrigin();
		if (!RICON_PairPlacementGate.Finite(observed.APosition) || !RICON_PairPlacementGate.Finite(observed.BPosition)) return false;
		if (m_iPlayer == 1 && Replication.IsServer()) observed.View = "A_SERVER";
		else if (m_iPlayer == 2 && Replication.IsClient() && !Replication.IsServer()) observed.View = "B_LOCAL";
		else return false;
		if (manager.GetPlayerControlledEntity(1) != a || manager.GetPlayerControlledEntity(2) != b || RplText(a) != aRpl || RplText(b) != bRpl || !IdentityCurrent()) return false;
		observed.ActorA = a; observed.ActorB = b; observed.ARpl = aRpl; observed.BRpl = bRpl; observed.Binding = 1;
		observed.ObservedAt = now; observed.Eligible = RICON_DirectCaptureGate.Near(observed.APosition, observed.BPosition);
		int completed = System.GetTickCount();
		if (completed < now || completed - now > 500 || completed - now >= remaining) return false;
		geometry = observed;
		return true;
	}
	protected int Match(BaseTransceiver transceiver)
	{
		if (!transceiver) return -1;
		SCR_GadgetManagerComponent manager = SCR_GadgetManagerComponent.GetGadgetManager(m_Entity);
		for (int i = 0; i < m_Catalog.Count(); i++)
		{
			RICON_NativeRxEndpoint endpoint = m_Catalog[i];
			if (endpoint.m_Transceiver != transceiver) continue;
			if (!manager || !endpoint.m_Gadget || !manager.IsGadgetOwned(endpoint.m_Gadget) || !endpoint.m_Radio) return -1;
			IEntity radioEntity = endpoint.m_Gadget.GetOwner();
			if (!radioEntity || radioEntity.GetWorld() != m_World || radioEntity.FindComponent(BaseRadioComponent) != endpoint.m_Radio) return -1;
			if (endpoint.m_Radio.GetTransceiver(endpoint.m_iNativeIndex) != transceiver) return -1;
			return i;
		}
		return -1;
	}
	protected void ReadLocalEntry(RICON_RadioReadinessSnapshot snapshot)
	{
		SCR_VONController control = SCR_VONController.Cast(m_Controller.FindComponent(SCR_VONController));
		if (!control) return;
		SCR_VONEntry entry = control.GetActiveEntry();
		if (!entry) return;
		bool active = entry.IsActive(); bool usable = entry.IsUsable();
		SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(entry);
		BaseTransceiver tr;
		int index = -1; int nativeIndex = -1; int frequency = -1; bool refs;
		if (radioEntry)
		{
			tr = radioEntry.GetTransceiver(); index = Match(tr);
			if (index >= 0)
			{
				RICON_NativeRxEndpoint endpoint = m_Catalog[index];
				refs = radioEntry.GetGadget() == endpoint.m_Gadget && tr.GetRadio() == endpoint.m_Radio;
				refs = refs && radioEntry.GetTransceiverNumber() == endpoint.m_iNativeIndex + 1;
				nativeIndex = endpoint.m_iNativeIndex;
				frequency = tr.GetFrequency();
			}
		}
		if (GetGame().GetPlayerController() != m_Controller || m_Controller.FindComponent(SCR_VONController) != control
			|| control.GetActiveEntry() != entry || (radioEntry && radioEntry.GetTransceiver() != tr)) return;
		snapshot.EntryKnown = 1;
		snapshot.EntryRadio = RICON_RadioReadinessGate.Bit(true, radioEntry != null);
		snapshot.EntryIndex = index;
		snapshot.EntryNativeIndex = nativeIndex; snapshot.EntryFrequency = frequency;
		snapshot.EntryActive = RICON_RadioReadinessGate.Bit(true, active);
		snapshot.EntryUsable = RICON_RadioReadinessGate.Bit(true, usable);
		bool binding = RICON_RadioReadinessGate.BindingAllowed(true, radioEntry != null, index, active, refs);
		snapshot.EntryBindingAllowed = RICON_RadioReadinessGate.Bit(true, binding);
	}
	protected BaseRadioComponent UniqueRadio(array<ref RICON_NativeRxEndpoint> catalog, out int count)
	{
		count = -1;
		if (!catalog || catalog.Count() > 16) return null;
		array<BaseRadioComponent> radios = {};
		foreach (RICON_NativeRxEndpoint endpoint : catalog)
		{
			if (!endpoint || !endpoint.m_Radio) return null;
			if (radios.Find(endpoint.m_Radio) < 0) radios.Insert(endpoint.m_Radio);
		}
		count = radios.Count();
		if (count != 1) return null;
		return radios[0];
	}
	protected bool CatalogRefsCurrent(IEntity entity, array<ref RICON_NativeRxEndpoint> catalog)
	{
		if (!entity || entity.GetWorld() != m_World || !catalog || catalog.IsEmpty() || catalog.Count() > 16) return false;
		SCR_GadgetManagerComponent manager = SCR_GadgetManagerComponent.GetGadgetManager(entity);
		if (!manager) return false;
		foreach (RICON_NativeRxEndpoint endpoint : catalog)
		{
			if (!endpoint || !endpoint.m_Gadget || !endpoint.m_Radio || !endpoint.m_Transceiver) return false;
			if (!manager.IsGadgetOwned(endpoint.m_Gadget)) return false;
			IEntity owner = endpoint.m_Gadget.GetOwner();
			if (!owner || owner.GetWorld() != m_World || owner.FindComponent(BaseRadioComponent) != endpoint.m_Radio) return false;
			if (endpoint.m_Radio.GetTransceiver(endpoint.m_iNativeIndex) != endpoint.m_Transceiver) return false;
		}
		return true;
	}
	protected string CatalogFrequencies(array<ref RICON_NativeRxEndpoint> catalog)
	{
		string text;
		foreach (RICON_NativeRxEndpoint endpoint : catalog)
		{
			if (!text.IsEmpty()) text = text + "|";
			text = text + endpoint.m_Transceiver.GetFrequency().ToString();
		}
		return text;
	}
	protected void ReadPairKeys(RICON_RadioReadinessSnapshot snapshot)
	{
		if (snapshot.View == "UNQUALIFIED_VIEW") return;
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager) return;
		IEntity a = manager.GetPlayerControlledEntity(1); IEntity b = manager.GetPlayerControlledEntity(2);
		if (!a || !b || a == b || a.GetWorld() != m_World || b.GetWorld() != m_World) return;
		if ((m_iPlayer == 1 && a != m_Entity) || (m_iPlayer == 2 && (b != m_Entity || a != m_ProxyA))) return;
		string aRpl = RplText(a); string bRpl = RplText(b);
		if (aRpl == "UNKNOWN" || bRpl == "UNKNOWN") return;
		array<ref RICON_NativeRxEndpoint> aCatalog; array<ref RICON_NativeRxEndpoint> bCatalog;
		bool aKnown = BuildCatalog(a, aCatalog); bool bKnown = BuildCatalog(b, bCatalog);
		BaseRadioComponent aRadio; BaseRadioComponent bRadio;
		if (aKnown) aRadio = UniqueRadio(aCatalog, snapshot.UniqueA);
		if (bKnown) bRadio = UniqueRadio(bCatalog, snapshot.UniqueB);
		bool refs = aRadio && bRadio && CatalogRefsCurrent(a, aCatalog) && CatalogRefsCurrent(b, bCatalog);
		if (!RICON_RadioReadinessGate.PairKnown(true, aKnown && bKnown, snapshot.UniqueA, snapshot.UniqueB, refs)) return;
		string aKey = aRadio.GetEncryptionKey(); string bKey = bRadio.GetEncryptionKey();
		string aFrequencies = CatalogFrequencies(aCatalog); string bFrequencies = CatalogFrequencies(bCatalog);
		refs = manager.GetPlayerControlledEntity(1) == a && manager.GetPlayerControlledEntity(2) == b;
		refs = refs && RplText(a) == aRpl && RplText(b) == bRpl;
		refs = refs && CatalogRefsCurrent(a, aCatalog) && CatalogRefsCurrent(b, bCatalog);
		if (refs && IdentityCurrent())
		{
			snapshot.PairKnown = 1;
			snapshot.AEmpty = RICON_RadioReadinessGate.Bit(true, aKey.IsEmpty());
			snapshot.BEmpty = RICON_RadioReadinessGate.Bit(true, bKey.IsEmpty());
			snapshot.KeysEqual = RICON_RadioReadinessGate.Equality(true, aKey, bKey);
			snapshot.AFrequencies = aFrequencies; snapshot.BFrequencies = bFrequencies;
			snapshot.AOwnerRpl = RplText(aRadio.GetOwner()); snapshot.BOwnerRpl = RplText(bRadio.GetOwner());
		}
		aKey = ""; bKey = "";
	}
	protected string RadioReadinessText()
	{
		RICON_RadioReadinessSnapshot snapshot = new RICON_RadioReadinessSnapshot();
		snapshot.View = RICON_RadioReadinessGate.View(m_iPlayer, Replication.IsServer(), Replication.IsClient());
		ReadLocalEntry(snapshot); ReadPairKeys(snapshot);
		if (!IdentityCurrent())
		{
			string view = snapshot.View;
			snapshot = new RICON_RadioReadinessSnapshot(); snapshot.View = view;
		}
		else snapshot.OwnIdentityCurrent = 1;
		return snapshot.Text();
	}
	protected bool CallbackReady(RICON_NativeRxTap tap, int now)
	{
		if (!m_bRunning || !m_bBound || !tap || tap.Subscriptions() == 0) return false;
		if (tap != m_OwnTap && tap != m_ProxyTap) return false;
		if (RICON_NativeRxGate.ClockGap(m_iLastClock, now)) { m_bInvalid = true; Stop("CLOCK_OR_STALL"); return false; }
		m_iLastClock = now;
		if (RICON_NativeRxGate.Expired(now - m_iT0)) { Stop("LIMIT"); return false; }
		return true;
	}
	protected string OwnerText(RICON_NativeRxTap tap)
	{
		IEntity owner = m_Entity;
		if (tap == m_ProxyTap) owner = m_ProxyA;
		return "sourceOwnerPlayer=" + tap.OwnerPlayer().ToString() + ",sourceOwnerRpl=" + RplText(owner)
			+ ",subscriptionGeneration=" + tap.Generation().ToString() + ",catalogScope=OWN_HANDHELD_BACKPACK";
	}
	//! One public component-index observation; opaque tokens do not establish a native destination.
	protected void ObserveSndIndices(int now, IEntity entity, string role, string expectedRpl)
	{
		if (!IdentityCurrent() || !entity || entity.GetWorld() != m_World || RplText(entity) != expectedRpl) return;
		SndComponent snd = SndComponent.Cast(entity.FindComponent(SndComponent));
		int gainIndex = -2; int qualityIndex = -2;
		string token = "NONE";
		if (snd)
		{
			gainIndex = snd.GetSignalIndex("RICON_RadioOnlyProbeGain");
			qualityIndex = snd.GetSignalIndex("TransmissionQuality");
			token = snd.ToString();
			if (token.Length() > 128) token = token.Substring(0, 128);
		}
		if (!IdentityCurrent() || entity.GetWorld() != m_World || RplText(entity) != expectedRpl
			|| entity.FindComponent(SndComponent) != snd) return;
		string fields = "run=" + RICON_LiveText.Esc(m_sRun) + ",localPlayer=2,bindingGeneration=1,sender=1,rawTickMs=" + now.ToString();
		fields = fields + ",role=" + role + ",entityRpl=" + expectedRpl + ",ownRpl=" + m_sOwnRpl + ",proxyARpl=" + m_sProxyRpl;
		fields = fields + ",component=SndComponent,componentPresent=" + Bit(snd != null) + ",componentRef=" + RICON_LiveText.Esc(token);
		fields = fields + ",gainIndex=" + gainIndex.ToString() + ",qualityIndex=" + qualityIndex.ToString();
		fields = fields + ",identityCurrent=1,indexMinus2=NOT_QUERIED,indexMinus1=NOT_FOUND,scope=OPAQUE_MANAGED_TOKEN_ONLY,privateDestination=UNQUALIFIED";
		PrintFormat("[RICON_SNDIDX] %1", fields);
	}
	void Receive(RICON_NativeRxTap tap, int sender, BaseTransceiver receiver, int frequency, float quality)
	{
		int now = System.GetTickCount();
		if (!CallbackReady(tap, now)) return;
		m_iCallbacks++;
		bool current = IdentityCurrent(); int index = Match(receiver);
		bool finite = RICON_Num.IsFinite(quality); string rawQuality = "NONFINITE";
		if (finite) rawQuality = quality.ToString(-1, 9);
		string fields = OwnerText(tap) + ",contextValid=" + Bit(current) + ",senderA=" + Bit(sender == 1);
		fields = fields + ",receiverPresent=" + Bit(receiver != null) + ",catalogIndex=" + index.ToString();
		fields = fields + "," + EndpointText(receiver, index);
		fields = fields + ",route=" + RICON_NativeRxGate.Route(receiver != null, index >= 0);
		fields = fields + ",quality=" + rawQuality + ",qualityValid=" + Bit(RICON_NativeRxGate.QualityValid(quality));
		fields = fields + ",direct=UNQUALIFIED,delivery=NOT_TESTED";
		string owner = "ownB"; if (tap == m_ProxyTap) owner = "sourceAproxy";
		bool emitted = Emit(now, owner, "RAW_RX", sender, index, frequency, fields);
		string sndIndexProbe;
		if (!m_bSndIndexObserved && System.GetCLIParam("riconNativeSndIndexProbe", sndIndexProbe) && sndIndexProbe == "1"
			&& emitted && m_iPlayer == 2 && tap == m_OwnTap && tap.OwnerPlayer() == 2 && tap.Generation() == 1
			&& current && receiver && index >= 0 && sender == 1 && RICON_NativeRxGate.QualityValid(quality))
		{
			m_bSndIndexObserved = true;
			IEntity sourceA = m_ProxyA; IEntity ownB = m_Entity;
			ObserveSndIndices(now, sourceA, "sourceAproxy", m_sProxyRpl);
			ObserveSndIndices(now, ownB, "ownB", m_sOwnRpl);
		}
		if (m_BlindBWindow && emitted && m_iPlayer == 2 && tap == m_OwnTap && tap.OwnerPlayer() == 2 && tap.Generation() == 1
			&& current && receiver && index == 0 && m_Catalog.Count() > 0 && m_Catalog[0].m_iNativeIndex == 0
			&& sender == 1 && frequency == 32000 && RICON_NativeRxGate.QualityValid(quality))
			m_BlindBWindow.FirstRx(now, true);
		if (!current || !RICON_NativeRxGate.QualityValid(quality) || sender != 1 || (receiver && index < 0))
		{
			m_bInvalid = true; Stop("RX_CONTEXT_OR_ARGUMENT");
		}
	}
	void Capture(RICON_NativeRxTap tap, BaseTransceiver transmitter)
	{
		int now = System.GetTickCount();
		if (!CallbackReady(tap, now)) return;
		m_iCallbacks++;
		bool current = IdentityCurrent(); int index = Match(transmitter); int frequency = -1;
		if (transmitter) frequency = transmitter.GetFrequency();
		string fields = OwnerText(tap) + ",contextValid=" + Bit(current) + ",transmitterPresent=" + Bit(transmitter != null);
		fields = fields + ",catalogIndex=" + index.ToString() + ",directCapture=" + Bit(transmitter == null) + ",media=NOT_TESTED";
		fields = fields + "," + EndpointText(transmitter, index);
		Emit(now, "ownA", "RAW_CAPTURE", 1, index, frequency, fields);
		if (!current || (transmitter && index < 0)) { m_bInvalid = true; Stop("CAPTURE_CONTEXT"); }
	}
	void Used(RICON_NativeRxTap tap, int sender)
	{
		int now = System.GetTickCount();
		if (!CallbackReady(tap, now)) return;
		m_iCallbacks++;
		bool current = IdentityCurrent();
		Emit(now, "server", "RAW_USED", sender, -1, -1, "scope=SERVER_GLOBAL,contextValid=" + Bit(current)
			+ ",senderA=" + Bit(sender == 1) + ",delivery=NOT_TESTED");
		if (!current || sender != 1) { m_bInvalid = true; Stop("USED_CONTEXT_OR_SENDER"); }
	}
	protected string SourceSignalText()
	{
		string token = m_SourceSignalManager.ToString();
		if (token.Length() > 128) token = token.Substring(0, 128);
		return ",sourceSignalReady=1,sourceSignalValue=1,sourceSignalIndex=" + m_iSourceSignalIndex.ToString()
			+ ",sourceSignalRpl=" + m_sProxyRpl + ",sourceSignalManager=" + RICON_LiveText.Esc(token);
	}
	protected bool SourceSignalReady(int now)
	{
		if (!m_bSourceSignalPreregister) return true;
		if (m_iPlayer != 2 || Replication.IsServer() || !Replication.IsClient() || !IdentityCurrent())
		{ m_bInvalid = true; Stop("SOURCE_SIGNAL_CONTEXT"); return false; }
		SignalsManagerComponent manager = SignalsManagerComponent.Cast(m_ProxyA.FindComponent(SignalsManagerComponent));
		if (!manager) { m_bInvalid = true; Stop("SOURCE_SIGNAL_MANAGER_MISSING"); return false; }
		string signalName = "RICON_RadioOnlyProbeGain";
		bool first = !m_bSourceSignalAttempted;
		if (first)
		{
			m_bSourceSignalAttempted = true;
			int found = manager.FindSignal(signalName);
			if (found != -1 || m_iCallbacks != 0 || !IdentityCurrent() || m_ProxyA.FindComponent(SignalsManagerComponent) != manager)
			{ m_bInvalid = true; Stop("SOURCE_SIGNAL_PREREG_PRECONDITION"); return false; }
			m_SourceSignalManager = manager;
			m_iSourceSignalIndex = manager.AddOrFindSignal(signalName, 1);
		}
		if (manager != m_SourceSignalManager || m_iSourceSignalIndex < 0 || manager.FindSignal(signalName) != m_iSourceSignalIndex
			|| !IdentityCurrent() || m_ProxyA.FindComponent(SignalsManagerComponent) != manager)
		{ m_bInvalid = true; Stop("SOURCE_SIGNAL_BINDING"); return false; }
		float value = manager.GetSignalValue(m_iSourceSignalIndex);
		if (!RICON_Num.IsFinite(value) || value != 1 || !IdentityCurrent() || m_ProxyA.FindComponent(SignalsManagerComponent) != manager || (first && m_iCallbacks != 0))
		{ m_bInvalid = true; Stop("SOURCE_SIGNAL_VALUE"); return false; }
		if (first) return Emit(now, "sourceAproxy", "SOURCE_SIGNAL_PREREG_READY", 1, -1, -1,
			"contextValid=1,currentIdentity=1,firstRxSeen=0,preFindResult=-1,newness=NEGATIVE_PREFIND_ONLY,ownership=UNQUALIFIED,addIndex=" + m_iSourceSignalIndex.ToString()
			+ ",sourceSignalName=RICON_RadioOnlyProbeGain,ownRpl=" + m_sOwnRpl
			+ ",proxyARpl=" + m_sProxyRpl + SourceSignalText());
		return true;
	}
	void Tick(BaseWorld world, float timeSlice)
	{
		if (!m_bRunning) return;
		int now = System.GetTickCount();
		if (!world || world != m_World || world.IsEditMode()) { m_bInvalid = true; Stop("WORLD_CHANGED"); return; }
		if (RICON_NativeRxGate.ClockGap(m_iLastClock, now)) { m_bInvalid = true; Stop("CLOCK_OR_STALL"); return; }
		m_iLastClock = now;
		if (RICON_NativeRxGate.Expired(now - m_iT0)) { Stop("LIMIT"); return; }
		int pollMs = 500;
		if (m_BlindBWindow) pollMs = m_BlindBWindow.PollMs();
		if (now - m_iLastPoll < pollMs) return;
		m_iLastPoll = now;
		if (!m_bBound) { Bind(now); return; }
		if (!IdentityCurrent()) { m_bInvalid = true; Stop("BINDING_CHANGED"); return; }
		array<ref RICON_NativeRxEndpoint> current;
		if (!BuildCatalog(m_Entity, current) || current.Count() != m_Catalog.Count()) { m_bInvalid = true; Stop("CATALOG_CHANGED"); return; }
		for (int i = 0; i < current.Count(); i++)
		{
			if (current[i].m_Gadget != m_Catalog[i].m_Gadget || current[i].m_Transceiver != m_Catalog[i].m_Transceiver
				|| current[i].m_Radio != m_Catalog[i].m_Radio || current[i].m_iNativeIndex != m_Catalog[i].m_iNativeIndex)
			{
				m_bInvalid = true; Stop("CATALOG_CHANGED"); return;
			}
		}
		if (!SourceSignalReady(now)) return;
		int heartbeatMs = 1000;
		bool fastHeartbeat = m_BlindBWindow && m_BlindBWindow.Active();
		if (m_BlindBWindow) heartbeatMs = m_BlindBWindow.HeartbeatMs();
		if (now - m_iLastHeartbeat >= heartbeatMs)
		{
			m_iLastHeartbeat = now;
			string fields = "contextValid=1,catalog=" + m_Catalog.Count().ToString() + ",callbacksSeen=" + m_iCallbacks.ToString();
			fields = fields + ",callbackOwner=UNQUALIFIED,delivery=NOT_TESTED," + RadioReadinessText();
			if (m_bSourceSignalPreregister) fields = fields + SourceSignalText();
			int heartbeatTick = now;
			if (fastHeartbeat)
			{
				heartbeatTick = System.GetTickCount();
				if (RICON_NativeRxGate.ClockGap(m_iLastClock, heartbeatTick)) { m_bInvalid = true; Stop("CLOCK_OR_STALL"); return; }
				if (RICON_NativeRxGate.Expired(heartbeatTick - m_iT0)) { m_bInvalid = true; Stop("LIMIT"); return; }
				m_iLastClock = heartbeatTick; m_iLastHeartbeat = heartbeatTick;
			}
			bool heartbeatWritten = Emit(heartbeatTick, "process", "HEARTBEAT", -1, -1, -1, fields);
			if (fastHeartbeat && heartbeatWritten) m_BlindBWindow.HeartbeatWritten(heartbeatTick);
			if (RICON_DirectCaptureGate.Requested())
			{
				RICON_SeparatedPairGeometry nearGeometry;
				string nearText = "geoView=UNKNOWN,geoKnown=0,geoEligible=0,geoScope=LIVE_REFERENCES";
				if (RICON_ReadDirectNearGeometry(System.GetTickCount(), m_sRun, m_World, nearGeometry)) nearText = RICON_DirectCaptureGate.Text(nearGeometry);
				Emit(System.GetTickCount(), "process", "LIVE_DIRECT_NEAR", -1, -1, -1, nearText);
			}
			if (RICON_CaptureSeparatedGate.Requested())
			{
				RICON_SeparatedPairGeometry geometry;
				string live = "geoView=UNKNOWN,geoKnown=0,geoEligible=0,geoScope=LIVE_REFERENCES";
				if (RICON_ReadSeparatedGeometry(System.GetTickCount(), m_sRun, m_World, geometry)) live = geometry.Text();
				Emit(System.GetTickCount(), "process", "LIVE_GEOMETRY", -1, -1, -1, live);
			}
		}
	}
	void Stop(string reason)
	{
		if (!m_bRunning) return;
		m_bRunning = false;
		if (m_OwnTap) m_OwnTap.Detach();
		if (m_ProxyTap) m_ProxyTap.Detach();
		int now = System.GetTickCount();
		if (reason == "OVERFLOW" || reason == "CLOCK_OR_STALL")
			Append(now, "process", "GAP", -1, -1, -1, "reason=" + reason + ",missing=UNKNOWN");
		string fields = "reason=" + reason + ",invalid=" + Bit(m_bInvalid) + ",bound=" + Bit(m_bBound);
		fields = fields + ",subscriptionsRemaining=0,callbacksSeen=" + m_iCallbacks.ToString() + ",delivery=NOT_TESTED";
		Append(now, "process", "TERMINAL", -1, -1, -1, fields);
		m_Catalog.Clear(); m_World = null; m_Controller = null; m_Entity = null; m_ProxyA = null;
		m_OwnVon = null; m_ProxyVon = null;
		m_SourceSignalManager = null; m_iSourceSignalIndex = -1; m_bSourceSignalPreregister = false;
	}
}

//! TOOL_ONLY initial B loadout change; resource/ownership/radio qualification only.
modded class SCR_SpawnPointRespawnRequestComponent
{
	protected bool m_bRICONOwnReceiverPrefabUsed;

	protected static bool RICON_OwnReceiverIncomingAllowed(ResourceName resource)
	{
		return resource == "{C5574DAD3956486C}Prefabs/Characters/Campaign/Final/BLUFOR/US_army/Regular/Campaign_US_Player_Rifleman.et"
			|| resource == "{33CDB2B7C3F1EB63}Prefabs/Characters/Campaign/Final/BLUFOR/US_army/Regular/Campaign_US_Player_AR.et"
			|| resource == "{61DA6CADADBEFF80}Prefabs/Characters/Campaign/Final/BLUFOR/US_army/Regular/Campaign_US_Player_MG.et"
			|| resource == "{3E18CC9634468249}Prefabs/Characters/Campaign/Final/BLUFOR/US_army/Regular/Campaign_US_Player_GL.et"
			|| resource == "{9F4976C06FD1C839}Prefabs/Characters/Campaign/Final/BLUFOR/US_army/Regular/Campaign_US_Player_LAT.et"
			|| resource == "{E27CC9493BFAF668}Prefabs/Characters/Campaign/Final/BLUFOR/US_army/Regular/Campaign_US_Player_Sniper.et";
	}

	protected bool RICON_RejectOwnReceiverPrefab(string reason)
	{
		PrintFormat("[RICON_OWN_PREFAB] event=REFUSED,player=2,reason=%1,scope=INITIAL_RESOURCE_ONLY", reason);
		return false;
	}

	override bool DoRequestRespawn(SCR_SpawnData data)
	{
		string flag;
		if (!System.GetCLIParam("riconOwnReceiverPrefab", flag) || flag != "1" || !Replication.IsServer())
			return super.DoRequestRespawn(data);

		PlayerController controller = GetPlayerController();
		if (controller && controller.GetPlayerId() != 2)
			return super.DoRequestRespawn(data);
		if (m_bRICONOwnReceiverPrefabUsed)
			return false;
		m_bRICONOwnReceiverPrefabUsed = true;

		if (!controller)
			return RICON_RejectOwnReceiverPrefab("CONTROLLER_MISSING");
		string run;
		if (!System.GetCLIParam("riconNativeRxRun", run) || !RICON_D0Gate.ValidRunId(run))
			return RICON_RejectOwnReceiverPrefab("RUN_INVALID");
		if (!GetGame() || !RICON_PairPlacementGate.WorldAllowed(GetGame().GetWorldFile()))
			return RICON_RejectOwnReceiverPrefab("WORLD_NOT_MPTEST");
		PlayerManager manager = GetGame().GetPlayerManager();
		RespawnComponent respawn = controller.GetRespawnComponent();
		if (!manager || manager.GetPlayerController(2) != controller || !respawn || manager.GetPlayerRespawnComponent(2) != respawn)
			return RICON_RejectOwnReceiverPrefab("CONTROLLER_RESPAWN_MISMATCH");
		if (controller.GetControlledEntity())
			return RICON_RejectOwnReceiverPrefab("ALREADY_CONTROLLED");

		SCR_SpawnPointSpawnData original = SCR_SpawnPointSpawnData.Cast(data);
		if (!original || !original.GetSpawnPoint())
			return RICON_RejectOwnReceiverPrefab("SPAWN_POINT_DATA_INVALID");
		ResourceName incoming = original.GetPrefab();
		if (!RICON_OwnReceiverIncomingAllowed(incoming))
			return RICON_RejectOwnReceiverPrefab("INCOMING_NOT_US_FIXTURE");
		ResourceName childName = "{0DE75817F6D87DD2}Prefabs/RICON_VoNProbeCharacter.et";
		Resource child = Resource.Load(childName);
		if (!child || !child.IsValid())
			return RICON_RejectOwnReceiverPrefab("CHILD_RESOURCE_INVALID");

		SCR_SpawnPointSpawnData selected = new SCR_SpawnPointSpawnData(childName, original.GetRplId());
		bool dispatched = super.DoRequestRespawn(selected);
		PrintFormat("[RICON_OWN_PREFAB] event=SELECT,run=%1,player=2,incoming=%2,child=%3,spawnPoint=%4,dispatched=%5,meaning=DISPATCH_ONLY,loadoutChange=EXPLICIT", run, incoming, childName, original.GetRplId().ToString(), dispatched);
		return dispatched;
	}
}
