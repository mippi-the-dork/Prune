# Installation and Compatibility — Technical

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

## Declared compatibility and build

`Prune.uplugin` reports `VersionName: "1.0.0"`, `EngineVersion: "5.8.0"`, `Type: "Editor"`, `LoadingPhase: "PostEngineInit"`, and `SupportedTargetPlatforms: ["Win64"]`. These are declared targets, not evidence for other engines or platforms.

`Source/Prune/Prune.Build.cs` contains the editor-facing dependencies required for Slate, Property Editor and Details customization integration.

## Startup checks

`FPruneModule::StartupModule` avoids initialization in commandlets and double-registration. It deliberately loads `DetailCustomizations` so `OnExtendActorDetails` exists, then registers its callback. `NotifyCustomizationModuleChanged` refreshes open Details views.

## Shutdown

The module unregisters its Details extension, removes its native Property Section modifications through `RemoveAllNativeSections()`, clears shared state and notifies Property Editor when appropriate.

## Diagnostic levels

Distinguish missing plugin load, unsupported engine APIs, non-Actor Details contexts, and a valid Details view that has not rebuilt. A build completing is not proof that each native filter-row insertion path behaves correctly in the target project.

---

For the step-by-step user workflow, [read Design](DESIGN.md).
