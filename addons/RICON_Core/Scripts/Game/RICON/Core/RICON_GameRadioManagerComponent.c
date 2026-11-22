class RICON_GameRadioManagerComponentClass : RICON_CommsManagerComponentClass {}

//! Explicit scenario profile for sampled, open-channel RF before media.
class RICON_GameOpenRadioRfProfile
{
	string m_sBandId;
	ref RICON_RadioRfProfile m_Radio;
	ref RICON_StaticLosProfile m_Los;
	ref RICON_PairServiceLimits m_Limits;
	vector m_vAntennaOffset;
}

class RICON_GameRadioRfPoint
{
	int m_iAttempt, m_iTickMs, m_iDeadlineMs, m_iTxGeneration, m_iTxAdmission, m_iRxAdmission;
	float m_fDistanceM;
	int m_iLinkId, m_iIssuedAtMs, m_iListenerIncarnation, m_iRawTune;
	float m_fFrequencyMHz;
	ref RICON_MediaEgressRoute m_Route;
	ref RICON_GameRadioPttContext m_Context;

	RICON_GameRadioRfPoint ClonePoint()
	{
		RICON_GameRadioRfPoint copy = new RICON_GameRadioRfPoint();
		copy.m_iAttempt = m_iAttempt; copy.m_iTickMs = m_iTickMs; copy.m_iDeadlineMs = m_iDeadlineMs;
		copy.m_iTxGeneration = m_iTxGeneration; copy.m_iTxAdmission = m_iTxAdmission; copy.m_iRxAdmission = m_iRxAdmission;
		copy.m_fDistanceM = m_fDistanceM; copy.m_iLinkId = m_iLinkId; copy.m_iIssuedAtMs = m_iIssuedAtMs;
		copy.m_iListenerIncarnation = m_iListenerIncarnation; copy.m_iRawTune = m_iRawTune; copy.m_fFrequencyMHz = m_fFrequencyMHz;
		copy.m_Route = m_Route.CloneRoute(); copy.m_Context = m_Context.CloneContext();
		return copy;
	}

	bool SameScope(RICON_GameRadioRfPoint other)
	{
		return other && m_Route && m_Route.Same(other.m_Route) && m_Context && m_Context.Same(other.m_Context)
			&& m_iLinkId == other.m_iLinkId && m_iIssuedAtMs == other.m_iIssuedAtMs
			&& m_iListenerIncarnation == other.m_iListenerIncarnation && m_iRawTune == other.m_iRawTune
			&& m_fFrequencyMHz == other.m_fFrequencyMHz && m_iTxGeneration == other.m_iTxGeneration
			&& m_iTxAdmission == other.m_iTxAdmission && m_iRxAdmission == other.m_iRxAdmission;
	}

	bool SamePoint(RICON_GameRadioRfPoint other)
	{
		return SameScope(other) && m_iAttempt == other.m_iAttempt && m_iTickMs == other.m_iTickMs
			&& m_iDeadlineMs == other.m_iDeadlineMs && m_fDistanceM == other.m_fDistanceM;
	}
}

//! Borrowed from the ordinary producer for the duration of its synchronous commit callback.
class RICON_GameRadioRfCommit
{
	int m_iAttempt, m_iEpoch, m_iTickMs;
	int m_iPositionReadStartedMs = -1;
	ref RICON_GameRadioInventory m_Inventory;
	ref RICON_GameRadioPttOwner m_Ptt;
	ref RICON_GameOpenRadioRfProfile m_Profile;
	ref RICON_TemporalPublicationOwner m_Publication;
	ref RICON_LifecycleStateMachine m_Lifecycle;
	ref RICON_MediaReceiverAuthority m_Receivers;
	ref RICON_TemporalProducerTick m_Tick;
	ref RICON_TemporalPublicationResult m_Result;
	ref array<ref RICON_GameRadioPttEmission> m_Emissions;
	ref array<ref RICON_GameRadioOpenSlot> m_Slots;
}

//! One private first-leg observation. Its identity belongs to the station input, independently of direct listeners.
class RICON_GameRelayInputRfPoint
{
	int m_iAttempt, m_iSkewMs, m_iTxAdmission, m_iRxAdmission, m_iTxGeneration, m_iRxGeneration;
	int m_iIssuedAtMs, m_iDeadlineMs;
	float m_fDistanceM;
	BaseRadioComponent m_TxRadio, m_RxRadio;
	vector m_vRxAntennaOffset;
	ref RICON_RelayPcmPath m_Path;
	ref RICON_GameRadioPttContext m_Context;
	ref RICON_RadioRfProfile m_TxProfile, m_RxProfile;
	ref RICON_TemporalRfBudget m_Budget;
	string m_sBandId, m_sIntelligibilityModelId;
	ref RICON_RelayStationAuthority m_Authority;
	ref RICON_MediaRfHistory m_History;
}

//! Authored GameMode opt-in: replace the base manager, never install two managers on the same mode.
class RICON_GameRadioManagerComponent : RICON_CommsManagerComponent
{
	[Attribute("64", UIWidgets.EditBox, "Player binding capacity; match the scenario/server budget")]
	protected int m_iInventoryMaxPlayers;
	[Attribute("256", UIWidgets.EditBox, "Lifecycle active lease capacity")]
	protected int m_iInventoryMaxActiveLeases;
	[Attribute("1024", UIWidgets.EditBox, "Lifecycle terminal lease retention capacity")]
	protected int m_iInventoryMaxTerminalLeases;
	[Attribute("60000", UIWidgets.EditBox, "Terminal lease retention milliseconds")]
	protected int m_iInventoryTerminalLeaseTtlMs;
	[Attribute("128", UIWidgets.EditBox, "Player tombstone retention capacity")]
	protected int m_iInventoryMaxTombstones;
	[Attribute("30000", UIWidgets.EditBox, "Player tombstone retention milliseconds")]
	protected int m_iInventoryTombstoneTtlMs;
	[Attribute("64", UIWidgets.EditBox, "Cached lifecycle responses per player")]
	protected int m_iInventoryMaxResponses;
	[Attribute("1024", UIWidgets.EditBox, "Lifecycle rate bookkeeping capacity")]
	protected int m_iInventoryMaxRateEntries;
	[Attribute("1000", UIWidgets.EditBox, "Lifecycle rate window milliseconds")]
	protected int m_iInventoryRateWindowMs;
	[Attribute("30", UIWidgets.EditBox, "Lifecycle requests per rate window")]
	protected int m_iInventoryRateMaxRequests;
	[Attribute("8192", UIWidgets.EditBox, "Physical RX registration capacity; a resource budget, not a radio topology")]
	protected int m_iInventoryRxSlots;
	[Attribute("250", UIWidgets.EditBox, "Server inventory reconciliation interval in milliseconds")]
	protected int m_iInventoryPollMs;
	[Attribute("0", UIWidgets.EditBox, "Optional server control network; zero keeps inventory-only mode")]
	protected int m_iInventoryNetworkId;
	[Attribute("4096", UIWidgets.EditBox, "Distinct exact tuning/key control channel capacity")]
	protected int m_iInventoryChannelCapacity;
	[Attribute("0", UIWidgets.CheckBox, "Admit powered unkeyed inventory transceivers to OpenRadio RF")]
	protected bool m_bInventoryOpenRadioAdmissions;
	[Attribute("0", UIWidgets.CheckBox, "Enable assumed pocket, manpack and driver-operated RF profiles")]
	protected bool m_bInventoryMixedEquipment;
	[Attribute("38000", UIWidgets.EditBox, "Common open radio carrier in kHz for the mixed-equipment demo")]
	protected int m_iMixedRadioTuneKHz;
	[Attribute("4096", UIWidgets.EditBox, "Maximum RF pair computations per mixed-equipment sample")]
	protected int m_iMixedRadioPairBudget;
	protected ref map<string, ref RICON_GameRadioHardwareSettings> m_MixedRadioHardware;
	protected ref map<IEntity, ref RICON_GmJammerRole> m_GmJammers = new map<IEntity, ref RICON_GmJammerRole>();
	protected int m_iGmEmissionHighWater;
	protected ref map<BaseTransceiver, ref RICON_StockJammerSlot> m_StockJammerMutes = new map<BaseTransceiver, ref RICON_StockJammerSlot>();
	[Attribute("1500", UIWidgets.EditBox, "Bounded server PTT lease milliseconds; no automatic renewal")]
	protected int m_iInventoryPttTtlMs;

	protected ref RICON_LifecycleStateMachine m_InventoryLifecycle;
	protected ref RICON_GameControllerLifecycle m_InventoryBridge;
	protected ref RICON_MediaReceiverAuthority m_InventoryReceivers;
	// Keep the last scope identity while an already committed result drains after StopInventory.
	protected RICON_LifecycleStateMachine m_InventoryConsumeIdentity;
	protected ref RICON_GameRadioNativeInventorySource m_InventorySource;
	protected ref RICON_GameRadioInventory m_Inventory;
	protected ref RICON_GameRadioControlPolicy m_InventoryControlPolicy;
	protected ref RICON_GameRadioPttNativeSource m_InventoryPttSource;
	protected ref RICON_GameRadioPttOwner m_InventoryPtt;
	protected ref RICON_GameTs3BindingStore m_InventoryTs3Bindings;
	protected ref RICON_GameTs3ProofAdapter m_InventoryTs3Proof;
	protected ref RICON_GameOpenRadioRfProfile m_GameRfProfile;
	protected ref RICON_OpenRadioListenerRoster m_GameRfRoster;
	protected int m_iLastGameRfSampleMs = -1;
	protected int m_iDevRfTraceMs = -1, m_iDevPcmTraceMs = -1;
	protected string m_sDevRfState;
	protected int m_iGameRfAttempt;
	protected ref RICON_GameRadioRfCommit m_GameRfCommit;
	protected ref map<int, ref RICON_GameRadioRfPoint> m_PreviousGameRfPoints, m_CurrentGameRfPoints;
	protected ref RICON_MediaStreamOwner m_GameOpenRadioPcmOwner;
	protected int m_iGameOpenRadioPcmEpoch, m_iGameOpenRadioPcmMaxAgeMs;
	protected bool m_bReceivingGameOpenRadioPcm, m_bConfiguringGameRelayStation;
	protected ref RICON_GameRelayInputRfPoint m_GameRelayInputRfPoint;
	protected ref RICON_GameRelayInputRfPoint m_GameRelayInputPcmPoint;
	protected ref RICON_RelayInputSeries m_GameRelayInputPcmSeries;
	protected ref array<ref RICON_QueuedMediaBlock> m_GameRelayInputPcmBacklog;
	protected bool m_bObservingGameRelayRf, m_bGameRelayRfCancelled, m_bBindingGameRelayInputPcm;
	protected UUID m_InventoryPublicDomain;
	protected int m_iInventoryDomainEpoch;
	protected int m_iInventoryBootstrapAttempts;
	protected bool m_bInventoryStopping, m_bInventoryStarted;
	protected string m_sInventoryStatus;
	protected static const int BOOTSTRAP_ATTEMPTS = 20;
	protected static const int BOOTSTRAP_INTERVAL_MS = 100;

	override void OnWorldPostProcess(World world)
	{
		StopInventory();
		super.OnWorldPostProcess(world);
		m_Inventory = null; m_InventorySource = null;
		m_InventoryBridge = null; m_InventoryReceivers = null; m_InventoryLifecycle = null;
		m_InventoryControlPolicy = null;
		m_InventoryPtt = null; m_InventoryPttSource = null;
		m_InventoryConsumeIdentity = null; m_bInventoryStarted = false;
		if (!IsAuthority()) return;
		m_InventoryPublicDomain = UUID.GenV4();
		m_iInventoryDomainEpoch = GetSessionEpoch();
		m_iInventoryBootstrapAttempts = 0;
		m_sInventoryStatus = "";
		GetGame().GetCallqueue().CallLater(BootstrapInventory, 0, false, GetSessionEpoch());
	}

	protected RICON_LifecycleConfig InventoryLifecycleConfig()
	{
		RICON_LifecycleConfig config = new RICON_LifecycleConfig();
		config.m_iMaxPlayers = m_iInventoryMaxPlayers;
		config.m_iMaxActiveLeases = m_iInventoryMaxActiveLeases;
		config.m_iMaxTerminalLeases = m_iInventoryMaxTerminalLeases;
		config.m_iTerminalLeaseTtlMs = m_iInventoryTerminalLeaseTtlMs;
		config.m_iMaxTombstones = m_iInventoryMaxTombstones;
		config.m_iTombstoneTtlMs = m_iInventoryTombstoneTtlMs;
		config.m_iMaxResponseCachePerPlayer = m_iInventoryMaxResponses;
		config.m_iMaxRateEntries = m_iInventoryMaxRateEntries;
		config.m_iRateWindowMs = m_iInventoryRateWindowMs;
		config.m_iRateMaxRequests = m_iInventoryRateMaxRequests;
		return config;
	}

	protected bool CurrentBootstrap(int expectedEpoch, RICON_LifecycleStateMachine lifecycle)
	{
		return GetGame() && IsAuthority() && !m_bInventoryStopping && GetSessionEpoch() == expectedEpoch
			&& GetInstance() == this && m_InventoryLifecycle == lifecycle;
	}

	protected void BootstrapInventory(int expectedEpoch)
	{
		if (!IsAuthority() || !GetGame() || m_bInventoryStarted || m_bInventoryStopping || GetSessionEpoch() != expectedEpoch) return;
		IEntity owner = GetOwner();
		BaseWorld world = GetGame().GetWorld();
		if (!owner || !world || world.IsEditMode() || owner.GetWorld() != world
			|| GetGame().GetGameMode() != owner || GetInstance() != this || GetSessionEpoch() <= 0)
		{
			m_iInventoryBootstrapAttempts++;
			if (m_iInventoryBootstrapAttempts < BOOTSTRAP_ATTEMPTS)
				GetGame().GetCallqueue().CallLater(BootstrapInventory, BOOTSTRAP_INTERVAL_MS, false, expectedEpoch);
			else ReportInventory("GAME_MODE_NOT_READY");
			return;
		}
		RplMode mode = RplSession.Mode();
		if (mode != RplMode.Dedicated && mode != RplMode.Listen)
		{
			ReportInventory("NETWORKED_REMOTE_SCOPE_REQUIRED");
			return;
		}
		RICON_LifecycleConfig config = InventoryLifecycleConfig();
		if (!config.IsValid() || m_iInventoryRxSlots <= 0 || m_iInventoryPollMs <= 0 || m_iInventoryNetworkId < 0
			|| (m_bInventoryOpenRadioAdmissions && m_iInventoryNetworkId == 0)
			|| (m_bInventoryMixedEquipment && (!m_bInventoryOpenRadioAdmissions || m_iMixedRadioTuneKHz <= 0
				|| m_iMixedRadioPairBudget <= 0 || m_iMixedRadioPairBudget > 65536))
			|| (m_iInventoryNetworkId > 0 && (m_iInventoryChannelCapacity <= 0 || m_iInventoryPttTtlMs <= 0
				|| m_iInventoryPttTtlMs > RICON_GameRadioPttOwner.MAX_SERVER_TTL_MS)))
		{
			ReportInventory("INVALID_RESOURCE_BUDGETS");
			return;
		}
		if (m_iInventoryDomainEpoch != expectedEpoch || !RICON_GameRadioControlPolicy.IsValidIdentity(m_InventoryPublicDomain))
		{
			FailInventory("PUBLIC_DOMAIN_UNAVAILABLE");
			return;
		}
		m_MixedRadioHardware = null;
		if (m_bInventoryMixedEquipment && !RICON_GameRadioEquipment.ReadHardwareProfiles(m_MixedRadioHardware))
		{
			ReportInventory("INVALID_RADIO_HARDWARE_CONFIG");
			return;
		}
		if (m_MixedRadioHardware) Print("[RICON_GAME_RADIO_INVENTORY] hardwareConfig=ricon-radio-hardware.json");
		ref RICON_GameRadioControlPolicy policy;
		if (m_iInventoryNetworkId > 0)
		{
			policy = new RICON_GameRadioControlPolicy();
			if (!policy.Configure(expectedEpoch, m_InventoryPublicDomain, m_iInventoryNetworkId, m_iInventoryChannelCapacity))
			{
				FailInventory("CONTROL_POLICY_CONFIGURATION_FAILED");
				return;
			}
		}
		if (!ConfigureGameRadioTemporal()) { FailInventory("AUTHORITY_CONFIGURATION_FAILED"); return; }
		ref RICON_LifecycleStateMachine lifecycle = new RICON_LifecycleStateMachine();
		m_InventoryLifecycle = lifecycle;
		m_InventoryConsumeIdentity = lifecycle;
		array<ref RICON_LifecycleDevice> emptyCatalogue = {};
		RICON_LifecycleResult initialized = lifecycle.Initialize(expectedEpoch, 0, config, emptyCatalogue);
		int processAnchor = System.GetTickCount();
		if (!initialized || initialized.m_eResultCode != RICON_ELifecycleResult.OK || processAnchor < 0)
		{
			FailInventory("LIFECYCLE_CONFIGURATION_FAILED");
			return;
		}
		ref RICON_GameControllerLifecycle bridge = EnableGameControllerLifecycle(lifecycle, processAnchor, m_iInventoryMaxPlayers, false);
		if (!CurrentBootstrap(expectedEpoch, lifecycle)) return;
		if (!bridge) { FailInventory("CONTROLLER_CONFIGURATION_FAILED"); return; }
		m_InventoryBridge = bridge;
		ref RICON_MediaReceiverAuthority receivers = ConfigureMediaReceiverAuthority(lifecycle, m_iInventoryRxSlots);
		if (!CurrentBootstrap(expectedEpoch, lifecycle)) return;
		if (!receivers) { FailInventory("RX_CONFIGURATION_FAILED"); return; }
		m_InventoryReceivers = receivers;
		ref RICON_GameRadioNativeInventorySource source = new RICON_GameRadioNativeInventorySource();
		ref RICON_GameRadioInventory inventory = new RICON_GameRadioInventory();
		bool configured = source.Configure(this, lifecycle, bridge, receivers, policy, m_bInventoryOpenRadioAdmissions, m_bInventoryMixedEquipment)
			&& inventory.Configure(lifecycle, receivers, source);
		if (!CurrentBootstrap(expectedEpoch, lifecycle)) { inventory.Close(); return; }
		if (!configured)
		{
			inventory.Close();
			FailInventory("INVENTORY_CONFIGURATION_FAILED");
			return;
		}
		m_InventorySource = source; m_Inventory = inventory; m_InventoryControlPolicy = policy;
		m_bInventoryStarted = true;
		if (policy)
		{
			ref RICON_GameRadioPttNativeSource pttSource = new RICON_GameRadioPttNativeSource();
			ref RICON_GameRadioPttOwner ptt = new RICON_GameRadioPttOwner();
			bool pttConfigured = pttSource.Configure(this, lifecycle, bridge) && ptt.Configure(lifecycle, pttSource, m_iInventoryPttTtlMs);
			if (!CurrentInventory(inventory, expectedEpoch)) { ptt.Close(); return; }
			if (!pttConfigured) { ptt.Close(); FailInventory("PTT_CONFIGURATION_FAILED"); return; }
			m_InventoryPttSource = pttSource; m_InventoryPtt = ptt;
			m_InventoryTs3Bindings = new RICON_GameTs3BindingStore();
			if (!m_InventoryTs3Bindings.Configure(m_InventoryPublicDomain, expectedEpoch, m_iInventoryMaxPlayers))
				m_InventoryTs3Bindings = null;
		}
		if (policy) ReportInventory("CONTROL_POLICY_READY");
		else ReportInventory("CONTROL_ONLY_READY");
		if (!CurrentInventory(inventory, expectedEpoch)) return;
		GetGame().GetCallqueue().CallLater(PollInventory, m_iInventoryPollMs, true, expectedEpoch);
		PollInventory(expectedEpoch);
	}

