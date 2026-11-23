---
name: corekit-maintainable-development
description: Implement or modify CoreKit Unreal Engine runtime modules, public C++ contracts, or reflected Blueprint APIs. Use when deciding module dependencies, state ownership, or the shape of a reusable feature.
---

# CoreKit maintainable development

1. Read [AGENTS.md](../../../AGENTS.md) and the affected module guide. Start with [CoreKitData](../../../Docs/CoreKitData.md) for authored data, fragment lookups, or asset discovery.
2. Identify the consumer, the owner of mutable state, and the lifetime of the service. CoreKitData assets and inline fragments are shared authored configuration. Runtime state belongs to the consumer.
3. Expose the smallest contract a clean project needs. Put implementation dependencies in `PrivateDependencyModuleNames` and keep unrelated CoreKit modules independent. Export and reflect only types and functions that consumers use.
4. Verify the feature through the documented consumer entry point. Build an Editor target for reflected changes and a game target when changing runtime dependencies. Check discovery in a cooked game when it depends on content or Asset Registry data.
5. Update the affected module guide with any new observable behavior or required setup.
