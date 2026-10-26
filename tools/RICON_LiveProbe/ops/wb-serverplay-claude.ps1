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
      [switch]$ReadAuthOnly,
      [switch]$Client,
      [switch]$DryRun)

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
$world = "{3048828FE14AE687}worlds/Tests/TestFramework/Autotest_GameMode_Plain.ent"
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

if ($Tag -notmatch '^[a-z0-9][a-z0-9-]{0,39}$') { "BAD_TAG"; exit 2 }
if (-not (Test-Path $OutDir -PathType Container)) { "OUTDIR_MISSING"; exit 2 }
$OutDir = (Resolve-Path $OutDir).Path
if ($OutDir.StartsWith($repo, [StringComparison]::OrdinalIgnoreCase)) { "OUTDIR_INSIDE_REPO"; exit 2 }
if ($Seconds -lt 60 -or $Seconds -gt 600 -or $MaxChildren -lt 0) { "BAD_LIMITS"; exit 2 }
if ($Client -and $Seconds -lt 300) { "BAD_LIMITS (-Client needs -Seconds 300 or more)"; exit 2 }
if ($Client -and $ReadAuthOnly) { "BAD_MODES (-ReadAuthOnly never reaches Play)"; exit 2 }
$logsDir = "ricon-$Tag-" + (Get-Date -Format "yyyyMMdd-HHmmss")
$log = "$logsRoot\$logsDir\console.log"
$raw = Join-Path $OutDir "$logsDir-raw.jsonl"
$argList = @("-gproj", "`"$gp`"", "-addonsDir", "`"$addons`"", "-noBackend", "-wbModule=WorldEditor",
	"-logLevel", "verbose", "-logsDir", $logsDir, "-noFocus", "-maxFPS", "30",
	"-plugin=RICON_LiveBenchLauncherPlugin", "-riconBenchWorld=`"$world`"", "-riconBenchMode=inspect",
	"-riconEndpointProbe=1", "-riconBenchPluginStart=1", "-log-scr-checksum")
if ($Client) { $argList += @("-riconEndpointSamples=120", "-forceUpdate") }
"ARGS " + ($argList -join " ")
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
	$clientArgs = @("-gproj", "`"$gp`"", "-addonsDir", "`"$clientAddons`"", "-window", "-screenWidth", "960", "-screenHeight", "540",
		"-maxFPS", "20", "-noFocus", "-forceUpdate", "-noBackend", "-username", $clientUser, "-client", $ha.address, "-profile", "`"$(Join-Path $OutDir $ppName)`"",
		"-logsDir", "`"$(Join-Path $OutDir $clName)`"", "-logLevel", "verbose", "-log-scr-checksum", "-riconEndpointProbe=1")
	"CLIENT_ARGS " + ($clientArgs -join " ").Replace($ha.address, "<host PRIVATE_V4>").Replace($OutDir, "<scratchpad>")
	$script:secrets = @($ha.address, $OutDir)
}
if ($DryRun) { "DRY_RUN no process started"; exit 0 }
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
$wbProc = Start-Process -FilePath $wb -ArgumentList $argList -PassThru -WindowStyle Hidden -WorkingDirectory $repo
$wbPid = [uint32]$wbProc.Id

# One observation step; "" or the stop reason. Before Play nothing may start and no prompt may show.
function Tick {
	$t = [int]((Get-Date) - $started).TotalSeconds
	$wbProc.Refresh()
	if ($wbProc.HasExited) { return "WORKBENCH_EXITED" }
	if ((Get-Date) -gt $deadline) { return "DEADLINE" }
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
		$y = YesGuardSites $wbPid $t (SitesAllowed $script:played $yText $(if ($Client -and $script:clientProc) { $peerJoined } else { "" })) $yesTotal
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
	if (-not $reason) {
		$phase = "SWITCH_ON"
		$base = StrictSnapshot $script:main $wbPid $null $required
		if ($null -eq $base) { $reason = "BASELINE_READ_FAILED" }
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
	if ($reason -and $script:switchedOn -and -not $script:played) {
		$put = FlipTarget "On"
		$back = StrictSnapshot $script:main $wbPid @($base.Keys) $required
		$result = if (-not $put -and $null -ne $back -and (ToggleDiff $base $back).Count -eq 0) { "PUT_BACK_OK" } else { "PUT_BACK_FAILED" }
		"PUT_BACK flip=`"$put`" state=$(if ($back) { MapText $back } else { 'UNREAD' }) result=$result"
	}
	if (-not $reason) {
		$phase = "OBSERVE"
		$reason = WaitFor $mEditStable "EDIT_STABLE"
	}
	if (-not $reason) {
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
	if (-not $reason) {
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
} catch { $reason = "RUNNER_ERROR"; "RUNNER_ERROR $($_.Exception.GetType().Name): $($_.Exception.Message)" }
finally {
	$elapsed = [int]((Get-Date) - $started).TotalSeconds
	if ($script:switchedOn -and $result -notin @("RESTORE_OK", "ALREADY_BASELINE", "PUT_BACK_OK")) { $result = "SETTING_MAY_REMAIN_ON" }
	"STOP reason=$reason phase=$phase result=$result t=$elapsed yesPresses=$($script:yesPressed)$(if ($Client) { " client=$($script:clientState)" })"
	# the own client first (codex #4906), through the handle Start-Process returned
	if ($script:clientProc) {
		$cs = StopOwned $script:clientProc 5000
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
}

"log=$logsDir/console.log"
if (Test-Path $log) {
	$lines = Get-Content $log
	"scriptErrors=" + @($lines | Select-String -Pattern "SCRIPT\s*\(E\)").Count
	$lines | Select-String -Pattern "RICON_Live[A-Za-z]*\.c' CRC|\[RICON_LIVEB\]|\[RICON_LIVE\]|\[RICON_NET\]|\[RICON_WBMODE\]|\[RICON_PROBE\]|SCRIPT\s*\(E\)" | Select-Object -First 400 | ForEach-Object { MaskText $_.Line $script:secrets }
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
		$clines | Select-String -Pattern "RICON_Live[A-Za-z]*\.c' CRC|\[RICON_LIVE\]|\[RICON_NET\]|\[RICON_PROBE\]|SCRIPT\s*\(E\)" | Select-Object -First 300 | ForEach-Object { "C " + (MaskText $_.Line $script:secrets) }
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
