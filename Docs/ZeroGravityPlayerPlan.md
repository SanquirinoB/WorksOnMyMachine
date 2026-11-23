# Zero Gravity Player and Magnetic Platforms

Derelict is a 2.5D side-scrolling game with platforming, metroidvania, and survival elements. Players navigate ruined ships, stations, and the space around them in zero gravity. Magnetic boots and gloves let the character attach to steel and walk or run on floors, walls, and ceilings. Jumping from steel starts a drift that keeps its speed and direction until a force or collision changes it. Throwing objects, including a limited supply of weighted plates, is the primary way to redirect in flight. The mass of the character and each object determines how much their velocities change.

Navigation puzzles include turning corners without touching a surface, grabbing an item in flight and using it to redirect toward a platform or door, and throwing an object at a switch while continuing along a different path.

The setting uses dim lighting and advanced technology that is damaged, largely inoperative, and abandoned. Open hull breaches and gaps between torn sections expose the void and leave ample room for free-flight traversal. Use Metroid Dread as a reference for 2.5D environment composition, side-on framing, and the character's on-screen scale.

The project in this repository starts in `/Game/Maps/L_DHangar` and spawns `BP_DPlayer` through `BP_DGameMode`. The native pawn, `ADPawn`, currently sends horizontal movement and jump input to standalone Mover. `L_DTestLane` holds the movement and camera test fixtures. The Delerict handoff in `MachineVault/VaultIndex/Delerict.md` describes the foundation for that setup. [Pawn](../Source/Delerict/Player/DPawn.cpp) · [Startup configuration](../Config/DefaultEngine.ini) · [Project README](../README.md)

## Free flight and input

1. Start `UDMoverComponent` in built-in Flying mode and keep the standalone backend. Apply the initial capsule settings, rotation, zero gravity override, and spawn plane before Mover captures its initial state. Configure flight with zero acceleration and braking. Supply a zero vector as `DirectionalIntent` on every flight tick so Flying integrates the current velocity. Keep controller rotation disabled, send zero orientation intent, and disable Mover's upright correction so the collision root keeps the configured world rotation. [Mover component](../Source/Delerict/Player/DMoverComponent.cpp) · [Epic Mover input API](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/Mover/FCharacterDefaultInputs?application_version=5.8)
2. Drive the crosshair with the mouse. Deproject its screen position through the active camera and intersect the ray with the pawn's XZ plane. Aim from the pawn toward that point, retaining the last valid direction when the intersection gives no usable aim. Use the right stick as the gamepad aim input. Read the current aim when the player releases Jump or Throw.
3. Build the transient Enhanced Input context from the tuning asset. Map movement to its own `Axis2D` action and use it for surface travel. Bind Run, Jump, Grab, Select Throwable, and Throw separately. Track Jump and Throw press, release, and cancellation so a release commits a charge once and cancellation clears it. Retain input cleanup on unpossession and end play.
4. Keep the capsule as Mover's swept collision shape and keep mesh collision disabled. Flying sweeps and slides the capsule along blocking geometry. Keep the side camera's horizontal velocity look-ahead and move its settings into the tuning asset.

## Throws and shared impulse handling

For the first playable test, give the player a finite stack of weighted plates and one slot for a grabbed item. Select between the plate stack and the grabbed item. A grab transfers an item within reach into the free slot and gives the player and item a shared velocity equal to their combined momentum divided by their total mass. Include the plates and any grabbed item in the player's carried mass.

Hold **Right Mouse Button** to charge the selected throw. Release it to launch that item along the current crosshair aim. Interpolate the separation speed from the tap value to the maximum over the configured charge time. A successful launch consumes one selected item and applies equal and opposite impulses. An empty selection, a canceled charge, or a change of selection during the hold clears the request. Released items remain in the level and can be recovered.

The released item starts with the shared drift velocity plus its share of the throw. The player retains that drift plus recoil. With remaining player and cargo mass `mRemaining`, released item mass `mItem`, and target separation speed `s`, the impulse magnitude is `s / (1/mRemaining + 1/mItem)`. A heavier item produces more recoil at the same separation speed for a fixed remaining mass. Use a plate, a heavier grabbable item, and a switch a thrown item can hit in the test lane.

Route throws, weapon hits, recoil, and magnetic jumps through one pawn impulse path. Project each impulse onto the movement plane, use `Δv = impulse / mass`, and apply the accumulated change once at the next game-thread Mover step. Process grabs, releases, and hits in event order so each uses the mass at the time of the event. Use an additive `FApplyVelocityEffect` in free flight. [Epic velocity effect API](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/Mover/FApplyVelocityEffect?application_version=5.8)

