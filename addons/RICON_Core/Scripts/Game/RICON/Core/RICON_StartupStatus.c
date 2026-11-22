//! Local startup metadata. These files describe plugin status and grant no voice permission.
class RICON_StartupStatusProtocol
{
	static const string MOD_VERSION = "0.6.4";
	static const string WANTED_PLUGIN_VERSION = "0.6.3";
	static const string GAME_PATH = "$profile:ricon-status-game.txt";
	static const string PLUGIN_PATH = "$profile:ricon-status-plugin.txt";
	static const string ACK_PATH = "$profile:ricon-status-ack.txt";
	static const int TOKEN_MS = 3000, FRESH_MS = 6500, MISSING_MS = 5000, BANNER_MS = 12000;

	static bool InGameVoiceRequested()
	{
		string enabled;
		return System.GetCLIParam("riconInGameVoice", enabled) && enabled == "1";
	}

	static bool Ts3StatusRequested()
	{
		string enabled;
		return (System.GetCLIParam("riconTS3Status", enabled) && enabled == "1")
			|| RICON_TS3GameBridge.Requested() || RICON_TS3GameBridge.PttRequested()
			|| RICON_TS3GameBridge.TabBindingRequested() || RICON_TS3GameBridge.RouteRequested();
	}

	static bool LocalClient()
	{
		return GetGame() && !System.IsConsoleApp() && !System.IsCLIParam("server") && RplSession.Mode() != RplMode.Dedicated;
	}

	protected static int VersionPart(string part)
	{
		if (part.IsEmpty() || part.Length() > 5 || (part.Length() > 1 && part.ToAscii(0) == 48)) return -1;
		int number = 0;
		for (int index = 0; index < part.Length(); index++)
		{
			int digit = part.ToAscii(index) - 48;
			if (digit < 0 || digit > 9) return -1;
			number = number * 10 + digit;
		}
		if (number > 65535) return -1;
		return number;
	}

	static bool Version(string value)
	{
		array<string> parts = {};
		value.Split(".", parts, true);
		if (parts.Count() != 3 || value != parts[0] + "." + parts[1] + "." + parts[2]) return false;
		foreach (string part : parts) if (VersionPart(part) < 0) return false;
		return true;
	}

	static int CompareVersion(string left, string right)
	{
		if (!Version(left) || !Version(right)) return 0;
		array<string> a = {}, b = {};
		left.Split(".", a, true); right.Split(".", b, true);
		for (int index = 0; index < 3; index++)
		{
			int av = VersionPart(a[index]), bv = VersionPart(b[index]);
			if (av < bv) return -1;
			if (av > bv) return 1;
		}
		return 0;
	}

	static bool Token(string token)
	{
		array<string> parts = {};
		token.Split("-", parts, true);
		if (parts.Count() != 2 || token != parts[0] + "-" + parts[1]) return false;
		foreach (string part : parts)
		{
			if (part.IsEmpty() || part.Length() > 10 || part.ToAscii(0) == 48) return false;
			for (int index = 0; index < part.Length(); index++)
			{
				int digit = part.ToAscii(index);
				if (digit < 48 || digit > 57) return false;
			}
		}
		return true;
	}

	static bool Nonce(string nonce)
	{
		if (nonce.Length() != 32) return false;
		for (int index = 0; index < 32; index++)
		{
			int c = nonce.ToAscii(index);
			if (!((c >= 48 && c <= 57) || (c >= 97 && c <= 102))) return false;
		}
		return true;
	}

