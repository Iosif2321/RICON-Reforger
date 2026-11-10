# claude, TOOL_ONLY: B0b step 2 - one Play of the own World Editor in the "Server localhost" mode, switched on in the UI
# and started by the launcher after the operator's plugin command. Only after START -> codex ACK; END after the run.
# Plan: agora #4691-#4698 (launcher inspect wait/return #4694/#4697), A2 #4731-#4742, plugin signal #4847-#4850.
#   1. Workbench with the Plain world in edit mode (-riconBenchMode=inspect) and the endpoint probe gate
#      (-riconEndpointProbe=1); wait for the editor window, "[RICON_WBMODE] registered=1", OPEN_ONLY isGameMode=0 and
#      the launcher's inspect=WAIT_PLUGIN.
#   2. Strict snapshot B of Game > Play Game Options (both server items Off); InvokePattern of "Server localhost" after
#      the fresh Off check; strict S1 must differ from B by exactly Server localhost=On (else put back and stop).
#   3. Play is started by the launcher, not through the UI (A2, codex #4731-#4736): the UIA Invoke of Game > Play Game
#      blocked runs 125954/131244, and in 131244 the Script Authorization window of the own PID could not be read while
#      that call was pending. After S1 and a Tick the runner invokes the launcher's own command Plugins > "RICON: Play"
#      (-riconBenchPluginStart=1): its Run() only arms a flag in memory and returns, the launcher's thread logs
#      PLUGIN_SEEN and calls SwitchToGameMode once. The Invoke runs in a worker runspace (PrepareMenuCommand +
#      StartAsyncInvoke), and the menus are not touched again until the launcher's EDIT_STABLE (codex #4850). The A2
#      file flag is gone: in run 172649 one "Yes" on the launcher's FileIO.FileExists poll did not cover its next pass.
#   4. Observation once per second (as wb-run-claude.ps1): the Workbench PID and its descendants (held handles), other
#      new ArmaReforger* processes, their UDP endpoints/TCP listeners as address class and port, their windows; one
#      guarded "Yes" in all (YesGuardSites), only on the exact vanilla :1101 prompt of the Workbench PID and only after
#      PLUGIN_SEEN and before OBSERVATION_END/EDIT_* (SitesAllowed); with -Client one more on a new window of the same
#      prompt once the host logged creating PlayerId=2 for the test client (owner 2026-10-26, after run 144401);
#      any prompt before that, a prompt in another
#      process, an unknown window, too many processes, a launcher refusal or SCRIPT (E) stops the run. The own Qt window
#      "Arma Reforger Workbench" (visible right after PLUGIN_PLAY_REQUESTED in run 174552) is passive from PLUGIN_SEEN
#      until EDIT_STABLE (codex #4870): never any input; class, size and element names are read once per new handle.
#   5. After the launcher's inspect=EDIT_STABLE: strict S2 must be B except Server localhost=On; Invoke it back from On
#      and the final strict snapshot must equal B (RESTORE_OK), else RESTORE_INCOMPLETE is reported.
#   6. Teardown in finally: descendants only through their held handles, the root through its Process object.
# -ReadAuthOnly (codex #4804, after run 154043 met a Script Authorization window before Play): the same launch, but a
# prompt of the own PID before Play is only read (its UIA names, never a pattern call) and stops the run with
# AUTH_PROMPT_READ; without a prompt the run stops after WAIT_PLUGIN and one settle as READ_AUTH_NO_PROMPT. In this mode
# the menus are never opened, nothing is toggled and the plugin command is not invoked.
# -Client (B0c, codex #4906; the owner allowed the client window on 2026-10-25, agora #4901): the host also gets
# -riconEndpointSamples=120 and -forceUpdate. Once the log shows PLAY_ENTERED and [RICON_NET] Listen, no observation end
# yet, the Workbench PID has UDP :2001 in the current endpoint snapshot and the host address chosen before the run is
# still the one chosen (ClientStartBlock, ClientAddressRecheck; codex #4922), the runner starts one Diag game client (own Process handle and watchdog, cwd = the
# repository root): the same gproj, -client <the one local IPv4 of SelectHostAddress>, a new empty profile and logs
# directory under -OutDir, -riconEndpointProbe=1, -username RICON_B0c_peer1 (after the duplicate identity kick of run
# 202016, codex #4953; the summary prints only counts of that name and of "same identity" lines). Its windows are read once per handle and title and never get input
# (ClientWindowStep); a Script Authorization window there (never answered) or any child process of it stops the client
# part: the client, then its children through held handles. So does a client that writes no console.log within 60 s
# or logs SCRIPT (E), a lost dependency or a probe config refusal; a client that exits is recorded. In all these cases
# the host still returns to edit and restores the setting (plan B0c, 3f2ce83). The client is stopped as planned once the
# host logs OBSERVATION_END, before the host leaves Play. Teardown stops the client before Workbench.
# Repo-facing stdout carries address classes, ports, PIDs, switch names and hashes; raw addresses and command lines go to
# the raw JSONL in -OutDir, outside the repository. Never writes the registry or profile.
param([Parameter(Mandatory = $true)][string]$Tag,
      [Parameter(Mandatory = $true)][string]$OutDir,
      [int]$Seconds = 240,
      [int]$Settle = 15,
      [int]$MaxChildren = 3,
      # B0e: only two verified vanilla worlds; Plain remains the default.
      [ValidateSet("{3048828FE14AE687}worlds/Tests/TestFramework/Autotest_GameMode_Plain.ent", "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent")]
      [string]$World = "{3048828FE14AE687}worlds/Tests/TestFramework/Autotest_GameMode_Plain.ent",
      [switch]$ReadAuthOnly,
      [switch]$Client,
      # Passive TOOL_ONLY subscriptions; never requests capture or PTT.
      [switch]$AudioObserve,
      # D0 queue discovery only; public run identity is not a payload/nonce.
      [switch]$CommunicationDiscovery,
      # V0 native input-shape discovery; never combines with D0.
      [switch]$VoNInputShapeDiscovery,
      # Public wrong-frequency falsifier; never activates outside the native paired fixture.
      [switch]$WrongFrequencyControl,
      # I0 getter-only mapping; mutually exclusive with emitters.
      [switch]$VoNMappingInspection,
      # Passive NRX metadata only; paired MpTest with fresh bounded-discovery profiles.
      [switch]$NativeRxObserve,
      # Пассивная placement capability; typed transaction без REST, Pulse и capture.
      [switch]$NativePairPlacement,
      # Одна пассивная проверка состояния: A наблюдает, B меняет питание радио.
      [switch]$NativeRadioPower,
      # Capability-only, host-held file read; no pulse/Arm/capture.
      [switch]$FileHandleCapabilityProbe,
      # One loopback-only native REST capability request; no pulse/Arm/capture.
      [switch]$RestCapabilityProbe,
      # ?????? paired guarded REHEARSAL, ??? capture ? Plugins dispatch.
      [switch]$RestGuardedRehearsal,
      # Отдельный guarded capture; rehearsal и solo несовместимы.
      [switch]$RestGuardedCapture,
      # Свежая геометрия разделённой пары только для guarded capture.
      [switch]$CaptureSeparatedPair,
      # Единственный короткий intended stop 400 мс; длительность не передаётся B.
      [switch]$CaptureShort400,
      # Одна приватная поздняя 400/1000 проба; B получает прежние аргументы.
      [switch]$CaptureBlindTrial,
      # Нейтральное наблюдение собственного B; выбор A в этот flag не входит.
      [switch]$BlindBObserve,
      # Только пассивные sound callbacks собственного B в обычном RADIO опыте.
      [switch]$VoNSoundObserve,
      # Явные negative controls; разрешён только один внутри guarded capture.
      [switch]$CaptureNoSend,
      [switch]$CaptureWrongFrequency,
      # Одна проверка OFF всего устройства B с заранее фиксированным бюджетом.
      [switch]$CaptureBPowerOff,
      # Отдельная near-fixture DIRECT, без RADIO readiness и иных controls.
      [switch]$CaptureDirect,
      # Explicit comparative control: host only, never changes the client.
      [switch]$RestBackendControl,
      # Narrow REST-only topology control; paired VoN remains required separately.
      [switch]$RestSingleHostControl,
      # Optional read-only engine settings and unarmed host-only pulse rehearsal.
      [string]$EngineConfig = "",
      [switch]$PulseRehearsal,
      # Explicit one-attempt armed rehearsal; no capture mode.
      [switch]$ArmPulseRehearsal,
      [string]$PulseAttempt = "",
      # Own-B local meter calibration only; host stays endpoint-only.
      [switch]$AudioMeterProbe,
      # Diagnostic control: retain meter setup but omit the two client meter flags.
      [switch]$AudioMeterFlagOffControl,
      # Optional host AutoSpawn hint, not a guarantee of the actual spawned faction.
      [string]$AutoDeployFaction = "",
      [string]$CommunicationRun = "",
      [switch]$DryRun)

