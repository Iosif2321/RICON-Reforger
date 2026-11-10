//! TOOL_ONLY D0: one bounded CommunicationSound queue request and read-only local observers.
//! This is discovery evidence only: no VoN transport, sender, audibility or radio-delivery claim.
enum RICON_ED0Phase
{
	WAIT_BINDING = 0,
	WAIT_INITIAL_ACK = 1,
	QUIET_BASELINE = 2,
	READY = 3,
	WAIT_ACK = 4,
	PRECALL = 5,
	OBSERVE_REQUEST = 6,
	POST_OBSERVE = 7,
	STOPPED = 8
}

enum RICON_ECommunicationDiscoveryMode
{
	NONE = 0,
	D0 = 1,
	V0 = 2
}

enum RICON_EV0ReceiveClass
{
	INVALID = 0,
	DIRECT_UNQUALIFIED = 1,
	RADIO_UNQUALIFIED = 2
}

enum RICON_ED0RequestGate
{
	OK = 0,
	ALREADY_USED = 1,
	CLOCK = 2,
	A_DEADLINE = 3,
	ACK_RTT = 4,
	ACK_AGE = 5,
	B_DEADLINE = 6,
	GENERATION = 7
}

//! Pure bounded log accounting. Ordinary traffic can never consume the 96 terminal/GAP slots.
class RICON_D0LogBudget
{
	static const int ORDINARY_LIMIT = 4000;
	static const int TOTAL_LIMIT = 4096;
	protected int m_iOrdinary;
	protected int m_iReserved;
	protected bool m_bGap;

	bool TakeOrdinary()
	{
		if (m_iOrdinary >= ORDINARY_LIMIT || m_iOrdinary + m_iReserved >= TOTAL_LIMIT)
		{
			m_bGap = true;
			return false;
		}
		m_iOrdinary++;
		return true;
	}

	bool TakeReserved()
	{
		if (m_iReserved >= TOTAL_LIMIT - ORDINARY_LIMIT || m_iOrdinary + m_iReserved >= TOTAL_LIMIT)
			return false;
		m_iReserved++;
		return true;
	}

	int Ordinary() { return m_iOrdinary; }
	int Reserved() { return m_iReserved; }
	int Total() { return m_iOrdinary + m_iReserved; }
	bool HasGap() { return m_bGap; }
}

//! Pure gates are exercised by the ScriptEditor check plugin before any runtime authorization.
class RICON_D0Gate
{
	static const int OBSERVER_LIMIT_MS = 70000;
	static const int REQUEST_DEADLINE_MS = 39000;
	static const int V0_REQUEST_DEADLINE_MS = 37000;
	static const int ACK_LIMIT_MS = 1000;
	static const int CALL_BUDGET_MS = 1000;
	static const int REQUIRED_REMAINING_MS = 30000;
	static const int V0_REQUIRED_PRECALL_MS = 32000;

	static bool ResolveRequested(bool configured, string value)
	{
		return configured && value == "1";
	}

	static RICON_ECommunicationDiscoveryMode ResolveMode(bool d0Configured, string d0Value,
		bool v0Configured, string v0Value)
	{
		bool d0 = ResolveRequested(d0Configured, d0Value);
		bool v0 = ResolveRequested(v0Configured, v0Value);
		if (d0 == v0)
			return RICON_ECommunicationDiscoveryMode.NONE;
		if (v0)
			return RICON_ECommunicationDiscoveryMode.V0;
		return RICON_ECommunicationDiscoveryMode.D0;
	}

	static bool ValidRunId(string value)
	{
		if (value.IsEmpty() || value.Length() > 64)
			return false;
		for (int i = 0; i < value.Length(); i++)
		{
			int code = value.ToAscii(i);
			bool alpha = (code >= 65 && code <= 90) || (code >= 97 && code <= 122);
			bool digit = code >= 48 && code <= 57;
			if (!alpha && !digit && code != 45 && code != 46 && code != 95)
				return false;
		}
		return true;
	}

	static bool ValidControlKey(string value)
	{
		if (value.Length() < 4 || value.Length() > 67)
			return false;
		string suffix = value.Substring(value.Length() - 3, 3);
		if (suffix != ".d0" && suffix != ".v0")
			return false;
		return ValidRunId(value.Substring(0, value.Length() - 3));
	}

	static bool Forward(int previous, int current)
	{
		return previous >= 0 && current >= previous;
	}

	static bool ValidCallbackContext(bool clockValid, bool scriptedEnabled, bool controllerMatch, bool entityMatch,
		bool communicationMatch, bool vonMatch, bool radiosMatch, bool subscriptionsReady,
		int actualRole, int expectedRole, int entityGeneration, int componentGeneration, int subscriptionGeneration)
	{
		return clockValid && scriptedEnabled && controllerMatch && entityMatch && communicationMatch && vonMatch && radiosMatch
			&& subscriptionsReady && actualRole == expectedRole && entityGeneration > 0
			&& componentGeneration > 0 && subscriptionGeneration > 0;
	}

	static bool ValidRadioReceive(bool observing, int localRole, int expectedLocalRole, int senderId,
		int expectedSenderId, bool receiverPresent, int receiverIndex)
	{
		return observing && localRole == expectedLocalRole && senderId == expectedSenderId
			&& receiverPresent && receiverIndex >= 0;
	}

	static bool ValidV0Values(bool nonNull, int count)
	{
		return nonNull && count == 2;
	}

	static bool AllowV1StartupGap(bool v0, bool waiting, int entityGeneration, int componentGeneration,
		int subscriptionGeneration, bool nativeWrites)
	{
		return v0 && waiting && entityGeneration == 0 && componentGeneration == 0
			&& subscriptionGeneration == 0 && !nativeWrites;
	}

	static bool ValidV1ControlFlag(bool configured, string value, bool nativeMode)
	{
		return !configured || (nativeMode && value == "1");
	}

	static int V1ExpectedFrequency(bool wrongFrequency, int role)
	{
		if (role == 1) return 48000;
		if (role == 2)
		{
			if (wrongFrequency) return 48200;
			return 48000;
		}
		return -1;
	}

	static bool V1ControlFrequenciesClear(bool control, array<int> frequencies)
	{
		if (!control) return true;
		if (!frequencies || frequencies.IsEmpty() || frequencies.Count() > 8) return false;
		foreach (int frequency : frequencies)
		{
			if (frequency != 48200) return false;
		}
		return true;
	}

	static bool V1FrequencyFits(int minimum, int maximum, int step, int frequency)
	{
		return minimum <= maximum && step > 0 && frequency >= minimum && frequency <= maximum
			&& (frequency - minimum) % step == 0;
	}

	static bool ValidV1Signal(array<float> values, int seedIndex)
	{
		return values && ValidV0Values(true, values.Count()) && seedIndex >= 0
			&& values[0] == seedIndex && values[1] >= 0 && values[1] <= 1;
	}

	static RICON_EV0ReceiveClass ClassifyV0Receive(bool observing, int localRole, int senderId,
		bool receiverPresent, int receiverIndex)
	{
		if (!observing || localRole != 2 || senderId != 1)
			return RICON_EV0ReceiveClass.INVALID;
		if (!receiverPresent)
			return RICON_EV0ReceiveClass.DIRECT_UNQUALIFIED;
		if (receiverIndex >= 0)
			return RICON_EV0ReceiveClass.RADIO_UNQUALIFIED;
		return RICON_EV0ReceiveClass.INVALID;
	}

	static bool ValidChallengeSequence(int challengeId, int attempt, int lastChallengeId, int lastAttempt)
	{
		if (challengeId < 1 || challengeId > 2 || attempt < 1)
			return false;
		if (challengeId == lastChallengeId)
			return attempt > lastAttempt;
		return challengeId == lastChallengeId + 1;
	}

	static bool CanRetryChallenge(bool awaiting, int attempt, int sinceSentMs, int elapsedMs)
	{
		return awaiting && attempt < 20 && sinceSentMs >= 1500 && elapsedMs <= REQUEST_DEADLINE_MS;
	}

	static bool V1QuietComplete(bool initialAck, bool contextValid, int quietSince, int now, int quietMs)
	{
		return initialAck && contextValid && quietMs > 0 && Forward(quietSince, now) && now - quietSince >= quietMs;
	}

	static bool CanStartV1FreshChallenge(int now, int t0)
	{
		return Forward(t0, now) && now - t0 <= V0_REQUEST_DEADLINE_MS
			&& OBSERVER_LIMIT_MS - (now - t0) - CALL_BUDGET_MS >= V0_REQUIRED_PRECALL_MS;
	}

	static bool CanRetryChallengeV0(bool awaiting, int attempt, int sinceSentMs, int elapsedMs)
	{
		return awaiting && attempt < 20 && sinceSentMs >= 1500 && elapsedMs <= V0_REQUEST_DEADLINE_MS;
	}

	static bool PostWindowComplete(int now, int postUntil)
	{
		return postUntil >= 0 && now >= postUntil;
	}

	static bool CanMutateAfterClaim(bool oneShotClaimed, bool requestBeginReserved, bool stillValid,
		bool finalTimeGate, bool currentContext, bool serverBIdentity)
	{
		return oneShotClaimed && requestBeginReserved && stillValid && finalTimeGate
			&& currentContext && serverBIdentity;
	}

	protected static RICON_ED0RequestGate CanRequestWithLimits(bool requestUsed, int nowA, int t0A,
		int ackSentA, int ackReceivedA, int reportedRemainingB, int ackEntityGeneration,
		int ackComponentGeneration, int ackSubscriptionGeneration, int expectedEntityGeneration,
		int expectedComponentGeneration, int expectedSubscriptionGeneration, int deadlineMs, int requiredRemainingMs)
	{
		if (requestUsed)
			return RICON_ED0RequestGate.ALREADY_USED;
		if (t0A < 0 || ackSentA < t0A || ackReceivedA < ackSentA || nowA < ackReceivedA)
			return RICON_ED0RequestGate.CLOCK;
		int elapsedA = nowA - t0A;
		int rttA = ackReceivedA - ackSentA;
		int receiveAgeA = nowA - ackReceivedA;
		if (elapsedA > deadlineMs || OBSERVER_LIMIT_MS - elapsedA - CALL_BUDGET_MS < requiredRemainingMs)
			return RICON_ED0RequestGate.A_DEADLINE;
		if (rttA > ACK_LIMIT_MS)
			return RICON_ED0RequestGate.ACK_RTT;
		if (receiveAgeA > ACK_LIMIT_MS)
			return RICON_ED0RequestGate.ACK_AGE;
		if (reportedRemainingB - rttA - receiveAgeA - CALL_BUDGET_MS < requiredRemainingMs)
			return RICON_ED0RequestGate.B_DEADLINE;
		if (ackEntityGeneration < 1 || ackComponentGeneration < 1 || ackSubscriptionGeneration < 1)
			return RICON_ED0RequestGate.GENERATION;
		if (ackEntityGeneration != expectedEntityGeneration
			|| ackComponentGeneration != expectedComponentGeneration
			|| ackSubscriptionGeneration != expectedSubscriptionGeneration)
			return RICON_ED0RequestGate.GENERATION;
		return RICON_ED0RequestGate.OK;
	}

