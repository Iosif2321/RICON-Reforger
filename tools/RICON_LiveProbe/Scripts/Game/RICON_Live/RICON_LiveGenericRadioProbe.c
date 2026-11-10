//! TOOL_ONLY integration fixture: real RICON model admits generic native radio TX to a simulated B radio.
//! No authority lifecycle, VoN, cross-process payload, receiver mute or audio claim.
class RICON_LiveGenericRadioMessage : ScriptedRadioMessage
{
	BaseTransceiver m_ExpectedReceiver;
	int m_iAttempt;
	int m_iExpectedReceives;
	int m_iOtherReceives;
	int m_iInvalidExpected;
	ref RICON_GenericPairReceipt m_PairReceipt;

	override void OnDelivery(BaseTransceiver receiver, int freq, float quality)
	{
		bool expected = receiver && receiver == m_ExpectedReceiver;
		if (expected) m_iExpectedReceives++;
		else m_iOtherReceives++;
		if (expected && (freq != RICON_LiveGenericRadioProbe.FREQUENCY_KHZ || !RICON_Num.IsFinite(quality) || quality <= 0)) m_iInvalidExpected++;
		// Record every native callback; filtering the observable would manufacture a negative control.
		PrintFormat("[RICON_GENERIC] delivery attempt=%1 expectedB=%2 receiver=%3 freqKHz=%4 quality=%5 server=%6", m_iAttempt, expected, receiver, freq, quality, Replication.IsServer());
		if (receiver && receiver.GetRadio())
			PrintFormat("[RICON_GENERIC] receiverOwner attempt=%1 entity=%2", m_iAttempt, receiver.GetRadio().GetOwner());
		if (m_PairReceipt) m_PairReceipt.OnNativeDelivery(receiver, freq, quality, m_iAttempt, GetEncryptionKey());
	}
}

//! TEST_ONLY same-world information admission. Native callback counts above are never suppressed.
class RICON_GenericPairReceipt
{
	static const int TTL_MS = 2500;
	protected BaseTransceiver m_Tx;
	protected BaseTransceiver m_Rx;
	protected BaseRadioComponent m_TxRadio;
	protected BaseRadioComponent m_RxRadio;
	protected IEntity m_TxOwner;
	protected IEntity m_RxOwner;
	protected BaseWorld m_World;
	protected int m_TxCount;
	protected int m_RxCount;
	protected int m_Issued;
	protected int m_Generation;
	protected float m_TestPower;
	protected string m_Key;
	protected bool m_Active;
	protected bool m_Seen;
	bool m_Accepted;
	bool m_ModelLost;
	int m_EvaluatorCalls;
	string m_Decision = "NO_NATIVE_CALLBACK";

