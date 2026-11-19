# Editing, Duplicating and Deleting — Design

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

<!-- Media target: GIF | prune-edit-duplicate | docs/assets/media/prune-edit-duplicate.gif -->
<figure class="doc-media" data-kind="gif" data-media-id="prune-edit-duplicate">
  <img src="../../../assets/media-placeholder.png" alt="Editing and duplicating a Prune filter" loading="lazy">
  <figcaption>Prune lets users adjust existing filters or create independent variants without changing Actor properties.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record activating a single Gameplay filter, clicking Edit, changing one category, saving, then using Duplicate to create Gameplay Copy. Show both buttons in the native Details filter row at the end.</span></figcaption>
</figure>

## Edit a custom filter

Activate exactly one editable filter and click **Edit Filter** beside the native filter buttons, or use the Filter Manager. You can change its description, scope, included categories and category order. For a custom filter you can also rename it.

## Why Edit is disabled

Edit is intentionally unavailable when **All** is selected, several filters are Ctrl-selected, or the active filter cannot be identified as a single editable target.

## Duplicate instead of overwriting

Duplicate a custom or compatible Epic filter when you need a variation. Prune creates an independent custom filter and assigns a collision-safe name such as **Gameplay Copy**.

## Deleting and resetting are different

**Delete** removes a custom Prune filter. **Reset to Epic Default** applies to an overridden Epic filter and removes the saved customization while retaining the built-in Epic filter.

## Recommended workflow

When experimenting with a built-in Rendering or Physics filter, duplicate it first if you want a long-term independent version. Use an override only when you specifically intend to change the built-in filter's effective behavior.

## Safety

Filter changes affect editor presentation and personal/project editor configuration, not Actor property data. A deleted filter can be recreated or reimported from a saved export.

---

For the native Unreal implementation and limitations, [read Technical](TECHNICAL.md).