## Weapon impulse types

Group weapons by their primary impulse type in a dedicated weapon tuning asset. Start with **Direct** for the pistol, rifle, and shotgun, and **Radial** for the launcher. Each group has its own direct hit strength, radial hit strength, and shot recoil strength. A radial weapon can still apply a direct impulse when its projectile hits the player.

Use CoreKit's `WeaponId` to select the group and its impact kind to select the strength. Direct impacts push along the incoming direction. Area impacts push away from the blast origin using the direction CoreKit supplies. Apply the impulse once per accepted impact. Apply recoil once per fired shot, opposite the aim used for that shot, without rotating the pawn. Both go through the same mass-based impulse path and magnetic breakaway check. [CoreKit impact types](../Plugins/CoreKit/Source/CoreKitWeapons/Public/CoreKitWeapons/CoreKitWeaponTypes.h) · [CoreKit weapon events](../Plugins/CoreKit/Source/CoreKitWeapons/Private/CoreKitWeaponsComponent.cpp)

Apply a block response on the player capsule for the trace channel configured in the weapon tuning asset. Match that channel to the CoreKit weapon definitions used by the loadout. The default `Pawn` profile ignores `Visibility`. [CoreKit weapon definition](../Plugins/CoreKit/Source/CoreKitWeapons/Public/CoreKitWeapons/CoreKitWeaponDefinition.h)

## Magnetic steel surfaces and charged jumps

Add a platform component that marks a finite steel collision face using platform-local geometry. In free flight, look for the nearest eligible face in the player's XZ lane. Measure the gap from the capsule to the face. When the gap is within `CaptureDistance`, sweep the capsule to the face with the chosen clearance. Enter the Magnetic Surface Mover mode only when the sweep reaches the contact point.

On capture, keep tangential velocity. Use the same tangent movement and swept collision for floors, walls, and ceilings. Movement defaults to walking and holding Run selects the running speed. Use the outward surface normal as the mesh's local up so the boots appear planted when the character is upright, sideways, or upside down. Drive the walk and run blend from speed relative to the face, including on vertical faces. Keep the capsule's world rotation fixed and hold the mesh orientation it had on departure throughout free flight. [Current animation instance](../Source/Delerict/Player/DLocomotionAnimInstance.cpp)

Store the contact point in platform-local space so a translating platform can carry the pawn. Sweep that carry motion, keep the XZ plane constraint, and release if the surface disappears or safe contact is lost. Built-in Flying clears its movement base each tick, so surface contact belongs to the magnetic mode. [Mover sync state](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/Mover/FMoverDefaultSyncState?application_version=5.8)

Hold **Jump** while attached to charge the jump impulse from the tap value to the maximum. On release, return to Flying with the current world velocity, including any platform carry, plus the charged impulse. Measure the aim angle from the surface tangent in XZ. Use the crosshair direction when it meets the configured minimum outward angle of **20°** or more. Clamp shallower or inward aim to the nearest permitted outward angle on the aimed side. If that aim has no usable tangent, launch along the outward normal. Losing surface contact cancels a pending jump.

Walking or running off the end of a face returns the player to Flying. While attached, the tangential part of throw recoil or a weapon impact changes travel speed. An outward velocity change that exceeds the breakaway threshold returns the player to Flying and applies the full in-plane change once. Give departure priority over capture in the same step and rearm the departed face once the capsule clears the recapture distance.

## Tuning data assets

Every variable in both assets, including each nested struct field, gets an explicit default and a descriptive `/** ... */` comment directly above its `UPROPERTY`. Describe what the value controls, its units, and the behavior at zero or at a threshold where relevant. Verify each description is visible as a tooltip in the asset's Details panel. Set required asset references to usable values in the shipped assets. Document any optional reference that defaults to `nullptr`.

### Player tuning

Ship `DA_DPlayerTuning` and reference it from `BP_DPlayer`. Remove duplicate editable values from the pawn and apply the asset on every spawn. Platform instances supply their face geometry and placement, and item instances identify their type. The player asset supplies movement, player mass, throw tuning, and magnetic behavior. Capsule dimensions and camera look-ahead use the current pawn's values. Impulse magnitudes are in kg·cm/s so dividing by mass in kilograms gives a velocity change in cm/s.

