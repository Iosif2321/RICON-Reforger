//! Metadata only. Public game lifecycle + profile FileIO; no native extension or PCM file transport.
class RICON_TS3GameBridge
{
	static const string REQUEST = "$profile:ricon-ts3-request.txt";
	static const string RESPONSE = "$profile:ricon-ts3-response.txt";
	static const string PTT_REQUEST = "$profile:ricon-ptt-request.txt";
	static const string PTT_RESPONSE = "$profile:ricon-ptt-response.txt";
	static const string PTT_QUERY = "RICON_PTT_QUERY_1";
	static const string TAB_QUERY_PATH = "$profile:ricon-ts3-tab-query.txt";
	static const string TAB_RESPONSE_PATH = "$profile:ricon-ts3-tab-response.txt";
	static const string TAB_QUERY = "RICON_TS3_TAB_QUERY_1";
	static const string TAB_RESPONSE = "RICON_TS3_TAB_1";
	static const string ROUTE_QUERY_PATH = "$profile:ricon-ts3-route-query.txt";
	static const string ROUTE_RESPONSE_PATH = "$profile:ricon-ts3-route-response.txt";
	static const int TAB_ATTEMPT_MS = 3000;

	static bool Requested()
	{
		string enabled;
		return System.GetCLIParam("riconTS3Live", enabled) && enabled == "1";
	}

	static bool PttRequested()
	{
		string enabled;
		return System.GetCLIParam("riconTS3Ptt", enabled) && enabled == "1";
	}

	static bool TabBindingRequested()
	{
		string enabled;
		return System.GetCLIParam("riconTS3Binding", enabled) && enabled == "1";
	}

	static bool RouteRequested()
	{
		string enabled;
		return System.GetCLIParam("riconTS3Route", enabled) && enabled == "1";
	}

	static bool ParseRouteQuery(string row, out string nonce, out int sequence)
	{
		nonce = ""; sequence = 0;
		if (row.Length() > 96) return false;
		row = TabRow(row);
		if (!ParseRequest(row, nonce, sequence, RICON_GameTs3RouteWire.QUERY_HEADER)) return false;
		return row == RICON_GameTs3RouteWire.QUERY_HEADER + " " + nonce + " " + sequence.ToString() + " END";
	}

	static bool ReadRouteQuery(out string nonce, out int sequence)
	{
		string row;
		return ReadTabRow(ROUTE_QUERY_PATH, 96, row) && ParseRouteQuery(row, nonce, sequence);
	}

	static void ClearRouteResponse()
	{
		if (FileIO.FileExists(ROUTE_RESPONSE_PATH)) FileIO.DeleteFile(ROUTE_RESPONSE_PATH);
	}

	static bool ReplyRoute(string nonce, int sequence, int session, RICON_GameTs3RouteReply reply, int remaining)
	{
		string currentNonce;
		int currentSequence;
		if (!ReadRouteQuery(currentNonce, currentSequence) || nonce != currentNonce || sequence != currentSequence) return false;
		string row = RICON_GameTs3RouteWire.Row(nonce, sequence, session, reply, remaining);
		if (row.IsEmpty()) return false;
		FileHandle output = FileIO.OpenFile(ROUTE_RESPONSE_PATH, FileMode.WRITE);
		if (!output) return false;
		row += "\r\n";
		bool complete = output.Write(row, row.Length()) == row.Length();
		output.Close();
		if (!complete) ClearRouteResponse();
		return complete;
	}

	static bool ParseRequest(string row, out string nonce, out int sequence, string header = "RICON_TS3_1")
	{
		nonce = "";
		sequence = 0;
		int size = row.Length();
		if (size > 0 && row.ToAscii(size - 1) == 10) row = row.Substring(0, size - 1);
		size = row.Length();
		if (size > 0 && row.ToAscii(size - 1) == 13) row = row.Substring(0, size - 1);
		array<string> fields = {};
		row.Split(" ", fields, true);
		if (fields.Count() != 4 || fields[0] != header || fields[3] != "END" || fields[1].Length() != 32) return false;
		for (int i = 0; i < 32; i++)
		{
			int ch = fields[1].ToAscii(i);
			if (!((ch >= 48 && ch <= 57) || (ch >= 97 && ch <= 102))) return false;
		}
		string number = fields[2];
		if (number.Length() < 1 || number.Length() > 10 || number.ToAscii(0) == 48) return false;
		int parsed = 0;
		for (int n = 0; n < number.Length(); n++)
		{
			int digit = number.ToAscii(n) - 48;
			if (digit < 0 || digit > 9 || parsed > (int.MAX - digit) / 10) return false;
			parsed = parsed * 10 + digit;
		}
		if (row != header + " " + fields[1] + " " + parsed.ToString() + " END") return false;
		nonce = fields[1];
		sequence = parsed;
		return true;
	}