	static RICON_ED0RequestGate CanRequest(bool requestUsed, int nowA, int t0A, int ackSentA, int ackReceivedA,
		int reportedRemainingB, int ackEntityGeneration, int ackComponentGeneration, int ackSubscriptionGeneration,
		int expectedEntityGeneration, int expectedComponentGeneration, int expectedSubscriptionGeneration)
	{
		return CanRequestWithLimits(requestUsed, nowA, t0A, ackSentA, ackReceivedA, reportedRemainingB,
			ackEntityGeneration, ackComponentGeneration, ackSubscriptionGeneration, expectedEntityGeneration,
			expectedComponentGeneration, expectedSubscriptionGeneration, REQUEST_DEADLINE_MS, REQUIRED_REMAINING_MS);
	}

	static RICON_ED0RequestGate CanRequestV0(bool requestUsed, int nowA, int t0A, int ackSentA, int ackReceivedA,
		int reportedRemainingB, int ackEntityGeneration, int ackComponentGeneration, int ackSubscriptionGeneration,
		int expectedEntityGeneration, int expectedComponentGeneration, int expectedSubscriptionGeneration)
	{
		return CanRequestWithLimits(requestUsed, nowA, t0A, ackSentA, ackReceivedA, reportedRemainingB,
			ackEntityGeneration, ackComponentGeneration, ackSubscriptionGeneration, expectedEntityGeneration,
			expectedComponentGeneration, expectedSubscriptionGeneration, V0_REQUEST_DEADLINE_MS, V0_REQUIRED_PRECALL_MS);
	}

	static bool CanObserveV0AfterReturn(int returnNowA, int t0A, int ackSentA, int ackReceivedA,
		int reportedRemainingB)
	{
		if (t0A < 0 || ackSentA < t0A || ackReceivedA < ackSentA || returnNowA < ackReceivedA)
			return false;
		int rttA = ackReceivedA - ackSentA;
		int ageA = returnNowA - ackReceivedA;
		return rttA <= ACK_LIMIT_MS && OBSERVER_LIMIT_MS - (returnNowA - t0A) >= REQUIRED_REMAINING_MS
			&& reportedRemainingB - rttA - ageA >= REQUIRED_REMAINING_MS;
	}
}

//! Pure state-trace model for late B readiness and final pre-call veto regressions; no engine calls.
class RICON_D0TraceModel
{
	protected int m_iStep;
	protected int m_iEntityGeneration;
	protected int m_iComponentGeneration;
	protected int m_iSubscriptionGeneration;
	protected bool m_bBlocked;
	protected bool m_bRequestUsed;
	protected bool m_bRequestCalled;

	void StartA()
	{
		m_iStep = 1;
	}

	bool InitialReadiness(bool bReady, bool timedOut, int entityGeneration, int componentGeneration,
		int subscriptionGeneration)
	{
		if (m_iStep != 1 || m_bBlocked)
			return false;
		if (timedOut)
		{
			m_bBlocked = true;
			return false;
		}
		if (!bReady)
			return false;
		if (entityGeneration < 1 || componentGeneration < 1 || subscriptionGeneration < 1)
		{
			m_bBlocked = true;
			return false;
		}
		m_iEntityGeneration = entityGeneration;
		m_iComponentGeneration = componentGeneration;
		m_iSubscriptionGeneration = subscriptionGeneration;
		m_iStep = 2;
		return true;
	}

	bool BaselineComplete()
	{
		if (m_iStep != 2 || m_bBlocked)
			return false;
		m_iStep = 3;
		return true;
	}

	bool FreshReadiness(bool bReady, bool identityMatch, int entityGeneration, int componentGeneration,
		int subscriptionGeneration)
	{
		if (m_iStep != 3 || m_bBlocked || !bReady)
			return false;
		if (!identityMatch || entityGeneration != m_iEntityGeneration
			|| componentGeneration != m_iComponentGeneration || subscriptionGeneration != m_iSubscriptionGeneration)
		{
			m_bBlocked = true;
			return false;
		}
		m_iStep = 4;
		return true;
	}

	bool PreCall(bool requestBeginReserved, bool finalTimeGate, bool currentContext,
		bool serverBIdentity, bool invalid)
	{
		if (m_iStep != 4 || m_bBlocked || m_bRequestUsed)
			return false;
		m_bRequestUsed = true;
		bool allow = RICON_D0Gate.CanMutateAfterClaim(m_bRequestUsed, requestBeginReserved, !invalid,
			finalTimeGate, currentContext, serverBIdentity);
		if (!allow)
		{
			m_bBlocked = true;
			return false;
		}
		m_bRequestCalled = true;
		m_iStep = 5;
		return true;
	}

	bool IsBlocked() { return m_bBlocked; }
	bool IsRequestUsed() { return m_bRequestUsed; }
	bool IsRequestCalled() { return m_bRequestCalled; }
}

//! One captured scratch endpoint; only the server-A coordinator calls Apply/Restore.
class RICON_V1RadioAlignment
{
	protected IEntity m_Entity;
	protected BaseRadioComponent m_Radio;
	protected BaseTransceiver m_Transceiver;
	protected int m_iPreviousFrequency;
	protected string m_sPreviousKey;
	protected int m_iFrequency;
	protected string m_sKey;
	protected bool m_bApplied;

	bool Capture(IEntity entity, BaseTransceiver transceiver, int frequency, string key)
	{
		if (!entity || !transceiver || !transceiver.GetRadio()
			|| !RICON_D0Gate.V1FrequencyFits(transceiver.GetMinFrequency(), transceiver.GetMaxFrequency(),
				transceiver.GetFrequencyResolution(), frequency))
			return false;
		m_Entity = entity;
		m_Transceiver = transceiver;
		m_Radio = transceiver.GetRadio();
		m_iPreviousFrequency = transceiver.GetFrequency();
		m_sPreviousKey = m_Radio.GetEncryptionKey();
		m_iFrequency = frequency;
		m_sKey = key;
		return true;
	}

	void Apply()
	{
		if (m_bApplied)
			return;
		m_bApplied = true;
		if (m_iPreviousFrequency != m_iFrequency)
			m_Transceiver.SetFrequency(m_iFrequency);
		if (m_sPreviousKey != m_sKey && m_Radio.GetEncryptionKey() != m_sKey)
			m_Radio.SetEncryptionKey(m_sKey);
	}

	bool Matches(IEntity entity, BaseTransceiver transceiver)
	{
		return entity && entity == m_Entity && transceiver && transceiver == m_Transceiver
			&& m_Radio && transceiver.GetRadio() == m_Radio;
	}

	static string Describe(IEntity entity, BaseTransceiver transceiver, int frequency, string key)
	{
		if (!entity || !transceiver || !transceiver.GetRadio())
			return "binding=0,worldKnown=0,nativeManager=UNKNOWN";
		BaseRadioComponent radio = transceiver.GetRadio();
		ChimeraWorld world = ChimeraWorld.CastFrom(entity.GetWorld());
		int frequencyMatch, keyMatch, powered, muted, worldKnown, nativeManager;
		if (transceiver.GetFrequency() == frequency) frequencyMatch = 1;
		if (radio.GetEncryptionKey() == key) keyMatch = 1;
		if (radio.IsPowered()) powered = 1;
		if (transceiver.IsMuted()) muted = 1;
		if (world) worldKnown = 1;
		if (world && world.GetRadioManager()) nativeManager = 1;
		return string.Format("binding=1,frequency=%1,freqMatch=%2,keyMatch=%3,powered=%4,muted=%5,worldKnown=%6,nativeManager=%7",
			transceiver.GetFrequency(), frequencyMatch, keyMatch, powered, muted, worldKnown, nativeManager);
	}

	string Readback()
	{
		return Describe(m_Entity, m_Transceiver, m_iFrequency, m_sKey);
	}

	bool Ready()
	{
		return m_Transceiver && m_Radio && m_Transceiver.GetFrequency() == m_iFrequency
			&& m_Radio.GetEncryptionKey() == m_sKey && m_Radio.IsPowered() && !m_Transceiver.IsMuted();
	}

	void Restore(IEntity entity, BaseTransceiver transceiver, out bool frequencyRestored, out bool keyRestored)
	{
		frequencyRestored = !m_bApplied;
		keyRestored = !m_bApplied;
		if (!m_bApplied)
			return;
		m_bApplied = false;
		if (!Matches(entity, transceiver))
			return;
		if (m_Transceiver.GetFrequency() == m_iPreviousFrequency)
			frequencyRestored = true;
		else if (m_iPreviousFrequency != m_iFrequency && m_Transceiver.GetFrequency() == m_iFrequency)
		{
			m_Transceiver.SetFrequency(m_iPreviousFrequency);
			frequencyRestored = m_Transceiver.GetFrequency() == m_iPreviousFrequency;
		}
		if (m_Radio.GetEncryptionKey() == m_sPreviousKey)
			keyRestored = true;
		else if (m_sPreviousKey != m_sKey && m_Radio.GetEncryptionKey() == m_sKey)
		{
			m_Radio.SetEncryptionKey(m_sPreviousKey);
			keyRestored = m_Radio.GetEncryptionKey() == m_sPreviousKey;
		}
		m_sPreviousKey = string.Empty;
	}
}

class RICON_LiveCommunicationDiscovery
{
	static const int ROLE_A = 1;
	static const int ROLE_B = 2;
	static const int STABLE_MS = 5000;
	static const int QUIET_MS = 6000;
	static const int HEARTBEAT_MS = 1000;
	static const int HEARTBEAT_GAP_MS = 3000;
	static const int TERMINAL_WAIT_MS = 20000;

