# claude, TOOL_ONLY: self-test of wb-run-lib-claude.ps1 without Workbench. Pure helpers plus the guarded "Yes" on an
# off-screen WPF stand-in (wb-run-fakedlg-claude.ps1) in its own process. Prints one line per case and PASS n/m.
param([string]$WorkDir = $env:TEMP)
. (Join-Path $PSScriptRoot "wb-run-lib-claude.ps1")
$script:pass = 0; $script:total = 0
function Case($name, $got, $want) {
	$script:total++
	$ok = ($got -join "|") -ceq ($want -join "|")
	if ($ok) { $script:pass++ }
	"{0} {1} got=`"{2}`" want=`"{3}`"" -f $(if ($ok) { "PASS" } else { "FAIL" }), $name, ($got -join "|"), ($want -join "|")
}

# parse and ASCII checks of every ops script (Windows PowerShell 5.1 misreads non-ASCII in BOM-less files)
foreach ($f in @(Get-ChildItem $PSScriptRoot -Filter *.ps1)) {
	$tok = $null; $err = $null
	[void][System.Management.Automation.Language.Parser]::ParseFile($f.FullName, [ref]$tok, [ref]$err)
	Case "parse $($f.Name)" @($err).Count 0
	Case "ascii $($f.Name)" @([IO.File]::ReadAllBytes($f.FullName) | Where-Object { $_ -gt 127 }).Count 0
}

Case "addr any4" (AddrClass "0.0.0.0") "ANY"
Case "addr any6" (AddrClass "::") "ANY"
Case "addr loop4" (AddrClass "127.0.0.1") "LOOPBACK"
Case "addr loop6" (AddrClass "::1") "LOOPBACK"
Case "addr 10" (AddrClass "10.1.2.3") "PRIVATE_V4"
Case "addr 172.16" (AddrClass "172.16.0.9") "PRIVATE_V4"
Case "addr 172.32" (AddrClass "172.32.0.9") "OTHER"
Case "addr 192.168" (AddrClass "192.168.50.7") "PRIVATE_V4"
Case "addr apipa" (AddrClass "169.254.3.4") "LINKLOCAL_V4"
Case "addr fe80" (AddrClass "fe80::1") "LINKLOCAL_V6"
Case "addr fd" (AddrClass "fd12:3456::1") "PRIVATE_V6"
Case "addr public" (AddrClass "8.8.8.8") "OTHER"

Case "flags typical" (FlagNames '"C:\x\a.exe" -gproj "C:\p q\a.gproj" -client 192.168.1.5 -riconEndpointProbe=1 -logsDir x -client') @("-gproj", "-client", "-riconEndpointProbe", "-logsDir")
Case "flags quoted dash" (FlagNames 'a.exe -profile "C:\a -b\c" -x') @("-profile", "-x")
Case "flags none" (FlagNames "") @()
Case "flags negative number" (FlagNames "a.exe -maxFPS -1") @("-maxFPS")

$d0 = Get-Date "2026-10-25 10:00:00"
$procs = @(
	[pscustomobject]@{ ProcessId = 100; ParentProcessId = 4; CreationDate = $d0 },
	[pscustomobject]@{ ProcessId = 101; ParentProcessId = 100; CreationDate = $d0.AddSeconds(5) },
	[pscustomobject]@{ ProcessId = 102; ParentProcessId = 101; CreationDate = $d0.AddSeconds(6) },
	[pscustomobject]@{ ProcessId = 200; ParentProcessId = 4; CreationDate = $d0 },
	[pscustomobject]@{ ProcessId = 300; ParentProcessId = 301; CreationDate = $d0 },
	[pscustomobject]@{ ProcessId = 301; ParentProcessId = 100; CreationDate = $d0.AddSeconds(9) })
Case "descendants" (@(Descendants $procs 100) | Sort-Object) @(100, 101, 102, 301)
Case "descendants missing root" (@(Descendants $procs 999)).Count 0

