param([Parameter(Mandatory)][string]$OutputDirectory,
      [string]$PluginVersion = '0.6.3',
      [uri]$ReleaseFeed = 'https://github.com/Iosif2321/RICON-Reforger/releases/download/',
      [string]$SdkIncludeDirectory = $env:RICON_TS3_SDK_INCLUDE)
$ErrorActionPreference = 'Stop'
if ($PluginVersion -cnotmatch '\A(0|[1-9][0-9]{0,4})\.(0|[1-9][0-9]{0,4})\.(0|[1-9][0-9]{0,4})\z' -or
    ($PluginVersion.Split('.') | Where-Object { [int]$_ -gt 65535 })) { throw 'Invalid plugin version' }
if (!$ReleaseFeed.IsAbsoluteUri -or $ReleaseFeed.Scheme -ne 'https' -or $ReleaseFeed.UserInfo -or $ReleaseFeed.Query -or $ReleaseFeed.Fragment) { throw 'Expected a fixed HTTPS release directory.' }
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../../..')).Path
if (!$SdkIncludeDirectory) { $SdkIncludeDirectory = Join-Path $repo 'external/ts3client-pluginsdk/include' }
$sdk = (Resolve-Path -LiteralPath $SdkIncludeDirectory).Path
if (!(Test-Path -LiteralPath (Join-Path $sdk 'ts3_functions.h') -PathType Leaf)) { throw 'Set -SdkIncludeDirectory to the official TS3 Plugin SDK include directory.' }
$destination = [IO.Path]::GetFullPath($OutputDirectory)
if (-not $destination.StartsWith($repo + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Build output must remain in the workspace' }
if (Test-Path -LiteralPath $destination) { throw 'Use a fresh build output directory' }
New-Item -ItemType Directory -Path $destination | Out-Null
$arma3 = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../dsp')).Path
$inputs = @('tsplugin/src/audio_chain.c', 'tsplugin/src/audio_chain.h', 'common/ricon_protocol.h', 'tsplugin/tests/audio_chain_smoke.c', 'LICENSE', 'NOTICE.md')
$pins = @()
foreach ($relative in $inputs) {
    $original = Join-Path $arma3 $relative
    $copy = Join-Path $destination ('arma3-source/' + $relative)
    New-Item -ItemType Directory -Path (Split-Path -Parent $copy) -Force | Out-Null
    Copy-Item -LiteralPath $original -Destination $copy
    $hash = (Get-FileHash -LiteralPath $original).Hash
    if ((Get-FileHash -LiteralPath $copy).Hash -ne $hash) { throw 'Arma3 source changed while copying' }
    $pins += @{ path = $original; copiedPath = $copy; sha256 = $hash }
}
$pins | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $destination 'arma3-source-pins.json') -Encoding UTF8
$dsp = Join-Path $destination 'arma3-source/tsplugin/src'
$dspObject = Join-Path $destination 'audio_chain.obj'
$liveSource = Join-Path $dsp 'audio_chain_live150.c'
$liveObject = Join-Path $destination 'audio_chain_live150.obj'
$originalText = [IO.File]::ReadAllText((Join-Path $dsp 'audio_chain.c'))
$oldCutoff = 'ricon_biquad_make(&g_hp750, 750.0f, 0.7f, 1);'
$newCutoff = 'ricon_biquad_make(&g_hp750, 150.0f, 0.7f, 1);'
if ([regex]::Matches($originalText, [regex]::Escape($oldCutoff)).Count -ne 1) { throw 'Expected exactly one HP750 constructor' }
$liveText = $originalText.Replace($oldCutoff, $newCutoff)
$newLine = if ($originalText.Contains("`r`n")) { "`r`n" } else { "`n" }
$oldMix = '            s += ricon_white(state) * noise_amp;'
$newMix = @'
            const float passed = s;
            const float raw_rf = ricon_white(state) * noise_amp;
            const float band_rf = noise_lp_state != NULL
                ? ricon_biquad_process(&g_lp4000, noise_lp_state, ch, raw_rf) : raw_rf;
            s += band_rf;
'@
$oldOutput = '            samples[i] = (short)s;'
$newOutput = @'
            samples[i] = (short)s;
            if (noise_metrics != NULL) {
                const double source = (double)passed;
                const double rf = (double)band_rf;
                const double mix = source + rf;
                const double output = (double)samples[i];
                const double residual = output - mix;
                noise_metrics->values++;
                noise_metrics->sourceSquares += source * source;
                noise_metrics->rawRfSquares += (double)raw_rf * raw_rf;
                noise_metrics->bandRfSquares += rf * rf;
                noise_metrics->sourceRfCross += 2.0 * source * rf;
                noise_metrics->mixedSquares += mix * mix;
                noise_metrics->outputSquares += output * output;
                noise_metrics->residualSquares += residual * residual;
            }
'@
$transforms = @(
    @{old='#include "audio_chain.h"'; new=('#include "audio_chain.h"' + $newLine + '#include "live_audio_profile.h"')},
    @{old='int channels) {'; new='int channels, RiconAudioBiquadState *noise_lp_state, RiconLiveNoiseMetrics *noise_metrics) {'},
    @{old=$oldMix; new=([regex]::Replace($newMix, '\r?\n', $newLine))},
    @{old=$oldOutput; new=([regex]::Replace($newOutput, '\r?\n', $newLine))}
)
foreach ($transform in $transforms) {
    if ([regex]::Matches($liveText, [regex]::Escape($transform.old)).Count -ne 1) { throw 'Expected exactly one declared LIVE transform site' }
    $liveText = $liveText.Replace($transform.old, $transform.new)
}
$symbols = @('ricon_audio_init','ricon_audio_state_init','ricon_audio_metrics_reset','ricon_audio_metrics_get','ricon_audio_process_playback_voice')
$symbolMap = @()
foreach ($symbol in $symbols) {
    $renamed = $symbol.Replace('ricon_audio_', 'ricon_live150_audio_')
    $liveText = $liveText.Replace($symbol, $renamed)
    if ([regex]::Matches($liveText, ('(?m)^(?:void|RiconAudioMetrics)\s+' + $renamed + '\s*\(')).Count -ne 1) { throw "Derived public definition mismatch: $renamed" }
    $symbolMap += @{ original=$symbol; derived=$renamed }
}
$restored = $liveText.Replace($newCutoff, $oldCutoff)
foreach ($mapping in $symbolMap) { $restored = $restored.Replace($mapping.derived, $mapping.original) }
foreach ($transform in $transforms) { $restored = $restored.Replace($transform.new, $transform.old) }
if ($restored -cne $originalText) { throw 'Derived source contains an undeclared change' }
[IO.File]::WriteAllText($liveSource, $liveText, [Text.UTF8Encoding]::new($false))
@{profile='TEST_ONLY_LIVE_HP150_LP4000_NOISE_LP4000_V2'; originalSha256=(Get-FileHash -LiteralPath (Join-Path $dsp 'audio_chain.c')).Hash; derivedSha256=(Get-FileHash -LiteralPath $liveSource).Hash; oldConstructor=$oldCutoff; newConstructor=$newCutoff; publicSymbols=$symbolMap; declaredTransforms=$transforms; exactOriginalReconstruction=$true; rfLogicChanged=$false; rawNoiseGeneratorChanged=$false; noiseAudioBandHz=4000; diagnosticPasses=1} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $destination 'live-audio-profile.json') -Encoding UTF8
$hostDirectory = Join-Path $destination 'host-source'
New-Item -ItemType Directory -Path $hostDirectory | Out-Null
$hostPins = @()
foreach ($name in @('pcm_probe.cpp','pcm_probe_selftest.cpp','network_probe.inl','game_live.inl','whisper_probe.inl','ts3_tab_probe.inl',
                    'ts3_tab_actual.inl','ts3_binding_route.inl','ts3_route_voice.inl','ricon_status.inl','build.ps1','live_audio_profile.h')) {
    $original = Join-Path $PSScriptRoot $name
    $copy = Join-Path $hostDirectory $name
    Copy-Item -LiteralPath $original -Destination $copy
    $hash = (Get-FileHash -LiteralPath $original).Hash
    if ((Get-FileHash -LiteralPath $copy).Hash -ne $hash) { throw 'Host source changed while copying' }
    $hostPins += @{ path = $original; copiedPath = $copy; sha256 = $hash }
}
$hostPins | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $destination 'host-source-pins.json') -Encoding UTF8
$updaterSrc = Join-Path $PSScriptRoot '../updater'
$publicVerifier = [Security.Cryptography.RSA]::Create()
try {
    $publicVerifier.FromXmlString([IO.File]::ReadAllText((Resolve-Path -LiteralPath (Join-Path $updaterSrc 'alpha-public.xml')).Path))
    $publicKey = $publicVerifier.ToXmlString($false)
} finally { $publicVerifier.Dispose() }
$feed = $ReleaseFeed.AbsoluteUri.TrimEnd('/') + '/'
$helperSource = Join-Path $hostDirectory 'RiconUpdate.cs'
Copy-Item -LiteralPath (Join-Path $updaterSrc 'RiconUpdate.cs') -Destination $helperSource
$helperConfig = Join-Path $hostDirectory 'RiconUpdate.Config.cs'
[IO.File]::WriteAllText($helperConfig,('static class RiconReleaseConfig { public const string Feed = "' + $feed + '"; public const string PublicKey = "' + $publicKey + '"; }'),[Text.Encoding]::ASCII)
$helper = Join-Path $destination 'RiconUpdate.exe'
$csc = "$env:WINDIR/Microsoft.NET/Framework64/v4.0.30319/csc.exe"
& $csc /nologo /warnaserror /optimize+ /target:winexe /platform:x64 /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll "/out:$helper" $helperSource $helperConfig
if ($LASTEXITCODE) { throw 'Updater compiler failed.' }
$recoverySource = Join-Path $hostDirectory 'RiconUpdateRecoveryTests.cs'
Copy-Item -LiteralPath (Join-Path $updaterSrc 'RiconUpdateRecoveryTests.cs') -Destination $recoverySource
$recoveryTest = Join-Path $destination 'recovery-test.exe'
& $csc /nologo /warnaserror /optimize+ /target:exe /platform:x64 /main:RiconUpdateRecoveryTests /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll "/out:$recoveryTest" $helperSource $helperConfig $recoverySource
if ($LASTEXITCODE) { throw 'Recovery test compiler failed.' }
$recoveryTemp = [IO.Directory]::CreateDirectory((Join-Path $repo 'output/ts3-qa-temp')).FullName
$helperHash = (Get-FileHash -LiteralPath $helper).Hash.ToLowerInvariant()
@('#pragma once',('#define RICON_TS3_VERSION "' + $PluginVersion + '"'),('#define RICON_UPDATE_HELPER_SHA256 "' + $helperHash + '"')) | Set-Content -LiteralPath (Join-Path $hostDirectory 'ricon_release.h') -Encoding ASCII
$source = Join-Path $hostDirectory 'pcm_probe.cpp'
$vcvars = 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
$batch = Join-Path $destination 'compile.cmd'
$exitGuard = 'if %errorlevel% neq 0 exit /b %errorlevel%'
$guardFixture = Join-Path $destination 'negative-exit-guard.cmd'
@('@echo off', 'call :guarded', 'if %errorlevel% neq -7 exit /b 1',
    'echo RICON_TS3_NEGATIVE_EXIT_GUARD_PASS', 'exit /b 0', ':guarded', 'call :negative',
    $exitGuard, 'exit /b 0', ':negative', 'exit /b -7') | Set-Content -LiteralPath $guardFixture -Encoding Ascii
