# N1 value model, recorded plan v0.2. Definitions only; PS5.1 syntax.
# No adapter consumes the symbolic intents. All milliseconds/handles are fake values.
# Public API: New-K0ModelState, Test-K0ModelInput, Update-K0Model, Get-K0ModelVerdict.

function Test-K0Shape {
    param($Value, [string[]]$Keys)
    if ($Value -isnot [hashtable] -or $Value.Count -ne $Keys.Count) { return $false }
    foreach ($key in $Value.Keys) {
        if ($key -isnot [string] -or $Keys -cnotcontains $key) { return $false }
    }
    return $true
}

function Test-K0Text {
    param($Value)
    return ($Value -is [string] -and $Value.Length -le 256 -and $Value -cmatch '\A[\x20-\x7E]*\z')
}

function Test-K0Time {
    param($Value)
    return ($Value -is [long] -and $Value -ge 0 -and $Value -le 1000000000)
}

function New-K0RoleValue {
    return @{
        Owned = $false; IdentityValid = $false; HandleToken = 'NONE'
        BindingGeneration = [long]1; SubscriptionGeneration = [long]1
        LastFrameSeq = [long]0; LastReceiveMs = $null; LastSnapshotMs = $null
        SnapshotCount = [long]0; RawCount = [long]0
        SafetyCount = [long]0; DiagnosticCount = [long]0; CleanupCount = [long]0
        BudgetBytes = [long]0; RejectedCount = [long]0
        PartialMs = $null; PartialFault = $false
        IntentLatched = $false; IntentId = ''; BeginObserved = $false; BeginMs = $null
        CallId = ''; ReturnObserved = $false; ReturnMs = $null; ReturnSuccess = $false
        ExitConfirmed = $false; NaturalExit = $false
        SubscriptionsZero = $false; TerminalComplete = $false
    }
}

function New-K0ModelState {
    param($RunId, $StartMs)
    if ($RunId -isnot [string] -or $RunId -cnotmatch '\A[0-9a-f]{32}\z' -or
        -not (Test-K0Time $StartMs) -or $StartMs -gt 999800000) { throw 'N1_NEW_SCHEMA' }
    # Every addition is bounded before it occurs. Max future offset is 120000.
    return @{
        Schema = [long]1; RunId = $RunId; AttemptId = [long]1
        StartMs = $StartMs; NowMs = $StartMs; StartupDue = [long]($StartMs + 120000)
        Phase = 'STARTUP'; PhaseEnteredMs = $StartMs
        ObservationStart = $null; AbsoluteDue = $null; ArmDue = $null
        PendingDue = $null; FallbackDue = $null; HeldDue = $null; PostUpDue = $null
        CleanupDue = $null; DownMarkMs = $null
        AcceptedDownReceiveMs = $null; AcceptedDownCallbackHighMs = $null
        PrimaryReason = 'NONE'; PrimaryRank = [long]99; EarliestReference = $null
        ReferenceUnknown = $false; ProtocolFault = $false; SafetyFault = $false
        IntegrityFault = $false; Invalid = $false; AbortTransition = $false
        ModelCaptureWindow = $false; ModelRxWindow = $false
        Roles = @{ A = (New-K0RoleValue); B = (New-K0RoleValue) }
    }
}

