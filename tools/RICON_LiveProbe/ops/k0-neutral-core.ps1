# TOOL_ONLY N0. Definitions only: no clock sampling, IO, dot-source or process operations.
# Plan 2880204; runtime gates are independent. All clocks/bytes/facts are explicit inputs.
function New-N0State {
    param([string]$RunId, [string]$OwnerToken, [string]$Scenario,
        [long]$Frequency, [long]$T0, [int]$HelperPid = 0)
    if ($RunId -cnotmatch '^[0-9a-f]{32}$' -or $OwnerToken -cnotmatch '^[0-9a-f]{32}$' -or
        $Scenario -cnotin @('Normal','DelayEmit','Hanging') -or $Frequency -le 0 -or $T0 -lt 0) {
        throw 'N0_BAD_STATE_ARGUMENTS'
    }
    # Decimal intermediate prevents Int64 multiplication/addition overflow.
    $absolute = [decimal]$T0 + [decimal]$Frequency * 10
    if ($absolute -gt [long]::MaxValue) { throw 'N0_CLOCK_OVERFLOW' }
    return @{
        RunId=$RunId; OwnerToken=$OwnerToken; Scenario=$Scenario; Frequency=$Frequency
        T0=$T0; LastTick=$T0; AbsoluteDeadline=[long]$absolute; Phase='STARTING'
        HelperPid=$HelperPid; HasHandle=$false; NextSeq=0; Records=0; Samples=0
        HelloRead=$null; FirstDeadline=$null; ProbeDeadline=$null; LastEmit=$null; Done=$false
        SampleSummaries=(New-Object 'System.Collections.Generic.List[object]')
        InvalidReason=''; InvalidTick=$null; DisqualifyingFault=''; ProtocolResult='NOT_OBSERVED'
        SampleObservation='NOT_OBSERVED'; CommandAttempted=$false; CommandSuccess=$false
        CommandBegin=$null; CommandReturn=$null; CommandDeadline=$null; StopReason=''
        NaturalExit=$false; ExitConfirmed=$false; ExitTick=$null; CleanupResult='NOT_RUN'
        Integrity=$true; ClockValid=$true; WireFinalValid=$false; TailComplete=$false
        Ordinary=0; ReserveInvalid=0; ReserveStop=0; ReserveCleanup=0
        RunnerBytes=0; WireBytes=0; DiagnosticPrefix=''; RawComplete=$true
    }
}

