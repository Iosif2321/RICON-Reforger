# TOOL_ONLY N0. Default-off, one fixed own helper, no game/input/focus/audio.
[CmdletBinding()]
param([switch]$EnableN0, [string]$Scenario, [string]$OutRoot)
if (-not $EnableN0) { Write-Output 'N0_DISABLED'; return }
Set-StrictMode -Version 2.0
$ErrorActionPreference='Stop'
if ($Scenario -cnotin @('Normal','DelayEmit','Hanging') -or -not $OutRoot) { throw 'N0_BAD_ARGUMENTS' }
if ($env:OS -ne 'Windows_NT' -or $PSVersionTable.PSVersion.Major -ne 5 -or
    $PSVersionTable.PSEdition -ne 'Desktop') { throw 'N0_UNSUPPORTED_HOST' }
# Fixed sibling only. Loading this definitions-only file requires the separate execution gate.
. (Join-Path $PSScriptRoot 'k0-neutral-core.ps1')
$utf8=New-Object Text.UTF8Encoding($false,$true)
$process=$null; $wireReader=$null; $logger=$null; $wireSha=$null
$script:n0State=$null; $script:n0Decision=$null; $script:n0Held=$false; $script:n0KillAttempted=$false
$script:n0CommandError=''; $script:n0WaitError=''; $script:n0Wake=$null
$script:n0Pending=New-Object byte[] 1024; $script:n0PendingCount=0
$script:n0ReadBuffer=New-Object byte[] 4096; $script:n0ChunkCount=0; $script:n0ChunkOffset=0
$script:n0Ready=$false; $script:n0ReadMark=[long]0; $script:n0WireTotal=0
$script:n0WireCount=0; $script:n0FinalError=''; $script:n0Ended=$false
$runDir=$null; $manifestPath=$null; $wirePath=$null; $runnerPath=$null; $receiptPath=$null
$manifestDigest=$null; $wireDigest=$null; $runnerDigest=$null; $scriptHashes=[ordered]@{}
$identity=@{pid=$null;startTimeUtc=$null;executable=$null;handleHeld=$false}
$script:n0LastClock=$null
function Get-N0Clock {
    $tick=[Diagnostics.Stopwatch]::GetTimestamp()
    if ($null -ne $script:n0LastClock -and $tick -lt $script:n0LastClock -and $script:n0State) {
        $script:n0State.ClockValid=$false
        if (-not $script:n0State.InvalidReason) {
            $script:n0State.InvalidReason='INVALID_CLOCK'; $script:n0State.InvalidTick=$tick
            $script:n0State.Phase='INVALID'
        }
    }
    $script:n0LastClock=$tick
    return $tick
}