function Test-K0StateValue {
    param($State)
    $template = New-K0ModelState -RunId '00000000000000000000000000000000' -StartMs ([long]0)
    if (-not (Test-K0Shape $State @($template.Keys))) { return $false }
    if (-not (Test-K0Shape $State.Roles @('A','B'))) { return $false }
    foreach ($key in $template.Keys) {
        if ($key -ceq 'Roles') { continue }
        $v = $State[$key]; $t = $template[$key]
        if ($null -eq $t) {
            if ($null -ne $v -and -not (Test-K0Time $v)) { return $false }
        } elseif ($t -is [bool]) {
            if ($v -isnot [bool]) { return $false }
        } elseif ($t -is [long]) {
            if (-not (Test-K0Time $v)) { return $false }
        } elseif (-not (Test-K0Text $v)) { return $false }
    }
    if ($State.Schema -ne 1 -or $State.AttemptId -ne 1 -or
        $State.RunId -cnotmatch '\A[0-9a-f]{32}\z' -or $State.StartMs -gt 999800000 -or
        $State.NowMs -lt $State.StartMs -or $State.PhaseEnteredMs -gt $State.NowMs -or
        @('STARTUP','READINESS','ARMED','PENDING_DOWN','HELD','POST_UP','TEARDOWN','TERMINAL') -cnotcontains $State.Phase) { return $false }
    $reasons = @('NONE','NORMAL_POST_UP','STARTUP_DUE','ARM_DUE','PENDING_DUE','FALLBACK_DUE','HELD_DUE','ABSOLUTE_DUE',
        'INVALID_STATE_SCHEMA','INVALID_INPUT_SCHEMA','INVALID_IDENTITY','INVALID_CLOCK','INVALID_GENERATION',
        'INVALID_FRAME_SEQUENCE','INVALID_FRAME_TIME_ORDER','INCONCLUSIVE_EDGE_TIME_ORDER','INVALID_INTERVAL',
        'INVALID_NOTIFICATION','INVALID_ERROR_BUDGET','INVALID_ROUTE','INVALID_PHASE','INVALID_SNAPSHOT_CADENCE',
        'INVALID_ENCODING','INVALID_FRAME_SCHEMA','OVERSIZE_RECORD','INVALID_FIELD_SIZE','INVALID_QUOTA',
        'PARTIAL_FAULT','FAULT_FRAME','FAULT_FRAMING','FAULT_IO','FAULT_QUOTA','FAULT_FOCUS','FAULT_OWNERSHIP',
        'FAULT_GENERATION','FAULT_CLOCK','UNKNOWN_REFERENCE','BLOCKED_OWNERSHIP','INVALID_COMMAND',
        'OWNER_CONFLICT','WATCHDOG_SUSPENDED','BLOCKING_FIRST_CALL','CRASH_AFTER_LATCH','HOST_HANG',
        'INCONCLUSIVE_LOG_INTEGRITY','TERMINAL_EXTRA_INPUT')
    if ($reasons -cnotcontains $State.PrimaryReason -or
        @(1,2,3,4,99) -notcontains $State.PrimaryRank) { return $false }
    foreach ($name in @('A','B')) {
        $role = $State.Roles[$name]; $rt = $template.Roles[$name]
        if (-not (Test-K0Shape $role @($rt.Keys))) { return $false }
        foreach ($key in $rt.Keys) {
            $v = $role[$key]; $t = $rt[$key]
            if ($null -eq $t) {
                if ($null -ne $v -and (-not (Test-K0Time $v) -or $v -gt $State.NowMs)) { return $false }
            } elseif ($t -is [bool]) {
                if ($v -isnot [bool]) { return $false }
            } elseif ($t -is [long]) {
                if ($v -isnot [long] -or $v -lt 0) { return $false }
            } elseif (-not (Test-K0Text $v)) { return $false }
        }
        if ($role.BindingGeneration -ne 1 -or $role.SubscriptionGeneration -ne 1 -or
            $role.LastFrameSeq -gt 400 -or $role.SnapshotCount -gt 240 -or $role.RawCount -gt 128 -or
            $role.SafetyCount -gt 4 -or $role.DiagnosticCount -gt 16 -or $role.CleanupCount -gt 12 -or
            $role.RejectedCount -gt 400 -or $role.BudgetBytes -gt 1638400 -or
            @('NONE',('N1_HANDLE_' + $name + '_1')) -cnotcontains $role.HandleToken -or
            $role.IntentId.Length -gt 64 -or $role.CallId.Length -gt 64) { return $false }
    }
    return $true
}

function Copy-K0StateValue {
    param([hashtable]$State)
    # Called only after fixed shape/scalar validation; no recursive arbitrary graph walk.
    $copy = @{}
    foreach ($key in $State.Keys) { if ($key -cne 'Roles') { $copy[$key] = $State[$key] } }
    $copy.Roles = @{ A = @{}; B = @{} }
    foreach ($name in @('A','B')) {
        foreach ($key in $State.Roles[$name].Keys) { $copy.Roles[$name][$key] = $State.Roles[$name][$key] }
    }
    return $copy
}

function Get-K0InputReference {
    param($EventInput)
    if ($EventInput -is [hashtable] -and $EventInput.Count -le 24 -and
        (Test-K0Time $EventInput.NowMs) -and (Test-K0Time $EventInput.ReceiveMs) -and
        $EventInput.ReceiveMs -le $EventInput.NowMs) { return $EventInput.ReceiveMs }
    return $null
}

function New-K0ValidationValue {
    param([bool]$Valid, [string]$Reason, $ReferenceMs)
    return @{ Valid = $Valid; Reason = $Reason; ReferenceMs = $ReferenceMs }
}