	protected bool CurrentInventory(RICON_GameRadioInventory inventory, int expectedEpoch)
	{
		return inventory && m_Inventory == inventory && m_bInventoryStarted && !m_bInventoryStopping
			&& GetSessionEpoch() == expectedEpoch && GetInstance() == this;
	}

	protected void PollInventory(int expectedEpoch)
	{
		ref RICON_GameRadioInventory inventory = m_Inventory;
		if (!CurrentInventory(inventory, expectedEpoch)) return;
		m_GameRfCommit = null;
		m_PreviousGameRfPoints = m_CurrentGameRfPoints;
		m_CurrentGameRfPoints = null;
		if (m_iGameRfAttempt < int.MAX) m_iGameRfAttempt++;
		int attempt = m_iGameRfAttempt;
		string status = inventory.Reconcile();
		if (!CurrentInventory(inventory, expectedEpoch)) return;
		ref RICON_GameRadioPttOwner ptt = m_InventoryPtt;
		if (ptt)
		{
			bool pumped = ptt.Pump();
			if (!CurrentPtt(ptt, expectedEpoch)) return;
			if (!pumped) status = "PTT_CLOCK_OR_SCOPE_UNAVAILABLE";
			PollTs3Bindings();
			if (m_GameRfProfile && status == "OK" && attempt == m_iGameRfAttempt) PublishGameOpenRadioRfTick(expectedEpoch);
		}
		PollStockJamming(inventory, expectedEpoch, status);
		ReportInventory(status);
		if (CurrentInventory(inventory, expectedEpoch) && (status == "CLOSED" || status == "SCOPE_LOST")) StopInventory();
	}

	//! A scenario opts in before BootstrapInventory, supplying radio and LOS parameters.
	protected RICON_GameOpenRadioRfProfile ReadAuthoredGameRfProfile()
	{
		if (!m_bInventoryMixedEquipment) return null;
		RICON_GameOpenRadioRfProfile profile = new RICON_GameOpenRadioRfProfile();
		profile.m_sBandId = "demo_open_vhf";
		profile.m_Radio = new RICON_RadioRfProfile();
		profile.m_Radio.m_iDeviceId = 1; profile.m_Radio.m_iTransceiverIndex = 0;
		profile.m_Radio.m_iExpectedTuningValue = m_iMixedRadioTuneKHz;
		profile.m_Radio.m_fFrequencyMHz = m_iMixedRadioTuneKHz * 0.001;
		profile.m_Radio.m_fRxBandwidthHz = 25000;
		if (!RICON_GameRadioEquipment.ApplyHardware(RICON_EGameRadioEquipment.POCKET, profile.m_Radio)) return null;
		profile.m_Los = new RICON_StaticLosProfile();
		profile.m_Los.m_sProfileId = "demo_open_los_v1";
		profile.m_Los.m_iProfileVersion = 1; profile.m_Los.m_iEnvironmentRevision = 1;
		profile.m_Los.m_iValidFromMs = 0; profile.m_Los.m_iValidUntilMs = int.MAX;
		profile.m_Los.m_fFrequencyMHz = profile.m_Radio.m_fFrequencyMHz;
		profile.m_Los.m_fBandwidthHz = profile.m_Radio.m_fRxBandwidthHz;
		profile.m_Los.m_fMinimumDistanceM = 1;
		profile.m_Los.m_fMaximumDistanceM = 12000;
		profile.m_Los.m_fDistanceErrorM = 0.001;
		profile.m_Los.m_fMaxPhaseErrorTurns = 0.02;
		profile.m_Los.m_bSinglePathQualified = true;
		profile.m_Limits = new RICON_PairServiceLimits();
		profile.m_Limits.m_iMaxBindings = m_iInventoryMaxActiveLeases;
		profile.m_Limits.m_iMaxPlayers = m_iInventoryMaxPlayers;
		profile.m_Limits.m_iMaxListeners = Math.Min(m_iInventoryRxSlots, 4096);
		profile.m_Limits.m_iMaxDevices = profile.m_Limits.m_iMaxListeners;
		profile.m_Limits.m_iMaxEndpoints = profile.m_Limits.m_iMaxListeners;
		profile.m_Limits.m_iMaxPairs = m_iMixedRadioPairBudget;
		return profile;
	}

	protected bool ConfigureGameRadioTemporal()
	{
		ref RICON_GameOpenRadioRfProfile profile = ReadAuthoredGameRfProfile();
		RICON_TemporalLimits temporal = new RICON_TemporalLimits();
		if (!profile) return !m_bInventoryMixedEquipment && ConfigureTemporalAuthority(temporal);
		if (!m_bInventoryOpenRadioAdmissions || !profile.m_Radio
			|| (!m_bInventoryMixedEquipment && profile.m_Radio.m_sProvenance != RICON_RadioRfProfile.PROVENANCE_TEST_ONLY)
			|| !profile.m_Limits || !RICON_StaticLosChannel.ValidProfile(profile.m_Los)
			|| !RICON_TemporalSnapshots.ValidKey(profile.m_sBandId) || !RICON_Num.IsFiniteVector(profile.m_vAntennaOffset)
			|| profile.m_Radio.m_iExpectedTuningValue <= 0 || profile.m_Radio.m_fFrequencyMHz != profile.m_Los.m_fFrequencyMHz
			|| profile.m_Radio.m_fRxBandwidthHz != profile.m_Los.m_fBandwidthHz) return false;
		RICON_RadioRfProfile template = RICON_OpenRadioPairProjector.CopyProfile(profile.m_Radio);
		template.m_iDeviceId = 1;
		template.m_iTransceiverIndex = 0;
		if (!RICON_RadioQueryBuilder.IsValidProfile(template)) return false;
		RICON_LinkPublicationLimits links = new RICON_LinkPublicationLimits();
		links.m_iMaxBindings = profile.m_Limits.m_iMaxBindings;
		links.m_iMaxPairs = profile.m_Limits.m_iMaxPairs;
		links.m_iMaxActiveLinks = profile.m_Limits.m_iMaxPairs;
		if (!RegisterPropagation(new RICON_FreeSpaceModel()) || !RegisterNoise(new RICON_NoiseModel(), "TEST_ONLY", 1)
			|| !RegisterIntelligibility(new RICON_LinearTestIntelligibility(-15, 15, 0.8, 0.5))
			|| !RegisterBand(new RICON_BandConfig(profile.m_sBandId, "free_space", "none", "linear_test", 1))
			|| !FreezeRegistries() || !ConfigureTemporalProducerV2(profile.m_sBandId, links, temporal)) return false;
		m_GameRfProfile = profile;
		return true;
	}

	override protected int TemporalRosterNetworkId()
	{
		if (m_GameRfProfile && m_InventoryControlPolicy) return m_InventoryControlPolicy.NetworkId();
		return super.TemporalRosterNetworkId();
	}

	protected void PublishGameOpenRadioRfTick(int expectedEpoch)
	{
		ref RICON_GameRadioInventory inventory = m_Inventory;
		ref RICON_GameRadioPttOwner ptt = m_InventoryPtt;
		ref RICON_GameOpenRadioRfProfile profile = m_GameRfProfile;
		if (!profile || !m_InventoryControlPolicy || !CurrentPtt(ptt, expectedEpoch)) { TraceRadioDevRf("NO_AUTHORITY"); return; }
		int attempt = m_iGameRfAttempt;
		int rawTune = profile.m_Radio.m_iExpectedTuningValue;
		int networkId = m_InventoryControlPolicy.NetworkId();
		int channelId = m_InventoryControlPolicy.ReadChannel(expectedEpoch, rawTune, "");
		if (channelId <= 0) { TraceRadioDevRf("WAIT_COMMON_CHANNEL"); return; }
		if (!m_GameRfRoster)
		{
			m_GameRfRoster = CreateCarriedOpenRadioListenerRoster(rawTune, profile.m_Limits.m_iMaxListeners);
			if (!m_GameRfRoster) { TraceRadioDevRf("NO_LISTENER_ROSTER"); return; }
		}
		array<ref RICON_GameRadioPttEmission> emissions = {};
		if (!ptt.CaptureChannelEmissions(networkId, channelId, emissions) || !CurrentPtt(ptt, expectedEpoch)) { TraceRadioDevRf("EMISSIONS_UNAVAILABLE"); return; }
		ref RICON_OpenRadioListenerRoster roster = m_GameRfRoster;
		array<ref RICON_GameRadioOpenSlot> slots = {};
		int sampleMs;
		// shortcut: reuse the complete RF snapshot after the control poll, combine the scans if measured cost requires it.
		int positionReadStartedMs = System.GetTickCount();
		if (positionReadStartedMs < 0) { TraceRadioDevRf("CLOCK_UNAVAILABLE"); return; }
		if (!roster.RefreshCarried(slots, sampleMs) || !CurrentInventory(inventory, expectedEpoch) || m_GameRfRoster != roster
			|| sampleMs <= m_iLastGameRfSampleMs) { TraceRadioDevRf("SNAPSHOT_STALE"); return; }
		m_iLastGameRfSampleMs = sampleMs;
		RICON_TemporalProducerTick tick;
		string built = BuildGameOpenRadioRfTick(profile, roster, slots, emissions, networkId, channelId, sampleMs, tick);
		if (built != "OK") { TraceRadioDevRf(built); ReportInventory("GAME_RF_" + built); return; }
		if (!CurrentInventory(inventory, expectedEpoch) || m_GameRfRoster != roster) return;
		ref RICON_LifecycleStateMachine lifecycle = m_InventoryLifecycle;
		ref RICON_TemporalPublicationOwner publication = RICON_TemporalPublicationOwner.Cast(m_TemporalAuthorityJournal);
		ref RICON_MediaReceiverAuthority receivers = m_InventoryReceivers;
		RICON_TemporalPublicationResult result = PublishTemporalPairs2(tick, lifecycle, sampleMs);
		if (!result) { TraceRadioDevRf("NO_PUBLICATION"); return; }
		if (result.m_sCode != "OK") { TraceRadioDevRf(result.m_sCode); ReportInventory("GAME_RF_" + result.m_sCode); return; }
		if (CurrentInventory(inventory, expectedEpoch) && m_GameRfRoster == roster)
		{
			ref RICON_GameRadioRfCommit committed;
			if (attempt > 0 && attempt < int.MAX && attempt == m_iGameRfAttempt && m_InventoryLifecycle == lifecycle
				&& publication && publication == m_TemporalAuthorityJournal && receivers == m_InventoryReceivers)
			{
				committed = new RICON_GameRadioRfCommit();
				committed.m_iAttempt = attempt; committed.m_iEpoch = expectedEpoch; committed.m_iTickMs = sampleMs;
				committed.m_iPositionReadStartedMs = positionReadStartedMs;
				committed.m_Inventory = inventory; committed.m_Ptt = ptt; committed.m_Profile = profile;
				committed.m_Publication = publication;
				committed.m_Lifecycle = lifecycle; committed.m_Receivers = receivers;
				committed.m_Tick = tick; committed.m_Result = result; committed.m_Emissions = emissions; committed.m_Slots = slots;
				m_GameRfCommit = committed;
			}
			if (committed) TraceRadioDevRf("OK", committed);
			if (committed) RICON_PublishTs3RoutePulls(committed);
			OnGameOpenRadioRfCommitted(tick, result, emissions);
			if (m_GameRfCommit == committed) m_GameRfCommit = null;
		}
	}

	protected void TraceRadioDevRf(string status, RICON_GameRadioRfCommit committed = null)
	{
		if (!RICON_RadioDev.Enabled()) return;
		int tickMs = -1;
		int txCount, slotCount, linkCount, endCount;
		if (committed)
		{
			tickMs = committed.m_iTickMs;
			txCount = committed.m_Emissions.Count(); slotCount = committed.m_Slots.Count();
			linkCount = committed.m_Result.m_aActiveIds.Count(); endCount = committed.m_Result.m_aEnds.Count();
		}
		int nowMs = System.GetTickCount();
		string signature = string.Format("%1/%2/%3/%4", status, txCount, slotCount, linkCount);
		if (signature == m_sDevRfState && m_iDevRfTraceMs >= 0 && nowMs >= m_iDevRfTraceMs && nowMs - m_iDevRfTraceMs < 1000) return;
		m_iDevRfTraceMs = nowMs;
		m_sDevRfState = signature;
		int tune;
		if (m_GameRfProfile && m_GameRfProfile.m_Radio) tune = m_GameRfProfile.m_Radio.m_iExpectedTuningValue;
		PrintFormat("[RICON_DEV] SERVER RF state=%1 tick=%2 tuneKHz=%3 radios=%4 RX=%5 TX=%6 links=%7 ends=%8 pcmOwner=%9",
			status, tickMs, tune, GetInventoryRadioCount(), slotCount, txCount, linkCount, endCount, m_GameOpenRadioPcmOwner != null);
		if (!committed) return;
		const int detailLimit = 48;
		if (slotCount > detailLimit || linkCount > detailLimit || txCount > detailLimit)
			PrintFormat("[RICON_DEV] SERVER RF detail limited to %1 entries per group", detailLimit);
		for (int i = 0; i < Math.Min(slotCount, detailLimit); i++)
		{
			RICON_GameRadioOpenSlot slot = committed.m_Slots[i];
			RICON_OpenRadioPairListener row = slot.m_Listener;
			PrintFormat("[RICON_DEV] SERVER RX player=%1 radio=%2 slot=%3 gen=%4 admission=%5 type=%6", row.m_iPlayerId,
				row.m_iDeviceId, row.m_iIndex, row.m_iGeneration, slot.m_iAdmission,
				typename.EnumToString(RICON_EGameRadioEquipment, RICON_GameRadioEquipment.ReadType(slot.m_Radio)));
		}
		for (int iTx = 0; iTx < Math.Min(txCount, detailLimit); iTx++)
		{
			RICON_GameRadioPttEmission tx = committed.m_Emissions[iTx];
			PrintFormat("[RICON_DEV] SERVER TX lease=%1 radio=%2 slot=%3 net=%4 channel=%5 deadlineMs=%6",
				tx.m_iLeaseId, tx.m_iDeviceId, tx.m_iTxTransceiverIndex, tx.m_iNetworkId, tx.m_iChannelId, tx.m_iDeadlineMs);
		}
		for (int iLink = 0; iLink < Math.Min(linkCount, detailLimit); iLink++)
		{
			int linkId = committed.m_Result.m_aActiveIds[iLink];
			committed.m_Publication.TraceRadioDevLink(linkId, committed.m_Result.m_ExpectedByLink.Get(linkId), tickMs);
		}
	}

