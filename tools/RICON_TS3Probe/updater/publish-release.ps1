[CmdletBinding(DefaultParameterSetName='Sign')]
param(
    [Parameter(Mandatory,ParameterSetName='Init')][switch]$InitializePublisherKey,
    [Parameter(Mandatory,ParameterSetName='Sign')][string]$BuildDirectory,
    [Parameter(Mandatory,ParameterSetName='Sign')][string]$FeedDirectory
)
$ErrorActionPreference = 'Stop'
$keyName = 'RICON.Reforger.TS3.Alpha.1'
$publicFile = Join-Path $PSScriptRoot 'alpha-public.xml'
$provider = [Security.Cryptography.CngProvider]::MicrosoftSoftwareKeyStorageProvider
if (![Security.Cryptography.CngKey]::Exists($keyName,$provider)) {
    if (!$InitializePublisherKey -or (Test-Path -LiteralPath $publicFile)) { throw 'The private publisher key for this channel is unavailable.' }
    $options = [Security.Cryptography.CngKeyCreationParameters]::new()
    $options.Provider = $provider
    $options.ExportPolicy = [Security.Cryptography.CngExportPolicies]::None
    $options.KeyUsage = [Security.Cryptography.CngKeyUsages]::Signing
    $options.Parameters.Add([Security.Cryptography.CngProperty]::new('Length',[BitConverter]::GetBytes(3072),0))
    $key = [Security.Cryptography.CngKey]::Create([Security.Cryptography.CngAlgorithm]::Rsa,$keyName,$options)
} else {
    $key = [Security.Cryptography.CngKey]::Open($keyName,$provider)
}
$signer = [Security.Cryptography.RSACng]::new($key)
try {
    $public = $signer.ToXmlString($false)
    if ($InitializePublisherKey -and !(Test-Path -LiteralPath $publicFile)) {
        [IO.File]::WriteAllText($publicFile,$public,[Text.Encoding]::ASCII)
    }
    if ([IO.File]::ReadAllText($publicFile).Trim() -cne $public) { throw 'Publisher key differs from the pinned channel key.' }
    if ($InitializePublisherKey) { return }

    $build = (Resolve-Path -LiteralPath $BuildDirectory).Path
    $info = Get-Content -LiteralPath (Join-Path $build 'plugin.json') -Raw | ConvertFrom-Json
    $version = [string]$info.version
    if ($version -cnotmatch '\A(0|[1-9][0-9]{0,4})\.(0|[1-9][0-9]{0,4})\.(0|[1-9][0-9]{0,4})\z' -or
        ($version.Split('.') | Where-Object { [int]$_ -gt 65535 }) -or !$info.updaterSha256) { throw 'Invalid updater release.' }
    $feed = [IO.Directory]::CreateDirectory([IO.Path]::GetFullPath($FeedDirectory)).FullName
    $release = Join-Path $feed $version
    if (Test-Path -LiteralPath $release) { throw "Release $version already exists." }
    $stage = [IO.Directory]::CreateDirectory((Join-Path $feed ('.release-' + [guid]::NewGuid()))).FullName
    $archive = Join-Path $stage 'RICON.ts3_plugin'
    python -B (Join-Path $PSScriptRoot '../package_manual.py') $build $archive
    if ($LASTEXITCODE) { throw 'Package build failed.' }
    $hash = (Get-FileHash -LiteralPath $archive).Hash.ToLowerInvariant()
    $message = [Text.Encoding]::ASCII.GetBytes("RICON-TS3-PACKAGE-1`n$version`n$hash`n")
    $signature = $signer.SignData($message,[Security.Cryptography.HashAlgorithmName]::SHA256,[Security.Cryptography.RSASignaturePadding]::Pkcs1)
    [IO.File]::WriteAllText((Join-Path $stage 'RICON.sig'),[Convert]::ToBase64String($signature),[Text.Encoding]::ASCII)
    [IO.Directory]::Move($stage,$release)
    Get-FileHash -LiteralPath (Join-Path $release 'RICON.ts3_plugin')
} finally { $signer.Dispose(); $key.Dispose() }
