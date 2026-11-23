# CoreKit example project

The sample is a separate UE 5.8.3 project with editable Blueprints and two maps. The built-in [CoreKitFrontend](CoreKitFrontend.md) supplies the menu, settings and checkpoint flow. A small C++ consumer supplies the playground mechanics. Use the [installer command in the Quick Start](QuickStart.md#open-the-example-project) to stage a fresh copy. Build the Editor target and launch **Menu** in **Standalone Game**.

## Play the example

1. Open **Settings**, choose **Rebind selected action** under **Controls**, and press **F**. The playground glyph shows the new key. **E**, **F**, **G** and **Q** have keyboard glyphs in the sample. The gamepad binding is the left face button. **Reset selected action on all devices** restores the default mappings.
2. Return to the menu and choose **New Game**. The loading widget appears before the playground opens. Toggle the door with the button or the mapped input. The door angle is authored on **BP_ExampleDoor**, and the tween duration comes from the fragment in **DA_Interactions**.
3. Choose **Save checkpoint** and wait for the status message. Restart the game and choose **Continue**. The door restores by its GUID while the overlay holds input. The menu checks for a checkpoint using the slot catalog. The playground reports whether it restored the latest checkpoint, recovered a backup or skipped missing participants.
4. Choose **Close IASA for 2 seconds**, then hold **Block** or Left Shift through recovery. The action starts when IASA reopens and ends on release. Try releasing before the end of recovery. The held action stays inactive.

## Find the configuration

| Item | Where to look |
| --- | --- |
| Startup and gameplay maps | **Menu** and **Playground** in `/Game/CoreKitExample/Maps`. The sample's **Maps & Modes** sets **Menu** as the default map. |
| Frontend options | The sample's [DefaultGame.ini](../Samples/CoreKitExample/Config/DefaultGame.ini) sets the two map assets, **WBP_ExampleLoading**, the slot prefix `example` and the editable door mapping. |
| Gameplay presentation | **BP_ExampleGameInstance** assigns the playground widget, glyph catalog, action class and Enhanced Input assets. The native widget parents build the runtime button layout. |
| Input | **IMC_Example** maps **IA_ExampleDoor** and **IA_ExampleBlock**. **DA_Glyphs** supplies the door glyph. |
| Checkpoint and door state | **BP_ExampleDoor** is a placed save participant. **BP_DoorInteraction** is the fragment in **DA_Interactions** that provides the tween duration. |

The default checkpoint produces banks named `example-checkpoint.a.cks` and `example-checkpoint.b.cks` as the slot receives writes. They live under `Saved/CoreKit/Slots` in the active user directory. Use the [packaged checklist](Validation.md#reproduce-the-frontend-flow) to exercise persistence and recovery with an isolated profile.
