[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Archive,
    [string]$GameDirectory = (Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'My Games/ArmaReforger')
)
$ErrorActionPreference = 'Stop'
$zipPath = (Resolve-Path -LiteralPath $Archive).Path
$pin = '36a75321e41ff815cbf222b8aabd92fcc9dbdc13ebe24ed3fa374f2dbfd2adc1'
if ((Get-FileHash -LiteralPath $zipPath).Hash -ine $pin) { throw 'Expected the qualified RICON-Core-0.6.3.zip.' }
$gameRoot = [IO.Path]::GetFullPath($GameDirectory)
$addons = Join-Path $gameRoot 'addons'
$target = Join-Path $addons 'RICON_Core'
foreach ($p in $gameRoot,$addons,$target) {
    if ([IO.Directory]::Exists($p) -and ([IO.File]::GetAttributes($p) -band [IO.FileAttributes]::ReparsePoint)) { throw "Indirect installation directory: $p" }
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $files = @($zip.Entries | Where-Object { $_.FullName.StartsWith('RICON_Core/') -and $_.Name })
    if ([IO.Directory]::Exists($target)) {
        $sha = [Security.Cryptography.SHA256]::Create()
        try {
            foreach ($f in $files) {
                $dest = Join-Path $addons $f.FullName
                $inputFile = $f.Open()
                try { $expected = [BitConverter]::ToString($sha.ComputeHash($inputFile)).Replace('-','') } finally { $inputFile.Dispose() }
                if (!(Test-Path -LiteralPath $dest -PathType Leaf) -or (Get-FileHash -LiteralPath $dest).Hash -ine $expected) { throw "Existing addon differs. Preserved: $target" }
            }
        } finally { $sha.Dispose() }
        if (@(Get-ChildItem -LiteralPath $target -File -Recurse -Force).Count -ne $files.Count) { throw "Existing addon contains other files. Preserved: $target" }
        "Already installed: $target"
        return
    }
    $null = [IO.Directory]::CreateDirectory($addons)
    $stage = [IO.Directory]::CreateDirectory((Join-Path $gameRoot ('.ricon-install-' + [guid]::NewGuid()))).FullName
    foreach ($f in $files) {
        $dest = [IO.Path]::GetFullPath((Join-Path $stage $f.FullName.Substring('RICON_Core/'.Length)))
        if (!$dest.StartsWith($stage + '\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected archive path.' }
        $null = [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($dest))
        [IO.Compression.ZipFileExtensions]::ExtractToFile($f,$dest,$false)
    }
    [IO.Directory]::Move($stage,$target)
    "Installed: $target"
} finally { $zip.Dispose() }
