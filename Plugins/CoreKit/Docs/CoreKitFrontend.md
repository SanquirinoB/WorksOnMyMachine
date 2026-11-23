# CoreKitFrontend

CoreKitFrontend is the opt-in title screen for a local single-player game. It provides **New Game**, **Continue** and **Settings** inside the CoreKit plugin. A project can use its existing Game Instance and Game Mode.

## Start a new project

1. Install and enable CoreKit using the [Quick Start](QuickStart.md). To try the finished flow first, open the [example project](CoreKitExample.md).
2. Create a menu level and a separate gameplay level. In the gameplay level, configure your Game Mode, pawn and Player Start. The menu needs a local player controller.
3. In **Project Settings → Game → CoreKit Frontend**, enable **Enabled** and assign **Main Menu Map** and **First Game Map**. In **Project Settings → Maps & Modes**, set **Game Default Map** to the same menu level. **Editor Startup Map** is optional.
4. Under **Project Settings → Packaging**, include the menu, the first gameplay map and every other level that a checkpoint can reopen. Include optional widget, input and sound assets referenced only by settings through your project's cook rules.
5. Launch the menu in **Standalone Game**. With a fresh save directory, **Continue** starts disabled. **New Game** opens the first map. **Settings** opens the built-in options screen.

The following configuration belongs in the consumer project. Replace the two paths with your own World assets. The values after the final dot are the World object names.

`Config/DefaultGame.ini`

```ini
[/Script/CoreKitFrontend.CoreKitFrontendSettings]
bEnabled=True
MainMenuMap=/Game/Maps/MainMenu.MainMenu
FirstGameMap=/Game/Maps/FirstLevel.FirstLevel
SlotPrefix=mygame
```

`Config/DefaultEngine.ini`

```ini
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Game/Maps/MainMenu
```

### Frontend project settings

| Setting | What it controls |
| --- | --- |
| **Enabled** | Creates the frontend subsystem for a new Game Instance. The default is off. |
| **Main Menu Map** | Opens the frontend when the local controller is ready in this map. Also serves as the destination for **Return to Main Menu**. |
| **First Game Map** | Destination of **New Game**. Assign a different map from the menu. |
| **Menu Widget Class**, **Loading Widget Class** | Optional presentation overrides. Leave them empty to use the built-in widgets. |
| **Require Possessed Pawn** | Waits for the local controller to possess a pawn before completing gameplay arrival. Defaults to off. |
| **Wait For Project Ready** | Adds a project-defined arrival gate for streaming or other initialization. Defaults to off. |
| **Travel Timeout Seconds** | Time limit for managed travel and arrival. Defaults to 30 seconds. |
| **Slot Prefix** | Scopes the checkpoint catalog. A prefix of `mygame` and a slot of `checkpoint` produce `mygame-checkpoint`. Defaults to `corekit`. Keep the prefix stable across releases that should share checkpoints. |
| **Allow Missing Participants** | Restores available participants and reports missing ones when enabled. Defaults to on. |
| **User Volume Mix**, **Master/Music/Effects Sound Class** | Opt-in audio controls. Configure the mix and the sound classes the game uses. |
| **Editable Bindings**, **Concurrent Binding Contexts** | Opt-in input rows and the mapping contexts to check for conflicts. |

## Gameplay and checkpoints

### New Game

The title screen starts managed travel to **First Game Map**. Loading paints its overlay before travel and holds gameplay input until the destination is ready. The frontend waits for Begin Play and a local controller, plus a possessed pawn when required. New Game leaves existing checkpoints available. The first write happens when gameplay explicitly requests it.

Bind **On Game Arrived** from the destination's setup, such as a gameplay Player Controller or HUD at Begin Play. Use the event to show the HUD, start a tutorial or enable game-specific input. **Continuing** is false for New Game and true for Continue. The event belongs to managed travel. For a direct launch into a gameplay map during development, initialize the HUD from the map's normal Begin Play path. For a destination that requires additional initialization before the player gains control, use the readiness gate below.

### Save a checkpoint from Blueprint