function Test-N0Record {
    param([byte[]]$Bytes, [hashtable]$State, [string]$Mode='Wire', [hashtable]$Facts)
    # All results have the same shape. Wire accepts a deliberately canonical flat JSON grammar:
    # strings without escapes (all schema values have an ASCII representation), integer tokens,
    # and true/false. Escaped property aliases are rejected, never normalised past duplicate checks.
    $result = @{ Valid=$false; Reason='INVALID_PROTOCOL'; Record=$null }
    if ($Mode -eq 'Arguments') {
        $result.Valid = $Facts.Enabled -and $Facts.Scenario -cin @('Normal','DelayEmit','Hanging') -and
            -not $Facts.UnknownParameters
        if ($result.Valid) { $result.Reason='OK' } else { $result.Reason='N0_BAD_ARGUMENTS' }
        return $result
    }
    if ($Mode -eq 'PathFacts') {
        $result.Valid = $Facts.Local -and $Facts.Exists -and -not $Facts.Reparse -and
            -not $Facts.InCheckout -and -not $Facts.UnsafeCharacters -and -not $Facts.OutputExists
        if ($result.Valid) { $result.Reason='OK' } else { $result.Reason='N0_UNSAFE_PATH' }
        return $result
    }
    if ($Mode -eq 'Serialized') {
        $result.Valid = $Bytes -and $Bytes.Length -le $Facts.Limit -and $Bytes.Length -gt 0
        if ($result.Valid) { $result.Reason='OK' } else { $result.Reason='INVALID_QUOTA' }
        return $result
    }
    if ($Mode -ne 'Wire' -or -not $State) { return $result }
    if (-not $Bytes -or $Bytes.Length -gt 1024) { $result.Reason='INVALID_QUOTA'; return $result }
    if ($Bytes[$Bytes.Length-1] -ne 10) { $result.Reason='PARTIAL_RECORD'; return $result }
    try {
        $encoding = New-Object System.Text.UTF8Encoding($false, $true)
        $text = $encoding.GetString($Bytes, 0, $Bytes.Length-1)
    } catch { $result.Reason='INVALID_UTF8'; return $result }
    if ($text -match '[\x00-\x1f\x7f-\uffff]' -or $text -notmatch '^ *\{.*\} *$') { return $result }
    $body = $text.Trim(' ')
    $body = $body.Substring(1,$body.Length-2)
    $values = New-Object 'System.Collections.Generic.Dictionary[string,object]' ([StringComparer]::Ordinal)
    $kinds = New-Object 'System.Collections.Generic.Dictionary[string,string]' ([StringComparer]::Ordinal)
    $offset=0
    $pattern='\G *"(?<key>[^"\\\x00-\x1f]*)" *: *(?:"(?<s>[^"\\\x00-\x1f]*)"|(?<n>-?(?:0|[1-9][0-9]*))|(?<b>true|false)) *(?<end>,|$)'
    $rx=New-Object System.Text.RegularExpressions.Regex($pattern,
        [System.Text.RegularExpressions.RegexOptions]::None,[timespan]::FromMilliseconds(50))
    while ($offset -lt $body.Length) {
        # \G prevents scanning past malformed bytes; timeout is bounded by the 1024-byte input.
        try { $match=$rx.Match($body,$offset) }
        catch { $result.Reason='INVALID_PROTOCOL'; return $result }
        if (-not $match.Success -or $match.Index -ne $offset) { return $result }
        $key=$match.Groups['key'].Value
        if ($key.Length -gt 128 -or $values.ContainsKey($key) -or $values.Count -ge 16) { return $result }
        if ($match.Groups['s'].Success) {
            $value=$match.Groups['s'].Value
            if ($value.Length -gt 128) { return $result }
            $values.Add($key,$value); $kinds.Add($key,'string')
        } elseif ($match.Groups['n'].Success) {
            [long]$number=0
            if (-not [long]::TryParse($match.Groups['n'].Value,
                [Globalization.NumberStyles]::AllowLeadingSign,[Globalization.CultureInfo]::InvariantCulture,[ref]$number)) {
                $result.Reason='INVALID_INT64'; return $result
            }
            $values.Add($key,$number); $kinds.Add($key,'integer')
        } else {
            $values.Add($key,($match.Groups['b'].Value -ceq 'true')); $kinds.Add($key,'boolean')
        }
        $offset += $match.Length
        if ($match.Groups['end'].Value -eq ',' -and $offset -ge $body.Length) { return $result }
    }
    $common=@('schema','runId','ownerToken','helperPid','scenario','frequency','highResolution','kind','seq')
    foreach ($name in $common) { if (-not $values.ContainsKey($name)) { return $result } }
    foreach ($name in @('schema','helperPid','seq')) { if ($kinds[$name] -ne 'integer') { return $result } }
    foreach ($name in @('runId','ownerToken','scenario','frequency','kind')) {
        if ($kinds[$name] -ne 'string') { return $result }
    }
    if ($kinds['highResolution'] -ne 'boolean' -or -not $values['highResolution']) {
        $result.Reason='INVALID_CLOCK'; return $result
    }
    if ($values['schema'] -ne 1 -or $values['helperPid'] -le 0 -or $values['helperPid'] -gt [int]::MaxValue -or
        $values['helperPid'] -ne $State.HelperPid -or $values['runId'] -cne $State.RunId -or
        $values['ownerToken'] -cne $State.OwnerToken -or $values['scenario'] -cne $State.Scenario -or
        $values['seq'] -lt 0 -or $values['seq'] -gt 15) { return $result }
    [long]$freq=0
    if ($values['frequency'] -cnotmatch '^[1-9][0-9]*$' -or
        -not [long]::TryParse($values['frequency'],[ref]$freq) -or $freq -ne $State.Frequency) {
        $result.Reason='INVALID_CLOCK'; return $result
    }
    $allowed=@($common)
    switch -CaseSensitive ($values['kind']) {
        'N0_HELLO' { }
        'N0_SAMPLE' {
            $allowed+=@('tEmit','payload')
            if (-not $values.ContainsKey('tEmit') -or -not $values.ContainsKey('payload') -or
                $kinds['tEmit'] -ne 'string' -or $kinds['payload'] -ne 'string' -or
                $values['payload'] -cne 'N0_NEUTRAL') { return $result }
            [long]$emit=0
            if ($values['tEmit'] -cnotmatch '^(0|[1-9][0-9]*)$' -or
                -not [long]::TryParse($values['tEmit'],[ref]$emit)) {
                $result.Reason='INVALID_INT64'; return $result
            }
        }
        'N0_DONE' {
            $allowed+=@('sampleCount')
            if (-not $values.ContainsKey('sampleCount') -or $kinds['sampleCount'] -ne 'integer' -or
                $values['sampleCount'] -ne 8) { return $result }
        }
        default { return $result }
    }
    if ($values.Count -ne $allowed.Count) { return $result }
    foreach ($key in $values.Keys) { if ($allowed -cnotcontains $key) { return $result } }
    $result.Valid=$true; $result.Reason='OK'; $result.Record=$values
    return $result
}

