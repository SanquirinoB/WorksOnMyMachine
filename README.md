# Delerict

Delerict is an Unreal Engine 5.8.4 project for Windows in the WorksOnMyMachine repository.

## Open the project

1. Install Unreal Engine 5.8.4, Git LFS, and Visual Studio with the **Game development with C++** workload and a Windows SDK.
2. From the repository root, run `git lfs install` and `git lfs pull`.
3. Build the Win64 Development editor target in PowerShell. Adjust `$engineRoot` for your Unreal installation.

   ```powershell
   $engineRoot = 'C:\Program Files\Epic Games\UE_5.8'
   $project = (Resolve-Path .\Delerict.uproject).Path
   & "$engineRoot\Engine\Build\BatchFiles\Build.bat" DelerictEditor Win64 Development "-Project=$project" -WaitMutex
   ```

4. Open [Delerict.uproject](Delerict.uproject) in Unreal Editor.

Press **Play** in the startup map, `L_DHangar`. Move with **A/D**, the
**left/right arrows**, or the **left stick**. Jump with **Space** or the **bottom
gamepad face button**. Open `L_DTestLane` for the movement and camera test fixtures.
The workshop has steam, sparks, and an ambient loop. The source soundtrack is
under `Art/Music/`.

CoreKit is included in [Plugins/CoreKit](Plugins/CoreKit). Its
[quick start](Plugins/CoreKit/Docs/QuickStart.md) covers the reusable systems.
The GASP animation library and its DataDriven dependency live under `Plugins/`.
GASP assets use the `/GASP/` mount point.
The player Blueprint, retargeter, and four locomotion clips are in
`Content/Delerict/Characters/Player/`.
Project assets use their Unreal type prefix followed by `D`, as in `BP_DPlayer`,
`SK_DPlayer`, and `L_DTestLane`. Native types use the Unreal class prefix followed
by `D`, as in `ADPawn` and `UDMoverComponent`.

Git LFS tracks Unreal packages and imported binary source assets. Unreal generates
local build output and caches in `Binaries/`, `DerivedDataCache/`, `Intermediate/`,
and `Saved/`.
