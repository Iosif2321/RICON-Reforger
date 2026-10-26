# claude, TOOL_ONLY: self-test of wb-run-lib-claude.ps1 without Workbench. Pure helpers plus the guarded "Yes" on an
# off-screen WPF stand-in (wb-run-fakedlg-claude.ps1) in its own process. Prints one line per case and PASS n/m.
param([string]$WorkDir = $env:TEMP)
. (Join-Path $PSScriptRoot "wb-run-lib-claude.ps1")
. (Join-Path $PSScriptRoot "wb-restore-lib-claude.ps1")
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
# B0c host address (codex #4899): one PRIVATE_V4 Preferred Dhcp on an Up adapter with a gateway, else no client
function A($ip, $if, $st = "Preferred", $po = "Dhcp") { [pscustomobject]@{ IPAddress = $ip; InterfaceIndex = $if; AddressState = $st; PrefixOrigin = $po } }
$lan = A "192.168.1.20" 20
$set = @((A "127.0.0.1" 1 "Preferred" "WellKnown"), (A "169.254.9.9" 12 "Tentative" "WellKnown"), (A "100.70.1.2" 19 "Preferred" "Manual"),
	(A "192.168.56.1" 34 "Preferred" "Manual"), (A "10.0.75.1" 2 "Preferred" "Manual"), $lan, (A "172.20.0.1" 4 "Preferred" "Manual"))
$r = SelectHostAddress $set @(19, 20)
Case "host addr like this PC" @($r.result, $r.count, $r.address, $r.ifIndex) @("OK", "1", "192.168.1.20", "20")
Case "host addr no gateway if" (SelectHostAddress $set @(19)).result "NONE"
Case "host addr manual only" (SelectHostAddress @((A "192.168.1.20" 20 "Preferred" "Manual")) @(20)).result "NONE"
Case "host addr tentative" (SelectHostAddress @((A "192.168.1.20" 20 "Tentative")) @(20)).result "NONE"
Case "host addr not private" (SelectHostAddress @((A "100.70.1.2" 19)) @(19)).result "NONE"
$two = SelectHostAddress @($lan, (A "10.1.1.5" 21)) @(20, 21)
Case "host addr two dhcp" @($two.result, $two.count) @("AMBIGUOUS", "2")
Case "host addr empty" (SelectHostAddress @() @()).result "NONE"
Case "host addr recheck same" (ClientAddressRecheck $r (SelectHostAddress $set @(19, 20))) ""
Case "host addr recheck changed" (ClientAddressRecheck $r @{ result = "OK"; address = "192.168.1.21" }) "ADDRESS_CHANGED"
Case "host addr recheck ambiguous" (ClientAddressRecheck $r $two) "ADDRESS_AMBIGUOUS"
# B0c client start (codex #4922): only in the current Play, with UDP 2001 of the host in the current snapshot
$lp = "[RICON_LIVEB] inspect=PLAY_ENTERED`n[RICON_NET] cycle=1 rplMode=2 rplName=Listen listenPresent=1`n"
$ep = @(@{ proto = "UDP"; addr = "0.0.0.0"; port = 2001; pid = [uint32]500 })
Case "client start ready" (ClientStartBlock $true $lp $ep 500) ""
Case "client start not played" (ClientStartBlock $false $lp $ep 500) "NOT_PLAYED"
Case "client start no play entered" (ClientStartBlock $true "[RICON_NET] cycle=1 rplMode=2 rplName=Listen listenPresent=1`n" $ep 500) "NO_PLAY_ENTERED"
Case "client start no listen" (ClientStartBlock $true "[RICON_LIVEB] inspect=PLAY_ENTERED`n" $ep 500) "NO_LISTEN"
Case "client start after observation" (ClientStartBlock $true ($lp + "[RICON_LIVEB] inspect=OBSERVATION_END samples=120`n") $ep 500) "OBSERVATION_OVER"
Case "client start after edit stable" (ClientStartBlock $true ($lp + "[RICON_LIVEB] inspect=EDIT_STABLE checks=51`n") $ep 500) "OBSERVATION_OVER"
Case "client start no udp now" (ClientStartBlock $true $lp @() 500) "NO_UDP_2001"
Case "client start udp other pid" (ClientStartBlock $true $lp $ep 501) "NO_UDP_2001"
Case "client start tcp only" (ClientStartBlock $true $lp @(@{ proto = "TCP_LISTEN"; addr = "0.0.0.0"; port = 2001; pid = [uint32]500 }) 500) "NO_UDP_2001"
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
# per-site texts (codex #4813): only the vanilla site is real now; two test-only sites keep the per-site rules covered
Case "site table only 1101" (@($script:SiteMessages.Keys) -join ",") "ARSENAL_1101"
$m174 = "Allow script 'Scripts/WorkbenchGame/RICON_SelfTest.c:174' to access potentially dangerous method 'FileIO.FileExists'?"
$m187 = $m174.Replace(":174", ":187")
$script:SiteMessages["TEST_174"] = $m174
$script:SiteMessages["TEST_187"] = $m187
Case "site 1101" (PromptSite @($msg) $four) "ARSENAL_1101"
Case "site 174" (PromptSite @($m174) $four) "TEST_174"
Case "site 187" (PromptSite @($m187) $four) "TEST_187"
Case "site other line" (PromptSite @($m174.Replace(":174", ":175")) $four) "UNEXPECTED"
Case "site other method" (PromptSite @($m174.Replace("FileExists", "OpenFile")) $four) "UNEXPECTED"
Case "site other script" (PromptSite @($m174.Replace("RICON_SelfTest", "RICON_Other")) $four) "UNEXPECTED"
Case "site extra text" (PromptSite @($m174, "more") $four) "UNEXPECTED"
Case "site three buttons" (PromptSite @($m174) @("Yes", "No", "No to All")) "UNEXPECTED"
Case "site not ready" (PromptSite @($m174) @()) "NOT_READY"
# phase of a plugin-start run (codex #4817, #4819, #4850)
$lSeen = "[RICON_LIVEB] inspect=WAIT_PLUGIN`n[RICON_LIVEB] inspect=PLUGIN_SEEN`n"
Case "allowed before play" (SitesAllowed $false "") @()
Case "allowed before play, seen in log" (SitesAllowed $false $lSeen) @()
Case "allowed played, no PLUGIN_SEEN" (SitesAllowed $true "[RICON_LIVEB] inspect=WAIT_PLUGIN`n") @()
Case "allowed played, old FLAG_SEEN only" (SitesAllowed $true "[RICON_LIVEB] inspect=FLAG_SEEN`n") @()
Case "allowed played, PLUGIN_SEEN" (SitesAllowed $true $lSeen) @("ARSENAL_1101")
Case "allowed after OBSERVATION_END" (SitesAllowed $true ($lSeen + "[RICON_LIVEB] inspect=OBSERVATION_END samples=3`n")) @()
Case "allowed after EDIT_REQUESTED" (SitesAllowed $true ($lSeen + "[RICON_LIVEB] inspect=EDIT_REQUESTED`n")) @()
Case "allowed after EDIT_STABLE" (SitesAllowed $true ($lSeen + "[RICON_LIVEB] inspect=EDIT_STABLE checks=51`n")) @()
# B0c (owner 2026-10-26, after run 144401): a second :1101 only once the host logged creating the test client's player
$pj = '(?m)^\d\d:\d\d:\d\d\.\d{3}[ \t]+NETWORK[ \t]+: ### Creating player: PlayerId=2, Name=RICON_T\r?$'
$joinLine = "14:45:06.978   NETWORK      : ### Creating player: PlayerId=2, Name=RICON_T`r`n"
$lJoin = $lSeen + $joinLine
Case "allowed peer joined" (SitesAllowed $true $lJoin $pj) @("ARSENAL_1101", "ARSENAL_1101")
Case "allowed peer not joined yet" (SitesAllowed $true $lSeen $pj) @("ARSENAL_1101")
Case "allowed peer line without -Client" (SitesAllowed $true $lJoin "") @("ARSENAL_1101")
Case "allowed peer line before PLUGIN_SEEN" (SitesAllowed $true $joinLine $pj) @()
Case "allowed peer line not played" (SitesAllowed $false $lJoin $pj) @()
Case "allowed peer joined after OBSERVATION_END" (SitesAllowed $true ($lJoin + "[RICON_LIVEB] inspect=OBSERVATION_END samples=3`n") $pj) @()
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

