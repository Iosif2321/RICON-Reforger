# R29 Workbench regression (claude): each RICON probe plugin once, hidden, no world/Play/server.
$steam = "C:\Program Files (x86)\Steam\steamapps\common"
$wb = "$steam\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe"
$repo = "C:\Users\Ded\Documents\Projects\ARMA 3 Scripts\RICON-Reforger"
$gp = "$repo\tools\RICON_TestRunner\addon.gproj"
$addons = "$steam\Arma Reforger\addons,$steam\Arma Reforger Tools\Workbench\addons,$repo\addons"
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$plugins = @("RICON_TestRunnerPlugin", "RICON_L0ProbePlugin", "RICON_LinkPhysicsProbePlugin", "RICON_P526MatrixProbePlugin",
	"RICON_RouteProbePlugin", "RICON_SnapshotProbePlugin", "RICON_SensitivityProbePlugin")
foreach ($plugin in $plugins) {
	$logsDir = "ricon-r29-claude-$stamp-$plugin"
	$argList = @("-gproj", "`"$gp`"", "-addonsDir", "`"$addons`"", "-noBackend", "-wbModule=ScriptEditor",
		"-plugin=$plugin", "-wbSilent", "-exitAfterInit", "-noThrow", "-logLevel", "verbose", "-log-scr-checksum",
		"-logsDir", $logsDir)
	$p = Start-Process -FilePath $wb -ArgumentList $argList -PassThru -WindowStyle Hidden
	$null = $p.Handle
	if (-not $p.WaitForExit(600000)) { Stop-Process -Id $p.Id -Confirm:$false; "$plugin TIMEOUT $logsDir"; continue }
	"$plugin exit=$($p.ExitCode) logs=$logsDir"
}
"ALL_DONE"