	protected bool Read(out RICON_RadioObservation a, out RICON_RadioObservation b)
	{
		a = null; b = null;
		if (!Replication.IsServer() || !m_World || m_World.IsEditMode() || GetGame().GetWorld() != m_World) return false;
		if (!m_Tx || !m_Rx || !m_TxRadio || !m_RxRadio || !m_TxOwner || !m_RxOwner) return false;
		if (m_TxRadio.GetOwner() != m_TxOwner || m_RxRadio.GetOwner() != m_RxOwner
			|| m_TxOwner.GetWorld() != m_World || m_RxOwner.GetWorld() != m_World
			|| m_Tx.GetRadio() != m_TxRadio || m_Rx.GetRadio() != m_RxRadio) return false;
		if (m_TxRadio.TransceiversCount() != m_TxCount || m_RxRadio.TransceiversCount() != m_RxCount
			|| m_TxRadio.GetTransceiver(0) != m_Tx || m_RxRadio.GetTransceiver(0) != m_Rx) return false;
		for (int i = 0; i < m_TxCount; i++)
		{
			BaseTransceiver tx = m_TxRadio.GetTransceiver(i);
			if (!tx || tx.GetRadio() != m_TxRadio || (i > 0 && tx == m_Tx)) return false;
		}
		for (int j = 0; j < m_RxCount; j++)
		{
			BaseTransceiver rx = m_RxRadio.GetTransceiver(j);
			if (!rx || rx.GetRadio() != m_RxRadio || (j > 0 && rx == m_Rx)) return false;
		}
		if (m_Tx.IsMuted() || m_Rx.IsMuted()) return false;
		if (!RICON_BaseRadioObservationReader.TryRead(m_TxRadio, 1, 0, m_TxOwner.GetOrigin(), a)
			|| !RICON_BaseRadioObservationReader.TryRead(m_RxRadio, 2, 0, m_RxOwner.GetOrigin(), b)) return false;
		return a.m_bPowered && b.m_bPowered && a.m_iTuningValue == RICON_LiveGenericRadioProbe.FREQUENCY_KHZ
			&& b.m_iTuningValue == RICON_LiveGenericRadioProbe.FREQUENCY_KHZ
			&& a.m_sNetworkKey == m_Key && b.m_sNetworkKey == m_Key;
	}
	static bool Same(RICON_RadioObservation a, RICON_RadioObservation b)
	{
		return a && b && a.m_iDeviceId == b.m_iDeviceId && a.m_iTransceiverIndex == b.m_iTransceiverIndex
			&& a.m_bPowered == b.m_bPowered && a.m_iTuningValue == b.m_iTuningValue
			&& a.m_sNetworkKey == b.m_sNetworkKey && a.m_vAntennaPos == b.m_vAntennaPos;
	}
	bool Init(BaseTransceiver tx, BaseTransceiver rx, int generation, float testPower)
	{
		if (!tx || !rx || tx == rx || !tx.GetRadio() || !rx.GetRadio() || tx.GetRadio() == rx.GetRadio()) return false;
		m_Tx = tx; m_Rx = rx; m_TxRadio = tx.GetRadio(); m_RxRadio = rx.GetRadio();
		m_TxOwner = m_TxRadio.GetOwner(); m_RxOwner = m_RxRadio.GetOwner();
		if (!m_TxOwner || !m_RxOwner || m_TxOwner == m_RxOwner) return false;
		m_World = m_TxOwner.GetWorld();
		m_TxCount = m_TxRadio.TransceiversCount(); m_RxCount = m_RxRadio.TransceiversCount();
		if (m_TxCount < 1 || m_TxCount > 4 || m_RxCount < 1 || m_RxCount > 4 || generation < 1 || generation > 4) return false;
		m_Key = m_TxRadio.GetEncryptionKey();
		m_Issued = System.GetTickCount(); m_Generation = generation; m_TestPower = testPower;
		RICON_RadioObservation a; RICON_RadioObservation b;
		m_Active = m_Issued >= 0 && !m_Key.IsEmpty() && Read(a, b);
		return m_Active;
	}
	protected void Decision(string code)
	{
		m_Decision = code;
		PrintFormat("[RICON_GENERIC_PAIR] receipt generation=%1 decision=%2 accepted=%3 modelLost=%4 evaluatorCalls=%5 scope=TEST_ONLY_SAMEWORLD_INFORMATION not=VON|AUDIO|REMOTE_AUTHORITY", m_Generation, code, m_Accepted, m_ModelLost, m_EvaluatorCalls);
	}
	void OnNativeDelivery(BaseTransceiver receiver, int freq, float nativeQuality, int generation, string messageKey)
	{
		if (receiver != m_Rx)
		{
			PrintFormat("[RICON_GENERIC_PAIR] receipt generation=%1 decision=OTHER_RECEIVER accepted=0", generation);
			return;
		}
		if (m_Seen) { PrintFormat("[RICON_GENERIC_PAIR] receipt generation=%1 decision=DUPLICATE_CALLBACK accepted=0", generation); return; }
		m_Seen = true;
		int now = System.GetTickCount();
		if (!m_Active || generation != m_Generation || now < m_Issued || now - m_Issued >= TTL_MS) { Decision("STALE_OR_CLOSED"); return; }
		if (freq != RICON_LiveGenericRadioProbe.FREQUENCY_KHZ || !RICON_Num.IsFinite(nativeQuality) || nativeQuality <= 0 || nativeQuality > 1
			|| messageKey != m_Key) { Decision("NATIVE_METADATA_INVALID"); return; }
		RICON_RadioObservation a; RICON_RadioObservation b;
		if (!Read(a, b)) { Decision("NATIVE_BINDING_INVALID"); return; }
		RICON_PairServiceInput input = RICON_LivePairShadow.CandidateInput(now, a, b);
		// Reuse accepted TEST_ONLY DTO/profile; only this fixture's model TX power is varied.
		input.m_aEndpoints[0].m_Profile.m_fTxPowerDbm = m_TestPower;
		input.m_aBindings[0].m_iLeaseId = m_Generation;
		input.m_aBindings[0].m_iIssuedAtMs = m_Issued; input.m_aBindings[0].m_iTtlMs = TTL_MS;
		input.m_aBindings[0].m_iTxGeneration = m_Generation;
		input.m_aPlayers[0].m_iGeneration = m_Generation; input.m_aPlayers[1].m_iGeneration = m_Generation;
		input.m_aListeners[0].m_iGeneration = m_Generation;
		RICON_WirelessLinkEvaluator evaluator = new RICON_WirelessLinkEvaluator(RICON_TestSupport.BAND_ID, new RICON_FreeSpaceModel(), new RICON_NoiseModel(), RICON_TestSupport.NewTestIntelligibility());
		RICON_PairService service = new RICON_PairService();
		RICON_PairServiceResult result = service.Evaluate(input, evaluator);
		m_EvaluatorCalls = result.m_iEvaluatorCalls;
		RICON_RadioObservation afterA; RICON_RadioObservation afterB;
		bool current = Read(afterA, afterB) && Same(a, afterA) && Same(b, afterB);
		int completed = System.GetTickCount();
		if (!current || completed < now || completed - now > 500 || completed - m_Issued >= TTL_MS || !m_Active) { Decision("POSTQUERY_CHANGED_OR_EXPIRED"); return; }
		if (result.m_eCallCode != RICON_EPairServiceCode.OK || result.m_aPairs.Count() != 1 || result.m_iS2Calls != 1 || result.m_iEvaluatorCalls != 1) { Decision("PAIR_SERVICE_REFUSED"); return; }
		RICON_PairResult row = result.m_aPairs[0];
		if (row.m_iLeaseId != m_Generation || row.m_iTxDeviceId != 1 || row.m_iRxPlayerId != 2 || row.m_iRxDeviceId != 2 || row.m_iRxTransceiverIndex != 0
			|| row.m_ePairCode != RICON_EPairCode.EVALUATED || !row.m_bHasDeviceCode || row.m_eDeviceCode != RICON_ERadioQueryCode.READY
			|| !row.m_bHasBudget || !row.m_bValid || row.m_eReason != RICON_EReason.OK
			|| !RICON_Num.IsFinite(row.m_fIntelligibility) || row.m_fIntelligibility < 0 || row.m_fIntelligibility > 1) { Decision("PAIR_RESULT_INVALID"); return; }
		m_ModelLost = row.m_eState == RICON_ELinkState.LOST;
		m_Accepted = row.m_eState == RICON_ELinkState.WEAK || row.m_eState == RICON_ELinkState.DEGRADED || row.m_eState == RICON_ELinkState.CLEAN;
		if (m_Accepted)
		{
			Decision("PAYLOAD_ADMITTED");
			PrintFormat("[RICON_GENERIC_PAIR] informationalPayload=GENERIC_PAIR_INFO_V1 receiptGeneration=%1 pairQuality=%2", m_Generation, row.m_fIntelligibility);
		}
		else if (m_ModelLost) Decision("MODEL_LOST_PAYLOAD_REFUSED");
		else Decision("PAIR_STATE_UNKNOWN");
	}
	void Close()
	{
		m_Active = false;
		m_Tx = null; m_Rx = null; m_TxRadio = null; m_RxRadio = null;
		m_TxOwner = null; m_RxOwner = null; m_World = null; m_Key = string.Empty;
	}
}