# YesGuardSites: one press per allowed site, a repeat or a site outside the phase stops, 3 in all
$script:yesPressed = 0; $script:yesHandles = @(); $script:yesAt = $null; $script:sitePressed = @{}
$pre = @("TEST_174", "TEST_187")
$f = FakeDlg $script:AuthTitle $m174 $btns "s174"
Case "sites 174 pressed" (YesGuardSites ([uint32]$f.p.Id) 1 $pre 3 6>$null) "PRESSED"
Case "sites 174 clicked Yes" (Clicks $f) @("Yes")
Done $f
$f = FakeDlg $script:AuthTitle $m174 $btns "s174b"
Case "sites 174 again refused" (YesGuardSites ([uint32]$f.p.Id) 2 $pre 3 6>$null) "SITE_REPEAT"
Case "sites 174 again no click" (Clicks $f).Count 0
Done $f
$f = FakeDlg $script:AuthTitle $msg $btns "s1101pre"
Case "sites 1101 before play refused" (YesGuardSites ([uint32]$f.p.Id) 3 $pre 3 6>$null) "SITE_NOT_ALLOWED"
Case "sites 1101 before play no click" (Clicks $f).Count 0
Done $f
$f = FakeDlg $script:AuthTitle $m187 $btns "s187"
Case "sites 187 pressed" (YesGuardSites ([uint32]$f.p.Id) 4 $pre 3 6>$null) "PRESSED"
Case "sites 187 clicked Yes" (Clicks $f) @("Yes")
Done $f
$f = FakeDlg $script:AuthTitle $m187 $btns "s187play"
Case "sites 187 after play refused" (YesGuardSites ([uint32]$f.p.Id) 5 @("ARSENAL_1101") 3 6>$null) "SITE_REPEAT"
Done $f
$f = FakeDlg $script:AuthTitle $msg $btns "s1101"
Case "sites 1101 after play pressed" (YesGuardSites ([uint32]$f.p.Id) 6 @("ARSENAL_1101") 3 6>$null) "PRESSED"
Case "sites 1101 clicked Yes" (Clicks $f) @("Yes")
Case "sites presses" $script:yesPressed 3
Done $f
$script:sitePressed = @{}
$f = FakeDlg $script:AuthTitle $m174 $btns "s4th"
Case "sites fourth refused" (YesGuardSites ([uint32]$f.p.Id) 7 $pre 3 6>$null) "LIMIT_REACHED"
Case "sites fourth no click" (Clicks $f).Count 0
Done $f
$script:yesPressed = 0; $script:yesHandles = @(); $script:yesAt = $null; $script:sitePressed = @{}
$f = FakeDlg $script:AuthTitle $m174.Replace(":174", ":175") $btns "sline"
Case "sites other line refused" (YesGuardSites ([uint32]$f.p.Id) 8 $pre 3 6>$null) "UNEXPECTED_DIALOG"
Case "sites other line no click" (Clicks $f).Count 0
Done $f
$f = FakeDlg $script:AuthTitle $m174 $btns "snone"
Case "sites none allowed refused" (YesGuardSites ([uint32]$f.p.Id) 9 @() 3 6>$null) "SITE_NOT_ALLOWED"
Case "sites none allowed no click" (Clicks $f).Count 0
Case "sites none allowed presses" $script:yesPressed 0
Done $f
# a handle reused by a prompt of another site (codex #4819), simulated by recording 174 as pressed on this window
$f = FakeDlg $script:AuthTitle $m187 $btns "sreuse"
$hw = [RiconWin]::FindByTitle([uint32]$f.p.Id, $script:AuthTitle)
$script:yesPressed = 1; $script:sitePressed = @{ "TEST_174" = @{ h = $hw; at = Get-Date } }
Case "sites reused handle other site pressed" (YesGuardSites ([uint32]$f.p.Id) 10 $pre 3 6>$null) "PRESSED"
Case "sites reused handle clicked Yes" (Clicks $f) @("Yes")
Done $f
$f = FakeDlg $script:AuthTitle $m174 $btns "sgrace"
$hw = [RiconWin]::FindByTitle([uint32]$f.p.Id, $script:AuthTitle)
$script:yesPressed = 1; $script:sitePressed = @{ "TEST_174" = @{ h = $hw; at = Get-Date } }
Case "sites same site same handle in grace" (YesGuardSites ([uint32]$f.p.Id) 11 $pre 3 6>$null) "CLOSING"
$script:sitePressed["TEST_174"].at = (Get-Date).AddSeconds(-10)
Case "sites same site same handle after grace" (YesGuardSites ([uint32]$f.p.Id) 12 $pre 3 6>$null) "PRESS_NO_EFFECT"
Case "sites same handle no click" (Clicks $f).Count 0
Case "sites same handle presses" $script:yesPressed 1
Done $f
# the plugin-start run (codex #4850): no site before the Invoke, then :1101 once, total 1
$script:yesPressed = 0; $script:yesHandles = @(); $script:yesAt = $null; $script:sitePressed = @{}
$f = FakeDlg $script:AuthTitle $msg $btns "p1101pre"
Case "plugin 1101 before invoke refused" (YesGuardSites ([uint32]$f.p.Id) 1 (SitesAllowed $false $lSeen) 1 6>$null) "SITE_NOT_ALLOWED"
Case "plugin 1101 before invoke no click" (Clicks $f).Count 0
Done $f
$f = FakeDlg $script:AuthTitle $msg $btns "p1101"
Case "plugin 1101 after PLUGIN_SEEN pressed" (YesGuardSites ([uint32]$f.p.Id) 2 (SitesAllowed $true $lSeen) 1 6>$null) "PRESSED"
Case "plugin 1101 clicked Yes" (Clicks $f) @("Yes")
Done $f
$f = FakeDlg $script:AuthTitle $msg $btns "p1101b"
Case "plugin 1101 second window refused" (YesGuardSites ([uint32]$f.p.Id) 3 (SitesAllowed $true $lSeen) 1 6>$null) "SITE_REPEAT"
Case "plugin 1101 second window no click" (Clicks $f).Count 0
Done $f
$f = FakeDlg $script:AuthTitle $m174 $btns "p174"
Case "plugin other site after the one press" (YesGuardSites ([uint32]$f.p.Id) 4 @("ARSENAL_1101", "TEST_174") 1 6>$null) "LIMIT_REACHED"
Case "plugin presses" $script:yesPressed 1
Done $f
# B0c with -Client: total 2; the second window of :1101 waits for the host's PlayerId=2 line, a third one stops
$script:yesPressed = 0; $script:yesHandles = @(); $script:yesAt = $null; $script:sitePressed = @{}
$f = FakeDlg $script:AuthTitle $msg $btns "c1101"
Case "client 1101 first pressed" (YesGuardSites ([uint32]$f.p.Id) 1 (SitesAllowed $true $lSeen $pj) 2 6>$null) "PRESSED"
Case "client 1101 first clicked Yes" (Clicks $f) @("Yes")
Done $f
$f = FakeDlg $script:AuthTitle $msg $btns "c1101early"
Case "client 1101 second before join refused" (YesGuardSites ([uint32]$f.p.Id) 2 (SitesAllowed $true $lSeen $pj) 2 6>$null) "SITE_REPEAT"
Case "client 1101 second before join no click" (Clicks $f).Count 0
Done $f
$f = FakeDlg $script:AuthTitle $msg $btns "c1101join"
Case "client 1101 second after join pressed" (YesGuardSites ([uint32]$f.p.Id) 3 (SitesAllowed $true $lJoin $pj) 2 6>$null) "PRESSED"
Case "client 1101 second clicked Yes" (Clicks $f) @("Yes")
Case "client 1101 site count" $script:sitePressed["ARSENAL_1101"].n 2
Done $f
$f = FakeDlg $script:AuthTitle $msg $btns "c1101third"
Case "client 1101 third refused" (YesGuardSites ([uint32]$f.p.Id) 4 (SitesAllowed $true $lJoin $pj) 2 6>$null) "SITE_REPEAT"
Case "client 1101 third no click" (Clicks $f).Count 0
Case "client presses" $script:yesPressed 2
Done $f
# the same two allowed entries with total 1 (a run without -Client) still stop at the limit
$script:yesPressed = 1; $script:sitePressed = @{ "ARSENAL_1101" = @{ h = [IntPtr]::Zero; at = (Get-Date).AddSeconds(-30) } }
$f = FakeDlg $script:AuthTitle $msg $btns "c1101tot"
Case "client 1101 second over total 1" (YesGuardSites ([uint32]$f.p.Id) 5 (SitesAllowed $true $lJoin $pj) 1 6>$null) "LIMIT_REACHED"
Case "client 1101 over total no click" (Clicks $f).Count 0
Done $f
$script:yesPressed = 0; $script:yesHandles = @(); $script:yesAt = $null; $script:sitePressed = @{}
$f = FakeDlg "Background Server" "" "OK" "child"
Case "unknown child window" (UnknownWindow ([uint32]$f.p.Id) $false) "Background Server"
Case "unknown child auth title" (UnknownWindow ([uint32]$f.p.Id) $true) "Background Server"
Done $f
$f = FakeDlg "Arma Reforger Workbench" "" "OK" "startup"
Case "startup window early passes" (UnknownWindowStartup ([uint32]$f.p.Id) $false 3) ""
Case "startup window after main stops" (UnknownWindowStartup ([uint32]$f.p.Id) $true 3) "Arma Reforger Workbench"
Case "startup window late stops" (UnknownWindowStartup ([uint32]$f.p.Id) $false 31) "Arma Reforger Workbench"
# the server Play window (run 174552): let through only in Play; its one read reports buttons, it never stops on them
Case "play window in play passes" (UnknownWindowStartup ([uint32]$f.p.Id) $true 60 $true) ""
Case "play window outside play stops" (UnknownWindowStartup ([uint32]$f.p.Id) $true 60 $false) "Arma Reforger Workbench"
$pr = PlayWindowRead ([RiconWin]::FindByTitle([uint32]$f.p.Id, "Arma Reforger Workbench"))
Case "play window read buttons" @($pr.buttons, ($pr.lines -join ",")) @("1", 'ControlType.Button "OK",ControlType.Text "OK"')
Case "play window read size" ($pr.size -match '^[1-9][0-9]*x[1-9][0-9]*$') $true
Done $f
$f = FakeDlg "Arma Reforger Workbench" "Starting" "" "playnobtn"
$pr = PlayWindowRead ([RiconWin]::FindByTitle([uint32]$f.p.Id, "Arma Reforger Workbench"))
Case "play window no button" @($pr.buttons, ($pr.lines -join ",")) @("0", 'ControlType.Text "Starting"')
# B0c client windows (claude #4904, codex #4908): read once per handle and title, passive also with buttons, stop on a prompt
$cseen = @{}
Case "client plain window passes" (ClientWindowStep ([uint32]$f.p.Id) $cseen 1 6>$null) ""
Case "client window recorded" $cseen.Count 1
Case "client same window not reread" (ClientWindowStep ([uint32]$f.p.Id) $cseen 2 6>&1 | Where-Object { "$_" -like "CLIENT_WINDOW*" }).Count 0
Done $f
$f = FakeDlg "Arma Reforger" "Failed to load" "OK" "clienterr"
Case "client dialog passive" (ClientWindowStep ([uint32]$f.p.Id) @{} 3 6>$null) ""
Case "client dialog no click" (Clicks $f).Count 0
Done $f
$f = FakeDlg $script:AuthTitle $msg $btns "clientauth"
Case "client prompt stops" (ClientWindowStep ([uint32]$f.p.Id) @{} 4 6>$null) "PROMPT_IN_CLIENT"
Case "client prompt no click" (Clicks $f).Count 0
Done $f
# secrets are masked in the window lines, also past the 80-character cut (codex #4918)
$sec = "10.99.88.77"
$f = FakeDlg ("Arma Reforger " + $sec) ("x" * 75 + " " + $sec) "" "clientmask"
$out = @(ClientWindowStep ([uint32]$f.p.Id) @{} 5 @($sec) 6>&1 | ForEach-Object { "$_" })
Case "client window lines masked" (@($out | Where-Object { $_ -like "*10.99*" -or $_ -like "*88.77*" }).Count) 0
Case "client window mask shown" (@($out | Where-Object { $_ -like "*<masked>*" }).Count -ge 1) $true
Done $f
Case "mask text" (MaskText "a 10.1.2.3 b C:\tmp\x" @("10.1.2.3", "C:\tmp")) "a <masked> b <masked>\x"
Case "mask text no secrets" (MaskText "abc" @()) "abc"
# the client as a second owned root (codex #4917): its chain survives the root's exit, a reused PID does not join it
$t0 = Get-Date "2026-10-25T19:00:00"
function PRow($id, $pp, $s) { [pscustomobject]@{ ProcessId = $id; ParentProcessId = $pp; CreationDate = $t0.AddSeconds($s) } }
$tree = @((PRow 100 4 0), (PRow 101 100 5), (PRow 102 101 6), (PRow 200 4 1))
Case "owned kids root alive" (@(OwnedDescendants $tree 100 $t0) | Sort-Object) @("101", "102")
Case "owned kids root gone" (@(OwnedDescendants @((PRow 101 100 5), (PRow 102 101 6)) 100 $t0 $t0.AddSeconds(10)) | Sort-Object) @("101", "102")
Case "owned kids reused pid" @(OwnedDescendants @((PRow 101 100 5), (PRow 103 100 20)) 100 $t0 $t0.AddSeconds(10)) @("101")
Case "owned kids before root" @(OwnedDescendants @((PRow 104 100 -5)) 100 $t0).Count 0
# stop request versus confirmed exit, with stand-ins (codex #4917)
function Stub([bool]$exited, [bool]$killThrows) {
	$o = [pscustomobject]@{ gone = $exited; throws = $killThrows; kills = 0 }
	$o | Add-Member -MemberType ScriptProperty -Name HasExited -Value { $this.gone }
	$o | Add-Member -MemberType ScriptMethod -Name Refresh -Value { }
	$o | Add-Member -MemberType ScriptMethod -Name Kill -Value { $this.kills++; if ($this.throws) { throw [ComponentModel.Win32Exception]::new(5) }; $this.gone = $true }
	$o | Add-Member -MemberType ScriptMethod -Name WaitForExit -Value { param($ms) $this.gone }
	return $o
}
$s1 = Stub $false $true
$r = StopOwned $s1 10
Case "stop kill throws not confirmed" @($r.requested, $r.exited, $r.error, $s1.kills) @("True", "False", "KILL_Win32Exception", "1")
$s2 = Stub $false $false
$r = StopOwned $s2 10
Case "stop kill confirmed" @($r.requested, $r.exited, $r.error) @("True", "True", "")
$s3 = Stub $true $false
$r = StopOwned $s3 10
Case "stop already gone" @($r.requested, $r.exited, $s3.kills) @("False", "True", "0")
$pp = NewPeerProfile $WorkDir ("ricon-peer-selftest-" + [Guid]::NewGuid().ToString("N").Substring(0, 8))
Case "peer profile new" @($pp.ok, (Test-Path $pp.path), @(Get-ChildItem $pp.path -Force).Count) @("True", "True", "0")
Set-Content -Path (Join-Path $pp.path "x.txt") -Value "x"
Case "peer profile not empty refused" (NewPeerProfile (Split-Path $pp.path) (Split-Path $pp.path -Leaf)).why "PROFILE_NOT_EMPTY"
Remove-Item $pp.path -Recurse -Force
$f = FakeDlg "MainWindow" "" "OK" "mainwindow"
Case "MainWindow window early passes" (UnknownWindowStartup ([uint32]$f.p.Id) $false 8) ""
Case "MainWindow window after main stops" (UnknownWindowStartup ([uint32]$f.p.Id) $true 8) "MainWindow"
Case "MainWindow not a play title" (UnknownWindowStartup ([uint32]$f.p.Id) $true 60 $true) "MainWindow"
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

