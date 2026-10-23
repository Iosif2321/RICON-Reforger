//! TOOL_ONLY: самопроверка журнала прибора без мира, Play и сервера: порядок seq, вытеснение с явным GAP,
//! учёт потерь и формат строки. Хуки движка здесь не вызываются.
[WorkbenchPluginAttribute(name: "RICON: журнал live-прибора", description: "Кольцевой буфер, GAP, формат", wbModules: {"ScriptEditor"})]
class RICON_LiveJournalProbePlugin : WorkbenchPlugin
{
	protected int m_iChecks;
	protected int m_iPassed;

	protected void Check(bool condition, string what)
	{
		m_iChecks++;
		if (condition)
			m_iPassed++;
		else
			Print("[RICON_LIVEJ] mismatch: " + what);
	}

	override void RunCommandline()
	{
		// Сообщения проверок — ASCII: кириллица в console.log Workbench печатается нечитаемо.
		RICON_LiveJournal j = new RICON_LiveJournal(8);
		for (int i = 0; i < 5; i++)
		{
			int seq = j.Add(1000 + i, "client", "RADIO_STATE", 7, 100, 0, 32000 + i, "freq");
			Check(seq == i, "seq in order");
		}
		Check(j.Count() == 5 && j.Dropped() == 0, "no loss before overflow");
		for (int k = 5; k < 20; k++)
		{
			j.Add(1000 + k, "server", "TX_SEEN", 7, -1, -1, -1, "von");
		}
		// 20 добавлений, ёмкость 8, один слот под GAP: сохранены seq 13..19, потеряны 0..12 (13 событий).
		Check(j.Count() == 8, "count equals capacity");
		Check(j.Dropped() == 13, "exactly 13 events dropped");
		RICON_LiveEvent first = j.At(0);
		Check(first.m_sKind == "GAP" && first.m_iValue == 13 && first.m_iTimeMs == 1012, "first record is GAP with loss count and last lost time");
		Check(first.m_iSeq == 0, "GAP keeps seq of the first evicted event");
		RICON_LiveEvent second = j.At(1);
		Check(second.m_iSeq == 13, "oldest kept event follows GAP in order");
		RICON_LiveEvent last = j.At(7);
		Check(last.m_iSeq == 19 && last.m_sKind == "TX_SEEN", "last record is the newest");
		Check(j.NextSeq() == 20, "next seq equals number of adds");
		string line = RICON_LiveJournal.Format(last);
		Check(line == "[RICON_LIVE] seq=19 t=1019 side=server kind=TX_SEEN player=7 device=-1 tr=-1 value=-1 text=von", "line format");
		Check(line.IndexOf("FAIL") < 0, "line has no FAIL token");
		j.Dump();
		PrintFormat("[RICON_LIVEJ] checks=%1 passed=%2 count=%3 dropped=%4", m_iChecks, m_iPassed, j.Count(), j.Dropped());
		if (m_iChecks == m_iPassed && m_iChecks == 15)
		{
			Print("[RICON_LIVEJ] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_LIVEJ] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