function Update-N0State {
    param([hashtable]$State, [string]$Event, [long]$Now, [hashtable]$Data=@{})
    $reason=''
    if ($Now -lt $State.LastTick) {
        $reason='INVALID_CLOCK'; $State.ClockValid=$false
        if (-not $State.DisqualifyingFault) { $State.DisqualifyingFault='INVALID_CLOCK' }
    } else { $State.LastTick=$Now }
    switch ($Event) {
        'Ownership' {
            if (-not $Data.HasHandle -or $Data.HelperPid -le 0) { $reason='BLOCKED_OWNERSHIP' }
            else { $State.HasHandle=$true; $State.HelperPid=[int]$Data.HelperPid }
        }
        'Invalidate' { $reason=[string]$Data.Reason }
        'Record' {
            $check=Test-N0Record -Bytes $Data.Bytes -State $State
            if (-not $check.Valid) { $reason=$check.Reason; break }
            $r=$check.Record
            if ($State.CommandAttempted -or $State.InvalidReason) { $reason='INVALID_LATE_RECORD'; break }
            if ($r['seq'] -ne $State.NextSeq -or $State.Records -ge 16) { $reason='INVALID_PROTOCOL'; break }
            $kind=$r['kind']
            if ($State.NextSeq -eq 0) {
                if ($kind -cne 'N0_HELLO' -or $Now -ge $State.AbsoluteDeadline -or
                    [decimal]$Now - $State.T0 -ge [decimal]$State.Frequency*5) { $reason='INVALID_STARTUP'; break }
                $State.HelloRead=$Now
                $probe=[decimal]$Now + [decimal]$State.Frequency*2
                $first=[decimal]$Now + [decimal]::Ceiling([decimal]$State.Frequency/2)
                if ($probe -gt [long]::MaxValue) { $reason='INVALID_CLOCK'; break }
                $State.ProbeDeadline=[long]$probe; $State.FirstDeadline=[long]$first
                $State.Phase='OBSERVING'; $State.ProtocolResult='OBSERVING'
            } elseif ($State.NextSeq -le 8) {
                if ($kind -cne 'N0_SAMPLE') { $reason='INVALID_PROTOCOL'; break }
                if ($Now -ge $State.AbsoluteDeadline -or $Now -ge $State.ProbeDeadline) {
                    $reason='INVALID_LATE_RECORD'; break
                }
                if ($State.Samples -eq 0 -and $Now -ge $State.FirstDeadline) {
                    $reason='INVALID_MISSING_SAMPLE'; break
                }
                $emitTick=[long]$r['tEmit']
                if ($null -ne $State.LastEmit -and $emitTick -lt $State.LastEmit) { $reason='INVALID_CLOCK'; break }
                $State.LastEmit=$emitTick
                $delta=[decimal]$Now - [decimal]$emitTick
                if ($delta -lt -1) { $reason='INVALID_CLOCK'; break }
                if ($delta -le 1) { $reason='ORDER_AMBIGUOUS'; break }
                $ms=($delta*1000)/[decimal]$State.Frequency
                $State.Samples++
                $State.SampleSummaries.Add(@{ seq=[long]$r['seq']; tEmit=$r['tEmit'];
                    tRead=$Now.ToString([Globalization.CultureInfo]::InvariantCulture); deltaTicks=$delta.ToString();
                    observedMs=$ms.ToString([Globalization.CultureInfo]::InvariantCulture) })
                if ($ms -gt 250) { $reason='INVALID_SAMPLE_THRESHOLD' }
                $State.SampleObservation='OBSERVED'
            } elseif ($State.NextSeq -eq 9) {
                if ($kind -cne 'N0_DONE' -or $State.Samples -ne 8) { $reason='INVALID_PROTOCOL'; break }
                if ($Now -ge $State.AbsoluteDeadline -or $Now -ge $State.ProbeDeadline) {
                    $reason='INVALID_LATE_RECORD'; break
                }
                $State.Done=$true; $State.ProtocolResult='VALID_COMPLETE'
            } else { $reason='INVALID_PROTOCOL'; break }
            $State.NextSeq++; $State.Records++
        }
        'Log' {
            $bucket=[string]$Data.Bucket; $size=[int]$Data.Bytes
            $limit=64
            if ($bucket -eq 'ReserveInvalid' -or $bucket -eq 'ReserveStop') { $limit=4 }
            elseif ($bucket -eq 'ReserveCleanup') { $limit=8 }
            elseif ($bucket -ne 'Ordinary') { $reason='INVALID_QUOTA'; break }
            if ($size -le 0 -or $size -gt 2048 -or $State[$bucket] -ge $limit -or
                $State.RunnerBytes+$size -gt 163840) { $reason='INVALID_QUOTA'; break }
            $State[$bucket]++; $State.RunnerBytes+=$size
        }
        'CommandBegin' {
            if (-not $State.HasHandle -or $State.CommandAttempted -or $State.NaturalExit) {
                $reason='BLOCKED_OWNERSHIP'; break
            }
            $State.CommandAttempted=$true; $State.CommandBegin=$Now
            $State.CommandDeadline=[long]$Data.Deadline; $State.StopReason=[string]$Data.Reason
            $State.Phase='STOP_REQUESTED'
        }
        'CommandReturn' {
            if (-not $State.CommandAttempted -or $State.CommandReturn -ne $null) { $reason='INVALID_COMMAND'; break }
            $State.CommandReturn=$Now; $State.CommandSuccess=[bool]$Data.Success
        }
        'NaturalExit' { $State.NaturalExit=$true; $State.Phase='EXIT_CONFIRMED' }
        'Exit' {
            $State.ExitConfirmed=[bool]$Data.Confirmed
            if ($State.ExitConfirmed) { $State.ExitTick=$Now; $State.CleanupResult='EXIT_CONFIRMED'; $State.Phase='EXIT_CONFIRMED' }
            else { $State.CleanupResult='INCONCLUSIVE_CLEANUP' }
        }
        'IntegrityFailure' { $State.Integrity=$false }
        default { $reason='INVALID_EVENT' }
    }
    if ($reason -eq 'INVALID_CLOCK') { $State.ClockValid=$false }
    if ($reason) {
        $expectedFault=($reason -eq 'INVALID_MISSING_SAMPLE' -and $State.Scenario -cin @('Hanging','DelayEmit')) -or
            ($reason -eq 'INVALID_SAMPLE_THRESHOLD' -and $State.Scenario -ceq 'DelayEmit')
        # Keep the first cause, but never let an expected timeout hide a later unrelated fault.
        if ((-not $expectedFault -or ($State.InvalidReason -and $reason -cne $State.InvalidReason)) -and
            -not $State.DisqualifyingFault) { $State.DisqualifyingFault=$reason }
    }
    if ($reason -and -not $State.InvalidReason) {
        $State.InvalidReason=$reason; $State.InvalidTick=$Now; $State.Phase='INVALID'
        if ($reason -eq 'INVALID_PROTOCOL' -or $reason -eq 'INVALID_UTF8' -or $reason -eq 'INVALID_INT64') {
            $State.ProtocolResult='INVALID'
        }
    }
    return $State
}