class RICON_LiveGenericRadioProbe
{
	protected BaseTransceiver m_Tx;
	protected BaseTransceiver m_Rx;
	protected ref array<ref RICON_LiveGenericRadioMessage> m_aMessages = {};
	protected ref RICON_LiveGenericRadioMessage m_Current;
	protected int m_iPhase;
	protected int m_iWrongFrequency;
	protected float m_fNext;
	protected bool m_bStopped;
	protected bool m_bPassed;
	protected bool m_bPairMode;
	protected int m_LastPairClock;
	protected int m_PairStarted;
	static const int FREQUENCY_KHZ = 32000;

	static bool Admits(RICON_LinkBudget budget)
	{
		return budget && budget.m_bValid && budget.m_eReason == RICON_EReason.OK
			&& (budget.m_eState == RICON_ELinkState.WEAK || budget.m_eState == RICON_ELinkState.DEGRADED || budget.m_eState == RICON_ELinkState.CLEAN);
	}

	static RICON_LinkBudget Evaluate(vector txPosition, vector rxPosition, float txPowerDbm)
	{
		RICON_LinkQuery query = RICON_TestSupport.NewQuery(RICON_FreeSpaceModel.MODEL_ID, FREQUENCY_KHZ / 1000.0);
		query.m_vTxPos = txPosition;
		query.m_vRxPos = rxPosition;
		query.m_fTxPowerDbm = txPowerDbm;
		RICON_FreeSpaceModel propagation = new RICON_FreeSpaceModel();
		RICON_NoiseModel noise = new RICON_NoiseModel();
		RICON_LinearTestIntelligibility quality = RICON_TestSupport.NewTestIntelligibility();
		RICON_WirelessLinkEvaluator evaluator = new RICON_WirelessLinkEvaluator(RICON_TestSupport.BAND_ID, propagation, noise, quality);
		return evaluator.Evaluate(query);
	}

