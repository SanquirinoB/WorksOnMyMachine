# WorksOnMyMachine

WorksOnMyMachine is an Unreal Engine 5.8.4 project for Windows.

## Open the project

1. Install Unreal Engine 5.8.4, Git LFS, and Visual Studio with the **Game development with C++** workload and a Windows SDK.
2. From the repository root, run `git lfs install` and `git lfs pull`.
3. Build the Win64 Development editor target in PowerShell. Adjust `$engineRoot` for your Unreal installation.

   ```powershell
   $engineRoot = 'C:\Program Files\Epic Games\UE_5.8'
   $project = (Resolve-Path .\WorksOnMyMachine.uproject).Path
   & "$engineRoot\Engine\Build\BatchFiles\Build.bat" WorksOnMyMachineEditor Win64 Development "-Project=$project" -WaitMutex
   ```

4. Open [WorksOnMyMachine.uproject](WorksOnMyMachine.uproject) in Unreal Editor.

Press **Play** in the startup map. Move with **A/D**, the **left/right arrows**, or
the **left stick**. Jump with **Space** or the **bottom gamepad face button**.

CoreKit is included in [Plugins/CoreKit](Plugins/CoreKit). Its
[quick start](Plugins/CoreKit/Docs/QuickStart.md) covers the reusable systems.
The GASP animation library and its DataDriven dependency live under `Plugins/`.
GASP assets use the `/GASP/` mount point.

Git LFS tracks Unreal packages and imported binary source assets. Unreal generates
local build output and caches in `Binaries/`, `DerivedDataCache/`, `Intermediate/`,
and `Saved/`.
