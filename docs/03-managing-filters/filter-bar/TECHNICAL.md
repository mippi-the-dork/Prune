# Filter Buttons and Context Menus — Technical

**Prune v1.0.0 · Editor-only · UE 5.8.x**

## Native filter button presentation

`FPruneLayoutContext::SetSectionSelectorWidget` records the native selector widget. `ApplySectionButtonPresentation` can arrange button presentation according to saved order keys and add description tooltips while keeping native checkbox behavior.

## Stable order keys

`FPruneState::MakePresetOrderKey` uses a `P:` prefix plus GUID for Prune presets. `MakeNativeOrderKey` uses `N:` plus native section `FName`. `SetFilterButtonOrder` stores a clean deduplicated sequence by Actor class and saves state.

## Dynamic refresh

Filter button display responds to native Details layout refresh and current active sections. The implementation tracks live layout contexts with weak references and refreshes eligible views without directly altering selected Actor data.

## Multiple selection

`GetActiveCategoryOrder` depends on a single resolved filter. With multiple active sections it returns no filter-specific order, leaving Unreal's normal order in effect.

## Context menu boundary

Prune supplies filter-specific context actions for compatible native Slate filter controls; other property editors and custom Details implementations may not expose the same native row structure.

---

For the practical workflow, [read Design](DESIGN.md).
