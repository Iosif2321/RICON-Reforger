# claude, TOOL_ONLY: helpers for wb-run-claude.ps1 (B0b, LIVE-peer-local-plan-codex.md section B0b).
# Read-only observation of the process set started by one Workbench run: descendants, their UDP/TCP endpoints and
# windows, plus the single guarded "Yes" on the vanilla Script Authorization prompt. Windows PowerShell 5.1; ASCII only.
# Nothing here writes to the registry, profile or other processes; Stop-Process is called only by the main script.

Add-Type -TypeDefinition @'
using System; using System.Text; using System.Collections.Generic; using System.Runtime.InteropServices;
public static class RiconWin {
	delegate bool EnumProc(IntPtr h, IntPtr l);
	[DllImport("user32.dll")] static extern bool EnumWindows(EnumProc f, IntPtr l);
	[DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
	[DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
	[DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetClassName(IntPtr h, StringBuilder s, int n);
	[DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
	// "V|class|title" or "h|class|title" for every top-level window of the process
	public static string[] List(uint target) {
		var r = new List<string>();
		EnumWindows((h, l) => {
			uint pid; GetWindowThreadProcessId(h, out pid);
			if (pid == target) {
				var t = new StringBuilder(256); var c = new StringBuilder(256);
				GetWindowText(h, t, 256); GetClassName(h, c, 256);
				r.Add((IsWindowVisible(h) ? "V" : "h") + "|" + c + "|" + t);
			}
			return true;
		}, IntPtr.Zero);
		return r.ToArray();
	}
	public static IntPtr FindByTitle(uint target, string title) {
		IntPtr found = IntPtr.Zero;
		EnumWindows((h, l) => {
			uint pid; GetWindowThreadProcessId(h, out pid);
			if (pid == target && IsWindowVisible(h)) {
				var t = new StringBuilder(256); GetWindowText(h, t, 256);
				if (t.ToString() == title) { found = h; return false; }
			}
			return true;
		}, IntPtr.Zero);
		return found;
	}
}
'@
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes

# --- pure helpers (covered by wb-run-selftest-claude.ps1) ---

# Address class only; the raw address never leaves the raw JSONL in the operator's temp directory.
function AddrClass([string]$a) {
	if ($a -eq "0.0.0.0" -or $a -eq "::") { return "ANY" }
	if ($a -match '^127\.' -or $a -eq "::1") { return "LOOPBACK" }
	if ($a -match '^10\.' -or $a -match '^192\.168\.' -or $a -match '^172\.(1[6-9]|2[0-9]|3[01])\.') { return "PRIVATE_V4" }
	if ($a -match '^169\.254\.') { return "LINKLOCAL_V4" }
	if ($a -match '^(?i)fe[89ab][0-9a-f]:') { return "LINKLOCAL_V6" }
	if ($a -match '^(?i)f[cd][0-9a-f]{2}:') { return "PRIVATE_V6" }
	return "OTHER"
}

# Names of the command-line switches only, in order, without values (paths and addresses are values).
function FlagNames([string]$cmd) {
	if (-not $cmd) { return , @() }
	$names = New-Object System.Collections.Generic.List[string]
	$bare = [regex]::Replace($cmd, '"[^"]*"', '""')   # a quoted value such as "C:\a -b\c" holds no switch
	foreach ($m in [regex]::Matches($bare, '(?:^|\s)(-[A-Za-z][A-Za-z0-9_]*)(?==|\s|$)')) {
		$n = $m.Groups[1].Value
		if (-not $names.Contains($n)) { $names.Add($n) }
	}
	return , $names.ToArray()
}

# Root plus every process whose parent chain reaches the root. $procs: objects with ProcessId, ParentProcessId,
# CreationDate. A parent PID is trusted only if the parent was created before the child (Windows reuses PIDs).
# Call as @(Descendants ...): one PID comes back unrolled, none as nothing.
function Descendants($procs, [uint32]$root) {
	$byId = @{}
	foreach ($p in $procs) { $byId[[uint32]$p.ProcessId] = $p }
	$set = New-Object System.Collections.Generic.HashSet[uint32]
	if (-not $byId.ContainsKey($root)) { return @() }
	[void]$set.Add($root)
	$changed = $true
	while ($changed) {
		$changed = $false
		foreach ($p in $procs) {
			$id = [uint32]$p.ProcessId; $pp = [uint32]$p.ParentProcessId
			if ($set.Contains($id) -or -not $set.Contains($pp) -or $id -eq $pp) { continue }
			$parent = $byId[$pp]
			if ($parent.CreationDate -and $p.CreationDate -and $parent.CreationDate -gt $p.CreationDate) { continue }
			[void]$set.Add($id); $changed = $true
		}
	}
	return @($set)
}

# --- teardown ownership (codex #4631) ---
# A child is stopped only through a Process object whose handle was opened when the child was first seen and whose
# StartTime then matched the CIM CreationDate: while that handle is open Windows cannot hand the PID to another process,
# and Kill() acts on the handle, not on a PID looked up again. No handle means the child is never stopped.
# Same start = equal UTC ticks after truncation to the microsecond (codex #4640): CIM CreationDate is the start
# FILETIME truncated to microseconds (on this PC 263 accessible processes lay 0..9 ticks below Process.StartTime), so
# truncating both and comparing Int64 exactly is the precise relation; no tolerance, no double arithmetic.
function UsTicks([datetime]$d) { $t = $d.ToUniversalTime().Ticks; return $t - ($t % 10) }
function SameStart($a, $b) { return ($null -ne $a -and $null -ne $b -and (UsTicks $a) -eq (UsTicks $b)) }
function OpenHeld([uint32]$id, $created) {
	try {
		$p = [System.Diagnostics.Process]::GetProcessById([int]$id)
		$null = $p.Handle
		if (SameStart $p.StartTime $created) { return $p }
		$p.Dispose()
	} catch {}
	return $null
}

# Separate watchdog process (codex #4639): holds its own handle on the target, checks that it is the same process (start
# FILETIME: equal UTC ticks) and kills it only if it is still running after $ms. Guards against a hung UI Automation call in the caller.
function StartWatchdog([System.Diagnostics.Process]$target, [int]$ms) {
	$ticks = $target.StartTime.ToUniversalTime().Ticks
	# the handle is opened before the start-time check and reused by WaitForExit/Kill (codex #4642)
	$cmd = "`$p = [System.Diagnostics.Process]::GetProcessById($($target.Id)); `$null = `$p.Handle; if (`$p.StartTime.ToUniversalTime().Ticks -eq $ticks) { if (-not `$p.WaitForExit($ms)) { `$p.Kill() } }"
	$enc = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($cmd))
	$w = Start-Process -FilePath "powershell.exe" -ArgumentList @("-NoProfile", "-NonInteractive", "-EncodedCommand", $enc) -PassThru -WindowStyle Hidden
	$null = $w.Handle
	return $w
}

# One decision for a tracked non-root process. $e: descendant, recordedCreated, launch, held (bool), heldExited (bool),
# heldStart (DateTime or $null).
function TeardownDecision($e) {
	if (-not $e.descendant) { return "SKIP_NOT_DESCENDANT" }
	if ($e.recordedCreated -lt $e.launch.AddSeconds(-1)) { return "SKIP_BEFORE_LAUNCH" }
	if (-not $e.held) { return "SKIP_NO_HANDLE" }
	if ($e.heldExited) { return "SKIP_EXITED" }
	if (-not (SameStart $e.heldStart $e.recordedCreated)) { return "SKIP_REUSED" }
	return "KILL_CHILD"
}

# Children first, the root last; the root is in the plan whatever $entries holds (an empty $tracked after a failed first
# poll still stops it through the Process object Start-Process returned).
function TeardownPlan([uint32]$rootPid, [bool]$rootExited, $entries) {
	$plan = @()
	foreach ($e in @($entries)) { $plan += [pscustomobject]@{ pid = $e.pid; action = (TeardownDecision $e) } }
	$plan += [pscustomobject]@{ pid = $rootPid; action = $(if ($rootExited) { "ROOT_EXITED" } else { "KILL_ROOT" }) }
	return , $plan
}

# Exact checks for the vanilla prompt (SCR_ArsenalManagerComponent.c:1101), unchanged from the B0a runner.
$script:ExpectedSite = "Scripts/Game/GameMode/Loadout/SCR_ArsenalManagerComponent.c:1101"
$script:ExpectedMethod = "BaseContainerTools.LoadContainer"
$script:ExpectedMessage = "Allow script '" + $script:ExpectedSite + "' to access potentially dangerous method '" + $script:ExpectedMethod + "'?"
$script:AuthTitle = "Script Authorization Required"
# bare module titles: an editor without an opened world (menu read, claude #4627) shows no " - <world>" suffix
$script:KnownTitles = @("Script Authorization Required", "World Editor", "Enfusion Workbench")
$script:KnownPrefixes = @("World Editor - ", "Enfusion Workbench - ")

# texts: non-empty names of non-button elements minus the button captions; captions: button names.
function PromptVerdict([string[]]$texts, [string[]]$captions) {
	if ($captions.Count -eq 0) { return "NOT_READY" }
	$okText = $texts.Count -eq 1 -and $texts[0] -ceq $script:ExpectedMessage
	$okButtons = (@($captions | Sort-Object) -join ",") -ceq "No,No to All,Yes,Yes to All"
	$yes = @($captions | Where-Object { $_ -ceq "Yes" }).Count
	if ($okText -and $okButtons -and $yes -eq 1) { return "EXACT" }
	return "UNEXPECTED"
}

# How many guarded presses may exist so far: 1, or 2 only in an explicit restart run after the launcher's own marker.
function YesLimit([bool]$restartRun, [bool]$secondPlayMarker) {
	if ($restartRun -and $secondPlayMarker) { return 2 }
	return 1
}

function WindowKnown([string]$title) {
	if ($title -eq "" -or $script:KnownTitles -ccontains $title) { return $true }
	foreach ($k in $script:KnownPrefixes) { if ($title.StartsWith($k, [StringComparison]::Ordinal)) { return $true } }
	return $false
}

# --- UI Automation (read; Invoke only through YesGuard) ---

function DialogElements($h, [bool]$quiet = $false) {
	$el = [System.Windows.Automation.AutomationElement]::FromHandle($h)
	$all = @($el.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition))
	if ($quiet) { return , $all }
	"  elements=$($all.Count)" | Write-Host
	$i = 0
	foreach ($e in $all) {
		if ($i -ge 20) { break }
		$name = $e.Current.Name
		if ($name.Length -gt 200) { $name = $name.Substring(0, 200) }
		"  UIA $i $($e.Current.ControlType.ProgrammaticName) name=`"$($name -replace "`r|`n", ' ')`"" | Write-Host
		$i++
	}
	return , $all
}

$script:yesPressed = 0; $script:yesHandles = @(); $script:yesAt = $null
# Only for the Workbench PID itself. Returns NONE, CLOSING, NOT_READY, PRESSED or a refusal (the caller stops the run).
function YesGuard([uint32]$wbPid, [int]$t, [bool]$restartRun, [bool]$secondPlayMarker) {
	$h = [RiconWin]::FindByTitle($wbPid, $script:AuthTitle)
	if ($h -eq [IntPtr]::Zero) { return "NONE" }
	if ($script:yesHandles -contains $h) {
		if (((Get-Date) - $script:yesAt).TotalSeconds -lt 6) { return "CLOSING" }
		"DIALOG_AFTER_PRESS t=$t handle=$h same handle, read only" | Write-Host
		try { $null = DialogElements $h } catch { "  UIA_ERROR $($_.Exception.Message)" | Write-Host }
		return "PRESS_NO_EFFECT"
	}
	$limit = YesLimit $restartRun $secondPlayMarker
	"DIALOG t=$t title=`"$($script:AuthTitle)`" handle=$h presses=$($script:yesPressed) limit=$limit" | Write-Host
	try { $all = DialogElements $h } catch { "  UIA_ERROR $($_.Exception.Message)" | Write-Host; return "UIA_ERROR" }
	$btn = [System.Windows.Automation.ControlType]::Button
	$captions = @($all | Where-Object { $_.Current.ControlType -eq $btn } | ForEach-Object { $_.Current.Name })
	$texts = @($all | Where-Object { $_.Current.ControlType -ne $btn } | ForEach-Object { $_.Current.Name -replace "`r|`n", " " } |
		Where-Object { $_ -ne "" -and $captions -cnotcontains $_ })
	$v = PromptVerdict $texts $captions
	"YES_GUARD verdict=$v buttons=`"$(@($captions | Sort-Object) -join ',')`"" | Write-Host
	if ($v -eq "NOT_READY") { return "NOT_READY" }
	if ($v -ne "EXACT") { return "UNEXPECTED_DIALOG" }
	if ($script:yesPressed -ge $limit) { return "LIMIT_REACHED" }
	$yes = @($all | Where-Object { $_.Current.ControlType -eq $btn -and $_.Current.Name -ceq "Yes" })
	try { $ip = $yes[0].GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern) } catch { return "NO_INVOKE" }
	$script:yesPressed++; $script:yesHandles += $h; $script:yesAt = Get-Date
	try { $ip.Invoke() } catch { "  INVOKE_ERROR $($_.Exception.Message)" | Write-Host; return "INVOKE_ERROR" }
	"YES_GUARD pressed button=`"Yes`" count=$($script:yesPressed) t=$t at=$($script:yesAt.ToString('HH:mm:ss.fff'))" | Write-Host
	return "PRESSED"
}

# Exact prompt texts per call site (codex #4813). Only the vanilla :1101 one is left: the A2 file flag and with it the
# launcher's FileIO.FileExists sites :174/:187 are gone (run 172649: a "Yes" on :187 did not cover the next poll pass).
$script:SiteMessages = [ordered]@{
	"ARSENAL_1101" = $script:ExpectedMessage
}
# Site key of a prompt, NOT_READY or UNEXPECTED; the same exact text and four-button checks as PromptVerdict.
function PromptSite([string[]]$texts, [string[]]$captions) {
	if ($captions.Count -eq 0) { return "NOT_READY" }
	$okButtons = (@($captions | Sort-Object) -join ",") -ceq "No,No to All,Yes,Yes to All"
	$yes = @($captions | Where-Object { $_ -ceq "Yes" }).Count
	if (-not $okButtons -or $yes -ne 1 -or $texts.Count -ne 1) { return "UNEXPECTED" }
	foreach ($k in $script:SiteMessages.Keys) { if ($texts[0] -ceq $script:SiteMessages[$k]) { return $k } }
	return "UNEXPECTED"
}
# Sites that may get a "Yes" in the current phase of a plugin-start run (codex #4848, #4850): none before the plugin
# command was invoked (played); after it only the vanilla one, once the launcher logged PLUGIN_SEEN (printed right
# before its SwitchToGameMode) and before the observation ended or edit mode was requested (codex #4817, #4819).
function SitesAllowed([bool]$played, [string]$logText) {
	if (-not $played) { return , @() }
	if ($logText -notmatch '\[RICON_LIVEB\] inspect=PLUGIN_SEEN') { return , @() }
	if ($logText -match '\[RICON_LIVEB\] inspect=(OBSERVATION_END|EDIT_REQUESTED|EDIT_STABLE)') { return , @() }
	return , @("ARSENAL_1101")
}
# site -> @{ h = window handle; at = press time }
$script:sitePressed = @{}
# Like YesGuard, but by site: at most one "Yes" per site of $allowed and process, at most $total in all. The site is
# read from the text first (a new prompt may reuse a closed window's handle, codex #4819); the closing grace of 6 s
# covers only the same site on the handle it was pressed on. A new window of a pressed site is SITE_REPEAT, a known
# site outside $allowed is SITE_NOT_ALLOWED; both stop the run.
function YesGuardSites([uint32]$wbPid, [int]$t, [string[]]$allowed, [int]$total = 3) {
	$h = [RiconWin]::FindByTitle($wbPid, $script:AuthTitle)
	if ($h -eq [IntPtr]::Zero) { return "NONE" }
	$prev = $null
	foreach ($k in @($script:sitePressed.Keys)) { if ($script:sitePressed[$k].h -eq $h) { $prev = $k } }
	$grace = $prev -and ((Get-Date) - $script:sitePressed[$prev].at).TotalSeconds -lt 6
	if (-not $grace) { "DIALOG t=$t title=`"$($script:AuthTitle)`" handle=$h presses=$($script:yesPressed) total=$total allowed=`"$($allowed -join ',')`"" | Write-Host }
	try { $all = DialogElements $h $grace } catch {
		if ($grace) { return "CLOSING" }
		"  UIA_ERROR $($_.Exception.Message)" | Write-Host; return "UIA_ERROR"
	}
	$btn = [System.Windows.Automation.ControlType]::Button
	$captions = @($all | Where-Object { $_.Current.ControlType -eq $btn } | ForEach-Object { $_.Current.Name })
	$texts = @($all | Where-Object { $_.Current.ControlType -ne $btn } | ForEach-Object { $_.Current.Name -replace "`r|`n", " " } |
		Where-Object { $_ -ne "" -and $captions -cnotcontains $_ })
	$site = PromptSite $texts $captions
	if ($grace -and ($site -eq "NOT_READY" -or $site -ceq $prev)) { return "CLOSING" }
	if ($grace) { "DIALOG t=$t handle=$h reused after site $prev" | Write-Host; $null = DialogElements $h }
	"YES_GUARD site=$site buttons=`"$(@($captions | Sort-Object) -join ',')`"" | Write-Host
	if ($prev -and $site -ceq $prev) { "DIALOG_AFTER_PRESS t=$t handle=$h site=$site same handle, read only" | Write-Host; return "PRESS_NO_EFFECT" }
	if ($site -eq "NOT_READY") { return "NOT_READY" }
	if ($site -eq "UNEXPECTED") { return "UNEXPECTED_DIALOG" }
	if ($script:sitePressed.ContainsKey($site)) { return "SITE_REPEAT" }
	if ($allowed -cnotcontains $site) { return "SITE_NOT_ALLOWED" }
	if ($script:yesPressed -ge $total) { return "LIMIT_REACHED" }
	$yes = @($all | Where-Object { $_.Current.ControlType -eq $btn -and $_.Current.Name -ceq "Yes" })
	try { $ip = $yes[0].GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern) } catch { return "NO_INVOKE" }
	$script:yesPressed++; $script:yesHandles += $h; $script:yesAt = Get-Date; $script:sitePressed[$site] = @{ h = $h; at = $script:yesAt }
	try { $ip.Invoke() } catch { "  INVOKE_ERROR $($_.Exception.Message)" | Write-Host; return "INVOKE_ERROR" }
	"YES_GUARD pressed button=`"Yes`" site=$site count=$($script:yesPressed) t=$t at=$($script:yesAt.ToString('HH:mm:ss.fff'))" | Write-Host
	return "PRESSED"
}

# Menu reader only (codex #4647, #4656): two exact startup titles of the own Workbench (both seen at t=2..11 s in earlier
# runs) are passive while the editor window has not appeared yet and at most 30 s have passed; they are never opened
# or pressed. Not part of WindowKnown; once the editor is found or 30 s passed, they stop the run like any other title.
$script:StartupTitles = @("Arma Reforger Workbench", "MainWindow")
function StartupWindowAllowed([string]$title, [bool]$mainFound, [double]$elapsed) {
	return ($script:StartupTitles -ccontains $title -and -not $mainFound -and $elapsed -le 30)
}

# Server Play (run 174552): about 1 s after the launcher's PLUGIN_PLAY_REQUESTED the own Workbench showed its Qt window
# "Arma Reforger Workbench" visible (the title Qt gives an untitled window; before, seen visible only at startup). In a
# plugin-start run the caller lets it through ($inPlay) only from PLUGIN_SEEN until EDIT_STABLE and only in the
# Workbench PID; it never gets input and is read once per new handle with PlayWindowRead (codex #4870).
$script:PlayTitles = @("Arma Reforger Workbench")

# Like UnknownWindow for the Workbench PID, but every visible titled window is checked and the startup title is let
# through only under StartupWindowAllowed, the Play titles only while $inPlay.
function UnknownWindowStartup([uint32]$id, [bool]$mainFound, [double]$elapsed, [bool]$inPlay = $false) {
	foreach ($w in @([RiconWin]::List($id))) {
		if (-not $w.StartsWith("V|")) { continue }
		$title = ($w -split "\|", 3)[2]
		if ($title -eq "" -or (WindowKnown $title) -or (StartupWindowAllowed $title $mainFound $elapsed)) { continue }
		if ($inPlay -and $script:PlayTitles -ccontains $title) { continue }
		return $title
	}
	return ""
}
# Read only, no pattern call: @{ size; count; buttons; lines } of one window, lines = its first 20 elements.
function PlayWindowRead([IntPtr]$h) {
	$el = [System.Windows.Automation.AutomationElement]::FromHandle($h)
	$r = $el.Current.BoundingRectangle
	$all = @($el.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition))
	$lines = @($all | Select-Object -First 20 | ForEach-Object { $_.Current.ControlType.ProgrammaticName + " `"" + (Short $_.Current.Name) + "`"" })
	$buttons = @($all | Where-Object { $_.Current.ControlType -eq [System.Windows.Automation.ControlType]::Button }).Count
	return @{ size = "$([int]$r.Width)x$([int]$r.Height)"; count = $all.Count; buttons = $buttons; lines = $lines }
}

# First visible window of a tracked PID whose title is not known; "" when none. For a child PID even the
# authorization title is unknown here: a prompt in another process is read, never pressed.
function UnknownWindow([uint32]$id, [bool]$isWorkbench) {
	foreach ($w in @([RiconWin]::List($id))) {
		if (-not $w.StartsWith("V|")) { continue }
		$title = ($w -split "\|", 3)[2]
		if ($title -eq "") { continue }
		if ($isWorkbench -and (WindowKnown $title)) { continue }
		return $title
	}
	return ""
}

# --- menu reading (wb-menu-read-claude.ps1) ---
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
# Read-only state of one menu item (codex #4663): enabled, ExpandCollapse state, current Toggle and SelectionItem state when
# those patterns exist. Only .Current properties are read; no pattern method is called.
function ItemInfo($i) {
	$s = " enabled=" + $i.Current.IsEnabled
	try { $s += " state=" + $i.GetCurrentPattern($ECP::Pattern).Current.ExpandCollapseState } catch {}
	try { $s += " toggle=" + $i.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern).Current.ToggleState } catch {}
	try { $s += " selected=" + $i.GetCurrentPattern([System.Windows.Automation.SelectionItemPattern]::Pattern).Current.IsSelected } catch {}
	return $s
}
# Items of an opened menu: popups are separate top-level windows of the own PID in Qt, so read both the element's
# own descendants ([el]) and every own top-level window other than the main one ([win], with the window title, codex
# review of run 114055: the other editor window's menu bar showed up there). Nested items: name and state only.
function OpenedItems($el, [uint32]$ownPid, [IntPtr]$mainHandle) {
	$names = New-Object System.Collections.Generic.List[string]
	foreach ($i in (OfType $el $CT::MenuItem)) { $names.Add("[el] " + (Short $i.Current.Name) + (ItemInfo $i)) }
	$root = [System.Windows.Automation.AutomationElement]::RootElement
	$c = New-Object System.Windows.Automation.PropertyCondition($U::ProcessIdProperty, [int]$ownPid)
	foreach ($w in @($root.FindAll([System.Windows.Automation.TreeScope]::Children, $c))) {
		if ($w.Current.NativeWindowHandle -eq $mainHandle.ToInt32()) { continue }
		$src = "[win `"" + (Short $w.Current.Name) + "`"] "
		foreach ($i in (OfType $w $CT::MenuItem)) { $names.Add($src + (Short $i.Current.Name) + (ItemInfo $i)) }
	}
	return , $names.ToArray()
}
# Opens $el only through ExpandCollapse, prints its items to the host and closes it. Returns ONE bool: $false when the
# state after Collapse is not exactly Collapsed or Collapse throws (the caller stops). All text goes through Write-Host: in the first
# OpenWorld read (ricon-b0b-menu-claude-20261025-113705) pipeline output here became part of the return value, so
# the lines were lost and a failed Collapse could not be seen.
# Closed means exactly Collapsed (codex #4662): PartiallyExpanded or LeafNode after Collapse() is not a closed menu.
function ClosedOk($state) { return ([string]$state -ceq "Collapsed") }
# Child menu item with exactly this name, first among the element's descendants, then in the own popup windows.
function FindMenuChild($el, [string]$name, [uint32]$ownPid, [IntPtr]$mainHandle) {
	foreach ($i in (OfType $el $CT::MenuItem)) { if ($i.Current.Name -ceq $name) { return $i } }
	$root = [System.Windows.Automation.AutomationElement]::RootElement
	$c = New-Object System.Windows.Automation.PropertyCondition($U::ProcessIdProperty, [int]$ownPid)
	foreach ($w in @($root.FindAll([System.Windows.Automation.TreeScope]::Children, $c))) {
		if ($w.Current.NativeWindowHandle -eq $mainHandle.ToInt32()) { continue }
		foreach ($i in (OfType $w $CT::MenuItem)) { if ($i.Current.Name -ceq $name) { return $i } }
	}
	return $null
}
# $childName (codex #4661 proposal): while $el is open, open exactly that one child the same way, read it, close it,
# then close $el. One level only: the inner call gets no child name.
function ExpandRead($el, [string]$label, [uint32]$ownPid, [IntPtr]$mainHandle, [string]$childName = "") {
	try { $p = $el.GetCurrentPattern($ECP::Pattern) } catch { "  $label NO_EXPAND_COLLAPSE (not opened)" | Write-Host; return $true }
	try { $p.Expand() } catch { "  $label EXPAND_ERROR $($_.Exception.GetType().Name)" | Write-Host; return $true }
	# the element's own items can appear late (read 131708: Game showed only the other window's 8 items after 700 ms)
	Start-Sleep -Milliseconds 700
	for ($k = 0; $k -lt 12 -and @(OfType $el $CT::MenuItem).Count -eq 0; $k++) { Start-Sleep -Milliseconds 200 }
	$items = OpenedItems $el $ownPid $mainHandle
	"  $label OPENED items=$($items.Count)" | Write-Host
	foreach ($n in @($items | Select-Object -First 40)) { "    ITEM `"$n`"" | Write-Host }
	$childOk = $true
	if ($childName) {
		$child = FindMenuChild $el $childName $ownPid $mainHandle
		if ($null -eq $child) { "  $label CHILD_NOT_FOUND `"$childName`"" | Write-Host }
		else {
			$childLabel = "$label > `"$childName`""
			"  $childLabel type=$($child.Current.ControlType.ProgrammaticName) patterns=$(Patterns $child)" | Write-Host
			$childOk = [bool](ExpandRead $child $childLabel $ownPid $mainHandle "")
		}
	}
	try { $p.Collapse() } catch { "  $label COLLAPSE_ERROR $($_.Exception.GetType().Name)" | Write-Host; return $false }
	Start-Sleep -Milliseconds 400
	$state = $p.Current.ExpandCollapseState
	"  $label CLOSED state=$state" | Write-Host
	return ($childOk -and (ClosedOk $state))
}

# --- toggle roundtrip (codex #4668) ---
# One target toggle of Game > Play Game Options, changed once and put back, judged only by re-read states:
#   baseline B (all Toggle items) -> target must be Off -> Toggle(target) -> re-read S1 -> D = items that differ from B
#   -> Toggle(target) again only if it differs from B -> re-read -> for every other item of D that still differs, one
#   Toggle each (an exclusive group may have switched it), at most $maxRestore -> final read must equal B.
# Only items changed by this run are ever toggled; every step opens the menus fresh and closes them (Collapsed).
function FindTopMenu([IntPtr]$mainHandle, [string]$name) {
	foreach ($i in (OfType ($U::FromHandle($mainHandle)) $CT::MenuItem)) { if ($i.Current.Name -ceq $name) { return $i } }
	return $null
}
function OpenOptions([IntPtr]$mainHandle, [uint32]$ownPid) {
	$game = FindTopMenu $mainHandle "Game"
	if ($null -eq $game) { "  ROUNDTRIP no Game menu" | Write-Host; return $null }
	try { $game.GetCurrentPattern($ECP::Pattern).Expand() } catch { "  ROUNDTRIP Game expand error" | Write-Host; return $null }
	Start-Sleep -Milliseconds 600
	$opt = FindMenuChild $game "Play Game Options" $ownPid $mainHandle
	if ($null -eq $opt) { "  ROUNDTRIP no Play Game Options" | Write-Host; $null = CloseOptions @{ game = $game; opt = $null } $mainHandle; return $null }
	try { $opt.GetCurrentPattern($ECP::Pattern).Expand() } catch { "  ROUNDTRIP Options expand error" | Write-Host; return $null }
	# the submenu items can appear late after a quick re-open (self-test: NO_ITEM on the second open; under load even
	# a 3 s wait missed once): wait up to 6 s
	for ($k = 0; $k -lt 30; $k++) {
		Start-Sleep -Milliseconds 200
		if (@(OfType $opt $CT::MenuItem).Count -gt 0) { return @{ game = $game; opt = $opt } }
	}
	"  ROUNDTRIP Options opened without items" | Write-Host
	$null = CloseOptions @{ game = $game; opt = $opt } $mainHandle
	return $null
}
# Collapses Options then Game when still open; true only if a fresh Game element reads exactly Collapsed.
function CloseOptions($h, [IntPtr]$mainHandle) {
	foreach ($el in @($h.opt, $h.game)) {
		if ($null -eq $el) { continue }
		try {
			$p = $el.GetCurrentPattern($ECP::Pattern)
			if (-not (ClosedOk $p.Current.ExpandCollapseState)) { $p.Collapse(); Start-Sleep -Milliseconds 300 }
		} catch {}
	}
	$game = FindTopMenu $mainHandle "Game"
	if ($null -eq $game) { return $false }
	try { return (ClosedOk $game.GetCurrentPattern($ECP::Pattern).Current.ExpandCollapseState) } catch { return $false }
}
# Ordered name -> ToggleState of the Toggle-capable items of the open Options element, plus "bad" (codex #4675): an item
# that offers TogglePattern but whose state cannot be read, or a repeated name, makes the whole snapshot unusable.
function ReadToggles($opt) {
	$map = [ordered]@{}; $bad = ""
	foreach ($i in (OfType $opt $CT::MenuItem)) {
		$tp = $null
		$name = $i.Current.Name
		if (-not $i.TryGetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern, [ref]$tp)) { continue }
		if ($map.Contains($name)) { $bad = "DUPLICATE `"$name`""; continue }
		try { $map[$name] = [string]$tp.Current.ToggleState } catch { $bad = "READ_ERROR `"$name`"" }
	}
	return @{ map = $map; bad = $bad }
}
# Pure (codex #4675): "" when the snapshot is complete and definite, else the reason. Every state must be exactly On or
# Off, every required name present, and after the baseline the same names in the same order.
function SnapshotProblem($map, [string[]]$baseKeys, [string[]]$required) {
	foreach ($k in @($map.Keys)) { if ($map[$k] -cne "On" -and $map[$k] -cne "Off") { return "STATE_NOT_ON_OFF `"$k`"=$($map[$k])" } }
	foreach ($r in @($required | Where-Object { $_ })) { if (-not $map.Contains($r)) { return "MISSING `"$r`"" } }
	if ($null -ne $baseKeys -and (@($map.Keys) -join "|") -cne (@($baseKeys) -join "|")) { return "KEYS_CHANGED" }
	return ""
}
# Toggles exactly one item of the open Options element, only if it is found once and its state, read right now in this
# fresh menu, is exactly $expected (codex #4677). Returns TOGGLED, STALE (state differs), NO_ITEM, AMBIGUOUS, NO_PATTERN,
# READ_ERROR or TOGGLE_ERROR; only TOGGLED means Toggle() was called.
# $method (claude #4684 option A): "Toggle" calls TogglePattern.Toggle(); "Invoke" calls InvokePattern.Invoke() of the same
# item (a menu click; the first live roundtrip showed Toggle() leaves a Qt menu action unchanged). The state check before
# the action is identical for both.
function ToggleExpected($opt, [string]$name, [string]$expected, [string]$method = "Toggle") {
	$hits = @(OfType $opt $CT::MenuItem | Where-Object { $_.Current.Name -ceq $name })
	if ($hits.Count -eq 0) { return "NO_ITEM" }
	if ($hits.Count -gt 1) { return "AMBIGUOUS" }
	$tp = $null
	if (-not $hits[0].TryGetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern, [ref]$tp)) { return "NO_PATTERN" }
	try { $now = [string]$tp.Current.ToggleState } catch { return "READ_ERROR" }
	if ($now -cne $expected) { return "STALE" }
	if ($method -ceq "Invoke") {
		$ip = $null
		if (-not $hits[0].TryGetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern, [ref]$ip)) { return "NO_PATTERN" }
		try { $ip.Invoke() } catch { return "TOGGLE_ERROR" }
		return "TOGGLED"
	}
	if ($method -cne "Toggle") { return "BAD_METHOD" }
	try { $tp.Toggle() } catch { return "TOGGLE_ERROR" }
	return "TOGGLED"
}
# Pure (codex #4686): PIDs present now but not in the baseline set (both: arrays of PIDs).
function NewPids($baseline, $now) { return , @(@($now) | Where-Object { @($baseline) -notcontains $_ }) }

# Pure: names whose state differs between two maps (a missing name counts as different).
function ToggleDiff($a, $b) {
	$names = New-Object System.Collections.Generic.List[string]
	foreach ($k in @($a.Keys) + @($b.Keys)) {
		if ($names.Contains($k)) { continue }
		if (-not ($a.Contains($k) -and $b.Contains($k) -and $a[$k] -ceq $b[$k])) { $names.Add($k) }
	}
	return , $names.ToArray()
}
function MapText($m) { (@($m.Keys | ForEach-Object { "$_=$($m[$_])" }) -join " | ") }
# $mustBeOff (codex #4674): every listed item must read exactly Off in the baseline, else nothing is toggled.
function RoundTrip([IntPtr]$mainHandle, [uint32]$ownPid, [string]$target, [scriptblock]$blocked, [string[]]$mustBeOff, [int]$maxRestore = 3, [string]$method = "Toggle") {
	$script:roundTripToggles = 0
	$script:rtBlockedAfter = ""
	$required = @($target) + @($mustBeOff | Where-Object { $_ })
	$baseKeys = $null
	# a snapshot is returned only when complete and definite; otherwise $null and no further toggle follows. A failed
	# read toggles nothing, so it is retried up to 3 times (self-test: rare UI timing misses on a quick re-open).
	$snap = {
		for ($try = 1; $try -le 3; $try++) {
			$h = OpenOptions $mainHandle $ownPid
			if ($null -ne $h) {
				$r = ReadToggles $h.opt
				$closed = CloseOptions $h $mainHandle
				$why = if (-not $closed) { "menus not Collapsed after read" } elseif ($r.bad) { $r.bad } else { SnapshotProblem $r.map $baseKeys $required }
				if (-not $why) { return $r.map }
				"  ROUNDTRIP snapshot try=$try unusable: $why" | Write-Host
				if (-not $closed) { return $null }
			} else { "  ROUNDTRIP snapshot try=$($try) menus not opened" | Write-Host }
			Start-Sleep -Milliseconds 500
		}
		return $null
	}
	# one toggle from a known state: $expected is the item's state in the last complete snapshot
	$flip = {
		param([string]$name, [string]$expected)
		$b = & $blocked
		if ($b) { "  ROUNDTRIP blocked before toggle: $b" | Write-Host; return $false }
		# a second attempt only when Toggle() was certainly not called (item or pattern not found, state unreadable)
		for ($try = 1; $try -le 3; $try++) {
			$h = OpenOptions $mainHandle $ownPid
			# not opened: nothing was acted on, so another attempt is safe
			if ($null -eq $h) { "  ROUNDTRIP open for action try=$($try) failed" | Write-Host; Start-Sleep -Milliseconds 700; continue }
			$res = ToggleExpected $h.opt $name $expected $method
			if ($res -ceq "TOGGLED") { $script:roundTripToggles++ }
			"  ROUNDTRIP $($method.ToUpper()) `"$name`" from=$expected result=$res try=$try count=$($script:roundTripToggles)" | Write-Host
			Start-Sleep -Milliseconds 700
			$closed = CloseOptions $h $mainHandle
			if ($res -ceq "TOGGLED") {
				# after every action (codex #4686): a prompt, an unknown window, a Play marker or a new process ends the
				# experiment at once, without a further read or toggle
				$after = & $blocked
				if ($after) { $script:rtBlockedAfter = $after; "  ROUNDTRIP blocked after action: $after" | Write-Host; return $false }
				return $closed
			}
			if ($res -notin @("NO_ITEM", "NO_PATTERN", "READ_ERROR") -or -not $closed) { return $false }
			Start-Sleep -Milliseconds 500
		}
		return $false
	}
	$base = & $snap
	if ($null -eq $base) { return "BASELINE_READ_FAILED" }
	$baseKeys = @($base.Keys)
	"  ROUNDTRIP BASELINE $(MapText $base)" | Write-Host
	foreach ($n in @($target) + @($mustBeOff | Where-Object { $_ })) { if (-not $base.Contains($n) -or $base[$n] -cne "Off") { "  ROUNDTRIP baseline `"$n`" is not Off" | Write-Host; return "BASELINE_NOT_OFF" } }
	if (-not (& $flip $target $base[$target])) {
		if ($script:rtBlockedAfter) { return "STOPPED_AFTER_ACTION|$($script:rtBlockedAfter)" }
		# the first toggle was not confirmed; read once more to report, never toggle again from here
		$s = & $snap
		if ($s) { "  ROUNDTRIP AFTER_FAILED_TOGGLE $(MapText $s) diff=$((ToggleDiff $base $s) -join ',')" | Write-Host }
		if ($script:roundTripToggles -gt 0) { return "TOGGLE_FAILED|CHECK_STATE" }
		return "TOGGLE_FAILED"
	}
	$s1 = & $snap
	if ($null -eq $s1) { return "STATE1_READ_FAILED" }
	$d = ToggleDiff $base $s1
	"  ROUNDTRIP STATE1 $(MapText $s1) diff=$($d -join ',')" | Write-Host
	$phase = if ($s1[$target] -ceq "On") { "TOGGLED_ON" } else { "TOGGLE_NO_EFFECT" }
	$cur = $s1
	if ($cur[$target] -cne $base[$target]) {
		if (-not (& $flip $target $cur[$target])) { if ($script:rtBlockedAfter) { return "$phase|STOPPED_AFTER_ACTION|$($script:rtBlockedAfter)" }; return "$phase|RESTORE_TOGGLE_FAILED" }
		$cur = & $snap
		if ($null -eq $cur) { return "$phase|RESTORE_READ_FAILED" }
		"  ROUNDTRIP STATE2 $(MapText $cur) diff=$((ToggleDiff $base $cur) -join ',')" | Write-Host
	}
	$extra = 0
	foreach ($n in $d) {
		if ($n -ceq $target -or $cur[$n] -ceq $base[$n]) { continue }
		if ($extra -ge $maxRestore) { break }
		if (-not (& $flip $n $cur[$n])) { if ($script:rtBlockedAfter) { return "$phase|STOPPED_AFTER_ACTION|$($script:rtBlockedAfter)" }; return "$phase|RESTORE_TOGGLE_FAILED" }
		$extra++
		$cur = & $snap
		if ($null -eq $cur) { return "$phase|RESTORE_READ_FAILED" }
		"  ROUNDTRIP STATE_AFTER_$($n -replace ' ', '_') $(MapText $cur)" | Write-Host
	}
	$left = ToggleDiff $base $cur
	"  ROUNDTRIP FINAL $(MapText $cur) diffFromBaseline=$($left -join ',') toggles=$($script:roundTripToggles)" | Write-Host
	if ($left.Count -eq 0) { return "$phase|RESTORE_OK" }
	return "$phase|RESTORE_INCOMPLETE"
}

# --- server-mode Play from the UI (B0b step 2, codex #4693/#4694) ---
# One strict snapshot of Game > Play Game Options outside RoundTrip: up to 3 reads (a read toggles nothing), each complete
# and definite as in RoundTrip; $null when none is.
function StrictSnapshot([IntPtr]$mainHandle, [uint32]$ownPid, $baseKeys, [string[]]$required) {
	for ($try = 1; $try -le 3; $try++) {
		$h = OpenOptions $mainHandle $ownPid
		if ($null -ne $h) {
			$r = ReadToggles $h.opt
			$closed = CloseOptions $h $mainHandle
			$why = if (-not $closed) { "menus not Collapsed after read" } elseif ($r.bad) { $r.bad } else { SnapshotProblem $r.map $baseKeys $required }
			if (-not $why) { return $r.map }
			"  SNAPSHOT try=$($try) unusable: $why" | Write-Host
			if (-not $closed) { return $null }
		} else { "  SNAPSHOT try=$($try) menus not opened" | Write-Host }
		Start-Sleep -Milliseconds 500
	}
	return $null
}
# Invokes one command of a top-level menu: the child must be found exactly once by exact name among the open menu's
# items and must be a plain command (InvokePattern, no TogglePattern, no ExpandCollapse). Returns INVOKED, NO_MENU,
# NO_ITEM, AMBIGUOUS, NOT_A_COMMAND, NO_INVOKE, INVOKE_ERROR or NOT_CLOSED (the menu stayed open after Invoke).
# $script:menuCommandAttempted (codex #4703) turns true right before Invoke() is called and stays so: whatever the result,
# the caller must then treat the command as possibly executed.
function InvokeMenuCommand([IntPtr]$mainHandle, [string]$menu, [string]$item) {
	$script:menuCommandAttempted = $false
	$top = FindTopMenu $mainHandle $menu
	if ($null -eq $top) { return "NO_MENU" }
	try { $top.GetCurrentPattern($ECP::Pattern).Expand() } catch { return "NO_MENU" }
	$hits = @()
	for ($k = 0; $k -lt 15 -and $hits.Count -eq 0; $k++) {
		Start-Sleep -Milliseconds 200
		$hits = @(OfType $top $CT::MenuItem | Where-Object { $_.Current.Name -ceq $item })
	}
	$close = { try { $p = $top.GetCurrentPattern($ECP::Pattern); if (-not (ClosedOk $p.Current.ExpandCollapseState)) { $p.Collapse() } } catch {} }
	if ($hits.Count -eq 0) { & $close; return "NO_ITEM" }
	if ($hits.Count -gt 1) { & $close; return "AMBIGUOUS" }
	$x = $null
	if ($hits[0].TryGetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern, [ref]$x) -or
		$hits[0].TryGetCurrentPattern($ECP::Pattern, [ref]$x)) { & $close; return "NOT_A_COMMAND" }
	$ip = $null
	if (-not $hits[0].TryGetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern, [ref]$ip)) { & $close; return "NO_INVOKE" }
	$script:menuCommandAttempted = $true
	try { $ip.Invoke() } catch { & $close; return "INVOKE_ERROR" }
	Start-Sleep -Milliseconds 700
	& $close
	Start-Sleep -Milliseconds 300
	$fresh = FindTopMenu $mainHandle $menu
	try { if (-not (ClosedOk $fresh.GetCurrentPattern($ECP::Pattern).Current.ExpandCollapseState)) { return "NOT_CLOSED" } } catch { return "NOT_CLOSED" }
	return "INVOKED"
}
# Async Play (codex #4717/#4719): the run 125954 sat ~125 s inside a synchronous InvokePattern.Invoke() of Game > Play
# Game with no observation. PrepareMenuCommand does every check of InvokeMenuCommand and leaves the menu open, returning
# @{ result = "READY"; ip; top } (or a refusal with the menu closed); StartAsyncInvoke runs only ip.Invoke() in a separate
# runspace of this process, so the caller's loop keeps observing. EndInvoke only after IsCompleted.
function PrepareMenuCommand([IntPtr]$mainHandle, [string]$menu, [string]$item) {
	$top = FindTopMenu $mainHandle $menu
	if ($null -eq $top) { return @{ result = "NO_MENU" } }
	try { $top.GetCurrentPattern($ECP::Pattern).Expand() } catch { return @{ result = "NO_MENU" } }
	$hits = @()
	for ($k = 0; $k -lt 15 -and $hits.Count -eq 0; $k++) {
		Start-Sleep -Milliseconds 200
		$hits = @(OfType $top $CT::MenuItem | Where-Object { $_.Current.Name -ceq $item })
	}
	$why = ""
	$x = $null; $ip = $null
	if ($hits.Count -eq 0) { $why = "NO_ITEM" }
	elseif ($hits.Count -gt 1) { $why = "AMBIGUOUS" }
	elseif ($hits[0].TryGetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern, [ref]$x) -or
		$hits[0].TryGetCurrentPattern($ECP::Pattern, [ref]$x)) { $why = "NOT_A_COMMAND" }
	elseif (-not $hits[0].TryGetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern, [ref]$ip)) { $why = "NO_INVOKE" }
	if ($why) {
		# names only, for the stop line: the first read of a menu (Plugins, codex #4850) must show what was there instead
		$names = @(OfType $top $CT::MenuItem | ForEach-Object { Short $_.Current.Name } | Select-Object -First 40)
		try { $p = $top.GetCurrentPattern($ECP::Pattern); if (-not (ClosedOk $p.Current.ExpandCollapseState)) { $p.Collapse() } } catch {}
		return @{ result = $why; names = $names }
	}
	return @{ result = "READY"; ip = $ip; top = $top }
}
function StartAsyncInvoke($ip) {
	$ps = [powershell]::Create()
	[void]$ps.AddScript({ param($p) $p.Invoke() }).AddArgument($ip)
	$ar = $ps.BeginInvoke()
	return @{ ps = $ps; ar = $ar; started = Get-Date; done = $false; result = "" }
}
# Non-blocking: once the worker completed, EndInvoke once and record COMPLETED or ERROR <type>; returns $true when done.
function PollAsyncInvoke($job) {
	if ($job.done) { return $true }
	if (-not $job.ar.IsCompleted) { return $false }
	try {
		$null = $job.ps.EndInvoke($job.ar)
		$job.result = if ($job.ps.HadErrors) { "ERROR " + (@($job.ps.Streams.Error | ForEach-Object { $_.Exception.GetType().Name }) -join ",") } else { "COMPLETED" }
	} catch { $job.result = "ERROR " + $_.Exception.GetType().Name }
	$job.done = $true
	try { $job.ps.Dispose() } catch {}
	return $true
}
# After the own Workbench is gone: wait at most $ms for the worker, then dispose it or leave it (never a blocking Stop).
function CleanupAsyncInvoke($job, [int]$ms) {
	if ($null -eq $job) { return "NONE" }
	$until = (Get-Date).AddMilliseconds($ms)
	while (-not (PollAsyncInvoke $job) -and (Get-Date) -lt $until) { Start-Sleep -Milliseconds 200 }
	if ($job.done) { return "DONE " + $job.result }
	try { [void]$job.ps.BeginStop($null, $null) } catch {}
	return "WORKER_LEFT"
}

# Pure: the only allowed difference after switching the target On is exactly {target}; "" when so, else the reason.
function OnlyTargetChanged($base, $snap, [string]$target) {
	$d = ToggleDiff $base $snap
	if ($d.Count -ne 1 -or $d[0] -cne $target) { return "DIFF " + ($d -join ",") }
	if ($snap[$target] -cne "On") { return "TARGET_NOT_ON" }
	return ""
}
