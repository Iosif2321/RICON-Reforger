//! Held by the bridge so a late notification carries the exact subscription that received it.
class RICON_GameControllerBinding
{
	RICON_GameControllerLifecycle m_Bridge;
	SCR_PlayerController m_Controller;
	RplIdentity m_Owner;
	ref OnControlledEntityChangedPlayerControllerInvoker m_ControlChanged;
	IEntity m_Entity;
	int m_iPlayerId, m_iConnectionId, m_iGeneration, m_iEntityId;

	void Attach(RICON_GameControllerLifecycle bridge, OnControlledEntityChangedPlayerControllerInvoker changed)
	{
		m_Bridge = bridge;
		m_ControlChanged = changed;
		m_ControlChanged.Insert(OnControlledEntityChanged);
	}

	void Detach()
	{
		m_Bridge = null;
		if (m_ControlChanged) m_ControlChanged.Remove(OnControlledEntityChanged);
		m_ControlChanged = null;
	}

	protected void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		// Terminal failure can detach this entry while the invoker is dispatching it.
		ref RICON_GameControllerBinding binding = this;
		if (m_Bridge) m_Bridge.OnControlChanged(binding, from, to);
	}
}

//! An opt-in server adapter for one exact manager, Lifecycle, world and epoch.
class RICON_GameControllerLifecycle
{
	protected static int s_iConnectionSerial, s_iEntitySerial;
	protected RICON_CommsManagerComponent m_Manager;
	protected RICON_LifecycleStateMachine m_Lifecycle;
	protected BaseWorld m_World;
	protected ref map<int, ref RICON_GameControllerBinding> m_Bindings = new map<int, ref RICON_GameControllerBinding>();
	protected int m_iEpoch, m_iProcessAnchorMs, m_iLastRawMs, m_iMaxPlayers;
	protected bool m_bStandaloneLocal, m_bHalted;

	bool Configure(RICON_CommsManagerComponent manager, RICON_LifecycleStateMachine lifecycle,
		int processAnchorMs, int maxPlayers, bool allowStandaloneLocal = false)
	{
		if (m_bHalted || m_iEpoch || !manager || !lifecycle || !GetGame()
			|| processAnchorMs < 0 || maxPlayers <= 0 || !RICON_CommsManagerComponent.IsAuthority()) return false;
		if (RICON_CommsManagerComponent.GetInstance() != manager || lifecycle.GetPlayerCount() != 0) return false;
		int epoch = manager.GetSessionEpoch();
		if (epoch <= 0 || lifecycle.GetSessionEpoch() != epoch) return false;
		BaseWorld world = GetGame().GetWorld();
		IEntity owner = manager.GetOwner();
		if (!world || world.IsEditMode() || !owner || owner.GetWorld() != world) return false;
		int rawMs = System.GetTickCount();
		if (rawMs < processAnchorMs || rawMs == int.MAX || !lifecycle.CanQueryAt(rawMs - processAnchorMs)) return false;
		m_Manager = manager;
		m_Lifecycle = lifecycle;
		m_World = world;
		m_iEpoch = epoch;
		m_iProcessAnchorMs = processAnchorMs;
		m_iLastRawMs = rawMs;
		m_iMaxPlayers = maxPlayers;
		m_bStandaloneLocal = allowStandaloneLocal;
		return true;
	}

	bool MatchesLifecycle(RICON_LifecycleStateMachine lifecycle)
	{
		return lifecycle && lifecycle == m_Lifecycle && IsCurrent();
	}

	//! Only the trusted registration hook or bootstrap may pass registered=true.
	bool Reconcile(int playerId, bool registered = false)
	{
		if (!IsCurrent() || playerId <= 0) return false;
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players) return false;
		SCR_PlayerController controller = SCR_PlayerController.Cast(players.GetPlayerController(playerId));
		RplIdentity owner;
		if (!ReadOwner(playerId, controller, owner)) return false;
		OnControlledEntityChangedPlayerControllerInvoker changed = controller.m_OnControlledEntityChanged;
		if (!changed) return false;
		int nowMs = ReadNowMs();
		if (nowMs < 0) return false;