	//! Synchronous scenario hook. The published tick and detached emissions describe one authority sample.
	protected void OnGameOpenRadioRfCommitted(RICON_TemporalProducerTick tick,
		RICON_TemporalPublicationResult result, array<ref RICON_GameRadioPttEmission> emissions) {}

	//! The scenario attests motion, uninterrupted attachment and fixed RF inputs over this exact interval.
	protected RICON_MediaFreeSpaceMotionLimits ReadAuthoredGameRfCoverageLimits(RICON_MediaEgressRoute route, int fromMs, int untilMs) { return null; }

	protected bool GameRfCommitCurrent(RICON_GameRadioRfCommit committed, int nowMs)
	{
		if (!committed || committed != m_GameRfCommit || !IsAuthority() || nowMs != committed.m_iTickMs
			|| nowMs != m_iLastGameRfSampleMs || m_iGameRfAttempt != committed.m_iAttempt
			|| !CurrentInventory(committed.m_Inventory, committed.m_iEpoch) || !CurrentPtt(committed.m_Ptt, committed.m_iEpoch)
			|| !committed.m_Receivers || m_InventoryReceivers != committed.m_Receivers || m_InventoryLifecycle != committed.m_Lifecycle
			|| !committed.m_Profile || m_GameRfProfile != committed.m_Profile || !committed.m_Result || committed.m_Result.m_sCode != "OK"
			|| !committed.m_Publication || committed.m_Publication != m_TemporalAuthorityJournal) return false;
		RICON_TemporalProducerTick tick = committed.m_Tick;
		return tick && tick.m_ListenerRoster == m_GameRfRoster && tick.m_PairInput && tick.m_PairInput.m_iNowMs == nowMs
			&& tick.m_PairInput.m_iSessionEpoch == committed.m_iEpoch && committed.m_Publication.HasCommittedMediaTick(committed.m_Lifecycle, nowMs);
	}

	//! The game RF publisher may attach one existing V3 ingress owner per epoch.
	protected bool BindGameOpenRadioPcm(RICON_MediaStreamOwner owner, int nowMs, int maxCaptureAgeMs)
	{
		ref RICON_GameRadioRfCommit committed = m_GameRfCommit;
		if (maxCaptureAgeMs <= 0 || !GameRfCommitCurrent(committed, nowMs)
			|| !GameOpenRadioPcmInfrastructureCurrent(owner, committed.m_iEpoch)
			|| !GameRfCommitCurrent(committed, nowMs)) return false;
		if (m_iGameOpenRadioPcmEpoch == committed.m_iEpoch)
			return m_GameOpenRadioPcmOwner == owner && m_iGameOpenRadioPcmMaxAgeMs == maxCaptureAgeMs;
		if (m_GameOpenRadioPcmOwner) return false;
		m_GameOpenRadioPcmOwner = owner;
		m_iGameOpenRadioPcmEpoch = committed.m_iEpoch;
		m_iGameOpenRadioPcmMaxAgeMs = maxCaptureAgeMs;
		return true;
	}

	protected bool GameOpenRadioPcmInfrastructureCurrent(RICON_MediaStreamOwner owner, int epoch)
	{
		return IsAuthority() && CurrentPtt(m_InventoryPtt, epoch) && owner && owner.IsScope(m_InventoryLifecycle)
			&& !m_OpenRadioTickSource && m_InventoryBridge && m_InventoryBridge == m_GameControllerLifecycle
			&& m_InventoryBridge.MatchesLifecycle(m_InventoryLifecycle)
			&& m_InventoryReceivers && m_InventoryReceivers == m_MediaReceiverAuthority && m_MediaListenerAuthority
			&& m_MediaListenerAuthority.IsScope(m_InventoryLifecycle, owner, m_InventoryReceivers)
			&& m_MediaWireTransport && m_MediaWireTransport.GetWireVersion() == 3
			&& m_MediaWireTransport.GetRfTimebaseId() == "game-open-rf" && m_MediaWireTransport.IsCurrent();
	}

	protected bool GameOpenRadioPcmBound()
	{
		return m_GameOpenRadioPcmOwner || (m_iGameOpenRadioPcmEpoch > 0 && m_iGameOpenRadioPcmEpoch == GetSessionEpoch());
	}

	override bool StartOpenRadioLoop(RICON_LifecycleStateMachine lifecycle, RICON_MediaStreamOwner sourceOwner,
		RICON_OpenRadioTickSource tickSource, RICON_MediaFanoutTransport transport, int intervalMs, int maxVisits, int maxCaptureAgeMs)
	{
		if (GameOpenRadioPcmBound()) return false;
		return super.StartOpenRadioLoop(lifecycle, sourceOwner, tickSource, transport, intervalMs, maxVisits, maxCaptureAgeMs);
	}

	override void ReceiveOpenRadioPcm(SCR_PlayerController sender, int epoch, int leaseId, int streamId,
		int sequence, int firstSampleIndex, array<int> payload)
	{
		if (!GameOpenRadioPcmBound())
		{
			TraceRadioDevPcm(sender, leaseId, streamId, sequence, "DEFER_TO_BASE");
			super.ReceiveOpenRadioPcm(sender, epoch, leaseId, streamId, sequence, firstSampleIndex, payload);
			return;
		}
		if (m_bReceivingGameOpenRadioPcm) return;
		m_bReceivingGameOpenRadioPcm = true;
		string result = ReceiveBoundGameOpenRadioPcm(sender, epoch, leaseId, streamId, sequence, firstSampleIndex, payload);
		TraceRadioDevPcm(sender, leaseId, streamId, sequence, result);
		OnGameOpenRadioPcmResult(sender, leaseId, streamId, sequence, result);
		m_bReceivingGameOpenRadioPcm = false;
	}

	protected void TraceRadioDevPcm(SCR_PlayerController sender, int leaseId, int streamId, int sequence, string result)
	{
		if (!RICON_RadioDev.Enabled()) return;
		int nowMs = System.GetTickCount();
		if (m_iDevPcmTraceMs >= 0 && nowMs >= m_iDevPcmTraceMs && nowMs - m_iDevPcmTraceMs < 1000) return;
		m_iDevPcmTraceMs = nowMs;
		int player;
		if (sender) player = sender.GetPlayerId();
		PrintFormat("[RICON_DEV] SERVER PCM player=%1 lease=%2 stream=%3 sequence=%4 result=%5",
			player, leaseId, streamId, sequence, result);
	}

	protected string ReceiveBoundGameOpenRadioPcm(SCR_PlayerController sender, int epoch, int leaseId, int streamId,
		int sequence, int firstSampleIndex, array<int> payload)
	{
		ref RICON_MediaStreamOwner owner = m_GameOpenRadioPcmOwner;
		ref RICON_LifecycleStateMachine lifecycle = m_InventoryLifecycle;
		ref RICON_GameControllerLifecycle bridge = m_InventoryBridge;
		ref RICON_MediaRpcTransport transport = m_MediaWireTransport;
		if (epoch != m_iGameOpenRadioPcmEpoch || !owner || !bridge || !CurrentPtt(m_InventoryPtt, epoch)) return "SCOPE_UNAVAILABLE";
		if (leaseId <= 0 || streamId <= 0 || sequence < 0 || firstSampleIndex < 0 || !payload
			|| payload.Count() != RICON_MediaPcmTestProfile.PAYLOAD_BYTES) return "INVALID_FRAME";
		int connectionId, generation, entityId;
		if (!bridge.ReadRemoteSender(sender, connectionId, generation, entityId)) return "SENDER_MISMATCH";
		ref RICON_GameRadioPttEmission emission = ReadGameRadioEmission(sender, leaseId);
		if (!emission || !emission.m_Context) return "NO_EMISSION";
		RICON_GameRadioPttContext context = emission.m_Context;
		if (context.m_iEpoch != epoch || context.m_iPlayerId != sender.GetPlayerId() || context.m_iConnectionId != connectionId
			|| context.m_iGeneration != generation || context.m_iEntityId != entityId || emission.m_iLeaseId != leaseId) return "NO_EMISSION";
		int nowMs = bridge.ReadWireAuthorityTime(lifecycle);
		if (!RICON_OpenRadioPairProjector.BindGameEmission(lifecycle, emission, nowMs)) return "NO_EMISSION";
		if (owner != m_GameOpenRadioPcmOwner || lifecycle != m_InventoryLifecycle || bridge != m_InventoryBridge
			|| transport != m_MediaWireTransport || !GameOpenRadioPcmInfrastructureCurrent(owner, epoch)) return "SCOPE_UNAVAILABLE";
		ref RICON_MediaStreamBinding source = owner.ReadCurrentSource(epoch, leaseId, nowMs);
		if (!source) return "STREAM_NOT_ADMITTED";
		if (source.m_iSessionEpoch != epoch || source.m_iLeaseId != leaseId || source.m_iStreamId != streamId
			|| source.m_iPlayerId != context.m_iPlayerId || source.m_iConnectionId != connectionId
			|| source.m_iPlayerGeneration != generation || source.m_iEntityId != entityId || source.m_iDeviceId != emission.m_iDeviceId
			|| source.m_iTxTransceiverIndex != emission.m_iTxTransceiverIndex || source.m_iNetworkTypeId != emission.m_iNetworkId
			|| source.m_iChannelId != emission.m_iChannelId) return "BINDING_MISMATCH";
		RICON_EMediaWindowDecision window;
		RICON_EMediaIngressDecision decision;
		ref RICON_QueuedMediaBlock accepted;
		if (m_GameRelayInputPcmPoint && m_GameRelayInputPcmPoint.m_Path.m_UpstreamRoute.m_Source.Same(source))
		{
			decision = owner.AcceptRemotePcmWithReceipt(epoch, leaseId, streamId, context.m_iPlayerId, connectionId, generation,
				entityId, nowMs, sequence, firstSampleIndex, payload, m_iGameOpenRadioPcmMaxAgeMs, window, accepted, transport.GetRfTimebaseId());
			if (decision == RICON_EMediaIngressDecision.ACCEPTED && accepted)
			{
				if (m_GameRelayInputPcmBacklog) PumpGameRelayInputPcmBacklog();
				PrepareGameRelayInputPcm(sender, accepted, owner, false);
			}
		}
		else
		{
			decision = owner.AcceptRemotePcm(epoch, leaseId, streamId, context.m_iPlayerId, connectionId, generation,
				entityId, nowMs, sequence, firstSampleIndex, payload, m_iGameOpenRadioPcmMaxAgeMs, window, transport.GetRfTimebaseId());
		}
		return typename.EnumToString(RICON_EMediaIngressDecision, decision);
	}

	protected void OnGameOpenRadioPcmResult(SCR_PlayerController sender, int leaseId, int streamId, int sequence, string result) {}

	//! Attach one station interpolator after the installed capture anchor has completed RF coverage.
	protected bool BindGameRelayInputPcm(RICON_RelayStationAuthority authority, RICON_RelayPcmPath path,
		RICON_MediaRfHistory history, int firstSampleIndex, int nowMs)
	{
		ref RICON_GameRadioManagerComponent held = this;
		if (m_bBindingGameRelayInputPcm) return false;
		m_bBindingGameRelayInputPcm = true;
		bool bound = BindGameRelayInputPcmHeld(authority, path, history, firstSampleIndex, nowMs);
		m_bBindingGameRelayInputPcm = false;
		return bound;
	}

	protected bool BindGameRelayInputPcmHeld(RICON_RelayStationAuthority authority, RICON_RelayPcmPath path,
		RICON_MediaRfHistory history, int firstSampleIndex, int nowMs)
	{
		ref RICON_GameRadioRfCommit committed = m_GameRfCommit;
		ref RICON_GameRelayInputRfPoint point = m_GameRelayInputRfPoint;
		ref RICON_MediaStreamOwner owner = m_GameOpenRadioPcmOwner;
		if (m_GameRelayInputPcmPoint || m_bReceivingGameOpenRadioPcm || !GameRfCommitCurrent(committed, nowMs)
			|| !point || !point.m_Budget || point.m_Budget.m_iTickMs != nowMs || !path || !point.m_Path.SamePath(path)
			|| point.m_Authority != authority || point.m_History != history || !authority || !history
			|| !GameOpenRadioPcmInfrastructureCurrent(owner, committed.m_iEpoch)) return false;
		RICON_MediaStreamBinding source = point.m_Path.m_UpstreamRoute.m_Source;
		ref RICON_MediaCaptureAnchor anchor = owner.ReadCurrentCaptureAnchor(source, nowMs);
		ref RICON_MediaCaptureEnvelope first = RICON_MediaCaptureClock.Map(anchor, source, firstSampleIndex,
			RICON_MediaPcmTestProfile.FRAME_SAMPLES);
		if (!first || !history.Select(point.m_Path.m_UpstreamRoute, first, nowMs, false)) return false;
		RICON_OpenRadioPairEndpoint stationRx = new RICON_OpenRadioPairEndpoint();
		stationRx.m_Radio = point.m_RxRadio;
		stationRx.m_iGeneration = point.m_iRxGeneration;
		stationRx.m_vLocalAntennaOffset = point.m_vRxAntennaOffset;
		stationRx.m_Profile = RICON_OpenRadioPairProjector.CopyProfile(point.m_RxProfile);
		if (!GameRelayInputRfPointCurrent(committed, point, stationRx)) return false;
		ref RICON_RelayInputSeries series = new RICON_RelayInputSeries();
		if (!series.Configure(point.m_Path.m_UpstreamRoute, firstSampleIndex, anchor.m_sTimebaseId, anchor, history)
			|| m_GameRelayInputPcmPoint || owner != m_GameOpenRadioPcmOwner || point != m_GameRelayInputRfPoint
			|| !GameRelayInputRfPointCurrent(committed, point, stationRx)) return false;
		array<ref RICON_QueuedMediaBlock> backlog;
		if (!owner.CopyAdmittedPcmBacklog(source, firstSampleIndex, nowMs, backlog)) return false;
		if (!backlog.IsEmpty() && (!GetGame() || !GetGame().GetCallqueue())) return false;
		m_GameRelayInputPcmPoint = point;
		m_GameRelayInputPcmSeries = series;
		if (!backlog.IsEmpty())
		{
			m_GameRelayInputPcmBacklog = backlog;
			// Use fresh authority time only after the current RF publication has returned.
			GetGame().GetCallqueue().CallLater(PumpGameRelayInputPcmBacklog, 1);
		}
		return true;
	}

	protected void PumpGameRelayInputPcmBacklog()
	{
		ref array<ref RICON_QueuedMediaBlock> backlog = m_GameRelayInputPcmBacklog;
		m_GameRelayInputPcmBacklog = null;
		ref RICON_GameRelayInputRfPoint point = m_GameRelayInputPcmPoint;
		ref RICON_MediaStreamOwner owner = m_GameOpenRadioPcmOwner;
		if (!backlog || !point || !owner || !m_InventoryBridge) return;
		RICON_MediaStreamBinding source = point.m_Path.m_UpstreamRoute.m_Source;
		SCR_PlayerController sender = m_InventoryBridge.ResolveRemoteRecipient(source.m_iPlayerId,
			source.m_iConnectionId, source.m_iPlayerGeneration, source.m_iEntityId);
		foreach (RICON_QueuedMediaBlock accepted : backlog)
		{
			if (point != m_GameRelayInputPcmPoint || !m_GameRelayInputPcmSeries) break;
			PrepareGameRelayInputPcm(sender, accepted, owner, true);
		}
	}

