# claude, TOOL_ONLY: B0b step 2 - one Play of the own World Editor in the "Server localhost" mode, started from the UI.
# Only after START -> codex ACK; END after the run. Plan: agora #4691-#4698 (codex chose Game > Play Game in #4693 and
# the launcher's inspect wait/return in #4694/#4697).
#   1. Workbench with the Plain world in edit mode (-riconBenchMode=inspect) and the endpoint probe gate
#      (-riconEndpointProbe=1); wait for the editor window, "[RICON_WBMODE] registered=1", OPEN_ONLY isGameMode=0 and
#      the launcher's inspect=WAIT_PLAY.
#   2. Strict snapshot B of Game > Play Game Options (both server items Off); InvokePattern of "Server localhost" after
#      the fresh Off check; strict S1 must differ from B by exactly Server localhost=On (else put back and stop).
#   3. Invoke of the plain command Game > Play Game (found once; no Toggle, no ExpandCollapse), in a separate runspace
#      (codex #4717/#4719): the synchronous UIA call blocked run 125954 for ~125 s; observation now goes on meanwhile.
#   4. Observation once per second (as wb-run-claude.ps1): the Workbench PID and its descendants (held handles), other
#      new ArmaReforger* processes, their UDP endpoints/TCP listeners as address class and port, their windows; the
#      vanilla Script Authorization prompt gets at most one guarded "Yes", only on the Workbench PID; a prompt in another
#      process, an unknown window, too many processes, a launcher refusal or SCRIPT (E) stops the run.
#   5. After the launcher's inspect=EDIT_STABLE: strict S2 must be B except Server localhost=On; Invoke it back from On
#      and the final strict snapshot must equal B (RESTORE_OK), else RESTORE_INCOMPLETE is reported.
#   6. Teardown in finally: descendants only through their held handles, the root through its Process object.
# Repo-facing stdout carries address classes, ports, PIDs, switch names and hashes; raw addresses and command lines go to
# the raw JSONL in -OutDir, outside the repository. Never writes the registry or profile.
param([Parameter(Mandatory = $true)][string]$Tag,
      [Parameter(Mandatory = $true)][string]$OutDir,
      [int]$Seconds = 240,
      [int]$Settle = 15,
      [int]$MaxChildren = 3,
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
$mWaitPlay = '\[RICON_LIVEB\] inspect=WAIT_PLAY'
$mEditStable = '\[RICON_LIVEB\] inspect=EDIT_STABLE'
$failPattern = '\[RICON_LIVEB\] (state=(NO_WORLD_ARGUMENT|NO_WORLD_EDITOR|NO_API|PLACEMENT_REFUSED|SETUP_REFUSED|PLAY_REQUESTED|BENCH_CREATED|ENDPOINT_ONLY)|inspect=REFUSED|restart=)|\[RICON_WBMODE\] registered=0|SCRIPT\s*\(E\)'
$earlyPlay = 'kind=ENDPOINT_START|\[RICON_WBMODE\] event=|\[RICON_LIVEB\] inspect=PLAY_ENTERED'

if ($Tag -notmatch '^[a-z0-9][a-z0-9-]{0,39}$') { "BAD_TAG"; exit 2 }
if (-not (Test-Path $OutDir -PathType Container)) { "OUTDIR_MISSING"; exit 2 }
$OutDir = (Resolve-Path $OutDir).Path
if ($OutDir.StartsWith($repo, [StringComparison]::OrdinalIgnoreCase)) { "OUTDIR_INSIDE_REPO"; exit 2 }
if ($Seconds -lt 60 -or $Seconds -gt 600 -or $MaxChildren -lt 0) { "BAD_LIMITS"; exit 2 }
$logsDir = "ricon-$Tag-" + (Get-Date -Format "yyyyMMdd-HHmmss")
$log = "$logsRoot\$logsDir\console.log"
$raw = Join-Path $OutDir "$logsDir-raw.jsonl"
$argList = @("-gproj", "`"$gp`"", "-addonsDir", "`"$addons`"", "-noBackend", "-wbModule=WorldEditor",
	"-logLevel", "verbose", "-logsDir", $logsDir, "-noFocus", "-maxFPS", "30",
	"-plugin=RICON_LiveBenchLauncherPlugin", "-riconBenchWorld=`"$world`"", "-riconBenchMode=inspect",
	"-riconEndpointProbe=1", "-log-scr-checksum")
"ARGS " + ($argList -join " ")
"RUNNER " + (Get-FileHash $PSCommandPath -Algorithm SHA256).Hash.Substring(0, 16) + " LIB " + (Get-FileHash (Join-Path $PSScriptRoot "wb-run-lib-claude.ps1") -Algorithm SHA256).Hash.Substring(0, 16)
"LIMITS seconds=$Seconds watchdogSeconds=$($Seconds + 30) settle=$Settle maxChildren=$MaxChildren"
$busy = @(Get-Process | Where-Object { $_.ProcessName -match '(?i)reforger|workbench|enfusion' })
if ($busy.Count -gt 0) { "BUSY count=$($busy.Count)"; exit 3 }
if ($DryRun) { "DRY_RUN no process started"; exit 0 }

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
$script:mainFound = $false; $script:played = $false; $script:switchedOn = $false; $script:playJob = $null
$phase = "START"; $reason = ""; $result = "NOT_REACHED"; $base = $null; $main = [IntPtr]::Zero
$started = Get-Date
$deadline = $started.AddSeconds($Seconds)
$wbProc = Start-Process -FilePath $wb -ArgumentList $argList -PassThru -WindowStyle Hidden
$wbPid = [uint32]$wbProc.Id

# One observation step; "" or the stop reason. Before Play nothing may start and no prompt may show.
function Tick {
	$t = [int]((Get-Date) - $started).TotalSeconds
	if ($script:playJob -and -not $script:playJob.done -and (PollAsyncInvoke $script:playJob)) {
		"PLAY_INVOKE_DONE t=$t afterMs=$([int]((Get-Date) - $script:playJob.started).TotalMilliseconds) result=$($script:playJob.result)" | Write-Host
		Raw @{ t = $t; kind = "PLAY_INVOKE_DONE"; result = $script:playJob.result }
		try { $p = $script:playTop.GetCurrentPattern($ECP::Pattern); if (-not (ClosedOk $p.Current.ExpandCollapseState)) { $p.Collapse() } } catch {}
	}
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
	if ($script:played) {
		$y = YesGuard $wbPid $t $false $false
		if ($y -notin @("NONE", "CLOSING", "NOT_READY", "PRESSED")) { return "DIALOG_$y" }
	} elseif ([RiconWin]::FindByTitle($wbPid, $script:AuthTitle) -ne [IntPtr]::Zero) { "AUTH_PROMPT before Play (read only)" | Write-Host; return "DIALOG_AUTH_PROMPT_BEFORE_PLAY" }
	foreach ($id in $live) {
		if ($id -eq $wbPid) {
			$u = UnknownWindowStartup $wbPid $script:mainFound $t
			if ($u) { "UNKNOWN_WINDOW t=$t pid=$id title=`"$u`" (read only)" | Write-Host; return "DIALOG_UNKNOWN_WINDOW" }
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
	for ($try = 1; $try -le 2; $try++) {
		$h = OpenOptions $script:main $wbPid
		if ($null -eq $h) { return "OPTIONS_NOT_OPENED" }
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
	foreach ($step in @(@("MAIN", "MAIN_WINDOW"), @($mRegistered, "REGISTERED"), @($mOpenOnly, "OPEN_ONLY"), @($mWaitPlay, "WAIT_PLAY"))) {
		$reason = WaitFor $step[0] $step[1]
		if ($reason) { break }
	}
	if (-not $reason) {
		$tSettle = [math]::Min($Settle, [int]($deadline - (Get-Date)).TotalSeconds)
		for ($k = 0; $k -lt $tSettle -and -not $reason; $k++) { Start-Sleep -Seconds 1; $reason = Tick }
	}
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
		$prep = PrepareMenuCommand $script:main "Game" "Play Game"
		if ($prep.result -cne "READY") { $reason = "PLAY_$($prep.result)"; "PLAY_PREPARE result=$($prep.result) (not attempted)" }
		else {
			# codex #4703/#4717: the attempt counts from the worker start: no pre-Play put-back, no second Play
			$script:played = $true
			$script:playTop = $prep.top
			$script:playJob = StartAsyncInvoke $prep.ip
			"PLAY_INVOKE_STARTED t=$([int]((Get-Date) - $started).TotalSeconds) (worker runspace)"
			Raw @{ kind = "PLAY_INVOKE_STARTED" }
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
		# the menus are touched again only after the Play worker has returned (at most 10 s more after EDIT_STABLE)
		for ($k = 0; $k -lt 10 -and $script:playJob -and -not $script:playJob.done -and -not $reason; $k++) { Start-Sleep -Seconds 1; $reason = Tick }
		if (-not $reason -and $script:playJob -and -not $script:playJob.done) { $reason = "PLAY_INVOKE_STILL_RUNNING" }
	}
	if (-not $reason) {
		$phase = "RESTORE"
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
	"STOP reason=$reason phase=$phase result=$result t=$elapsed yesPresses=$($script:yesPressed)"
	$entries = @()
	foreach ($id in @($tracked.Keys)) {
		if ($id -eq $wbPid) { continue }
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
	foreach ($id in @($tracked.Keys)) {
		if ($id -eq $wbPid) { continue }
		$h = $tracked[$id].held
		$alive = if ($h) { [string](-not $h.HasExited) } else { "UNVERIFIED_NO_HANDLE" }
		"PROC_END pid=$id name=$($tracked[$id].name) descendant=$($tracked[$id].descendant) held=$([bool]$h) alive=$alive"
	}
	# the worker only after the own Workbench is gone (its blocked UIA call then fails or returns); never a blocking Stop
	"PLAY_WORKER " + (CleanupAsyncInvoke $script:playJob 10000)
	if ($watchdog) { $watchdog.Refresh(); if (-not $watchdog.HasExited) { try { $watchdog.Kill() } catch {} }; "WATCHDOG_END exited=$($watchdog.HasExited)" }
	"others=" + @(Get-Process | Where-Object { $_.ProcessName -match '(?i)reforger|workbench|enfusion' }).Count
	Raw @{ t = $elapsed; kind = "STOP"; reason = $reason; result = $result }
}

"log=$logsDir/console.log"
if (Test-Path $log) {
	$lines = Get-Content $log
	"scriptErrors=" + @($lines | Select-String -Pattern "SCRIPT\s*\(E\)").Count
	$lines | Select-String -Pattern "RICON_Live[A-Za-z]*\.c' CRC|\[RICON_LIVEB\]|\[RICON_LIVE\]|\[RICON_NET\]|\[RICON_WBMODE\]|SCRIPT\s*\(E\)" | Select-Object -First 400 | ForEach-Object { $_.Line }
	"LOGSHA " + (Get-FileHash $log -Algorithm SHA256).Hash + " size " + (Get-Item $log).Length
} else { "NO_LOG" }
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
