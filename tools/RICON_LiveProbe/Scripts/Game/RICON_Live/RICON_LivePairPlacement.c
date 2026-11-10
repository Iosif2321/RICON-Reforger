//! Один passive owner-placement capability. Ни RPC, ни Pulse/REST/capture здесь нет.
class RICON_PairPlacementGate
{
	static const int LIMIT_MS = 55000;
	static const int SAMPLE_MS = 500;
	static const int SAMPLE_COUNT = 8;
	static const float BODY_RADIUS = 0.5;
	static const float BODY_HEIGHT = 2;
	static bool Exact(bool configured, string value) { return configured && value == "1"; }
	static bool WorldAllowed(string worldFile)
	{
		if (worldFile == "$ArmaReforger:worlds/MP/MpTest/MpTest.ent") return true;
		worldFile.Replace("\\", "/"); worldFile.ToLower();
		return worldFile == "{96a8af57260a7392}worlds/mp/mptest/mptest.ent" || worldFile == "worlds/mp/mptest/mptest.ent";
	}
	static bool Finite(vector position)
	{
		return RICON_Num.IsFinite(position[0]) && RICON_Num.IsFinite(position[1]) && RICON_Num.IsFinite(position[2]);
	}
	//! Только публичные формы выбранного asset; произвольные/absolute пути не выводятся.
	static string WorldRepresentation(string value)
	{
		if (value.IsEmpty()) return "EMPTY";
		value.Replace("\\", "/");
		string normalized = value; normalized.ToLower();
		string asset = "worlds/mp/mptest/mptest.ent";
		string expectedGuid = "{96a8af57260a7392}";
		if (normalized == asset || normalized == expectedGuid + asset || normalized == expectedGuid || normalized == "mptest.ent") return value;
		if (value.Length() <= asset.Length() || value.Length() > 128) return "OTHER_MASKED";
		int prefixLength = value.Length() - asset.Length();
		if (normalized.Substring(prefixLength, asset.Length()) != asset) return "OTHER_MASKED";
		string prefix = value.Substring(0, prefixLength);
		if (prefix.Length() < 3 || prefix.Length() > 48 || prefix.Substring(0, 1) != "$" || prefix.Substring(prefix.Length() - 1, 1) != ":") return "OTHER_MASKED";
		for (int i = 1; i < prefix.Length() - 1; i++)
		{
			int code = prefix.ToAscii(i);
			bool letter = (code >= 65 && code <= 90) || (code >= 97 && code <= 122);
			if (!letter && (code < 48 || code > 57) && code != 45 && code != 95) return "OTHER_MASKED";
		}
		return value;
	}
	static bool NearCandidateXZ(vector position)
	{
		if (!Finite(position)) return false;
		return vector.DistanceSqXZ(position, Vector(28, 0, 128)) <= BODY_HEIGHT * BODY_HEIGHT;
	}
	static bool OwnerAllowed(int player, int localPlayer, bool client, bool server, bool refs, bool alive, bool onFoot, bool root)
	{
		return player == 2 && localPlayer == 2 && client && !server && refs && alive && onFoot && root;
	}
}

//! Live test geometry; никаких cached terminal/admission или native voice radius inference.
class RICON_CaptureSeparatedGate
{
	static bool Configured()
	{
		string value;
		return System.GetCLIParam("riconCaptureSeparatedPair", value);
	}
	static bool Requested()
	{
		string value;
		return System.GetCLIParam("riconCaptureSeparatedPair", value) && value == "1" && !RICON_LivePairPlacement.Configured();
	}
	static bool Eligible(vector a, vector b, vector radio, int player, float surfaceA, float surfaceB)
	{
		if (!RICON_PairPlacementGate.Finite(a) || !RICON_PairPlacementGate.Finite(b) || !RICON_PairPlacementGate.Finite(radio)) return false;
		if (!RICON_Num.IsFinite(surfaceA) || !RICON_Num.IsFinite(surfaceB) || (player != 1 && player != 2)) return false;
		if (vector.DistanceSqXZ(a, Vector(128, 0, 128)) > 0.25 || !RICON_PairPlacementGate.NearCandidateXZ(b)) return false;
		if (Math.AbsFloat(a[1] - surfaceA) > 0.5 || Math.AbsFloat(b[1] - surfaceB) > 0.5) return false;
		float distance = vector.Distance(a, b);
		if (Math.AbsFloat(distance - 100) > 2) return false;
		if (player == 1) return vector.Distance(radio, a) <= 0.5;
		return vector.Distance(radio, b) <= 0.5;
	}
	static bool Fresh(int observed, int now) { return observed >= 0 && now >= observed && now - observed <= 500; }
}

