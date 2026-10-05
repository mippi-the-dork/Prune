# Prune 0.3.0

Prune is an Unreal Engine editor plugin for hiding whole categories in the standard Level Editor Actor Details panel.

0.3.0 adds persistent per-class state and reusable named presets on top of the category discovery, plugin compatibility, duplicate-label grouping, and native-order preservation validated in 0.1.x and 0.2.0.

## Core Workflow

For an Actor selected in the Level Editor, Prune adds a **Prune** category with two controls:

- **Preset** - apply or manage reusable category deny-lists,
- **Categories** - manually show or hide the categories currently available for the selection.

The selection label remains visible beside the controls so it is clear which Actor class is being edited.

## Persistent Per-Class State

Exact Actor-class selections now persist for the current user and project through `EditorPerProjectUserSettings.ini`.

Examples:

- hide **Physics** and **Rendering** on `StaticMeshActor`,
- hide **Volumetric Fog** on `ExponentialHeightFog`,
- restart Unreal,
- each class restores its own saved Prune state when selected again.

Blueprint-generated Actor classes use the generating Blueprint asset path as their identity so normal Blueprint recompilation does not create a new saved state.

Mixed-class selections remain session-only. They are selection compositions rather than Actor classes, so Prune does not write those temporary combinations to disk.

## Named Presets

Presets are class-agnostic deny-lists of internal category IDs.

A preset can hide categories such as:

```text
Physics
Navigation
HLOD
Collision
```

When that preset is applied to a class that does not contain one of those categories, nothing special happens. Unknown or newly introduced categories remain visible by default.

The **Preset** menu contains:

- **All Categories** - clear the saved filter for the current selection,
- every saved named preset,
- **Save Current as Preset...**,
- **Delete Active Preset...** when a named preset is active.

Saving a preset records the current hidden internal category IDs and immediately marks that preset as active for the current selection.

Deleting a preset does not unexpectedly reveal categories. Any class that was using it keeps its current hidden-category rules and becomes **Custom** instead.

## Preset State

The Preset button displays one of these states:

```text
Preset: All Categories
Preset: Custom
Preset: Level Design
Preset: Lighting
```

Directly changing a category checkbox while a named preset is active changes that selection to **Custom**. The named preset itself is not edited.

This keeps preset definitions predictable and avoids silently changing every other class that uses the same preset.

## Duplicate Display Names

Unreal can expose multiple internal category IDs with the same visible label. A common example is:

```text
Transform
TransformCommon
```

Both may display as **Transform**.

Prune presents those IDs as one checkbox while keeping the real IDs separate internally for state, presets, logging, and compatibility.

## Finished Category Discovery

Prune attaches through UE 5.8.3's public `OnExtendActorDetails` delegate and uses `SortCategories()` as the completed-category discovery point.

This allows Prune to see categories contributed later by other Details customizations, including plugin-added categories such as:

- `Surface - Favorites`,
- `Surface - Component Details`.

Prune's own internal category ID, `PruneControl`, is explicitly excluded and can never appear as a Prune checkbox.

## Native Category Order Preservation

UE 5.8 changes category grouping when any `SortCategories()` callback exists. Prune compensates for that behavior before reading the completed category map so simply enabling Prune does not interleave advanced-only categories into Unreal's normal category block.

Interactive filtering uses `IDetailCategoryBuilder::SetCategoryVisibility()` rather than rebuilding the Details layout. Hiding and restoring categories therefore preserves the established category order, including categories positioned by other plugins.

## Multiple Details Views

Each live Actor Details layout gets its own category-builder context while saved state is keyed by Actor class.

This means:

- two live Details views showing the same Actor class share that class's Prune state,
- a locked Details view showing another class keeps its own state,
- destroyed or rebuilt Details layouts do not leave permanent raw category-builder pointers in Prune state.

## Target

- Unreal Engine 5.8.0 - 5.8.3
- Windows 64-bit
- Editor Only
- No Engine source modifications
- Standalone plugin

## Current Limitations

- Level Editor instance Actor Details only.
- Mixed-class selection state is session-only.
- No preset rename or full preset editor yet.
- No Project Settings UI yet.
- No Component Details preset family.
- No Class Defaults integration.
- No native filter-row injection yet.
- The Prune control still lives in its own Details category, so Unreal's native General/Actor/LOD/etc. filters can temporarily hide the Prune row itself.

## 0.3.0 Test Focus

The main validation targets are:

- per-class Custom state survives an editor restart,
- named presets survive an editor restart,
- a named preset can be applied to multiple unrelated Actor classes,
- unknown categories remain visible when a preset is applied,
- direct category edits switch a named preset to Custom without changing the preset,
- deleting a preset preserves current visibility and converts affected classes to Custom,
- Blueprint recompilation keeps persistent class identity,
- mixed selections remain session-only,
- multiple Details views continue to synchronize correctly,
- Surface categories and native category order remain intact.
