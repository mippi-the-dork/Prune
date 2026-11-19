# Editing Epic Filters — Design

**Prune v1.0.0 · Editor-only · UE 5.8.x**

<!-- Media target: GIF | prune-epic-override | docs/assets/media/prune-epic-override.gif -->
<figure class="doc-media" data-kind="gif" data-media-id="prune-epic-override">
  <img src="../../assets/media-placeholder.png" alt="Editing and resetting an Epic Details filter" loading="lazy">
  <figcaption>Prune can customize a built-in Epic filter and later return it to Unreal’s original definition.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record selecting a native Rendering filter, opening Edit, hiding an unrelated category, saving, then using Reset to Epic Default. Show the category differences before and after reset.</span></figcaption>
</figure>

## Built-in filters are editable without replacing them

Prune can customize compatible Epic Property Section filters, including **General**, **Actor**, **LOD**, **Misc**, **Physics**, **Rendering** and **Streaming** where available for the selected Actor.

## Create an Epic override

1. Select an Actor and activate exactly one compatible Epic filter in its Details filter row.
2. Use Prune's **Edit Filter** control or choose Edit from its context menu.
3. Set the desired category visibility, category order, description and Class/Global scope.
4. Save to create a **Prune override** for that Epic filter.

Epic's original definition is preserved underneath. The filter button keeps its native name; Prune stores only your customization.

## Reset to Epic Default

When you no longer want the override, choose **Reset to Epic Default** for the overridden filter. This removes Prune's customization and returns to Unreal's underlying filter definition.

## Duplicate an Epic filter

If you want an independent filter instead of changing the effective Epic button, choose **Duplicate**. Prune creates a new **custom** filter initialized from the Epic filter's current settings. That copy can be renamed, exported and deleted without touching Epic's original definition.

## Why two options?

Use an **override** when you want to keep using the familiar Epic button with adjusted categories. Use **Duplicate** when you want a separate workflow-specific button.

---

For native implementation details, [read Technical](TECHNICAL.md).
