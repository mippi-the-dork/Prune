# Your First Filter — Design

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

<!-- Media target: GIF | prune-first-filter | docs/assets/media/prune-first-filter.gif -->
<figure class="doc-media" data-kind="gif" data-media-id="prune-first-filter">
  <img src="../../assets/media-placeholder.png" alt="Creating and activating a custom Prune filter" loading="lazy">
  <figcaption>A named filter reduces a complex Actor's Details panel to the categories needed for the current task.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record a 10–15 second Unreal Editor animation: select a complex Actor, click +, name the filter Gameplay, uncheck two categories, reorder one, Save, and click Gameplay to show the simplified Details panel. End briefly on All.</span></figcaption>
</figure>

## Start with an Actor that has several categories

Select a Blueprint Actor or Static Mesh Actor in the Level Editor. Open its ordinary **Details** panel and locate the native filter buttons.

## Create a Gameplay view

1. Click Prune's **+ New Filter** button.
2. Give the filter a useful name, such as **Gameplay**. Optionally add a short description.
3. Choose **Class** scope for this Actor family, or **Global** if the same view should be available across Actor classes.
4. In the category list, leave the categories needed for the task checked, and uncheck unwanted categories.
5. Drag category rows into the order you want to read them.
6. Save the filter, then click its new button in the native filter row.

The Details panel now shows the categories selected for that filter. Click **All** to return to the ordinary unfiltered view.

## Try editing

Activate exactly one editable filter, then use Prune's **Edit Filter** control. The Edit control is unavailable when **All** or multiple Ctrl-selected filters are active.

## A good first test

Create a **Visuals** filter showing Transform and Rendering-related categories. Switch between it, Gameplay, and All, and confirm the Actor's property values remain unchanged. Prune is organizing the view, not editing Actor data.

## Need to share a filter?

Use the [Filter Manager](../../03-managing-filters/manager/DESIGN.md) to export compatible custom filters.

---

For the native Unreal implementation and limitations, [read Technical](TECHNICAL.md).