	static bool ParsePlugin(string row, string token, out string nonce, out string installed, out string state)
	{
		nonce = ""; installed = ""; state = "";
		if (row.Length() > 256 || !Token(token)) return false;
		int length = row.Length();
		if (length > 0 && row.ToAscii(length - 1) == 10)
		{
			length--;
			if (length > 0 && row.ToAscii(length - 1) == 13) length--;
			if (length == 0) return false;
			row = row.Substring(0, length);
		}
		array<string> fields = {};
		row.Split(" ", fields, true);
		if (fields.Count() != 6 || fields[0] != "RICON_TS_1" || fields[1] != token || fields[5] != "END") return false;
		if (!Nonce(fields[2]) || !Version(fields[3])) return false;
		string status = fields[4];
		if (status != "CURRENT" && status != "NEWER" && status != "CHECKING" && status != "STAGED" && status != "ERROR") return false;
		if (row != "RICON_TS_1 " + token + " " + fields[2] + " " + fields[3] + " " + status + " END") return false;
		nonce = fields[2]; installed = fields[3]; state = status;
		return true;
	}

	static bool Write(string path, string row)
	{
		if (!LocalClient() || row.IsEmpty() || row.Length() > 254) return false;
		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file) return false;
		row += "\r\n";
		bool complete = file.Write(row, row.Length()) == row.Length();
		file.Close();
		return complete;
	}

	static bool Read(out string row)
	{
		return ReadAt(PLUGIN_PATH, row);
	}

	static bool ReadAt(string path, out string row)
	{
		row = "";
		if (!FileIO.FileExists(path)) return false;
		FileHandle file = FileIO.OpenFile(path, FileMode.READ);
		if (!file) return false;
		int size = file.GetLength();
		bool complete = size > 0 && size <= 256 && file.Read(row, size) == size;
		complete = complete && file.GetLength() == size && row.Length() == size;
		file.Close();
		return complete;
	}
}

//! One process-local poller serves the main menu and a directly entered world.
class RICON_StartupStatus
{
	protected static ref RICON_StartupStatus s_Instance;
	protected int m_iStartedAt = -1, m_iTokenAt = -1, m_iLastTick = -1, m_iMatchedAt = -1;
	protected int m_iSession, m_iTokenSequence;
	protected string m_sPreviousGameToken, m_sPreviousPluginToken;
	protected string m_sToken, m_sAckToken, m_sAckNonce, m_sVersion, m_sHostState, m_sLastReported;
	protected bool m_bStarted, m_bFileError;

	static RICON_StartupStatus Start()
	{
		if (!RICON_StartupStatusProtocol.LocalClient()) return null;
		if (!s_Instance) s_Instance = new RICON_StartupStatus();
		if (!s_Instance.m_bStarted)
		{
			s_Instance.m_bStarted = true;
			s_Instance.m_iStartedAt = System.GetTickCount();
			if (!RICON_StartupStatusProtocol.Ts3StatusRequested())
			{
				Print("[RICON_STARTUP] mode=STOCK_VOICE");
				return s_Instance;
			}
			s_Instance.m_iSession = System.GetUnixTime();
			s_Instance.m_sPreviousGameToken = s_Instance.ExistingToken(RICON_StartupStatusProtocol.GAME_PATH);
			s_Instance.m_sPreviousPluginToken = s_Instance.ExistingToken(RICON_StartupStatusProtocol.PLUGIN_PATH);
			GetGame().GetCallqueue().CallLater(s_Instance.Tick, 250, true);
			s_Instance.Tick();
		}
		return s_Instance;
	}

	protected string ExistingToken(string path)
	{
		string row;
		if (!RICON_StartupStatusProtocol.ReadAt(path, row)) return "";
		array<string> fields = {};
		row.Split(" ", fields, true);
		if (fields.Count() < 2 || !RICON_StartupStatusProtocol.Token(fields[1])) return "";
		return fields[1];
	}

	protected string NextToken()
	{
		if (m_iSession <= 0) return "";
		for (int attempt = 0; attempt < 3; attempt++)
		{
			if (m_iTokenSequence == int.MAX) return "";
			m_iTokenSequence++;
			string candidate = m_iSession.ToString() + "-" + m_iTokenSequence.ToString();
			if (candidate != m_sPreviousGameToken && candidate != m_sPreviousPluginToken) return candidate;
		}
		return "";
	}

