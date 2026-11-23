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

Start **Standalone Game** in `L_MainMenu` and choose **New Game** to enter
`L_DCampaign`. The opening quarters, weighted plate tutorial, grapple bay and
escape pods share one playable map. Press **Esc** or the gamepad **Menu** button
to pause. Open **Settings**
from the title or pause menu to adjust video and audio or view the controls.
Use **WASD**, the **arrow keys**, or the **left stick** for directional input.
Aim with the mouse or right stick. Hold and release **Space** or the bottom
gamepad face button for the jump charge. Free flight retains velocity after an
impulse. Hold **Right Mouse Button** to charge a throw and release it to launch
the selected item. Press **F** to grab a nearby item. Use **1** for Weighted
Plate, **2** for Completo Italiano, **3** for Grappling Hook, and **4** for
Portable Black Hole Device. **Tab** cycles through carried types. While
floating, hold **E** to latch onto steel.
You can release **E** once attached. Hold **Left Shift** to run, and use a jump or
throw recoil to break contact. Press **Q** to activate the most recently
deployed remote gadget. With an anchored grapple, press **Q** to pull toward it
and **Q** again to release and reel it in. Open `L_DTestLane` to try the steel floor, wall, and
ceiling alongside the grappling hook, remote grenade, portable black hole,
plates, and throw switch.
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

Open `L_DLevelDesignLab` to try the platforming kit. The
[level-design guide](Docs/LevelDesign.md) covers the Place Actors palette and
building a room with hazards, checkpoints, and exits.

## Package for Windows

Run this command from the repository root.

```powershell
powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -File .\Scripts\Build-Win64Shipping.ps1
```

The default archive is `Saved/Builds/Win64Shipping/Windows/`. Open
`Delerict.exe` from that directory to play the packaged game.

Git LFS tracks Unreal packages and imported binary source assets. Unreal generates
local build output and caches in `Binaries/`, `DerivedDataCache/`, `Intermediate/`,
and `Saved/`.

## License

Project-owned material is licensed under the [MIT License](LICENSE). Bundled
third-party assets and dependencies remain subject to their respective terms.
