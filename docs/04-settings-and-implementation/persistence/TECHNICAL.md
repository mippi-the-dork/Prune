# Saving, Scope and Editor State — Technical

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

## Persistent state structure

`FPruneState::LoadPersistentState` and `SavePersistentState` read/write `GEditorPerProjectIni`. Root config section `Prune.UserState` contains preset and override ID arrays. Preset sections include name, description, Global/Class scope, class name, hidden category IDs and order. Override sections include added/removed category IDs and source section association.

## Stable identities

`MakeSectionName` derives native section identifiers from preset GUIDs. Filter bar order keys use `P:` and `N:` prefixes to differentiate custom presets and native sections without depending on mutable display names.

## On Actor selection

The live `FPruneLayoutContext` discovers the final category list for that Actor/class. The registered native sections are reconstructed/reconciled from persisted definitions. This config is separate from `AActor` serialization.

## Scope and multi-selection

`DoesScopeApplyToClass` decides whether custom/native overrides apply to the current Actor class. When multiple Actor classes are selected, a single class-specific editing context may be unavailable; this prevents ambiguous edits.

## Boundary

No actor package save overlay, runtime replication hook, or World Partition actor manipulation is added by Prune. Native Property Section membership is the presentation mechanism.

---

For the corresponding user workflow, [read Design](DESIGN.md).