	protected bool ReadGameRelayInputPcmNow(SCR_PlayerController sender, RICON_GameRelayInputRfPoint point,
		RICON_MediaStreamOwner owner, bool qualifyHeldOrigin, out int nowMs)
	{
		nowMs = -1;
		ref RICON_MediaReceiverAuthority receivers = m_InventoryReceivers;
		if (!point || point != m_GameRelayInputPcmPoint || point.m_Authority != m_RelayStationAuthority
			|| !m_GameRelayInputPcmSeries || !point.m_Budget || !receivers || !m_InventoryBridge) return false;
		RICON_MediaEgressRoute inlet = point.m_Path.m_UpstreamRoute;
		RICON_MediaStreamBinding source = inlet.m_Source;
		if (qualifyHeldOrigin)
		{
			if (!ReadGameRelayOrigin(sender, source, point.m_iTxGeneration, owner, nowMs)) return false;
		}
		else
		{
			// Ingress already qualified the held PTT. Recheck current identities and time without repeating the native inventory scan.
			nowMs = m_InventoryBridge.ReadWireAuthorityTime(m_InventoryLifecycle);
			if (!sender || m_InventoryBridge.ResolveRemoteRecipient(source.m_iPlayerId, source.m_iConnectionId,
				source.m_iPlayerGeneration, source.m_iEntityId) != sender) return false;
		}
		if (nowMs < point.m_Budget.m_iTickMs || nowMs >= point.m_iDeadlineMs) return false;
		if (!point.m_Authority.MatchesNativeInputRadio(point.m_Path, point.m_RxRadio)
			|| !point.m_Authority.CanUseInput(point.m_Path, nowMs)
			|| !receivers.MatchesRadio(source.m_iDeviceId, source.m_iTxTransceiverIndex, point.m_iTxGeneration, point.m_TxRadio)
			|| !receivers.MatchesRadio(inlet.m_iRxDeviceId, inlet.m_iRxTransceiverIndex, point.m_iRxGeneration, point.m_RxRadio))
			return false;
		if (receivers.ReadRecordedOpenEndpointAdmission(source.m_iDeviceId, source.m_iTxTransceiverIndex,
			point.m_iTxGeneration, source.m_iNetworkTypeId, source.m_iChannelId, point.m_TxProfile.m_iExpectedTuningValue, nowMs) != point.m_iTxAdmission
			|| receivers.ReadRecordedOpenEndpointAdmission(inlet.m_iRxDeviceId, inlet.m_iRxTransceiverIndex,
				point.m_iRxGeneration, source.m_iNetworkTypeId, source.m_iChannelId, point.m_RxProfile.m_iExpectedTuningValue, nowMs) != point.m_iRxAdmission)
			return false;
		return point == m_GameRelayInputPcmPoint && point.m_Authority == m_RelayStationAuthority
			&& owner == m_GameOpenRadioPcmOwner && receivers == m_InventoryReceivers
			&& GameOpenRadioPcmInfrastructureCurrent(owner, source.m_iSessionEpoch);
	}

	protected void PrepareGameRelayInputPcm(SCR_PlayerController sender, RICON_QueuedMediaBlock accepted,
		RICON_MediaStreamOwner owner, bool qualifyHeldOrigin)
	{
		ref RICON_GameRadioManagerComponent held = this;
		ref RICON_GameRelayInputRfPoint point = m_GameRelayInputPcmPoint;
		ref RICON_RelayInputSeries series = m_GameRelayInputPcmSeries;
		if (!point || !series || !accepted || !point.m_Path.m_UpstreamRoute.m_Source.Same(accepted.m_Binding)) return;
		int nowMs;
		string status = "REVOKED";
		RICON_RelayInputSeriesWindow prepared;
		if (ReadGameRelayInputPcmNow(sender, point, owner, qualifyHeldOrigin, nowMs))
		{
			RICON_LifecycleLease lease = m_InventoryLifecycle.QueryActiveLease(accepted.m_Binding.m_iLeaseId, nowMs);
			if (lease && lease.m_iIssuedAtMs == point.m_iIssuedAtMs
				&& accepted.m_iCaptureLowerMs <= int.MAX - RELAY_INPUT_MAX_AGE_MS)
			{
				int deadlineMs = Math.Min(point.m_iDeadlineMs, lease.EffectiveDeadlineMs());
				deadlineMs = Math.Min(deadlineMs, accepted.m_iCaptureLowerMs + RELAY_INPUT_MAX_AGE_MS);
				status = series.Process(point.m_Path.m_UpstreamRoute, accepted, nowMs, deadlineMs, prepared);
				if (status == "OK")
				{
					if (!prepared || !ReadGameRelayInputPcmNow(sender, point, owner, false, nowMs)
						|| series != m_GameRelayInputPcmSeries) status = "REVOKED";
					else if (nowMs >= prepared.m_iDeadlineMs) status = "EXPIRED";
				}
			}
		}
		if (status == "OK")
		{
			OnGameRelayInputPcmPrepared(sender, point.m_Path.ClonePath(), prepared, nowMs);
			return;
		}
		series.Close();
		if (m_GameRelayInputPcmSeries == series) m_GameRelayInputPcmSeries = null;
		OnGameRelayInputPcmRefused(sender, point.m_Path.ClonePath(), accepted.m_iSequence, status, nowMs);
	}

	//! Prepared station input PCM and its exact completed RF proof. A later output stage still needs its own permit.
	protected void OnGameRelayInputPcmPrepared(SCR_PlayerController sender, RICON_RelayPcmPath path,
		RICON_RelayInputSeriesWindow window, int nowMs) {}

	protected void OnGameRelayInputPcmRefused(SCR_PlayerController sender, RICON_RelayPcmPath path,
		int sequence, string reason, int nowMs) {}

	//! Trusted game scenario binding. The station uses the current carried PTT and its existing V3 media owner.
	RICON_RelayStationAuthority ConfigureGameRelayStationAuthority(SCR_PlayerController sender, RICON_RelayStationComponent station,
		RICON_RelayPcmPath path, RICON_RelayStationDirection direction)
	{
		if (m_bConfiguringGameRelayStation || m_RelayStationAuthority || !sender || !station || !path || !path.IsValid()
			|| !direction || !direction.IsValid()) return null;
		m_bConfiguringGameRelayStation = true;
		ref RICON_RelayStationAuthority authority = ConfigureGameRelayStationAuthorityHeld(sender, station, path, direction);
		m_bConfiguringGameRelayStation = false;
		return authority;
	}

	protected RICON_RelayStationAuthority ConfigureGameRelayStationAuthorityHeld(SCR_PlayerController sender,
		RICON_RelayStationComponent station, RICON_RelayPcmPath path, RICON_RelayStationDirection direction)
	{
		ref RICON_MediaStreamOwner owner = m_GameOpenRadioPcmOwner;
		ref RICON_LifecycleStateMachine lifecycle = m_InventoryLifecycle;
		ref RICON_GameControllerLifecycle bridge = m_InventoryBridge;
		ref RICON_MediaRpcTransport transport = m_MediaWireTransport;
		IEntity stationEntity = station.GetOwner();
		ref RICON_RelayPcmPath boundPath = path.ClonePath();
		RICON_MediaStreamBinding origin = boundPath.m_UpstreamRoute.m_Source;
		int txGeneration = direction.m_iOriginTxGeneration;
		int nowMs;
		if (!stationEntity || !ReadGameRelayOrigin(sender, origin, txGeneration, owner, nowMs)
			|| m_RelayStationAuthority || direction.m_iOriginTxGeneration != txGeneration) return null;

		ref RICON_RelayStationAuthority authority = super.ConfigureRelayStationAuthority(boundPath, direction, lifecycle, owner, station);
		if (!authority) return null;
		nowMs = bridge.ReadWireAuthorityTime(lifecycle);
		if (m_RelayStationAuthority == authority && m_RelayLifecycle == lifecycle && nowMs >= 0
			&& authority.CanUseInput(boundPath, nowMs) && authority.CanUseOutput(boundPath, nowMs)
			&& ReadGameRelayOrigin(sender, origin, txGeneration, owner, nowMs)
			&& m_RelayStationAuthority == authority && m_RelayLifecycle == lifecycle
			&& station.GetOwner() == stationEntity && direction.m_iOriginTxGeneration == txGeneration
			&& owner == m_GameOpenRadioPcmOwner && lifecycle == m_InventoryLifecycle && bridge == m_InventoryBridge
			&& transport == m_MediaWireTransport && GameOpenRadioPcmInfrastructureCurrent(owner, origin.m_iSessionEpoch))
			return authority;

		// A callback may have replaced the scope. Retire only the authority created by this call.
		if (m_RelayStationAuthority == authority && m_RelayLifecycle == lifecycle && !m_bRelayOutputInstalled)
		{
			m_RelayStationAuthority = null;
			m_RelayLifecycle = null;
		}
		authority.Close();
		return null;
	}

	protected bool ReadGameRelayOrigin(SCR_PlayerController sender, RICON_MediaStreamBinding origin, int txGeneration,
		RICON_MediaStreamOwner owner, out int nowMs)
	{
		nowMs = -1;
		int epoch = origin.m_iSessionEpoch;
		ref RICON_LifecycleStateMachine lifecycle = m_InventoryLifecycle;
		ref RICON_GameControllerLifecycle bridge = m_InventoryBridge;
		ref RICON_MediaRpcTransport transport = m_MediaWireTransport;
		if (epoch != m_iGameOpenRadioPcmEpoch || owner != m_GameOpenRadioPcmOwner
			|| !GameOpenRadioPcmInfrastructureCurrent(owner, epoch)) return false;
		ref RICON_GameRadioPttEmission emission = ReadGameRadioEmission(sender, origin.m_iLeaseId);
		if (!emission || !emission.m_Context || emission.m_iLeaseId != origin.m_iLeaseId || emission.m_iTxGeneration != txGeneration
			|| emission.m_iDeviceId != origin.m_iDeviceId || emission.m_iTxTransceiverIndex != origin.m_iTxTransceiverIndex
			|| emission.m_iNetworkId != origin.m_iNetworkTypeId || emission.m_iChannelId != origin.m_iChannelId) return false;
		RICON_GameRadioPttContext context = emission.m_Context;
		if (context.m_iEpoch != epoch || context.m_iPlayerId != origin.m_iPlayerId || context.m_iConnectionId != origin.m_iConnectionId
			|| context.m_iGeneration != origin.m_iPlayerGeneration || context.m_iEntityId != origin.m_iEntityId) return false;

		int observedMs = bridge.ReadWireAuthorityTime(lifecycle);
		if (!RICON_OpenRadioPairProjector.BindGameEmission(lifecycle, emission, observedMs)) return false;
		ref RICON_MediaStreamBinding current = owner.ReadCurrentSource(epoch, origin.m_iLeaseId, observedMs);
		if (!origin.Same(current) || owner != m_GameOpenRadioPcmOwner || lifecycle != m_InventoryLifecycle
			|| bridge != m_InventoryBridge || transport != m_MediaWireTransport
			|| !GameOpenRadioPcmInfrastructureCurrent(owner, epoch)) return false;
		nowMs = observedMs;
		return true;
	}

	//! Record only a completed first-leg interval supplied by consecutive real game RF callbacks.
	string ObserveGameRelayInputRfCoverage(RICON_RelayStationAuthority authority, RICON_RelayPcmPath path,
		RICON_OpenRadioPairEndpoint stationRx, RICON_MediaRfHistory history, int nowMs)
	{
		if (m_bObservingGameRelayRf)
		{
			m_bGameRelayRfCancelled = true;
			return "REFUSED";
		}
		m_bObservingGameRelayRf = true;
		m_bGameRelayRfCancelled = false;
		string result = ObserveGameRelayInputRfCoverageHeld(authority, path, stationRx, history, nowMs);
		if (result == "REFUSED") m_GameRelayInputRfPoint = null;
		m_bObservingGameRelayRf = false;
		return result;
	}

	protected string ObserveGameRelayInputRfCoverageHeld(RICON_RelayStationAuthority authority, RICON_RelayPcmPath path,
		RICON_OpenRadioPairEndpoint stationRx, RICON_MediaRfHistory history, int nowMs)
	{
		ref RICON_GameRadioRfCommit committed = m_GameRfCommit;
		ref RICON_GameRelayInputRfPoint previous = m_GameRelayInputRfPoint;
		m_GameRelayInputRfPoint = null;
		if (!GameRfCommitCurrent(committed, nowMs) || !authority || authority != m_RelayStationAuthority
			|| !path || !path.IsValid() || !stationRx || !stationRx.m_Profile || !history
			|| !committed.m_Profile.m_Radio || committed.m_iPositionReadStartedMs < 0) return "REFUSED";
		RICON_MediaStreamBinding source = path.m_UpstreamRoute.m_Source;
		if (m_RelayLifecycle != committed.m_Lifecycle || source.m_iSessionEpoch != committed.m_iEpoch
			|| !GameOpenRadioPcmInfrastructureCurrent(m_GameOpenRadioPcmOwner, committed.m_iEpoch)
			|| !source.Same(m_GameOpenRadioPcmOwner.ReadCurrentSource(committed.m_iEpoch, source.m_iLeaseId, nowMs))) return "REFUSED";
		RICON_RadioQueryContext context = committed.m_Tick.m_PairInput.m_Context;
		if (!context || context.m_sModelId != RICON_FreeSpaceModel.MODEL_ID || context.m_sNoiseEnvironmentId != "none"
			|| context.m_bHasInterference || committed.m_Tick.m_PairInput.m_bUseRxEnvironments) return "REFUSED";
		RICON_WirelessLinkEvaluator evaluator = GetBandEvaluator(context.m_sBandId);
		if (!evaluator || !evaluator.UsesFixedRfModels()) return "REFUSED";
		RICON_GameRadioPttEmission emission;
		foreach (RICON_GameRadioPttEmission candidate : committed.m_Emissions)
		{
			if (!candidate || candidate.m_iLeaseId != source.m_iLeaseId) continue;
			if (emission) return "REFUSED";
			emission = candidate;
		}
		if (!RICON_OpenRadioPairProjector.BindGameEmission(committed.m_Lifecycle, emission, nowMs)
			|| !emission.m_Context || !GameRelayInputSourceMatches(source, emission)) return "REFUSED";
		RICON_GameRadioOpenSlot txSlot;
		foreach (RICON_GameRadioOpenSlot slot : committed.m_Slots)
		{
			if (slot && slot.m_Listener && MatchesGameRfSlot(slot.m_Listener, source.m_iDeviceId, source.m_iTxTransceiverIndex,
				emission.m_iTxGeneration, source.m_iPlayerId, source.m_iPlayerGeneration, source.m_iConnectionId, source.m_iEntityId))
			{
				if (txSlot) return "REFUSED";
				txSlot = slot;
			}
		}
		RICON_PairEndpoint tx;
		foreach (RICON_PairEndpoint endpoint : committed.m_Tick.m_PairInput.m_aEndpoints)
		{
			if (endpoint && endpoint.m_Observation && endpoint.m_Observation.m_iDeviceId == source.m_iDeviceId
				&& endpoint.m_Observation.m_iTransceiverIndex == source.m_iTxTransceiverIndex)
			{
				if (tx) return "REFUSED";
				tx = endpoint;
			}
		}
		if (!txSlot || txSlot.m_iAdmission <= 0 || !tx || !tx.m_Profile
			|| !authority.MatchesNativeInputRadio(path, stationRx.m_Radio)) return "REFUSED";
		RICON_PairEndpoint rx = ObserveRelayEndpoint(stationRx);
		int stationReadMs = System.GetTickCount();
		if (!rx || stationReadMs < committed.m_iPositionReadStartedMs || stationRx.m_iGeneration <= 0
			|| stationRx.m_Profile.m_iDeviceId != path.m_UpstreamRoute.m_iRxDeviceId
			|| stationRx.m_Profile.m_iTransceiverIndex != path.m_UpstreamRoute.m_iRxTransceiverIndex
			|| stationRx.m_iGeneration != path.m_UpstreamRoute.m_iRxGeneration
			|| !authority.MatchesInputEndpoints(path, tx.m_Observation, emission.m_iTxGeneration, rx.m_Observation, stationRx.m_iGeneration))
			return "REFUSED";
		RICON_LinkQuery query;
		if (RICON_RadioQueryBuilder.Build(tx.m_Observation, tx.m_Profile, rx.m_Observation, rx.m_Profile, context, query)
			!= RICON_ERadioQueryCode.READY) return "REFUSED";
		RICON_LinkBudget evaluated = evaluator.Evaluate(query);
		float distanceM = vector.Distance(tx.m_Observation.m_vAntennaPos, rx.m_Observation.m_vAntennaPos);
		if (!evaluated || !evaluated.m_bValid || !RICON_Num.IsFinite(distanceM) || distanceM <= 0
			|| !RICON_MediaWireRfV2.ValidPowerDbm(evaluated.m_fRxPowerDbm) || !RICON_MediaWireRfV2.ValidPowerDbm(evaluated.m_fNoiseDbm)
			|| query.m_fBandwidthHz < 1 || query.m_fBandwidthHz > RICON_MediaWireRfV2.MAX_BANDWIDTH_HZ
			|| query.m_fBandwidthHz != Math.Floor(query.m_fBandwidthHz)) return "REFUSED";

		ref RICON_GameRelayInputRfPoint point = new RICON_GameRelayInputRfPoint();
		point.m_iAttempt = committed.m_iAttempt;
		point.m_iSkewMs = stationReadMs - committed.m_iPositionReadStartedMs;
		point.m_iTxGeneration = emission.m_iTxGeneration; point.m_iRxGeneration = stationRx.m_iGeneration;
		point.m_iTxAdmission = txSlot.m_iAdmission;
		point.m_iRxAdmission = committed.m_Receivers.ReadRecordedOpenEndpointAdmission(path.m_UpstreamRoute.m_iRxDeviceId,
			path.m_UpstreamRoute.m_iRxTransceiverIndex, stationRx.m_iGeneration, source.m_iNetworkTypeId, source.m_iChannelId,
			tx.m_Observation.m_iTuningValue, nowMs);
		point.m_iIssuedAtMs = emission.m_iIssuedAtMs; point.m_iDeadlineMs = emission.m_iDeadlineMs;
		point.m_fDistanceM = distanceM; point.m_TxRadio = txSlot.m_Radio; point.m_RxRadio = stationRx.m_Radio;
		point.m_vRxAntennaOffset = stationRx.m_vLocalAntennaOffset;
		point.m_Path = path.ClonePath(); point.m_Context = emission.m_Context.CloneContext();
		point.m_TxProfile = RICON_OpenRadioPairProjector.CopyProfile(tx.m_Profile);
		point.m_RxProfile = RICON_OpenRadioPairProjector.CopyProfile(rx.m_Profile);
		point.m_sBandId = context.m_sBandId; point.m_sIntelligibilityModelId = context.m_sIntelligibilityModelId;
		point.m_Authority = authority; point.m_History = history;
		point.m_Budget = new RICON_TemporalRfBudget();
		point.m_Budget.m_iLinkId = path.m_iRelayEmissionId;
		point.m_Budget.m_iTickMs = nowMs; point.m_Budget.m_iModelMs = nowMs;
		point.m_Budget.m_sTimebaseId = committed.m_Tick.m_sTimebaseId;
		point.m_Budget.m_fRxPowerDbm = evaluated.m_fRxPowerDbm; point.m_Budget.m_fNoiseDbm = evaluated.m_fNoiseDbm;
		point.m_Budget.m_fBandwidthHz = query.m_fBandwidthHz;
		if (!GameRelayInputRfPointCurrent(committed, point, stationRx)) return "REFUSED";
		if (!previous)
		{
			m_GameRelayInputRfPoint = point;
			return "SEEDED";
		}
		if (previous.m_iAttempt + 1 != point.m_iAttempt || previous.m_Budget.m_iTickMs >= nowMs
			|| nowMs >= previous.m_iDeadlineMs || !GameRelayInputRfScopeSame(previous, point)) return "REFUSED";
		RICON_MediaFreeSpaceMotionLimits limits = ReadAuthoredGameRelayInputRfCoverageLimits(point.m_Path.ClonePath(),
			previous.m_Budget.m_iTickMs, nowMs);
		if (!limits || point.m_iSkewMs > limits.m_iMaxSampleTimeErrorMs || previous.m_iSkewMs > limits.m_iMaxSampleTimeErrorMs
			|| limits.m_fFrequencyMHz != point.m_TxProfile.m_fFrequencyMHz
			|| !GameRelayInputRfPointCurrent(committed, point, stationRx)) return "REFUSED";
		if (!history.RecordCompletedFreeSpace(point.m_Path.m_UpstreamRoute, previous.m_Budget, point.m_Budget,
			previous.m_fDistanceM, point.m_fDistanceM, limits, nowMs)) return "REFUSED";
		m_GameRelayInputRfPoint = point;
		return "COMPLETED";
	}

