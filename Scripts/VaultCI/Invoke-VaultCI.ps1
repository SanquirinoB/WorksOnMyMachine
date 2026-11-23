#Requires -Version 5.1
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('Poll', 'Build')]
    [string]$Mode,

    [switch]$Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = Join-Path $PSScriptRoot 'data'
$config = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'config.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$checkout = Join-Path $root 'checkout'
$workspace = Join-Path $root 'workspace'
$pending = Join-Path $root 'pending'
$history = Join-Path $root 'history'
$latestFile = Join-Path $root 'latest.json'
$statusFile = Join-Path $root "$Mode-status.json"
$logFile = Join-Path $root "$Mode.log"
$env:GIT_TERMINAL_PROMPT = '0'
$env:GCM_INTERACTIVE = 'Never'
$env:GIT_SSH_COMMAND = 'ssh -o BatchMode=yes -o StrictHostKeyChecking=yes'

function Write-JsonFile($Path, $Value) {
    $temporary = "$Path.tmp"
    [IO.File]::WriteAllText($temporary, ($Value | ConvertTo-Json -Depth 5))
    if ([IO.File]::Exists($Path)) {
        [IO.File]::Replace($temporary, $Path, [NullString]::Value)
    } else {
        [IO.File]::Move($temporary, $Path)
    }
}

function Enter-VaultLock($Name, $WaitSeconds = 0) {
    $path = Join-Path $root "$Name.lock"
    $timer = [Diagnostics.Stopwatch]::StartNew()
    do {
        try {
            return [IO.File]::Open($path, 'OpenOrCreate', 'ReadWrite', 'None')
        } catch [IO.IOException] {
            if (($_.Exception.HResult -band 0xffff) -ne 32) { throw }
            if ($timer.Elapsed.TotalSeconds -ge $WaitSeconds) { return }
            Start-Sleep -Milliseconds 250
        }
    } while ($true)
}

function Assert-PlainDirectory($Path) {
    $item = Get-Item -LiteralPath $Path -Force
    if (!$item.PSIsContainer -or $item.Attributes.HasFlag([IO.FileAttributes]::ReparsePoint)) {
        throw "Expected a regular directory at $Path."
    }
}

function Invoke-Git {
    param([string[]]$Arguments)
    $ErrorActionPreference = 'Continue'
    $result = & $config.git @Arguments 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "git $($Arguments -join ' ') exited with code $LASTEXITCODE. $($result -join [Environment]::NewLine)"
    }
    $result
}

function Invoke-LoggedCommand($File, [string[]]$Arguments, [int]$MaxSuccessCode = 0) {
    $ErrorActionPreference = 'Continue'
    $log = [IO.StreamWriter]::new($logFile, $true)
    $log.AutoFlush = $true
    try {
        $log.WriteLine("[$([DateTime]::UtcNow.ToString('o'))] $File $($Arguments -join ' ')")
        & $File @Arguments 2>&1 | ForEach-Object { $log.WriteLine("$_") }
        if ($LASTEXITCODE -gt $MaxSuccessCode -or $LASTEXITCODE -lt 0) {
            throw "$File exited with $LASTEXITCODE. See $logFile."
        }
    } finally {
        $log.Dispose()
    }
}

function Sync-Workspace {
    Assert-PlainDirectory $checkout
    Assert-PlainDirectory $workspace
    $excluded = @('.git', '.vs', 'Binaries', 'Intermediate', 'Saved', 'DerivedDataCache')
    Invoke-LoggedCommand "$env:SystemRoot\System32\robocopy.exe" (
        @($checkout, $workspace, '/MIR', '/XJ', '/SL', '/R:2', '/W:2', '/MT:8', '/NP', '/XD') + $excluded + @('/XF', '.git')
    ) -MaxSuccessCode 7
}

