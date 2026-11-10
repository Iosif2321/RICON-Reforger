# TOOL_ONLY: run the RICON_CoreTests fixtures inside Workbench (see RICON_TestRunnerPlugin.c). Steam must be running.
# No Play, world or server. Example: .\run.ps1 -Name lifecycle
param(
    [Parameter(Mandatory)][string]$Name,
    [ValidateSet('RICON_TestRunnerPlugin', 'RICON_TemporalIntegrationProbePlugin', 'RICON_LinkPublicationProbePlugin')]
    [string]$Plugin = 'RICON_TestRunnerPlugin',
    [switch]$DryRun
)
$steam = "C:\Program Files (x86)\Steam\steamapps\common"
$wb = "$steam\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe"
$gp = Join-Path $PSScriptRoot "addon.gproj"
$repoAddons = Join-Path (Split-Path (Split-Path $PSScriptRoot)) "addons"
$addons = "$steam\Arma Reforger\addons,$steam\Arma Reforger Tools\Workbench\addons,$repoAddons"
# A fresh folder per run, so an early failure can never return an older log.
$logsDir = "ricon-tests-$Name-" + (Get-Date -Format "yyyyMMdd-HHmmss")
$argList = @("-gproj", "`"$gp`"", "-addonsDir", "`"$addons`"", "-noBackend", "-wbModule=ScriptEditor",
	"-plugin=$Plugin", "-wbSilent", "-exitAfterInit", "-noThrow", "-logLevel", "normal",
	"-logsDir", $logsDir)
$taskTag = '[RICON_TestRunner]'
if ($Plugin -eq 'RICON_TemporalIntegrationProbePlugin') { $taskTag = '[RICON_TEMPORAL_INTEGRATION]' }
if ($Plugin -eq 'RICON_LinkPublicationProbePlugin') { $taskTag = '[RICON_LINKPUB]' }
if ($DryRun) {
    [pscustomobject]@{
        status = 'DRY_RUN_NO_WORKBENCH_STARTED'
        executable = $wb
        project = $gp
        plugin = $Plugin
        arguments = $argList
        expectedMarker = "$taskTag done: PASS"
    } | ConvertTo-Json -Depth 4
    return
}
$taskExisting = @(Get-Process -Name ArmaReforgerWorkbenchSteamDiag,ArmaReforgerWorkbenchSteam,ArmaReforgerWorkbench -ErrorAction SilentlyContinue)
if ($taskExisting.Count -gt 0) { throw 'Workbench already running; preserve the existing operator session.' }
$p = Start-Process -FilePath $wb -ArgumentList $argList -PassThru -WindowStyle Hidden
$null = $p.Handle
if (-not $p.WaitForExit(300000)) { Stop-Process -Id $p.Id -Confirm:$false; "FAIL: TIMEOUT"; exit 1 }
$code = $p.ExitCode
"exit=$code"
$log = "$env:USERPROFILE\Documents\My Games\ArmaReforgerWorkbench\logs\$logsDir\console.log"
if (-not (Test-Path $log)) { "FAIL: NO_LOG $log"; exit 1 }
"log=$log"
$lines = Get-Content $log
$lines | Select-String -Pattern "RICON_TestRunner|RICON_TEST|RICON_TEMPORAL_INTEGRATION|RICON_LINKPUB|SCRIPT.*\(E\)" | ForEach-Object { $_.Line }
# PASS only when Workbench exited with 0 and the plugin reached its final line.
if ($code -ne 0) { "FAIL: Workbench exit $code"; exit 1 }
if (-not ($lines | Select-String -SimpleMatch "$taskTag done: PASS")) { "FAIL: runner did not pass"; exit 1 }
"PASS"
exit 0
