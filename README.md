# Prune 0.8.1

Prune is an Unreal Engine editor plugin for creating and editing category filters in the standard Level Editor Actor Details panel.

Prune works directly with Unreal's native Property Section row, alongside filters such as **General**, **Actor**, **LOD**, **Physics**, **Rendering**, and **All**.



## 0.8.1 Filter Manager Apply Feedback and Reset Refresh

- Fixed **Reset Button Order** so the visible Actor Details filter row returns to Unreal's normal order immediately. Reselecting the Actor is no longer required.
- Reset now removes the saved per-class custom order instead of retaining an empty order record.
- The Filter Manager footer now explicitly states that manager changes apply immediately.
- Renamed the manager's **Close** button to **Done** so its behavior matches the immediate-apply model.
- **Reset Button Order** is disabled when the current Actor class is already using Unreal's normal filter-button order.
- Added an **Active** marker in the manager for every currently selected editable filter. This also handles Unreal's Ctrl multi-selection by marking each selected filter.
- Active filter metadata uses Unreal Accent Blue for quick scanning.
- Added **Settings...** to the Filter Manager footer for direct access to **Project Settings > Plugins > Prune**.
- Existing live drag reorder, manager search, duplication, descriptions, scopes, Epic overrides, and category ordering remain unchanged.

## 0.8.0 Live Filter Manager Reorder and Duplication

- Fixed Filter Manager reordering so the manager list itself updates immediately after a drop instead of only showing the new order after the modal closes.
- The manager now defers its list rebuild until the next Slate tick, after Unreal's drag widget has finished the drop event and released its cached slot geometry.
- Added **Search filters** to the Filter Manager. Search matches filter names, descriptions, and scope/type labels.
- Filter-button dragging is disabled while manager search is active so reorder indices always refer to the complete list.
- Added **Duplicate...** directly to every Filter Manager row.
- Epic filters can now be duplicated into new independent Prune filters without creating or modifying an Epic override.
- **Duplicate...** is also available while editing an Epic filter and carries the currently edited visibility, category order, description, and scope into the new Prune filter.
- Existing custom-filter duplication, per-class button ordering, descriptions, Project Settings, and Actor Details integration remain unchanged.

## 0.7.0 Filter Manager, Button Ordering, and Descriptions

- Added a dedicated **Filter Manager** for the current Actor class.
- Added a third compact manager control beside **+** and **Filter + Gear** in the native Details filter row.
- The manager lists the Epic and Prune filters available to the current Actor class, including scope and override state.
- Drag filters in the manager to reorder their buttons in Actor Details. Filter-button order is remembered per Actor class.
- **All** remains after the editable category filters, while Unreal helper sections retain their native trailing behavior.
- Added **Reset Button Order** to return the current Actor class to Unreal's normal filter order.
- Added optional filter descriptions to both Prune filters and Epic filter overrides.
- Filter descriptions appear in the manager and, by default, as filter-button tooltips.
- Added **Show Description Tooltips** under **Project Settings > Plugins > Prune**.
- Added configurable default width and height for the Filter Manager window.
- Existing filter creation, editing, duplication, deletion, Epic reset, Global/Class scope, category ordering, and persistence remain intact.

## 0.6.0 Project Settings and Reorder Insertion Line

- Added **Project Settings > Plugins > Prune**.
- Added configurable **Reorder Drag Color** and **Dragged Content Color** for the category row being moved.
- Added configurable **Drop Line Color** and **Drop Line Thickness**.
- Replaced the full-row drop preview with a thin insertion line above or below the target category so the final placement is explicit.
- Added configurable default width and height for newly opened New Filter and Edit Filter windows.
- Reorder appearance settings are read when a filter editor opens, so close and reopen an existing editor window after changing them.

## 0.5.7 UE 5.8 Compile Fix

- Fixed the Accent Blue drag-and-drop indicator brush declaration for MSVC/UE 5.8.
- Preserves the 0.5.6 drag-row Accent Blue highlight, pure-white dragged-row content, drop indicator styling, and category search behavior.

## 0.5.6 Drag Feedback and Category Search

- Reworked category drag feedback so the category being moved uses Unreal's theme-aware **Accent Blue** instead of the unreadable white drag state.
- Dragged category labels and the reorder grip switch to pure white for clear contrast against the blue selection.
- Changed the above/below drop indicators to the same Unreal Accent Blue for consistent reorder feedback.
- Added a native-style **Search categories** field to the New/Edit Filter window. Search matches both the visible category label and internal category IDs.
- Added an empty-search result message when no categories match.
- Category search is temporary editor UI only and never changes the saved filter. Clear the search before drag reordering so the saved full-list order stays unambiguous.
- Existing filter visibility, ordering, scope, duplication, Epic overrides, persistence, and filter-row behavior are unchanged.

## 0.5.5 Filter Editor Quality of Life

- Added **Show All** and **Hide All** controls to the category header for fast visibility editing.
- Added a live `shown / total` category count so large filters are easier to audit.
- Added **Duplicate...** for custom Prune filters.
- Duplicating opens a fresh New Filter window prefilled with the current visibility, ordering, and scope instead of silently creating a copy.
- Duplicate names are suggested safely as `Name Copy`, `Name Copy 2`, and so on without colliding with existing Prune or Epic filter names.
- Edit window titles now include the filter being edited.
- Existing filter-row integration, drag ordering, native All fallback, persistence, and Epic overrides are unchanged.

