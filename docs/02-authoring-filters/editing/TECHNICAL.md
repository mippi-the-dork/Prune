# Editing, Duplicating and Deleting — Technical

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

## Distinct edit paths

`FPruneState::BuildEditorData` builds a working editor definition for either a custom preset or applicable native filter override. `UpdatePreset` changes a custom preset with name validation and merges current category IDs with prior hidden/order state to avoid losing unknown categories.

## Duplication

`FPruneState::MakeSuggestedDuplicateName` generates a collision-safe display name. `OpenDuplicateFilterEditor` copies the source's visibility/order settings into a new custom filter rather than reusing its internal identity.

## Deleting

`DeletePreset` removes an owned custom preset, persists the new state, updates Property Sections and refreshes Details. Native filters remain owned by Unreal and are not deletable Prune presets.

## Active resolution

`ResolveSingleActiveFilter` verifies a unique matching custom/native section and rejects ambiguous active selections. This powers `CanEditActiveFilter` and the Edit button's enabled state.

---

For the step-by-step user workflow, [read Design](DESIGN.md).
