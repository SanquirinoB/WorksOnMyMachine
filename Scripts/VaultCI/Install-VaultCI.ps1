#Requires -Version 5.1
#Requires -RunAsAdministrator
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [PSCredential]$BuildCredential,

    [string]$VaultRoot = 'C:\DelerictCI',
    [string]$Repository = 'https://github.com/SanquirinoB/WorksOnMyMachine.git',
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$Git = (Get-Command git.exe -CommandType Application).Source,
    [string]$SevenZip = 'C:\Program Files\7-Zip\7z.exe'
)

$ErrorActionPreference = 'Stop'
$machine = Get-CimInstance -ClassName Win32_ComputerSystem
if ($machine.Manufacturer -ne 'Microsoft Corporation' -or $machine.Model -ne 'Virtual Machine' -or
    $env:COMPUTERNAME -ne 'VaultCI') {
    throw 'Install Vault CI in the Hyper-V guest after naming it VaultCI and restarting it.'
}
$accountName = $BuildCredential.UserName -replace ('^(\.|' + [regex]::Escape($env:COMPUTERNAME) + ')\\'), ''
$account = Get-LocalUser -Name $accountName
if (!$account.Enabled) { throw "Enable the local build account $accountName first." }
$accountId = $account.SID.Value
$administrators = Get-LocalGroupMember -SID 'S-1-5-32-544'
if ($administrators.SID.Value -contains $accountId) {
    throw 'Use a standard Windows account for the Vault build tasks.'
}

$root = [IO.Path]::GetFullPath($VaultRoot).TrimEnd('\')
if ($VaultRoot -notmatch '^[A-Za-z]:\\' -or $root -match '^[A-Za-z]:$') {
    throw 'VaultRoot must be a dedicated directory on a local drive.'
}
for ($folder = [IO.DirectoryInfo]::new($root); $folder; $folder = $folder.Parent) {
    if ($folder.Exists -and $folder.Attributes.HasFlag([IO.FileAttributes]::ReparsePoint)) {
        throw "VaultRoot crosses a reparse point at $($folder.FullName)."
    }
}
$resolvedEngine = (Resolve-Path -LiteralPath $EngineRoot).Path
$gitExe = (Resolve-Path -LiteralPath $Git).Path
$sevenZipExe = (Resolve-Path -LiteralPath $SevenZip).Path
foreach ($file in (Join-Path $resolvedEngine 'Engine\Build\BatchFiles\RunUAT.bat'),
                 (Join-Path $resolvedEngine 'Engine\Build\Build.version'), $gitExe, $sevenZipExe) {
    if (!(Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing prerequisite file $file." }
}

& $gitExe lfs version
if ($LASTEXITCODE -ne 0) { throw 'Install Git LFS before setting up Delerict CI.' }

$tasks = @('Delerict CI Build', 'Delerict CI Poll')
foreach ($name in $tasks) {
    $task = Get-ScheduledTask -TaskName $name -ErrorAction SilentlyContinue
    if ($task -and $task.State -eq 'Running') { throw "Wait for $name to finish before updating Vault CI." }
}

$configPath = Join-Path $root 'config.json'
if ((Test-Path -LiteralPath $root) -and !(Test-Path -LiteralPath $configPath) -and
    (Get-ChildItem -LiteralPath $root -Force | Select-Object -First 1)) {
    throw 'Use an empty dedicated VaultRoot or the existing Vault CI installation directory.'
}
$null = New-Item -ItemType Directory -Path $root -Force
& icacls.exe $root /setowner '*S-1-5-32-544' | Out-Null
if ($LASTEXITCODE -ne 0) { throw "Could not set the administrator owner on $root." }
& icacls.exe $root /inheritance:r /grant:r '*S-1-5-32-544:(OI)(CI)F' '*S-1-5-18:(OI)(CI)F' "*$($accountId):(OI)(CI)RX" | Out-Null
if ($LASTEXITCODE -ne 0) { throw "Could not set the permissions on $root." }
$dataPath = Join-Path $root 'data'
$data = New-Item -ItemType Directory -Path $dataPath -Force
if ($data.Attributes.HasFlag([IO.FileAttributes]::ReparsePoint)) { throw "Expected a regular directory at $dataPath." }
& icacls.exe $dataPath /grant:r "*$($accountId):(OI)(CI)M" | Out-Null
if ($LASTEXITCODE -ne 0) { throw "Could not grant $accountName access to $dataPath." }

@{ repository = $Repository; engine = $resolvedEngine; git = $gitExe; sevenZip = $sevenZipExe } |
    ConvertTo-Json | Set-Content -LiteralPath $configPath -Encoding UTF8
$worker = Join-Path $root 'Invoke-VaultCI.ps1'
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Invoke-VaultCI.ps1') -Destination $worker -Force

$shell = [Security.SecurityElement]::Escape("$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe")
$workingDirectory = [Security.SecurityElement]::Escape($root)
$start = (Get-Date).AddMinutes(10).ToString('yyyy-MM-ddTHH:mm:ss')
$user = "$env:COMPUTERNAME\$accountName"
$password = $BuildCredential.GetNetworkCredential().Password
try {
    foreach ($mode in 'Build', 'Poll') {
        $trigger = if ($mode -eq 'Poll') {
            "<Triggers><TimeTrigger><StartBoundary>$start</StartBoundary><Repetition><Interval>PT10M</Interval></Repetition></TimeTrigger></Triggers>"
        }
        $arguments = [Security.SecurityElement]::Escape(
            ('-NoProfile -NonInteractive -ExecutionPolicy RemoteSigned -File "{0}" -Mode {1}' -f $worker, $mode)
        )
        $xml = @"
<?xml version="1.0" encoding="UTF-16"?>
<Task version="1.3" xmlns="http://schemas.microsoft.com/windows/2004/02/mit/task">
  $trigger
  <Principals>
    <Principal id="VaultUser">
      <UserId>$accountId</UserId><LogonType>Password</LogonType><RunLevel>LeastPrivilege</RunLevel>
    </Principal>
  </Principals>
  <Settings>
    <MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy>
    <DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>
    <StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>
    <StartWhenAvailable>true</StartWhenAvailable>
    <ExecutionTimeLimit>PT0S</ExecutionTimeLimit>
  </Settings>
  <Actions Context="VaultUser">
    <Exec><Command>$shell</Command><Arguments>$arguments</Arguments><WorkingDirectory>$workingDirectory</WorkingDirectory></Exec>
  </Actions>
</Task>
"@
        Register-ScheduledTask -TaskName "Delerict CI $mode" -Xml $xml -User $user -Password $password -Force | Out-Null
    }
} finally {
    Remove-Variable password
}
Start-ScheduledTask -TaskName 'Delerict CI Poll'
Write-Output "Vault CI registered for $user. Logs and packages are under $dataPath."