	void Tick()
	{
		if (!RICON_StartupStatusProtocol.LocalClient()) return;
		int now = System.GetTickCount();
		if (now < 0 || now < m_iLastTick)
		{
			m_bFileError = true;
			m_iMatchedAt = -1;
			return;
		}
		m_iLastTick = now;
		if (m_iTokenAt < 0 || now - m_iTokenAt >= RICON_StartupStatusProtocol.TOKEN_MS)
		{
			string token = NextToken();
			if (token.IsEmpty() || !RICON_StartupStatusProtocol.Write(RICON_StartupStatusProtocol.GAME_PATH,
				"RICON_MENU_1 " + token + " " + RICON_StartupStatusProtocol.MOD_VERSION + " " + RICON_StartupStatusProtocol.WANTED_PLUGIN_VERSION + " END"))
			{
				m_bFileError = true;
				Report(now);
				return;
			}
			m_sToken = token; m_iTokenAt = now;
			m_bFileError = false;
		}
		string row, nonce, installed, state;
		if (RICON_StartupStatusProtocol.Read(row) && RICON_StartupStatusProtocol.ParsePlugin(row, m_sToken, nonce, installed, state))
		{
			bool changed = m_sAckToken != m_sToken || m_sAckNonce != nonce || installed != m_sVersion || state != m_sHostState;
			if (m_sAckToken != m_sToken || m_sAckNonce != nonce)
			{
				if (!RICON_StartupStatusProtocol.Write(RICON_StartupStatusProtocol.ACK_PATH, "RICON_ACK_1 " + m_sToken + " " + nonce + " END"))
				{
					m_bFileError = true;
					Report(now);
					return;
				}
				m_sAckToken = m_sToken; m_sAckNonce = nonce;
			}
			m_sVersion = installed; m_sHostState = state; m_bFileError = false;
			if (changed) m_iMatchedAt = now;
		}
		Report(now);
	}

	string State(int now)
	{
		if (m_bFileError) return "ERROR";
		if (m_iMatchedAt < 0)
		{
			if (now >= m_iStartedAt && now - m_iStartedAt >= RICON_StartupStatusProtocol.MISSING_MS) return "MISSING";
			return "WAITING";
		}
		if (now < m_iMatchedAt || now - m_iMatchedAt >= RICON_StartupStatusProtocol.FRESH_MS) return "STALE";
		if (m_sHostState == "CHECKING" || m_sHostState == "STAGED" || m_sHostState == "ERROR") return m_sHostState;
		int comparison = RICON_StartupStatusProtocol.CompareVersion(m_sVersion, RICON_StartupStatusProtocol.WANTED_PLUGIN_VERSION);
		if (comparison < 0) return "OLD";
		if (comparison > 0) return "NEWER";
		return "CURRENT";
	}

	protected void Report(int now)
	{
		string status = State(now);
		string key = status + " " + m_sVersion;
		if (key == m_sLastReported) return;
		m_sLastReported = key;
		PrintFormat("[RICON_STARTUP] state=%1 mod=%2 wanted=%3 installed=%4", status,
			RICON_StartupStatusProtocol.MOD_VERSION, RICON_StartupStatusProtocol.WANTED_PLUGIN_VERSION, m_sVersion);
	}