function Get-N0DeadlineDecision {
    param([hashtable]$State, [long]$Now)
    $decision=@{ Stop=$false; Authority=$State.HasHandle; Deadline=$null; Reason=''; WakeTick=$Now }
    if ($State.CommandAttempted -or $State.NaturalExit) { return $decision }
    if ($Now -lt $State.LastTick) { $decision.Deadline=$Now; $decision.Reason='INVALID_CLOCK' }
    elseif ($State.InvalidReason) { $decision.Deadline=$State.InvalidTick; $decision.Reason=$State.InvalidReason }
    else {
        $deadline=$State.AbsoluteDeadline; $reason='INVALID_ABSOLUTE_TIMEOUT'
        if ($null -eq $State.HelloRead) {
            $startup=[long]([decimal]$State.T0+[decimal]$State.Frequency*5)
            if ($startup -lt $deadline) { $deadline=$startup; $reason='INVALID_STARTUP' }
        } else {
            if ($State.Samples -eq 0 -and $State.FirstDeadline -lt $deadline) {
                $deadline=$State.FirstDeadline; $reason='INVALID_MISSING_SAMPLE'
            }
            if ($State.ProbeDeadline -lt $deadline) { $deadline=$State.ProbeDeadline; $reason='PROBE_COMPLETE' }
        }
        if ($Now -ge $deadline) { $decision.Deadline=$deadline; $decision.Reason=$reason }
    }
    if ($null -ne $decision.Deadline) { $decision.Stop=[bool]$State.HasHandle }
    return $decision
}

