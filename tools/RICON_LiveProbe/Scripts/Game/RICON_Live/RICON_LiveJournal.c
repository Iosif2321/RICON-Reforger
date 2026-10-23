//! TOOL_ONLY: журнал прибора live-проверки (LIVE-order-frequency-mute.md §3, AUDIO_CHAIN_CONTROL_RU.md §2.1).
//! Чистая часть: кольцевой буфер событий с последовательным номером, явной записью переполнения (GAP) и строгим
//! форматом строк [RICON_LIVE]. Хуков движка здесь нет; ключи шифрования сюда не попадают по построению (поля
//! только числовые и короткий текст без секретов — ответственность вызывающего).
class RICON_LiveEvent
{
	int m_iSeq;
	int m_iTimeMs;      //!< время источника (сторона указана в m_sSide), не общая аудиошкала
	string m_sSide;     //!< "server" | "client" | "probe"
	string m_sKind;     //!< TX_CAPTURE (OnCapture — колбэк каждый кадр записи, не фронт), TX_SEEN, RX_EVENT,
	                    //!< RADIO_STATE, MUTE_CALL, GAP, NOTE; TX_START/TX_STOP — только после отдельной детекции фронта
	int m_iPlayerId;
	int m_iDeviceId;    //!< RplId рации или -1
	int m_iTransceiver; //!< индекс трансивера или -1
	int m_iValue;       //!< сырое целое (например GetFrequency) или -1
	string m_sText;     //!< короткий ASCII-комментарий без пробелов и ключей
}

class RICON_LiveJournal
{
	static const int DEFAULT_CAPACITY = 256;
	protected int m_iCapacity;
	protected int m_iNextSeq;
	protected int m_iDropped;      //!< события, вытесненные без записи (учтены в последней GAP-записи)
	protected ref array<ref RICON_LiveEvent> m_aEvents = {};

	void RICON_LiveJournal(int capacity = 256)
	{
		m_iCapacity = capacity;
		if (m_iCapacity < 2)
			m_iCapacity = 2;
	}

	int Count()
	{
		return m_aEvents.Count();
	}

	int Dropped()
	{
		return m_iDropped;
	}

	int NextSeq()
	{
		return m_iNextSeq;
	}

	RICON_LiveEvent At(int index)
	{
		return m_aEvents[index];
	}

	//! Переполнение: самые старые события вытесняются, а первая запись буфера становится GAP с числом потерь
	//! (m_iValue) и временем последнего потерянного. GAP занимает слот, поэтому при его создании вытесняются два
	//! события; далее — по одному, счётчик растёт. GAP никогда не вытесняется молча.
	//! Только RemoveOrdered: array.Remove переставляет последний элемент на место удалённого и ломает порядок.
	protected void MakeRoom()
	{
		if (m_aEvents.Count() < m_iCapacity)
			return;
		RICON_LiveEvent first = m_aEvents[0];
		if (first.m_sKind == "GAP")
		{
			RICON_LiveEvent victim = m_aEvents[1];
			m_aEvents.RemoveOrdered(1);
			first.m_iValue = first.m_iValue + 1;
			first.m_iTimeMs = victim.m_iTimeMs;
			m_iDropped++;
			return;
		}
		RICON_LiveEvent second = m_aEvents[1];
		RICON_LiveEvent gap = new RICON_LiveEvent();
		gap.m_iSeq = first.m_iSeq;
		gap.m_iTimeMs = second.m_iTimeMs;
		gap.m_sSide = "probe";
		gap.m_sKind = "GAP";
		gap.m_iPlayerId = -1;
		gap.m_iDeviceId = -1;
		gap.m_iTransceiver = -1;
		gap.m_iValue = 2;
		gap.m_sText = "overflow";
		m_aEvents.RemoveOrdered(1);
		m_aEvents.RemoveOrdered(0);
		m_aEvents.InsertAt(gap, 0);
		m_iDropped = m_iDropped + 2;
	}

	int Add(int timeMs, string side, string kind, int playerId, int deviceId, int transceiver, int value, string text)
	{
		MakeRoom();
		RICON_LiveEvent e = new RICON_LiveEvent();
		e.m_iSeq = m_iNextSeq;
		m_iNextSeq++;
		e.m_iTimeMs = timeMs;
		e.m_sSide = side;
		e.m_sKind = kind;
		e.m_iPlayerId = playerId;
		e.m_iDeviceId = deviceId;
		e.m_iTransceiver = transceiver;
		e.m_iValue = value;
		e.m_sText = text;
		m_aEvents.Insert(e);
		return e.m_iSeq;
	}

	//! Одна строка на событие; без слова FAIL (gate регрессии) и без значений ключей.
	//! Конкатенация по шагам: длинная цепочка «+» не проходит парсер Enforce («Formula too complex»).
	static string Format(RICON_LiveEvent e)
	{
		int seq = e.m_iSeq;
		int t = e.m_iTimeMs;
		int p = e.m_iPlayerId;
		int d = e.m_iDeviceId;
		int tr = e.m_iTransceiver;
		int v = e.m_iValue;
		string text = "[RICON_LIVE] seq=" + seq.ToString();
		text = text + " t=" + t.ToString();
		text = text + " side=" + e.m_sSide;
		text = text + " kind=" + e.m_sKind;
		text = text + " player=" + p.ToString();
		text = text + " device=" + d.ToString();
		text = text + " tr=" + tr.ToString();
		text = text + " value=" + v.ToString();
		text = text + " text=" + e.m_sText;
		return text;
	}

	void Dump()
	{
		foreach (RICON_LiveEvent e : m_aEvents)
		{
			Print(Format(e));
		}
	}
}
