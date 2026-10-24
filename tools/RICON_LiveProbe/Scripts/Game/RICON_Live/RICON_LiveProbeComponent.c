//! TOOL_ONLY: добавить один компонент на GameMode отдельного тестового мира.
//! Подписки работают только в Play; существующие миры, рации и физика сигнала не изменяются.
class RICON_LiveProbeComponentClass : ScriptComponentClass
{
}

class RICON_LiveProbeComponent : ScriptComponent
{
	protected ref RICON_LiveJournal m_Journal;
	protected ref RICON_LiveObserver m_Server;
	protected ref RICON_LiveObserver m_Client;
	protected ref RICON_LiveRadioReader m_Radios;   //!< показания раций для Q-F, только чтение (claude #4075)
	protected SCR_VoNComponent m_CurrentVon;
	protected bool m_bRunning;
	protected bool m_bClientBindingObserved;
	protected float m_fPollElapsed;
	protected float m_fSessionElapsed;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}

	override void EOnInit(IEntity owner)
	{
		Start();
	}

	//! Явный запуск/повтор после Stop; в редакторском мире подписки не устанавливаются.
	bool Start()
	{
		if (m_bRunning)
			return false;
		IEntity owner = GetOwner();
		if (!owner || !owner.GetWorld() || owner.GetWorld().IsEditMode())
			return false;
		SCR_BaseGameMode mode = SCR_BaseGameMode.Cast(owner);
		if (!mode)
		{
			Print("[RICON_LIVE] state=INACTIVE reason=GAME_MODE_REQUIRED");
			return false;
		}
		m_Journal = new RICON_LiveJournal(256);
		// Поток (codex #4035, claude #4028): OnCapture вызывается каждый кадр записи, буфер 256 — только снимок.
		m_Journal.SetStream(true);
		if (Replication.IsServer())
		{
			m_Server = new RICON_LiveObserver(m_Journal, "server");
			bool seen = m_Server.AttachVonUsed();
			bool spawned = m_Server.AttachGameMode(mode);
			if (!seen || !spawned)
			{
				m_Server.Detach();
				m_Server = null;
				Print("[RICON_LIVE] state=INACTIVE reason=SERVER_BINDING_UNAVAILABLE");
				return false;
			}
		}
		m_Client = new RICON_LiveObserver(m_Journal, "client");
		m_Radios = new RICON_LiveRadioReader(m_Journal, "client");
		m_bRunning = true;
		m_bClientBindingObserved = false;
		m_fPollElapsed = 0;
		m_fSessionElapsed = 0;
		PollClient();
		Print("[RICON_LIVE] state=RUNNING binding=UNKNOWN");
		return true;
	}

	//! Контроллер может появиться позже GameMode или смениться; прежние invokers сначала отсоединяются.
	protected void PollClient()
	{
		ChimeraGame game = GetGame();
		PlayerController controller;
		if (game)
			controller = game.GetPlayerController();
		SCR_VoNComponent von;
		IEntity controlled;
		m_Client.m_iLocalPlayerId = -1;
		if (controller)
		{
			m_Client.m_iLocalPlayerId = controller.GetPlayerId();
			controlled = controller.GetControlledEntity();
			if (controlled)
				von = SCR_VoNComponent.Cast(controlled.FindComponent(SCR_VoNComponent));
		}
		// Q-F: показания раций каждый опрос; запись только изменений, идентичность — сама сущность рации
		// (смена персонажа или предмета даёт RADIO_GONE и новое наблюдение, codex #4083).
		m_Radios.ReadEntity(m_Client.m_iLocalPlayerId, controlled);
		bool bindingStable = (von && m_Client.m_iAttached == 2) || (!von && m_Client.m_iAttached == 0);
		if (m_bClientBindingObserved && von == m_CurrentVon && bindingStable)
			return;
		m_bClientBindingObserved = true;
		m_Client.Detach();
		m_CurrentVon = von;
		bool attached = false;
		if (von)
			attached = m_Client.AttachVonComponent(von);
		PrintFormat("[RICON_LIVE] state=CLIENT_BINDING player=%1 attached=%2", m_Client.m_iLocalPlayerId, attached);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bRunning || timeSlice <= 0)
			return;
		m_fSessionElapsed += timeSlice;
		if (m_fSessionElapsed >= 900)
		{
			Stop();
			return;
		}
		m_fPollElapsed += timeSlice;
		if (m_fPollElapsed >= 1)
		{
			m_fPollElapsed = 0;
			PollClient();
		}
	}

	void Dump()
	{
		if (m_Journal)
			m_Journal.Dump();
	}

	//! Повторный Stop безопасен; пользовательский mute и настройки рации не затрагиваются.
	void Stop()
	{
		if (m_Server)
			m_Server.Detach();
		if (m_Client)
			m_Client.Detach();
		m_Server = null;
		m_Client = null;
		m_Radios = null;
		m_CurrentVon = null;
		if (m_bRunning)
		{
			Dump();
			Print("[RICON_LIVE] state=STOPPED");
		}
		m_bRunning = false;
	}

	override void OnDelete(IEntity owner)
	{
		Stop();
		super.OnDelete(owner);
	}
}
