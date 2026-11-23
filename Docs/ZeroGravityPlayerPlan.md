# Zero Gravity Player and Magnetic Platforms

Derelict is a 2.5D side-scrolling game with platforming, metroidvania, and survival elements. Players navigate ruined ships, stations, and the space around them in zero gravity. A push-off carries the player in a straight line until another force or a collision changes their velocity. There is no gravitational up or down. Throwing objects from a limited supply is the primary way to change direction in free flight. The mass of the character and each object determines how much their velocities change.

Navigation puzzles include turning corners without touching a surface, grabbing an item in flight and using it to redirect toward a platform or door, and throwing an object at a switch while continuing along a different path.

The setting uses dim lighting and advanced technology that is damaged, largely inoperative, and abandoned. Open hull breaches and gaps between torn sections expose the void and leave ample room for free-flight traversal. Use Metroid Dread as a reference for 2.5D environment composition, side-on framing, and the character's on-screen scale.

The project in this repository starts in `/Game/Maps/L_DHangar` and spawns `BP_DPlayer` through `BP_DGameMode`. The native pawn, `ADPawn`, currently sends horizontal movement and jump input to standalone Mover. `L_DTestLane` holds the movement and camera test fixtures. The Delerict handoff in `MachineVault/VaultIndex/Delerict.md` describes the foundation for that setup. [Pawn](../Source/Delerict/Player/DPawn.cpp) · [Startup configuration](../Config/DefaultEngine.ini) · [Project README](../README.md)

## Free flight and input

1. Start `UDMoverComponent` in built-in Flying mode and keep the standalone backend. Apply the initial capsule settings, rotation, zero gravity override, and spawn plane before Mover captures its initial state. Configure flight with zero acceleration and braking. Supply a zero vector as `DirectionalIntent` on every flight tick so Flying integrates the current velocity. Keep controller rotation disabled, send zero orientation intent, and disable Mover's upright correction so the pawn keeps the configured world rotation. [Mover component](../Source/Delerict/Player/DMoverComponent.cpp) · [Epic Mover input API](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/Mover/FCharacterDefaultInputs?application_version=5.8)
2. Build the transient Enhanced Input context from the tuning asset. Map one `Axis2D` action to the world XZ plane. In flight it controls aim, which holds its last valid direction when input returns to neutral. In Magnetic Surface mode its tangential component also controls travel. Use the former jump binding to push off a magnetic face and use dedicated actions to grab and throw. Retain input cleanup on unpossession and end play.
3. Keep the capsule as Mover's swept collision shape and keep mesh collision disabled. Flying sweeps and slides the capsule along blocking geometry. Keep the side camera's horizontal velocity look-ahead and move its settings into the tuning asset.

## Throws and shared impulse handling

Use one held-item slot for the first playable test. A grab transfers an item within reach into that slot and gives the player and item a shared velocity equal to their combined momentum divided by their total mass. A throw empties the slot and applies equal and opposite impulses. The released item starts with the shared drift velocity plus its share of the throw. The player retains that drift velocity plus the recoil. With player mass `mPlayer`, item mass `mItem`, and target separation speed `s`, the impulse magnitude is `s / (1/mPlayer + 1/mItem)`. A heavier item produces more recoil at the same separation speed. To throw again, the player must reach another item or recover the one they threw.

Route throws, weapon hits, recoil, and surface push-off through one pawn impulse path. Project each impulse onto the movement plane, use `Δv = impulse / mass`, and apply the accumulated change once at the next game-thread Mover step. Process grabs, releases, and hits in event order so each uses the mass at the time of the event. Include held mass for external impacts and exclude the released item from the player's side of a throw. Use an additive `FApplyVelocityEffect` in free flight. [Epic velocity effect API](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/Mover/FApplyVelocityEffect?application_version=5.8)

Start with two test items of different masses and a switch a thrown item can hit.

## Weapon impulse types

Group weapons by their primary impulse type in a dedicated weapon tuning asset. Start with **Direct** for the pistol, rifle, and shotgun, and **Radial** for the launcher. Each group has its own direct hit strength, radial hit strength, and shot recoil strength. A radial weapon can still apply a direct impulse when its projectile hits the player.

Use CoreKit's `WeaponId` to select the group and its impact kind to select the strength. Direct impacts push along the incoming direction. Area impacts push away from the blast origin using the direction CoreKit supplies. Apply the impulse once per accepted impact. Apply recoil once per fired shot, opposite the aim used for that shot, without rotating the pawn. Both go through the same mass-based impulse path and magnetic breakaway check. [CoreKit impact types](../Plugins/CoreKit/Source/CoreKitWeapons/Public/CoreKitWeapons/CoreKitWeaponTypes.h) · [CoreKit weapon events](../Plugins/CoreKit/Source/CoreKitWeapons/Private/CoreKitWeaponsComponent.cpp)

