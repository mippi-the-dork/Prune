# Your First Filter — Technical

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

## Create path

The New Filter Slate dialog is constructed by `ShowFilterEditor` in `PruneDetailsCustomization.cpp`. The completed result calls `FPruneState::CreatePreset`, which validates the name/scope, allocates a new GUID-backed preset identifier, stores the chosen hidden category IDs and order, persists state, rebuilds custom Property Sections and refreshes Details views.

## What counts as a category?

`FPruneLayoutContext::UpdateFromFinalCategories` reads the final category map from the live `IDetailLayoutBuilder`. This includes applicable customizations already added by other editor code.

## Why All and multiple selections are special

`ResolveSingleActiveFilter` only returns an editable filter when the native selector identifies a single unambiguous compatible section. The Edit control relies on this check, rather than assuming the last clicked button is always an editable Prune preset.

## Technical verification

Create a class-scoped filter, change category visibility/order, rebuild the Details view and reopen the editor. Confirm the persisted definition and the native section membership remain coherent. This is a recommended manual test, not a claimed passing test.

---

For the step-by-step user workflow, [read Design](DESIGN.md).
