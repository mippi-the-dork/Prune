# Choosing and Searching Categories — Technical

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

## Late category discovery

`FPruneLayoutContext::UpdateFromFinalCategories` reads the final map of category IDs and `IDetailCategoryBuilder` objects, preserving native sort order. Prune groups entries sharing a displayed label into `FPruneCategoryGroupInfo` with multiple underlying `FName` IDs.

## Search versus state

The filter editor builds category rows from a working copy of discovered groups and hidden IDs. Search only affects the displayed rows; it does not rewrite the committed hidden-category set.

## Category reconciliation

`SyncNativeSectionsForContext` adds newly observed IDs to `KnownCategoryIds`, rebuilds custom sections when needed, and reapplies matching native overrides. This supports categories introduced by other editor customizations once present in the final layout.

## Nested category implementation constraint

Unreal's Property Section API expects top-level category IDs. `GetSectionCategoryId` collapses IDs such as `Omni|Generation` to their root at that API boundary, while Prune retains the complete internal ID for its own ordering and grouping.

---

For the step-by-step user workflow, [read Design](DESIGN.md).