Apply a block response on the player capsule for the trace channel configured in the weapon tuning asset. Match that channel to the CoreKit weapon definitions used by the loadout. The default `Pawn` profile ignores `Visibility`. [CoreKit weapon definition](../Plugins/CoreKit/Source/CoreKitWeapons/Public/CoreKitWeapons/CoreKitWeaponDefinition.h)

## Magnetic platforms

Add a platform component that marks a finite collision face using platform-local geometry. In free flight, look for the nearest eligible face in the player's XZ lane. Measure the gap from the capsule to the face. When the gap is within `CaptureDistance`, sweep the capsule to the face with the chosen clearance. Enter the Magnetic Surface Mover mode only when the sweep reaches the contact point.

On capture, keep the velocity along the face and discard the normal component. In Magnetic Surface mode, project input and retained velocity onto the face tangent and continue moving with capsule sweeps. Store the contact point in platform-local space so a translating platform can carry the pawn. Sweep that carry motion as well, keep the pawn's world rotation and plane constraint, and release if the surface disappears or safe contact is lost. Built-in Flying clears its movement base each tick, so surface contact belongs to the magnetic mode. [Mover sync state](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/Mover/FMoverDefaultSyncState?application_version=5.8)

Return to Flying on manual release or when movement leaves the end of the face. Manual release applies an outward impulse. While attached, the tangential part of a throw recoil or weapon impact changes travel speed. When the outward velocity change exceeds the breakaway threshold, return to Flying and apply the full in-plane velocity change once. Give release priority over capture in the same step and rearm the released platform once the capsule clears the recapture distance.

## Tuning data assets

Every variable in both assets, including each nested struct field, gets an explicit default and a descriptive `/** ... */` comment directly above its `UPROPERTY`. Describe what the value controls, its units, and the behavior at zero or at a threshold where relevant. Verify each description is visible as a tooltip in the asset's Details panel. Set required asset references to usable values in the shipped assets. Document any optional reference that defaults to `nullptr`.

### Player tuning

Ship `DA_DPlayerTuning` and reference it from `BP_DPlayer`. Remove duplicate editable values from the pawn and apply the asset on every spawn. Platform instances supply their face geometry and placement, and item instances identify their type. The player asset supplies movement, player mass, throw tuning, and magnetic behavior. Capsule dimensions and camera look-ahead use the current pawn's values. Impulse magnitudes are in kg·cm/s so dividing by mass in kilograms gives a velocity change in cm/s.

| Field | Default | Designer description |
| --- | --- | --- |
| `GravityAcceleration` | `(0, 0, 0)` cm/s² | Gravity override reported by Mover. The zero vector gives the player zero gravity. |
| `LockedRotation` | `(0, 0, 0)` degrees | World rotation the capsule and player visual hold during movement. |
| `MovementPlaneNormal` | `(0, 1, 0)` | Normal of the plane through the spawn location. This default limits movement to X/Z. |
| `AimLeftKeys` | `A`, Left Arrow | Keys for negative world X when aiming or traveling along a magnetic face. |
| `AimRightKeys` | `D`, Right Arrow | Keys for positive world X when aiming or traveling along a magnetic face. |
| `AimUpKeys` | `W`, Up Arrow | Keys for positive world Z when aiming or traveling along a magnetic face. |
| `AimDownKeys` | `S`, Down Arrow | Keys for negative world Z when aiming or traveling along a magnetic face. |
| `AimStickKey` | Left Stick 2D | Stick used to aim or travel along a magnetic face. |
| `AimStickDeadZone` | `0.2` | Stick magnitude below which the input is ignored, on a scale of 0 to 1. |
| `InitialAimDirection` | `(1, 0, 0)` | World direction used until the player first aims. It defaults to positive X. |
| `GrabKeys` | `E`, left shoulder button | Buttons that grab a reachable throwable when the player's slot is empty. |
| `GrabDistance` | `60` cm | Maximum gap from the player capsule to a throwable that the player can grab. |
| `ThrowKeys` | Left mouse button, right shoulder button | Buttons that throw the held item along the current aim. |
| `ReleaseKeys` | Space, bottom gamepad face button | Buttons that push the player off a magnetic face. |
| `CapsuleRadius` | `34` cm | Radius used by pawn collision sweeps and magnetic distance checks. |
| `CapsuleHalfHeight` | `88` cm | Distance from the capsule center to either end, including the rounded cap. |
| `CapsuleCollisionProfile` | `Pawn` | Base collision profile used by the player capsule. |
| `WeaponImpulseTuning` | `DA_DWeaponImpulseTuning` | Weapon impulse asset assigned in the player's Weapons category. |
| `PlayerMass` | `80` kg | Character mass without a held item. Use a positive value. |
| `ThrowableMassByType` | `Light` = `2` kg, `Heavy` = `8` kg | Mass of each throwable type. Every throwable used in the level needs a positive entry. |
| `ThrowSeparationSpeed` | `1,600` cm/s | Relative speed between player and item immediately after release in free flight. |
| `CaptureDistance` | `60` cm | Maximum gap between the capsule and a magnetic face at which capture can occur. |
| `ContactClearance` | `2` cm | Gap kept between the capsule and its attached face during travel. |
| `SurfaceSpeed` | `350` cm/s | Speed full tangential input targets on a magnetic face. |
| `SurfaceAcceleration` | `1,500` cm/s² | Rate at which input accelerates the player along a magnetic face. |
| `SurfaceBraking` | `1,500` cm/s² | How quickly attached travel slows when above the speed requested by input. |
| `ReleaseImpulseMagnitude` | `16,000` kg·cm/s | Outward impulse when the player manually pushes off a magnetic face. |
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

