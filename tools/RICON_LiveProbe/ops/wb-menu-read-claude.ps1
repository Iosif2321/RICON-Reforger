# claude, TOOL_ONLY: read the World Editor menu bar and toolbar names of one own Workbench process (B0b step 1: where
# "Server localhost" lives, claude #4623/#4627, codex #4625/#4629). Only after START -> codex ACK; END after the run.
# No world, no launcher plugin, no Play. UI Automation only: names, control types and supported patterns. All top-level
# menu names are printed; with -Expand only a top-level menu named exactly Game or Play and toolbar items whose name
# contains "Play" are opened (codex #4639: File/Edit/... would list resource history), ONLY through
# ExpandCollapsePattern.Expand; their direct item names are read and they are closed with Collapse; submenus stay shut.
# Never Invoke, Toggle, SelectionItem, Value, keyboard or mouse. The Script Authorization prompt, any unknown window, a
# Collapse that fails or the deadline stops the process (no press). A separate watchdog process kills the own Workbench
# after -Seconds + 30 s even if a UI Automation call hangs. Settings are snapshotted before and after.
# -OpenWorld (codex #4650): the editor shows no "World Editor - ..." window without a world, so the launcher plugin opens
# the fixed Plain world in edit mode (-riconBenchMode=inspect: OPEN_ONLY, no entity, no Play). The UI is read only after
# the launcher's "state=OPEN_ONLY" line; PLAY_REQUESTED, ENDPOINT_START or a [RICON_WBMODE] event in the log stops the run.
param([Parameter(Mandatory = $true)][string]$Tag,
      [Parameter(Mandatory = $true)][string]$OutDir,
      [int]$Seconds = 120,
      [int]$Settle = 15,
      [switch]$Expand,
      [switch]$OpenWorld,
      [string]$SubMenu = "",
      [switch]$Roundtrip,
      [string]$RoundtripMethod = "Toggle",
      [switch]$DryRun)

. (Join-Path $PSScriptRoot "wb-run-lib-claude.ps1")
$steam = "C:\Program Files (x86)\Steam\steamapps\common"
$wb = "$steam\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$gp = "$repo\tools\RICON_LiveProbe\addon.gproj"
$addons = "$steam\Arma Reforger\addons,$steam\Arma Reforger Tools\Workbench\addons"
$myGames = "$env:USERPROFILE\Documents\My Games"
$regKey = "HKCU\Software\Bohemia Interactive"
$profileDir = "$myGames\ArmaReforgerWorkbench\profile"

if ($Tag -notmatch '^[a-z0-9][a-z0-9-]{0,39}$') { "BAD_TAG"; exit 2 }
# -SubMenu: the one nested item of Game that may be opened (codex #4661 proposal); nothing else is accepted
if ($SubMenu -and $SubMenu -cne "Play Game Options") { "BAD_SUBMENU"; exit 2 }
if ($SubMenu -and -not $Expand) { "SUBMENU_NEEDS_EXPAND"; exit 2 }
# -Roundtrip (codex #4668): Server localhost Off -> On -> back to the baseline of every Play Game Options toggle, through
# the UI TogglePattern of the own editor window only; needs -OpenWorld; no menu listing in this mode.
if ($Roundtrip -and (-not $OpenWorld -or $Expand -or $SubMenu)) { "ROUNDTRIP_NEEDS_OPENWORLD_ONLY"; exit 2 }
if ($RoundtripMethod -cnotin @("Toggle", "Invoke")) { "BAD_ROUNDTRIP_METHOD"; exit 2 }
if (-not (Test-Path $OutDir -PathType Container)) { "OUTDIR_MISSING"; exit 2 }
$OutDir = (Resolve-Path $OutDir).Path
if ($OutDir.StartsWith($repo, [StringComparison]::OrdinalIgnoreCase)) { "OUTDIR_INSIDE_REPO"; exit 2 }
$logsDir = "ricon-$Tag-" + (Get-Date -Format "yyyyMMdd-HHmmss")
$argList = @("-gproj", "`"$gp`"", "-addonsDir", "`"$addons`"", "-noBackend", "-wbModule=WorldEditor",
	"-logLevel", "verbose", "-logsDir", $logsDir, "-noFocus", "-maxFPS", "30")
