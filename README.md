# Prune 0.1.6 Prototype

Prune is an Unreal Engine editor plugin for hiding whole categories in the standard Level Editor Actor Details panel.

This is still a **feasibility prototype**. Presets and persistence are intentionally deferred until category discovery, hiding, restoration, plugin coexistence, and ordering are reliable.

## 0.1.6 Finished-Category Discovery

Prune continues to attach through UE 5.8.3's public `OnExtendActorDetails` delegate. It no longer treats the category set visible at that callback as final.

Instead, Prune registers a `SortCategories()` callback. Unreal invokes category-sort callbacks after native/default/custom categories have been generated, so Prune receives the complete public category map for that Details layout. This includes categories added later by other customizations, such as Surface's Actor Details sections.

Prune's own internal category ID, `PruneControl`, is explicitly excluded and can never appear as a Prune checkbox.

The Prune dropdown is alphabetized by display label. This is independent of the Details panel's visual category ordering; Prune does not modify that ordering.

## 0.1.6 Dynamic Visibility

Interactive filtering no longer uses:

- `IDetailLayoutBuilder::HideCategory()` for checkbox changes,
- `IDetailsView::ForceRefresh()`,
- layout reconstruction to restore a category.

Prune now keeps the live `IDetailCategoryBuilder` objects for each active Actor Details layout and calls `SetCategoryVisibility()`.

This is important because Unreal's category implementation changes only the visibility flag, reruns the existing filter, and notifies the layout. It does not recreate the category or change its sort order.

Expected result:

- hiding a category removes it in place,
- showing it restores the same category in the same established position,
- restoring categories in a different order does not reorder the Details panel.

## Plugin-Added Categories

Prune discovers the finished category set after all participating customizations have contributed their categories.

With Surface enabled, the dropdown should now include:

- `Surface - Favorites`
- `Surface - Component Details`

Those categories are treated like any other Details category and may be hidden or restored.

Prune does not special-case Surface by name. Any plugin category using Unreal's normal `IDetailCategoryBuilder` path should be discoverable through the same finished category map.

## Multiple Details Layouts

The hidden-category deny-list remains shared for the current editor session, but each live Details layout now has its own category-builder context.

Changing a category updates every currently live Actor Details layout that contains that category. When a Details layout is destroyed, its context is released with its Slate control and Prune retains only a weak reference.

This is safer than keeping one global `IDetailsView` or one global set of raw category builders.

## Target

- Unreal Engine 5.8.0 - 5.8.3
- Windows 64-bit
- Editor Only
- No Engine source modifications
- Standalone plugin

## Current UI

For an Actor selected in the Level Editor, Prune adds a **Prune** category containing a **Categories** dropdown.

Each entry is a category from the completed Actor Details layout:

- checked = visible
- unchecked = hidden by Prune

The menu also contains:

- **Show All** - clears the session deny-list and restores all currently live categories in place
- **Log IDs** - prints the completed category set, internal IDs, display labels, and visibility state

## Current Limitations

- Level Editor instance Actor Details only.
- Hidden categories are session-only.
- Hidden state is not per class yet.
- No named presets yet.
- No Project Settings UI yet.
- No Component Details preset family.
- No Class Defaults integration.
- No native filter-row injection.
- The Prune control still lives in its own temporary Details category.

## First Test

1. Compile and launch UE 5.8.3 with Surface enabled.
2. Select a StaticMeshActor.
3. Open **Prune > Categories**.
4. Confirm **Prune** itself is absent from the list.
5. Confirm **Surface - Favorites** and **Surface - Component Details** are present.
6. Hide **Rendering**, then restore it.
7. Hide **Transform**, then restore it.
8. Hide several categories, then re-enable them in a different order.
9. Confirm every category returns to its original Details-panel position.
10. Hide and restore both Surface categories.
11. Use **Show All** and confirm ordering remains unchanged.