	protected RICON_MediaFreeSpaceMotionLimits ReadAuthoredGameRelayInputRfCoverageLimits(
		RICON_RelayPcmPath path, int fromMs, int untilMs) { return null; }

	protected bool GameRelayInputRfPointCurrent(RICON_GameRadioRfCommit committed, RICON_GameRelayInputRfPoint point,
		RICON_OpenRadioPairEndpoint stationRx)
	{
		int nowMs = point.m_Budget.m_iTickMs;
		if (m_bGameRelayRfCancelled || !GameRfCommitCurrent(committed, nowMs) || !committed.m_Profile.m_Radio) return false;
		RICON_MediaStreamBinding source = point.m_Path.m_UpstreamRoute.m_Source;
		RICON_LifecycleLease lease = committed.m_Lifecycle.QueryActiveLease(source.m_iLeaseId, nowMs);
		RICON_RadioQueryContext context = committed.m_Tick.m_PairInput.m_Context;
		if (!lease || !lease.MatchesSource(source.m_iPlayerId, source.m_iPlayerGeneration, source.m_iDeviceId,
			source.m_iNetworkTypeId, source.m_iChannelId) || lease.m_iIssuedAtMs != point.m_iIssuedAtMs
			|| lease.EffectiveDeadlineMs() != point.m_iDeadlineMs || nowMs >= point.m_iDeadlineMs || !context
			|| context.m_sBandId != point.m_sBandId || context.m_sIntelligibilityModelId != point.m_sIntelligibilityModelId
			|| context.m_sModelId != RICON_FreeSpaceModel.MODEL_ID || context.m_sNoiseEnvironmentId != "none"
			|| context.m_bHasInterference || point.m_Context.m_iPolicyRevision != GetInventoryPolicyRevision()) return false;
		RICON_RadioRfProfile txProfile = ReadGameEndpointProfile(committed.m_Profile, point.m_TxRadio,
			source.m_iDeviceId, source.m_iTxTransceiverIndex);
		if (!GameRelayRfProfilesSame(txProfile, point.m_TxProfile)) return false;
		if (m_bGameRelayRfCancelled || !GameRfCommitCurrent(committed, nowMs)
			|| point.m_Authority != m_RelayStationAuthority || m_RelayLifecycle != committed.m_Lifecycle
			|| !GameOpenRadioPcmInfrastructureCurrent(m_GameOpenRadioPcmOwner, committed.m_iEpoch)) return false;
		if (!source.Same(m_GameOpenRadioPcmOwner.ReadCurrentSource(committed.m_iEpoch, source.m_iLeaseId, nowMs))
			|| !m_InventoryBridge.ResolveRemoteRecipient(source.m_iPlayerId, source.m_iConnectionId,
				source.m_iPlayerGeneration, source.m_iEntityId)) return false;
		if (stationRx.m_Radio != point.m_RxRadio || stationRx.m_iGeneration != point.m_iRxGeneration
			|| stationRx.m_vLocalAntennaOffset != point.m_vRxAntennaOffset
			|| !GameRelayRfProfilesSame(stationRx.m_Profile, point.m_RxProfile)) return false;
		if (!point.m_Authority.MatchesNativeInputRadio(point.m_Path, point.m_RxRadio)
			|| !point.m_Authority.CanUseInput(point.m_Path, nowMs)
			|| point.m_iTxAdmission <= 0 || point.m_iRxAdmission <= 0) return false;
		RICON_MediaEgressRoute inlet = point.m_Path.m_UpstreamRoute;
		if (!committed.m_Receivers.MatchesRadio(source.m_iDeviceId, source.m_iTxTransceiverIndex,
			point.m_iTxGeneration, point.m_TxRadio) || !committed.m_Receivers.MatchesRadio(inlet.m_iRxDeviceId,
			inlet.m_iRxTransceiverIndex, point.m_iRxGeneration, point.m_RxRadio)) return false;
		if (committed.m_Receivers.ReadRecordedOpenEndpointAdmission(source.m_iDeviceId, source.m_iTxTransceiverIndex,
			point.m_iTxGeneration, source.m_iNetworkTypeId, source.m_iChannelId,
			point.m_TxProfile.m_iExpectedTuningValue, nowMs) != point.m_iTxAdmission) return false;
		if (committed.m_Receivers.ReadRecordedOpenEndpointAdmission(inlet.m_iRxDeviceId, inlet.m_iRxTransceiverIndex,
			point.m_iRxGeneration, source.m_iNetworkTypeId, source.m_iChannelId,
			point.m_RxProfile.m_iExpectedTuningValue, nowMs) != point.m_iRxAdmission) return false;
		return !m_bGameRelayRfCancelled && GameRfCommitCurrent(committed, nowMs);
	}

	protected static bool GameRelayInputSourceMatches(RICON_MediaStreamBinding source, RICON_GameRadioPttEmission emission)
	{
		RICON_GameRadioPttContext context = emission.m_Context;
		return context.m_iEpoch == source.m_iSessionEpoch && context.m_iPlayerId == source.m_iPlayerId
			&& context.m_iConnectionId == source.m_iConnectionId && context.m_iGeneration == source.m_iPlayerGeneration
			&& context.m_iEntityId == source.m_iEntityId && emission.m_iDeviceId == source.m_iDeviceId
			&& emission.m_iTxTransceiverIndex == source.m_iTxTransceiverIndex && emission.m_iNetworkId == source.m_iNetworkTypeId
			&& emission.m_iChannelId == source.m_iChannelId;
	}

	protected static bool GameRelayInputRfScopeSame(RICON_GameRelayInputRfPoint first, RICON_GameRelayInputRfPoint second)
	{
		if (first.m_Authority != second.m_Authority || first.m_History != second.m_History
			|| !first.m_Path.SamePath(second.m_Path) || !first.m_Context.Same(second.m_Context)) return false;
		if (first.m_TxRadio != second.m_TxRadio || first.m_RxRadio != second.m_RxRadio
			|| first.m_iTxGeneration != second.m_iTxGeneration || first.m_iRxGeneration != second.m_iRxGeneration
			|| first.m_iTxAdmission != second.m_iTxAdmission || first.m_iRxAdmission != second.m_iRxAdmission) return false;
		return first.m_iIssuedAtMs == second.m_iIssuedAtMs && first.m_vRxAntennaOffset == second.m_vRxAntennaOffset
			&& first.m_sBandId == second.m_sBandId && first.m_sIntelligibilityModelId == second.m_sIntelligibilityModelId
			&& GameRelayRfProfilesSame(first.m_TxProfile, second.m_TxProfile)
			&& GameRelayRfProfilesSame(first.m_RxProfile, second.m_RxProfile);
	}

	protected static bool GameRelayRfProfilesSame(RICON_RadioRfProfile first, RICON_RadioRfProfile second)
	{
		if (!first || !second || first.m_sProfileId != second.m_sProfileId || first.m_sProvenance != second.m_sProvenance
			|| first.m_iDeviceId != second.m_iDeviceId || first.m_iTransceiverIndex != second.m_iTransceiverIndex) return false;
		if (first.m_iExpectedTuningValue != second.m_iExpectedTuningValue || first.m_fFrequencyMHz != second.m_fFrequencyMHz
			|| first.m_fRxBandwidthHz != second.m_fRxBandwidthHz || first.m_fTxPowerDbm != second.m_fTxPowerDbm
			|| first.m_fTxGainDbi != second.m_fTxGainDbi || first.m_fRxGainDbi != second.m_fRxGainDbi) return false;
		return first.m_fRxNoiseFigureDb == second.m_fRxNoiseFigureDb && first.m_bHasRxSensitivity == second.m_bHasRxSensitivity
			&& first.m_fRxSensitivityDbm == second.m_fRxSensitivityDbm && first.m_bHasRequiredSinr == second.m_bHasRequiredSinr
			&& first.m_fRequiredSinrDb == second.m_fRequiredSinrDb;
	}

	bool RICON_AcceptTs3RouteQuery(SCR_PlayerController sender, RICON_GameTs3RouteQuery query, out int now)
	{
		now = -1;
		if (!RICON_TS3GameBridge.RouteRequested() || !IsAuthority() || !sender || !query || !query.IsValid()) return false;
		ref RICON_GameRadioPttNativeSource source = m_InventoryPttSource;
		ref RICON_LifecycleStateMachine lifecycle = m_InventoryLifecycle;
		if (!source || !lifecycle) return false;
		RICON_GameRadioPttContext current = source.ReadContext(new RICON_GameRadioPttNativeCaller(sender));
		now = source.ReadNowMs();
		if (!current || !current.Same(query.m_Context) || now < 0 || source != m_InventoryPttSource || lifecycle != m_InventoryLifecycle) return false;
		RICON_LifecycleLease lease = lifecycle.QueryActiveLease(query.m_iLeaseId, now);
		return lease && lease.m_iPlayerId == current.m_iPlayerId && lease.m_iGeneration == current.m_iGeneration
			&& lease.m_iStartCorrelationId == query.m_iHoldToken;
	}

	RICON_GameTs3RecipientSnapshot RICON_ReadTs3RouteQuery(SCR_PlayerController sender, RICON_GameTs3RouteQuery query, int now)
	{
		if (!query || !query.IsValid() || !m_InventoryLifecycle || !RICON_TS3GameBridge.RouteRequested()) return null;
		RICON_LifecycleLease lease = m_InventoryLifecycle.QueryActiveLease(query.m_iLeaseId, now);
		if (!lease || lease.m_iPlayerId != query.m_Context.m_iPlayerId || lease.m_iGeneration != query.m_Context.m_iGeneration
			|| lease.m_iStartCorrelationId != query.m_iHoldToken) return null;
		return ReadGameTs3Recipients(sender, query.m_iLeaseId, now);
	}

	protected void RICON_PublishTs3RoutePulls(RICON_GameRadioRfCommit committed)
	{
		if (!committed || !RICON_TS3GameBridge.RouteRequested() || !GameRfCommitCurrent(committed, committed.m_iTickMs)) return;
		foreach (RICON_GameRadioPttEmission emission : committed.m_Emissions)
		{
			if (!emission || !emission.m_Context) continue;
			RICON_GameRadioPttContext context = emission.m_Context;
			SCR_PlayerController sender = m_InventoryBridge.ResolveRemoteRecipient(context.m_iPlayerId,
				context.m_iConnectionId, context.m_iGeneration, context.m_iEntityId);
			if (sender) sender.RICON_FulfillTs3Route(this, emission.m_iLeaseId, committed.m_iTickMs);
			if (!GameRfCommitCurrent(committed, committed.m_iTickMs)) return;
		}
	}

