//! TOOL_ONLY I0: bounded getter-only mapping inspection. No callbacks, RPC, emitters or setters.
class RICON_I0Budget
{
	static const int ORDINARY_LIMIT = 1024;
	static const int RESERVED_LIMIT = 64;
	static const int TOTAL_LIMIT = 1088;
	protected int m_iOrdinary;
	protected int m_iReserved;
	bool TakeOrdinary()
	{
		if (m_iOrdinary >= ORDINARY_LIMIT || m_iOrdinary + m_iReserved >= TOTAL_LIMIT) return false;
		m_iOrdinary++; return true;
	}
	bool TakeReserved()
	{
		if (m_iReserved >= RESERVED_LIMIT || m_iOrdinary + m_iReserved >= TOTAL_LIMIT) return false;
		m_iReserved++; return true;
	}
	int Ordinary() { return m_iOrdinary; }
	int Reserved() { return m_iReserved; }
	int Total() { return m_iOrdinary + m_iReserved; }
}

class RICON_I0Gate
{
	static bool ResolveRequested(bool configured, string value) { return configured && value == "1"; }
	static bool ModesExclusive(bool i0, bool d0, bool v0) { return i0 && !d0 && !v0; }
	static bool IdentityExpired(int elapsedMs) { return elapsedMs > 20000; }
	static bool WindowExpired(int elapsedMs) { return elapsedMs >= 45000; }
	static bool CanSnapshot(int now, int previous, int count)
	{
		return count < 16 && now >= previous && now - previous >= 3000;
	}
	static int MatchState(bool left, bool right, bool same)
	{
		if (!left || !right) return 0;
		if (same) return 1;
		return 2;
	}
	static bool NonNullMatch(bool left, bool right, bool same) { return MatchState(left, right, same) == 1; }
	static bool IdentityStable(bool ready, bool controller, bool entity, bool component)
	{
		return !ready || (controller && entity && component);
	}
	static bool EntryBindingAllowed(bool selectedRef, bool activeFlag, bool transceiverNonNull)
	{
		return selectedRef && activeFlag && transceiverNonNull;
	}
	static bool SameWorldGeometry(bool a, bool b, bool sameWorld) { return a && b && sameWorld; }
	static bool RplRoundTripAllowed(bool validId, bool roundSame) { return validId && roundSame; }
	static bool EntriesAllowed(int preCount, int actualCount, int returnedCount)
	{
		return preCount >= 0 && preCount <= 8 && actualCount == preCount && returnedCount == actualCount;
	}
	static bool CatalogAllowed(int gadgets, int transceivers) { return gadgets <= 4 && transceivers <= 16; }
}

class RICON_LiveVoNMappingInspection
{
	protected ref RICON_I0Budget m_Budget;
	protected BaseWorld m_World;
	protected PlayerController m_Controller;
	protected IEntity m_Entity;
	protected SCR_VONController m_VonController;
	protected SCR_VoNComponent m_LastEntityVon;
	protected SCR_VoNComponent m_LastActiveVon;
	protected SCR_VONEntry m_LastActiveEntry;
	protected BaseTransceiver m_LastActiveTransceiver;
	protected int m_iPlayerId = -1;
	protected int m_iT0 = -1;
	protected int m_iLastTick = -1;
	protected int m_iLastSnapshot = -3000;
	protected int m_iSnapshots;
	protected int m_iGeneration;
	protected int m_iSequence;
	protected bool m_bRunning;
	protected bool m_bIdentityReady;
	protected bool m_bMappingObserved;
	protected bool m_bInvalid;
	protected string m_sRunId;

	static bool IsRequested()
	{
		string value;
		bool configured = System.GetCLIParam("riconVoNMapping", value);
		bool i0 = RICON_I0Gate.ResolveRequested(configured, value);
		string d0; string v0;
		bool d0Configured = System.GetCLIParam("riconCommunicationDiscovery", d0);
		bool v0Configured = System.GetCLIParam("riconVoNInputShape", v0);
		bool d0On = RICON_D0Gate.ResolveRequested(d0Configured, d0);
		bool v0On = RICON_D0Gate.ResolveRequested(v0Configured, v0);
		if (!RICON_I0Gate.ModesExclusive(i0, d0On, v0On)) return false;
		string runId;
		return System.GetCLIParam("riconCommunicationRun", runId) && RICON_D0Gate.ValidRunId(runId);
	}