| Field | Default | Designer description |
| --- | --- | --- |
| `GravityAcceleration` | `(0, 0, 0)` cm/s² | Gravity override reported by Mover. The zero vector gives the player zero gravity. |
| `LockedRotation` | `(0, 0, 0)` degrees | Fixed capsule world rotation and initial mesh orientation before the first attachment. |
| `MovementPlaneNormal` | `(0, 1, 0)` | Normal of the plane through the spawn location. This default limits movement to X/Z. |
| `MoveLeftKeys` | `A`, Left Arrow | Keys that request travel toward negative world X while attached. |
| `MoveRightKeys` | `D`, Right Arrow | Keys that request travel toward positive world X while attached. |
| `MoveUpKeys` | `W`, Up Arrow | Keys that request travel toward positive world Z while attached. |
| `MoveDownKeys` | `S`, Down Arrow | Keys that request travel toward negative world Z while attached. |
| `MoveStickKey` | Left Stick 2D | Stick that requests travel along a magnetic face. |
| `MoveStickDeadZone` | `0.2` | Move-stick magnitude below which input is ignored, on a scale of 0 to 1. |
| `RunKeys` | Left Shift, left stick click | Hold to use running speed while attached. |
| `AimStickKey` | Right Stick 2D | Gamepad stick that sets the XZ aim direction used by jumps and throws. |
| `AimStickDeadZone` | `0.2` | Aim-stick magnitude below which the last valid aim is retained, on a scale of 0 to 1. |
| `InitialAimDirection` | `(1, 0, 0)` | World direction used until the player first aims. It defaults to positive X. |
| `GrabKeys` | `E`, left shoulder button | Buttons that grab a reachable throwable when the grabbed-item slot is empty. |
| `GrabDistance` | `60` cm | Maximum gap from the player capsule to a throwable that the player can grab. |
| `SelectThrowableKeys` | `Q`, D-pad Up | Buttons that cycle between available throwable selections. |
| `ThrowKeys` | Right Mouse Button, right shoulder button | Hold to charge the selected item and release to throw it. |
| `JumpKeys` | Space, bottom gamepad face button | Hold to charge while attached and release to jump along the permitted aim direction. |
| `CapsuleRadius` | `34` cm | Radius used by pawn collision sweeps and magnetic distance checks. |
| `CapsuleHalfHeight` | `88` cm | Distance from the capsule center to either end, including the rounded cap. |
| `CapsuleCollisionProfile` | `Pawn` | Base collision profile used by the player capsule. |
| `WeaponImpulseTuning` | `DA_DWeaponImpulseTuning` | Weapon impulse asset assigned in the player's Weapons category. |
| `PlayerMass` | `80` kg | Character mass before adding plates or a grabbed item. Use a positive value. |
| `ThrowableMassByType` | `WeightedPlate` = `2` kg, `Heavy` = `8` kg | Mass of each throwable type. Every throwable used in the level needs a positive entry. |
| `StartingPlateType` | `WeightedPlate` | Throwable type placed in the player's starting plate stack. Looks up mass in `ThrowableMassByType`. |
| `StartingPlateCount` | `3` | Number of plates granted at spawn. Zero starts the player without plates. |
| `ThrowChargeTime` | `0.65` s | Hold time to reach full throw strength. Extra hold time maintains the maximum. |
| `MinThrowSeparationSpeed` | `400` cm/s | Relative player-to-item speed immediately after a tap throw in free flight. |
| `MaxThrowSeparationSpeed` | `1,600` cm/s | Relative player-to-item speed immediately after a fully charged throw in free flight. |
| `CaptureDistance` | `60` cm | Maximum gap between the capsule and a magnetic face at which capture can occur. |
| `ContactClearance` | `2` cm | Gap kept between the capsule and its attached face during travel. |
| `SurfaceWalkSpeed` | `200` cm/s | Speed full tangential input targets while walking on a magnetic face. |
| `SurfaceRunSpeed` | `350` cm/s | Speed full tangential input targets while holding Run on a magnetic face. |
| `SurfaceAcceleration` | `1,500` cm/s² | Rate at which input accelerates the player along a magnetic face. |
| `SurfaceBraking` | `1,500` cm/s² | How quickly attached travel slows when above the speed requested by input. |
| `JumpChargeTime` | `0.70` s | Hold time to reach full jump impulse. Extra hold time maintains the maximum. |
| `MinJumpImpulseMagnitude` | `8,000` kg·cm/s | Impulse added by a tap jump along the permitted launch direction. |
| `MaxJumpImpulseMagnitude` | `16,000` kg·cm/s | Impulse added by a fully charged jump along the permitted launch direction. |
| `MinJumpAngleDegrees` | `20` degrees | Minimum outward launch angle from the current surface tangent. Valid range is 20–90 degrees. |
| `BreakawayDeltaSpeed` | `200` cm/s | Outward velocity change that a throw recoil or weapon impact must exceed to break contact. |
| `RecaptureDistance` | `80` cm | Gap the player must clear before the same face can capture them again. Set it above `CaptureDistance`. |
| `CameraLookAheadTime` | `0.18` s | Multiplies the player's X velocity to determine the camera lead. |
| `CameraMaxLookAhead` | `90` cm | Maximum horizontal camera lead in either direction. Zero removes look-ahead. |