	//! Metadata for the held source during the committed server RF callback. No media permission is issued here.
	RICON_GameTs3RecipientSnapshot ReadGameTs3Recipients(SCR_PlayerController sender, int leaseId, int nowMs)
	{
		ref RICON_GameRadioRfCommit committed = m_GameRfCommit;
		ref RICON_GameTs3BindingStore store = m_InventoryTs3Bindings;
		ref RICON_GameControllerLifecycle bridge = m_InventoryBridge;
		ref RICON_GameRadioControlPolicy policy = m_InventoryControlPolicy;
		if (!sender || leaseId <= 0 || !store || !bridge || !policy || !GameRfCommitCurrent(committed, nowMs)) return null;
		if (!committed.m_Profile.m_Radio || !committed.m_Profile.m_Limits || !committed.m_Emissions || !committed.m_Slots
			|| committed.m_Result.m_aActiveIds.Count() > committed.m_Profile.m_Limits.m_iMaxPairs) return null;

		RICON_GameRadioPttEmission emission;
		foreach (RICON_GameRadioPttEmission captured : committed.m_Emissions)
		{
			if (!captured || captured.m_iLeaseId != leaseId) continue;
			if (emission) return null;
			emission = captured;
		}
		ref RICON_TransmissionBinding tx = RICON_OpenRadioPairProjector.BindGameEmission(committed.m_Lifecycle, emission, nowMs);
		if (!tx) return null;
		RICON_GameRadioPttContext source = emission.m_Context;
		string domain = GetInventoryPublicDomain();
		int rawTune = committed.m_Profile.m_Radio.m_iExpectedTuningValue;
		if (source.m_sDomain != domain || source.m_iEpoch != committed.m_iEpoch
			|| source.m_iPolicyRevision != policy.Revision() || tx.m_iNetworkTypeId != policy.NetworkId()
			|| tx.m_iChannelId != policy.ReadChannel(committed.m_iEpoch, rawTune, "")) return null;
		int connectionId, generation, entityId;
		if (sender.GetPlayerId() != source.m_iPlayerId || !bridge.ReadRemoteSender(sender, connectionId, generation, entityId)
			|| connectionId != source.m_iConnectionId || generation != source.m_iGeneration || entityId != source.m_iEntityId) return null;

		map<string, ref RICON_GameRadioOpenSlot> slots = new map<string, ref RICON_GameRadioOpenSlot>();
		foreach (RICON_GameRadioOpenSlot slot : committed.m_Slots)
		{
			if (!slot || !slot.m_Listener) return null;
			string key = GameTs3SlotKey(slot.m_Listener.m_iDeviceId, slot.m_Listener.m_iIndex);
			if (slots.Contains(key)) return null;
			slots.Set(key, slot);
		}
		RICON_GameRadioOpenSlot txSlot = slots.Get(GameTs3SlotKey(tx.m_iTxDeviceId, tx.m_iTxTransceiverIndex));
		if (!txSlot || !MatchesGameRfSlot(txSlot.m_Listener, tx.m_iTxDeviceId, tx.m_iTxTransceiverIndex, emission.m_iTxGeneration,
			source.m_iPlayerId, generation, connectionId, entityId)
			|| !GameTs3SlotCurrent(committed, txSlot, tx, rawTune, nowMs)) return null;

		RICON_OpenRadioListenerRoster roster = committed.m_Tick.m_ListenerRoster;
		if (!roster || !roster.MatchesInput(committed.m_Tick.m_PairInput, committed.m_Lifecycle)) return null;
		array<ref RICON_GameRadioPttContext> admitted = {};
		array<ref RICON_GameRadioOpenSlot> admittedSlots = {};
		set<int> listeners = new set<int>();
		foreach (int linkId : committed.m_Result.m_aActiveIds)
		{
			RICON_TemporalExpected expected = committed.m_Result.m_ExpectedByLink.Get(linkId);
			if (!expected || !expected.m_Binding) return null;
			RICON_TemporalBinding pair = expected.m_Binding;
			if (pair.m_iLeaseId != leaseId) continue;
			if (expected.m_iLinkId != linkId || expected.m_iSessionEpoch != source.m_iEpoch
				|| expected.m_iTalkerPlayerId != source.m_iPlayerId || expected.m_sMode != "SERVER"
				|| expected.m_sTimebaseId != committed.m_Tick.m_sTimebaseId
				|| pair.m_iTxGeneration != tx.m_iTxGeneration || pair.m_iTxDeviceId != tx.m_iTxDeviceId
				|| pair.m_iTxTransceiverIndex != tx.m_iTxTransceiverIndex || pair.m_iNetworkTypeId != tx.m_iNetworkTypeId
				|| pair.m_iChannelId != tx.m_iChannelId) return null;
			ref RICON_TemporalSnapshot active = committed.m_Publication.ReadActive(linkId);
			if (!active || active.m_iRouteNodeCount != 2 || active.m_iSessionEpoch != source.m_iEpoch
				|| active.m_iTalkerPlayerId != source.m_iPlayerId || active.m_iListenerPlayerId != expected.m_iListenerPlayerId
				|| active.m_sTimebaseId != expected.m_sTimebaseId || !pair.Same(active.m_Binding)) return null;
			if (active.m_eState == RICON_ELinkState.LOST) continue;

			RICON_GameRadioOpenSlot rxSlot = slots.Get(GameTs3SlotKey(pair.m_iRxDeviceId, pair.m_iRxTransceiverIndex));
			if (!rxSlot) return null;
			RICON_OpenRadioPairListener row = rxSlot.m_Listener;
			if (row.m_iPlayerId != expected.m_iListenerPlayerId || row.m_iPlayerId == source.m_iPlayerId
				|| row.m_iPlayerGeneration != pair.m_iRxGeneration || row.m_iConnectionId == source.m_iConnectionId
				|| !roster.AllowsDirectCaptureListener(row.m_iPlayerId, row.m_iDeviceId, row.m_iIndex, nowMs, row.m_iGeneration)
				|| !bridge.ResolveRemoteRecipient(row.m_iPlayerId, row.m_iConnectionId, row.m_iPlayerGeneration, row.m_iEntityId)
				|| !GameTs3SlotCurrent(committed, rxSlot, tx, rawTune, nowMs)) return null;
			if (listeners.Contains(row.m_iPlayerId)) continue;
			if (admitted.Count() >= m_iInventoryMaxPlayers) return null;
			listeners.Insert(row.m_iPlayerId);
			RICON_GameRadioPttContext receiver = source.CloneContext();
			receiver.m_iPlayerId = row.m_iPlayerId;
			receiver.m_iConnectionId = row.m_iConnectionId;
			receiver.m_iGeneration = row.m_iPlayerGeneration;
			receiver.m_iEntityId = row.m_iEntityId;
			admitted.Insert(receiver);
			admittedSlots.Insert(rxSlot);
		}
		ref RICON_GameTs3RecipientSnapshot snapshot = RICON_GameTs3RecipientSnapshot.Build(store, emission, admitted, nowMs);
		if (!snapshot || bridge != m_InventoryBridge || store != m_InventoryTs3Bindings || policy != m_InventoryControlPolicy
			|| source.m_iPolicyRevision != policy.Revision()
			|| !RICON_OpenRadioPairProjector.BindGameEmission(committed.m_Lifecycle, emission, nowMs)
			|| !GameTs3SlotCurrent(committed, txSlot, tx, rawTune, nowMs)) return null;
		foreach (RICON_GameRadioOpenSlot admittedSlot : admittedSlots)
			if (!GameTs3SlotCurrent(committed, admittedSlot, tx, rawTune, nowMs)) return null;
		if (!GameRfCommitCurrent(committed, nowMs)) return null;
		return snapshot;
	}

	protected static string GameTs3SlotKey(int device, int index) { return string.Format("%1:%2", device, index); }

	protected static bool GameTs3SlotCurrent(RICON_GameRadioRfCommit committed, RICON_GameRadioOpenSlot slot,
		RICON_TransmissionBinding tx, int rawTune, int nowMs)
	{
		RICON_OpenRadioPairListener row = slot.m_Listener;
		return slot.m_iAdmission > 0 && committed.m_Receivers.MatchesRadio(row.m_iDeviceId, row.m_iIndex, row.m_iGeneration, slot.m_Radio)
			&& committed.m_Receivers.ReadRecordedOpenEndpointAdmission(row.m_iDeviceId, row.m_iIndex, row.m_iGeneration,
				tx.m_iNetworkTypeId, tx.m_iChannelId, rawTune, nowMs) == slot.m_iAdmission;
	}

	RICON_GameRadioRfPoint ReadGameRfCoveragePoint(RICON_MediaEgressRoute route, int linkId, int nowMs, RICON_TemporalPublicationOwner publication)
	{
		ref RICON_GameRadioRfCommit committed = m_GameRfCommit;
		if (!GameRfCommitCurrent(committed, nowMs) || publication != committed.m_Publication || !route || !route.IsValid()) return null;
		RICON_TemporalProducerTick tick = committed.m_Tick;
		RICON_PairServiceInput input = tick.m_PairInput;
		RICON_RadioQueryContext context = input.m_Context;
		if (!context || context.m_sModelId != RICON_FreeSpaceModel.MODEL_ID || context.m_sNoiseEnvironmentId != "none"
			|| context.m_bHasInterference || tick.m_sMode != "SERVER") return null;
		RICON_WirelessLinkEvaluator evaluator = GetBandEvaluator(context.m_sBandId);
		if (!evaluator || !evaluator.UsesFixedRfModels() || !publication.MatchesCurrentMediaLink(linkId, route, committed.m_Lifecycle, nowMs)) return null;
		RICON_TemporalRfBudget budget = publication.ReadMediaRfBudget(linkId, route, committed.m_Lifecycle, nowMs);
		int incarnation = publication.GetMediaListenerIncarnation(route);
		if (!budget || budget.m_iTickMs != nowMs || budget.m_iModelMs != nowMs || budget.m_iModelSubUs != 0
			|| budget.m_RxEnvironment || budget.m_sTimebaseId != tick.m_sTimebaseId || incarnation <= 0) return null;

		RICON_MediaStreamBinding source = route.m_Source;
		RICON_GameRadioPttEmission emission;
		foreach (RICON_GameRadioPttEmission captured : committed.m_Emissions)
		{
			if (captured && captured.m_iLeaseId == source.m_iLeaseId) { emission = captured; break; }
		}
		string domain = GetInventoryPublicDomain();
		if (!emission || !emission.m_Context || emission.m_Context.m_iEpoch != source.m_iSessionEpoch
			|| emission.m_Context.m_sDomain != domain || emission.m_Context.m_iPlayerId != source.m_iPlayerId
			|| emission.m_Context.m_iGeneration != source.m_iPlayerGeneration || emission.m_Context.m_iConnectionId != source.m_iConnectionId
			|| emission.m_Context.m_iEntityId != source.m_iEntityId || emission.m_iDeviceId != source.m_iDeviceId
			|| emission.m_iTxTransceiverIndex != source.m_iTxTransceiverIndex || emission.m_iNetworkId != source.m_iNetworkTypeId
			|| emission.m_iChannelId != source.m_iChannelId || emission.m_iServerNowMs > nowMs) return null;

		RICON_GameRadioOpenSlot txSlot, rxSlot;
		foreach (RICON_GameRadioOpenSlot slot : committed.m_Slots)
		{
			if (!slot || !slot.m_Listener) continue;
			RICON_OpenRadioPairListener row = slot.m_Listener;
			if (MatchesGameRfSlot(row, source.m_iDeviceId, source.m_iTxTransceiverIndex, emission.m_iTxGeneration,
				source.m_iPlayerId, source.m_iPlayerGeneration, source.m_iConnectionId, source.m_iEntityId)) txSlot = slot;
			if (MatchesGameRfSlot(row, route.m_iRxDeviceId, route.m_iRxTransceiverIndex, route.m_iRxGeneration,
				route.m_iListenerPlayerId, route.m_iListenerGeneration, route.m_iListenerConnectionId, route.m_iListenerEntityId)) rxSlot = slot;
		}
		if (!txSlot || !rxSlot || txSlot.m_iAdmission <= 0 || rxSlot.m_iAdmission <= 0) return null;

		RICON_PairEndpoint tx, rx;
		foreach (RICON_PairEndpoint endpoint : input.m_aEndpoints)
		{
			if (!endpoint || !endpoint.m_Observation) continue;
			RICON_RadioObservation observation = endpoint.m_Observation;
			if (observation.m_iDeviceId == source.m_iDeviceId && observation.m_iTransceiverIndex == source.m_iTxTransceiverIndex) tx = endpoint;
			if (observation.m_iDeviceId == route.m_iRxDeviceId && observation.m_iTransceiverIndex == route.m_iRxTransceiverIndex) rx = endpoint;
		}
		if (!tx || !rx || !tx.m_Profile || !rx.m_Profile || !committed.m_Profile.m_Radio) return null;
		float frequencyMHz = committed.m_Profile.m_Radio.m_fFrequencyMHz;
		int rawTune = committed.m_Profile.m_Radio.m_iExpectedTuningValue;
		float distanceM = vector.Distance(tx.m_Observation.m_vAntennaPos, rx.m_Observation.m_vAntennaPos);
		if (!RICON_Num.IsFinite(distanceM) || distanceM <= 0 || !RICON_Num.IsFinite(frequencyMHz) || frequencyMHz <= 0
			|| tx.m_Profile.m_fFrequencyMHz != frequencyMHz || rx.m_Profile.m_fFrequencyMHz != frequencyMHz
			|| tx.m_Observation.m_iTuningValue != rawTune || rx.m_Observation.m_iTuningValue != rawTune) return null;

		RICON_GameRadioRfPoint point = new RICON_GameRadioRfPoint();
		point.m_iAttempt = committed.m_iAttempt; point.m_iTickMs = nowMs; point.m_iDeadlineMs = emission.m_iDeadlineMs;
		point.m_iTxGeneration = emission.m_iTxGeneration; point.m_iTxAdmission = txSlot.m_iAdmission; point.m_iRxAdmission = rxSlot.m_iAdmission;
		point.m_fDistanceM = distanceM; point.m_iLinkId = linkId; point.m_iIssuedAtMs = emission.m_iIssuedAtMs;
		point.m_iListenerIncarnation = incarnation; point.m_iRawTune = rawTune; point.m_fFrequencyMHz = frequencyMHz;
		point.m_Route = route.CloneRoute(); point.m_Context = emission.m_Context.CloneContext();
		if (!GameRfPointCurrent(committed, route, point)) return null;
		if (!m_CurrentGameRfPoints) m_CurrentGameRfPoints = new map<int, ref RICON_GameRadioRfPoint>();
		RICON_GameRadioRfPoint recorded = m_CurrentGameRfPoints.Get(linkId);
		if (recorded && !recorded.SamePoint(point)) return null;
		if (!recorded) m_CurrentGameRfPoints.Set(linkId, point.ClonePoint());
		return point;
	}

	protected static bool MatchesGameRfSlot(RICON_OpenRadioPairListener row, int device, int index, int physicalGeneration,
		int player, int generation, int connection, int entity)
	{
		return row.m_iDeviceId == device && row.m_iIndex == index && row.m_iGeneration == physicalGeneration
			&& row.m_iPlayerId == player && row.m_iPlayerGeneration == generation && row.m_iConnectionId == connection && row.m_iEntityId == entity;
	}

	protected bool GameRfPointCurrent(RICON_GameRadioRfCommit committed, RICON_MediaEgressRoute route, RICON_GameRadioRfPoint point)
	{
		int nowMs = point.m_iTickMs;
		if (!GameRfCommitCurrent(committed, nowMs) || !point.m_Route.Same(route) || !committed.m_Profile.m_Radio
			|| committed.m_Profile.m_Radio.m_iExpectedTuningValue != point.m_iRawTune
			|| committed.m_Profile.m_Radio.m_fFrequencyMHz != point.m_fFrequencyMHz
			|| committed.m_Publication.GetMediaListenerIncarnation(route) != point.m_iListenerIncarnation) return false;
		RICON_MediaStreamBinding source = route.m_Source;
		RICON_LifecycleLease lease = committed.m_Lifecycle.QueryActiveLease(source.m_iLeaseId, nowMs);
		if (!lease || lease.m_iIssuedAtMs != point.m_iIssuedAtMs || lease.EffectiveDeadlineMs() != point.m_iDeadlineMs
			|| point.m_iIssuedAtMs > nowMs || nowMs >= point.m_iDeadlineMs) return false;
		if (committed.m_Receivers.ReadRecordedOpenEndpointAdmission(source.m_iDeviceId, source.m_iTxTransceiverIndex,
			point.m_iTxGeneration, source.m_iNetworkTypeId, source.m_iChannelId, point.m_iRawTune, nowMs) != point.m_iTxAdmission
			|| committed.m_Receivers.ReadRecordedOpenEndpointAdmission(route.m_iRxDeviceId, route.m_iRxTransceiverIndex,
				route.m_iRxGeneration, source.m_iNetworkTypeId, source.m_iChannelId, point.m_iRawTune, nowMs) != point.m_iRxAdmission) return false;
		return committed.m_Publication.MatchesCurrentMediaLink(point.m_iLinkId, route, committed.m_Lifecycle, nowMs)
			&& GameRfCommitCurrent(committed, nowMs);
	}

	RICON_MediaFreeSpaceMotionLimits ReadGameRfCoverageLimits(RICON_MediaEgressRoute route, RICON_GameRadioRfPoint previous, RICON_GameRadioRfPoint current)
	{
		ref RICON_GameRadioRfCommit committed = m_GameRfCommit;
		if (!route || !route.IsValid() || !previous || !current || !m_PreviousGameRfPoints || !m_CurrentGameRfPoints) return null;
		ref RICON_GameRadioRfPoint first = m_PreviousGameRfPoints.Get(current.m_iLinkId);
		ref RICON_GameRadioRfPoint last = m_CurrentGameRfPoints.Get(current.m_iLinkId);
		if (!first || !last || !first.SamePoint(previous) || !last.SamePoint(current) || !first.SameScope(last)
			|| first.m_iAttempt + 1 != last.m_iAttempt || first.m_iTickMs >= last.m_iTickMs || last.m_iTickMs >= first.m_iDeadlineMs
			|| !GameRfPointCurrent(committed, route, last)) return null;
		RICON_MediaFreeSpaceMotionLimits limits = ReadAuthoredGameRfCoverageLimits(route.CloneRoute(), first.m_iTickMs, last.m_iTickMs);
		if (!limits || !first.SamePoint(previous) || !last.SamePoint(current) || !GameRfPointCurrent(committed, route, last)
			|| limits.m_fFrequencyMHz != last.m_fFrequencyMHz) return null;
		return limits;
	}

