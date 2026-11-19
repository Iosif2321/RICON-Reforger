//! Local radio identity/watch only. No native VON activation or capture calls belong here.
class RICON_GameRadioInputNativeSelection : RICON_GameRadioInputSelection
{
	SCR_PlayerController m_Controller;
	IEntity m_Entity, m_RadioOwner, m_Mode;
	BaseRadioComponent m_Radio;
	BaseTransceiver m_Transceiver;
	RplComponent m_Replication;
	RplId m_Id;
	protected bool m_bClosed, m_bChanged;
	protected ref ScriptInvoker<BaseTransceiver, int, int> m_FrequencyEvent;
	protected ref ScriptInvoker<bool> m_PowerEvent;
	protected ref ScriptInvoker<string> m_KeyEvent;

	bool Bind(SCR_PlayerController controller, BaseRadioComponent radio, BaseTransceiver transceiver, int index)
	{
		if (m_Controller || m_bClosed || !controller || !radio || !transceiver || index < 0) return false;
		if (!GetGame() || !GetGame().GetGameMode()) return false;
		IEntity owner = radio.GetOwner();
		if (!owner || !controller.GetControlledEntity() || index >= radio.TransceiversCount()) return false;
		RplComponent replication = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!replication || !replication.Id().IsValid() || radio.GetTransceiver(index) != transceiver || transceiver.GetRadio() != radio) return false;
		if (!radio.m_OnTansceiverFrequencyChangedInvoker || !radio.m_OnPowerChanged || !radio.m_OnEncryptionKeyChanged) return false;
		m_Controller = controller; m_Entity = controller.GetControlledEntity(); m_RadioOwner = owner; m_Mode = GetGame().GetGameMode();
		m_Radio = radio; m_Transceiver = transceiver; m_iIndex = index; m_Replication = replication; m_Id = replication.Id();
		m_FrequencyEvent = radio.m_OnTansceiverFrequencyChangedInvoker; m_PowerEvent = radio.m_OnPowerChanged; m_KeyEvent = radio.m_OnEncryptionKeyChanged;
		m_FrequencyEvent.Insert(OnFrequency); m_PowerEvent.Insert(OnPower); m_KeyEvent.Insert(OnKey);
		return Current();
	}
	protected void OnFrequency(BaseTransceiver transceiver, int oldFrequency, int newFrequency)
	{
		if (!m_bClosed && oldFrequency != newFrequency) m_bChanged = true;
	}
	protected void OnPower(bool powered) { if (!m_bClosed) m_bChanged = true; }
	protected void OnKey(string ignored) { if (!m_bClosed) m_bChanged = true; }
	bool Current()
	{
		if (m_bClosed || m_bChanged || !GetGame() || !m_Controller || !m_RadioOwner || !m_Radio || !m_Transceiver) return false;
		if (!m_Replication) return false;
		if (!m_Mode || GetGame().GetGameMode() != m_Mode) return false;
		if (m_Controller.GetControlledEntity() != m_Entity || m_RadioOwner.IsDeleted() || m_RadioOwner.GetWorld() != GetGame().GetWorld()) return false;
		if (m_Radio.GetOwner() != m_RadioOwner || m_iIndex >= m_Radio.TransceiversCount() || m_Radio.GetTransceiver(m_iIndex) != m_Transceiver) return false;
		if (m_Transceiver.GetRadio() != m_Radio || !m_Radio.IsPowered() || m_RadioOwner.FindComponent(RplComponent) != m_Replication) return false;
		if (m_Replication.Id() != m_Id || Replication.FindItem(m_Id) != m_Replication) return false;
		return m_Radio.m_OnTansceiverFrequencyChangedInvoker == m_FrequencyEvent
			&& m_Radio.m_OnPowerChanged == m_PowerEvent && m_Radio.m_OnEncryptionKeyChanged == m_KeyEvent;
	}
	override void Close()
	{
		if (m_bClosed) return;
		m_bClosed = true;
		if (m_FrequencyEvent) m_FrequencyEvent.Remove(OnFrequency);
		if (m_PowerEvent) m_PowerEvent.Remove(OnPower);
		if (m_KeyEvent) m_KeyEvent.Remove(OnKey);
		m_FrequencyEvent = null; m_PowerEvent = null; m_KeyEvent = null;
	}
}

