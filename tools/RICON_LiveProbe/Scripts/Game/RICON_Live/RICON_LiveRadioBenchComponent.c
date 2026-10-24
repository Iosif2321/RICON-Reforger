//! TOOL_ONLY: стенд Q-F/Q-M на изолированных рациях (вариант (а′), claude #4089, условия codex #4091).
//! Компонент на временной GenericEntity, которую плагин RICON_LiveBenchLauncherPlugin создаёт в памяти World Editor
//! (мир не сохраняется). В Play стенд сам спавнит рации из BenchPrefabs(), и только с ними работает: игроков, меню VON
//! и пользовательских раций нет — это «изолированный экземпляр под исключительным управлением» LIVE-order §2 Q-M.
//! Уровень всех записей — NATIVE_ROUNDTRIP: значения getter-ов и ответ Set* на этой рации в этом процессе.
//! UI_MHz, направление RX/TX, слышимость, репликация и per-pair mute этим стендом НЕ проверяются.
//! Шаги по игровому времени: 1 с — спавн и чтение; 2 с — частота на один шаг; чтение t+0/+1/+5 с;
//! 9 с — mute on/off с чтением; 11 с — удаление своих раций и BENCH_DONE. Ключи шифрования не читаются.
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

	protected ref array<string> m_aPrefabs;

	protected ref RICON_LiveJournal m_Journal;
	protected ref array<IEntity> m_aRadios = {};       //!< слабые ссылки на свои рации
	protected ref array<int> m_aTargets = {};          //!< целевая сырая частота трансивера 0 после шага
	protected float m_fElapsed;
	protected int m_iStep;
	protected bool m_bRunning;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	}

	override void EOnInit(IEntity owner)
	{
		if (!owner.GetWorld() || owner.GetWorld().IsEditMode())
			return;
		m_Journal = new RICON_LiveJournal(256);
		m_Journal.SetStream(true);
		m_aPrefabs = BenchPrefabs();
		m_bRunning = true;
		Note("BENCH_START", -1, "level=NATIVE_ROUNDTRIP,prefabs=" + m_aPrefabs.Count().ToString());
	}

	protected int Note(string kind, int value, string text)
	{
		return m_Journal.Add(RICON_LiveObserver.Now(), "probe", kind, -1, -1, -1, value, text);
	}

	protected int Record(string kind, int device, int transceiver, int value, string text)
	{
		return m_Journal.Add(RICON_LiveObserver.Now(), "probe", kind, -1, -1, transceiver, value, "bench=" + device.ToString() + "," + text);
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
			if (radio)
				ok = 1;
			Record("BENCH_SPAWN", i, -1, ok, "prefab=" + m_aPrefabs[i]);
		}
	}

	//! Все трансиверы своей рации: сырые getter-ы, без единиц.
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
			int minRaw = transceiver.GetMinFrequency();
			int maxRaw = transceiver.GetMaxFrequency();
			int resolution = transceiver.GetFrequencyResolution();
			string text = "phase=" + phase + ",min=" + minRaw.ToString();
			text = text + ",max=" + maxRaw.ToString();
			text = text + ",res=" + resolution.ToString();
			text = text + ",powered=" + powered.ToString();
			text = text + ",muted=" + muted.ToString();
			text = text + ",target=" + m_aTargets[device].ToString();
			Record("BENCH_READ", device, t, transceiver.GetFrequency(), text);
		}
	}

	//! Q-F: трансивер 0 своей рации на один шаг разрешения (вверх, а у верхней границы — вниз).
	protected void StepFrequency(int device)
	{
		BaseRadioComponent radio = RadioOf(device);
		if (!radio || radio.TransceiversCount() < 1)
			return;
		BaseTransceiver transceiver = radio.GetTransceiver(0);
		if (!transceiver)
			return;
		int current = transceiver.GetFrequency();
		int resolution = transceiver.GetFrequencyResolution();
		int target = current + resolution;
		if (target > transceiver.GetMaxFrequency())
			target = current - resolution;
		m_aTargets[device] = target;
		radio.SetTransceiverFrequency(transceiver, target);
		Record("BENCH_SET_FREQUENCY", device, 0, target, "from=" + current.ToString() + ",res=" + resolution.ToString());
	}

	//! Q-M: только своя рация; вызов, ответ и IsMuted до/после; затем возврат к исходному состоянию по своему журналу.
	protected void Mute(int device)
	{
		BaseRadioComponent radio = RadioOf(device);
		if (!radio || radio.TransceiversCount() < 1)
			return;
		BaseTransceiver transceiver = radio.GetTransceiver(0);
		if (!transceiver)
			return;
		bool before = transceiver.IsMuted();
		bool returnedOn = transceiver.SetMuteState(true);
		bool afterOn = transceiver.IsMuted();
		bool returnedOff = transceiver.SetMuteState(before);
		bool afterOff = transceiver.IsMuted();
		string text = "before=" + Bit(before) + ",call=1,ret=" + Bit(returnedOn) + ",after=" + Bit(afterOn);
		text = text + ",restore=" + Bit(before) + ",ret2=" + Bit(returnedOff) + ",after2=" + Bit(afterOff);
		Record("BENCH_MUTE", device, 0, -1, text);
	}

	protected static string Bit(bool value)
	{
		if (value)
			return "1";
		return "0";
	}

	protected void Cleanup()
	{
		for (int i = 0; i < m_aRadios.Count(); i++)
		{
			IEntity radio = m_aRadios[i];
			if (radio)
				delete radio;
		}
		m_aRadios.Clear();
		Note("BENCH_DONE", -1, "level=NATIVE_ROUNDTRIP,not=UI_MHz|RX_TX|AUDIBLE|REPLICATION|PER_PAIR");
		m_Journal.Dump();
		m_bRunning = false;
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bRunning || timeSlice <= 0)
			return;
		m_fElapsed += timeSlice;
		if (m_iStep == 0 && m_fElapsed >= 1)
		{
			Spawn();
			for (int a = 0; a < m_aRadios.Count(); a++)
				Read(a, "spawn");
			m_iStep = 1;
		}
		else if (m_iStep == 1 && m_fElapsed >= 2)
		{
			for (int b = 0; b < m_aRadios.Count(); b++)
			{
				StepFrequency(b);
				Read(b, "t0");
			}
			m_iStep = 2;
		}
		else if (m_iStep == 2 && m_fElapsed >= 3)
		{
			for (int c = 0; c < m_aRadios.Count(); c++)
				Read(c, "t1");
			m_iStep = 3;
		}
		else if (m_iStep == 3 && m_fElapsed >= 7)
		{
			for (int d = 0; d < m_aRadios.Count(); d++)
				Read(d, "t5");
			m_iStep = 4;
		}
		else if (m_iStep == 4 && m_fElapsed >= 9)
		{
			for (int e = 0; e < m_aRadios.Count(); e++)
				Mute(e);
			m_iStep = 5;
		}
		else if (m_iStep == 5 && m_fElapsed >= 11)
		{
			Cleanup();
		}
	}

	override void OnDelete(IEntity owner)
	{
		if (m_bRunning)
			Cleanup();
		super.OnDelete(owner);
	}
}