	protected string BuildGameOpenRadioRfTick(RICON_GameOpenRadioRfProfile profile, RICON_OpenRadioListenerRoster roster,
		array<ref RICON_GameRadioOpenSlot> slots, array<ref RICON_GameRadioPttEmission> emissions,
		int networkId, int channelId, int sampleMs, out RICON_TemporalProducerTick tick)
	{
		tick = null;
		if (sampleMs < profile.m_Los.m_iValidFromMs || sampleMs >= profile.m_Los.m_iValidUntilMs) return "OUTSIDE_PROFILE";
		int revision = roster.GetRevision();
		array<ref RICON_OpenRadioPairListener> listeners = {};
		if (revision <= 0 || !roster.Read(listeners)) return "STALE_ROSTER";
		array<ref RICON_OpenRadioPairEndpoint> endpoints = {};
		foreach (RICON_GameRadioOpenSlot slot : slots)
		{
			RICON_OpenRadioPairEndpoint endpoint = new RICON_OpenRadioPairEndpoint();
			endpoint.m_Radio = slot.m_Radio;
			endpoint.m_iGeneration = slot.m_Listener.m_iGeneration;
			endpoint.m_vLocalAntennaOffset = profile.m_vAntennaOffset;
			endpoint.m_Profile = ReadGameEndpointProfile(profile, slot.m_Radio, slot.m_Listener.m_iDeviceId, slot.m_Listener.m_iIndex);
			if (!endpoint.m_Profile) return "INVALID_EQUIPMENT_PROFILE";
			endpoints.Insert(endpoint);
		}
		RICON_OpenRadioPairScope scope = new RICON_OpenRadioPairScope();
		scope.m_iNetworkId = networkId;
		scope.m_iChannelId = channelId;
		scope.m_iRawTune = profile.m_Radio.m_iExpectedTuningValue;
		scope.m_iMaxSpanMs = m_iInventoryPollMs;
		scope.m_fCarrierMHz = profile.m_Radio.m_fFrequencyMHz;
		scope.m_Limits = profile.m_Limits;
		scope.m_Context = new RICON_RadioQueryContext();
		scope.m_Context.m_sBandId = profile.m_sBandId;
		scope.m_Context.m_sModelId = "free_space";
		scope.m_Context.m_sNoiseEnvironmentId = "none";
		scope.m_Context.m_sIntelligibilityModelId = "linear_test";
		RICON_PairServiceInput input;
		string projected = RICON_OpenRadioPairProjector.BuildFromGameEmissions(m_InventoryLifecycle, emissions,
			m_InventoryReceivers, GetGame().GetWorld(), scope, endpoints, listeners, sampleMs, input);
		if (projected != "OK") return projected;
		if (!BuildGmRxEnvironments(input)) return "INVALID_GM_RX_ENVIRONMENT";
		input.m_bExcludeOwnReceivers = m_bInventoryMixedEquipment;
		RICON_StaticLosProfile sample = RICON_StaticLosGeometryAuthority.CopyProfile(profile.m_Los);
		sample.m_iValidFromMs = sampleMs;
		sample.m_iValidUntilMs = sampleMs + 1;
		string built = RICON_StaticLosTick.Build(input, sample, sampleMs, 0, "game-open-rf", 1, 0.125, tick);
		if (built != "OK") return built;
		tick.m_ListenerRoster = roster;
		tick.m_iListenerRosterRevision = revision;
		return "OK";
	}

	protected RICON_RadioRfProfile ReadGameEndpointProfile(RICON_GameOpenRadioRfProfile channel, BaseRadioComponent radio,
		int deviceId, int index)
	{
		ref RICON_RadioRfProfile profile = RICON_OpenRadioPairProjector.CopyProfile(channel.m_Radio);
		profile.m_iDeviceId = deviceId;
		profile.m_iTransceiverIndex = index;
		if (m_bInventoryMixedEquipment && !RICON_GameRadioEquipment.ApplyHardware(RICON_GameRadioEquipment.ReadType(radio), profile, RICON_GameRadioEquipment.ReadOverride(radio, m_MixedRadioHardware))) return null;
		return profile;
	}

	static RICON_GameRadioManagerComponent FindGmManager()
	{
		RICON_GameRadioManagerComponent manager = RICON_GameRadioManagerComponent.Cast(GetInstance());
		if (!manager || !manager.m_bInventoryOpenRadioAdmissions) return null;
		return manager;
	}

	float ReadGmJammerField(IEntity target, RICON_EGmRfField field)
	{
		RICON_GmJammerRole role = m_GmJammers.Get(target);
		if (!role) role = new RICON_GmJammerRole(target);
		return role.ReadField(field);
	}

	bool WriteGmJammerField(IEntity target, RICON_EGmRfField field, float value)
	{
		if (!IsAuthority() || !RICON_GmJammerRole.IsTarget(target) || target.GetWorld() != GetOwner().GetWorld()) return false;
		RICON_GmJammerRole role = m_GmJammers.Get(target);
		if (role) return role.SetField(field, value);
		role = new RICON_GmJammerRole(target);
		if (!role.SetField(field, value) || m_iGmEmissionHighWater >= int.MAX - 1) return false;
		m_iGmEmissionHighWater++;
		role.m_iId = m_iGmEmissionHighWater;
		m_GmJammers.Set(target, role);
		return true;
	}

	protected bool BuildGmRxEnvironments(RICON_PairServiceInput input)
	{
		if (m_GmJammers.IsEmpty()) return true;
		array<IEntity> active = {};
		for (int i = m_GmJammers.Count() - 1; i >= 0; i--)
		{
			IEntity source = m_GmJammers.GetKey(i);
			if (!RICON_GmJammerRole.IsTarget(source))
				m_GmJammers.RemoveElement(i);
			else if (m_GmJammers.GetElement(i).m_bEnabled)
				active.Insert(source);
		}
		if (active.IsEmpty()) return true;
		RICON_RxEnvironmentLimits limits = new RICON_RxEnvironmentLimits();
		limits.m_iMaxEmitters = 4096;
		limits.m_iMaxBandsPerEmitter = 1;
		RICON_LinkEvaluator evaluator = GetBandEvaluator(input.m_Context.m_sBandId);
		bool hasInterference;
		foreach (RICON_PairEndpoint rx : input.m_aEndpoints)
		{
			RICON_RxEnvironmentInput sample = new RICON_RxEnvironmentInput();
			sample.m_iSessionEpoch = input.m_iSessionEpoch;
			sample.m_iRevision = Math.Max(1, m_iGameRfAttempt);
			sample.m_iObservedAtMs = input.m_iNowMs;
			sample.m_iTtlMs = m_iInventoryPollMs;
			sample.m_Receiver = rx.m_Observation;
			sample.m_Profile = rx.m_Profile;
			sample.m_Limits = limits;
			sample.m_Context = input.m_Context;
			foreach (IEntity entity : active)
			{
				RICON_GmJammerRole role = m_GmJammers.Get(entity);
				vector origin = entity.GetOrigin();
				if (role.m_fRadiusM > 0 && vector.Distance(origin, rx.m_Observation.m_vAntennaPos) > role.m_fRadiusM) continue;
				sample.m_aEmitters.Insert(role.ReadEmission(origin));
			}
			RICON_RxEnvironment environment = RICON_RxEnvironmentService.Evaluate(sample, evaluator);
			if (!environment.m_bValid) return false;
			if (environment.m_bHasInterference)
			{
				hasInterference = true;
				// Preserve the existing PCM proof for clean receivers. Noise-aware wire coverage is a separate gate.
				foreach (RICON_PairListener listener : input.m_aListeners)
				{
					if (listener.m_iDeviceId != environment.m_iRxDeviceId || listener.m_iTransceiverIndex != environment.m_iRxTransceiverIndex) continue;
					environment.m_iRxGeneration = listener.m_iRxGeneration;
					environment.m_sTimebaseId = "game-open-rf";
					break;
				}
			}
			input.m_aRxEnvironments.Insert(environment);
		}
		if (!hasInterference) input.m_aRxEnvironments.Clear();
		input.m_bUseRxEnvironments = hasInterference;
		input.m_bPublishCleanRxEnvironments = false;
		return true;
	}

	bool OwnsInventoryLifecycle(RICON_LifecycleStateMachine lifecycle, int expectedEpoch)
	{
		return lifecycle && m_InventoryConsumeIdentity == lifecycle && GetSessionEpoch() == expectedEpoch
			&& lifecycle.GetSessionEpoch() == expectedEpoch;
	}

	int GetInventoryRadioCount()
	{
		if (!IsAuthority() || !m_Inventory) return 0;
		return m_Inventory.GetRecordCount();
	}

	//! Read-only identity lookup; it never grants a channel or opens an audio route.
	int ReadInventoryRadioId(int playerId, BaseRadioComponent radio)
	{
		ref RICON_GameRadioInventory inventory = m_Inventory;
		int expectedEpoch = GetSessionEpoch();
		if (!IsAuthority() || !radio || !CurrentInventory(inventory, expectedEpoch)) return 0;
		RICON_GameRadioNativeHandle query = new RICON_GameRadioNativeHandle();
		if (!query.Query(radio) || !CurrentInventory(inventory, expectedEpoch)) return 0;
		int deviceId = inventory.ReadOwnedDeviceId(playerId, query);
		if (!CurrentInventory(inventory, expectedEpoch)) return 0;
		return deviceId;
	}

	//! Exact per-transceiver server control identity; it does not admit RX, PTT, RPC or voice.
	bool ReadInventoryRadioControl(int playerId, BaseRadioComponent radio, int index,
		out int deviceId, out int networkId, out int channelId, out int rxGeneration)
	{
		deviceId = 0; networkId = 0; channelId = 0; rxGeneration = 0;
		ref RICON_GameRadioInventory inventory = m_Inventory;
		int expectedEpoch = GetSessionEpoch();
		if (!IsAuthority() || !radio || !CurrentInventory(inventory, expectedEpoch)) return false;
		RICON_GameRadioNativeHandle query = new RICON_GameRadioNativeHandle();
		if (!query.Query(radio) || !CurrentInventory(inventory, expectedEpoch)) return false;
		bool accepted = inventory.ReadOwnedControl(playerId, query, index, deviceId, networkId, channelId, rxGeneration);
		if (!CurrentInventory(inventory, expectedEpoch))
		{
			deviceId = 0; networkId = 0; channelId = 0; rxGeneration = 0;
			return false;
		}
		return accepted;
	}

	//! For the RF producer on this authority. Canonical channel identity remains owned by the game policy.
	bool CaptureGameOpenRadioSlots(int rawTune, int maxSlots, array<ref RICON_GameRadioOpenSlot> slots,
		out int channelId, out int sampleMs)
	{
		channelId = 0; sampleMs = -1;
		if (!slots) return false;
		slots.Clear();
		ref RICON_GameRadioInventory inventory = m_Inventory;
		ref RICON_GameRadioControlPolicy policy = m_InventoryControlPolicy;
		int expectedEpoch = GetSessionEpoch();
		if (!IsAuthority() || !m_bInventoryOpenRadioAdmissions || !policy || !CurrentInventory(inventory, expectedEpoch)) return false;
		int selectedChannel = policy.ReadChannel(expectedEpoch, rawTune, "");
		if (selectedChannel <= 0) return false;
		int capturedAt;
		if (!inventory.CaptureOpenRadioSlots(policy.NetworkId(), selectedChannel, rawTune, maxSlots, slots, capturedAt)
			|| !CurrentInventory(inventory, expectedEpoch) || m_InventoryControlPolicy != policy
			|| policy.ReadChannel(expectedEpoch, rawTune, "") != selectedChannel)
		{
			slots.Clear();
			return false;
		}
		channelId = selectedChannel; sampleMs = capturedAt;
		return true;
	}

	//! Create once per producer channel; RefreshCarried supplies each complete tick's slots and listeners.
	RICON_OpenRadioListenerRoster CreateCarriedOpenRadioListenerRoster(int rawTune, int maxSlots)
	{
		ref RICON_GameRadioInventory inventory = m_Inventory;
		ref RICON_GameRadioControlPolicy policy = m_InventoryControlPolicy;
		int expectedEpoch = GetSessionEpoch();
		if (!IsAuthority() || !m_bInventoryOpenRadioAdmissions || !policy || !m_InventoryBridge
			|| !CurrentInventory(inventory, expectedEpoch)) return null;
		int selectedChannel = policy.ReadChannel(expectedEpoch, rawTune, "");
		RICON_OpenRadioListenerRoster roster = new RICON_OpenRadioListenerRoster();
		if (selectedChannel <= 0 || !roster.Configure(m_InventoryLifecycle, m_InventoryReceivers, policy.NetworkId(), selectedChannel, maxSlots)
			|| !roster.UseGameControllerLifecycle(m_InventoryBridge) || !roster.UseCarriedInventory(inventory, rawTune)
			|| !CurrentInventory(inventory, expectedEpoch) || m_InventoryControlPolicy != policy) return null;
		return roster;
	}

	UUID GetInventoryPublicDomain()
	{
		if (!IsAuthority() || m_iInventoryDomainEpoch != GetSessionEpoch() || GetInstance() != this) return UUID.NULL_UUID;
		return m_InventoryPublicDomain;
	}

	//! Authored scenario opt-in, also readable on clients before the server backend is ready.
	bool IsGameRadioInputConfigured() { return m_iInventoryNetworkId > 0; }
	bool IsGameMixedEquipmentConfigured() { return m_bInventoryMixedEquipment && IsGameRadioInputConfigured(); }

	int GetInventoryPolicyRevision()
	{
		if (!IsAuthority() || !CurrentInventory(m_Inventory, GetSessionEpoch()) || !m_InventoryControlPolicy) return 0;
		return m_InventoryControlPolicy.Revision();
	}

	bool HasInventoryPttScope(RICON_LifecycleStateMachine lifecycle, int expectedEpoch)
	{
		return IsAuthority() && OwnsInventoryLifecycle(lifecycle, expectedEpoch) && m_InventoryLifecycle == lifecycle
			&& CurrentInventory(m_Inventory, expectedEpoch) && m_InventoryControlPolicy && m_InventoryControlPolicy.NetworkId() > 0;
	}

	protected bool CurrentPtt(RICON_GameRadioPttOwner ptt, int expectedEpoch)
	{
		return ptt && m_InventoryPtt == ptt && CurrentInventory(m_Inventory, expectedEpoch);
	}

	protected void PollTs3Bindings()
	{
		ref RICON_GameTs3BindingStore store = m_InventoryTs3Bindings;
		ref RICON_GameRadioPttNativeSource source = m_InventoryPttSource;
		ref RICON_GameControllerLifecycle bridge = m_InventoryBridge;
		ref RICON_LifecycleStateMachine lifecycle = m_InventoryLifecycle;
		if (!store || !source || !bridge || !lifecycle) return;
		array<int> players = {};
		store.ReadPlayerIds(players);
		foreach (int playerId : players)
		{
			int connectionId, generation, entityId;
			ref RICON_GameRadioPttContext current;
			if (bridge.ReadBoundPlayer(playerId, connectionId, generation, entityId))
			{
				SCR_PlayerController controller = bridge.ResolveRemoteRecipient(playerId, connectionId, generation, entityId);
				if (controller) current = source.ReadContext(new RICON_GameRadioPttNativeCaller(controller));
			}
			// An inconclusive native lookup must not discard the sequence watermark.
			RICON_EPlayerLifecycleState state = lifecycle.GetPlayerState(playerId);
			if (!current && state != RICON_EPlayerLifecycleState.NONE && state != RICON_EPlayerLifecycleState.DISCONNECTED) continue;
			int nowMs = source.ReadNowMs();
			if (nowMs < 0 || store != m_InventoryTs3Bindings) return;
			store.ReconcilePlayer(playerId, current, nowMs);
		}
		if (m_InventoryTs3Proof) m_InventoryTs3Proof.Poll();
	}

	//! Owner-only receipt for an unverified TS3 claim. No route is granted here.
	RICON_GameTs3PendingBinding ProcessGameTs3Claim(SCR_PlayerController sender, RICON_GameTs3BindingClaim claim, out int serverNowMs)
	{
		serverNowMs = -1;
		ref RICON_GameTs3BindingStore store = m_InventoryTs3Bindings;
		ref RICON_GameRadioPttNativeSource source = m_InventoryPttSource;
		if (!IsAuthority() || !sender || !claim || !store || !source) return null;
		ref RICON_GameRadioPttNativeCaller caller = new RICON_GameRadioPttNativeCaller(sender);
		ref RICON_GameRadioPttContext current = source.ReadContext(caller);
		if (!current || !current.Same(claim.m_Context)) return null;
		int nowMs = source.ReadNowMs();
		if (nowMs < 0 || store != m_InventoryTs3Bindings) return null;
		ref RICON_GameTs3PendingBinding pending = store.Claim(claim, current, nowMs, UUID.GenV4());
		if (!pending) return null;
		current = source.ReadContext(caller);
		nowMs = source.ReadNowMs();
		if (!current || !current.Same(claim.m_Context) || nowMs < 0 || store != m_InventoryTs3Bindings) return null;
		ref RICON_GameTs3PendingBinding receipt = store.Read(current, nowMs);
		if (!receipt || receipt.m_sBindingId != pending.m_sBindingId || !receipt.m_Claim.Same(claim)) return null;
		if (RICON_GameTs3ProofChallengeFile.Enabled())
		{
			if (!m_InventoryTs3Proof && m_InventoryLifecycle && m_InventoryBridge)
			{
				ref RICON_GameTs3ProofAdapter proof = new RICON_GameTs3ProofAdapter();
				if (proof.Configure(m_InventoryLifecycle, m_InventoryBridge, source, store,
					current.m_sDomain, current.m_iEpoch, m_iInventoryMaxPlayers)) m_InventoryTs3Proof = proof;
			}
			if (m_InventoryTs3Proof) m_InventoryTs3Proof.Track(receipt);
			current = source.ReadContext(caller);
			nowMs = source.ReadNowMs();
			if (!current || !current.Same(claim.m_Context) || nowMs < 0 || store != m_InventoryTs3Bindings) return null;
			receipt = store.Read(current, nowMs);
			if (!receipt || receipt.m_sBindingId != pending.m_sBindingId || !receipt.m_Claim.Same(claim)) return null;
		}
		serverNowMs = nowMs;
		return receipt;
	}

