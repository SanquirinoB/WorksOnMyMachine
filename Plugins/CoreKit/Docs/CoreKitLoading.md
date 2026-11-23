# CoreKitLoading

The Game Instance subsystem combines explicit loading requests with optional map-travel tracking. It shows a project Widget Blueprint while a request is active. The overlay takes local Slate focus and suppresses gameplay input in that viewport. Closing it releases the overlay and restores the prior viewport input setting.

## Blueprint example

1. Create a Widget Blueprint with a full-screen background and a loading message. In the menu, get the **Core Kit Loading Subsystem** and call **Configure Loading Screen** with that class and **Track Map Travel** enabled.
2. For an asynchronous operation, call **Begin Loading** and retain its token. Call **End Loading** with that token on success, failure, or cancellation. Overlapping operations can hold independent tokens.
3. For map travel, call **Open Level** after configuring the screen. Travel hooks hold the overlay until the map finishes loading or Unreal signals travel failure. For a blocking local load, show a manual request before the travel and allow a frame to render. Finish the manual request from the destination or the project's failure path.

The widget should present progress and status without owning loading requests. The module provides an in-game overlay. Its animation follows the game thread during a blocking map load. **Is Loading** reports outstanding requests, while **Is Loading Screen Visible** reports whether the overlay was attached.

## Travel with destination readiness

Use **Open Level With Loading** with a soft World reference after configuring a loading widget. Include the destination in the project's cook. The request waits for Slate to paint the overlay before opening the map. The **Map Loaded** output reports that the destination is available and provides a **Travel Id**.

List additional conditions such as `PlayerReady` or `RestoreComplete` on the node. The destination calls **Resolve Travel Condition** with the travel ID, condition name and outcome. It can read the active ID from the subsystem and capture it before beginning asynchronous work. The **Completed** output reports success after the map and all conditions are ready, or reports travel failure, readiness failure or timeout. Only one managed travel runs per Game Instance.

Use **Begin Loading Request** to attach an owner and reason to manual loading work. **Get Active Loading Requests** shows those details and elapsed time. **On Loading Screen Visibility Changed** signals when the viewport overlay appears or disappears. Manual requests can keep the overlay visible after a managed travel finishes.
