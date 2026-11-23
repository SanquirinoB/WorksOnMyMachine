# CoreKit validation

The modules were built and checked in this order on Unreal Engine 5.8.3, Win64. Each module passed its gate before work moved to the next one. The checks used a clean consumer project and its Editor and Development targets.

| Module | Passed behavior |
| --- | --- |
| Data | A cooked game discovered Data Assets by path, recovered inline Blueprint fragments in authored order, and resolved an exact Gameplay Tag. |
| Navigation | Slate focus moved to a UMG button and restored the previous widget. Injected keyboard, mouse, and gamepad events updated the device. Enhanced Input mappings resolved different textures for keyboard and gamepad. |
| Tween | A UMG opacity value and a movable door component received interpolated values. Pause, resume, cancellation from a callback, and owner destruction behaved as expected. |
| Loading | Overlapping requests held the overlay. A real map reload and a simulated travel failure released it and restored the previous viewport input setting. |
| Save | Two processes verified disk persistence across a restart, separate slots, failed-write recovery, fallback from a corrupt newest bank, and the result when both banks are corrupt. |
| Actions | All 20 automation cases passed, including asynchronous cancellation, queued request identity, state authority, and exact montage playback ownership for IASA. |

The smoke runs use Unreal's Null RHI and synthetic input. The [module examples](QuickStart.md) are steps to reproduce in a consumer project.

The Win64 Development package also passed an integrated test in two processes. It focused and animated a menu, opened a different map with the loading overlay, restored viewport input, discovered the cooked data assets, saved a checkpoint, and loaded that checkpoint after restarting.

## Expansion

The [expansion](ExpansionPlan.md) was checked on UE 5.8.3 Win64 with separate consumer projects for the module fixtures and the integrated example.

| Stage | Observed result |
| --- | --- |
| Save | A packaged game listed slots by metadata, restored a door after restart and exercised duplicate GUIDs, missing and late participants, failed writes and backup recovery. |
| Loading | A D3D11 run recorded a Slate paint before travel, kept input held through readiness, rejected invalid destinations and stale signals, and restored focus and input on completion or failure. |
| Navigation | D3D11 runs exercised out-of-order focus scopes, owner teardown, synthetic keyboard, mouse and controller input, and rebinding with conflict detection, swap, reset and restart persistence. |
| Tween | The fixture exercised game time versus real time during pause and dilation, sequencing, cancellation, channel replacement and owner cleanup. |
| Data | Editor validated fragment rules. The packaged fixture discovered soft references, loaded the selection and reported a missing asset. The example also verified discovery during uncooked Standalone startup. |
| Actions | The held-input fixture exercised deferred admission with two IASA owners, release before recovery and owner destruction. In rendered PIE the Gameplay Debugger displayed the controller's IASA block, active execution ID and request result. All 20 Actions automation tests passed. |
| Example | Four rendered Win64 Development processes exercised reassignment to **F** and the updated glyph, two checkpoint writes, normal restore after restart, backup restore after corrupting the latest bank, and restore with the door removed. The restore ran with loading active and viewport input held, and the game released input afterward. |

The packaged example used synthetic keyboard input and a separate save directory. Follow the [example guide](CoreKitExample.md) to reproduce the user flow from the staged project.

`BuildPlugin` also passed for Win64 Editor, Development and Shipping. The distribution contained the sample project, installer, module guides, project skills and the Actions test tags.

## Frontend

The frontend and the updated example built for UE 5.8.3 Win64 Editor and Game Development. A separate rendered package with test saves and synthetic keyboard input exercised:

- Main menu focus, a disabled Continue action on an empty catalog, and focus returning to Settings after leaving the page.
- Draft Apply and Discard, display preview timeout, and VSync, audio preference and binding persistence across separate processes.
- New Game creating an empty session, three explicit writes across two slots, and continued world state after restart. The example also resolved the remapped glyph.
- Continue selecting a verified checkpoint when the newest bank failed its checksum, including a newer checkpoint in another slot and the remaining backup when that other slot was damaged.
- The loading overlay holding input through project readiness and restore, rejecting stale IDs and returning to the title after a readiness failure.
- New Game using the engine's default Game Instance and Game Mode, with the same configured frontend.

The audio check covered the saved volume preference. The sample's gameplay audio is configurable through project sound classes. `BuildPlugin` passed for Win64 Editor, Development and Shipping, and the distribution included the frontend guide and sample input assets.

### Reproduce the frontend flow

