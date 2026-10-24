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
		m_aTargets[device] = target;
		radio.SetTransceiverFrequency(transceiver, target);
		Record("BENCH_SET_FREQUENCY", device, 0, target, range);
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

	//! Удаление только своих раций штатным SCR_EntityHelper.DeleteEntityAndChildren (обёртка
	//! RplComponent.DeleteRplEntity, codex #4108). BENCH_DONE — лишь после всех фаз; иначе BENCH_ABORTED с фазой.
	protected void Finish(bool complete)
	{
		// codex #4180: очередь снимается до удаления своих раций.
		StopHeartbeat("FINISH");
		for (int i = 0; i < m_aRadios.Count(); i++)
		{
			IEntity radio = m_aRadios[i];
			if (radio)
				SCR_EntityHelper.DeleteEntityAndChildren(radio);
		}
		m_aRadios.Clear();
		if (complete)
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
		if (m_iPhase == PHASE_IDLE && m_fElapsed >= 1)
		{
			Spawn();
			m_fSpawnedAt = m_fElapsed;
			m_iPhase = PHASE_SPAWNED;
			for (int a = 0; a < m_aRadios.Count(); a++)
				Read(a, "spawn");
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
