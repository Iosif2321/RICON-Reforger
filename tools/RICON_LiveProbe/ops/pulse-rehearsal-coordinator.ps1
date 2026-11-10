# TOOL_ONLY, opt-in rehearsal. Caller supplies original Start-Process objects; no PID lookup or capture.
function Initialize-RehearsalProxy {
    if ('RiconRehearsalInvokeV1' -as [type]) { return }
    Add-Type -ReferencedAssemblies @([Windows.Automation.InvokePattern].Assembly.Location,[Windows.Automation.AutomationPattern].Assembly.Location) -TypeDefinition @'
using System;
using System.Diagnostics;
using System.Threading;
using System.Runtime.InteropServices;
using System.Windows.Automation;
public sealed class RiconRehearsalInvokeV1 {
    [DllImport("user32.dll")] static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    public static string ForegroundRole(uint a, uint b) {
        IntPtr window=GetForegroundWindow();
        if(window==IntPtr.Zero) return "NONE";
        uint owner; GetWindowThreadProcessId(window,out owner);
        if(owner==0) return "UNKNOWN";
        return owner==a ? "A" : (owner==b ? "B" : "OTHER");
    }
    readonly Action action;
    long begin, end;
    int calls, failed;
    public long InvokeBeginTicks { get { return Interlocked.Read(ref begin); } }
    public long InvokeReturnTicks { get { return Interlocked.Read(ref end); } }
    public int Calls { get { return Interlocked.CompareExchange(ref calls,0,0); } }
    public bool Failed { get { return Interlocked.CompareExchange(ref failed,0,0)!=0; } }
    public RiconRehearsalInvokeV1(InvokePattern pattern) {
        if(pattern==null) throw new ArgumentNullException("pattern");
        action=pattern.Invoke;
    }
    RiconRehearsalInvokeV1(Action dummy) { action=dummy; }
    public static RiconRehearsalInvokeV1 SelfCheckDummy(int delayMs, bool fail) {
        return new RiconRehearsalInvokeV1(delegate {
            if(delayMs>0) Thread.Sleep(delayMs);
            if(fail) throw new InvalidOperationException("DUMMY_FAILURE");
        });
    }
    public void Invoke() {
        if(Interlocked.Increment(ref calls)!=1) {
            Interlocked.Exchange(ref failed,1); throw new InvalidOperationException("ONE_USE");
        }
        Interlocked.Exchange(ref begin,Stopwatch.GetTimestamp());
        try { action(); }
        catch { Interlocked.Exchange(ref failed,1); throw; }
        finally { Interlocked.Exchange(ref end,Stopwatch.GetTimestamp()); }
    }
}
'@
    [Runtime.CompilerServices.RuntimeHelpers]::PrepareMethod([RiconRehearsalInvokeV1].GetMethod('Invoke').MethodHandle)
}

function New-RehearsalCoordinator([string]$Run,[string]$Attempt,[switch]$Capture,[switch]$NoSend,[switch]$WrongFrequency,[switch]$SeparatedPair,[switch]$Short400,[switch]$BlindTrial,[switch]$VoNSoundObserve,[switch]$BPowerOff,[switch]$Direct) {
    if($Direct -and (-not $Capture -or $SeparatedPair -or $NoSend -or $WrongFrequency -or $Short400 -or $BlindTrial -or $VoNSoundObserve)){throw 'INVALID_DIRECT_MODE'}
    if($BPowerOff -and (-not $Capture -or (-not $SeparatedPair -and -not $Direct) -or $Short400 -or $BlindTrial -or $NoSend -or $WrongFrequency -or $VoNSoundObserve)){throw 'INVALID_B_POWER_OFF_MODE'}
    if($VoNSoundObserve -and (-not $Capture -or -not $SeparatedPair -or $BlindTrial -or $Short400 -or $NoSend -or $WrongFrequency)){throw 'INVALID_SOUND_OBSERVE_MODE'}
    if($BlindTrial -and (-not $Capture -or -not $SeparatedPair -or $Short400 -or $NoSend -or $WrongFrequency)){throw 'INVALID_BLIND_MODE'}
    if($Short400 -and (-not $Capture -or -not $SeparatedPair -or $NoSend -or $WrongFrequency)){throw 'INVALID_SHORT400_MODE'}
    if($SeparatedPair -and -not $Capture){throw 'SEPARATED_PAIR_REQUIRES_CAPTURE'}
    if($WrongFrequency -and (-not $Capture -or $NoSend)){throw 'INVALID_WRONG_FREQUENCY_MODE'}
    if($NoSend -and -not $Capture){throw 'NO_SEND_REQUIRES_CAPTURE'}
    if ($Run -cnotmatch '\A[A-Za-z0-9_.-]{1,64}\z' -or $Attempt -cnotmatch '\A[A-Za-z0-9_.-]{1,64}\z') { throw 'INVALID_IDENTITY' }
    return [pscustomobject]@{ Run=$Run; Attempt=$Attempt; Capture=[bool]$Capture; NoSend=[bool]$NoSend; WrongFrequency=[bool]$WrongFrequency; SeparatedPair=[bool]$SeparatedPair; Short400=[bool]$Short400; BlindTrial=[bool]$BlindTrial; VoNSoundObserve=[bool]$VoNSoundObserve; ControlTag=$(if($Direct -and $BPowerOff){'DIRECT_B_POWER_OFF'}else{''}); Direct=[bool]$Direct; DirectEvidence=$null; BPowerOff=[bool]$BPowerOff; PowerTracking=$null; PowerAdmission=$null; SoundAdmission=$null; SoundTracking=$null; SoundStatus='UNKNOWN'; Blind=$null; CaptureTiming=$null; BObservation=$null; GeometryTracking=$null; FrequencyRestore=$null; FrequencyInterpretation=$null; CaptureRequests=$null; RadioEvidence=$null; Target=$null; TargetStart=0L; TargetHandle=[IntPtr]::Zero;
        Client=$null; ClientStart=0L; Main=[IntPtr]::Zero; Attempted=$false; Phase='WAIT'; Status='NOT_ATTEMPTED';
        Failure=''; Snapshot=$null; Guard=$null; GuardResult=$null; Proxy=$null; Job=$null; MenuDiagnostic=$null; InitialMenuDiagnostic=$null;
        DispatchBeforeTicks=0L; DispatchAfterTicks=0L; GameValid=$false; Valid=$false; Cleanup='NOT_RUN';
        RestPending=$null;RestResult=$null;ReleaseBeforeTicks=0L;ReleaseAfterTicks=0L;RestErrors=@() }
}

function Get-RehearsalFields([string]$Text) {
    $fields=@{}
    foreach($match in [regex]::Matches($Text,'(?:^|[ ,])([A-Za-z][A-Za-z0-9]*)=([^,\s]+)')) { $fields[$match.Groups[1].Value]=$match.Groups[2].Value }
    return $fields
}

