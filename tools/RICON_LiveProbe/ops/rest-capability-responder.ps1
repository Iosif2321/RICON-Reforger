# One trusted own-fixture loopback request. No game API, web framework, Arm or capture.
function Get-RestCapabilityDigest([string]$Value) {
    $sha=[Security.Cryptography.SHA256]::Create()
    try{return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($Value)))).Replace('-','')}finally{$sha.Dispose()}
}
function Add-RestOwnedError($State,[string]$Scope,[Exception]$ErrorValue,[switch]$Primary) {
    if($Primary -and -not $State.Failure){$State.Failure=if($ErrorValue.Message -cmatch '\A[A-Z_]+\z'){$ErrorValue.Message}else{$ErrorValue.GetType().Name}}
    while($ErrorValue){
        $code=if($ErrorValue.Message -cmatch '\A[A-Z_]+\z'){$ErrorValue.Message}else{''}
        $State.OwnedErrors.Add([pscustomobject]@{scope=$Scope;type=$ErrorValue.GetType().FullName;hResult=$ErrorValue.HResult;code=$code})
        $ErrorValue=$ErrorValue.InnerException
    }
}
function New-RestCapabilityResponder([string]$Run,[string]$Token,[switch]$GuardedRehearsal,[string]$Attempt='',[string]$HostLog='',[switch]$GuardedCapture,[switch]$NoSend,[switch]$WrongFrequency,[switch]$SeparatedPair,[switch]$Short400,[switch]$BlindTrial,[switch]$BPowerOff,[switch]$Direct) {
    if($Direct -and (-not $GuardedCapture -or $SeparatedPair -or $NoSend -or $WrongFrequency -or $Short400 -or $BlindTrial)){throw 'INVALID_DIRECT_MODE'}
    if($BPowerOff -and (-not $GuardedCapture -or (-not $SeparatedPair -and -not $Direct) -or $NoSend -or $WrongFrequency -or $Short400 -or $BlindTrial)){throw 'INVALID_B_POWER_OFF_MODE'}
    if($BlindTrial -and (-not $GuardedCapture -or -not $SeparatedPair -or $Short400 -or $NoSend -or $WrongFrequency -or $Token -cnotmatch '\A[0-9a-fA-F]{32}\z')){throw 'INVALID_BLIND_MODE'}
    if($Short400 -and (-not $GuardedCapture -or -not $SeparatedPair -or $NoSend -or $WrongFrequency)){throw 'INVALID_SHORT400_MODE'}
    if($SeparatedPair -and -not $GuardedCapture){throw 'SEPARATED_PAIR_REQUIRES_CAPTURE'}
    if($WrongFrequency -and (-not $GuardedCapture -or $NoSend)){throw 'INVALID_WRONG_FREQUENCY_MODE'}
    if($NoSend -and -not $GuardedCapture){throw 'NO_SEND_REQUIRES_GUARDED_CAPTURE'}
    if($GuardedCapture -and $GuardedRehearsal){throw 'REST_GUARDED_MODE_CONFLICT'}
    if($Run -cnotmatch '\A[A-Za-z0-9_.-]{1,64}\z' -or $Token -cnotmatch '\A[A-Za-z0-9_.-]{1,64}\z'){throw 'INVALID_REST_IDENTITY'}
    if(($GuardedRehearsal -or $GuardedCapture) -and $Attempt -cnotmatch '\A[A-Za-z0-9_.-]{1,64}\z'){throw 'INVALID_REST_ATTEMPT'}
    $listener=[Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback,0)
    try {
        $listener.Start(1);$port=([Net.IPEndPoint]$listener.LocalEndpoint).Port
        if($port -lt 1024 -or $port -gt 65535){throw 'INVALID_EPHEMERAL_PORT'}
        $anchorBefore=[Diagnostics.Stopwatch]::GetTimestamp();$utcAnchor=[datetime]::UtcNow;$anchorAfter=[Diagnostics.Stopwatch]::GetTimestamp()
        return [pscustomobject]@{Listener=$listener;Port=$port;Run=$Run;Token=$Token;ExpectedPath=('/ricon-control-capability/'+$Run);
            RunDigest=(Get-RestCapabilityDigest $Run);PayloadDigest=(Get-RestCapabilityDigest $Token);
            Client=$null;Stream=$null;Header=[Collections.Generic.List[byte]]::new();Accepted=0;Responses=0;
            StartedTicks=[Diagnostics.Stopwatch]::GetTimestamp();AcceptedTicks=0L;ResponseBeginTicks=0L;ResponseReturnTicks=0L;ClosedTicks=0L;
            Target=$null;TargetStart=0L;TargetHandle=[IntPtr]::Zero;Done=$false;Status='LISTENING';Failure='';
            QpcFrequency=[Diagnostics.Stopwatch]::Frequency;UtcAnchor=$utcAnchor.ToString('o');UtcAnchorTicks=$utcAnchor.Ticks;AnchorBeforeTicks=$anchorBefore;AnchorAfterTicks=$anchorAfter;
            OwnedErrors=[Collections.Generic.List[object]]::new();CleanupFailure='';WorkerStartedTicks=0L;
            ControlTag=$(if($Direct -and $BPowerOff){'DIRECT_B_POWER_OFF'}else{''});Direct=[bool]$Direct;BPowerOff=[bool]$BPowerOff;PowerRemainingAtHttpMs=$null;BlindTrial=[bool]$BlindTrial;Short400=[bool]$Short400;SeparatedPair=[bool]$SeparatedPair;WrongFrequency=[bool]$WrongFrequency;NoSend=[bool]$NoSend;GuardedCapture=[bool]$GuardedCapture;GuardedRehearsal=[bool]$GuardedRehearsal;Attempt=$Attempt;HostLog=$HostLog;RequestValidated=$false;RequestValidatedTicks=0L;ReleaseObservedTicks=0L;ReadyEvent=$null;ReleaseEvent=$null;Gate=$null;ReleaseTuple=$null;
            WitnessStream=$null;WitnessReader=$null;DispatchObservedTicks=0L;DispatchBeforeGameMs=0L;DispatchAfterGameMs=0L;DispatchWitnessValid=$false}
    } catch {$listener.Stop();throw}
}
function Close-RestCapabilityResponder($State) {
    if(-not $State -or $State.ClosedTicks -gt 0){return}
    try{if($State.WitnessReader){$State.WitnessReader.Dispose()}elseif($State.WitnessStream){$State.WitnessStream.Dispose()}}catch{Add-RestOwnedError $State 'CLOSE_WITNESS' $_.Exception;$State.CleanupFailure='CLOSE_ERROR'}finally{$State.WitnessReader=$null;$State.WitnessStream=$null}
    try{if($State.Stream){$State.Stream.Dispose()}}catch{Add-RestOwnedError $State 'CLOSE_STREAM' $_.Exception;$State.CleanupFailure='CLOSE_ERROR'}finally{$State.Stream=$null}
    try{if($State.Client){$State.Client.Close()}}catch{Add-RestOwnedError $State 'CLOSE_CLIENT' $_.Exception;$State.CleanupFailure='CLOSE_ERROR'}finally{$State.Client=$null}
    try{if($State.Listener){$State.Listener.Stop()}}catch{Add-RestOwnedError $State 'CLOSE_LISTENER' $_.Exception;$State.CleanupFailure='CLOSE_ERROR'}
    $State.Done=$true;$State.ClosedTicks=[Diagnostics.Stopwatch]::GetTimestamp()
    if($State.Status -ceq 'LISTENING' -or $State.Status -ceq 'READING_HEADERS'){$State.Status='NO_REQUEST_OR_INCOMPLETE'}
    if($State.CleanupFailure){$State.Status='RESPONDER_INVALID'}
}
function Invoke-RestCapabilityStep($State,$Target) {
    if($State.Done){return ''}
    try {
        if(-not [object]::ReferenceEquals($State.Target,$Target) -or $Target.HasExited -or $Target.StartTime.ToUniversalTime().Ticks -ne $State.TargetStart -or $Target.Handle -ne $State.TargetHandle){throw 'ORIGINAL_TARGET_CHANGED'}
        if(-not $State.Client) {
            if(-not $State.Listener.Pending()){return ''}
            $State.Client=$State.Listener.AcceptTcpClient();$State.Accepted=1;$State.Listener.Stop()
            if(-not [Net.IPAddress]::IsLoopback(([Net.IPEndPoint]$State.Client.Client.RemoteEndPoint).Address)){throw 'NON_LOOPBACK_PEER'}
            $State.AcceptedTicks=[Diagnostics.Stopwatch]::GetTimestamp();$State.Status='READING_HEADERS'
            $State.Stream=$State.Client.GetStream();$State.Stream.ReadTimeout=250;$State.Stream.WriteTimeout=250
        }
        if(-not $State.RequestValidated) {
        if(1000.0*([Diagnostics.Stopwatch]::GetTimestamp()-$State.AcceptedTicks)/[Diagnostics.Stopwatch]::Frequency -ge 2000){throw 'HEADER_TIMEOUT'}
        $buffer=New-Object byte[] 1024
        while($State.Stream.DataAvailable) {
            $count=$State.Stream.Read($buffer,0,$buffer.Length)
            if($count -le 0){throw 'PEER_CLOSED'}
            if($State.Header.Count+$count -gt 4096){throw 'HEADER_LIMIT'}
            for($i=0;$i -lt $count;$i++) {
                $value=$buffer[$i]
                if($value -ne 9 -and $value -ne 10 -and $value -ne 13 -and ($value -lt 32 -or $value -gt 126)){throw 'NON_ASCII_HEADER'}
                $State.Header.Add($value)
            }
        }
        $header=[Text.Encoding]::ASCII.GetString($State.Header.ToArray());$end=$header.IndexOf("`r`n`r`n",[StringComparison]::Ordinal)
        if($end -lt 0) {
            if($State.Header.Count -ge 4096){throw 'HEADER_LIMIT'}
            if($State.Client.Client.Poll(0,[Net.Sockets.SelectMode]::SelectRead) -and $State.Client.Client.Available -eq 0){throw 'PEER_CLOSED'}
            return ''
        }
        if($header.Length -ne $end+4){throw 'REQUEST_BODY_NOT_ALLOWED'}
        $lines=$header.Substring(0,$end).Split(@("`r`n"),[StringSplitOptions]::None)
        if($lines[0] -cne ('GET '+$State.ExpectedPath+' HTTP/1.1') -and $lines[0] -cne ('GET '+$State.ExpectedPath+' HTTP/1.0')){throw 'UNEXPECTED_REQUEST'}
        $headers=@{}
        foreach($line in $lines[1..($lines.Length-1)]) {
            $m=[regex]::Match($line,'\A([A-Za-z0-9-]+):[ \t]*([^\r\n]*)\z')
            if(-not $m.Success){throw 'MALFORMED_HEADER'}
            $name=$m.Groups[1].Value.ToLowerInvariant()
            if($headers.ContainsKey($name)){throw 'DUPLICATE_HEADER'}
            $headers[$name]=$m.Groups[2].Value.Trim()
        }
        if($headers['host'] -cne ('127.0.0.1:'+[string]$State.Port)){throw 'UNEXPECTED_HOST'}
        if($headers.ContainsKey('transfer-encoding') -or ($headers.ContainsKey('content-length') -and $headers['content-length'] -cne '0')){throw 'REQUEST_BODY_NOT_ALLOWED'}
        if(1000.0*([Diagnostics.Stopwatch]::GetTimestamp()-$State.AcceptedTicks)/[Diagnostics.Stopwatch]::Frequency -ge 2000){throw 'HEADER_TIMEOUT'}
            $State.RequestValidated=$true;$State.RequestValidatedTicks=[Diagnostics.Stopwatch]::GetTimestamp()
            if($State.GuardedRehearsal -or $State.GuardedCapture){$State.Status='PENDING_RELEASE';$null=$State.ReadyEvent.Set()}
        }
        if(($State.GuardedRehearsal -or $State.GuardedCapture) -and -not $State.ReleaseObservedTicks) {
            if(1000.0*([Diagnostics.Stopwatch]::GetTimestamp()-$State.RequestValidatedTicks)/[Diagnostics.Stopwatch]::Frequency -ge 50000){throw 'PENDING_RELEASE_TIMEOUT'}
            if(-not $State.ReleaseEvent.WaitOne(0)){return ''}
            $State.ReleaseObservedTicks=[Diagnostics.Stopwatch]::GetTimestamp()
            $binding=$null
            if(-not $State.Gate.TryTake([ref]$binding,0)){throw 'MISSING_RELEASE_TUPLE'}
            if($binding.Run -cne $State.Run -or $binding.Attempt -cne $State.Attempt -or $binding.PreparedGeneration -lt 1 -or $binding.WorldGeneration -lt 1 -or
                $binding.TargetId -ne $State.Target.Id -or $binding.TargetStart -ne $State.TargetStart -or $binding.TargetHandle -ne $State.TargetHandle.ToInt64() -or
                $binding.Frequency -ne [Diagnostics.Stopwatch]::Frequency -or $binding.AckTicks -lt $binding.ArmTicks -or $binding.ReleaseTicks -lt $binding.AckTicks -or
                1500.0-1000.0*($binding.ReleaseTicks-$binding.ArmTicks)/$binding.Frequency -lt 1100){throw 'INVALID_RELEASE_TUPLE'}
            if($State.GuardedCapture -and $binding.Mode -cne 'capture'){throw 'RELEASE_MODE_MISMATCH'}
            if([bool]$binding.NoSend -ne $State.NoSend){throw 'RELEASE_CONTROL_MISMATCH'}
            if([bool]$binding.WrongFrequency -ne $State.WrongFrequency){throw 'RELEASE_WRONG_FREQUENCY_MISMATCH'}
            if([bool]$binding.SeparatedPair -ne $State.SeparatedPair){throw 'RELEASE_SEPARATION_MISMATCH'}
            if([bool]$binding.BlindTrial -ne $State.BlindTrial){throw 'RELEASE_BLIND_MISMATCH'}
            if($State.Direct -and $State.BPowerOff -and $binding.ControlTag -cne $State.ControlTag){throw 'RELEASE_COMBINED_CONTROL_MISMATCH'}
            if([bool]$binding.Direct -ne $State.Direct){throw 'RELEASE_DIRECT_MODE_MISMATCH'}
            if([bool]$binding.BPowerOff -ne $State.BPowerOff){throw 'RELEASE_POWER_MODE_MISMATCH'}
            if($State.BPowerOff -and ($binding.PowerPlanId -cne $(if($State.Direct){'BPowerOff.Direct10s.1'}else{'BPowerOff.1'}) -or -not $binding.PowerOffIdentity -or $binding.PowerDeadlineTick-$binding.PowerOffCallTick -ne $(if($State.Direct){10000}else{6000}) -or
                $binding.PowerBTick -lt $binding.PowerOffCallTick -or $binding.PowerBTick -ge $binding.PowerDeadlineTick -or $binding.PowerAObservedTicks -le 0 -or $binding.PowerBObservedTicks -le 0 -or
                $binding.PowerAObservedTicks -gt $binding.ReleaseTicks -or $binding.PowerBObservedTicks -gt $binding.ReleaseTicks)){throw 'INVALID_POWER_OFF_TUPLE'}
            if($State.BlindTrial -and ($binding.BlindChoice -notin @(0,1) -or $binding.PlannedIntendedMs -ne $(if($binding.BlindChoice -eq 0){400}else{1000}) -or $binding.BlindCommitment -cnotmatch '\A[0-9A-F]{64}\z')){throw 'INVALID_BLIND_TUPLE'}
            if(-not $State.BlindTrial -and ([bool]$binding.Short400 -ne $State.Short400 -or ($State.Short400 -and $binding.PlannedIntendedMs -ne 400) -or
                (-not $State.Short400 -and $binding.PlannedIntendedMs -and $binding.PlannedIntendedMs -ne 1000))){throw 'RELEASE_DURATION_MISMATCH'}
            if(($State.SeparatedPair -or $State.Direct) -and (-not $binding.GeometryStamp -or $binding.GeometryObservedATicks -le 0 -or $binding.GeometryObservedBTicks -le 0 -or
                $binding.GeometryVerifiedTicks -lt $binding.GeometryObservedATicks -or $binding.GeometryVerifiedTicks -lt $binding.GeometryObservedBTicks -or
                $binding.GeometryVerifiedTicks -gt $binding.ReleaseTicks)){throw 'INVALID_GEOMETRY_TUPLE'}
            $State.ReleaseTuple=$binding
            $State.WitnessStream=[IO.File]::Open($State.HostLog,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
            if($State.WitnessStream.Length -gt 8388608){throw 'WITNESS_LOG_LIMIT'}
            $State.WitnessStream.Position=$State.WitnessStream.Length
            $State.WitnessReader=New-Object IO.StreamReader($State.WitnessStream,[Text.Encoding]::UTF8)
        }
        $payload=if($State.BlindTrial){$State.Token+'.'+[string]$State.ReleaseTuple.BlindChoice}else{$State.Token}
        $body=[Text.Encoding]::ASCII.GetBytes($payload)
        if($State.BlindTrial -and $body.Length -ne 34){throw 'BLIND_BODY_LENGTH'}
        $response=[Text.Encoding]::ASCII.GetBytes("HTTP/1.1 200 OK`r`nContent-Length: $($body.Length)`r`nContent-Type: text/plain; charset=utf-8`r`nConnection: close`r`n`r`n"+$payload)
        if($State.SeparatedPair -or $State.Direct) {
            $geometryNow=[Diagnostics.Stopwatch]::GetTimestamp();$geometryBinding=$State.ReleaseTuple
            if(1000.0*($geometryNow-$geometryBinding.GeometryObservedATicks)/$geometryBinding.Frequency -gt 2000 -or
                1000.0*($geometryNow-$geometryBinding.GeometryObservedBTicks)/$geometryBinding.Frequency -gt 2000){throw 'GEOMETRY_ADMISSION_EXPIRED'}
        }
        if($State.BPowerOff) {
            $powerNow=[Diagnostics.Stopwatch]::GetTimestamp();$p=$State.ReleaseTuple
            $ageA=1000.0*($powerNow-$p.PowerAObservedTicks)/$p.Frequency;$ageB=1000.0*($powerNow-$p.PowerBObservedTicks)/$p.Frequency
            $State.PowerRemainingAtHttpMs=$p.PowerDeadlineTick-$p.PowerBTick-500-[math]::Ceiling($ageB)
            if($ageA -lt 0 -or $ageB -lt 0 -or $ageA -gt 500 -or $ageB -gt 500 -or $State.PowerRemainingAtHttpMs -lt 2000){throw 'POWER_OFF_BUDGET_OR_STALE_READ'}
        }
        $State.ResponseBeginTicks=[Diagnostics.Stopwatch]::GetTimestamp()
        $State.Stream.Write($response,0,$response.Length);$State.Stream.Flush()
        $State.ResponseReturnTicks=[Diagnostics.Stopwatch]::GetTimestamp();$State.Responses=1;$State.Status='HTTP200_SENT'
        if($State.GuardedRehearsal -or $State.GuardedCapture){$State.Stream.Dispose();$State.Stream=$null;$State.Client.Close();$State.Client=$null;Watch-RestRehearsalDispatch $State}
        Close-RestCapabilityResponder $State
        return ''
    } catch {
        $State.Status='RESPONDER_INVALID';Add-RestOwnedError $State 'REQUEST' $_.Exception -Primary
        Close-RestCapabilityResponder $State
        return 'REST_RESPONDER_INVALID'
    }
}
function Watch-RestRehearsalDispatch($State) {
    $binding=$State.ReleaseTuple;$pending=''
    $until=$binding.ArmTicks+[long]($binding.Frequency*0.4)
    $tag=if($State.GuardedCapture){'RICON_REST_CAPTURE'}else{'RICON_REST_ARM'}
    $suffix=if($State.Direct -and $State.BPowerOff){' control=DIRECT_B_POWER_OFF'}elseif($State.Direct){' control=DIRECT'}elseif($State.BPowerOff){' control=B_POWER_OFF'}elseif($State.BlindTrial){' blindTrial=1'}elseif($State.NoSend){' control=NO_SEND'}elseif($State.WrongFrequency){' control=WRONG_FREQUENCY'}else{''}
    $pattern='\['+$tag+'\] run='+[regex]::Escape($State.Run)+' attempt='+[regex]::Escape($State.Attempt)+' kind=DISPATCH preparedGeneration='+$binding.PreparedGeneration+' beforeTickMs=(?<before>-?[0-9]+) afterTickMs=(?<after>-?[0-9]+) accepted=(?:1|(?i:true)) guardVerifiedByGame=0'+$suffix+'\r?$'
    while([Diagnostics.Stopwatch]::GetTimestamp() -le $until) {
        $pending+=$State.WitnessReader.ReadToEnd()
        if($pending.Length -gt 65536){throw 'WITNESS_SEGMENT_LIMIT'}
        $lines=$pending.Split([char]10);$pending=$lines[$lines.Length-1]
        if($lines.Length -gt 1) {
            foreach($line in $lines[0..($lines.Length-2)]) {
                $m=[regex]::Match($line,$pattern)
                if($m.Success) {
                    $State.DispatchObservedTicks=[Diagnostics.Stopwatch]::GetTimestamp()
                    $State.DispatchBeforeGameMs=[long]$m.Groups['before'].Value;$State.DispatchAfterGameMs=[long]$m.Groups['after'].Value
                    $State.DispatchWitnessValid=($State.DispatchObservedTicks -le $until -and $State.DispatchAfterGameMs -ge $State.DispatchBeforeGameMs)
                    return
                }
            }
        }
        [Threading.Thread]::Sleep(1)
    }
}
function Get-RestCapabilityReceipt($State) {
    [pscustomobject]@{status=$State.Status;failure=$State.Failure;runDigest=$State.RunDigest;payloadDigest=$State.PayloadDigest;
        accepted=$State.Accepted;responses=$State.Responses;headerBytes=$State.Header.Count;done=$State.Done;
        startedTicks=$State.StartedTicks;acceptedTicks=$State.AcceptedTicks;responseBeginTicks=$State.ResponseBeginTicks;responseReturnTicks=$State.ResponseReturnTicks;closedTicks=$State.ClosedTicks;
        qpcFrequency=$State.QpcFrequency;utcAnchor=$State.UtcAnchor;utcAnchorTicks=$State.UtcAnchorTicks;anchorBeforeTicks=$State.AnchorBeforeTicks;anchorAfterTicks=$State.AnchorAfterTicks;
        workerStartedTicks=$State.WorkerStartedTicks;guardedRehearsal=$State.GuardedRehearsal;guardedCapture=$State.GuardedCapture;noSend=$State.NoSend;wrongFrequency=$State.WrongFrequency;separatedPair=$State.SeparatedPair;short400=$State.Short400;
        controlTag=$State.ControlTag;direct=$State.Direct;bPowerOff=$State.BPowerOff;powerOffIdentity=$(if($State.BPowerOff -and $State.ReleaseTuple){$State.ReleaseTuple.PowerOffIdentity}else{''});powerRemainingAtHttpMs=$State.PowerRemainingAtHttpMs;
        blindTrial=$State.BlindTrial;blindCommitment=$(if($State.BlindTrial -and $State.ReleaseTuple){$State.ReleaseTuple.BlindCommitment}else{''});
        plannedIntendedMs=$(if($State.BlindTrial -and $State.ReleaseTuple){$State.ReleaseTuple.PlannedIntendedMs}elseif($State.Short400){400}else{1000});requestValidatedTicks=$State.RequestValidatedTicks;releaseObservedTicks=$State.ReleaseObservedTicks;
        geometryStamp=$(if($State.ReleaseTuple){$State.ReleaseTuple.GeometryStamp}else{''});geometryVerifiedTicks=$(if($State.ReleaseTuple){$State.ReleaseTuple.GeometryVerifiedTicks}else{0});
        dispatchObservedTicks=$State.DispatchObservedTicks;dispatchBeforeGameMs=$State.DispatchBeforeGameMs;dispatchAfterGameMs=$State.DispatchAfterGameMs;dispatchWitnessValid=$State.DispatchWitnessValid;
        preparedGeneration=$(if($State.ReleaseTuple){$State.ReleaseTuple.PreparedGeneration}else{0});worldGeneration=$(if($State.ReleaseTuple){$State.ReleaseTuple.WorldGeneration}else{0});
        guardToDispatchObservedMs=$(if($State.DispatchObservedTicks -and $State.ReleaseTuple){1000.0*($State.DispatchObservedTicks-$State.ReleaseTuple.ArmTicks)/$State.QpcFrequency}else{$null});
        releaseToDispatchObservedMs=$(if($State.DispatchObservedTicks -and $State.ReleaseTuple){1000.0*($State.DispatchObservedTicks-$State.ReleaseTuple.ReleaseTicks)/$State.QpcFrequency}else{$null});
        ownedErrors=@($State.OwnedErrors.ToArray());cleanupFailure=$State.CleanupFailure;
        limits='HEADER4096_BYTES_BODY64_MAX_HEADER2000MS_SYNC_IO250MS';binding='IPV4_LOOPBACK_EPHEMERAL';nativeCallback='UNQUALIFIED';voice='NOT_TESTED';capture=$(if($State.NoSend){'NO_SEND_CONTROL_RELEASE_ONLY'}elseif($State.GuardedCapture){'GAME_REQUEST_NOT_PROVEN'}else{'NOT_REQUESTED'})}
}

function Start-RestCapabilityWorker($State,$Target,[string]$HelperPath,[int]$LifetimeMs) {
    $ps=$null;$stop=$null;$ready=$null;$release=$null;$gate=$null
    try {
        if($LifetimeMs -lt 1 -or $LifetimeMs -gt 600000){throw 'INVALID_WORKER_LIFETIME'}
        $source=[IO.File]::ReadAllText($HelperPath)
        $stop=[Threading.ManualResetEvent]::new($false);$ps=[powershell]::Create()
        if($State.GuardedRehearsal -or $State.GuardedCapture){$ready=[Threading.ManualResetEvent]::new($false);$release=[Threading.ManualResetEvent]::new($false);$gate=[Collections.Concurrent.BlockingCollection[object]]::new(1);$State.ReadyEvent=$ready;$State.ReleaseEvent=$release;$State.Gate=$gate}
        $loop=@'
try {
    $state.WorkerStartedTicks=[Diagnostics.Stopwatch]::GetTimestamp()
    $until=$state.WorkerStartedTicks+[long]($lifetime*[Diagnostics.Stopwatch]::Frequency/1000.0)
    while(-not $state.Done) {
        if($stop.WaitOne(10)){$state.Status='WORKER_STOP_REQUESTED';break}
        if([Diagnostics.Stopwatch]::GetTimestamp() -ge $until){$state.Status='WORKER_LIFETIME_EXPIRED';break}
        $null=Invoke-RestCapabilityStep $state $target
    }
} catch {Add-RestOwnedError $state 'WORKER' $_.Exception -Primary;$state.Status='RESPONDER_INVALID'}
finally {Close-RestCapabilityResponder $state}
Get-RestCapabilityReceipt $state
'@
        $scriptText='param($state,$target,$stop,$lifetime)' + "`n`$ErrorActionPreference='Stop'`n"+$source+"`n"+$loop
        $tokens=$null;$errors=$null;$null=[Management.Automation.Language.Parser]::ParseInput($scriptText,[ref]$tokens,[ref]$errors)
        if($errors.Count){throw 'WORKER_PARSE_ERROR'}
        $null=$ps.AddScript($scriptText).AddArgument($State).AddArgument($Target).AddArgument($stop).AddArgument($LifetimeMs)
        $job=[pscustomobject]@{PowerShell=$ps;Stop=$stop;Ready=$ready;Release=$release;Gate=$gate;Async=$null;Done=$false;Receipt=$null;Run=$State.Run;Attempt=$State.Attempt;Capture=$State.GuardedCapture;NoSend=$State.NoSend;WrongFrequency=$State.WrongFrequency;SeparatedPair=$State.SeparatedPair;Short400=$State.Short400;BlindTrial=$State.BlindTrial;BPowerOff=$State.BPowerOff;Direct=$State.Direct;ControlTag=$State.ControlTag;Target=$Target;TargetStart=$State.TargetStart;TargetHandle=$State.TargetHandle}
        $job.Async=$ps.BeginInvoke()
        return $job # ? ????? ??????? ?????? worker ?????????? ? ????????? ? ?????? responder.
    } catch {
        Add-RestOwnedError $State 'WORKER_STARTUP' $_.Exception -Primary;$State.Status='RESPONDER_INVALID';Close-RestCapabilityResponder $State
        if($ps){$ps.Dispose()};if($stop){$stop.Dispose()};if($ready){$ready.Dispose()};if($release){$release.Dispose()};if($gate){$gate.Dispose()};throw 'REST_WORKER_STARTUP_FAILED'
    }
}
function Complete-RestCapabilityWorker($Job) {
    if($Job.Done){return $Job.Receipt}
    $null=$Job.Stop.Set()
    if(-not $Job.Async.AsyncWaitHandle.WaitOne(3000)) {
        # ?? ????????? ? ?? ??????????? ??????? ??????????? ? ????????? ??? worker.
        $Job.Receipt=[pscustomobject]@{status='WORKER_UNJOINED';failure='WORKER_JOIN_TIMEOUT';nativeCallback='UNQUALIFIED';capture='NOT_REQUESTED'}
        return $Job.Receipt
    }
    try {
        $output=@($Job.PowerShell.EndInvoke($Job.Async))
        if($output.Count -ne 1 -or -not $output[0].status){throw 'INVALID_WORKER_RECEIPT'}
        $Job.Receipt=$output[0]
    } catch {
        $safe=@();$e=$_.Exception
        while($e){$safe+=[pscustomobject]@{type=$e.GetType().FullName;hResult=$e.HResult};$e=$e.InnerException}
        $Job.Receipt=[pscustomobject]@{status='WORKER_JOIN_INVALID';failure='WORKER_JOIN_ERROR';ownedErrors=$safe;nativeCallback='UNQUALIFIED';capture='NOT_REQUESTED'}
    } finally {$Job.Done=$true;$Job.PowerShell.Dispose();$Job.Stop.Dispose();if($Job.Ready){$Job.Ready.Dispose()};if($Job.Release){$Job.Release.Dispose()};if($Job.Gate){$Job.Gate.Dispose()}}
    return $Job.Receipt
}
