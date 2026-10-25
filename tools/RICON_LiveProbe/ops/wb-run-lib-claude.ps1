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

# Exact checks for the vanilla prompt (SCR_ArsenalManagerComponent.c:1101), unchanged from the B0a runner.
$script:ExpectedSite = "Scripts/Game/GameMode/Loadout/SCR_ArsenalManagerComponent.c:1101"
$script:ExpectedMethod = "BaseContainerTools.LoadContainer"
$script:ExpectedMessage = "Allow script '" + $script:ExpectedSite + "' to access potentially dangerous method '" + $script:ExpectedMethod + "'?"
$script:AuthTitle = "Script Authorization Required"
$script:KnownTitles = @("Script Authorization Required")
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