### Weapons tuning

Ship `DA_DWeaponImpulseTuning` with the shared trace channel, weapon group assignments, and both impulse profiles. CoreKit weapon definitions continue to supply firing, damage, and delivery settings. The values below are starting points for playtesting.

| Field | Default | Designer description |
| --- | --- | --- |
| `WeaponTraceChannel` | `Visibility` | Trace channel the player capsule blocks for incoming weapon hits. Match the equipped weapon definitions. |
| `WeaponImpulseTypeById` | `Pistol`, `Rifle`, `Shotgun` → Direct. `Launcher` → Radial. | Selects an impulse profile by CoreKit weapon ID. |
| `DefaultWeaponImpulseType` | Direct | Profile used when a weapon ID has no assignment. |
| `DirectProfile` | Direct defaults below | Impact and recoil magnitudes for weapons in the Direct group. |
| `RadialProfile` | Radial defaults below | Impact and recoil magnitudes for weapons in the Radial group. |

| Profile field | Direct default | Radial default | Designer description |
| --- | --- | --- | --- |
| `DirectHitImpulseMagnitude` | `24,000` kg·cm/s | `24,000` kg·cm/s | Impulse per accepted direct hit along the incoming direction. Zero disables direct knockback. |
| `RadialHitImpulseMagnitude` | `0` kg·cm/s | `24,000` kg·cm/s | Impulse per accepted area hit away from the blast origin. Zero disables radial knockback. |
| `ShotRecoilImpulseMagnitude` | `8,000` kg·cm/s | `8,000` kg·cm/s | Impulse per fired shot opposite the shot aim. Zero disables weapon recoil movement. |

## Development gameplay diagnostics