Stage the [example project](CoreKitExample.md) or use a consumer project that follows the [frontend setup](CoreKitFrontend.md#start-a-new-project). Package the project for **Win64 Development**. To package the example from PowerShell, use its installed project path and a build directory outside the CoreKit checkout.

```powershell
$engine = 'C:\Program Files\Epic Games\UE_5.8\Engine'
$project = 'C:\Projects\CoreKitStarter\CoreKitExample.uproject'
$archive = 'C:\Builds\CoreKitStarter'

& "$engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$project" `
    -NoP4 -unattended -targetplatform=Win64 -clientconfig=Development `
    -build -cook -stage -pak -archive "-archivedirectory=$archive"
```

Launch the executable with a disposable user directory. The example below uses the packaged game binary directly. For restarts, run only the `Start-Process` line so the game keeps the same profile.

```powershell
$game = Join-Path $archive 'Windows\CoreKitExample\Binaries\Win64\CoreKitExample.exe'
$profile = Join-Path $env:TEMP ('CoreKitFrontendSmoke-' + [guid]::NewGuid().ToString('N'))

Start-Process -FilePath $game -ArgumentList "-UserDir=`"$profile`"" -Wait
```

| Step | Expected result |
| --- | --- |
| Start with the fresh profile | **Menu** appears with initial focus on **New Game**. **Continue** stays disabled when the catalog is empty. |
| Open Settings and change VSync | **Discard** returns to the saved value. **Apply** commits the new value. Back from Settings restores focus to the Settings button. |
| Apply a different window mode or resolution | Wait for the 15-second preview to expire and check that the previous mode and resolution return. |
| Rebind the door to **F** and choose **New Game** | The loading overlay appears. The playground opens and accepts input after arrival. The door glyph shows **F**. The slot directory still contains no checkpoint. |
| Open the door, save, and restart | Wait for the saved message before closing. Relaunch using the same `$profile`. **Continue** becomes available, restores the open door and returns control. The remapped key and applied VSync setting persist. |
| Exercise the project readiness gate in a consumer project | With **Wait For Project Ready** on, delay the response and verify that the overlay keeps gameplay input blocked. Confirm that a stale ID is rejected and that reporting failure returns to the title. |

#### Reproduce backup recovery

Use the same disposable profile and the example's default `example-checkpoint` slot. Save once with the door open, wait for completion, then save again with the door closed. Exit the game. The following PowerShell snippet changes the final byte of the newest bank and leaves its header intact. It is scoped to the disposable profile created above.

```powershell
$tempRoot = [IO.Path]::GetFullPath($env:TEMP)
$testRoot = [IO.Path]::GetFullPath($profile)
if ((Split-Path -Parent $testRoot) -ne $tempRoot.TrimEnd('\') -or
    (Split-Path -Leaf $testRoot) -notmatch '^CoreKitFrontendSmoke-[0-9a-f]{32}$') {
    throw 'Use the disposable profile from this checklist.'
}
$slotDir = Join-Path $testRoot 'Saved\CoreKit\Slots'
$banks = @(Get-ChildItem -LiteralPath $slotDir -File -Filter 'example-checkpoint.*.cks' |
    Where-Object { $_.Name -in @('example-checkpoint.a.cks', 'example-checkpoint.b.cks') } |
    Sort-Object LastWriteTimeUtc -Descending)
if ($banks.Count -ne 2 -or $banks[0].LastWriteTimeUtc -eq $banks[1].LastWriteTimeUtc) {
    throw 'Create two separate checkpoint writes first.'
}
$bytes = [IO.File]::ReadAllBytes($banks[0].FullName)
if ($bytes.Length -lt 64) { throw 'Expected a checkpoint bank with a payload.' }
$bytes[$bytes.Length - 1] = $bytes[$bytes.Length - 1] -bxor 255
[IO.File]::WriteAllBytes($banks[0].FullName, $bytes)
```

Relaunch with the same profile and choose **Continue**. The older checkpoint restores the open door and the playground reports backup recovery. To check selection across multiple slots, start another disposable profile. Have a consumer call **Save Checkpoint** with `alpha`, then `beta`, then `alpha` again. Give the slots distinguishable world states. Adapt the bank filter and exact filenames in the snippet to `example-alpha` or your configured prefix. If the newest `alpha` bank fails checksum verification, Continue should use `beta` when it is newer than the remaining `alpha` bank.

## Repeat the build and Actions suite

From the plugin root, run these PowerShell commands with your project and output paths. The project should have CoreKit enabled and an up-to-date Editor build.

```powershell
$engine = 'C:\Program Files\Epic Games\UE_5.8\Engine'
$project = 'C:\Projects\MyProject\MyProject.uproject'
$package = 'C:\Builds\CoreKit'

& "$engine\Build\BatchFiles\RunUAT.bat" BuildPlugin `
    "-Plugin=$((Resolve-Path .\CoreKit.uplugin).Path)" "-Package=$package" -TargetPlatforms=Win64 -NoP4

& "$engine\Binaries\Win64\UnrealEditor-Cmd.exe" $project `
    -unattended -nop4 -nosplash -nullrhi '-ExecCmds=Automation RunTests CoreKitActions' `
    '-TestExit=Automation Test Queue Empty'
```