if ($CaptureDirect -and (-not $RestGuardedCapture -or $AutoDeployFaction -cne "US" -or $CaptureSeparatedPair -or $NativeRadioPower -or $NativePairPlacement -or $CaptureNoSend -or $CaptureWrongFrequency -or $CaptureShort400 -or $CaptureBlindTrial -or $BlindBObserve -or $VoNSoundObserve -or $RestGuardedRehearsal -or $PulseRehearsal -or $ArmPulseRehearsal -or $RestSingleHostControl)) { "BAD_CAPTURE_DIRECT_MODE"; exit 2 }
if ($CaptureBPowerOff -and (-not $RestGuardedCapture -or (-not $CaptureSeparatedPair -and -not $CaptureDirect) -or $AutoDeployFaction -cne "US" -or $NativeRadioPower -or $NativePairPlacement -or $CaptureShort400 -or $CaptureBlindTrial -or $CaptureNoSend -or $CaptureWrongFrequency -or $VoNSoundObserve -or $BlindBObserve -or $RestGuardedRehearsal -or $PulseRehearsal -or $ArmPulseRehearsal -or $RestSingleHostControl)) { "BAD_CAPTURE_B_POWER_OFF_MODE"; exit 2 }
if ($NativeRadioPower -and (-not $Client -or -not $NativeRxObserve -or $World -cne "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent" -or $AutoDeployFaction -cne "US" -or $CommunicationRun -cnotmatch '\A[A-Za-z0-9][A-Za-z0-9_.-]{0,63}\z' -or $NativePairPlacement -or $RestCapabilityProbe -or $RestBackendControl -or $RestGuardedCapture -or $RestGuardedRehearsal -or $RestSingleHostControl -or $PulseRehearsal -or $ArmPulseRehearsal -or $PulseAttempt -or $CaptureSeparatedPair -or $CaptureShort400 -or $CaptureBlindTrial -or $CaptureNoSend -or $CaptureWrongFrequency -or $VoNSoundObserve -or $BlindBObserve -or $FileHandleCapabilityProbe -or $ReadAuthOnly -or $AudioObserve -or $CommunicationDiscovery -or $VoNInputShapeDiscovery -or $VoNMappingInspection -or $WrongFrequencyControl -or $AudioMeterProbe -or $AudioMeterFlagOffControl)) { "BAD_NATIVE_RADIO_POWER_MODE"; exit 2 }
if ($VoNSoundObserve -and (-not $RestGuardedCapture -or -not $CaptureSeparatedPair -or $CaptureBlindTrial -or $CaptureShort400 -or $CaptureNoSend -or $CaptureWrongFrequency -or $NativePairPlacement -or $RestGuardedRehearsal -or $PulseRehearsal -or $ArmPulseRehearsal -or $RestSingleHostControl)) { "BAD_VON_SOUND_OBSERVE_MODE"; exit 2 }
if ($BlindBObserve -and (-not $RestGuardedCapture -or -not $CaptureSeparatedPair -or $CaptureShort400 -or $CaptureNoSend -or $CaptureWrongFrequency -or $NativePairPlacement -or $RestGuardedRehearsal -or $RestSingleHostControl)) { "BAD_BLIND_B_OBSERVE_MODE"; exit 2 }
if ($CaptureBlindTrial -and (-not $BlindBObserve -or -not $RestGuardedCapture -or -not $CaptureSeparatedPair -or $CaptureShort400 -or $CaptureNoSend -or $CaptureWrongFrequency -or $NativePairPlacement -or $RestGuardedRehearsal -or $PulseRehearsal -or $ArmPulseRehearsal -or $RestSingleHostControl)) { "BAD_CAPTURE_BLIND_MODE"; exit 2 }
if ($CaptureShort400 -and (-not $RestGuardedCapture -or -not $CaptureSeparatedPair -or $CaptureNoSend -or $CaptureWrongFrequency -or $NativePairPlacement -or $RestGuardedRehearsal -or $PulseRehearsal -or $ArmPulseRehearsal -or $RestSingleHostControl)) { "BAD_CAPTURE_SHORT400_MODE"; exit 2 }
if ($CaptureSeparatedPair -and (-not $RestGuardedCapture -or $NativePairPlacement -or $AutoDeployFaction -cne "US")) { "BAD_CAPTURE_SEPARATED_PAIR_MODE"; exit 2 }
if ($NativePairPlacement -and (-not $Client -or -not $NativeRxObserve -or $World -cne "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent" -or $AutoDeployFaction -cne "US" -or $CommunicationRun -cnotmatch '\A[A-Za-z0-9][A-Za-z0-9_.-]{0,63}\z' -or $RestCapabilityProbe -or $RestBackendControl -or $RestGuardedRehearsal -or $RestGuardedCapture -or $RestSingleHostControl -or $FileHandleCapabilityProbe -or $PulseRehearsal -or $ArmPulseRehearsal -or $PulseAttempt -or $CaptureNoSend -or $CaptureWrongFrequency -or $ReadAuthOnly -or $AudioObserve -or $CommunicationDiscovery -or $VoNInputShapeDiscovery -or $VoNMappingInspection -or $WrongFrequencyControl -or $AudioMeterProbe -or $AudioMeterFlagOffControl)) {
    "BAD_NATIVE_PAIR_PLACEMENT_MODE"; exit 2
}
if (($CaptureNoSend -or $CaptureWrongFrequency) -and (-not $RestGuardedCapture -or ($CaptureNoSend -and $CaptureWrongFrequency))) {
    "BAD_CAPTURE_CONTROL_MODE"; exit 2
}
if ($RestGuardedCapture -and (-not $RestCapabilityProbe -or -not $RestBackendControl -or -not $NativeRxObserve -or -not $Client -or $RestSingleHostControl -or $RestGuardedRehearsal -or $PulseRehearsal -or $ArmPulseRehearsal -or $World -cne "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent" -or $CommunicationRun -cnotmatch '\A[A-Za-z0-9][A-Za-z0-9_.-]{0,63}\z' -or $PulseAttempt -cnotmatch '\A[A-Za-z0-9][A-Za-z0-9_.-]{0,63}\z')) {
    "BAD_REST_GUARDED_CAPTURE_MODE"; exit 2
}
if ($RestGuardedRehearsal -and (-not $RestCapabilityProbe -or -not $RestBackendControl -or -not $NativeRxObserve -or -not $Client -or $RestSingleHostControl -or -not $PulseRehearsal -or -not $ArmPulseRehearsal -or $World -cne "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent" -or $CommunicationRun -cnotmatch '\A[A-Za-z0-9][A-Za-z0-9_.-]{0,63}\z' -or $PulseAttempt -cnotmatch '\A[A-Za-z0-9][A-Za-z0-9_.-]{0,63}\z')) {
    "BAD_REST_GUARDED_REHEARSAL_MODE"; exit 2
}
if ($RestSingleHostControl -and (-not $RestCapabilityProbe -or -not $RestBackendControl -or -not $NativeRxObserve -or $Client -or $World -cne "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent")) { "BAD_REST_SINGLE_HOST_MODE"; exit 2 }
if ($RestBackendControl -and -not $RestCapabilityProbe) { "BAD_REST_BACKEND_CONTROL_MODE"; exit 2 }
if ($RestCapabilityProbe -and (-not $NativeRxObserve -or (-not $Client -and -not $RestSingleHostControl) -or $World -cne "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent" -or $FileHandleCapabilityProbe -or $ReadAuthOnly -or $AudioObserve -or $CommunicationDiscovery -or $VoNInputShapeDiscovery -or $VoNMappingInspection -or $WrongFrequencyControl -or $AudioMeterProbe -or $AudioMeterFlagOffControl -or (($PulseRehearsal -or $ArmPulseRehearsal -or $PulseAttempt) -and -not ($RestGuardedRehearsal -or $RestGuardedCapture)))) {
    "BAD_REST_CAPABILITY_MODE"; exit 2
}
if ($FileHandleCapabilityProbe -and (-not $NativeRxObserve -or -not $Client -or $World -cne "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent" -or $ReadAuthOnly -or $AudioObserve -or $CommunicationDiscovery -or $VoNInputShapeDiscovery -or $VoNMappingInspection -or $WrongFrequencyControl -or $AudioMeterProbe -or $AudioMeterFlagOffControl -or $PulseRehearsal -or $ArmPulseRehearsal -or $PulseAttempt)) {
    "BAD_FILE_HANDLE_CAPABILITY_MODE"; exit 2
}
if ($ArmPulseRehearsal -and (-not $PulseRehearsal -or -not $NativeRxObserve -or -not $Client -or $World -cne "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent")) {
    "BAD_ARMED_REHEARSAL_MODE"; exit 2
}
if ($EngineConfig -and (-not $NativeRxObserve -or (-not $Client -and -not $RestSingleHostControl) -or $World -cne "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent")) {
    "BAD_ENGINE_CONFIG_MODE"; exit 2
}
if (($PulseRehearsal -or $PulseAttempt) -and -not $RestGuardedCapture -and (-not $PulseRehearsal -or -not $NativeRxObserve -or -not $Client -or $World -cne "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent")) {
    "BAD_PULSE_REHEARSAL_MODE"; exit 2
}
if ($PulseRehearsal -and $PulseAttempt -cnotmatch '\A[A-Za-z0-9_.-]{1,64}\z') { "BAD_PULSE_ATTEMPT"; exit 2 }
if ($AudioMeterFlagOffControl -and (-not $AudioMeterProbe -or -not $Client -or $World -cne "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent")) {
    "BAD_AUDIO_METER_FLAGOFF_MODE"; exit 2
}
if ($AudioMeterProbe -and (-not $Client -or $ReadAuthOnly -or $AudioObserve -or $CommunicationDiscovery -or $VoNInputShapeDiscovery -or $VoNMappingInspection -or $NativeRxObserve -or $WrongFrequencyControl -or $AutoDeployFaction -or $World -cne "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent")) {
    "BAD_AUDIO_METER_MODES"; exit 2
}
if ($NativeRxObserve -and ((-not $Client -and -not $RestSingleHostControl) -or $ReadAuthOnly -or $AudioObserve -or $CommunicationDiscovery -or $VoNInputShapeDiscovery -or $VoNMappingInspection -or $WrongFrequencyControl -or $World -cne "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent")) {
    "BAD_NRX_MODES"; exit 2
}
if ($AutoDeployFaction -and -not $NativeRxObserve) { "BAD_NRX_FACTION_MODE"; exit 2 }
if ($AutoDeployFaction -and $AutoDeployFaction -cnotmatch '\A[A-Za-z][A-Za-z0-9_]{0,31}\z') { "BAD_NRX_FACTION_KEY"; exit 2 }
if ($WrongFrequencyControl -and (-not $VoNInputShapeDiscovery -or -not $Client -or $ReadAuthOnly -or $AudioObserve -or $CommunicationDiscovery -or $VoNMappingInspection -or $NativeRxObserve)) {
    "BAD_WRONG_FREQUENCY_MODE"; exit 2
}
$restGuarded = $RestGuardedRehearsal -or $RestGuardedCapture
$armedPulse = $ArmPulseRehearsal -or $RestGuardedCapture
$typedTransaction = $restGuarded -or $NativePairPlacement -or $NativeRadioPower
$transactionAttemptId = if ($NativeRadioPower) { "RadioPower.1" } elseif ($NativePairPlacement) { "PairPlacement.1" } else { $PulseAttempt }
. (Join-Path $PSScriptRoot "wb-run-lib-claude.ps1")
$steam = "C:\Program Files (x86)\Steam\steamapps\common"
$wb = "$steam\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$gp = "$repo\tools\RICON_LiveProbe\addon.gproj"
$addons = "$steam\Arma Reforger\addons,$steam\Arma Reforger Tools\Workbench\addons"
$myGames = "$env:USERPROFILE\Documents\My Games"
$logsRoot = "$myGames\ArmaReforgerWorkbench\logs"
$regKey = "HKCU\Software\Bohemia Interactive"
$profileDir = "$myGames\ArmaReforgerWorkbench\profile"
$target = "Server localhost"
$required = @("Server localhost", "Server localhost + PeerTool")
# launcher markers (codex #4697)
$mRegistered = '\[RICON_WBMODE\] registered=1'
$mOpenOnly = '\[RICON_LIVEB\] state=OPEN_ONLY saved=0 isGameMode=0'
$mWaitPlugin = '\[RICON_LIVEB\] inspect=WAIT_PLUGIN'
$mEditStable = '\[RICON_LIVEB\] inspect=EDIT_STABLE'
$failPattern = '\[RICON_LIVEB\] (state=(NO_WORLD_ARGUMENT|NO_WORLD_EDITOR|NO_API|PLACEMENT_REFUSED|SETUP_REFUSED|PLAY_REQUESTED|BENCH_CREATED|ENDPOINT_ONLY)|inspect=REFUSED|restart=)|\[RICON_WBMODE\] registered=0|SCRIPT\s*\(E\)|\(E\): Addon \S+ dependency \S+ can.t be added|\[RICON_PROBE\] config=REFUSED'
$earlyPlay = 'kind=ENDPOINT_START|\[RICON_WBMODE\] event=|\[RICON_LIVEB\] inspect=(PLAY_ENTERED|PLUGIN_SEEN|PLUGIN_PLAY_REQUESTED)'
# the launcher's own command (codex #4850): exactly this name directly in the World Editor's Plugins menu
$pluginMenu = "Plugins"
$pluginItem = "RICON: Play"
# B0c client (codex #4906)
$clientExe = "$steam\Arma Reforger\ArmaReforgerSteamDiag.exe"
# game and Tools addons plus the repository's addons and tools (codex #4908); -addons is not set, as for the host
$clientAddons = "$steam\Arma Reforger\addons,$steam\Arma Reforger Tools\Workbench\addons,$repo\addons,$repo\tools"
$clientFail = 'SCRIPT\s*\(E\)|\(E\): Addon \S+ dependency \S+ can.t be added|\[RICON_PROBE\] config=REFUSED'
# run 202016: with -noBackend the host matched the client to the host player by the same Steam name and dropped it
# (kickCode 0x00060002). The SDK's -username (Page_CLI, Platform Service; documented visibility ENF_DEVELOPER) overrides
# that name for local test clients (codex #4953); only the client gets it, the host and its identity check stay as they
# are. Run 144401 showed the Diag client applies it: "signed in as" this name, host FindMatch made PlayerId=2.
$clientUser = "RICON_B0c_peer1"
# run 144401: right after that line the host showed the vanilla :1101 prompt again (once per joining player). The owner
# allowed one more exact "Yes" on it on 2026-10-26, only after this host line; 2 in all, only with -Client
# a whole log line from its time stamp on, spaces or tabs between the fields (codex #4975, #4976)
$peerJoined = '(?m)^\d\d:\d\d:\d\d\.\d{3}[ \t]+NETWORK[ \t]+: ### Creating player: PlayerId=2, Name=' + [regex]::Escape($clientUser) + '\r?$'
$yesTotal = if ($Client) { 2 } else { 1 }
# tagged log lines shown per side: the stream and its dump of up to 1024 events each, plus CRC and plugin lines
$taggedCap = 2400

