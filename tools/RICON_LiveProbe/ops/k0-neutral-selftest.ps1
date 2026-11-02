# TOOL_ONLY N0 deterministic vectors. Written under G_WRITE; execution needs G_PURE.
[CmdletBinding()]
param([switch]$EnableN0,[string]$Mode)
if (-not $EnableN0) { Write-Output 'N0_DISABLED'; return }
if ($Mode -cne 'Pure') { throw 'N0_BAD_MODE' }
Set-StrictMode -Version 2.0
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'k0-neutral-core.ps1')
$script:cases=New-Object 'System.Collections.Generic.List[object]'
$encoding=New-Object Text.UTF8Encoding($false,$true)
function New-N0Fake {
    param([string]$Scenario='Normal')
    $s=New-N0State ('a'*32) ('b'*32) $Scenario 1000 10000 42
    return (Update-N0State $s 'Ownership' 10000 @{HasHandle=$true;HelperPid=42})
}
function New-N0FakeWire([int]$Sequence,[string]$Kind,[hashtable]$Changes=@{}) {
    $r=[ordered]@{schema=1;runId=('a'*32);ownerToken=('b'*32);helperPid=42;scenario='Normal'
        frequency='1000';highResolution=$true;kind=$Kind;seq=$Sequence}
    if ($Kind -ceq 'N0_SAMPLE') { $r.tEmit='10100'; $r.payload='N0_NEUTRAL' }
    if ($Kind -ceq 'N0_DONE') { $r.sampleCount=8 }
    foreach ($k in $Changes.Keys) { $r[$k]=$Changes[$k] }
    return ,$encoding.GetBytes(($r | ConvertTo-Json -Compress)+"`n")
}
function Test-N0Case([string]$Name,[scriptblock]$Body) {
    $passed=$false; $errorName=''
    try {
        $answers=@(& $Body)
        $passed=($answers.Count -eq 1 -and $answers[0] -is [bool] -and $answers[0])
    } catch { $errorName=$_.Exception.GetType().Name }
    $script:cases.Add(@{name=$Name;passed=$passed;error=$errorName})
}
function New-N0CompleteFake {
    $s=New-N0Fake
    $s=Update-N0State $s 'Record' 10001 @{Bytes=(New-N0FakeWire 0 'N0_HELLO')}
    for ($i=1;$i -le 8;$i++) {
        $read=10001+$i*100
        $s=Update-N0State $s 'Record' $read @{Bytes=(New-N0FakeWire $i 'N0_SAMPLE' @{tEmit=($read-10).ToString()})}
    }
    $s=Update-N0State $s 'Record' 10901 @{Bytes=(New-N0FakeWire 9 'N0_DONE')}
    $s=Update-N0State $s 'CommandBegin' 12001 @{Deadline=12001;Reason='PROBE_COMPLETE'}
    $s=Update-N0State $s 'CommandReturn' 12051 @{Success=$true}
    $s=Update-N0State $s 'Exit' 12080 @{Confirmed=$true}
    $s.WireFinalValid=$true; $s.TailComplete=$true
    return $s
}

function New-N0NegativeFake {
    param([string]$Scenario='Hanging',[string]$Cause='Timeout',[int]$ReturnOffset=20,
        [int]$DeadlineOffset=0,[string]$CommandReason='')
    $s=New-N0Fake $Scenario
    $helloRead=10001
    if ($Cause -ceq 'Threshold') { $helloRead=10500 }
    $s=Update-N0State $s 'Record' $helloRead @{Bytes=(New-N0FakeWire 0 'N0_HELLO' @{scenario=$Scenario})}
    if ($Cause -ceq 'Timeout') {
        $reference=[long]$s.FirstDeadline
        $decision=Get-N0DeadlineDecision $s $reference
        $s=Update-N0State $s 'Invalidate' $reference @{Reason=$decision.Reason}
    } elseif ($Cause -ceq 'Threshold') {
        # HELLO can be read after helper emitted SAMPLE1. This fake injects the specified
        # 600 ms emit-to-read delay, while the read still precedes HELLO-read + 500 ms.
        $reference=10700
        $s=Update-N0State $s 'Record' $reference @{Bytes=(New-N0FakeWire 1 'N0_SAMPLE' @{scenario=$Scenario;tEmit='10100'})}
        $decision=Get-N0DeadlineDecision $s ($reference+5)
    } else { throw 'N0_BAD_FAKE_CAUSE' }
    $reason=$decision.Reason
    if ($CommandReason) { $reason=$CommandReason }
    $s=Update-N0State $s 'CommandBegin' ($decision.WakeTick+10) @{Deadline=($decision.Deadline+$DeadlineOffset);Reason=$reason}
    $s=Update-N0State $s 'CommandReturn' ($reference+$ReturnOffset) @{Success=$true}
    $s=Update-N0State $s 'Exit' ($reference+$ReturnOffset+30) @{Confirmed=$true}
    $s.WireFinalValid=$true; $s.TailComplete=$true
    return $s
}

