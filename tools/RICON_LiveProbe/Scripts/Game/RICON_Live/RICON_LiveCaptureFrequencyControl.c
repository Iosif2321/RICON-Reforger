//! Frequency-only reuse V1 snapshot/match/readback/restore. Ни один ключ не записывается.
class RICON_CaptureFrequencyGate
{
	static int Target(int source, int current, int minimum, int maximum, int step)
	{
		if (current != source) return current;
		if (minimum < 0 || maximum < minimum || step <= 0 || step > maximum - minimum) return -1;
		if (current <= maximum - step && RICON_D0Gate.V1FrequencyFits(minimum, maximum, step, current + step)) return current + step;
		if (current >= minimum + step && RICON_D0Gate.V1FrequencyFits(minimum, maximum, step, current - step)) return current - step;
		return -1;
	}
	static bool AllMismatch(array<int> frequencies, int source)
	{
		if (!frequencies || frequencies.IsEmpty() || frequencies.Count() > 16 || source < 0) return false;
		foreach (int value : frequencies) if (value < 0 || value == source) return false;
		return true;
	}
	static bool CanRestore(bool refs, int current, int original, int target)
	{
		return refs && (current == original || current == target);
	}
}

class RICON_FrequencyOnlyAlignment
{
	protected IEntity m_Entity;
	protected SCR_GadgetComponent m_Gadget;
	protected BaseRadioComponent m_Radio;
	protected BaseTransceiver m_Transceiver;
	protected int m_Index;
	protected int m_Original;
	protected int m_Target;
	protected string m_PrivateKey;
	protected bool m_Powered;
	protected bool m_Muted;
	bool Capture(IEntity entity, SCR_GadgetComponent gadget, BaseRadioComponent radio, BaseTransceiver tr, int index, int target)
	{
		if (!entity || !gadget || !radio || !tr || target < 0) return false;
		m_Entity = entity; m_Gadget = gadget; m_Radio = radio; m_Transceiver = tr; m_Index = index;
		m_Original = tr.GetFrequency(); m_Target = target;
		m_PrivateKey = radio.GetEncryptionKey(); m_Powered = radio.IsPowered(); m_Muted = tr.IsMuted();
		return Matches(entity);
	}
	bool Matches(IEntity entity)
	{
		if (!entity || entity != m_Entity || !m_Gadget || !m_Radio || !m_Transceiver) return false;
		SCR_GadgetManagerComponent manager = SCR_GadgetManagerComponent.GetGadgetManager(entity);
		if (!manager || !manager.IsGadgetOwned(m_Gadget)) return false;
		IEntity owner = m_Gadget.GetOwner();
		return owner && owner.GetWorld() == entity.GetWorld() && owner.FindComponent(BaseRadioComponent) == m_Radio
			&& m_Transceiver.GetRadio() == m_Radio && m_Radio.GetTransceiver(m_Index) == m_Transceiver;
	}
	bool SameRef(RICON_FrequencyOnlyAlignment other)
	{
		return other && m_Entity == other.m_Entity && m_Gadget == other.m_Gadget && m_Radio == other.m_Radio
			&& m_Transceiver == other.m_Transceiver && m_Index == other.m_Index;
	}
	bool Apply()
	{
		if (!Matches(m_Entity)) return false;
		if (m_Transceiver.GetFrequency() != m_Original || !KeysSame() || !PowerMuteSame()) return false;
		if (m_Original != m_Target) m_Transceiver.SetFrequency(m_Target);
		return m_Transceiver.GetFrequency() == m_Target;
	}
	bool Restore(IEntity entity)
	{
		if (!Matches(entity)) return false;
		int current = m_Transceiver.GetFrequency();
		if (!RICON_CaptureFrequencyGate.CanRestore(true, current, m_Original, m_Target)) return false;
		if (current != m_Original) m_Transceiver.SetFrequency(m_Original);
		return m_Transceiver.GetFrequency() == m_Original;
	}
	int Frequency() { return m_Transceiver.GetFrequency(); }
	int Original() { return m_Original; }
	int Target() { return m_Target; }
	bool KeysSame() { return m_Radio.GetEncryptionKey() == m_PrivateKey; }
	bool PowerMuteSame() { return m_Radio.IsPowered() == m_Powered && m_Transceiver.IsMuted() == m_Muted; }
	void ClearPrivate() { m_PrivateKey = ""; }
}