function Get-BuildPlan($Latest, $Commit, $EngineKey, $ProjectRoot, $CacheFile) {
    if ($Force -or !$Latest -or $Latest.engineKey -ne $EngineKey -or
        !(Test-Path -LiteralPath (Join-Path $root $Latest.archive) -PathType Leaf)) {
        return 'BuildAndCook'
    }

    $plan = 'Skip'
    $changes = Invoke-Git @('-C', $checkout, '-c', 'core.quotePath=false', 'diff', '--name-status', '--no-renames', $Latest.commit, $Commit, '--')
    foreach ($change in $changes) {
        $kind, $path = "$change" -split "\t", 2
        if ($path -match '^(README\.md$|\.gitignore$|\.vsconfig$|Art/|Docs/|Plugins/[^/]+/(AGENTS\.md$|PLAN\.md$|\.agents/|Docs/|Samples/|Tests/|Binaries/))') {
            continue
        }
        if ($path -ieq 'Config/DefaultPakFileRules.ini') {
            if ($plan -eq 'Skip') { $plan = 'StageOnly' }
        } elseif ($path -match '^(Plugins/[^/]+/)?Content/') {
            if ($kind -notin 'A', 'M' -or $plan -eq 'Cook') { $plan = 'Cook' }
            else { $plan = 'IterativeCook' }
        } elseif ($path -match '^(Config/|Source/|Plugins/|Delerict\.uproject$|\.gitattributes$)') {
            return 'BuildAndCook'
        }
    }
    if ($plan -eq 'Skip') { return $plan }
    if (!(Test-Path -LiteralPath $CacheFile -PathType Leaf) -or
        (Get-Content -LiteralPath $CacheFile -Raw).Trim() -ne $Latest.commit -or
        !(Test-Path -LiteralPath (Join-Path $ProjectRoot 'Binaries\Win64\Delerict-Win64-Shipping.target') -PathType Leaf) -or
        !(Test-Path -LiteralPath (Join-Path $ProjectRoot 'Binaries\Win64\UnrealEditor-Delerict.dll') -PathType Leaf)) {
        return 'BuildAndCook'
    }
    if (!(Test-Path -LiteralPath (Join-Path $ProjectRoot 'Saved\Cooked\Windows') -PathType Container)) {
        return 'Cook'
    }
    $plan
}

Assert-PlainDirectory $root
$runLock = Enter-VaultLock $Mode
if (!$runLock) {
    Write-Output "A Vault $Mode run is already in progress."
    return
}

