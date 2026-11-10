# TOOL_ONLY parent filesystem writer. No engine FileHandle, UI, Arm, capture or PID lookup.
function Get-FhcDigest([string]$Value) {
    $sha=[Security.Cryptography.SHA256]::Create()
    try{return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($Value)))).Replace('-','')}finally{$sha.Dispose()}
}
function Assert-FhcOwnedPath([string]$Root,[string]$Path) {
    $rootPath=[IO.Path]::GetFullPath($Root).TrimEnd('\')
    $full=[IO.Path]::GetFullPath($Path)
    if(-not [IO.Directory]::Exists($rootPath) -or ($full -ine $rootPath -and -not $full.StartsWith($rootPath+'\',[StringComparison]::OrdinalIgnoreCase))){throw 'PATH_OUTSIDE_OWN_ROOT'}
    $node=New-Object IO.FileInfo($full)
    while($node){
        try {if([IO.File]::GetAttributes($node.FullName) -band [IO.FileAttributes]::ReparsePoint){throw 'REPARSE_PATH'}}
        catch [IO.FileNotFoundException] {} catch [IO.DirectoryNotFoundException] {}
        if($node -is [IO.FileInfo]){$node=$node.Directory}else{$node=$node.Parent}
    }
    return $full
}
function New-FhcWriter([string]$OwnedProfileRoot,[string]$QualifiedProfilePath,[string]$OwnedLogRoot,[string]$LogPath,[string]$Run,[string]$Token) {
    if($Run -cnotmatch '\A[A-Za-z0-9_.-]{1,64}\z' -or $Token -cnotmatch '\A[A-Za-z0-9_.-]{1,64}\z'){throw 'INVALID_FHC_IDENTITY'}
    $profile=Assert-FhcOwnedPath $OwnedProfileRoot $QualifiedProfilePath
    $null=[IO.Directory]::CreateDirectory($profile)
    $path=Assert-FhcOwnedPath $OwnedProfileRoot (Join-Path $profile ('RICON_FileHandleCapability_'+$Run+'.txt'))
    $log=Assert-FhcOwnedPath $OwnedLogRoot $LogPath
    if([IO.File]::Exists($log)){throw 'HOST_LOG_NOT_FRESH'}
    $stream=$null
    try{$stream=[IO.File]::Open($path,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::ReadWrite);$stream.Flush($true)}finally{if($stream){$stream.Dispose()}}
    return [pscustomobject]@{Root=[IO.Path]::GetFullPath($OwnedProfileRoot);Path=$path;LogRoot=[IO.Path]::GetFullPath($OwnedLogRoot);LogPath=$log;
        Run=$Run;Token=$Token;PayloadDigest=(Get-FhcDigest ($Token+"`n"));RunDigest=(Get-FhcDigest $Run);PathDigest=(Get-FhcDigest $path);
        CreatedTicks=[IO.File]::GetCreationTimeUtc($path).Ticks;LogCreatedTicks=0L;Offset=0L;Pending='';Consumed=$false;Attempted=$false;Writes=0;
        Status='EMPTY_CREATED';Target=$null;TargetStart=0L;TargetHandle=[IntPtr]::Zero;ReadyObservedTicks=0L;WriteBeginTicks=0L;WriteReturnTicks=0L;ClosedTicks=0L;Failure=''}
}
function Read-FhcLines($State) {
    $log=Assert-FhcOwnedPath $State.LogRoot $State.LogPath
    if(-not [IO.File]::Exists($log)){return @()}
    $born=[IO.File]::GetCreationTimeUtc($log).Ticks
    if($State.LogCreatedTicks -and $born -ne $State.LogCreatedTicks){throw 'HOST_LOG_REPLACED'}
    $State.LogCreatedTicks=$born
    $stream=$null;$reader=$null
    try {
        $stream=[IO.File]::Open($log,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
        if($stream.Length -lt $State.Offset){throw 'HOST_LOG_SHRANK'}
        $stream.Position=$State.Offset
        $reader=New-Object IO.StreamReader($stream,[Text.Encoding]::UTF8)
        $text=$State.Pending+$reader.ReadToEnd();$State.Offset=$stream.Position
    } finally {if($reader){$reader.Dispose()}elseif($stream){$stream.Dispose()}}
    $parts=$text.Split([char]10);$State.Pending=$parts[$parts.Length-1]
    if($parts.Length -gt 1){foreach($line in $parts[0..($parts.Length-2)]){$line.TrimEnd([char]13)}}
}
function Invoke-FhcWriterStep($State,$Target) {
    if($State.Consumed){return ''}
    try {
        $lines=@(Read-FhcLines $State)
        $ready=@();$stale=$false
        foreach($line in $lines){
            if($line -match '\[RICON_FHC\] kind=WB_REFUSED(?:\s|$)'){$stale=$true}
            $m=[regex]::Match($line,'\[RICON_FHC\] run='+[regex]::Escape($State.Run)+' kind=(?<kind>[A-Z_]+) (?<fields>[^\r\n]*)$')
            if(-not $m.Success){continue}
            if($m.Groups['kind'].Value -in @('READ_ATTEMPT','READ_RETURN','TERMINAL','WB_REFUSED')){$stale=$true}
            if($m.Groups['kind'].Value -ceq 'READY_IN_PLAY'){$ready+= $m.Groups['fields'].Value}
        }
        if($stale){$State.Consumed=$true;$State.Status='NO_WRITE_STALE_OR_REFUSED';return 'FHC_NO_WRITE_STALE_OR_REFUSED'}
        if(-not $ready.Count){return ''}
        $State.Consumed=$true;$State.ReadyObservedTicks=[Diagnostics.Stopwatch]::GetTimestamp()
        if($ready.Count -ne 1 -or $ready[0] -cnotmatch '\AreadyTickMs=-?[0-9]+,readDelayMs=5000,timeoutMs=10000,externalWrite=ONE_IN_PLACE_TOKEN_LF,media=NOT_TESTED\z'){throw 'INVALID_READY_MARKER'}
        if(-not [object]::ReferenceEquals($State.Target,$Target) -or $Target.HasExited -or $Target.StartTime.ToUniversalTime().Ticks -ne $State.TargetStart -or $Target.Handle -ne $State.TargetHandle){throw 'ORIGINAL_TARGET_CHANGED'}
        $path=Assert-FhcOwnedPath $State.Root $State.Path
        if($path -cne $State.Path -or -not [IO.File]::Exists($path) -or [IO.File]::GetCreationTimeUtc($path).Ticks -ne $State.CreatedTicks){throw 'OWN_FILE_CHANGED'}
        $State.Attempted=$true;$stream=$null
        try {
            $stream=[IO.File]::Open($path,[IO.FileMode]::Open,[IO.FileAccess]::Write,[IO.FileShare]::ReadWrite)
            if($stream.Length -ne 0){throw 'OWN_FILE_NOT_EMPTY'}
            $bytes=[Text.Encoding]::ASCII.GetBytes($State.Token+"`n")
            $State.WriteBeginTicks=[Diagnostics.Stopwatch]::GetTimestamp();$stream.Position=0
            $stream.Write($bytes,0,$bytes.Length);$stream.Flush($true);$State.WriteReturnTicks=[Diagnostics.Stopwatch]::GetTimestamp();$State.Writes=1
        } finally {if($stream){$stream.Dispose();$State.ClosedTicks=[Diagnostics.Stopwatch]::GetTimestamp()}}
        $State.Status='TOKEN_WRITTEN';return ''
    } catch {
        $State.Consumed=$true;$State.Status='PARENT_WRITE_INVALID';$detail=$_.Exception.Message
        if($detail -cnotmatch '\A[A-Z_]+\z'){$detail=$_.Exception.GetType().Name};$State.Failure=$detail
        return 'FHC_PARENT_WRITE_INVALID'
    }
}
function Get-FhcWriterReceipt($State) {
    [pscustomobject]@{status=$State.Status;runDigest=$State.RunDigest;pathDigest=$State.PathDigest;payloadDigest=$State.PayloadDigest;
        consumed=$State.Consumed;writeAttempted=$State.Attempted;writes=$State.Writes;failure=$State.Failure;
        readyObservedTicks=$State.ReadyObservedTicks;writeBeginTicks=$State.WriteBeginTicks;writeReturnTicks=$State.WriteReturnTicks;closedTicks=$State.ClosedTicks;
        clockScope='PARENT_QPC_NOT_GAME_TICK';nativeSharing='UNKNOWN';voice='NOT_TESTED';capture='NOT_REQUESTED'}
}
