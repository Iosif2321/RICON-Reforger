# TOOL_ONLY, R27: read one node of von.acp with RICON_AcpGraphReaderPlugin. Steam must be running.
# Without -Read the plugin stops before loading the resource; -SelfTest checks the line formatter only. Examples:
# .\run_acp.ps1 -Name selftest -SelfTest      .\run_acp.ps1 -Name root -Read -Show children
# Gate (R29, codex #3111): exit 0, exactly one "[RICON_ACP27] done", no SCRIPT (E), no tagged FAIL, at most 15
# tagged lines, and no untagged line between the first and the last tagged one (a raw line break would show there).
param(
	[Parameter(Mandatory)][string]$Name,
	[switch]$Read,
	[switch]$SelfTest,
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
if ($SelfTest) { $argList += "-riconAcpSelfTest=yes" }
if ($Read) { $argList += "-riconAcpRead=yes" }
if ($Node) { $argList += "-riconAcpNode=$Node" }
$p = Start-Process -FilePath $wb -ArgumentList $argList -PassThru -WindowStyle Hidden
$null = $p.Handle
"pid=$($p.Id)"
if (-not $p.WaitForExit(300000)) { Stop-Process -Id $p.Id -Confirm:$false; "FAIL: TIMEOUT"; exit 1 }
$code = $p.ExitCode
"exit=$code"
$log = "$env:USERPROFILE\Documents\My Games\ArmaReforgerWorkbench\logs\$logsDir\console.log"
if (-not (Test-Path $log)) { "FAIL: NO_LOG $log"; exit 1 }
"log=$log"
"sha256=" + (Get-FileHash -Algorithm SHA256 $log).Hash.ToLower()
$lines = @(Get-Content $log -Encoding UTF8)
$tagged = @()
for ($i = 0; $i -lt $lines.Count; $i++) { if ($lines[$i].Contains("[RICON_ACP27]")) { $tagged += $i } }
$tagged | ForEach-Object { $lines[$_] }
$done = @($lines | Select-String -SimpleMatch "[RICON_ACP27] done").Count
$errors = @($lines | Select-String -Pattern "SCRIPT\s*\(E\)").Count
$fails = @($lines | Select-String -Pattern "\[RICON_[^\]]+\].*FAIL").Count
$span = 0
if ($tagged.Count -gt 0) { $span = $tagged[-1] - $tagged[0] + 1 }
"exit=$code done=$done scriptErrors=$errors fails=$fails tagged=$($tagged.Count) span=$span"
$ok = ($code -eq 0) -and ($done -eq 1) -and ($errors -eq 0) -and ($fails -eq 0) -and ($tagged.Count -le 15) -and ($span -eq $tagged.Count)
if (-not $ok) { "FAIL"; exit 1 }
"PASS"
exit 0