# The stand-in windows are off-screen, but WPF keeps a menu popup inside the visible screen, so the submenus flashed in
# the owner's top-left corner (2026-10-25). Every menu stand-in makes its submenu popups fully transparent (the popups
# have AllowsTransparency=True); UIA still sees and drives the items.
$hidePopups = '[System.Windows.EventManager]::RegisterClassHandler([System.Windows.Controls.MenuItem], [System.Windows.Controls.MenuItem]::SubmenuOpenedEvent, [System.Windows.RoutedEventHandler]{ param($s, $e) if ($e.OriginalSource -ne $s) { return }; $pp = $s.Template.FindName("PART_Popup", $s); if ($pp -and $pp.Child) { $pp.Child.Opacity = 0 } })'

# ExpandRead on a WPF stand-in menu (ExpandCollapse provider of WPF MenuItem): one bool back, items on the host
$mk = Join-Path $WorkDir "ricon-fakemenu-claude.ps1"
(@'
Add-Type -AssemblyName PresentationFramework
#HIDE_POPUPS
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
'@).Replace("#HIDE_POPUPS", $hidePopups) | Set-Content -Path $mk -Encoding ASCII
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
$hidePopups
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
# snapshot completeness (codex #4675): no restore from an unknown state
$req = @("Server localhost", "Server localhost + PeerTool")
$good = [ordered]@{ "Play inside Viewport" = "On"; "Server localhost" = "Off"; "Server localhost + PeerTool" = "Off" }
Case "snapshot complete" (SnapshotProblem $good $null $req) ""
Case "snapshot same keys later" (SnapshotProblem $good @("Play inside Viewport", "Server localhost", "Server localhost + PeerTool") $req) ""
$noTarget = [ordered]@{ "Play inside Viewport" = "On"; "Server localhost + PeerTool" = "Off" }
Case "snapshot missing target" (SnapshotProblem $noTarget $null $req) 'MISSING "Server localhost"'
$noOther = [ordered]@{ "Server localhost" = "Off"; "Server localhost + PeerTool" = "Off" }
Case "snapshot missing other" (SnapshotProblem $noOther @("Play inside Viewport", "Server localhost", "Server localhost + PeerTool") $req) "KEYS_CHANGED"
$indet = [ordered]@{ "Play inside Viewport" = "On"; "Server localhost" = "Indeterminate"; "Server localhost + PeerTool" = "Off" }
Case "snapshot indeterminate" (SnapshotProblem $indet $null $req) 'STATE_NOT_ON_OFF "Server localhost"=Indeterminate'
$extra = [ordered]@{ "Play inside Viewport" = "On"; "Server localhost" = "On"; "Server localhost + PeerTool" = "Off"; "New" = "Off" }
Case "snapshot keys changed" (SnapshotProblem $extra @("Play inside Viewport", "Server localhost", "Server localhost + PeerTool") $req) "KEYS_CHANGED"
$reordered = [ordered]@{ "Server localhost" = "Off"; "Play inside Viewport" = "On"; "Server localhost + PeerTool" = "Off" }
Case "snapshot reordered" (SnapshotProblem $reordered @("Play inside Viewport", "Server localhost", "Server localhost + PeerTool") $req) "KEYS_CHANGED"
Case "toggle diff" (ToggleDiff $a $b2) @("x", "z")
Case "toggle diff equal" (ToggleDiff $a $a).Count 0
$fo = FakeOptions $true $false "excl"
$rt = RoundTrip $fo.h ([uint32]$fo.p.Id) "Server localhost" { "" } @("Server localhost + PeerTool") 6>$null
Case "roundtrip exclusive result" $rt "TOGGLED_ON|RESTORE_OK"
Case "roundtrip exclusive toggles" $script:roundTripToggles 3
$h = OpenOptions $fo.h ([uint32]$fo.p.Id); $m = (ReadToggles $h.opt).map; $null = CloseOptions $h $fo.h
Case "roundtrip exclusive final" (MapText $m) "Play inside Viewport=On | Server localhost=Off | Server localhost + PeerTool=Off"
DoneOpts $fo
$fo = FakeOptions $false $false "indep"
$rt = RoundTrip $fo.h ([uint32]$fo.p.Id) "Server localhost" { "" } @("Server localhost + PeerTool") 6>$null
Case "roundtrip independent result" $rt "TOGGLED_ON|RESTORE_OK"
Case "roundtrip independent toggles" $script:roundTripToggles 2
DoneOpts $fo
$fo = FakeOptions $true $false "inv"
$rt = RoundTrip $fo.h ([uint32]$fo.p.Id) "Server localhost" { "" } @("Server localhost + PeerTool") 3 "Invoke" 6>$null
Case "roundtrip invoke exclusive result" $rt "TOGGLED_ON|RESTORE_OK"
Case "roundtrip invoke exclusive actions" $script:roundTripToggles 3
DoneOpts $fo
$fo = FakeOptions $false $false "invstale"
# up to 3 opens, as StrictSnapshot does: a fresh stand-in once missed "Play Game Options" on the first open (run on
# a82c788 + #4917 fixes), as the real editor does after a world load
$h = $null
for ($try = 1; $try -le 3 -and $null -eq $h; $try++) { $h = OpenOptions $fo.h ([uint32]$fo.p.Id) 6>$null; if ($null -eq $h) { Start-Sleep -Milliseconds 500 } }
Case "invoke expected stale" (ToggleExpected $h.opt "Server localhost" "On" "Invoke") "STALE"
Case "invoke bad method" (ToggleExpected $h.opt "Server localhost" "Off" "Click") "BAD_METHOD"
Case "invoke stale left state" ((ReadToggles $h.opt).map["Server localhost"]) "Off"
$null = CloseOptions $h $fo.h
DoneOpts $fo
Case "new pids none" (NewPids @(10, 11) @(11, 10)).Count 0
Case "new pids one" (NewPids @(10) @(10, 12)) @(12)
Case "new pids empty baseline" (NewPids @() @(5)) @(5)
# a stop signal right after the first action: no read, no second action
$fo = FakeOptions $false $false "after"
$script:afterCalls = 0
$rt = RoundTrip $fo.h ([uint32]$fo.p.Id) "Server localhost" { $script:afterCalls++; if ($script:afterCalls -ge 2) { "NEW_PROCESS" } else { "" } } @("Server localhost + PeerTool") 3 "Invoke" 6>$null
Case "roundtrip stop after action" $rt "STOPPED_AFTER_ACTION|NEW_PROCESS"
Case "roundtrip stop after action count" $script:roundTripToggles 1
DoneOpts $fo
# contingency RestoreOff (codex #4690): from On to Off only; Off -> nothing; PeerTool On -> nothing
$fo = FakeOptions $false $true "restore"
$rs = RestoreOff $fo.h ([uint32]$fo.p.Id) "Server localhost" { "" } @("Server localhost + PeerTool") 6>$null
Case "restore off from on" $rs "RESTORED_OFF"
Case "restore off one action" $script:restoreActions 1
$rs = RestoreOff $fo.h ([uint32]$fo.p.Id) "Server localhost" { "" } @("Server localhost + PeerTool") 6>$null
Case "restore off when off" $rs "NOT_ON"
Case "restore off when off no action" $script:restoreActions 0
DoneOpts $fo
$fo = FakeOptions $false $true "restorepeer" $true
$rs = RestoreOff $fo.h ([uint32]$fo.p.Id) "Server localhost" { "" } @("Server localhost + PeerTool") 6>$null
Case "restore off peer on refused" $rs "OTHER_NOT_OFF"
Case "restore off peer on no action" $script:restoreActions 0
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
# expected state right before the toggle (codex #4677): a stale expectation never toggles
$fo = FakeOptions $false $false "stale"
$h = OpenOptions $fo.h ([uint32]$fo.p.Id)
Case "toggle expected stale" (ToggleExpected $h.opt "Server localhost" "On") "STALE"
Case "toggle expected no item" (ToggleExpected $h.opt "Server localhost 2" "Off") "NO_ITEM"
$m = (ReadToggles $h.opt).map
Case "toggle stale left state" $m["Server localhost"] "Off"
Case "toggle expected ok" (ToggleExpected $h.opt "Server localhost" "Off") "TOGGLED"
Start-Sleep -Milliseconds 300
$m = (ReadToggles $h.opt).map
Case "toggle expected changed state" $m["Server localhost"] "On"
Case "toggle back expected ok" (ToggleExpected $h.opt "Server localhost" "On") "TOGGLED"
$null = CloseOptions $h $fo.h
DoneOpts $fo
$fo = FakeOptions $false $false "blocked"
$rt = RoundTrip $fo.h ([uint32]$fo.p.Id) "Server localhost" { "DEADLINE" } @("Server localhost + PeerTool") 6>$null
Case "roundtrip blocked before first toggle" $rt "TOGGLE_FAILED"
Case "roundtrip blocked no toggle" $script:roundTripToggles 0
DoneOpts $fo

# server-mode helpers: OnlyTargetChanged (pure), StrictSnapshot and InvokeMenuCommand on a stand-in with a Play Game command
$bb = [ordered]@{ "Play inside Viewport" = "On"; "Server localhost" = "Off"; "Server localhost + PeerTool" = "Off" }
$s1ok = [ordered]@{ "Play inside Viewport" = "On"; "Server localhost" = "On"; "Server localhost + PeerTool" = "Off" }
$s1two = [ordered]@{ "Play inside Viewport" = "Off"; "Server localhost" = "On"; "Server localhost + PeerTool" = "Off" }
Case "only target changed ok" (OnlyTargetChanged $bb $s1ok "Server localhost") ""
Case "only target changed two" (OnlyTargetChanged $bb $s1two "Server localhost") "DIFF Play inside Viewport,Server localhost"
Case "only target changed none" (OnlyTargetChanged $bb $bb "Server localhost") "DIFF "
$mk = Join-Path $WorkDir "ricon-fakeplay-claude.ps1"
$clickFile = Join-Path $WorkDir "ricon-fakeplay-click.txt"
if (Test-Path $clickFile) { Remove-Item $clickFile -Force }
@"
Add-Type -AssemblyName PresentationFramework
$hidePopups
`$w = New-Object System.Windows.Window
`$w.Title = "World Editor - Fake play"; `$w.Width = 300; `$w.Height = 120; `$w.Left = -3000; `$w.Top = -3000
`$w.ShowActivated = `$false; `$w.ShowInTaskbar = `$false; `$w.WindowStartupLocation = "Manual"
`$menu = New-Object System.Windows.Controls.Menu
`$game = New-Object System.Windows.Controls.MenuItem; `$game.Header = "Game"
`$play = New-Object System.Windows.Controls.MenuItem; `$play.Header = "Play Game"
`$play.Add_Click({ Add-Content -Path "$clickFile" -Value "Play Game" })
`$opt = New-Object System.Windows.Controls.MenuItem; `$opt.Header = "Play Game Options"
`$sv = New-Object System.Windows.Controls.MenuItem; `$sv.Header = "Server localhost"; `$sv.IsCheckable = `$true
`$pt = New-Object System.Windows.Controls.MenuItem; `$pt.Header = "Server localhost + PeerTool"; `$pt.IsCheckable = `$true
[void]`$opt.Items.Add(`$sv); [void]`$opt.Items.Add(`$pt)
`$dbg = New-Object System.Windows.Controls.MenuItem; `$dbg.Header = "Script Debug Mode"; `$dbg.IsCheckable = `$true
foreach (`$i in @(`$play, `$opt, `$dbg)) { [void]`$game.Items.Add(`$i) }
[void]`$menu.Items.Add(`$game)
`$w.Content = `$menu
`$t = New-Object System.Windows.Threading.DispatcherTimer; `$t.Interval = [TimeSpan]::FromSeconds(40); `$t.Add_Tick({ `$w.Close() }.GetNewClosure()); `$t.Start()
[void]`$w.ShowDialog()
"@ | Set-Content -Path $mk -Encoding ASCII
$pp = Start-Process -FilePath "powershell.exe" -PassThru -WindowStyle Hidden -ArgumentList @("-NoProfile", "-STA", "-ExecutionPolicy", "Bypass", "-File", "`"$mk`"")
for ($i = 0; $i -lt 60 -and [RiconWin]::FindByTitle([uint32]$pp.Id, "World Editor - Fake play") -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 250 }
Start-Sleep -Milliseconds 500
$ph = [RiconWin]::FindByTitle([uint32]$pp.Id, "World Editor - Fake play")
$snapP = StrictSnapshot $ph ([uint32]$pp.Id) $null @("Server localhost", "Server localhost + PeerTool") 6>$null
Case "strict snapshot fake" (MapText $snapP) "Server localhost=Off | Server localhost + PeerTool=Off"
Case "invoke command options refused" (InvokeMenuCommand $ph "Game" "Play Game Options") "NOT_A_COMMAND"
Case "invoke command toggle refused" (InvokeMenuCommand $ph "Game" "Script Debug Mode") "NOT_A_COMMAND"
Case "invoke refused not attempted" $script:menuCommandAttempted $false
Case "invoke command missing" (InvokeMenuCommand $ph "Game" "Stop Game") "NO_ITEM"
Case "invoke missing not attempted" $script:menuCommandAttempted $false
Case "invoke command no menu" (InvokeMenuCommand $ph "Run" "Play Game") "NO_MENU"
Case "invoke refused cases no click" (Test-Path $clickFile) $false
Case "invoke command play" (InvokeMenuCommand $ph "Game" "Play Game") "INVOKED"
Case "invoke play attempted" $script:menuCommandAttempted $true
Start-Sleep -Milliseconds 500
Case "invoke command clicked once" (@(Get-Content $clickFile -ErrorAction SilentlyContinue)) @("Play Game")
if (-not $pp.HasExited) { $pp.Kill() }
Remove-Item $mk, $clickFile -Force -ErrorAction SilentlyContinue

# async Play (codex #4719): a Play Game command whose click opens a modal prompt; the caller keeps observing, answers it
# through YesGuard, and the worker completes; WPF may run the click asynchronously, so this checks the mechanics only
$mk = Join-Path $WorkDir "ricon-fakeasync-claude.ps1"
$msgA = $script:ExpectedMessage
@"
Add-Type -AssemblyName PresentationFramework
$hidePopups
`$w = New-Object System.Windows.Window
`$w.Title = "World Editor - Fake async"; `$w.Width = 300; `$w.Height = 120; `$w.Left = -3000; `$w.Top = -3000
`$w.ShowActivated = `$false; `$w.ShowInTaskbar = `$false; `$w.WindowStartupLocation = "Manual"
`$menu = New-Object System.Windows.Controls.Menu
`$game = New-Object System.Windows.Controls.MenuItem; `$game.Header = "Game"
`$play = New-Object System.Windows.Controls.MenuItem; `$play.Header = "Play Game"
`$play.Add_Click({
	`$d = New-Object System.Windows.Window
	`$d.Title = "Script Authorization Required"; `$d.Width = 420; `$d.Height = 160; `$d.Left = -3000; `$d.Top = -2700
	`$d.ShowActivated = `$false; `$d.WindowStartupLocation = "Manual"
	`$sp = New-Object System.Windows.Controls.StackPanel
	`$tb = New-Object System.Windows.Controls.TextBlock; `$tb.Text = "$msgA"; [void]`$sp.Children.Add(`$tb)
	foreach (`$c in @("Yes", "Yes to All", "No", "No to All")) { `$b = New-Object System.Windows.Controls.Button; `$b.Content = `$c; `$b.Add_Click({ `$d.Close() }.GetNewClosure()); [void]`$sp.Children.Add(`$b) }
	`$d.Content = `$sp
	[void]`$d.ShowDialog()
}.GetNewClosure())
[void]`$game.Items.Add(`$play); [void]`$menu.Items.Add(`$game)
`$plugins = New-Object System.Windows.Controls.MenuItem; `$plugins.Header = "Plugins"
foreach (`$n in @("RICON: Play", "RICON: Other")) { `$pi = New-Object System.Windows.Controls.MenuItem; `$pi.Header = `$n; [void]`$plugins.Items.Add(`$pi) }
[void]`$menu.Items.Add(`$plugins)
`$empty = New-Object System.Windows.Controls.MenuItem; `$empty.Header = "Empty"; [void]`$empty.Items.Add((New-Object System.Windows.Controls.Separator))
[void]`$menu.Items.Add(`$empty)
`$w.Content = `$menu
`$t = New-Object System.Windows.Threading.DispatcherTimer; `$t.Interval = [TimeSpan]::FromSeconds(40); `$t.Add_Tick({ `$w.Close() }.GetNewClosure()); `$t.Start()
[void]`$w.ShowDialog()
"@ | Set-Content -Path $mk -Encoding ASCII
$ap = Start-Process -FilePath "powershell.exe" -PassThru -WindowStyle Hidden -ArgumentList @("-NoProfile", "-STA", "-ExecutionPolicy", "Bypass", "-File", "`"$mk`"")
for ($i = 0; $i -lt 60 -and [RiconWin]::FindByTitle([uint32]$ap.Id, "World Editor - Fake async") -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 250 }
Start-Sleep -Milliseconds 500
$ah = [RiconWin]::FindByTitle([uint32]$ap.Id, "World Editor - Fake async")
$prepA = PrepareMenuCommand $ah "Game" "Play Game"
Case "async prepare ready" $prepA.result "READY"
Case "async prepare refuses submenu-less missing" (PrepareMenuCommand $ah "Game" "Stop Game").result "NO_ITEM"
$prepA = PrepareMenuCommand $ah "Game" "Play Game"
$jobA = StartAsyncInvoke $prepA.ip
$seen = $false; $ticks = 0
for ($i = 0; $i -lt 40 -and -not $seen; $i++) { Start-Sleep -Milliseconds 250; $ticks++; $seen = [RiconWin]::FindByTitle([uint32]$ap.Id, $script:AuthTitle) -ne [IntPtr]::Zero }
Case "async caller keeps running and sees the prompt" $seen $true
$script:yesPressed = 0; $script:yesHandles = @(); $script:yesAt = $null
Case "async prompt answered" (YesGuard ([uint32]$ap.Id) 1 $false $false 6>$null) "PRESSED"
$doneA = $false
for ($i = 0; $i -lt 40 -and -not $doneA; $i++) { Start-Sleep -Milliseconds 250; $doneA = PollAsyncInvoke $jobA }
Case "async worker completed" $doneA $true
Case "async worker result" $jobA.result "COMPLETED"
Case "async cleanup done" (CleanupAsyncInvoke $jobA 1000) "DONE COMPLETED"
Case "async cleanup none" (CleanupAsyncInvoke $null 1000) "NONE"
# the plugin start (codex #4850): a missing item invokes nothing and lists what was there; the exact item goes through
# the same worker, and the menu ends Collapsed without being touched again
# As in the runner (FlipTarget closed the menus before the Invoke), Game must be closed first. A full run on 019aa1a once
# read no Plugins items at all (NO_ITEM, empty names); in the two repeat runs Game was already Collapsed here and all
# passed, so the cause is open and this check only rules out an open Game menu.
$gm = FindTopMenu $ah "Game"
$gs = try { [string]$gm.GetCurrentPattern($ECP::Pattern).Current.ExpandCollapseState } catch { "UNREAD" }
"INFO game menu before plugins: $gs"
if (-not (ClosedOk $gs)) { try { $gm.GetCurrentPattern($ECP::Pattern).Collapse() } catch {}; Start-Sleep -Milliseconds 400 }
Case "game closed before plugins" (ClosedOk (FindTopMenu $ah "Game").GetCurrentPattern($ECP::Pattern).Current.ExpandCollapseState) $true
$prepM = PrepareMenuCommand $ah "Plugins" "RICON: Missing"
Case "plugins missing item refused" $prepM.result "NO_ITEM"
Case "plugins missing item lists names" (@($prepM.names) -join ",") "RICON: Play,RICON: Other"
Case "plugins closed after refusal" (ClosedOk (FindTopMenu $ah "Plugins").GetCurrentPattern($ECP::Pattern).Current.ExpandCollapseState) $true
$prepP = PrepareMenuCommand $ah "Plugins" "RICON: Play"
Case "plugins item ready" $prepP.result "READY"
$jobP = StartAsyncInvoke $prepP.ip
$doneP = $false
for ($i = 0; $i -lt 40 -and -not $doneP; $i++) { Start-Sleep -Milliseconds 250; $doneP = PollAsyncInvoke $jobP }
Case "plugins invoke completed" $jobP.result "COMPLETED"
Start-Sleep -Milliseconds 500
Case "plugins closed after invoke" (ClosedOk (FindTopMenu $ah "Plugins").GetCurrentPattern($ECP::Pattern).Current.ExpandCollapseState) $true
Case "plugins cleanup done" (CleanupAsyncInvoke $jobP 1000) "DONE COMPLETED"
# a menu that never shows an item is opened at most 3 times, then refused with nothing invoked and the menu closed
$prepE = PrepareMenuCommand $ah "Empty" "RICON: Play"
Case "empty menu reopened then refused" @($prepE.result, $prepE.opens, @($prepE.names).Count) @("NO_ITEM", "3", "0")
Case "empty menu closed" (ClosedOk (FindTopMenu $ah "Empty").GetCurrentPattern($ECP::Pattern).Current.ExpandCollapseState) $true
"INFO plugins opens: missing=$($prepM.opens) ready=$($prepP.opens) (more than 1 = the stand-in showed no item at first)"
if (-not $ap.HasExited) { $ap.Kill() }
Remove-Item $mk -Force -ErrorAction SilentlyContinue

# The runner's own ClientTick (from its AST, codex #4922, #4926), without processes or UI: a refused start stays
# refused on the next Ticks, starts nothing and looks for no client children. Last block: it stubs Start-Process and
# LocalHostAddress for the rest of this script.
$spAst = [System.Management.Automation.Language.Parser]::ParseFile((Join-Path $PSScriptRoot "wb-serverplay-claude.ps1"), [ref]$null, [ref]$null)
foreach ($d in $spAst.FindAll({ param($n) $n -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -in @("StopClient", "StopClientPart", "ClientChildren", "ClientTick") }, $false)) { . ([scriptblock]::Create($d.Extent.Text)) }
# the masking list in the runner's real order of assignments (codex #4935): empty first, then the -Client preflight, never reset
$asg = @($spAst.FindAll({ param($n) $n -is [System.Management.Automation.Language.AssignmentStatementAst] -and $n.Left.Extent.Text -eq '$script:secrets' }, $true) |
	Sort-Object { $_.Extent.StartOffset } | ForEach-Object { $_.Right.Extent.Text })
Case "secrets assignments in order" ($asg -join " ; ") '@() ; @($ha.address, $OutDir)'
$ha = @{ address = "192.168.1.20" }; $OutDir = "C:\scratch\x"
. ([scriptblock]::Create('$script:secrets = ' + $asg[-1]))
Case "secrets mask after the last assignment" (MaskText "to 192.168.1.20 in C:\scratch\x\log" $script:secrets) "to <masked> in <masked>\log"
# the runner's own peer line regex and Yes total (owner 2026-10-26), against the host line shape of run 144401
$Client = $true
foreach ($n in @('$clientUser', '$peerJoined', '$yesTotal')) {
	$a = @($spAst.FindAll({ param($x) $x -is [System.Management.Automation.Language.AssignmentStatementAst] -and $x.Left.Extent.Text -eq $n }, $true))
	Case "runner assigns $n once" $a.Count 1
	. ([scriptblock]::Create($a[0].Extent.Text))
}
Case "runner client user" $clientUser "RICON_B0c_peer1"
Case "runner yes total with -Client" $yesTotal 2
$hostJoin = "14:45:06.977    NETWORK   (D): ### Matching UserName=RICON_B0c_peer1, IdentityID=00000000-0000-0000-0000-000000000000`r`n14:45:06.978   NETWORK      : ### Creating player: PlayerId=2, Name=RICON_B0c_peer1`r`n"
Case "runner peer line matches 144401" ($hostJoin -match $peerJoined) $true
$ts = "14:45:06.978   NETWORK      : ### Creating player: "
Case "runner peer line without CR" (("a`n" + $ts + "PlayerId=2, Name=RICON_B0c_peer1") -match $peerJoined) $true
Case "runner peer line tab" ("14:45:06.978`tNETWORK`t: ### Creating player: PlayerId=2, Name=RICON_B0c_peer1`r`n" -match $peerJoined) $true
Case "runner peer line longer name" (($ts + "PlayerId=2, Name=RICON_B0c_peer10`r`n") -match $peerJoined) $false
Case "runner peer line other id" (($ts + "PlayerId=3, Name=RICON_B0c_peer1`r`n") -match $peerJoined) $false
Case "runner peer line id 20" (($ts + "PlayerId=20, Name=RICON_B0c_peer1`r`n") -match $peerJoined) $false
Case "runner peer line host player" (($ts + "PlayerId=1, Name=RICON_B0c_peer1`r`n") -match $peerJoined) $false
Case "runner peer line matching only" ("14:45:06.977    NETWORK   (D): ### Matching UserName=RICON_B0c_peer1, IdentityID=0`r`n" -match $peerJoined) $false
Case "runner peer line inside other text" ("14:45:07.000 SCRIPT       : echo NETWORK      : ### Creating player: PlayerId=2, Name=RICON_B0c_peer1`r`n" -match $peerJoined) $false
Case "runner peer line without time stamp" ("NETWORK      : ### Creating player: PlayerId=2, Name=RICON_B0c_peer1`r`n" -match $peerJoined) $false
Remove-Variable Client
# tagged log lines in stdout (before B0d): room for the stream and the dump of 1024 events per side, a cut is printed
$a = @($spAst.FindAll({ param($x) $x -is [System.Management.Automation.Language.AssignmentStatementAst] -and $x.Left.Extent.Text -eq '$taggedCap' }, $true))
Case "runner assigns taggedCap once" $a.Count 1
. ([scriptblock]::Create($a[0].Extent.Text))
Case "runner taggedCap fits two dumps of 1024" ($taggedCap -ge 2 * 1024 + 100) $true
$firsts = @($spAst.FindAll({ param($x) $x -is [System.Management.Automation.Language.CommandAst] -and $x.GetCommandName() -eq "Select-Object" -and $x.Extent.Text -match '-First' }, $true) | ForEach-Object { ($_.Extent.Text -split '-First\s+', 2)[1].Split(' ')[0] })
Case "runner tagged excerpts use taggedCap" (@($firsts | Where-Object { $_ -eq '$taggedCap' }).Count) 2
Case "runner tagged cuts reported" (@($spAst.FindAll({ param($x) $x -is [System.Management.Automation.Language.StringConstantExpressionAst] -or $x -is [System.Management.Automation.Language.ExpandableStringExpressionAst] }, $true) | Where-Object { $_.Value -match '^(C )?TAGGED_TRUNCATED shown=' }).Count) 2
$script:startCalls = 0; $script:addrCalls = 0; $script:addrNext = @{ result = "OK"; address = "192.168.1.20" }
function Start-Process { $script:startCalls++; throw "no process in the self-test" }
function LocalHostAddress { $script:addrCalls++; return $script:addrNext }
$wbPid = [uint32]500; $ha = @{ result = "OK"; address = "192.168.1.20" }; $tracked = @{}; $script:secrets = @()
$script:played = $true; $script:clientProc = $null; $script:clientPid = [uint32]0; $script:clientStart = $null
$over = $lp + "[RICON_LIVEB] inspect=OBSERVATION_END samples=120`n"
$script:clientState = "NOT_STARTED"
Case "tick refused after observation" @((ClientTick 90 $over @() $ep 6>$null), $script:clientState, $script:startCalls, $script:addrCalls) @("", "NOT_STARTED_OBSERVATION_OVER", "0", "0")
Case "tick after refusal stays idle" @((ClientTick 91 $over @() $ep 6>$null), $script:clientState, $script:startCalls, $tracked.Count) @("", "NOT_STARTED_OBSERVATION_OVER", "0", "0")
$script:clientState = "NOT_STARTED"; $script:addrNext = @{ result = "OK"; address = "192.168.1.21" }
Case "tick refused on changed address" @((ClientTick 60 $lp @() $ep 6>$null), $script:clientState, $script:startCalls, $script:addrCalls) @("", "NOT_STARTED_ADDRESS_CHANGED", "0", "1")
Case "tick after address refusal stays idle" @((ClientTick 61 $lp @() $ep 6>$null), $script:clientState, $script:startCalls, $script:addrCalls) @("", "NOT_STARTED_ADDRESS_CHANGED", "0", "1")
$script:clientState = "NOT_STARTED"
Case "tick waits without udp" @((ClientTick 50 $lp @() @() 6>$null), $script:clientState, $script:startCalls) @("", "NOT_STARTED", "0")
# a live client root (stand-in, a PID no process has) without and with one child (codex #4930: the empty child list
# came back as one element and stopped a healthy client)
function Raw($o) { }
Case "children without client" @(ClientChildren 1 @()).Count 0
$cdir = Join-Path $WorkDir ("ricon-client-selftest-" + [Guid]::NewGuid().ToString("N").Substring(0, 8)); $null = New-Item -ItemType Directory -Path $cdir
$cl = @{ path = $cdir }; $pp = @{ path = $cdir }; $script:clientSeen = @{}; $script:clientLog = ""; $script:clientDog = $null
$script:clientProc = Stub $false $false; $script:clientPid = [uint32]4000000001; $script:clientStart = $t0; $script:clientAt = Get-Date
$script:clientState = "RUNNING"; $tracked = @{}
$alone = @((PRow 4000000001 4 0))
Case "tick live client no children" @((ClientTick 70 $lp $alone $ep 6>$null), $script:clientState, $script:clientProc.kills, $tracked.Count) @("", "RUNNING", "0", "0")
Case "children of live client none" @(ClientChildren 71 $alone).Count 0
$withKid = @((PRow 4000000001 4 0), (PRow 4000000002 4000000001 5))
Case "tick live client one child" @((ClientTick 72 $lp $withKid $ep 6>$null), $script:clientState, $script:clientProc.kills, $tracked.Count, [bool]$tracked[[uint32]4000000002].client) @("", "STOPPED_CHILD_OF_CLIENT", "1", "1", "True")
Case "tick child reported once" @((ClientTick 73 $lp $withKid $ep 6>&1 | Where-Object { "$_" -like "CHILD_OF_CLIENT*" }).Count, $script:clientProc.kills) @("0", "1")
Remove-Item $cdir -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item function:Start-Process, function:LocalHostAddress, function:Raw -ErrorAction SilentlyContinue

"PASS $($script:pass)/$($script:total)"
