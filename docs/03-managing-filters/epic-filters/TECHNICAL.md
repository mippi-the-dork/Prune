# Editing Epic Filters — Technical

**Prune v1.0.0 · Editor-only · UE 5.8.x**

## Native filter ownership

Epic's Property Sections remain Unreal-owned. `EPruneFilterKind::Native` identifies them; `FNativeOverrideData` stores the section ID, display name, description, class/global scope, added/removed category IDs and ordered category IDs.

## Delta-based override

`FPruneState::SaveNativeOverride` calculates desired category membership against the original baseline and stores addition/removal deltas. The original section is not permanently replaced. `ApplyNativeOverridesForContext` applies an appropriate override to the current Actor's native section context.

## Reversibility

`ResetNativeOverride` removes the matching Prune override, then restores the underlying native category membership and refreshes the Details view. The module also calls `RemoveAllNativeSections` on unload, restoring runtime modifications.

## Scope specificity

An override can be Class or Global. `FindApplicableNativeOverride` resolves the applicable saved definition for the current Actor class. A conflicting scope may be rejected rather than overwriting another scope silently.

## Duplication isolation

Duplicating a native filter transfers its effective settings into a new GUID-backed `FPresetData`, not a second reference to the Epic section.

---

For the practical workflow, [read Design](DESIGN.md).
