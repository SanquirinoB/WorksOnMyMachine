# Working on CoreKit

CoreKit is one Unreal Engine plugin for new projects. Read [the plan](PLAN.md) for the module sequence and [the quick start](Docs/QuickStart.md) for the consumer workflow. Use the module guide for the current behavior and validation steps.

CoreKitBasicUI is the reusable DeadDraw presentation module added in the MachineVault copy. Read [its guide](Docs/CoreKitBasicUI.md) before changing it. Keep it independent of DeadDraw and CombatKit. Content belongs under `/CoreKit/BasicUI`; keep its external-game dependency audit clean. BasicUI is opt-in through its settings, and game-specific state/commands belong to the consumer. Validation projects and generated files stay outside this plugin checkout.

## Project skills

- [corekit-maintainable-development](.agents/skills/corekit-maintainable-development/SKILL.md) for module boundaries, runtime C++, and reflected API changes.
- [corekit-blueprint-authoring](.agents/skills/corekit-blueprint-authoring/SKILL.md) for Blueprint examples, graph changes, and editor validation.
- [corekit-quality-review](.agents/skills/corekit-quality-review/SKILL.md) for reviewing code, assets, and validation evidence.

Open the linked `SKILL.md` directly when the client does not discover repository skills. Keep the consumer API and its examples in `Docs/`. Keep validation projects and their generated files outside the plugin checkout. Validate new consumer workflows in a clean UE 5.8.3 project before documenting them as verified.