$msg = $script:ExpectedMessage
$four = @("Yes", "Yes to All", "No", "No to All")
Case "verdict exact" (PromptVerdict @($msg) $four) "EXACT"
Case "verdict other text" (PromptVerdict @($msg.Replace("1101", "1102")) $four) "UNEXPECTED"
Case "verdict extra text" (PromptVerdict @($msg, "more") $four) "UNEXPECTED"
Case "verdict three buttons" (PromptVerdict @($msg) @("Yes", "No", "No to All")) "UNEXPECTED"
Case "verdict lowercase yes" (PromptVerdict @($msg) @("yes", "Yes to All", "No", "No to All")) "UNEXPECTED"
Case "verdict not ready" (PromptVerdict @($msg) @()) "NOT_READY"
Case "limit plain" (YesLimit $false $false) 1
Case "limit marker without restart" (YesLimit $false $true) 1
Case "limit restart before marker" (YesLimit $true $false) 1
Case "limit restart after marker" (YesLimit $true $true) 2
Case "known auth" (WindowKnown "Script Authorization Required") $true
Case "known editor" (WindowKnown "World Editor - Plain") $true
Case "known lowercase" (WindowKnown "world editor - Plain") $false
Case "known save prompt" (WindowKnown "World needs to be saved") $false
Case "known bare editor" (WindowKnown "World Editor") $true
Case "known bare editor suffix" (WindowKnown "World Editor 2") $false

# startup title of the menu reader (codex #4647): exact, before the editor window, within 30 s
Case "startup exact early" (StartupWindowAllowed "Arma Reforger Workbench" $false 2) $true
Case "startup lowercase" (StartupWindowAllowed "arma reforger workbench" $false 2) $false
Case "startup suffix" (StartupWindowAllowed "Arma Reforger Workbench 2" $false 2) $false
Case "startup after main" (StartupWindowAllowed "Arma Reforger Workbench" $true 2) $false
Case "startup after 30 s" (StartupWindowAllowed "Arma Reforger Workbench" $false 30.5) $false
Case "startup at 30 s" (StartupWindowAllowed "Arma Reforger Workbench" $false 30) $true
Case "startup not globally known" (WindowKnown "Arma Reforger Workbench") $false
Case "startup MainWindow early" (StartupWindowAllowed "MainWindow" $false 8) $true
Case "startup mainwindow lowercase" (StartupWindowAllowed "mainwindow" $false 8) $false
Case "startup MainWindow suffix" (StartupWindowAllowed "MainWindow 2" $false 8) $false
Case "startup MainWindow after main" (StartupWindowAllowed "MainWindow" $true 8) $false
Case "startup MainWindow after 30 s" (StartupWindowAllowed "MainWindow" $false 31) $false
Case "MainWindow not globally known" (WindowKnown "MainWindow") $false

