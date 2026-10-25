# claude, TOOL_ONLY: read the World Editor menu bar and toolbar names of one own Workbench process (B0b step 1: where
# "Server localhost" lives, claude #4623/#4627, codex #4625/#4629). Only after START -> codex ACK; END after the run.
# No world, no launcher plugin, no Play. UI Automation only: names, control types and supported patterns. All top-level
# menu names are printed; with -Expand only a top-level menu named exactly Game or Play and toolbar items whose name
# contains "Play" are opened (codex #4639: File/Edit/... would list resource history), ONLY through
# ExpandCollapsePattern.Expand; their direct item names are read and they are closed with Collapse; submenus stay shut.
# Never Invoke, Toggle, SelectionItem, Value, keyboard or mouse. The Script Authorization prompt, any unknown window, a
# Collapse that fails or the deadline stops the process (no press). A separate watchdog process kills the own Workbench
# after -Seconds + 30 s even if a UI Automation call hangs. Settings are snapshotted before and after.
param([Parameter(Mandatory = $true)][string]$Tag,
      [Parameter(Mandatory = $true)][string]$OutDir,
      [int]$Seconds = 120,
      [int]$Settle = 15,
      [switch]$Expand,
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
if (-not (Test-Path $OutDir -PathType Container)) { "OUTDIR_MISSING"; exit 2 }
$OutDir = (Resolve-Path $OutDir).Path
if ($OutDir.StartsWith($repo, [StringComparison]::OrdinalIgnoreCase)) { "OUTDIR_INSIDE_REPO"; exit 2 }
$logsDir = "ricon-$Tag-" + (Get-Date -Format "yyyyMMdd-HHmmss")
$argList = @("-gproj", "`"$gp`"", "-addonsDir", "`"$addons`"", "-noBackend", "-wbModule=WorldEditor",
	"-logLevel", "verbose", "-logsDir", $logsDir, "-noFocus", "-maxFPS", "30")
"ARGS " + ($argList -join " ")
"RUNNER " + (Get-FileHash $PSCommandPath -Algorithm SHA256).Hash.Substring(0, 16) + " LIB " + (Get-FileHash (Join-Path $PSScriptRoot "wb-run-lib-claude.ps1") -Algorithm SHA256).Hash.Substring(0, 16)
"LIMITS seconds=$Seconds settle=$Settle expand=$Expand"
$busy = @(Get-Process | Where-Object { $_.ProcessName -match '(?i)reforger|workbench|enfusion' })
if ($busy.Count -gt 0) { "BUSY count=$($busy.Count)"; exit 3 }
if ($DryRun) { "DRY_RUN no process started"; exit 0 }

function SettingsSnap($label) {
	$out = Join-Path $OutDir "$logsDir-settings-$label.reg"
	$null = & reg.exe export $regKey $out /y
	$prof = @(Get-ChildItem $profileDir -Recurse -File -Force | Sort-Object FullName | ForEach-Object { (Get-FileHash $_.FullName -Algorithm SHA256).Hash + "  " + $_.Name })
	return @{ reg = @(Get-Content $out); prof = $prof; file = $out }
}
$U = [System.Windows.Automation.AutomationElement]
$CT = [System.Windows.Automation.ControlType]
$ECP = [System.Windows.Automation.ExpandCollapsePattern]
function Patterns($el) { (@($el.GetSupportedPatterns() | ForEach-Object { $_.ProgrammaticName -replace 'PatternIdentifiers\.Pattern$', '' }) -join "+") }
function Short([string]$n) { $n = $n -replace "`r|`n", " "; if ($n.Length -gt 80) { $n.Substring(0, 80) + "..." } else { $n } }
function Children($el) { @($el.FindAll([System.Windows.Automation.TreeScope]::Children, [System.Windows.Automation.Condition]::TrueCondition)) }
function OfType($el, $type) {
	$c = New-Object System.Windows.Automation.PropertyCondition($U::ControlTypeProperty, $type)
	@($el.FindAll([System.Windows.Automation.TreeScope]::Descendants, $c))
}
# Items of an opened menu: popups are separate top-level windows of the own PID in Qt, so read both the element's
# own descendants and every visible own window other than the main one.
function OpenedItems($el, [uint32]$wbPid, $mainHandle) {
	$names = New-Object System.Collections.Generic.List[string]
	foreach ($i in (OfType $el $CT::MenuItem)) { $names.Add((Short $i.Current.Name)) }
	$root = [System.Windows.Automation.AutomationElement]::RootElement
	$c = New-Object System.Windows.Automation.PropertyCondition($U::ProcessIdProperty, [int]$wbPid)
	foreach ($w in @($root.FindAll([System.Windows.Automation.TreeScope]::Children, $c))) {
		if ($w.Current.NativeWindowHandle -eq $mainHandle.ToInt32()) { continue }
		foreach ($i in (OfType $w $CT::MenuItem)) {
			$state = ""
			try { $state = " state=" + $i.GetCurrentPattern($ECP::Pattern).Current.ExpandCollapseState } catch {}
			$names.Add((Short $i.Current.Name) + $state)
		}
	}
	return , $names.ToArray()
}
function ExpandRead($el, $label, [uint32]$wbPid, $mainHandle) {
	try { $p = $el.GetCurrentPattern($ECP::Pattern) } catch { "  $label NO_EXPAND_COLLAPSE (not opened)"; return $true }
	try { $p.Expand() } catch { "  $label EXPAND_ERROR $($_.Exception.GetType().Name)"; return $true }
	Start-Sleep -Milliseconds 700
	$items = OpenedItems $el $wbPid $mainHandle
	"  $label OPENED items=$($items.Count)"
	$items | Select-Object -First 40 | ForEach-Object { "    ITEM `"$_`"" }
	try { $p.Collapse() } catch { "  $label COLLAPSE_ERROR $($_.Exception.GetType().Name)"; return $false }
	Start-Sleep -Milliseconds 400
	$state = $p.Current.ExpandCollapseState
	"  $label CLOSED state=$state"
	return ($state -ne [System.Windows.Automation.ExpandCollapseState]::Expanded)
}

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
$reason = "TIMEOUT"; $main = [IntPtr]::Zero; $mainTitle = ""; $script:mainFound = $false
# "" when the UI may be read now, else the stop reason: prompt, unknown window, exited process or deadline
function Blocked {
	$proc.Refresh()
	if ($proc.HasExited) { return "WORKBENCH_EXITED" }
	if ((Get-Date) -gt $deadline) { return "DEADLINE" }
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
		Start-Sleep -Seconds 1
	}
	if ($main -ne [IntPtr]::Zero) {
		"MAIN_WINDOW t=$([int]((Get-Date) - $started).TotalSeconds) title=`"$(Short $mainTitle)`""
		@([RiconWin]::List($wbPid)) | Where-Object { $_.StartsWith("V|") } | ForEach-Object { "  OWN_WINDOW $(Short $_)" }
		$left = ($deadline - (Get-Date)).TotalSeconds
		Start-Sleep -Seconds ([math]::Max(0, [math]::Min($Settle, [int]$left)))
		$b = Blocked
		if ($b) { $reason = $b }
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
						if (-not (ExpandRead $item $label $wbPid $main)) { $reason = "COLLAPSE_FAILED"; break }
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
$log = "$myGames\ArmaReforgerWorkbench\logs\$logsDir\console.log"
if (Test-Path $log) {
	"scriptErrors=" + @(Select-String -Path $log -Pattern "SCRIPT\s*\(E\)").Count
	"LOGSHA " + (Get-FileHash $log -Algorithm SHA256).Hash + " size " + (Get-Item $log).Length
} else { "NO_LOG" }
$setAfter = SettingsSnap "after"
$regDiff = @(Compare-Object $setBefore.reg $setAfter.reg)
$profDiff = @(Compare-Object $setBefore.prof $setAfter.prof)
"SETTINGS_AFTER regLines=$($setAfter.reg.Count) regDiffLines=$($regDiff.Count) profileDiffLines=$($profDiff.Count)"
"  REGSHA before=" + (Get-FileHash $setBefore.file -Algorithm SHA256).Hash.Substring(0, 16) + " after=" + (Get-FileHash $setAfter.file -Algorithm SHA256).Hash.Substring(0, 16)
"  SECURITY_TERMS_IN_DIFF=" + @($regDiff | Where-Object { $_.InputObject -match "(?i)secur|authori|danger|trust|allow|LoadContainer|Arsenal" }).Count
"  PLAYMODE_TERMS_IN_DIFF=" + @($regDiff | Where-Object { $_.InputObject -match "(?i)server|peer|play|localhost|gamemode" }).Count