class RICON_GameRadioInputNativeSource : RICON_GameRadioInputSource
{
	protected SCR_VONController m_Hook;
	protected SCR_PlayerController m_Controller;
	protected IEntity m_ScopeEntity;
	protected BaseWorld m_ScopeWorld;
	protected int m_iHeldStartRequest, m_iHeldStartHold, m_iHeldStartDevice, m_iHeldStartIndex = -1;
	void RICON_GameRadioInputNativeSource(SCR_VONController hook, SCR_PlayerController controller) { m_Hook = hook; m_Controller = controller; }
	bool ControllerCurrent()
	{
		if (!GetGame() || !m_Controller || GetGame().GetPlayerController() != m_Controller || !m_Controller.GetControlledEntity()) return false;
		if (Replication.IsServer() || !Replication.IsClient() || RplSession.Mode() != RplMode.Client) return false;
		RplId item = Replication.FindItemId(m_Controller);
		if (!item.IsValid()) return false;
		RplNode node = Replication.FindNode(item);
		return node && node.IsOwner();
	}
	override bool IsCurrent() { return ControllerCurrent() && m_Hook && m_Hook.RICON_InputReady(); }
	override int NowMs() { return System.GetTickCount(); }
	override bool SelectionCurrent(RICON_GameRadioInputSelection selection)
	{
		RICON_GameRadioInputNativeSelection held = RICON_GameRadioInputNativeSelection.Cast(selection);
		return IsCurrent() && held && held.m_Controller == m_Controller && held.Current() && m_Hook.RICON_MatchesInputSelection(held);
	}
	override bool ContextCurrent(RICON_GameRadioPttContext context)
	{
		if (!ControllerCurrent() || !context) return false;
		RICON_GameRadioPttContext current = m_Controller.RICON_ReadRadioPttContext();
		return current && current.Same(context);
	}
	override bool ScopeLost(RICON_GameRadioPttContext context)
	{
		if (!ControllerCurrent() || !GetGame()) return true;
		if (m_ScopeEntity && m_Controller.GetControlledEntity() != m_ScopeEntity) return true;
		if (m_ScopeWorld && GetGame().GetWorld() != m_ScopeWorld) return true;
		RICON_GameRadioPttContext current = m_Controller.RICON_ReadRadioPttContext();
		if (!current) return false;
		return !context || !current.Same(context);
	}
	//! The input owner qualifies time and the live selection before using held START evidence.
	RICON_GameRadioPttReply ReadStartEvidence()
	{
		if (!ControllerCurrent()) return null;
		if (!SupportsRenewalV3()) return m_Controller.RICON_ReadLatestRadioPttReply(RICON_GameRadioPttReply.PTT);
		ref RICON_GameRadioPttReply reply = m_Controller.RICON_ReadHeldRadioPttStart();
		if (!RICON_GameRadioInputOwner.ValidPttMetadata(reply)) return null;
		if (reply.m_iRequestId != m_iHeldStartRequest || reply.m_iHoldToken != m_iHeldStartHold) return null;
		if (reply.m_iDeviceId != m_iHeldStartDevice || reply.m_iIndex != m_iHeldStartIndex) return null;
		return reply;
	}

