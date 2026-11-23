# CoreKitNavigation

Use standard UMG navigation rules for directional movement and Enhanced Input for action bindings. CoreKit tracks the last device per local player, resolves project-authored key glyphs, and restores focus when nested menus close.

## Blueprint example

1. Create a Widget Blueprint with two focusable buttons. Set its directional Navigation rules as needed. Add it to the owning player's screen and set the input mode appropriate for the menu.
2. Get the owning Local Player's **Core Kit Navigation Subsystem**. Enable **Set Device Tracking Enabled** and call **Push Menu Focus** with the first button after attaching the widget.
3. Open a second widget and push focus to its first button. When closing it, call **Pop Menu Focus** once and remove the widget. Close menus in reverse opening order.
4. Create a **Core Kit Navigation Glyphs** Data Asset and assign textures by key. Use **EnhancedPlayerInput** and **EnhancedInputComponent** in the project's Input settings. Add your Enhanced Input mapping context through the Enhanced Input local player subsystem. On **On Input Device Changed**, call **Find Action Glyph** for the action and apply the texture to an Image. Refresh the same way when the project changes bindings.

Check that the first button receives focus, the previous button receives focus after closing a submenu, and a key, mouse click, or gamepad input updates the active device. The project owns the input mode, mapping contexts, cursor visibility, and widget lifetime. Use **Find Glyph** to resolve a specific key directly.

## Tracked focus and automatic glyphs

Use **Push Menu Focus Scope** with a focusable child and its owning menu widget. Retain the returned ID to release that menu independently. Passing the owner also releases the scope on widget destruction. An earlier menu can close while a later menu keeps focus. Releasing the last scope restores the best available prior focus.

Place **Core Kit Action Glyph** in the Widget Blueprint and assign an Input Action and glyph Data Asset. With device tracking enabled, it refreshes when the input device changes and when Enhanced Input rebuilds control mappings.

## Rebind controls

Enable Enhanced Input user settings and register the project's player-mappable mapping contexts. Call **Begin Key Capture** and read **On Key Capture Finished**. Escape or **Cancel Key Capture** closes capture. Use the returned key in **Apply Key Binding** with the mapping name, slot, hardware device and optional profile ID. Pass the other contexts that can be active with the target context.

A conflict returns the conflicting mapping names. Offer an interchange when there is one compatible conflict and retry with **Swap** after the player confirms. Call **Restore Binding Defaults** for the mapping row to restore its authored keys. Accepted changes use the player's Enhanced Input settings for persistence.

## Project icon sets and gamepad types

Create one **Core Kit Input Icon Set** Data Asset for keyboard and mouse, and one per gamepad type (for example Xbox, PlayStation, Switch). Each entry maps a key to an icon and an optional display name. In **Project Settings > Plugins > CoreKit Navigation**, assign **Keyboard Mouse Icons**, fill **Gamepad Icons** by type name and set **Default Gamepad Type**.

The subsystem reads the most recently used hardware device from the Input Device subsystem and matches **Hardware Substrings** (case-insensitive) against the hardware identifier and input class to pick the type. Unidentified hardware, such as plain XInput on Windows, uses the default type. Call **Set Gamepad Type Override** from a settings menu to force a type; pass None to return to detection. **On Gamepad Type Changed** fires only when the effective type changes.

**Find Key Presentation** returns the icon and name for a key from the set for its device family, falling back to the default type's set and then to the engine key name. **Find Action Presentation** picks a key mapped to the action for the current device family, preferring one with an icon.

**Core Kit Action Glyph** without a glyph Data Asset now uses these project sets. Place **Core Kit Input Prompt** for an icon with a text fallback: assign an Input Action or a Key. It builds an Image + Text row, or binds widgets named **Icon** and **Label** in a subclass layout. It refreshes on device, gamepad type and mapping changes. Enable **Always Show Label** to show the name next to the icon. Use **On Presentation Changed** for custom styling.

Automation tests: `CoreKit.Navigation.*` in the DeadDraw validation module.

## Menus, layers and Return

Derive menus from **Core Kit Menu Widget**. Push them with **Push Menu** on a layer (**Game**, **Menu** or **Modal** by default, configured in **Project Settings > Plugins > CoreKit Navigation > Menus**). The top menu (highest layer, latest pushed) owns:

- **Focus**: its **Get Initial Focus** widget receives focus; closing it restores the previous focus.
- **Input mode and cursor**: **Input Mode** (Game, Game And Menu, Menu) and **Show Cursor** are applied to the player controller; the game's input returns when the last menu closes. With **Auto Hide Cursor With Gamepad**, the cursor hides while a gamepad is in use.
- **Return** (Escape / B by default) calls **Handle Return**, which closes the menu unless you override it. **Next/Previous Section** (E/Q, RB/LB) call **Handle Section**. Any other key reaches **Handle Menu Key**. These keys are read before widgets, so they also work while the game is paused.
- **Activation cooldown**: for **Activation Input Cooldown** seconds after becoming the top menu, Return and accept keys are ignored and Core Kit Buttons do not activate.

Optional animations named **Anim_In** and **Anim_Out** play when the menu is pushed and before it is removed. **Pop Menu** or **Close Menu** removes it; **Pop All Menus** clears a layer.

### Buttons, tabs, prompts and value controls

- **Core Kit Button**: hover focuses it, and focus uses the hovered style, so only one element looks selected. **On Selected**, **On Deselected** and **On Activated** report the state; navigation and activation sounds come from the project settings. Use **Set Base Style** instead of Set Style.
- **Core Kit Section List**: one tab button per section, optional **Widget Switcher** sync, wrap-around, **On Section Changed**. The owning menu's section keys drive it.
- **Core Kit Prompt Widget**: title, body and options. Use **Push Prompt** on the subsystem; the callback runs once, Return picks the cancel option.
- **Core Kit Option Selector**: segmented or ◀ value ▶ control; left/right navigation changes the value; **On Value Changed** reports it. Sliders keep UMG's own keyboard/gamepad stepping.

Automation tests: `CoreKit.Navigation.*` in the DeadDraw validation module.