## Commit plan

| Commit | Depends on | Scope | Acceptance check |
| --- | --- | --- | --- |
| 1. Add player and weapon impulse tuning assets | Current project | Add the reflected schemas, explicit defaults and comments, both data assets, and the player asset's Weapons reference. | Both assets open in the editor with readable tooltips, populated defaults, and a valid Weapons reference. |
| 2. Add inertial flight and the shared impulse path | 1 | Own `DPawn` and `DMoverComponent` changes. Add the Flying startup, fixed rotation, capsule settings, XZ aim input, camera tuning, and ordered impulse processing. Expose the shared input and motion hooks used by commits 3–5. | Replace the ground checks with tests for rest, constant drift while aiming, fixed rotation, lane confinement, swept collision, camera follow, and two impulses in one step. |
| 3. Add throwable momentum and switch interaction | 2 | Add the throwable actor, grab and throw behavior for one held slot, and a test switch. | Verify free-flight momentum conservation, different recoil for light and heavy items, an empty slot after a throw, and a switch hit from a diverging path. |
| 4. Route CoreKit weapon impulses by profile | 2 | Add the CoreKit consumer integration. Resolve weapon IDs, dispatch direct and radial hits, and apply recoil once per shot. | Verify direct and radial directions, launcher direct hits, default profile lookup, and recoil without a pawn turn. |
| 5. Add magnetic surface movement | 2 | Add the platform face component and Magnetic Surface mode, including swept capture, tangential movement, carry, release, and breakaway. | Verify blocked snaps, vertical travel, carry, manual and edge release, recapture clearance, and impulses on either side of the breakaway threshold. |
| 6. Integrate and validate the traversal prototype | 3, 4, 5 | Wire the three features into `BP_DPlayer`, finish the test lane fixtures, and update the control instructions. | Build `DelerictEditor`, run the movement tests in `L_DTestLane`, and use the normal `L_DHangar` startup to verify the pawn resolves both tuning assets. Play through a push-off, mid-air grab and turn, switch hit, and magnetic capture. |

The shared hooks pass aim and one-shot actions from input, use world-space impulse vectors in kg·cm/s, and process grab, release, and impact events in order. The movement step owns the mass conversion, plane projection, and final velocity update. [Current movement test](../Source/Delerict/Player/Tests/DMovementTests.cpp)

### Parallel work

- **While commits 1 and 2 are in progress**, develop the throwable momentum calculations, weapon profile lookup and CoreKit event adapter, and platform face geometry and capture queries. Exercise each with isolated fixtures until the shared movement hooks are ready. Level design can block out an open gap, a switch approach, and a vertical magnetic face in a separate working map.
- **After commit 2**, develop and merge commits 3, 4, and 5 independently. They share the movement hooks and data schemas. Keep feature code and tests in separate files so the core pawn, Mover, and shared assets have one owner during development.
- **After commits 3–5 merge**, wire the production pawn and save the shared Blueprint and map packages in commit 6. Run the full route and cross-feature checks there, including weapon and throw recoil while magnetically attached.
