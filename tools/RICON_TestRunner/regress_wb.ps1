# R29 Workbench regression (claude): each RICON probe plugin once, hidden, no world/Play/server.
# Gate (added after the receipted run, codex #2859): a probe passes only with exit 0, exactly one "done: PASS",
# no "SCRIPT (E)" and no tagged FAIL line ("[RICON_TEST] FAIL", "done: FAIL", "suite x: FAIL", codex #2867);
# the script exits 1 if any probe fails.
$steam = "C:\Program Files (x86)\Steam\steamapps\common"
$wb = "$steam\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe"
$repo = Split-Path (Split-Path $PSScriptRoot)
$gp = Join-Path $PSScriptRoot "addon.gproj"
$addons = "$steam\Arma Reforger\addons,$steam\Arma Reforger Tools\Workbench\addons,$repo\addons"
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$plugins = @("RICON_TestRunnerPlugin", "RICON_L0ProbePlugin", "RICON_LinkPhysicsProbePlugin", "RICON_P526MatrixProbePlugin",
	"RICON_RouteProbePlugin", "RICON_SnapshotProbePlugin", "RICON_SensitivityProbePlugin", "RICON_WorldProfileProbePlugin",
	"RICON_RadioDeviceProbePlugin", "RICON_PairServiceProbePlugin", "RICON_LinkPublicationProbePlugin",
	"RICON_ChannelCutoffProbePlugin")
# FAIL и суффикс _FAIL — ошибки тестов; значение NUMERIC_FAILURE ошибкой теста не является.
$taggedFailPattern = '\[RICON_[^\]]+\].*(?<![A-Za-z])FAIL(?![A-Za-z])'
$failed = 0
foreach ($plugin in $plugins) {
	$logsDir = "ricon-r29-claude-$stamp-$plugin"
	$argList = @("-gproj", "`"$gp`"", "-addonsDir", "`"$addons`"", "-noBackend", "-wbModule=ScriptEditor",
		"-plugin=$plugin", "-wbSilent", "-exitAfterInit", "-noThrow", "-logLevel", "verbose", "-log-scr-checksum",
		"-logsDir", $logsDir)
	$p = Start-Process -FilePath $wb -ArgumentList $argList -PassThru -WindowStyle Hidden
	$null = $p.Handle
	if (-not $p.WaitForExit(600000)) {
		Stop-Process -Id $p.Id -Confirm:$false
		"FAIL $plugin TIMEOUT $logsDir"
		$failed++
		continue
	}
	$log = "$env:USERPROFILE\Documents\My Games\ArmaReforgerWorkbench\logs\$logsDir\console.log"
	if (-not (Test-Path $log)) { "FAIL $plugin NO_LOG $logsDir"; $failed++; continue }
	$lines = Get-Content $log
	$done = @($lines | Select-String -SimpleMatch "done: PASS").Count
	$errors = @($lines | Select-String -Pattern "SCRIPT\s*\(E\)").Count
	$fails = @($lines | Select-String -Pattern $taggedFailPattern).Count  # [RICON_TEST] FAIL, done: FAIL, suite: FAIL
	$ok = ($p.ExitCode -eq 0) -and ($done -eq 1) -and ($errors -eq 0) -and ($fails -eq 0)
	if (-not $ok) { $failed++ }
	"{0} {1} exit={2} donePass={3} scriptErrors={4} fails={5} logs={6}" -f $(if ($ok) { "PASS" } else { "FAIL" }),
		$plugin, $p.ExitCode, $done, $errors, $fails, $logsDir
}
"TOTAL $($plugins.Count - $failed)/$($plugins.Count) PASS"
if ($failed -gt 0) { exit 1 }
exit 0