## 0.5.4 Interaction and Layout Fixes

- Restored visible icons for the +, filter-edit, and Global controls.
- Kept the + and filter-edit controls anchored to the bottom-right even when the selected Actor has no native Epic section buttons.
- Added a safety fallback that selects All only when the current Details view has an All section and no section is currently selected. Valid persisted section selections remain untouched.
- Completed the drag start path for category reordering so drag-and-drop now creates Unreal's native vertical-box drag operation.
- Kept the resizable filter editor, checkbox spacing, Global scope toggle, and per-filter category ordering introduced in 0.5.2.

## Filter Row UX

Prune 0.8.1 keeps the compact filter-row controls introduced in 0.5.0 and fixes their interaction, icon, and anchoring behavior:

- **+** creates a new filter.
- **Filter Manager** opens the class-aware filter management window and filter-button reorder list.
- **Filter + Gear** edits the single active editable filter.
- All three controls use the same fixed footprint.
- The controls anchor to the bottom-right of the section-row area when Unreal wraps section buttons onto multiple lines.

The gear is disabled when **All** is active, when Ctrl multi-section selection is active, or when the selected section is not an editable category filter.

The row is conceptually:

`[ General | Actor | LOD | ... | Custom Filter | All ] [ + ] [ Manager ] [ Filter + Gear ]`

All three controls use Unreal's compact SimpleButton treatment with matching 24x24 footprints and remain anchored to the bottom-right of the section-row area.

## Filter Manager

The Filter Manager is scoped to the Actor class currently shown in that Details panel. It lists the Epic and Prune filters that are available to that class.

From the manager you can:

- see which editable filters are currently **Active**,

- drag filters to reorder their buttons for the current Actor class, with the list updating immediately after each drop,
- search filter names, descriptions, and scope/type labels,
- edit any listed Epic or Prune filter,
- duplicate any listed filter into a new Prune filter, including Epic filters,
- create a new filter,
- see Global, Class, Epic, and Epic Override state at a glance,
- see optional filter descriptions,
- reset the current class back to Unreal's normal filter-button order immediately,
- open **Project Settings > Plugins > Prune** directly from **Settings...**.

Drag reordering is disabled while the manager search field contains text. Clear the search to restore reorder interaction. Manager changes apply immediately, and the footer communicates whether the current button order is custom or Unreal default.

The **All** button is not manually reordered. Prune keeps it after the editable category filters, while native helper sections such as Favorites and Modified keep their trailing behavior.

## New Filters

The New Filter window provides:

- filter name,
- optional filter description,
- category visibility checkboxes,
- category search by display label or internal category ID,
- a live visible-category count,
- **Show All** and **Hide All** visibility actions,
- drag-and-drop category ordering,
- a globe scope toggle in the bottom action bar,
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

Both custom Prune filters and Epic filters provide **Duplicate...**. Duplicating an Epic filter creates a new independent Prune filter and does not modify the Epic filter or create an override. Duplicate names use a collision-free suggested copy name.

**All** is never editable.

Helper sections such as Favorites and Modified are not treated as editable category presets.

## Category Ordering

Each editable filter can store its own Details category order.

The filter editor provides drag-and-drop reordering for every visible category group. Drag the grip at the left side of a row to place it above or below another category. The active drag row is highlighted, while a thin insertion line previews the exact destination above or below the hovered category. These colors and the insertion-line thickness are configurable in Project Settings. When that filter is active, Prune applies the stored order during Details layout generation. Clear category search before reordering so the operation always refers to the full category list.

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

Inside the filter area, Unreal owns the native `SectionView` `SWrapBox`. Prune keeps that exact widget intact and places the **+**, **Filter Manager**, and **Filter + Gear** controls beside it as siblings.

Unreal may rebuild the contents of `SectionView` normally without deleting Prune's controls.

## Native Category Order Preservation

Prune uses `IDetailLayoutBuilder::SortCategories()` as its finished-category discovery point.

UE 5.8 changes the normal simple and advanced-only grouping when any sort callback is installed. Prune reconstructs Unreal's native grouping before discovery, then applies a filter-specific stored order only when required.

With **All** or an unmodified filter active, Unreal's normal category order is preserved.

## Project Settings

Open **Project Settings > Plugins > Prune** to customize filter-editor appearance, description tooltips, and default window sizes.

Available settings:

- **Reorder Drag Color** - background of the category row currently being dragged.
- **Dragged Content Color** - grip, text, and checkbox foreground while dragging.
- **Drop Line Color** - color of the insertion line showing the pending reorder location.
- **Drop Line Thickness** - insertion-line thickness in Slate units.
- **Default Width** - initial width of New Filter and Edit Filter windows.
- **Default Height** - initial height of New Filter and Edit Filter windows.
- **Show Description Tooltips** - show saved Prune descriptions when hovering filter buttons in Actor Details.
- **Filter Manager Default Width** - initial width of the Filter Manager window.
- **Filter Manager Default Height** - initial height of the Filter Manager window.

The filter editor and Filter Manager remain resizable regardless of the default window-size settings. Reopen an existing window to pick up appearance changes made in Project Settings.

## Persistence

Prune persists per-user, per-project filter data through `EditorPerProjectUserSettings.ini`.

Saved data includes:

- custom filter definitions and descriptions,
- Global or Class scope,
- hidden category IDs,
- category order,
- per-class filter-button order,
- Epic filter override deltas,
- Epic filter override descriptions, scope, and order.

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
