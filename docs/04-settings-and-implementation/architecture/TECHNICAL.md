# Native Details Integration — Technical

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

## Module startup and delegation

`FPruneModule::StartupModule` loads `DetailCustomizations`, binds `OnExtendActorDetails` and uses `NotifyCustomizationModuleChanged` to refresh open views. On shutdown it unregisters the delegate and calls `FPruneState::RemoveAllNativeSections`.

## Live layout context

`FPruneLayoutContext` stores a weak reference to the Details view, selected class, native section selector widget, raw category map and display-grouped categories. It avoids owning Actor instances and keeps view lifetimes separate from persisted filter definitions.

## State/controller division

`FPruneState` owns presets, Epic overrides, filter order, state serialization, section synchronization and layout context registration.

`FPruneDetailsCustomization` constructs the native Slate New/Edit dialog, Filter Manager, contextual actions and button injection.

## Grouping and ordering

A final `IDetailLayoutBuilder` category map supplies `IDetailCategoryBuilder` values. Duplicate display labels are grouped; native section registration expects top-level IDs, so nested category IDs are normalized at its boundary. `GetActiveCategoryOrder` only returns a custom order for an unambiguous single active filter.

## Extension constraint

Prune integrates with native interfaces, with no engine source patch required. However, engine API changes or custom Details tabs not exposing Actor Details extension points can affect integration and must be verified for each engine version.

---

For the corresponding user workflow, [read Design](DESIGN.md).