	protected static RICON_LiveCommunicationDiscovery s_Active;
	protected ref RICON_D0LogBudget m_Budget;
	protected BaseWorld m_World;
	protected PlayerController m_Controller;
	protected IEntity m_Entity;
	protected SCR_PlayerController m_ServerBController;
	protected IEntity m_ServerBEntity;
	protected SCR_PlayerController m_FreshBController;
	protected IEntity m_FreshBEntity;
	protected SCR_CommunicationSoundComponent m_Communication;
	protected SCR_VoNComponent m_Von;
	protected OnSoundEventStartedInvoker m_SoundStarted;
	protected OnSoundEventFinishedInvoker m_SoundFinished;
	protected ScriptInvoker m_VonCapture;
	protected ScriptInvoker m_VonReceive;
	protected ref array<BaseTransceiver> m_Radios = {};
	protected ref array<float> m_V0Values;
	protected int m_iSeedIndex = -1;
	protected bool m_bWrongFrequency;
	static const string V1_FIXTURE_KEY = "RICON_TOOL_V1";
	static const int V1_SETTLE_MS = 8000;
	protected ref RICON_V1RadioAlignment m_V1AlignmentA;
	protected ref RICON_V1RadioAlignment m_V1AlignmentB;
	protected ref array<ref RICON_V1RadioAlignment> m_V1ControlBExtras = {};
	protected ref array<BaseTransceiver> m_V1ControlBCatalog = {};
	protected SCR_PlayerController m_V1BController;
	protected bool m_bV1PairPrepared;
	protected int m_iV1PairPreparedAt = -1;
	protected int m_iV1PeerSettleAt = -1;
	protected ECommMethod m_PreviousMethod;
	protected BaseTransceiver m_PreviousTransmit;
	protected BaseTransceiver m_SelectedTransmit;
	protected bool m_bTransmitRestorePending;
	protected bool m_bPreviousTransmitPresent;
	protected IEntity m_ProxyAEntity;
	protected SCR_CommunicationSoundComponent m_ProxyASound;
	protected OnSoundEventStartedInvoker m_ProxyAStarted;
	protected int m_iProxyAStarted;
	protected bool m_bProxyAUnavailableLogged;
	protected bool m_bV1PeerReadbackLogged;
	protected RICON_ECommunicationDiscoveryMode m_Mode = RICON_ECommunicationDiscoveryMode.NONE;
	protected string m_sRunId;
	protected int m_iPlayerId = -1;
	protected int m_iT0 = -1;
	protected int m_iLastTick = -1;
	protected int m_iStableSince = -1;
	protected int m_iBaselineSince = -1;
	protected int m_iLastHeartbeat = -1;
	protected int m_iEntityGeneration;
	protected int m_iComponentGeneration;
	protected int m_iSubscriptionGeneration;
	protected int m_iChallenge;
	protected int m_iChallengeAttempt;
	protected int m_iLastChallengeHandled;
	protected int m_iLastChallengeAttempt;
	protected int m_iChallengeSent = -1;
	protected int m_iInitialEntityGeneration = -1;
	protected int m_iInitialComponentGeneration = -1;
	protected int m_iInitialSubscriptionGeneration = -1;
	protected int m_iAckReceived = -1;
	protected int m_iAckRemainingB = -1;
	protected int m_iAckPlayerId = -1;
	protected int m_iAckEntityGeneration = -1;
	protected int m_iAckComponentGeneration = -1;
	protected int m_iAckSubscriptionGeneration = -1;
	protected int m_iRequestTick = -1;
	protected int m_iPostUntil = -1;
	protected int m_iSequence;
	protected int m_iStarted;
	protected int m_iFinished;
	protected int m_iVonReceive;
	protected int m_iVonDirect;
	protected int m_iVonRadio;
	protected AudioHandle m_RequestHandle = AudioHandle.Invalid;
	protected RICON_ED0Phase m_Phase = RICON_ED0Phase.WAIT_BINDING;
	protected bool m_bRunning;
	protected bool m_bRequestUsed;
	protected bool m_bInvalid;
	protected bool m_bGapRecorded;
	protected bool m_bInNativeCall;
	protected string m_sDeferredStop;

	static RICON_LiveCommunicationDiscovery Active()
	{
		return s_Active;
	}

	static RICON_ECommunicationDiscoveryMode RequestedMode()
	{
		string d0Value;
		string v0Value;
		bool d0Configured = System.GetCLIParam("riconCommunicationDiscovery", d0Value);
		bool v0Configured = System.GetCLIParam("riconVoNInputShape", v0Value);
		return RICON_D0Gate.ResolveMode(d0Configured, d0Value, v0Configured, v0Value);
	}

	static bool IsRequested()
	{
		if (RequestedMode() == RICON_ECommunicationDiscoveryMode.NONE)
			return false;
		string runId;
		return System.GetCLIParam("riconCommunicationRun", runId) && RICON_D0Gate.ValidRunId(runId);
	}

	protected static string Bit(bool value)
	{
		if (value)
			return "1";
		return "0";
	}

	protected string Prefix()
	{
		if (m_Mode == RICON_ECommunicationDiscoveryMode.V0)
			return "[RICON_V0]";
		return "[RICON_D0]";
	}

	protected int ExpectedV1Frequency(int role)
	{
		return RICON_D0Gate.V1ExpectedFrequency(m_bWrongFrequency, role);
	}

	protected string V1Case()
	{
		if (m_bWrongFrequency) return "A48000_B48200";
		return "A48000_B48000";
	}

	protected bool IsV0()
	{
		return m_Mode == RICON_ECommunicationDiscoveryMode.V0;
	}

	protected string ControlKey()
	{
		if (IsV0())
			return m_sRunId + ".v0";
		return m_sRunId + ".d0";
	}

	protected bool V0ValuesReady()
	{
		if (!m_V0Values)
			return false;
		return RICON_D0Gate.ValidV1Signal(m_V0Values, m_iSeedIndex);
	}

	protected int RequestDeadlineMs()
	{
		if (IsV0())
			return RICON_D0Gate.V0_REQUEST_DEADLINE_MS;
		return RICON_D0Gate.REQUEST_DEADLINE_MS;
	}

	protected bool CanRetryCurrent(bool awaiting, int attempt, int sinceSentMs, int elapsedMs)
	{
		if (IsV0())
			return RICON_D0Gate.CanRetryChallengeV0(awaiting, attempt, sinceSentMs, elapsedMs);
		return RICON_D0Gate.CanRetryChallenge(awaiting, attempt, sinceSentMs, elapsedMs);
	}

	protected int Now()
	{
		return System.GetTickCount();
	}

	protected bool Emit(string kind, string fields = "", bool reserved = false)
	{
		bool accepted;
		if (reserved)
			accepted = m_Budget.TakeReserved();
		else
			accepted = m_Budget.TakeOrdinary();
		if (!accepted)
		{
			m_bInvalid = true;
			if (!m_bGapRecorded && m_Budget.TakeReserved())
			{
				m_bGapRecorded = true;
				PrintFormat("%1 seq=%2 t=%3 run=%4 role=%5 kind=GAP ordinary=%6 reserved=%7", Prefix(),
					m_iSequence++, Now(), m_sRunId, m_iPlayerId, m_Budget.Ordinary(), m_Budget.Reserved());
			}
			return false;
		}
		int sequence = m_iSequence;
		m_iSequence++;
		string line = Prefix() + " seq=" + sequence.ToString();
		line = line + " t=" + Now().ToString() + " run=" + m_sRunId;
		line = line + " role=" + m_iPlayerId.ToString() + " phase=" + m_Phase.ToString();
		line = line + " eg=" + m_iEntityGeneration.ToString() + " cg=" + m_iComponentGeneration.ToString();
		if (IsV0()) fields = fields + ",case=" + V1Case();
		line = line + " sg=" + m_iSubscriptionGeneration.ToString() + " kind=" + kind + " " + fields;
		PrintFormat("%1", line);
		return true;
	}

	protected bool ReadRunId(out string runId)
	{
		return System.GetCLIParam("riconCommunicationRun", runId) && RICON_D0Gate.ValidRunId(runId);
	}

	bool Start(BaseWorld world)
	{
		m_Mode = RequestedMode();
		if (m_Mode == RICON_ECommunicationDiscoveryMode.NONE || m_bRunning || s_Active
			|| !world || world.IsEditMode() || !ReadRunId(m_sRunId))
			return false;
		string controlValue;
		bool controlGiven = System.GetCLIParam("riconVoNWrongFrequency", controlValue);
		if (!RICON_D0Gate.ValidV1ControlFlag(controlGiven, controlValue, IsV0())) return false;
		m_bWrongFrequency = controlGiven;
		m_World = world;
		m_Budget = new RICON_D0LogBudget();
		m_iT0 = Now();
		if (m_iT0 < 0)
			return false;
		m_iLastTick = m_iT0;
		m_iLastHeartbeat = m_iT0;
		m_bRunning = true;
		s_Active = this;
		Emit("START", "mode=" + m_Mode.ToString() + ",mutation=QUEUE_POSSIBLE,transport=UNPROVEN", true);
		return true;
	}

	protected void Detach()
	{
		if (m_ProxyAStarted)
			m_ProxyAStarted.Remove(OnProxyASoundStarted);
		m_ProxyAStarted = null;
		m_ProxyASound = null;
		m_ProxyAEntity = null;
		if (m_SoundStarted)
			m_SoundStarted.Remove(OnSoundStarted);
		if (m_SoundFinished)
			m_SoundFinished.Remove(OnSoundFinished);
		if (m_VonCapture)
			m_VonCapture.Remove(OnVonCapture);
		if (m_VonReceive)
			m_VonReceive.Remove(OnVonReceive);
		m_SoundStarted = null;
		m_SoundFinished = null;
		m_VonCapture = null;
		m_VonReceive = null;
		m_iSubscriptionGeneration++;
	}

	protected bool Attach()
	{
		if (!m_Communication || !m_Von || !m_Communication.IsScriptedMethodsCallEnabled())
			return false;
		m_SoundStarted = m_Communication.GetOnSoundEventStarted();
		m_SoundFinished = m_Communication.GetOnSoundEventFinished();
		m_VonCapture = m_Von.m_OnCaptureVON;
		m_VonReceive = m_Von.m_OnReceivedVON;
		if (!m_SoundStarted || !m_SoundFinished || !m_VonCapture || !m_VonReceive)
			return false;
		m_SoundStarted.Insert(OnSoundStarted);
		m_SoundFinished.Insert(OnSoundFinished);
		m_VonCapture.Insert(OnVonCapture);
		m_VonReceive.Insert(OnVonReceive);
		m_iSubscriptionGeneration++;
		return true;
	}

	protected void EnumerateRadios(IEntity entity, out array<BaseTransceiver> radios)
	{
		radios = {};
		if (!entity)
			return;
		SCR_GadgetManagerComponent manager = SCR_GadgetManagerComponent.GetGadgetManager(entity);
		if (!manager)
			return;
		AppendRadioType(manager, EGadgetType.RADIO, radios);
		AppendRadioType(manager, EGadgetType.RADIO_BACKPACK, radios);
	}

	protected void AppendRadioType(SCR_GadgetManagerComponent manager, EGadgetType type, array<BaseTransceiver> radios)
	{
		array<SCR_GadgetComponent> gadgets = manager.GetGadgetsByType(type);
		if (!gadgets)
			return;
		foreach (SCR_GadgetComponent gadget : gadgets)
		{
			if (!gadget || !gadget.GetOwner())
				continue;
			BaseRadioComponent radio = BaseRadioComponent.Cast(gadget.GetOwner().FindComponent(BaseRadioComponent));
			if (!radio)
				continue;
			for (int i = 0; i < radio.TransceiversCount(); i++)
			{
				BaseTransceiver transceiver = radio.GetTransceiver(i);
				if (transceiver)
					radios.Insert(transceiver);
			}
		}
	}

	protected bool SameRadios(array<BaseTransceiver> current)
	{
		if (!current || current.Count() != m_Radios.Count())
			return false;
		for (int i = 0; i < current.Count(); i++)
		{
			if (current[i] != m_Radios[i])
				return false;
		}
		return true;
	}

	protected int ReceiverIndex(BaseTransceiver receiver)
	{
		for (int i = 0; i < m_Radios.Count(); i++)
		{
			if (m_Radios[i] == receiver)
				return i;
		}
		return -1;
	}