		RICON_GameControllerBinding binding = m_Bindings.Get(playerId);
		if (!binding || binding.m_Controller != controller || binding.m_Owner != owner)
		{
			if (!binding && m_Bindings.Count() >= m_iMaxPlayers) return false;
			binding = Connect(playerId, controller, owner, changed, nowMs);
			if (!binding) return false;
		}
		if (binding.m_ControlChanged != changed || !LifecycleBindingCurrent(binding)) return false;
		if (m_Lifecycle.GetPlayerState(playerId) == RICON_EPlayerLifecycleState.CONNECTED_UNREGISTERED)
		{
			if (!registered) return true;
			if (!Consume(m_Lifecycle.Registered(nowMs, playerId, binding.m_iGeneration))) return false;
		}

		return SyncNativeEntity(binding, nowMs);
	}

	//! The pump can refresh or retire held identities. It cannot admit a replacement controller.
	bool ReconcileKnownAt(int nowMs)
	{
		if (!IsCurrent() || nowMs < 0) return false;
		int currentMs = ReadNowMs();
		if (currentMs < nowMs || !m_Lifecycle.CanQueryAt(nowMs)) return false;
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players) return false;
		bool allCurrent = true;
		for (int index = m_Bindings.Count() - 1; index >= 0; index--)
		{
			RICON_GameControllerBinding binding = m_Bindings.GetElement(index);
			PlayerController nativeController = players.GetPlayerController(binding.m_iPlayerId);
			if (!nativeController && !players.IsPlayerConnected(binding.m_iPlayerId))
			{
				if (!RemoveBinding(binding, nowMs)) allCurrent = false;
			}
			else if (!BindingMatchesController(binding, nativeController))
			{
				allCurrent = false;
			}
			else if (m_Lifecycle.GetPlayerState(binding.m_iPlayerId) != RICON_EPlayerLifecycleState.CONNECTED_UNREGISTERED)
			{
				if (!SyncNativeEntity(binding, nowMs)) allCurrent = false;
			}
			if (m_bHalted) return false;
		}
		return allCurrent;
	}

	bool AllBindingsCurrent()
	{
		if (!IsCurrent()) return false;
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players) return false;
		foreach (int playerId, RICON_GameControllerBinding binding : m_Bindings)
		{
			if (!BindingMatchesController(binding, players.GetPlayerController(playerId))) return false;
			if (m_Lifecycle.GetPlayerState(playerId) == RICON_EPlayerLifecycleState.CONNECTED_UNREGISTERED) continue;
			IEntity entity;
			if (!ReadNativeEntity(binding.m_Controller, entity) || entity != binding.m_Entity) return false;
			if (!entity && binding.m_iEntityId) return false;
		}
		return true;
	}

	void OnControlChanged(RICON_GameControllerBinding binding, IEntity from, IEntity to)
	{
		if (!binding || CurrentBinding(binding.m_iPlayerId) != binding) return;
		if (from != binding.m_Entity || from == to) return;
		if (m_Lifecycle.GetPlayerState(binding.m_iPlayerId) == RICON_EPlayerLifecycleState.CONNECTED_UNREGISTERED) return;
		if (binding.m_Controller.GetControlledEntity() != to) return;
		if (to && (to.IsDeleted() || to.GetWorld() != m_World)) return;
		int nowMs = ReadNowMs();
		if (nowMs >= 0) SyncEntity(binding, to, nowMs);
	}

	//! The numeric callback is only a hint. A mapped controller can still be current or a replacement.
	void OnDisconnected(int playerId)
	{
		if (!IsCurrent() || playerId <= 0) return;
		RICON_GameControllerBinding binding = m_Bindings.Get(playerId);
		if (!binding) return;
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players) return;
		if (players.GetPlayerController(playerId) || players.IsPlayerConnected(playerId)) return;
		int nowMs = ReadNowMs();
		if (nowMs >= 0) RemoveBinding(binding, nowMs);
	}

	int GetConnectionId(int playerId)
	{
		RICON_GameControllerBinding binding = CurrentBinding(playerId);
		if (!binding) return 0;
		return binding.m_iConnectionId;
	}

	int GetEntityId(int playerId)
	{
		RICON_GameControllerBinding binding = CurrentBoundPlayer(playerId);
		if (!binding) return 0;
		return binding.m_iEntityId;
	}

	bool ReadBoundPlayer(int playerId, out int connectionId, out int generation, out int entityId)
	{
		connectionId = 0;
		generation = 0;
		entityId = 0;
		RICON_GameControllerBinding binding = CurrentBoundPlayer(playerId);
		if (!binding) return false;
		connectionId = binding.m_iConnectionId;
		generation = binding.m_iGeneration;
		entityId = binding.m_iEntityId;
		return true;
	}

	//! Recheck the addressed incarnation immediately before dispatch.
	SCR_PlayerController ResolveRemoteRecipient(int playerId, int connectionId, int generation, int entityId)
	{
		RplMode mode = RplSession.Mode();
		if (mode != RplMode.Dedicated && mode != RplMode.Listen) return null;
		RICON_GameControllerBinding binding = CurrentBoundPlayer(playerId);
		if (!binding || binding.m_Owner == RplIdentity.Local()) return null;
		if (binding.m_iConnectionId != connectionId || binding.m_iGeneration != generation || binding.m_iEntityId != entityId)
			return null;
		return binding.m_Controller;
	}

	//! Текущая серверная привязка контроллера, через который пришёл RPC.
	bool ReadRemoteSender(SCR_PlayerController sender, out int connectionId, out int generation, out int entityId)
	{
		connectionId = 0;
		generation = 0;
		entityId = 0;
		RplMode mode = RplSession.Mode();
		if (!sender || (mode != RplMode.Dedicated && mode != RplMode.Listen)) return false;
		RICON_GameControllerBinding binding = CurrentBoundPlayer(sender.GetPlayerId());
		if (!binding || binding.m_Controller != sender || binding.m_Owner == RplIdentity.Local()) return false;
		connectionId = binding.m_iConnectionId;
		generation = binding.m_iGeneration;
		entityId = binding.m_iEntityId;
		return true;
	}

	//! Same authority timebase as reconciliation and the managed RADIO tick.
	int ReadWireAuthorityTime(RICON_LifecycleStateMachine lifecycle)
	{
		if (!MatchesLifecycle(lifecycle)) return -1;
		return ReadNowMs();
	}

	void Close()
	{
		m_bHalted = true;
		DetachBindings();
		m_Lifecycle = null;
		m_Manager = null;
		m_World = null;
	}

	protected bool IsCurrent()
	{
		if (m_bHalted || !m_iEpoch || !m_World || !m_Manager || !m_Lifecycle || !GetGame()) return false;
		if (!RICON_CommsManagerComponent.IsAuthority() || GetGame().GetWorld() != m_World || m_World.IsEditMode()) return false;
		if (RICON_CommsManagerComponent.GetInstance() != m_Manager || m_Manager.GetSessionEpoch() != m_iEpoch) return false;
		return m_Lifecycle.GetSessionEpoch() == m_iEpoch;
	}

	protected int ReadNowMs()
	{
		int rawMs = System.GetTickCount();
		if (rawMs < m_iLastRawMs || rawMs == int.MAX)
		{
			Halt();
			return -1;
		}
		m_iLastRawMs = rawMs;
		int nowMs = rawMs - m_iProcessAnchorMs;
		if (!m_Lifecycle.CanQueryAt(nowMs)) return -1;
		return nowMs;
	}

	protected bool ReadOwner(int playerId, SCR_PlayerController controller, out RplIdentity owner)
	{
		if (!controller || controller.GetPlayerId() != playerId) return false;
		owner = controller.GetRplIdentity();
		if (!owner.IsValid()) return false;
		if (RplSession.Mode() == RplMode.None)
		{
			if (!m_bStandaloneLocal || Replication.IsClient() || owner != RplIdentity.Local()) return false;
			return GetGame().GetPlayerController() == controller && SCR_PlayerController.GetLocalPlayerId() == playerId;
		}
		if (owner == RplIdentity.Local()) return false;
		RplId item = Replication.FindItemId(controller);
		return item.IsValid() && Replication.FindOwner(item) == owner;
	}

	protected RICON_GameControllerBinding CurrentBinding(int playerId)
	{
		if (!IsCurrent() || playerId <= 0) return null;
		RICON_GameControllerBinding binding = m_Bindings.Get(playerId);
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players || !BindingMatchesController(binding, players.GetPlayerController(playerId))) return null;
		return binding;
	}

	protected RICON_GameControllerBinding CurrentBoundPlayer(int playerId)
	{
		RICON_GameControllerBinding binding = CurrentBinding(playerId);
		if (!binding || m_Lifecycle.GetPlayerState(playerId) != RICON_EPlayerLifecycleState.BOUND_ACTIVE) return null;
		IEntity entity;
		if (!ReadNativeEntity(binding.m_Controller, entity) || !entity || entity != binding.m_Entity) return null;
		return binding;
	}

	protected bool BindingMatchesController(RICON_GameControllerBinding binding, PlayerController nativeController)
	{
		if (!binding || !nativeController || nativeController != binding.m_Controller) return false;
		if (!LifecycleBindingCurrent(binding) || binding.m_Controller.m_OnControlledEntityChanged != binding.m_ControlChanged)
			return false;
		RplIdentity owner;
		return ReadOwner(binding.m_iPlayerId, binding.m_Controller, owner) && binding.m_Owner == owner;
	}

	protected bool LifecycleBindingCurrent(RICON_GameControllerBinding binding)
	{
		int playerId = binding.m_iPlayerId;
		return m_Lifecycle.GetPlayerConnectionId(playerId) == binding.m_iConnectionId
			&& m_Lifecycle.GetPlayerGeneration(playerId) == binding.m_iGeneration
			&& m_Lifecycle.GetPlayerEntityId(playerId) == binding.m_iEntityId;
	}

	protected RICON_GameControllerBinding Connect(int playerId, SCR_PlayerController controller, RplIdentity owner,
		OnControlledEntityChangedPlayerControllerInvoker changed, int nowMs)
	{
		if (s_iConnectionSerial >= int.MAX) return null;
		s_iConnectionSerial++;
		int connection = s_iConnectionSerial;
		if (!Consume(m_Lifecycle.Connected(nowMs, playerId, connection))) return null;

		RICON_GameControllerBinding previous = m_Bindings.Get(playerId);
		if (previous) previous.Detach();
		RICON_GameControllerBinding binding = new RICON_GameControllerBinding();
		binding.m_iPlayerId = playerId;
		binding.m_iConnectionId = connection;
		binding.m_iGeneration = m_Lifecycle.GetPlayerGeneration(playerId);
		binding.m_Controller = controller;
		binding.m_Owner = owner;
		binding.Attach(this, changed);
		m_Bindings.Set(playerId, binding);
		return binding;
	}

	protected bool RemoveBinding(RICON_GameControllerBinding binding, int nowMs)
	{
		if (!LifecycleBindingCurrent(binding)) return false;
		int playerId = binding.m_iPlayerId;
		if (!Consume(m_Lifecycle.Disconnected(nowMs, playerId, binding.m_iGeneration, binding.m_iConnectionId))) return false;
		binding.Detach();
		m_Bindings.Remove(playerId);
		return true;
	}

	protected bool ReadNativeEntity(SCR_PlayerController controller, out IEntity entity)
	{
		entity = controller.GetControlledEntity();
		if (entity && entity.GetWorld() != m_World) return false;
		if (entity && entity.IsDeleted()) entity = null;
		return true;
	}

	protected bool SyncNativeEntity(RICON_GameControllerBinding binding, int nowMs)
	{
		IEntity entity;
		return ReadNativeEntity(binding.m_Controller, entity) && SyncEntity(binding, entity, nowMs);
	}

	protected bool SyncEntity(RICON_GameControllerBinding binding, IEntity entity, int nowMs)
	{
		if (entity == binding.m_Entity && (entity || !binding.m_iEntityId)) return true;
		int playerId = binding.m_iPlayerId;
		int generation = binding.m_iGeneration;
		int oldId = binding.m_iEntityId;
		int newId;
		ref RICON_LifecycleResult result;
		if (entity)
		{
			if (s_iEntitySerial >= int.MAX) return false;
			s_iEntitySerial++;
			newId = s_iEntitySerial;
			if (oldId) result = m_Lifecycle.EntityChanged(nowMs, playerId, generation, oldId, newId);
			else result = m_Lifecycle.EntityBound(nowMs, playerId, generation, newId);
		}
		else result = m_Lifecycle.EntityUnbound(nowMs, playerId, generation, oldId, RICON_EEntityUnbindReason.CONTROL_LOST);
		if (!Consume(result)) return false;
		binding.m_Entity = entity;
		binding.m_iEntityId = newId;
		return true;
	}

	protected bool Consume(RICON_LifecycleResult result)
	{
		RICON_TemporalCaptureResult accepted = m_Manager.ConsumeTemporalLifecycleResult(result, m_iEpoch);
		if (!accepted || accepted.m_sCode != "OK")
		{
			Halt();
			return false;
		}
		return result && result.m_eResultCode == RICON_ELifecycleResult.OK;
	}

	protected void Halt()
	{
		m_bHalted = true;
		m_Manager.StopOpenRadioLoop();
		DetachBindings();
	}

	protected void DetachBindings()
	{
		foreach (int playerId, RICON_GameControllerBinding binding : m_Bindings) binding.Detach();
		m_Bindings.Clear();
	}
}