function Get-N0Verdict {
    param([hashtable]$State)
    $command='NOT_OBSERVED'; $sample=$State.SampleObservation
    if ($State.CommandAttempted -and $State.CommandSuccess -and $null -ne $State.CommandReturn -and
        $null -ne $State.CommandBegin -and $null -ne $State.CommandDeadline) {
        $dispatch=([decimal]$State.CommandReturn-[decimal]$State.CommandDeadline)*1000/[decimal]$State.Frequency
        if ($dispatch -ge 0 -and $dispatch -le 100 -and $State.CommandBegin -ge $State.CommandDeadline -and
            $State.CommandReturn -ge $State.CommandBegin) { $command='OBSERVED_WITHIN_RUN_THRESHOLD' }
        else { $command='THRESHOLD_EXCEEDED' }
    }
    if ($State.NaturalExit) { $command='NATURAL_EXIT_NO_COMMAND' }
    $negativeTrace=$false
    if ($State.HasHandle -and -not $State.NaturalExit -and -not $State.Done -and $null -ne $State.HelloRead -and
        $null -ne $State.InvalidTick -and $null -ne $State.CommandBegin -and $State.CommandBegin -ge $State.InvalidTick) {
        $timeoutTrace=$State.Scenario -cin @('Hanging','DelayEmit') -and
            $State.InvalidReason -ceq 'INVALID_MISSING_SAMPLE' -and $State.StopReason -ceq 'INVALID_MISSING_SAMPLE' -and
            $State.Samples -eq 0 -and $State.Records -eq 1 -and $State.NextSeq -eq 1 -and
            $State.SampleSummaries.Count -eq 0 -and $State.InvalidTick -ge $State.FirstDeadline -and
            $State.CommandDeadline -eq $State.FirstDeadline
        $thresholdTrace=$false
        if ($State.Scenario -ceq 'DelayEmit' -and $State.InvalidReason -ceq 'INVALID_SAMPLE_THRESHOLD' -and
            $State.StopReason -ceq 'INVALID_SAMPLE_THRESHOLD' -and $State.Samples -eq 1 -and
            $State.Records -eq 2 -and $State.NextSeq -eq 2 -and $State.SampleSummaries.Count -eq 1 -and
            $State.CommandDeadline -eq $State.InvalidTick) {
            $summary=$State.SampleSummaries[0]; [long]$readTick=0; [long]$emitTick=0; [decimal]$observedMs=0
            $thresholdTrace=[long]::TryParse($summary.tRead,[ref]$readTick) -and
                [long]::TryParse($summary.tEmit,[ref]$emitTick) -and
                [decimal]::TryParse($summary.observedMs,[Globalization.NumberStyles]::Number,
                    [Globalization.CultureInfo]::InvariantCulture,[ref]$observedMs) -and
                $summary.seq -eq 1 -and $observedMs -gt 250 -and $readTick -eq $State.InvalidTick -and
                $readTick -lt $State.FirstDeadline -and $emitTick -ge $State.T0 -and $emitTick -lt $readTick
        }
        $negativeTrace=$timeoutTrace -or $thresholdTrace
    }
    $cleanup=$State.CleanupResult
    if ($State.CommandAttempted -and $State.ExitConfirmed -and $null -ne $State.CommandBegin) {
        $exitMs=([decimal]$State.ExitTick-[decimal]$State.CommandBegin)*1000/[decimal]$State.Frequency
        if ($exitMs -lt 0 -or $exitMs -gt 5000) { $cleanup='INCONCLUSIVE_CLEANUP' }
    }
    $verdict='N0_INVALID'; $expectation='NOT_MET'
    if (-not $State.ExitConfirmed -or $cleanup -ne 'EXIT_CONFIRMED') {
        $verdict='INCONCLUSIVE_CLEANUP'
    } elseif (-not $State.Integrity -or -not $State.WireFinalValid -or -not $State.TailComplete) {
        $verdict='INCONCLUSIVE_LOG_INTEGRITY'
    } elseif (-not $State.ClockValid) { $verdict='N0_INVALID'; $command='INVALID_CLOCK' }
    elseif ($State.DisqualifyingFault) { $verdict='N0_INVALID' }
    elseif ($State.Scenario -ceq 'Normal' -and -not $State.InvalidReason -and $State.Done -and
        $State.Samples -eq 8 -and $State.Records -eq 10 -and $State.StopReason -eq 'PROBE_COMPLETE' -and
        $command -eq 'OBSERVED_WITHIN_RUN_THRESHOLD') {
        $verdict='N0_NEUTRAL_OBSERVED'; $expectation='MET_THIS_RUN'
    } elseif ($negativeTrace -and $State.CommandAttempted -and $State.CommandSuccess -and
        $command -eq 'OBSERVED_WITHIN_RUN_THRESHOLD') {
        $verdict='N0_NEGATIVE_DETECTED'; $expectation='EXPECTED_FAILURE_DETECTED_NOT_POSITIVE_QUALIFICATION'
    }
    return @{ verdict=$verdict; protocolResult=$State.ProtocolResult; sampleObservation=$sample
        commandObservation=$command; cleanupResult=$cleanup; scenarioExpectation=$expectation
        reason=$State.InvalidReason; disqualifyingFault=$State.DisqualifyingFault
        hardRealtimeGuarantee=$false; gameQualification=$false }
}