	protected int Now() { return System.GetTickCount(); }
	protected static string Bit(bool value)
	{
		if (value)
			return "1";
		else
			return "0";
	}

	protected bool Emit(string kind, string fields = "", bool reserved = false)
	{
		bool accepted;
		if (reserved) accepted = m_Budget.TakeReserved(); else accepted = m_Budget.TakeOrdinary();
		if (!accepted) { m_bInvalid = true; return false; }
		string line = "[RICON_I0] seq=" + m_iSequence.ToString() + " t=" + Now().ToString();
		m_iSequence++;
		line = line + " run=" + m_sRunId + " player=" + m_iPlayerId.ToString();
		line = line + " gen=" + m_iGeneration.ToString() + " kind=" + kind + " " + fields;
		PrintFormat("%1", line);
		return true;
	}

	bool Start(BaseWorld world)
	{
		if (m_bRunning || !world || world.IsEditMode()) return false;
		string runId;
		if (!System.GetCLIParam("riconCommunicationRun", runId) || !RICON_D0Gate.ValidRunId(runId)) return false;
		m_sRunId = runId; m_World = world; m_Budget = new RICON_I0Budget();
		m_iT0 = Now(); if (m_iT0 < 0) return false;
		m_iLastTick = m_iT0; m_iLastSnapshot = m_iT0 - 3000; m_bRunning = true;
		Emit("START", "subscriptions=0,bulkGadgetAllocation=UNKNOWN", true);
		return true;
	}

	protected bool ResolveIdentity()
	{
		PlayerController controller = GetGame().GetPlayerController();
		IEntity entity; int playerId = -1;
		if (controller) { entity = controller.GetControlledEntity(); playerId = controller.GetPlayerId(); }
		SCR_VONController vonController;
		if (controller) vonController = SCR_VONController.Cast(controller.FindComponent(SCR_VONController));
		bool complete = controller && entity && vonController && (playerId == 1 || playerId == 2);
		if (!complete)
		{
			if (m_bIdentityReady) { m_bInvalid = true; Stop("IDENTITY_LOST"); }
			return false;
		}
		if (!m_bIdentityReady)
		{
			m_Controller = controller; m_Entity = entity; m_VonController = vonController; m_iPlayerId = playerId;
			m_iGeneration++; m_bIdentityReady = true;
			Emit("IDENTITY", "ready=1");
			return true;
		}
		if (controller != m_Controller || entity != m_Entity || vonController != m_VonController || playerId != m_iPlayerId)
		{
			m_bInvalid = true; Stop("GENERATION_CHANGED"); return false;
		}
		return true;
	}

	protected string RplText(IEntity entity)
	{
		if (!entity) return "entity=0,rpl=UNKNOWN";
		RplId id = Replication.FindItemId(entity);
		bool validId = id != RplId.Invalid();
		if (!validId) return "entity=1,rplId=INVALID,round=UNKNOWN";
		Managed round = Replication.FindItem(id);
		RplComponent rpl = RplComponent.Cast(entity.FindComponent(RplComponent));
		string text = "entity=1,rplId=" + string.Format("%1", id);
		if (!round) text = text + ",round=UNKNOWN"; else text = text + ",round=" + Bit(round == entity);
		if (!rpl) return text + ",rplComponent=0";
		text = text + ",owner=" + Bit(rpl.IsOwner()) + ",proxy=" + Bit(rpl.IsProxy());
		return text + ",master=" + Bit(rpl.IsMaster());
	}