function Test-K0ModelInput {
    param($State, [Alias('Input')]$EventInput)
    if (-not (Test-K0StateValue $State)) { return (New-K0ValidationValue $false 'INVALID_STATE_SCHEMA' $null) }
    $ref = Get-K0InputReference $EventInput
    if ($EventInput -isnot [hashtable] -or $EventInput.Count -gt 24 -or
        $EventInput.Kind -isnot [string]) { return (New-K0ValidationValue $false 'INVALID_INPUT_SCHEMA' $ref) }
    $keys = @('Kind','RunId','AttemptId','NowMs')
    switch -CaseSensitive ($EventInput.Kind) {
        'ROLES_READY' { $keys += @('AOwned','BOwned','AIdentityValid','BIdentityValid','AHandleToken','BHandleToken') }
        'FRAME' { $keys += @('Role','FrameSeq','Event','ReceiveMs','BindingGeneration','SubscriptionGeneration','SenderMatch',
            'RefClass','EncodingValid','SchemaValid','EncodedBytes','FieldChars','RawArgCount','DiagnosticBytes',
            'CallbackLowMs','CallbackEstimateMs','CallbackHighMs','OtherErrorMs') }
        'PARTIAL' { $keys += @('Role','ReadBeginMs','BufferedBytes') }
        'FAULT' { $keys += @('Role','Class','OperationBeginMs','ReceiveMs','PartialFault','DiagnosticBytes') }
        'COMMAND_BEGIN' { $keys += @('Role','IntentId','Owner','HandleToken','CallId') }
        'COMMAND_RETURN' { $keys += @('Role','IntentId','CallId','Success') }
        'ROLE_EXIT' { $keys += @('Role','HandleToken','ExitConfirmed','NaturalExit') }
        'DETACH' { $keys += @('Role','SubscriptionsZero','TerminalComplete') }
        { @('ARM','DOWN_MARK','TICK','FINALIZE','OWNER_CONFLICT','WATCHDOG_SUSPENDED',
            'BLOCKING_FIRST_CALL','CRASH_AFTER_LATCH','HOST_HANG') -ccontains $_ } { }
        default { return (New-K0ValidationValue $false 'INVALID_INPUT_SCHEMA' $ref) }
    }
    if (-not (Test-K0Shape $EventInput $keys)) { return (New-K0ValidationValue $false 'INVALID_INPUT_SCHEMA' $ref) }
    foreach ($key in $keys) {
        $v = $EventInput[$key]
        if ($key -cin @('Kind','RunId','Role','Event','RefClass','Class','IntentId','Owner','HandleToken','CallId','AHandleToken','BHandleToken')) {
            if (-not (Test-K0Text $v)) { return (New-K0ValidationValue $false 'INVALID_INPUT_SCHEMA' $ref) }
        } elseif ($key -cin @('AOwned','BOwned','AIdentityValid','BIdentityValid','SenderMatch','EncodingValid','SchemaValid',
            'PartialFault','Success','ExitConfirmed','NaturalExit','SubscriptionsZero','TerminalComplete')) {
            if ($v -isnot [bool]) { return (New-K0ValidationValue $false 'INVALID_INPUT_SCHEMA' $ref) }
        } elseif ($key -cin @('ReceiveMs','ReadBeginMs','OperationBeginMs','CallbackLowMs','CallbackEstimateMs','CallbackHighMs')) {
            if ($null -ne $v -and (-not (Test-K0Time $v) -or -not (Test-K0Time $EventInput.NowMs) -or $v -gt $EventInput.NowMs)) {
                return (New-K0ValidationValue $false 'INVALID_INPUT_SCHEMA' $ref)
            }
        } elseif ($v -isnot [long] -or $v -lt 0 -or $v -gt 1000000000) {
            return (New-K0ValidationValue $false 'INVALID_INPUT_SCHEMA' $ref)
        }
    }
    if ($EventInput.RunId -cne $State.RunId -or $EventInput.AttemptId -ne 1) {
        return (New-K0ValidationValue $false 'INVALID_IDENTITY' $ref)
    }
    if (-not (Test-K0Time $EventInput.NowMs) -or $EventInput.NowMs -lt $State.NowMs) {
        return (New-K0ValidationValue $false 'INVALID_CLOCK' $ref)
    }
    if ($keys -ccontains 'Role' -and @('A','B') -cnotcontains $EventInput.Role) {
        return (New-K0ValidationValue $false 'INVALID_INPUT_SCHEMA' $ref)
    }
    if ($EventInput.Kind -ceq 'ROLES_READY') {
        foreach ($name in @('A','B')) {
            if (@('NONE',('N1_HANDLE_' + $name + '_1')) -cnotcontains $EventInput[$name + 'HandleToken']) {
                return (New-K0ValidationValue $false 'INVALID_INPUT_SCHEMA' $null)
            }
        }
    }
    if ($EventInput.Kind -cin @('COMMAND_BEGIN','COMMAND_RETURN')) {
        if ($EventInput.IntentId.Length -gt 64 -or $EventInput.CallId.Length -lt 1 -or $EventInput.CallId.Length -gt 64) {
            return (New-K0ValidationValue $false 'INVALID_INPUT_SCHEMA' $null)
        }
    }
    if ($EventInput.Kind -ceq 'PARTIAL' -and ($EventInput.BufferedBytes -lt 1 -or $EventInput.BufferedBytes -gt 4096)) {
        return (New-K0ValidationValue $false 'OVERSIZE_RECORD' $null)
    }
    if ($EventInput.Kind -ceq 'FAULT') {
        if (@('FRAME','FRAMING','IO','QUOTA','FOCUS','OWNERSHIP','GENERATION','CLOCK','UNKNOWN') -cnotcontains $EventInput.Class) {
            return (New-K0ValidationValue $false 'INVALID_INPUT_SCHEMA' $ref)
        }
        if ($EventInput.DiagnosticBytes -lt 1 -or $EventInput.DiagnosticBytes -gt 4096) {
            return (New-K0ValidationValue $false 'OVERSIZE_RECORD' $ref)
        }
    }
    if ($EventInput.Kind -ceq 'FRAME') {
        $r = $State.Roles[$EventInput.Role]
        if (@('DOWN','UP','CAPTURE','RX','SNAPSHOT') -cnotcontains $EventInput.Event -or
            @('MATCH','RADIO_MISMATCH','DIRECT','NULL_OR_UNKNOWN') -cnotcontains $EventInput.RefClass) {
            return (New-K0ValidationValue $false 'INVALID_INPUT_SCHEMA' $ref)
        }
        if ($EventInput.BindingGeneration -ne $r.BindingGeneration -or $EventInput.SubscriptionGeneration -ne $r.SubscriptionGeneration) {
            return (New-K0ValidationValue $false 'INVALID_GENERATION' $ref)
        }
        if ($EventInput.FrameSeq -lt 1 -or $EventInput.FrameSeq -gt 400 -or $EventInput.FrameSeq -ne $r.LastFrameSeq + 1) {
            return (New-K0ValidationValue $false 'INVALID_FRAME_SEQUENCE' $ref)
        }
        if ($null -eq $ref -or $null -eq $State.ObservationStart -or $ref -lt $State.ObservationStart -or
            $ref -lt $State.PhaseEnteredMs -or ($null -ne $r.LastReceiveMs -and $ref -lt $r.LastReceiveMs)) {
            return (New-K0ValidationValue $false 'INVALID_FRAME_TIME_ORDER' $ref)
        }
        if (-not $EventInput.EncodingValid) { return (New-K0ValidationValue $false 'INVALID_ENCODING' $ref) }
        if (-not $EventInput.SchemaValid) { return (New-K0ValidationValue $false 'INVALID_FRAME_SCHEMA' $ref) }
        if ($EventInput.EncodedBytes -lt 1 -or $EventInput.EncodedBytes -gt 4096 -or
            $EventInput.DiagnosticBytes -lt 1 -or $EventInput.DiagnosticBytes -gt 4096) {
            return (New-K0ValidationValue $false 'OVERSIZE_RECORD' $ref)
        }
        if ($EventInput.FieldChars -gt 256 -or $EventInput.RawArgCount -gt 16) {
            return (New-K0ValidationValue $false 'INVALID_FIELD_SIZE' $ref)
        }
        if ($EventInput.RefClass -cne 'MATCH' -or
            ($EventInput.Event -ceq 'RX' -and (-not $EventInput.SenderMatch -or $EventInput.Role -cne 'B'))) {
            return (New-K0ValidationValue $false 'INVALID_ROUTE' $ref)
        }
        if ($EventInput.Event -cin @('DOWN','UP')) {
            if ($EventInput.Role -cne 'A') { return (New-K0ValidationValue $false 'INVALID_PHASE' $ref) }
            $lo = $EventInput.CallbackLowMs; $est = $EventInput.CallbackEstimateMs; $hi = $EventInput.CallbackHighMs
            if ($null -eq $lo -or $null -eq $est -or $null -eq $hi -or $lo -gt $est -or $est -gt $hi -or $hi -gt $ref) {
                return (New-K0ValidationValue $false 'INVALID_INTERVAL' $ref)
            }
            $n = [long]($ref - $est); $left = [long]($est - $lo); $right = [long]($hi - $est)
            $mapping = $left; if ($right -gt $mapping) { $mapping = $right }
            if ($n -gt 100) { return (New-K0ValidationValue $false 'INVALID_NOTIFICATION' $ref) }
            if ($EventInput.OtherErrorMs -gt 50 -or $mapping -gt 50 -or
                $mapping -gt 50 - $EventInput.OtherErrorMs -or $ref - $lo -gt $n + $mapping) {
                return (New-K0ValidationValue $false 'INVALID_ERROR_BUDGET' $ref)
            }
            if ($EventInput.Event -ceq 'DOWN') {
                if ($State.Phase -cne 'PENDING_DOWN') { return (New-K0ValidationValue $false 'INVALID_PHASE' $ref) }
                if ($null -eq $State.DownMarkMs -or $lo -le $State.DownMarkMs -or $ref -le $State.DownMarkMs) {
                    return (New-K0ValidationValue $false 'INCONCLUSIVE_EDGE_TIME_ORDER' $ref)
                }
            } else {
                if ($State.Phase -cne 'HELD') { return (New-K0ValidationValue $false 'INVALID_PHASE' $ref) }
                if ($null -eq $State.AcceptedDownReceiveMs -or $null -eq $State.AcceptedDownCallbackHighMs -or
                    $ref -le $State.AcceptedDownReceiveMs -or $lo -le $State.AcceptedDownCallbackHighMs) {
                    return (New-K0ValidationValue $false 'INCONCLUSIVE_EDGE_TIME_ORDER' $ref)
                }
            }
        } else {
            if ($null -ne $EventInput.CallbackLowMs -or $null -ne $EventInput.CallbackEstimateMs -or
                $null -ne $EventInput.CallbackHighMs -or $EventInput.OtherErrorMs -ne 0) {
                return (New-K0ValidationValue $false 'INVALID_INPUT_SCHEMA' $ref)
            }
            if ($EventInput.Event -ceq 'SNAPSHOT') {
                if ($State.Phase -cnotin @('READINESS','ARMED','PENDING_DOWN','HELD','POST_UP')) {
                    return (New-K0ValidationValue $false 'INVALID_PHASE' $ref)
                }
                if ($null -ne $r.LastSnapshotMs -and $ref - $r.LastSnapshotMs -lt 200) {
                    return (New-K0ValidationValue $false 'INVALID_SNAPSHOT_CADENCE' $ref)
                }
            } elseif ($State.Phase -cnotin @('HELD','POST_UP') -or
                ($EventInput.Event -ceq 'CAPTURE' -and $EventInput.Role -cne 'A')) {
                return (New-K0ValidationValue $false 'INVALID_PHASE' $ref)
            }
        }
    }
    return (New-K0ValidationValue $true 'VALID' $ref)
}

