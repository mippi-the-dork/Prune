# Class and Global Filters — Technical

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

## Scope fields

`FPresetData` and `FNativeOverrideData` both store `bGlobal` plus an `FName` class identifier. `DoesScopeApplyToClass` resolves availability using the Actor class relationship. Class filters can appear on derived Actor classes.

## Validation and transitions

`CreatePreset` rejects Class scope if the current layout cannot resolve an Actor class. `UpdatePreset` maintains or changes the persisted class association according to the requested scope transition.

## Global does not mean every category is present

Category membership is still derived from the final Details layout of the selected Actor. Prune's class/global logic governs section availability, not the set of Unreal properties that exist.

---

For the step-by-step user workflow, [read Design](DESIGN.md).
