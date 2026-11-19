# What Is Prune? — Design

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

<!-- Media target: IMAGE | prune-overview | docs/assets/media/prune-overview.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-overview">
  <img src="../../../assets/media-placeholder.png" alt="Prune controls beside Unreal's native Actor Details filter row" loading="lazy">
  <figcaption>Prune extends the existing Actor Details filter bar, keeping custom views alongside Unreal's normal filters.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture an Unreal Editor Details panel for a complex Actor with several native filters, two clearly named custom Prune filters, and all three Prune controls visible. Keep the Actor name and some expanded categories in frame.</span></figcaption>
</figure>

## The problem Prune solves

Complex Actors can expose dozens of categories in the Unreal Engine **Details** panel. Unreal's built-in Property Section filters are useful, but their groupings don't always match a designer's task. **Prune** lets you create a focused view of the categories you need, without moving or deleting any Actor properties.

## What changes when Prune is enabled?

Prune adds **New Filter**, **Filter Manager**, and **Edit Filter** controls beside the existing Details filter row. You can create a **Gameplay** filter that shows Transform, Movement, AI and Collision while hiding unrelated Rendering or Debug categories. Selecting **All** returns to Unreal's ordinary unrestricted Details view.

Prune also supports editing compatible Epic filter definitions through reversible overrides, global and Actor-class scopes, category order, filter-button order, descriptions and portable presets.

## What Prune does not do

A Prune filter changes **property presentation**, not gameplay values, Actor serialization, compiled Blueprints or runtime replication. It is an **Editor-only** plugin and does not add runtime components.

## Example

A gameplay designer and environment artist can select the same complex Actor, but use different named filters to see Gameplay or Visuals-oriented Details categories. The underlying Actor remains identical.

## Where to continue

[Install and enable Prune](../installation/DESIGN.md), then [create your first filter](../first-use/DESIGN.md).

---

For the native Unreal implementation and limitations, [read Technical](TECHNICAL.md).
