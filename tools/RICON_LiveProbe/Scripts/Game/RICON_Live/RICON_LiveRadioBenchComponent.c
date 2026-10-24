//! TOOL_ONLY: стенд Q-F/Q-M на изолированных рациях (вариант (а′), claude #4089, условия codex #4091, #4105, #4106).
//! Компонент на временной GenericEntity, которую плагин RICON_LiveBenchLauncherPlugin создаёт в памяти World Editor
//! (мир не сохраняется). В Play стенд сам спавнит рации из BenchPrefabs() и работает только с ними: игроков, меню VON
//! и пользовательских раций нет — это «изолированный экземпляр под исключительным управлением» LIVE-order §2 Q-M.
//! Уровень всех записей — NATIVE_ROUNDTRIP: значения getter-ов и ответ Set* на этой рации в этом процессе.
//! UI_MHz, направление RX/TX, слышимость, репликация и per-pair mute этим стендом НЕ проверяются.
//! Фазы (codex #4105): через 1 с после старта — спавн и чтение; через 1 с после спавна — шаг частоты трансивера 0;
//! чтения «t1»/«t5» — когда от фактического вызова setter прошло ≥ 1 / ≥ 5 с, в записи actualDeltaMs; через 7 с после
//! setter — mute на противоположное состояние и возврат; через 9 с — удаление своих раций и BENCH_DONE.
//! Ранний выход (удаление компонента до конца) пишет BENCH_ABORTED с достигнутой фазой, не BENCH_DONE.
//! A0b: наличие RadioManagerEntity пишется в BENCH_START и перед спавном (BENCH_MANAGER); расхождение с явным
//! -riconBenchRadioManager=0|1 — BENCH_NOT_READY и BENCH_ABORTED с фазой 0 до спавна раций; без параметра — только запись.
//! V2 (codex #4229, #4262): метод шага частоты по -riconBenchSetter — без параметра прежний
//! BaseRadioComponent.SetTransceiverFrequency, «transceiver» — прямой BaseTransceiver.SetFrequency; другое значение, в том
//! числе пустое, — BENCH_NOT_READY до спавна. Метод пишется в BENCH_START и в каждый BENCH_SET_FREQUENCY; фазы те же.
//! A1-E (claude #4309, codex #4315, #4326): «entry» — сразу после спавна стенд создаёт для трансивера 0 каждой своей рации
//! собственную ванильную SCR_VONEntryRadio (BENCH_ENTRY), шаг частоты делает её AdjustEntryModif(±1), mute — её
//! ToggleMuteEntry дважды и затем прямой SetMuteState туда и обратно; чтения трансивера 0 дополняются частотой, mute и
//! сырым текстом entry (%XX, RICON_LiveText). Уровень MENU_ENTRY_RUNTIME: исполнен ванильный объект меню на настоящем
//! трансивере, но не экран, не VON-контроллер игрока и не ветка GetKnownChannel. BENCH_START пишет наличие контроллера
//! игрока и менеджера фракций. A1-P0 (claude #4360, codex #4365): режим -riconBenchMode=player вместо радиофаз —
//! 30 записей PLAYER_SAMPLE раз в секунду (роль процесса, игроки, локальный контроллер, управляемая сущность, её VoN
//! и GadgetManager, SCR_VONController) и BENCH_DONE level=PLAYER_OBSERVATION; ни спавна, ни Set*, ни контроля.
//! A1-P1 (claude #4385, codex #4384, #4388): после каждого среза — PLAYER_ENTRY на каждую entry VON-контроллера игрока
//! (только getter-ы, связь entry ↔ трансивер ↔ рация в инвентаре). A1-P2 (claude #4399, codex #4388, #4390): режим
//! «playermenu» — то же плюс одна последовательность пути меню на одной полностью привязанной radio-entry игрока
//! (PLAYER_ACT: выбор, шаг AdjustEntryModif, чтения t0/t1/t5, возврат, ToggleMuteEntry туда и обратно, итог); каждая
//! запись предварена проверкой ожидаемого состояния, иначе STOP без перезаписи; BENCH_DONE level=PLAYER_MENU_RUNTIME.
//! Ключи шифрования не читаются.
class RICON_LiveRadioBenchComponentClass : ScriptComponentClass
{
}

class RICON_LiveRadioBenchComponent : ScriptComponent
{
	//! Точные ResourceName (GUID самого префаба) из вывода RICON_LiveResourceProbePlugin, лог
	//! ricon-liveradio2-claude-20261024-062851 (sha256 4769705f…): четыре модели с паспортными диапазонами в каталоге
	//! radio_model_catalog_v1.json — сырые min/max сравниваются с паспортом офлайн, без вывода единицы в коде.
	static array<string> BenchPrefabs()
	{
		array<string> prefabs = {
			"{73950FBA2D7DB5C5}Prefabs/Items/Equipment/Radios/Radio_ANPRC68.et",
			"{9B6B61BB3FE3DFB0}Prefabs/Items/Equipment/Radios/Radio_ANPRC77.et",
			"{54C68E438DD34265}Prefabs/Items/Equipment/Radios/Radio_R107M.et",
			"{73EFD7591F4134D3}Prefabs/Items/Equipment/Radios/Radio_RF10/Radio_RF10.et"
		};
		return prefabs;
	}

	static const int PHASE_IDLE = 0;
	static const int PHASE_SPAWNED = 1;
	static const int PHASE_STEPPED = 2;
	static const int PHASE_READ_T1 = 3;
	static const int PHASE_READ_T5 = 4;
	static const int PHASE_MUTED = 5;
	static const int PHASE_DONE = 6;

	protected ref RICON_LiveJournal m_Journal;
	protected ref array<string> m_aPrefabs;
	protected ref array<IEntity> m_aRadios = {};       //!< слабые ссылки на свои рации
	protected ref array<int> m_aTargets = {};          //!< целевая сырая частота трансивера 0 после шага или −1
	protected ref array<ref SCR_VONEntryRadio> m_aEntries = {}; //!< A1-E: собственная entry трансивера 0 или null
	protected float m_fElapsed;
	protected float m_fSpawnedAt;
	protected float m_fSetAt;                          //!< игровое время фактического вызова setter
	protected int m_iPhase;
	protected bool m_bRunning;

	protected bool m_bFirstFrameLogged;
	protected bool m_bStarted;

	//! Сердцебиение через игровую очередь (codex #4176): идёт независимо от EOnFrame сущности и отделяет «у сущности
	//! нет кадров» от «игра не обновляется». Повтор раз в HEARTBEAT_MS, не больше MAX_HEARTBEATS; Remove в Finish и OnDelete.
	static const int HEARTBEAT_MS = 1000;
	static const int MAX_HEARTBEATS = 90;
	protected int m_iFrames;
	protected int m_iHeartbeats;
	protected bool m_bHeartbeat;

	//! Внутри ли сущность границ своего мира (BaseWorld.GetBoundBox) по всем трём осям.
	protected static bool IsInside(IEntity owner)
	{
		if (!owner || !owner.GetWorld())
			return false;
		vector mins, maxs;
		owner.GetWorld().GetBoundBox(mins, maxs);
		vector origin = owner.GetOrigin();
		for (int k = 0; k < 3; k++)
		{
			if (origin[k] < mins[k] || origin[k] > maxs[k])
				return false;
		}
		return true;
	}

