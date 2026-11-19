# Creating Custom Filters — Design

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

<!-- Media target: IMAGE | prune-create-dialog | docs/assets/media/prune-create-dialog.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-create-dialog">
  <img src="../../assets/media-placeholder.png" alt="Prune's New Filter window showing name, description and category choices" loading="lazy">
  <figcaption>A custom filter combines its workflow name, scope and visible Details categories in one editor dialog.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture the New Filter dialog with Gameplay entered as the name, a realistic description, Class selected and several category checkboxes visible. Keep Save and Cancel in frame.</span></figcaption>
</figure>

## What a custom filter contains

A Prune custom filter stores a **name**, optional **description**, **Class or Global scope**, category visibility choices, and an optional category order.

## Create a filter

Click **+** beside the native Actor Details filter row, enter a short workflow name, select a scope, and customize the category checkboxes. Save to add its button beside the existing Property Section filters.

Good names describe the task: **Gameplay**, **Environment**, **Combat**, **Lighting**, or **Debug**. Avoid two indistinguishable filters for the same context.

## What Save does

Saving creates an independent Prune filter definition. It does **not** create an Actor, alter property values, or delete hidden categories. Activate the named button to view its subset, or click **All** to return to the complete Details layout.

## Descriptions

Use a one-sentence description when the filter name could be ambiguous. The description appears in Filter Manager and may appear as a button tooltip when the related setting is enabled.

## If a name collides

Prune validates names within the applicable native/custom filter context. When duplicating or importing, it generates collision-safe names such as **Gameplay Copy** and **Gameplay Copy 2**.

## Recommended workflow

Start with Class scope when a filter is specific to an Actor family. Use Global for common categories useful across many different Actors.

---

For the native Unreal implementation and limitations, [read Technical](TECHNICAL.md).