Add a small native debug helper to the `Delerict` module. In `Delerict.Build.cs`, define its compile flag from `Target.Configuration == UnrealTargetConfiguration.Development`. Gate console registration, diagnostic data collection, and drawing with that flag. This enables the system in Development Editor sessions and packaged Development games. [Unreal build configurations](https://dev.epicgames.com/documentation/unreal-engine/build-configurations-reference-for-unreal-engine) · [Delerict build rules](../Source/Delerict/Delerict.Build.cs)

Register `Delerict.Debug` with `FAutoConsoleCommandWithWorldArgsAndOutputDevice`. Track enabled channels for the current game world, starting with all channels off. Turn on several channels in one command or add and remove individual channels while playing. `List` prints the active channels and available names to the console. [Epic console command API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Core/FAutoConsoleCommandWithWorldArgs-)

```text
Delerict.Debug On Flight Aim Collision
Delerict.Debug On Magnet Jump
Delerict.Debug Off Aim
Delerict.Debug List
Delerict.Debug On All
Delerict.Debug Off
```

Draw arrows, lines, capsule outlines, and short-lived labels through `UKismetSystemLibrary`. Draw the actual inputs, sweep results, and finalized movement values used by gameplay. Build diagnostics only for enabled channels and render them on the game thread. Use a distinct label and color for each channel so several can be read together. Stop refreshing a channel when it is disabled and let its drawings expire. The command controls visualization and reads the resolved tuning values. [Epic Kismet System Library API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UKismetSystemLibrary?application_version=5.8)

| Channel | Values and drawings |
| --- | --- |
| `Tuning` | Resolved player and weapon asset names, active tuning values, and the weapon profile in use. |
| `Flight` | Mover mode, gravity, XZ velocity vector and speed, effective mass, accumulated impulse and resulting velocity change, and camera lead. |
| `Aim` | Crosshair ray, intersection with the movement plane, resolved aim vector, and active input source. |
| `Collision` | Player capsule, attempted sweep, blocking point and normal, and resulting slide. |
| `Throws` | Selection, plate count, grabbed item mass, charge and launch speed, item and player recoil vectors, and switch hit result. |
| `Weapons` | Weapon ID, resolved impulse profile and impact kind, hit origin or shot aim, impulse vector, and applied velocity change. |
| `Magnet` | Steel face candidate, measured gap and capture distance, snap result, normal and tangent, walk or run speed, and carry or breakaway result. |
| `Jump` | Hold time and charge, raw and clamped aim vectors, launch angle and limit, and applied impulse or cancellation reason. |

Each gameplay commit includes its channel instrumentation and a console check that makes the feature's values visible in play.

## Commit plan

| Commit | Depends on | Scope | Acceptance check |
| --- | --- | --- | --- |
| 1. Add Development-only Kismet gameplay diagnostics | Current project | Add the build flag, console command, per-world channel state, and shared Kismet drawing helpers. | Use `List` to verify the initial state, two channels enabled together, one disabled independently, and all cleared with `Off`. Verify a Shipping build omits the command. |
| 2. Add player and weapon impulse tuning assets | 1 | Add the reflected schemas, explicit defaults and comments, both assets, and the player asset's Weapons reference. Add the `Tuning` readout. | Check asset defaults and tooltips in the editor. Confirm `Tuning` shows the resolved asset names and values. |
| 3. Add inertial flight and the shared impulse path | 2 | Own `DPawn` and `DMoverComponent` changes. Add Flying startup, fixed collision rotation, mouse crosshair and XZ aim, camera tuning, and ordered impulse processing. Expose move, run, and charge hooks. Add `Flight`, `Aim`, and `Collision` diagnostics. | Check rest, drift while aiming, fixed rotation, lane confinement, wall sweeps, camera follow, and combined impulses. Enable all three channels together and verify a blocked sweep shows the actual contact and slide. |
| 4. Add charged throws and switch interaction | 3 | Add a plate stack and grabbed-item slot, item selection, grab momentum, and hold-and-release throws. Add `Throws` diagnostics. | Check right mouse press and release, tap and full charge, cancellation, plate depletion, mass-dependent recoil, momentum conservation, and a switch hit. Verify `Throws` shows charge, selection, inventory, and launch results. |
| 5. Route CoreKit weapon impulses by profile | 3 | Add the CoreKit consumer integration. Resolve weapon IDs, dispatch direct and radial hits, and apply recoil once per shot. Add `Weapons` diagnostics. | Check direct and radial directions, launcher direct hits, default profile lookup, and recoil. Verify `Weapons` shows the chosen profile and the actual impulse vector for each event. |
| 6. Add magnetic walking, running, and charged jumps | 3 | Add the steel face component and Magnetic Surface mode, including swept capture, posture, walk and run, carry, and aimed jump release. Add `Magnet` and `Jump` diagnostics. | Check floor, wall, and ceiling travel, blocked snaps, posture, platform carry, tap and full-charge jumps, cancellation, edge release, recapture, and breakaway. Verify `Magnet` shows the capture result and `Jump` shows raw aim, clamped aim, and the 20° limit. |
| 7. Integrate and validate the traversal prototype | 4, 5, 6 | Wire the three features into `BP_DPlayer`, finish the test lane fixtures, and update the controls, debug command examples, and level design lab route. | Build `DelerictEditor`, run the movement and lab route tests, and verify `L_DHangar` resolves both tuning assets. Play the full route with combined channels in a packaged Development game and confirm the same movement results with diagnostics enabled and disabled. |

The shared hooks pass aim, movement, and button phases from input. Process grab, jump release, throw release, and weapon events in order using world-space impulse vectors in kg·cm/s. The movement step owns the mass conversion, plane projection, and final velocity update. [Current movement test](../Source/Delerict/Player/Tests/DMovementTests.cpp) · [Current lab route test](../Source/Delerict/LevelDesign/Tests/DLevelDesignTests.cpp) · [Level design lab](LevelDesign.md)

### Parallel work

- **While commits 1–3 are in progress**, develop throw charge and momentum calculations, weapon profile lookup and CoreKit events, and steel face geometry with jump-angle clamp tests. Exercise each with isolated fixtures until the shared movement and debug hooks are ready. Level design can block out an open gap, a switch approach, and steel faces in floor, wall, and ceiling orientations in a separate working map.
- **After commit 3**, develop and merge commits 4, 5, and 6 independently. They share the movement hooks, data schemas, and debug helper. Keep each feature's diagnostics and tests with its code so the core pawn, Mover, and shared assets have one owner during development.
- **After commits 4–6 merge**, wire the production pawn and save the shared Blueprint and map packages in commit 7. Run the full route with multiple debug channels and check weapon and throw recoil while magnetically attached.
