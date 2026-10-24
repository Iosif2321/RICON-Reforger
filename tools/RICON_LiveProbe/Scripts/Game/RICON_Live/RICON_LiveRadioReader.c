//! TOOL_ONLY: показания раций управляемого персонажа для Q-F (LIVE-order-frequency-mute.md §2; claude #4075).
//! Только чтение: штатные getter-ы, ни одного Set*/SetMuteState. Ключи шифрования не читаются.
//! Перечисление — SCR_GadgetManagerComponent.GetGadgetManager(entity).GetGadgetsByType(RADIO, затем RADIO_BACKPACK)
//! (Script API: «Get all owned gadgets by type»).
//! Идентичность (codex #4083): поле device журнала обещает RplId или −1, поэтому здесь всегда −1; привязка
//! binding=LOCAL_ONLY, а для связи строк между собой — номер наблюдения obs, выдаваемый при первом появлении пары
//! (сущность рации, индекс трансивера). Кеш хранит слабую ссылку на саму сущность, не порядковый номер: перестановка
//! списка событий не даёт, замена экземпляра с теми же числами и повторное появление после исчезновения дают новое
//! наблюдение; исчезнувшая пара пишется как RADIO_GONE. slot — позиция в выдаче за этот опрос, только для чтения
//! человеком. Канонический ID рации, lease и сопоставление с сервером этот TOOL_ONLY reader не вводит.
class RICON_LiveRadioSeen
{
	Managed m_Key;        //!< слабая ссылка на сущность рации; обнуляется при удалении сущности
	int m_iTransceiver;
	int m_iObservation;
	string m_sLast;       //!< "частота|состояние" последней записи
	bool m_bSeen;         //!< встречена в текущем опросе
}

class RICON_LiveRadioReader
{
	protected ref RICON_LiveJournal m_Journal;
	protected string m_sSide;
	protected ref array<ref RICON_LiveRadioSeen> m_aSeen = {};
	protected int m_iNextObservation;
	int m_iReads;     //!< вызовы ReadEntity
	int m_iRecorded;  //!< записанные RADIO_STATE
	int m_iGone;      //!< записанные RADIO_GONE

	void RICON_LiveRadioReader(RICON_LiveJournal journal, string side)
	{
		m_Journal = journal;
		m_sSide = side;
	}

	int Tracked()
	{
		return m_aSeen.Count();
	}

	//! Состояние трансивера одной строкой без пробелов; сырые int без единиц (единица — вопрос Q-F).
	static string StateText(string prefab, int minRaw, int maxRaw, int resolution, bool powered, bool muted)
	{
		int p = 0;
		if (powered)
			p = 1;
		int m = 0;
		if (muted)
			m = 1;
		string text = "prefab=" + prefab;
		text = text + ",min=" + minRaw.ToString();
		text = text + ",max=" + maxRaw.ToString();
		text = text + ",res=" + resolution.ToString();
		text = text + ",powered=" + p.ToString();
		text = text + ",muted=" + m.ToString();
		return text;
	}

	//! Начало опроса: все известные пары считаются невстреченными.
	void BeginPoll()
	{
		foreach (RICON_LiveRadioSeen seen : m_aSeen)
		{
			seen.m_bSeen = false;
		}
	}

	protected RICON_LiveRadioSeen Find(Managed key, int transceiver)
	{
		foreach (RICON_LiveRadioSeen seen : m_aSeen)
		{
			if (seen.m_Key && seen.m_Key == key && seen.m_iTransceiver == transceiver)
				return seen;
		}
		return null;
	}

	//! Пишет RADIO_STATE при первом появлении пары или изменении частоты/состояния; иначе −1.
	int Observe(int playerId, Managed key, int slot, int transceiver, int rawFrequency, string state)
	{
		if (!key)
			return -1;
		string current = rawFrequency.ToString() + "|" + state;
		RICON_LiveRadioSeen seen = Find(key, transceiver);
		if (!seen)
		{
			seen = new RICON_LiveRadioSeen();
			seen.m_Key = key;
			seen.m_iTransceiver = transceiver;
			seen.m_iObservation = m_iNextObservation;
			m_iNextObservation++;
			m_aSeen.Insert(seen);
		}
		else if (seen.m_sLast == current)
		{
			seen.m_bSeen = true;
			return -1;
		}
		seen.m_bSeen = true;
		seen.m_sLast = current;
		m_iRecorded++;
		string text = "binding=LOCAL_ONLY,obs=" + seen.m_iObservation.ToString();
		text = text + ",slot=" + slot.ToString();
		text = text + "," + state;
		return m_Journal.Add(RICON_LiveObserver.Now(), m_sSide, "RADIO_STATE", playerId, -1, transceiver, rawFrequency, text);
	}

	//! Конец опроса: невстреченные пары и пары с удалённой сущностью пишутся как RADIO_GONE и забываются.
	int EndPoll(int playerId)
	{
		int gone = 0;
		for (int i = m_aSeen.Count() - 1; i >= 0; i--)
		{
			RICON_LiveRadioSeen seen = m_aSeen[i];
			if (seen.m_bSeen && seen.m_Key)
				continue;
			string text = "binding=LOCAL_ONLY,obs=" + seen.m_iObservation.ToString();
			m_Journal.Add(RICON_LiveObserver.Now(), m_sSide, "RADIO_GONE", playerId, -1, seen.m_iTransceiver, -1, text);
			m_aSeen.RemoveOrdered(i);
			gone++;
		}
		m_iGone = m_iGone + gone;
		return gone;
	}

	//! Один опрос всех раций сущности; возвращает число прочитанных трансиверов. Без сущности все пары — RADIO_GONE.
	int ReadEntity(int playerId, IEntity controlled)
	{
		m_iReads++;
		BeginPoll();
		int read = 0;
		if (controlled)
		{
			SCR_GadgetManagerComponent manager = SCR_GadgetManagerComponent.GetGadgetManager(controlled);
			if (manager)
			{
				int slot = 0;
				read = ReadType(playerId, manager, EGadgetType.RADIO, slot);
				read = read + ReadType(playerId, manager, EGadgetType.RADIO_BACKPACK, slot);
			}
		}
		EndPoll(playerId);
		return read;
	}

	protected int ReadType(int playerId, SCR_GadgetManagerComponent manager, EGadgetType type, inout int slot)
	{
		array<SCR_GadgetComponent> gadgets = manager.GetGadgetsByType(type);
		if (!gadgets)
			return 0;
		int read = 0;
		foreach (SCR_GadgetComponent gadget : gadgets)
		{
			int position = slot;
			slot++;
			if (!gadget)
				continue;
			IEntity owner = gadget.GetOwner();
			if (!owner)
				continue;
			BaseRadioComponent radio = BaseRadioComponent.Cast(owner.FindComponent(BaseRadioComponent));
			if (!radio)
				continue;
			string prefab = "UNKNOWN";
			EntityPrefabData data = owner.GetPrefabData();
			if (data)
				prefab = data.GetPrefabName();
			bool powered = radio.IsPowered();
			int count = radio.TransceiversCount();
			for (int t = 0; t < count; t++)
			{
				BaseTransceiver transceiver = radio.GetTransceiver(t);
				if (!transceiver)
					continue;
				int minRaw = transceiver.GetMinFrequency();
				int maxRaw = transceiver.GetMaxFrequency();
				int resolution = transceiver.GetFrequencyResolution();
				string state = StateText(prefab, minRaw, maxRaw, resolution, powered, transceiver.IsMuted());
				Observe(playerId, owner, position, t, transceiver.GetFrequency(), state);
				read++;
			}
		}
		return read;
	}
}
