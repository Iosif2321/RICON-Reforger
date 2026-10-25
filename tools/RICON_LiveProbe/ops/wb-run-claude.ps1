# claude, TOOL_ONLY: one bounded Workbench run for B0b (LIVE-peer-local-plan-codex.md, section B0b, steps 2-3).
# Only after START -> codex ACK; END after the run. Successor of the B0a runner wb_bench.ps1 (1F7081BB..., lost with the
# old session scratchpad); same launch arguments, same exact prompt checks, plus observation of the whole process set.
#
# Observes, read-only, once per second: the Workbench PID and every descendant (CIM ParentProcessId chain), any other new
# ArmaReforger* process, their UDP endpoints and TCP listeners, and their visible windows. The repo-facing output (stdout)
# carries only address classes, ports, PIDs, switch names and hashes; raw addresses and command lines go to the raw JSONL
# in -OutDir, which must be outside the repository.
#
# Stops on: -StopPattern seen -StopCount times (+2 s), launcher failure or SCRIPT (E), an unknown window (after the arm
# marker), a prompt in a non-Workbench PID, more than -MaxChildren new processes, the Workbench PID exiting, or -Seconds.
# Teardown stops only processes this run created (Workbench PID and its descendants that started after launch).
# Never writes the registry or profile; never sends input other than the guarded UIA Invoke of "Yes".
param([Parameter(Mandatory = $true)][string]$Tag,
      [Parameter(Mandatory = $true)][string]$OutDir,
      [int]$Seconds = 120,
      [string]$World = "{3048828FE14AE687}worlds/Tests/TestFramework/Autotest_GameMode_Plain.ent",
      [string]$BenchMode = "",
      [string]$Extra = "",
      [switch]$PressYesOnce,
      [switch]$RestartRun,
      [string]$StopPattern = "",
      [int]$StopCount = 1,
      [string]$ArmPattern = '\[RICON_LIVEB\] state=(PLAY_REQUESTED|SERVER_PLAY_REQUESTED)',
      [int]$MaxChildren = 3,
      [switch]$DryRun)

# -Extra: comma-separated launcher switches without the leading dash (a leading "-" would be bound as a parameter name),
# e.g. -Extra "riconEndpointProbe=1,riconEndpointRestart=1".
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
$secondMarker = '\[RICON_LIVEB\] restart=SECOND_PLAY_REQUESTED'
# launcher refusals, the B0b mode-observer check (codex #4626) and script errors end the run at once
$failPattern = '\[RICON_LIVEB\] (state=(NO_WORLD_ARGUMENT|NO_WORLD_EDITOR|NO_API|PLACEMENT_REFUSED|SETUP_REFUSED)|restart=REFUSED)|\[RICON_WBMODE\] registered=0|SCRIPT\s*\(E\)'

# --- argument checks ---
if ($Tag -notmatch '^[a-z0-9][a-z0-9-]{0,39}$') { "BAD_TAG"; exit 2 }
if (-not (Test-Path $OutDir -PathType Container)) { "OUTDIR_MISSING"; exit 2 }
$OutDir = (Resolve-Path $OutDir).Path
if ($OutDir.StartsWith($repo, [StringComparison]::OrdinalIgnoreCase)) { "OUTDIR_INSIDE_REPO"; exit 2 }
$extraArgs = @()
foreach ($e in @($Extra -split ',' | Where-Object { $_ -ne "" })) {
	if ($e -notmatch '^ricon[A-Za-z]+=[A-Za-z0-9_.:|{}/-]*$') { "BAD_EXTRA $e"; exit 2 }
	$extraArgs += "-$e"
}
if ($BenchMode -and $BenchMode -notmatch '^[a-z]+$') { "BAD_BENCHMODE"; exit 2 }
if ($RestartRun -and -not $PressYesOnce) { "RESTART_RUN_NEEDS_PRESS_YES_ONCE"; exit 2 }
if ($StopCount -lt 1 -or $MaxChildren -lt 0 -or $Seconds -lt 10 -or $Seconds -gt 900) { "BAD_LIMITS"; exit 2 }

