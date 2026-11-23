# CoreKitActions

CoreKitActions coordinates local, single-player gameplay actions. The actor's action component owns admission, execution IDs, a pending request, scoped tag claims, and cancellation. Each action owns its callbacks and cleanup. Domain components own gameplay state and effects.

## Blueprint example

1. In Project Settings, create a Gameplay Tag such as `Action.Demo.Interact`. Create a Blueprint derived from **Core Kit Action**. Implement **Activate Action** to perform the interaction, then call **Complete Action** or **Fail Action**. Implement **End Action** to release any timers or bindings the action owns.
2. Create a **Core Kit Action Set** Data Asset. Add an entry under **Actions** keyed by the tag. Set its inline **Action Template** to the Blueprint and configure its **Definition** with requirements, conflicts, and repeat or queue behavior as needed.
3. Add a **Core Kit Action Component** to the actor and assign the Action Set. From input or another consumer, call **Request Action With Context** with the action tag. Branch on the returned status and detailed result. **Started** carries the execution ID. **Queued** carries a separate request ID. Use **Cancel Execution** or **Cancel Queued Request** when you need to target an exact instance.
4. Test one accepted request and one rejected request by giving the definition a requirement the actor lacks. For an async action, confirm cancellation finishes only after the action has cleaned up its work and called **Finish Cancellation**.

**Evaluate Action** checks admission for a prompt without starting work. C++ callers can use **ExecuteActionCommand** for immediate, typed operations. Use a **Core Kit State Component** when the actor needs exclusive state domains and one writer per domain.

## IASA and animation

`State.Action.IASA` represents readiness to accept voluntary gameplay. A lifetime action can require readiness or close it on start. Bind its montage playback using **Bind IASA Animation** and use **Core Kit Anim Notify IASA** at the interruptible point. The notify releases only that playback's block. Replaying the same montage needs its own marker. The consumer or a newly admitted action can then call **Interrupt IASA Montages**. Keep forced reactions and cleanup eligible according to their own action policies.

## Source and tests

The runtime and tests adapt ActionFramework 0.1.1 from SignalVeil commit `d369a6a5e7f4afb98f155769d93c8a2bd7c03179`. The CoreKit types live under `/Script/CoreKitActions` and the C++ includes start with `CoreKitActions/`. The project supplies input bindings, combat, movement, and replication if needed.

Run the `CoreKitActions` automation suite in an Editor target with CoreKit enabled. The Editor-only `CoreKitActionsTests` module reads its fixture tags from `Tests/CoreKitActions/Config/Tags`.

## Gameplay Debugger and held input

During PIE, open Unreal's Gameplay Debugger and enable **CoreKitActions**. Select the actor that owns the action component. The panel shows active execution IDs, the queued request, recent results, and the reason and source of each IASA block. In the sample the controller owns the component, and the category falls back to the local controller when its pawn has no action component.

In the sample, choose **Close IASA for 2 seconds** and hold **Block** through recovery. The adapter remembers the held intent, requests the action when readiness returns, and cancels that execution on release. Release during recovery to discard the pending intent. The adapter and its input bindings live in the sample project.
