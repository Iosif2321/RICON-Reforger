param(
    [Parameter(Mandatory = $true)][string]$SourcePath
)
$ErrorActionPreference = 'Stop'
$taskSource = Get-Content -Raw -Encoding utf8 -LiteralPath $SourcePath
$taskHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $SourcePath).Hash.ToLowerInvariant()
if ($taskSource -notmatch 'class RICON_TemporalClock' -or $taskSource -match '\b(float|vector|int64)\b[^\r\n]*;') {
    throw 'Unexpected source shape for the isolated integer clock helper'
}
# Syntax-only mapping: the bodies being checked come from the actual Enforce source.
$taskMapped = $taskSource.Replace('class RICON_TemporalClock', 'public static class RICON_TemporalClock')
$taskMapped = $taskMapped.Replace('static int MapInterval(', 'public static int MapInterval(')
$taskMapped = $taskMapped.Replace('static bool ContainsInterval(', 'public static bool ContainsInterval(')
$taskMapped = $taskMapped.Replace('int.MAX', 'int.MaxValue')
$taskHarness = @'
public static class Differential
{
    static int mapCases, intervalCases;
    static long Limit = (long)int.MaxValue * 1000 + 999;
    static void Map(int now, int sub, int local, int server, int from, int until, int error)
    {
        int em, es, lm, ls, um, us;
        int got = RICON_TemporalClock.MapInterval(now, sub, local, server, from, until, error,
            out em, out es, out lm, out ls, out um, out us);
        // Independent exact formula: long intermediates stay below 2^43 for int32 inputs.
        int want = 0;
        long estimate = ((long)server + now - local) * 1000 + sub;
        long lower = estimate - error, upper = estimate + error;
        if (now < 0 || sub < 0 || sub > 999 || local < 0 || server < 0 || from < 0 || until < 0 || error < 0
            || from > local || local >= until || now < from || now >= until)
            want = 1;
        else if (lower < 0 || upper > Limit)
            want = 2;
        if (got != want || (want == 0 &&
            ((long)em * 1000 + es != estimate || (long)lm * 1000 + ls != lower || (long)um * 1000 + us != upper)))
            throw new System.InvalidOperationException("MAP semantic mismatch: " +
                string.Join(",", new int[] {now, sub, local, server, from, until, error, got, want}));
        if (got == 0 && (em < 0 || lm < 0 || um < 0 || es < 0 || es > 999 || ls < 0 || ls > 999 || us < 0 || us > 999))
            throw new System.InvalidOperationException("MAP output domain mismatch");
        mapCases++;
    }
    static void Contains(int lowerMs, int lowerSub, int upperMs, int upperSub, int from, int until)
    {
        bool got = RICON_TemporalClock.ContainsInterval(lowerMs, lowerSub, upperMs, upperSub, from, until);
        long lower = (long)lowerMs * 1000 + lowerSub, upper = (long)upperMs * 1000 + upperSub;
        bool want = lowerMs >= 0 && upperMs >= 0 && lowerSub >= 0 && lowerSub <= 999 &&
            upperSub >= 0 && upperSub <= 999 && from >= 0 && until > from &&
            lower <= upper && lower >= (long)from * 1000 && upper < (long)until * 1000;
        if (got != want)
            throw new System.InvalidOperationException("CONTAINS semantic mismatch");
        intervalCases++;
    }
    static int Next(System.Random random) { return random.Next(int.MaxValue); }
    public static int[] Run()
    {
        mapCases = intervalCases = 0;
        int[] edges = {-1, 0, 1, 99, 100, 999, 1000, 1199, 1200, int.MaxValue-1, int.MaxValue};
        int[] micros = {-1, 0, 1, 998, 999, 1000};
        int[] errors = {-1, 0, 1, 999, 1000, 1001, int.MaxValue};
        foreach (int now in edges)
            foreach (int local in edges)
                foreach (int server in edges)
                    foreach (int sub in micros)
                        foreach (int error in errors)
                            Map(now, sub, local, server, 0, int.MaxValue, error);
        // Explicit high-domain and carry/borrow cases.
        Map(int.MaxValue-1, 999, int.MaxValue-1, int.MaxValue, 0, int.MaxValue, 0);
        Map(int.MaxValue-1, 999, int.MaxValue-1, int.MaxValue, 0, int.MaxValue, 1);
        Map(100, 0, 100, 0, 0, 1000, 1);
        Map(100, 999, 100, 0, 0, 1000, 999);
        Map(1199, 999, 100, 100, 100, 2000, 1);
        foreach (int lo in edges)
            foreach (int hi in edges)
                foreach (int ls in micros)
                    foreach (int us in micros)
                        Contains(lo, ls, hi, us, 100, 1200);
        Contains(100, 0, 1199, 999, 100, 1200);
        Contains(100, 0, 1200, 0, 100, 1200);
        Contains(int.MaxValue-1, 0, int.MaxValue-1, 999, int.MaxValue-1, int.MaxValue);
        System.Random random = new System.Random(1031010);
        for (int i = 0; i < 30000; i++)
        {
            int now = Next(random), local = Next(random), server = Next(random), error = Next(random);
            Map(now, random.Next(1000), local, server, 0, int.MaxValue, error);
            int lo = Next(random), hi = Next(random), from = Next(random), until = Next(random);
            Contains(lo, random.Next(1000), hi, random.Next(1000), from, until);
        }
        return new int[] {mapCases, intervalCases};
    }
}
'@
$taskActualCode = "namespace Ct10Actual { " + $taskMapped + $taskHarness + " }"
Add-Type -TypeDefinition $taskActualCode -CompilerOptions '/checked+' -ErrorAction Stop
$taskCounts = [Ct10Actual.Differential]::Run()
# Two deliberate semantic defects must be rejected by the same independent formula.
$taskMutations = @(
    @('UpperInclusive', 'upperMs < validUntilMs', 'upperMs <= validUntilMs'),
    @('BorrowLost', 'subtractMs += 1;', 'subtractMs += 0;')
)
$taskDetected = @()
foreach ($taskMutation in $taskMutations) {
    if (-not $taskMapped.Contains($taskMutation[1])) { throw 'Mutation source marker missing' }
    $taskBroken = $taskMapped.Replace($taskMutation[1], $taskMutation[2])
    $taskNamespace = 'Ct10Broken' + $taskMutation[0]
    Add-Type -TypeDefinition ("namespace " + $taskNamespace + " { " + $taskBroken + $taskHarness + " }") -CompilerOptions '/checked+' -ErrorAction Stop
    $taskRejected = $false
    try {
        $taskType = ($taskNamespace + '.Differential') -as [type]
        $null = $taskType.GetMethod('Run').Invoke($null, @())
    } catch {
        $taskInner = $_.Exception
        while ($taskInner.InnerException) { $taskInner = $taskInner.InnerException }
        if ($taskInner -is [System.InvalidOperationException] -and $taskInner.Message -match 'semantic mismatch') {
            $taskRejected = $true
        } else { throw }
    }
    if (-not $taskRejected) { throw 'Broken arithmetic escaped independent formula' }
    $taskDetected += $taskMutation[0]
}
[pscustomobject]@{
    status = 'PASS'
    scope = 'SOURCE_EXTRACTED_CHECKED_INT32_ALGORITHM_ONLY'
    sourceSha256 = $taskHash
    mappingCases = $taskCounts[0]
    containmentCases = $taskCounts[1]
    negativeControlsDetected = $taskDetected
    enforceCompiled = $false
    engineStarted = $false
} | ConvertTo-Json -Depth 4