function Get-BlindParentHash([byte[]]$Bytes) {
    $sha=[Security.Cryptography.SHA256]::Create()
    try{return ([BitConverter]::ToString($sha.ComputeHash($Bytes))).Replace('-','').ToUpperInvariant()}finally{$sha.Dispose()}
}
function Write-BlindParentRecord([string]$Path,$Value) {
    $bytes=[Text.Encoding]::UTF8.GetBytes((ConvertTo-Json -InputObject $Value -Depth 18 -Compress))
    $stream=[IO.File]::Open($Path,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
    try{$stream.Write($bytes,0,$bytes.Length);$stream.Flush($true)}finally{$stream.Dispose()}
    return Get-BlindParentHash $bytes
}
function Initialize-BlindParent($State,[string]$OutDir,[string]$RulePath,[string]$JudgePath) {
    $ruleBytes=[IO.File]::ReadAllBytes($RulePath);$rule=([Text.Encoding]::UTF8.GetString($ruleBytes)|ConvertFrom-Json)
    if(-not $rule.window){throw 'BLIND_WINDOW_REQUIRED'}
    $ruleHash=Get-BlindParentHash $ruleBytes;$judgeHash=Get-BlindParentHash ([IO.File]::ReadAllBytes($JudgePath))
    $windowHash=Get-BlindParentHash ([Text.Encoding]::UTF8.GetBytes((ConvertTo-Json -InputObject $rule.window -Depth 8 -Compress)))
    if($ruleHash -cne '1ED9601F51CC0BE9020F58BC9D200FB9A2DC81CD2DD6CBFADD94674C35C7C714' -or $judgeHash -cne 'C91D7020B22EB2DB282409B62E0DF0E7CDBDD9DEF7C52DB3190B2724E4EEE600' -or $windowHash -cne '786623D04EC0D6BC478E29BD1BF447C08E4E0CB128D6565D7FDA175AA2CA42AA'){throw 'BLIND_RULE_PIN'}
    $public=Join-Path $OutDir 'blind-public';$private=Join-Path $OutDir 'blind-private';$bOnly=Join-Path $OutDir 'blind-b-only'
    foreach($path in @($public,$private,$bOnly)){if([IO.Directory]::Exists($path) -or [IO.File]::Exists($path)){throw 'BLIND_DIRECTORIES_NOT_FRESH'}}
    foreach($path in @($public,$private)){$null=[IO.Directory]::CreateDirectory($path)}
    $freeze=[ordered]@{schema='ricon.blind.rule-freeze.v1';run=$State.Run;attempt=$State.Attempt;ruleHash=$ruleHash;windowHash=$windowHash;judgeHash=$judgeHash;createdUtc=[datetime]::UtcNow.ToString('o')}
    $hash=Write-BlindParentRecord (Join-Path $public 'rule-freeze.json') $freeze
    $State.Blind=[pscustomobject]@{Public=$public;Private=$private;BOnly=$bOnly;RulePath=$RulePath;JudgePath=$JudgePath;RuleHash=$ruleHash;WindowHash=$windowHash;JudgeHash=$judgeHash;
        FreezeHash=$hash;FreezeTicks=[Diagnostics.Stopwatch]::GetTimestamp();Choice=$null;Nonce='';PlannedMs=0;Commitment='';CommitHash='';PrivateChoiceHash='';PrivateAHash='';PrivateRestHash='';Selected=$false;Revealed=$false}
}
function Get-BlindParentEntropy {
    $rng=[Security.Cryptography.RandomNumberGenerator]::Create();$bit=New-Object byte[] 1;$nonce=New-Object byte[] 32
    try{$rng.GetBytes($bit);$rng.GetBytes($nonce)}finally{$rng.Dispose()}
    return [pscustomobject]@{Choice=([int]$bit[0] -band 1);Nonce=([BitConverter]::ToString($nonce)).Replace('-','')}
}
function Set-BlindParentChoice($State) {
    $b=$State.Blind
    if(-not $b -or $b.Selected -or $b.FreezeTicks -le 0 -or -not $State.Attempted -or -not $State.Snapshot -or -not $State.RestPending -or
        -not $State.Snapshot.Geometry.Freshness -or $State.RestPending.PreparedGeneration -ne [int]$State.Snapshot.PreparedGeneration){throw 'BLIND_SELECTION_ORDER'}
    if((Get-BlindParentHash ([IO.File]::ReadAllBytes($b.RulePath))) -cne $b.RuleHash -or (Get-BlindParentHash ([IO.File]::ReadAllBytes($b.JudgePath))) -cne $b.JudgeHash){throw 'BLIND_RULE_CHANGED'}
    $entropy=Get-BlindParentEntropy
    if($entropy.Choice -notin @(0,1) -or $entropy.Nonce -cnotmatch '\A[0-9A-F]{64}\z'){throw 'BLIND_ENTROPY_INVALID'}
    $b.Selected=$true;$b.Choice=[int]$entropy.Choice;$b.Nonce=$entropy.Nonce;$b.PlannedMs=if($b.Choice -eq 0){400}else{1000}
    $canonical=@('RICON_BLIND_V1',$State.Run,$State.Attempt,$b.RuleHash,$b.WindowHash,[string]$b.Choice,$b.Nonce) -join "`n"
    $b.Commitment=Get-BlindParentHash ([Text.Encoding]::UTF8.GetBytes($canonical))
    $private=[ordered]@{schema='ricon.blind.private-choice.v1';run=$State.Run;attempt=$State.Attempt;choice=$b.Choice;nonce=$b.Nonce;plannedMs=$b.PlannedMs;ruleHash=$b.RuleHash;windowHash=$b.WindowHash;commitment=$b.Commitment;selectedTicks=[Diagnostics.Stopwatch]::GetTimestamp()}
    $b.PrivateChoiceHash=Write-BlindParentRecord (Join-Path $b.Private 'choice.json') $private
    $public=[ordered]@{schema='ricon.blind.commit.v1';run=$State.Run;attempt=$State.Attempt;ruleHash=$b.RuleHash;windowHash=$b.WindowHash;judgeHash=$b.JudgeHash;ruleFreezeHash=$b.FreezeHash;commitment=$b.Commitment;createdUtc=[datetime]::UtcNow.ToString('o')}
    $b.CommitHash=Write-BlindParentRecord (Join-Path $b.Public 'commit.json') $public
}
function Complete-BlindParent($State,[string]$BLog) {
    $b=$State.Blind
    if(-not $b -or -not $b.Selected){return [pscustomobject]@{status='BLIND_NOT_SELECTED';revealHash=''}}
    if($b.Revealed){throw 'BLIND_REVEAL_REPLAY'}
    if(-not $b.PrivateAHash -or -not $b.PrivateRestHash){throw 'BLIND_PRIVATE_A_INCOMPLETE'}
    if(-not $State.BObservation -or $State.BObservation.stopExited -ne $true -or $State.BObservation.finalReadTicks -lt $State.BObservation.stopEndTicks){throw 'BLIND_B_NOT_SEALED_AFTER_EXIT'}
    if((Get-BlindParentHash ([IO.File]::ReadAllBytes((Join-Path $b.Private 'a-result.json')))) -cne $b.PrivateAHash -or
        (Get-BlindParentHash ([IO.File]::ReadAllBytes((Join-Path $b.Private 'rest-result.json')))) -cne $b.PrivateRestHash){throw 'BLIND_PRIVATE_RESULT_CHANGED'}
    if((Get-BlindParentHash ([IO.File]::ReadAllBytes($b.RulePath))) -cne $b.RuleHash -or (Get-BlindParentHash ([IO.File]::ReadAllBytes($b.JudgePath))) -cne $b.JudgeHash){throw 'BLIND_RULE_CHANGED'}
    $sourceBytes=[IO.File]::ReadAllBytes($BLog)
    $raw=([Text.UTF8Encoding]::new($false,$true)).GetString($sourceBytes)
    $nrx=([regex]::Matches($raw,'[^\r\n]*(?:\r\n|\n|\r|$)')|Where-Object{$_.Value.Contains('[RICON_NRX]')}|ForEach-Object{$_.Value}) -join ''
    $inputPath=Join-Path $b.Public 'b-input.nrx';$inputBytes=[Text.Encoding]::UTF8.GetBytes($nrx)
    $stream=[IO.File]::Open($inputPath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
    try{$stream.Write($inputBytes,0,$inputBytes.Length);$stream.Flush($true)}finally{$stream.Dispose()}
    $inputHash=Get-BlindParentHash $inputBytes
    $inputSeal=[ordered]@{schema='ricon.blind.b-input.v1';run=$State.Run;inputHash=$inputHash;sourceHash=(Get-BlindParentHash $sourceBytes);scope='ALL_OWN_B_NRX_LINES_NO_VALIDITY_FILTER'}
    $null=Write-BlindParentRecord (Join-Path $b.Public 'b-input-seal.json') $inputSeal
    foreach($path in @($b.JudgePath,$b.RulePath,$inputPath,$b.BOnly)){if($path -match '["\x00-\x1F\x7F]'){throw 'BLIND_ARGUMENT_PATH'}}
    $args=@('-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',('"'+$b.JudgePath+'"'),'-InputPath',('"'+$inputPath+'"'),'-RulePath',('"'+$b.RulePath+'"'),'-OutDir',('"'+$b.BOnly+'"'),'-Run',$State.Run)
    $judge=$null;$primary=$null;$cleanupErrors=@();$judgeExited=$false
    try {
        $judge=Start-Process -FilePath (Join-Path $PSHOME 'powershell.exe') -ArgumentList $args -PassThru -WindowStyle Hidden -RedirectStandardOutput (Join-Path $b.Public 'bjudge.stdout') -RedirectStandardError (Join-Path $b.Public 'bjudge.stderr')
        $null=$judge.Handle
        if(-not $judge.WaitForExit(10000)){throw 'BLIND_JUDGE_TIMEOUT'}
        $judge.Refresh()
        if($judge.ExitCode -ne 0){throw 'BLIND_JUDGE_FAILED'}
        $judgeExited=$true
    } catch {$primary=$_;throw}
    finally {
        if($judge) {
            try {if(-not $judge.HasExited){$judge.Kill();if(-not $judge.WaitForExit(1000)){throw 'BLIND_JUDGE_EXIT_UNCONFIRMED'}};$judgeExited=$judge.HasExited}
            catch {$e=$_.Exception;while($e){$cleanupErrors+=[pscustomobject]@{type=$e.GetType().FullName;hResult=$e.HResult};$e=$e.InnerException}}
            try {$judge.Dispose()}catch{$cleanupErrors+=[pscustomobject]@{type=$_.Exception.GetType().FullName;hResult=$_.Exception.HResult}}
        }
        try {$null=Write-BlindParentRecord (Join-Path $b.Private 'judge-cleanup.json') ([ordered]@{schema='ricon.blind.judge-cleanup.v1';originalChildExited=$judgeExited;primaryPresent=[bool]$primary;secondary=$cleanupErrors})}
        catch {if(-not $primary){throw 'BLIND_JUDGE_CLEANUP_RECORD_FAILED'}}
        if(-not $primary -and (-not $judgeExited -or $cleanupErrors.Count)){throw 'BLIND_JUDGE_CLEANUP_UNCONFIRMED'}
    }
    $decisionPath=Join-Path $b.BOnly 'decision.json';$decisionBytes=[IO.File]::ReadAllBytes($decisionPath);$decisionHash=Get-BlindParentHash $decisionBytes
    $sealText=[IO.File]::ReadAllText($decisionPath+'.sha256').Trim()
    if($sealText -cnotmatch '\A[0-9A-Fa-f]{64}(?:\s+decision\.json)?\z' -or $sealText.Substring(0,64).ToUpperInvariant() -cne $decisionHash){throw 'BLIND_DECISION_SEAL'}
    $decision=([Text.Encoding]::UTF8.GetString($decisionBytes)|ConvertFrom-Json)
    if($decision.run -cne $State.Run -or $decision.decision -cnotin @('SHORT','LONG','INVALID') -or
        $decision.inputHash.ToUpperInvariant() -cne $inputHash -or $decision.ruleHash.ToUpperInvariant() -cne $b.RuleHash -or
        $decision.windowHash.ToUpperInvariant() -cne $b.WindowHash -or $decision.codeHash.ToUpperInvariant() -cne $b.JudgeHash){throw 'BLIND_DECISION_BINDING'}
    $commitPath=Join-Path $b.Public 'commit.json'
    if((Get-BlindParentHash ([IO.File]::ReadAllBytes($commitPath))) -cne $b.CommitHash){throw 'BLIND_COMMIT_CHANGED'}
    $privatePath=Join-Path $b.Private 'choice.json';$privateBytes=[IO.File]::ReadAllBytes($privatePath)
    if((Get-BlindParentHash $privateBytes) -cne $b.PrivateChoiceHash){throw 'BLIND_PRIVATE_CHANGED'}
    $private=([Text.Encoding]::UTF8.GetString($privateBytes)|ConvertFrom-Json)
    $canonical=@('RICON_BLIND_V1',$State.Run,$State.Attempt,$b.RuleHash,$b.WindowHash,[string]$private.choice,$private.nonce) -join "`n"
    if((Get-BlindParentHash ([Text.Encoding]::UTF8.GetBytes($canonical))) -cne $b.Commitment){throw 'BLIND_COMMIT_VERIFY'}
    $expected=if([int]$private.choice -eq 0){'SHORT'}else{'LONG'}
    $outcome=if($decision.decision -ceq 'INVALID' -or -not $State.Valid){'SINGLE_BLIND_TRIAL_INVALID'}elseif($decision.decision -ceq $expected){'SINGLE_BLIND_TRIAL_MATCH'}else{'SINGLE_BLIND_TRIAL_MISMATCH'}
    $reveal=[ordered]@{schema='ricon.blind.reveal.v1';run=$State.Run;attempt=$State.Attempt;ruleHash=$b.RuleHash;windowHash=$b.WindowHash;inputHash=$inputHash;decisionHash=$decisionHash;
        commitment=$b.Commitment;choice=$private.choice;nonce=$private.nonce;plannedMs=$private.plannedMs;actualTiming=$State.CaptureTiming;decision=$decision.decision;outcome=$outcome;chance='1/2';scope='ONE_TRIAL_NOT_CONTENT_RELIABILITY_OR_BASELINE_PASS'}
    $hash=Write-BlindParentRecord (Join-Path $b.Public 'reveal.json') $reveal;$b.Revealed=$true
    return [pscustomobject]@{status=$outcome;revealHash=$hash}
}

# Это наблюдение script-state; питание/mute и разрешение native capture здесь не доказываются.
function Get-CaptureRadioReadiness($HostFields,$ClientFields,[string]$NativeIndex,[string]$FrequencyKHz,[switch]$WrongFrequency) {
    if($NativeIndex -cnotmatch '\A[0-3]\z' -or $FrequencyKHz -cnotmatch '\A[1-9][0-9]*\z'){return $null}
    foreach($view in @(@($HostFields,'A_SERVER_VIEW'),@($ClientFields,'B_OWN_A_PROXY_VIEW'))) {
        $f=$view[0]
        if(-not $f -or $f['rrView'] -cne $view[1] -or $f['rrOwnIdentityCurrent'] -cne '1' -or $f['rrReplication'] -cne 'UNQUALIFIED' -or
            $f['rrNativeOwnership'] -cne 'UNQUALIFIED' -or $f['rrPermission'] -cne 'SCRIPT_STATE_ONLY' -or $f['rrAtomic'] -cne '0'){return $null}
    }
    # Ключи и обе inventory квалифицирует A server view; отсутствие A inventory у B proxy не является mismatch.
    if($HostFields['rrPairKnown'] -cne '1' -or $HostFields['rrAUnique'] -cne '1' -or $HostFields['rrBUnique'] -cne '1' -or
        $HostFields['rrKeysEqual'] -cne '1' -or $HostFields['rrAKeyEmpty'] -cnotmatch '\A[01]\z' -or $HostFields['rrBKeyEmpty'] -cne $HostFields['rrAKeyEmpty']){return $null}
    foreach($name in @('rrAFrequenciesKHz','rrBFrequenciesKHz')) {
        if($HostFields[$name] -cnotmatch '\A[1-9][0-9]*(?:\|[1-9][0-9]*){0,3}\z'){return $null}
        if($WrongFrequency -and $name -ceq 'rrBFrequenciesKHz'){if($FrequencyKHz -cin $HostFields[$name].Split('|')){return $null}}
        elseif($FrequencyKHz -cnotin $HostFields[$name].Split('|')){return $null}
    }
    if($ClientFields['rrPairKnown'] -ceq '1') {
        if($ClientFields['rrAUnique'] -cne '1' -or $ClientFields['rrBUnique'] -cne '1' -or $ClientFields['rrKeysEqual'] -cne '1' -or
            $ClientFields['rrAKeyEmpty'] -cne $HostFields['rrAKeyEmpty'] -or $ClientFields['rrBKeyEmpty'] -cne $HostFields['rrBKeyEmpty']){return $null}
        foreach($name in @('rrAFrequenciesKHz','rrBFrequenciesKHz')){if($HostFields[$name] -cne $ClientFields[$name]){return $null}}
    } elseif($ClientFields['rrPairKnown'] -cne '0' -or $ClientFields['rrAUnique'] -cne '0' -or $ClientFields['rrBUnique'] -cne '1' -or
        $ClientFields['rrKeysEqual'] -cne '-1' -or $ClientFields['rrAKeyEmpty'] -cne '-1' -or $ClientFields['rrBKeyEmpty'] -cne '-1') {return $null}
    if($ClientFields['rrEntryKnown'] -cne '1' -or $ClientFields['rrEntryRadio'] -cne '1' -or
        $ClientFields['rrEntryIndex'] -cnotmatch '\A(?:[0-9]|1[0-5])\z' -or $ClientFields['rrEntryNativeIndex'] -cnotmatch '\A[0-3]\z' -or
        $ClientFields['rrEntryActive'] -cnotmatch '\A[01]\z' -or $ClientFields['rrEntryUsable'] -cnotmatch '\A[01]\z' -or $ClientFields['rrEntryBindingAllowed'] -cnotmatch '\A[01]\z' -or
        (-not $WrongFrequency -and $ClientFields['rrEntryFrequencyKHz'] -cne $FrequencyKHz)){return $null}
    if($WrongFrequency) {
        $bFrequencies=$HostFields['rrBFrequenciesKHz'].Split('|');$bNative=[int]$ClientFields['rrEntryNativeIndex']
        if($bNative -ge $bFrequencies.Count -or $ClientFields['rrEntryFrequencyKHz'] -cne $bFrequencies[$bNative]){return $null}
    }
    $sourceFrequencies=$HostFields['rrAFrequenciesKHz'].Split('|')
    if([int]$NativeIndex -ge $sourceFrequencies.Count -or $sourceFrequencies[[int]$NativeIndex] -cne $FrequencyKHz){return $null}
    if($HostFields['rrEntryKnown'] -cne '1' -or $HostFields['rrEntryRadio'] -cne '1' -or
        $HostFields['rrEntryIndex'] -cnotmatch '\A(?:[0-9]|1[0-5])\z' -or $HostFields['rrEntryNativeIndex'] -cne $NativeIndex -or
        $HostFields['rrEntryActive'] -cne '1' -or $HostFields['rrEntryUsable'] -cne '1' -or
        $HostFields['rrEntryBindingAllowed'] -cne '1' -or $HostFields['rrEntryFrequencyKHz'] -cne $FrequencyKHz){return $null}
    # Неактивный entry B не является запретом приёма; UNKNOWN radioOwnerRpl не превращается в ownership proof.
    return [pscustomobject]@{Observation='SCRIPT_STATE_ONLY';KeyObservationSource='A_SERVER_VIEW';BProxyPair=$(if($ClientFields['rrPairKnown'] -ceq '1'){'OBSERVED'}else{'UNQUALIFIED'});NativePermission='UNQUALIFIED';PowerMute='UNQUALIFIED';
        Signature=($NativeIndex+'|'+$FrequencyKHz+'|'+$HostFields['rrEntryIndex']+'|'+$HostFields['rrAFrequenciesKHz']+'|'+$HostFields['rrBFrequenciesKHz']+'|'+$HostFields['rrAKeyEmpty']+'|'+$ClientFields['rrPairKnown']+'|'+$ClientFields['rrEntryIndex']+'|'+$ClientFields['rrEntryNativeIndex'])}
}

function Get-WrongFrequencyRows([string]$Text,[string]$Run) {
    $pattern='\[RICON_CAPTURE_FREQ\] run='+[regex]::Escape($Run)+' kind=(?<kind>APPLIED|READBACK|RESTORE) view=(?<view>A_SERVER_B_CATALOG|B_LOCAL) (?<fields>[^\r\n]*)'
    foreach($m in [regex]::Matches($Text,$pattern)) {
        [pscustomobject]@{Kind=$m.Groups['kind'].Value;View=$m.Groups['view'].Value;Position=$m.Index;Fields=(Get-RehearsalFields $m.Groups['fields'].Value)}
    }
}

function Test-WrongFrequencyVector([string]$Value,[int]$Count) {
    return ($Count -ge 1 -and $Count -le 16 -and $Value -cmatch '\A[1-9][0-9]*(?:\|[1-9][0-9]*){0,15}\z' -and $Value.Split('|').Count -eq $Count)
}

# Две локальные шкалы времени не вычитаются друг из друга; proof остаётся sampled script-state.
function Get-WrongFrequencyProof([string]$HostText,[string]$ClientText,[string]$Run,[string]$BRpl,[string]$AFrequency,$HostFields,$ClientFields) {
    $server=@(Get-WrongFrequencyRows $HostText $Run)
    $applied=@($server|Where-Object{$_.Kind -ceq 'APPLIED' -and $_.View -ceq 'A_SERVER_B_CATALOG'})
    if($applied.Count -ne 1 -or @($server|Where-Object{$_.Kind -ceq 'RESTORE'}).Count){return $null}
    $a=$applied[0].Fields
    foreach($bit in @('known','catalogSame','allMismatch','aUnchanged','keysSame','powerMuteSame')){if($a[$bit] -cne '1'){return $null}}
    if($a['bEntityRpl'] -cne $BRpl -or $a['aFrequencyKHz'] -cne $AFrequency -or $a['catalogCount'] -cnotmatch '\A(?:[1-9]|1[0-6])\z' -or
        $a['rawTickMs'] -cnotmatch '\A[0-9]{1,10}\z' -or $a['nativeOwnership'] -cne 'UNQUALIFIED'){return $null}
    if([long]$a['rawTickMs'] -lt [long]$HostFields['startTickMs'] -or ([long]$a['rawTickMs']-[long]$HostFields['startTickMs']) -gt 57000){return $null}
    $count=[int]$a['catalogCount'];$original=$a['originalFrequenciesKHz'];$control=$a['frequenciesKHz']
    if(-not (Test-WrongFrequencyVector $original $count) -or -not (Test-WrongFrequencyVector $control $count) -or
        $AFrequency -cin $control.Split('|') -or $HostFields['rrBFrequenciesKHz'] -cne $control){return $null}
    $old=$original.Split('|');$new=$control.Split('|')
    for($i=0;$i -lt $count;$i++){if($old[$i] -cne $AFrequency -and $new[$i] -cne $old[$i]){return $null}}
    $last=@()
    foreach($view in @(@($server,'A_SERVER_B_CATALOG',$HostFields),@(@(Get-WrongFrequencyRows $ClientText $Run),'B_LOCAL',$ClientFields))) {
        $row=@($view[0]|Where-Object{$_.Kind -ceq 'READBACK' -and $_.View -ceq $view[1]}|Select-Object -Last 1)
        if($row.Count -ne 1){return $null};$f=$row[0].Fields;$heartbeat=$view[2]
        if($f['known'] -cne '1' -or $f['catalogSame'] -cne '1' -or $f['bEntityRpl'] -cne $BRpl -or $f['catalogCount'] -cne [string]$count -or
            $f['frequenciesKHz'] -cne $control -or $f['rawTickMs'] -cnotmatch '\A[0-9]{1,10}\z' -or
            [long]$f['rawTickMs'] -lt [long]$heartbeat['rawTickMs'] -or [long]$f['rawTickMs'] -lt [long]$heartbeat['startTickMs'] -or
            ([long]$f['rawTickMs']-[long]$heartbeat['startTickMs']) -gt 57000){return $null}
        if($view[1] -ceq 'A_SERVER_B_CATALOG') {
            if($row[0].Position -le $applied[0].Position -or [long]$f['rawTickMs'] -lt [long]$a['rawTickMs'] -or $f['aFrequencyKHz'] -cne $AFrequency){return $null}
            foreach($bit in @('allMismatch','aUnchanged','keysSame','powerMuteSame')){if($f[$bit] -cne '1'){return $null}}
        }
        $last+=$row[0]
    }
    return [pscustomobject]@{Signature=($a['rawTickMs']+'|'+$BRpl+'|'+$AFrequency+'|'+$count+'|'+$original+'|'+$control);
        BEntityRpl=$BRpl;AFrequencyKHz=$AFrequency;CatalogCount=$count;OriginalFrequenciesKHz=$original;ControlFrequenciesKHz=$control;
        Applied=$applied[0];ServerReadback=$last[0];BLocalReadback=$last[1];NativeOwnership='UNQUALIFIED';Replication='UNQUALIFIED';Atomic=$false}
}

function Get-WrongFrequencyRestore([string]$Text,[string]$Run,[string]$Attempt,$Proof) {
    $result=[pscustomobject]@{valid=$false;status='INCOMPLETE';serverRestore=$null;bReplication='UNQUALIFIED';nativeOwnership='UNQUALIFIED'}
    $rows=@(Get-WrongFrequencyRows $Text $Run|Where-Object{$_.Kind -ceq 'RESTORE' -and $_.View -ceq 'A_SERVER_B_CATALOG'})
    if(-not $Proof -or $rows.Count -ne 1){return $result}
    $result.serverRestore=$rows[0];$f=$rows[0].Fields;$result.status='CONFLICT_OR_MISMATCH'
    $prefix='\[RICON_PULSE\] seq=\d+ t=\d+ run='+[regex]::Escape($Run)+' attempt='+[regex]::Escape($Attempt)+' mode=capture kind='
    $stop=@([regex]::Matches($Text,$prefix+'STOP_REQUEST_RETURN '));$terminal=@([regex]::Matches($Text,$prefix+'TERMINAL '))
    if($stop.Count -ne 1 -or $terminal.Count -ne 1 -or $rows[0].Position -le $stop[0].Index -or $rows[0].Position -ge $terminal[0].Index){return $result}
    foreach($bit in @('frequenciesRestored','catalogSame','keysSame','powerMuteSame','aUnchanged')){if($f[$bit] -cne '1'){return $result}}
    if($f['bEntityRpl'] -cne $Proof.BEntityRpl -or $f['expectedCount'] -cne [string]$Proof.CatalogCount -or $f['restoredCount'] -cne [string]$Proof.CatalogCount -or
        $f['frequenciesKHz'] -cne $Proof.OriginalFrequenciesKHz -or $f['originalFrequenciesKHz'] -cne $Proof.OriginalFrequenciesKHz -or $f['conflict'] -cne '0' -or
        $f['rawTickMs'] -cnotmatch '\A[0-9]{1,10}\z' -or [long]$f['rawTickMs'] -lt [long]$Proof.ServerReadback.Fields['rawTickMs']){return $result}
    $result.valid=$true;$result.status='SERVER_RESTORE_OBSERVED';return $result
}

function Get-WrongFrequencyRxPhases([string]$ClientText,[string]$Run,$Proof,$Candidates) {
    $counts=[pscustomobject]@{afterDetunedBSample=0;afterRestoredBSample=0;unknown=0}
    $readback=@(Get-WrongFrequencyRows $ClientText $Run|Where-Object{$_.Kind -ceq 'READBACK' -and $_.View -ceq 'B_LOCAL'})
    $seen=@{};$rows=@()
    $pattern='\[RICON_NRX\] seq=(?<seq>\d+) t=\d+ side=(?<side>ownB|sourceAproxy) kind=RAW_RX[^\r\n]*? text=run='+[regex]::Escape($Run)+'(?=,)(?<fields>[^\r\n]*)'
    foreach($m in [regex]::Matches($ClientText,$pattern)) {
        $seq=[long]$m.Groups['seq'].Value;$side=$m.Groups['side'].Value;$key=[string]$seq+'|'+$side
        if($seen.ContainsKey($key) -or -not @($Candidates|Where-Object{$_.seq -eq $seq -and $_.tapOwner -ceq $side}).Count){continue};$seen[$key]=$true
        $last=@($readback|Where-Object{$_.Position -lt $m.Index}|Select-Object -Last 1);$phase='UNKNOWN'
        if($last.Count -eq 1 -and $last[0].Fields['known'] -ceq '1' -and $last[0].Fields['catalogSame'] -ceq '1' -and $last[0].Fields['bEntityRpl'] -ceq $Proof.BEntityRpl) {
            if($last[0].Fields['frequenciesKHz'] -ceq $Proof.ControlFrequenciesKHz){$phase='AFTER_DETUNED_B_SAMPLE'}
            elseif($last[0].Fields['frequenciesKHz'] -ceq $Proof.OriginalFrequenciesKHz){$phase='AFTER_RESTORED_B_SAMPLE'}
        }
        if($phase -ceq 'AFTER_DETUNED_B_SAMPLE'){$counts.afterDetunedBSample++}elseif($phase -ceq 'AFTER_RESTORED_B_SAMPLE'){$counts.afterRestoredBSample++}else{$counts.unknown++}
        $rows+=[pscustomobject]@{seq=$seq;tapOwner=$side;phase=$phase;tuningAtCallback='UNQUALIFIED'}
    }
    return [pscustomobject]@{counts=$counts;callbacks=$rows;phaseBasis='LAST_B_LOCAL_SAMPLE_ONLY';crossProcessClockAlignment='UNQUALIFIED';queueDrained='UNKNOWN';rfConclusion='NOT_EVALUATED'}
}

function Get-RehearsalRows([string]$Text,[string]$Run,[string]$Attempt,[switch]$Nrx,[ValidateSet('rehearsal','capture')][string]$Mode='rehearsal') {
    $runPattern=[regex]::Escape($Run)
    $pattern=if($Nrx){'\[RICON_NRX\] seq=(?<seq>\d+) t=(?<tick>\d+) side=process kind=(?<kind>[A-Za-z_]+)[^\r\n]*? text=run='+$runPattern+'(?=,)(?<fields>[^\r\n]*)'}
        else{'\[RICON_PULSE\] seq=(?<seq>\d+) t=(?<tick>\d+) run='+$runPattern+' attempt='+[regex]::Escape($Attempt)+' mode='+$Mode+' kind=(?<kind>[A-Za-z_]+) (?<fields>[^\r\n]*)'}
    foreach($match in [regex]::Matches($Text,$pattern)) {
        [pscustomobject]@{ Seq=[long]$match.Groups['seq'].Value; Tick=[long]$match.Groups['tick'].Value;
            Kind=$match.Groups['kind'].Value; Fields=(Get-RehearsalFields $match.Groups['fields'].Value) }
    }
}

function Get-SeparatedDistance($A,$B) {
    $sum=0.0;for($i=0;$i -lt 3;$i++){$delta=[double]$A[$i]-[double]$B[$i];$sum+=$delta*$delta};return [math]::Sqrt($sum)
}

function Get-SeparatedGeometry([string]$HostText,[string]$ClientText,[string]$Run,[string]$Attempt,$Bound,$Heart) {
    $views=@()
    foreach($side in @(@($HostText,'A_SERVER',1,0),@($ClientText,'B_LOCAL',2,1))) {
        $row=@(Get-RehearsalRows $side[0] $Run $Attempt -Nrx|Where-Object{$_.Kind -ceq 'LIVE_GEOMETRY'}|Select-Object -Last 1)
        if($row.Count -ne 1){return $null};$r=$row[0];$f=$r.Fields;$index=$side[3]
        if($f['geoView'] -cne $side[1] -or $f['localPlayer'] -cne [string]$side[2] -or $f['geoKnown'] -cne '1' -or $f['geoEligible'] -cne '1' -or
            $f['geoOwnRadioKnown'] -cne '1' -or $f['geoScope'] -cne 'LIVE_REFERENCES' -or $f['bindingGeneration'] -cne $Bound[$index].Fields['bindingGeneration'] -or
            $f['startTickMs'] -cne $Bound[$index].Fields['startTickMs'] -or $r.Seq -le $Heart[$index].Seq -or
            $f['geoActorARpl'] -cne $Bound[0].Fields['ownRpl'] -or $f['geoActorBRpl'] -cne $Bound[1].Fields['ownRpl'] -or
            $f['geoCatalogCount'] -cnotmatch '\A(?:[1-9]|1[0-6])\z' -or $f['geoCatalogCount'] -cne $Bound[$index].Fields['catalog'] -or
            $f['geoCatalogCount'] -cne $Heart[$index].Fields['catalog'] -or $f['geoNativeIndices'] -cnotmatch '\A[0-3](?:\|[0-3]){0,15}\z' -or
            $f['geoNativeIndices'].Split('|').Count -ne [int]$f['geoCatalogCount'] -or
            $f['rawTickMs'] -cnotmatch '\A[0-9]{1,10}\z' -or $f['geoObservedTickMs'] -cnotmatch '\A[0-9]{1,10}\z'){return $null}
        $raw=[long]$f['rawTickMs'];$observed=[long]$f['geoObservedTickMs']
        if($raw -lt [long]$Heart[$index].Fields['rawTickMs'] -or $raw -lt [long]$f['startTickMs'] -or
            $raw-[long]$f['startTickMs'] -gt 57000 -or $observed -gt $raw -or $raw-$observed -gt 500){return $null}
        $n=@{};$keys=@('geoAX','geoAY','geoAZ','geoBX','geoBY','geoBZ','geoDistance3D','geoOwnRadioX','geoOwnRadioY','geoOwnRadioZ','geoOwnRadioOffset')
        foreach($key in $keys) {
            $value=0.0
            if(-not [double]::TryParse([string]$f[$key],[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$value) -or
                [double]::IsNaN($value) -or [double]::IsInfinity($value)){return $null};$n[$key]=$value
        }
        $a=@($n.geoAX,$n.geoAY,$n.geoAZ);$b=@($n.geoBX,$n.geoBY,$n.geoBZ);$radio=@($n.geoOwnRadioX,$n.geoOwnRadioY,$n.geoOwnRadioZ)
        $distance=Get-SeparatedDistance $a $b;$owner=if($index -eq 0){$a}else{$b};$offset=Get-SeparatedDistance $owner $radio
        if((Get-SeparatedDistance @($a[0],0,$a[2]) @(128,0,128)) -gt 0.5 -or (Get-SeparatedDistance @($b[0],0,$b[2]) @(28,0,128)) -gt 2 -or
            [math]::Abs($a[1]-1) -gt 0.5 -or [math]::Abs($b[1]-1) -gt 0.5 -or [math]::Abs($distance-100) -gt 2 -or
            [math]::Abs($distance-$n.geoDistance3D) -gt 0.05 -or $offset -gt 0.5 -or $n.geoOwnRadioOffset -lt 0 -or
            $n.geoOwnRadioOffset -gt 0.5 -or [math]::Abs($offset-$n.geoOwnRadioOffset) -gt 0.05){return $null}
        $stamp=@([string]$r.Seq,$f['rawTickMs'],$f['geoObservedTickMs'])+@($keys|ForEach-Object{$f[$_]})
        $views+=[pscustomobject]@{Seq=$r.Seq;RawTick=$raw;StartTick=$f['startTickMs'];Generation=$f['bindingGeneration'];ActorA=$a;ActorB=$b;Radio=$radio;
            Stamp=($stamp -join '|');View=$f['geoView'];Fields=$f}
    }
    if((Get-SeparatedDistance $views[0].ActorA $views[1].ActorA) -gt 0.5 -or (Get-SeparatedDistance $views[0].ActorB $views[1].ActorB) -gt 0.5){return $null}
    $radioDistance=Get-SeparatedDistance $views[0].Radio $views[1].Radio
    if([math]::Abs($radioDistance-100) -gt 2){return $null}
    $identity=@($Run,$Bound[0].Fields['ownRpl'],$Bound[1].Fields['ownRpl'],$views[0].Generation,$views[1].Generation,$views[0].StartTick,$views[1].StartTick,
        $views[0].Fields['geoCatalogCount'],$views[0].Fields['geoNativeIndices'],$views[1].Fields['geoCatalogCount'],$views[1].Fields['geoNativeIndices']) -join '|'
    return [pscustomobject]@{Identity=$identity;Stamp=($identity+'|'+$views[0].Stamp+'|'+$views[1].Stamp);A=$views[0];B=$views[1];RadioDistance=$radioDistance;
        Freshness=$null;scope='SAMPLED_LIVE_GEOMETRY_ONLY';radioDelivery='UNQUALIFIED'}
}

# Первое чтение только задаёт baseline; свежесть требует нового LIVE tick обеих views между чтениями.
function Test-SeparatedGeometryFresh($State,$Geometry) {
    $now=[Diagnostics.Stopwatch]::GetTimestamp();$frequency=[Diagnostics.Stopwatch]::Frequency
    if(-not $State.GeometryTracking){
        $State.GeometryTracking=[pscustomobject]@{Identity=$Geometry.Identity;A=[pscustomobject]@{Seq=$Geometry.A.Seq;Raw=$Geometry.A.RawTick;Stamp=$Geometry.A.Stamp;AdvanceTicks=0L};B=[pscustomobject]@{Seq=$Geometry.B.Seq;Raw=$Geometry.B.RawTick;Stamp=$Geometry.B.Stamp;AdvanceTicks=0L}}
        return $false
    }
    if($State.GeometryTracking.Identity -cne $Geometry.Identity){throw 'GEOMETRY_IDENTITY_CHANGED'}
    foreach($name in @('A','B')) {
        $seen=$State.GeometryTracking.$name;$current=$Geometry.$name
        if($current.Seq -lt $seen.Seq -or $current.RawTick -lt $seen.Raw){throw 'GEOMETRY_CLOCK_OR_SEQUENCE_RESET'}
        if($current.Seq -gt $seen.Seq -and $current.RawTick -gt $seen.Raw){$seen.Seq=$current.Seq;$seen.Raw=$current.RawTick;$seen.Stamp=$current.Stamp;$seen.AdvanceTicks=$now}
        elseif($current.Stamp -cne $seen.Stamp){throw 'GEOMETRY_EVENT_CHANGED'}
    }
    foreach($name in @('A','B')){if($State.GeometryTracking.$name.AdvanceTicks -le 0 -or 1000.0*($now-$State.GeometryTracking.$name.AdvanceTicks)/$frequency -gt 2000){return $false}}
    $Geometry.Freshness=[pscustomobject]@{verifiedTicks=$now;observedATicks=$State.GeometryTracking.A.AdvanceTicks;observedBTicks=$State.GeometryTracking.B.AdvanceTicks;frequency=$frequency;maxAgeMs=2000}
    return $true
}

function Get-VoNSoundAdmission([string]$Text,[string]$Run,[string]$Attempt) {
    $result=[pscustomobject]@{status='UNKNOWN';reason='';proof=$null}
    $rows=@(Get-RehearsalRows $Text $Run $Attempt -Nrx);$bound=@($rows|Where-Object{$_.Kind -ceq 'BOUND'}|Select-Object -Last 1);$heart=@($rows|Where-Object{$_.Kind -ceq 'HEARTBEAT'}|Select-Object -Last 1)
    if($bound.Count -ne 1 -or $bound[0].Fields['localPlayer'] -cne '2'){return $result};$b=$bound[0].Fields
    $pattern='\[RICON_VSOUND\] seq=(?<seq>\d+) run='+[regex]::Escape($Run)+' kind=(?<kind>READY|NOT_AVAILABLE|TERMINAL) rawTickMs=(?<raw>\d+) startTickMs=(?<start>\d+) text=(?<fields>[^\r\n]*)'
    $markers=@([regex]::Matches($Text,$pattern));if(-not $markers.Count){return $result}
    $m=$markers[-1];$f=Get-RehearsalFields $m.Groups['fields'].Value
    if($f['localPlayer'] -cne '2' -or $f['bindingGeneration'] -cne $b['bindingGeneration'] -or $m.Groups['start'].Value -cne $b['startTickMs'] -or
        $f['ownBRpl'] -cne $b['ownRpl'] -or $f['proxyARpl'] -cne $b['proxyARpl'] -or -not $f['ownBRpl'] -or $f['ownBRpl'] -ceq 'UNKNOWN' -or -not $f['proxyARpl'] -or $f['proxyARpl'] -ceq 'UNKNOWN'){return $result}
    $kind=$m.Groups['kind'].Value
    if($kind -ceq 'NOT_AVAILABLE') {
        if($f['current'] -ceq '0' -and $f['reason'] -cin @('COMPONENT_MISSING','SCRIPTED_DISABLED','INVOKER_MISSING','ALIASED_HOOK')){$result.status='NOT_AVAILABLE';$result.reason=$f['reason']}
        return $result
    }
    if($kind -ceq 'TERMINAL'){
        if($f['result'] -ceq 'NOT_AVAILABLE' -and $f['current'] -ceq '0' -and $f['reason'] -cin @('COMPONENT_MISSING','SCRIPTED_DISABLED','INVOKER_MISSING','ALIASED_HOOK') -and
            -not @($markers|Where-Object{$_.Groups['kind'].Value -ceq 'READY'}).Count){$result.status='NOT_AVAILABLE';$result.reason=$f['reason']}
        else{$result.status='INVALID_BINDING';$result.reason=if($f['reason'] -cin @('BINDING_CHANGED','CLOCK_OR_STALL','LIMIT')){$f['reason']}else{'UNQUALIFIED_TERMINAL'}}
        return $result
    }
    if($heart.Count -ne 1 -or $f['ownBReady'] -cne '1' -or $f['aProxyReady'] -cne '1' -or $f['ownBSubscriptions'] -cne '2' -or $f['aProxySubscriptions'] -cne '2' -or
        $f['subscriptionGeneration'] -cne '1' -or $f['current'] -cne '1' -or $f['observerStartTickMs'] -cnotmatch '\A[0-9]{1,10}\z' -or
        [long]$m.Groups['raw'].Value -lt [long]$b['startTickMs']){return $result}
    $readyTick=[long]$m.Groups['raw'].Value;$heartbeatTick=[long]$heart[0].Fields['rawTickMs']
    if($heartbeatTick-$readyTick -gt 500){$result.status='STALE_READ';$result.reason='READY_BEHIND_B_ACTIVITY';return $result}
    # Это граница опережения расписания HB1s, а не разрешённый возраст READY.
    if($readyTick-$heartbeatTick -gt 1000){$result.status='INVALID_CLOCK';$result.reason='READY_FUTURE_OF_B_CADENCE';return $result}
    $result.status='READY'
    $result.proof=[pscustomobject]@{Signature=($Run+'|'+$b['startTickMs']+'|'+$b['bindingGeneration']+'|'+$f['ownBRpl']+'|'+$f['proxyARpl']+'|'+$f['observerStartTickMs']+'|1|2|2');
        seq=[long]$m.Groups['seq'].Value;rawTickMs=[long]$m.Groups['raw'].Value;startTickMs=$b['startTickMs'];ownBRpl=$f['ownBRpl'];proxyARpl=$f['proxyARpl'];Freshness=$null;scope='OBSERVER_SUBSCRIPTIONS_ONLY';nativeStreamBinding='UNQUALIFIED';rxJointToken='ABSENT'}
    return $result
}
function Test-VoNSoundFresh($State,$Proof) {
    $now=[Diagnostics.Stopwatch]::GetTimestamp();$frequency=[Diagnostics.Stopwatch]::Frequency
    if(-not $State.SoundTracking){$State.SoundTracking=[pscustomobject]@{Signature=$Proof.Signature;Seq=$Proof.seq;Raw=$Proof.rawTickMs;AdvanceTicks=0L};$State.SoundStatus='WAIT_READY_PROGRESS';return $false}
    $seen=$State.SoundTracking
    if($seen.Signature -cne $Proof.Signature){throw 'SOUND_BINDING_DRIFT'}
    if($Proof.seq -lt $seen.Seq -or $Proof.rawTickMs -lt $seen.Raw){throw 'SOUND_CLOCK_OR_SEQUENCE_RESET'}
    if($Proof.seq -gt $seen.Seq -and $Proof.rawTickMs -gt $seen.Raw){$seen.Seq=$Proof.seq;$seen.Raw=$Proof.rawTickMs;$seen.AdvanceTicks=$now}
    elseif($Proof.seq -ne $seen.Seq -or $Proof.rawTickMs -ne $seen.Raw){throw 'SOUND_EVENT_INCONSISTENT'}
    if($seen.AdvanceTicks -le 0){$State.SoundStatus='WAIT_READY_PROGRESS';return $false}
    if(1000.0*($now-$seen.AdvanceTicks)/$frequency -gt 500){$State.SoundStatus='STALE_READ';return $false}
    $State.SoundStatus='READY';$Proof.Freshness=[pscustomobject]@{observedProgressTicks=$seen.AdvanceTicks;verifiedTicks=$now;frequency=$frequency;maxAgeMs=500};return $true
}
function Get-BPowerOffProof([string]$HostText,[string]$ClientText,[string]$Run,[string]$Attempt,$Bound,$Heart,[switch]$Direct) {
    # Preset выбирается по явному режиму до проверки sample, дедлайн не продлевается.
    $expectedPlan=if($Direct){'BPowerOff.Direct10s.1'}else{'BPowerOff.1'};$expectedHold=if($Direct){10000}else{6000}
    $views=@();$pattern='\[RICON_CPOWER\] run='+[regex]::Escape($Run)+' view=(?<view>A_SERVER_B_MIRROR|B_LOCAL) kind=READBACK rawTickMs=(?<tick>[0-9]{1,10}) text=(?<fields>[^\r\n]*)'
    foreach($side in @(@($HostText,'A_SERVER_B_MIRROR',0),@($ClientText,'B_LOCAL',1))) {
        $matches=@([regex]::Matches($side[0],$pattern)|Where-Object{$_.Groups['view'].Value -ceq $side[1]})
        if(-not $matches.Count){return $null};$m=$matches[-1];$f=Get-RehearsalFields $m.Groups['fields'].Value;$i=$side[2];$tick=[long]$m.Groups['tick'].Value
        foreach($bit in @('currentRefs','catalogSame','protectedSame','powerKnown')){if($f[$bit] -cne '1'){return $null}}
        if($f['powered'] -cne '0' -or $f['powerPlanId'] -cne $expectedPlan -or $f['bEntityRpl'] -cne $Bound[1].Fields['ownRpl'] -or
            $f['bindingGeneration'] -cne $Bound[$i].Fields['bindingGeneration'] -or $f['startTickMs'] -cne $Bound[$i].Fields['startTickMs'] -or
            $f['trCount'] -cne $Bound[1].Fields['catalog'] -or $f['trCount'] -cnotmatch '\A[1-4]\z' -or $f['nativeIndices'] -cnotmatch '\A[0-3](?:\|[0-3]){0,3}\z' -or
            $f['nativeIndices'].Split('|').Count -ne [int]$f['trCount'] -or $f['observerStampMs'] -cnotmatch '\A[0-9]{1,10}\z' -or
            [long]$f['observerStampMs'] -gt $tick -or $tick-[long]$f['observerStampMs'] -gt 500 -or $tick -lt [long]$f['startTickMs'] -or
            $tick-[long]$f['startTickMs'] -gt 57000 -or [long]$Heart[$i].Fields['rawTickMs']-$tick -gt 500 -or $tick-[long]$Heart[$i].Fields['rawTickMs'] -gt 1000 -or
            $f['radioRefLocal'] -cnotmatch '\A[A-Za-z_][A-Za-z0-9_]*<0x[0-9A-Fa-f]+>\z' -or $f['radioRefLocal'] -match '<0x0+>'){return $null}
        $views+=[pscustomobject]@{Tick=$tick;Fields=$f}
    }
    $a=$views[0].Fields;$b=$views[1].Fields
    if($a['nativeIndices'] -cne $b['nativeIndices'] -or $a['trCount'] -cne $b['trCount']){return $null}
    $catalog=@(Get-RehearsalRows $ClientText $Run $Attempt -Nrx|Where-Object{$_.Kind -ceq 'BOUND_CATALOG'})
    if($catalog.Count -ne [int]$b['trCount'] -or (@($catalog|ForEach-Object{$_.Fields['nativeIndex']}) -join '|') -cne $b['nativeIndices']){return $null}
    foreach($entry in $catalog){if($entry.Fields['radioRefLocal'] -cne $b['radioRefLocal'] -or $entry.Fields['bindingGeneration'] -cne $b['bindingGeneration'] -or $entry.Fields['startTickMs'] -cne $b['startTickMs']){return $null}}
    foreach($key in @('offCallTickMs','holdDeadlineTickMs','holdRemainingMs')){if($b[$key] -cnotmatch '\A[0-9]{1,10}\z'){return $null}}
    $off=[long]$b['offCallTickMs'];$deadline=[long]$b['holdDeadlineTickMs']
    if($b['holdMs'] -cne [string]$expectedHold -or $b['offCalls'] -cne '1' -or $b['restoreOnCalls'] -cne '0' -or $deadline-$off -ne $expectedHold -or $off -lt [long]$b['startTickMs'] -or $views[1].Tick -lt $off -or
        [long]$b['holdRemainingMs'] -ne $deadline-$views[1].Tick){return $null}
    $identity=@($Run,$expectedPlan,$b['bEntityRpl'],$a['startTickMs'],$b['startTickMs'],$a['bindingGeneration'],$b['bindingGeneration'],$a['radioRefLocal'],$b['radioRefLocal'],$b['trCount'],$b['nativeIndices'],[string]$off,[string]$deadline) -join '|'
    return [pscustomobject]@{Identity=$identity;PlanId=$expectedPlan;OffCallTick=$off;DeadlineTick=$deadline;BTick=$views[1].Tick;A=$views[0];B=$views[1];Freshness=$null;RemainingConservativeMs=$null;scope='ACTUAL_GETTER_OFF_SAMPLED_NOT_RADIO_EFFECT_PROOF'}
}
function Test-BPowerOffFresh($State,$Proof) {
    $now=[Diagnostics.Stopwatch]::GetTimestamp();$frequency=[Diagnostics.Stopwatch]::Frequency
    if(-not $State.PowerTracking){$State.PowerTracking=[pscustomobject]@{Identity=$Proof.Identity;A=[pscustomobject]@{Tick=$Proof.A.Tick;Observed=0L};B=[pscustomobject]@{Tick=$Proof.B.Tick;Observed=0L}};return $false}
    if($State.PowerTracking.Identity -cne $Proof.Identity){throw 'POWER_PLAN_OR_REFS_DRIFT'}
    foreach($view in @('A','B')){$old=$State.PowerTracking.$view;$tick=$Proof.$view.Tick;if($tick -lt $old.Tick){throw 'POWER_CLOCK_RESET'};if($tick -gt $old.Tick){$old.Tick=$tick;$old.Observed=$now}}
    foreach($view in @('A','B')){if($State.PowerTracking.$view.Observed -le 0 -or 1000.0*($now-$State.PowerTracking.$view.Observed)/$frequency -gt 500){return $false}}
    $Proof.Freshness=[pscustomobject]@{ObservedATicks=$State.PowerTracking.A.Observed;ObservedBTicks=$State.PowerTracking.B.Observed;VerifiedTicks=$now;Frequency=$frequency}
    return $true
}
function Get-BPowerOffRemaining($Proof,[long]$NowTicks) {
    if(-not $Proof.Freshness -or $NowTicks -lt $Proof.Freshness.ObservedBTicks){return -1}
    return $Proof.DeadlineTick-$Proof.BTick-500-[math]::Ceiling(1000.0*($NowTicks-$Proof.Freshness.ObservedBTicks)/$Proof.Freshness.Frequency)
}

# Дистанция 3 м ограничивает одну fixture, а не определяет native voice range.
function Get-DirectNear([string]$HostText,[string]$ClientText,[string]$Run,[string]$Attempt,$Bound,$Heart) {
    $views=@()
    foreach($side in @(@($HostText,'A_SERVER',1,0),@($ClientText,'B_LOCAL',2,1))) {
        $rows=@(Get-RehearsalRows $side[0] $Run $Attempt -Nrx|Where-Object{$_.Kind -ceq 'LIVE_DIRECT_NEAR'}|Select-Object -Last 1)
        if($rows.Count -ne 1){return $null};$r=$rows[0];$f=$r.Fields;$i=$side[3]
        if($f['geoView'] -cne $side[1] -or $f['localPlayer'] -cne [string]$side[2] -or $f['geoKnown'] -cne '1' -or $f['geoEligible'] -cne '1' -or
            $f['geoScope'] -cne 'LIVE_REFERENCES' -or $f['bindingGeneration'] -cne $Bound[$i].Fields['bindingGeneration'] -or $f['startTickMs'] -cne $Bound[$i].Fields['startTickMs'] -or
            $f['geoActorARpl'] -cne $Bound[0].Fields['ownRpl'] -or $f['geoActorBRpl'] -cne $Bound[1].Fields['ownRpl'] -or $r.Seq -le $Heart[$i].Seq -or
            $f['rawTickMs'] -cnotmatch '\A[0-9]{1,10}\z' -or $f['geoObservedTickMs'] -cnotmatch '\A[0-9]{1,10}\z'){return $null}
        $raw=[long]$f['rawTickMs'];$observed=[long]$f['geoObservedTickMs']
        if($raw -lt [long]$Heart[$i].Fields['rawTickMs'] -or $raw -lt [long]$f['startTickMs'] -or $raw-[long]$f['startTickMs'] -gt 57000 -or $observed -gt $raw -or $raw-$observed -gt 500){return $null}
        $n=@{};$keys=@('geoAX','geoAY','geoAZ','geoBX','geoBY','geoBZ','geoDistance3D')
        foreach($key in $keys){$v=0.0;if(-not [double]::TryParse([string]$f[$key],[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$v) -or [double]::IsNaN($v) -or [double]::IsInfinity($v)){return $null};$n[$key]=$v}
        $a=@($n.geoAX,$n.geoAY,$n.geoAZ);$b=@($n.geoBX,$n.geoBY,$n.geoBZ);$distance=Get-SeparatedDistance $a $b
        if($distance -gt 3 -or $n.geoDistance3D -lt 0 -or [math]::Abs($distance-$n.geoDistance3D) -gt 0.05){return $null}
        $stamp=@([string]$r.Seq,$f['rawTickMs'],$f['geoObservedTickMs'])+@($keys|ForEach-Object{$f[$_]})
        $views+=[pscustomobject]@{Seq=$r.Seq;RawTick=$raw;StartTick=$f['startTickMs'];Generation=$f['bindingGeneration'];ActorA=$a;ActorB=$b;Stamp=($stamp -join '|');View=$f['geoView'];Fields=$f}
    }
    if((Get-SeparatedDistance $views[0].ActorA $views[1].ActorA) -gt 0.5 -or (Get-SeparatedDistance $views[0].ActorB $views[1].ActorB) -gt 0.5){return $null}
    $identity=@($Run,'DIRECT',$Bound[0].Fields['ownRpl'],$Bound[1].Fields['ownRpl'],$views[0].Generation,$views[1].Generation,$views[0].StartTick,$views[1].StartTick) -join '|'
    return [pscustomobject]@{Identity=$identity;Stamp=($identity+'|'+$views[0].Stamp+'|'+$views[1].Stamp);A=$views[0];B=$views[1];Freshness=$null;scope='DIRECT_SAMPLED_NEAR_TEST_FIXTURE';nativeRange='UNQUALIFIED'}
}

function Get-RehearsalSnapshot([string]$HostText,[string]$ClientText,[string]$Run,[string]$Attempt,[switch]$Capture,[switch]$NoSend,[switch]$WrongFrequency,[switch]$SeparatedPair,[switch]$Short400,[switch]$BlindTrial,[switch]$VoNSoundObserve,[switch]$BPowerOff,[switch]$Direct) {
    if($BPowerOff -and (-not $Capture -or (-not $SeparatedPair -and -not $Direct) -or $NoSend -or $WrongFrequency -or $Short400 -or $BlindTrial -or $VoNSoundObserve)){return $null}
    if($Direct -and (-not $Capture -or $SeparatedPair -or $NoSend -or $WrongFrequency -or $Short400 -or $BlindTrial -or $VoNSoundObserve)){return $null}
    $mode=if($Capture){'capture'}else{'rehearsal'}
    $pulse=@(Get-RehearsalRows $HostText $Run $Attempt -Mode $mode)
    if(-not $pulse.Count -or $pulse[-1].Kind -cne 'PREPARED') { return $null }
    $prepared=$pulse[-1]; $p=$prepared.Fields
    if($BlindTrial){if(-not $Capture -or -not $SeparatedPair -or $Short400 -or $NoSend -or $WrongFrequency -or $p['blindTrial'] -cne '1' -or $p['choiceState'] -cne 'UNSELECTED' -or $p['plannedIntendedMs'] -cne 'UNSELECTED' -or $p['blindChoice']){return $null}}
    elseif($p['blindTrial']){return $null}
    elseif($Short400){if(-not $Capture -or -not $SeparatedPair -or $NoSend -or $WrongFrequency -or $p['short400'] -cne '1' -or $p['plannedIntendedMs'] -cne '400'){return $null}}
    elseif($p['short400'] -or $p['plannedIntendedMs']){return $null}
    $control=if($Direct -and $BPowerOff){'DIRECT_B_POWER_OFF'}elseif($Direct){'DIRECT'}elseif($BPowerOff){'B_POWER_OFF'}elseif($NoSend){'NO_SEND'}elseif($WrongFrequency){'WRONG_FREQUENCY'}else{''}
    if(($NoSend -and $WrongFrequency) -or (($NoSend -or $WrongFrequency) -and -not $Capture) -or (($p['control'] -cne $control) -and ($control -or $p['control']))){return $null}
    if($p['scriptIdleOnly'] -cne '1' -or $p['preparedGeneration'] -cnotmatch '\A[1-9][0-9]*\z') { return $null }
    $bound=@(); $heart=@()
    foreach($side in @(@($HostText,1),@($ClientText,2))) {
        $rows=@(Get-RehearsalRows $side[0] $Run $Attempt -Nrx)
        if(@($rows|Where-Object{$_.Kind -in @('TERMINAL','GAP')}).Count) { return $null }
        $b=@($rows|Where-Object{$_.Kind -ceq 'BOUND'}|Select-Object -Last 1)
        $h=@($rows|Where-Object{$_.Kind -ceq 'HEARTBEAT'}|Select-Object -Last 1)
        if($b.Count -ne 1 -or $h.Count -ne 1) { return $null }
        $bf=$b[0].Fields; $hf=$h[0].Fields
        if($bf['localPlayer'] -cne [string]$side[1] -or $hf['localPlayer'] -cne [string]$side[1] -or $hf['contextValid'] -cne '1' -or
            $hf['bindingGeneration'] -cne $bf['bindingGeneration'] -or $hf['startTickMs'] -cne $bf['startTickMs'] -or
            $h[0].Seq -le $b[0].Seq -or ([long]$hf['rawTickMs']-[long]$hf['startTickMs']) -gt 57000) { return $null }
        $bound+=$b[0]; $heart+=$h[0]
    }
    if(-not $bound[0].Fields['ownRpl'] -or $bound[0].Fields['ownRpl'] -ceq 'UNKNOWN' -or
        $bound[1].Fields['proxyARpl'] -cne $bound[0].Fields['ownRpl'] -or -not $bound[1].Fields['ownRpl'] -or $bound[1].Fields['ownRpl'] -ceq 'UNKNOWN') { return $null }
    if($Direct -and ($p['directRequested'] -cne '1' -or $p['plannedMethodDIRECT'] -cne '1' -or $p['actualMethodDIRECT'] -cnotmatch '\A[01]\z' -or $p['actualTxNull'] -cnotmatch '\A[01]\z')){return $null}
    $rr=$null
    if($Capture -and -not $Direct){$rr=Get-CaptureRadioReadiness $heart[0].Fields $heart[1].Fields $p['nativeIndex'] $p['frequencyKHz'] -WrongFrequency:$WrongFrequency;if(-not $rr){return $null}}
    $frequencyProof=$null
    if($WrongFrequency){$frequencyProof=Get-WrongFrequencyProof $HostText $ClientText $Run $bound[1].Fields['ownRpl'] $p['frequencyKHz'] $heart[0].Fields $heart[1].Fields;if(-not $frequencyProof){return $null}}
    $geometry=$null
    if($Direct){$geometry=Get-DirectNear $HostText $ClientText $Run $Attempt $bound $heart;if(-not $geometry){return $null}}
    if($SeparatedPair){if(-not $Capture){return $null};$geometry=Get-SeparatedGeometry $HostText $ClientText $Run $Attempt $bound $heart;if(-not $geometry){return $null}}
    $sound=$null
    if($VoNSoundObserve){$admission=Get-VoNSoundAdmission $ClientText $Run $Attempt;if($admission.status -cne 'READY'){return $null};$sound=$admission.proof}
    $power=$null
    if($BPowerOff){$power=Get-BPowerOffProof $HostText $ClientText $Run $Attempt $bound $heart -Direct:$Direct;if(-not $power){return $null}}
    $signature=@($prepared.Tick,$p['preparedGeneration'],$p['nativeIndex'],$p['frequencyKHz'])
    foreach($b in $bound){$signature+=@($b.Fields['bindingGeneration'],$b.Fields['startTickMs'],$b.Fields['ownRpl'],$b.Fields['proxyARpl'])}
    return [pscustomobject]@{ Signature=(($signature -join '|')+$(if($rr){'|'+$rr.Signature}else{''})+$(if($frequencyProof){'|'+$frequencyProof.Signature}else{''})+$(if($geometry){'|'+$geometry.Identity}else{''})+$(if($sound){'|'+$sound.Signature}else{''})+$(if($power){'|'+$power.Identity}else{''})+$(if($BlindTrial){'|blind|UNSELECTED'}elseif($Short400){'|short400|400'}else{''})); PowerAdmission=$power; SoundAdmission=$sound; PlannedIntendedMs=$(if($BlindTrial){0}elseif($Short400){400}else{1000}); Readiness=$rr; FrequencyProof=$frequencyProof; Geometry=$geometry; NativeIndex=$p['nativeIndex']; FrequencyKHz=$p['frequencyKHz']; PreparedGeneration=$p['preparedGeneration']; HostOwnRpl=$bound[0].Fields['ownRpl']; BOwnRpl=$bound[1].Fields['ownRpl']; HostHeartSeq=$heart[0].Seq; BHeartSeq=$heart[1].Seq; HostStart=$bound[0].Fields['startTickMs']; BStart=$bound[1].Fields['startTickMs'] }
}

function Assert-RehearsalTargetWindow($State,[Diagnostics.Process]$Target,[Diagnostics.Process]$Client,[IntPtr]$Main) {
    if(-not [object]::ReferenceEquals($State.Target,$Target) -or $Target.HasExited -or $Target.Handle -ne $State.TargetHandle -or
        $Target.StartTime.ToUniversalTime().Ticks -ne $State.TargetStart -or $Main -eq [IntPtr]::Zero -or $Main -ne $State.Main) { throw 'TARGET_OR_WINDOW_CHANGED' }
    if(-not [object]::ReferenceEquals($State.Client,$Client) -or $Client.HasExited -or $Client.StartTime.ToUniversalTime().Ticks -ne $State.ClientStart) { throw 'CLIENT_CHANGED' }
    $element=[Windows.Automation.AutomationElement]::FromHandle($Main)
    if(-not $element -or $element.Current.ProcessId -ne $Target.Id -or [IntPtr]$element.Current.NativeWindowHandle -ne $Main -or
        -not $element.Current.Name.StartsWith('World Editor - ',[StringComparison]::Ordinal)) { throw 'WINDOW_NOT_OWNED_EDITOR' }
}

function Get-RehearsalForegroundRole([uint32]$TargetId,[uint32]$ClientId) {
    try { return [RiconRehearsalInvokeV1]::ForegroundRole($TargetId,$ClientId) } catch { return 'UNKNOWN' }
}

function Get-RehearsalMenuObservation([IntPtr]$Main,[uint32]$TargetId,[uint32]$ClientId,$Prep=$null,[ValidateSet('Plugins','Game')][string]$MenuName='Plugins') {
    $d=[pscustomobject]@{menu=$MenuName;foreground='UNKNOWN';mainPresent=$false;mainIsEnabled=$null;mainIsOffscreen=$null;
        present=$false;isEnabled=$null;isOffscreen=$null;expandState='UNKNOWN';
        prepResult='NOT_RETURNED';opens=$null;patternType='NULL';labels=@();labelCap=40;labelSource='MENU_READ';errors=@()}
    try { $d.foreground=Get-RehearsalForegroundRole $TargetId $ClientId } catch { $d.errors+=$_.Exception.GetType().Name }
    try {
        $mainElement=$U::FromHandle($Main);$d.mainPresent=($null -ne $mainElement)
        if($mainElement){$d.mainIsEnabled=$mainElement.Current.IsEnabled;$d.mainIsOffscreen=$mainElement.Current.IsOffscreen}
    } catch { $d.errors+=$_.Exception.GetType().Name }
    $names=@();$top=$null
    if($Prep) {
        try {
            $d.prepResult=if($Prep.result -cmatch '\A[A-Z_]+\z'){$Prep.result}else{'UNEXPECTED_RESULT'}
            $d.opens=$Prep.opens
            if($Prep.ip){$d.patternType=$Prep.ip.GetType().FullName}
            if($Prep.names){$names=@($Prep.names);$d.labelSource='PREP_NAMES'}
        } catch { $d.errors+=$_.Exception.GetType().Name }
    }
    try { $top=FindTopMenu $Main $MenuName;$d.present=($null -ne $top) } catch { $d.errors+=$_.Exception.GetType().Name }
    if($top) {
        try { $d.isEnabled=$top.Current.IsEnabled;$d.isOffscreen=$top.Current.IsOffscreen } catch { $d.errors+=$_.Exception.GetType().Name }
        try { $d.expandState=[string]$top.GetCurrentPattern($ECP::Pattern).Current.ExpandCollapseState } catch { $d.errors+=$_.Exception.GetType().Name }
        if($d.labelSource -ceq 'MENU_READ') {
            try { $names=@(OfType $top $CT::MenuItem|ForEach-Object{$_.Current.Name}) } catch { $d.errors+=$_.Exception.GetType().Name }
        }
    }
    foreach($name in @($names|Select-Object -First 40)) {
        $label=[string]$name -replace '[\x00-\x1F\x7F]',' '
        if($label.Length -gt 80){$label=$label.Substring(0,80)+'...'}
        $d.labels+=$label
    }
    return $d
}

function Test-NoSendGame([string]$Text,[string]$Run,[string]$Attempt,[string]$PreparedGeneration,[int]$WorldGeneration) {
    $rows=@(Get-RehearsalRows $Text $Run $Attempt -Mode capture);$previous=-1L
    foreach($kind in @('ARM_REQUEST','NO_SEND_START_SKIPPED','NO_SEND_STOP_SKIPPED','TERMINAL')) {
        $r=@($rows|Where-Object{$_.Kind -ceq $kind})
        if($r.Count -ne 1 -or $r[0].Seq -le $previous){return $false};$previous=$r[0].Seq;$f=$r[0].Fields
        if($f['control'] -cne 'NO_SEND'){return $false}
        if($kind -ceq 'ARM_REQUEST' -and $f['preparedGeneration'] -cne $PreparedGeneration){return $false}
        if($kind -ne 'ARM_REQUEST' -and ($f['nativeCalls'] -cne '0' -or $f['actualCaptureCalls'] -cne '0')){return $false}
        if($kind -in @('NO_SEND_START_SKIPPED','NO_SEND_STOP_SKIPPED') -and $f['acceptedNative'] -cne 'NOT_REQUESTED'){return $false}
        if($kind -ceq 'NO_SEND_START_SKIPPED' -and ($f['route'] -cne 'SQUAD_RADIO' -or $f['exactTx'] -cne '1')){return $false}
        if($kind -ceq 'NO_SEND_STOP_SKIPPED' -and $f['restorePriorRoute'] -cne '0'){return $false}
        if($kind -ceq 'TERMINAL' -and ($f['result'] -cne 'NO_SEND_CONTROL_CYCLE_OBSERVED' -or $f['reason'] -cne 'INTENDED_STOP')){return $false}
    }
    if(@($rows|Where-Object{$_.Kind -in @('START_INTENT','START_REQUEST_RETURN','STOP_INTENT','STOP_REQUEST_RETURN','GAP') -or $_.Kind -cmatch '\AWould'}).Count){return $false}
    $dispatch='\[RICON_REST_CAPTURE\] run='+[regex]::Escape($Run)+' attempt='+[regex]::Escape($Attempt)+' kind=DISPATCH preparedGeneration='+$PreparedGeneration+' beforeTickMs=-?[0-9]+ afterTickMs=-?[0-9]+ accepted=(?:1|(?i:true)) guardVerifiedByGame=0 control=NO_SEND(?=\r?$)'
    $terminal='\[RICON_REST\] run='+[regex]::Escape($Run)+' generation='+$WorldGeneration+' kind=TERMINAL result=REST_CAPTURE_NO_SEND_ARM_ACCEPTED(?:,|\s|$)'
    return ([regex]::Matches($Text,$dispatch,[Text.RegularExpressions.RegexOptions]::Multiline).Count -eq 1 -and [regex]::Matches($Text,$terminal).Count -eq 1)
}

function Get-CaptureBlindTiming([string]$Text,[string]$Run,[string]$Attempt,[int]$Choice) {
    $r=[pscustomobject]@{valid=$false;plannedIntendedMs=$(if($Choice -eq 0){400}else{1000});actualRequestSpanMs=$null;actualReturnSpanMs=$null;stopOvershootMs=$null;nativeState='UNKNOWN'}
    if($Choice -notin @(0,1)){return $r}
    $rows=@(Get-RehearsalRows $Text $Run $Attempt -Mode capture);$start=@($rows|Where-Object{$_.Kind -ceq 'START_REQUEST_RETURN'});$stop=@($rows|Where-Object{$_.Kind -ceq 'STOP_REQUEST_RETURN'})
    if($start.Count -ne 1 -or $stop.Count -ne 1){return $r}
    foreach($row in @($start[0],$stop[0])) {
        if($row.Fields['blindTrial'] -cne '1' -or $row.Fields['choiceState'] -cne 'SELECTED' -or $row.Fields['blindChoice'] -cne [string]$Choice -or $row.Fields['plannedIntendedMs'] -cne [string]$r.plannedIntendedMs){return $r}
        foreach($key in @('callStartTick','returnTickMs','plannedStopTickMs')){if($row.Fields[$key] -cnotmatch '\A-?[0-9]{1,10}\z'){return $r}}
    }
    $a=[long]$start[0].Fields['callStartTick'];$ar=[long]$start[0].Fields['returnTickMs'];$b=[long]$stop[0].Fields['callStartTick'];$br=[long]$stop[0].Fields['returnTickMs'];$planned=$a+$r.plannedIntendedMs
    $r.actualRequestSpanMs=$b-$a;$r.actualReturnSpanMs=$br-$ar;$r.stopOvershootMs=$b-$planned
    $r.valid=($a -ge 0 -and $ar -ge $a -and $b -ge $ar -and $br -ge $b -and $b -ge $planned -and $start[0].Seq -lt $stop[0].Seq -and
        [long]$start[0].Fields['plannedStopTickMs'] -eq $planned -and [long]$stop[0].Fields['plannedStopTickMs'] -eq $planned)
    return $r
}
function Get-CaptureShortTiming([string]$Text,[string]$Run,[string]$Attempt) {
    $result=[pscustomobject]@{valid=$false;plannedIntendedMs=400;startCallTick=$null;startReturnTick=$null;stopCallTick=$null;stopReturnTick=$null;
        actualRequestSpanMs=$null;actualReturnSpanMs=$null;plannedStopTickMs=$null;stopOvershootMs=$null;feasibility='NOT_EVALUATED';nativeState='UNKNOWN'}
    $rows=@(Get-RehearsalRows $Text $Run $Attempt -Mode capture)
    $start=@($rows|Where-Object{$_.Kind -ceq 'START_REQUEST_RETURN'});$stop=@($rows|Where-Object{$_.Kind -ceq 'STOP_REQUEST_RETURN'})
    if($start.Count -ne 1 -or $stop.Count -ne 1){return $result}
    foreach($row in @($start[0],$stop[0])) {
        if($row.Fields['short400'] -cne '1' -or $row.Fields['plannedIntendedMs'] -cne '400'){return $result}
        foreach($name in @('callStartTick','returnTickMs','plannedStopTickMs')){if($row.Fields[$name] -cnotmatch '\A-?[0-9]{1,10}\z'){return $result}}
    }
    $a=[long]$start[0].Fields['callStartTick'];$ar=[long]$start[0].Fields['returnTickMs'];$b=[long]$stop[0].Fields['callStartTick'];$br=[long]$stop[0].Fields['returnTickMs']
    $planned=$a+400
    $result.startCallTick=$a;$result.startReturnTick=$ar;$result.stopCallTick=$b;$result.stopReturnTick=$br;$result.plannedStopTickMs=$planned
    $result.actualRequestSpanMs=$b-$a;$result.actualReturnSpanMs=$br-$ar;$result.stopOvershootMs=$b-$planned
    if($a -lt 0 -or $ar -lt 0 -or $b -lt 0 -or $br -lt 0 -or $start[0].Seq -ge $stop[0].Seq -or $ar -lt $a -or $b -lt $ar -or $br -lt $b -or $b -lt $planned -or
        [long]$start[0].Fields['plannedStopTickMs'] -ne $planned -or [long]$stop[0].Fields['plannedStopTickMs'] -ne $planned){return $result}
    $result.valid=$true;return $result
}

function Get-BOwnObservation([string]$Text,[string]$Run,[string]$ExpectedStart) {
    $result=[pscustomobject]@{readStatus='NO_MATCHING_OWN_B_ROWS';lastSeq=$null;lastKind='UNKNOWN';lastSide='UNKNOWN';lastRawTick=$null;lastStartTick=$null;lastLocalElapsedMs=$null;
        lastBindingGeneration='UNKNOWN';lastHeartbeatRawTick=$null;terminalObserved=$false;terminalReason='UNKNOWN';ownRows=0;afterGuardReadTicks=0L;finalReadTicks=0L;
        commonGuardDone=$false;guardExitObservedTicks=0L;stopBeginTicks=0L;stopEndTicks=0L;stopRequested=$null;stopExited=$null;readErrors=@();
        expectedCommonCutoffMs=1500;expectedValidationMs=2000;windowUsesSelectedAStop=$false;truncation='UNQUALIFIED_REQUIRES_OFFLINE_AUDIT';clock='B_LOCAL_ONLY';qpcFrequency=[Diagnostics.Stopwatch]::Frequency}
    $pattern='\[RICON_NRX\] seq=(?<seq>\d+) t=-?\d+ side=(?<side>[A-Za-z0-9_]+) kind=(?<kind>[A-Za-z_]+)[^\r\n]*? text=run='+[regex]::Escape($Run)+'(?=,)(?<fields>[^\r\n]*)'
    $rows=@()
    foreach($m in [regex]::Matches($Text,$pattern)) {
        $f=Get-RehearsalFields $m.Groups['fields'].Value
        if($f['localPlayer'] -cne '2' -or ($ExpectedStart -and $f['startTickMs'] -cne $ExpectedStart) -or
            $f['rawTickMs'] -cnotmatch '\A-?[0-9]{1,10}\z' -or $f['startTickMs'] -cnotmatch '\A[0-9]{1,10}\z'){continue}
        $rows+=[pscustomobject]@{Seq=[long]$m.Groups['seq'].Value;Kind=$m.Groups['kind'].Value;Side=$m.Groups['side'].Value;Fields=$f}
    }
    if(-not $rows.Count){return $result}
    $last=@($rows|Sort-Object Seq|Select-Object -Last 1)[0];$result.readStatus='OWN_B_LOG_HORIZON_OBSERVED';$result.ownRows=$rows.Count
    $result.lastSeq=$last.Seq;$result.lastKind=$last.Kind;$result.lastSide=$last.Side;$result.lastRawTick=[long]$last.Fields['rawTickMs'];$result.lastStartTick=[long]$last.Fields['startTickMs']
    $result.lastLocalElapsedMs=$result.lastRawTick-$result.lastStartTick;$result.lastBindingGeneration=$last.Fields['bindingGeneration']
    $heartbeat=@($rows|Where-Object{$_.Kind -ceq 'HEARTBEAT'}|Sort-Object Seq|Select-Object -Last 1)
    if($heartbeat.Count){$result.lastHeartbeatRawTick=[long]$heartbeat[0].Fields['rawTickMs']}
    $terminal=@($rows|Where-Object{$_.Kind -ceq 'TERMINAL'}|Sort-Object Seq|Select-Object -Last 1)
    if($terminal.Count){$result.terminalObserved=$true;if($terminal[0].Fields['reason'] -cmatch '\A[A-Z0-9_]+\z'){$result.terminalReason=$terminal[0].Fields['reason']}}
    return $result
}

function Test-CaptureGame([string]$Text,[string]$Run,[string]$Attempt,[string]$PreparedGeneration,[int]$WorldGeneration,[switch]$WrongFrequency,[switch]$Short400,[switch]$BlindTrial,[int]$BlindChoice=-1,[switch]$BPowerOff,[switch]$Direct) {
    $rows=@(Get-RehearsalRows $Text $Run $Attempt -Mode capture)
    $expectedControl=if($Direct -and $BPowerOff){'DIRECT_B_POWER_OFF'}elseif($Direct){'DIRECT'}else{'B_POWER_OFF'}
    $previous=-1L
    foreach($kind in @('ARM_REQUEST','START_INTENT','START_REQUEST_RETURN','STOP_INTENT','STOP_REQUEST_RETURN','TERMINAL')) {
        $r=@($rows|Where-Object{$_.Kind -ceq $kind})
        if($r.Count -ne 1 -or $r[0].Seq -le $previous){return $false};$previous=$r[0].Seq
        $f=$r[0].Fields
        if($Direct -and ($f['control'] -cne $expectedControl -or ($kind -in @('START_REQUEST_RETURN','STOP_REQUEST_RETURN') -and ($f['actualMethodDIRECT'] -cne '1' -or $f['actualTxNull'] -cne '1')))){return $false}
        if($BPowerOff -and $f['control'] -cne $expectedControl){return $false}
        if($BlindTrial){if($Short400 -or $WrongFrequency -or $BlindChoice -notin @(0,1) -or $f['blindTrial'] -cne '1' -or $f['choiceState'] -cne 'SELECTED' -or $f['blindChoice'] -cne [string]$BlindChoice -or $f['plannedIntendedMs'] -cne $(if($BlindChoice -eq 0){'400'}else{'1000'})){return $false}}
        elseif($f['blindTrial']){return $false}
        elseif($Short400 -and ($WrongFrequency -or $f['short400'] -cne '1' -or $f['plannedIntendedMs'] -cne '400')){return $false}
        elseif(-not $Short400 -and ($f['short400'] -or $f['plannedIntendedMs'])){return $false}
        if($WrongFrequency -and $f['control'] -cne 'WRONG_FREQUENCY'){return $false}
        if($kind -ceq 'ARM_REQUEST' -and $f['preparedGeneration'] -cne $PreparedGeneration){return $false}
        if($kind -in @('START_REQUEST_RETURN','STOP_REQUEST_RETURN') -and $f['accepted'] -cnotmatch '\A(?:1|(?i:true))\z'){return $false}
        if($kind -ceq 'START_REQUEST_RETURN' -and ($f['requestOnly'] -cne '1' -or $f['nativeState'] -cne 'UNKNOWN')){return $false}
        if($kind -ceq 'STOP_REQUEST_RETURN' -and ($f['nativeRelease'] -cne 'UNKNOWN' -or $f['restorePriorRoute'] -cne '0')){return $false}
        if($kind -ceq 'TERMINAL' -and ($f['result'] -cne 'CAPTURE_REQUEST_CYCLE_OBSERVED' -or $f['reason'] -cne 'INTENDED_STOP')){return $false}
    }
    if(@($rows|Where-Object{$_.Kind -cmatch '\AWould' -or $_.Kind -ceq 'GAP'}).Count){return $false}
    if($Direct){
        $route=@($rows|Where-Object{$_.Kind -ceq 'DIRECT_ROUTE_READBACK'});$start=@($rows|Where-Object{$_.Kind -ceq 'START_INTENT'})
        if($route.Count -ne 1 -or $route[0].Fields['control'] -cne $expectedControl -or $route[0].Fields['actualMethodDIRECT'] -cne '1' -or $route[0].Fields['actualTxNull'] -cne '1' -or $route[0].Seq -le (@($rows|Where-Object{$_.Kind -ceq 'ARM_REQUEST'}))[0].Seq -or $route[0].Seq -ge $start[0].Seq){return $false}
    }
    if($Short400 -and -not (Get-CaptureShortTiming $Text $Run $Attempt).valid){return $false}
    if($BlindTrial -and -not (Get-CaptureBlindTiming $Text $Run $Attempt $BlindChoice).valid){return $false}
    $dispatch='\[RICON_REST_CAPTURE\] run='+[regex]::Escape($Run)+' attempt='+[regex]::Escape($Attempt)+' kind=DISPATCH preparedGeneration='+$PreparedGeneration+' beforeTickMs=-?[0-9]+ afterTickMs=-?[0-9]+ accepted=(?:1|(?i:true)) guardVerifiedByGame=0'
    if($WrongFrequency){$dispatch+=' control=WRONG_FREQUENCY(?=\r?$)'}
    if($BlindTrial){$dispatch+=' blindTrial=1(?=\r?$)'}
    if($Direct -and $BPowerOff){$dispatch+=' control=DIRECT_B_POWER_OFF(?=\r?$)'}elseif($Direct){$dispatch+=' control=DIRECT(?=\r?$)'}elseif($BPowerOff){$dispatch+=' control=B_POWER_OFF(?=\r?$)'}
    $accepted=if($WrongFrequency){'REST_CAPTURE_WRONG_FREQUENCY_ARM_ACCEPTED'}else{'REST_CAPTURE_ARM_ACCEPTED'}
    $terminal='\[RICON_REST\] run='+[regex]::Escape($Run)+' generation='+$WorldGeneration+' kind=TERMINAL result='+$accepted+'(?:,|\s|$)'
    return ([regex]::Matches($Text,$dispatch,[Text.RegularExpressions.RegexOptions]::Multiline).Count -eq 1 -and [regex]::Matches($Text,$terminal).Count -eq 1)
}

# Сохраняем candidate RADIO callbacks обеих подписок B; tap owner не является доказательством native ownership.
function Get-CaptureRadioEvidence([string]$HostText,[string]$ClientText,[string]$Run,$Snapshot) {
    $rx=@();$capture=@()
    $pattern='\[RICON_NRX\] seq=(?<seq>\d+) t=\d+ side=(?<side>ownA|ownB|sourceAproxy) kind=(?<kind>RAW_RX|RAW_CAPTURE) player=(?<sender>\d+) device=-?\d+ tr=(?<tr>-?\d+) value=(?<frequency>-?\d+) text=run='+[regex]::Escape($Run)+'(?=,)(?<fields>[^\r\n]*)'
    foreach($role in @(@($HostText,1,$Snapshot.HostHeartSeq,$Snapshot.HostStart),@($ClientText,2,$Snapshot.BHeartSeq,$Snapshot.BStart))) {
        foreach($m in [regex]::Matches($role[0],$pattern)) {
            $f=Get-RehearsalFields $m.Groups['fields'].Value;$kind=$m.Groups['kind'].Value;$side=$m.Groups['side'].Value
            # Ссылка проверяется только как непрозрачный non-null local token; это не ownership proof.
            if([long]$m.Groups['seq'].Value -le $role[2] -or $f['localPlayer'] -cne [string]$role[1] -or
                $f['bindingGeneration'] -cne '1' -or $f['startTickMs'] -cne [string]$role[3] -or $f['contextValid'] -cne '1' -or
                $f['catalogIndex'] -cnotmatch '\A(?:[0-9]|1[0-5])\z' -or $m.Groups['tr'].Value -cne $f['catalogIndex'] -or
                $f['nativeRefLocal'] -cnotmatch '\A[A-Za-z_][A-Za-z0-9_]*<0x[0-9A-Fa-f]+>\z' -or $f['nativeRefLocal'] -match '<0x0+>' -or
                $f['radioRefLocal'] -cnotmatch '\A[A-Za-z_][A-Za-z0-9_]*<0x[0-9A-Fa-f]+>\z' -or $f['radioRefLocal'] -match '<0x0+>' -or $f['nativeIndex'] -cnotmatch '\A[0-3]\z' -or
                $m.Groups['frequency'].Value -cne $Snapshot.FrequencyKHz){continue}
            $item=[pscustomobject]@{seq=[long]$m.Groups['seq'].Value;tapOwner=$side;sourceOwnerPlayer=$f['sourceOwnerPlayer'];sourceOwnerRpl=$f['sourceOwnerRpl'];
                receiverCatalogIndex=$f['catalogIndex'];nativeIndex=$f['nativeIndex'];frequencyKHz=$m.Groups['frequency'].Value;quality=$f['quality'];nativeOwnership='UNQUALIFIED'}
            if($role[1] -eq 2 -and $kind -ceq 'RAW_RX' -and $side -in @('ownB','sourceAproxy') -and $m.Groups['sender'].Value -ceq '1' -and
                $f['senderA'] -ceq '1' -and $f['receiverPresent'] -ceq '1' -and $f['qualityValid'] -ceq '1' -and $f['catalogScope'] -ceq 'OWN_HANDHELD_BACKPACK' -and
                (($side -ceq 'ownB' -and $f['sourceOwnerPlayer'] -ceq '2' -and $f['sourceOwnerRpl'] -ceq $Snapshot.BOwnRpl) -or
                 ($side -ceq 'sourceAproxy' -and $f['sourceOwnerPlayer'] -ceq '1' -and $f['sourceOwnerRpl'] -ceq $Snapshot.HostOwnRpl))){$rx+=$item}
            if($role[1] -eq 1 -and $side -ceq 'ownA' -and $kind -ceq 'RAW_CAPTURE' -and $f['transmitterPresent'] -ceq '1' -and $f['directCapture'] -ceq '0' -and $f['nativeIndex'] -ceq $Snapshot.NativeIndex){$capture+=$item}
        }
    }
    return [pscustomobject]@{sourceRadioCapture=$capture;receiverRadioCandidates=$rx;inputVad='UNKNOWN';missingRxConclusion='UNQUALIFIED';media='NOT_TESTED';proxySoundIsRx=$false}
}


# Нулевой receiver сам по себе не квалифицирует DIRECT или слышимость.
function Get-CaptureDirectEvidence([string]$HostText,[string]$ClientText,[string]$Run,$Snapshot) {
    $capture=@();$rx=@();$raw=@()
    $pattern='\[RICON_NRX\] seq=(?<seq>\d+) t=\d+ side=(?<side>ownA|ownB|sourceAproxy) kind=(?<kind>RAW_RX|RAW_CAPTURE) player=(?<sender>\d+) device=-?\d+ tr=(?<tr>-?\d+) value=(?<frequency>-?\d+) text=run='+[regex]::Escape($Run)+'(?=,)(?<fields>[^\r\n]*)'
    foreach($role in @(@($HostText,1,$Snapshot.HostHeartSeq,$Snapshot.HostStart),@($ClientText,2,$Snapshot.BHeartSeq,$Snapshot.BStart))) {
        foreach($m in [regex]::Matches($role[0],$pattern)) {
            $f=Get-RehearsalFields $m.Groups['fields'].Value;$side=$m.Groups['side'].Value;$kind=$m.Groups['kind'].Value
            $item=[pscustomobject]@{raw=$m.Value;seq=[long]$m.Groups['seq'].Value;tapOwner=$side;kind=$kind;receiverPresent=$f['receiverPresent'];sourceOwnerPlayer=$f['sourceOwnerPlayer'];sourceOwnerRpl=$f['sourceOwnerRpl'];nativeStreamAttribution='UNQUALIFIED'}
            $raw+=$item
            if($item.seq -le $role[2] -or $f['localPlayer'] -cne [string]$role[1] -or $f['startTickMs'] -cne [string]$role[3] -or $f['bindingGeneration'] -cne '1' -or $f['contextValid'] -cne '1' -or
                $f['rawTickMs'] -cnotmatch '\A[0-9]{1,10}\z' -or [long]$f['rawTickMs'] -lt [long]$role[3] -or [long]$f['rawTickMs']-[long]$role[3] -gt 60000){continue}
            if($role[1] -eq 1 -and $side -ceq 'ownA' -and $kind -ceq 'RAW_CAPTURE' -and $f['sourceOwnerPlayer'] -ceq '1' -and $f['sourceOwnerRpl'] -ceq $Snapshot.HostOwnRpl -and
                $f['transmitterPresent'] -ceq '0' -and $f['directCapture'] -ceq '1' -and $f['catalogIndex'] -ceq '-1' -and $f['nativeIndex'] -ceq '-1' -and $m.Groups['tr'].Value -ceq '-1' -and $m.Groups['frequency'].Value -ceq '-1'){$capture+=$item}
            if($role[1] -eq 2 -and $kind -ceq 'RAW_RX' -and $m.Groups['sender'].Value -ceq '1' -and $f['senderA'] -ceq '1' -and $f['qualityValid'] -ceq '1' -and
                (($side -ceq 'ownB' -and $f['sourceOwnerPlayer'] -ceq '2' -and $f['sourceOwnerRpl'] -ceq $Snapshot.BOwnRpl) -or ($side -ceq 'sourceAproxy' -and $f['sourceOwnerPlayer'] -ceq '1' -and $f['sourceOwnerRpl'] -ceq $Snapshot.HostOwnRpl))){$rx+=$item}
        }
    }
    return [pscustomobject]@{sourceDirectCapture=$capture;receiverNativeCandidates=$rx;allRawCallbacks=$raw;missingRxConclusion='NOT_OBSERVED_UNQUALIFIED';directAttribution='UNQUALIFIED';media='NOT_TESTED'}
}

function Test-RehearsalGame([string]$Text,[string]$Run,[string]$Attempt,[string]$PreparedGeneration,[switch]$RestArm,[int]$WorldGeneration=0) {
    $rows=@(Get-RehearsalRows $Text $Run $Attempt)
    $expected=@('ARM_REQUEST','WouldRequestTrue','WouldReturnStart','WouldRequestFalse','WouldReturnStop','TERMINAL')
    $previous=-1L
    foreach($kind in $expected) {
        $row=@($rows|Where-Object{$_.Kind -ceq $kind})
        if($row.Count -ne 1 -or $row[0].Seq -le $previous) { return $false }; $previous=$row[0].Seq
        if($kind -ceq 'ARM_REQUEST' -and $row[0].Fields['preparedGeneration'] -cne $PreparedGeneration){return $false}
        if($kind -ceq 'TERMINAL' -and ($row[0].Fields['result'] -cne 'REHEARSAL_ONLY' -or $row[0].Fields['reason'] -cne 'INTENDED_STOP')){return $false}
        if($kind -in @('WouldRequestTrue','WouldReturnStart','WouldRequestFalse','WouldReturnStop') -and $row[0].Fields['actualCaptureCalls'] -cne '0'){return $false}
    }
    if(@($rows|Where-Object{$_.Kind -in @('START_INTENT','START_REQUEST_RETURN','STOP_INTENT','STOP_REQUEST_RETURN','GAP')}).Count){return $false}
    if($RestArm) {
        $dispatch='\[RICON_REST_ARM\] run='+[regex]::Escape($Run)+' attempt='+[regex]::Escape($Attempt)+' kind=DISPATCH preparedGeneration='+$PreparedGeneration+' beforeTickMs=-?[0-9]+ afterTickMs=-?[0-9]+ accepted=(?:1|(?i:true)) guardVerifiedByGame=0'
        $terminal='\[RICON_REST\] run='+[regex]::Escape($Run)+' generation='+$WorldGeneration+' kind=TERMINAL result=REST_REHEARSAL_ARM_ACCEPTED(?:,|\s|$)'
        return ([regex]::Matches($Text,$dispatch).Count -eq 1 -and [regex]::Matches($Text,$terminal).Count -eq 1)
    }
    $ui='\[RICON_PULSE_UI\] run='+[regex]::Escape($Run)+' attempt='+[regex]::Escape($Attempt)+' mode=rehearsal accepted=(?:1|(?i:true))(?=\s|$)'
    return ([regex]::Matches($Text,$ui).Count -eq 1)
}

function Test-RehearsalOutcome($State) {
    $g=$State.GuardResult; $p=$State.Proxy
    if(-not $g -or -not $p -or -not $g.valid -or $g.status -cne 'KILLED' -or -not $g.killRequested -or -not $g.targetExitConfirmed -or -not $State.GameValid){return $false}
    $begin=$p.InvokeBeginTicks; $end=$p.InvokeReturnTicks
    return ($p.Calls -eq 1 -and -not $p.Failed -and $begin -ge $g.armedAckTicks -and $end -ge $begin -and
        $end -le $g.exitObservedTicks -and $State.DispatchBeforeTicks -ge $g.armedAckTicks -and
        $State.DispatchAfterTicks -ge $State.DispatchBeforeTicks -and
        (1500.0-1000.0*($begin-$g.armTicks)/$g.frequency) -ge 1100)
}

function Invoke-RehearsalStep($State,[Diagnostics.Process]$Target,[Diagnostics.Process]$Client,[IntPtr]$Main,[scriptblock]$ReadHost,[scriptblock]$ReadClient) {
    if($State.Attempted){return 'REHEARSAL_'+$State.Status}
    $snapshot=Get-RehearsalSnapshot (& $ReadHost) (& $ReadClient) $State.Run $State.Attempt
    if(-not $snapshot){return ''}
    $State.Attempted=$true; $State.Phase='PREPARE'; $State.Main=$Main; $State.Client=$Client
    try {
        $State.ClientStart=$Client.StartTime.ToUniversalTime().Ticks
        Assert-RehearsalTargetWindow $State $Target $Client $Main
        $State.Snapshot=$snapshot
        $State.MenuDiagnostic=[pscustomobject]@{before=(Get-RehearsalMenuObservation $Main $Target.Id $Client.Id);after=$null}
        $prep=PrepareMenuCommand $Main 'Plugins' 'RICON: Arm short capture pulse'
        $State.MenuDiagnostic.after=Get-RehearsalMenuObservation $Main $Target.Id $Client.Id $prep
        if($prep.result -cne 'READY' -or $prep.ip -isnot [Windows.Automation.InvokePattern]){throw 'MENU_NOT_READY'}
        $State.Proxy=[RiconRehearsalInvokeV1]::new($prep.ip)
        $State.Phase='GUARD_READY'; $State.Guard=Start-CutoffGuard -Target $Target -ExpectedStartUtcTicks $State.TargetStart
        if(-not $State.Guard.Ready){throw 'GUARD_NOT_READY'}
        $fresh=Get-RehearsalSnapshot (& $ReadHost) (& $ReadClient) $State.Run $State.Attempt
        if(-not $fresh -or $fresh.Signature -cne $snapshot.Signature){throw 'BINDING_CHANGED'}
        Assert-RehearsalTargetWindow $State $Target $Client $Main
        $State.Phase='ARM'; if(-not (Arm-CutoffGuard $State.Guard)){throw 'ARM_NOT_ACKNOWLEDGED'}
        $ack=$State.Guard.ArmAck
        if(-not $ack -or $ack.channel -cne $State.Guard.Name -or $ack.targetId -ne $Target.Id -or
            $ack.startUtcTicks -ne $State.TargetStart -or $ack.frequency -ne [Diagnostics.Stopwatch]::Frequency -or
            $ack.armedAckTicks -lt $ack.armTicks){throw 'ACK_TUPLE'}
        Assert-RehearsalTargetWindow $State $Target $Client $Main
        $State.Phase='DISPATCH'; $State.DispatchBeforeTicks=[Diagnostics.Stopwatch]::GetTimestamp()
        if(1500.0-1000.0*($State.DispatchBeforeTicks-$ack.armTicks)/$ack.frequency -lt 1100){throw 'TRIGGER_BUDGET'}
        $State.Job=StartAsyncInvoke $State.Proxy
        $State.DispatchAfterTicks=[Diagnostics.Stopwatch]::GetTimestamp()
        $State.Phase='OBSERVE'; $State.GuardResult=Get-CutoffResult $State.Guard
        $null=PollAsyncInvoke $State.Job
        $State.GameValid=Test-RehearsalGame (& $ReadHost) $State.Run $State.Attempt $snapshot.PreparedGeneration
        $State.Valid=(Test-RehearsalOutcome $State) -and $State.Job.done -and $State.Job.result -ceq 'COMPLETED'
        $State.Status=if($State.Valid){'REHEARSAL_ONLY_GUARDED'}else{'INVALID'}
    } catch {
        $detail=$_.Exception.Message
        if($detail -cnotmatch '\A[A-Z_]+\z'){$detail=$_.Exception.GetType().Name}
        $State.Failure=$State.Phase+':'+$detail; $State.Status='INVALID'; $State.Valid=$false
    }
    return 'REHEARSAL_'+$State.Status
}

function Close-RehearsalCoordinator($State) {
    if(-not $State){return}
    if($State.Guard -and $State.Guard.ArmRequested -and -not (Test-CutoffTargetExited $State.Guard)) {
        $State.Valid=$false; $State.Status='INVALID'; $State.Cleanup='ARMED_TARGET_ALIVE_OR_UNKNOWN'; return
    }
    if($State.Job){$State.Cleanup=CleanupAsyncInvoke $State.Job 3000}else{$State.Cleanup='NO_ASYNC_JOB'}
    if($State.Guard){Close-CutoffGuard $State.Guard}
}

function Get-RestRehearsalPending([string]$Text,[string]$Run,[string]$Attempt,[switch]$Capture,[switch]$NoSend,[switch]$WrongFrequency,[switch]$BlindTrial,[switch]$BPowerOff,[switch]$Direct) {
    $prefix='\[RICON_REST\] run='+[regex]::Escape($Run)+' generation=(?<g>[1-9][0-9]*) kind=(?<kind>[A-Z_]+) '
    $rest=@([regex]::Matches($Text,$prefix))
    if(@($rest|Where-Object{$_.Groups['kind'].Value -ceq 'TERMINAL'}).Count){throw 'REST_TERMINAL_BEFORE_RELEASE'}
    $tag=if($Capture){'RICON_REST_CAPTURE'}else{'RICON_REST_ARM'}
    $suffix=if($Direct -and $BPowerOff){' control=DIRECT_B_POWER_OFF'}elseif($Direct){' control=DIRECT'}elseif($BPowerOff){' control=B_POWER_OFF'}elseif($BlindTrial){' blindTrial=1'}elseif($NoSend){' control=NO_SEND'}elseif($WrongFrequency){' control=WRONG_FREQUENCY'}else{''}
    $pattern='\['+$tag+'\] run='+[regex]::Escape($Run)+' attempt='+[regex]::Escape($Attempt)+' kind=PENDING preparedGeneration=(?<n>[1-9][0-9]*) worldGeneration=(?<g>[1-9][0-9]*) requestTickMs=(?<tick>-?[0-9]+)'+$suffix+'(?=\r?$)'
    $matches=@([regex]::Matches($Text,$pattern,[Text.RegularExpressions.RegexOptions]::Multiline))
    if($matches.Count -gt 1){throw 'REST_PENDING_REPLAY'}
    if(-not $matches.Count){return $null}
    $p=$matches[0]
    if(-not $rest.Count -or $rest[-1].Groups['g'].Value -cne $p.Groups['g'].Value){throw 'REST_WORLD_GENERATION_MISMATCH'}
    return [pscustomobject]@{PreparedGeneration=[int]$p.Groups['n'].Value;WorldGeneration=[int]$p.Groups['g'].Value;RequestGameTickMs=[long]$p.Groups['tick'].Value;
        Signature=($p.Groups['n'].Value+'|'+$p.Groups['g'].Value+'|'+$p.Groups['tick'].Value)}
}
function Assert-RestRehearsalWorker($Worker,$State,$Target) {
    if(-not $Worker -or $Worker.Done -or $Worker.Async.IsCompleted -or -not $Worker.Ready -or -not $Worker.Release -or -not $Worker.Gate -or
        $Worker.Run -cne $State.Run -or $Worker.Attempt -cne $State.Attempt -or [bool]$Worker.Capture -ne $State.Capture -or [bool]$Worker.NoSend -ne $State.NoSend -or [bool]$Worker.WrongFrequency -ne $State.WrongFrequency -or [bool]$Worker.SeparatedPair -ne $State.SeparatedPair -or [bool]$Worker.Short400 -ne $State.Short400 -or [bool]$Worker.BlindTrial -ne $State.BlindTrial -or [bool]$Worker.Direct -ne $State.Direct -or ($State.Direct -and $State.BPowerOff -and $Worker.ControlTag -cne $State.ControlTag) -or [bool]$Worker.BPowerOff -ne $State.BPowerOff -or -not [object]::ReferenceEquals($Worker.Target,$Target) -or
        $Worker.TargetStart -ne $State.TargetStart -or $Worker.TargetHandle -ne $State.TargetHandle){throw 'REST_WORKER_IDENTITY'}
}
function Test-RestRehearsalOutcome($State) {
    $g=$State.GuardResult;$r=$State.RestResult
    if($State.NoSend -and (-not $r -or $r.noSend -ne $true)){return $false}
    if($State.WrongFrequency -and (-not $r -or $r.wrongFrequency -ne $true -or -not $State.FrequencyRestore -or -not $State.FrequencyRestore.valid)){return $false}
    if($State.SeparatedPair -and (-not $r -or $r.separatedPair -ne $true -or -not $State.Snapshot.Geometry -or $r.geometryStamp -cne $State.Snapshot.Geometry.Stamp)){return $false}
    if($State.Short400 -and (-not $r -or $r.short400 -ne $true -or $r.plannedIntendedMs -ne 400 -or -not $State.CaptureTiming -or -not $State.CaptureTiming.valid)){return $false}
    if($State.BlindTrial -and (-not $r -or $r.blindTrial -ne $true -or $r.blindCommitment -cne $State.Blind.Commitment -or $r.plannedIntendedMs -ne $State.Blind.PlannedMs -or -not $State.CaptureTiming.valid)){return $false}
    if($State.Direct -and $State.BPowerOff -and $r.controlTag -cne 'DIRECT_B_POWER_OFF'){return $false}
    if($State.Direct -and (-not $r -or $r.direct -ne $true -or $r.geometryStamp -cne $State.Snapshot.Geometry.Stamp -or -not $State.DirectEvidence -or $State.DirectEvidence.sourceDirectCapture.Count -lt 1)){return $false}
    if($State.BPowerOff -and (-not $r -or $r.bPowerOff -ne $true -or $r.powerOffIdentity -cne $State.PowerAdmission.Identity -or $r.powerRemainingAtHttpMs -lt 2000)){return $false}
    return ($State.GameValid -and $g -and $g.valid -and $g.status -ceq 'KILLED' -and $g.targetExitConfirmed -and $g.killRequested -and
        $r -and $r.status -ceq 'HTTP200_SENT' -and $r.responses -eq 1 -and $r.dispatchWitnessValid -and
        $r.preparedGeneration -eq $State.RestPending.PreparedGeneration -and $r.worldGeneration -eq $State.RestPending.WorldGeneration -and
        $r.guardToDispatchObservedMs -ge 0 -and $r.guardToDispatchObservedMs -le 400 -and
        $State.ReleaseBeforeTicks -ge $g.armedAckTicks -and $State.ReleaseAfterTicks -ge $State.ReleaseBeforeTicks)
}
function Invoke-RestGuardedRehearsalStep($State,$Worker,$Target,$Client,[IntPtr]$Main,[scriptblock]$ReadHost,[scriptblock]$ReadClient) {
    if($State.Attempted){return $(if($State.Capture){'CAPTURE_'}else{'REHEARSAL_'})+$State.Status}
    try {
        $soundPeer=$null
        if($State.VoNSoundObserve) {
            $soundPeer=& $ReadClient;$soundState=Get-VoNSoundAdmission $soundPeer $State.Run $State.Attempt;$State.SoundStatus=$soundState.status
            if($soundState.status -cin @('NOT_AVAILABLE','INVALID_BINDING','INVALID_CLOCK')) {
                $State.Attempted=$true;$State.Valid=$false;$State.Status=if($soundState.status -ceq 'NOT_AVAILABLE'){'NOT_AVAILABLE'}else{'INVALID'}
                $State.Failure='SOUND_'+$soundState.status+':'+$soundState.reason
                return 'CAPTURE_SOUND_'+$soundState.status
            }
            if($soundState.status -cne 'READY'){return ''}
        }
        $text=& $ReadHost;$pending=Get-RestRehearsalPending $text $State.Run $State.Attempt -Capture:$State.Capture -NoSend:$State.NoSend -WrongFrequency:$State.WrongFrequency -BlindTrial:$State.BlindTrial -BPowerOff:$State.BPowerOff -Direct:$State.Direct
        $peerForSnapshot=if($State.VoNSoundObserve){$soundPeer}else{& $ReadClient}
        $snapshot=Get-RehearsalSnapshot $text $peerForSnapshot $State.Run $State.Attempt -Capture:$State.Capture -NoSend:$State.NoSend -WrongFrequency:$State.WrongFrequency -SeparatedPair:$State.SeparatedPair -Short400:$State.Short400 -BlindTrial:$State.BlindTrial -VoNSoundObserve:$State.VoNSoundObserve -BPowerOff:$State.BPowerOff -Direct:$State.Direct
        if(-not $pending -or -not $snapshot){return ''}
        $geometryFresh=$true;$soundFresh=$true;$powerFresh=$true
        if($State.SeparatedPair -or $State.Direct){$geometryFresh=Test-SeparatedGeometryFresh $State $snapshot.Geometry}
        if($State.VoNSoundObserve){$soundFresh=Test-VoNSoundFresh $State $snapshot.SoundAdmission}
        if($State.BPowerOff){$powerFresh=Test-BPowerOffFresh $State $snapshot.PowerAdmission}
        if(-not $geometryFresh -or -not $soundFresh -or -not $powerFresh){return ''}
        if($State.BPowerOff){$snapshot.PowerAdmission.RemainingConservativeMs=Get-BPowerOffRemaining $snapshot.PowerAdmission ([Diagnostics.Stopwatch]::GetTimestamp());if($snapshot.PowerAdmission.RemainingConservativeMs -lt 2000){$State.Attempted=$true;$State.Status='NO_RELEASE';$State.Failure='POWER_OFF_BUDGET';return 'CAPTURE_NO_RELEASE_POWER_BUDGET'}}
        Assert-RestRehearsalWorker $Worker $State $Target
        if(-not $Worker.Ready.WaitOne(0)){return ''}
        if($pending.PreparedGeneration -ne [int]$snapshot.PreparedGeneration){throw 'REST_PREPARED_GENERATION_MISMATCH'}
        $State.Attempted=$true;$State.Phase='REST_PREPARE';$State.Main=$Main;$State.Client=$Client;$State.ClientStart=$Client.StartTime.ToUniversalTime().Ticks
        Assert-RehearsalTargetWindow $State $Target $Client $Main
        $State.Snapshot=$snapshot;$State.RestPending=$pending
        if($State.VoNSoundObserve){$State.SoundAdmission=$snapshot.SoundAdmission}
        if($State.BlindTrial){Set-BlindParentChoice $State}
        $binding=[pscustomobject]@{Run=$State.Run;Attempt=$State.Attempt;PreparedGeneration=$pending.PreparedGeneration;WorldGeneration=$pending.WorldGeneration;
            ControlTag=$State.ControlTag;Mode=$(if($State.Capture){'capture'}else{'rehearsal'});NoSend=$State.NoSend;WrongFrequency=$State.WrongFrequency;SeparatedPair=$State.SeparatedPair;Short400=$State.Short400;BPowerOff=$State.BPowerOff;Direct=$State.Direct;
            PowerOffIdentity='';PowerPlanId='';PowerOffCallTick=0L;PowerDeadlineTick=0L;PowerBTick=0L;PowerAObservedTicks=0L;PowerBObservedTicks=0L;
            BlindTrial=$State.BlindTrial;BlindChoice=$(if($State.BlindTrial){$State.Blind.Choice}else{-1});BlindCommitment=$(if($State.BlindTrial){$State.Blind.Commitment}else{''});
            PlannedIntendedMs=$(if($State.BlindTrial){$State.Blind.PlannedMs}else{$snapshot.PlannedIntendedMs});
            GeometryStamp='';GeometryVerifiedTicks=0L;GeometryObservedATicks=0L;GeometryObservedBTicks=0L;
            TargetId=$Target.Id;TargetStart=$State.TargetStart;TargetHandle=$State.TargetHandle.ToInt64();ArmTicks=0L;AckTicks=0L;Frequency=0L;ReleaseTicks=0L}
        if($State.BPowerOff) {
            $powerGuardNow=[Diagnostics.Stopwatch]::GetTimestamp();$powerGuardFreshness=$snapshot.PowerAdmission.Freshness
            if(1000.0*($powerGuardNow-$powerGuardFreshness.ObservedATicks)/$powerGuardFreshness.Frequency -gt 500 -or 1000.0*($powerGuardNow-$powerGuardFreshness.ObservedBTicks)/$powerGuardFreshness.Frequency -gt 500 -or
                (Get-BPowerOffRemaining $snapshot.PowerAdmission $powerGuardNow) -lt 2000){throw 'POWER_OFF_BUDGET_OR_STALE_READ'}
        }
        $State.Phase='GUARD_READY';$State.Guard=Start-CutoffGuard -Target $Target -ExpectedStartUtcTicks $State.TargetStart
        if(-not $State.Guard.Ready){throw 'GUARD_NOT_READY'}
        $freshText=& $ReadHost;$freshPending=Get-RestRehearsalPending $freshText $State.Run $State.Attempt -Capture:$State.Capture -NoSend:$State.NoSend -WrongFrequency:$State.WrongFrequency -BlindTrial:$State.BlindTrial -BPowerOff:$State.BPowerOff -Direct:$State.Direct
        $freshPeer=& $ReadClient
        if($State.VoNSoundObserve){$State.SoundStatus=(Get-VoNSoundAdmission $freshPeer $State.Run $State.Attempt).status}
        $fresh=Get-RehearsalSnapshot $freshText $freshPeer $State.Run $State.Attempt -Capture:$State.Capture -NoSend:$State.NoSend -WrongFrequency:$State.WrongFrequency -SeparatedPair:$State.SeparatedPair -Short400:$State.Short400 -BlindTrial:$State.BlindTrial -VoNSoundObserve:$State.VoNSoundObserve -BPowerOff:$State.BPowerOff -Direct:$State.Direct
        if($State.VoNSoundObserve -and $State.SoundStatus -ceq 'STALE_READ'){throw 'SOUND_READY_STALE_READ'}
        if(-not $fresh -or -not $freshPending -or $fresh.Signature -cne $snapshot.Signature -or $freshPending.Signature -cne $pending.Signature){throw 'REST_FRESH_BINDING_CHANGED'}
        if($State.VoNSoundObserve -and -not (Test-VoNSoundFresh $State $fresh.SoundAdmission)){throw 'SOUND_READY_STALE_READ'}
        if($State.BPowerOff) {
            if(-not (Test-BPowerOffFresh $State $fresh.PowerAdmission)){throw 'POWER_OFF_STALE_READ'}
            $fresh.PowerAdmission.RemainingConservativeMs=Get-BPowerOffRemaining $fresh.PowerAdmission ([Diagnostics.Stopwatch]::GetTimestamp())
            if($fresh.PowerAdmission.RemainingConservativeMs -lt 2000){throw 'POWER_OFF_BUDGET'}
            $State.PowerAdmission=$fresh.PowerAdmission
            $binding.PowerOffIdentity=$fresh.PowerAdmission.Identity;$binding.PowerPlanId=$fresh.PowerAdmission.PlanId;$binding.PowerOffCallTick=$fresh.PowerAdmission.OffCallTick
            $binding.PowerDeadlineTick=$fresh.PowerAdmission.DeadlineTick;$binding.PowerBTick=$fresh.PowerAdmission.BTick
            $binding.PowerAObservedTicks=$fresh.PowerAdmission.Freshness.ObservedATicks;$binding.PowerBObservedTicks=$fresh.PowerAdmission.Freshness.ObservedBTicks
        }
        if($State.SeparatedPair -or $State.Direct) {
            if(-not (Test-SeparatedGeometryFresh $State $fresh.Geometry)){throw 'GEOMETRY_FRESHNESS_EXPIRED'}
            $binding.GeometryStamp=$fresh.Geometry.Stamp;$binding.GeometryVerifiedTicks=$fresh.Geometry.Freshness.verifiedTicks
            $binding.GeometryObservedATicks=$fresh.Geometry.Freshness.observedATicks;$binding.GeometryObservedBTicks=$fresh.Geometry.Freshness.observedBTicks
        }
        if($State.WrongFrequency -or $State.SeparatedPair -or $State.Direct){$State.Snapshot=$fresh}
        if($State.VoNSoundObserve){$State.SoundAdmission=$fresh.SoundAdmission}
        Assert-RehearsalTargetWindow $State $Target $Client $Main
        Assert-RestRehearsalWorker $Worker $State $Target
        $State.Phase='ARM';if(-not (Arm-CutoffGuard $State.Guard)){throw 'ARM_NOT_ACKNOWLEDGED'}
        $ack=$State.Guard.ArmAck
        if(-not $ack -or $ack.channel -cne $State.Guard.Name -or $ack.targetId -ne $Target.Id -or $ack.startUtcTicks -ne $State.TargetStart -or
            $ack.frequency -ne [Diagnostics.Stopwatch]::Frequency -or $ack.armedAckTicks -lt $ack.armTicks){throw 'ACK_TUPLE'}
        Assert-RehearsalTargetWindow $State $Target $Client $Main
        $binding.ArmTicks=$ack.armTicks;$binding.AckTicks=$ack.armedAckTicks;$binding.Frequency=$ack.frequency
        $State.Phase='RELEASE';$State.ReleaseBeforeTicks=[Diagnostics.Stopwatch]::GetTimestamp();$binding.ReleaseTicks=$State.ReleaseBeforeTicks
        if($State.BPowerOff -and (1000.0*($State.ReleaseBeforeTicks-$binding.PowerAObservedTicks)/$ack.frequency -gt 500 -or 1000.0*($State.ReleaseBeforeTicks-$binding.PowerBObservedTicks)/$ack.frequency -gt 500 -or (Get-BPowerOffRemaining $State.PowerAdmission $State.ReleaseBeforeTicks) -lt 2000)){throw 'POWER_OFF_BUDGET_OR_STALE_READ'}
        if($State.VoNSoundObserve -and 1000.0*($State.ReleaseBeforeTicks-$State.SoundTracking.AdvanceTicks)/$ack.frequency -gt 500){$State.SoundStatus='STALE_READ';throw 'SOUND_READY_STALE_READ'}
        if(1500.0-1000.0*($State.ReleaseBeforeTicks-$ack.armTicks)/$ack.frequency -lt 1100){throw 'TRIGGER_BUDGET'}
        if(-not $Worker.Gate.TryAdd($binding,0)){throw 'RELEASE_REPLAY'}
        $null=$Worker.Release.Set();$State.ReleaseAfterTicks=[Diagnostics.Stopwatch]::GetTimestamp()
        $State.Phase='OBSERVE';$State.GuardResult=Get-CutoffResult $State.Guard
        if($Worker.Async.IsCompleted){$State.RestResult=Complete-RestCapabilityWorker $Worker}
        $observedHost=& $ReadHost
        if($State.Capture) {
            if($State.NoSend){$State.GameValid=Test-NoSendGame $observedHost $State.Run $State.Attempt $snapshot.PreparedGeneration $pending.WorldGeneration}
            else{$State.GameValid=Test-CaptureGame $observedHost $State.Run $State.Attempt $snapshot.PreparedGeneration $pending.WorldGeneration -WrongFrequency:$State.WrongFrequency -Short400:$State.Short400 -BlindTrial:$State.BlindTrial -BlindChoice:$(if($State.BlindTrial){$State.Blind.Choice}else{-1}) -BPowerOff:$State.BPowerOff -Direct:$State.Direct}
            if($State.Short400){$State.CaptureTiming=Get-CaptureShortTiming $observedHost $State.Run $State.Attempt}
            if($State.BlindTrial){$State.CaptureTiming=Get-CaptureBlindTiming $observedHost $State.Run $State.Attempt $State.Blind.Choice}
            $requestRows=@(Get-RehearsalRows $observedHost $State.Run $State.Attempt -Mode capture | Where-Object {$_.Kind -in @('START_INTENT','START_REQUEST_RETURN','STOP_INTENT','STOP_REQUEST_RETURN','NO_SEND_START_SKIPPED','NO_SEND_STOP_SKIPPED')})
            $State.CaptureRequests=[pscustomobject]@{markers=$requestRows;control=$(if($State.Direct){'DIRECT'}elseif($State.NoSend){'NO_SEND'}elseif($State.WrongFrequency){'WRONG_FREQUENCY'}else{'NONE'});requestCycleValid=($State.GameValid -and -not $State.NoSend);noSendCycleValid=($State.GameValid -and $State.NoSend);nativeCalls=$(if($State.NoSend -and $State.GameValid){'0'}else{'UNKNOWN'});nativeCaptureState='UNKNOWN';media='NOT_TESTED'}
            $observedPeer=& $ReadClient
            if($State.Short400 -or $State.BlindTrial) {
                $State.BObservation=Get-BOwnObservation $observedPeer $State.Run $snapshot.BStart
                $State.BObservation.afterGuardReadTicks=[Diagnostics.Stopwatch]::GetTimestamp()
                $State.BObservation.commonGuardDone=($State.GuardResult.status -ceq 'KILLED' -and $State.GuardResult.targetExitConfirmed -eq $true)
                $State.BObservation.guardExitObservedTicks=[long]$State.GuardResult.exitObservedTicks
            }
            if($State.Direct){$State.DirectEvidence=Get-CaptureDirectEvidence $observedHost $observedPeer $State.Run $State.Snapshot}
            else{$State.RadioEvidence=Get-CaptureRadioEvidence $observedHost $observedPeer $State.Run $snapshot}
            if($State.WrongFrequency){$State.FrequencyRestore=Get-WrongFrequencyRestore $observedHost $State.Run $State.Attempt $State.Snapshot.FrequencyProof;$State.FrequencyInterpretation=Get-WrongFrequencyRxPhases $observedPeer $State.Run $State.Snapshot.FrequencyProof $State.RadioEvidence.receiverRadioCandidates}
        } else {$State.GameValid=Test-RehearsalGame $observedHost $State.Run $State.Attempt $snapshot.PreparedGeneration -RestArm -WorldGeneration $pending.WorldGeneration}
        $State.Valid=Test-RestRehearsalOutcome $State
        $State.Status=if($State.Valid){if($State.NoSend){'REST_NO_SEND_CONTROL_GUARDED'}elseif($State.WrongFrequency){'REST_WRONG_FREQUENCY_CONTROL_GUARDED'}elseif($State.Capture){'REST_CAPTURE_REQUEST_CYCLE_GUARDED'}else{'REST_REHEARSAL_ONLY_GUARDED'}}else{'INVALID'}
    } catch {
        $e=$_.Exception;$detail=if($e.Message -cmatch '\A[A-Z_]+\z'){$e.Message}else{$e.GetType().Name}
        $State.Attempted=$true;$State.Failure=$State.Phase+':'+$detail;$State.Status=if($State.BPowerOff -and $detail -cmatch '\APOWER_OFF_(?:BUDGET|STALE)'){'NO_RELEASE'}else{'INVALID'};$State.Valid=$false
        while($e){$State.RestErrors+=[pscustomobject]@{type=$e.GetType().FullName;hResult=$e.HResult};$e=$e.InnerException}
    }
    return $(if($State.Capture){'CAPTURE_'}else{'REHEARSAL_'})+$State.Status
}

function Get-RehearsalReceipt($State) {
    [pscustomobject]@{ run=$State.Run; attempt=$State.Attempt; attempted=$State.Attempted; status=$State.Status; valid=$State.Valid;
        failure=$State.Failure; phase=$State.Phase; targetId=$(if($State.Target){$State.Target.Id}else{0}); startUtcTicks=$State.TargetStart;
        tuple=$(if($State.Snapshot){$State.Snapshot.Signature}else{''}); dispatchBeforeTicks=$State.DispatchBeforeTicks; dispatchAfterTicks=$State.DispatchAfterTicks;
        invokeBeginTicks=$(if($State.Proxy){$State.Proxy.InvokeBeginTicks}else{0}); invokeReturnTicks=$(if($State.Proxy){$State.Proxy.InvokeReturnTicks}else{0});
        invokeCalls=$(if($State.Proxy){$State.Proxy.Calls}else{0}); invokeFailed=$(if($State.Proxy){$State.Proxy.Failed}else{$false});
        restPending=$State.RestPending;restResult=$State.RestResult;releaseBeforeTicks=$State.ReleaseBeforeTicks;releaseAfterTicks=$State.ReleaseAfterTicks;restErrors=$State.RestErrors;
        frequencyProof=$(if($State.Snapshot){$State.Snapshot.FrequencyProof}else{$null});frequencyRestore=$State.FrequencyRestore;frequencyInterpretation=$State.FrequencyInterpretation;
        geometryAdmission=$(if($State.Snapshot){$State.Snapshot.Geometry}else{$null});geometryTracking=$State.GeometryTracking;captureTiming=$State.CaptureTiming;bObservation=$State.BObservation;
        controlTag=$State.ControlTag;directMode=$State.Direct;directEvidence=$State.DirectEvidence;soundObserverStatus=$State.SoundStatus;soundObserverAdmission=$State.SoundAdmission;soundObserverTracking=$State.SoundTracking;powerOffAdmission=$State.PowerAdmission;powerOffTracking=$State.PowerTracking;
        menuDiagnostic=$State.MenuDiagnostic; initialMenuDiagnostic=$State.InitialMenuDiagnostic; gameMarkersValid=$State.GameValid; guard=$State.GuardResult; cleanup=$State.Cleanup; content='NOT_TESTED'; audio='NOT_TESTED'; capture=$(if($State.NoSend){'NOT_REQUESTED_NO_SEND_CONTROL'}elseif($State.Capture){'REQUEST_EVIDENCE_ONLY'}else{'NOT_REQUESTED_REHEARSAL'});readiness=$(if($State.Snapshot){$State.Snapshot.Readiness}else{$null});captureRequests=$State.CaptureRequests;radioEvidence=$State.RadioEvidence }
}