1. For each actor that should survive a restart, add a subclass of **Core Kit Save Participant Component**. On the placed component, choose **Generate Persistence Id** and keep that ID stable. Give duplicated actors their own IDs. Implement **Capture Save State**, **Can Restore Save State** and **Apply Save State** using a versioned Instanced Struct. A door can capture a boolean and restore its open state. See the [Save participant contract](CoreKitSave.md#opt-in-world-state).
2. From the checkpoint trigger or a gameplay menu, get **Core Kit Frontend Subsystem**. Bind **On Checkpoint Saved** before making a request. Display the saved or failed result from the event's **Succeeded** pin.
3. Call **Save Checkpoint** with the default slot `checkpoint`, or a stable name such as `chapter-02` if your project keeps several checkpoints. The prefix and slot together follow [CoreKitSave's naming rules](CoreKitSave.md#blueprint-example). Reusing the same name updates that slot. A return value of false means the frontend rejected the request or could not capture the snapshot. A true value means it started the request. Read the outcome from **On Checkpoint Saved**, which can also report a rejected save during the call.
4. After saving, call **Return to Main Menu** from gameplay or restart the executable. The frontend blocks that menu travel while a checkpoint write is in flight. A snapshot contains its world package and the state of actors with save participants. Add participants for any player or world state the game needs to restore.

### Continue

Native pause menus can use `CanRestartFromCheckpoint()` and `RestartFromCheckpoint()` to reopen the current run's last captured or continued snapshot, including its map and participants. New Game clears this in-memory snapshot. Disk persistence is confirmed by **On Checkpoint Saved**. Pass **Queue If Busy** to **Save Checkpoint** for closely spaced triggers. While a write is running the newest queued capture for the same slot replaces the previous queued capture.

The title screen queries slot headers within **Slot Prefix** to decide whether to enable **Continue**. The headers identify candidates. Clicking Continue verifies the stored records, requires a readable world snapshot and opens the saved map. A verified checkpoint in another slot takes precedence if it is newer than a recovered backup.

Continue applies the snapshot after arrival readiness and before the loading overlay releases gameplay input. Read **Get Status** on **On Game Arrived** to display recovery or missing-participant messages. If every candidate fails verification or points to an unavailable map, the title screen reports that no checkpoint could be loaded. If travel or restoration fails after a destination has started loading, the frontend attempts to return to the title with an error message. For details on slot banks and metadata, see [CoreKitSave](CoreKitSave.md#slot-catalog).

### Wait for streamed content

1. Enable **Wait For Project Ready**. In the destination's Begin Play setup, bind **On Prepare Arrival** on the frontend subsystem.
2. When the event fires, store its **Travel Id** with the asynchronous work. If your run state lives in the Game Instance, use **Continuing** to distinguish a new run from a restored run.
3. Once the entry area, required actors and other game-specific prerequisites are ready, call **Resolve Project Ready** with that ID and **Succeeded** set to true. Pass false when preparation fails. Each ID belongs to one travel and accepts one response. The overlay holds input through readiness and checkpoint restoration. The travel times out if the project leaves the signal unresolved.

With **Allow Missing Participants** off, missing actors cause the arrival to fail. Resolve readiness only after the actors you intend to restore are present. [CoreKitLoading](CoreKitLoading.md#travel-with-destination-readiness) documents the underlying travel condition and diagnostics.

## Settings

### Video and ordinary options

The built-in screen stages window mode, resolution, overall graphics quality, frame limit, VSync and configured volume changes in a draft. **Apply** commits the draft. **Discard** restores the current values. Back with a dirty draft leaves Settings open and asks the player to apply or discard changes.

A window mode or resolution change starts a 15-second preview. **Keep display settings** commits it. **Revert display settings**, Back, app deactivation or the timeout restore the previous mode and resolution. Other draft edits remain available after the preview reverts. Test display changes in Standalone or in the packaged game. The built-in mode and resolution controls are disabled in PIE.

### Audio

1. Create a dedicated Sound Mix for user preferences and assign it to **User Volume Mix**.
2. Assign one or more of **Master Sound Class**, **Music Sound Class** and **Effects Sound Class**. Route your audio to those classes and include the mix and classes in the cook.
3. Launch the game and open **Settings**. A configured class gets a volume control. Change a value and choose **Apply**. Check the intended audio route and restart to verify the saved value.

The frontend applies volume preferences at startup and travel. It stores them under `[CoreKit.Frontend.Audio]` in the active `GameUserSettings.ini`, separately from checkpoints.

### Controls

1. In **Project Settings → Enhanced Input → User Settings**, turn on **Enable User Settings**. In the project's Input settings, use **EnhancedPlayerInput** and **EnhancedInputComponent** as the default classes.
2. In your Input Mapping Context, add the action's authored keys and set its **Player Mappable Key Settings**. Add a row to **Editable Bindings** with the label, context, mapping name, zero-based slot and device family. Fill in **Hardware Device Id** when your mapping uses a device-specific identifier.
3. List simultaneously active contexts under **Concurrent Binding Contexts**. The built-in menu checks named player mappings in those contexts and rejects conflicts. Use the game's normal player setup to activate its gameplay contexts.
4. In Settings, select the action and choose **Rebind selected action**. Escape or the controller Menu button cancels capture. Accepted changes save immediately through Enhanced Input. **Reset selected action on all devices** restores the authored mapping row. The ordinary **Discard** action applies to the draft options.
5. For a matching gameplay icon, add a [Core Kit Action Glyph](CoreKitNavigation.md#tracked-focus-and-automatic-glyphs) with the same Input Action and a glyph catalog.

## Customize the menu

Native consumers can embed the built-in options in a custom title or pause screen. Create a separate `UCoreKitFrontendMenu`, call `ConfigureEmbeddedSettings` with a close delegate before attaching, and focus `GetInitialFocus()` after attaching. An optional help text shows the project's controls alongside any configured editable bindings. The host owns the input mode, focus restoration and world pause.

Use **Loading Widget Class** for a custom loading screen. Its job is presentation. The frontend and Loading own the request lifetime.

For a custom title screen, derive a Widget Blueprint from **Core Kit Frontend Menu**, add a Designer root, and assign it to **Menu Widget Class**. Use **Get Frontend** to connect the buttons to **New Game**, **Continue Game**, **Open Settings** and **Close Settings**. In **On Frontend Updated**, use **Get State**, **Can Continue** and **Get Status** to change pages, enable controls and show errors. Override **Get Initial Focus** to return a focusable button for the current page. A custom Designer root supplies its own options controls and presentation. The built-in options screen is part of the default layout.

### Ownership

| Owner | Responsibility |
| --- | --- |
| **CoreKitFrontend** | Owns the title menu and flow states, selects a checkpoint, and coordinates the transition back to gameplay. |
| **CoreKitSave** | Owns slot I/O, verification, backup recovery and the world participant contract. |
| **CoreKitLoading** | Owns map travel, overlay lifetime and viewport input suppression during travel. |
| **CoreKitNavigation** | Owns focus scopes and key capture. Enhanced Input owns the saved control profile. |
| **Consumer project** | Owns its maps, actors, checkpoints, loading presentation and any project-specific arrival work. |

The reference from SignalVeil was the explicit first save, selection of the newest readable checkpoint, destination readiness and the separation of an options draft from saved preferences. CoreKit uses its own Save, Loading and Navigation modules for those behaviors.

### C++ consumers

Add `CoreKitFrontend` to the consumer module's `Build.cs` dependencies and include `CoreKitFrontend/CoreKitFrontendSubsystem.h`. From the Game Instance, get `GetSubsystem<UCoreKitFrontendSubsystem>()`. Bind `OnCheckpointSaved`, `OnPrepareArrival` or `OnGameArrived` before making the corresponding request. For example, the sample binds the delegates at Game Instance startup and builds its playground HUD from `OnGameArrived`.

## Troubleshooting

| Symptom | What to check |
| --- | --- |
| The title screen does not appear | Check **Enabled**, then match **Game Default Map** to **Main Menu Map** and confirm the menu has a local player controller. |
| New Game works in Standalone but fails in the package | Confirm that **First Game Map** and its startup assets are cooked. If **Require Possessed Pawn** is on, verify that the gameplay Game Mode spawns and possesses the expected pawn. |
| Continue is disabled after saving | Check **On Checkpoint Saved** and confirm the same user directory and slot prefix are active. |
| The overlay stays visible at arrival | If the readiness gate is enabled, check that the destination subscribed before **On Prepare Arrival** and resolved the ID it received. Review [Loading diagnostics](CoreKitLoading.md#travel-with-destination-readiness) for any other active request. |
| A placed actor loses its saved state | Check its participant's stable GUID, supported payload version and readiness at restore time. A duplicated actor needs a different GUID. |
| A control row says **Unavailable** | Check **Enable User Settings**, the mapping name, the slot and its device family. Confirm that the Mapping Context is included in the build. |

## Test and package

Run the [packaged smoke checklist](Validation.md#reproduce-the-frontend-flow) before shipping a new consumer project. The [example project](CoreKitExample.md) shows a concrete door participant, loading widget and editable control binding. Use the [Frontend validation record](Validation.md#frontend) for the cases already exercised in UE 5.8.3.