$lines = @('@echo off', ('call "' + $guardFixture + '"'), $exitGuard,
    'setlocal', ('set "TEMP=' + $recoveryTemp + '"'), ('set "TMP=' + $recoveryTemp + '"'), ('"' + $recoveryTest + '"'), $exitGuard, 'endlocal',
    ('call "' + $vcvars + '" >nul'), $exitGuard,
    ('cl /nologo /TC /std:c11 /W4 /WX /MT /c "' + (Join-Path $dsp 'audio_chain.c') + '" /Fo:"' + $dspObject + '"'),
    $exitGuard,
    ('cl /nologo /TC /std:c11 /W4 /WX /MT /I"' + $hostDirectory + '" /I"' + $dsp + '" /c "' + $liveSource + '" /Fo:"' + $liveObject + '"'),
    $exitGuard,
    ('cl /nologo /std:c11 /W4 /WX /MT /I"' + $dsp + '" "' + (Join-Path $destination 'arma3-source/tsplugin/tests/audio_chain_smoke.c') + '" "' + $dspObject + '" /Fe:"' + (Join-Path $destination 'audio_chain_smoke.exe') + '"'),
    $exitGuard,
    ('"' + (Join-Path $destination 'audio_chain_smoke.exe') + '"'),
    $exitGuard,
    ('cl /nologo /std:c++17 /W4 /WX /EHsc /MT /I"' + $sdk + '" /I"' + $dsp + '" "' + (Join-Path $hostDirectory 'pcm_probe_selftest.cpp') + '" "' + $dspObject + '" "' + $liveObject + '" /Fe:"' + (Join-Path $destination 'pcm_probe_selftest.exe') + '"'),
    $exitGuard,
    ('"' + (Join-Path $destination 'pcm_probe_selftest.exe') + '"'), $exitGuard,
    ('cl /nologo /std:c++17 /W4 /WX /EHsc /MT /LD /I"' + $sdk + '" /I"' + $dsp + '" "' + $source + '" "' + $dspObject + '" "' + $liveObject + '" /link /OUT:"' + (Join-Path $destination 'ricon_pcm_probe.dll') + '"'),
    'exit /b %errorlevel%')