class RICON_SeparatedPairGeometry
{
	IEntity ActorA;
	IEntity ActorB;
	string ARpl = "UNKNOWN";
	string BRpl = "UNKNOWN";
	string View = "UNKNOWN";
	int Binding;
	int ObservedAt;
	bool Eligible;
	vector APosition;
	vector BPosition;
	vector OwnRadioPosition;
	float OwnRadioOffset;
	int CatalogCount;
	string NativeIndices;
	bool SameIdentity(RICON_SeparatedPairGeometry other)
	{
		if (!ActorA || !ActorB || Binding < 1 || ARpl == "UNKNOWN" || BRpl == "UNKNOWN") return false;
		return other && ActorA == other.ActorA && ActorB == other.ActorB && ARpl == other.ARpl && BRpl == other.BRpl && Binding == other.Binding;
	}
	string Text()
	{
		string fields = "geoView=" + View + ",geoKnown=1,geoEligible=";
		if (Eligible) fields = fields + "1"; else fields = fields + "0";
		fields = fields + ",geoActorARpl=" + ARpl + ",geoActorBRpl=" + BRpl;
		fields = fields + ",geoAX=" + APosition[0].ToString() + ",geoAY=" + APosition[1].ToString();
		fields = fields + ",geoAZ=" + APosition[2].ToString() + ",geoBX=" + BPosition[0].ToString();
		fields = fields + ",geoBY=" + BPosition[1].ToString() + ",geoBZ=" + BPosition[2].ToString();
		fields = fields + ",geoDistance3D=" + vector.Distance(APosition, BPosition).ToString() + ",geoOwnRadioKnown=1";
		fields = fields + ",geoOwnRadioX=" + OwnRadioPosition[0].ToString() + ",geoOwnRadioY=" + OwnRadioPosition[1].ToString();
		fields = fields + ",geoOwnRadioZ=" + OwnRadioPosition[2].ToString() + ",geoOwnRadioOffset=" + OwnRadioOffset.ToString();
		fields = fields + ",geoCatalogCount=" + CatalogCount.ToString() + ",geoNativeIndices=" + NativeIndices;
		return fields + ",geoObservedTickMs=" + ObservedAt.ToString() + ",geoScope=LIVE_REFERENCES";
	}
}

class RICON_PairPlacementAttempt
{
	bool Used;
	bool Begin(bool valid)
	{
		if (Used) return false;
		Used = true;
		return valid;
	}
}

