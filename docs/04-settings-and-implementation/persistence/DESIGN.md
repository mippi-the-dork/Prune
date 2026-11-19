# Saving, Scope and Editor State — Design

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

<!-- Media target: IMAGE | prune-state-ownership | docs/assets/media/prune-state-ownership.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-state-ownership">
  <img src="../../assets/media-placeholder.png" alt="Diagram of Prune config storage and Actor property data" loading="lazy">
  <figcaption>Prune stores filter definitions in editor-user configuration, separately from the Actors being inspected.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Create a three-part diagram: Prune editor-user config containing presets/overrides/order, the native Details filter row reading that config, and Actor/Component properties remaining untouched. Include a `.prunefilters.json` export as a separate portable file.</span></figcaption>
</figure>

## Filters are editor configuration

Prune's custom filters, descriptions, scope, category visibility and order, Epic overrides, and per-class button order persist through **EditorPerProjectUserSettings.ini**. The filters are editor-user state rather than data stored on individual Actors.

## What this means

Creating, editing, reordering or importing a filter does not change an Actor's actual property values. It changes which categories the editor displays and the order in which they appear.

## After restarting Unreal

Saved Prune filter definitions and scoped button order are intended to return when the editor starts again. The applicable controls and category availability still depend on the selected Actor's final Details layout.

## Sharing with teammates

Do not assume another developer receives your personal filters automatically. For reusable layouts, export supported custom filters to `.prunefilters.json` and share the file intentionally.

## Perforce and OFPA

Prune filter changes do not need to modify Actor or External Actor package data. If an Actor file becomes dirty, investigate other editor actions; Prune is not an Actor-data source-control management tool.

## Recovery

If you want a default Epic filter again, use **Reset to Epic Default** on its Prune override. If you want native button order, use **Reset Button Order**. These are distinct from deleting custom filters.

---

For engine details and implementation boundaries, [read Technical](TECHNICAL.md).
