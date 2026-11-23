param(
	[string]$Destination = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'CoreKitExample')
)

$ErrorActionPreference = 'Stop'
$pluginRoot = Split-Path -Parent $PSScriptRoot
$sampleRoot = Join-Path $PSScriptRoot 'CoreKitExample'
$target = [IO.Path]::GetFullPath($Destination)
$rootPrefix = [IO.Path]::GetFullPath($pluginRoot).TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if ($target.Equals($pluginRoot, [StringComparison]::OrdinalIgnoreCase) -or
	$target.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
	throw 'Choose a destination outside the CoreKit plugin checkout.'
}
if (Test-Path -LiteralPath $target) {
	throw "Destination already exists: $target"
}

$sampleItems = @('CoreKitExample.uproject', 'Config', 'Content', 'Source') |
	ForEach-Object { Get-Item -LiteralPath (Join-Path $sampleRoot $_) }
$pluginItems = @('CoreKit.uplugin', 'Config', 'Content', 'Resources', 'Source', 'Tests') |
	ForEach-Object { Get-Item -LiteralPath (Join-Path $pluginRoot $_) }

$pluginTarget = Join-Path $target 'Plugins/CoreKit'
New-Item -ItemType Directory -Path $pluginTarget -Force | Out-Null
foreach ($item in $sampleItems) {
	Copy-Item -LiteralPath $item.FullName -Destination $target -Recurse
}
foreach ($item in $pluginItems) {
	Copy-Item -LiteralPath $item.FullName -Destination $pluginTarget -Recurse
}

Write-Output (Join-Path $target 'CoreKitExample.uproject')
