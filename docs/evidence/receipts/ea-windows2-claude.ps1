# claude, TOOL_ONLY: follow-up read-only windows 8-12 (E-von-windows-ea.md, "не достигнуто капом"), one run at a time,
# hidden, no world/Play/server, <= 15 source lines each; then one compile+self-check run of the live-probe journal.
$runner = "C:\Users\Ded\Documents\Projects\ARMA 3 Scripts\RICON-Reforger\tools\RICON_WbReader\run.ps1"
$out = Join-Path $PSScriptRoot "ea_windows"
New-Item -ItemType Directory -Force $out | Out-Null
$windows = @(
	@{Name = "ea8-activatevon-def";    Path = "SCR_VONController.c"; Grep = "void ActivateVON";   Context = 7; Line = 0},
	@{Name = "ea9-deactivatevon-def";  Path = "SCR_VONController.c"; Grep = "void DeactivateVON"; Context = 7; Line = 0},
	@{Name = "ea10-oncapture-def";     Path = "SCR_VoNComponent.c";  Grep = "void OnCapture";     Context = 7; Line = 0},
	@{Name = "ea11-togglemute-240";    Path = "SCR_VONEntryRadio.c"; Grep = "ToggleMuteEntry";    Context = 7; Line = 240},
	@{Name = "ea12-getonplayerspawned"; Path = "SCR_BaseGameMode.c"; Grep = "GetOnPlayerSpawned"; Context = 3; Line = 0}
)
foreach ($w in $windows) {
	$params = @{Name = $w.Name; Root = '$ArmaReforger:scripts'; Ext = 'c'; Grep = $w.Grep; PathFilter = $w.Path; Context = $w.Context; MaxFiles = 1}
	if ($w.Line -gt 0) { $params.Line = $w.Line }
	"=== $($w.Name)"
	& $runner @params 2>&1 | Tee-Object -FilePath (Join-Path $out "$($w.Name).txt")
}
"=== ea13 live journal probe"
$steam = "C:\Program Files (x86)\Steam\steamapps\common"
$wb = "$steam\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe"
$gp = "C:\Users\Ded\Documents\Projects\ARMA 3 Scripts\RICON-Reforger\tools\RICON_LiveProbe\addon.gproj"
$addons = "$steam\Arma Reforger\addons,$steam\Arma Reforger Tools\Workbench\addons"
$logsDir = "ricon-livejournal-claude-" + (Get-Date -Format "yyyyMMdd-HHmmss")
$argList = @("-gproj", "`"$gp`"", "-addonsDir", "`"$addons`"", "-noBackend", "-wbModule=ScriptEditor",
	"-plugin=RICON_LiveJournalProbePlugin", "-wbSilent", "-exitAfterInit", "-noThrow", "-logLevel", "verbose",
	"-log-scr-checksum", "-logsDir", $logsDir)
$p = Start-Process -FilePath $wb -ArgumentList $argList -PassThru -WindowStyle Hidden
$null = $p.Handle
if (-not $p.WaitForExit(600000)) { Stop-Process -Id $p.Id -Confirm:$false; "FAIL TIMEOUT $logsDir"; exit 1 }
$log = "$env:USERPROFILE\Documents\My Games\ArmaReforgerWorkbench\logs\$logsDir\console.log"
"exit=$($p.ExitCode)"
"log=$log"
if (-not (Test-Path $log)) { "NO_LOG"; exit 1 }
$lines = Get-Content $log
"scriptErrors=" + @($lines | Select-String -Pattern "SCRIPT\s*\(E\)").Count
$lines | Select-String -Pattern "SCRIPT\s*\(E\)|\[RICON_LIVE|RICON_Live" | ForEach-Object { $_.Line }
"=== all done"
