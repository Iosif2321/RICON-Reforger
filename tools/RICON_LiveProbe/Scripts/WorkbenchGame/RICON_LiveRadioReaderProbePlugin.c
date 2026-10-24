//! TOOL_ONLY: самопроверка читателя раций без мира, Play и сервера (codex #4083): запись только изменений,
//! идентичность по сущности и индексу трансивера (не по порядку), перестановка, замена экземпляра, исчезновение и
//! повторное появление, изменение числа трансиверов, удалённая сущность, device = −1. Сущности заменены
//! синтетическими ключами Managed; настоящие рации — только в Play (START → ACK → END).
class RICON_LiveRadioKey : Managed
{
}

[WorkbenchPluginAttribute(name: "RICON: показания раций live-прибора", description: "RADIO_STATE/RADIO_GONE по сущности", wbModules: {"ScriptEditor"})]
class RICON_LiveRadioReaderProbePlugin : WorkbenchPlugin
{
	protected int m_iChecks;
	protected int m_iPassed;

	protected void Check(bool condition, string what)
	{
		m_iChecks++;
		if (condition)
			m_iPassed++;
		else
			Print("[RICON_LIVER] mismatch: " + what);
	}

	override void RunCommandline()
	{
		// Сообщения проверок — ASCII: кириллица в console.log Workbench печатается нечитаемо.
		RICON_LiveJournal j = new RICON_LiveJournal(64);
		RICON_LiveRadioReader r = new RICON_LiveRadioReader(j, "client");
		string prefab = "{0000000000000000}Prefabs/Items/Equipment/Radios/Radio_Test.et";
		string idle = RICON_LiveRadioReader.StateText(prefab, 30000, 79950, 50, true, false);
		string muted = RICON_LiveRadioReader.StateText(prefab, 30000, 79950, 50, true, true);
		Check(idle == "prefab={0000000000000000}Prefabs/Items/Equipment/Radios/Radio_Test.et,min=30000,max=79950,res=50,powered=1,muted=0", "state text");
		Check(idle.IndexOf(" ") < 0 && idle.IndexOf("FAIL") < 0, "state text has no spaces and no FAIL token");
		RICON_LiveRadioKey a = new RICON_LiveRadioKey();
		RICON_LiveRadioKey b = new RICON_LiveRadioKey();
		// Опрос 1: две рации — две записи, obs 0 и 1, device = -1.
		r.BeginPoll();
		r.Observe(7, a, 0, 0, 48000, idle);
		r.Observe(7, b, 1, 0, 48000, idle);
		Check(r.EndPoll(7) == 0 && j.Count() == 2 && r.Tracked() == 2, "first poll records both radios");
		RICON_LiveEvent first = j.At(0);
		Check(first.m_sKind == "RADIO_STATE" && first.m_iDeviceId == -1 && first.m_iTransceiver == 0 && first.m_iValue == 48000, "device is -1, transceiver and raw value kept");
		Check(first.m_sText.IndexOf("binding=LOCAL_ONLY,obs=0,slot=0,prefab=") == 0, "binding LOCAL_ONLY with observation id");
		// Опрос 2: та же пара в другом порядке, те же числа — без записей.
		r.BeginPoll();
		r.Observe(7, b, 0, 0, 48000, idle);
		r.Observe(7, a, 1, 0, 48000, idle);
		Check(r.EndPoll(7) == 0 && j.Count() == 2, "permutation of the list records nothing");
		// Опрос 3: изменение частоты и mute у a записываются.
		r.BeginPoll();
		r.Observe(7, a, 0, 0, 48050, muted);
		r.Observe(7, b, 1, 0, 48000, idle);
		r.EndPoll(7);
		Check(j.Count() == 3 && j.At(2).m_iValue == 48050 && j.At(2).m_sText.IndexOf("obs=0,") > 0 && j.At(2).m_sText.IndexOf("muted=1") > 0, "change recorded under the same observation");
		// Опрос 4: b заменена другим экземпляром c с теми же числами — новое наблюдение и RADIO_GONE для b.
		RICON_LiveRadioKey c = new RICON_LiveRadioKey();
		r.BeginPoll();
		r.Observe(7, a, 0, 0, 48050, muted);
		r.Observe(7, c, 1, 0, 48000, idle);
		Check(r.EndPoll(7) == 1 && j.Count() == 5, "replacement with equal numbers gives a new observation and a GONE");
		Check(j.At(3).m_sText.IndexOf("obs=2,") > 0 && j.At(4).m_sKind == "RADIO_GONE" && j.At(4).m_sText == "binding=LOCAL_ONLY,obs=1", "new obs for c, GONE for b");
		// Опрос 5: c пропала; опрос 6: c вернулась — новое наблюдение.
		r.BeginPoll();
		r.Observe(7, a, 0, 0, 48050, muted);
		r.EndPoll(7);
		r.BeginPoll();
		r.Observe(7, a, 0, 0, 48050, muted);
		r.Observe(7, c, 1, 0, 48000, idle);
		r.EndPoll(7);
		Check(j.Count() == 7 && j.At(5).m_sKind == "RADIO_GONE" && j.At(6).m_sText.IndexOf("obs=3,") > 0, "disappearance and reappearance give GONE and a new observation");
		// Опрос 7: у a появился второй трансивер; опрос 8: он исчез — GONE только для него.
		r.BeginPoll();
		r.Observe(7, a, 0, 0, 48050, muted);
		r.Observe(7, a, 0, 1, 52000, idle);
		r.Observe(7, c, 1, 0, 48000, idle);
		r.EndPoll(7);
		Check(j.Count() == 8 && j.At(7).m_iTransceiver == 1 && r.Tracked() == 3, "added transceiver recorded separately");
		r.BeginPoll();
		r.Observe(7, a, 0, 0, 48050, muted);
		r.Observe(7, c, 1, 0, 48000, idle);
		Check(r.EndPoll(7) == 1 && j.At(8).m_sKind == "RADIO_GONE" && j.At(8).m_iTransceiver == 1, "removed transceiver gives GONE only for it");
		// Удалённая сущность: слабая ссылка обнуляется, конец опроса пишет GONE.
		c = null;
		r.BeginPoll();
		r.Observe(7, a, 0, 0, 48050, muted);
		Check(r.EndPoll(7) == 1 && j.At(9).m_sKind == "RADIO_GONE" && r.Tracked() == 1, "deleted entity gives GONE");
		// Без сущности весь набор исчезает.
		Check(r.ReadEntity(7, null) == 0 && j.Count() == 11 && j.At(10).m_sKind == "RADIO_GONE" && r.Tracked() == 0, "null entity: everything GONE");
		Check(r.m_iRecorded == 6 && r.m_iGone == 5 && r.m_iReads == 1, "counters");
		string line = RICON_LiveJournal.Format(j.At(2));
		Check(line.IndexOf("kind=RADIO_STATE") > 0 && line.IndexOf("device=-1") > 0, "formatted line");
		// Регрессия A1: контекст игрока меняется без замены самой рации и без изменения её настроек.
		int unknownIndex = j.Count();
		r.BeginPoll();
		r.Observe(-1, a, 0, 0, 48000, idle);
		r.EndPoll(-1);
		Check(j.Count() == unknownIndex + 1 && j.At(unknownIndex).m_iPlayerId == -1 && j.At(unknownIndex).m_sText.IndexOf("obs=4,") > 0, "unknown player recorded under new observation");
		int resolvedIndex = j.Count();
		r.BeginPoll();
		r.Observe(7, a, 0, 0, 48000, idle);
		r.EndPoll(7);
		Check(j.Count() == resolvedIndex + 1 && j.At(resolvedIndex).m_iPlayerId == 7 && j.At(resolvedIndex).m_sText.IndexOf("obs=4,") > 0, "resolved player records state under same observation");
		int changedIndex = j.Count();
		r.BeginPoll();
		r.Observe(8, a, 0, 0, 48000, idle);
		r.EndPoll(8);
		Check(j.Count() == changedIndex + 1 && j.At(changedIndex).m_iPlayerId == 8 && j.At(changedIndex).m_sText.IndexOf("obs=4,") > 0, "changed player records state under same observation");
		int stableCount = j.Count();
		r.BeginPoll();
		r.Observe(8, a, 3, 0, 48000, idle);
		r.EndPoll(8);
		Check(j.Count() == stableCount, "same player and state do not duplicate after slot change");
		// Частота меняется явно: последняя запись игрока 8 есть и в старом reader, независимо от предыдущих ошибок.
		int changedStateIndex = j.Count();
		r.BeginPoll();
		r.Observe(8, a, 0, 0, 48050, muted);
		r.EndPoll(8);
		Check(j.Count() == changedStateIndex + 1 && j.At(changedStateIndex).m_iPlayerId == 8, "changed state establishes last observed player");
		int goneIndex = j.Count();
		r.ReadEntity(-1, null);
		Check(j.Count() == goneIndex + 1 && j.At(goneIndex).m_sKind == "RADIO_GONE" && j.At(goneIndex).m_iPlayerId == 8 && j.At(goneIndex).m_sText == "binding=LOCAL_ONLY,obs=4", "GONE keeps last observed player when controller disappears");
		j.Dump();
		PrintFormat("[RICON_LIVER] checks=%1 passed=%2 count=%3 recorded=%4 gone=%5", m_iChecks, m_iPassed, j.Count(), r.m_iRecorded, r.m_iGone);
		if (m_iChecks == m_iPassed && m_iChecks == 22)
		{
			Print("[RICON_LIVER] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_LIVER] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