function Get-N0Digest([string]$Path,[long]$Limit) {
    $stream=$null; $hash=$null
    try {
        $stream=New-Object IO.FileStream($Path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
        if ($stream.Length -gt $Limit) { throw 'N0_HASH_SIZE_LIMIT' }
        $hash=[Security.Cryptography.SHA256]::Create(); $buf=New-Object byte[] 4096; [long]$total=0
        while ($true) {
            $take=[int][Math]::Min(4096,([decimal]$Limit-$total+1))
            $n=$stream.Read($buf,0,$take)
            if ($n -eq 0) { break }
            $total+=$n
            if ($total -gt $Limit) { throw 'N0_HASH_SIZE_LIMIT' }
            $null=$hash.TransformBlock($buf,0,$n,$buf,0)
        }
        $null=$hash.TransformFinalBlock((New-Object byte[] 0),0,0)
        return ([BitConverter]::ToString($hash.Hash)).Replace('-','').ToLowerInvariant()
    } finally { if ($stream) { $stream.Dispose() }; if ($hash) { $hash.Dispose() } }
}
function Assert-N0Root([string]$Path) {
    if ($Path -match '[\x00-\x1f\x7f"]' -or $Path -notmatch '^[a-zA-Z]:\\' -or
        $Path.Substring(2).Contains(':')) { throw 'N0_UNSAFE_PATH' }
    $full=[IO.Path]::GetFullPath($Path)
    $item=Get-Item -LiteralPath $full -Force
    if (-not $item.PSIsContainer) { throw 'N0_NOT_DIRECTORY' }
    $walk=$item
    while ($walk) {
        if (($walk.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw 'N0_REPARSE_PATH' }
        $walk=$walk.Parent
    }
    $drive=New-Object IO.DriveInfo([IO.Path]::GetPathRoot($full))
    if ($drive.DriveType -notin @([IO.DriveType]::Fixed,[IO.DriveType]::Removable,[IO.DriveType]::Ram)) {
        throw 'N0_NONLOCAL_PATH'
    }
    $checkout=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..')).TrimEnd('\')
    if ($full.Equals($checkout,[StringComparison]::OrdinalIgnoreCase) -or
        $full.StartsWith($checkout+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'N0_CHECKOUT_OUTPUT' }
    return $item.FullName
}
function Write-N0NewJson([string]$Path,$Value,[int]$Limit) {
    $bytes=$utf8.GetBytes(($Value | ConvertTo-Json -Depth 12 -Compress))
    if ($bytes.Length -gt $Limit) { throw 'N0_JSON_SIZE_LIMIT' }
    $out=$null
    try {
        $out=New-Object IO.FileStream($Path,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
        $out.Write($bytes,0,$bytes.Length); $out.Flush($true)
    } finally { if ($out) { $out.Dispose() } }
}
function Write-N0Log([string]$Kind,[string]$Bucket='Ordinary',[hashtable]$Fields=@{}) {
    # Logging never precedes a due containment command: caller stops helper first on failure.
    foreach ($key in @('begin','returned','deadline','wake','tick','readTick')) {
        if ($Fields.ContainsKey($key) -and $null -ne $Fields[$key]) {
            $Fields[$key]=([long]$Fields[$key]).ToString([Globalization.CultureInfo]::InvariantCulture)
        }
    }
    $row=[ordered]@{schema=1;kind=$Kind;runId=$runId;scenario=$Scenario
        tick=((Get-N0Clock)).ToString();fields=$Fields}
    $bytes=$utf8.GetBytes(($row | ConvertTo-Json -Depth 6 -Compress)+"`n")
    $before=$script:n0State[$Bucket]
    $script:n0State=Update-N0State $script:n0State 'Log' ((Get-N0Clock)) @{Bucket=$Bucket;Bytes=$bytes.Length}
    if ($script:n0State[$Bucket] -eq $before) { throw 'N0_LOG_QUOTA' }
    $logger.Write($bytes,0,$bytes.Length)
    # No blocking durable Flush in the observation loop; close/flush happens after teardown.
}
function Set-N0Failure([string]$Reason) {
    if ($script:n0State) {
        $script:n0State=Update-N0State $script:n0State 'Invalidate' ((Get-N0Clock)) @{Reason=$Reason}
    }
}
function Stop-N0Owned([hashtable]$Decision) {
    if (-not $process -or $script:n0KillAttempted -or $script:n0State.CommandAttempted -or $script:n0State.NaturalExit) { return }
    if (-not $script:n0Held) { $script:n0State.CleanupResult='INCONCLUSIVE_CLEANUP'; return }
    try {
        if ($process.HasExited) {
            $now=(Get-N0Clock)
            $script:n0State=Update-N0State $script:n0State 'NaturalExit' $now
            $script:n0State=Update-N0State $script:n0State 'Exit' $now @{Confirmed=$true}
            return
        }
        if (-not $Decision -or $null -eq $Decision.Deadline) {
            $Decision=@{Deadline=(Get-N0Clock);Reason='INVALID_ABORT';WakeTick=(Get-N0Clock)}
        }
        $script:n0Wake=$Decision.WakeTick
        $begin=(Get-N0Clock)
        $script:n0KillAttempted=$true
        $script:n0State=Update-N0State $script:n0State 'CommandBegin' $begin @{Deadline=$Decision.Deadline;Reason=$Decision.Reason}
        if (-not $script:n0State.CommandAttempted) { throw 'N0_NO_COMMAND_AUTHORITY' }
        $ok=$false
        try { $process.Kill(); $ok=$true }
        catch { $script:n0CommandError=$_.Exception.GetType().FullName }
        $returned=(Get-N0Clock)
        $script:n0State=Update-N0State $script:n0State 'CommandReturn' $returned @{Success=$ok}
        $confirmed=$false
        try { $confirmed=$process.WaitForExit(5000) -and $process.HasExited }
        catch { $script:n0WaitError=$_.Exception.GetType().FullName }
        $script:n0State=Update-N0State $script:n0State 'Exit' ((Get-N0Clock)) @{Confirmed=$confirmed}
    } catch {
        # Never recover/reopen by PID, and never repeat a registered command attempt.
        $script:n0WaitError=$_.Exception.GetType().FullName
        $script:n0State.CleanupResult='INCONCLUSIVE_CLEANUP'
    }
}
function Save-N0Prefix([byte]$Extra,[bool]$HasExtra) {
    $length=$script:n0PendingCount
    if ($HasExtra) { $length++ }
    $prefix=New-Object byte[] $length
    if ($script:n0PendingCount) { [Array]::Copy($script:n0Pending,$prefix,$script:n0PendingCount) }
    if ($HasExtra) { $prefix[$length-1]=$Extra }
    $script:n0State.DiagnosticPrefix=[Convert]::ToBase64String($prefix)
    $script:n0State.RawComplete=$false
}
function Read-N0Wire([switch]$Final) {
    while ($true) {
        if (-not $Final) {
            $script:n0Decision=Get-N0DeadlineDecision $script:n0State ((Get-N0Clock))
            if ($script:n0Decision.Stop) { return }
        }
        if ($script:n0Ready) {
            if (-not $Final) {
                $script:n0Decision=Get-N0DeadlineDecision $script:n0State ((Get-N0Clock))
                if ($script:n0Decision.Stop) { return }
            }
            if ($script:n0WireCount -ge 16) {
                Save-N0Prefix 0 $false; Set-N0Failure 'INVALID_QUOTA'; $script:n0FinalError='WIRE_RECORD_LIMIT'; return
            }
            $line=New-Object byte[] $script:n0PendingCount
            [Array]::Copy($script:n0Pending,$line,$script:n0PendingCount)
            $script:n0State=Update-N0State $script:n0State 'Record' $script:n0ReadMark @{Bytes=$line}
            $script:n0WireCount++
            $script:n0PendingCount=0; $script:n0Ready=$false
            if (-not $Final -and $script:n0State.InvalidReason) { return }
            # Raw wire is already evidence; defer all runner-file IO until after containment.
        }
        if ($script:n0ChunkOffset -ge $script:n0ChunkCount) {
            if ($wireReader.Length -gt 16384) { Set-N0Failure 'INVALID_QUOTA'; $script:n0FinalError='WIRE_OVERSIZE'; return }
            $script:n0ChunkCount=$wireReader.Read($script:n0ReadBuffer,0,4096); $script:n0ChunkOffset=0
            if ($script:n0ChunkCount -eq 0) { return }
            $script:n0WireTotal+=$script:n0ChunkCount
            if ($script:n0WireTotal -gt 16384) { Set-N0Failure 'INVALID_QUOTA'; $script:n0FinalError='WIRE_OVERSIZE'; return }
            $null=$wireSha.TransformBlock($script:n0ReadBuffer,0,$script:n0ChunkCount,$script:n0ReadBuffer,0)
            $script:n0State.WireBytes=$script:n0WireTotal
        }
        while ($script:n0ChunkOffset -lt $script:n0ChunkCount) {
            if (-not $Final) {
                $script:n0Decision=Get-N0DeadlineDecision $script:n0State ((Get-N0Clock))
                if ($script:n0Decision.Stop) { return }
            }
            $byte=$script:n0ReadBuffer[$script:n0ChunkOffset]
            if ($script:n0PendingCount -ge 1024) {
                Save-N0Prefix $byte $true; Set-N0Failure 'INVALID_QUOTA'; $script:n0FinalError='RECORD_OVERSIZE'; return
            }
            $script:n0Pending[$script:n0PendingCount]=$byte; $script:n0PendingCount++; $script:n0ChunkOffset++
            if ($byte -eq 10) {
                $script:n0ReadMark=(Get-N0Clock); $script:n0Ready=$true
                break
            }
        }
    }
}
function Test-N0FinalWire {
    # Second bounded streaming pass: no positive receipt may rely on an unchecked final tail.
    $audit=$null; $auditSha=$null
    try {
        if (-not $wireReader -or $script:n0FinalError -or $script:n0PendingCount -ne 0 -or $script:n0Ready) {
            if ($script:n0PendingCount) { Save-N0Prefix 0 $false }
            return $false
        }
        $null=$wireSha.TransformFinalBlock((New-Object byte[] 0),0,0)
        $seenHash=([BitConverter]::ToString($wireSha.Hash)).Replace('-','').ToLowerInvariant()
        $audit=New-Object IO.FileStream($wirePath,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
        if ($audit.Length -gt 16384 -or $audit.Length -ne $script:n0WireTotal) { return $false }
        $auditSha=[Security.Cryptography.SHA256]::Create(); $buf=New-Object byte[] 4096
        $pending=New-Object byte[] 1024; $count=0; $records=0; $total=0
        while ($true) {
            $n=$audit.Read($buf,0,4096)
            if (-not $n) { break }
            $total+=$n; if ($total -gt 16384) { return $false }
            $null=$auditSha.TransformBlock($buf,0,$n,$buf,0)
            for ($i=0;$i -lt $n;$i++) {
                if ($count -ge 1024) { return $false }
                $pending[$count]=$buf[$i]; $count++
                if ($buf[$i] -eq 10) {
                    if ($records -ge 16) { return $false }
                    $line=New-Object byte[] $count; [Array]::Copy($pending,$line,$count)
                    $check=Test-N0Record -Bytes $line -State $script:n0State
                    if (-not $check.Valid -or $check.Record['seq'] -ne $records) { return $false }
                    $expectedKind='N0_SAMPLE'
                    if ($records -eq 0) { $expectedKind='N0_HELLO' }
                    elseif ($records -eq 9) { $expectedKind='N0_DONE' }
                    elseif ($records -gt 9) { return $false }
                    if ($check.Record['kind'] -cne $expectedKind) { return $false }
                    $records++; $count=0
                }
            }
        }
        $null=$auditSha.TransformFinalBlock((New-Object byte[] 0),0,0)
        $finalHash=([BitConverter]::ToString($auditSha.Hash)).Replace('-','').ToLowerInvariant()
        if ($count -ne 0 -or $records -ne $script:n0WireCount -or $seenHash -cne $finalHash) { return $false }
        $script:n0FinalDigest=$finalHash
        return $true
    } finally { if ($audit) { $audit.Dispose() }; if ($auditSha) { $auditSha.Dispose() } }
}

try {
    if (-not [Diagnostics.Stopwatch]::IsHighResolution -or [Diagnostics.Stopwatch]::Frequency -le 0) { throw 'N0_BLOCKED_CLOCK' }
    $root=Assert-N0Root $OutRoot
    $runId=[guid]::NewGuid().ToString('N'); $token=[guid]::NewGuid().ToString('N')
    $runDir=Join-Path $root ('ricon-n0-'+$runId)
    if (Test-Path -LiteralPath $runDir) { throw 'N0_PREEXISTING_OUTPUT' }
    $null=New-Item -ItemType Directory -Path $runDir -ErrorAction Stop
    $null=Assert-N0Root $runDir
    $manifestPath=Join-Path $runDir 'manifest.json'; $wirePath=Join-Path $runDir 'wire.jsonl'
    $runnerPath=Join-Path $runDir 'runner.jsonl'; $receiptPath=Join-Path $runDir 'receipt.json'
    $fixedExe='C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe'
    $helper=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'k0-neutral-helper.ps1'))
    if ($helper -match '[\x00-\x1f\x7f"]' -or $runDir -match '[\x00-\x1f\x7f"]') { throw 'N0_UNSAFE_ARGUMENT' }
    foreach ($name in @('k0-neutral-core.ps1','k0-neutral-helper.ps1','k0-neutral-preflight.ps1','k0-neutral-selftest.ps1')) {
        $scriptHashes[$name]=Get-N0Digest (Join-Path $PSScriptRoot $name) 1048576
    }
    $frequency=[Diagnostics.Stopwatch]::Frequency
    $version=[Diagnostics.FileVersionInfo]::GetVersionInfo($fixedExe).FileVersion
    $manifest=[ordered]@{schema=1;runId=$runId;ownerToken=$token;scenario=$Scenario;inputGit='2880204'
        writingBaseline='1e58666';runDir=$runDir;helperScript=$helper;executable=$fixedExe;executableVersion=$version
        host=[Environment]::MachineName;os=[Environment]::OSVersion.Version.ToString();psVersion=$PSVersionTable.PSVersion.ToString()
        clockSource='QPC_STOPWATCH_RAW';frequency=$frequency.ToString();highResolution=$true;runnerPid=$PID;scripts=$scriptHashes}
    Write-N0NewJson $manifestPath $manifest 8192
    $manifestDigest=Get-N0Digest $manifestPath 8192
    $logger=New-Object IO.FileStream($runnerPath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
    $psi=New-Object Diagnostics.ProcessStartInfo
    $psi.FileName=$fixedExe; $psi.UseShellExecute=$false; $psi.CreateNoWindow=$true
    $psi.RedirectStandardOutput=$false; $psi.RedirectStandardError=$false; $psi.RedirectStandardInput=$false
    $psi.Arguments='-NoProfile -NonInteractive -File "'+$helper+'" -EnableN0 -RunId '+$runId+
        ' -RunDir "'+$runDir+'" -OwnerToken '+$token+' -Scenario '+$Scenario
    Write-Output ('N0_START run='+$runId+' scenario='+$Scenario+' helperCLI='+$psi.FileName+' '+$psi.Arguments)
    # t0 is sampled once, immediately before Start. Start itself is not promised bounded.
    $t0=(Get-N0Clock)
    $script:n0State=New-N0State $runId $token $Scenario $frequency $t0
    $process=[Diagnostics.Process]::Start($psi)
    if (-not $process) { throw 'N0_START_FAILED' }
    $heldHandle=$process.Handle
    if ($heldHandle -eq [IntPtr]::Zero) { throw 'N0_HANDLE_FAILED' }
    $script:n0Held=$true
    $script:n0State=Update-N0State $script:n0State 'Ownership' ((Get-N0Clock)) @{HasHandle=$true;HelperPid=$process.Id}
    $identity=@{pid=$process.Id;startTimeUtc=$process.StartTime.ToUniversalTime().ToString('o');executable=$fixedExe;handleHeld=$true}
    $wireSha=[Security.Cryptography.SHA256]::Create()
    while ($true) {
        $now=(Get-N0Clock)
        $script:n0Decision=Get-N0DeadlineDecision $script:n0State $now
        if ($script:n0Decision.Stop) {
            if ($script:n0Decision.Reason -ne 'PROBE_COMPLETE') { Set-N0Failure $script:n0Decision.Reason }
            Stop-N0Owned $script:n0Decision; break
        }
        if ($process.HasExited) {
            $script:n0State=Update-N0State $script:n0State 'NaturalExit' $now
            $script:n0State=Update-N0State $script:n0State 'Exit' $now @{Confirmed=$true}; break
        }
        if (-not $wireReader -and (Test-Path -LiteralPath $wirePath)) {
            $item=Get-Item -LiteralPath $wirePath -Force
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw 'N0_WIRE_REPARSE' }
            $wireReader=New-Object IO.FileStream($wirePath,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
        }
        if ($wireReader) { Read-N0Wire }
        if ($script:n0State.InvalidReason) {
            $script:n0Decision=Get-N0DeadlineDecision $script:n0State ((Get-N0Clock))
            Stop-N0Owned $script:n0Decision; break
        }
        if ($script:n0Decision -and $script:n0Decision.Stop) {
            if ($script:n0Decision.Reason -ne 'PROBE_COMPLETE') { Set-N0Failure $script:n0Decision.Reason }
            Stop-N0Owned $script:n0Decision; break
        }
        [Threading.Thread]::Sleep(10)
    }
} catch {
    $script:n0FinalError=$_.Exception.Message
    if ($script:n0FinalError.Length -gt 128) { $script:n0FinalError=$script:n0FinalError.Substring(0,128) }
    Set-N0Failure 'INVALID_RUNNER_EXCEPTION'
    if ($script:n0State) { Stop-N0Owned (Get-N0DeadlineDecision $script:n0State ((Get-N0Clock))) }
} finally {
    # Owned Process is the sole authority. No PID lookup/name/tree cleanup, no second Kill.
    if ($script:n0State -and $process -and -not $script:n0State.CommandAttempted -and -not $script:n0State.NaturalExit) {
        Set-N0Failure 'INVALID_FINALLY'
        Stop-N0Owned (Get-N0DeadlineDecision $script:n0State ((Get-N0Clock)))
    }
    if ($script:n0State) {
        try {
            if ($script:n0State.ExitConfirmed -and $wireReader -and -not $script:n0FinalError) { Read-N0Wire -Final }
            if ($script:n0FinalError -eq 'WIRE_OVERSIZE') {
                $script:n0State.RawComplete=$false
                if ($script:n0State.ExitConfirmed -and $wireReader) {
                    $script:n0State.WireBytes=$wireReader.Length
                    $wireReader.Position=0; $prefix=New-Object byte[] 1025; $prefixCount=0
                    while ($prefixCount -lt 1025) {
                        $n=$wireReader.Read($prefix,$prefixCount,1025-$prefixCount)
                        if (-not $n) { break }; $prefixCount+=$n
                    }
                    $script:n0State.DiagnosticPrefix=[Convert]::ToBase64String($prefix,0,$prefixCount)
                }
            }
            $script:n0State.TailComplete=($script:n0PendingCount -eq 0 -and -not $script:n0Ready -and -not $script:n0FinalError)
            $script:n0FinalDigest=$null
            $script:n0State.WireFinalValid=Test-N0FinalWire
            $wireDigest=$script:n0FinalDigest
            if (-not $script:n0State.WireFinalValid) { $script:n0State.Integrity=$false }
            $finalManifestDigest=Get-N0Digest $manifestPath 8192
            if ($finalManifestDigest -cne $manifestDigest) {
                $script:n0State.Integrity=$false; $script:n0FinalError='N0_MANIFEST_CHANGED'
            }
            if ($logger) {
                if ($script:n0State.InvalidReason) {
                    Write-N0Log 'N0_INVALID' 'ReserveInvalid' @{reason=$script:n0State.InvalidReason
                        rawComplete=$script:n0State.RawComplete;prefixBase64=$script:n0State.DiagnosticPrefix
                        observedWireBytes=$script:n0State.WireBytes;pendingBytes=$script:n0PendingCount}
                }
                Write-N0Log 'N0_COMMAND' 'ReserveStop' @{begin=$script:n0State.CommandBegin;returned=$script:n0State.CommandReturn
                    deadline=$script:n0State.CommandDeadline;wake=$script:n0Wake;success=$script:n0State.CommandSuccess;error=$script:n0CommandError}
                Write-N0Log 'N0_EXIT' 'ReserveStop' @{confirmed=$script:n0State.ExitConfirmed;tick=$script:n0State.ExitTick;error=$script:n0WaitError}
                Write-N0Log 'N0_TERMINAL' 'ReserveCleanup' @{wireFinalValid=$script:n0State.WireFinalValid;tailComplete=$script:n0State.TailComplete}
                $logger.Flush($true); $logger.Dispose(); $logger=$null
                $runnerDigest=Get-N0Digest $runnerPath 163840
            } else { $script:n0State.Integrity=$false }
        } catch { $script:n0State.Integrity=$false; $script:n0FinalError='N0_FINAL_INTEGRITY_FAILURE' }
        try {
            $verdict=Get-N0Verdict $script:n0State
            $receipt=[ordered]@{schema=1;runId=$runId;scenario=$Scenario;inputGit='2880204';writingBaseline='1e58666'
                scripts=$scriptHashes;manifestSha256=$manifestDigest;wireSha256=$wireDigest;runnerSha256=$runnerDigest
                host=[Environment]::MachineName;psVersion=$PSVersionTable.PSVersion.ToString();clockSource='QPC_STOPWATCH_RAW'
                frequency=$script:n0State.Frequency.ToString();highResolution=[Diagnostics.Stopwatch]::IsHighResolution
                process=$identity;t0=$script:n0State.T0.ToString();helloRead=$script:n0State.HelloRead
                firstDeadline=$script:n0State.FirstDeadline;probeDeadline=$script:n0State.ProbeDeadline
                absoluteDeadline=$script:n0State.AbsoluteDeadline.ToString();commandDeadline=$script:n0State.CommandDeadline
                wakeTick=$script:n0Wake;commandBegin=$script:n0State.CommandBegin;commandReturn=$script:n0State.CommandReturn
                commandError=$script:n0CommandError;exitTick=$script:n0State.ExitTick;waitError=$script:n0WaitError
                samples=$script:n0State.SampleSummaries.ToArray();wireRecords=$script:n0WireCount;wireBytes=$script:n0State.WireBytes
                ordinary=$script:n0State.Ordinary;reserveInvalid=$script:n0State.ReserveInvalid;reserveStop=$script:n0State.ReserveStop
                reserveCleanup=$script:n0State.ReserveCleanup
                droppedRecords=$(if ($script:n0State.RawComplete -and $script:n0State.TailComplete) { 0 } else { 'UNKNOWN' })
                rawComplete=$script:n0State.RawComplete
                incompleteTailBytes=$script:n0PendingCount;terminalWritten=($null -ne $runnerDigest);failure=$script:n0FinalError
                thresholds=@{sampleMs=250;dispatchMs=100;exitMs=5000};results=$verdict
                realFocus='NOT_RUN';enforceClock='UNKNOWN';nativeRelease='UNKNOWN';trustedSameUserEnvironment=$true}
            # All raw ticks/frequencies in the receipt are decimal strings, including optional marks.
            foreach ($key in @('helloRead','firstDeadline','probeDeadline','commandDeadline','wakeTick','commandBegin','commandReturn','exitTick')) {
                if ($null -ne $receipt[$key]) { $receipt[$key]=([long]$receipt[$key]).ToString([Globalization.CultureInfo]::InvariantCulture) }
            }
            if (-not $receiptPath) { throw 'N0_NO_RECEIPT_PATH' }
            Write-N0NewJson $receiptPath $receipt 8192
            $receiptHash=Get-N0Digest $receiptPath 8192
            Write-Output ('N0_END verdict='+$verdict.verdict+' receipt='+$receiptPath+' sha256='+$receiptHash)
        } catch { Write-Output 'N0_END INCONCLUSIVE_LOG_INTEGRITY receipt could not be completed' }
    } else { Write-Output ('N0_BLOCKED_SETUP '+$script:n0FinalError) }
    # Retained native/.NET resources: one Process + handle, reader, logger, wire SHA;
    # temporary audit/digest/file streams own try/finally scopes. Buffers 4096+1024,
    # final audit 4096+1024, at most 8 sample summaries. No retained foreign process refs.
    if ($wireReader) { $wireReader.Dispose() }
    if ($logger) { $logger.Dispose() }
    if ($wireSha) { $wireSha.Dispose() }
    if ($process) { $process.Dispose() }
}