	//! Strip one LF or CRLF. A bare or additional CR remains invalid input.
	protected static string TabRow(string row)
	{
		int size = row.Length();
		if (size > 0 && row.ToAscii(size - 1) == 10)
		{
			size--;
			if (size > 0 && row.ToAscii(size - 1) == 13) size--;
			return row.Substring(0, size);
		}
		return row;
	}

	protected static bool ValidTabNonce(string nonce)
	{
		if (nonce.Length() != 32) return false;
		for (int index = 0; index < 32; index++)
		{
			int ch = nonce.ToAscii(index);
			if (!((ch >= 48 && ch <= 57) || (ch >= 97 && ch <= 102))) return false;
		}
		return true;
	}

	protected static int ParseTabInt(string number)
	{
		if (number.Length() < 1 || number.Length() > 10 || number.ToAscii(0) == 48) return 0;
		int parsed = 0;
		for (int index = 0; index < number.Length(); index++)
		{
			int digit = number.ToAscii(index) - 48;
			if (digit < 0 || digit > 9 || parsed > (int.MAX - digit) / 10) return 0;
			parsed = parsed * 10 + digit;
		}
		return parsed;
	}

	static bool ParseTabQuery(string row, out string nonce, out int sequence)
	{
		nonce = ""; sequence = 0;
		if (row.Length() > 96) return false;
		row = TabRow(row);
		array<string> fields = {};
		row.Split(" ", fields, true);
		if (fields.Count() != 4 || fields[0] != TAB_QUERY || fields[3] != "END" || !ValidTabNonce(fields[1])) return false;
		int parsed = ParseTabInt(fields[2]);
		if (parsed <= 0 || row != TAB_QUERY + " " + fields[1] + " " + parsed.ToString() + " END") return false;
		nonce = fields[1]; sequence = parsed;
		return true;
	}

	static bool ParseTabResponse(string row, string nonce, int sequence, out int revision, out string serverUid, out string clientUid)
	{
		revision = 0; serverUid = ""; clientUid = "";
		if (row.Length() > 384 || sequence <= 0 || !ValidTabNonce(nonce)) return false;
		row = TabRow(row);
		array<string> fields = {};
		row.Split(" ", fields, true);
		if (fields.Count() != 7 || fields[0] != TAB_RESPONSE || fields[6] != "END") return false;
		if (fields[1] != nonce || fields[2] != sequence.ToString()) return false;
		int parsed = ParseTabInt(fields[3]);
		if (parsed <= 0 || !RICON_GameTs3BindingClaim.IsValidUid(fields[4]) || !RICON_GameTs3BindingClaim.IsValidUid(fields[5])) return false;
		string canonical = TAB_RESPONSE + " " + nonce + " " + sequence.ToString() + " " + parsed.ToString();
		canonical += " " + fields[4] + " " + fields[5] + " END";
		if (row != canonical) return false;
		revision = parsed; serverUid = fields[4]; clientUid = fields[5];
		return true;
	}

	static bool TabAttemptLive(int issuedMs, int nowMs)
	{
		return issuedMs >= 0 && nowMs >= issuedMs && nowMs < int.MAX && nowMs - issuedMs < TAB_ATTEMPT_MS;
	}

	protected static bool ReadTabRow(string path, int sizeLimit, out string row)
	{
		row = "";
		FileHandle input = FileIO.OpenFile(path, FileMode.READ);
		if (!input) return false;
		int size = input.GetLength();
		bool complete = size > 0 && size <= sizeLimit && input.Read(row, size) == size;
		complete = complete && input.GetLength() == size && row.Length() == size;
		input.Close();
		return complete;
	}

	static string NewTabNonce()
	{
		string id = UUID.GenV4();
		id.ToLower();
		if (!RICON_GameTs3PendingBinding.IsValidId(id)) return "";
		string nonce = id.Substring(0, 8) + id.Substring(9, 4) + id.Substring(14, 4)
			+ id.Substring(19, 4) + id.Substring(24, 12);
		if (!ValidTabNonce(nonce)) return "";
		return nonce;
	}

	static bool WriteTabQuery(string nonce, int sequence)
	{
		if (sequence <= 0 || !ValidTabNonce(nonce)) return false;
		FileHandle output = FileIO.OpenFile(TAB_QUERY_PATH, FileMode.WRITE);
		if (!output) return false;
		string row = TAB_QUERY + " " + nonce + " " + sequence.ToString() + " END\r\n";
		int size = row.Length();
		bool complete = output.Write(row, size) == size;
		output.Close();
		return complete;
	}