class RICON_LivePairPlacement
{
	protected RICON_LiveNativeRxProbe m_Probe;
	protected BaseWorld m_World;
	protected PlayerController m_Controller;
	protected IEntity m_Own;
	protected IEntity m_A;
	protected IEntity m_B;
	protected string m_ARpl;
	protected string m_BRpl;
	protected string m_Run;
	protected string m_View = "UNBOUND";
	protected int m_Player = -1;
	protected int m_Binding;
	protected int m_StartedAt;
	protected int m_Last;
	protected int m_LastSample = -1;
	protected int m_Samples;
	protected int m_LocalSettleSamples;
	protected int m_SetterResult = -1;
	protected bool m_Finished;
	protected bool m_PairBound;
	protected bool m_ServerSampling;
	protected bool m_ServerBaseline;
	protected bool m_Separated;
	protected ref RICON_PairPlacementAttempt m_Attempt = new RICON_PairPlacementAttempt();
	protected vector m_Target;
	protected vector m_FirstA;
	static bool Configured()
	{
		string value;
		return System.GetCLIParam("riconPairPlacement", value);
	}
	protected static bool Conflict(bool separated = false)
	{
		array<string> flags = {"riconRestCapability", "riconRestCapturePulse", "riconRestRehearsalArm", "riconPulseMode", "riconPulseAttempt",
			"riconCaptureNoSend", "riconCaptureWrongFrequency", "riconAudioMeter", "riconCommunicationDiscovery", "riconVoNInputShape", "riconVoNMapping"};
		string value;
		foreach (string flag : flags)
		{
			if (separated && flag == "riconCaptureWrongFrequency") continue;
			if (separated && Replication.IsServer() && (flag == "riconRestCapability" || flag == "riconRestCapturePulse"
				|| flag == "riconPulseMode" || flag == "riconPulseAttempt" || flag == "riconCaptureNoSend")) continue;
			if (System.GetCLIParam(flag, value)) return true;
		}
		return false;
	}
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
	protected static vector Position(IEntity entity)
	{
		vector matrix[4]; entity.GetWorldTransform(matrix); return matrix[3];
	}
	protected static bool Alive(IEntity entity)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		return character && character.GetCharacterController() && character.GetCharacterController().GetLifeState() == ECharacterLifeState.ALIVE;
	}
	protected bool OwnerCurrent(int now)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || controller != m_Controller || controller.GetControlledEntity() != m_Own || m_Own != m_B) return false;
		if (m_Probe.RICON_PairPlacementBinding(now, m_Run, m_World, controller, m_Own) != m_Binding) return false;
		SCR_CompartmentAccessComponent compartment = SCR_CompartmentAccessComponent.Cast(m_B.FindComponent(SCR_CompartmentAccessComponent));
		bool onFoot = !compartment || !compartment.GetVehicle();
		bool localRef = SCR_PlayerController.GetLocalControlledEntity() == m_B;
		return RICON_PairPlacementGate.OwnerAllowed(controller.GetPlayerId(), SCR_PlayerController.GetLocalPlayerId(),
			Replication.IsClient(), Replication.IsServer(), localRef, Alive(m_B), onFoot, m_B.GetRootParent() == m_B);
	}
	protected bool PairCurrent()
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager || !m_A || !m_B || GetGame().GetWorld() != m_World || m_World.IsEditMode()) return false;
		return manager.GetPlayerControlledEntity(1) == m_A && manager.GetPlayerControlledEntity(2) == m_B
			&& m_A.GetWorld() == m_World && m_B.GetWorld() == m_World && Rpl(m_A) == m_ARpl && Rpl(m_B) == m_BRpl
			&& Alive(m_A) && Alive(m_B);
	}
	protected void Emit(string kind, string fields)
	{
		string common = "rawTickMs=" + System.GetTickCount().ToString() + ",startTickMs=" + m_StartedAt.ToString();
		common = common + ",localPlayer=" + m_Player.ToString() + ",bindingGeneration=" + m_Binding.ToString();
		common = common + ",attempted=" + Bit(m_Attempt.Used) + ",setterResult=" + m_SetterResult.ToString();
		PrintFormat("[RICON_PLACE] run=%1 view=%2 kind=%3 %4,%5,actualCaptureCalls=0", m_Run, m_View, kind, common, fields);
	}
	bool Start(BaseWorld world, RICON_LiveNativeRxProbe probe)
	{
		m_World = world; m_Probe = probe; m_StartedAt = System.GetTickCount(); m_Last = m_StartedAt;
		string flag;
		if (!System.GetCLIParam("riconNativeRxRun", m_Run) || !RICON_D0Gate.ValidRunId(m_Run)) { m_Run = "UNKNOWN"; Stop("REFUSED", "RUN"); return false; }
		bool configured;
		if (m_Separated) configured = System.GetCLIParam("riconCaptureSeparatedPair", flag);
		else configured = System.GetCLIParam("riconPairPlacement", flag);
		bool exact = RICON_PairPlacementGate.Exact(configured, flag); bool conflict = Conflict(m_Separated);
		if (RICON_CaptureSeparatedGate.Configured() && RICON_LivePairPlacement.Configured()) conflict = true;
		bool probePresent = probe != null; bool worldPresent = world != null; int edit = -1;
		if (world)
		{
			edit = 0;
			if (world.IsEditMode()) edit = 1;
		}
		Game game = GetGame(); bool fileKnown = game != null; string worldFile;
		if (game) worldFile = game.GetWorldFile();
		bool allowed = RICON_PairPlacementGate.WorldAllowed(worldFile);
		string check = "flagPresent=" + Bit(configured) + ",flagExact=" + Bit(exact);
		check = check + ",conflict=" + Bit(conflict) + ",probePresent=" + Bit(probePresent);
		check = check + ",worldPresent=" + Bit(worldPresent) + ",worldEdit=" + edit.ToString();
		check = check + ",worldFileKnown=" + Bit(fileKnown) + ",worldFileEmpty=" + Bit(worldFile.IsEmpty());
		check = check + ",worldAllowed=" + Bit(allowed) + ",worldFileLength=" + worldFile.Length().ToString();
		check = check + ",worldFileRepr=" + RICON_LiveText.Esc(RICON_PairPlacementGate.WorldRepresentation(worldFile));
		Emit("CONFIG_CHECK", check + ",worldRepresentationScope=PUBLIC_ASSET_ONLY");
		if (!exact) { Stop("REFUSED", "FLAG_NOT_EXACT"); return false; }
		if (conflict) { Stop("REFUSED", "CONFLICTING_FLAGS"); return false; }
		if (!probePresent) { Stop("REFUSED", "PROBE_MISSING"); return false; }
		if (!worldPresent) { Stop("REFUSED", "WORLD_MISSING"); return false; }
		if (edit != 0) { Stop("REFUSED", "EDIT_WORLD"); return false; }
		if (!fileKnown || worldFile.IsEmpty()) { Stop("REFUSED", "WORLD_FILE_EMPTY_OR_UNKNOWN"); return false; }
		if (!allowed) { Stop("REFUSED", "WORLD_NOT_ALLOWED"); return false; }
		if (m_Separated && Replication.IsServer())
		{
			string pulseMode; System.GetCLIParam("riconPulseMode", pulseMode);
			if (!RICON_RestCaptureGate.Requested() || pulseMode != "capture") { Stop("REFUSED", "SEPARATED_REQUIRES_GUARDED_CAPTURE"); return false; }
		}
		if (m_Separated && Replication.IsServer()) Emit("START", "captureIngress=EXISTING_GUARDED_A,placementScope=CAPTURE_SEPARATED,limitMs=55000,candidateX=28,candidateZ=128");
		else if (m_Separated) Emit("START", "captureIngress=ABSENT,placementScope=CAPTURE_SEPARATED,limitMs=55000,candidateX=28,candidateZ=128");
		else Emit("START", "captureIngress=ABSENT,limitMs=55000,candidateX=28,candidateZ=128");
		return true;
	}
	bool StartSeparated(BaseWorld world, RICON_LiveNativeRxProbe probe)
	{
		m_Separated = true;
		return Start(world, probe);
	}
	protected bool Landing()
	{
		if (!GetGame().GetWorldEntity() || !GetGame().GetWorldEntity().GetTerrain(0, 0)) return false;
		float surface = m_World.GetSurfaceY(28, 128);
		if (!RICON_Num.IsFinite(surface)) return false;
		vector center = Vector(28, surface, 128);
		if (!SCR_Global.IsPositionWithinTerrainBounds(center)) return false;
		// Размеры взяты из defaults штатного helper; это четыре trace, не полная capsule гарантия.
		if (!SCR_WorldTools.FindEmptyTerrainPosition(m_Target, center, RICON_PairPlacementGate.BODY_HEIGHT,
			RICON_PairPlacementGate.BODY_RADIUS, RICON_PairPlacementGate.BODY_HEIGHT, TraceFlags.ENTS | TraceFlags.OCEAN | TraceFlags.WORLD, m_World)) return false;
		if (!RICON_PairPlacementGate.Finite(m_Target) || !SCR_Global.IsPositionWithinTerrainBounds(m_Target)) return false;
		string fields = "surfaceY=" + surface.ToString() + ",targetX=" + m_Target[0].ToString();
		fields = fields + ",targetY=" + m_Target[1].ToString() + ",targetZ=" + m_Target[2].ToString();
		Emit("LANDING", fields + ",freeCylinderObserved=1,safeCollisionGuarantee=UNQUALIFIED");
		return true;
	}
	void Tick(BaseWorld world)
	{
		if (m_Finished) return;
		int now = System.GetTickCount();
		if (world != m_World || !world || world.IsEditMode() || now < m_Last || now - m_Last > 5000)
		{ Stop("INCOMPLETE", "WORLD_OR_CLOCK"); return; }
		m_Last = now;
		if (now - m_StartedAt >= RICON_PairPlacementGate.LIMIT_MS) { Stop("INCOMPLETE", "LIMIT"); return; }
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller) { if (m_PairBound) Stop("INCOMPLETE", "IDENTITY_LOST"); return; }
		IEntity entity = controller.GetControlledEntity();
		int generation = m_Probe.RICON_PairPlacementBinding(now, m_Run, world, controller, entity);
		if (generation < 1) { if (m_PairBound) Stop("INCOMPLETE", "BINDING_CHANGED"); return; }
		int player = controller.GetPlayerId();
		if (!m_PairBound)
		{
			if (player == 1 && Replication.IsServer()) m_View = "A_SERVER";
			else if (player == 2 && Replication.IsClient() && !Replication.IsServer()) m_View = "B_LOCAL";
			else { Stop("REFUSED", "ROLE"); return; }
			PlayerManager manager = GetGame().GetPlayerManager();
			if (!manager) return;
			m_A = manager.GetPlayerControlledEntity(1); m_B = manager.GetPlayerControlledEntity(2);
			if (!m_A || !m_B || m_A == m_B || m_A.GetWorld() != world || m_B.GetWorld() != world) return;
			m_ARpl = Rpl(m_A); m_BRpl = Rpl(m_B); if (m_ARpl == "UNKNOWN" || m_BRpl == "UNKNOWN") return;
			if ((player == 1 && entity != m_A) || (player == 2 && entity != m_B)) { Stop("REFUSED", "OWN_PAIR_MISMATCH"); return; }
			m_Controller = controller; m_Own = entity; m_Player = player; m_Binding = generation;
			m_PairBound = true; m_FirstA = Position(m_A);
			if (!RICON_PairPlacementGate.Finite(m_FirstA) || !RICON_PairPlacementGate.Finite(Position(m_B))) { Stop("REFUSED", "NONFINITE_PAIR"); return; }
		}
		if (controller != m_Controller || entity != m_Own || generation != m_Binding || !PairCurrent())
		{ Stop("INCOMPLETE", "PAIR_REF_OR_LIFE_CHANGED"); return; }
		if (m_Player == 2 && !m_Attempt.Used)
		{
			if (!OwnerCurrent(now) || !Landing()) { Stop("REFUSED", "OWNER_OR_LANDING"); return; }
			now = System.GetTickCount();
			bool current = PairCurrent() && OwnerCurrent(now);
			if (!m_Attempt.Begin(current)) { Stop("REFUSED", "FINAL_OWNER_CONTEXT"); return; }
			m_SetterResult = 0;
			if (SCR_Global.TeleportLocalPlayer(m_Target, SCR_EPlayerTeleportedReason.DEFAULT)) m_SetterResult = 1;
			Emit("MOVE_RETURN", "operation=B_LOCAL_OWNER_ONCE,networkConvergence=UNQUALIFIED");
			if (m_SetterResult != 1 || !PairCurrent()) { Stop("INCOMPLETE", "SETTER_OR_POST_CONTEXT"); return; }
		}
		if (m_Player == 2 && m_Attempt.Used && !OwnerCurrent(System.GetTickCount())) { Stop("INCOMPLETE", "POST_OWNER_CHANGED"); return; }
		if (m_Player == 1 && !m_ServerSampling)
		{
			vector observedB = Position(m_B);
			if (!RICON_PairPlacementGate.Finite(observedB)) { Stop("INCOMPLETE", "NONFINITE_SERVER_B"); return; }
			if (!m_ServerBaseline)
			{
				m_ServerBaseline = true;
				string baseline = "bX=" + observedB[0].ToString() + ",bY=" + observedB[1].ToString();
				baseline = baseline + ",bZ=" + observedB[2].ToString() + ",postPlacementBudgetUsed=0";
				Emit("BASELINE", baseline);
			}
			if (!RICON_PairPlacementGate.NearCandidateXZ(observedB)) return;
			m_ServerSampling = true;
			Emit("POST_PLACEMENT_WINDOW", "trigger=SERVER_B_IN_CANDIDATE_XZ,agreement=OPERATOR_ONLY");
		}
		if (m_LastSample >= 0 && now - m_LastSample < RICON_PairPlacementGate.SAMPLE_MS) return;
		m_LastSample = now;
		vector aPosition = Position(m_A); vector bPosition = Position(m_B); vector radioPosition;
		if (!RICON_PairPlacementGate.Finite(aPosition) || !RICON_PairPlacementGate.Finite(bPosition)) { Stop("INCOMPLETE", "NONFINITE"); return; }
		bool radioKnown = m_Probe.RICON_PairPlacementOwnRadioPosition(radioPosition);
		float targetDistance = -1;
		if (m_Player == 2) targetDistance = vector.Distance(bPosition, m_Target);
		if (m_Player == 2 && targetDistance <= RICON_PairPlacementGate.BODY_RADIUS) m_LocalSettleSamples++;
		else m_LocalSettleSamples = 0;
		m_Samples++;
		string fields = "sample=" + m_Samples.ToString() + ",actorARpl=" + m_ARpl + ",actorBRpl=" + m_BRpl;
		fields = fields + ",aX=" + aPosition[0].ToString() + ",aY=" + aPosition[1].ToString();
		fields = fields + ",aZ=" + aPosition[2].ToString() + ",bX=" + bPosition[0].ToString();
		fields = fields + ",bY=" + bPosition[1].ToString() + ",bZ=" + bPosition[2].ToString();
		fields = fields + ",distance3D=" + vector.Distance(aPosition, bPosition).ToString() + ",aDelta=" + vector.Distance(m_FirstA, aPosition).ToString();
		fields = fields + ",ownRadioKnown=" + Bit(radioKnown) + ",ownRadioX=" + radioPosition[0].ToString();
		fields = fields + ",ownRadioY=" + radioPosition[1].ToString() + ",ownRadioZ=" + radioPosition[2].ToString();
		fields = fields + ",targetKnown=" + Bit(m_Player == 2 && m_Attempt.Used) + ",targetX=" + m_Target[0].ToString();
		fields = fields + ",targetY=" + m_Target[1].ToString();
		fields = fields + ",targetZ=" + m_Target[2].ToString() + ",targetDistance=" + targetDistance.ToString();
		fields = fields + ",settleSamples=" + m_LocalSettleSamples.ToString() + ",agreement=OPERATOR_ONLY";
		Emit("SAMPLE", fields);
		if (m_Samples >= RICON_PairPlacementGate.SAMPLE_COUNT) Stop("OBSERVATION_END", "EIGHT_SAMPLES");
	}
	void Stop(string result, string reason)
	{
		if (m_Finished) return;
		m_Finished = true;
		Emit("TERMINAL", "result=" + result + ",reason=" + reason + ",samples=" + m_Samples.ToString() + ",agreement=OPERATOR_ONLY");
		m_Probe = null; m_World = null; m_Controller = null; m_Own = null; m_A = null; m_B = null;
	}
}