function Set-K0ReferenceValue {
    param([hashtable]$State, $ReferenceMs)
    if ($null -eq $ReferenceMs) { $State.ReferenceUnknown = $true; return }
    if ($null -eq $State.EarliestReference -or $ReferenceMs -lt $State.EarliestReference) {
        $State.EarliestReference = $ReferenceMs
    }
}

function Set-K0ReasonValue {
    param([hashtable]$State, [string]$Reason, [long]$Rank, $ReferenceMs, [string]$Category)
    Set-K0ReferenceValue $State $ReferenceMs
    if ($Rank -lt $State.PrimaryRank) { $State.PrimaryReason = $Reason; $State.PrimaryRank = $Rank }
    if ($Category -cne 'NORMAL') {
        $State.Invalid = $true
        switch -CaseSensitive ($Category) {
            'PROTOCOL' { $State.ProtocolFault = $true }
            'SAFETY' { $State.SafetyFault = $true }
            'INTEGRITY' { $State.IntegrityFault = $true }
        }
    }
}

function Add-K0BudgetValue {
    param([hashtable]$State, [string]$Role, [string]$Channel, [long]$Bytes)
    $r = $State.Roles[$Role]
    $key = ''; $cap = [long]0
    switch -CaseSensitive ($Channel) {
        'SNAPSHOT' { $key = 'SnapshotCount'; $cap = [long]240 }
        'RAW' { $key = 'RawCount'; $cap = [long]128 }
        'SAFETY' { $key = 'SafetyCount'; $cap = [long]4 }
        'DIAGNOSTIC' { $key = 'DiagnosticCount'; $cap = [long]16 }
        'CLEANUP' { $key = 'CleanupCount'; $cap = [long]12 }
        default { throw 'N1_INTERNAL_CHANNEL' }
    }
    if ($Bytes -lt 1 -or $Bytes -gt 4096 -or $r[$key] -ge $cap -or $r.BudgetBytes -gt 1638400 - $Bytes) {
        if ($r.RejectedCount -lt 400) { $r.RejectedCount = [long]($r.RejectedCount + 1) }
        $State.IntegrityFault = $true; $State.Invalid = $true
        return $false
    }
    $r[$key] = [long]($r[$key] + 1); $r.BudgetBytes = [long]($r.BudgetBytes + $Bytes)
    return $true
}