	protected bool InspectEntries()
	{
		int preCount = m_VonController.GetVONEntryCount();
		if (preCount < 0 || preCount > 8)
		{
			Emit("CATALOG_LIMIT", "catalog=entries,pre=" + preCount.ToString(), true); return false;
		}
		array<ref SCR_VONEntry> entries = {};
		int returned = m_VonController.GetVONEntries(entries);
		if (!RICON_I0Gate.EntriesAllowed(preCount, entries.Count(), returned))
		{
			Emit("CATALOG_LIMIT", "catalog=entries,pre=" + preCount.ToString() + ",actual=" + entries.Count().ToString(), true);
			return false;
		}
		SCR_VONEntry active = m_VonController.GetActiveEntry();
		for (int i = 0; i < entries.Count(); i++)
		{
			SCR_VONEntry entry = entries[i];
			if (!entry) { Emit("ENTRY", "i=" + i.ToString() + ",null=1"); continue; }
			string text = "i=" + i.ToString() + ",activeRef=" + Bit(entry == active);
			text = text + ",method=" + entry.GetVONMethod().ToString() + ",usable=" + Bit(entry.IsUsable());
			text = text + ",activeFlag=" + Bit(entry.IsActive());
			SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(entry);
			if (radioEntry)
			{
				BaseTransceiver tr = radioEntry.GetTransceiver();
				SCR_GadgetComponent gadget = radioEntry.GetGadget();
				bool bindingAllowed = RICON_I0Gate.EntryBindingAllowed(entry == active, entry.IsActive(), tr != null);
				text = text + ",radio=1,muted=" + Bit(radioEntry.GetIsMuted());
				text = text + ",entryNumber1=" + radioEntry.GetTransceiverNumber().ToString();
				text = text + ",trRef=" + Bit(tr != null) + ",gadget=" + Bit(gadget != null);
				SCR_VoNComponent controllerComponent = m_VonController.GetVONComponent();
				SCR_VoNComponent entityComponent = SCR_VoNComponent.Cast(m_Entity.FindComponent(SCR_VoNComponent));
				BaseTransceiver controllerTransmit; BaseTransceiver entityTransmit;
				if (controllerComponent) controllerTransmit = controllerComponent.GetTransmitRadio();
				if (entityComponent) entityTransmit = entityComponent.GetTransmitRadio();
				int controllerTxState; int entityTxState;
				if (bindingAllowed)
				{
					controllerTxState = RICON_I0Gate.MatchState(true, controllerTransmit != null, tr == controllerTransmit);
					entityTxState = RICON_I0Gate.MatchState(true, entityTransmit != null, tr == entityTransmit);
				}
				text = text + ",bindingAllowed=" + Bit(bindingAllowed);
				text = text + ",controllerTxState=" + controllerTxState.ToString();
				text = text + ",entityTxState=" + entityTxState.ToString();
				int nativeIndex = -1; int nativeCount = -1;
				if (bindingAllowed && gadget && gadget.GetOwner())
				{
					BaseRadioComponent radio = BaseRadioComponent.Cast(gadget.GetOwner().FindComponent(BaseRadioComponent));
					if (radio)
					{
						nativeCount = radio.TransceiversCount();
						if (nativeCount > 4) { Emit("CATALOG_LIMIT", "catalog=transceivers,count=" + nativeCount.ToString(), true); return false; }
						for (int t = 0; t < nativeCount; t++) if (radio.GetTransceiver(t) == tr) nativeIndex = t;
					}
				}
				text = text + ",nativeIndex0=" + nativeIndex.ToString() + ",nativeCount=" + nativeCount.ToString();
			}
			else text = text + ",radio=0";
			Emit("ENTRY", text);
		}
		return true;
	}

	protected bool InspectGadgetCaps()
	{
		SCR_GadgetManagerComponent manager = SCR_GadgetManagerComponent.GetGadgetManager(m_Entity);
		if (!manager) { Emit("GADGETS", "manager=0,bulkAllocation=UNKNOWN"); return true; }
		array<SCR_GadgetComponent> handheld = manager.GetGadgetsByType(EGadgetType.RADIO);
		array<SCR_GadgetComponent> backpack = manager.GetGadgetsByType(EGadgetType.RADIO_BACKPACK);
		int count; if (handheld) count += handheld.Count(); if (backpack) count += backpack.Count();
		if (count > 4) { Emit("CATALOG_LIMIT", "catalog=gadgets,count=" + count.ToString() + ",bulkAllocation=UNKNOWN", true); return false; }
		Emit("GADGETS", "count=" + count.ToString() + ",bulkAllocation=UNKNOWN"); return true;
	}

