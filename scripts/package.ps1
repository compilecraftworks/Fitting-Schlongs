[CmdletBinding()]
param(
    [string]$Version = '1.0.4',
    [string]$BuildMode = 'releasedbg'
)

$ErrorActionPreference = 'Stop'

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildDll = Join-Path $projectRoot "build/windows/x64/$BuildMode/FittingSchlongs.dll"
$releaseRoot = Join-Path $projectRoot 'Release'
$sourceRoot = Join-Path $projectRoot 'Sources'
$stageRoot = Join-Path $env:TEMP "FittingSchlongs-$Version-package"
$releaseStage = Join-Path $stageRoot 'release'
$sourceStage = Join-Path $stageRoot 'source'

if (-not (Test-Path -LiteralPath $buildDll)) {
    throw "Built DLL not found: $buildDll"
}

$productVersion = (Get-Item -LiteralPath $buildDll).VersionInfo.ProductVersion
if (-not $productVersion.StartsWith($Version, [StringComparison]::Ordinal)) {
    throw "DLL version '$productVersion' does not match package version '$Version'."
}

$resolvedStage = [IO.Path]::GetFullPath($stageRoot)
if (-not $resolvedStage.StartsWith([IO.Path]::GetFullPath($env:TEMP), [StringComparison]::OrdinalIgnoreCase)) {
    throw "Unsafe staging path: $resolvedStage"
}

if (Test-Path -LiteralPath $stageRoot) {
    Remove-Item -LiteralPath $stageRoot -Recurse -Force
}

New-Item -ItemType Directory -Path (Join-Path $releaseStage 'SKSE/Plugins') -Force | Out-Null
New-Item -ItemType Directory -Path $sourceStage -Force | Out-Null
New-Item -ItemType Directory -Path $releaseRoot -Force | Out-Null
New-Item -ItemType Directory -Path $sourceRoot -Force | Out-Null

Copy-Item -LiteralPath $buildDll -Destination (Join-Path $releaseStage 'SKSE/Plugins/FittingSchlongs.dll')
foreach ($name in @('LICENSE', 'NOTICE.md', 'THIRD_PARTY_NOTICES.md')) {
    Copy-Item -LiteralPath (Join-Path $projectRoot $name) -Destination $releaseStage
}

foreach ($name in @(
    'src', 'scripts', 'LICENSE', 'NOTICE.md', 'THIRD_PARTY_NOTICES.md',
    'DEPENDENCIES.md', 'COMPATIBILITY_AUDIT.md', 'README.md', 'CHANGELOG.md', 'CHANGELOG.ko.md', '.gitignore', '.gitmodules',
    'xmake.lua', 'xmake-requires.lock'
)) {
    Copy-Item -LiteralPath (Join-Path $projectRoot $name) -Destination $sourceStage -Recurse
}

$commonLibSource = Join-Path $projectRoot 'lib/commonlibsse-ng'
$commonLibDestination = Join-Path $sourceStage 'lib/commonlibsse-ng'
New-Item -ItemType Directory -Path $commonLibDestination -Force | Out-Null
$robocopyArgs = @(
    $commonLibSource, $commonLibDestination, '/E',
    '/XD', '.git', '.xmake', 'build', 'lib',
    '/XF', '*.obj', '*.pch', '*.pdb', '*.ilk', '*.exp', '*.lib', '*.dll',
    '/NFL', '/NDL', '/NJH', '/NJS', '/NP'
)
& robocopy @robocopyArgs | Out-Null
if ($LASTEXITCODE -ge 8) {
    throw "Failed to copy CommonLibSSE-NG source (robocopy exit $LASTEXITCODE)."
}

$releaseZip = Join-Path $releaseRoot "Fitting Schlongs v$Version SE-AE.zip"
$sourceZip = Join-Path $sourceRoot "Fitting Schlongs v$Version Source.zip"
foreach ($archive in @($releaseZip, $sourceZip)) {
    if (Test-Path -LiteralPath $archive) {
        Remove-Item -LiteralPath $archive -Force
    }
}

Compress-Archive -Path (Join-Path $releaseStage '*') -DestinationPath $releaseZip -CompressionLevel Optimal
Compress-Archive -Path (Join-Path $sourceStage '*') -DestinationPath $sourceZip -CompressionLevel Optimal

$packedDllPath = Join-Path $stageRoot 'verify/SKSE/Plugins/FittingSchlongs.dll'
Expand-Archive -LiteralPath $releaseZip -DestinationPath (Join-Path $stageRoot 'verify')
$builtHash = (Get-FileHash -LiteralPath $buildDll -Algorithm SHA256).Hash
$packedHash = (Get-FileHash -LiteralPath $packedDllPath -Algorithm SHA256).Hash
if ($builtHash -ne $packedHash) {
    throw 'The DLL in the release archive does not match the built DLL.'
}

$checksums = @(
    ("{0}  {1}" -f (Get-FileHash $releaseZip -Algorithm SHA256).Hash, (Split-Path $releaseZip -Leaf))
    ("{0}  {1}" -f (Get-FileHash $sourceZip -Algorithm SHA256).Hash, (Split-Path $sourceZip -Leaf))
)
$checksumPath = Join-Path $releaseRoot 'SHA256SUMS.txt'
[IO.File]::WriteAllLines($checksumPath, $checksums, [Text.UTF8Encoding]::new($false))

Remove-Item -LiteralPath $stageRoot -Recurse -Force

Get-Item -LiteralPath $releaseZip, $sourceZip, $checksumPath |
    Select-Object FullName, Length, LastWriteTime
