# Prune 0.4.1

Prune is an Unreal Engine editor plugin that adds editable category presets to the standard Level Editor Actor Details panel.

0.4.1 moves Prune onto Unreal's native **Property Section** system. Prune presets now appear in the same filter row as Unreal's built-in **General**, **Actor**, **LOD**, **Physics**, **Rendering**, and other section buttons.

## Native Preset Row

Prune no longer adds a collapsible **Prune** category to the Details tree.

Instead:

- saved Prune presets appear as normal native section buttons,
- Unreal's existing **All** button remains the reset/show-everything state,
- clicking a Prune preset uses Unreal's own section filtering path,
- Ctrl-click continues to use Unreal's native multi-section behavior,
- Unreal itself remembers selected sections per Details class/view using its normal Details configuration.

Prune-created section names are internally unique and are registered on `AActor`, so they are inherited by Actor subclasses.

## Prune Management Control

A compact **Prune** menu is injected at the right side of the native section row.

The menu provides:

- **New Preset...**
- **Edit** for each existing Prune preset
- **Delete** for each existing Prune preset
- **Log Current Category IDs** for diagnostics

The preset editor lists the finished categories available for the current Actor layout. Checked categories are included by the preset. Unchecked categories are hidden by that preset.

If Unreal exposes multiple internal category IDs with the same display label, such as `Transform` and `TransformCommon`, Prune presents them as one checkbox while keeping both internal IDs in the saved preset.

## Class-Agnostic Deny-List Semantics

Prune still stores presets as class-agnostic deny-lists of internal category IDs.

When a category is discovered for the first time, Prune automatically adds it to every native Prune section unless that preset explicitly hides the category ID.

This preserves the existing Prune rule:

> Unknown or newly introduced categories default to visible.

A preset therefore remains useful across unrelated Actor classes without needing every class's category set ahead of time.

## Plugin Categories

Prune performs finished-category discovery after native and plugin customizations have contributed their categories.

This includes ordinary plugin-added Details categories such as:

- `Surface - Favorites`
- `Surface - Component Details`

Those categories can be included or excluded by a Prune preset just like native Unreal categories.

## Native Category Order Preservation

Prune still uses `SortCategories()` as the public finished-category discovery point.

UE 5.8 changes category grouping whenever a sort callback exists, so Prune reconstructs Unreal's normal simple/advanced-only grouping before reading the completed category map. Enabling Prune should therefore not reshuffle the standard Details category order.

## Section-Row Integration

UE 5.8 does not expose a public extension slot for the right side of the section-selector row.

Prune uses a narrow UE 5.8-specific Slate bridge:

- the standard Details view tags its native section `SWrapBox` as `SectionView`,
- `IDetailsView` is itself a Slate widget,
- Prune finds that tagged row after Unreal rebuilds it,
- Prune appends one management widget to the row,
- no engine source files are modified or replaced.

The native preset buttons themselves use only public `FPropertyEditorModule` / `FPropertySection` APIs.

## Persistence

Named Prune preset definitions persist per user/project through `EditorPerProjectUserSettings.ini`.

Existing 0.3.0 named presets use the same saved preset format and are loaded by 0.4.1.

The old 0.3.0 per-class **Custom** hidden-category state is no longer the active interaction model. Native Details sections now own the active filter selection.

## Target

- Unreal Engine 5.8.0 - 5.8.3
- Windows 64-bit
- Editor Only
- No engine source modifications
- Standalone plugin

## Current Limitations

- Level Editor instance Actor Details only.
- Preset editing lists categories available on the currently viewed Actor layout. Hidden IDs from other Actor classes are preserved when editing an existing preset.
- A preset that excludes every category available on a particular Actor may not produce a native section button for that Actor because Unreal only displays sections that map to at least one populated category.
- Deleting a preset that is selected in a Details view resets currently live views during refresh, but an unopened class can still have Unreal's own saved section selection pointing at the deleted section until the user selects **All**. This is an Unreal section-selection persistence edge case to harden in a later pass.
- The right-side management control depends on UE 5.8's `SectionView` Slate tag and section-row structure. It should be regression-tested on every supported engine update.
- No Component Details preset family yet.
- No Class Defaults integration yet.

## 0.4.1 Test Focus

Primary validation targets:

- Prune's old Details category is gone,
- the **Prune** management control appears on the right side of the native section row,
- creating a preset adds a native preset button before **All**,
- clicking a Prune preset filters through Unreal's native section system,
- **All** restores the complete Details view,
- editing and renaming a preset updates the same native section button,
- deleting a preset removes its button,
- preset definitions survive an editor restart,
- new category IDs default visible unless explicitly hidden by the preset,
- duplicate visible category names remain grouped in the editor,
- Surface categories remain available to presets,
- native category order remains unchanged,
- multiple Details panels each receive one Prune management control.

### 0.4.1 prototype fix

- Fixed a layout-context lifetime regression introduced when the old Prune Details category was removed.
- Finished-category discovery, native-order restoration, native preset-section syncing, and the section-row Prune management control now remain alive for the full lifetime of each Details layout.