	void Text(out string title, out string detail)
	{
		if (!RICON_StartupStatusProtocol.Ts3StatusRequested())
		{
			title = "Штатная связь Reforger";
			detail = "Радио и DIRECT работают через VoN игры.";
			return;
		}
		string state = State(System.GetTickCount());
		title = "Проверка плагина TeamSpeak…";
		detail = "Ожидается RICON " + RICON_StartupStatusProtocol.WANTED_PLUGIN_VERSION + ".";
		if (state == "CURRENT")
		{
			title = "Плагин TeamSpeak " + m_sVersion + " отвечает";
			detail = "Установлена нужная версия плагина.";
		}
		else if (state == "OLD")
		{
			title = "Плагин TeamSpeak " + m_sVersion + " требует обновления";
			detail = "Для этой версии мода нужен плагин " + RICON_StartupStatusProtocol.WANTED_PLUGIN_VERSION + ".";
		}
		else if (state == "NEWER")
		{
			title = "Плагин TeamSpeak " + m_sVersion + " новее требуемой версии";
			detail = "Целевая версия плагина для этого мода — " + RICON_StartupStatusProtocol.WANTED_PLUGIN_VERSION + ".";
		}
		else if (state == "STAGED")
		{
			title = "Плагин TeamSpeak " + m_sVersion + " · обновление подготовлено";
			detail = "Обновление до " + RICON_StartupStatusProtocol.WANTED_PLUGIN_VERSION + " применится после закрытия TeamSpeak.";
		}
		else if (state == "CHECKING")
		{
			title = "Плагин TeamSpeak " + m_sVersion + " проверяет обновление";
			detail = "Состояние обновится автоматически.";
		}
		else if (state == "MISSING" || state == "STALE")
		{
			if (state == "MISSING") title = "Плагин TeamSpeak пока не отвечает";
			else title = "Нет свежего подтверждения плагина TeamSpeak";
			detail = "Запустите TeamSpeak с RICON. Проверка продолжится в фоне.";
		}
		else if (state == "ERROR")
		{
			title = "Не удалось подтвердить состояние плагина TeamSpeak";
			if (!m_bFileError && m_sHostState == "ERROR" && m_iMatchedAt >= 0
				&& System.GetTickCount() >= m_iMatchedAt && System.GetTickCount() - m_iMatchedAt < RICON_StartupStatusProtocol.FRESH_MS)
				title = "Плагин TeamSpeak " + m_sVersion + " сообщил об ошибке";
			detail = "Подробности доступны в журнале RICON и TeamSpeak.";
		}
	}
}

//! Disposable menu widgets. Their lifetime is independent of the ongoing status poller.
class RICON_StartupStatusPanel
{
	protected Widget m_Panel;
	protected ref array<TextWidget> m_Rows = {};
	protected int m_iWidth, m_iHeight;
	protected string m_sRendered;

	protected bool Create(WorkspaceWidget workspace, int width, int height)
	{
		float referenceWidth = workspace.DPIUnscale(width);
		int panelWidth = Math.Min(referenceWidth * 0.90, 1120);
		int left = (referenceWidth - panelWidth) / 2;
		int top = workspace.DPIUnscale(14);
		int lineHeight = 32;
		int padding = 12;
		WidgetFlags flags = WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS | WidgetFlags.BLEND;
		m_Panel = workspace.CreateWidgetInWorkspace(WidgetType.PanelWidgetTypeID, left, top, panelWidth,
			3 * lineHeight + 2 * padding, flags, Color.FromRGBA(12, 20, 26, 238), 150);
		if (!m_Panel) return false;
		m_Panel.SetName("RICON_StartupStatus");
		for (int index = 0; index < 3; index++)
		{
			TextWidget row = TextWidget.Cast(workspace.CreateWidgetInWorkspace(WidgetType.TextWidgetTypeID,
				left + padding, top + padding + index * lineHeight, panelWidth - 2 * padding, lineHeight,
				flags, Color.FromRGBA(230, 238, 241, 255), 151));
			if (!row) { Hide("CREATE_FAILED"); return false; }
			row.SetExactFontSize(26);
			if (index == 0) { row.SetBold(true); row.SetColor(Color.FromRGBA(139, 227, 201, 255)); }
			m_Rows.Insert(row);
		}
		m_iWidth = width; m_iHeight = height;
		return true;
	}