if ($Tag -notmatch '^[a-z0-9][a-z0-9-]{0,39}$') { "BAD_TAG"; exit 2 }
if (-not (Test-Path $OutDir -PathType Container)) { "OUTDIR_MISSING"; exit 2 }
$OutDir = (Resolve-Path $OutDir).Path
if ($OutDir.StartsWith($repo, [StringComparison]::OrdinalIgnoreCase)) { "OUTDIR_INSIDE_REPO"; exit 2 }
if ($Seconds -lt 60 -or $Seconds -gt 600 -or $MaxChildren -lt 0) { "BAD_LIMITS"; exit 2 }
if ($Client -and $Seconds -lt 300) { "BAD_LIMITS (-Client needs -Seconds 300 or more)"; exit 2 }
if ($Client -and $ReadAuthOnly) { "BAD_MODES (-ReadAuthOnly never reaches Play)"; exit 2 }
$boundedDiscovery = $CommunicationDiscovery -or $VoNInputShapeDiscovery -or $VoNMappingInspection -or $NativeRxObserve -or $AudioMeterProbe
$discoveryModeCount = ([int][bool]$CommunicationDiscovery) + ([int][bool]$VoNInputShapeDiscovery) + ([int][bool]$VoNMappingInspection) + ([int][bool]$NativeRxObserve) + ([int][bool]$AudioMeterProbe)
if ($discoveryModeCount -gt 1) { "BAD_DISCOVERY_BOTH_MODES"; exit 2 }
$discoveryArgument = if ($AudioMeterProbe) { "-riconAudioMeter=1" } elseif ($NativeRxObserve) { "-riconNativeRx=1" } elseif ($VoNMappingInspection) { "-riconVoNMapping=1" } elseif ($VoNInputShapeDiscovery) { "-riconVoNInputShape=1" } else { "-riconCommunicationDiscovery=1" }
$discoveryRunArgument = if ($AudioMeterProbe) { "-riconAudioMeterRun" } elseif ($NativeRxObserve) { "-riconNativeRxRun" } else { "-riconCommunicationRun" }
$discoveryPrefix = if ($AudioMeterProbe) { "meter-" } elseif ($NativeRxObserve) { "nrx-" } elseif ($VoNMappingInspection) { "i0-" } elseif ($VoNInputShapeDiscovery) { "v0-" } else { "d0-" }
$d0RunId = ""
if ($boundedDiscovery) {
	if ((-not $Client -and -not $RestSingleHostControl) -or $ReadAuthOnly -or $AudioObserve -or $World -cne "{96A8AF57260A7392}worlds/MP/MpTest/MpTest.ent") { "BAD_D0_MODES"; exit 2 }
	$d0RunId = $CommunicationRun
	if (-not $d0RunId) { $d0RunId = $discoveryPrefix + (Get-Date -Format "yyyyMMdd-HHmmss") + "-" + [guid]::NewGuid().ToString("N").Substring(0, 8) }
	if ($d0RunId -cnotmatch '\A[A-Za-z0-9_.-]{1,64}\z') { "BAD_D0_RUN"; exit 2 }
} elseif ($CommunicationRun) { "BAD_D0_RUN_WITHOUT_MODE"; exit 2 }
$engineConfigPath = ""
$engineConfigSha = ""
if ($EngineConfig) {
    try {
        if ($EngineConfig -match '[\x00-\x1F\x7F\x22\x27]') { throw "Unsafe config path" }
        $resolvedEngineConfig = Resolve-Path -LiteralPath $EngineConfig -ErrorAction Stop
        if ($resolvedEngineConfig.Provider.Name -cne "FileSystem") { throw "Config is not a filesystem leaf" }
        $engineConfigPath = $resolvedEngineConfig.ProviderPath
        if (-not [IO.File]::Exists($engineConfigPath) -or [IO.Path]::GetExtension($engineConfigPath) -ine ".conf") { throw "Config is not a conf leaf" }
        $configNode = New-Object IO.FileInfo($engineConfigPath)
        while ($configNode) {
            if ($configNode.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Reparse config path" }
            if ($configNode -is [IO.FileInfo]) { $configNode = $configNode.Directory } else { $configNode = $configNode.Parent }
        }
        $engineConfigSha = (Get-FileHash -LiteralPath $engineConfigPath -Algorithm SHA256 -ErrorAction Stop).Hash
        if ($engineConfigSha -cnotmatch '\A[A-Fa-f0-9]{64}\z') { throw "Missing config digest" }
    } catch { "BAD_ENGINE_CONFIG_PATH"; exit 2 }
}
$fhcToken = if ($FileHandleCapabilityProbe) { [guid]::NewGuid().ToString("N") } else { "" }
$restToken = if ($RestCapabilityProbe) { [guid]::NewGuid().ToString("N") } else { "" }
$logsDir = "ricon-$Tag-" + (Get-Date -Format "yyyyMMdd-HHmmss")
$log = "$logsRoot\$logsDir\console.log"
$raw = Join-Path $OutDir "$logsDir-raw.jsonl"
$hostLogsArgument = $logsDir
$d0HostLogsName = ""
if ($boundedDiscovery) {
	$d0HostLogsName = "$logsDir-host-logs"
	$d0HostLogsPath = Join-Path $OutDir $d0HostLogsName
	$hostLogsArgument = "`"$d0HostLogsPath`""
	$log = Join-Path $d0HostLogsPath "console.log"
}
$argList = @("-gproj", "`"$gp`"", "-addonsDir", "`"$addons`"", "-noBackend")
if ($engineConfigPath) { $argList += @("-cfg", "`"$engineConfigPath`"") }
$argList += @("-wbModule=WorldEditor",
	"-logLevel", "verbose", "-logsDir", $hostLogsArgument, "-noFocus", "-maxFPS", "30",
	"-plugin=RICON_LiveBenchLauncherPlugin", "-riconBenchWorld=`"$world`"", "-riconBenchMode=inspect",
	"-riconEndpointProbe=1", "-riconBenchPluginStart=1", "-log-scr-checksum")
if ($Client -or $RestSingleHostControl) { $argList += @("-riconEndpointSamples=120", "-forceUpdate") }
if ($AudioObserve) { $argList += "-riconAudioObserve=1" }
$d0HostProfileName = ""
if ($boundedDiscovery) {
	$d0HostProfileName = "$logsDir-host-profile"
	if ($AudioMeterProbe) {
		$argList += @("-profile", "`"$(Join-Path $OutDir $d0HostProfileName)`"")
	} else {
		$argList += @($discoveryArgument, "$discoveryRunArgument=$d0RunId", "-profile", "`"$(Join-Path $OutDir $d0HostProfileName)`"")
	}
}
if ($PulseRehearsal) { $argList += @("-riconPulseMode=rehearsal", "-riconPulseAttempt=$PulseAttempt") }
if ($FileHandleCapabilityProbe) { $argList += @("-riconFileHandleCapability=1", "-riconFileHandleRun=$d0RunId", "-riconFileHandleToken=$fhcToken") }
if ($RestCapabilityProbe) { $argList += @("-riconRestCapability=1", "-riconRestPort=0", "-riconRestRun=$d0RunId", "-riconRestToken=$restToken") }
if ($RestGuardedRehearsal) { $argList += "-riconRestRehearsalArm=1" }
if ($RestGuardedCapture) { $argList += @("-riconRestCapturePulse=1", "-riconPulseMode=capture", "-riconPulseAttempt=$PulseAttempt") }
if ($CaptureNoSend) { $argList += "-riconCaptureNoSend=1" }
if ($CaptureWrongFrequency) { $argList += "-riconCaptureWrongFrequency=1" }
if ($CaptureDirect) { $argList += "-riconCaptureDirect=1" }
if ($CaptureBPowerOff) { $argList += "-riconCaptureBPowerOff=1" }
if ($NativePairPlacement) { $argList += "-riconPairPlacement=1" }
if ($NativeRadioPower) { $argList += "-riconNativeRadioPower=1" }
if ($CaptureSeparatedPair) { $argList += "-riconCaptureSeparatedPair=1" }
if ($CaptureShort400) { $argList += "-riconCaptureShort400=1" }
if ($CaptureBlindTrial) { $argList += "-riconCaptureBlindTrial=1" }
if ($WrongFrequencyControl) { $argList += "-riconVoNWrongFrequency=1" }
# Vanilla may ignore this hint or fall back to a random faction; runtime readback is required.
if ($AutoDeployFaction) { $argList += "-autodeployFaction=$AutoDeployFaction" }
if ($AudioMeterProbe) { "AUDIO_METER role=B_ONLY run=$d0RunId hostModes=NONE" }
if ($AudioMeterFlagOffControl) { "AUDIO_METER_CONTROL case=FLAG_OFF meterFlags=OMITTED" }
if ($engineConfigPath) { "ENGINE_CONFIG sha256=$engineConfigSha" }
if ($PulseRehearsal) { "PULSE_REHEARSAL attempt=$PulseAttempt autoArm=0 source=HOST_ONLY" }
if ($NativePairPlacement) { "NATIVE_PAIR_PLACEMENT configured=1 passive=1 typed=1 attempt=$transactionAttemptId geometryAudit=REQUIRED" }
if ($NativeRadioPower) { "NATIVE_RADIO_POWER configured=1 passive=1 typed=1 attempt=$transactionAttemptId stateAudit=REQUIRED" }
if ($RestBackendControl) {
    if (@($argList | Where-Object { $_ -ceq "-noBackend" }).Count -ne 1) { "BAD_REST_BACKEND_CONTROL_FLAG_COUNT"; exit 2 }
    $argList = @($argList | Where-Object { $_ -cne "-noBackend" })
    "REST_BACKEND_CONTROL hostFlagOmitted=1 clientUnchanged=1"
}
$hostArgsText = $argList -join " "
if ($engineConfigPath) { $hostArgsText = $hostArgsText.Replace($engineConfigPath, "<engine-config>") }
if ($FileHandleCapabilityProbe) { $hostArgsText = $hostArgsText.Replace($fhcToken, "<fhc-token>") }
if ($RestCapabilityProbe) { $hostArgsText = $hostArgsText.Replace($restToken, "<rest-token>").Replace("-riconRestPort=0", "-riconRestPort=<ephemeral>") }
if ($RestSingleHostControl) { "REST_SINGLE_HOST_CONTROL hostOnly=1 clientStarted=0" }
"ARGS " + $hostArgsText.Replace($OutDir, "<scratchpad>")
"RUNNER " + (Get-FileHash $PSCommandPath -Algorithm SHA256).Hash.Substring(0, 16) + " LIB " + (Get-FileHash (Join-Path $PSScriptRoot "wb-run-lib-claude.ps1") -Algorithm SHA256).Hash.Substring(0, 16)
"LIMITS seconds=$Seconds watchdogSeconds=$($Seconds + 30) settle=$Settle maxChildren=$MaxChildren readAuthOnly=$ReadAuthOnly yesTotal=$yesTotal"
"PLUGIN menu=`"$pluginMenu`" item=`"$pluginItem`""
$busy = @(Get-Process | Where-Object { $_.ProcessName -match '(?i)reforger|workbench|enfusion' })
if ($busy.Count -gt 0) { "BUSY count=$($busy.Count)"; exit 3 }
# the client's host address and the scratchpad path, masked in every repo-facing line (codex #4918); set once here and
# by the -Client preflight below, never reset later (codex #4935)
$script:secrets = @()
if ($Client) {
	if (-not (Test-Path $clientExe)) { "CLIENT_EXE_MISSING"; exit 3 }
	$ha = LocalHostAddress
	"CLIENT exe=ArmaReforgerSteamDiag.exe ver=$((Get-Item $clientExe).VersionInfo.FileVersion) hostAddress=$($ha.result) candidates=$($ha.count) class=$(if ($ha.address) { AddrClass $ha.address } else { '-' })"
	if ($ha.result -ne "OK") { "HOST_ADDRESS_$($ha.result) (no client is started)"; exit 3 }
	$ppName = "$logsDir-peer1-profile"; $clName = "$logsDir-peer1-logs"
	$clientArgs = @("-gproj", "`"$gp`"", "-addonsDir", "`"$clientAddons`"")
	if ($engineConfigPath) { $clientArgs += @("-cfg", "`"$engineConfigPath`"") }
	$clientArgs += @("-window", "-screenWidth", "960", "-screenHeight", "540",
		"-maxFPS", "20", "-noFocus", "-forceUpdate", "-noBackend", "-username", $clientUser, "-client", $ha.address, "-profile", "`"$(Join-Path $OutDir $ppName)`"",
		"-logsDir", "`"$(Join-Path $OutDir $clName)`"", "-logLevel", "verbose", "-log-scr-checksum", "-riconEndpointProbe=1")
	if ($AudioObserve) { $clientArgs += "-riconAudioObserve=1" }
	if ($boundedDiscovery -and -not $AudioMeterFlagOffControl) { $clientArgs += @($discoveryArgument, "$discoveryRunArgument=$d0RunId") }
	if ($WrongFrequencyControl) { $clientArgs += "-riconVoNWrongFrequency=1" }
	if ($CaptureWrongFrequency) { $clientArgs += "-riconCaptureWrongFrequency=1" }
	if ($CaptureDirect) { $clientArgs += "-riconCaptureDirect=1" }
	if ($CaptureBPowerOff) { $clientArgs += "-riconCaptureBPowerOff=1" }
	if ($NativePairPlacement) { $clientArgs += "-riconPairPlacement=1" }
	if ($NativeRadioPower) { $clientArgs += "-riconNativeRadioPower=1" }
	if ($CaptureSeparatedPair) { $clientArgs += "-riconCaptureSeparatedPair=1" }
	if ($BlindBObserve) { $clientArgs += "-riconBlindBObserve=1" }
	if ($VoNSoundObserve) { $clientArgs += "-riconVoNSoundObserve=1" }
	$clientArgsText = $clientArgs -join " "
	if ($engineConfigPath) { $clientArgsText = $clientArgsText.Replace($engineConfigPath, "<engine-config>") }
	"CLIENT_ARGS " + $clientArgsText.Replace($ha.address, "<host PRIVATE_V4>").Replace($OutDir, "<scratchpad>")
	$script:secrets = @($ha.address, $OutDir)
}
if ($RestSingleHostControl) { $script:secrets += $OutDir }
if ($engineConfigPath) { $script:secrets += $engineConfigPath }
if ($RestCapabilityProbe) { $script:secrets += $restToken }
if ($FileHandleCapabilityProbe) { $script:secrets += $fhcToken }
if ($armedPulse) {
    try {
        $pulseHelperPath = Join-Path $PSScriptRoot "pulse-rehearsal-coordinator.ps1"
        $pulseGuardPath = Join-Path $PSScriptRoot "cutoff-guard.ps1"
        if ((Get-FileHash -LiteralPath $pulseHelperPath -Algorithm SHA256 -ErrorAction Stop).Hash -cne "911E12C49032390A5479EBA4A3FE39465CF10A86F9D19C28E8C48F59FA4EC7E9") { throw "Helper pin" }
        if ((Get-FileHash -LiteralPath $pulseGuardPath -Algorithm SHA256 -ErrorAction Stop).Hash -cne "C3D17A4E805783DD889D1C74EAE85EFEAE721F251149E8CE341730065663FD85") { throw "Guard pin" }
    } catch { "BAD_ARMED_REHEARSAL_HELPERS"; exit 2 }
    if ($RestGuardedCapture) { "GUARDED_CAPTURE optIn=1 autoArmAtStartup=0 oneAttempt=1 media=NOT_TESTED" } else { "ARMED_REHEARSAL optIn=1 mode=rehearsal autoArmAtStartup=0 oneAttempt=1" }
}
if ($FileHandleCapabilityProbe) {
    try {
        $fhcHelperPath = Join-Path $PSScriptRoot "file-handle-capability-writer.ps1"
        if ((Get-FileHash -LiteralPath $fhcHelperPath -Algorithm SHA256 -ErrorAction Stop).Hash -cne "B6F851C47AE1BAECE18705CAC6F2509BAC6256B2268A3835CC33034C1C240D45") { throw "Writer pin" }
    } catch { "BAD_FILE_HANDLE_CAPABILITY_HELPER"; exit 2 }
    "FHC_PARENT status=CONFIGURED"
}
if ($RestCapabilityProbe) {
    try {
        $restHelperPath = Join-Path $PSScriptRoot "rest-capability-responder.ps1"
        if ((Get-FileHash -LiteralPath $restHelperPath -Algorithm SHA256 -ErrorAction Stop).Hash -cne "1373A6FA6924F81E463C275133F7FEAA3B2BD6B7CE8DC83B7C9489F4177DD30B") { throw "Responder pin" }
    } catch { "BAD_REST_CAPABILITY_HELPER"; exit 2 }
    "REST_PARENT status=CONFIGURED"
}
if ($typedTransaction) {
    $rrWriterPath = "C:\Users\Ded\Documents\Projects\ARMA 3 Scripts\Cellular System\output\agora-native-von-20261103\recovery-staging\full-restore-live-candidate\recovery-transaction.ps1"
    $rrPolicyPath = "C:\Users\Ded\Documents\Projects\ARMA 3 Scripts\Cellular System\output\agora-native-von-20261103\recovery-staging\recovery-policy.ps1"
    $rrWorkerPath = "C:\Users\Ded\Documents\Projects\ARMA 3 Scripts\Cellular System\output\agora-native-von-20261103\recovery-staging\full-restore-live-candidate\wb-edit-recovery-stage.ps1"
    try {
        foreach ($pair in @(@($rrWriterPath,"2997C681814A2829F777718CB134716ED8C9CF5F5ABCCE824A5620C43104AAC7"),@($rrPolicyPath,"E705D29EE79A469D9E2C9A844B08E7F233C941D236B794E33A402A80999C4570"),@($rrWorkerPath,"1C870AF8AF62EEB6C1A541D2BFB1B7EB471388FAB705E7818E7830D2479C1544"))) {
            if ((Get-FileHash -LiteralPath $pair[0] -Algorithm SHA256 -ErrorAction Stop).Hash -cne $pair[1]) { throw "Recovery pin" }
        }
    } catch { "BAD_REST_REHEARSAL_RECOVERY_PIN"; exit 2 }
}
if ($DryRun) { "DRY_RUN no process started"; exit 0 }
if ($typedTransaction) { . $rrWriterPath; $script:rrTransaction=$null; $script:rrWriterEntered=$false; $script:rrClosed=$null; $script:rrCloseErrors=@(); $script:rrWriterErrors=@() }
if ($RestCapabilityProbe) {
    . $restHelperPath
    $script:restWorker = $null
    $script:restWorkerReceipt = $null
}
if ($FileHandleCapabilityProbe) { . $fhcHelperPath }
if ($armedPulse) {
    . $pulseGuardPath
    . $pulseHelperPath
    if (-not $restGuarded) { Initialize-RehearsalProxy }
    $script:pulseCoordinator = New-RehearsalCoordinator $d0RunId $PulseAttempt -Capture:$RestGuardedCapture -NoSend:$CaptureNoSend -WrongFrequency:$CaptureWrongFrequency -SeparatedPair:$CaptureSeparatedPair -Short400:$CaptureShort400 -BlindTrial:$CaptureBlindTrial -VoNSoundObserve:$VoNSoundObserve -BPowerOff:$CaptureBPowerOff -Direct:$CaptureDirect
    if ($CaptureBlindTrial) { Initialize-BlindParent $script:pulseCoordinator $OutDir (Join-Path $PSScriptRoot 'capture-blind-rule.json') (Join-Path $PSScriptRoot 'capture-blind-bjudge.ps1') }
    if ($CaptureShort400 -or $CaptureBlindTrial) { $script:shortBStopFacts = [pscustomobject]@{beginTicks=0L;endTicks=0L;requested=$null;exited=$null} }
}
if ($boundedDiscovery) {
	if ($FileHandleCapabilityProbe -and ([IO.Directory]::Exists((Join-Path $OutDir $d0HostProfileName)) -or [IO.Directory]::Exists((Join-Path $OutDir $d0HostLogsName)))) { "FHC_PARENT status=DIRS_NOT_FRESH"; exit 3 }
	$d0HostProfile = NewPeerProfile $OutDir $d0HostProfileName
	if (-not $d0HostProfile.ok) { "D0_HOST_PROFILE why=$($d0HostProfile.why)"; exit 3 }
	$d0HostLogs = NewPeerProfile $OutDir $d0HostLogsName
	if (-not $d0HostLogs.ok) { "D0_HOST_LOGS why=$($d0HostLogs.why)"; exit 3 }
	"D0_HOST_PROFILE new=1 logsNew=1 root=<scratchpad>"
	if ($FileHandleCapabilityProbe) {
		try { $script:fhcWriter = New-FhcWriter $d0HostProfile.path (Join-Path $d0HostProfile.path "profile") $d0HostLogs.path $log $d0RunId $fhcToken }
		catch { "FHC_PARENT status=EMPTY_CREATE_FAILED"; exit 3 }
		"FHC_PARENT status=EMPTY_CREATED digest=$($script:fhcWriter.PayloadDigest)"
	}
}
if ($Client) {
	# both directories new and empty, outside the repository (-OutDir is checked above)
	$pp = NewPeerProfile $OutDir $ppName
	$cl = NewPeerProfile $OutDir $clName
	if (-not $pp.ok -or -not $cl.ok) { "CLIENT_DIRS profile=$($pp.why) logs=$($cl.why)"; exit 3 }
}

$srcFiles = Get-ChildItem "$repo\tools\RICON_LiveProbe" -Recurse -Include *.c, *.gproj | Sort-Object FullName
function SrcSnap { $srcFiles | ForEach-Object { (Get-FileHash $_.FullName -Algorithm SHA256).Hash + "  " + $_.FullName.Substring($repo.Length + 1) } }
function SettingsSnap($label) {
	$out = Join-Path $OutDir "$logsDir-settings-$label.reg"
	$null = & reg.exe export $regKey $out /y
	$prof = @(Get-ChildItem $profileDir -Recurse -File -Force | Sort-Object FullName | ForEach-Object { (Get-FileHash $_.FullName -Algorithm SHA256).Hash + "  " + $_.Name })
	$games = @(Get-ChildItem $myGames -Directory | Where-Object { $_.Name -match '(?i)reforger' } | ForEach-Object { $_.Name + "=" + @(Get-ChildItem $_.FullName -Recurse -File -Force -ErrorAction SilentlyContinue).Count })
	return @{ reg = @(Get-Content $out); prof = $prof; games = $games; file = $out }
}
function Raw($obj) { ($obj | ConvertTo-Json -Compress -Depth 4) | Add-Content -Path $raw -Encoding UTF8 }
function LogText { if (Test-Path $log) { return (Get-Content $log -Raw) } else { return "" } }

$srcBefore = SrcSnap
$setBefore = SettingsSnap "before"
"SETTINGS_BEFORE regLines=$($setBefore.reg.Count) profileFiles=$($setBefore.prof.Count) reforgerDirs=`"$($setBefore.games -join ',')`""
$baseline = @{}
foreach ($p in @(Get-CimInstance Win32_Process -Property ProcessId)) { $baseline[[uint32]$p.ProcessId] = $true }
$logDirsBefore = @(Get-ChildItem $logsRoot -Directory | ForEach-Object { $_.Name })

$tracked = @{}; $netSeen = @{}; $newCount = 0
$script:mainFound = $false; $script:played = $false; $script:switchedOn = $false; $script:invokeJob = $null
$script:playWin = @{}   # Play title -> visible handle already read once
# B0c client: NOT_STARTED -> RUNNING -> EXITED | STOPPED_<why> | STOP_FAILED_<why>; started at most once
$script:clientProc = $null; $script:clientPid = [uint32]0; $script:clientState = "NOT_STARTED"; $script:clientSeen = @{}
$script:clientAt = $null; $script:clientStart = $null; $script:clientLog = ""; $script:clientDog = $null

# A stop is requested, then confirmed or not (codex #4917); only a confirmed exit counts as stopped.
function StopClient([string]$why) {
	$r = StopOwned $script:clientProc 5000
	$script:clientState = if ($r.exited) { "STOPPED_$why" } else { "STOP_FAILED_$why" }
	"CLIENT_STOP why=$why requested=$($r.requested) exited=$($r.exited) error=`"$($r.error)`"" | Write-Host
}
# The client part only (plan B0c in LIVE-peer-local-plan-codex.md, 3f2ce83): the client root, then its children through
# their held handles; the host goes on to edit and restores the setting. Done once.
function StopClientPart([string]$why) {
	if ($script:clientState -eq "RUNNING") { StopClient $why }
	foreach ($k in @($tracked.Keys | Where-Object { $tracked[$_].client })) {
		$h = $tracked[$k].held
		if (-not $h) { "CLIENT_CHILD_STOP pid=$k skipped=NO_HANDLE" | Write-Host; continue }
		$r = StopOwned $h 3000
		"CLIENT_CHILD_STOP pid=$k requested=$($r.requested) exited=$($r.exited) error=`"$($r.error)`"" | Write-Host
	}
}
# Children of the own client root, also after the root exited (codex #4917): each gets a held handle at first sight,
# so teardown can stop it through that handle like a Workbench descendant. Returns the PIDs unrolled; the caller
# collects them with @() (codex #4930: "return , $kids" made an empty list one element there).
function ClientChildren([int]$t, $procs) {
	if (-not $script:clientProc) { return }
	$exitAt = $null
	try { $script:clientProc.Refresh(); if ($script:clientProc.HasExited) { $exitAt = $script:clientProc.ExitTime } } catch {}
	$kids = @(OwnedDescendants $procs $script:clientPid $script:clientStart $exitAt)
	foreach ($k in $kids) {
		$row = @($procs | Where-Object { [uint32]$_.ProcessId -eq $k })[0]
		if (-not $tracked.ContainsKey($k)) {
			$tracked[$k] = @{ name = $row.Name; ppid = [uint32]$row.ParentProcessId; created = $row.CreationDate; descendant = $true; held = (OpenHeld $k $row.CreationDate); client = $true }
			Raw @{ t = $t; kind = "CLIENT_CHILD"; pid = $k; ppid = [uint32]$row.ParentProcessId; name = $row.Name; cmd = $row.CommandLine }
		} elseif (-not $tracked[$k].held) {
			# seen first by the general scan as a new game process without a handle
			$tracked[$k].held = OpenHeld $k $row.CreationDate; $tracked[$k].descendant = $true; $tracked[$k].client = $true
		}
	}
	return $kids
}
# One client step inside Tick; "" or the stop reason. A failing client is only stopped: the host goes on to edit and restore.
function ClientTick([int]$t, [string]$text, $procs, $eps) {
	if ($script:clientState -eq "NOT_STARTED") {
		$why = ClientStartBlock $script:played $text $eps $wbPid
		if ($why -eq "OBSERVATION_OVER") { $script:clientState = "NOT_STARTED_OBSERVATION_OVER"; "CLIENT_NOT_STARTED t=$t reason=OBSERVATION_OVER" | Write-Host; return "" }
		if ($why) { return "" }
		# the address once more, right before the start; the host goes on either way
		$why = ClientAddressRecheck $ha (LocalHostAddress)
		if ($why) { $script:clientState = "NOT_STARTED_$why"; "CLIENT_NOT_STARTED t=$t reason=$why" | Write-Host; return "" }
		$script:clientState = "RUNNING"
		$script:clientProc = Start-Process -FilePath $clientExe -ArgumentList $clientArgs -PassThru -WorkingDirectory $repo
		$null = $script:clientProc.Handle
		$script:clientPid = [uint32]$script:clientProc.Id
		$script:clientStart = $script:clientProc.StartTime
		$script:clientAt = Get-Date
		$script:clientDog = StartWatchdog $script:clientProc ([int][math]::Max(1000, ($deadline - (Get-Date)).TotalMilliseconds + 20000))
		"CLIENT_STARTED t=$t pid=$($script:clientPid) watchdog=$($script:clientDog.Id)" | Write-Host
		Raw @{ t = $t; kind = "CLIENT_STARTED"; pid = $script:clientPid; address = $ha.address; args = ($clientArgs -join " ") }
		return ""
	}
	if (-not $script:clientProc) { return "" }   # NOT_STARTED_<why>: no client root, nothing to watch
	$kids = @(ClientChildren $t $procs)
	$fresh = @($kids | Where-Object { -not $tracked[$_].reported })
	if ($fresh.Count -gt 0) {
		foreach ($k in $fresh) { $tracked[$k].reported = $true }
		$names = @($procs | Where-Object { $fresh -contains [uint32]$_.ProcessId } | ForEach-Object { $_.Name }) -join ","
		"CHILD_OF_CLIENT t=$t pids=$($fresh -join ',') names=$names (read only; the client part is stopped)" | Write-Host
		StopClientPart "CHILD_OF_CLIENT"
		if ($script:clientState -notlike "STOP*") { $script:clientState = "STOPPED_CHILD_OF_CLIENT" }
		return ""
	}
	if ($script:clientState -ne "RUNNING") { return "" }
	if ($script:clientProc.HasExited) {
		$script:clientState = "EXITED"
		"CLIENT_EXITED t=$t code=$($script:clientProc.ExitCode) afterS=$([int]((Get-Date) - $script:clientAt).TotalSeconds)" | Write-Host
		return ""
	}
	# planned end before the host leaves Play; not a guarantee that the client goes before the session ends (codex #4912),
	# only that it is stopped before Workbench. Checked before the client's windows are read.
	if ($text -match '\[RICON_LIVEB\] inspect=(OBSERVATION_END|EDIT_REQUESTED)') { StopClientPart "HOST_OBSERVATION_END"; return "" }
	$w = ClientWindowStep $script:clientPid $script:clientSeen $t $script:secrets
	# a permission prompt in the client is never answered; the client part stops, the host restores
	if ($w) { "CLIENT_DIALOG t=$t kind=$w (read only)" | Write-Host; StopClientPart $w; return "" }
	if (-not $script:clientLog) {
		$f = @(Get-ChildItem $cl.path, $pp.path -Recurse -Filter console.log -File -ErrorAction SilentlyContinue | Select-Object -First 1)
		if ($f.Count -gt 0) {
			$script:clientLog = $f[0].FullName
			"CLIENT_LOG t=$t under=$(if ($f[0].FullName.StartsWith($cl.path)) { 'logsDir' } else { 'profile' })" | Write-Host
		} elseif (((Get-Date) - $script:clientAt).TotalSeconds -gt 60) { "CLIENT_NO_LOG t=$t" | Write-Host; StopClientPart "NO_LOG"; return "" }
	}
	if ($script:clientLog) {
		# as LogText: Get-Content shares the file with the writing game
		$ct = Get-Content $script:clientLog -Raw -ErrorAction SilentlyContinue
		if ($ct -match $clientFail) { "CLIENT_LOG_FAILURE t=$t" | Write-Host; StopClientPart "LOG_FAILURE" }
	}
	return ""
}
$phase = "START"; $reason = ""; $result = "NOT_REACHED"; $base = $null; $main = [IntPtr]::Zero
$started = Get-Date
$deadline = $started.AddSeconds($Seconds)
# the gproj dependencies resolve through "./addons", so Workbench must start in the repository root (run 171812 started
# in ops\, lost RICON_Core/RICON_CoreTests and with them the launcher)
if ($RestCapabilityProbe) {
    try {
        $script:restResponder = New-RestCapabilityResponder $d0RunId $restToken -GuardedRehearsal:$RestGuardedRehearsal -GuardedCapture:$RestGuardedCapture -Attempt $PulseAttempt -HostLog $log -NoSend:$CaptureNoSend -WrongFrequency:$CaptureWrongFrequency -SeparatedPair:$CaptureSeparatedPair -Short400:$CaptureShort400 -BlindTrial:$CaptureBlindTrial -BPowerOff:$CaptureBPowerOff -Direct:$CaptureDirect
        $restPortIndex = [array]::IndexOf($argList, "-riconRestPort=0")
        if ($restPortIndex -lt 0) { throw "Missing port placeholder" }
        $argList[$restPortIndex] = "-riconRestPort=$($script:restResponder.Port)"
        "REST_PARENT status=LISTENING_LOOPBACK"
    } catch { if ($script:restResponder) { Close-RestCapabilityResponder $script:restResponder }; "REST_PARENT status=STARTUP_FAILED"; exit 3 }
}
try {
    $wbProc = Start-Process -FilePath $wb -ArgumentList $argList -PassThru -WindowStyle Hidden -WorkingDirectory $repo
} catch { if ($RestCapabilityProbe) { Close-RestCapabilityResponder $script:restResponder }; throw }
$wbPid = [uint32]$wbProc.Id

# One observation step; "" or the stop reason. Before Play nothing may start and no prompt may show.
function Tick {
	$t = [int]((Get-Date) - $started).TotalSeconds
	$wbProc.Refresh()
	if ($wbProc.HasExited) { return "WORKBENCH_EXITED" }
	if ((Get-Date) -gt $deadline) { return "DEADLINE" }
	if ($FileHandleCapabilityProbe) {
		$fhcBefore = $script:fhcWriter.Status
		$fhcStop = Invoke-FhcWriterStep $script:fhcWriter $wbProc
		if ($script:fhcWriter.Status -cne $fhcBefore) { "FHC_PARENT status=$($script:fhcWriter.Status) digest=$($script:fhcWriter.PayloadDigest)" | Write-Host }
		if ($fhcStop) { return $fhcStop }
	}
	$procs = @(Get-CimInstance Win32_Process -Property ProcessId, ParentProcessId, Name, CreationDate, CommandLine)
	$desc = @(Descendants $procs $wbPid)
	$now = @{}
	foreach ($p in $procs) {
		$id = [uint32]$p.ProcessId
		$isDesc = $desc -contains $id
		$isNewGame = (-not $baseline.ContainsKey($id)) -and $p.Name -match '(?i)reforger'
		if (-not ($isDesc -or $isNewGame)) { continue }
		$now[$id] = $true
		if ($tracked.ContainsKey($id)) { continue }
		$held = if ($isDesc -and $id -ne $wbPid) { OpenHeld $id $p.CreationDate } else { $null }
		$tracked[$id] = @{ name = $p.Name; ppid = [uint32]$p.ParentProcessId; created = $p.CreationDate; descendant = $isDesc; held = $held }
		if ($id -eq $wbPid) { continue }
		$script:newCount++
		$cmdSha = if ($p.CommandLine) { [BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash([Text.Encoding]::UTF8.GetBytes($p.CommandLine))).Replace("-", "").Substring(0, 16) } else { "NONE" }
		"PROC_NEW t=$t phase=$phase pid=$id ppid=$($p.ParentProcessId) name=$($p.Name) descendant=$isDesc held=$([bool]$held) cmdSha=$cmdSha flags=`"$((FlagNames $p.CommandLine) -join ' ')`"" | Write-Host
		Raw @{ t = $t; kind = "PROC_NEW"; pid = $id; ppid = [uint32]$p.ParentProcessId; name = $p.Name; descendant = $isDesc; cmd = $p.CommandLine }
		if (-not $script:played) { return "NEW_PROCESS_BEFORE_PLAY" }
	}
	foreach ($id in @($tracked.Keys)) {
		if (-not $now.ContainsKey($id) -and -not $tracked[$id].ContainsKey("gone")) { $tracked[$id]["gone"] = $t; "PROC_GONE t=$t pid=$id name=$($tracked[$id].name)" | Write-Host; Raw @{ t = $t; kind = "PROC_GONE"; pid = $id } }
	}
	if ($script:newCount -gt $MaxChildren) { return "TOO_MANY_PROCESSES" }
	$live = @($now.Keys)
	$eps = @()
	$eps += @(Get-NetUDPEndpoint -ErrorAction SilentlyContinue | Where-Object { $live -contains [uint32]$_.OwningProcess } | ForEach-Object { @{ proto = "UDP"; addr = $_.LocalAddress; port = $_.LocalPort; pid = [uint32]$_.OwningProcess } })
	$eps += @(Get-NetTCPConnection -State Listen -ErrorAction SilentlyContinue | Where-Object { $live -contains [uint32]$_.OwningProcess } | ForEach-Object { @{ proto = "TCP_LISTEN"; addr = $_.LocalAddress; port = $_.LocalPort; pid = [uint32]$_.OwningProcess } })
	foreach ($e in $eps) {
		$key = "$($e.pid)|$($e.proto)|$($e.addr)|$($e.port)"
		if ($netSeen.ContainsKey($key)) { continue }
		$netSeen[$key] = $t
		"NET_NEW t=$t phase=$phase pid=$($e.pid) proto=$($e.proto) class=$(AddrClass $e.addr) port=$($e.port)" | Write-Host
		Raw @{ t = $t; kind = "NET_NEW"; pid = $e.pid; proto = $e.proto; addr = $e.addr; port = $e.port }
	}
	$text = LogText
	if ($text -match $failPattern) { return "LOG_FAILURE" }
	if (-not $script:played -and $text -match $earlyPlay) { return "PLAY_BEFORE_REQUEST" }
	$j = $script:invokeJob
	if ($j -and -not $j.done -and (PollAsyncInvoke $j)) {
		"PLUGIN_INVOKE_DONE t=$t result=$($j.result) afterMs=$([int]((Get-Date) - $j.started).TotalMilliseconds)" | Write-Host
		Raw @{ t = $t; kind = "PLUGIN_INVOKE_DONE"; result = $j.result }
	}
	if (-not $ReadAuthOnly) {
		# with -Client the log is read again once a prompt is up: in run 144401 the host wrote the PlayerId=2 line before
		# the prompt, and the Tick's $text may be older than the window. Without the line the prompt still stops the run.
		# The second press also needs our own client started in this run (codex #4971)
		$yText = $text
		if ($Client -and [RiconWin]::FindByTitle($wbPid, $script:AuthTitle) -ne [IntPtr]::Zero) { $yText = LogText }
		$yAllowed = SitesAllowed $script:played $yText $(if ($Client -and $script:clientProc) { $peerJoined } else { "" })
		$y = YesGuardSites $wbPid $t $yAllowed $yesTotal
		if ($restGuarded -and $y -ceq "SITE_REPEAT" -and $Client -and $script:clientProc -and $yesTotal -eq 2 -and $script:yesPressed -lt $yesTotal) {
		    $peerOwnLive = $false
		    try { $script:clientProc.Refresh(); $peerOwnLive = -not $script:clientProc.HasExited } catch {}
		    if ($peerOwnLive) {
		        $peerFreshText = LogText
		        $peerFreshAllowed = SitesAllowed $script:played $peerFreshText $peerJoined
		        $peerBeforeQuota = @($yAllowed | Where-Object { $_ -ceq "ARSENAL_1101" }).Count
		        $peerAfterQuota = @($peerFreshAllowed | Where-Object { $_ -ceq "ARSENAL_1101" }).Count
		        $peerProofChanged = ($peerBeforeQuota -eq 1 -and $peerAfterQuota -eq 2 -and $yText -notmatch $peerJoined -and $peerFreshText -match $peerJoined)
		        "AUTH_PEER_REFRESH beforeQuota=$peerBeforeQuota afterQuota=$peerAfterQuota proofChanged=$([int]$peerProofChanged)" | Write-Host
		        $peerStillLive = $false
		        try { $peerStillLive = -not $script:clientProc.HasExited } catch {}
		        if ($peerProofChanged -and $peerStillLive -and $script:yesPressed -lt $yesTotal) { $y = YesGuardSites $wbPid $t $peerFreshAllowed $yesTotal }
		    }
		}
		if ($y -notin @("NONE", "CLOSING", "NOT_READY", "PRESSED")) { return "DIALOG_$y" }
	} elseif (($ah = [RiconWin]::FindByTitle($wbPid, $script:AuthTitle)) -ne [IntPtr]::Zero) {
		"AUTH_PROMPT before Play (read only) t=$t handle=$ah" | Write-Host
		# names only, at most 3 reads 1 s apart until a button shows (the window may still be filling)
		for ($r = 1; $r -le 3; $r++) {
			Start-Sleep -Seconds 1
			"AUTH_READ try=$r t=$([int]((Get-Date) - $started).TotalSeconds)" | Write-Host
			try { $all = DialogElements $ah } catch { "  UIA_ERROR $($_.Exception.GetType().Name): $($_.Exception.Message)" | Write-Host; break }
			if (@($all | Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button }).Count -gt 0) { break }
		}
		return "AUTH_PROMPT_READ"
	}
	if ($Client) {
		$c = ClientTick $t $text $procs $eps
		if ($c) { return $c }
	}
	if ($armedPulse -and $phase -ceq "OBSERVE" -and $script:clientState -ceq "RUNNING" -and $script:invokeJob.done) {
		$readPeer = { if ($script:clientLog -and (Test-Path $script:clientLog)) { Get-Content $script:clientLog -Raw } else { "" } }
		if ($restGuarded) {
			$armReason = Invoke-RestGuardedRehearsalStep $script:pulseCoordinator $script:restWorker $wbProc $script:clientProc $script:main { LogText } $readPeer
		} else {
			$armReason = Invoke-RehearsalStep $script:pulseCoordinator $wbProc $script:clientProc $script:main { LogText } $readPeer
		}
		if ($armReason) { return $armReason }
	}
	foreach ($id in $live) {
		# the own client and its children: ClientTick reads the client's windows and stops the client part for a child
		if ($Client -and ($id -eq $script:clientPid -or ($tracked.ContainsKey($id) -and $tracked[$id].client))) { continue }
		if ($id -eq $wbPid) {
			$inPlay = $script:played -and $text -match '\[RICON_LIVEB\] inspect=PLUGIN_SEEN' -and $text -notmatch $mEditStable
			$u = UnknownWindowStartup $wbPid $script:mainFound $t $inPlay
			if ($u) { "UNKNOWN_WINDOW t=$t pid=$id title=`"$u`" (read only)" | Write-Host; return "DIALOG_UNKNOWN_WINDOW" }
			if ($inPlay) {
				# passive: no input ever; one diagnostic read per new visible handle (codex #4870), not one per Tick
				foreach ($pt in $script:PlayTitles) {
					$ph = [RiconWin]::FindByTitle($wbPid, $pt)
					if ($ph -eq [IntPtr]::Zero) {
						if ($script:playWin.ContainsKey($pt)) { "PLAY_WINDOW_HIDDEN t=$t title=`"$pt`" (no visible window with it)" | Write-Host; $script:playWin.Remove($pt) }
						continue
					}
					if ($script:playWin.ContainsKey($pt) -and $script:playWin[$pt] -eq $ph) { continue }
					$script:playWin[$pt] = $ph
					$cls = @([RiconWin]::List($wbPid) | Where-Object { $_.StartsWith("V|") -and ($_ -split "\|", 3)[2] -ceq $pt } | ForEach-Object { ($_ -split "\|", 3)[1] }) -join ","
					"PLAY_WINDOW t=$t title=`"$pt`" handle=$ph class=$cls (passive, read once)" | Write-Host
					try {
						$pr = PlayWindowRead $ph
						"  PW size=$($pr.size) elements=$($pr.count) buttons=$($pr.buttons)" | Write-Host
						foreach ($l in $pr.lines) { "  PW $l" | Write-Host }
						Raw @{ t = $t; kind = "PLAY_WINDOW"; title = $pt; cls = $cls; size = $pr.size; elements = $pr.count; buttons = $pr.buttons }
					} catch { "  PW READ_ERROR $($_.Exception.GetType().Name)" | Write-Host }
				}
			}
			continue
		}
		if ([RiconWin]::FindByTitle($id, $script:AuthTitle) -ne [IntPtr]::Zero) { "PROMPT_IN_CHILD pid=$id (read only)" | Write-Host; return "DIALOG_PROMPT_IN_CHILD" }
		$u = UnknownWindow $id $false
		if ($u) { "UNKNOWN_WINDOW t=$t pid=$id title=`"$u`" (read only)" | Write-Host; return "DIALOG_UNKNOWN_WINDOW" }
	}
	return ""
}
# Tick until $pattern is in the log (or until the editor window is found when $pattern is "MAIN"); "" or the stop reason.
function WaitFor([string]$pattern, [string]$label) {
	while ($true) {
		$b = Tick
		if ($b) { return $b }
		if ($pattern -eq "MAIN") {
			foreach ($w in @([RiconWin]::List($wbPid))) {
				$parts = $w -split "\|", 3
				if ($parts[0] -eq "V" -and $parts[2].StartsWith("World Editor - ", [StringComparison]::Ordinal)) {
					$script:mainFound = $true
					$script:main = [RiconWin]::FindByTitle($wbPid, $parts[2])
					"MAIN_WINDOW t=$([int]((Get-Date) - $started).TotalSeconds) title=`"$(Short $parts[2])`"" | Write-Host
					return ""
				}
			}
		} elseif ((LogText) -match $pattern) { "SEEN $label t=$([int]((Get-Date) - $started).TotalSeconds)" | Write-Host; return "" }
		Start-Sleep -Seconds 1
	}
}
# One Invoke of the target from $expected in a fresh menu, then a Tick; "" or the reason.
function FlipTarget([string]$expected) {
	$b = Tick
	if ($b) { return $b }
	for ($try = 1; $try -le 3; $try++) {
		$h = OpenOptions $script:main $wbPid
		# not opened: nothing was acted on, so another attempt is safe
		if ($null -eq $h) { "OPTIONS_NOT_OPENED try=$($try)" | Write-Host; Start-Sleep -Milliseconds 700; continue }
		$res = ToggleExpected $h.opt $target $expected "Invoke"
		"INVOKE `"$target`" from=$expected result=$res try=$try" | Write-Host
		Start-Sleep -Milliseconds 700
		$closed = CloseOptions $h $script:main
		if ($res -ceq "TOGGLED") { if (-not $closed) { return "MENUS_NOT_CLOSED" }; return (Tick) }
		if ($res -notin @("NO_ITEM", "NO_PATTERN", "READ_ERROR") -or -not $closed) { return "FLIP_$res" }
	}
	return "FLIP_NOT_DONE"
}

try {
	$null = $wbProc.Handle
	"PID $wbPid started $($started.ToString('HH:mm:ss'))"
	$watchdog = StartWatchdog $wbProc (($Seconds + 30) * 1000)
	if ($RestCapabilityProbe) {
		$script:restResponder.Target = $wbProc
		$script:restResponder.TargetStart = $wbProc.StartTime.ToUniversalTime().Ticks
		$script:restResponder.TargetHandle = $wbProc.Handle
		$script:restWorker = Start-RestCapabilityWorker $script:restResponder $wbProc $restHelperPath ([int][math]::Max(1, ($deadline - (Get-Date)).TotalMilliseconds))
		"REST_PARENT status=ASYNC_WORKER_STARTED"
	}
	if ($FileHandleCapabilityProbe) {
		$script:fhcWriter.Target = $wbProc
		$script:fhcWriter.TargetStart = $wbProc.StartTime.ToUniversalTime().Ticks
		$script:fhcWriter.TargetHandle = $wbProc.Handle
	}
	if ($armedPulse) {
		$script:pulseCoordinator.Target = $wbProc
		$script:pulseCoordinator.TargetStart = $wbProc.StartTime.ToUniversalTime().Ticks
		$script:pulseCoordinator.TargetHandle = $wbProc.Handle
	}
	"WATCHDOG pid=$($watchdog.Id) limitMs=$(($Seconds + 30) * 1000)"
	Raw @{ t = 0; kind = "START"; pid = $wbPid; logsDir = $logsDir }
	$phase = "OPEN"
	foreach ($step in @(@("MAIN", "MAIN_WINDOW"), @($mRegistered, "REGISTERED"), @($mOpenOnly, "OPEN_ONLY"), @($mWaitPlugin, "WAIT_PLUGIN"))) {
		$reason = WaitFor $step[0] $step[1]
		if ($reason) { break }
	}
	if (-not $reason) {
		$tSettle = [math]::Min($Settle, [int]($deadline - (Get-Date)).TotalSeconds)
		for ($k = 0; $k -lt $tSettle -and -not $reason; $k++) { Start-Sleep -Seconds 1; $reason = Tick }
	}
	if (-not $reason -and $ReadAuthOnly) { $reason = "READ_AUTH_NO_PROMPT" }
	if (-not $reason -and $typedTransaction) {
		$phase = "TRANSACTION_SERVER_ON"; $script:rrWriterEntered=$true
		$script:rrTransaction = Invoke-RrOwnedServerOn -OriginalProcess $wbProc -MainHandle $script:main -RunId $d0RunId -AttemptId $transactionAttemptId -ProfilePath $d0HostProfile.path -HostLog $log -OutDir $OutDir -LaunchDeadlineUtc $deadline.ToUniversalTime()
		$base=$script:rrTransaction.baseline; $s1=$script:rrTransaction.state1; $script:switchedOn=$true
		"RR_TRANSACTION status=STATE1_CONFIRMED preparedSha=$($script:rrTransaction.preparedSha256) appliedSha=$($script:rrTransaction.appliedSha256)"
	} else {
	if (-not $reason) {
		$phase = "SWITCH_ON"
		if ($armedPulse) {
			$script:pulseCoordinator.InitialMenuDiagnostic = [pscustomobject]@{
				beforeGame = (Get-RehearsalMenuObservation -Main $script:main -TargetId $wbPid -ClientId $script:clientPid -MenuName Game)
				beforePlugins = (Get-RehearsalMenuObservation -Main $script:main -TargetId $wbPid -ClientId $script:clientPid -MenuName Plugins)
				failedGame = $null; failedPlugins = $null
			}
		}
		$base = StrictSnapshot $script:main $wbPid $null $required
		if ($null -eq $base) {
			if ($armedPulse) {
				$script:pulseCoordinator.InitialMenuDiagnostic.failedGame = Get-RehearsalMenuObservation -Main $script:main -TargetId $wbPid -ClientId $script:clientPid -MenuName Game
				$script:pulseCoordinator.InitialMenuDiagnostic.failedPlugins = Get-RehearsalMenuObservation -Main $script:main -TargetId $wbPid -ClientId $script:clientPid -MenuName Plugins
			}
			$reason = "BASELINE_READ_FAILED"
		}
		else {
			"BASELINE $(MapText $base)"
			foreach ($n in $required) { if ($base[$n] -cne "Off") { $reason = "BASELINE_NOT_OFF" } }
		}
	}
	if (-not $reason) {
		$reason = FlipTarget "Off"
		$s1 = if (-not $reason) { StrictSnapshot $script:main $wbPid @($base.Keys) $required } else { $null }
		if (-not $reason -and $null -eq $s1) { $reason = "STATE1_READ_FAILED" }
		if (-not $reason) {
			"STATE1 $(MapText $s1)"
			$why = OnlyTargetChanged $base $s1 $target
			if ($why) {
				"STATE1_UNEXPECTED $why"
				if ($s1[$target] -ceq "On") { $script:switchedOn = $true }
				$reason = "STATE1_UNEXPECTED"
			} else { $script:switchedOn = $true }
		}
	}
	}
	if (-not $reason) { $reason = Tick }
	if (-not $reason) {
		$phase = "PLAY"
		# every check before Invoke (one exact plain command, menu left open only when READY); a refusal invoked nothing
		$prep = PrepareMenuCommand $script:main $pluginMenu $pluginItem
		"PLUGIN_ITEM t=$([int]((Get-Date) - $started).TotalSeconds) result=$($prep.result) opens=$($prep.opens)"
		if ($prep.result -ne "READY") {
			foreach ($n in @($prep.names)) { "  PLUGINS_ITEM `"$n`"" }
			$reason = "PLUGIN_ITEM_$($prep.result)"
		} else {
			# the attempt counts from the Invoke on, as from the flag before (codex #4746): the launcher may switch to Play at
			# once, so no pre-Play put-back and no second Invoke, whatever the worker reports
			$script:played = $true
			$script:invokeJob = StartAsyncInvoke $prep.ip
			"PLUGIN_INVOKE_STARTED t=$([int]((Get-Date) - $started).TotalSeconds)"
			Raw @{ kind = "PLUGIN_INVOKE_STARTED" }
		}
	}
	# a stop after switching On but before Play: put the target back from On in the same process, then re-read
	if (-not $typedTransaction -and $reason -and $script:switchedOn -and -not $script:played) {
		$put = FlipTarget "On"
		$back = StrictSnapshot $script:main $wbPid @($base.Keys) $required
		$result = if (-not $put -and $null -ne $back -and (ToggleDiff $base $back).Count -eq 0) { "PUT_BACK_OK" } else { "PUT_BACK_FAILED" }
		"PUT_BACK flip=`"$put`" state=$(if ($back) { MapText $back } else { 'UNREAD' }) result=$result"
	}
	if (-not $reason) {
		$phase = "OBSERVE"
		$reason = WaitFor $mEditStable "EDIT_STABLE"
	}
	if (-not $reason -and -not $typedTransaction) {
		$phase = "RESTORE"
		# first menu contact since the Invoke (codex #4850): only once the worker is done, since a pending Invoke may block
		# the next UIA call on that menu (codex #4860); then Plugins should have been closed by Qt itself
		$j = $script:invokeJob
		$null = PollAsyncInvoke $j
		"PLUGIN_INVOKE_AT_EDIT done=$($j.done) result=`"$($j.result)`""
		if (-not $j.done) { $reason = "PLUGIN_INVOKE_PENDING" }
		else {
			$pm = FindTopMenu $script:main $pluginMenu
			$pms = try { [string]$pm.GetCurrentPattern($ECP::Pattern).Current.ExpandCollapseState } catch { "UNREAD" }
			"PLUGINS_MENU state=$pms"
		}
		if (-not $reason -and -not (ClosedOk $pms)) {
			try { $pm.GetCurrentPattern($ECP::Pattern).Collapse() } catch {}
			Start-Sleep -Milliseconds 400
			$pms = try { [string](FindTopMenu $script:main $pluginMenu).GetCurrentPattern($ECP::Pattern).Current.ExpandCollapseState } catch { "UNREAD" }
			"PLUGINS_MENU collapse state=$pms"
			if (-not (ClosedOk $pms)) { $reason = "PLUGINS_MENU_NOT_CLOSED" }
		}
	}
	if (-not $reason -and -not $typedTransaction) {
		$s2 = StrictSnapshot $script:main $wbPid @($base.Keys) $required
		if ($null -eq $s2) { $reason = "STATE2_READ_FAILED" }
		else {
			"STATE2 $(MapText $s2)"
			if ((ToggleDiff $base $s2).Count -eq 0) { $result = "ALREADY_BASELINE" }
			elseif (OnlyTargetChanged $base $s2 $target) { $reason = "STATE2_UNEXPECTED" }
			else {
				$reason = FlipTarget "On"
				if (-not $reason) {
					$fin = StrictSnapshot $script:main $wbPid @($base.Keys) $required
					if ($null -eq $fin) { $reason = "FINAL_READ_FAILED" }
					else {
						"FINAL $(MapText $fin) diffFromBaseline=$((ToggleDiff $base $fin) -join ',')"
						$result = if ((ToggleDiff $base $fin).Count -eq 0) { "RESTORE_OK" } else { "RESTORE_INCOMPLETE" }
					}
				}
			}
		}
		if (-not $reason) { $reason = "DONE" }
	}
	if ($NativePairPlacement -and -not $reason) { $reason = "PAIR_PLACEMENT_OBSERVATION_ENDED" }
	if ($NativeRadioPower -and -not $reason) { $reason = "RADIO_POWER_OBSERVATION_ENDED" }
} catch {
    $reason = "RUNNER_ERROR"
    if ($typedTransaction) {
        $e=$_.Exception; while ($e) { $script:rrWriterErrors += [pscustomobject]@{type=$e.GetType().FullName;hResult=$e.HResult;code=$(if($e.Message -cmatch '\A[A-Z_]+\z'){$e.Message}else{''})}; $e=$e.InnerException }
        if ($NativeRadioPower) { "RUNNER_ERROR status=RADIO_POWER_TRANSACTION_FAILED" } elseif ($NativePairPlacement) { "RUNNER_ERROR status=PAIR_PLACEMENT_TRANSACTION_FAILED" } elseif ($RestGuardedCapture) { "RUNNER_ERROR status=REST_CAPTURE_TRANSACTION_OR_CONTROL_FAILED" } else { "RUNNER_ERROR status=REST_REHEARSAL_TRANSACTION_OR_CONTROL_FAILED" }
    } else { "RUNNER_ERROR $($_.Exception.GetType().Name): $($_.Exception.Message)" }
}
finally {
	if ($RestCapabilityProbe) {
		try {
			if ($script:restWorker) {
				$script:restWorkerReceipt = Complete-RestCapabilityWorker $script:restWorker
			} else {
				Close-RestCapabilityResponder $script:restResponder
				$script:restWorkerReceipt = Get-RestCapabilityReceipt $script:restResponder
			}
		} catch {
			$restCleanupErrors = @(); $restCleanupException = $_.Exception
			while ($restCleanupException) {
				$restCleanupErrors += [pscustomobject]@{type=$restCleanupException.GetType().FullName;hResult=$restCleanupException.HResult}
				$restCleanupException = $restCleanupException.InnerException
			}
			$script:restWorkerReceipt = [pscustomobject]@{status="WORKER_CLEANUP_INVALID";failure="SECONDARY_WORKER_CLEANUP_ERROR";ownedErrors=$restCleanupErrors;nativeCallback="UNQUALIFIED";capture="NOT_REQUESTED"}
		}
	}
	$elapsed = [int]((Get-Date) - $started).TotalSeconds
	if (($script:switchedOn -or ($typedTransaction -and $script:rrWriterEntered)) -and $result -notin @("RESTORE_OK", "ALREADY_BASELINE", "PUT_BACK_OK")) { $result = "SETTING_MAY_REMAIN_ON" }
	"STOP reason=$reason phase=$phase result=$result t=$elapsed yesPresses=$($script:yesPressed)$(if ($Client) { " client=$($script:clientState)" })"
	# the own client first (codex #4906), through the handle Start-Process returned
	if ($script:clientProc) {
		if ($CaptureShort400 -or $CaptureBlindTrial) { $script:shortBStopFacts.beginTicks = [Diagnostics.Stopwatch]::GetTimestamp() }
		$cs = StopOwned $script:clientProc 5000
		if ($CaptureShort400 -or $CaptureBlindTrial) { $script:shortBStopFacts.endTicks = [Diagnostics.Stopwatch]::GetTimestamp(); $script:shortBStopFacts.requested=$cs.requested; $script:shortBStopFacts.exited=$cs.exited }
		"TEARDOWN pid=$($script:clientPid) action=$(if ($cs.requested) { 'KILL_CLIENT' } else { 'CLIENT_EXITED' }) exited=$($cs.exited) error=`"$($cs.error)`""
	}
	$entries = @()
	foreach ($id in @($tracked.Keys)) {
		if ($id -eq $wbPid -or ($script:clientProc -and $id -eq $script:clientPid)) { continue }
		$h = $tracked[$id].held
		$entries += @{ pid = $id; descendant = $tracked[$id].descendant; recordedCreated = $tracked[$id].created; launch = $started;
			held = [bool]$h; heldExited = $(if ($h) { $h.HasExited } else { $false }); heldStart = $(if ($h) { try { $h.StartTime } catch { $null } } else { $null }) }
	}
	$wbProc.Refresh()
	foreach ($step in (TeardownPlan $wbPid $wbProc.HasExited $entries)) {
		"TEARDOWN pid=$($step.pid) action=$($step.action)"
		try {
			if ($step.action -eq "KILL_CHILD") { $tracked[[uint32]$step.pid].held.Kill() }
			if ($step.action -eq "KILL_ROOT") { $wbProc.Kill() }
		} catch { "  KILL_ERROR pid=$($step.pid) $($_.Exception.GetType().Name)" }
	}
	Start-Sleep -Seconds 3
	$wbProc.Refresh()
	"PROC_END pid=$wbPid name=root alive=$(-not $wbProc.HasExited)"
	if ($typedTransaction -and $script:rrTransaction -and $wbProc.HasExited) {
		try { $script:rrClosed = Write-RrClosed -OriginalProcess $wbProc -Transaction $script:rrTransaction; "RR_TRANSACTION status=CLOSED_WRITTEN digest=$($script:rrClosed.closedSha256)" }
		catch {
			$e=$_.Exception; while ($e) { $script:rrCloseErrors += [pscustomobject]@{type=$e.GetType().FullName;hResult=$e.HResult}; $e=$e.InnerException }
			"RR_TRANSACTION status=CLOSED_WRITE_FAILED"
		}
	}
	if ($armedPulse) {
		try { Close-RehearsalCoordinator $script:pulseCoordinator } catch { $script:pulseCoordinator.Valid=$false; $script:pulseCoordinator.Status="INVALID"; $script:pulseCoordinator.Cleanup="CLOSE_ERROR" }
		if ($CaptureShort400 -or $CaptureBlindTrial) {
			try {
				$previousB=$script:pulseCoordinator.BObservation
				$bText=if ($script:clientLog -and (Test-Path $script:clientLog)) { Get-Content $script:clientLog -Raw } else { "" }
				$expectedBStart=if ($script:pulseCoordinator.Snapshot) { $script:pulseCoordinator.Snapshot.BStart } else { "" }
				$finalB=Get-BOwnObservation $bText $d0RunId $expectedBStart
				$finalB.finalReadTicks=[Diagnostics.Stopwatch]::GetTimestamp()
				if ($previousB) { $finalB.afterGuardReadTicks=$previousB.afterGuardReadTicks; $finalB.commonGuardDone=$previousB.commonGuardDone; $finalB.guardExitObservedTicks=$previousB.guardExitObservedTicks }
				$finalB.stopBeginTicks=$script:shortBStopFacts.beginTicks; $finalB.stopEndTicks=$script:shortBStopFacts.endTicks; $finalB.stopRequested=$script:shortBStopFacts.requested; $finalB.stopExited=$script:shortBStopFacts.exited
				$script:pulseCoordinator.BObservation=$finalB
			} catch {
				if (-not $script:pulseCoordinator.BObservation) { $script:pulseCoordinator.BObservation=Get-BOwnObservation "" $d0RunId "" }
				$e=$_.Exception; while ($e) { $script:pulseCoordinator.BObservation.readErrors += [pscustomobject]@{type=$e.GetType().FullName;hResult=$e.HResult}; $e=$e.InnerException }
			}
		}
		$pulseReceipt = Get-RehearsalReceipt $script:pulseCoordinator
		if ($CaptureBlindTrial) {
			try {
				$privatePulseHash=Write-BlindParentRecord (Join-Path $script:pulseCoordinator.Blind.Private 'a-result.json') ([ordered]@{schema='ricon.blind.private-a-result.v1';nonce=$script:pulseCoordinator.Blind.Nonce;payload=$pulseReceipt})
				$script:pulseCoordinator.Blind.PrivateAHash=$privatePulseHash
				"BLIND_A_RECEIPT_SEALED hash=$privatePulseHash"
				Raw @{kind='BLIND_A_RECEIPT_SEALED';hash=$privatePulseHash}
			} catch { "BLIND_A_RECEIPT_SEAL_FAILED type=$($_.Exception.GetType().Name)" }
		} else {
			$(if ($RestGuardedCapture) { "PULSE_CAPTURE_RESULT " } else { "PULSE_REHEARSAL_RESULT " }) + ($pulseReceipt | ConvertTo-Json -Compress -Depth 5)
			Raw @{ kind=$(if ($RestGuardedCapture) { "PULSE_CAPTURE_RESULT" } else { "PULSE_REHEARSAL_RESULT" }); receipt=$pulseReceipt }
		}
	}

	if ($script:clientProc) {
		$script:clientProc.Refresh()
		$clientGone = $script:clientProc.HasExited
		"PROC_END pid=$($script:clientPid) name=client alive=$(-not $clientGone) exitCode=$(if ($clientGone) { $script:clientProc.ExitCode })"
		# the client's watchdog stays while the client is not confirmed gone (codex #4917)
		if ($script:clientDog) {
			if ($clientGone) { $script:clientDog.Refresh(); if (-not $script:clientDog.HasExited) { try { $script:clientDog.Kill() } catch {} }; "CLIENT_WATCHDOG_END exited=$($script:clientDog.HasExited)" }
			else { "CLIENT_WATCHDOG_LEFT pid=$($script:clientDog.Id) (client still alive)" }
		}
		$leftKids = @($tracked.Keys | Where-Object { $tracked[$_].client -and $tracked[$_].held -and -not $tracked[$_].held.HasExited })
		$noHandle = @($tracked.Keys | Where-Object { $tracked[$_].client -and -not $tracked[$_].held })
		"CLEANUP client=$(if ($clientGone -and $leftKids.Count -eq 0 -and $noHandle.Count -eq 0) { 'OK' } else { 'FAILED' }) clientAlive=$(-not $clientGone) clientChildrenAlive=$($leftKids.Count) clientChildrenUnverified=$($noHandle.Count)"
	}
	foreach ($id in @($tracked.Keys)) {
		if ($id -eq $wbPid -or ($script:clientProc -and $id -eq $script:clientPid)) { continue }
		$h = $tracked[$id].held
		$alive = if ($h) { [string](-not $h.HasExited) } else { "UNVERIFIED_NO_HANDLE" }
		"PROC_END pid=$id name=$($tracked[$id].name) descendant=$($tracked[$id].descendant) held=$([bool]$h) alive=$alive"
	}
	if ($script:invokeJob) { "PLUGIN_INVOKE_CLEANUP $(CleanupAsyncInvoke $script:invokeJob 3000)" }
	if ($watchdog) { $watchdog.Refresh(); if (-not $watchdog.HasExited) { try { $watchdog.Kill() } catch {} }; "WATCHDOG_END exited=$($watchdog.HasExited)" }
	"others=" + @(Get-Process | Where-Object { $_.ProcessName -match '(?i)reforger|workbench|enfusion' }).Count
	Raw @{ t = $elapsed; kind = "STOP"; reason = $reason; result = $result }
	if ($typedTransaction) {
		$rrSummary=[pscustomobject]@{entered=$script:rrWriterEntered;state1Confirmed=[bool]$script:rrTransaction;closedWritten=[bool]$script:rrClosed;writerErrors=$script:rrWriterErrors;closedErrors=$script:rrCloseErrors}
		"RR_TRANSACTION_RESULT " + ($rrSummary | ConvertTo-Json -Compress -Depth 5)
		Raw @{kind="RR_TRANSACTION_RESULT";receipt=$rrSummary}
	}
	if ($RestCapabilityProbe) {
		$restReceipt = $script:restWorkerReceipt
		if ($CaptureBlindTrial) {
			try {
				$privateRestHash=Write-BlindParentRecord (Join-Path $script:pulseCoordinator.Blind.Private 'rest-result.json') ([ordered]@{schema='ricon.blind.private-rest-result.v1';nonce=$script:pulseCoordinator.Blind.Nonce;payload=$restReceipt})
				$script:pulseCoordinator.Blind.PrivateRestHash=$privateRestHash
				"BLIND_REST_RECEIPT_SEALED hash=$privateRestHash"
				Raw @{kind='BLIND_REST_RECEIPT_SEALED';hash=$privateRestHash}
			} catch { "BLIND_REST_RECEIPT_SEAL_FAILED type=$($_.Exception.GetType().Name)" }
		} else {
			"REST_PARENT_RECEIPT " + ($restReceipt | ConvertTo-Json -Compress -Depth 4)
			Raw @{ kind="REST_PARENT_RECEIPT"; receipt=$restReceipt }
		}
	}
	if ($CaptureBlindTrial) {
		try { $blindOutcome=Complete-BlindParent $script:pulseCoordinator $script:clientLog; "BLIND_TRIAL_FINAL status=$($blindOutcome.status) hash=$($blindOutcome.revealHash)" }
		catch { "BLIND_TRIAL_FINAL status=UNREVEALED_ARTIFACT_FAILURE type=$($_.Exception.GetType().Name)" }
	}
	if ($FileHandleCapabilityProbe) {
		$fhcReceipt = Get-FhcWriterReceipt $script:fhcWriter
		"FHC_PARENT_RECEIPT " + ($fhcReceipt | ConvertTo-Json -Compress -Depth 4)
		Raw @{ kind="FHC_PARENT_RECEIPT"; receipt=$fhcReceipt }
	}
}

if ($boundedDiscovery) { "log=<scratchpad>/$d0HostLogsName/console.log" } else { "log=$logsDir/console.log" }
if (-not $CaptureBlindTrial -and (Test-Path $log)) {
	$lines = Get-Content $log
	"scriptErrors=" + @($lines | Select-String -Pattern "SCRIPT\s*\(E\)").Count
	# the stream and its dump both come out, so the cap leaves room for 2 x 1024 events; a cut is never silent
	$tagged = @($lines | Select-String -Pattern "RICON_Live[A-Za-z]*\.c' CRC|\[RICON_LIVEB\]|\[RICON_LIVE\]|\[RICON_AUDIO\]|\[RICON_D0\]|\[RICON_V0\]|\[RICON_I0\]|\[RICON_NET\]|\[RICON_WBMODE\]|\[RICON_PROBE\]|SCRIPT\s*\(E\)")
	$tagged | Select-Object -First $taggedCap | ForEach-Object { MaskText $_.Line $script:secrets }
	if ($tagged.Count -gt $taggedCap) { "TAGGED_TRUNCATED shown=$taggedCap total=$($tagged.Count)" }
	# counts only: the host player's lines carry the owner's profile name. Diagnostics, not the B0c criterion (a distinct
	# PlayerId in the host's members is, codex #4960); the client's count includes its own CLI Params line
	if ($Client) { "IDENTITY host sameIdentity=" + @($lines | Select-String -SimpleMatch "same identity").Count + " peerName=" + @($lines | Select-String -SimpleMatch $clientUser).Count }
	"LOGSHA " + (Get-FileHash $log -Algorithm SHA256).Hash + " size " + (Get-Item $log).Length
} else { "NO_LOG" }
if ($Client) {
	# the client's own log (outside the repository); only tagged lines come out, as for the host
	if (-not $script:clientLog) { $f = @(Get-ChildItem $cl.path, $pp.path -Recurse -Filter console.log -File -ErrorAction SilentlyContinue | Select-Object -First 1); if ($f.Count -gt 0) { $script:clientLog = $f[0].FullName } }
	"client=$($script:clientState) log=$(if ($script:clientLog) { 'found' } else { 'none' }) profileFiles=$(@(Get-ChildItem $pp.path -Recurse -File -Force -ErrorAction SilentlyContinue).Count) logFiles=$(@(Get-ChildItem $cl.path -Recurse -File -Force -ErrorAction SilentlyContinue).Count)"
	if ($script:clientLog) {
		$clines = Get-Content $script:clientLog
		"clientScriptErrors=" + @($clines | Select-String -Pattern "SCRIPT\s*\(E\)").Count + " clientDependencyErrors=" + @($clines | Select-String -Pattern '\(E\): Addon \S+ dependency \S+ can.t be added').Count + " clientPeerName=" + @($clines | Select-String -SimpleMatch $clientUser).Count
		$ctagged = @($clines | Select-String -Pattern "RICON_Live[A-Za-z]*\.c' CRC|\[RICON_LIVE\]|\[RICON_AUDIO\]|\[RICON_D0\]|\[RICON_V0\]|\[RICON_I0\]|\[RICON_NET\]|\[RICON_PROBE\]|SCRIPT\s*\(E\)")
		$ctagged | Select-Object -First $taggedCap | ForEach-Object { "C " + (MaskText $_.Line $script:secrets) }
		if ($ctagged.Count -gt $taggedCap) { "C TAGGED_TRUNCATED shown=$taggedCap total=$($ctagged.Count)" }
		"CLIENT_LOGSHA " + (Get-FileHash $script:clientLog -Algorithm SHA256).Hash + " size " + (Get-Item $script:clientLog).Length
	}
}
foreach ($d in @(Get-ChildItem $logsRoot -Directory | Where-Object { $logDirsBefore -notcontains $_.Name -and $_.Name -ne $logsDir })) {
	$files = @(Get-ChildItem $d.FullName -File)
	"OTHER_LOG_DIR $($d.Name) files=$($files.Count) bytes=$(($files | Measure-Object Length -Sum).Sum)"
}
$setAfter = SettingsSnap "after"
$regDiff = @(Compare-Object $setBefore.reg $setAfter.reg)
$profDiff = @(Compare-Object $setBefore.prof $setAfter.prof)
"SETTINGS_AFTER regLines=$($setAfter.reg.Count) regDiffLines=$($regDiff.Count) profileDiffLines=$($profDiff.Count) reforgerDirs=`"$($setAfter.games -join ',')`""
"  REGSHA before=" + (Get-FileHash $setBefore.file -Algorithm SHA256).Hash.Substring(0, 16) + " after=" + (Get-FileHash $setAfter.file -Algorithm SHA256).Hash.Substring(0, 16)
"  PROFILE_CHANGED_FILES=" + (@($profDiff | Where-Object { $_.SideIndicator -eq "=>" } | ForEach-Object { ($_.InputObject -split "  ", 2)[1] }) -join ",")
"  SECURITY_TERMS_IN_DIFF=" + @($regDiff | Where-Object { $_.InputObject -match "(?i)secur|authori|danger|trust|allow|LoadContainer|Arsenal" }).Count
"  PLAYMODE_TERMS_IN_DIFF=" + @($regDiff | Where-Object { $_.InputObject -match "(?i)server|peer|play|localhost|gamemode" }).Count
$srcAfter = SrcSnap
if (($srcBefore -join "`n") -eq ($srcAfter -join "`n")) { "SOURCES_STABLE" } else { "SOURCES_CHANGED" }
"RAW $raw (local only, not for the repository)"
