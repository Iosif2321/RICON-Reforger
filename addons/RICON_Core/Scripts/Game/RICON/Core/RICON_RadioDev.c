//! Observational diagnostics. The flag is local to this process and does not change permissions.
class RICON_RadioDev
{
	protected static bool s_bChecked, s_bEnabled;

	static bool Enabled()
	{
		if (!s_bChecked)
		{
			string value;
			s_bEnabled = System.GetCLIParam("riconDev", value) && value == "1";
			s_bChecked = true;
		}
		return s_bEnabled;
	}

	static string ReplyCode(RICON_GameRadioPttReply reply)
	{
		if (!reply) return "none";
		return typename.EnumToString(RICON_ELifecycleResult, reply.m_eCode) + " #" + reply.m_iRequestId.ToString();
	}

	static void TracePtt(string side, RICON_GameRadioPttReply reply)
	{
		if (!Enabled() || !reply || !reply.m_Context) return;
		PrintFormat("[RICON_DEV] %1 CONTROL player=%2 kind=%3 method=%4 req=%5 result=%6 hold=%7 lease=%8 active=%9",
			side, reply.m_Context.m_iPlayerId, reply.m_iKind, reply.m_iMethod, reply.m_iRequestId,
			typename.EnumToString(RICON_ELifecycleResult, reply.m_eCode), reply.m_iHoldToken, reply.m_iLeaseId, reply.m_bActive);
		if (reply.m_iDeviceId > 0 || reply.m_bHasLease)
			PrintFormat("[RICON_DEV] %1 CONTROL_SLOT req=%2 radio=%3 slot=%4 network=%5 channel=%6 rxGen=%7 deadlineMs=%8 terminal=%9",
				side, reply.m_iRequestId, reply.m_iDeviceId, reply.m_iIndex, reply.m_iNetworkId, reply.m_iChannelId, reply.m_iRxGeneration,
				reply.m_iDeadlineMs, typename.EnumToString(RICON_ELeaseTerminalReason, reply.m_eTerminalReason));
	}
}

//! Non-modal display, local observations only. Server RF detail is written to its console.
class RICON_RadioDevHud
{
	protected const int ROWS = 12, WIDTH = 626, LEFT = 22, TOP = 24, ROW_HEIGHT = 21;
	protected ref array<TextWidget> m_Rows = {};
	protected Widget m_Panel;
	protected int m_iLastSampleMs = -1;
	protected bool m_bRendered;
	protected string m_sPriorPtt, m_sPriorSelection, m_sPriorBridge;

	protected bool Create(WorkspaceWidget workspace)
	{
		WidgetFlags flags = WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS | WidgetFlags.BLEND;
		m_Panel = workspace.CreateWidgetInWorkspace(WidgetType.PanelWidgetTypeID, LEFT, TOP, WIDTH, ROWS * ROW_HEIGHT + 16,
			flags, Color.FromRGBA(12, 20, 26, 226), 150);
		if (!m_Panel) return false;
		for (int i = 0; i < ROWS; i++)
		{
			TextWidget row = TextWidget.Cast(workspace.CreateWidgetInWorkspace(WidgetType.TextWidgetTypeID,
				LEFT + 12, TOP + 8 + i * ROW_HEIGHT, WIDTH - 24, ROW_HEIGHT, flags, Color.FromRGBA(224, 235, 240, 255), 151));
			if (!row) { Close(); return false; }
			row.SetExactFontSize(15);
			if (i == 0)
			{
				row.SetExactFontSize(18);
				row.SetBold(true);
				row.SetColor(Color.FromRGBA(139, 227, 201, 255));
			}
			m_Rows.Insert(row);
		}
		PrintFormat("[RICON_DEV] HUD_READY rows=%1", m_Rows.Count());
		return true;
	}

	void Update(SCR_PlayerController actor, string bridge)
	{
		if (!actor || !actor.GetControlledEntity() || !GetGame().GetWorkspace()) { Close(); return; }
		int nowMs = System.GetTickCount();
		if (nowMs < 0 || (m_iLastSampleMs >= 0 && nowMs >= m_iLastSampleMs && nowMs - m_iLastSampleMs < 250)) return;
		m_iLastSampleMs = nowMs;
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(RICON_CommsManagerComponent.GetInstance());
		if (!m_Panel && !Create(GetGame().GetWorkspace())) return;

		bool configured = manager && manager.IsGameRadioInputConfigured();
		bool mixed = manager && manager.IsGameMixedEquipmentConfigured();
		array<string> lines = {"RICON DEV / RADIO", string.Format("LOCAL player=%1  control=%2  mixed=%3  server RF: console",
			actor.GetPlayerId(), configured, mixed)};
		SCR_VONController input = SCR_VONController.Cast(actor.FindComponent(SCR_VONController));
		if (input) input.RICON_AppendDevInput(lines);
		else lines.Insert("PTT: no VON controller");
		actor.RICON_AppendDevControl(lines);
		actor.RICON_AppendDevMedia(lines);
		lines.Insert(bridge);
		for (int i = 0; i < m_Rows.Count(); i++)
		{
			if (i < lines.Count()) m_Rows[i].SetText(lines[i]);
			else m_Rows[i].SetText("");
		}
		if (!m_bRendered && m_Panel.IsVisible() && m_Rows[0].IsVisible() && m_Rows[0].GetText() == lines[0])
		{
			float width, height;
			m_Panel.GetScreenSize(width, height);
			if (width > 0 && height > 0)
			{
				m_bRendered = true;
				PrintFormat("[RICON_DEV] HUD_VISIBLE width=%1 height=%2 title=%3", width, height, lines[0]);
			}
		}
		string selected = "no selection";
		if (lines.Count() > 3 && input) selected = lines[3];
		if (selected != m_sPriorSelection || lines[2] != m_sPriorPtt || bridge != m_sPriorBridge)
		{
			m_sPriorSelection = selected; m_sPriorPtt = lines[2]; m_sPriorBridge = bridge;
			PrintFormat("[RICON_DEV] CLIENT %1 | %2 | %3", lines[2], selected, bridge);
		}
	}

	void Close()
	{
		foreach (TextWidget row : m_Rows) if (row) row.RemoveFromHierarchy();
		m_Rows.Clear();
		if (m_Panel) m_Panel.RemoveFromHierarchy();
		m_Panel = null;
		m_bRendered = false;
		m_sPriorPtt = ""; m_sPriorSelection = ""; m_sPriorBridge = "";
	}
}
