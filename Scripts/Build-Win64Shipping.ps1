#Requires -Version 5.1
[CmdletBinding()]
param(
    [string] $EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string] $ArchiveDirectory,
    [string] $CookerOptions
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'Delerict.uproject'
$uat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if (!(Test-Path -LiteralPath $uat -PathType Leaf)) {
    throw "Cannot find Unreal Automation Tool at $uat."
}
if (!$ArchiveDirectory) {
    $ArchiveDirectory = Join-Path $root 'Saved\Builds\Win64Shipping'
}
$archive = (New-Item -ItemType Directory -Path $ArchiveDirectory -Force).FullName
$windowsPackage = Join-Path $archive 'Windows'
$arguments = @(
    'BuildCookRun', "-project=$project", '-target=Delerict', '-platform=Win64',
    '-clientconfig=Shipping', '-build', '-cook', '-stage', '-package', '-pak',
    '-iostore', '-archive', "-archivedirectory=$windowsPackage", '-unattended',
    '-nop4', '-utf8output'
)
if ($CookerOptions) { $arguments += "-AdditionalCookerOptions=$CookerOptions" }
& $uat @arguments
if ($LASTEXITCODE -ne 0) { throw "Unreal Automation Tool exited with $LASTEXITCODE." }

$game = Join-Path $windowsPackage 'Delerict.exe'
if (!(Test-Path -LiteralPath $game -PathType Leaf)) {
    throw "The packaged launcher is missing at $game."
}
Write-Output "Packaged Delerict at $game"
