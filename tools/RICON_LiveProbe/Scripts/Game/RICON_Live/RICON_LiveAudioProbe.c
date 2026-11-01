//! TOOL_ONLY: opt-in passive radio observer. No capture, PTT, radio or direct state mutations.
//! Separate prefix keeps endpoint evidence/parser independent from this journal.
class RICON_LiveAudioJournal : RICON_LiveJournal
{
	static string AudioFormat(RICON_LiveEvent e)
	{
		string line = RICON_LiveJournal.Format(e);
		line.Replace("[RICON_LIVE]", "[RICON_AUDIO]");
		return line;
	}

	override int Add(int timeMs, string side, string kind, int playerId, int deviceId, int transceiver, int value, string text)
	{
		// Base streaming stays off: exactly one independently sequenced audio stream.
		int seq = super.Add(timeMs, side, kind, playerId, deviceId, transceiver, value, text);
		PrintFormat("%1", AudioFormat(At(Count() - 1)));
		return seq;
	}

	override void Dump()
	{
		// The complete stream was printed on Add; do not duplicate it or masquerade as an endpoint dump.
		PrintFormat("[RICON_AUDIO] snapshot count=%1 dropped=%2 nextSeq=%3", Count(), Dropped(), NextSeq());
	}
}

class RICON_LiveAudioProbe
{
	protected ref RICON_LiveAudioJournal m_Journal;
	protected ref RICON_LiveObserver m_Server;
	protected ref RICON_LiveObserver m_Client;
	protected BaseWorld m_World;
	protected SCR_VoNComponent m_CurrentVon;
	protected int m_iLocalPlayerId = -1;
	protected bool m_bBindingObserved;
	protected bool m_bRunning;
	protected float m_fElapsed;
	protected float m_fPollElapsed;

	static bool ResolveRequested(bool configured, string value)
	{
		return configured && value == "1";
	}

	static bool IsRequested()
	{
		string value;
		bool configured = System.GetCLIParam("riconAudioObserve", value);
		return ResolveRequested(configured, value);
	}

	bool Start(BaseWorld world)
	{
		if (m_bRunning || !world || world.IsEditMode())
			return false;
		m_World = world;
		m_Journal = new RICON_LiveAudioJournal();
		m_Journal.SetStream(false);
		if (Replication.IsServer())
		{
			m_Server = new RICON_LiveObserver(m_Journal, "server");
			SCR_BaseGameMode mode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
			bool used = m_Server.AttachVonUsed();
			bool spawned = m_Server.AttachGameMode(mode);
			if (!used || !spawned)
			{
				m_Server.Detach();
				m_Server = null;
				m_Journal = null;
				m_World = null;
				Print("[RICON_AUDIO] state=INACTIVE reason=SERVER_BINDING_UNAVAILABLE");
				return false;
			}
			PrintFormat("[RICON_AUDIO] state=SERVER_BINDING subscriptions=%1", m_Server.m_iAttached);
		}
		m_Client = new RICON_LiveObserver(m_Journal, "client");
		m_bRunning = true;
		m_bBindingObserved = false;
		m_fElapsed = 0;
		m_fPollElapsed = 0;
		PollClient();
		Print("[RICON_AUDIO] state=RUNNING passive=1 binding=UNKNOWN");
		return true;
	}

	protected void PollClient()
	{
		PlayerController controller = GetGame().GetPlayerController();
		IEntity controlled;
		SCR_VoNComponent von;
		int playerId = -1;
		if (controller)
		{
			playerId = controller.GetPlayerId();
			controlled = controller.GetControlledEntity();
			if (controlled)
				von = SCR_VoNComponent.Cast(controlled.FindComponent(SCR_VoNComponent));
		}
		bool stable = (von && m_Client.m_iAttached == 2) || (!von && m_Client.m_iAttached == 0);
		if (m_bBindingObserved && von == m_CurrentVon && playerId == m_iLocalPlayerId && stable)
			return;
		m_Client.Detach();
		m_bBindingObserved = true;
		m_CurrentVon = von;
		m_iLocalPlayerId = playerId;
		m_Client.m_iLocalPlayerId = playerId;
		bool attached;
		if (von)
			attached = m_Client.AttachVonComponent(von);
		PrintFormat("[RICON_AUDIO] state=CLIENT_BINDING player=%1 attached=%2 subscriptions=%3", playerId, attached, m_Client.m_iAttached);
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
		if (m_fElapsed >= 900)
		{
			Stop("LIMIT");
			return;
		}
		m_fPollElapsed += timeSlice;
		if (m_fPollElapsed >= 1)
		{
			m_fPollElapsed = 0;
			PollClient();
		}
	}

	void Stop(string reason)
	{
		if (m_Server)
			m_Server.Detach();
		if (m_Client)
			m_Client.Detach();
		int remaining;
		if (m_Server)
			remaining += m_Server.m_iAttached;
		if (m_Client)
			remaining += m_Client.m_iAttached;
		m_Server = null;
		m_Client = null;
		m_CurrentVon = null;
		m_World = null;
		if (m_bRunning)
		{
			m_Journal.Dump();
			PrintFormat("[RICON_AUDIO] state=STOPPED reason=%1 subscriptionsRemaining=%2", reason, remaining);
		}
		m_Journal = null;
		m_bRunning = false;
	}
}
