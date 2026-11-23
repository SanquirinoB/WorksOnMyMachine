# Delerict on Vault CI

This worker builds the root `Delerict.uproject` from the `main` branch of
WorksOnMyMachine. It uses the existing **VaultCI** Windows guest and keeps its
checkout, build cache, and history under `C:\DelerictCI\data`.

## Install on the guest

Install UE 5.8.4, the Visual Studio C++ toolchain, Git for Windows with Git LFS,
and 7-Zip. Give the standard Vault build account read access to the
WorksOnMyMachine repository. For a private repository, configure its deploy key
and GitHub host alias under that account before registering the tasks.

From an elevated Windows PowerShell on the guest, run the installer from a
checkout with the appropriate repository URL and build account.

```powershell
$buildCredential = Get-Credential -UserName '.\vaultci' -Message 'Vault build account'
.\Scripts\VaultCI\Install-VaultCI.ps1 -BuildCredential $buildCredential
```

Use `-Repository` to set the authenticated repository URL when the guest uses
an SSH deploy key. The installer registers **Delerict CI Poll** and
**Delerict CI Build** and starts the first poll.

## Operate from MainMachine

```powershell
.\Scripts\VaultCI\Invoke-Vault.ps1 -Action Status
.\Scripts\VaultCI\Invoke-Vault.ps1 -Action Run
.\Scripts\VaultCI\Invoke-Vault.ps1 -Action DownloadLatest -Destination .\Builds
```

The client defaults to `vaultci@Vault` on SSH port 2222. If you customized the
connection, pass `-Target`, `-Port`, or `-VaultRoot`. The worker publishes a
Win64 Shipping archive for each successful build. The client verifies its
size and SHA-256. Extract the archive and open `Windows\Delerict.exe`.
