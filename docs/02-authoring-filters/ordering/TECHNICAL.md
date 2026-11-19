# Reordering Details Categories — Technical

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

## Stored ordering

`FPresetData::OrderedCategoryIds` contains category `FName` IDs, independent of the visibility set. `FPruneDetailsCustomization::ApplySavedOrderToGroups` applies the authored priority and native sort order when arranging the filter editor's grouped category rows.

## Applying the order

After the Details layout is constructed, `RestoreStandardCategoryGrouping` and `ApplyStoredCategoryOrder` operate on `IDetailCategoryBuilder` order for the appropriate active single filter. Unknown entries retain predictable relative placement rather than being dropped.

## Drag operation

The Slate dialog uses `SDragAndDropVerticalBox` and tracks source/target indices and insertion zones. Drag colors/line thickness come from `UPruneSettings`.

## Multiple filters

`GetActiveCategoryOrder` resolves exactly one active custom filter or native override. If no single unambiguous active filter is available, there is no Prune-specific category order to impose.

---

For the step-by-step user workflow, [read Design](DESIGN.md).