	bool Start(BaseRadioComponent txRadio, BaseRadioComponent rxRadio, float now)
	{
		string pairMode;
		if (System.GetCLIParam("riconGenericPair", pairMode))
		{
			if (pairMode != "1" || !Replication.IsServer() || System.GetTickCount() < 0 || !RICON_Num.IsFinite(now)) return false;
			m_bPairMode = true;
			m_LastPairClock = System.GetTickCount();
			m_PairStarted = m_LastPairClock;
		}
		if (!txRadio || !rxRadio || txRadio == rxRadio || !txRadio.GetOwner() || !rxRadio.GetOwner()) return false;
		m_Tx = txRadio.GetTransceiver(0);
		m_Rx = rxRadio.GetTransceiver(0);
		if (!m_Tx || !m_Rx || m_Tx == m_Rx || m_Tx.IsMuted() || m_Rx.IsMuted()) return false;
		if (txRadio.GetEncryptionKey() != rxRadio.GetEncryptionKey()) return false;
		int resolution = m_Rx.GetFrequencyResolution();
		if (resolution <= 0 || FREQUENCY_KHZ < m_Rx.GetMinFrequency() || FREQUENCY_KHZ >= m_Rx.GetMaxFrequency()
			|| resolution > m_Rx.GetMaxFrequency() - FREQUENCY_KHZ
			|| FREQUENCY_KHZ < m_Tx.GetMinFrequency() || FREQUENCY_KHZ > m_Tx.GetMaxFrequency()) return false;
		m_iWrongFrequency = FREQUENCY_KHZ + resolution;
		// These handles belong only to the existing A0 fixture and are deleted by its Finish.
		txRadio.SetPower(true);
		rxRadio.SetPower(true);
		m_Tx.SetFrequency(FREQUENCY_KHZ);
		m_Rx.SetFrequency(FREQUENCY_KHZ);
		m_fNext = now + 1;
		PrintFormat("[RICON_GENERIC] start level=RICON_MODEL_NATIVE_RADIO_FIXTURE tx=%1 rx=%2 txEntity=%3 rxEntity=%4 server=%5", m_Tx, m_Rx, txRadio.GetOwner(), rxRadio.GetOwner(), Replication.IsServer());
		Print("[RICON_GENERIC] keyMatch=1 nativeTxEnd=UNKNOWN observation=FINITE_WINDOW modelPower=TEST_ONLY_NOT_NATIVE_READBACK");
		if (m_bPairMode) Print("[RICON_GENERIC_PAIR] mode=ENABLED default=OFF fixtureLifecycle=TEST_ONLY epoch=1 playerIds=TEST_ONLY_1_2 deviceIds=TEST_ONLY_1_2 antennaPoint=RADIO_ENTITY_ORIGIN profiles=TEST_ONLY nativeRfPower=NOT_READ");
		return true;
	}

	protected bool Ready()
	{
		return m_Tx && m_Rx && m_Tx.GetRadio() && m_Rx.GetRadio()
			&& m_Tx.GetRadio().IsPowered() && m_Rx.GetRadio().IsPowered()
			&& m_Tx.GetRadio().GetEncryptionKey() == m_Rx.GetRadio().GetEncryptionKey()
			&& !m_Tx.IsMuted() && !m_Rx.IsMuted() && m_Tx.GetFrequency() == FREQUENCY_KHZ;
	}

