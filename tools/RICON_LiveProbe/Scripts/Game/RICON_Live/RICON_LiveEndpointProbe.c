//! TOOL_ONLY B0a: наблюдение одной локальной роли; не требует компонента на реплицируемой сущности.
class RICON_LiveEndpointProbe
{
	static const int SAMPLE_LIMIT = 30;
	protected ref RICON_LiveJournal m_Journal;
	protected ref RICON_LiveRadioReader m_Radios;
	protected BaseWorld m_World;
	protected PlayerController m_PreviousController;
	protected IEntity m_PreviousControlled;
	protected int m_iPreviousPlayerId = -1;
	protected bool m_bHadController;
	protected bool m_bHadControlled;
	protected bool m_bBindingObserved;
	protected bool m_bRunning;
	protected int m_iCycle;
	protected int m_iSamples;
	protected int m_iBindingRevision;
	protected float m_fElapsed;
	protected float m_fLastSample;

	static bool IsRequested()
	{
		string value;
		return System.GetCLIParam("riconEndpointProbe", value) && value == "1";
	}

	int SampleCount()
	{
		return m_iSamples;
	}

	protected static string Bit(bool value)
	{
		if (value)
			return "1";
		return "0";
	}

	//! Число игроков не доказывает личность peer: сохраняем также фактический список ID, без имён профилей.
	protected static string PlayerIds()
	{
		if (!GetGame().GetPlayerManager())
			return "UNKNOWN";
		array<int> ids = {};
		GetGame().GetPlayerManager().GetPlayers(ids);
		if (ids.IsEmpty())
			return "NONE";
		string text;
		foreach (int id : ids)
		{
			if (!text.IsEmpty())
				text = text + "|";
			text = text + id.ToString();
		}
		return text;
	}

	bool Start(BaseWorld world, int cycle)
	{
		if (m_bRunning || !world || world.IsEditMode())
			return false;
		m_World = world;
		m_iCycle = cycle;
		m_Journal = new RICON_LiveJournal(256);
		m_Journal.SetStream(true);
		m_Radios = new RICON_LiveRadioReader(m_Journal, "probe");
		m_bRunning = true;
		string text = "cycle=" + cycle.ToString() + ",limit=" + SAMPLE_LIMIT.ToString();
		text = text + ",isServer=" + Bit(Replication.IsServer()) + ",isClient=" + Bit(Replication.IsClient());
		int rplMode = RplSession.Mode();
		text = text + ",rplMode=" + rplMode.ToString();
		Note("ENDPOINT_START", -1, text);
		return true;
	}

	protected void Note(string kind, int playerId, string text)
	{
		m_Journal.Add(RICON_LiveObserver.Now(), "probe", kind, playerId, -1, -1, m_iSamples, text);
	}

	//! Сохранённые признаки наличия обнаруживают исчезновение даже после обнуления слабой ссылки движком.
	protected void ObserveBinding(PlayerController controller, IEntity controlled, int playerId)
	{
		bool hasController = controller != null;
		bool hasControlled = controlled != null;
		bool pcChanged = controller != m_PreviousController || hasController != m_bHadController;
		bool entityChanged = controlled != m_PreviousControlled || hasControlled != m_bHadControlled;
		bool idChanged = playerId != m_iPreviousPlayerId;
		if (!m_bBindingObserved || pcChanged || entityChanged || idChanged)
		{
			m_iBindingRevision++;
			string text = "cycle=" + m_iCycle.ToString() + ",revision=" + m_iBindingRevision.ToString();
			text = text + ",initial=" + Bit(!m_bBindingObserved) + ",pcChanged=" + Bit(pcChanged);
			text = text + ",entityChanged=" + Bit(entityChanged) + ",idChanged=" + Bit(idChanged);
			text = text + ",pc=" + Bit(hasController) + ",controlled=" + Bit(hasControlled);
			text = text + ",previousPlayerId=" + m_iPreviousPlayerId.ToString();
			Note("ENDPOINT_BINDING", playerId, text);
		}
		m_PreviousController = controller;
		m_PreviousControlled = controlled;
		m_iPreviousPlayerId = playerId;
		m_bHadController = hasController;
		m_bHadControlled = hasControlled;
		m_bBindingObserved = true;
	}

	void Tick(BaseWorld world, float timeSlice)
	{
		if (!m_bRunning)
			return;
		if (!world || world != m_World || world.IsEditMode())
		{
			Stop("WORLD_CHANGED");
			return;
		}
		if (!RICON_Num.IsFinite(timeSlice) || timeSlice <= 0)
			return;
		m_fElapsed += timeSlice;
		if (m_fElapsed < m_fLastSample + 1)
			return;
		m_fLastSample = m_fElapsed;
		m_iSamples++;
		PlayerController controller = GetGame().GetPlayerController();
		IEntity controlled;
		int playerId = -1;
		if (controller)
		{
			playerId = controller.GetPlayerId();
			controlled = controller.GetControlledEntity();
		}
		ObserveBinding(controller, controlled, playerId);
		float ms = m_fElapsed * 1000.0;
		int elapsedMs = ms;
		string text = "cycle=" + m_iCycle.ToString() + ",elapsedMs=" + elapsedMs.ToString();
		text = text + ",bindingRevision=" + m_iBindingRevision.ToString() + "," + RICON_LiveRadioBenchComponent.PlayerState();
		text = text + ",members=" + PlayerIds();
		int rplMode = RplSession.Mode();
		text = text + ",rplMode=" + rplMode.ToString();
		Note("ENDPOINT_SAMPLE", playerId, text);
		m_Radios.ReadEntity(playerId, controlled);
		if (m_iSamples >= SAMPLE_LIMIT)
			Stop("LIMIT");
	}

	//! Повторное завершение не добавляет события; ссылки прошлого мира освобождаются до штатной очистки.
	void Stop(string reason)
	{
		if (!m_bRunning)
			return;
		string kind = "ENDPOINT_ABORTED";
		if (reason == "LIMIT" && m_iSamples == SAMPLE_LIMIT)
			kind = "ENDPOINT_DONE";
		string text = "cycle=" + m_iCycle.ToString() + ",samples=" + m_iSamples.ToString() + ",reason=" + reason;
		Note(kind, m_iPreviousPlayerId, text);
		m_Journal.Dump();
		m_Radios = null;
		m_Journal = null;
		m_World = null;
		m_PreviousController = null;
		m_PreviousControlled = null;
		m_bRunning = false;
	}
}