	protected bool BindProxyA()
	{
		if (!IsV0() || m_iPlayerId != ROLE_B)
			return true;
		PlayerManager manager = GetGame().GetPlayerManager();
		IEntity source;
		if (manager)
			source = manager.GetPlayerControlledEntity(ROLE_A);
		SCR_CommunicationSoundComponent sound;
		if (source)
			sound = SCR_CommunicationSoundComponent.Cast(source.FindComponent(SCR_CommunicationSoundComponent));
		if (m_ProxyAStarted)
			return source == m_ProxyAEntity && sound == m_ProxyASound;
		if (!sound || !sound.IsScriptedMethodsCallEnabled())
		{
			if (!m_bProxyAUnavailableLogged)
				Emit("V1_PROXY_A_BINDING", "available=0,provenance=UNKNOWN,negativeDeliveryInference=0");
			m_bProxyAUnavailableLogged = true;
			return true;
		}
		m_ProxyAEntity = source;
		m_ProxyASound = sound;
		m_ProxyAStarted = sound.GetOnSoundEventStarted();
		if (!m_ProxyAStarted)
		{
			m_ProxyAEntity = null;
			m_ProxyASound = null;
			if (!m_bProxyAUnavailableLogged)
				Emit("V1_PROXY_A_BINDING", "available=0,invoker=0,provenance=UNKNOWN,negativeDeliveryInference=0");
			m_bProxyAUnavailableLogged = true;
			return true;
		}
		m_ProxyAStarted.Insert(OnProxyASoundStarted);
		Emit("V1_PROXY_A_BINDING", "sourcePlayer=1,localRole=2,scripted=1,sourceRpl="
			+ string.Format("%1", Replication.FindItemId(source)) + ",provenance=UNQUALIFIED");
		return true;
	}

	protected void OnProxyASoundStarted(SCR_CommunicationSoundComponent component, string eventName, AudioHandle handle, int priority)
	{
		bool valid = BindProxyA() && component == m_ProxyASound && CurrentContextValid(ROLE_B);
		string controlSnapshot;
		if (m_bWrongFrequency)
		{
			array<BaseTransceiver> current;
			EnumerateRadios(m_Entity, current);
			controlSnapshot = "," + V1ControlCatalogText(current, SameRadios(current));
		}
		Emit("V1_PROXY_A_SOUND_START", "sourcePlayer=1,event=" + RICON_LiveText.Esc(eventName)
			+ ",handle=" + string.Format("%1", handle) + ",priority=" + priority.ToString()
			+ ",bindingValid=" + Bit(valid) + ",controlRoutesClear=" + Bit(V1ControlRoutesClear(m_Radios))
			+ controlSnapshot + ",remoteDelivery=UNQUALIFIED");
		if (!valid)
			Invalidate("V1_PROXY_A_REBIND");
		else
			m_iProxyAStarted++;
	}

	protected bool V1ControlRoutesClear(array<BaseTransceiver> radios)
	{
		if (!m_bWrongFrequency) return true;
		if (!radios || radios.IsEmpty() || radios.Count() > 8) return false;
		array<int> frequencies = {};
		foreach (BaseTransceiver transceiver : radios)
		{
			if (!transceiver) return false;
			frequencies.Insert(transceiver.GetFrequency());
		}
		return RICON_D0Gate.V1ControlFrequenciesClear(true, frequencies);
	}

	protected bool SameV1ControlBCatalog(array<BaseTransceiver> current)
	{
		if (!m_bWrongFrequency) return true;
		if (!current || current.Count() != m_V1ControlBCatalog.Count()) return false;
		for (int i = 0; i < current.Count(); i++)
		{
			if (current[i] != m_V1ControlBCatalog[i]) return false;
		}
		return true;
	}

	protected bool V1ControlExtrasReady()
	{
		if (!m_bWrongFrequency) return true;
		if (m_V1ControlBCatalog.Count() != m_V1ControlBExtras.Count() + 1) return false;
		foreach (RICON_V1RadioAlignment endpoint : m_V1ControlBExtras)
		{
			if (!endpoint || !endpoint.Ready()) return false;
		}
		return true;
	}

	protected string V1ControlCatalogText(array<BaseTransceiver> radios, bool same)
	{
		int count;
		if (radios) count = radios.Count();
		bool noA48000 = count > 0 && count <= 8;
		string frequencies, indices;
		for (int i = 0; i < Math.Min(count, 8); i++)
		{
			if (i > 0) { frequencies = frequencies + "/"; indices = indices + "/"; }
			indices = indices + i.ToString();
			if (!radios[i]) { frequencies = frequencies + "UNKNOWN"; noA48000 = false; }
			else
			{
				int frequency = radios[i].GetFrequency();
				frequencies = frequencies + frequency.ToString();
				if (frequency == 48000) noA48000 = false;
			}
		}
		return "endpointCount=" + count.ToString() + ",catalogSame=" + Bit(same) + ",noA48000=" + Bit(noA48000)
			+ ",allExpected=" + Bit(V1ControlRoutesClear(radios)) + ",catalogIndices=" + indices
			+ ",frequencies=" + frequencies + ",sampledOnly=1";
	}

	protected bool EmitV1ControlSnapshot(string where)
	{
		if (!m_bWrongFrequency) return true;
		IEntity entity = m_Entity;
		if (m_iPlayerId == ROLE_A)
		{
			SCR_PlayerController remote = ResolveRemoteBController();
			entity = null;
			if (remote) entity = remote.GetControlledEntity();
		}
		array<BaseTransceiver> radios;
		EnumerateRadios(entity, radios);
		bool same = SameRadios(radios);
		if (m_iPlayerId == ROLE_A) same = SameV1ControlBCatalog(radios);
		bool reserved = where != "SAMPLED_WINDOW";
		return Emit("V1_CONTROL_CATALOG", "where=" + where + ",targetRole=2," + V1ControlCatalogText(radios, same), reserved);
	}

	protected bool V1RadioAligned(BaseTransceiver transceiver, int role)
	{
		if (!transceiver || transceiver.GetFrequency() != ExpectedV1Frequency(role))
			return false;
		BaseRadioComponent radio = transceiver.GetRadio();
		return radio && radio.IsPowered() && !transceiver.IsMuted() && radio.GetEncryptionKey() == V1_FIXTURE_KEY;
	}

	protected void EmitV1PairReadback(string where)
	{
		if (m_V1AlignmentA)
			Emit("V1_ENDPOINT_READBACK", "where=" + where + ",targetRole=1," + m_V1AlignmentA.Readback(), true);
		if (m_V1AlignmentB)
			Emit("V1_ENDPOINT_READBACK", "where=" + where + ",targetRole=2,catalogIndex=0," + m_V1AlignmentB.Readback(), true);
		for (int i = 0; i < m_V1ControlBExtras.Count(); i++)
			Emit("V1_ENDPOINT_READBACK", "where=" + where + ",targetRole=2,catalogIndex=" + (i + 1).ToString()
				+ "," + m_V1ControlBExtras[i].Readback(), true);
	}

	protected bool PrepareV1Pair(int now)
	{
		if (m_bV1PairPrepared)
			return true;
		if (!Replication.IsServer() || !CurrentContextValid(ROLE_A) || m_Radios.IsEmpty())
			return false;
		SCR_PlayerController remote = ResolveRemoteBController();
		PlayerManager manager = GetGame().GetPlayerManager();
		IEntity entityB;
		if (manager)
			entityB = manager.GetPlayerControlledEntity(ROLE_B);
		array<BaseTransceiver> radiosB;
		EnumerateRadios(entityB, radiosB);
		if (!remote || !entityB || remote.GetControlledEntity() != entityB || radiosB.IsEmpty())
			return false;
		if (m_bWrongFrequency && radiosB.Count() > 8)
		{
			Invalidate("CONTROL_INCONCLUSIVE_B_CATALOG_CAP");
			return false;
		}
		m_V1AlignmentA = new RICON_V1RadioAlignment();
		m_V1AlignmentB = new RICON_V1RadioAlignment();
		if (!m_V1AlignmentA.Capture(m_Entity, m_Radios[0], ExpectedV1Frequency(ROLE_A), V1_FIXTURE_KEY)
			|| !m_V1AlignmentB.Capture(entityB, radiosB[0], ExpectedV1Frequency(ROLE_B), V1_FIXTURE_KEY))
		{
			Invalidate("V1_PAIR_ILLEGAL_FREQUENCY");
			return false;
		}
		if (m_bWrongFrequency)
		{
			m_V1ControlBCatalog = radiosB;
			for (int i = 1; i < radiosB.Count(); i++)
			{
				RICON_V1RadioAlignment extra = new RICON_V1RadioAlignment();
				if (!extra.Capture(entityB, radiosB[i], ExpectedV1Frequency(ROLE_B), V1_FIXTURE_KEY))
				{
					Invalidate("CONTROL_INCONCLUSIVE_B_FREQUENCY_RANGE");
					return false;
				}
				m_V1ControlBExtras.Insert(extra);
			}
		}
		EmitV1PairReadback("BEFORE_SETUP");
		if (m_bInvalid)
			return false;
		m_V1BController = remote;
		m_bV1PairPrepared = true;
		m_iV1PairPreparedAt = now;
		m_bInNativeCall = true;
		m_V1AlignmentA.Apply();
		m_V1AlignmentB.Apply();
		foreach (RICON_V1RadioAlignment extraEndpoint : m_V1ControlBExtras) extraEndpoint.Apply();
		m_bInNativeCall = false;
		EmitV1PairReadback("AFTER_SETUP");
		bool logged = Emit("V1_SERVER_PAIR_SETUP", "writesOnce=1,frequencyA=48000,frequencyB=" + ExpectedV1Frequency(ROLE_B).ToString() + ",Areadback=" + Bit(m_V1AlignmentA.Ready())
			+ ",Breadback=" + Bit(m_V1AlignmentB.Ready()) + ",keyValuesLogged=0,settleMs=8000,sameFrameOnly=1", true);
		return logged && !m_bInvalid;
	}

	protected bool V1PairBindingValid()
	{
		if (!m_bV1PairPrepared || !m_V1AlignmentA || !m_V1AlignmentB || m_Radios.IsEmpty())
			return false;
		SCR_PlayerController remote = ResolveRemoteBController();
		IEntity entityB;
		if (remote)
			entityB = remote.GetControlledEntity();
		array<BaseTransceiver> radiosB;
		EnumerateRadios(entityB, radiosB);
		return remote == m_V1BController && !radiosB.IsEmpty() && SameV1ControlBCatalog(radiosB)
			&& m_V1AlignmentA.Matches(m_Entity, m_Radios[0]) && m_V1AlignmentB.Matches(entityB, radiosB[0]);
	}

	protected bool ServerBV1RadioAligned()
	{
		return ServerBIdentityValid() && V1PairBindingValid() && m_V1AlignmentA.Ready() && m_V1AlignmentB.Ready() && V1ControlExtrasReady();
	}