	protected void Snapshot(int now)
	{
		m_iSnapshots++; m_iLastSnapshot = now;
		SCR_VoNComponent entityVon = SCR_VoNComponent.Cast(m_Entity.FindComponent(SCR_VoNComponent));
		SCR_VoNComponent activeVon = m_VonController.GetVONComponent();
		SCR_VONEntry activeEntry = m_VonController.GetActiveEntry();
		SCR_VONEntryRadio activeRadio = SCR_VONEntryRadio.Cast(activeEntry);
		BaseTransceiver activeTransceiver;
		if (activeRadio && activeEntry.IsActive()) activeTransceiver = activeRadio.GetTransceiver();
		if (m_bMappingObserved && (entityVon != m_LastEntityVon || activeVon != m_LastActiveVon
			|| activeEntry != m_LastActiveEntry || activeTransceiver != m_LastActiveTransceiver))
		{
			m_bInvalid = true; Emit("MAPPING_CHANGED", "generation=" + m_iGeneration.ToString(), true);
		}
		m_LastEntityVon = entityVon; m_LastActiveVon = activeVon; m_LastActiveEntry = activeEntry;
		m_LastActiveTransceiver = activeTransceiver; m_bMappingObserved = true;
		int matchState = RICON_I0Gate.MatchState(entityVon != null, activeVon != null, entityVon == activeVon);
		string text = "snapshot=" + m_iSnapshots.ToString() + ",disabled=" + Bit(m_VonController.IsVONDisabled());
		text = text + ",entityVon=" + Bit(entityVon != null) + ",activeVon=" + Bit(activeVon != null);
		text = text + ",vonMatchState=" + matchState.ToString() + ",scriptActive=" + Bit(m_VonController.RICON_I0ScriptActive());
		text = text + ",toggledDirect=" + Bit(m_VonController.RICON_I0ToggledDirect());
		text = text + ",vonType=" + m_VonController.RICON_I0TransmitType().ToString();
		text = text + ",hold=" + RICON_LiveText.Esc(m_VonController.RICON_I0ActiveHoldAction());
		Emit("SNAPSHOT", text);
		PlayerManager pm = GetGame().GetPlayerManager(); IEntity a; IEntity b;
		if (pm) { a = pm.GetPlayerControlledEntity(1); b = pm.GetPlayerControlledEntity(2); }
		bool sameWorld = a && b && a.GetWorld() == m_World && b.GetWorld() == m_World;
		string geometry = "a=" + Bit(a != null) + ",b=" + Bit(b != null) + ",sameWorld=" + Bit(sameWorld);
		if (sameWorld)
		{
			vector originA = a.GetOrigin(); vector originB = b.GetOrigin();
			geometry = geometry + ",originA=" + originA.ToString() + ",originB=" + originB.ToString();
			geometry = geometry + ",distance=" + vector.Distance(originA, originB).ToString();
		}
		else geometry = geometry + ",originA=UNKNOWN,originB=UNKNOWN,distance=UNKNOWN";
		Emit("GEOMETRY", geometry); Emit("RPL_A", RplText(a)); Emit("RPL_B", RplText(b));
		if (!InspectEntries() || !InspectGadgetCaps()) m_bInvalid = true;
	}

	void Tick(BaseWorld world, float timeSlice)
	{
		if (!m_bRunning) return;
		if (m_bInvalid) { Stop("INVALID_STATE"); return; }
		int now = Now();
		if (!world || world != m_World || world.IsEditMode()) { m_bInvalid = true; Stop("WORLD_CHANGED"); return; }
		if (now < m_iLastTick) { m_bInvalid = true; Stop("CLOCK_RESET"); return; }
		m_iLastTick = now; int elapsed = now - m_iT0;
		if (RICON_I0Gate.WindowExpired(elapsed)) { Stop("LIMIT"); return; }
		if (!ResolveIdentity())
		{
			if (!m_bRunning) return;
			if (RICON_I0Gate.IdentityExpired(elapsed)) { Stop("BLOCKED_IDENTITY"); return; }
			return;
		}
		if (RICON_I0Gate.CanSnapshot(now, m_iLastSnapshot, m_iSnapshots)) Snapshot(now);
	}

	void Stop(string reason)
	{
		if (!m_bRunning) return;
		string verdict = "SOURCE_MAPPING_OBSERVED"; if (!m_bIdentityReady) verdict = "BLOCKED"; if (m_bInvalid) verdict = "UNKNOWN_INCOMPLETE";
		Emit("VERDICT", "value=" + verdict + ",reason=" + reason, true);
		Emit("TERMINAL", "snapshots=" + m_iSnapshots.ToString() + ",subscriptions=0,invalid=" + Bit(m_bInvalid)
			+ ",ordinary=" + m_Budget.Ordinary().ToString() + ",reserved=" + m_Budget.Reserved().ToString(), true);
		m_bRunning = false; m_World = null; m_Controller = null; m_Entity = null; m_VonController = null;
	}
}