# teardown ownership (codex #4631): pure decisions, no process is killed here
$launch = Get-Date "2026-10-25 10:00:00"
function Ent($o) { $e = @{ pid = 101; descendant = $true; recordedCreated = $launch.AddSeconds(5); launch = $launch; held = $true; heldExited = $false; heldStart = $launch.AddSeconds(5) }; foreach ($k in $o.Keys) { $e[$k] = $o[$k] }; $e }
Case "teardown own child" (TeardownDecision (Ent @{})) "KILL_CHILD"
Case "teardown pid reused, other start" (TeardownDecision (Ent @{ heldStart = $launch.AddSeconds(40) })) "SKIP_REUSED"
Case "teardown no start time" (TeardownDecision (Ent @{ heldStart = $null })) "SKIP_REUSED"
Case "teardown start 1 ms apart" (TeardownDecision (Ent @{ heldStart = $launch.AddSeconds(5).AddMilliseconds(1) })) "SKIP_REUSED"
Case "teardown start 9 ticks apart" (TeardownDecision (Ent @{ heldStart = $launch.AddSeconds(5).AddTicks(9) })) "KILL_CHILD"
Case "teardown start 10 ticks apart" (TeardownDecision (Ent @{ heldStart = $launch.AddSeconds(5).AddTicks(10) })) "SKIP_REUSED"
Case "teardown start 1 tick before" (TeardownDecision (Ent @{ heldStart = $launch.AddSeconds(5).AddTicks(-1) })) "SKIP_REUSED"
Case "teardown start 100 ms apart" (TeardownDecision (Ent @{ heldStart = $launch.AddSeconds(5).AddMilliseconds(100) })) "SKIP_REUSED"
Case "same start large ticks exact" (SameStart ([datetime]::new(638900000000000009)) ([datetime]::new(638900000000000000))) $true
Case "same start large ticks next us" (SameStart ([datetime]::new(638900000000000010)) ([datetime]::new(638900000000000009))) $false
Case "teardown no handle" (TeardownDecision (Ent @{ held = $false; heldStart = $null })) "SKIP_NO_HANDLE"
Case "teardown child exited" (TeardownDecision (Ent @{ heldExited = $true })) "SKIP_EXITED"
Case "teardown not descendant" (TeardownDecision (Ent @{ descendant = $false })) "SKIP_NOT_DESCENDANT"
Case "teardown older than launch" (TeardownDecision (Ent @{ recordedCreated = $launch.AddSeconds(-30); heldStart = $launch.AddSeconds(-30) })) "SKIP_BEFORE_LAUNCH"
$plan = TeardownPlan 100 $false @()
Case "plan empty tracked keeps root" (@($plan | ForEach-Object { "$($_.pid):$($_.action)" })) @("100:KILL_ROOT")
$plan = TeardownPlan 100 $true @()
Case "plan root exited" (@($plan | ForEach-Object { "$($_.pid):$($_.action)" })) @("100:ROOT_EXITED")
$plan = TeardownPlan 100 $false @((Ent @{ pid = 102 }), (Ent @{ pid = 103; heldStart = $launch.AddSeconds(60) }))
Case "plan children first, root last" (@($plan | ForEach-Object { "$($_.pid):$($_.action)" })) @("102:KILL_CHILD", "103:SKIP_REUSED", "100:KILL_ROOT")
$self = Get-Process -Id $PID
$h = OpenHeld ([uint32]$PID) $self.StartTime
Case "open held own process" ([bool]$h) $true
if ($h) { $h.Dispose() }
Case "open held start mismatch" ([bool](OpenHeld ([uint32]$PID) $self.StartTime.AddSeconds(-10))) $false
Case "open held start 1 ms off" ([bool](OpenHeld ([uint32]$PID) $self.StartTime.AddMilliseconds(-1))) $false
$cimSelf = (Get-CimInstance Win32_Process -Filter "ProcessId=$PID" -Property CreationDate).CreationDate
$h = OpenHeld ([uint32]$PID) $cimSelf
Case "open held with CIM CreationDate" ([bool]$h) $true
if ($h) { $h.Dispose() }
Case "open held no such pid" ([bool](OpenHeld ([uint32]4294967000) $self.StartTime)) $false

# watchdog (codex #4639): kills a stand-in target after the limit; a wrong start time leaves it alone
$target = Start-Process -FilePath "powershell.exe" -ArgumentList @("-NoProfile", "-Command", "Start-Sleep -Seconds 40") -PassThru -WindowStyle Hidden
$null = $target.Handle
$wd = StartWatchdog $target 2000
Start-Sleep -Seconds 6
$target.Refresh()
Case "watchdog kills after limit" $target.HasExited $true
$wd.Refresh(); Case "watchdog ends itself" $wd.HasExited $true
$target2 = Start-Process -FilePath "powershell.exe" -ArgumentList @("-NoProfile", "-Command", "Start-Sleep -Seconds 40") -PassThru -WindowStyle Hidden
$null = $target2.Handle
$fake = [pscustomobject]@{ Id = $target2.Id; StartTime = $target2.StartTime.AddSeconds(-5) }
$ticks = $fake.StartTime.ToUniversalTime().Ticks
$cmd = "`$p = [System.Diagnostics.Process]::GetProcessById($($fake.Id)); `$null = `$p.Handle; if (`$p.StartTime.ToUniversalTime().Ticks -eq $ticks) { if (-not `$p.WaitForExit(1000)) { `$p.Kill() } }"
$wd2 = Start-Process -FilePath "powershell.exe" -ArgumentList @("-NoProfile", "-NonInteractive", "-EncodedCommand", [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($cmd))) -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 5
$target2.Refresh()
Case "watchdog start mismatch spares target" $target2.HasExited $false
if (-not $target2.HasExited) { $target2.Kill() }

