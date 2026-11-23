# CoreKitSave

CoreKit saves a project-owned **Save Game** object to a named local slot. The node serializes the object on the game thread, performs disk I/O asynchronously, and returns to the game thread for completion. A load returns the project's object on success, plus the slot name, UTC timestamp, and CoreKit envelope version.

## Blueprint example

1. Create a Blueprint derived from **Save Game**, with a field such as `CheckpointName`. Create it with **Create Save Game Object** and copy the current checkpoint into the object.
2. Use **Save Core Kit Slot** with the name `profile-1`. Check that **Completed** reports **Success** before offering Continue.
3. Restart and use **Load Core Kit Slot** with the same name. On **Success**, cast **Loaded Game** to the project's Save Game class and use the checkpoint to restore your gameplay state. Handle **Not Found**, **Corrupt**, **Unsupported Version**, **IOError**, and **Invalid Request** separately as needed.

Names use up to 64 ASCII letters, digits, underscores, or hyphens. They are case-insensitive and exclude Windows device names. The payload limit is 32 MiB. Save requests snapshot the object's serialized data on activation. When one request depends on another, start it from the first request's completion.

CoreKit alternates between two files under `Saved/CoreKit/Slots`. Each commit writes and flushes a temporary file before publishing it as the older bank. It preserves the newest valid bank and checks each record's checksum when loading. The envelope is for one running game process and local Win64 storage.

## Slot catalog

**List Slots** reads the headers and merges the two files into one entry per slot, newest first. **Get Slot Info** queries a single case-insensitive name. Each entry reports its own disk result. A header-only result has **Verified** disabled. A successful load verifies the checksum and marks **Recovered Backup** when the other bank was damaged or unreadable.

## Opt-in world state

1. Create a Blueprint subclass of **Core Kit Save Participant Component**. Add it to an actor, select the instance component in the level and use **Generate Persistence Id**. Assign a new ID to any duplicated actor.
2. Implement **Capture Save State** to return a version and an Instanced Struct payload. For a door, store a struct with an `IsOpen` boolean. Implement **Can Restore Save State** to check the payload type and version, then apply the value from **Apply Save State**.
3. Create a **Core Kit World Save Game**, call **Capture World** on the **Core Kit World Save Subsystem**, and save the object after capture succeeds. Use a subclass of the save object for player or checkpoint fields specific to the game.
4. Load the slot, travel to the same map and call **Restore World** after the actors are ready. On **Waiting For Participants**, keep the snapshot and retry after the relevant levels or actors become ready. For optional actors, pass **Allow Missing** when readiness is final and use the returned GUIDs to diagnose skipped participants.

Capture and restore check duplicate GUIDs before dispatching participant callbacks. Restore preflights each ready participant before applying state. Keep preflight hooks read-only. Apply hooks should update domain state idempotently so the project can safely retry its own workflow.
