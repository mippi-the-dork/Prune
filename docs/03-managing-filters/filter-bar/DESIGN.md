# Filter Buttons and Context Menus — Design

**Prune v1.0.0 · Editor-only · UE 5.8.x**

<!-- Media target: IMAGE | prune-filter-bar | docs/assets/media/prune-filter-bar.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-filter-bar">
  <img src="../../../assets/media-placeholder.png" alt="Prune's native filter row with controls and contextual actions" loading="lazy">
  <figcaption>Filter buttons remain native Unreal controls while Prune adds authoring shortcuts and context menus.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture a close-up of the Actor Details filter row showing All, at least two Epic filters, two Prune filters and +, Manager, Edit icons. Open one Prune filter's right-click menu with its available actions legible.</span></figcaption>
</figure>

<!-- Media target: GIF | prune-button-order | docs/assets/media/prune-button-order.gif -->
<figure class="doc-media" data-kind="gif" data-media-id="prune-button-order">
  <img src="../../../assets/media-placeholder.png" alt="Reordering filter buttons from the Prune Filter Manager" loading="lazy">
  <figcaption>The Filter Manager can move favorite filter buttons ahead of less-used native filters.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record dragging a Gameplay filter row above General in Filter Manager, closing the manager, then showing the reordered native filter buttons. Include Reset Button Order if it fits within a short sequence.</span></figcaption>
</figure>

## Prune keeps the native filter bar

Normal **left-click** selection and **Ctrl-click** combination of Property Section filters remain Unreal-owned. Prune extends the row with authoring and management controls, rather than replacing the native selection behavior.

## Reorder filter buttons

1. Open Filter Manager for the selected Actor class.
2. Drag filter rows into the order you want the buttons to appear.
3. Close the manager. The new button order applies immediately and is remembered for that Actor class.

For example, **Gameplay** and **Debug** can precede Epic's **General** and **Rendering** filters.

## Reset Button Order

Choose **Reset Button Order** to discard the custom order for the current Actor class and restore native order. The reset action is unavailable if no custom order exists.

## Right-click actions

Right-click a compatible filter button to access contextual actions such as Edit, Duplicate, Export, Delete, Reset, Filter Manager and Settings. Menu entries depend on whether the filter is custom, native or overridden.

## Several filters selected with Ctrl

Unreal determines the combined category visibility. Prune deliberately disables its single-filter Edit action and does not impose one filter's custom category order over another.

## Two kinds of ordering

**Category order** changes the position of Details categories *inside* a filter. **Button order** changes where filter buttons appear in the filter row. These are separate preferences.

---

For native implementation details, [read Technical](TECHNICAL.md).