# UI Automation cases on stand-in windows in a separate process
function FakeDlg($title, $message, $buttons, $tag) {
	$click = Join-Path $WorkDir "ricon-fakedlg-$tag.txt"
	if (Test-Path $click) { Remove-Item $click -Force }
	$p = Start-Process -FilePath "powershell.exe" -PassThru -WindowStyle Hidden -ArgumentList @("-NoProfile", "-STA", "-ExecutionPolicy", "Bypass",
		"-File", "`"$(Join-Path $PSScriptRoot 'wb-run-fakedlg-claude.ps1')`"", "-Title", "`"$title`"", "-Message", "`"$message`"",
		"-Buttons", "`"$buttons`"", "-ClickFile", "`"$click`"", "-Seconds", "25")
	for ($i = 0; $i -lt 60 -and [RiconWin]::FindByTitle([uint32]$p.Id, $title) -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 250 }
	Start-Sleep -Milliseconds 500
	return @{ p = $p; click = $click }
}
function Clicks($f) { Start-Sleep -Milliseconds 800; if (Test-Path $f.click) { return , @(Get-Content $f.click) } else { return , @() } }
function Done($f) { if (-not $f.p.HasExited) { Stop-Process -Id $f.p.Id -Force -ErrorAction SilentlyContinue } }
$btns = "Yes,Yes to All,No,No to All"

$script:yesPressed = 0; $script:yesHandles = @(); $script:yesAt = $null
$f = FakeDlg $script:AuthTitle $msg $btns "exact"
Case "uia exact pressed" (YesGuard ([uint32]$f.p.Id) 1 $false $false 6>$null) "PRESSED"
Case "uia exact clicked Yes" (Clicks $f) @("Yes")
Done $f
$f = FakeDlg $script:AuthTitle $msg $btns "second"
$script:yesAt = (Get-Date).AddSeconds(-10)
Case "uia second dialog refused" (YesGuard ([uint32]$f.p.Id) 2 $false $false 6>$null) "LIMIT_REACHED"
Case "uia second dialog no click" (Clicks $f).Count 0
Case "uia restart before marker refused" (YesGuard ([uint32]$f.p.Id) 3 $true $false 6>$null) "LIMIT_REACHED"
Case "uia restart after marker pressed" (YesGuard ([uint32]$f.p.Id) 4 $true $true 6>$null) "PRESSED"
Case "uia restart clicked Yes" (Clicks $f) @("Yes")
Done $f
$f = FakeDlg $script:AuthTitle $msg $btns "third"
$script:yesAt = (Get-Date).AddSeconds(-10)
Case "uia third refused" (YesGuard ([uint32]$f.p.Id) 5 $true $true 6>$null) "LIMIT_REACHED"
Case "uia third no click" (Clicks $f).Count 0
Done $f

$script:yesPressed = 0; $script:yesHandles = @(); $script:yesAt = $null
$f = FakeDlg $script:AuthTitle $msg.Replace("LoadContainer", "SaveContainer") $btns "text"
Case "uia wrong text refused" (YesGuard ([uint32]$f.p.Id) 6 $false $false 6>$null) "UNEXPECTED_DIALOG"
Case "uia wrong text no click" (Clicks $f).Count 0
Case "uia wrong text presses" $script:yesPressed 0
Done $f
$f = FakeDlg $script:AuthTitle $msg "Yes,No" "buttons"
Case "uia two buttons refused" (YesGuard ([uint32]$f.p.Id) 7 $false $false 6>$null) "UNEXPECTED_DIALOG"
Case "uia two buttons no click" (Clicks $f).Count 0
Done $f
$f = FakeDlg "Background Server" "" "OK" "child"
Case "unknown child window" (UnknownWindow ([uint32]$f.p.Id) $false) "Background Server"
Case "unknown child auth title" (UnknownWindow ([uint32]$f.p.Id) $true) "Background Server"
Done $f
$f = FakeDlg "Arma Reforger Workbench" "" "OK" "startup"
Case "startup window early passes" (UnknownWindowStartup ([uint32]$f.p.Id) $false 3) ""
Case "startup window after main stops" (UnknownWindowStartup ([uint32]$f.p.Id) $true 3) "Arma Reforger Workbench"
Case "startup window late stops" (UnknownWindowStartup ([uint32]$f.p.Id) $false 31) "Arma Reforger Workbench"
Done $f
$f = FakeDlg "MainWindow" "" "OK" "mainwindow"
Case "MainWindow window early passes" (UnknownWindowStartup ([uint32]$f.p.Id) $false 8) ""
Case "MainWindow window after main stops" (UnknownWindowStartup ([uint32]$f.p.Id) $true 8) "MainWindow"
Done $f
$f = FakeDlg "World Editor - Plain" "" "OK" "editor"
Case "known editor window" (UnknownWindow ([uint32]$f.p.Id) $true) ""
Case "startup check passes editor" (UnknownWindowStartup ([uint32]$f.p.Id) $false 3) ""
Case "editor title in child" (UnknownWindow ([uint32]$f.p.Id) $false) "World Editor - Plain"
Done $f

Case "closed collapsed" (ClosedOk ([System.Windows.Automation.ExpandCollapseState]::Collapsed)) $true
Case "closed partially" (ClosedOk ([System.Windows.Automation.ExpandCollapseState]::PartiallyExpanded)) $false
Case "closed expanded" (ClosedOk ([System.Windows.Automation.ExpandCollapseState]::Expanded)) $false
Case "closed leaf" (ClosedOk ([System.Windows.Automation.ExpandCollapseState]::LeafNode)) $false
Case "closed null" (ClosedOk $null) $false

# ExpandRead on a WPF stand-in menu (ExpandCollapse provider of WPF MenuItem): one bool back, items on the host
$mk = Join-Path $WorkDir "ricon-fakemenu-claude.ps1"
@'
Add-Type -AssemblyName PresentationFramework
$w = New-Object System.Windows.Window
$w.Title = "World Editor - Fake"; $w.Width = 300; $w.Height = 120; $w.Left = -3000; $w.Top = -3000
$w.ShowActivated = $false; $w.ShowInTaskbar = $false; $w.WindowStartupLocation = "Manual"
$menu = New-Object System.Windows.Controls.Menu
$game = New-Object System.Windows.Controls.MenuItem; $game.Header = "Game"
foreach ($h in @("Play Fake", "Server Fake")) { $i = New-Object System.Windows.Controls.MenuItem; $i.Header = $h; [void]$game.Items.Add($i) }
$opt = New-Object System.Windows.Controls.MenuItem; $opt.Header = "Play Game Options"
foreach ($h in @("Inner Server Fake", "Inner Peer Fake")) { $i = New-Object System.Windows.Controls.MenuItem; $i.Header = $h; $i.IsCheckable = $true; $i.IsChecked = ($h -eq "Inner Peer Fake"); [void]$opt.Items.Add($i) }
[void]$game.Items.Add($opt)
[void]$menu.Items.Add($game)
$w.Content = $menu
$t = New-Object System.Windows.Threading.DispatcherTimer; $t.Interval = [TimeSpan]::FromSeconds(25); $t.Add_Tick({ $w.Close() }.GetNewClosure()); $t.Start()
[void]$w.ShowDialog()
'@ | Set-Content -Path $mk -Encoding ASCII
$mp = Start-Process -FilePath "powershell.exe" -PassThru -WindowStyle Hidden -ArgumentList @("-NoProfile", "-STA", "-ExecutionPolicy", "Bypass", "-File", "`"$mk`"")
for ($i = 0; $i -lt 60 -and [RiconWin]::FindByTitle([uint32]$mp.Id, "World Editor - Fake") -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 250 }
Start-Sleep -Milliseconds 500
$mh = [RiconWin]::FindByTitle([uint32]$mp.Id, "World Editor - Fake")
$gameItem = @(OfType ($U::FromHandle($mh)) $CT::MenuItem | Where-Object { $_.Current.Name -eq "Game" })
Case "fake menu has Game" $gameItem.Count 1
if ($gameItem.Count -eq 1) {
	$printed = @(ExpandRead $gameItem[0] "MENU[0]" ([uint32]$mp.Id) $mh 6>&1 | ForEach-Object { [string]$_ })
	$ret = @(ExpandRead $gameItem[0] "MENU[0]" ([uint32]$mp.Id) $mh 6>$null)
	Case "expandread returns one bool" (@($ret | ForEach-Object { $_.GetType().Name + ":" + $_ })) @("Boolean:True")
	Case "expandread prints items" (@($printed | Where-Object { $_ -match 'ITEM "\[(el|win[^]]*)\] (Play Fake|Server Fake)' }).Count) 2
	Case "expandread no inner items without child" (@($printed | Where-Object { $_ -match 'Inner (Server|Peer) Fake' }).Count) 0
	Case "expandread closed again" (@($printed | Where-Object { $_ -match 'CLOSED state=Collapsed' }).Count) 1
	Case "expandread state after" ($gameItem[0].GetCurrentPattern($ECP::Pattern).Current.ExpandCollapseState.ToString()) "Collapsed"
	$printed2 = @(ExpandRead $gameItem[0] "MENU[0]" ([uint32]$mp.Id) $mh "Play Game Options" 6>&1 | ForEach-Object { [string]$_ })
	$ret2 = @(ExpandRead $gameItem[0] "MENU[0]" ([uint32]$mp.Id) $mh "Play Game Options" 6>$null)
	Case "child returns one bool" (@($ret2 | ForEach-Object { $_.GetType().Name + ":" + $_ })) @("Boolean:True")
	Case "child items printed" (@($printed2 | Where-Object { $_ -match 'ITEM "\[(el|win[^]]*)\] Inner (Server|Peer) Fake' }).Count -ge 2) $true
	Case "child toggle state read" (@($printed2 | Where-Object { $_ -match 'Inner Peer Fake enabled=True.* toggle=On' }).Count -ge 1) $true
	Case "child toggle off read" (@($printed2 | Where-Object { $_ -match 'Inner Server Fake enabled=True.* toggle=Off' }).Count -ge 1) $true
	Case "child closed then parent closed" (@($printed2 | Where-Object { $_ -match 'CLOSED state=Collapsed' }).Count) 2
	Case "child missing reported" (@(ExpandRead $gameItem[0] "MENU[0]" ([uint32]$mp.Id) $mh "No Such Item" 6>&1 | ForEach-Object { [string]$_ } | Where-Object { $_ -match 'CHILD_NOT_FOUND' }).Count) 1
	Case "parent state after child run" ($gameItem[0].GetCurrentPattern($ECP::Pattern).Current.ExpandCollapseState.ToString()) "Collapsed"
}
if (-not $mp.HasExited) { $mp.Kill() }
Remove-Item $mk -Force

# RoundTrip on a WPF stand-in Options menu (codex #4668): exclusive pair Viewport/Server, PeerTool independent
function FakeOptions($exclusive, $serverOn, $tag, $peerOn = $false) {
	$mk = Join-Path $WorkDir "ricon-fakeopts-$tag.ps1"
	$ex = if ($exclusive) { '$true' } else { '$false' }
	$so = if ($serverOn) { '$true' } else { '$false' }
	$po = if ($peerOn) { '$true' } else { '$false' }
	@"
Add-Type -AssemblyName PresentationFramework
`$w = New-Object System.Windows.Window
`$w.Title = "World Editor - Fake $tag"; `$w.Width = 300; `$w.Height = 120; `$w.Left = -3000; `$w.Top = -3000
`$w.ShowActivated = `$false; `$w.ShowInTaskbar = `$false; `$w.WindowStartupLocation = "Manual"
`$menu = New-Object System.Windows.Controls.Menu
`$game = New-Object System.Windows.Controls.MenuItem; `$game.Header = "Game"
`$opt = New-Object System.Windows.Controls.MenuItem; `$opt.Header = "Play Game Options"
`$vp = New-Object System.Windows.Controls.MenuItem; `$vp.Header = "Play inside Viewport"; `$vp.IsCheckable = `$true; `$vp.IsChecked = -not $so
`$sv = New-Object System.Windows.Controls.MenuItem; `$sv.Header = "Server localhost"; `$sv.IsCheckable = `$true; `$sv.IsChecked = $so
`$pt = New-Object System.Windows.Controls.MenuItem; `$pt.Header = "Server localhost + PeerTool"; `$pt.IsCheckable = `$true; `$pt.IsChecked = $po
if ($ex) { `$sv.Add_Checked({ `$vp.IsChecked = `$false }.GetNewClosure()); `$vp.Add_Checked({ `$sv.IsChecked = `$false }.GetNewClosure()) }
foreach (`$i in @(`$vp, `$sv, `$pt)) { [void]`$opt.Items.Add(`$i) }
[void]`$game.Items.Add(`$opt); [void]`$menu.Items.Add(`$game)
`$w.Content = `$menu
`$t = New-Object System.Windows.Threading.DispatcherTimer; `$t.Interval = [TimeSpan]::FromSeconds(40); `$t.Add_Tick({ `$w.Close() }.GetNewClosure()); `$t.Start()
[void]`$w.ShowDialog()
"@ | Set-Content -Path $mk -Encoding ASCII
	$p = Start-Process -FilePath "powershell.exe" -PassThru -WindowStyle Hidden -ArgumentList @("-NoProfile", "-STA", "-ExecutionPolicy", "Bypass", "-File", "`"$mk`"")
	$title = "World Editor - Fake $tag"
	for ($i = 0; $i -lt 60 -and [RiconWin]::FindByTitle([uint32]$p.Id, $title) -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 250 }
	Start-Sleep -Milliseconds 500
	return @{ p = $p; h = [RiconWin]::FindByTitle([uint32]$p.Id, $title); mk = $mk }
}
function DoneOpts($f) { if (-not $f.p.HasExited) { $f.p.Kill() }; Remove-Item $f.mk -Force -ErrorAction SilentlyContinue }
$a = [ordered]@{ "x" = "On"; "y" = "Off" }; $b2 = [ordered]@{ "x" = "Off"; "y" = "Off"; "z" = "Off" }
Case "toggle diff" (ToggleDiff $a $b2) @("x", "z")
Case "toggle diff equal" (ToggleDiff $a $a).Count 0
$fo = FakeOptions $true $false "excl"
$rt = RoundTrip $fo.h ([uint32]$fo.p.Id) "Server localhost" { "" } @("Server localhost + PeerTool") 6>$null
Case "roundtrip exclusive result" $rt "TOGGLED_ON|RESTORE_OK"
Case "roundtrip exclusive toggles" $script:roundTripToggles 3
$h = OpenOptions $fo.h ([uint32]$fo.p.Id); $m = ReadToggles $h.opt; $null = CloseOptions $h $fo.h
Case "roundtrip exclusive final" (MapText $m) "Play inside Viewport=On | Server localhost=Off | Server localhost + PeerTool=Off"
DoneOpts $fo
$fo = FakeOptions $false $false "indep"
$rt = RoundTrip $fo.h ([uint32]$fo.p.Id) "Server localhost" { "" } @("Server localhost + PeerTool") 6>$null
Case "roundtrip independent result" $rt "TOGGLED_ON|RESTORE_OK"
Case "roundtrip independent toggles" $script:roundTripToggles 2
DoneOpts $fo
$fo = FakeOptions $false $true "on"
$rt = RoundTrip $fo.h ([uint32]$fo.p.Id) "Server localhost" { "" } @("Server localhost + PeerTool") 6>$null
Case "roundtrip baseline on refused" $rt "BASELINE_NOT_OFF"
Case "roundtrip baseline on no toggle" $script:roundTripToggles 0
DoneOpts $fo
$fo = FakeOptions $false $false "peeron" $true
$rt = RoundTrip $fo.h ([uint32]$fo.p.Id) "Server localhost" { "" } @("Server localhost + PeerTool") 6>$null
Case "roundtrip peertool on refused" $rt "BASELINE_NOT_OFF"
Case "roundtrip peertool on no toggle" $script:roundTripToggles 0
DoneOpts $fo
$fo = FakeOptions $false $false "blocked"
$rt = RoundTrip $fo.h ([uint32]$fo.p.Id) "Server localhost" { "DEADLINE" } @("Server localhost + PeerTool") 6>$null
Case "roundtrip blocked before first toggle" $rt "TOGGLE_FAILED"
Case "roundtrip blocked no toggle" $script:roundTripToggles 0
DoneOpts $fo

"PASS $($script:pass)/$($script:total)"