$status = [ordered]@{ state = 'Running'; startedUtc = [DateTime]::UtcNow.ToString('o') }
$pendingBuild = $null
try {
    Set-Content -LiteralPath $logFile -Value '' -Encoding UTF8
    if ($Mode -eq 'Build') { Write-JsonFile $statusFile $status }
    $sourceLock = Enter-VaultLock 'source' -WaitSeconds 300
    if (!$sourceLock) { throw 'Timed out waiting for the Vault checkout.' }

    try {
        if ($Mode -eq 'Poll') {
            Write-JsonFile $statusFile $status
            if (!(Test-Path -LiteralPath $checkout)) {
                Invoke-LoggedCommand $config.git @('clone', '--branch', 'main', '--single-branch', '--', $config.repository, $checkout)
                Invoke-LoggedCommand $config.git @('-C', $checkout, 'lfs', 'install', '--local')
            }
            Assert-PlainDirectory $checkout
            $branch = Invoke-Git @('-C', $checkout, 'branch', '--show-current')
            $origin = Invoke-Git @('-C', $checkout, 'config', '--get', 'remote.origin.url')
            $dirty = Invoke-Git @('-C', $checkout, 'status', '--porcelain', '--untracked-files=all')
            if ($branch -cne 'main' -or $origin -cne $config.repository -or $dirty) {
                throw 'Vault checkout must have a clean main branch and the configured origin.'
            }
            Invoke-LoggedCommand $config.git @('-C', $checkout, 'pull', '--ff-only', '--no-rebase', 'origin', 'main')
            Invoke-LoggedCommand $config.git @('-C', $checkout, 'lfs', 'pull')
            $status.commit = Invoke-Git @('-C', $checkout, 'rev-parse', 'HEAD')
            if ($status.commit -cne (Invoke-Git @('-C', $checkout, 'rev-parse', 'FETCH_HEAD'))) {
                throw 'Vault checkout differs from origin/main after pull.'
            }
            $status.state = 'Succeeded'
            $status.finishedUtc = [DateTime]::UtcNow.ToString('o')
            Write-JsonFile $statusFile $status
        } else {
            foreach ($directory in $workspace, $pending, $history) {
                New-Item -ItemType Directory -Path $directory -Force | Out-Null
                Assert-PlainDirectory $directory
            }
            Assert-PlainDirectory $checkout
            $status.commit = Invoke-Git @('-C', $checkout, 'rev-parse', 'HEAD')
            $poll = Get-Content -LiteralPath (Join-Path $root 'Poll-status.json') -Raw -Encoding UTF8 | ConvertFrom-Json
            if ($poll.state -ne 'Succeeded' -or $poll.commit -cne $status.commit) {
                throw 'Vault must complete a successful poll before building this revision.'
            }
            $projectRoot = $workspace
            $engineKey = (Get-FileHash -LiteralPath (Join-Path $config.engine 'Engine\Build\Build.version')).Hash
            $latest = $null
            if (Test-Path -LiteralPath $latestFile) {
                $latest = Get-Content -LiteralPath $latestFile -Raw -Encoding UTF8 | ConvertFrom-Json
            }
            $cacheFile = Join-Path $root 'workspace-commit'
            $status.plan = Get-BuildPlan $latest $status.commit $engineKey $projectRoot $cacheFile
            if ($status.plan -ne 'Skip') {
                if (Test-Path -LiteralPath $cacheFile) { Remove-Item -LiteralPath $cacheFile }
                Write-JsonFile $statusFile $status
                Sync-Workspace
            }
        }
    } finally {
        $sourceLock.Dispose()
    }

    if ($Mode -eq 'Poll') {
        if ((Get-ScheduledTask -TaskName 'Delerict CI Build').State -ne 'Running') {
            Start-ScheduledTask -TaskName 'Delerict CI Build'
        }
        return
    }
    if ($status.plan -eq 'Skip') {
        $status.state = 'Skipped'
        return
    }

    $buildId = '{0}-{1}' -f [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'), $status.commit.Substring(0, 12)
    $pendingBuild = (New-Item -ItemType Directory -Path (Join-Path $pending $buildId)).FullName
    $package = Join-Path $pendingBuild 'Windows'
    $project = Join-Path $projectRoot 'Delerict.uproject'
    $uat = Join-Path $config.engine 'Engine\Build\BatchFiles\RunUAT.bat'
    $arguments = @(
        'BuildCookRun', "-project=$project", '-target=Delerict', '-platform=Win64', '-clientconfig=Shipping',
        '-unattended', '-nop4', '-utf8output', '-stage', '-package', '-pak', '-iostore', '-archive',
        "-archivedirectory=$package"
    )
    switch ($status.plan) {
        'BuildAndCook'  { $arguments += '-build', '-cook' }
        'IterativeCook' { $arguments += '-skipbuild', '-cook', '-cookincremental' }
        'Cook'          { $arguments += '-skipbuild', '-cook' }
        'StageOnly'     { $arguments += '-skipbuild', '-skipcook' }
    }
    Invoke-LoggedCommand $uat $arguments

    if (!(Test-Path -LiteralPath (Join-Path $package 'Delerict.exe') -PathType Leaf) -or
        !(Test-Path -LiteralPath (Join-Path $package 'Delerict\Binaries\Win64\Delerict-Win64-Shipping.exe') -PathType Leaf)) {
        throw 'Unreal Automation Tool did not archive the expected Win64 Shipping executables.'
    }
    $archiveName = "Delerict-Win64-Shipping-$buildId.7z"
    Push-Location -LiteralPath $pendingBuild
    try {
        Invoke-LoggedCommand $config.sevenZip @('a', '-t7z', '-mx=1', $archiveName, 'Windows')
        Invoke-LoggedCommand $config.sevenZip @('t', $archiveName)
    } finally {
        Pop-Location
    }
    $archive = Get-Item -LiteralPath (Join-Path $pendingBuild $archiveName)
    $manifest = [ordered]@{
        buildId = $buildId
        commit = $status.commit
        engineKey = $engineKey
        plan = $status.plan
        publishedUtc = [DateTime]::UtcNow.ToString('o')
        package = "history/$buildId/Windows"
        archive = "history/$buildId/$archiveName"
        archiveBytes = $archive.Length
        archiveSha256 = (Get-FileHash -LiteralPath $archive.FullName).Hash
    }
    Write-JsonFile (Join-Path $pendingBuild 'build.json') $manifest
    Copy-Item -LiteralPath $logFile -Destination (Join-Path $pendingBuild 'build.log')
    [IO.Directory]::Move($pendingBuild, (Join-Path $history $buildId))
    $pendingBuild = $null
    Write-JsonFile $latestFile $manifest
    Set-Content -LiteralPath $cacheFile -Value $status.commit -Encoding ASCII
    $status.buildId = $buildId
    $status.state = 'Succeeded'
} catch {
    $status.state = 'Failed'
    $status.error = $_.Exception.Message
    throw
} finally {
    try {
        if ($Mode -ne 'Poll' -or $status.state -ne 'Succeeded') {
            $status.finishedUtc = [DateTime]::UtcNow.ToString('o')
            Write-JsonFile $statusFile $status
        }
        if ($pendingBuild) {
            Assert-PlainDirectory $pending
            Assert-PlainDirectory $pendingBuild
            if ([IO.Directory]::GetParent($pendingBuild).FullName -eq $pending) {
                [IO.Directory]::Delete($pendingBuild, $true)
            }
        }
    } finally {
        $runLock.Dispose()
    }
}
