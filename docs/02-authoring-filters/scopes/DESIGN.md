# Class and Global Filters — Design

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

<!-- Media target: IMAGE | prune-scopes | docs/assets/media/prune-scopes.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-scopes">
  <img src="../../assets/media-placeholder.png" alt="Prune Class versus Global scope option" loading="lazy">
  <figcaption>Class filters serve an Actor family; Global filters expose reusable views across Actor classes.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture the filter editor with Class/Global scope clearly visible and an Actor class label or tooltip. If possible include two side-by-side views showing the same Global filter available for different Actor types.</span></figcaption>
</figure>

## Two availability scopes

**Class** filters apply to the chosen Actor class and are available to appropriate derived classes. **Global** filters are available across different Actor classes.

## Choose Class when...

You are authoring a specialized filter for a family such as `BP_Enemy` and its derived `BP_BossEnemy`. The relevant gameplay categories might not exist on unrelated environment Actors.

## Choose Global when...

You want the same general-purpose filter (for example **Rendering** or **Debug**) accessible on a wide variety of Actor classes.

## Creating and changing scope

Select a representative Actor, open New Filter, and choose the desired scope. You can modify an existing editable filter's scope later. A Class filter requires a resolvable Actor class.

## Category availability still varies

Global means a filter button can be offered across classes; it does not make nonexistent categories appear on an Actor. Native Details category availability still depends on the selected objects and their customizations.

## Multiple Actor classes

When an editor selection spans different Actor classes, a single unambiguous class-specific authoring context may not be available. Use an appropriate single-Actor selection to create the intended Class filter.

---

For the native Unreal implementation and limitations, [read Technical](TECHNICAL.md).