$world = "{3048828FE14AE687}worlds/Tests/TestFramework/Autotest_GameMode_Plain.ent"
if ($OpenWorld) { $argList += @("-plugin=RICON_LiveBenchLauncherPlugin", "-riconBenchWorld=`"$world`"", "-riconBenchMode=inspect", "-log-scr-checksum") }
$log = "$myGames\ArmaReforgerWorkbench\logs\$logsDir\console.log"
$openPattern = '\[RICON_LIVEB\] state=OPEN_ONLY'
$playPattern = '\[RICON_LIVEB\] state=PLAY_REQUESTED|kind=ENDPOINT_START|\[RICON_WBMODE\] event=|\[RICON_LIVEB\] state=(BENCH_CREATED|ENDPOINT_ONLY)'
function LogText { if (Test-Path $log) { return (Get-Content $log -Raw) } else { return "" } }
"ARGS " + ($argList -join " ")
"RUNNER " + (Get-FileHash $PSCommandPath -Algorithm SHA256).Hash.Substring(0, 16) + " LIB " + (Get-FileHash (Join-Path $PSScriptRoot "wb-run-lib-claude.ps1") -Algorithm SHA256).Hash.Substring(0, 16)
"LIMITS seconds=$Seconds settle=$Settle expand=$Expand openWorld=$OpenWorld subMenu=`"$SubMenu`" roundtrip=$Roundtrip method=$RoundtripMethod"
$busy = @(Get-Process | Where-Object { $_.ProcessName -match '(?i)reforger|workbench|enfusion' })
if ($busy.Count -gt 0) { "BUSY count=$($busy.Count)"; exit 3 }
if ($DryRun) { "DRY_RUN no process started"; exit 0 }

function SettingsSnap($label) {
	$out = Join-Path $OutDir "$logsDir-settings-$label.reg"
	$null = & reg.exe export $regKey $out /y
	$prof = @(Get-ChildItem $profileDir -Recurse -File -Force | Sort-Object FullName | ForEach-Object { (Get-FileHash $_.FullName -Algorithm SHA256).Hash + "  " + $_.Name })
	return @{ reg = @(Get-Content $out); prof = $prof; file = $out }
}
# UI Automation helpers (Patterns, Short, Children, OfType, OpenedItems, ExpandRead) live in wb-run-lib-claude.ps1
$setBefore = SettingsSnap "before"
"SETTINGS_BEFORE regLines=$($setBefore.reg.Count) profileFiles=$($setBefore.prof.Count)"
$started = Get-Date
$deadline = $started.AddSeconds($Seconds)
$proc = Start-Process -FilePath $wb -ArgumentList $argList -PassThru -WindowStyle Hidden
$null = $proc.Handle
$wbPid = [uint32]$proc.Id
"PID $wbPid started $($started.ToString('HH:mm:ss'))"
$watchdog = StartWatchdog $proc (($Seconds + 30) * 1000)
"WATCHDOG pid=$($watchdog.Id) limitMs=$(($Seconds + 30) * 1000)"
$reason = "TIMEOUT"; $main = [IntPtr]::Zero; $mainTitle = ""; $script:mainFound = $false; $nextList = 5
# "" when the UI may be read now, else the stop reason: prompt, unknown window, exited process or deadline
function Blocked {
	$proc.Refresh()
	if ($proc.HasExited) { return "WORKBENCH_EXITED" }
	if ((Get-Date) -gt $deadline) { return "DEADLINE" }
	if ($OpenWorld -and (LogText) -match $playPattern) { "PLAY_OR_BENCH_MARKER in log (open-only run)" | Write-Host; return "PLAY_SEEN" }
	if ([RiconWin]::FindByTitle($wbPid, $script:AuthTitle) -ne [IntPtr]::Zero) { "AUTH_PROMPT seen (read only, not pressed)" | Write-Host; return "DIALOG_AUTH_PROMPT" }
	$u = UnknownWindowStartup $wbPid $script:mainFound ((Get-Date) - $started).TotalSeconds
	if ($u) { "UNKNOWN_WINDOW title=`"$u`" mainFound=$($script:mainFound) (read only)" | Write-Host; return "DIALOG_UNKNOWN_WINDOW" }
	return ""
}
try {
	# wait for the main editor window, then let the UI settle; a prompt or an unknown window stops the run
	while ($true) {
		$b = Blocked
		if ($b) { $reason = $b; break }
		foreach ($w in @([RiconWin]::List($wbPid))) {
			$parts = $w -split "\|", 3
			if ($parts[0] -eq "V" -and ($parts[2] -ceq "World Editor" -or $parts[2].StartsWith("World Editor - ", [StringComparison]::Ordinal))) { $mainTitle = $parts[2] }
		}
		if ($mainTitle) { $main = [RiconWin]::FindByTitle($wbPid, $mainTitle); $script:mainFound = $true; break }
		# every 10 s while waiting: the own visible windows (class|title, title cut at 80), so a refusal shows what was there
		$tw = [int]((Get-Date) - $started).TotalSeconds
		if ($tw -ge $nextList) {
			$vis = @([RiconWin]::List($wbPid) | Where-Object { $_.StartsWith("V|") })
			"OWN_WINDOWS t=$tw visible=$($vis.Count)"
			$vis | Select-Object -First 8 | ForEach-Object { "  WINDOW $(Short $_)" }
			$nextList += 10
		}
		Start-Sleep -Seconds 1
	}
	if ($main -ne [IntPtr]::Zero) {
		"MAIN_WINDOW t=$([int]((Get-Date) - $started).TotalSeconds) title=`"$(Short $mainTitle)`""
		@([RiconWin]::List($wbPid)) | Where-Object { $_.StartsWith("V|") } | ForEach-Object { "  OWN_WINDOW $(Short $_)" }
		if ($OpenWorld) {
			# the UI is read only once the launcher reported the open edit world
			while (-not ((LogText) -match $openPattern)) {
				$b = Blocked
				if ($b) { break }
				Start-Sleep -Seconds 1
			}
			if (-not $b) { "OPEN_ONLY seen t=$([int]((Get-Date) - $started).TotalSeconds)" }
		}
		$left = ($deadline - (Get-Date)).TotalSeconds
		if (-not $b) { Start-Sleep -Seconds ([math]::Max(0, [math]::Min($Settle, [int]$left))) }
		if (-not $b) { $b = Blocked }
		if ($b) { $reason = $b }
		elseif ($Roundtrip) {
			$before = @(Get-Process | Where-Object { $_.ProcessName -match '(?i)reforger|workbench|enfusion' -and $_.Id -ne $wbPid }).Count
			$rt = RoundTrip $main $wbPid "Server localhost" { Blocked } @("Server localhost + PeerTool") 3 $RoundtripMethod
			$after = @(Get-Process | Where-Object { $_.ProcessName -match '(?i)reforger|workbench|enfusion' -and $_.Id -ne $wbPid }).Count
			"ROUNDTRIP_RESULT $rt toggles=$($script:roundTripToggles) otherProcessesBefore=$before after=$after"
			$reason = "ROUNDTRIP_DONE"
			$b = Blocked
			if ($b) { "BLOCKED_AFTER_ROUNDTRIP $b" }
		}
		else {
			$el = $U::FromHandle($main)
			$bars = OfType $el $CT::MenuBar
			"MENUBARS count=$($bars.Count)"
			$k = 0
			foreach ($bar in $bars) {
				foreach ($item in (Children $bar)) {
					$label = "MENU[$k] `"$(Short $item.Current.Name)`""
					"$label type=$($item.Current.ControlType.ProgrammaticName) patterns=$(Patterns $item)"
					if ($Expand -and $item.Current.Name -match '^(?i)(game|play)$') {
						$b = Blocked
						if ($b) { $reason = $b; break }
						$child = if ($item.Current.Name -ceq "Game") { $SubMenu } else { "" }
						if (-not (ExpandRead $item $label $wbPid $main $child)) { $reason = "COLLAPSE_FAILED"; break }
					}
					$k++
				}
				if ($reason -ne "TIMEOUT") { break }
			}
			if ($reason -eq "TIMEOUT") {
				$bars = OfType $el $CT::ToolBar
				"TOOLBARS count=$($bars.Count)"
				$k = 0
				foreach ($bar in $bars) {
					foreach ($item in (Children $bar)) {
						$name = $item.Current.Name
						if (-not $name) { continue }
						$label = "TOOL[$k] `"$(Short $name)`""
						"$label type=$($item.Current.ControlType.ProgrammaticName) patterns=$(Patterns $item)"
						if ($Expand -and $name -match '(?i)play') {
							$b = Blocked
							if ($b) { $reason = $b; break }
							if (-not (ExpandRead $item $label $wbPid $main)) { $reason = "COLLAPSE_FAILED"; break }
						}
						$k++
					}
					if ($reason -ne "TIMEOUT") { break }
				}
			}
			if ($reason -eq "TIMEOUT") { $reason = "READ_DONE" }
			$b = Blocked
			if ($b) { "BLOCKED_AFTER_READ $b" }
		}
	}
} catch { $reason = "RUNNER_ERROR"; "RUNNER_ERROR $($_.Exception.GetType().Name): $($_.Exception.Message)" }

"STOP reason=$reason t=$([int]((Get-Date) - $started).TotalSeconds)"
# the own process is stopped through the Process object whose handle is held since launch (codex #4631), not by PID
$proc.Refresh()
if (-not $proc.HasExited) { try { $proc.Kill() } catch { "KILL_ERROR $($_.Exception.GetType().Name)" } }
Start-Sleep -Seconds 3
$proc.Refresh()
if ($proc.HasExited) { "OWN_PID_GONE" } else { "OWN_PID_STILL_RUNNING" }
$watchdog.Refresh()
if (-not $watchdog.HasExited) { try { $watchdog.Kill() } catch {} }
"WATCHDOG_END exited=$($watchdog.HasExited)"
"others=" + @(Get-Process | Where-Object { $_.ProcessName -match '(?i)reforger|workbench|enfusion' }).Count
if (Test-Path $log) {
	"scriptErrors=" + @(Select-String -Path $log -Pattern "SCRIPT\s*\(E\)").Count
	if ($OpenWorld) {
		"openOnlyLines=" + @(Select-String -Path $log -Pattern $openPattern).Count + " playOrBenchLines=" + @(Select-String -Path $log -Pattern $playPattern).Count
		Select-String -Path $log -Pattern "\[RICON_LIVEB\]|RICON_Live[A-Za-z]*\.c' CRC" | Select-Object -First 30 | ForEach-Object { $_.Line }
	}
	"LOGSHA " + (Get-FileHash $log -Algorithm SHA256).Hash + " size " + (Get-Item $log).Length
} else { "NO_LOG" }
$setAfter = SettingsSnap "after"
$regDiff = @(Compare-Object $setBefore.reg $setAfter.reg)
$profDiff = @(Compare-Object $setBefore.prof $setAfter.prof)
"SETTINGS_AFTER regLines=$($setAfter.reg.Count) regDiffLines=$($regDiff.Count) profileDiffLines=$($profDiff.Count)"
"  REGSHA before=" + (Get-FileHash $setBefore.file -Algorithm SHA256).Hash.Substring(0, 16) + " after=" + (Get-FileHash $setAfter.file -Algorithm SHA256).Hash.Substring(0, 16)
"  SECURITY_TERMS_IN_DIFF=" + @($regDiff | Where-Object { $_.InputObject -match "(?i)secur|authori|danger|trust|allow|LoadContainer|Arsenal" }).Count
"  PLAYMODE_TERMS_IN_DIFF=" + @($regDiff | Where-Object { $_.InputObject -match "(?i)server|peer|play|localhost|gamemode" }).Count
