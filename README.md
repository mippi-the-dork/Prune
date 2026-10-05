# Prune 0.5.1

Prune is an Unreal Engine editor plugin for creating and editing category filters in the standard Level Editor Actor Details panel.

Prune works directly with Unreal's native Property Section row, alongside filters such as **General**, **Actor**, **LOD**, **Physics**, **Rendering**, and **All**.

## 0.5.1 Compile Fix

- Corrected the UE 5.8 Slate include path for `SOverlay` to `Widgets/SOverlay.h`.
- No behavior changes from 0.5.0.

## Filter Row UX

Prune 0.5.1 replaces the previous Prune management menu with two compact controls:

- **+** creates a new filter.
- **Filter + Gear** edits the single active editable filter.

The gear is disabled when **All** is active, when Ctrl multi-section selection is active, or when the selected section is not an editable category filter.

The row is conceptually:

`[ General | Actor | LOD | ... | Custom Filter | All ] [ + ] [ Filter + Gear ]`

The **+** button uses Unreal's native Details section-button style.

## New Filters

The New Filter window provides:

- filter name,
- **Global** scope toggle,
- category visibility checkboxes,
- category ordering controls,
- **Reset Order**,
- Save and Cancel.

When **Global** is enabled, the filter is available to all Actor classes.

When **Global** is disabled, the filter belongs to the currently selected Actor class and is inherited by derived Actor classes through Unreal's normal Property Section inheritance.

Existing Prune 0.4.x filters are migrated as Global filters so their previous behavior is preserved.

## Editing Epic Filters

Unreal's existing Property Section filters can now be edited through the same gear button.

Examples include:

- General
- Actor
- LOD
- Misc
- Physics
- Rendering
- Streaming

Prune stores an override rather than replacing the engine source definition. Native filter names are read-only in the editor.

An edited Epic filter can be scoped to the current Actor class or made Global. **Reset to Epic Default** removes the applicable Prune override and restores the captured native behavior.

**All** is never editable.

Helper sections such as Favorites and Modified are not treated as editable category presets.

## Category Ordering

Each editable filter can store its own Details category order.

The filter editor provides up and down controls for every visible category group. When that filter is active, Prune applies the stored order during Details layout generation.

Rules:

- categories explicitly arranged by the user follow the saved order,
- categories not yet known to the saved filter remain visible and fall after the stored entries using Unreal's normal relative order,
- duplicate visible labels such as `Transform` and `TransformCommon` move together as one editor row,
- Ctrl multi-section selection falls back to Unreal's native category order,
- switching into or out of a filter with a custom order requests one Details refresh so the correct order is applied.

## Filter Scope

Prune supports two scopes:

### Class

The filter or Epic-filter override is associated with the currently selected Actor class. Derived classes inherit it through Unreal's Property Section rules.

### Global

The filter is available across Actor classes.

For custom Prune filters, Global sections are registered on `AActor`.

For Epic-filter overrides, Prune applies the same saved override lazily to each encountered Actor class. This avoids replacing Unreal's global section registry and keeps the engine-owned section definition available underneath the override.

## Native Filter Row Integration

UE 5.8's `SActorDetails` creates its Details view with `bCustomFilterAreaLocation = true` and places `IDetailsView::GetFilterAreaWidget()` into the Level Editor layout.

Prune uses that public accessor. It does not crawl the whole Details widget tree.

Inside the filter area, Unreal owns the native `SectionView` `SWrapBox`. Prune keeps that exact widget intact and places the **+** and **Filter + Gear** controls beside it as siblings.

Unreal may rebuild the contents of `SectionView` normally without deleting Prune's controls.

## Native Category Order Preservation

Prune uses `IDetailLayoutBuilder::SortCategories()` as its finished-category discovery point.

UE 5.8 changes the normal simple and advanced-only grouping when any sort callback is installed. Prune reconstructs Unreal's native grouping before discovery, then applies a filter-specific stored order only when required.

With **All** or an unmodified filter active, Unreal's normal category order is preserved.

## Persistence

Prune persists per-user, per-project filter data through `EditorPerProjectUserSettings.ini`.

Saved data includes:

- custom filter definitions,
- Global or Class scope,
- hidden category IDs,
- category order,
- Epic filter override deltas,
- Epic filter override scope and order.

## Plugin Categories

Prune discovers the finished category map after Actor Details customizations have run, so plugin-added categories can participate too.

This includes categories such as:

- `Surface - Favorites`
- `Surface - Component Details`

## Target

- Unreal Engine 5.8.0 - 5.8.3
- Windows 64-bit
- Editor Only
- No engine source modifications
- Standalone plugin

## Current Limitations

- Level Editor instance Actor Details only.
- Component Details and Class Defaults are not integrated yet.
- An Epic filter override can only modify categories Prune has encountered while that override is active. Newly encountered categories retain their underlying Unreal behavior until explicitly changed.
- A custom filter that contains no populated category for the current Actor may not appear because Unreal only displays Property Sections that map to populated categories.