	//! Called by the actual controller's owner RPC. No external observation can arrive through this RPC.
	string ReadGameTs3Proof(SCR_PlayerController sender, string bindingId, int requestId, out int remainingMs, out bool matched)
	{
		remainingMs = 0;
		matched = false;
		ref RICON_GameTs3ProofAdapter proof = m_InventoryTs3Proof;
		if (!IsAuthority() || !proof || !sender) return "";
		int nowMs;
		ref RICON_GameTs3MatchedBinding match = proof.ReadMatched(sender, bindingId, requestId, nowMs);
		if (match && proof == m_InventoryTs3Proof && nowMs >= 0 && nowMs < match.m_iDeadlineMs)
		{
			remainingMs = match.m_iDeadlineMs - nowMs;
			matched = true;
			return "";
		}
		ref RICON_GameTs3Challenge challenge = proof.ReadChallenge(sender, bindingId, requestId, nowMs);
		if (!challenge || proof != m_InventoryTs3Proof || !challenge.m_Pending || nowMs < 0
			|| nowMs >= challenge.m_Pending.m_iDeadlineMs) return "";
		remainingMs = challenge.m_Pending.m_iDeadlineMs - nowMs;
		return challenge.m_sNonce;
	}

	RICON_GameRadioPttReply ReadGameRadioPttContext(SCR_PlayerController sender, int requestId)
	{
		ref RICON_GameRadioPttOwner ptt = m_InventoryPtt;
		int expectedEpoch = GetSessionEpoch();
		if (!IsAuthority() || !sender || !CurrentPtt(ptt, expectedEpoch)) return null;
		RICON_GameRadioPttReply reply = ptt.ReadContext(new RICON_GameRadioPttNativeCaller(sender), requestId);
		if (!CurrentPtt(ptt, expectedEpoch)) return null;
		return reply;
	}

	RICON_GameRadioPttReply ReadGameRadioPttControl(SCR_PlayerController sender, string domain, array<int> fields, RplId radioId)
	{
		ref RICON_GameRadioPttOwner ptt = m_InventoryPtt;
		int expectedEpoch = GetSessionEpoch();
		if (!IsAuthority() || !sender || !CurrentPtt(ptt, expectedEpoch)) return null;
		RICON_GameRadioPttControlRequest request = RICON_GameRadioPttWire.DecodeControl(domain, fields, new RICON_GameRadioPttNativeSelector(radioId));
		if (!request) return null;
		RICON_GameRadioPttReply reply = ptt.ReadRadio(new RICON_GameRadioPttNativeCaller(sender), request);
		if (!CurrentPtt(ptt, expectedEpoch)) return null;
		return reply;
	}

	RICON_GameRadioPttReply ProcessGameRadioPtt(SCR_PlayerController sender, string domain, array<int> fields, RplId radioId)
	{
		ref RICON_GameRadioPttOwner ptt = m_InventoryPtt;
		int expectedEpoch = GetSessionEpoch();
		if (!IsAuthority() || !sender || !CurrentPtt(ptt, expectedEpoch)) return null;
		RICON_GameRadioPttRequest request = RICON_GameRadioPttWire.DecodeRequest(domain, fields, new RICON_GameRadioPttNativeSelector(radioId));
		if (!request) return null;
		RICON_GameRadioPttReply reply = ptt.Process(new RICON_GameRadioPttNativeCaller(sender), request);
		if (!CurrentPtt(ptt, expectedEpoch)) return null;
		return reply;
	}

	//! A trusted game-side caller may resolve an emission owned by this exact remote controller.
	RICON_GameRadioPttEmission ReadGameRadioEmission(SCR_PlayerController sender, int leaseId)
	{
		ref RICON_GameRadioPttOwner ptt = m_InventoryPtt;
		int expectedEpoch = GetSessionEpoch();
		if (!IsAuthority() || !sender || !CurrentPtt(ptt, expectedEpoch)) return null;
		ref RICON_GameRadioPttEmission emission = ptt.ReadCurrentEmission(new RICON_GameRadioPttNativeCaller(sender), leaseId);
		if (!CurrentPtt(ptt, expectedEpoch)) return null;
		return emission;
	}

	RICON_GameRadioCancelReply ProcessGameRadioCancel(SCR_PlayerController sender, string domain, array<int> fields)
	{
		ref RICON_GameRadioPttOwner ptt = m_InventoryPtt;
		int expectedEpoch = GetSessionEpoch();
		if (!IsAuthority() || !sender || !CurrentPtt(ptt, expectedEpoch)) return null;
		RICON_GameRadioCancelRequest request = RICON_GameRadioCancelWire.DecodeRequest(domain, fields);
		if (!request) return null;
		RICON_GameRadioCancelReply reply = ptt.Cancel(new RICON_GameRadioPttNativeCaller(sender), request);
		if (!CurrentPtt(ptt, expectedEpoch)) return null;
		return reply;
	}
	bool AuthorizeGameRadioCancelReply(SCR_PlayerController sender, RICON_GameRadioCancelReply reply)
	{
		ref RICON_GameRadioPttOwner ptt = m_InventoryPtt;
		int expectedEpoch = GetSessionEpoch();
		if (!IsAuthority() || !sender || !CurrentPtt(ptt, expectedEpoch)) return false;
		return ptt.AuthorizeCancelReply(new RICON_GameRadioPttNativeCaller(sender), reply) && CurrentPtt(ptt, expectedEpoch);
	}

	RICON_GameRadioRenewReply ProcessGameRadioRenew(SCR_PlayerController sender, string domain, array<int> fields)
	{
		ref RICON_GameRadioPttOwner ptt = m_InventoryPtt;
		int expectedEpoch = GetSessionEpoch();
		if (!IsAuthority() || !sender || !CurrentPtt(ptt, expectedEpoch)) return null;
		RICON_GameRadioRenewRequest request = RICON_GameRadioRenewWire.DecodeRequest(domain, fields);
		if (!request) return null;
		RICON_GameRadioRenewReply reply = ptt.Renew(new RICON_GameRadioPttNativeCaller(sender), request);
		if (!CurrentPtt(ptt, expectedEpoch)) return null;
		return reply;
	}

	bool AuthorizeGameRadioRenewReply(SCR_PlayerController sender, RICON_GameRadioRenewReply reply)
	{
		ref RICON_GameRadioPttOwner ptt = m_InventoryPtt;
		int expectedEpoch = GetSessionEpoch();
		if (!IsAuthority() || !sender || !CurrentPtt(ptt, expectedEpoch)) return false;
		return ptt.AuthorizeRenewReply(new RICON_GameRadioPttNativeCaller(sender), reply) && CurrentPtt(ptt, expectedEpoch);
	}

	bool AuthorizeGameRadioPttReply(SCR_PlayerController sender, RICON_GameRadioPttReply reply)
	{
		ref RICON_GameRadioPttOwner ptt = m_InventoryPtt;
		int expectedEpoch = GetSessionEpoch();
		if (!IsAuthority() || !sender || !CurrentPtt(ptt, expectedEpoch)) return false;
		bool accepted = ptt.AuthorizeReply(new RICON_GameRadioPttNativeCaller(sender), reply);
		return accepted && CurrentPtt(ptt, expectedEpoch);
	}

	protected void ReportInventory(string status)
	{
		if (status == m_sInventoryStatus) return;
		m_sInventoryStatus = status;
		PrintFormat("[RICON_GAME_RADIO_INVENTORY] state=%1 epoch=%2 radios=%3 policyEnabled=%4 noAudio=1 remotePlayersOnly=1",
			status, GetSessionEpoch(), GetInventoryRadioCount(), m_InventoryControlPolicy != null);
		if (RICON_RadioDev.Enabled())
			PrintFormat("[RICON_DEV] SERVER AUTH inventory=%1 epoch=%2 radios=%3 PTT=%4 relayInput=%5",
				status, GetSessionEpoch(), GetInventoryRadioCount(), m_InventoryPtt != null, m_GameRelayInputRfPoint != null);
	}

	protected void FailInventory(string status)
	{
		ref RICON_LifecycleStateMachine lifecycle = m_InventoryLifecycle;
		int expectedEpoch = GetSessionEpoch();
		ReportInventory(status);
		if (GetSessionEpoch() != expectedEpoch || m_InventoryLifecycle != lifecycle) return;
		StopInventory();
		CloseMediaReceiverAuthority();
	}

	protected void StopInventory()
	{
		if (m_bInventoryStopping) return;
		m_bInventoryStopping = true;
		UpdateStockJammerMutes(null);
		m_GameOpenRadioPcmOwner = null;
		m_iGameOpenRadioPcmMaxAgeMs = 0;
		if (m_GameRelayInputPcmSeries) m_GameRelayInputPcmSeries.Close();
		m_GameRelayInputPcmSeries = null; m_GameRelayInputPcmPoint = null;
		m_GameRelayInputPcmBacklog = null;
		if (GetGame() && GetGame().GetCallqueue()) GetGame().GetCallqueue().Remove(PumpGameRelayInputPcmBacklog);
		m_GameRelayInputRfPoint = null;
		if (m_bObservingGameRelayRf) m_bGameRelayRfCancelled = true;
		m_GameRfCommit = null; m_PreviousGameRfPoints = null; m_CurrentGameRfPoints = null; m_iGameRfAttempt = 0;
		if (m_GameRfRoster) m_GameRfRoster.Close();
		m_GameRfRoster = null; m_GameRfProfile = null; m_iLastGameRfSampleMs = -1;
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(BootstrapInventory);
			GetGame().GetCallqueue().Remove(PollInventory);
		}
		ref RICON_GameRadioInventory inventory = m_Inventory;
		int expectedEpoch = GetSessionEpoch();
		ref RICON_GameRadioPttOwner ptt = m_InventoryPtt;
		if (m_InventoryTs3Proof) m_InventoryTs3Proof.Close();
		m_InventoryTs3Proof = null;
		if (m_InventoryTs3Bindings) m_InventoryTs3Bindings.Close();
		m_InventoryTs3Bindings = null;
		if (ptt) ptt.Close();
		if (inventory) inventory.Close();
		if (m_Inventory == inventory && GetSessionEpoch() == expectedEpoch)
		{
			m_Inventory = null; m_InventorySource = null;
			m_InventoryControlPolicy = null;
			m_InventoryPtt = null; m_InventoryPttSource = null;
			m_InventoryReceivers = null; m_InventoryBridge = null; m_InventoryLifecycle = null;
			m_bInventoryStarted = false;
		}
		m_bInventoryStopping = false;
	}

	protected void PollStockJamming(RICON_GameRadioInventory inventory, int epoch, string status)
	{
		if (m_GmJammers.IsEmpty() && m_StockJammerMutes.IsEmpty()) return;
		map<BaseTransceiver, ref RICON_StockJammerSlot> requested = new map<BaseTransceiver, ref RICON_StockJammerSlot>();
		if (RplSession.Mode() == RplMode.Dedicated && m_GameRfProfile && m_bInventoryMixedEquipment
			&& !RICON_StartupStatusProtocol.InGameVoiceRequested() && status == "OK" && CurrentInventory(inventory, epoch))
		{
			bool active;
			foreach (IEntity entity, RICON_GmJammerRole role : m_GmJammers)
			{
				if (!role.m_bEnabled) continue;
				active = true;
				break;
			}
			if (active && !BuildStockJammerMutes(inventory, epoch, requested)) requested.Clear();
		}
		UpdateStockJammerMutes(requested);
	}

	protected bool BuildStockJammerMutes(RICON_GameRadioInventory inventory, int epoch,
		map<BaseTransceiver, ref RICON_StockJammerSlot> requested)
	{
		array<ref RICON_StockJammerRadio> radios = {};
		if (!inventory.ReadStockRadios(radios)) return false;
		RICON_PairServiceInput input = new RICON_PairServiceInput();
		input.m_iNowMs = m_InventoryBridge.ReadWireAuthorityTime(m_InventoryLifecycle);
		input.m_iSessionEpoch = epoch;
		input.m_Context = new RICON_RadioQueryContext();
		input.m_Context.m_sBandId = m_GameRfProfile.m_sBandId;
		input.m_Context.m_sModelId = "free_space";
		input.m_Context.m_sNoiseEnvironmentId = "none";
		input.m_Context.m_sIntelligibilityModelId = "linear_test";
		RICON_LinkEvaluator evaluator = GetBandEvaluator(input.m_Context.m_sBandId);
		array<ref RICON_StockJammerSlot> slots = {};
		foreach (RICON_StockJammerRadio inventoried : radios)
		{
			BaseRadioComponent radio = inventoried.m_Radio;
			int device = inventoried.m_iDeviceId;
			vector position = RICON_OpenRadioPairProjector.AntennaPosition(radio.GetOwner(), m_GameRfProfile.m_vAntennaOffset);
			for (int index = 0; index < radio.TransceiversCount(); index++)
			{
				RadioTransceiver slot = RadioTransceiver.Cast(radio.GetTransceiver(index));
				if (!slot || slot.GetFrequency() <= 0) continue;
				RICON_PairEndpoint endpoint = new RICON_PairEndpoint();
				if (!RICON_BaseRadioObservationReader.TryRead(radio, device, index, position, endpoint.m_Observation)) return false;
				endpoint.m_Profile = ReadGameEndpointProfile(m_GameRfProfile, radio, device, index);
				if (!endpoint.m_Profile) return false;
				endpoint.m_Profile.m_iExpectedTuningValue = endpoint.m_Observation.m_iTuningValue;
				endpoint.m_Profile.m_fFrequencyMHz = endpoint.m_Observation.m_iTuningValue * 0.001;
				RICON_StockJammerSlot candidate = new RICON_StockJammerSlot();
				candidate.m_iPlayerId = inventoried.m_iPlayerId;
				candidate.m_Radio = radio;
				candidate.m_Slot = slot;
				candidate.m_Endpoint = endpoint;
				slots.Insert(candidate);
				input.m_aEndpoints.Insert(endpoint);
			}
		}
		if (!evaluator || !BuildGmRxEnvironments(input)) return false;
		if (!input.m_bUseRxEnvironments) return true;
		if (input.m_aRxEnvironments.Count() != slots.Count()) return false;
		int budget = m_GameRfProfile.m_Limits.m_iMaxPairs;
		for (int i = 0; i < slots.Count(); i++)
		{
			RICON_RxEnvironment environment = input.m_aRxEnvironments[i];
			if (!environment.m_bHasInterference) continue;
			RICON_StockJammerSlot rx = slots[i];
			if (rx.DominatedBy(environment, input.m_Context, evaluator, slots, budget))
				requested.Insert(rx.m_Slot, rx);
			if (budget < 0) return false;
		}
		return CurrentInventory(inventory, epoch);
	}

	protected void UpdateStockJammerMutes(map<BaseTransceiver, ref RICON_StockJammerSlot> requested)
	{
		for (int i = m_StockJammerMutes.Count() - 1; i >= 0; i--)
		{
			RICON_StockJammerSlot held = m_StockJammerMutes.GetElement(i);
			if (requested && requested.Contains(held.m_Slot) && held.SameSlot(true) && held.m_Slot.IsMuted()) continue;
			if (held.SameSlot(false) && held.m_Slot.IsMuted())
			{
				held.m_Slot.SetMuteState(false);
				if (held.m_Slot.IsMuted()) continue;
				held.Trace(false);
			}
			m_StockJammerMutes.RemoveElement(i);
		}
		if (!requested) return;
		foreach (BaseTransceiver slot, RICON_StockJammerSlot rx : requested)
		{
			if (m_StockJammerMutes.Contains(slot) || !rx.SameSlot(true) || slot.IsMuted()) continue;
			slot.SetMuteState(true);
			if (!slot.IsMuted()) continue;
			m_StockJammerMutes.Insert(slot, rx);
			rx.Trace(true);
		}
	}

	override void OnGameEnd()
	{
		m_GmJammers.Clear();
		StopInventory();
		super.OnGameEnd();
	}
	override void OnDelete(IEntity owner) { StopInventory(); super.OnDelete(owner); }
}
