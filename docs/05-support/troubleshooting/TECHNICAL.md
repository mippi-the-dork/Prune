# Troubleshooting Prune — Technical

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

## Technical diagnosis sequence

1. Confirm the Editor module loaded and `OnExtendActorDetails` callback was registered.
2. Confirm the selected object is an Actor in a supported native Details context.
3. Inspect `FPruneLayoutContext::GetCategoryGroups()` after final layout construction.
4. Distinguish a valid custom preset from an applicable native override via `ResolveSingleActiveFilter`.
5. Check `GEditorPerProjectIni` state for configuration-specific issues.
6. For import, inspect validation errors from `ImportFiltersFromFile` before suspecting UI state.

## Native section mismatch

Categories with a nested `|` identifier must be normalized to the root at the native Property Section API boundary. Failure to handle that may cause assertions in engine APIs expecting a top-level category.

## Live widget lifetime

`FPruneLayoutContext` uses weak Details view references; plugin shutdown removes native sections and hooks. Investigate stale control behavior after editor layout rebuilds and hot reload/recompilation separately from persisted preset definitions.

## Reproducibility

Test a simple built-in Actor, a Blueprint Actor with customized categories, and a class hierarchy. Then repeat after restarting the editor. The documentation's verification matrix records tests to execute; source review alone is not a passed integration test.

---

For the corresponding user workflow, [read Design](DESIGN.md).
