# Choosing and Searching Categories — Design

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

<!-- Media target: IMAGE | prune-categories | docs/assets/media/prune-categories.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-categories">
  <img src="../../../assets/media-placeholder.png" alt="Category checkboxes and category search in the Prune filter editor" loading="lazy">
  <figcaption>Prune lets each filter include the relevant Details categories while leaving unrelated property data intact.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture the filter editor with at least eight category names, several checked and unchecked rows, and a search query narrowing the visible choices. Keep a representative row's checkbox and category label readable.</span></figcaption>
</figure>

## Show only the categories that matter

The filter editor lists categories discovered for the selected Actor's Details layout. Check a category to include it. Uncheck it to hide it while the filter is active.

For a gameplay view, keep **Transform, Movement, Interaction, Collision and Gameplay** where those categories exist; uncheck categories unrelated to the task.

## The search field

Use the category search box to narrow the list by category display name. **Searching does not toggle checkboxes or delete categories.** Clear the search to review your complete selection before saving.

## Newly discovered categories

Details layouts change as Actor classes, Blueprints or other editor customizations evolve. Previously unknown category IDs aren't silently discarded. Prune preserves explicitly stored ordering and lets new categories appear after the stored entries using Unreal's normal relative order.

## Duplicate labels

Several internal category identifiers may share one displayed label. Prune groups these into a single user-facing row so the editor does not ask you to manage duplicate-looking entries.

## Inspecting results

Save and activate the filter, then compare its categories with **All**. Hidden categories are still available when switching to another filter; their property data is untouched.

---

For the native Unreal implementation and limitations, [read Technical](TECHNICAL.md).