$lines | Set-Content -LiteralPath $batch -Encoding Ascii
if (!('Ricon.BuildErrorMode' -as [type])) {
    Add-Type -Namespace Ricon -Name BuildErrorMode -MemberDefinition '[System.Runtime.InteropServices.DllImport("kernel32.dll")] public static extern uint SetErrorMode(uint mode);'
}
# Unattended self-tests must return the Windows error to the build rather than wait for a user dialog.
$oldErrorMode = [Ricon.BuildErrorMode]::SetErrorMode(3)
Push-Location -LiteralPath $destination
try {
    & cmd.exe /d /c $batch 2>&1 | Tee-Object -FilePath (Join-Path $destination 'build.log')
    if ($LASTEXITCODE -ne 0) { throw ('Native build or test failed with 0x{0:X8}. See build.log.' -f [int]$LASTEXITCODE) }
} finally { Pop-Location; $null = [Ricon.BuildErrorMode]::SetErrorMode($oldErrorMode) }
$buildLog = [IO.File]::ReadAllText((Join-Path $destination 'build.log'))
if ($buildLog -notmatch '(?m)^RICON_UPDATE_RECOVERY_PASS checks=[1-9][0-9]*\r?$') { throw 'Updater recovery tests did not complete.' }
$expectedMarkers = @('RICON_TS3_NEGATIVE_EXIT_GUARD_PASS', 'audio_chain_smoke ok',
    'RICON_ARMA3_CALLBACK_ADAPTER_SELFTEST_PASS', 'RICON_TS3_LIVE_BOOTSTRAP_SELFTEST_PASS',
    'RICON_TS3_NETWORK_INPUT_SELFTEST_PASS', 'RICON_TS3_LIVE_GAME_GATE_SELFTEST_PASS',
    'RICON_TS3_LEVELS_AND_CONTROL_SELFTEST_PASS', 'RICON_TS3_SYNTHETIC_SIGNAL_NOISE_SELFTEST_PASS',
    'RICON_TS3_LIVE_AUDIO_PROFILE_SELFTEST_PASS', 'RICON_TS3_LIVE_NOISE_PROFILE_SELFTEST_PASS',
    'RICON_TS3_TAB_PROFILE_SELFTEST_PASS', 'RICON_TS3_TAB_ACTUAL_GUARD_SELFTEST_PASS',
    'RICON_TS3_TAB_PROOF_SELFTEST_PASS', 'RICON_STATUS_PROTOCOL_SELFTEST_PASS', 'RICON_TS3_HOST_SELFTEST_PASS')
foreach ($marker in $expectedMarkers) {
    if ([regex]::Matches($buildLog, ('(?m)^' + [regex]::Escape($marker) + '\r?$')).Count -ne 1) {
        throw "Missing or duplicated build regression marker: $marker"
    }
}
foreach ($pin in $pins) { if ((Get-FileHash -LiteralPath $pin.path).Hash -ne $pin.sha256) { throw 'Arma3 source changed during build; result not qualified' } }
foreach ($pin in $hostPins) { if ((Get-FileHash -LiteralPath $pin.path).Hash -ne $pin.sha256) { throw 'Host source changed during build; result not qualified' } }
$result = Get-FileHash -LiteralPath (Join-Path $destination 'ricon_pcm_probe.dll') -Algorithm SHA256
[ordered]@{api=26;version=$PluginVersion;sha256=$result.Hash.ToLowerInvariant();updaterSha256=$helperHash;releaseFeed=$feed} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $destination 'plugin.json') -Encoding UTF8
$result
