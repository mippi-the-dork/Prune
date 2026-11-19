# Reordering Details Categories — Design

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

<!-- Media target: GIF | prune-category-order | docs/assets/media/prune-category-order.gif -->
<figure class="doc-media" data-kind="gif" data-media-id="prune-category-order">
  <img src="../../../assets/media-placeholder.png" alt="Reordering categories in Prune's filter editor" loading="lazy">
  <figcaption>Each filter can present the same Actor categories in a different order that suits its workflow.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record a GIF showing at least eight filter-editor categories, dragging Gameplay above Rendering, the blue insertion line, Save, and the resulting Details panel category order. Keep the dragged category visible throughout.</span></figcaption>
</figure>

## Put useful categories first

A filter can arrange the visible Details categories in a deliberate order. You might place **Transform** first, followed by **Movement**, **Interaction**, and **Gameplay**, rather than using the Actor's default ordering.

## How to reorder

1. Open New Filter or Edit Filter.
2. Find a category's drag grip.
3. Drag the row up or down. Prune shows the dragged-row feedback and an insertion line.
4. Drop it at the new position, then Save.
5. Activate the filter to see the resulting order in the ordinary Details panel.

Unselected categories remain hidden while the filter is active. A filter can maintain its own order without changing another filter's order.

## Search and reordering

If you are searching categories, clear the search before final review so you can see the intended complete order.

## Multiple active filters

When **Ctrl** combines multiple native Property Section filters, Prune deliberately avoids imposing one custom filter's order; Unreal's native category ordering is used instead.

## Resetting the order

Editing a filter and restoring its normal category order returns it to Unreal's underlying layout order. See [Button Ordering](../../03-managing-filters/filter-bar/DESIGN.md) for the separate operation that rearranges filter **buttons**, not Details categories.

---

For the native Unreal implementation and limitations, [read Technical](TECHNICAL.md).
