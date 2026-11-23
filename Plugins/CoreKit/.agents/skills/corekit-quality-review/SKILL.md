---
name: corekit-quality-review
description: Review CoreKit changes for observable regressions, unclear ownership, excessive coupling, and gaps in clean-project validation. Use for a requested audit or a review before delivering a module.
---

# CoreKit quality review

1. Establish the review scope and read the current module guide from [the quick start](../../../Docs/QuickStart.md). Inspect the changed entry points, actual consumers or examples, and tests that exercise the behavior.
2. Check ownership and lifetime, the documented success and failure behavior, and whether dependencies leak through public headers. For CoreKitData, check authored order, missing results, exact tag matching, and whether discovery assumes uncooked assets.
3. Match evidence to the change. Check links and frontmatter for skills and docs. Use compilation for reflection and module dependencies. Use PIE or a cooked test for behavior that depends on the editor, serialization, or cooking.
4. Report findings with the file, observable consequence, and smallest correction. Separate observed results from remaining validation. A review request authorizes review within its stated scope.
