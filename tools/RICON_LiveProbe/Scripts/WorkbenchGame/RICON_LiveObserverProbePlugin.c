//! TOOL_ONLY: самопроверка наблюдателя без мира, Play и сервера: подписка на статический invoker ванильного
//! SCR_VoNComponent и на синтетические invokers тех же типов, записи radio-only, игнорирование прямой речи,
//! полная отписка, повторная подписка. Настоящие трансиверы и часы мира — только в Play (START → ACK → END).
[WorkbenchPluginAttribute(name: "RICON: observer live-прибора", description: "Attach/detach к invokers, записи radio-only", wbModules: {"ScriptEditor"})]
class RICON_LiveObserverProbePlugin : WorkbenchPlugin
{
	protected int m_iChecks;
	protected int m_iPassed;

	protected void Check(bool condition, string what)
	{
		m_iChecks++;
		if (condition)
			m_iPassed++;
		else
			Print("[RICON_LIVEO] mismatch: " + what);
	}

	override void RunCommandline()
	{
		// Сообщения проверок — ASCII: кириллица в console.log Workbench печатается нечитаемо.
		RICON_LiveJournal j = new RICON_LiveJournal(32);
		RICON_LiveObserver o = new RICON_LiveObserver(j, "server");
		// 1. Статический invoker ванильного компонента: существует без мира, подписка ровно одна.
		Check(o.AttachVonUsed(), "attach GetOnVoNUsed");
		Check(!o.AttachVonUsed(), "second attach refused");
		Check(o.m_iAttached == 1, "one subscription");
		ScriptInvokerInt used = SCR_VoNComponent.GetOnVoNUsed();
		used.Invoke(7);
		RICON_LiveEvent seen = j.At(0);
		Check(j.Count() == 1 && seen.m_sKind == "TX_SEEN" && seen.m_iPlayerId == 7 && seen.m_sSide == "server", "TX_SEEN via vanilla static invoker");
		Check(seen.m_iDeviceId == -1 && seen.m_iTransceiver == -1 && seen.m_sText == "binding=UNKNOWN,channel=UNKNOWN", "TX_SEEN binding and channel unknown");
		// 2. Типизированный invoker жизненного цикла того же типа, что GetOnPlayerSpawned().
		ScriptInvokerBase<SCR_BaseGameMode_PlayerIdAndEntity> spawned = new ScriptInvokerBase<SCR_BaseGameMode_PlayerIdAndEntity>();
		Check(o.AttachSpawned(spawned) && o.m_iAttached == 2, "attach spawned invoker");
		Check(!o.AttachSpawned(spawned), "second spawned attach refused");
		Check(!o.AttachGameMode(null), "null game mode refused");
		spawned.Invoke(5, null);
		RICON_LiveEvent spawn = j.At(1);
		Check(j.Count() == 2 && spawn.m_sKind == "SPAWN" && spawn.m_iPlayerId == 5, "SPAWN recorded");
		// 3. Нетипизированные invokers capture/receive: без трансивера (прямая речь) — игнор без записи.
		ScriptInvoker capture = new ScriptInvoker();
		ScriptInvoker receive = new ScriptInvoker();
		Check(!o.AttachClientInvokers(capture, null) && o.m_iAttached == 2, "partial pair refused without subscriptions");
		capture.Invoke(null);
		Check(o.m_iDirectIgnored == 0, "refused pair left no capture callback");
		Check(o.AttachClientInvokers(capture, receive) && o.m_iAttached == 4, "attach capture and receive");
		Check(!o.AttachClientInvokers(capture, receive) && o.m_iAttached == 4, "second pair refused without duplicates");
		Check(!o.AttachVonComponent(null), "null von component refused");
		capture.Invoke(null);
		receive.Invoke(3, null, 48000, 0.5);
		Check(j.Count() == 2 && o.m_iDirectIgnored == 2, "direct speech callbacks ignored and not recorded");
		// 4. Пути записи с явными значениями (настоящий BaseTransceiver есть только в Play).
		o.m_iLocalPlayerId = 7;
		int s1 = o.RecordCapture(48000);
		int s2 = o.RecordReceive(3, 48000, 0.5);
		RICON_LiveEvent c = j.At(2);
		RICON_LiveEvent r = j.At(3);
		Check(s1 == 2 && c.m_sKind == "TX_CAPTURE" && c.m_iValue == 48000 && c.m_iPlayerId == 7 && c.m_sText == "binding=UNKNOWN", "TX_CAPTURE record");
		Check(s2 == 3 && r.m_sKind == "RX_EVENT" && r.m_iPlayerId == 3 && r.m_iValue == 48000 && r.m_sText == "binding=UNKNOWN,listener=7,q=0.500000000", "RX_EVENT record");
		// 5. Полная отписка: события больше не записываются; повторная отписка — пустая.
		o.Detach();
		Check(o.m_iAttached == 0, "all subscriptions removed");
		used.Invoke(8);
		spawned.Invoke(9, null);
		capture.Invoke(null);
		receive.Invoke(1, null, 1, 1.0);
		Check(j.Count() == 4 && o.m_iDirectIgnored == 2, "no records after detach");
		o.Detach();
		Check(o.m_iAttached == 0, "second detach is a no-op");
		// 6. Повторная подписка после отписки.
		Check(o.AttachVonUsed() && o.m_iAttached == 1, "re-attach after detach");
		used.Invoke(11);
		RICON_LiveEvent again = j.At(4);
		Check(j.Count() == 5 && again.m_iPlayerId == 11 && again.m_iSeq == 4, "record after re-attach");
		o.Detach();
		// 7. Сторона — та, где стоит наблюдатель; часы без мира дают -1 (значение печатается, не утверждается).
		RICON_LiveObserver client = new RICON_LiveObserver(j, "client");
		client.RecordSeen(2);
		RICON_LiveEvent last = j.At(5);
		Check(last.m_sSide == "client" && last.m_iSeq == 5, "side is the observer side");
		int now = RICON_LiveObserver.Now();
		Check(last.m_iTimeMs == now, "time stamp comes from Now()");
		string line = RICON_LiveJournal.Format(r);
		Check(line.IndexOf("FAIL") < 0 && line.IndexOf("kind=RX_EVENT") > 0, "formatted line has kind and no FAIL token");
		RICON_LiveJournal precisionJournal = new RICON_LiveJournal(2);
		RICON_LiveObserver precision = new RICON_LiveObserver(precisionJournal, "client");
		precision.RecordReceive(3, 48000, 0.0001);
		Check(precisionJournal.At(0).m_sText == "binding=UNKNOWN,listener=-1,q=0.000100000", "quality below one permille preserved");
		// Частичная прежняя подписка не должна смешиваться с новой парой; исчезнувший invoker не оставляет счётчик.
		RICON_LiveObserver partial = new RICON_LiveObserver(precisionJournal, "probe");
		ScriptInvoker oldCapture = new ScriptInvoker();
		ScriptInvoker otherCapture = new ScriptInvoker();
		ScriptInvoker otherReceive = new ScriptInvoker();
		Check(partial.AttachCapture(oldCapture), "single_capture_attached");
		Check(!partial.AttachClientInvokers(otherCapture, otherReceive) && partial.m_iAttached == 1, "mixed_pair_rejected");
		otherCapture.Invoke(null);
		otherReceive.Invoke(3, null, 48000, 0.5);
		Check(partial.m_iDirectIgnored == 0, "mixed_pair_no_callback_leak");
		oldCapture = null;
		partial.Detach();
		Check(partial.m_iAttached == 0, "deleted_invoker_detach_resets_count");
		j.Dump();
		PrintFormat("[RICON_LIVEO] checks=%1 passed=%2 count=%3 ignored=%4 now=%5", m_iChecks, m_iPassed, j.Count(), o.m_iDirectIgnored, now);
		if (m_iChecks == m_iPassed && m_iChecks == 30)
		{
			Print("[RICON_LIVEO] done: PASS");
			Workbench.Exit(0);
		}
		else
		{
			Print("[RICON_LIVEO] done: FAIL");
			Workbench.Exit(5);
		}
	}
}
