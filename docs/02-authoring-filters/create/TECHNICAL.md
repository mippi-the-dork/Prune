# Creating Custom Filters — Technical

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

## Definition structure

`FPruneState::FPresetData` stores a GUID string ID, name, description, `bGlobal`, class name, hidden `FName` category IDs and ordered category IDs. The preset's stable internal ID is separate from its user-facing name.

## Creation and validation

`CreatePreset` trims the requested name, calls `ValidateCustomPresetName`, rejects a class scope when no single Actor class can be resolved, saves the definition and triggers `RebuildAllCustomSections` and `RefreshDetailsViews`.

## Underlying Property Section

`MakeSectionName` generates a native section identifier prefixed `Prune_` from the preset GUID. This lets Prune coexist with Unreal's native Property Section infrastructure without replacing it.

## Not an asset

Custom filters are editor user configuration, not a Content Browser asset and not serialized into selected Actor packages. Refer to [Persistence Technical](../../04-settings-and-implementation/persistence/TECHNICAL.md).

---

For the step-by-step user workflow, [read Design](DESIGN.md).
