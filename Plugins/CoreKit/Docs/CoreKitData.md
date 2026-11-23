# CoreKitData

Use a **Core Kit Data Container** Data Asset when a consumer needs a configurable set of typed fragments. Each fragment is an inline object owned by the asset. Treat the asset and its fragments as read-only during play, since consumers can share the same instance. A component or other consumer can store mutable gameplay state separately.

| Blueprint node | Result |
| --- | --- |
| **Find Data Fragment** | First compatible fragment in authored order, or `None`. |
| **Find Data Fragments** | All compatible fragments in authored order, or an empty array. |
| **Find Data Container** | Container assigned to the exact tag in a **Core Kit Data Component**, or `None`. |
| **Load Data Containers** | Matching assets and derived classes in object path order. An optional long package path restricts the query recursively, such as `/Game/CoreKitExample`. |
| **Discover Data Containers** | Soft references to matching assets in the same order. The query reads the Asset Registry without loading the objects. |
| **Load Data Containers Async** | Loaded objects and missing references from the selected input. Each output preserves the corresponding input order. |

An empty entry in **Fragments** is an asset validation error. Runtime lookups skip empty entries and accept subclasses of the requested fragment type. A parent Gameplay Tag resolves a container only if the map contains that exact parent tag.

For a container Blueprint subclass, use **Fragment Rules** in Class Defaults to declare a fragment class and its minimum and maximum count. A maximum of `-1` allows any count above the minimum. Derived fragments count as matches. Validate the Data Asset in Editor to see missing or excess fragments by class.

## Blueprint example

1. In `/Game/CoreKitExample`, create `BP_SpeedFragment` as a Blueprint Class derived from **Core Kit Data Fragment**. Add a `Speed` float variable with a default of `600`. Compile and save it.
2. In the same folder, create `DA_Movement` using **Miscellaneous → Data Asset** and select **Core Kit Data Container**. Add an item under **Fragments**, choose `BP_SpeedFragment`, and save the asset.
3. Create an Actor Blueprint named `BP_DataExample`. Add a **Core Kit Data Container** object reference variable named `MovementData`, compile, then assign `DA_Movement` as its default.
4. From `MovementData`, call **Find Data Fragment**. Choose `BP_SpeedFragment` for **Fragment Class**. From **Event BeginPlay**, validate the returned reference and print its `Speed` on the valid branch. Place the actor in the level and start PIE. The expected value is `600`.
5. To try a tag lookup, add a **Core Kit Data Component** to the actor. Create a Gameplay Tag such as `Data.Movement` in Project Settings and map it to `DA_Movement` in **Containers**. Use **Find Data Container** with that tag as the source for the same fragment query.

## Asset discovery in a packaged game

**Load Data Containers** waits for the Asset Registry and loads results synchronously. Call it during setup or loading. Retain returned objects in a reflected reference if the consumer needs them later.

For a larger catalog, use **Discover Data Containers** with a package path, select entries and pass those soft references to **Load Data Containers Async**. Discovery can scan the specified path when called before the initial Asset Registry scan in an uncooked Standalone Game. The missing output includes null paths, absent assets and assets of the wrong type. Retain the returned objects if they are needed after completion.

Discovery sees only assets present in the build. For a discovery-only test, clear the `MovementData` reference and any component map entry that references `DA_Movement`. Add `/Game/CoreKitExample` to **Project Settings → Packaging → Additional Asset Directories to Cook**. Use **Load Data Containers** with class **Core Kit Data Container** and package path `/Game/CoreKitExample`, then query the returned `DA_Movement` for `BP_SpeedFragment`. Package the level for Win64 Development to observe the example's **Print String**. A project can also use Asset Manager cook rules to include its assets. [Unreal Engine asset management](https://dev.epicgames.com/documentation/en-us/unreal-engine/asset-management-in-unreal-engine?application_version=5.8)

## C++ use

Add `CoreKitData` to the consumer's `Build.cs` dependencies. Use a public dependency when the consumer exposes CoreKitData types in public headers. A consumer plugin also declares `CoreKit` in its `.uplugin` dependencies.

```cpp
#include "CoreKitData/CoreKitDataContainer.h"

const UMySpeedFragment* Speed = IsValid(Container)
	? Cast<UMySpeedFragment>(Container->FindDataFragment(UMySpeedFragment::StaticClass()))
	: nullptr;
```

`CoreKitData` exposes `GameplayTags` for the component's public API and keeps Asset Registry access private. The design adapts the DataDriven plugin's authored fragment ordering, exact-tag mapping, and synchronous asset discovery for a new project.