	//! A0b (codex #4224, #4233): есть ли в мире стенда RadioManagerEntity — ChimeraWorld.GetRadioManager() (Script API).
	protected static bool HasRadioManager(IEntity owner)
	{
		if (!owner)
			return false;
		ChimeraWorld world = ChimeraWorld.CastFrom(owner.GetWorld());
		if (!world)
			return false;
		return world.GetRadioManager() != null;
	}

	static const int MANAGER_OBSERVE_ONLY = -1; //!< параметра нет: наличие только записывается (прежний запуск)
	static const int MANAGER_INVALID = -2;      //!< значение не 0|1, в том числе пустое (launcher отклоняет раньше)

	//! Ожидание по параметру запуска -riconBenchRadioManager (codex #4237): явные 0 / 1 проверяются, без параметра —
	//! только наблюдение, любое другое значение — неготовность.
	protected static int ExpectedManager()
	{
		string value;
		if (!System.GetCLIParam("riconBenchRadioManager", value))
			return MANAGER_OBSERVE_ONLY;
		if (value == "0")
			return 0;
		if (value == "1")
			return 1;
		return MANAGER_INVALID;
	}

	//! Перед спавном (регистрация менеджера может запоздать относительно EOnInit): при явном ожидании фактическое
	//! наличие должно с ним совпасть, иначе стенд не готов — BENCH_NOT_READY и выход до спавна, не результат по рациям.
	protected bool ManagerReady(IEntity owner)
	{
		bool present = HasRadioManager(owner);
		int expected = ExpectedManager();
		string text = "where=preSpawn,radioManager=" + Bit(present) + ",expected=" + expected.ToString();
		int presentValue = 0;
		if (present)
			presentValue = 1;
		Note("BENCH_MANAGER", presentValue, text);
		if (expected == MANAGER_OBSERVE_ONLY)
			return true;
		if (expected == MANAGER_INVALID || present != (expected == 1))
		{
			Note("BENCH_NOT_READY", -1, "reason=RADIO_MANAGER_MISMATCH," + text);
			return false;
		}
		return true;
	}

	static const string SETTER_COMPONENT = "component";     //!< BaseRadioComponent.SetTransceiverFrequency (прежний)
	static const string SETTER_TRANSCEIVER = "transceiver"; //!< BaseTransceiver.SetFrequency
	static const string SETTER_ENTRY = "entry";             //!< A1-E: собственная SCR_VONEntryRadio, путь меню VON
	static const string SETTER_INVALID = "INVALID";         //!< значение вне списка; сырое значение не печатается
	//! A1-E: явная настройка собственной entry до InitEntry (codex #4326). Без неё InitEntry идёт в
	//! SCR_VONMenu.GetKnownChannel, который при менеджере фракций разыменовывает GetPlayerController() без проверки
	//! (SCR_VONMenu.c:41–46, квитанция a1-s0); ветка GetKnownChannel поэтому стендом не исполняется.
	static const string ENTRY_CHANNEL_OVERWRITE = "RICON_BENCH";

	//! Задан ли -riconBenchSetter явно (в том числе пустым).
	protected static bool SetterGiven()
	{
		string value;
		return System.GetCLIParam("riconBenchSetter", value);
	}

	//! Метод шага частоты: без параметра — прежний, иначе только одно из двух точных имён.
	protected static string Setter()
	{
		string value;
		if (!System.GetCLIParam("riconBenchSetter", value))
			return SETTER_COMPONENT;
		if (value == SETTER_COMPONENT || value == SETTER_TRANSCEIVER || value == SETTER_ENTRY)
			return value;
		return SETTER_INVALID;
	}

	//! Перед спавном, после ManagerReady: неизвестный метод — неготовность, а не результат по рациям.
	protected bool SetterReady()
	{
		if (Setter() != SETTER_INVALID)
			return true;
		Note("BENCH_NOT_READY", -1, "reason=SETTER_PARAM,setter=" + SETTER_INVALID + ",setterGiven=" + Bit(SetterGiven()));
		return false;
	}

	//! Положение сущности относительно границ её мира: печатается в OnPostInit и в отказе TryStart.
	protected static string Placement(IEntity owner)
	{
		if (!owner || !owner.GetWorld())
			return "placement=NONE";
		vector mins, maxs;
		owner.GetWorld().GetBoundBox(mins, maxs);
		string text = "origin=" + owner.GetOrigin().ToString() + ",boundsMin=" + mins.ToString();
		text = text + ",boundsMax=" + maxs.ToString() + ",inside=" + Bit(IsInside(owner));
		return text;
	}

	protected void StartHeartbeat()
	{
		if (m_bHeartbeat || !GetGame() || !GetGame().GetCallqueue())
			return;
		GetGame().GetCallqueue().CallLater(Heartbeat, HEARTBEAT_MS, true);
		m_bHeartbeat = true;
		Print("[RICON_LIVEB] heartbeat=START");
	}

	protected void Heartbeat()
	{
		m_iHeartbeats++;
		PrintFormat("[RICON_LIVEB] heartbeat n=%1 frames=%2 phase=%3 running=%4 elapsed=%5", m_iHeartbeats, m_iFrames, m_iPhase, m_bRunning, m_fElapsed);
		if (m_iHeartbeats >= MAX_HEARTBEATS)
			StopHeartbeat("CAP");
	}

	//! A1-P0 (claude #4360, условия codex #4365): режим -riconBenchMode=player — только наблюдение локального игрока.
	//! Ни спавна, ни Set*, ни радиофаз: PLAYER_SAMPLES записей PLAYER_SAMPLE раз в секунду игрового времени, затем
	//! BENCH_DONE value=PLAYER_SAMPLES level=PLAYER_OBSERVATION. Без параметра — прежний радиостенд; другое значение,
	//! в том числе пустое, — BENCH_NOT_READY reason=MODE_PARAM до любых действий.
	static const string MODE_RADIO = "radio";
	static const string MODE_PLAYER = "player";
	static const string MODE_PLAYERMENU = "playermenu"; //!< A1-P2: player + путь меню на одной radio-entry игрока
	static const string MODE_INVALID = "INVALID";
	static const int PLAYER_SAMPLES = 30;
	protected int m_iSamples;
	protected float m_fLastSample;  //!< игровое время последнего среза (0 до первого: первый — при elapsed ≥ 1 с)

	protected static bool ModeGiven()
	{
		string value;
		return System.GetCLIParam("riconBenchMode", value);
	}

	protected static string Mode()
	{
		string value;
		if (!System.GetCLIParam("riconBenchMode", value))
			return MODE_RADIO;
		if (value == MODE_RADIO || value == MODE_PLAYER || value == MODE_PLAYERMENU)
			return value;
		return MODE_INVALID;
	}