	static void ClearTabQuery(string nonce, int sequence)
	{
		if (!FileIO.FileExists(TAB_QUERY_PATH)) return;
		string row, actualNonce;
		int actualSequence;
		if (!ReadTabRow(TAB_QUERY_PATH, 96, row) || !ParseTabQuery(row, actualNonce, actualSequence)) return;
		if (nonce == actualNonce && sequence == actualSequence) FileIO.DeleteFile(TAB_QUERY_PATH);
	}

	static bool ReadTabResponse(string nonce, int sequence, out int revision, out string serverUid, out string clientUid)
	{
		revision = 0; serverUid = ""; clientUid = "";
		if (!FileIO.FileExists(TAB_RESPONSE_PATH)) return false;
		string row;
		return ReadTabRow(TAB_RESPONSE_PATH, 384, row) && ParseTabResponse(row, nonce, sequence, revision, serverUid, clientUid);
	}

	protected static bool ReadRequest(string path, out string nonce, out int sequence, string header = "RICON_TS3_1")
	{
		FileHandle input = FileIO.OpenFile(path, FileMode.READ);
		if (!input) return false;
		int size = input.GetLength();
		string row;
		bool complete = size > 0 && size <= 96 && input.Read(row, size) == size;
		complete = complete && input.GetLength() == size && row.Length() == size;
		input.Close();
		return complete && ParseRequest(row, nonce, sequence, header);
	}

	static bool Reply(int session, int player, bool active)
	{
		string nonce;
		int sequence;
		if (!ReadRequest(REQUEST, nonce, sequence)) return false;
		FileHandle output = FileIO.OpenFile(RESPONSE, FileMode.WRITE);
		if (!output) return false;
		int state = 0;
		if (active) state = 1;
		output.WriteLine("RICON_GAME_1 " + nonce + " " + sequence.ToString() + " " + session.ToString() + " " + player.ToString() + " " + state.ToString() + " END");
		output.Close();
		return true;
	}

	//! Takes a parsed query and an optional detached local receipt.
	protected static string PttRow(string nonce, int sequence, int session, RICON_GameRadioPttReply reply, int remainingMs)
	{
		string row = "RICON_PTT_1 " + nonce + " " + sequence.ToString() + " " + session.ToString();
		if (remainingMs <= 0 || remainingMs > 60000 || !RICON_GameRadioInputOwner.ValidPttMetadata(reply)) return row + " 0 END";
		RICON_GameRadioPttContext context = reply.m_Context;
		row += " 1 " + context.m_sDomain + " " + context.m_iEpoch.ToString() + " " + context.m_iPlayerId.ToString();
		row += " " + context.m_iConnectionId.ToString() + " " + context.m_iGeneration.ToString() + " " + context.m_iEntityId.ToString();
		row += " " + context.m_iPolicyRevision.ToString() + " " + reply.m_iDeviceId.ToString() + " " + reply.m_iIndex.ToString();
		row += " " + reply.m_iNetworkId.ToString() + " " + reply.m_iChannelId.ToString() + " " + reply.m_iRxGeneration.ToString();
		row += " " + reply.m_iHoldToken.ToString() + " " + reply.m_iRequestId.ToString() + " " + reply.m_iLeaseId.ToString();
		return row + " " + remainingMs.ToString() + " END";
	}

	static void ClearPttResponse()
	{
		if (FileIO.FileExists(PTT_RESPONSE)) FileIO.DeleteFile(PTT_RESPONSE);
	}

	static bool ReplyPtt(int session, SCR_PlayerController local, inout string lastNonce, inout int lastSequence)
	{
		string nonce;
		int sequence;
		if (session <= 0 || !ReadRequest(PTT_REQUEST, nonce, sequence, PTT_QUERY))
		{
			ClearPttResponse();
			return false;
		}
		if (nonce == lastNonce && sequence == lastSequence) return false;
		int remainingMs = 0;
		ref RICON_GameRadioPttReply reply;
		if (local && PttRequested())
		{
			SCR_VONController hook = SCR_VONController.Cast(local.FindComponent(SCR_VONController));
			if (hook) reply = hook.RICON_ReadPttMetadata(remainingMs);
		}
		FileHandle output = FileIO.OpenFile(PTT_RESPONSE, FileMode.WRITE);
		if (!output) return false;
		string row = PttRow(nonce, sequence, session, reply, remainingMs) + "\r\n";
		int size = row.Length();
		bool complete = output.Write(row, size) == size;
		output.Close();
		if (!complete)
		{
			ClearPttResponse();
			return false;
		}
		lastNonce = nonce;
		lastSequence = sequence;
		return true;
	}