//! Один server writer и пассивный B reader. Только explicit flag; это не B command/handshake.
class RICON_CaptureFrequencyControl
{
	protected static ref RICON_CaptureFrequencyControl s_Server;
	protected static ref array<ref RICON_FrequencyOnlyAlignment> s_BReadCatalog;
	protected static IEntity s_BReadEntity;
	protected static string s_BReadRpl;
	protected static int s_LastBRead = -1;
	protected static int s_LastPrepare = -1;
	protected BaseWorld m_World;
	protected PlayerController m_AController;
	protected PlayerController m_BController;
	protected IEntity m_AEntity;
	protected IEntity m_BEntity;
	protected string m_BRpl;
	protected ref RICON_FrequencyOnlyAlignment m_A;
	protected ref array<ref RICON_FrequencyOnlyAlignment> m_B = {};
	protected string m_Run;
	protected string m_OriginalVector;
	protected int m_SourceFrequency;
	protected int m_LastRead = -1;
	protected bool m_Applied;
	protected bool m_Restored;
	protected bool m_Invalid;
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
	protected static bool Catalog(IEntity entity, int source, bool detune, out array<ref RICON_FrequencyOnlyAlignment> result)
	{
		result = {};
		if (!entity) return false;
		SCR_GadgetManagerComponent manager = SCR_GadgetManagerComponent.GetGadgetManager(entity);
		if (!manager) return false;
		array<SCR_GadgetComponent> gadgets = RICON_NativeRxGate.CombineCategories(
			manager.GetGadgetsByType(EGadgetType.RADIO), manager.GetGadgetsByType(EGadgetType.RADIO_BACKPACK));
		if (gadgets.IsEmpty() || gadgets.Count() > 4) return false;
		array<BaseRadioComponent> unique = {};
		foreach (SCR_GadgetComponent gadget : gadgets)
		{
			if (!gadget || !gadget.GetOwner() || !manager.IsGadgetOwned(gadget)) return false;
			BaseRadioComponent radio = BaseRadioComponent.Cast(gadget.GetOwner().FindComponent(BaseRadioComponent));
			if (!radio || radio.TransceiversCount() < 1 || radio.TransceiversCount() > 4) return false;
			if (unique.Find(radio) < 0) unique.Insert(radio);
			for (int index = 0; index < radio.TransceiversCount(); index++)
			{
				BaseTransceiver tr = radio.GetTransceiver(index);
				if (!tr) return false;
				int target = tr.GetFrequency();
				if (detune) target = RICON_CaptureFrequencyGate.Target(source, target, tr.GetMinFrequency(), tr.GetMaxFrequency(), tr.GetFrequencyResolution());
				RICON_FrequencyOnlyAlignment endpoint = new RICON_FrequencyOnlyAlignment();
				if (!endpoint.Capture(entity, gadget, radio, tr, index, target)) return false;
				foreach (RICON_FrequencyOnlyAlignment previous : result) if (previous.SameRef(endpoint)) return false;
				result.Insert(endpoint);
			}
		}
		return unique.Count() == 1 && !result.IsEmpty() && result.Count() <= 16;
	}
	protected static bool SameCatalog(IEntity entity, array<ref RICON_FrequencyOnlyAlignment> expected)
	{
		array<ref RICON_FrequencyOnlyAlignment> actual;
		if (!expected || !Catalog(entity, -1, false, actual) || actual.Count() != expected.Count()) return false;
		bool same = true;
		for (int i = 0; i < expected.Count(); i++)
		{
			if (!expected[i].Matches(entity) || !expected[i].SameRef(actual[i])) same = false;
			actual[i].ClearPrivate();
		}
		return same;
	}
	protected static string VectorText(array<ref RICON_FrequencyOnlyAlignment> catalog, bool original = false)
	{
		string text;
		foreach (RICON_FrequencyOnlyAlignment endpoint : catalog)
		{
			if (!text.IsEmpty()) text = text + "|";
			int frequency = endpoint.Frequency(); if (original) frequency = endpoint.Original();
			text = text + frequency.ToString();
		}
		return text;
	}
	protected bool Context()
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!m_World || !m_AController || !m_BController || !m_AEntity || !m_BEntity) return false;
		if (!Replication.IsServer() || GetGame().GetWorld() != m_World || !manager || m_World.IsEditMode()) return false;
		if (GetGame().GetPlayerController() != m_AController || m_AController.GetPlayerId() != 1) return false;
		if (m_AController.GetControlledEntity() != m_AEntity || manager.GetPlayerController(2) != m_BController) return false;
		if (manager.GetPlayerControlledEntity(2) != m_BEntity || m_BController.GetControlledEntity() != m_BEntity) return false;
		return m_AEntity.GetWorld() == m_World && m_BEntity.GetWorld() == m_World && Rpl(m_BEntity) == m_BRpl;
	}
	protected bool AUnchanged() { return m_A.Matches(m_AEntity) && m_A.Frequency() == m_SourceFrequency && m_A.KeysSame() && m_A.PowerMuteSame(); }
	protected bool KeysSame()
	{
		foreach (RICON_FrequencyOnlyAlignment endpoint : m_B) if (!endpoint.KeysSame()) return false;
		return m_A.KeysSame();
	}
	protected bool PowerMuteSame()
	{
		foreach (RICON_FrequencyOnlyAlignment endpoint : m_B) if (!endpoint.PowerMuteSame()) return false;
		return m_A.PowerMuteSame();
	}
	protected bool CurrentReady()
	{
		if (!m_Applied || m_Restored || m_Invalid || !Context() || !SameCatalog(m_BEntity, m_B) || !AUnchanged()) return false;
		array<int> frequencies = {};
		foreach (RICON_FrequencyOnlyAlignment endpoint : m_B)
		{
			if (endpoint.Frequency() != endpoint.Target()) return false;
			frequencies.Insert(endpoint.Frequency());
		}
		return KeysSame() && PowerMuteSame() && RICON_CaptureFrequencyGate.AllMismatch(frequencies, m_SourceFrequency);
	}
	protected string Fields(bool known)
	{
		string frequenciesText = "UNKNOWN"; bool sameKeys; bool samePower; bool sameA; bool mismatch;
		if (known)
		{
			frequenciesText = VectorText(m_B); sameKeys = KeysSame(); samePower = PowerMuteSame(); sameA = AUnchanged();
			array<int> frequencies = {};
			foreach (RICON_FrequencyOnlyAlignment endpoint : m_B) frequencies.Insert(endpoint.Frequency());
			mismatch = RICON_CaptureFrequencyGate.AllMismatch(frequencies, m_SourceFrequency);
		}
		string text = "rawTickMs=" + System.GetTickCount().ToString() + " aFrequencyKHz=" + m_SourceFrequency.ToString();
		text = text + " bEntityRpl=" + m_BRpl + " catalogCount=" + m_B.Count().ToString();
		text = text + " originalFrequenciesKHz=" + m_OriginalVector + " frequenciesKHz=" + frequenciesText;
		text = text + " known=" + Bit(known) + " catalogSame=" + Bit(known);
		text = text + " allMismatch=" + Bit(mismatch) + " aUnchanged=" + Bit(sameA);
		text = text + " keysSame=" + Bit(sameKeys) + " powerMuteSame=" + Bit(samePower);
		return text + " nativeOwnership=UNQUALIFIED";
	}
	protected bool Prepare(BaseWorld world, PlayerController controller, IEntity entity, SCR_VONEntryRadio entry)
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager || !entry || !entry.IsActive() || !entry.IsUsable()) return false;
		PlayerController remote = manager.GetPlayerController(2); IEntity b = manager.GetPlayerControlledEntity(2);
		if (!remote || !b || remote.GetControlledEntity() != b || b.GetWorld() != world || Rpl(b) == "UNKNOWN") return false;
		BaseTransceiver source = entry.GetTransceiver(); SCR_GadgetComponent gadget = entry.GetGadget();
		if (!source || !gadget) return false;
		m_World = world; m_AController = controller; m_AEntity = entity; m_BController = remote; m_BEntity = b; m_BRpl = Rpl(b);
		m_SourceFrequency = source.GetFrequency();
		m_A = new RICON_FrequencyOnlyAlignment();
		if (!m_A.Capture(entity, gadget, source.GetRadio(), source, entry.GetTransceiverNumber() - 1, m_SourceFrequency)) return false;
		if (!Catalog(b, m_SourceFrequency, true, m_B) || !Context()) return false;
		m_OriginalVector = VectorText(m_B, true);
		bool changed;
		foreach (RICON_FrequencyOnlyAlignment endpoint : m_B) if (endpoint.Original() != endpoint.Target()) changed = true;
		if (!changed) return false;
		System.GetCLIParam("riconNativeRxRun", m_Run);
		m_Applied = true;
		foreach (RICON_FrequencyOnlyAlignment endpoint : m_B)
		{
			if (!Context() || !AUnchanged() || !endpoint.Apply()) { m_Invalid = true; break; }
		}
		bool known = CurrentReady();
		PrintFormat("[RICON_CAPTURE_FREQ] run=%1 kind=APPLIED view=A_SERVER_B_CATALOG %2", m_Run, Fields(known));
		if (!known) { m_Invalid = true; Restore(); }
		return true;
	}
	protected void Restore()
	{
		if (!m_Applied || m_Restored) return;
		bool same = Context() && SameCatalog(m_BEntity, m_B);
		int restored; bool conflict;
		foreach (RICON_FrequencyOnlyAlignment endpoint : m_B)
		{
			if (same && endpoint.Restore(m_BEntity)) restored++;
			else conflict = true;
		}
		m_Restored = true;
		string text = Fields(same);
		text = text + " expectedCount=" + m_B.Count().ToString() + " restoredCount=" + restored.ToString();
		text = text + " frequenciesRestored=" + Bit(same && !conflict && restored == m_B.Count()) + " conflict=" + Bit(conflict);
		PrintFormat("[RICON_CAPTURE_FREQ] run=%1 kind=RESTORE view=A_SERVER_B_CATALOG %2", m_Run, text);
		foreach (RICON_FrequencyOnlyAlignment clear : m_B) clear.ClearPrivate();
		m_A.ClearPrivate();
	}
	static bool Ready() { return s_Server && s_Server.CurrentReady(); }
	static void RestoreOwned() { if (s_Server) s_Server.Restore(); }
	static void ClearReader()
	{
		if (s_BReadCatalog) foreach (RICON_FrequencyOnlyAlignment endpoint : s_BReadCatalog) endpoint.ClearPrivate();
		s_BReadCatalog = null; s_BReadEntity = null; s_BReadRpl = "";
	}
	static void Poll(BaseWorld world, RICON_LiveNativeRxProbe probe)
	{
		if (!RICON_CaptureControlGate.WrongFrequency() || !probe || !world || world.IsEditMode()) return;
		string run; if (!System.GetCLIParam("riconNativeRxRun", run)) return;
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller) return;
		int player = controller.GetPlayerId(); int remaining; int now = System.GetTickCount();
		if (!probe.RICON_PulseHealth(now, run, player, remaining)) return;
		if (player == 1 && Replication.IsServer())
		{
			if (!s_Server)
			{
				if (s_LastPrepare >= 0 && now - s_LastPrepare < 500) return;
				s_LastPrepare = now;
				string attempt; System.GetCLIParam("riconPulseAttempt", attempt);
				ArmaReforgerScripted game = ArmaReforgerScripted.Cast(GetGame());
				if (!game || game.RICON_RestCapturePreparedGeneration(run, attempt, world) < 1) return;
				SCR_VONController control = SCR_VONController.Cast(controller.FindComponent(SCR_VONController));
				SCR_VONEntryRadio entry; if (control) entry = SCR_VONEntryRadio.Cast(control.GetActiveEntry());
				RICON_CaptureFrequencyControl candidate = new RICON_CaptureFrequencyControl();
				if (candidate.Prepare(world, controller, controller.GetControlledEntity(), entry)) s_Server = candidate;
				return;
			}
			if (s_Server.m_Restored || now - s_Server.m_LastRead < 500) return;
			s_Server.m_LastRead = now;
			bool valid = s_Server.CurrentReady(); if (!valid) s_Server.m_Invalid = true;
			PrintFormat("[RICON_CAPTURE_FREQ] run=%1 kind=READBACK view=A_SERVER_B_CATALOG %2", run, s_Server.Fields(valid));
		}
		else if (player == 2 && Replication.IsClient() && !Replication.IsServer())
		{
			if (s_LastBRead >= 0 && now - s_LastBRead < 500) return;
			s_LastBRead = now; IEntity entity = controller.GetControlledEntity(); string rpl = Rpl(entity);
			if (!s_BReadEntity && entity && rpl != "UNKNOWN")
			{
				if (Catalog(entity, -1, false, s_BReadCatalog)) { s_BReadEntity = entity; s_BReadRpl = rpl; }
			}
			bool validB = entity && entity == s_BReadEntity && entity.GetWorld() == world && rpl == s_BReadRpl && SameCatalog(entity, s_BReadCatalog);
			string frequenciesText = "UNKNOWN"; int count;
			if (validB) { frequenciesText = VectorText(s_BReadCatalog); count = s_BReadCatalog.Count(); }
			PrintFormat("[RICON_CAPTURE_FREQ] run=%1 kind=READBACK view=B_LOCAL rawTickMs=%2 bEntityRpl=%3 catalogCount=%4 frequenciesKHz=%5 known=%6 catalogSame=%6", run, now, rpl, count, frequenciesText, Bit(validB));
		}
	}
}
