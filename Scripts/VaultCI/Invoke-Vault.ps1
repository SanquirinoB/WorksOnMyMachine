#Requires -Version 5.1
[CmdletBinding()]
param(
    [ValidateSet('Status', 'Run', 'DownloadLatest')]
    [string]$Action = 'Status',

    [ValidatePattern('^[\w][\w.@-]*$')]
    [string]$Target = 'vaultci@Vault',

    [ValidateRange(1, 65535)]
    [int]$Port = 2222,

    [ValidatePattern('^[A-Za-z]:[\\/]')]
    [string]$VaultRoot = 'C:\DelerictCI',

    [string]$Destination = '.'
)

$ErrorActionPreference = 'Stop'
$remoteRoot = '/' + $VaultRoot.Replace('\', '/').TrimEnd('/') + '/data'

if ($Action -eq 'Run') {
    $command = 'schtasks.exe /Run /TN "Delerict CI Poll"; exit $LASTEXITCODE'
} elseif ($Action -eq 'Status') {
    $command = @'
foreach ($file in 'Poll-status.json', 'Build-status.json', 'latest.json') {
    $path = Join-Path '__VAULT_ROOT__' "data\$file"
    if (Test-Path -LiteralPath $path) {
        Write-Output $file
        Get-Content -LiteralPath $path
    }
}
'@
    $command = $command.Replace('__VAULT_ROOT__', $VaultRoot.Replace("'", "''"))
}
if ($Action -ne 'DownloadLatest') {
    $encoded = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($command))
    & ssh.exe -p $Port -o BatchMode=yes $Target "powershell.exe -NoProfile -NonInteractive -EncodedCommand $encoded"
    if ($LASTEXITCODE -ne 0) { throw "Vault CI $Action failed on $Target." }
    return
}

$directory = (New-Item -ItemType Directory -Path $Destination -Force).FullName
$transferId = [Guid]::NewGuid().ToString('N')
$manifestFile = Join-Path $directory ".vault-$transferId.json"
$partialArchive = Join-Path $directory ".vault-$transferId.partial"
try {
    & scp.exe -P $Port -q -o BatchMode=yes "$($Target):$remoteRoot/latest.json" $manifestFile
    if ($LASTEXITCODE -ne 0) { throw "Could not read the latest successful build manifest on $Target." }
    $latest = Get-Content -LiteralPath $manifestFile -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($latest.buildId -cnotmatch '^\d{8}T\d{9}Z-[0-9a-f]{12}$' -or
        $latest.archiveSha256 -notmatch '^[0-9a-f]{64}$' -or $latest.archiveBytes -le 0) {
        throw 'Vault returned an invalid latest build manifest.'
    }
    $archiveName = "Delerict-Win64-Shipping-$($latest.buildId).7z"
    $archivePath = "history/$($latest.buildId)/$archiveName"
    if ($latest.archive -cne $archivePath) { throw 'Vault returned an unexpected archive path.' }
    $outputFile = Join-Path $directory $archiveName
    if (Test-Path -LiteralPath $outputFile) {
        if ((Get-FileHash -LiteralPath $outputFile).Hash -eq $latest.archiveSha256) { return $outputFile }
        throw "A different file already exists at $outputFile."
    }

    & scp.exe -P $Port -o BatchMode=yes "$($Target):$remoteRoot/$archivePath" $partialArchive
    if ($LASTEXITCODE -ne 0) { throw "Could not download $archiveName from $Target." }
    if ((Get-Item -LiteralPath $partialArchive).Length -ne $latest.archiveBytes -or
        (Get-FileHash -LiteralPath $partialArchive).Hash -ne $latest.archiveSha256) {
        throw "The downloaded archive failed verification for commit $($latest.commit)."
    }
    [IO.File]::Move($partialArchive, $outputFile)
    Write-Output $outputFile
} finally {
    foreach ($file in $manifestFile, $partialArchive) {
        if (Test-Path -LiteralPath $file) { Remove-Item -LiteralPath $file }
    }
}