	static bool SelfCheck()
	{
		return SelfCheckLive() && SelfCheckPtt();
	}

	static bool SelfCheckLive()
	{
		string nonce;
		int sequence;
		string prefix = "RICON_TS3_1 0123456789abcdef0123456789abcdef ";
		if (!ParseRequest(prefix + "1 END\r\n", nonce, sequence) || sequence != 1) return false;
		if (!ParseRequest(prefix + "2147483647 END", nonce, sequence) || sequence != int.MAX) return false;
		return !ParseRequest(prefix + "2147483648 END", nonce, sequence)
			&& !ParseRequest(prefix + "01 END", nonce, sequence)
			&& !ParseRequest(prefix + "1 END\nextra", nonce, sequence)
			&& !ParseRequest(prefix + "1  END", nonce, sequence)
			&& !ParseRequest("RICON_TS3_1 stale 1 END", nonce, sequence);
	}

	static bool SelfCheckPtt()
	{
		string nonce;
		int sequence;
		string token = "0123456789abcdef0123456789abcdef";
		string query = PTT_QUERY + " " + token + " ";
		if (!ParseRequest(query + "42 END", nonce, sequence, PTT_QUERY) || nonce != token || sequence != 42) return false;
		if (ParseRequest(query + "42 END", nonce, sequence) || ParseRequest(query + "042 END", nonce, sequence, PTT_QUERY)) return false;
		if (ParseRequest(query + "2147483648 END", nonce, sequence, PTT_QUERY)) return false;
		string prefix = "RICON_PTT_1 " + token + " 42 3";
		if (PttRow(token, 42, 3, null, 0) != prefix + " 0 END") return false;
		RICON_GameRadioPttContext context = new RICON_GameRadioPttContext();
		context.m_sDomain = "a282d93f-39c4-4ca9-a788-96881eeac410";
		context.m_iEpoch = 101;
		context.m_iPlayerId = 2;
		context.m_iConnectionId = 51;
		context.m_iGeneration = 6;
		context.m_iEntityId = 200;
		context.m_iPolicyRevision = 1;
		RICON_GameRadioPttReply reply = new RICON_GameRadioPttReply();
		reply.m_Context = context;
		reply.m_iKind = RICON_GameRadioPttReply.PTT;
		reply.m_iMethod = RICON_ELifecycleMethod.PTT_START;
		reply.m_eCode = RICON_ELifecycleResult.OK;
		reply.m_bProcessed = true;
		reply.m_bActive = true;
		reply.m_bHasLease = true;
		reply.m_eLeaseState = RICON_ETransmitLeaseState.ACTIVE;
		reply.m_iDeviceId = 21;
		reply.m_iIndex = 0;
		reply.m_iNetworkId = 1;
		reply.m_iChannelId = 30;
		reply.m_iRxGeneration = 8;
		reply.m_iHoldToken = 7;
		reply.m_iRequestId = 9;
		reply.m_iLeaseId = 4;
		string body = " 1 a282d93f-39c4-4ca9-a788-96881eeac410 101 2 51 6 200 1 21 0 1 30 8 7 9 4";
		if (PttRow(token, 42, 3, reply, 60000) != prefix + body + " 60000 END") return false;
		if (PttRow(token, 42, 3, reply, 60001) != prefix + " 0 END") return false;
		if (PttRow(token, 42, 3, reply, 0) != prefix + " 0 END") return false;
		reply.m_iIndex = -1;
		if (PttRow(token, 42, 3, reply, 1) != prefix + " 0 END") return false;
		reply.m_iIndex = 0;
		reply.m_bActive = false;
		if (PttRow(token, 42, 3, reply, 1) != prefix + " 0 END") return false;
		reply.m_bActive = true;
		context.m_sDomain = "A282d93f-39c4-4ca9-a788-96881eeac410";
		return PttRow(token, 42, 3, reply, 1) == prefix + " 0 END";
	}
}