function Enter-K0TeardownValue {
    param([hashtable]$State, [hashtable]$Output)
    if ($State.Phase -cin @('TEARDOWN','TERMINAL')) { return }
    if ($State.NowMs -gt 999990000) { throw 'N1_INTERNAL_TIME_BOUND' }
    $State.Phase = 'TEARDOWN'; $State.PhaseEnteredMs = $State.NowMs
    $State.CleanupDue = [long]($State.NowMs + 10000)
    $State.AbortTransition = $State.Invalid
    # Latch both before constructing either output DTO; no actual call exists.
    foreach ($name in @('A','B')) {
        $r = $State.Roles[$name]
        if ($r.Owned -and $r.IdentityValid -and $r.HandleToken -ceq ('N1_HANDLE_' + $name + '_1') -and
            -not $r.NaturalExit -and -not $r.ExitConfirmed -and -not $r.IntentLatched) {
            $r.IntentLatched = $true; $r.IntentId = $State.RunId + '/1/' + $name + '/1'
        }
    }
    foreach ($name in @('A','B')) {
        $r = $State.Roles[$name]
        if ($r.IntentLatched -and -not $r.BeginObserved) {
            $Output['Intent' + $name] = @{
                IntentId = $r.IntentId; RunId = $State.RunId; AttemptId = [long]1
                Role = $name; Ordinal = [long]1; Owner = 'N1_OWNER_1'; HandleToken = $r.HandleToken
            }
        }
    }
}

function Get-K0CleanupValue {
    param([hashtable]$State)
    if ($State.IntegrityFault) { return $false }
    foreach ($name in @('A','B')) {
        $r = $State.Roles[$name]
        if (-not $r.ExitConfirmed -or -not $r.SubscriptionsZero -or -not $r.TerminalComplete) { return $false }
    }
    return $true
}