	protected void RestoreV1Radio()
	{
		if (!m_bV1PairPrepared)
			return;
		m_bV1PairPrepared = false;
		PlayerController local = GetGame().GetPlayerController();
		PlayerManager manager = GetGame().GetPlayerManager();
		for (int role = ROLE_A; role <= ROLE_B; role++)
		{
			PlayerController controller;
			if (manager)
				controller = manager.GetPlayerController(role);
			IEntity entity;
			bool expected = local == m_Controller && controller
				&& ((role == ROLE_A && controller == m_Controller) || (role == ROLE_B && controller == m_V1BController));
			if (expected)
				entity = controller.GetControlledEntity();
			array<BaseTransceiver> radios;
			EnumerateRadios(entity, radios);
			if (role == ROLE_B && !SameV1ControlBCatalog(radios)) entity = null;
			BaseTransceiver transceiver;
			if (!radios.IsEmpty())
				transceiver = radios[0];
			RICON_V1RadioAlignment endpoint = m_V1AlignmentA;
			if (role == ROLE_B)
				endpoint = m_V1AlignmentB;
			bool frequencyRestored, keyRestored;
			m_bInNativeCall = true;
			endpoint.Restore(entity, transceiver, frequencyRestored, keyRestored);
			m_bInNativeCall = false;
			if (!frequencyRestored || !keyRestored)
				m_bInvalid = true;
			Emit("V1_SERVER_RADIO_RESTORE", "targetRole=" + role.ToString() + ",catalogIndex=0,frequencyRestored=" + Bit(frequencyRestored)
				+ ",keyRestored=" + Bit(keyRestored) + ",keyValuesLogged=0,sameFrameOnly=1", true);
			if (role == ROLE_B)
			{
				for (int i = 0; i < m_V1ControlBExtras.Count(); i++)
				{
					BaseTransceiver extra;
					if (i + 1 < radios.Count()) extra = radios[i + 1];
					m_bInNativeCall = true;
					m_V1ControlBExtras[i].Restore(entity, extra, frequencyRestored, keyRestored);
					m_bInNativeCall = false;
					if (!frequencyRestored || !keyRestored) m_bInvalid = true;
					Emit("V1_SERVER_RADIO_RESTORE", "targetRole=2,catalogIndex=" + (i + 1).ToString()
						+ ",frequencyRestored=" + Bit(frequencyRestored) + ",keyRestored=" + Bit(keyRestored)
						+ ",keyValuesLogged=0,sameFrameOnly=1", true);
				}
			}
		}
	}

	protected bool SelectV1Transmit()
	{
		if (!CurrentContextValid(ROLE_A) || m_Radios.IsEmpty()
			|| !V1RadioAligned(m_Radios[0], m_iPlayerId) || !ServerBV1RadioAligned())
			return false;
		if (!EmitV1ControlSnapshot("PRECALL")) return false;
		if (!Emit("V1_PAIR_ALIGNMENT", "localA=1,serverB=1,frequencyA=48000,frequencyB=" + ExpectedV1Frequency(ROLE_B).ToString() + ",keyMatch=1,sameFrameOnly=1", true))
			return false;
		m_SelectedTransmit = m_Radios[0];
		BaseRadioComponent radio = m_SelectedTransmit.GetRadio();
		if (!radio || !radio.IsPowered() || m_SelectedTransmit.IsMuted())
			return false;
		m_PreviousMethod = m_Von.GetCommMethod();
		m_PreviousTransmit = m_Von.GetTransmitRadio();
		m_bPreviousTransmitPresent = m_PreviousTransmit != null;
		if (m_PreviousTransmit && ReceiverIndex(m_PreviousTransmit) < 0)
			return false;
		m_bTransmitRestorePending = true;
		m_bInNativeCall = true;
		m_Von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		m_Von.SetTransmitRadio(m_SelectedTransmit);
		m_bInNativeCall = false;
		bool selected = m_Von.GetCommMethod() == ECommMethod.SQUAD_RADIO
			&& m_Von.GetTransmitRadio() == m_SelectedTransmit;
		bool logged = Emit("V1_TX_SELECTION", "selected=" + Bit(selected) + ",txIndex=0,frequency="
			+ m_SelectedTransmit.GetFrequency().ToString() + ",radioOwnerRpl="
			+ string.Format("%1", Replication.FindItemId(radio.GetOwner()))
			+ ",captureRequested=0,pttRequested=0", true);
		return selected && logged && !m_bInvalid;
	}

	protected void RestoreV1Transmit()
	{
		if (!m_bTransmitRestorePending)
			return;
		m_bTransmitRestorePending = false;
		PlayerController controller = GetGame().GetPlayerController();
		array<BaseTransceiver> currentRadios;
		EnumerateRadios(m_Entity, currentRadios);
		bool same = SameRadios(currentRadios) && m_bPreviousTransmitPresent == (m_PreviousTransmit != null)
			&& controller == m_Controller && controller && controller.GetControlledEntity() == m_Entity
			&& m_Entity && m_Von && m_Entity.FindComponent(SCR_VoNComponent) == m_Von;
		if (!same || m_Von.GetCommMethod() != ECommMethod.SQUAD_RADIO
			|| m_Von.GetTransmitRadio() != m_SelectedTransmit)
		{
			m_bInvalid = true;
			Emit("V1_TX_RESTORE", "restored=0,reason=OWNERSHIP_OR_BINDING_CONFLICT", true);
			return;
		}
		// shortcut: isolated no-input fixture only; general writers need ownership coordination.
		m_bInNativeCall = true;
		m_Von.SetTransmitRadio(m_PreviousTransmit);
		m_Von.SetCommMethod(m_PreviousMethod);
		m_bInNativeCall = false;
		bool restored = m_Von.GetTransmitRadio() == m_PreviousTransmit && m_Von.GetCommMethod() == m_PreviousMethod;
		if (!restored)
			m_bInvalid = true;
		Emit("V1_TX_RESTORE", "restored=" + Bit(restored) + ",readback=1", true);
	}

