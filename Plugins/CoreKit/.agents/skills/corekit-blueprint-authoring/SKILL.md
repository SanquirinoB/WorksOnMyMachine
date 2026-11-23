---
name: corekit-blueprint-authoring
description: Create, modify, or verify CoreKit Blueprint examples and Widget Blueprints in a clean Unreal Engine consumer project. Use for reflected node wiring, inline data authoring, and graph validation.
---

# CoreKit Blueprint authoring

1. Verify which project and editor instance own the assets. Start from [the quick start](../../../Docs/QuickStart.md) and the relevant module guide. Use the [CoreKitData example](../../../Docs/CoreKitData.md#blueprint-example) for inline fragments and typed queries, or the [CoreKitNavigation example](../../../Docs/CoreKitNavigation.md#blueprint-example) for widget focus and input glyphs.
2. Verify the parent class, function names, pin types, and class defaults from the loaded editor or current reflected source. Determine which assets are examples and which the project owns before editing.
3. Keep graphs small and connect the typed result to the consumer. Include the expected behavior for a missing reference or lookup result when that case matters. For targeted edits, preserve surrounding graph layout and defaults.
4. Compile the changed Blueprint, inspect the graph in the Editor, and exercise the documented scenario in PIE. Check saved asset references when preparing an example for a separate project. Report which of those checks actually ran.

Blueprint Semantic Extractor can provide semantic inspection if installed. Blueprint Assist can format changed blocks in a supported editor graph if installed. Verify the tool's coverage and target before using it.