	//! Один срез: роль процесса, игроки, локальный контроллер и управляемая сущность (только getter-ы).
	protected static string PlayerState()
	{
		int players = -1;
		if (GetGame().GetPlayerManager())
		{
			array<int> ids = {};
			players = GetGame().GetPlayerManager().GetPlayers(ids);
		}
		string text = "isServer=" + Bit(Replication.IsServer()) + ",isClient=" + Bit(Replication.IsClient());
		text = text + ",players=" + players.ToString();
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return text + ",pc=0";
		text = text + ",pc=1,playerId=" + controller.GetPlayerId().ToString();
		SCR_VONController von = SCR_VONController.Cast(controller.FindComponent(SCR_VONController));
		text = text + ",vonController=" + Bit(von != null);
		if (von)
			text = text + ",vonEntries=" + von.GetVONEntryCount().ToString();
		IEntity controlled = controller.GetControlledEntity();
		text = text + ",controlled=" + Bit(controlled != null);
		if (!controlled)
			return text;
		string prefab = "UNKNOWN";
		EntityPrefabData data = controlled.GetPrefabData();
		if (data)
			prefab = data.GetPrefabName();
		text = text + ",controlledPrefab=" + RICON_LiveText.Esc(prefab);
		text = text + ",entityVoN=" + Bit(controlled.FindComponent(SCR_VoNComponent) != null);
		return text + ",gadgetManager=" + Bit(SCR_GadgetManagerComponent.GetGadgetManager(controlled) != null);
	}

