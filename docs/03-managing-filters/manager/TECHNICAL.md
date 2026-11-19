# Filter Manager — Technical

**Prune v1.0.0 · Editor-only · UE 5.8.x**

## Model construction

`FPruneState::GetManagedFilters` gathers applicable custom presets and Epic Property Section definitions into `FPruneManagedFilterInfo`, including stable OrderKey, native/custom kind, override status, scope, description and native order.

## Action resolution

`ResolveManagedFilter` converts a selected manager row into a uniquely identified editable filter. This avoids relying on a display name alone where two sections might be ambiguous.

## Window and list

The Filter Manager's native Slate implementation resides in `PruneDetailsCustomization.cpp`. It keeps filter editing close to native Actor Details, with window size defaults from `UPruneSettings`.

## Data updates

Creating, deleting, resetting or reordering filters updates `FPruneState` and the live Details layouts, with persistence written to the editor's per-project user configuration. The manager is an editor UI, not a Content Browser asset or packaged runtime tool.

---

For the practical workflow, [read Design](DESIGN.md).
