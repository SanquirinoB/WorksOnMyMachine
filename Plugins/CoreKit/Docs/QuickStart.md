# CoreKit Quick Start

CoreKit targets new Unreal Engine 5.8.3 projects on Win64. Enable the plugin once and use the runtime modules your project needs.

For a ready-to-use title screen, follow the [CoreKitFrontend walkthrough](CoreKitFrontend.md). It covers the two startup maps, the first checkpoint, settings, Blueprint hooks and packaging.

1. Close the target Unreal Editor. Put this plugin under `Plugins/CoreKit/` in your project so the descriptor is at `Plugins/CoreKit/CoreKit.uplugin`.
2. Enable **CoreKit** in **Edit → Plugins** and restart. To enable it before launch, add `{ "Name": "CoreKit", "Enabled": true }` to the project's `Plugins` array in its `.uproject` file. For a source build, use a C++ project with the Unreal C++ toolchain installed and build its Editor target.
3. Follow the guide for the feature you want to use.

| Module | Example |
| --- | --- |
| [CoreKitData](CoreKitData.md) | Build a Data Asset from inline fragments and look it up by class or tag. |
| [CoreKitNavigation](CoreKitNavigation.md) | Set menu focus, return to the previous widget, and show a glyph for the active input device. |
| [CoreKitTween](CoreKitTween.md) | Animate menu opacity and move a door using the same interpolation node. |
| [CoreKitLoading](CoreKitLoading.md) | Show an overlay during map travel or an asynchronous operation. |
| [CoreKitSave](CoreKitSave.md) | Save a checkpoint and continue after restarting the game. |
| [CoreKitActions](CoreKitActions.md) | Request and cancel a gameplay action with tag-based admission. |
| [CoreKitFrontend](CoreKitFrontend.md) | Start with New Game, Continue and Settings. Configure two maps and use the built-in menu. |
| [CoreKitBasicUI](CoreKitBasicUI.md) | Reuse the DeadDraw visual UI: HUD, menus, cards, overlays, world signals and portable content. |
| [CoreKitWeapons](CoreKitWeapons.md) | Give, equip, fire and reload reusable weapons from 3D or planar aim input. |

CoreKitFrontend coordinates Navigation, Loading and Save for the built-in menu. Projects with their own flow can call those modules directly. See [validation](Validation.md) for the test coverage, [AGENTS.md](../AGENTS.md) for repository skills, and [the plan](../PLAN.md) for the implemented scope.

## Open the example project

From the plugin root, run the installer to copy the example and the plugin into a separate project. The default destination is a sibling `CoreKitExample` folder. Use `-Destination` to select another new folder.

```powershell
powershell -ExecutionPolicy Bypass -File .\Samples\Install-CoreKitExample.ps1 -Destination 'C:\Projects\CoreKitStarter'
```

Open the printed `.uproject` with UE 5.8.3 and build its Editor target if prompted. Launch the **Menu** map in **Standalone Game**. See the [example walkthrough](CoreKitExample.md) for the assets and checkpoint flow.