	protected void StopHeartbeat(string reason)
	{
		if (!m_bHeartbeat)
			return;
		m_bHeartbeat = false;
		if (GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().Remove(Heartbeat);
		PrintFormat("[RICON_LIVEB] heartbeat=STOP reason=%1 n=%2 frames=%3", reason, m_iHeartbeats, m_iFrames);
	}

	//! Диагностика жизненного цикла (codex #4129): видно, создаётся ли компонент заново при переходе в Play.
	protected static string WorldState(IEntity owner)
	{
		if (!owner || !owner.GetWorld())
			return "world=NONE";
		bool active = (owner.GetFlags() & EntityFlags.ACTIVE) != 0;
		string text = "world=1,editMode=" + Bit(owner.GetWorld().IsEditMode());
		text = text + ",active=" + Bit(active);
		return text;
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
		PrintFormat("[RICON_LIVEB] lifecycle=OnPostInit %1 %2", WorldState(owner), Placement(owner));
		// codex #4138: ACTIVE у своей сущности стенда сразу, чтобы первый EOnFrame мог вызвать TryStart; логика в
		// edit mode при этом не запускается (TryStart проверяет режим).
		if (owner && (owner.GetFlags() & EntityFlags.ACTIVE) == 0)
			owner.SetFlags(EntityFlags.ACTIVE, false);
	}

	override void EOnInit(IEntity owner)
	{
		PrintFormat("[RICON_LIVEB] lifecycle=EOnInit %1 isServer=%2", WorldState(owner), Replication.IsServer());
		TryStart(owner, "EOnInit");
	}

	//! Общий запуск из EOnInit и первого EOnFrame вне режима редактирования (codex #4131); повторно не стартует.
	//! Если у сущности стенда нет флага ACTIVE («actively updated by engine»), он включается только у неё.
	protected void TryStart(IEntity owner, string from)
	{
		if (m_bStarted)
			return;
		if (!owner || !owner.GetWorld() || owner.GetWorld().IsEditMode())
			return;
		m_bStarted = true;
		// codex #4180: координаты проверяются и в игровом экземпляре; вне границ — явный отказ до BENCH_START.
		if (!IsInside(owner))
		{
			PrintFormat("[RICON_LIVEB] state=PLACEMENT_REFUSED where=play from=%1 %2", from, Placement(owner));
			return;
		}
		bool activeBefore = (owner.GetFlags() & EntityFlags.ACTIVE) != 0;
		if (!activeBefore)
			owner.SetFlags(EntityFlags.ACTIVE, false);
		m_Journal = new RICON_LiveJournal(256);
		m_Journal.SetStream(true);
		m_aPrefabs = BenchPrefabs();
		m_bRunning = true;
		// Роль процесса — из Replication, а не из наличия владельца (codex #4108).
		string text = "level=NATIVE_ROUNDTRIP,prefabs=" + m_aPrefabs.Count().ToString();
		text = text + ",isServer=" + Bit(Replication.IsServer());
		text = text + ",isClient=" + Bit(Replication.IsClient());
		text = text + ",from=" + from + ",activeBefore=" + Bit(activeBefore);
		text = text + ",radioManager=" + Bit(HasRadioManager(owner)) + ",expectedManager=" + ExpectedManager().ToString();
		text = text + ",setter=" + Setter() + ",setterGiven=" + Bit(SetterGiven());
		// A1-E (codex #4327): наличие контроллера и менеджера фракций измеряется, а не предполагается.
		text = text + ",playerController=" + Bit(GetGame().GetPlayerController() != null);
		text = text + ",factionManager=" + Bit(GetGame().GetFactionManager() != null);
		text = text + ",mode=" + Mode() + ",modeGiven=" + Bit(ModeGiven());
		Note("BENCH_START", -1, text);
		StartHeartbeat();
	}

	protected int Note(string kind, int value, string text)
	{
		return m_Journal.Add(RICON_LiveObserver.Now(), "probe", kind, -1, -1, -1, value, text);
	}

	protected int Record(string kind, int device, int transceiver, int value, string text)
	{
		return m_Journal.Add(RICON_LiveObserver.Now(), "probe", kind, -1, -1, transceiver, value, "bench=" + device.ToString() + "," + text);
	}

	protected static string Bit(bool value)
	{
		if (value)
			return "1";
		return "0";
	}

	//! Миллисекунды от фактического вызова setter (−1 до вызова).
	protected int DeltaMs()
	{
		if (m_iPhase < PHASE_STEPPED)
			return -1;
		float delta = (m_fElapsed - m_fSetAt) * 1000.0;
		int ms = delta;
		return ms;
	}

	protected BaseRadioComponent RadioOf(int device)
	{
		if (device < 0 || device >= m_aRadios.Count())
			return null;
		IEntity entity = m_aRadios[device];
		if (!entity)
			return null;
		return BaseRadioComponent.Cast(entity.FindComponent(BaseRadioComponent));
	}

	protected void Spawn()
	{
		IEntity owner = GetOwner();
		if (m_aPrefabs.IsEmpty())
		{
			Note("BENCH_NO_PREFABS", -1, "level=NATIVE_ROUNDTRIP");
			return;
		}
		vector origin = owner.GetOrigin();
		for (int i = 0; i < m_aPrefabs.Count(); i++)
		{
			Resource resource = Resource.Load(m_aPrefabs[i]);
			EntitySpawnParams params = new EntitySpawnParams();
			params.TransformMode = ETransformMode.WORLD;
			vector position = origin;
			position[0] = origin[0] + 2.0 * i;
			params.Transform[3] = position;
			IEntity radio;
			if (resource && resource.IsValid())
				radio = GetGame().SpawnEntityPrefab(resource, owner.GetWorld(), params);
			m_aRadios.Insert(radio);
			m_aTargets.Insert(-1);
			m_aEntries.Insert(null);
			int ok = 0;
			string actual = "NONE";
			if (radio)
			{
				ok = 1;
				// Фактический префаб созданной сущности отдельно от запрошенного (codex #4115).
				EntityPrefabData data = radio.GetPrefabData();
				actual = "UNKNOWN";
				if (data)
					actual = data.GetPrefabName();
			}
			Record("BENCH_SPAWN", i, -1, ok, "prefab=" + m_aPrefabs[i] + ",actualPrefab=" + actual);
		}
	}

	protected SCR_VONEntryRadio EntryOf(int device)
	{
		if (device < 0 || device >= m_aEntries.Count())
			return null;
		return m_aEntries[device];
	}

	//! Состояние своей entry: локальная частота entry, mute по entry и сырой текст меню (%XX, RICON_LiveText).
	protected static string EntryText(SCR_VONEntryRadio entry)
	{
		string text = "entryFreq=" + entry.GetEntryFrequency().ToString();
		text = text + ",usable=" + Bit(entry.IsUsable()) + ",emuted=" + Bit(entry.GetIsMuted());
		return text + ",dtext=" + RICON_LiveText.Esc(entry.GetDisplayText());
	}

	//! A1-E: собственная SCR_VONEntryRadio для трансивера 0 своей рации (codex #4315, #4326). Номер трансивера
	//! 1-базный (GetIsMuted берёт GetTransceiver(номер − 1), SCR_VONEntryRadio.c:255); gadget нужен настоящий
	//! (IsLongRange разыменовывает его без проверки, :56–58); channel-overwrite до InitEntry — см. ENTRY_CHANNEL_OVERWRITE.
	//! InitEntry сам вызывает AdjustEntryModif(0): это первое исполнение пути меню, шаг 0 частоту не меняет.
	protected void CreateEntry(int device)
	{
		BaseRadioComponent radio = RadioOf(device);
		BaseTransceiver transceiver;
		if (radio && radio.TransceiversCount() > 0)
			transceiver = radio.GetTransceiver(0);
		if (!transceiver)
		{
			Record("BENCH_ENTRY", device, 0, -1, "phase=init,reason=NO_TRANSCEIVER");
			return;
		}
		SCR_GadgetComponent gadget = SCR_GadgetComponent.Cast(m_aRadios[device].FindComponent(SCR_GadgetComponent));
		if (!gadget)
		{
			Record("BENCH_ENTRY", device, 0, -1, "phase=init,reason=NO_GADGET");
			return;
		}
		int rawBefore = transceiver.GetFrequency();
		SCR_VONEntryRadio entry = new SCR_VONEntryRadio();
		entry.SetRadioEntry(transceiver, 1, gadget);
		entry.SetChannelTextOverwrite(ENTRY_CHANNEL_OVERWRITE);
		entry.InitEntry();
		m_aEntries[device] = entry;
		string text = "phase=init,rawBefore=" + rawBefore.ToString() + ",raw=" + transceiver.GetFrequency().ToString();
		Record("BENCH_ENTRY", device, 0, entry.GetEntryFrequency(), text + "," + EntryText(entry));
	}

	//! Все трансиверы своей рации: сырые getter-ы, без единиц. Цель шага относится только к трансиверу 0.
	protected void Read(int device, string phase)
	{
		BaseRadioComponent radio = RadioOf(device);
		if (!radio)
		{
			Record("BENCH_READ", device, -1, -1, "phase=" + phase + ",radio=NONE");
			return;
		}
		int powered = 0;
		if (radio.IsPowered())
			powered = 1;
		for (int t = 0; t < radio.TransceiversCount(); t++)
		{
			BaseTransceiver transceiver = radio.GetTransceiver(t);
			if (!transceiver)
				continue;
			int muted = 0;
			if (transceiver.IsMuted())
				muted = 1;
			int target = -1;
			if (t == 0)
				target = m_aTargets[device];
			int minRaw = transceiver.GetMinFrequency();
			int maxRaw = transceiver.GetMaxFrequency();
			int resolution = transceiver.GetFrequencyResolution();
			string text = "phase=" + phase + ",actualDeltaMs=" + DeltaMs().ToString();
			text = text + ",min=" + minRaw.ToString();
			text = text + ",max=" + maxRaw.ToString();
			text = text + ",res=" + resolution.ToString();
			text = text + ",powered=" + powered.ToString();
			text = text + ",muted=" + muted.ToString();
			text = text + ",targetTr0=" + target.ToString();
			SCR_VONEntryRadio entry = EntryOf(device);
			if (t == 0 && entry)
				text = text + "," + EntryText(entry);
			Record("BENCH_READ", device, t, transceiver.GetFrequency(), text);
		}
	}

	//! Q-F: трансивер 0 своей рации на один шаг разрешения внутри [min, max]; иначе явный BENCH_SKIP (codex #4106).
	protected void StepFrequency(int device)
	{
		BaseRadioComponent radio = RadioOf(device);
		if (!radio || radio.TransceiversCount() < 1)
		{
			Record("BENCH_SKIP", device, 0, -1, "reason=NO_TRANSCEIVER");
			return;
		}
		BaseTransceiver transceiver = radio.GetTransceiver(0);
		if (!transceiver)
		{
			Record("BENCH_SKIP", device, 0, -1, "reason=NO_TRANSCEIVER");
			return;
		}
		int current = transceiver.GetFrequency();
		int resolution = transceiver.GetFrequencyResolution();
		int minRaw = transceiver.GetMinFrequency();
		int maxRaw = transceiver.GetMaxFrequency();
		string range = "current=" + current.ToString();
		range = range + ",min=" + minRaw.ToString();
		range = range + ",max=" + maxRaw.ToString();
		range = range + ",res=" + resolution.ToString();
		if (resolution <= 0)
		{
			Record("BENCH_SKIP", device, 0, -1, "reason=RES_NONPOSITIVE," + range);
			return;
		}
		if (current < minRaw || current > maxRaw)
		{
			Record("BENCH_SKIP", device, 0, -1, "reason=CURRENT_OUT_OF_RANGE," + range);
			return;
		}
		int target;
		if (maxRaw - current >= resolution)
			target = current + resolution;
		else if (current - minRaw >= resolution)
			target = current - resolution;
		else
		{
			Record("BENCH_SKIP", device, 0, -1, "reason=NO_ROOM_FOR_STEP," + range);
			return;
		}
		// SetterReady до спавна исключает INVALID: здесь только один из трёх методов (V2, codex #4262; A1-E)
		string setter = Setter();
		if (setter == SETTER_ENTRY)
		{
			// Путь меню: AdjustEntryModif сам берёт свежий getter, шаг ±разрешение, ограничение и SetFrequency
			// (SCR_VONEntryRadio.c:107–133); стенд выбирает только знак шага тем же правилом «есть место».
			SCR_VONEntryRadio entry = EntryOf(device);
			if (!entry)
			{
				Record("BENCH_SKIP", device, 0, -1, "reason=NO_ENTRY," + range);
				return;
			}
			int modifier = 1;
			if (target < current)
				modifier = -1;
			m_aTargets[device] = target;
			entry.AdjustEntryModif(modifier);
			range = range + ",modifier=" + modifier.ToString() + "," + EntryText(entry);
		}
		else
		{
			m_aTargets[device] = target;
			if (setter == SETTER_TRANSCEIVER)
				transceiver.SetFrequency(target);
			else
				radio.SetTransceiverFrequency(transceiver, target);
		}
		Record("BENCH_SET_FREQUENCY", device, 0, target, range + ",setter=" + setter);
	}

	//! Q-M: только своя рация; переключение на противоположное состояние, чтение, затем возврат к исходному
	//! (codex #4106: при before = true вызов SetMuteState(true) был бы no-op).
	protected void Mute(int device)
	{
		BaseRadioComponent radio = RadioOf(device);
		if (!radio || radio.TransceiversCount() < 1)
			return;
		BaseTransceiver transceiver = radio.GetTransceiver(0);
		if (!transceiver)
			return;
		SCR_VONEntryRadio entry = EntryOf(device);
		if (entry)
		{
			MuteEntry(device, transceiver, entry);
			return;
		}
		bool before = transceiver.IsMuted();
		bool requested = !before;
		bool returnedSet = transceiver.SetMuteState(requested);
		bool afterSet = transceiver.IsMuted();
		bool returnedRestore = transceiver.SetMuteState(before);
		bool afterRestore = transceiver.IsMuted();
		string text = "before=" + Bit(before) + ",requested=" + Bit(requested) + ",ret=" + Bit(returnedSet);
		text = text + ",after=" + Bit(afterSet) + ",restoreTo=" + Bit(before) + ",ret2=" + Bit(returnedRestore);
		text = text + ",after2=" + Bit(afterRestore);
		Record("BENCH_MUTE", device, 0, -1, text);
	}

	//! A1-E: mute путём меню — ToggleMuteEntry дважды (SCR_VONEntryRadio.c:240–244), затем прямой SetMuteState туда и
	//! обратно с чтением entry.GetIsMuted (читает IsMuted трансивера, :255–259), чтобы увидеть, одно ли это состояние.
	//! Поля after/after2 — после первого и второго ToggleMuteEntry (те же имена, что у прямого пути); d* — прямой путь;
	//! e* — entry.GetIsMuted после соответствующего шага. Всё возвращается к исходному состоянию трансивера.
	protected void MuteEntry(int device, BaseTransceiver transceiver, SCR_VONEntryRadio entry)
	{
		bool before = transceiver.IsMuted();
		bool entryBefore = entry.GetIsMuted();
		bool requested = !before;
		bool returnedToggle = entry.ToggleMuteEntry();
		bool afterToggle = transceiver.IsMuted();
		bool entryAfterToggle = entry.GetIsMuted();
		bool returnedToggle2 = entry.ToggleMuteEntry();
		bool afterToggle2 = transceiver.IsMuted();
		bool entryAfterToggle2 = entry.GetIsMuted();
		bool returnedDirect = transceiver.SetMuteState(requested);
		bool afterDirect = transceiver.IsMuted();
		bool entryAfterDirect = entry.GetIsMuted();
		bool returnedDirect2 = transceiver.SetMuteState(before);
		bool afterDirect2 = transceiver.IsMuted();
		bool entryAfterDirect2 = entry.GetIsMuted();
		string text = "path=entry,before=" + Bit(before) + ",ebefore=" + Bit(entryBefore) + ",requested=" + Bit(requested);
		text = text + ",ret=" + Bit(returnedToggle) + ",after=" + Bit(afterToggle) + ",eafter=" + Bit(entryAfterToggle);
		text = text + ",restoreTo=" + Bit(before) + ",ret2=" + Bit(returnedToggle2) + ",after2=" + Bit(afterToggle2);
		text = text + ",eafter2=" + Bit(entryAfterToggle2) + ",dret=" + Bit(returnedDirect) + ",dafter=" + Bit(afterDirect);
		text = text + ",edafter=" + Bit(entryAfterDirect) + ",dret2=" + Bit(returnedDirect2) + ",dafter2=" + Bit(afterDirect2);
		text = text + ",edafter2=" + Bit(entryAfterDirect2);
		Record("BENCH_MUTE", device, 0, -1, text);
	}

	//! A1-P0: неверный режим — NOT_READY до любых действий; иначе один PLAYER_SAMPLE на каждую секунду игрового времени
	//! и после PLAYER_SAMPLES записей BENCH_DONE. Рации, entry и контроль не трогаются.
	protected void StepPlayer()
	{
		if (Mode() == MODE_INVALID)
		{
			Note("BENCH_NOT_READY", -1, "reason=MODE_PARAM,mode=" + MODE_INVALID + ",modeGiven=" + Bit(ModeGiven()));
			Finish(false);
			return;
		}
		// codex #4372: каждый срез не раньше чем через 1 с игрового времени после предыдущего, а не «догоняющая»
		// серия кадров после задержки — 30 срезов покрывают не меньше 30 с.
		if (m_fElapsed < m_fLastSample + 1)
			return;
		m_fLastSample = m_fElapsed;
		m_iSamples++;
		float ms = m_fElapsed * 1000.0;
		int elapsedMs = ms;
		Note("PLAYER_SAMPLE", m_iSamples, "elapsedMs=" + elapsedMs.ToString() + "," + PlayerState());
		RecordEntries(m_iSamples);
		if (Mode() == MODE_PLAYERMENU)
		{
			PlayerMenuStep(m_iSamples);
			// codex #4402: STOP никогда не становится успешным DONE — прогон сразу прерывается
			if (m_bActStopped)
			{
				Finish(false);
				return;
			}
		}
		if (m_iSamples >= PLAYER_SAMPLES)
			Finish(true);
	}

	//! A1-P2 (claude #4399, условия codex #4388, #4390, #4402): одна последовательность пути меню на одной radio-entry
	//! локального игрока в собственном временном Play. Цель выбирается на срезе 5 только при полной привязке:
	//! SCR_VONEntryRadio, НЕ активная для передачи (активную entry, передачу и DIRECT не меняем), usable, gadget, его
	//! владелец = рация трансивера, рация не IsEditorRadio, индекс трансивера = номер − 1, рация в инвентаре персонажа;
	//! шаг только при res > 0, raw в [min, max] и полном свободном шаге. Перед КАЖДОЙ записью — полная повторная
	//! проверка личности (тот же контроллер и playerId, та же управляемая сущность, entry есть в текущем GetVONEntries,
	//! тот же трансивер, рация, gadget, слот и номер, entry всё ещё не активна) и ожидаемого raw/mute; иначе
	//! PLAYER_ACT op=STOP и сразу BENCH_ABORTED — ни одной дальнейшей записи и не успешный DONE. Срез 6 — шаг
	//! AdjustEntryModif(±1); 6/7/11 — чтения t0/t1/t5 с фактическим временем от вызова шага; 12 — возврат ∓1 и проверка
	//! raw = исходное; 14/15 — ToggleMuteEntry туда (проверка ¬muted0) и обратно (перед ним проверка, что держится
	//! установленное нами ¬muted0); 20 — итоговая проверка raw0 и muted0. Снимки — защита изолированного опыта, а не
	//! вывод о владении bool-API в production.
	protected ref SCR_VONEntryRadio m_ActEntry;   //!< ref: цель удерживается до конца прогона
	protected BaseTransceiver m_ActTransceiver;
	protected PlayerController m_ActController;
	protected IEntity m_ActControlled;
	protected IEntity m_ActRadioOwner;
	protected SCR_GadgetComponent m_ActGadget;
	protected int m_iActPlayerId;
	protected int m_iActSlot;
	protected int m_iActTrIndex;
	protected int m_iActTrNumber;
	protected int m_iActIdx = -1;
	protected int m_iActRaw0;
	protected int m_iActTarget;
	protected int m_iActModifier;
	protected bool m_bActMuted0;
	protected bool m_bActChosen;
	protected bool m_bActStopped;
	protected float m_fActStepAt = -1;

	//! Есть ли цель в текущем списке entry VON-контроллера (тот же объект) и не стала ли она entry передачи (codex #4410).
	protected bool ActListed()
	{
		if (!m_ActController)
			return false;
		SCR_VONController von = SCR_VONController.Cast(m_ActController.FindComponent(SCR_VONController));
		if (!von || von.GetActiveEntry() == m_ActEntry)
			return false;
		array<ref SCR_VONEntry> entries = {};
		von.GetVONEntries(entries);
		bool found = false;
		foreach (SCR_VONEntry entry : entries)
		{
			if (entry == m_ActEntry)
				found = true;
		}
		entries.Clear();
		return found;
	}

	//! Полная повторная проверка личности цели (codex #4402).
	protected bool ActBound()
	{
		if (!m_ActEntry || !m_ActTransceiver || !m_ActController || !m_ActControlled || !m_ActRadioOwner || !m_ActGadget)
			return false;
		PlayerController controller = GetGame().GetPlayerController();
		if (controller != m_ActController || controller.GetPlayerId() != m_iActPlayerId)
			return false;
		if (controller.GetControlledEntity() != m_ActControlled || !ActListed())
			return false;
		if (m_ActEntry.GetTransceiver() != m_ActTransceiver || m_ActEntry.GetGadget() != m_ActGadget || m_ActEntry.IsActive())
			return false;
		if (!m_ActEntry.IsUsable())
			return false;
		BaseRadioComponent radio = m_ActTransceiver.GetRadio();
		if (!radio || radio.IsEditorRadio() || radio.GetOwner() != m_ActRadioOwner || m_ActGadget.GetOwner() != m_ActRadioOwner)
			return false;
		if (TransceiverIndex(radio, m_ActTransceiver) != m_iActTrIndex || m_ActEntry.GetTransceiverNumber() != m_iActTrNumber)
			return false;
		return InventorySlot(m_ActControlled, m_ActRadioOwner) == m_iActSlot;
	}

	//! Текущее состояние цели одной строкой (raw трансивера, локальная частота и mute entry, mute трансивера, текст).
	protected string ActState()
	{
		if (!ActBound())
			return "bound=0";
		string text = "bound=1,raw=" + m_ActTransceiver.GetFrequency().ToString();
		text = text + ",entryFreq=" + m_ActEntry.GetEntryFrequency().ToString() + ",muted=" + Bit(m_ActTransceiver.IsMuted());
		text = text + ",emuted=" + Bit(m_ActEntry.GetIsMuted());
		return text + ",dtext=" + RICON_LiveText.Esc(m_ActEntry.GetDisplayText());
	}

	protected int ActSinceStepMs()
	{
		if (m_fActStepAt < 0)
			return -1;
		float ms = (m_fElapsed - m_fActStepAt) * 1000.0;
		int value = ms;
		return value;
	}

	protected void ActNote(int sample, string op, string text)
	{
		Note("PLAYER_ACT", sample, "op=" + op + ",idx=" + m_iActIdx.ToString() + "," + text);
	}

	protected void ActStop(int sample, string reason)
	{
		m_bActStopped = true;
		ActNote(sample, "STOP", "reason=" + reason + "," + ActState());
	}

	//! Проверка перед записью: полная привязка цела, raw и mute трансивера равны ожидаемым; иначе STOP.
	protected bool ActExpect(int sample, int raw, bool muted, string reason)
	{
		if (!ActBound() || m_ActTransceiver.GetFrequency() != raw || m_ActTransceiver.IsMuted() != muted)
		{
			ActStop(sample, reason);
			return false;
		}
		return true;
	}

	protected void ActChoose(int sample)
	{
		m_bActChosen = true;
		PlayerController controller = GetGame().GetPlayerController();
		SCR_VONController von;
		if (controller)
			von = SCR_VONController.Cast(controller.FindComponent(SCR_VONController));
		if (!von || !controller.GetControlledEntity())
		{
			ActStop(sample, "NO_VON_CONTROLLER");
			return;
		}
		IEntity controlled = controller.GetControlledEntity();
		array<ref SCR_VONEntry> entries = {};
		von.GetVONEntries(entries);
		for (int i = 0; i < entries.Count() && !m_ActEntry; i++)
		{
			SCR_VONEntryRadio entry = SCR_VONEntryRadio.Cast(entries[i]);
			if (!entry || entry.IsActive() || entry == von.GetActiveEntry() || !entry.IsUsable() || !entry.GetGadget())
				continue;
			BaseTransceiver transceiver = entry.GetTransceiver();
			if (!transceiver || !transceiver.GetRadio())
				continue;
			BaseRadioComponent radio = transceiver.GetRadio();
			if (radio.IsEditorRadio() || entry.GetGadget().GetOwner() != radio.GetOwner())
				continue;
			int trIndex = TransceiverIndex(radio, transceiver);
			int slot = InventorySlot(controlled, radio.GetOwner());
			if (trIndex < 0 || trIndex != entry.GetTransceiverNumber() - 1 || slot < 0)
				continue;
			m_ActEntry = entry;
			m_ActTransceiver = transceiver;
			m_ActController = controller;
			m_ActControlled = controlled;
			m_ActRadioOwner = radio.GetOwner();
			m_ActGadget = entry.GetGadget();
			m_iActPlayerId = controller.GetPlayerId();
			m_iActSlot = slot;
			m_iActTrIndex = trIndex;
			m_iActTrNumber = entry.GetTransceiverNumber();
			m_iActIdx = i;
		}
		entries.Clear();
		if (!m_ActEntry)
		{
			ActStop(sample, "NO_CANDIDATE");
			return;
		}
		m_iActRaw0 = m_ActTransceiver.GetFrequency();
		m_bActMuted0 = m_ActTransceiver.IsMuted();
		int res = m_ActTransceiver.GetFrequencyResolution();
		int minRaw = m_ActTransceiver.GetMinFrequency();
		int maxRaw = m_ActTransceiver.GetMaxFrequency();
		string range = "trNumber=" + m_iActTrNumber.ToString() + ",min=" + minRaw.ToString();
		range = range + ",max=" + maxRaw.ToString() + ",res=" + res.ToString();
		if (res > 0 && m_iActRaw0 >= minRaw && m_iActRaw0 <= maxRaw && maxRaw - m_iActRaw0 >= res)
			m_iActModifier = 1;
		else if (res > 0 && m_iActRaw0 >= minRaw && m_iActRaw0 <= maxRaw && m_iActRaw0 - minRaw >= res)
			m_iActModifier = -1;
		else
		{
			ActStop(sample, "NO_ROOM_FOR_STEP");
			return;
		}
		m_iActTarget = m_iActRaw0 + m_iActModifier * res;
		range = range + ",raw0=" + m_iActRaw0.ToString() + ",muted0=" + Bit(m_bActMuted0) + ",target=" + m_iActTarget.ToString();
		ActNote(sample, "CHOOSE", range + ",modifier=" + m_iActModifier.ToString() + ",slot=" + m_iActSlot.ToString() + "," + ActState());
	}

	protected void PlayerMenuStep(int sample)
	{
		if (m_bActStopped)
			return;
		if (sample == 5)
			ActChoose(sample);
		else if (!m_bActChosen)
			return;
		else if (sample == 6)
		{
			if (!ActExpect(sample, m_iActRaw0, m_bActMuted0, "BEFORE_STEP"))
				return;
			m_fActStepAt = m_fElapsed;
			m_ActEntry.AdjustEntryModif(m_iActModifier);
			ActNote(sample, "STEP", "phase=t0,sinceStepMs=" + ActSinceStepMs().ToString() + ",target=" + m_iActTarget.ToString() + "," + ActState());
		}
		else if (sample == 7 || sample == 11)
		{
			string phase = "t1";
			if (sample == 11)
				phase = "t5";
			ActNote(sample, "READ", "phase=" + phase + ",sinceStepMs=" + ActSinceStepMs().ToString() + ",target=" + m_iActTarget.ToString() + "," + ActState());
		}
		else if (sample == 12)
		{
			if (!ActExpect(sample, m_iActTarget, m_bActMuted0, "BEFORE_RESTORE"))
				return;
			m_ActEntry.AdjustEntryModif(-m_iActModifier);
			ActNote(sample, "RESTORE", "raw0=" + m_iActRaw0.ToString() + "," + ActState());
			ActExpect(sample, m_iActRaw0, m_bActMuted0, "RESTORE_MISMATCH");
		}
		else if (sample == 14)
		{
			if (!ActExpect(sample, m_iActRaw0, m_bActMuted0, "BEFORE_MUTE"))
				return;
			bool returned = m_ActEntry.ToggleMuteEntry();
			ActNote(sample, "MUTE", "ret=" + Bit(returned) + ",muted0=" + Bit(m_bActMuted0) + "," + ActState());
			ActExpect(sample, m_iActRaw0, !m_bActMuted0, "MUTE_NOT_TOGGLED");
		}
		else if (sample == 15)
		{
			if (!ActExpect(sample, m_iActRaw0, !m_bActMuted0, "BEFORE_UNMUTE"))
				return;
			bool returned2 = m_ActEntry.ToggleMuteEntry();
			ActNote(sample, "UNMUTE", "ret=" + Bit(returned2) + ",muted0=" + Bit(m_bActMuted0) + "," + ActState());
			ActExpect(sample, m_iActRaw0, m_bActMuted0, "UNMUTE_MISMATCH");
		}
		else if (sample == 20)
		{
			if (ActExpect(sample, m_iActRaw0, m_bActMuted0, "FINAL_MISMATCH"))
				ActNote(sample, "FINAL", "raw0=" + m_iActRaw0.ToString() + ",muted0=" + Bit(m_bActMuted0) + "," + ActState());
		}
	}

	//! A1-P1 (claude #4385, codex #4384): сразу после среза — по одной записи PLAYER_ENTRY на каждую entry
	//! SCR_VONController локального контроллера, value = номер среза. Только getter-ы: класс, метод VON, активна ли
	//! (и активна ли для передачи), usable, текст (%XX); у SCR_VONEntryRadio ещё номер трансивера (1-базный),
	//! локальная частота и mute entry, дальнобойность, а у её трансивера — raw, min/max/res, IsMuted и prefab рации.
	protected void RecordEntries(int sample)
	{
		PlayerController controller = GetGame().GetPlayerController();
		if (!controller)
			return;
		SCR_VONController von = SCR_VONController.Cast(controller.FindComponent(SCR_VONController));
		if (!von)
			return;
		array<ref SCR_VONEntry> entries = {};
		von.GetVONEntries(entries);
		SCR_VONEntry active = von.GetActiveEntry();
		for (int i = 0; i < entries.Count(); i++)
		{
			SCR_VONEntry entry = entries[i];
			if (!entry)
			{
				Note("PLAYER_ENTRY", sample, "idx=" + i.ToString() + ",entry=0");
				continue;
			}
			string text = "idx=" + i.ToString() + ",entry=1,cls=" + entry.ClassName();
			int method = entry.GetVONMethod();
			text = text + ",method=" + method.ToString() + ",active=" + Bit(entry.IsActive());
			text = text + ",transmit=" + Bit(entry == active) + ",usable=" + Bit(entry.IsUsable());
			text = text + ",dtext=" + RICON_LiveText.Esc(entry.GetDisplayText());
			SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(entry);
			text = text + ",radioEntry=" + Bit(radioEntry != null);
			if (radioEntry)
				text = text + "," + RadioEntryText(radioEntry, controller.GetControlledEntity());
			Note("PLAYER_ENTRY", sample, text);
		}
		entries.Clear();
	}

	//! codex #4388: позиция рации трансивера entry среди раций управляемого персонажа (GadgetManager, RADIO затем
	//! RADIO_BACKPACK — тот же порядок, что у RICON_LiveRadioReader), −1 — не найдена или нет менеджера.
	protected static int InventorySlot(IEntity controlled, IEntity radioOwner)
	{
		if (!controlled || !radioOwner)
			return -1;
		SCR_GadgetManagerComponent manager = SCR_GadgetManagerComponent.GetGadgetManager(controlled);
		if (!manager)
			return -1;
		int slot = 0;
		array<EGadgetType> types = {EGadgetType.RADIO, EGadgetType.RADIO_BACKPACK};
		foreach (EGadgetType type : types)
		{
			array<SCR_GadgetComponent> gadgets = manager.GetGadgetsByType(type);
			if (!gadgets)
				continue;
			foreach (SCR_GadgetComponent gadget : gadgets)
			{
				if (gadget && gadget.GetOwner() == radioOwner)
					return slot;
				slot++;
			}
		}
		return -1;
	}

	//! Индекс трансивера у его рации (−1 — не найден): сверяется с 1-базным номером entry.
	protected static int TransceiverIndex(BaseRadioComponent radio, BaseTransceiver transceiver)
	{
		if (!radio || !transceiver)
			return -1;
		for (int t = 0; t < radio.TransceiversCount(); t++)
		{
			if (radio.GetTransceiver(t) == transceiver)
				return t;
		}
		return -1;
	}

	protected static string RadioEntryText(SCR_VONEntryRadio entry, IEntity controlled)
	{
		string text = "trNumber=" + entry.GetTransceiverNumber().ToString();
		text = text + ",entryFreq=" + entry.GetEntryFrequency().ToString() + ",emuted=" + Bit(entry.GetIsMuted());
		text = text + ",gadget=" + Bit(entry.GetGadget() != null);
		if (entry.GetGadget())
			text = text + ",longRange=" + Bit(entry.IsLongRange());
		BaseTransceiver transceiver = entry.GetTransceiver();
		text = text + ",transceiver=" + Bit(transceiver != null);
		if (!transceiver)
			return text;
		text = text + ",raw=" + transceiver.GetFrequency().ToString();
		text = text + ",min=" + transceiver.GetMinFrequency().ToString() + ",max=" + transceiver.GetMaxFrequency().ToString();
		text = text + ",res=" + transceiver.GetFrequencyResolution().ToString() + ",muted=" + Bit(transceiver.IsMuted());
		string prefab = "UNKNOWN";
		BaseRadioComponent radio = transceiver.GetRadio();
		IEntity radioOwner;
		if (radio)
			radioOwner = radio.GetOwner();
		if (radioOwner && radioOwner.GetPrefabData())
			prefab = radioOwner.GetPrefabData().GetPrefabName();
		text = text + ",radioPrefab=" + RICON_LiveText.Esc(prefab);
		// codex #4388: точная связь entry ↔ трансивер ↔ рация в инвентаре управляемого персонажа
		text = text + ",editorRadio=" + Bit(radio && radio.IsEditorRadio());
		text = text + ",trIndex=" + TransceiverIndex(radio, transceiver).ToString();
		text = text + ",inventorySlot=" + InventorySlot(controlled, radioOwner).ToString();
		if (entry.GetGadget())
			text = text + ",gadgetOwnerMatch=" + Bit(entry.GetGadget().GetOwner() == radioOwner);
		return text;
	}

	//! Удаление только своих раций штатным SCR_EntityHelper.DeleteEntityAndChildren (обёртка
	//! RplComponent.DeleteRplEntity, codex #4108). BENCH_DONE — лишь после всех фаз; иначе BENCH_ABORTED с фазой.
	protected void Finish(bool complete)
	{
		// codex #4180: очередь снимается до удаления своих раций.
		StopHeartbeat("FINISH");
		// A1-E: свои entry держат трансивер — освобождаются до удаления раций.
		m_aEntries.Clear();
		// A1-P2 (codex #4410): цель и сохранённые engine-ссылки после последней записи больше не используются
		m_ActEntry = null;
		m_ActTransceiver = null;
		m_ActController = null;
		m_ActControlled = null;
		m_ActRadioOwner = null;
		m_ActGadget = null;
		for (int i = 0; i < m_aRadios.Count(); i++)
		{
			IEntity radio = m_aRadios[i];
			if (radio)
				SCR_EntityHelper.DeleteEntityAndChildren(radio);
		}
		m_aRadios.Clear();
		if (Mode() == MODE_PLAYER)
		{
			if (complete)
				Note("BENCH_DONE", m_iSamples, "level=PLAYER_OBSERVATION,not=SPAWN|SET|CONTROL|RADIO|AUDIO");
			else
				Note("BENCH_ABORTED", m_iSamples, "level=PLAYER_OBSERVATION,samples=" + m_iSamples.ToString());
		}
		else if (Mode() == MODE_PLAYERMENU)
		{
			if (complete)
				Note("BENCH_DONE", m_iSamples, "level=PLAYER_MENU_RUNTIME,not=SCREEN|SPAWN|CONTROL|AUDIO|REPLICATION");
			else
				Note("BENCH_ABORTED", m_iSamples, "level=PLAYER_MENU_RUNTIME,stopped=" + Bit(m_bActStopped) + ",samples=" + m_iSamples.ToString());
		}
		else if (complete && Setter() == SETTER_ENTRY)
			Note("BENCH_DONE", m_iPhase, "level=MENU_ENTRY_RUNTIME,not=SCREEN|PLAYER_VON|KNOWN_CHANNEL|RX_TX|AUDIBLE|REPLICATION|PER_PAIR");
		else if (complete)
			Note("BENCH_DONE", m_iPhase, "level=NATIVE_ROUNDTRIP,not=UI_MHz|RX_TX|AUDIBLE|REPLICATION|PER_PAIR");
		else
			Note("BENCH_ABORTED", m_iPhase, "level=NATIVE_ROUNDTRIP,phase=" + m_iPhase.ToString());
		m_Journal.Dump();
		m_bRunning = false;
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		m_iFrames++;
		if (!m_bFirstFrameLogged)
		{
			m_bFirstFrameLogged = true;
			PrintFormat("[RICON_LIVEB] lifecycle=FirstFrame %1 running=%2", WorldState(owner), m_bRunning);
			TryStart(owner, "FirstFrame");
		}
		if (!m_bRunning || timeSlice <= 0)
			return;
		m_fElapsed += timeSlice;
		if (Mode() != MODE_RADIO)
		{
			StepPlayer();
			return;
		}
		if (m_iPhase == PHASE_IDLE && m_fElapsed >= 1)
		{
			if (!ManagerReady(owner) || !SetterReady())
			{
				Finish(false);
				return;
			}
			Spawn();
			m_fSpawnedAt = m_fElapsed;
			m_iPhase = PHASE_SPAWNED;
			for (int a = 0; a < m_aRadios.Count(); a++)
				Read(a, "spawn");
			if (Setter() == SETTER_ENTRY)
			{
				for (int g = 0; g < m_aRadios.Count(); g++)
					CreateEntry(g);
			}
		}
		else if (m_iPhase == PHASE_SPAWNED && m_fElapsed - m_fSpawnedAt >= 1)
		{
			m_fSetAt = m_fElapsed;
			m_iPhase = PHASE_STEPPED;
			for (int b = 0; b < m_aRadios.Count(); b++)
			{
				StepFrequency(b);
				Read(b, "t0");
			}
		}
		else if (m_iPhase == PHASE_STEPPED && m_fElapsed - m_fSetAt >= 1)
		{
			m_iPhase = PHASE_READ_T1;
			for (int c = 0; c < m_aRadios.Count(); c++)
				Read(c, "t1");
		}
		else if (m_iPhase == PHASE_READ_T1 && m_fElapsed - m_fSetAt >= 5)
		{
			m_iPhase = PHASE_READ_T5;
			for (int d = 0; d < m_aRadios.Count(); d++)
				Read(d, "t5");
		}
		else if (m_iPhase == PHASE_READ_T5 && m_fElapsed - m_fSetAt >= 7)
		{
			m_iPhase = PHASE_MUTED;
			for (int e = 0; e < m_aRadios.Count(); e++)
				Mute(e);
		}
		else if (m_iPhase == PHASE_MUTED && m_fElapsed - m_fSetAt >= 9)
		{
			m_iPhase = PHASE_DONE;
			Finish(true);
		}
	}

	override void OnDelete(IEntity owner)
	{
		StopHeartbeat("DELETE");
		if (m_bRunning)
			Finish(false);
		super.OnDelete(owner);
	}
}
