//! TOOL_ONLY: самопроверка потокового режима журнала без мира, Play и сервера (claude #4028, codex #4035).
//! 1000 записей при ёмкости 8: поток печатает каждую сразу, буфер-снимок ограничен и честно помечает вытеснение GAP.
//! Сами потоковые строки проверяет внешний разбор лога (docs/evidence/receipts/live-stream-log-check-claude.py):
//! плагин не читает собственный вывод.
[WorkbenchPluginAttribute(name: "RICON: поток журнала live-прибора", description: "1000 записей, поток, снимок с GAP", wbModules: {"ScriptEditor"})]
class RICON_LiveStreamProbePlugin : WorkbenchPlugin
{
	static const int ADDS = 1000;
	static const int CAPACITY = 8;
	protected int m_iChecks;
	protected int m_iPassed;

	protected void Check(bool condition, string what)
	{
		m_iChecks++;
		if (condition)
			m_iPassed++;
		else
			Print("[RICON_LIVES] mismatch: " + what);
	}

	override void RunCommandline()
	{
		// Сообщения проверок — ASCII: кириллица в console.log Workbench печатается нечитаемо.
		RICON_LiveJournal quiet = new RICON_LiveJournal(4);
		Check(!quiet.IsStream(), "stream is off by default");
		RICON_LiveJournal j = new RICON_LiveJournal(CAPACITY);
		j.SetStream(true);
		Check(j.IsStream(), "stream switched on");
		Print("[RICON_LIVES] stream=BEGIN adds=1000 capacity=8");
		for (int i = 0; i < ADDS; i++)
		{
			int seq = j.Add(5000 + i, "client", "TX_CAPTURE", 7, -1, -1, 48000, "binding=UNKNOWN");
			if (seq != i)
				Check(false, "seq in order");
		}
		Print("[RICON_LIVES] stream=END");
		// Буфер: GAP + 7 новейших; потеряно для снимка 1000 - 7 = 993, время последнего вытесненного seq 992.
		Check(j.Count() == CAPACITY, "buffer bounded by capacity");
		Check(j.Dropped() == 993, "dropped from snapshot 993");
		Check(j.NextSeq() == ADDS, "next seq equals adds");
		RICON_LiveEvent gap = j.At(0);
		Check(gap.m_sKind == "GAP" && gap.m_iValue == 993 && gap.m_iSeq == 0 && gap.m_iTimeMs == 5992, "GAP counts snapshot eviction");
		RICON_LiveEvent oldest = j.At(1);
		RICON_LiveEvent newest = j.At(CAPACITY - 1);
		Check(oldest.m_iSeq == 993 && newest.m_iSeq == 999, "snapshot keeps the newest seven in order");
		j.Dump();
		PrintFormat("[RICON_LIVES] checks=%1 passed=%2 count=%3 dropped=%4 nextSeq=%5", m_iChecks, m_iPassed, j.Count(), j.Dropped(), j.NextSeq());
		if (m_iChecks == m_iPassed && m_iChecks == 7)
		{
			Print("[RICON_LIVES] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_LIVES] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