function Update-K0Model {
    param($State, [Alias('Input')]$EventInput)
    $validation = Test-K0ModelInput -State $State -Input $EventInput
    if (Test-K0StateValue $State) { $s = Copy-K0StateValue $State } else {
        $s = New-K0ModelState -RunId '00000000000000000000000000000000' -StartMs ([long]0)
    }
    $previous = $s.Phase
    $o = @{ PreviousPhase = $previous; Phase = $previous; PrimaryReason = $s.PrimaryReason
        ReferenceMs = $s.EarliestReference; Disposition = 'NONE'; RawComplete = $false
        IntentA = $null; IntentB = $null; ModelInvalid = $s.Invalid; ValidationResult = $validation.Reason }
    if ($previous -ceq 'TERMINAL') {
        Set-K0ReasonValue $s 'TERMINAL_EXTRA_INPUT' ([long]2) $null 'INTEGRITY'
        $o.Disposition = 'REJECTED_TERMINAL'
    } else {
        # Bad Now never advances the model. Due set is sampled before any transition.
        if ($EventInput -is [hashtable] -and $EventInput.Count -le 24 -and
            (Test-K0Time $EventInput.NowMs) -and $EventInput.NowMs -ge $s.NowMs) { $s.NowMs = $EventInput.NowMs }
        $due = $false; $normalDue = $false
        if ($previous -cnotin @('TEARDOWN','TERMINAL')) {
            if ($null -ne $s.AbsoluteDue -and $s.NowMs -ge $s.AbsoluteDue) {
                Set-K0ReasonValue $s 'ABSOLUTE_DUE' ([long]1) $s.AbsoluteDue 'SAFETY'; $due = $true
            }
            $dueKeys = @(); $dueNames = @()
            switch -CaseSensitive ($previous) {
                'STARTUP' { $dueKeys = @('StartupDue'); $dueNames = @('STARTUP_DUE') }
                'ARMED' { $dueKeys = @('ArmDue'); $dueNames = @('ARM_DUE') }
                'PENDING_DOWN' { $dueKeys = @('PendingDue','FallbackDue'); $dueNames = @('PENDING_DUE','FALLBACK_DUE') }
                'HELD' { $dueKeys = @('HeldDue'); $dueNames = @('HELD_DUE') }
                'POST_UP' { $dueKeys = @('PostUpDue'); $dueNames = @('NORMAL_POST_UP') }
            }
            for ($i = 0; $i -lt $dueKeys.Count; $i++) {
                $mark = $s[$dueKeys[$i]]
                if ($null -ne $mark -and $s.NowMs -ge $mark) {
                    $due = $true
                    if ($previous -ceq 'POST_UP') { $normalDue = $true; Set-K0ReasonValue $s $dueNames[$i] ([long]4) $mark 'NORMAL' }
                    else { Set-K0ReasonValue $s $dueNames[$i] ([long]3) $mark 'SAFETY' }
                }
            }
        }
        if (-not $validation.Valid) {
            Set-K0ReasonValue $s $validation.Reason ([long]2) $validation.ReferenceMs 'PROTOCOL'
            $o.Disposition = 'REJECTED'
            if ($validation.Reason -cin @('OVERSIZE_RECORD','INVALID_ENCODING','INVALID_FRAME_SCHEMA','INVALID_FIELD_SIZE','INVALID_INPUT_SCHEMA','INVALID_STATE_SCHEMA')) {
                $s.IntegrityFault = $true
            }
        }
        $kind = ''; $roleName = 'A'; $diagBytes = [long]64
        if ($EventInput -is [hashtable] -and $EventInput.Count -le 24) {
            if ($EventInput.Kind -is [string]) { $kind = $EventInput.Kind }
            if ($EventInput.Role -is [string] -and @('A','B') -ccontains $EventInput.Role) { $roleName = $EventInput.Role }
            if ($EventInput.DiagnosticBytes -is [long]) { $diagBytes = $EventInput.DiagnosticBytes }
        }
        # Incoming fault reference can be earlier than an already-expired due.
        if ($validation.Valid -and $kind -ceq 'FAULT') {
            $r = $s.Roles[$roleName]; $ref = $null
            if ($EventInput.PartialFault) {
                $r.PartialFault = $true; $ref = $r.PartialMs; $reason = 'PARTIAL_FAULT'
            } else {
                $reason = 'FAULT_' + $EventInput.Class
                switch -CaseSensitive ($EventInput.Class) {
                    'FRAME' { $ref = $EventInput.ReceiveMs }
                    'FRAMING' {
                        $ref = $EventInput.OperationBeginMs
                        if ($null -ne $r.PartialMs -and ($null -eq $ref -or $r.PartialMs -lt $ref)) { $ref = $r.PartialMs }
                    }
                    { @('IO','QUOTA') -ccontains $_ } {
                        $ref = $EventInput.OperationBeginMs
                        if ($null -ne $ref -and $null -ne $EventInput.ReceiveMs -and $EventInput.ReceiveMs -lt $ref) { $ref = $EventInput.ReceiveMs }
                    }
                    { @('FOCUS','OWNERSHIP','GENERATION','CLOCK') -ccontains $_ } { $ref = $EventInput.OperationBeginMs }
                    'UNKNOWN' { $ref = $null; $reason = 'UNKNOWN_REFERENCE' }
                }
            }
            Set-K0ReasonValue $s $reason ([long]2) $ref 'SAFETY'
        }
        if ($validation.Valid -and $kind -cin @('OWNER_CONFLICT','WATCHDOG_SUSPENDED','BLOCKING_FIRST_CALL','CRASH_AFTER_LATCH','HOST_HANG')) {
            Set-K0ReasonValue $s $kind ([long]2) $null 'SAFETY'
        }
        if ($previous -ceq 'TEARDOWN') {
            if ($kind -cin @('FRAME','FAULT')) {
                $ok = Add-K0BudgetValue $s $roleName 'DIAGNOSTIC' $diagBytes
                $o.Disposition = 'DIAGNOSTIC_ONLY'; $o.RawComplete = ($ok -and $validation.Valid)
            } elseif ($validation.Valid) {
                $r = $s.Roles[$roleName]
                switch -CaseSensitive ($kind) {
                    'COMMAND_BEGIN' {
                        if (-not $r.IntentLatched -or $r.BeginObserved -or $EventInput.IntentId -cne $r.IntentId -or
                            $EventInput.Owner -cne 'N1_OWNER_1' -or $EventInput.HandleToken -cne $r.HandleToken -or
                            ($null -ne $s.EarliestReference -and $s.NowMs -lt $s.EarliestReference)) {
                            Set-K0ReasonValue $s 'INVALID_COMMAND' ([long]2) $null 'PROTOCOL'
                        } else { $r.BeginObserved = $true; $r.BeginMs = $s.NowMs; $r.CallId = $EventInput.CallId }
                    }
                    'COMMAND_RETURN' {
                        if (-not $r.BeginObserved -or $r.ReturnObserved -or $EventInput.IntentId -cne $r.IntentId -or
                            $EventInput.CallId -cne $r.CallId -or $s.NowMs -lt $r.BeginMs) {
                            Set-K0ReasonValue $s 'INVALID_COMMAND' ([long]2) $null 'PROTOCOL'
                        } else { $r.ReturnObserved = $true; $r.ReturnMs = $s.NowMs; $r.ReturnSuccess = $EventInput.Success }
                    }
                    'ROLE_EXIT' {
                        $ok = Add-K0BudgetValue $s $roleName 'CLEANUP' ([long]64)
                        if ($EventInput.HandleToken -cne $r.HandleToken -or $r.HandleToken -ceq 'NONE') {
                            Set-K0ReasonValue $s 'BLOCKED_OWNERSHIP' ([long]2) $null 'SAFETY'
                        } else { $r.ExitConfirmed = ($r.ExitConfirmed -or $EventInput.ExitConfirmed); $r.NaturalExit = ($r.NaturalExit -or $EventInput.NaturalExit) }
                    }
                    'DETACH' {
                        $ok = Add-K0BudgetValue $s $roleName 'CLEANUP' ([long]64)
                        $r.SubscriptionsZero = ($r.SubscriptionsZero -or $EventInput.SubscriptionsZero)
                        $r.TerminalComplete = ($r.TerminalComplete -or $EventInput.TerminalComplete)
                    }
                    'FINALIZE' {
                        foreach ($name in @('A','B')) { $ok = Add-K0BudgetValue $s $name 'CLEANUP' ([long]64) }
                        $s.Phase = 'TERMINAL'; $s.PhaseEnteredMs = $s.NowMs
                    }
                    'TICK' { }
                    { @('OWNER_CONFLICT','WATCHDOG_SUSPENDED','BLOCKING_FIRST_CALL','CRASH_AFTER_LATCH','HOST_HANG') -ccontains $_ } { }
                    default { Set-K0ReasonValue $s 'INVALID_PHASE' ([long]2) $null 'PROTOCOL' }
                }
            }
            if ($s.Phase -ceq 'TEARDOWN' -and ((Get-K0CleanupValue $s) -or $s.NowMs -ge $s.CleanupDue)) {
                $s.Phase = 'TERMINAL'; $s.PhaseEnteredMs = $s.NowMs
            }
        } elseif ($validation.Valid -and -not $due -and -not $s.Invalid) {
            $r = $s.Roles[$roleName]
            switch -CaseSensitive ($kind) {
                'ROLES_READY' {
                    if ($previous -cne 'STARTUP') { Set-K0ReasonValue $s 'INVALID_PHASE' ([long]2) $null 'PROTOCOL' }
                    else {
                        foreach ($name in @('A','B')) {
                            $rr = $s.Roles[$name]; $rr.Owned = $EventInput[$name + 'Owned']; $rr.IdentityValid = $EventInput[$name + 'IdentityValid']
                            $rr.HandleToken = $EventInput[$name + 'HandleToken']
                            if (-not $rr.Owned -or -not $rr.IdentityValid -or $rr.HandleToken -ceq 'NONE') {
                                Set-K0ReasonValue $s 'BLOCKED_OWNERSHIP' ([long]2) $null 'SAFETY'
                            }
                        }
                        if (-not $s.Invalid) {
                            $s.Phase = 'READINESS'; $s.ObservationStart = $s.NowMs
                            $s.AbsoluteDue = [long]($s.NowMs + 45000)
                        }
                    }
                }
                'ARM' {
                    if ($previous -cne 'READINESS' -or $s.NowMs - $s.ObservationStart -lt 3000) {
                        Set-K0ReasonValue $s 'INVALID_PHASE' ([long]2) $null 'PROTOCOL'
                    } else { $s.Phase = 'ARMED'; $s.ArmDue = [long]($s.NowMs + 5000) }
                }
                'DOWN_MARK' {
                    if ($previous -cne 'ARMED') { Set-K0ReasonValue $s 'INVALID_PHASE' ([long]2) $null 'PROTOCOL' }
                    else { $s.Phase = 'PENDING_DOWN'; $s.DownMarkMs = $s.NowMs; $s.PendingDue = [long]($s.NowMs + 1000); $s.FallbackDue = [long]($s.NowMs + 1600) }
                }
                'FRAME' {
                    $channel = 'RAW'; if ($EventInput.Event -ceq 'SNAPSHOT') { $channel = 'SNAPSHOT' }
                    $counter = $r.RawCount; $cap = 128
                    if ($channel -ceq 'SNAPSHOT') { $counter = $r.SnapshotCount; $cap = 240 }
                    if ($counter -ge $cap) {
                        Set-K0ReasonValue $s 'INVALID_QUOTA' ([long]2) $EventInput.ReceiveMs 'SAFETY'
                        $ok = Add-K0BudgetValue $s $roleName 'SAFETY' $EventInput.EncodedBytes
                        $o.Disposition = 'RESERVED_OVERFLOW'; $o.RawComplete = $ok
                    } else {
                        $ok = Add-K0BudgetValue $s $roleName $channel $EventInput.EncodedBytes
                        if ($ok) {
                            $r.LastFrameSeq = $EventInput.FrameSeq; $r.LastReceiveMs = $EventInput.ReceiveMs
                            if (-not $r.PartialFault) { $r.PartialMs = $null }
                            $o.Disposition = 'ACCEPT'; $o.RawComplete = $true
                            switch -CaseSensitive ($EventInput.Event) {
                                'DOWN' {
                                    # Replacement is one copied-state update, never a gap in the returned state.
                                    $s.HeldDue = [long]($EventInput.ReceiveMs + 1500)
                                    $s.PendingDue = $null; $s.FallbackDue = $null
                                    $s.AcceptedDownReceiveMs = $EventInput.ReceiveMs; $s.AcceptedDownCallbackHighMs = $EventInput.CallbackHighMs
                                    $s.Phase = 'HELD'
                                }
                                'UP' { $s.PostUpDue = [long]($EventInput.ReceiveMs + 3000); $s.HeldDue = $null; $s.Phase = 'POST_UP' }
                                'CAPTURE' { $s.ModelCaptureWindow = $true }
                                'RX' { $s.ModelRxWindow = $true }
                                'SNAPSHOT' { $r.LastSnapshotMs = $EventInput.ReceiveMs }
                            }
                        }
                    }
                }
                'PARTIAL' {
                    if ($null -eq $EventInput.ReadBeginMs) { Set-K0ReasonValue $s 'UNKNOWN_REFERENCE' ([long]2) $null 'PROTOCOL' }
                    elseif ($null -eq $r.PartialMs -or $EventInput.ReadBeginMs -lt $r.PartialMs) { $r.PartialMs = $EventInput.ReadBeginMs }
                }
                'ROLE_EXIT' {
                    $ok = Add-K0BudgetValue $s $roleName 'CLEANUP' ([long]64)
                    if ($EventInput.HandleToken -cne $r.HandleToken -or -not $EventInput.ExitConfirmed -or -not $EventInput.NaturalExit) {
                        Set-K0ReasonValue $s 'BLOCKED_OWNERSHIP' ([long]2) $null 'SAFETY'
                    } else { $r.ExitConfirmed = $true; $r.NaturalExit = $true }
                }
                'TICK' { }
                default { Set-K0ReasonValue $s 'INVALID_PHASE' ([long]2) $null 'PROTOCOL' }
            }
            if ($s.Phase -cne $previous) { $s.PhaseEnteredMs = $s.NowMs }
        }
        if ($previous -cne 'TEARDOWN' -and ($due -or $s.Invalid)) {
            # Safety diagnostic quota never gates intent generation.
            if (-not $normalDue -or $s.Invalid) {
                if ($o.Disposition -cne 'RESERVED_OVERFLOW') {
                    $ok = Add-K0BudgetValue $s $roleName 'SAFETY' $diagBytes
                    if ($o.Disposition -ceq 'NONE') { $o.Disposition = 'SAFETY_DIAGNOSTIC' }
                }
            }
            Enter-K0TeardownValue $s $o
        }
        if ($s.IntegrityFault -and $s.PrimaryReason -ceq 'NONE') {
            Set-K0ReasonValue $s 'INCONCLUSIVE_LOG_INTEGRITY' ([long]2) $null 'INTEGRITY'
        }
    }
    $o.Phase = $s.Phase; $o.PrimaryReason = $s.PrimaryReason; $o.ReferenceMs = $s.EarliestReference
    $o.ModelInvalid = $s.Invalid
    return @{ State = $s; Output = $o }
}