	override bool LeaseCurrent(RICON_GameRadioPttContext context, int holdToken, int leaseId)
	{
		if (!ContextCurrent(context)) return false;
		ref RICON_GameRadioPttReply reply = ReadStartEvidence();
		return reply && reply.m_bActive && reply.m_iHoldToken == holdToken && reply.m_iLeaseId == leaseId
			&& reply.m_Context && reply.m_Context.Same(context);
	}
	override bool RequestContext(int requestId)
	{
		if (!IsCurrent()) return false;
		m_ScopeEntity = m_Controller.GetControlledEntity(); m_ScopeWorld = GetGame().GetWorld();
		return m_Controller.RICON_RequestRadioContext(requestId);
	}
	override bool RequestRadio(int requestId, RICON_GameRadioPttContext context, RICON_GameRadioInputSelection selection)
	{
		RICON_GameRadioInputNativeSelection held = RICON_GameRadioInputNativeSelection.Cast(selection);
		if (!ContextCurrent(context) || !SelectionCurrent(held)) return false;
		return m_Controller.RICON_RequestRadioControl(requestId, held.m_Id, held.m_iIndex);
	}
	override bool RequestStart(int requestId, int holdToken, int requestSeq, RICON_GameRadioPttContext context,
		RICON_GameRadioInputSelection selection, int deviceHint)
	{
		RICON_GameRadioInputNativeSelection held = RICON_GameRadioInputNativeSelection.Cast(selection);
		if (!ContextCurrent(context) || !SelectionCurrent(held)) return false;
		m_iHeldStartRequest = requestId;
		m_iHeldStartHold = holdToken;
		m_iHeldStartDevice = deviceHint;
		m_iHeldStartIndex = held.m_iIndex;
		return m_Controller.RICON_RequestRadioPtt(requestId, holdToken, RICON_ELifecycleMethod.PTT_START,
			deviceHint, held.m_Id, held.m_iIndex, 0, requestSeq);
	}
	override bool RequestStop(int requestId, int holdToken, int requestSeq, RICON_GameRadioPttContext context, int leaseId)
	{
		if (!ContextCurrent(context)) return false;
		return m_Controller.RICON_RequestRadioPtt(requestId, holdToken, RICON_ELifecycleMethod.PTT_STOP,
			0, RplId.Invalid(), -1, leaseId, requestSeq);
	}
	override bool SupportsRenewalV3() { return true; }
	override bool RequestRenew(int requestId, int holdToken, int leaseId, int requestSeq, RICON_GameRadioPttContext expected)
	{
		if (!IsCurrent() || !ContextCurrent(expected)) return false;
		return m_Controller.RICON_RequestRadioRenew(requestId, holdToken, leaseId, requestSeq, expected);
	}
	override bool SupportsCancelV2() { return true; }
	override bool RequestCancel(int requestId, int holdToken, int requestSeq, int cancelStartSeq, RICON_GameRadioPttContext context)
	{
		if (!ContextCurrent(context)) return false;
		return m_Controller.RICON_RequestRadioCancel(requestId, holdToken, requestSeq, cancelStartSeq, context);
	}
	override void Release(int holdToken)
	{
		if (holdToken == m_iHeldStartHold)
		{
			m_iHeldStartRequest = 0;
			m_iHeldStartHold = 0;
			m_iHeldStartDevice = 0;
			m_iHeldStartIndex = -1;
		}
		if (m_Controller) m_Controller.RICON_ReleaseRadioPttHold(holdToken);
	}
}

