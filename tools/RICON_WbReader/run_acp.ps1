# TOOL_ONLY, R27: read one node of von.acp with RICON_AcpGraphReaderPlugin. Steam must be running.
# Without -Read the plugin stops before loading the resource (compile check). Example:
# .\run_acp.ps1 -Name root -Read -Show children        .\run_acp.ps1 -Name n2 -Read -Node 2 -VarFrom 12
param(
	[Parameter(Mandatory)][string]$Name,
	[switch]$Read,
	[string]$Node = "",
	[ValidateSet("vars", "children")][string]$Show = "vars",
	[int]$VarFrom = 0
)
$steam = "C:\Program Files (x86)\Steam\steamapps\common"
$wb = "$steam\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe"
$gp = Join-Path $PSScriptRoot "addon.gproj"
$addons = "$steam\Arma Reforger\addons,$steam\Arma Reforger Tools\Workbench\addons"
$logsDir = "ricon-acp27-$Name-" + (Get-Date -Format "yyyyMMdd-HHmmss")
$argList = @("-gproj", "`"$gp`"", "-addonsDir", "`"$addons`"", "-noBackend", "-wbModule=ScriptEditor",
	"-plugin=RICON_AcpGraphReaderPlugin", "-riconAcpShow=$Show", "-riconAcpVar=$VarFrom", "-wbSilent",
	"-exitAfterInit", "-noThrow", "-logLevel", "normal", "-logsDir", $logsDir)
if ($Read) { $argList += "-riconAcpRead=yes" }
if ($Node) { $argList += "-riconAcpNode=$Node" }
$p = Start-Process -FilePath $wb -ArgumentList $argList -PassThru -WindowStyle Hidden
$null = $p.Handle
if (-not $p.WaitForExit(300000)) { Stop-Process -Id $p.Id -Confirm:$false; "FAIL: TIMEOUT"; exit 1 }
$code = $p.ExitCode
"exit=$code"
$log = "$env:USERPROFILE\Documents\My Games\ArmaReforgerWorkbench\logs\$logsDir\console.log"
if (-not (Test-Path $log)) { "FAIL: NO_LOG $log"; exit 1 }
"log=$log"
"sha256=" + (Get-FileHash -Algorithm SHA256 $log).Hash.ToLower()
$lines = Get-Content $log
$lines | Select-String -Pattern "RICON_ACP27|SCRIPT\s*\(E\)" | ForEach-Object { $_.Line }
if ($code -ne 0) { "FAIL: Workbench exit $code"; exit 1 }
if (-not ($lines | Select-String -SimpleMatch "[RICON_ACP27] done")) { "FAIL: plugin did not finish"; exit 1 }
"PASS"
