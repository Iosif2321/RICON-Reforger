param([switch]$GuardWorker, [string]$ChannelName)

# STAGING ONLY. Caller supplies its own Process returned by Start-Process, never an arbitrary PID.
# Ready -> parent QPC arm request -> Armed ACK -> caller trigger. No log polling or shell source interpolation.
# 1500 ms is the kill target; observed exit must be within 2000 ms. Windows is not hard real time.
# Sources: learn.microsoft.com/dotnet/api/system.threading.eventwaithandle
# learn.microsoft.com/dotnet/api/system.io.memorymappedfiles.memorymappedfile.createnew
# learn.microsoft.com/dotnet/api/system.diagnostics.process.kill (asynchronous; confirm exit)
# learn.microsoft.com/windows/win32/sysinfo/acquiring-high-resolution-time-stamps (Stopwatch uses QPC)
# Map offsets: pid0/start8/cutoff16/armTimeout20/bound24/status28/ready32/arm40/armed48/freq56/
# killRequest64/killReturn72/exit80/done88/trigger96/verified108/actualStart112.

$script:CutoffGuardPath = $PSCommandPath

function Start-CutoffGuard {
    param([Parameter(Mandatory=$true)][System.Diagnostics.Process]$Target, [long]$ExpectedStartUtcTicks=0)
    $null = $Target.Handle
    if ($Target.HasExited) { throw 'TARGET_ALREADY_EXITED' }
    if (-not [Diagnostics.Stopwatch]::IsHighResolution) { throw 'QPC_REQUIRED' }
    $actualStart=$Target.StartTime.ToUniversalTime().Ticks
    if (-not $ExpectedStartUtcTicks) { $ExpectedStartUtcTicks = $actualStart }
    if ($script:CutoffGuardPath -match '["\r\n]') { throw 'UNSAFE_SCRIPT_PATH' }
    $g = [pscustomobject]@{ Name=('Local\RiconCutoff_' + [guid]::NewGuid().ToString('N')); Events=@{}; Map=$null; View=$null; Worker=$null; WorkerStart=0L; Ready=$false; ArmAck=$null; Target=$Target; TargetStart=$actualStart; ArmRequested=$false; Closed=$false }
    try {
        foreach ($kind in @('Ready','Arm','Armed','Done')) {
            $created=$false
            $g.Events[$kind]=[Threading.EventWaitHandle]::new($false,[Threading.EventResetMode]::ManualReset,($g.Name+'.'+$kind),[ref]$created)
            if (-not $created) { throw 'CHANNEL_ALREADY_EXISTS' }
        }
        $g.Map=[IO.MemoryMappedFiles.MemoryMappedFile]::CreateNew(($g.Name+'.State'),128)
        $g.View=$g.Map.CreateViewAccessor()
        $g.View.Write(0,[int]$Target.Id); $g.View.Write(8,[long]$ExpectedStartUtcTicks)
        $g.View.Write(16,[int]1500); $g.View.Write(20,[int]3000); $g.View.Write(24,[int]2000)
        $g.View.Write(56,[long][Diagnostics.Stopwatch]::Frequency); $g.View.Flush()
        $exe=Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
        $g.Worker=Start-Process -FilePath $exe -ArgumentList @('-NoLogo','-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',('"'+$script:CutoffGuardPath+'"'),'-GuardWorker','-ChannelName',$g.Name) -WindowStyle Hidden -PassThru
        $null=$g.Worker.Handle; $g.WorkerStart=$g.Worker.StartTime.ToUniversalTime().Ticks
        $which=[Threading.WaitHandle]::WaitAny([Threading.WaitHandle[]]@($g.Events.Ready,$g.Events.Done),5000)
        $g.Ready=($which -eq 0 -and -not $g.Events.Done.WaitOne(0))
        return $g
    } catch { Close-CutoffGuard $g; throw }
}

function Arm-CutoffGuard {
    param([Parameter(Mandatory=$true)]$Guard)
    if (-not $Guard.Ready -or $Guard.Events.Done.WaitOne(0)) { return $false }
    if ($Guard.View.ReadInt64(40) -ne 0) { throw 'ARM_ALREADY_REQUESTED' }
    # The deadline starts before signalling, so scheduling/ACK delay cannot extend the budget.
    $Guard.ArmRequested=$true
    $Guard.View.Write(40,[long][Diagnostics.Stopwatch]::GetTimestamp()); $Guard.View.Flush()
    $null=$Guard.Events.Arm.Set()
    $which=[Threading.WaitHandle]::WaitAny([Threading.WaitHandle[]]@($Guard.Events.Armed,$Guard.Events.Done),3000)
    $elapsed=1000.0*([Diagnostics.Stopwatch]::GetTimestamp()-$Guard.View.ReadInt64(40))/$Guard.View.ReadInt64(56)
    $accepted=($which -eq 0 -and -not $Guard.Events.Done.WaitOne(0) -and $elapsed -ge 0 -and $elapsed -lt 1500)
    if ($accepted) {
        $Guard.ArmAck=[pscustomobject]@{channel=$Guard.Name;targetId=$Guard.View.ReadInt32(0);startUtcTicks=$Guard.View.ReadInt64(8);armTicks=$Guard.View.ReadInt64(40);armedAckTicks=$Guard.View.ReadInt64(48);frequency=$Guard.View.ReadInt64(56);cutoffMs=1500;exitBoundaryMs=2000}
    }
    return $accepted
}