function Get-K0ModelVerdict {
    param($State)
    $validState = Test-K0StateValue $State
    $policy = 'MODEL_INVALID'; $dispatch = 'UNKNOWN'; $cleanup = 'INCONCLUSIVE'
    if ($validState) {
        if (-not $State.Invalid -and -not $State.ProtocolFault -and -not $State.SafetyFault -and -not $State.IntegrityFault) { $policy = 'MODEL_TRACE_CONSISTENT' }
        if (Get-K0CleanupValue $State) { $cleanup = 'MODEL_COMPLETE' }
        $a = $State.Roles.A; $b = $State.Roles.B
        if (-not $State.ReferenceUnknown -and $null -ne $State.EarliestReference) {
            if (-not $a.IntentLatched -and -not $b.IntentLatched) { $dispatch = 'NOT_OBSERVED' }
            elseif ($a.IntentLatched -and $b.IntentLatched) {
                if ($a.ReturnObserved -and $b.ReturnObserved -and $a.ReturnSuccess -and $b.ReturnSuccess) {
                    $last = $a.ReturnMs; if ($b.ReturnMs -gt $last) { $last = $b.ReturnMs }
                    if ($a.BeginObserved -and $b.BeginObserved -and $a.BeginMs -ge $State.EarliestReference -and
                        $b.BeginMs -ge $State.EarliestReference -and $last - $State.EarliestReference -le 150) { $dispatch = 'MODEL_WITHIN_THRESHOLD' }
                    else { $dispatch = 'MODEL_FAILED' }
                } elseif ($a.BeginObserved -or $b.BeginObserved) { $dispatch = 'MODEL_FAILED' }
            }
        } elseif (-not $State.ReferenceUnknown -and -not $a.IntentLatched -and -not $b.IntentLatched) { $dispatch = 'NOT_OBSERVED' }
    }
    return @{ Scope = 'N1_VALUE_MODEL'; PolicyResult = $policy; DispatchObservation = $dispatch
        CleanupObservation = $cleanup; NativeRelease = 'UNKNOWN'
        PhysicalSafety = 'NOT_ESTABLISHED'; PhysicalCommandOnce = 'NOT_ESTABLISHED' }
}
