# Filter Manager — Design

**Prune v1.0.0 · Editor-only · UE 5.8.x**

<!-- Media target: IMAGE | prune-manager | docs/assets/media/prune-manager.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-manager">
  <img src="../../assets/media-placeholder.png" alt="Prune Filter Manager showing different filter types and actions" loading="lazy">
  <figcaption>The manager combines custom presets, Epic filters and overrides in a searchable list.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture Filter Manager populated with an Epic filter, Epic Override, Prune Class and Prune Global filter. Show descriptions, active selection, visible-category counts, and the New, Import, Export, Reset Button Order and Done controls.</span></figcaption>
</figure>

## All relevant filters in one view

The **Filter Manager** opens from the Prune control beside Actor Details and shows filters applicable to the currently selected Actor class. Entries distinguish **Epic**, **Epic Override**, **Prune Class** and **Prune Global** filters. The active filter is identified.

## Available tasks

From the manager you can search for a filter by name, create a new filter, edit an existing one, duplicate, delete a Prune filter, reset an Epic override, import or export custom filters, and arrange filter-button order.

## Typical cleanup workflow

1. Open Filter Manager on a representative Actor.
2. Search for outdated or duplicated filter names.
3. Inspect filter type, scope, description and visible-category count.
4. Edit or duplicate a useful preset; delete only a truly unwanted Prune-owned filter.
5. Drag filter rows to reorder buttons. Choose **Reset Button Order** if you want to restore the native order.
6. Click **Done** to close the manager.

## Important ownership distinction

Delete is for custom Prune filters; Reset applies to Epic overrides. An Epic filter is not deleted from Unreal.

## Scope context

The manager is associated with the current Actor class. A Global filter may appear on many classes, while Class filters are limited to the relevant class family.

---

For native implementation details, [read Technical](TECHNICAL.md).