function Get-CutoffResult {
    param([Parameter(Mandatory=$true)]$Guard)
    if (-not $Guard.Events.Done.WaitOne(6000)) { throw 'GUARD_RESULT_TIMEOUT' }
    $v=$Guard.View; $freq=$v.ReadInt64(56); $arm=$v.ReadInt64(40); $ack=$v.ReadInt64(48)
    $kill=$v.ReadInt64(64); $killReturn=$v.ReadInt64(72); $exit=$v.ReadInt64(80); $trigger=$v.ReadInt64(96)
    $labels=@{0='STARTING';1='READY';2='ARMED';3='NATURAL_EXIT';4='KILLED';-1='INVALID_IDENTITY';-2='INVALID_NO_ARM';-3='INVALID_LATE_ARM';-4='INVALID_EXIT_TIMEOUT';-5='INVALID_BOUNDARY';-6='INVALID_INTERNAL';-7='INVALID_EXIT_BEFORE_ARM';-8='INVALID_CLOCK'}
    $code=$v.ReadInt32(28); $status=$labels[$code]
    if (-not $status) { $status='INVALID_UNKNOWN_STATUS' }
    $exitMs=$null; $killMs=$null; $overshoot=$null; $outerOvershoot=$null
    if ($arm -gt 0 -and $exit -gt 0) { $exitMs=1000.0*($exit-$arm)/$freq; $overshoot=$exitMs-1500.0; $outerOvershoot=$exitMs-2000.0 }
    if ($arm -gt 0 -and $kill -gt 0) { $killMs=1000.0*($kill-$arm)/$freq }
    $targetExitConfirmed=Test-CutoffTargetExited $Guard
    $valid=($code -in @(3,4) -and $targetExitConfirmed -and $v.ReadInt32(108) -eq 1 -and $arm -ge $v.ReadInt64(32) -and $ack -ge $arm -and $exit -ge $ack -and $exitMs -le 2000 -and $exitMs -ge 0)
    if ($kill -gt 0) { $valid=$valid -and $kill -ge $ack -and $killReturn -ge $kill -and $exit -ge $killReturn -and $killMs -le 2000 }
    if ($trigger -gt 0) { $valid=$valid -and $trigger -ge $ack -and $trigger -lt $exit }
    if ($code -in @(3,4) -and -not $valid) { $status='INVALID_BOUNDARY' }
    if ($code -in @(3,4) -and -not $targetExitConfirmed) { $status='INVALID_TARGET_EXIT_UNCONFIRMED' }
    [pscustomobject]@{ status=$status; valid=[bool]$valid; targetExitConfirmed=[bool]$targetExitConfirmed; frequency=$freq; readyTicks=$v.ReadInt64(32); armTicks=$arm; armedAckTicks=$ack; triggerTicks=$trigger; killRequestTicks=$kill; killReturnTicks=$killReturn; exitObservedTicks=$exit; doneTicks=$v.ReadInt64(88); killRequestMs=$killMs; exitMs=$exitMs; exitOvershoot1500Ms=$overshoot; exitOvershoot2000Ms=$outerOvershoot; identityVerified=($v.ReadInt32(108) -eq 1); killRequested=($kill -gt 0) }
}

function Test-CutoffTargetExited {
    param($Guard)
    try { return ($Guard.Target.StartTime.ToUniversalTime().Ticks -eq $Guard.TargetStart -and $Guard.Target.HasExited) }
    catch { return $false }
}

function Close-CutoffGuard {
    param($Guard)
    if ($null -eq $Guard -or $Guard.Closed) { return }
    # Refuse before any cleanup: Done or an exited worker cannot disarm a live/unknown target.
    if ($Guard.ArmRequested -and -not (Test-CutoffTargetExited $Guard)) { throw 'CLOSE_REFUSED_ARMED_TARGET_ALIVE_OR_UNKNOWN' }
    try {
        if ($Guard.Worker) {
            try {
                $wait=if($Guard.ArmRequested){1000}else{7000}
                if (-not $Guard.Worker.WaitForExit($wait) -and $Guard.Worker.StartTime.ToUniversalTime().Ticks -eq $Guard.WorkerStart) { $Guard.Worker.Kill(); if(-not $Guard.Worker.WaitForExit(1000)){throw 'WORKER_CLEANUP_EXIT_UNCONFIRMED'} }
            } finally { $Guard.Worker.Dispose() }
        }
    } finally {
        if ($Guard.View) { $Guard.View.Dispose() }
        if ($Guard.Map) { $Guard.Map.Dispose() }
        foreach ($event in $Guard.Events.Values) { $event.Dispose() }
        $Guard.Closed=$true
    }
}

