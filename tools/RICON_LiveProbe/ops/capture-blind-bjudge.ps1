param([string]$InputPath,[string]$RulePath,[string]$OutDir,[string]$Run)
$ErrorActionPreference='Stop'
$script:BlindJudgePath=$PSCommandPath

function Get-BlindHash([byte[]]$Bytes) {
    $sha=[Security.Cryptography.SHA256]::Create()
    try{return ([BitConverter]::ToString($sha.ComputeHash($Bytes))).Replace('-','').ToUpperInvariant()}finally{$sha.Dispose()}
}
function Write-BlindNewRecord([string]$Path,$Value) {
    $bytes=[Text.Encoding]::UTF8.GetBytes((ConvertTo-Json -InputObject $Value -Depth 16 -Compress))
    $stream=[IO.File]::Open($Path,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
    try{$stream.Write($bytes,0,$bytes.Length);$stream.Flush($true)}finally{$stream.Dispose()}
    return Get-BlindHash $bytes
}
function Get-BlindRule([string]$Path) {
    $rule=[ordered]@{schema='ricon.blind.rule.v1';tap='ownB';localPlayer=2;sourceOwnerPlayer=2;sender=1;catalogIndex=0;nativeIndex=0;frequencyKHz=32000;
        minimumDistinctTicks=3;shortMinMs=200;shortMaxMs=600;longMinMs=800;longMaxMs=1050;else='INVALID';foreignNullBadContextGapMalformed='INVALID';zeroRx='INVALID'}
    $window=[ordered]@{schema='ricon.blind.window.v1';anchor='FIRST_EXACT_OWN_B_RX';endOffsetMs=1100;requireOwnObservationThroughEnd=$true;anyQualifyingRxAfterEnd='INVALID';clock='ONE_B_LOCAL_NRX_BINDING';rawPreserved=$true}
    $expected=[ordered]@{schema='ricon.blind.fixed-rule.v1';rule=$rule;window=$window}
    $bytes=[IO.File]::ReadAllBytes($Path)
    $loaded=([Text.UTF8Encoding]::new($false,$true)).GetString($bytes)|ConvertFrom-Json
    if((ConvertTo-Json $loaded -Depth 8 -Compress) -cne (ConvertTo-Json $expected -Depth 8 -Compress)){throw 'FROZEN_RULE_MISMATCH'}
    $windowJson=ConvertTo-Json -InputObject $loaded.window -Depth 8 -Compress
    return [pscustomobject]@{rule=$loaded.rule;window=$loaded.window;ruleHash=(Get-BlindHash $bytes);windowHash=(Get-BlindHash ([Text.Encoding]::UTF8.GetBytes($windowJson)));
        judgeHash=(Get-BlindHash ([IO.File]::ReadAllBytes($script:BlindJudgePath)))}
}
function Get-BlindFields([string]$Text) {
    $f=@{}
    foreach($m in [regex]::Matches($Text,'(?:^|[ ,])([A-Za-z][A-Za-z0-9]*)=([^,\s]+)')) {
        if($f.ContainsKey($m.Groups[1].Value)){throw 'DUPLICATE_FIELD'}
        $f[$m.Groups[1].Value]=$m.Groups[2].Value
    }
    return $f
}
function Invoke-BlindBJudge([string]$Path,$Info,[string]$ExpectedRun) {
    $info=$Info;$bytes=[IO.File]::ReadAllBytes($Path);$actualHash=Get-BlindHash $bytes
    $result=[ordered]@{schema='ricon.blind.decision.v1';run=$ExpectedRun;inputHash=$actualHash;ruleHash=$info.ruleHash;windowHash=$info.windowHash;codeHash=$info.judgeHash;
        decision='INVALID';reason='MALFORMED';firstRxTick=$null;windowEndTick=$null;lastObservationTick=$null;lastRxTick=$null;spanMs=$null;distinctTicks=0;rawRowCount=0;
        inputScope='SEALED_OWN_B_NRX_ONLY';tails='NOT_DISCARDED';reliability='NOT_ESTABLISHED'}
    try {
        if($bytes.Length -gt 8388608){throw 'INPUT_LIMIT'}
        $text=([Text.UTF8Encoding]::new($false,$true)).GetString($bytes)
        if(-not $text.EndsWith([string][char]10)){throw 'TRUNCATED_FINAL_LINE'}
        $lastSeq=-1L;$lastRaw=-1L;$start='';$bound=$null;$catalog=@{};$rx=@();$horizon=$null;$seenStart=$false;$catalogClosed=$false
        foreach($line in [regex]::Split($text,'\r\n|\n|\r')) {
            if(-not $line.Trim()){continue}
            $m=[regex]::Match($line,'^(?:[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{3}\s+SCRIPT\s*:\s*)?\[RICON_NRX\] seq=(?<seq>\d+) t=(?<elapsed>-?\d+) side=(?<side>[A-Za-z0-9_]+) kind=(?<kind>[A-Za-z_]+) player=(?<player>-?\d+) device=(?<device>-?\d+) tr=(?<tr>-?\d+) value=(?<value>-?\d+) text=(?<fields>[^\r\n]*)$')
            if(-not $m.Success -or $line -match '\[RICON_(?:PULSE|REST)\]|blindChoice|plannedIntendedMs|CaptureShort400'){throw 'MALFORMED_OR_FORBIDDEN_INPUT'}
            $f=Get-BlindFields $m.Groups['fields'].Value;$seq=[long]$m.Groups['seq'].Value;$kind=$m.Groups['kind'].Value;$side=$m.Groups['side'].Value
            if($f['run'] -cnotmatch '\A[A-Za-z0-9][A-Za-z0-9_.-]{0,63}\z' -or $f['rawTickMs'] -cnotmatch '\A[0-9]{1,10}\z' -or $f['startTickMs'] -cnotmatch '\A[0-9]{1,10}\z'){throw 'MALFORMED_ID_OR_CLOCK'}
            $raw=[long]$f['rawTickMs']
            if($lastSeq -eq -1){$start=$f['startTickMs']}
            if($f['run'] -cne $ExpectedRun -or $f['startTickMs'] -cne $start){throw 'FOREIGN_RUN_OR_START'}
            if($seq -ne $lastSeq+1 -or $raw -lt $lastRaw -or [long]$m.Groups['elapsed'].Value -ne $raw-[long]$start){throw 'GAP_OR_CLOCK'}
            $lastSeq=$seq;$lastRaw=$raw;$result.rawRowCount++
            if($kind -ceq 'GAP'){throw 'GAP_EVENT'}
            if($kind -ceq 'START') {
                if($seenStart -or $seq -ne 0 -or $bound -or $side -cne 'process' -or $f['localPlayer'] -cne '-1' -or $f['bindingGeneration'] -cne '0'){throw 'BAD_START'}
                $seenStart=$true
                continue
            }
            if(-not $seenStart){throw 'START_MISSING'}
            if($kind -ceq 'BOUND') {
                if($bound -or $side -cne 'process' -or $f['localPlayer'] -cne '2' -or $f['bindingGeneration'] -cne '1' -or
                    $f['ownRpl'] -cnotmatch '\A-?[0-9]{1,10}\z' -or $f['catalog'] -cnotmatch '\A[1-8]\z'){throw 'BAD_BOUND'}
                $bound=$f;continue
            }
            if(-not $bound -or $f['localPlayer'] -cne '2' -or $f['bindingGeneration'] -cne '1'){throw 'FOREIGN_BINDING'}
            if($kind -ceq 'BOUND_CATALOG') {
                $index=$m.Groups['tr'].Value
                if($catalogClosed -or $side -cne 'process' -or $catalog.ContainsKey($index) -or $index -cnotmatch '\A[0-7]\z' -or [int]$index -ge [int]$bound['catalog']){throw 'CATALOG_REPLAY_OR_INDEX'}
                if($index -ceq '0' -and ($m.Groups['value'].Value -cne '32000' -or $f['nativeIndex'] -cne '0')){throw 'CATALOG_ZERO_MISMATCH'}
                $catalog[$index]=$f;continue
            }
            $catalogClosed=$true
            if($catalog.Count -ne [int]$bound['catalog'] -or -not $catalog.ContainsKey('0')){throw 'INCOMPLETE_CATALOG'}
            if($kind -ceq 'RAW_RX') {
                $c=$catalog['0'];$quality=0.0
                if($side -cne 'ownB' -or $m.Groups['player'].Value -cne '1' -or $m.Groups['tr'].Value -cne '0' -or $m.Groups['value'].Value -cne '32000' -or
                    $f['sourceOwnerPlayer'] -cne '2' -or $f['sourceOwnerRpl'] -cne $bound['ownRpl'] -or $f['subscriptionGeneration'] -cne '1' -or
                    $f['contextValid'] -cne '1' -or $f['senderA'] -cne '1' -or $f['receiverPresent'] -cne '1' -or $f['catalogIndex'] -cne '0' -or
                    $f['nativeIndex'] -cne '0' -or $f['catalogScope'] -cne 'OWN_HANDHELD_BACKPACK' -or $f['identityScope'] -cne 'PROCESS_AND_BINDING_GENERATION' -or
                    $f['route'] -cne 'CATALOG_REF_MATCH' -or $f['qualityValid'] -cne '1' -or -not $c -or
                    $f['nativeRefLocal'] -cne $c['nativeRefLocal'] -or $f['radioRefLocal'] -cne $c['radioRefLocal'] -or
                    $f['nativeRefLocal'] -cnotmatch '\A[A-Za-z_][A-Za-z0-9_]*<0x[0-9A-Fa-f]+>\z' -or $f['nativeRefLocal'] -match '<0x0+>' -or
                    $f['radioRefLocal'] -cnotmatch '\A[A-Za-z_][A-Za-z0-9_]*<0x[0-9A-Fa-f]+>\z' -or $f['radioRefLocal'] -match '<0x0+>' -or
                    -not [double]::TryParse([string]$f['quality'],[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$quality) -or
                    [double]::IsNaN($quality) -or [double]::IsInfinity($quality) -or $quality -lt 0 -or $quality -gt 1){throw 'FOREIGN_NULL_OR_BAD_RX_CONTEXT'}
                $rx+=$raw;$horizon=$raw;continue
            }
            if($kind -cin @('HEARTBEAT','LIVE_GEOMETRY')) {
                if($side -cne 'process'){throw 'FOREIGN_METADATA'}
                if($kind -ceq 'HEARTBEAT' -and $f['contextValid'] -cne '1'){throw 'BAD_CONTEXT'}
                $horizon=$raw
                continue
            }
            throw 'FOREIGN_OR_UNKNOWN_EVENT'
        }
        if(-not $bound -or $catalog.Count -ne [int]$bound['catalog'] -or -not $catalog.ContainsKey('0')){throw 'INCOMPLETE_CATALOG'}
        if(-not $rx.Count){throw 'NO_RX'}
        $first=[long]$rx[0];$end=$first+1100;$last=[long]$rx[-1];$distinct=@($rx|Select-Object -Unique)
        $result.firstRxTick=$first;$result.windowEndTick=$end;$result.lastObservationTick=$horizon;$result.lastRxTick=$last;$result.spanMs=$last-$first;$result.distinctTicks=$distinct.Count
        if($last -gt $end){throw 'QUALIFYING_RX_AFTER_WINDOW'}
        if($null -eq $horizon -or $horizon -lt $end){throw 'INADEQUATE_HORIZON'}
        if($distinct.Count -lt 3){throw 'INSUFFICIENT_DISTINCT_TICKS'}
        if($result.spanMs -ge 200 -and $result.spanMs -le 600){$result.decision='SHORT'}
        elseif($result.spanMs -ge 800 -and $result.spanMs -le 1050){$result.decision='LONG'}
        else{throw 'SPAN_OUTSIDE_FROZEN_BANDS'}
        $result.reason='FROZEN_RULE_DECISION'
    } catch {$code=$_.Exception.Message;$result.decision='INVALID';$result.reason=if($code -cmatch '\A[A-Z_]+\z'){$code}else{'MALFORMED'}}
    return $result
}

if($MyInvocation.InvocationName -ne '.') {
    if(-not $InputPath -or -not $RulePath -or -not $OutDir -or $Run -cnotmatch '\A[A-Za-z0-9][A-Za-z0-9_.-]{0,63}\z'){throw 'NAMED_B_INPUT_RULE_FRESH_OUTPUT_RUN_REQUIRED'}
    $info=Get-BlindRule $RulePath
    $decision=Invoke-BlindBJudge $InputPath $info $Run
    if([IO.Directory]::Exists($OutDir)){throw 'OUTPUT_DIRECTORY_EXISTS'}
    $null=[IO.Directory]::CreateDirectory($OutDir)
    $decisionPath=[IO.Path]::Combine($OutDir,'decision.json')
    $hash=Write-BlindNewRecord $decisionPath $decision
    $hashPath=[IO.Path]::Combine($OutDir,'decision.json.sha256')
    $hashBytes=[Text.Encoding]::ASCII.GetBytes($hash+[Environment]::NewLine)
    $hashStream=[IO.File]::Open($hashPath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
    try{$hashStream.Write($hashBytes,0,$hashBytes.Length);$hashStream.Flush($true)}finally{$hashStream.Dispose()}
    "B_DECISION_SEALED hash=$hash"
}
