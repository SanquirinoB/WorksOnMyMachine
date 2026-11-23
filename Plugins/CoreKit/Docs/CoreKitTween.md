# CoreKitTween

A tween generates an eased alpha from 0 to 1 using the current world's game time. The consumer uses the alpha to interpolate any value. Each world owns its tweens. Completion and cancellation are terminal. Destroying the owner or tearing down its world cancels the tween.

## Blueprint examples

- **Menu:** call **Play Tween** on the widget, choose **Smooth Step**, and use **Updated** to lerp from 0 to 1 and set Render Opacity.
- **Door:** call **Play Tween** on the actor. Lerp from its closed relative rotation to its open relative rotation and apply the value to a movable door component. Capture the endpoints once when starting the tween.
- **Simulated physics:** use the interpolated value as a target for the object's controller or constraint. That controller owns how forces are applied and how the body responds to collisions.

Use the **Tween** output to pause, resume, or cancel. A zero duration completes on the next eligible world tick. World pause and time dilation affect the tween. The **Cancelled** output carries the last emitted alpha. In C++, obtain **CoreKitTweenSubsystem** from the world and retain the handle returned by **StartTween**.

## Sequences, clocks and channels

Use **Play Tween Sequence** with one or more steps, each with a duration and easing. Read **Get Current Step** from the tween proxy inside **Updated** to decide how to apply the alpha. Cancelling the sequence ends the active step and drops the remaining steps.

In the options, choose **Game** to follow world pause and dilation or **Real Time** to continue while the game is paused. Explicitly pausing the tween stops either clock. Assign a named channel such as `Door.Motion` or `Menu.Opacity` and choose **Allow**, **Reject** or **Replace** when another tween is active for the same owner and channel. **Cancel Tweens By Channel** cancels the matching handles for that owner, including sequences.