	protected bool Request(int attempt, float power, bool expectAdmission)
	{
		RICON_LinkBudget budget = Evaluate(m_Tx.GetRadio().GetOwner().GetOrigin(), m_Rx.GetRadio().GetOwner().GetOrigin(), power);
		bool admitted = Admits(budget);
		PrintFormat("[RICON_GENERIC] model attempt=%1 valid=%2 reason=%3 state=%4 quality=%5 rxDbm=%6 admitted=%7 fixtureTxDbm=%8", attempt, budget.m_bValid, budget.m_eReason, budget.m_eState, budget.m_fIntelligibility, budget.m_fRxPowerDbm, admitted, power);
		if (admitted != expectAdmission || !budget.m_bValid || budget.m_eReason != RICON_EReason.OK) return false;
		if (!admitted)
		{
			PrintFormat("[RICON_GENERIC] tx=SKIPPED_MODEL_LOST attempt=%1", attempt);
			return budget.m_eState == RICON_ELinkState.LOST;
		}
		m_Current = new RICON_LiveGenericRadioMessage();
		m_Current.m_iAttempt = attempt;
		m_Current.m_ExpectedReceiver = m_Rx;
		m_Current.SetEncryptionKey(m_Tx.GetRadio().GetEncryptionKey());
		if (m_Current.GetEncryptionKey() != m_Rx.GetRadio().GetEncryptionKey()) return false;
		m_aMessages.Insert(m_Current);
		PrintFormat("[RICON_GENERIC] tx=BEGIN attempt=%1 tx=%2 expectedRx=%3 txFreqKHz=%4 rxFreqKHz=%5", attempt, m_Tx, m_Rx, m_Tx.GetFrequency(), m_Rx.GetFrequency());
		m_Tx.BeginTransmission(m_Current);
		PrintFormat("[RICON_GENERIC] tx=RETURNED attempt=%1", attempt);
		return true;
	}

	//! 0 pending, 1 controls passed, -1 blocked/failure. Clock is the bench's elapsed game time.
	int Step(float now)
	{
		if (m_bPairMode) return StepPair(now);
		if (m_bStopped)
		{
			if (m_bPassed) return 1;
			return -1;
		}
		if (now < m_fNext) return 0;
		if (!Ready()) return End(false, "BINDING_OR_RADIO_STATE");
		m_fNext = now + 3;
		if (m_iPhase == 0)
		{
			if (m_Rx.GetFrequency() != FREQUENCY_KHZ || !Request(1, 37, true)) return End(false, "BASELINE_NOT_READY");
		}
		else if (m_iPhase == 1)
		{
			if (!m_Current || m_Current.m_iExpectedReceives < 1 || m_Current.m_iInvalidExpected > 0) return End(false, "BASELINE_NO_VALID_NATIVE_B_DELIVERY");
			m_Rx.SetFrequency(m_iWrongFrequency);
		}
		else if (m_iPhase == 2)
		{
			if (m_Rx.GetFrequency() != m_iWrongFrequency || !Request(2, 37, true)) return End(false, "WRONG_FREQUENCY_NOT_READY");
		}
		else if (m_iPhase == 3)
		{
			if (!m_Current || m_Current.m_iExpectedReceives != 0) return End(false, "WRONG_FREQUENCY_DELIVERED");
			m_Rx.SetFrequency(FREQUENCY_KHZ);
		}
		else if (m_iPhase == 4)
		{
			if (m_Rx.GetFrequency() != FREQUENCY_KHZ || !Request(3, -200, false)) return End(false, "MODEL_LOST_NOT_PROVEN");
		}
		else if (m_iPhase == 5)
		{
			if (!Request(4, 37, true)) return End(false, "RECOVERY_NOT_READY");
		}
		else
		{
			if (!m_Current || m_Current.m_iExpectedReceives < 1 || m_Current.m_iInvalidExpected > 0 || m_aMessages.Count() != 3) return End(false, "RECOVERY_NO_VALID_NATIVE_B_DELIVERY");
			// Check the retained negative message again so late delivery cannot silently become a pass.
			if (m_aMessages[1].m_iExpectedReceives != 0) return End(false, "LATE_WRONG_FREQUENCY_DELIVERY");
			return End(true, "GENERIC_NATIVE_B_ENDPOINT_AND_MODEL_TX_GATE");
		}
		m_iPhase++;
		return 0;
	}

