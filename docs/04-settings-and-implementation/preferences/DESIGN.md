# Project Settings and Appearance — Design

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

<!-- Media target: IMAGE | prune-settings | docs/assets/media/prune-settings.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-settings">
  <img src="../../assets/media-placeholder.png" alt="Prune plugin preferences in Unreal Project Settings" loading="lazy">
  <figcaption>Appearance and default window sizes can be customized without changing any Actor properties.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture Project Settings → Plugins → Prune with Filter Editor Reordering, Filter Editor Window, Filter Bar, and Filter Manager Window groups expanded. Show default numeric values and the tooltip setting.</span></figcaption>
</figure>

## Find Prune preferences

Open **Project Settings → Plugins → Prune**. These settings customize presentation and editor usability; they do not change an Actor's gameplay values.

## Filter Editor reordering

Choose **Reorder Drag Color** for the row background, **Dragged Content Color** for text/checkbox/grip, **Drop Line Color** for the insertion indicator, and **Drop Line Thickness** (default 2.0) to make drop placement easier to read.

## Window dimensions

The **New/Edit Filter** default window size is **560 × 640**. The **Filter Manager** defaults to **680 × 620**. Both windows remain freely resizable.

## Filter button tooltips

**Show Description Tooltips** is enabled by default. A filter's optional description can appear when hovering its button, making short names more understandable.

## Practical setup

On a high-resolution monitor, increase the Filter Editor width so long category names don't crowd the checkboxes or drag grips. Choose contrasting drag/drop colors that are readable against your Unreal Editor theme.

## State is separate

Prune window appearance preferences are different from custom filter definitions and per-class button order. Both are editor-only; neither changes Actor property data.

---

For engine details and implementation boundaries, [read Technical](TECHNICAL.md).
