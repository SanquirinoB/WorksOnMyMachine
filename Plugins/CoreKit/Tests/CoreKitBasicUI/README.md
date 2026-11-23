# CoreKitBasicUI acceptance tests

These tests belong to a clean consumer's validation module, outside the plugin checkout. They do not add a runtime dependency or another automatically loaded module to CoreKit.

The verified consumer is at `C:/Users/Deltalo/MachineVault/CoreKitBasicUIValidation/Project`. To reuse the tests, copy the two `.cpp` files into an Editor/validation module that explicitly depends on Core, CoreUObject, Engine, CoreKitBasicUI, CoreKitNavigation and UMG. Build the consumer Editor target, then run `Automation RunTests CoreKit.BasicUI` with an automation report export directory. PortableContent expects the included BasicUI content to be installed.

See `Docs/BasicUIValidation.md` for the integration walkthrough and the limits of the evidence.