	protected bool HasExactlyFixturePlayers()
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager)
			return false;
		array<int> players = {};
		manager.GetPlayers(players);
		return players.Count() == 2 && players.Contains(ROLE_A) && players.Contains(ROLE_B);
	}

	protected bool IsAliveA(IEntity entity)
	{
		if (m_iPlayerId != ROLE_A)
			return true;
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		return character && character.GetCharacterController()
			&& character.GetCharacterController().GetLifeState() == ECharacterLifeState.ALIVE;
	}

	//! Read-only revalidation used immediately at ACK, REQUEST and every callback; it never advances generations.
	protected bool CurrentContextValid(int expectedRole)
	{
		PlayerController controller = GetGame().GetPlayerController();
		IEntity entity;
		int playerId = -1;
		if (controller)
		{
			playerId = controller.GetPlayerId();
			entity = controller.GetControlledEntity();
		}
		SCR_CommunicationSoundComponent communication;
		SCR_VoNComponent von;
		if (entity)
		{
			communication = SCR_CommunicationSoundComponent.Cast(entity.FindComponent(SCR_CommunicationSoundComponent));
			von = SCR_VoNComponent.Cast(entity.FindComponent(SCR_VoNComponent));
		}
		array<BaseTransceiver> radios;
		EnumerateRadios(entity, radios);
		int now = Now();
		bool clockValid = RICON_D0Gate.Forward(m_iLastTick, now) && now - m_iLastTick <= HEARTBEAT_GAP_MS
			&& now - m_iT0 < RICON_D0Gate.OBSERVER_LIMIT_MS;
		bool scripted = communication && communication.IsScriptedMethodsCallEnabled();
		bool subscriptions = m_SoundStarted && m_SoundFinished && m_VonCapture && m_VonReceive;
		return HasExactlyFixturePlayers() && IsAliveA(entity)
			&& RICON_D0Gate.ValidCallbackContext(clockValid, scripted, controller == m_Controller, entity == m_Entity,
				communication == m_Communication, von == m_Von, SameRadios(radios), subscriptions,
				playerId, expectedRole, m_iEntityGeneration, m_iComponentGeneration, m_iSubscriptionGeneration);
	}

	protected bool RefreshBinding(int now)
	{
		PlayerController controller = GetGame().GetPlayerController();
		IEntity entity;
		int playerId = -1;
		if (controller)
		{
			playerId = controller.GetPlayerId();
			entity = controller.GetControlledEntity();
		}
		SCR_CommunicationSoundComponent communication;
		SCR_VoNComponent von;
		if (entity)
		{
			communication = SCR_CommunicationSoundComponent.Cast(entity.FindComponent(SCR_CommunicationSoundComponent));
			von = SCR_VoNComponent.Cast(entity.FindComponent(SCR_VoNComponent));
		}
		array<BaseTransceiver> radios;
		EnumerateRadios(entity, radios);
		bool entityChanged = controller != m_Controller || entity != m_Entity || playerId != m_iPlayerId;
		bool componentChanged = communication != m_Communication || von != m_Von || !SameRadios(radios);
		if (!entityChanged && !componentChanged)
			return m_SoundStarted && m_SoundFinished && m_VonCapture && m_VonReceive && BindProxyA();
		if (m_Phase != RICON_ED0Phase.WAIT_BINDING)
		{
			Invalidate("REBIND");
			return false;
		}
		Detach();
		m_Controller = controller;
		m_Entity = entity;
		m_iPlayerId = playerId;
		m_Communication = communication;
		m_Von = von;
		m_Radios = radios;
		if (entityChanged)
			m_iEntityGeneration++;
		if (componentChanged)
			m_iComponentGeneration++;
		m_iStableSince = now;
		bool attached = playerId >= ROLE_A && playerId <= ROLE_B && entity && communication && von && Attach();
		Emit("BINDING", "attached=" + Bit(attached) + ",radios=" + m_Radios.Count().ToString()
			+ ",scripted=" + Bit(communication && communication.IsScriptedMethodsCallEnabled()));
		return attached && BindProxyA();
	}

	protected void Invalidate(string reason)
	{
		m_bInvalid = true;
		Emit("INVALID", "reason=" + reason, true);
		if (m_bInNativeCall)
		{
			if (m_sDeferredStop.IsEmpty())
				m_sDeferredStop = reason;
			return;
		}
		Stop(reason);
	}

	protected void BeginBaseline(int now)
	{
		if (IsV0())
		{
			if (m_Radios.IsEmpty() || !V1RadioAligned(m_Radios[0], m_iPlayerId)
				|| (m_iPlayerId == ROLE_B && !V1ControlRoutesClear(m_Radios)))
			{
				if (m_iPlayerId == ROLE_A)
					Invalidate("V1_A_ALIGNMENT_CHANGED");
				return;
			}
			if (m_iPlayerId == ROLE_A && !V1PairBindingValid())
			{
				Invalidate("V1_PAIR_REBIND");
				return;
			}
			Emit("V1_RADIO_ALIGNMENT", "frequency=" + m_Radios[0].GetFrequency().ToString() + ",keyMatch=1,passive=" + Bit(m_iPlayerId == ROLE_B) + ",sameFrameOnly=1", true);
		}
		if (IsV0() && m_iPlayerId == ROLE_A && m_iBaselineSince < 0)
		{
			Invalidate("V1_QUIET_NOT_ARMED");
			return;
		}
		m_Phase = RICON_ED0Phase.QUIET_BASELINE;
		if (!IsV0() || m_iPlayerId != ROLE_A)
			m_iBaselineSince = now;
		string text = "commMethod=" + m_Von.GetCommMethod().ToString();
		text = text + ",txRadio=" + Bit(m_Von.GetTransmitRadio() != null);
		text = text + ",radios=" + m_Radios.Count().ToString();
		Emit("BASELINE_BEGIN", text);
	}

	protected SCR_PlayerController ResolveRemoteBController()
	{
		if (!Replication.IsServer() || m_iPlayerId != ROLE_A)
			return null;
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager)
			return null;
		SCR_PlayerController remote = SCR_PlayerController.Cast(manager.GetPlayerController(ROLE_B));
		if (!remote || remote.GetPlayerId() != ROLE_B)
			return null;
		return remote;
	}

	protected bool ServerBIdentityValid()
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager)
			return false;
		SCR_PlayerController controller = SCR_PlayerController.Cast(manager.GetPlayerController(ROLE_B));
		IEntity entity = manager.GetPlayerControlledEntity(ROLE_B);
		return controller && entity && controller.GetPlayerId() == ROLE_B && controller.GetControlledEntity() == entity
			&& controller == m_ServerBController && entity == m_ServerBEntity
			&& controller == m_FreshBController && entity == m_FreshBEntity;
	}

	protected bool SendChallenge(int now, bool initial)
	{
		if (IsV0() && !initial && !RICON_D0Gate.CanStartV1FreshChallenge(now, m_iT0))
		{
			Invalidate("A_DEADLINE_BEFORE_CHALLENGE");
			return false;
		}
		SCR_PlayerController remote = ResolveRemoteBController();
		if (!remote)
			return false;
		if (!initial)
		{
			PlayerManager manager = GetGame().GetPlayerManager();
			IEntity freshEntity;
			if (manager)
				freshEntity = manager.GetPlayerControlledEntity(ROLE_B);
			if (!freshEntity || remote.GetControlledEntity() != freshEntity)
				return false;
			m_FreshBController = remote;
			m_FreshBEntity = freshEntity;
		}
		m_iChallenge++;
		m_iChallengeAttempt = 1;
		m_iChallengeSent = now;
		m_iAckReceived = -1;
		if (initial)
			m_Phase = RICON_ED0Phase.WAIT_INITIAL_ACK;
		else
			m_Phase = RICON_ED0Phase.WAIT_ACK;
		Emit("CHALLENGE_SENT", "challenge=" + m_iChallenge.ToString() + ",attempt=1,initial=" + Bit(initial));
		return remote.RICON_D0SendReadinessChallenge(ControlKey(), m_iChallenge, m_iChallengeAttempt);
	}

	protected bool RetryChallenge(int now)
	{
		bool awaiting = m_Phase == RICON_ED0Phase.WAIT_INITIAL_ACK || m_Phase == RICON_ED0Phase.WAIT_ACK;
		if (!CanRetryCurrent(awaiting, m_iChallengeAttempt, now - m_iChallengeSent, now - m_iT0))
			return false;
		SCR_PlayerController remote = ResolveRemoteBController();
		if (!remote)
			return false;
		if (m_Phase == RICON_ED0Phase.WAIT_ACK)
		{
			PlayerManager manager = GetGame().GetPlayerManager();
			if (!manager || remote != m_FreshBController
				|| manager.GetPlayerControlledEntity(ROLE_B) != m_FreshBEntity)
				return false;
		}
		m_iChallengeAttempt++;
		m_iChallengeSent = now;
		m_iAckReceived = -1;
		Emit("CHALLENGE_RETRY", "challenge=" + m_iChallenge.ToString() + ",attempt=" + m_iChallengeAttempt.ToString());
		return remote.RICON_D0SendReadinessChallenge(ControlKey(), m_iChallenge, m_iChallengeAttempt);
	}

	bool BuildReadinessAck(string runId, int challengeId, int attempt,
		out int remainingMs, out int playerId,
		out int entityGeneration, out int componentGeneration, out int subscriptionGeneration)
	{
		remainingMs = -1;
		playerId = m_iPlayerId;
		entityGeneration = -1;
		componentGeneration = -1;
		subscriptionGeneration = -1;
		if (!m_bRunning || m_bInvalid || runId != ControlKey() || m_iPlayerId != ROLE_B
			|| !RICON_D0Gate.ValidChallengeSequence(challengeId, attempt, m_iLastChallengeHandled, m_iLastChallengeAttempt))
			return false;
		int now = Now();
		if (!RICON_D0Gate.Forward(m_iLastTick, now) || now - m_iLastTick > HEARTBEAT_GAP_MS
			|| !HasExactlyFixturePlayers() || !RefreshBinding(now) || !CurrentContextValid(ROLE_B))
			return false;
		if (IsV0() && challengeId == 1 && (m_Radios.IsEmpty() || !V1RadioAligned(m_Radios[0], m_iPlayerId)))
		{
			if (m_iV1PeerSettleAt < 0)
				m_iV1PeerSettleAt = now;
			if (now - m_iV1PeerSettleAt >= V1_SETTLE_MS)
				Invalidate("V1_PEER_ALIGNMENT_SETTLE_TIMEOUT");
			return false;
		}
		if (challengeId == 1 && m_Phase == RICON_ED0Phase.WAIT_BINDING && now - m_iStableSince >= STABLE_MS)
			BeginBaseline(now);
		if (challengeId == 1 && m_Phase != RICON_ED0Phase.QUIET_BASELINE && m_Phase != RICON_ED0Phase.READY)
			return false;
		if (challengeId == 2 && m_Phase != RICON_ED0Phase.READY
			&& m_Phase != RICON_ED0Phase.OBSERVE_REQUEST)
			return false;
		if (IsV0() && (m_Radios.IsEmpty() || !V1RadioAligned(m_Radios[0], m_iPlayerId)
			|| !V1ControlRoutesClear(m_Radios)))
			return false;
		remainingMs = RICON_D0Gate.OBSERVER_LIMIT_MS - (now - m_iT0);
		entityGeneration = m_iEntityGeneration;
		componentGeneration = m_iComponentGeneration;
		subscriptionGeneration = m_iSubscriptionGeneration;
		m_iLastChallengeHandled = challengeId;
		m_iLastChallengeAttempt = attempt;
		if (challengeId == 2)
			m_Phase = RICON_ED0Phase.OBSERVE_REQUEST;
		Emit("READINESS_ACK", "challenge=" + challengeId.ToString() + ",attempt=" + attempt.ToString()
			+ ",remaining=" + remainingMs.ToString());
		return remainingMs > 0;
	}

	void AcceptReadinessAck(SCR_PlayerController source, string runId,
		int challengeId, int attempt,
		int remainingMs, int playerId, int entityGeneration, int componentGeneration, int subscriptionGeneration)
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		IEntity serverBEntity;
		if (manager)
			serverBEntity = manager.GetPlayerControlledEntity(ROLE_B);
		if (!manager || !serverBEntity || source != manager.GetPlayerController(ROLE_B)
			|| source.GetPlayerId() != ROLE_B || source.GetControlledEntity() != serverBEntity)
			return;
		if (IsV0() && (source != m_V1BController || !V1PairBindingValid()))
		{
			Invalidate("V1_ACK_PAIR_REBIND");
			return;
		}
		bool initial = m_Phase == RICON_ED0Phase.WAIT_INITIAL_ACK && challengeId == 1;
		bool fresh = m_Phase == RICON_ED0Phase.WAIT_ACK && challengeId == 2;
		if (!m_bRunning || m_bInvalid || runId != ControlKey() || m_iPlayerId != ROLE_A
			|| challengeId != m_iChallenge || attempt != m_iChallengeAttempt
			|| playerId != ROLE_B || (!initial && !fresh))
			return;
		if (fresh && (source != m_ServerBController || serverBEntity != m_ServerBEntity
			|| source != m_FreshBController || serverBEntity != m_FreshBEntity))
		{
			Invalidate("SERVER_B_IDENTITY_CHANGED");
			return;
		}
		int received = Now();
		if (!RICON_D0Gate.Forward(m_iChallengeSent, received) || received - m_iChallengeSent > RICON_D0Gate.ACK_LIMIT_MS
			|| entityGeneration < 1 || componentGeneration < 1 || subscriptionGeneration < 1)
		{
			Invalidate("READINESS_ACK_INVALID");
			return;
		}
		m_iAckReceived = received;
		m_iAckRemainingB = remainingMs;
		m_iAckPlayerId = playerId;
		m_iAckEntityGeneration = entityGeneration;
		m_iAckComponentGeneration = componentGeneration;
		m_iAckSubscriptionGeneration = subscriptionGeneration;
		Emit("READINESS_RECEIVED", "challenge=" + challengeId.ToString() + ",attempt=" + attempt.ToString()
			+ ",remainingB=" + remainingMs.ToString() + ",initial=" + Bit(initial));
		if (initial)
		{
			m_ServerBController = source;
			m_ServerBEntity = serverBEntity;
			m_iInitialEntityGeneration = entityGeneration;
			m_iInitialComponentGeneration = componentGeneration;
			m_iInitialSubscriptionGeneration = subscriptionGeneration;
			BeginBaseline(m_iAckReceived);
		}
	}

	protected RICON_ED0RequestGate EvaluateRequestGate(bool requestUsed, int now)
	{
		if (IsV0())
			return RICON_D0Gate.CanRequestV0(requestUsed, now, m_iT0, m_iChallengeSent, m_iAckReceived,
				m_iAckRemainingB, m_iAckEntityGeneration, m_iAckComponentGeneration, m_iAckSubscriptionGeneration,
				m_iInitialEntityGeneration, m_iInitialComponentGeneration, m_iInitialSubscriptionGeneration);
		return RICON_D0Gate.CanRequest(requestUsed, now, m_iT0, m_iChallengeSent, m_iAckReceived,
			m_iAckRemainingB, m_iAckEntityGeneration, m_iAckComponentGeneration, m_iAckSubscriptionGeneration,
			m_iInitialEntityGeneration, m_iInitialComponentGeneration, m_iInitialSubscriptionGeneration);
	}

	protected void TryRequest(int now)
	{
		if (m_iAckReceived < 0)
		{
			if (now - m_iT0 > RequestDeadlineMs())
				Invalidate("ACK_TIMEOUT");
			else if (now - m_iChallengeSent >= 1500 && !RetryChallenge(now))
				Invalidate("ACK_RETRY_LIMIT");
			return;
		}
		RICON_ED0RequestGate gate = EvaluateRequestGate(m_bRequestUsed, now);
		if (gate != RICON_ED0RequestGate.OK || !CurrentContextValid(ROLE_A) || !ServerBIdentityValid())
		{
			Invalidate("REQUEST_PRECONDITION");
			return;
		}
		if (IsV0())
		{
			SignalsManagerComponent signals = SignalsManagerComponent.Cast(m_Entity.FindComponent(SignalsManagerComponent));
			if (!signals)
			{
				Invalidate("V1_SIGNALS_MISSING");
				return;
			}
			m_iSeedIndex = signals.FindSignal("Seed");
			m_V0Values = new array<float>();
			SCR_AISoundHandling.SetSignal(m_iSeedIndex, Math.RandomFloat(0, 1), m_V0Values);
		}
		bool valuesReady = !IsV0() || V0ValuesReady();
		if (!valuesReady)
		{
			Invalidate("V0_VALUES_PRECONDITION");
			return;
		}
		ECommMethod beforeMethod = m_Von.GetCommMethod();
		BaseTransceiver beforeTransmit = m_Von.GetTransmitRadio();
		m_bRequestUsed = true;
		m_Phase = RICON_ED0Phase.PRECALL;
		string requestText = "ignoreQueue=0,mode=" + m_Mode.ToString();
		if (IsV0())
			requestText = requestText + ",candidate=V1,event=SOUND_REPORTS_STATUS_CLEAR,priority=50,valuesNonNull=1,valuesCount=2,seedIndex=" + m_iSeedIndex.ToString();
		else
			requestText = requestText + ",event=SOUND_VOICE_PAIN_RELIEVE,priority=SOUND_PAIN_RELIEVE";
		requestText = requestText + ",commMethod=" + beforeMethod.ToString();
		requestText = requestText + ",txRadio=" + Bit(beforeTransmit != null);
		bool requestBeginReserved = Emit("REQUEST_BEGIN", requestText, true);
		if (!m_bRunning)
			return;
		bool currentContext = CurrentContextValid(ROLE_A);
		bool serverBIdentity = ServerBIdentityValid();
		valuesReady = !IsV0() || V0ValuesReady();
		int mutationNow = Now();
		RICON_ED0RequestGate finalGate = EvaluateRequestGate(false, mutationNow);
		bool allowMutation = RICON_D0Gate.CanMutateAfterClaim(m_bRequestUsed, requestBeginReserved, !m_bInvalid,
			finalGate == RICON_ED0RequestGate.OK, currentContext && valuesReady, serverBIdentity);
		if (!allowMutation)
		{
			Invalidate("PRECALL_VETO");
			return;
		}
		if (IsV0() && !V0ValuesReady())
		{
			Invalidate("V0_VALUES_FINAL_BOUNDARY");
			return;
		}
		currentContext = CurrentContextValid(ROLE_A);
		serverBIdentity = ServerBIdentityValid();
		mutationNow = Now();
		finalGate = EvaluateRequestGate(false, mutationNow);
		if (finalGate != RICON_ED0RequestGate.OK || !currentContext || !serverBIdentity || m_bInvalid)
		{
			Invalidate("FINAL_MUTATION_VETO");
			return;
		}
		if (IsV0() && !SelectV1Transmit())
		{
			Invalidate("V1_TX_SELECTION_FAILED");
			return;
		}
		m_iRequestTick = mutationNow;
		m_Phase = RICON_ED0Phase.OBSERVE_REQUEST;
		m_bInNativeCall = true;
		if (IsV0())
			m_Von.SoundEventPriority(SCR_SoundEvent.SOUND_REPORTS_STATUS_CLEAR, m_V0Values, 50, false);
		else
			m_Communication.SoundEventPriority(SCR_SoundEvent.SOUND_VOICE_PAIN_RELIEVE,
				SCR_ECommunicationSoundEventPriority.SOUND_PAIN_RELIEVE, false);
		int returned = Now();
		m_bInNativeCall = false;
		if (!m_Von)
		{
			Invalidate("VON_LOST_DURING_CALL");
			return;
		}
		ECommMethod afterMethod = m_Von.GetCommMethod();
		BaseTransceiver afterTransmit = m_Von.GetTransmitRadio();
		string returnText = "durationMs=" + (returned - mutationNow).ToString();
		returnText = returnText + ",commMethodBefore=" + beforeMethod.ToString();
		returnText = returnText + ",commMethodAfter=" + afterMethod.ToString();
		returnText = returnText + ",txBefore=" + Bit(beforeTransmit != null) + ",txAfter=" + Bit(afterTransmit != null);
		bool requestEndLogged = Emit("REQUEST_END", returnText, true);
		if (!requestEndLogged || m_bInvalid)
		{
			if (!m_sDeferredStop.IsEmpty())
				Stop(m_sDeferredStop);
			else
				Invalidate("RETURN_LOG_OR_CALLBACK_INVALID");
			return;
		}
		if (!RICON_D0Gate.Forward(mutationNow, returned) || returned - mutationNow > RICON_D0Gate.CALL_BUDGET_MS)
		{
			Invalidate("CALL_BUDGET");
			return;
		}
		if (IsV0() && (afterMethod != ECommMethod.SQUAD_RADIO || afterTransmit != m_SelectedTransmit))
		{
			Invalidate("V0_READBACK_CHANGED");
			return;
		}
		if (IsV0() && !RICON_D0Gate.CanObserveV0AfterReturn(returned, m_iT0, m_iChallengeSent,
			m_iAckReceived, m_iAckRemainingB))
		{
			Invalidate("V0_POST_RETURN_REMAINING");
			return;
		}
		if (!m_bRunning)
			return;
		if (!m_sDeferredStop.IsEmpty())
		{
			Stop(m_sDeferredStop);
			return;
		}
		if (IsV0())
		{
			m_iPostUntil = returned + 20000;
			m_Phase = RICON_ED0Phase.POST_OBSERVE;
			if (!Emit("V0_CALL_RETURNED", "candidate=V1,observeMs=20000,drain=UNPROVEN", true))
				Invalidate("V0_RETURN_LOG_BUDGET");
		}
	}

	void Tick(BaseWorld world, float timeSlice)
	{
		if (!m_bRunning)
			return;
		int now = Now();
		if (m_bInvalid)
		{
			Stop("INVALID_STATE");
			return;
		}
		if (!world || world != m_World || world.IsEditMode())
		{
			Stop("WORLD_CHANGED");
			return;
		}
		if (!RICON_D0Gate.Forward(m_iLastTick, now))
		{
			Invalidate("CLOCK");
			return;
		}
		if (now - m_iLastTick > HEARTBEAT_GAP_MS
			&& !RICON_D0Gate.AllowV1StartupGap(IsV0(), m_Phase == RICON_ED0Phase.WAIT_BINDING,
				m_iEntityGeneration, m_iComponentGeneration, m_iSubscriptionGeneration, m_bV1PairPrepared || m_bTransmitRestorePending))
		{
			Invalidate("HEARTBEAT_GAP");
			return;
		}
		m_iLastTick = now;
		if (now - m_iT0 >= RICON_D0Gate.OBSERVER_LIMIT_MS)
		{
			Stop("LIMIT");
			return;
		}
		if (!HasExactlyFixturePlayers())
		{
			if (m_Phase != RICON_ED0Phase.WAIT_BINDING)
				Invalidate("FIXTURE_PLAYERS");
			return;
		}
		if (!RefreshBinding(now))
			return;
		if (!IsAliveA(m_Entity))
		{
			Invalidate("BLOCKED_ALIVE");
			return;
		}
		if (IsV0() && m_iPlayerId == ROLE_B && m_iV1PeerSettleAt >= 0
			&& m_Phase == RICON_ED0Phase.WAIT_BINDING && now - m_iV1PeerSettleAt >= V1_SETTLE_MS
			&& (m_Radios.IsEmpty() || !V1RadioAligned(m_Radios[0], m_iPlayerId)))
		{
			Invalidate("V1_PEER_ALIGNMENT_SETTLE_TIMEOUT");
			return;
		}
		if (m_bWrongFrequency && m_iPlayerId == ROLE_A && m_bV1PairPrepared
			&& m_Phase >= RICON_ED0Phase.WAIT_INITIAL_ACK && m_Phase <= RICON_ED0Phase.POST_OBSERVE
			&& (!V1PairBindingValid() || !m_V1AlignmentA.Ready() || !m_V1AlignmentB.Ready() || !V1ControlExtrasReady()))
		{
			EmitV1ControlSnapshot("UNEXPECTED_CONTROL_CHANGE");
			Invalidate("CONTROL_INCONCLUSIVE_CONFIGURATION_CHANGED");
			return;
		}
		if (now - m_iLastHeartbeat >= HEARTBEAT_MS)
		{
			m_iLastHeartbeat = now;
			EmitV1ControlSnapshot("SAMPLED_WINDOW");
			Emit("HEARTBEAT", "remaining=" + (RICON_D0Gate.OBSERVER_LIMIT_MS - (now - m_iT0)).ToString());
		}
		if (m_Phase == RICON_ED0Phase.WAIT_BINDING && now - m_iStableSince >= STABLE_MS)
		{
			if (m_iPlayerId == ROLE_A)
			{
				if (IsV0())
				{
					if (now - m_iT0 > RequestDeadlineMs())
					{
						Invalidate("V1_PAIR_SETUP_DEADLINE");
						return;
					}
					if (!PrepareV1Pair(now))
						return;
					if (!V1PairBindingValid())
					{
						Invalidate("V1_PAIR_REBIND");
						return;
					}
					if (!m_V1AlignmentA.Ready() || !m_V1AlignmentB.Ready() || !V1ControlExtrasReady())
					{
						if (now - m_iV1PairPreparedAt >= V1_SETTLE_MS)
						{
							EmitV1PairReadback("SETTLE_TIMEOUT");
							Invalidate("V1_SERVER_ALIGNMENT_SETTLE_TIMEOUT");
						}
						return;
					}
				}
				if (IsV0() && m_iBaselineSince < 0)
				{
					int quietNow = Now();
					if (!RICON_D0Gate.Forward(now, quietNow) || !CurrentContextValid(ROLE_A) || !V1PairBindingValid())
					{
						Invalidate("V1_QUIET_ARM_CONTEXT");
						return;
					}
					m_iBaselineSince = quietNow;
					if (!Emit("V1_QUIET_ARMED", "quietSince=" + quietNow.ToString() + ",quietMs=6000,initialAck=0,globalT0Unchanged=1", true))
					{
						Invalidate("V1_QUIET_ARM_LOG_BUDGET");
						return;
					}
				}
				if (!SendChallenge(now, true))
					Invalidate("INITIAL_CHALLENGE_SEND");
			}
			else
			{
				if (IsV0() && !m_bV1PeerReadbackLogged && !m_Radios.IsEmpty())
				{
					m_bV1PeerReadbackLogged = true;
					Emit("V1_ENDPOINT_READBACK", "where=PASSIVE_LOCAL_B,targetRole=2,"
						+ RICON_V1RadioAlignment.Describe(m_Entity, m_Radios[0], ExpectedV1Frequency(m_iPlayerId), V1_FIXTURE_KEY), true);
				}
				BeginBaseline(now);
			}
		}
		else if (m_Phase == RICON_ED0Phase.WAIT_INITIAL_ACK)
		{
			if (now - m_iT0 > RequestDeadlineMs())
				Invalidate("INITIAL_ACK_TIMEOUT");
			else if (now - m_iChallengeSent >= 1500 && !RetryChallenge(now))
				Invalidate("INITIAL_ACK_RETRY_LIMIT");
		}
		else if (m_Phase == RICON_ED0Phase.QUIET_BASELINE && now - m_iBaselineSince >= QUIET_MS)
		{
			if (IsV0() && m_iPlayerId == ROLE_A && !RICON_D0Gate.V1QuietComplete(m_iAckReceived >= 0,
				CurrentContextValid(ROLE_A) && V1PairBindingValid(), m_iBaselineSince, now, QUIET_MS))
			{
				Invalidate("V1_QUIET_BARRIER");
				return;
			}
			m_Phase = RICON_ED0Phase.READY;
			Emit("READY", "quietMs=" + QUIET_MS.ToString());
			if (m_iPlayerId == ROLE_A && !SendChallenge(now, false) && m_bRunning)
				Invalidate("FRESH_CHALLENGE_SEND");
		}
		else if (m_Phase == RICON_ED0Phase.WAIT_ACK)
			TryRequest(now);
		else if (m_iPlayerId == ROLE_A && m_Phase == RICON_ED0Phase.OBSERVE_REQUEST && m_bRequestUsed
			&& now - m_iRequestTick >= TERMINAL_WAIT_MS)
			Invalidate("INCONCLUSIVE_OUTSTANDING_EVENT");
		else if (m_iPlayerId == ROLE_A && m_Phase == RICON_ED0Phase.POST_OBSERVE
			&& RICON_D0Gate.PostWindowComplete(now, m_iPostUntil))
		{
			string postKind = "POST_OBSERVATION_END";
			string postReason = "POST_OBSERVATION_COMPLETE";
			if (IsV0())
			{
				postKind = "V0_OBSERVATION_END";
				postReason = "V0_OBSERVATION_COMPLETE";
			}
			if (!Emit(postKind, "drain=UNPROVEN", true))
			{
				Invalidate("POST_LOG_BUDGET");
				return;
			}
			Stop(postReason);
		}
	}

	protected void OnSoundStarted(SCR_CommunicationSoundComponent component, string eventName, AudioHandle handle, int priority)
	{
		string text = "event=" + RICON_LiveText.Esc(eventName) + ",handle=" + string.Format("%1", handle);
		text = text + ",priority=" + priority.ToString() + ",componentMatch=" + Bit(component == m_Communication);
		Emit("SOUND_START", text);
		if (!CurrentContextValid(m_iPlayerId))
		{
			Invalidate("CALLBACK_CONTEXT");
			return;
		}
		if (component != m_Communication)
		{
			Invalidate("FOREIGN_COMPONENT");
			return;
		}
		if (IsV0())
		{
			if (m_Phase != RICON_ED0Phase.OBSERVE_REQUEST && m_Phase != RICON_ED0Phase.POST_OBSERVE)
				Invalidate("FOREIGN_ACTIVITY");
			else
				m_iStarted++;
			return;
		}
		if (m_Phase != RICON_ED0Phase.OBSERVE_REQUEST)
		{
			Invalidate("FOREIGN_ACTIVITY");
			return;
		}
		if (eventName != SCR_SoundEvent.SOUND_VOICE_PAIN_RELIEVE
			|| priority != SCR_ECommunicationSoundEventPriority.SOUND_PAIN_RELIEVE || m_iStarted != 0)
		{
			Invalidate("FOREIGN_OR_DUPLICATE_START");
			return;
		}
		m_RequestHandle = handle;
		m_iStarted++;
	}

	protected void OnSoundFinished(SCR_CommunicationSoundComponent component, string eventName, AudioHandle handle,
		int priority, bool terminated)
	{
		string text = "event=" + RICON_LiveText.Esc(eventName) + ",handle=" + string.Format("%1", handle);
		text = text + ",priority=" + priority.ToString() + ",terminated=" + Bit(terminated);
		text = text + ",componentMatch=" + Bit(component == m_Communication);
		Emit("SOUND_FINISH", text);
		if (!CurrentContextValid(m_iPlayerId))
		{
			Invalidate("CALLBACK_CONTEXT");
			return;
		}
		if (component != m_Communication)
		{
			Invalidate("FOREIGN_FINISH");
			return;
		}
		if (IsV0())
		{
			if (m_Phase != RICON_ED0Phase.OBSERVE_REQUEST && m_Phase != RICON_ED0Phase.POST_OBSERVE)
				Invalidate("FOREIGN_FINISH");
			else
				m_iFinished++;
			return;
		}
		if (m_Phase != RICON_ED0Phase.OBSERVE_REQUEST)
		{
			Invalidate("FOREIGN_FINISH");
			return;
		}
		if (eventName != SCR_SoundEvent.SOUND_VOICE_PAIN_RELIEVE
			|| priority != SCR_ECommunicationSoundEventPriority.SOUND_PAIN_RELIEVE
			|| m_iStarted != 1 || m_iFinished != 0 || handle != m_RequestHandle)
		{
			Invalidate("FOREIGN_OR_DUPLICATE_FINISH");
			return;
		}
		m_iFinished++;
		if (m_iPlayerId == ROLE_A && m_bRequestUsed && m_iStarted == 1 && m_iFinished == 1)
		{
			int postStart = Now();
			if (!RICON_D0Gate.Forward(m_iLastTick, postStart))
			{
				Invalidate("POST_CLOCK");
				return;
			}
			m_iPostUntil = postStart + 10000;
			m_Phase = RICON_ED0Phase.POST_OBSERVE;
			if (!Emit("POST_OBSERVATION_BEGIN", "durationMs=10000,drain=UNPROVEN", true))
				Invalidate("POST_LOG_BUDGET");
		}
	}

	protected void OnVonCapture(BaseTransceiver transmitter)
	{
		Emit("VON_CAPTURE", "radio=" + Bit(transmitter != null));
		if (!CurrentContextValid(m_iPlayerId))
		{
			Invalidate("CALLBACK_CONTEXT");
			return;
		}
		if (IsV0() && (m_Phase == RICON_ED0Phase.OBSERVE_REQUEST || m_Phase == RICON_ED0Phase.POST_OBSERVE))
			Emit("UNCLASSIFIED_CAPTURE", "invalid=1", true);
		Invalidate("UNEXPECTED_CAPTURE");
	}

	protected void OnVonReceive(int senderId, BaseTransceiver receiver, int frequency, float quality)
	{
		int rx = ReceiverIndex(receiver);
		string text = "sender=" + senderId.ToString() + ",receiver=" + rx.ToString();
		text = text + ",radio=" + Bit(receiver != null) + ",frequency=" + frequency.ToString();
		text = text + ",quality=" + quality.ToString(-1, 9);
		Emit("VON_RECEIVE", text);
		if (!CurrentContextValid(m_iPlayerId))
		{
			Invalidate("CALLBACK_CONTEXT");
			return;
		}
		if (IsV0())
		{
			bool observing = m_Phase == RICON_ED0Phase.OBSERVE_REQUEST || m_Phase == RICON_ED0Phase.POST_OBSERVE;
			RICON_EV0ReceiveClass route = RICON_D0Gate.ClassifyV0Receive(observing, m_iPlayerId,
				senderId, receiver != null, rx);
			if (route == RICON_EV0ReceiveClass.INVALID)
			{
				Invalidate("FOREIGN_OR_UNBOUND_RX");
				return;
			}
			m_iVonReceive++;
			if (route == RICON_EV0ReceiveClass.DIRECT_UNQUALIFIED)
			{
				m_iVonDirect++;
				Emit("RAW_DIRECT_ROUTE_UNQUALIFIED", "sender=1,receiver=null");
				return;
			}
			m_iVonRadio++;
			Emit("RAW_RADIO_ROUTE_UNQUALIFIED", "sender=1,receiver=" + rx.ToString());
			return;
		}
		if (!RICON_D0Gate.ValidRadioReceive(m_Phase == RICON_ED0Phase.OBSERVE_REQUEST,
			m_iPlayerId, ROLE_B, senderId, ROLE_A, receiver != null, rx))
			Invalidate("DIRECT_OR_FOREIGN_RX");
	}

	void Stop(string reason)
	{
		if (!m_bRunning)
			return;
		EmitV1ControlSnapshot("TERMINAL_BEFORE_RESTORE");
		RestoreV1Transmit();
		RestoreV1Radio();
		if (!IsV0() && m_iStarted != m_iFinished)
			m_bInvalid = true;
		if (IsV0())
		{
			string verdict = "BLOCKED_NO_REQUEST";
			if (m_bInvalid)
				verdict = "INVALID";
			else if (m_iStarted + m_iFinished + m_iVonReceive > 0)
				verdict = "RAW_V0_OBSERVABLE";
			else if (m_bRequestUsed || m_iLastChallengeHandled >= 2)
				verdict = "INCONCLUSIVE_INPUT_OR_SINK";
			string verdictText = "candidate=V1,transport=UNPROVEN,verdict=" + verdict + ",reason=" + reason;
			verdictText = verdictText + ",soundStart=" + m_iStarted.ToString();
			verdictText = verdictText + ",soundFinish=" + m_iFinished.ToString();
			verdictText = verdictText + ",rx=" + m_iVonReceive.ToString();
			verdictText = verdictText + ",direct=" + m_iVonDirect.ToString();
			verdictText = verdictText + ",radio=" + m_iVonRadio.ToString() + ",proxyAStarted=" + m_iProxyAStarted.ToString();
			Emit("V0_VERDICT", verdictText, true);
		}
		Detach();
		m_Phase = RICON_ED0Phase.STOPPED;
		string terminalText = "reason=" + reason + ",mode=" + m_Mode.ToString();
		terminalText = terminalText + ",requestUsed=" + Bit(m_bRequestUsed);
		terminalText = terminalText + ",started=" + m_iStarted.ToString();
		terminalText = terminalText + ",finished=" + m_iFinished.ToString();
		terminalText = terminalText + ",rx=" + m_iVonReceive.ToString();
		terminalText = terminalText + ",invalid=" + Bit(m_bInvalid);
		terminalText = terminalText + ",subscriptions=0,ordinary=" + m_Budget.Ordinary().ToString();
		terminalText = terminalText + ",reserved=" + m_Budget.Reserved().ToString();
		Emit("TERMINAL", terminalText, true);
		m_bRunning = false;
		m_World = null;
		m_Controller = null;
		m_Entity = null;
		m_ServerBController = null;
		m_ServerBEntity = null;
		m_FreshBController = null;
		m_FreshBEntity = null;
		m_V0Values = null;
		m_V1AlignmentA = null;
		m_V1AlignmentB = null;
		m_V1ControlBExtras.Clear();
		m_V1ControlBCatalog.Clear();
		m_V1BController = null;
		m_PreviousTransmit = null;
		m_SelectedTransmit = null;
		m_Communication = null;
		m_Von = null;
		m_Radios.Clear();
		if (s_Active == this)
			s_Active = null;
	}
}