	bool Render(RICON_StartupStatus service)
	{
		if (!service || !GetGame() || !GetGame().GetWorkspace()) return false;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		int width = workspace.GetWidth(), height = workspace.GetHeight();
		if (width < 320 || height < 240) return false;
		if (m_Panel && (width != m_iWidth || height != m_iHeight)) Hide("RESIZE");
		if (!m_Panel && !Create(workspace, width, height)) return false;
		string title, detail;
		service.Text(title, detail);
		array<string> lines = {"RICON " + RICON_StartupStatusProtocol.MOD_VERSION + " · мод инициализирован", title, detail};
		for (int index = 0; index < m_Rows.Count(); index++) m_Rows[index].SetText(lines[index]);
		string rendered = title + " " + detail;
		float panelWidth, panelHeight;
		m_Panel.GetScreenSize(panelWidth, panelHeight);
		if (!m_Panel.IsVisible() || !m_Rows[1].IsVisible() || panelWidth <= 0 || panelHeight <= 0) return false;
		if (m_Rows[1].GetText() != title || m_Rows[2].GetText() != detail) return false;
		if (rendered != m_sRendered)
		{
			m_sRendered = rendered;
			PrintFormat("[RICON_STARTUP] VISIBLE width=%1 height=%2 title=%3 text=%4", panelWidth, panelHeight, m_Rows[0].GetText(), m_Rows[1].GetText());
			PrintFormat("[RICON_STARTUP] DETAIL text=%1", m_Rows[2].GetText());
		}
		return true;
	}

	void Hide(string reason)
	{
		bool visible = m_Panel != null;
		foreach (TextWidget row : m_Rows) if (row) row.RemoveFromHierarchy();
		m_Rows.Clear();
		if (m_Panel) m_Panel.RemoveFromHierarchy();
		m_Panel = null; m_sRendered = "";
		if (visible) PrintFormat("[RICON_STARTUP] HIDDEN reason=%1", reason);
	}
}

modded class MainMenuUI
{
	protected ref RICON_StartupStatusPanel m_RICON_StartupPanel;
	protected int m_iRICON_StartupRemainingMs, m_iRICON_StartupVisibleAt = -1;
	protected bool m_bRICON_StartupFocused;

	override protected void OnMenuOpen()
	{
		super.OnMenuOpen();
		if (!RICON_StartupStatus.Start()) return;
		m_iRICON_StartupRemainingMs = RICON_StartupStatusProtocol.BANNER_MS;
		m_iRICON_StartupVisibleAt = -1;
		m_RICON_StartupPanel = new RICON_StartupStatusPanel();
	}

	override protected void OnMenuFocusGained()
	{
		super.OnMenuFocusGained();
		m_bRICON_StartupFocused = true;
		m_iRICON_StartupVisibleAt = -1;
	}

	override protected void OnMenuFocusLost()
	{
		m_bRICON_StartupFocused = false;
		m_iRICON_StartupVisibleAt = -1;
		if (m_RICON_StartupPanel) m_RICON_StartupPanel.Hide("FOCUS_LOST");
		super.OnMenuFocusLost();
	}

	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (!m_RICON_StartupPanel) return;
		if (!m_bRICON_StartupFocused) return;
		int now = System.GetTickCount();
		if (now < 0 || (m_iRICON_StartupVisibleAt >= 0 && now < m_iRICON_StartupVisibleAt))
		{
			m_RICON_StartupPanel.Hide("CLOCK_ERROR");
			m_RICON_StartupPanel = null;
			return;
		}
		if (m_iRICON_StartupVisibleAt >= 0) m_iRICON_StartupRemainingMs -= now - m_iRICON_StartupVisibleAt;
		if (m_iRICON_StartupRemainingMs <= 0)
		{
			m_RICON_StartupPanel.Hide("TIMEOUT");
			m_RICON_StartupPanel = null;
			return;
		}
		if (m_RICON_StartupPanel.Render(RICON_StartupStatus.Start())) m_iRICON_StartupVisibleAt = now;
		else m_iRICON_StartupVisibleAt = -1;
	}

	override void OnMenuClose()
	{
		m_bRICON_StartupFocused = false;
		if (m_RICON_StartupPanel) m_RICON_StartupPanel.Hide("MENU_CLOSE");
		m_RICON_StartupPanel = null;
		m_iRICON_StartupRemainingMs = 0;
		m_iRICON_StartupVisibleAt = -1;
		super.OnMenuClose();
	}
}

modded class ArmaReforgerScripted
{
	override bool OnGameStart()
	{
		bool started = super.OnGameStart();
		if (started) RICON_StartupStatus.Start();
		return started;
	}
}