	protected bool PairRequest(int generation, float testPower)
	{
		m_Current = new RICON_LiveGenericRadioMessage();
		m_Current.m_iAttempt = generation; m_Current.m_ExpectedReceiver = m_Rx;
		m_Current.m_PairReceipt = new RICON_GenericPairReceipt();
		if (!m_Current.m_PairReceipt.Init(m_Tx, m_Rx, generation, testPower)) return false;
		m_Current.SetEncryptionKey(m_Tx.GetRadio().GetEncryptionKey());
		if (m_Current.GetEncryptionKey() != m_Rx.GetRadio().GetEncryptionKey()) return false;
		m_aMessages.Insert(m_Current);
		PrintFormat("[RICON_GENERIC_PAIR] nativeTx=BEGIN generation=%1 fixtureModelTxDbm=%2 nativeTxPowerMutation=NONE", generation, testPower);
		// Always transmit in this explicit mode, including MODEL_LOST: native callback and receipt are independent.
		m_Tx.BeginTransmission(m_Current);
		return true;
	}
	protected int StepPair(float now)
	{
		if (m_bStopped) { if (m_bPassed) return 1; return -1; }
		int clock = System.GetTickCount();
		if (!Replication.IsServer() || clock < m_LastPairClock || clock - m_PairStarted >= 20000 || !RICON_Num.IsFinite(now)) return End(false, "PAIR_CLOCK_OR_AUTHORITY");
		m_LastPairClock = clock;
		if (now < m_fNext) return 0;
		if (!Ready() || m_Rx.GetFrequency() != FREQUENCY_KHZ) return End(false, "PAIR_BINDING_OR_RADIO_STATE");
		m_fNext = now + 3;
		if (m_iPhase > 0)
		{
			if (!m_Current || !m_Current.m_PairReceipt || m_Current.m_iExpectedReceives < 1 || m_Current.m_iInvalidExpected > 0) return End(false, "PAIR_NO_VALID_RAW_NATIVE_CALLBACK");
			RICON_GenericPairReceipt receipt = m_Current.m_PairReceipt;
			bool expectedLost = m_iPhase == 2;
			if (receipt.m_EvaluatorCalls != 1 || receipt.m_ModelLost != expectedLost || receipt.m_Accepted == expectedLost) return End(false, "PAIR_RECEIPT_CONTROL_FAILED");
			receipt.Close();
		}
		if (m_iPhase == 4) return End(true, "GENERIC_CALLBACK_PAIR_SERVICE_INFORMATION_RECEIPT");
		float power = 37;
		if (m_iPhase == 1) power = -200;
		if (!PairRequest(m_iPhase + 1, power)) return End(false, "PAIR_REQUEST_REFUSED");
		m_iPhase++;
		return 0;
	}

	protected int End(bool passed, string reason)
	{
		m_bStopped = true;
		m_bPassed = passed;
		if (m_bPairMode)
		{
			if (m_Current && m_Current.m_PairReceipt) m_Current.m_PairReceipt.Close();
			foreach (RICON_LiveGenericRadioMessage retained : m_aMessages)
				if (retained.m_PairReceipt) retained.m_PairReceipt.Close();
		}
		PrintFormat("[RICON_GENERIC] terminal passed=%1 reason=%2 phase=%3 not=VON|AUDIO|CROSSPROCESS_PAYLOAD|AUTHORITY|RX_SUPPRESSION", passed, reason, m_iPhase);
		if (passed) return 1;
		return -1;
	}

	void Stop()
	{
		if (!m_bStopped) End(false, "TEARDOWN_BEFORE_TERMINAL");
		// No public EndTransmission exists; contain only our radios before the bench deletes them.
		bool txOff;
		bool rxOff;
		if (m_Tx && m_Tx.GetRadio())
		{
			m_Tx.GetRadio().SetPower(false);
			txOff = !m_Tx.GetRadio().IsPowered();
		}
		if (m_Rx && m_Rx.GetRadio())
		{
			m_Rx.GetRadio().SetPower(false);
			rxOff = !m_Rx.GetRadio().IsPowered();
		}
		PrintFormat("[RICON_GENERIC] teardown txPowerOffReadback=%1 rxPowerOffReadback=%2 nativeTxEnd=UNKNOWN next=DELETE_OWN_RADIOS", txOff, rxOff);
		foreach (RICON_LiveGenericRadioMessage message : m_aMessages)
		{
			if (message.m_PairReceipt) message.m_PairReceipt.Close();
			message.m_ExpectedReceiver = null;
		}
		m_Current = null;
		m_aMessages.Clear();
		m_Tx = null;
		m_Rx = null;
	}
}
