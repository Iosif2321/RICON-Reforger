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

function DialogElements($h) {
	$el = [System.Windows.Automation.AutomationElement]::FromHandle($h)
	$all = @($el.FindAll([System.Windows.Automation.TreeScope]::Descendants, [System.Windows.Automation.Condition]::TrueCondition))
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

# Menu reader only (codex #4647, #4656): two exact startup titles of the own Workbench (both seen at t=2..11 s in earlier
# runs) are passive while the editor window has not appeared yet and at most 30 s have passed; they are never opened
# or pressed. Not part of WindowKnown; once the editor is found or 30 s passed, they stop the run like any other title.
$script:StartupTitles = @("Arma Reforger Workbench", "MainWindow")
function StartupWindowAllowed([string]$title, [bool]$mainFound, [double]$elapsed) {
	return ($script:StartupTitles -ccontains $title -and -not $mainFound -and $elapsed -le 30)
}

# Like UnknownWindow for the Workbench PID, but every visible titled window is checked and the startup title is let
# through only under StartupWindowAllowed.
function UnknownWindowStartup([uint32]$id, [bool]$mainFound, [double]$elapsed) {
	foreach ($w in @([RiconWin]::List($id))) {
		if (-not $w.StartsWith("V|")) { continue }
		$title = ($w -split "\|", 3)[2]
		if ($title -eq "" -or (WindowKnown $title) -or (StartupWindowAllowed $title $mainFound $elapsed)) { continue }
		return $title
	}
	return ""
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
	Start-Sleep -Milliseconds 700
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
	Start-Sleep -Milliseconds 600
	return @{ game = $game; opt = $opt }
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
# Ordered name -> ToggleState of the Toggle-capable direct items of the open Options element.
function ReadToggles($opt) {
	$map = [ordered]@{}
	foreach ($i in (OfType $opt $CT::MenuItem)) {
		try { $map[$i.Current.Name] = [string]$i.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern).Current.ToggleState } catch {}
	}
	return $map
}
function ToggleItem($opt, [string]$name) {
	foreach ($i in (OfType $opt $CT::MenuItem)) {
		if ($i.Current.Name -ceq $name) {
			try { $i.GetCurrentPattern([System.Windows.Automation.TogglePattern]::Pattern).Toggle(); return $true } catch { return $false }
		}
	}
	return $false
}
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
function RoundTrip([IntPtr]$mainHandle, [uint32]$ownPid, [string]$target, [scriptblock]$blocked, [string[]]$mustBeOff, [int]$maxRestore = 3) {
	$script:roundTripToggles = 0
	$snap = {
		$h = OpenOptions $mainHandle $ownPid
		if ($null -eq $h) { return $null }
		$m = ReadToggles $h.opt
		if (-not (CloseOptions $h $mainHandle)) { "  ROUNDTRIP menus not Collapsed after read" | Write-Host; return $null }
		return $m
	}
	$flip = {
		param([string]$name)
		$b = & $blocked
		if ($b) { "  ROUNDTRIP blocked before toggle: $b" | Write-Host; return $false }
		$h = OpenOptions $mainHandle $ownPid
		if ($null -eq $h) { return $false }
		$ok = ToggleItem $h.opt $name
		$script:roundTripToggles++
		"  ROUNDTRIP TOGGLE `"$name`" invoked=$ok count=$($script:roundTripToggles)" | Write-Host
		Start-Sleep -Milliseconds 700
		$closed = CloseOptions $h $mainHandle
		return ($ok -and $closed)
	}
	$base = & $snap
	if ($null -eq $base) { return "BASELINE_READ_FAILED" }
	"  ROUNDTRIP BASELINE $(MapText $base)" | Write-Host
	foreach ($n in @($target) + @($mustBeOff | Where-Object { $_ })) { if (-not $base.Contains($n) -or $base[$n] -cne "Off") { "  ROUNDTRIP baseline `"$n`" is not Off" | Write-Host; return "BASELINE_NOT_OFF" } }
	if (-not (& $flip $target)) { $s = & $snap; if ($s) { "  ROUNDTRIP AFTER_FAILED_TOGGLE $(MapText $s) diff=$((ToggleDiff $base $s) -join ',')" | Write-Host }; return "TOGGLE_FAILED" }
	$s1 = & $snap
	if ($null -eq $s1) { return "STATE1_READ_FAILED" }
	$d = ToggleDiff $base $s1
	"  ROUNDTRIP STATE1 $(MapText $s1) diff=$($d -join ',')" | Write-Host
	$phase = if ($s1[$target] -ceq "On") { "TOGGLED_ON" } else { "TOGGLE_NO_EFFECT" }
	$cur = $s1
	if ($cur[$target] -cne $base[$target]) {
		if (-not (& $flip $target)) { return "$phase|RESTORE_TOGGLE_FAILED" }
		$cur = & $snap
		if ($null -eq $cur) { return "$phase|RESTORE_READ_FAILED" }
		"  ROUNDTRIP STATE2 $(MapText $cur) diff=$((ToggleDiff $base $cur) -join ',')" | Write-Host
	}
	$extra = 0
	foreach ($n in $d) {
		if ($n -ceq $target -or $cur[$n] -ceq $base[$n]) { continue }
		if ($extra -ge $maxRestore) { break }
		if (-not (& $flip $n)) { return "$phase|RESTORE_TOGGLE_FAILED" }
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