$logsDir = "ricon-$Tag-" + (Get-Date -Format "yyyyMMdd-HHmmss")
$log = "$logsRoot\$logsDir\console.log"
$raw = Join-Path $OutDir "$logsDir-raw.jsonl"
$argList = @("-gproj", "`"$gp`"", "-addonsDir", "`"$addons`"", "-noBackend", "-wbModule=WorldEditor",
	"-plugin=RICON_LiveBenchLauncherPlugin", "-riconBenchWorld=`"$World`"", "-logLevel", "verbose",
	"-log-scr-checksum", "-logsDir", $logsDir, "-forceUpdate", "-noFocus", "-maxFPS", "30")
if ($BenchMode) { $argList += "-riconBenchMode=$BenchMode" }
$argList += $extraArgs
"ARGS " + ($argList -join " ")
"RUNNER " + (Get-FileHash $PSCommandPath -Algorithm SHA256).Hash.Substring(0, 16) + " LIB " + (Get-FileHash (Join-Path $PSScriptRoot "wb-run-lib-claude.ps1") -Algorithm SHA256).Hash.Substring(0, 16)
"LIMITS seconds=$Seconds stopPattern=`"$StopPattern`" stopCount=$StopCount maxChildren=$MaxChildren pressYesOnce=$PressYesOnce restartRun=$RestartRun"

$busy = @(Get-Process | Where-Object { $_.ProcessName -match '(?i)reforger|workbench|enfusion' })
if ($busy.Count -gt 0) { "BUSY count=$($busy.Count)"; exit 3 }
if ($DryRun) { "DRY_RUN no process started"; exit 0 }

# --- snapshots ---
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

"SOURCES_BEFORE"; $srcBefore = SrcSnap; $srcBefore
$setBefore = SettingsSnap "before"
"SETTINGS_BEFORE regLines=$($setBefore.reg.Count) profileFiles=$($setBefore.prof.Count) reforgerDirs=`"$($setBefore.games -join ',')`""
$baseline = @{}
foreach ($p in @(Get-CimInstance Win32_Process -Property ProcessId)) { $baseline[[uint32]$p.ProcessId] = $true }
$logDirsBefore = @(Get-ChildItem $logsRoot -Directory | ForEach-Object { $_.Name })

# --- launch ---
$started = Get-Date
$wbProc = Start-Process -FilePath $wb -ArgumentList $argList -PassThru -WindowStyle Hidden
$null = $wbProc.Handle
$wbPid = [uint32]$wbProc.Id
"PID $wbPid started $($started.ToString('HH:mm:ss'))"
Raw @{ t = 0; kind = "START"; pid = $wbPid; logsDir = $logsDir }

$tracked = @{}      # pid -> @{ name; ppid; created; flags; descendant }
$netSeen = @{}      # key -> first t
$reason = "TIMEOUT"; $armed = $false; $stopSeenAt = $null; $newCount = 0
# any runner error ends the loop with RUNNER_ERROR and still reaches the teardown below
try { while ($true) {
	$t = [int]((Get-Date) - $started).TotalSeconds
	if ($t -ge $Seconds) { break }
	$wbProc.Refresh()
	if ($wbProc.HasExited) { $reason = "WORKBENCH_EXITED"; break }

	# process set
	$procs = @(Get-CimInstance Win32_Process -Property ProcessId, ParentProcessId, Name, CreationDate, CommandLine)
	$desc = @(Descendants $procs $wbPid)
	$now = @{}
	foreach ($p in $procs) {
		$id = [uint32]$p.ProcessId
		$isDesc = $desc -contains $id
		$isNewGame = (-not $baseline.ContainsKey($id)) -and $p.Name -match '(?i)reforger'
		if (-not ($isDesc -or $isNewGame)) { continue }
		$now[$id] = $true
		if (-not $tracked.ContainsKey($id)) {
			$flags = FlagNames $p.CommandLine
			$tracked[$id] = @{ name = $p.Name; ppid = [uint32]$p.ParentProcessId; created = $p.CreationDate; descendant = $isDesc }
			if ($id -ne $wbPid) { $newCount++ }
			$cmdSha = if ($p.CommandLine) { [BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash([Text.Encoding]::UTF8.GetBytes($p.CommandLine))).Replace("-", "").Substring(0, 16) } else { "NONE" }
			"PROC_NEW t=$t pid=$id ppid=$($p.ParentProcessId) name=$($p.Name) descendant=$isDesc cmdSha=$cmdSha flags=`"$($flags -join ' ')`""
			Raw @{ t = $t; kind = "PROC_NEW"; pid = $id; ppid = [uint32]$p.ParentProcessId; name = $p.Name; descendant = $isDesc; cmd = $p.CommandLine }
		}
	}
	foreach ($id in @($tracked.Keys)) {
		if (-not $now.ContainsKey($id) -and -not $tracked[$id].ContainsKey("gone")) { $tracked[$id]["gone"] = $t; "PROC_GONE t=$t pid=$id name=$($tracked[$id].name)"; Raw @{ t = $t; kind = "PROC_GONE"; pid = $id } }
	}
	if ($newCount -gt $MaxChildren) { $reason = "TOO_MANY_PROCESSES"; break }

	# endpoints of the tracked live PIDs
	$live = @($now.Keys)
	$eps = @()
	$eps += @(Get-NetUDPEndpoint -ErrorAction SilentlyContinue | Where-Object { $live -contains [uint32]$_.OwningProcess } | ForEach-Object { @{ proto = "UDP"; addr = $_.LocalAddress; port = $_.LocalPort; pid = [uint32]$_.OwningProcess } })
	$eps += @(Get-NetTCPConnection -State Listen -ErrorAction SilentlyContinue | Where-Object { $live -contains [uint32]$_.OwningProcess } | ForEach-Object { @{ proto = "TCP_LISTEN"; addr = $_.LocalAddress; port = $_.LocalPort; pid = [uint32]$_.OwningProcess } })
	foreach ($e in $eps) {
		$key = "$($e.pid)|$($e.proto)|$($e.addr)|$($e.port)"
		if ($netSeen.ContainsKey($key)) { continue }
		$netSeen[$key] = $t
		"NET_NEW t=$t pid=$($e.pid) proto=$($e.proto) class=$(AddrClass $e.addr) port=$($e.port)"
		Raw @{ t = $t; kind = "NET_NEW"; pid = $e.pid; proto = $e.proto; addr = $e.addr; port = $e.port }
	}

	# log markers
	$text = if (Test-Path $log) { Get-Content $log -Raw } else { "" }
	if (-not $armed -and $text -match $ArmPattern) { $armed = $true; "ARMED t=$t" }
	if ($text -match $failPattern) { $reason = "LOG_FAILURE"; break }
	$secondPlay = $RestartRun -and ($text -match $secondMarker)
	if ($StopPattern -and -not $stopSeenAt -and ([regex]::Matches($text, $StopPattern)).Count -ge $StopCount) { $stopSeenAt = Get-Date; "STOP_PATTERN_SEEN t=$t" }
	if ($stopSeenAt -and ((Get-Date) - $stopSeenAt).TotalSeconds -ge 2) { $reason = "STOP_PATTERN"; break }

	# windows: guarded prompt on the Workbench PID; anything unknown stops the run
	if ($PressYesOnce) {
		$y = YesGuard $wbPid $t ([bool]$RestartRun) $secondPlay
		if ($y -notin @("NONE", "CLOSING", "NOT_READY", "PRESSED")) { $reason = "DIALOG_$y"; break }
	}
	$unknown = ""
	foreach ($id in $live) {
		if ($id -ne $wbPid -and [RiconWin]::FindByTitle($id, $script:AuthTitle) -ne [IntPtr]::Zero) { $unknown = "PROMPT_IN_CHILD pid=$id"; break }
		if ($armed -or $id -ne $wbPid) {
			$w = UnknownWindow $id ($id -eq $wbPid)
			if ($w) { $unknown = "pid=$id title=`"$w`""; break }
		}
	}
	if ($unknown) {
		"UNKNOWN_WINDOW t=$t $unknown (read only, not pressed)"
		$reason = if ($unknown.StartsWith("PROMPT_IN_CHILD")) { "DIALOG_PROMPT_IN_CHILD" } else { "DIALOG_UNKNOWN_WINDOW" }
		break
	}
	Start-Sleep -Milliseconds 1000
} } catch { $reason = "RUNNER_ERROR"; "RUNNER_ERROR $($_.Exception.GetType().Name): $($_.Exception.Message)" }

# --- teardown: only processes this run created ---
$elapsed = [int]((Get-Date) - $started).TotalSeconds
"STOP reason=$reason t=$elapsed yesPresses=$($script:yesPressed)"
$own = @($tracked.Keys | Where-Object { $_ -eq $wbPid -or ($tracked[$_].descendant -and $tracked[$_].created -ge $started.AddSeconds(-1)) })
foreach ($id in $own) { if (Get-Process -Id $id -ErrorAction SilentlyContinue) { Stop-Process -Id $id -Force -Confirm:$false -ErrorAction SilentlyContinue } }
Start-Sleep -Seconds 3
foreach ($id in @($tracked.Keys)) {
	$alive = [bool](Get-Process -Id $id -ErrorAction SilentlyContinue)
	$mine = $own -contains $id
	"PROC_END pid=$id name=$($tracked[$id].name) descendant=$($tracked[$id].descendant) ownedByRun=$mine alive=$alive"
}
Raw @{ t = $elapsed; kind = "STOP"; reason = $reason }

# --- logs ---
"log=$logsDir/console.log"
if (Test-Path $log) {
	$lines = Get-Content $log
	"scriptErrors=" + @($lines | Select-String -Pattern "SCRIPT\s*\(E\)").Count
	$lines | Select-String -Pattern "RICON_Live[A-Za-z]*\.c' CRC|\[RICON_LIVEB\]|\[RICON_LIVE\]|SCRIPT\s*\(E\)" | Select-Object -First 300 | ForEach-Object { $_.Line }
	"LOGSHA " + (Get-FileHash $log -Algorithm SHA256).Hash + " size " + (Get-Item $log).Length
} else { "NO_LOG" }
foreach ($d in @(Get-ChildItem $logsRoot -Directory | Where-Object { $logDirsBefore -notcontains $_.Name -and $_.Name -ne $logsDir })) {
	$files = @(Get-ChildItem $d.FullName -File)
	"OTHER_LOG_DIR $($d.Name) files=$($files.Count) bytes=$(($files | Measure-Object Length -Sum).Sum)"
}

# --- settings and sources after ---
$setAfter = SettingsSnap "after"
$regDiff = @(Compare-Object $setBefore.reg $setAfter.reg)
$profDiff = @(Compare-Object $setBefore.prof $setAfter.prof)
"SETTINGS_AFTER regLines=$($setAfter.reg.Count) regDiffLines=$($regDiff.Count) profileDiffLines=$($profDiff.Count) reforgerDirs=`"$($setAfter.games -join ',')`""
"  REGSHA before=" + (Get-FileHash $setBefore.file -Algorithm SHA256).Hash.Substring(0, 16) + " after=" + (Get-FileHash $setAfter.file -Algorithm SHA256).Hash.Substring(0, 16)
"  PROFILE_CHANGED_FILES=" + (@($profDiff | Where-Object { $_.SideIndicator -eq "=>" } | ForEach-Object { ($_.InputObject -split "  ", 2)[1] }) -join ",")
"  SECURITY_TERMS_IN_DIFF=" + @($regDiff | Where-Object { $_.InputObject -match "(?i)secur|authori|danger|trust|allow|LoadContainer|Arsenal" }).Count
"  PLAYMODE_TERMS_IN_DIFF=" + @($regDiff | Where-Object { $_.InputObject -match "(?i)server|peer|play|localhost|gamemode" }).Count
"SOURCES_AFTER"; $srcAfter = SrcSnap
if (($srcBefore -join "`n") -eq ($srcAfter -join "`n")) { "SOURCES_STABLE" } else { "SOURCES_CHANGED"; $srcAfter }
"RAW $raw (local only, not for the repository)"
