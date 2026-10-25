# claude, TOOL_ONLY: read the World Editor menu bar and toolbar names of one own Workbench process (B0b step 1: where
# "Server localhost" lives, claude #4623/#4627, codex #4625/#4629). Only after START -> codex ACK; END after the run.
# No world, no launcher plugin, no Play. UI Automation only: names, control types and supported patterns. With -Expand,
# each top-level menu item and each toolbar item whose name contains "Play" is opened ONLY through
# ExpandCollapsePattern.Expand, its direct item names are read and it is closed with Collapse; submenus are not opened.
# Never Invoke, Toggle, SelectionItem, Value, keyboard or mouse. Any window other than the main editor window, or a
# Collapse that fails, stops the process. The own PID is stopped at the end; settings are snapshotted before and after.
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
$proc = Start-Process -FilePath $wb -ArgumentList $argList -PassThru -WindowStyle Hidden
$null = $proc.Handle
$wbPid = [uint32]$proc.Id
"PID $wbPid started $($started.ToString('HH:mm:ss'))"
$reason = "TIMEOUT"; $main = [IntPtr]::Zero; $mainTitle = ""
try {
	# wait for the main editor window, then let the UI settle; any unknown window stops the run
	while (((Get-Date) - $started).TotalSeconds -lt $Seconds) {
		$proc.Refresh(); if ($proc.HasExited) { $reason = "WORKBENCH_EXITED"; break }
		$unknown = UnknownWindow $wbPid $true
		if ($unknown) { "UNKNOWN_WINDOW title=`"$unknown`" (read only)"; $reason = "DIALOG_UNKNOWN_WINDOW"; break }
		foreach ($w in @([RiconWin]::List($wbPid))) {
			$parts = $w -split "\|", 3
			if ($parts[0] -eq "V" -and ($parts[2] -ceq "World Editor" -or $parts[2].StartsWith("World Editor - ", [StringComparison]::Ordinal))) { $mainTitle = $parts[2] }
		}
		if ($mainTitle) { $main = [RiconWin]::FindByTitle($wbPid, $mainTitle); break }
		Start-Sleep -Seconds 1
	}
	if ($main -ne [IntPtr]::Zero) {
		"MAIN_WINDOW t=$([int]((Get-Date) - $started).TotalSeconds) title=`"$(Short $mainTitle)`""
		@([RiconWin]::List($wbPid)) | Where-Object { $_.StartsWith("V|") } | ForEach-Object { "  OWN_WINDOW $(Short $_)" }
		Start-Sleep -Seconds $Settle
		$unknown = UnknownWindow $wbPid $true
		if ($unknown) { "UNKNOWN_WINDOW title=`"$unknown`" (read only)"; $reason = "DIALOG_UNKNOWN_WINDOW" }
		else {
			$el = $U::FromHandle($main)
			$bars = OfType $el $CT::MenuBar
			"MENUBARS count=$($bars.Count)"
			$k = 0
			foreach ($bar in $bars) {
				foreach ($item in (Children $bar)) {
					$label = "MENU[$k] `"$(Short $item.Current.Name)`""
					"$label type=$($item.Current.ControlType.ProgrammaticName) patterns=$(Patterns $item)"
					if ($Expand -and -not (ExpandRead $item $label $wbPid $main)) { $reason = "COLLAPSE_FAILED"; break }
					$k++
				}
				if ($reason -eq "COLLAPSE_FAILED") { break }
			}
			if ($reason -ne "COLLAPSE_FAILED") {
				$bars = OfType $el $CT::ToolBar
				"TOOLBARS count=$($bars.Count)"
				$k = 0
				foreach ($bar in $bars) {
					foreach ($item in (Children $bar)) {
						$name = $item.Current.Name
						if (-not $name) { continue }
						$label = "TOOL[$k] `"$(Short $name)`""
						"$label type=$($item.Current.ControlType.ProgrammaticName) patterns=$(Patterns $item)"
						if ($Expand -and $name -match '(?i)play' -and -not (ExpandRead $item $label $wbPid $main)) { $reason = "COLLAPSE_FAILED"; break }
						$k++
					}
					if ($reason -eq "COLLAPSE_FAILED") { break }
				}
			}
			if ($reason -eq "TIMEOUT") { $reason = "READ_DONE" }
			$unknown = UnknownWindow $wbPid $true
			if ($unknown) { "UNKNOWN_WINDOW_AFTER title=`"$unknown`" (read only)" }
		}
	}
} catch { $reason = "RUNNER_ERROR"; "RUNNER_ERROR $($_.Exception.GetType().Name): $($_.Exception.Message)" }

"STOP reason=$reason t=$([int]((Get-Date) - $started).TotalSeconds)"
if (Get-Process -Id $wbPid -ErrorAction SilentlyContinue) { Stop-Process -Id $wbPid -Force -Confirm:$false -ErrorAction SilentlyContinue }
Start-Sleep -Seconds 3
if (Get-Process -Id $wbPid -ErrorAction SilentlyContinue) { "OWN_PID_STILL_RUNNING" } else { "OWN_PID_GONE" }
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