# Fake facts only: these cases never inspect a path, process, host clock or filesystem.
Test-N0Case 'disabled-facts-have-no-authority' { -not (Test-N0Record -Mode Arguments -Facts @{Enabled=$false;Scenario='Normal';UnknownParameters=$false}).Valid }
Test-N0Case 'enabled-normal-arguments' { (Test-N0Record -Mode Arguments -Facts @{Enabled=$true;Scenario='Normal';UnknownParameters=$false}).Valid }
Test-N0Case 'unknown-scenario-rejected' { -not (Test-N0Record -Mode Arguments -Facts @{Enabled=$true;Scenario='Other';UnknownParameters=$false}).Valid }
Test-N0Case 'unknown-parameter-rejected' { -not (Test-N0Record -Mode Arguments -Facts @{Enabled=$true;Scenario='Normal';UnknownParameters=$true}).Valid }
foreach ($field in @('Reparse','InCheckout','UnsafeCharacters','OutputExists')) {
    Test-N0Case ('path-reject-'+$field) {
        $f=@{Local=$true;Exists=$true;Reparse=$false;InCheckout=$false;UnsafeCharacters=$false;OutputExists=$false}
        $f[$field]=$true; -not (Test-N0Record -Mode PathFacts -Facts $f).Valid
    }
}
Test-N0Case 'path-reject-nonlocal' { -not (Test-N0Record -Mode PathFacts -Facts @{Local=$false;Exists=$true;Reparse=$false;InCheckout=$false;UnsafeCharacters=$false;OutputExists=$false}).Valid }
Test-N0Case 'path-reject-missing-root' { -not (Test-N0Record -Mode PathFacts -Facts @{Local=$true;Exists=$false;Reparse=$false;InCheckout=$false;UnsafeCharacters=$false;OutputExists=$false}).Valid }
Test-N0Case 'hello-valid' { (Test-N0Record -Bytes (New-N0FakeWire 0 'N0_HELLO') -State (New-N0Fake)).Valid }
Test-N0Case 'sample-valid' { (Test-N0Record -Bytes (New-N0FakeWire 1 'N0_SAMPLE') -State (New-N0Fake)).Valid }
Test-N0Case 'normal-model-complete' { (Get-N0Verdict (New-N0CompleteFake)).verdict -eq 'N0_NEUTRAL_OBSERVED' }
foreach ($pair in @(
    @{name='wrong-run';key='runId';value=('c'*32)}, @{name='wrong-token';key='ownerToken';value=('c'*32)},
    @{name='wrong-pid';key='helperPid';value=43}, @{name='pid-string';key='helperPid';value='42'},
    @{name='wrong-schema';key='schema';value=2}, @{name='schema-string';key='schema';value='1'},
    @{name='frequency-mismatch';key='frequency';value='1001'}, @{name='frequency-number';key='frequency';value=1000},
    @{name='frequency-overflow';key='frequency';value='9223372036854775808'},
    @{name='not-high-resolution';key='highResolution';value=$false}, @{name='bool-string';key='highResolution';value='true'},
    @{name='unknown-field';key='extension';value='forbidden'}, @{name='unknown-kind';key='kind';value='N0_OTHER'},
    @{name='seq-float';key='seq';value=0.5}, @{name='seq-string';key='seq';value='0'},
    @{name='field-overlength';key='runId';value=('a'*129)})) {
    Test-N0Case $pair.name {
        $changes=@{}; $changes[$pair.key]=$pair.value
        -not (Test-N0Record -Bytes (New-N0FakeWire 0 'N0_HELLO' $changes) -State (New-N0Fake)).Valid
    }
}
Test-N0Case 'duplicate-json-property' {
    $b=New-N0FakeWire 0 'N0_HELLO'; $s=$encoding.GetString($b).Replace('"schema":1','"schema":1,"schema":1')
    -not (Test-N0Record -Bytes $encoding.GetBytes($s) -State (New-N0Fake)).Valid
}
Test-N0Case 'escaped-property-alias-rejected' {
    $b=New-N0FakeWire 0 'N0_HELLO'; $s=$encoding.GetString($b).Replace('"schema":1','"schema":1,"\u0073chema":1')
    -not (Test-N0Record -Bytes $encoding.GetBytes($s) -State (New-N0Fake)).Valid
}
Test-N0Case 'case-alias-unknown-field' {
    $b=New-N0FakeWire 0 'N0_HELLO'; $s=$encoding.GetString($b).Replace('"schema":1','"schema":1,"Schema":1')
    -not (Test-N0Record -Bytes $encoding.GetBytes($s) -State (New-N0Fake)).Valid
}
Test-N0Case 'unicode-soft-hyphen-kind-alias-rejected' {
    $alias='N0_HE'+[char]0x00ad+'LLO'
    -not (Test-N0Record -Bytes (New-N0FakeWire 0 $alias) -State (New-N0Fake)).Valid
}
Test-N0Case 'unicode-ignorable-run-alias-rejected' {
    $alias=('a'*16)+[char]0x200b+('a'*16)
    -not (Test-N0Record -Bytes (New-N0FakeWire 0 'N0_HELLO' @{runId=$alias}) -State (New-N0Fake)).Valid
}
foreach ($size in @(1023,1024,1025)) {
    Test-N0Case ('wire-byte-boundary-'+$size) {
        $b=New-N0FakeWire 0 'N0_HELLO'; $text=$encoding.GetString($b).TrimEnd("`n")
        $text+=(' '*($size-$b.Length))+"`n"
        $valid=(Test-N0Record -Bytes $encoding.GetBytes($text) -State (New-N0Fake)).Valid
        $valid -eq ($size -le 1024)
    }
}
Test-N0Case 'missing-lf' { (Test-N0Record -Bytes $encoding.GetBytes('{}') -State (New-N0Fake)).Reason -eq 'PARTIAL_RECORD' }
Test-N0Case 'partial-utf8' { (Test-N0Record -Bytes ([byte[]]@(123,194,10)) -State (New-N0Fake)).Reason -eq 'INVALID_UTF8' }
Test-N0Case 'malformed-json' { -not (Test-N0Record -Bytes $encoding.GetBytes("{bad}`n") -State (New-N0Fake)).Valid }
Test-N0Case 'trailing-comma' {
    $s=$encoding.GetString((New-N0FakeWire 0 'N0_HELLO')).Replace('}',',}')
    -not (Test-N0Record -Bytes $encoding.GetBytes($s) -State (New-N0Fake)).Valid
}
Test-N0Case 'seq-gap' { $s=New-N0Fake; $s=Update-N0State $s 'Record' 10001 @{Bytes=(New-N0FakeWire 1 'N0_SAMPLE')}; $s.InvalidReason -eq 'INVALID_PROTOCOL' }
Test-N0Case 'seq-duplicate' {
    $s=New-N0Fake; $b=New-N0FakeWire 0 'N0_HELLO'; $s=Update-N0State $s 'Record' 10001 @{Bytes=$b}
    $s=Update-N0State $s 'Record' 10002 @{Bytes=$b}; $s.InvalidReason -eq 'INVALID_PROTOCOL'
}
Test-N0Case 'early-done' {
    $s=New-N0Fake; $s=Update-N0State $s 'Record' 10001 @{Bytes=(New-N0FakeWire 0 'N0_HELLO')}
    $s=Update-N0State $s 'Record' 10002 @{Bytes=(New-N0FakeWire 1 'N0_DONE')}; $s.InvalidReason -eq 'INVALID_PROTOCOL'
}
Test-N0Case 'int64-emit-overflow' { -not (Test-N0Record -Bytes (New-N0FakeWire 1 'N0_SAMPLE' @{tEmit='9223372036854775808'}) -State (New-N0Fake)).Valid }
Test-N0Case 'int64-deadline-overflow' { try { $null=New-N0State ('a'*32) ('b'*32) 'Normal' ([long]::MaxValue) 1; $false } catch { $true } }
foreach ($delta in @(-2,-1,0,1,2)) {
    Test-N0Case ('clock-delta-'+$delta) {
        $s=New-N0Fake; $s=Update-N0State $s 'Record' 10001 @{Bytes=(New-N0FakeWire 0 'N0_HELLO')}
        $s=Update-N0State $s 'Record' 10100 @{Bytes=(New-N0FakeWire 1 'N0_SAMPLE' @{tEmit=(10100-$delta).ToString()})}
        if ($delta -lt -1) { $s.InvalidReason -eq 'INVALID_CLOCK' }
        elseif ($delta -le 1) { $s.InvalidReason -eq 'ORDER_AMBIGUOUS' }
        else { -not $s.InvalidReason }
    }
}
Test-N0Case 'local-clock-rollback' { $s=New-N0Fake; $s=Update-N0State $s 'Log' 9999 @{Bucket='Ordinary';Bytes=5}; $s.InvalidReason -eq 'INVALID_CLOCK' }
Test-N0Case 'startup-before-boundary' { -not (Get-N0DeadlineDecision (New-N0Fake) 14999).Stop }
Test-N0Case 'startup-at-boundary' { (Get-N0DeadlineDecision (New-N0Fake) 15000).Reason -eq 'INVALID_STARTUP' }
Test-N0Case 'absolute-precedence' { $s=New-N0Fake; $s.T0=15000; (Get-N0DeadlineDecision $s 20000).Reason -eq 'INVALID_ABSOLUTE_TIMEOUT' }
Test-N0Case 'first-sample-before-boundary' { $s=New-N0Fake; $s=Update-N0State $s 'Record' 10001 @{Bytes=(New-N0FakeWire 0 'N0_HELLO')}; -not (Get-N0DeadlineDecision $s 10500).Stop }
Test-N0Case 'first-sample-at-boundary' { $s=New-N0Fake; $s=Update-N0State $s 'Record' 10001 @{Bytes=(New-N0FakeWire 0 'N0_HELLO')}; (Get-N0DeadlineDecision $s 10501).Reason -eq 'INVALID_MISSING_SAMPLE' }
Test-N0Case 'first-guard-disarmed-probe-not-reset' {
    $s=New-N0Fake; $s=Update-N0State $s 'Record' 10001 @{Bytes=(New-N0FakeWire 0 'N0_HELLO')}
    $s=Update-N0State $s 'Record' 10110 @{Bytes=(New-N0FakeWire 1 'N0_SAMPLE')}
    (-not (Get-N0DeadlineDecision $s 10501).Stop) -and $s.ProbeDeadline -eq 12001 -and
        (Get-N0DeadlineDecision $s 12001).Reason -eq 'PROBE_COMPLETE'
}
Test-N0Case 'tick-ceiling' {
    $s=New-N0State ('a'*32) ('b'*32) 'Normal' 3 10000 42
    $s=Update-N0State $s 'Ownership' 10000 @{HasHandle=$true;HelperPid=42}
    $b=New-N0FakeWire 0 'N0_HELLO' @{frequency='3'}
    $s=Update-N0State $s 'Record' 10001 @{Bytes=$b}; $s.FirstDeadline -eq 10003
}
Test-N0Case 'ordinary-64-reserve-isolated' {
    $s=New-N0Fake; for ($i=0;$i -lt 64;$i++) { $s=Update-N0State $s 'Log' 10000 @{Bucket='Ordinary';Bytes=10} }
    $s.Ordinary -eq 64 -and $s.ReserveInvalid -eq 0 -and -not $s.InvalidReason
}
Test-N0Case 'ordinary-65-invalid' {
    $s=New-N0Fake; for ($i=0;$i -lt 65;$i++) { $s=Update-N0State $s 'Log' 10000 @{Bucket='Ordinary';Bytes=10} }
    $s.Ordinary -eq 64 -and $s.InvalidReason -eq 'INVALID_QUOTA'
}
Test-N0Case 'reserve-16-independent' {
    $s=New-N0Fake
    foreach ($bucket in @('ReserveInvalid','ReserveStop','ReserveCleanup')) {
        $limit=4; if ($bucket -eq 'ReserveCleanup') { $limit=8 }
        for ($i=0;$i -lt $limit;$i++) { $s=Update-N0State $s 'Log' 10000 @{Bucket=$bucket;Bytes=10} }
    }
    $s.ReserveInvalid+$s.ReserveStop+$s.ReserveCleanup -eq 16 -and -not $s.InvalidReason
}
Test-N0Case 'reserve-overflow' { $s=New-N0Fake; $s.ReserveStop=4; $s=Update-N0State $s 'Log' 10000 @{Bucket='ReserveStop';Bytes=10}; $s.InvalidReason -eq 'INVALID_QUOTA' }
Test-N0Case 'serialized-2048-allowed' { (Test-N0Record -Bytes (New-Object byte[] 2048) -Mode Serialized -Facts @{Limit=2048}).Valid }
Test-N0Case 'serialized-2049-rejected' { -not (Test-N0Record -Bytes (New-Object byte[] 2049) -Mode Serialized -Facts @{Limit=2048}).Valid }
Test-N0Case 'command-once-finally-idempotent-decision' { $s=New-N0CompleteFake; -not (Get-N0DeadlineDecision $s 13000).Stop }
Test-N0Case 'duplicate-command-invalid' { $s=New-N0CompleteFake; $s=Update-N0State $s 'CommandBegin' 13000 @{Deadline=13000;Reason='SECOND'}; $s.InvalidReason -eq 'BLOCKED_OWNERSHIP' }
Test-N0Case 'no-handle-no-kill-authority' { $s=New-N0Fake; $s.HasHandle=$false; $d=Get-N0DeadlineDecision $s 15000; -not $d.Stop -and -not $d.Authority }
Test-N0Case 'natural-exit-not-full-pass' { $s=New-N0CompleteFake; $s.NaturalExit=$true; (Get-N0Verdict $s).verdict -ne 'N0_NEUTRAL_OBSERVED' }
Test-N0Case 'kill-exception-not-pass' { $s=New-N0CompleteFake; $s.CommandSuccess=$false; (Get-N0Verdict $s).verdict -ne 'N0_NEUTRAL_OBSERVED' }
Test-N0Case 'wait-timeout' { $s=New-N0CompleteFake; $s.ExitConfirmed=$false; (Get-N0Verdict $s).verdict -eq 'INCONCLUSIVE_CLEANUP' }
Test-N0Case 'exit-over-threshold' { $s=New-N0CompleteFake; $s.ExitTick=18002; (Get-N0Verdict $s).verdict -eq 'INCONCLUSIVE_CLEANUP' }
Test-N0Case 'dispatch-over-threshold' { $s=New-N0CompleteFake; $s.CommandReturn=12102; (Get-N0Verdict $s).commandObservation -eq 'THRESHOLD_EXCEEDED' }
Test-N0Case 'missing-terminal' { $s=New-N0CompleteFake; $s.Integrity=$false; (Get-N0Verdict $s).verdict -eq 'INCONCLUSIVE_LOG_INTEGRITY' }
Test-N0Case 'unchecked-final-tail' { $s=New-N0CompleteFake; $s.WireFinalValid=$false; (Get-N0Verdict $s).verdict -eq 'INCONCLUSIVE_LOG_INTEGRITY' }
Test-N0Case 'partial-final-lf' { $s=New-N0CompleteFake; $s.TailComplete=$false; (Get-N0Verdict $s).verdict -eq 'INCONCLUSIVE_LOG_INTEGRITY' }
Test-N0Case 'read-reference-distinct-from-postvalidation-check' {
    $s=New-N0Fake 'DelayEmit'
    $s=Update-N0State $s 'Record' 10500 @{Bytes=(New-N0FakeWire 0 'N0_HELLO' @{scenario='DelayEmit'})}
    $s=Update-N0State $s 'Record' 10700 @{Bytes=(New-N0FakeWire 1 'N0_SAMPLE' @{scenario='DelayEmit';tEmit='10100'})}
    $decision=Get-N0DeadlineDecision $s 10705
    $s.InvalidTick -eq 10700 -and $decision.Deadline -eq 10700 -and $decision.WakeTick -eq 10705
}
Test-N0Case 'parser-cost-inside-reference-command-budget' {
    $s=New-N0Fake 'DelayEmit'
    $s=Update-N0State $s 'Record' 10500 @{Bytes=(New-N0FakeWire 0 'N0_HELLO' @{scenario='DelayEmit'})}
    $s=Update-N0State $s 'Record' 10700 @{Bytes=(New-N0FakeWire 1 'N0_SAMPLE' @{scenario='DelayEmit';tEmit='10100'})}
    $decision=Get-N0DeadlineDecision $s 10805
    $s=Update-N0State $s 'CommandBegin' 10810 @{Deadline=$decision.Deadline;Reason=$decision.Reason}
    $s=Update-N0State $s 'CommandReturn' 10815 @{Success=$true}
    $s=Update-N0State $s 'Exit' 10845 @{Confirmed=$true}
    $s.WireFinalValid=$true; $s.TailComplete=$true
    $v=Get-N0Verdict $s
    $decision.WakeTick -eq 10805 -and $v.commandObservation -eq 'THRESHOLD_EXCEEDED' -and
        $v.verdict -ne 'N0_NEGATIVE_DETECTED'
}
Test-N0Case 'hanging-hello-only-timeout-history' {
    $s=New-N0NegativeFake 'Hanging' 'Timeout'; $v=Get-N0Verdict $s
    $s.Records -eq 1 -and $s.Samples -eq 0 -and -not $s.Done -and $v.verdict -eq 'N0_NEGATIVE_DETECTED'
}
Test-N0Case 'delayemit-timeout-without-latency-history' {
    $s=New-N0NegativeFake 'DelayEmit' 'Timeout'; $v=Get-N0Verdict $s
    $s.SampleSummaries.Count -eq 0 -and $v.verdict -eq 'N0_NEGATIVE_DETECTED'
}
Test-N0Case 'delayemit-one-sample-threshold-history' {
    $s=New-N0NegativeFake 'DelayEmit' 'Threshold'; $v=Get-N0Verdict $s
    $s.Records -eq 2 -and $s.Samples -eq 1 -and -not $s.Done -and $v.verdict -eq 'N0_NEGATIVE_DETECTED'
}
Test-N0Case 'hanging-threshold-is-wrong-scenario' {
    (Get-N0Verdict (New-N0NegativeFake 'Hanging' 'Threshold')).verdict -ne 'N0_NEGATIVE_DETECTED'
}
Test-N0Case 'normal-failure-is-not-negative-case' {
    (Get-N0Verdict (New-N0NegativeFake 'Normal' 'Timeout')).verdict -ne 'N0_NEGATIVE_DETECTED'
}
Test-N0Case 'forged-complete-normal-is-not-hanging-negative' {
    $s=New-N0CompleteFake; $s.Scenario='Hanging'; $s.InvalidReason='INVALID_MISSING_SAMPLE'
    (Get-N0Verdict $s).verdict -ne 'N0_NEGATIVE_DETECTED'
}
foreach ($scenario in @('Hanging','DelayEmit')) {
    Test-N0Case ('negative-late-dispatch-'+$scenario) {
        $s=New-N0NegativeFake -Scenario $scenario -Cause Timeout -ReturnOffset 101
        $v=Get-N0Verdict $s
        $v.commandObservation -eq 'THRESHOLD_EXCEEDED' -and $v.verdict -ne 'N0_NEGATIVE_DETECTED'
    }
}
Test-N0Case 'negative-wrong-command-deadline' {
    $s=New-N0NegativeFake -Scenario Hanging -Cause Timeout -DeadlineOffset 1
    $v=Get-N0Verdict $s
    $v.commandObservation -eq 'OBSERVED_WITHIN_RUN_THRESHOLD' -and $v.verdict -ne 'N0_NEGATIVE_DETECTED'
}
Test-N0Case 'negative-wrong-command-reason' {
    $s=New-N0NegativeFake -Scenario DelayEmit -Cause Threshold -CommandReason PROBE_COMPLETE
    $v=Get-N0Verdict $s
    $v.commandObservation -eq 'OBSERVED_WITHIN_RUN_THRESHOLD' -and $v.verdict -ne 'N0_NEGATIVE_DETECTED'
}
Test-N0Case 'negative-secondary-quota-is-sticky' {
    $s=New-N0NegativeFake 'Hanging' 'Timeout'; $at=[long]$s.ExitTick+10
    for ($i=0;$i -lt 65;$i++) { $s=Update-N0State $s 'Log' $at @{Bucket='Ordinary';Bytes=10} }
    $s=Update-N0State $s 'Log' ($at+1) @{Bucket='ReserveCleanup';Bytes=10}
    $s.InvalidReason -eq 'INVALID_MISSING_SAMPLE' -and $s.DisqualifyingFault -eq 'INVALID_QUOTA' -and
        (Get-N0Verdict $s).verdict -ne 'N0_NEGATIVE_DETECTED'
}
Test-N0Case 'negative-secondary-protocol-is-sticky' {
    $s=New-N0NegativeFake 'DelayEmit' 'Timeout'; $at=[long]$s.ExitTick+10
    $bad=New-N0FakeWire 1 'N0_SAMPLE' @{scenario='DelayEmit';runId=('c'*32)}
    $s=Update-N0State $s 'Record' $at @{Bytes=$bad}
    $s.InvalidReason -eq 'INVALID_MISSING_SAMPLE' -and $s.DisqualifyingFault -eq 'INVALID_PROTOCOL' -and
        (Get-N0Verdict $s).verdict -ne 'N0_NEGATIVE_DETECTED'
}
Test-N0Case 'negative-unexpected-reason-not-detected' {
    $s=New-N0NegativeFake 'DelayEmit' 'Timeout'
    $s=Update-N0State $s 'Invalidate' ([long]$s.ExitTick+10) @{Reason='INVALID_PROTOCOL'}
    (Get-N0Verdict $s).verdict -eq 'N0_INVALID'
}
Test-N0Case 'positive-never-game-or-hard-bound' { $v=Get-N0Verdict (New-N0CompleteFake); -not $v.gameQualification -and -not $v.hardRealtimeGuarantee }
Test-N0Case 'secondary-clock-fault-blocks-negative' {
    $s=New-N0NegativeFake 'Hanging' 'Timeout'
    $s=Update-N0State $s 'Log' ([long]$s.ExitTick-1) @{Bucket='ReserveCleanup';Bytes=10}
    $s.InvalidReason -eq 'INVALID_MISSING_SAMPLE' -and $s.DisqualifyingFault -eq 'INVALID_CLOCK' -and
        (Get-N0Verdict $s).verdict -ne 'N0_NEGATIVE_DETECTED'
}
Test-N0Case 'emit-rollback' {
    $s=New-N0Fake; $s=Update-N0State $s 'Record' 10001 @{Bytes=(New-N0FakeWire 0 'N0_HELLO')}
    $s=Update-N0State $s 'Record' 10110 @{Bytes=(New-N0FakeWire 1 'N0_SAMPLE' @{tEmit='10100'})}
    $s=Update-N0State $s 'Record' 10120 @{Bytes=(New-N0FakeWire 2 'N0_SAMPLE' @{tEmit='10099'})}
    $s.InvalidReason -eq 'INVALID_CLOCK' -and -not $s.ClockValid
}
Test-N0Case 'wire-record-count-cap' {
    $s=New-N0Fake; $s.Records=16
    $s=Update-N0State $s 'Record' 10001 @{Bytes=(New-N0FakeWire 0 'N0_HELLO')}
    $s.InvalidReason -eq 'INVALID_PROTOCOL'
}
Test-N0Case 'serialized-8192-allowed' { (Test-N0Record -Bytes (New-Object byte[] 8192) -Mode Serialized -Facts @{Limit=8192}).Valid }
Test-N0Case 'serialized-8193-rejected' { -not (Test-N0Record -Bytes (New-Object byte[] 8193) -Mode Serialized -Facts @{Limit=8192}).Valid }
$failures=@($script:cases | Where-Object { -not $_.passed })
if ($script:cases.Count -gt 120) { throw 'N0_SELFTEST_STDOUT_BUDGET' }
foreach ($failure in $failures) { Write-Output ('N0_FAIL '+$failure.name+' '+$failure.error) }
if ($failures.Count) { throw ('N0_PURE_MODEL_FAIL cases='+$script:cases.Count+' failures='+$failures.Count) }
Write-Output ('N0_PURE_MODEL_PASS cases='+$script:cases.Count+' processEvidence=false gameEvidence=false')
