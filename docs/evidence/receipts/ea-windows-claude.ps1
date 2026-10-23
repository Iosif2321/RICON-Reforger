# claude, TOOL_ONLY: the seven read-only WbReader windows ordered by codex (#3939), one Workbench run at a time,
# hidden, no world/Play/server. Each run is capped by the plugin at <= 15 source lines. Output is tee'd per window.
$runner = "C:\Users\Ded\Documents\Projects\ARMA 3 Scripts\RICON-Reforger\tools\RICON_WbReader\run.ps1"
$out = Join-Path $PSScriptRoot "ea_windows"
New-Item -ItemType Directory -Force $out | Out-Null
$windows = @(
	@{Name = "ea1-activatevon";     Path = "SCR_VONController.c"; Grep = "ActivateVON";       Context = 6; Line = 0},
	@{Name = "ea2-deactivatevon";   Path = "SCR_VONController.c"; Grep = "DeactivateVON";     Context = 6; Line = 0},
	@{Name = "ea3-vonctrl-622";     Path = "SCR_VONController.c"; Grep = "SetActiveTransmit"; Context = 6; Line = 622},
	@{Name = "ea4-onreceive-78";    Path = "SCR_VoNComponent.c";  Grep = "OnReceive";         Context = 6; Line = 78},
	@{Name = "ea5-oncapture";       Path = "SCR_VoNComponent.c";  Grep = "OnCapture";         Context = 6; Line = 0},
	@{Name = "ea6-onplayerspawned"; Path = "SCR_BaseGameMode.c";  Grep = "OnPlayerSpawned";   Context = 5; Line = 0},
	@{Name = "ea7-togglemute";      Path = "SCR_VONEntryRadio.c"; Grep = "ToggleMute";        Context = 5; Line = 0}
)
foreach ($w in $windows) {
	$params = @{Name = $w.Name; Root = '$ArmaReforger:scripts'; Ext = 'c'; Grep = $w.Grep; PathFilter = $w.Path; Context = $w.Context; MaxFiles = 1}
	if ($w.Line -gt 0) { $params.Line = $w.Line }
	"=== $($w.Name)"
	& $runner @params 2>&1 | Tee-Object -FilePath (Join-Path $out "$($w.Name).txt")
}
"=== all windows done"
