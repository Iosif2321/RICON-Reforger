//! TOOL_ONLY: наблюдатель radio-only прибора live-проверки (LIVE-order-frequency-mute.md §3, codex #3971).
//! Только чтение: явная подписка на штатные invokers, подтверждённые чтением исходников (E-von-windows-ea.md),
//! запись событий в RICON_LiveJournal и явная отписка. Прямая речь (OnCapture/OnReceive без трансивера) не
//! журналируется и ни в чём не меняется; вызовов, меняющих состояние (SetMuteState и т. п.), здесь нет.
//! Привязка событий к рации/трансиверу ещё не выводится (binding=UNKNOWN): device = −1, tr = −1, сырое значение
//! частоты — как отдал движок, без единицы.
class RICON_LiveObserver
{
	protected ref RICON_LiveJournal m_Journal;
	protected string m_sSide;                                               //!< фактическая сторона: "server" | "client" | "probe"
	protected ScriptInvokerInt m_VonUsed;                                   //!< SCR_VoNComponent.GetOnVoNUsed() — статический
	protected ScriptInvokerBase<SCR_BaseGameMode_PlayerIdAndEntity> m_Spawned; //!< SCR_BaseGameMode.GetOnPlayerSpawned()
	protected ScriptInvoker m_Capture;                                      //!< SCR_VoNComponent.m_OnCaptureVON (клиент)
	protected ScriptInvoker m_Receive;                                      //!< SCR_VoNComponent.m_OnReceivedVON (клиент)
	int m_iAttached;       //!< число активных подписок
	int m_iDirectIgnored;  //!< колбэки без трансивера (прямая речь): не записываются
	int m_iLocalPlayerId = -1; //!< локальный игрок; неизвестен до привязки контроллера

	void RICON_LiveObserver(RICON_LiveJournal journal, string side)
	{
		m_Journal = journal;
		m_sSide = side;
	}

	RICON_LiveJournal Journal()
	{
		return m_Journal;
	}

	//! Время источника в мс по часам мира той стороны, где стоит наблюдатель; −1 без игры или мира (Workbench).
	static int Now()
	{
		ChimeraGame game = GetGame();
		if (!game)
			return -1;
		BaseWorld world = game.GetWorld();
		if (!world)
			return -1;
		float worldTime = world.GetWorldTime();
		int ms = worldTime;
		return ms;
	}

	//! «Кто говорит» (сервер): статический invoker ванильного компонента, существует и без мира.
	bool AttachVonUsed()
	{
		if (m_VonUsed)
			return false;
		ScriptInvokerInt invoker = SCR_VoNComponent.GetOnVoNUsed();
		if (!invoker)
			return false;
		invoker.Insert(OnVoNUsed);
		m_VonUsed = invoker;
		m_iAttached++;
		return true;
	}

	bool AttachSpawned(ScriptInvokerBase<SCR_BaseGameMode_PlayerIdAndEntity> spawned)
	{
		if (!spawned || m_Spawned)
			return false;
		spawned.Insert(OnPlayerSpawned);
		m_Spawned = spawned;
		m_iAttached++;
		return true;
	}

	bool AttachGameMode(SCR_BaseGameMode gameMode)
	{
		if (!gameMode)
			return false;
		return AttachSpawned(gameMode.GetOnPlayerSpawned());
	}

	bool AttachCapture(ScriptInvoker capture)
	{
		if (!capture || m_Capture)
			return false;
		capture.Insert(OnCapture);
		m_Capture = capture;
		m_iAttached++;
		return true;
	}

	bool AttachReceive(ScriptInvoker receive)
	{
		if (!receive || m_Receive)
			return false;
		receive.Insert(OnReceive);
		m_Receive = receive;
		m_iAttached++;
		return true;
	}

	//! Компонент VoN локального игрока (клиент): оба публичных invoker.
	bool AttachVonComponent(SCR_VoNComponent von)
	{
		if (!von)
			return false;
		return AttachClientInvokers(von.m_OnCaptureVON, von.m_OnReceivedVON);
	}

	//! Обе подписки устанавливаются вместе: отказ не оставляет половину подключения к другому компоненту.
	bool AttachClientInvokers(ScriptInvoker capture, ScriptInvoker receive)
	{
		if (!capture || !receive || m_Capture || m_Receive)
			return false;
		AttachCapture(capture);
		AttachReceive(receive);
		return true;
	}

	//! Полная отписка от всех invokers; повторный вызов — пустая операция.
	void Detach()
	{
		if (m_VonUsed)
		{
			m_VonUsed.Remove(OnVoNUsed);
			m_VonUsed = null;
		}
		if (m_Spawned)
		{
			m_Spawned.Remove(OnPlayerSpawned);
			m_Spawned = null;
		}
		if (m_Capture)
		{
			m_Capture.Remove(OnCapture);
			m_Capture = null;
		}
		if (m_Receive)
		{
			m_Receive.Remove(OnReceive);
			m_Receive = null;
		}
		m_iAttached = 0;
	}

	// Обработчики тонкие: вся запись — в Record*, которые проверяются в Workbench без мира.
	protected void OnVoNUsed(int senderId)
	{
		RecordSeen(senderId);
	}

	protected void OnPlayerSpawned(int playerId, IEntity player)
	{
		RecordSpawn(playerId);
	}

	protected void OnCapture(BaseTransceiver transmitter)
	{
		if (!transmitter)
		{
			m_iDirectIgnored++;
			return;
		}
		RecordCapture(transmitter.GetFrequency());
	}

	protected void OnReceive(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		if (!receiver)
		{
			m_iDirectIgnored++;
			return;
		}
		RecordReceive(playerId, frequency, quality);
	}

	int RecordSeen(int senderId)
	{
		return m_Journal.Add(Now(), m_sSide, "TX_SEEN", senderId, -1, -1, -1, "binding=UNKNOWN,channel=UNKNOWN");
	}

	int RecordSpawn(int playerId)
	{
		return m_Journal.Add(Now(), m_sSide, "SPAWN", playerId, -1, -1, -1, "lifecycle");
	}

	//! TX_CAPTURE — колбэк каждого кадра записи (не фронт); value — сырая частота передатчика без единицы.
	int RecordCapture(int rawFrequency)
	{
		return m_Journal.Add(Now(), m_sSide, "TX_CAPTURE", m_iLocalPlayerId, -1, -1, rawFrequency, "binding=UNKNOWN");
	}

	//! RX_EVENT — событие приёма, не пакет; качество печатается с девятью знаками после запятой, без пересчёта в промилле.
	int RecordReceive(int playerId, int rawFrequency, float quality)
	{
		string text = "binding=UNKNOWN,listener=" + m_iLocalPlayerId.ToString();
		text = text + ",q=" + quality.ToString(-1, 9);
		return m_Journal.Add(Now(), m_sSide, "RX_EVENT", playerId, -1, -1, rawFrequency, text);
	}
}
