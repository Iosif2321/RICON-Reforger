# TOOL_ONLY: run the read-only Workbench reader plugin (see RICON_WbReaderPlugin.c). Steam must be running.
# Example: .\run.ps1 -Name ecomm -Root '$ArmaReforger:Scripts' -Ext c -Grep 'enum ECommMethod' -Context 7
param(
	[Parameter(Mandatory)][string]$Name,
	[Parameter(Mandatory)][string]$Root,
	[Parameter(Mandatory)][string]$Ext,
	[Parameter(Mandatory)][string]$Grep,
	[string]$PathFilter = "",
	[int]$Context = 3,
	[int]$MaxFiles = 10
)
$steam = "C:\Program Files (x86)\Steam\steamapps\common"
$wb = "$steam\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe"
$gp = Join-Path $PSScriptRoot "addon.gproj"
$addons = "$steam\Arma Reforger\addons,$steam\Arma Reforger Tools\Workbench\addons"
# A fresh folder per run, so an early failure can never return an older log.
$logsDir = "ricon-wbreader-$Name-" + (Get-Date -Format "yyyyMMdd-HHmmss")
$argList = @("-gproj", "`"$gp`"", "-addonsDir", "`"$addons`"", "-noBackend", "-wbModule=ScriptEditor",
	"-plugin=RICON_WbReaderPlugin", "-riconRoot=`"$Root`"", "-riconExt=$Ext", "-riconGrep=`"$Grep`"",
	"-riconContext=$Context", "-riconMaxFiles=$MaxFiles", "-wbSilent", "-exitAfterInit", "-noThrow",
	"-logLevel", "normal", "-logsDir", $logsDir)
if ($PathFilter) { $argList += "-riconPath=`"$PathFilter`"" }
$p = Start-Process -FilePath $wb -ArgumentList $argList -PassThru -WindowStyle Hidden
$null = $p.Handle
if (-not $p.WaitForExit(300000)) { Stop-Process -Id $p.Id -Confirm:$false; "FAIL: TIMEOUT"; exit 1 }
$code = $p.ExitCode
"exit=$code"
$log = "$env:USERPROFILE\Documents\My Games\ArmaReforgerWorkbench\logs\$logsDir\console.log"
if (-not (Test-Path $log)) { "FAIL: NO_LOG $log"; exit 1 }
"log=$log"
$lines = Get-Content $log
$lines | Select-String -Pattern "RICON_WbReader|\(E\)" | ForEach-Object { $_.Line }
# PASS only when Workbench exited with 0 and the plugin reached its final line.
if ($code -ne 0) { "FAIL: Workbench exit $code"; exit 1 }
if (-not ($lines | Select-String -SimpleMatch "[RICON_WbReader] done")) { "FAIL: plugin did not finish"; exit 1 }
"PASS"
exit 0