modded class SCR_VONController
{
	protected ref RICON_GameRadioInputOwner m_RICON_Input;
	protected ref RICON_GameRadioInputNativeSource m_RICON_InputSource;
	protected ref ScriptInvoker m_RICON_InputReplies, m_RICON_InputCancelReplies, m_RICON_InputRenewReplies;
	protected ref OnControlledEntityChangedPlayerControllerInvoker m_RICON_InputEntityChanges;
	protected IEntity m_RICON_InputMode;
	protected BaseWorld m_RICON_InputWorld;
	protected bool m_bRICON_InputConfigured, m_bRICON_OwnedDown, m_bRICON_InputPolling;

	bool RICON_InputConfigured()
	{
		if (!GetGame()) return m_bRICON_InputConfigured;
		BaseWorld world = GetGame().GetWorld();
		IEntity mode = GetGame().GetGameMode();
		if (world && world != m_RICON_InputWorld)
		{
			m_RICON_InputWorld = world; m_RICON_InputMode = mode; m_bRICON_InputConfigured = false;
		}
		if (mode && mode != m_RICON_InputMode) { m_RICON_InputMode = mode; m_bRICON_InputConfigured = false; }
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(RICON_CommsManagerComponent.GetInstance());
		if (manager && mode && manager.GetOwner() == mode) m_bRICON_InputConfigured = manager.IsGameRadioInputConfigured();
		return m_bRICON_InputConfigured;
	}
	bool RICON_InputReady()
	{
		if (!GetGame() || !GetGame().GetWorld()) return false;
		IEntity mode = GetGame().GetGameMode();
		if (!mode) return false;
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(RICON_CommsManagerComponent.GetInstance());
		return manager && manager.GetOwner() == mode && manager.IsGameRadioInputConfigured();
	}
	protected bool RICON_ReadInputSelection(out BaseRadioComponent radio, out BaseTransceiver transceiver, out int index)
	{
		radio = null; transceiver = null; index = -1;
		if (m_bIsDisabled || m_bIsPauseDisabled || m_bIsUnconscious || m_eLifeState != ECharacterLifeState.ALIVE) return false;
		EVONTransmitType type = EVONTransmitType.CHANNEL;
		if (m_bIsLongRangeToggled) type = EVONTransmitType.LONG_RANGE;
		SCR_VONEntry entry = GetEntryByTransmitType(type);
		if (!m_ActiveEntry)
		{
			foreach (SCR_VONEntry candidate : m_aEntries)
			{
				SCR_VONEntryRadio available = SCR_VONEntryRadio.Cast(candidate);
				if (available && !available.GetIsMuted()) { entry = available; break; }
			}
		}
		SCR_VONEntryRadio selected = SCR_VONEntryRadio.Cast(entry);
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(RICON_CommsManagerComponent.GetInstance());
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetOwner());
		if (manager && manager.IsGameMixedEquipmentConfigured() && controller)
		{
			BaseCompartmentSlot seat;
			BaseRadioComponent mounted = RICON_GameRadioEquipment.ReadDriverRadio(controller.GetControlledEntity(), seat);
			if (mounted)
			{
				if (!mounted.IsPowered() || mounted.TransceiversCount() <= 0) return false;
				int ordinal;
				if (type == EVONTransmitType.LONG_RANGE) ordinal = 1;
				int fallback = RICON_GameRadioEquipment.RadioSlotIndex(mounted, ordinal);
				if (fallback < 0) fallback = RICON_GameRadioEquipment.RadioSlotIndex(mounted, 0);
				if (fallback < 0) return false;
				BaseTransceiver mountedTransceiver = mounted.GetTransceiver(fallback);
				if (selected && !selected.GetIsMuted() && selected.IsUsable())
				{
					BaseTransceiver selectedTransceiver = selected.GetTransceiver();
					if (RadioTransceiver.Cast(selectedTransceiver) && selectedTransceiver.GetRadio() == mounted) mountedTransceiver = selectedTransceiver;
				}
				if (!mountedTransceiver || mountedTransceiver.IsMuted() || mountedTransceiver.GetRadio() != mounted) return false;
				for (int mountedIndex = 0; mountedIndex < mounted.TransceiversCount(); mountedIndex++)
				{
					if (mounted.GetTransceiver(mountedIndex) != mountedTransceiver) continue;
					radio = mounted; transceiver = mountedTransceiver; index = mountedIndex;
					return true;
				}
				return false;
			}
		}
		if (!selected || selected.GetIsMuted() || !selected.IsUsable()) return false;
		transceiver = selected.GetTransceiver();
		if (!transceiver) return false;
		radio = transceiver.GetRadio();
		if (!radio) return false;
		for (int item = 0; item < radio.TransceiversCount(); item++)
			if (radio.GetTransceiver(item) == transceiver) { index = item; return true; }
		return false;
	}
	bool RICON_MatchesInputSelection(RICON_GameRadioInputNativeSelection held)
	{
		BaseRadioComponent radio; BaseTransceiver transceiver; int index;
		return held && RICON_ReadInputSelection(radio, transceiver, index)
			&& held.m_Radio == radio && held.m_Transceiver == transceiver && held.m_iIndex == index;
	}
	protected RICON_GameRadioInputSelection RICON_CaptureInputSelection(SCR_PlayerController controller)
	{
		BaseRadioComponent radio; BaseTransceiver transceiver; int index;
		if (!RICON_ReadInputSelection(radio, transceiver, index)) return null;
		RICON_GameRadioInputNativeSelection held = new RICON_GameRadioInputNativeSelection();
		if (!held.Bind(controller, radio, transceiver, index)) { held.Close(); return null; }
		return held;
	}
	protected void RICON_DetachInputCallbacks()
	{
		if (m_RICON_InputReplies) m_RICON_InputReplies.Remove(RICON_OnInputReply);
		if (m_RICON_InputCancelReplies) m_RICON_InputCancelReplies.Remove(RICON_OnInputCancelReply);
		if (m_RICON_InputEntityChanges) m_RICON_InputEntityChanges.Remove(RICON_OnInputEntityChanged);
		if (m_RICON_InputRenewReplies) m_RICON_InputRenewReplies.Remove(RICON_OnInputRenewReply);
		m_RICON_InputReplies = null; m_RICON_InputCancelReplies = null; m_RICON_InputRenewReplies = null; m_RICON_InputEntityChanges = null;
		if (GetGame()) GetGame().GetCallqueue().Remove(RICON_PollInput);
		m_bRICON_InputPolling = false;
	}
	protected bool RICON_EnsureInput()
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetOwner());
		if (!controller || !GetGame() || GetGame().GetPlayerController() != controller) return false;
		if (!m_RICON_Input)
		{
			m_RICON_InputSource = new RICON_GameRadioInputNativeSource(this, controller);
			m_RICON_Input = new RICON_GameRadioInputOwner();
			if (!m_RICON_Input.Configure(m_RICON_InputSource)) return false;
		}
		if (!m_RICON_InputReplies)
		{
			m_RICON_InputReplies = controller.RICON_GetRadioPttReplyInvoker();
			if (!m_RICON_InputReplies) return false;
			m_RICON_InputReplies.Insert(RICON_OnInputReply);
		}
		if (!m_RICON_InputCancelReplies)
		{
			m_RICON_InputCancelReplies = controller.RICON_GetRadioCancelReplyInvoker();
			if (!m_RICON_InputCancelReplies) return false;
			m_RICON_InputCancelReplies.Insert(RICON_OnInputCancelReply);
		}
		if (!m_RICON_InputRenewReplies && m_RICON_InputSource.SupportsRenewalV3())
		{
			m_RICON_InputRenewReplies = controller.RICON_GetRadioRenewReplyInvoker();
			if (!m_RICON_InputRenewReplies) return false;
			m_RICON_InputRenewReplies.Insert(RICON_OnInputRenewReply);
		}
		if (!m_RICON_InputEntityChanges)
		{
			m_RICON_InputEntityChanges = controller.m_OnControlledEntityChanged;
			if (m_RICON_InputEntityChanges) m_RICON_InputEntityChanges.Insert(RICON_OnInputEntityChanged);
		}
		return true;
	}
	protected void RICON_SyncInputPolling()
	{
		bool needed = m_RICON_Input && m_RICON_Input.NeedsTick();
		if (!GetGame()) return;
		if (needed && !m_bRICON_InputPolling)
		{
			m_bRICON_InputPolling = true; GetGame().GetCallqueue().CallLater(RICON_PollInput, 50, true);
		}
		if (!needed && m_bRICON_InputPolling)
		{
			GetGame().GetCallqueue().Remove(RICON_PollInput); m_bRICON_InputPolling = false;
		}
	}
	protected void RICON_OnInputReply(RICON_GameRadioPttReply reply)
	{
		if (m_RICON_Input) m_RICON_Input.OnReply(reply);
		RICON_SyncInputPolling();
	}
	protected void RICON_OnInputCancelReply(RICON_GameRadioCancelReply reply)
	{
		if (m_RICON_Input) m_RICON_Input.OnCancelReply(reply);
		RICON_SyncInputPolling();
	}
	protected void RICON_OnInputRenewReply(RICON_GameRadioRenewReply reply)
	{
		if (m_RICON_Input) m_RICON_Input.OnRenewReply(reply);
		RICON_SyncInputPolling();
	}

	protected void RICON_OnInputEntityChanged(IEntity from, IEntity to)
	{
		if (from == to) return;
		if (m_RICON_Input) m_RICON_Input.LoseScope();
		RICON_DetachInputCallbacks();
	}
	protected void RICON_PollInput()
	{
		if (!m_RICON_Input || !m_RICON_InputSource || !m_RICON_InputSource.ControllerCurrent())
		{
			if (m_RICON_Input) m_RICON_Input.LoseScope();
			RICON_DetachInputCallbacks(); return;
		}
		m_RICON_Input.Tick(); RICON_SyncInputPolling();
		if (!RICON_InputConfigured() && !m_RICON_Input.OwnsInput()) RICON_DetachInputCallbacks();
	}
	//! A detached local START receipt and its bounded remaining hold time.
	RICON_GameRadioPttReply RICON_ReadPttMetadata(out int remainingMs)
	{
		remainingMs = 0;
		ref RICON_GameRadioInputOwner input = m_RICON_Input;
		ref RICON_GameRadioInputNativeSource source = m_RICON_InputSource;
		if (!input || !source || !m_bRICON_OwnedDown || !source.IsCurrent()) return null;
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetOwner());
		if (!controller || controller != GetGame().GetPlayerController() || controller.FindComponent(SCR_VONController) != this) return null;
		if (!input.ActiveControl()) return null;
		ref RICON_GameRadioPttReply reply = source.ReadStartEvidence();
		if (!reply || !reply.m_Context || source.ScopeLost(reply.m_Context)) return null;
		int budget = input.ReadPttRemainingMs(reply);
		if (budget <= 0 || input != m_RICON_Input || source != m_RICON_InputSource || !m_bRICON_OwnedDown) return null;
		remainingMs = budget;
		return reply;
	}

	string RICON_RadioInputStatus()
	{
		if (!m_RICON_Input) return "IDLE";
		return m_RICON_Input.Status();
	}

	void RICON_AppendDevInput(array<string> lines)
	{
		lines.Insert(string.Format("PTT %1  held=%2  stock VON active=%3  long=%4",
			RICON_RadioInputStatus(), m_bRICON_OwnedDown, m_bIsActive, m_bIsLongRangeToggled));
		BaseRadioComponent radio;
		BaseTransceiver transceiver;
		int index;
		if (!RICON_ReadInputSelection(radio, transceiver, index))
		{
			lines.Insert("Selected radio: unavailable");
			lines.Insert("Native tuning: unavailable");
			return;
		}
		string kind = typename.EnumToString(RICON_EGameRadioEquipment, RICON_GameRadioEquipment.ReadType(radio));
		lines.Insert(string.Format("Selected %1  slot=%2/%3  %4 kHz  power=%5  mute=%6", kind,
			index, radio.TransceiversCount(), transceiver.GetFrequency(), radio.IsPowered(), transceiver.IsMuted()));
		lines.Insert(string.Format("Native tuning %1..%2 kHz  step=%3 kHz  nominal range=%4 m",
			transceiver.GetMinFrequency(), transceiver.GetMaxFrequency(), transceiver.GetFrequencyResolution(), transceiver.GetRange()));
	}

	override protected void ActionVONBroadcast(float value, EActionTrigger reason = EActionTrigger.UP)
	{
		bool configured = RICON_InputConfigured();
		bool hasOwnHold = m_bRICON_OwnedDown || (m_RICON_Input && m_RICON_Input.OwnsInput());
		if (!RICON_GameRadioInputOwner.ShouldConsume(configured, hasOwnHold))
		{
			RICON_DetachInputCallbacks(); super.ActionVONBroadcast(value, reason); return;
		}
		if (reason != EActionTrigger.UP) m_bRICON_OwnedDown = true;
		if (reason == EActionTrigger.UP) m_bRICON_OwnedDown = false;
		if (!RICON_EnsureInput()) return;
		if (reason == EActionTrigger.UP) m_RICON_Input.Up();
		else if (reason != EActionTrigger.UP)
		{
			if (m_RICON_Input.OwnsInput()) m_RICON_Input.Down(null);
			else m_RICON_Input.Down(RICON_CaptureInputSelection(SCR_PlayerController.Cast(GetOwner())));
		}
		RICON_SyncInputPolling();
	}
	override protected void SetVONBroadcast(bool activate, EVONTransmitType transmitType = EVONTransmitType.CHANNEL)
	{
		bool hasOwnHold = m_bRICON_OwnedDown || (m_RICON_Input && m_RICON_Input.OwnsInput());
		if (RICON_GameRadioInputOwner.ShouldConsume(RICON_InputConfigured(), hasOwnHold))
		{
			if (!activate && m_RICON_Input) m_RICON_Input.Up();
			RICON_SyncInputPolling(); return;
		}
		super.SetVONBroadcast(activate, transmitType);
	}
	override void OnDelete(IEntity owner)
	{
		if (m_RICON_Input) m_RICON_Input.Close();
		RICON_DetachInputCallbacks(); super.OnDelete(owner);
	}
}