function Invoke-CutoffWorker {
    param([string]$Name)
    if ($Name -cnotmatch '\ALocal\\RiconCutoff_[a-f0-9]{32}\z') { return }
    $events=@{}; $map=$null; $v=$null; $target=$null; $code=-6; $verified=$false; $armSeen=$false
    try {
        foreach($kind in @('Ready','Arm','Armed','Done')) { $events[$kind]=[Threading.EventWaitHandle]::OpenExisting($Name+'.'+$kind) }
        $map=[IO.MemoryMappedFiles.MemoryMappedFile]::OpenExisting($Name+'.State'); $v=$map.CreateViewAccessor()
        $target=[Diagnostics.Process]::GetProcessById($v.ReadInt32(0)); $null=$target.Handle
        $actual=$target.StartTime.ToUniversalTime().Ticks; $v.Write(112,[long]$actual)
        if ($actual -ne $v.ReadInt64(8)) { $code=-1; return }
        $verified=$true
        $v.Write(108,[int]1)
        if ($target.HasExited) { $code=-7; return }
        $freq=[Diagnostics.Stopwatch]::Frequency
        if (-not [Diagnostics.Stopwatch]::IsHighResolution -or $freq -ne $v.ReadInt64(56)) { $code=-8; return }
        $v.Write(32,[long][Diagnostics.Stopwatch]::GetTimestamp()); $v.Write(28,[int]1); $v.Flush(); $null=$events.Ready.Set()
        if (-not $events.Arm.WaitOne($v.ReadInt32(20))) { $code=-2; return }
        $armSeen=$true
        $arm=$v.ReadInt64(40); $now=[Diagnostics.Stopwatch]::GetTimestamp()
        if ($arm -lt $v.ReadInt64(32) -or $arm -gt $now) { $code=-8; return }
        if ($target.HasExited) { $code=-7; return }
        $cutoff=$v.ReadInt32(16); $bound=$v.ReadInt32(24)
        if ($cutoff -ne 1500 -or $bound -ne 2000) { $code=-8; return }
        $deadline=$arm+[long]($freq*($cutoff/1000.0)); $outer=$arm+[long]($freq*($bound/1000.0))
        $remaining=1000.0*($deadline-$now)/$freq
        if ($remaining -le 0) { $code=-3 } else {
            $v.Write(48,[long][Diagnostics.Stopwatch]::GetTimestamp()); $v.Write(28,[int]2); $v.Flush(); $null=$events.Armed.Set()
            $remaining=1000.0*($deadline-[Diagnostics.Stopwatch]::GetTimestamp())/$freq
            if ($remaining -gt 0 -and $target.WaitForExit([int][Math]::Ceiling($remaining))) {
                $v.Write(80,[long][Diagnostics.Stopwatch]::GetTimestamp()); $code=3; return
            }
        }
        if ($target.HasExited) { $v.Write(80,[long][Diagnostics.Stopwatch]::GetTimestamp()); if($code -ne -3){$code=3}; return }
        # Reuse the opened Process handle and exact start identity; never resolve a PID again here.
        if ($target.StartTime.ToUniversalTime().Ticks -ne $v.ReadInt64(8)) { $code=-1; return }
        $v.Write(64,[long][Diagnostics.Stopwatch]::GetTimestamp()); $target.Kill(); $v.Write(72,[long][Diagnostics.Stopwatch]::GetTimestamp())
        $remaining=1000.0*($outer-[Diagnostics.Stopwatch]::GetTimestamp())/$freq
        $gone=$target.WaitForExit([int][Math]::Max(0,[Math]::Ceiling($remaining)))
        if ($gone) { $v.Write(80,[long][Diagnostics.Stopwatch]::GetTimestamp()) }
        if (-not $gone) { $code=-4 } elseif ($code -ne -3) { $code=4 }
        if ($v.ReadInt64(80) -gt $outer -or $v.ReadInt64(64) -gt $outer) { $code=-5 }
    } catch {
        $code=-6
        if ($armSeen -and $verified -and $target) {
            # An internal error remains INVALID even if emergency containment succeeds.
            try {
                if ($target.StartTime.ToUniversalTime().Ticks -eq $actual -and -not $target.HasExited) {
                    $emergencyKill=[Diagnostics.Stopwatch]::GetTimestamp(); $target.Kill()
                    $emergencyReturn=[Diagnostics.Stopwatch]::GetTimestamp(); $gone=$target.WaitForExit(1000)
                    if($v){$v.Write(64,[long]$emergencyKill);$v.Write(72,[long]$emergencyReturn);if($gone){$v.Write(80,[long][Diagnostics.Stopwatch]::GetTimestamp())}}
                }
            } catch {}
        }
    } finally {
        if ($v) { $v.Write(28,[int]$code); $v.Write(88,[long][Diagnostics.Stopwatch]::GetTimestamp()); $v.Flush() }
        if ($events.Done) { $null=$events.Done.Set() }
        if ($target) { $target.Dispose() }
        if ($v) { $v.Dispose() }; if($map){$map.Dispose()}
        foreach($event in $events.Values){$event.Dispose()}
    }
}

if ($GuardWorker) { Invoke-CutoffWorker $ChannelName; exit 0 }
