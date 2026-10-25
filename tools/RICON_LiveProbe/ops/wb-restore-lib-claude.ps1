# claude, TOOL_ONLY: contingency for B0b step 2 (codex #4690) - put Server localhost back from an observed On when a
# server-play run could not. Kept out of wb-run-lib-claude.ps1 so the library reviewed for that run keeps its hash.
# Needs wb-run-lib-claude.ps1 dot-sourced first (StrictSnapshot, OpenOptions, ToggleExpected, CloseOptions, ToggleDiff).

# Contingency (codex #4690): put the target back from On when an earlier run could not. The strict snapshot must show the
# target exactly On and every $mustBeOff item Off; one Invoke from the fresh On check; the final strict snapshot must
# differ from the first exactly by the target, now Off. Returns RESTORED_OFF, NOT_ON (nothing done), or a failure.
function RestoreOff([IntPtr]$mainHandle, [uint32]$ownPid, [string]$target, [scriptblock]$blocked, [string[]]$mustBeOff) {
	$script:restoreActions = 0
	$required = @($target) + @($mustBeOff | Where-Object { $_ })
	$first = StrictSnapshot $mainHandle $ownPid $null $required
	if ($null -eq $first) { return "READ_FAILED" }
	"  RESTORE_OFF BEFORE $(MapText $first)" | Write-Host
	foreach ($n in @($mustBeOff | Where-Object { $_ })) { if ($first[$n] -cne "Off") { return "OTHER_NOT_OFF" } }
	if ($first[$target] -ceq "Off") { return "NOT_ON" }
	if ($first[$target] -cne "On") { return "TARGET_UNKNOWN" }
	$b = & $blocked
	if ($b) { return "BLOCKED_$b" }
	# not opened means nothing was acted on, so up to 3 attempts (self-test under load: OPTIONS_NOT_OPENED once)
	$h = $null
	for ($try = 1; $try -le 3 -and $null -eq $h; $try++) { $h = OpenOptions $mainHandle $ownPid; if ($null -eq $h) { Start-Sleep -Milliseconds 700 } }
	if ($null -eq $h) { return "OPTIONS_NOT_OPENED" }
	$res = ToggleExpected $h.opt $target "On" "Invoke"
	if ($res -ceq "TOGGLED") { $script:restoreActions++ }
	"  RESTORE_OFF INVOKE `"$target`" from=On result=$res" | Write-Host
	Start-Sleep -Milliseconds 700
	$closed = CloseOptions $h $mainHandle
	if ($res -cne "TOGGLED" -or -not $closed) { return "INVOKE_$res" }
	$b = & $blocked
	if ($b) { return "BLOCKED_AFTER_$b" }
	$fin = StrictSnapshot $mainHandle $ownPid @($first.Keys) $required
	if ($null -eq $fin) { return "FINAL_READ_FAILED" }
	$d = ToggleDiff $first $fin
	"  RESTORE_OFF AFTER $(MapText $fin) diff=$($d -join ',')" | Write-Host
	if ($d.Count -eq 1 -and $d[0] -ceq $target -and $fin[$target] -ceq "Off") { return "RESTORED_OFF" }
	return "RESTORE_UNEXPECTED"
}