modded class ArmaReforgerScripted
{
	protected ref RICON_RadioDevHud m_RICON_RadioDevHud;
	protected BaseWorld m_RICON_TS3World;
	protected int m_RICON_TS3Session, m_RICON_TS3LastTick = -1;
	protected string m_RICON_TS3PttLastNonce;
	protected int m_RICON_TS3PttLastSequence;
	protected bool m_RICON_TS3Active, m_RICON_TS3LiveEnabled, m_RICON_TS3PttEnabled;
	protected bool m_RICON_TS3RouteEnabled, m_bRICON_TS3RouteAnswerActive;
	protected string m_sRICON_TS3RouteNonce, m_sRICON_TS3RouteAnsweredNonce;
	protected int m_iRICON_TS3RouteSequence, m_iRICON_TS3RouteAnsweredSequence;
	protected int m_iRICON_TS3RouteRequest, m_iRICON_TS3RouteStarted = -1, m_iRICON_TS3RouteLastTick = -1;
	protected SCR_PlayerController m_RICON_TS3RouteActor;
	protected bool m_bRICON_TS3TabEnabled, m_bRICON_TS3TabConsumed;
	protected BaseWorld m_RICON_TS3TabWorld;
	protected SCR_PlayerController m_RICON_TS3TabActor;
	protected IEntity m_RICON_TS3TabEntity;
	protected int m_iRICON_TS3TabSequence, m_iRICON_TS3TabLastTick = -1, m_iRICON_TS3TabStartedAt = -1;
	protected int m_iRICON_TS3TabWindowMs;
	protected string m_sRICON_TS3TabNonce;
	protected ref RICON_GameRadioPttContext m_RICON_TS3TabContext;

	protected SCR_PlayerController RICON_TS3TabLocalActor(BaseWorld world, out IEntity entity)
	{
		entity = null;
		if (!world || world != m_RICON_TS3TabWorld || world != GetWorld() || world.IsEditMode()) return null;
		if (!GetGameStarted() || !RICON_CommsManagerComponent.GetInstance()) return null;
		if (Replication.IsServer() || !Replication.IsClient() || RplSession.Mode() != RplMode.Client) return null;
		SCR_PlayerController actor = SCR_PlayerController.Cast(GetPlayerController());
		if (!actor || actor.GetPlayerId() <= 0) return null;
		IEntity controlled = actor.GetControlledEntity();
		if (!controlled || controlled.IsDeleted() || controlled.GetWorld() != world) return null;
		RplId item = Replication.FindItemId(actor);
		if (!item.IsValid()) return null;
		RplNode node = Replication.FindNode(item);
		if (!node || !node.IsOwner()) return null;
		entity = controlled;
		return actor;
	}

	protected void RICON_RetireTs3TabChallenge()
	{
		string nonce = m_sRICON_TS3TabNonce;
		m_sRICON_TS3TabNonce = "";
		m_RICON_TS3TabContext = null;
		m_bRICON_TS3TabConsumed = true;
		if (!nonce.IsEmpty()) RICON_TS3GameBridge.ClearTabQuery(nonce, m_iRICON_TS3TabSequence);
	}

	protected void RICON_ClearTs3TabOwner()
	{
		RICON_RetireTs3TabChallenge();
		if (m_RICON_TS3TabActor)
		{
			m_RICON_TS3TabActor.RICON_ClearTs3Proof();
			m_RICON_TS3TabActor.RICON_ClearTs3GameContext();
		}
		m_RICON_TS3TabActor = null;
		m_RICON_TS3TabEntity = null;
		m_iRICON_TS3TabStartedAt = -1;
	}

	protected bool RICON_Ts3TabAttemptCurrent(BaseWorld world, RICON_GameRadioPttContext expected)
	{
		IEntity entity;
		SCR_PlayerController actor = RICON_TS3TabLocalActor(world, entity);
		if (!expected || !actor || actor != m_RICON_TS3TabActor || entity != m_RICON_TS3TabEntity) return false;
		RICON_GameRadioPttContext current = actor.RICON_ReadTs3GameContext(m_iRICON_TS3TabSequence);
		int nowMs = System.GetTickCount();
		return nowMs >= m_iRICON_TS3TabLastTick
			&& RICON_TS3GameBridge.TabAttemptLive(m_iRICON_TS3TabStartedAt, nowMs) && expected.Same(current);
	}

	protected void RICON_TickTs3TabBinding(BaseWorld world)
	{
		if (!m_bRICON_TS3TabEnabled) return;
		int nowMs = System.GetTickCount();
		if (nowMs < 0 || nowMs >= int.MAX || nowMs < m_iRICON_TS3TabLastTick)
		{
			RICON_ClearTs3TabOwner();
			m_bRICON_TS3TabEnabled = false;
			return;
		}
		if (m_iRICON_TS3TabLastTick >= 0 && nowMs - m_iRICON_TS3TabLastTick < 100) return;
		m_iRICON_TS3TabLastTick = nowMs;
		IEntity entity;
		SCR_PlayerController actor = RICON_TS3TabLocalActor(world, entity);
		if (!actor)
		{
			if (m_RICON_TS3TabActor || m_iRICON_TS3TabStartedAt >= 0) RICON_ClearTs3TabOwner();
			return;
		}
		if (actor != m_RICON_TS3TabActor || entity != m_RICON_TS3TabEntity)
		{
			RICON_ClearTs3TabOwner();
			m_RICON_TS3TabActor = actor;
			m_RICON_TS3TabEntity = entity;
		}
		actor.RICON_PollTs3Proof();
		if (m_iRICON_TS3TabStartedAt >= 0 && nowMs - m_iRICON_TS3TabStartedAt >= m_iRICON_TS3TabWindowMs)
		{
			RICON_RetireTs3TabChallenge();
			m_iRICON_TS3TabStartedAt = -1;
		}
		if (m_iRICON_TS3TabStartedAt < 0)
		{
			if (m_iRICON_TS3TabSequence == int.MAX) return;
			m_iRICON_TS3TabStartedAt = nowMs;
			m_iRICON_TS3TabWindowMs = RICON_TS3GameBridge.TAB_ATTEMPT_MS;
			m_iRICON_TS3TabSequence++;
			m_bRICON_TS3TabConsumed = !actor.RICON_RequestTs3GameContext(m_iRICON_TS3TabSequence);
			return;
		}
		if (m_bRICON_TS3TabConsumed) return;
		RICON_PollTs3TabChallenge(world, actor);
	}

	protected void RICON_PollTs3TabChallenge(BaseWorld world, SCR_PlayerController actor)
	{
		if (m_sRICON_TS3TabNonce.IsEmpty())
		{
			ref RICON_GameRadioPttContext current = actor.RICON_ReadTs3GameContext(m_iRICON_TS3TabSequence);
			if (!current || !RICON_Ts3TabAttemptCurrent(world, current)) return;
			m_sRICON_TS3TabNonce = RICON_TS3GameBridge.NewTabNonce();
			if (!RICON_TS3GameBridge.WriteTabQuery(m_sRICON_TS3TabNonce, m_iRICON_TS3TabSequence)
				|| !RICON_Ts3TabAttemptCurrent(world, current))
			{
				RICON_RetireTs3TabChallenge();
				return;
			}
			m_RICON_TS3TabContext = current;
			return;
		}
		ref RICON_GameRadioPttContext expected = m_RICON_TS3TabContext;
		if (!RICON_Ts3TabAttemptCurrent(world, expected))
		{
			RICON_RetireTs3TabChallenge();
			return;
		}
		int revision;
		string serverUid, clientUid;
		if (!RICON_TS3GameBridge.ReadTabResponse(m_sRICON_TS3TabNonce, m_iRICON_TS3TabSequence, revision, serverUid, clientUid)) return;
		RICON_RetireTs3TabChallenge();
		if (!RICON_Ts3TabAttemptCurrent(world, expected)) return;
		if (actor.RICON_RequestTs3Binding(m_iRICON_TS3TabSequence, revision, serverUid, clientUid))
		{
			// Keep a successful pending receipt available for its full validity window.
			m_iRICON_TS3TabStartedAt = System.GetTickCount();
			m_iRICON_TS3TabWindowMs = RICON_GameTs3BindingStore.TTL_MS;
		}
	}

	protected void RICON_TickTs3Route(BaseWorld world)
	{
		if (!m_RICON_TS3RouteEnabled || !m_RICON_TS3World) return;
		int now = System.GetTickCount();
		if (now < 0 || now < m_iRICON_TS3RouteLastTick)
		{
			RICON_TS3GameBridge.ClearRouteResponse();
			m_RICON_TS3RouteEnabled = false;
			return;
		}
		if (m_iRICON_TS3RouteLastTick >= 0 && now - m_iRICON_TS3RouteLastTick < 20) return;
		m_iRICON_TS3RouteLastTick = now;
		string nonce;
		int sequence;
		if (!RICON_TS3GameBridge.ReadRouteQuery(nonce, sequence))
		{
			RICON_TS3GameBridge.ClearRouteResponse();
			return;
		}
		bool answered = nonce == m_sRICON_TS3RouteAnsweredNonce && sequence == m_iRICON_TS3RouteAnsweredSequence;
		if (answered && !m_bRICON_TS3RouteAnswerActive) return;
		SCR_PlayerController actor;
		if (RICON_TS3GameBridge.RouteRequested() && world == m_RICON_TS3World && world == GetWorld()
			&& !world.IsEditMode() && GetGameStarted() && RICON_CommsManagerComponent.GetInstance()
			&& !Replication.IsServer() && Replication.IsClient() && RplSession.Mode() == RplMode.Client)
			actor = SCR_PlayerController.Cast(GetPlayerController());
		if (nonce != m_sRICON_TS3RouteNonce || sequence != m_iRICON_TS3RouteSequence)
		{
			m_sRICON_TS3RouteNonce = nonce; m_iRICON_TS3RouteSequence = sequence;
			m_iRICON_TS3RouteStarted = now; m_RICON_TS3RouteActor = actor;
			m_iRICON_TS3RouteRequest = 0;
			if (actor) m_iRICON_TS3RouteRequest = actor.RICON_RequestTs3Route();
		}
		int remaining;
		ref RICON_GameTs3RouteReply reply;
		if (actor && actor == m_RICON_TS3RouteActor && m_iRICON_TS3RouteRequest > 0)
			reply = actor.RICON_ReadTs3Route(m_iRICON_TS3RouteRequest, remaining);
		if (answered)
		{
			// Retire the exported row on local hold loss without renewing the original budget.
			if (!reply && RICON_TS3GameBridge.ReplyRoute(nonce, sequence, m_RICON_TS3Session, null, 0))
				m_bRICON_TS3RouteAnswerActive = false;
			return;
		}
		if (!reply && actor && actor == m_RICON_TS3RouteActor && m_iRICON_TS3RouteRequest > 0
			&& now - m_iRICON_TS3RouteStarted < RICON_GameTs3RouteWire.MAX_AGE_MS)
		{
			SCR_VONController hook = SCR_VONController.Cast(actor.FindComponent(SCR_VONController));
			int heldRemaining;
			if (hook && hook.RICON_ReadPttMetadata(heldRemaining) && heldRemaining > 0) return;
		}
		if (RICON_TS3GameBridge.ReplyRoute(nonce, sequence, m_RICON_TS3Session, reply, remaining))
		{
			m_sRICON_TS3RouteAnsweredNonce = nonce; m_iRICON_TS3RouteAnsweredSequence = sequence;
			m_bRICON_TS3RouteAnswerActive = reply && remaining > 0;
		}
	}

	override bool OnGameStart()
	{
		bool started = super.OnGameStart();
		if (m_RICON_RadioDevHud) m_RICON_RadioDevHud.Close();
		m_RICON_RadioDevHud = null;
		if (started && RICON_RadioDev.Enabled() && RplSession.Mode() != RplMode.Dedicated)
			m_RICON_RadioDevHud = new RICON_RadioDevHud();
		RICON_ClearTs3TabOwner();
		m_iRICON_TS3TabLastTick = -1;
		m_bRICON_TS3TabEnabled = started && RICON_TS3GameBridge.TabBindingRequested();
		m_RICON_TS3TabWorld = null;
		if (m_bRICON_TS3TabEnabled) m_RICON_TS3TabWorld = GetWorld();
		m_RICON_TS3World = null;
		m_RICON_TS3Active = false;
		m_RICON_TS3LastTick = -1;
		m_RICON_TS3PttLastNonce = "";
		m_RICON_TS3PttLastSequence = 0;
		m_RICON_TS3RouteEnabled = false;
		m_bRICON_TS3RouteAnswerActive = false;
		m_sRICON_TS3RouteNonce = ""; m_sRICON_TS3RouteAnsweredNonce = "";
		m_iRICON_TS3RouteSequence = 0; m_iRICON_TS3RouteAnsweredSequence = 0;
		m_iRICON_TS3RouteStarted = -1; m_iRICON_TS3RouteLastTick = -1; m_iRICON_TS3RouteRequest = 0;
		m_RICON_TS3RouteActor = null;
		m_RICON_TS3LiveEnabled = false;
		m_RICON_TS3PttEnabled = false;
		if (!started || m_RICON_TS3Session == int.MAX) return started;
		if (RICON_TS3GameBridge.Requested())
		{
			m_RICON_TS3LiveEnabled = RICON_TS3GameBridge.SelfCheckLive();
			if (!m_RICON_TS3LiveEnabled) Print("[RICON_TS3] SELF_CHECK_FAILED", LogLevel.ERROR);
		}
		if (RICON_TS3GameBridge.PttRequested())
		{
			RICON_TS3GameBridge.ClearPttResponse();
			m_RICON_TS3PttEnabled = RICON_TS3GameBridge.SelfCheckPtt();
			if (!m_RICON_TS3PttEnabled) Print("[RICON_TS3_PTT] SELF_CHECK_FAILED", LogLevel.ERROR);
		}
		if (RICON_TS3GameBridge.RouteRequested())
		{
			RICON_TS3GameBridge.ClearRouteResponse();
			m_RICON_TS3RouteEnabled = true;
		}
		if (!m_RICON_TS3LiveEnabled && !m_RICON_TS3PttEnabled && !m_RICON_TS3RouteEnabled) return started;
		m_RICON_TS3Session++;
		m_RICON_TS3World = GetWorld();
		if (m_RICON_TS3LiveEnabled) PrintFormat("[RICON_TS3] bridgeStarted session=%1 metadataOnly=1", m_RICON_TS3Session);
		if (m_RICON_TS3PttEnabled) PrintFormat("[RICON_TS3_PTT] bridgeStarted session=%1 metadataOnly=1", m_RICON_TS3Session);
		if (m_RICON_TS3RouteEnabled) PrintFormat("[RICON_TS3_ROUTE] bridgeStarted session=%1 metadataOnly=1", m_RICON_TS3Session);
		return started;
	}

	override void OnUpdate(BaseWorld world, float timeslice)
	{
		super.OnUpdate(world, timeslice);
		RICON_TickTs3TabBinding(world);
		RICON_TickTs3Route(world);
		if (m_RICON_RadioDevHud)
		{
			if (!world || world != GetWorld() || world.IsEditMode() || !GetGameStarted()) m_RICON_RadioDevHud.Close();
			else
			{
				string route = "OFF";
				if (m_RICON_TS3RouteEnabled)
				{
					route = "WAIT_QUERY";
					if (m_iRICON_TS3RouteSequence > 0) route = "PENDING";
					if (m_iRICON_TS3RouteAnsweredSequence > 0 && m_iRICON_TS3RouteAnsweredSequence == m_iRICON_TS3RouteSequence
						&& m_sRICON_TS3RouteAnsweredNonce == m_sRICON_TS3RouteNonce) route = "INACTIVE";
					if (route == "INACTIVE" && m_bRICON_TS3RouteAnswerActive) route = "ACTIVE";
				}
				string bridge = "Transport: stock VoN";
				if (m_RICON_TS3LiveEnabled || m_bRICON_TS3TabEnabled || m_RICON_TS3PttEnabled || m_RICON_TS3RouteEnabled)
					bridge = string.Format("TS3 metadata  tab=%1  PTT=%2  route=%3", m_bRICON_TS3TabEnabled, m_RICON_TS3PttEnabled, route);
				m_RICON_RadioDevHud.Update(SCR_PlayerController.Cast(GetPlayerController()), bridge);
			}
		}
		if (!m_RICON_TS3World) return;
		int now = System.GetTickCount();
		if (now >= m_RICON_TS3LastTick && now - m_RICON_TS3LastTick < 100) return;
		m_RICON_TS3LastTick = now;
		PlayerController local = GetPlayerController();
		int player = 0;
		if (local) player = local.GetPlayerId();
		bool currentLocal = world && world == m_RICON_TS3World && world == GetWorld() && !world.IsEditMode()
			&& GetGameStarted() && RICON_CommsManagerComponent.GetInstance() && local && player > 0 && local.GetControlledEntity();
		if (m_RICON_TS3LiveEnabled)
		{
			bool active = RICON_TS3GameBridge.Requested() && currentLocal;
			if (!active) player = 0;
			if (active != m_RICON_TS3Active)
				PrintFormat("[RICON_TS3] active=%1 session=%2 player=%3", active, m_RICON_TS3Session, player);
			m_RICON_TS3Active = active;
			RICON_TS3GameBridge.Reply(m_RICON_TS3Session, player, active);
		}
		if (m_RICON_TS3PttEnabled)
		{
			SCR_PlayerController actor;
			if (currentLocal) actor = SCR_PlayerController.Cast(local);
			RICON_TS3GameBridge.ReplyPtt(m_RICON_TS3Session, actor, m_RICON_TS3PttLastNonce, m_RICON_TS3PttLastSequence);
		}
	}

	override void OnGameEnd()
	{
		if (m_RICON_RadioDevHud) m_RICON_RadioDevHud.Close();
		m_RICON_RadioDevHud = null;
		RICON_ClearTs3TabOwner();
		m_RICON_TS3TabWorld = null;
		m_bRICON_TS3TabEnabled = false;
		if (m_RICON_TS3RouteEnabled) RICON_TS3GameBridge.ClearRouteResponse();
		m_RICON_TS3RouteEnabled = false; m_RICON_TS3RouteActor = null;
		if (m_RICON_TS3World && m_RICON_TS3LiveEnabled)
		{
			RICON_TS3GameBridge.Reply(m_RICON_TS3Session, 0, false);
			PrintFormat("[RICON_TS3] bridgeEnded session=%1", m_RICON_TS3Session);
		}
		if (m_RICON_TS3World && m_RICON_TS3PttEnabled)
		{
			RICON_TS3GameBridge.ClearPttResponse();
			PrintFormat("[RICON_TS3_PTT] bridgeEnded session=%1", m_RICON_TS3Session);
		}
		m_RICON_TS3World = null;
		m_RICON_TS3Active = false;
		m_RICON_TS3LiveEnabled = false;
		m_RICON_TS3PttEnabled = false;
		super.OnGameEnd();
	}
}
