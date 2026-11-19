# Native Details Integration — Design

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

<!-- Media target: IMAGE | prune-native-architecture | docs/assets/media/prune-native-architecture.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-native-architecture">
  <img src="../../assets/media-placeholder.png" alt="Diagram of Prune's source modules and Unreal Details integration" loading="lazy">
  <figcaption>Prune’s module, state controller and Slate customization cooperate with Unreal’s native Property Editor.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Draw a technical diagram showing FPruneModule binding OnExtendActorDetails, FPruneDetailsCustomization building controls, FPruneState managing definitions, FPruneLayoutContext observing final category groups, and GEditorPerProjectIni storing filters.</span></figcaption>
</figure>

## Why Prune looks native

Prune operates inside Unreal's ordinary Actor Details interface. It adds compact buttons, a native-style filter editor, and a Filter Manager instead of creating a second properties editor or converting properties to new assets.

## Category discovery

A Details panel is assembled from the selected Actor's properties and any applicable editor customizations. Prune discovers the **final available categories**, so relevant plugin-added categories can be included when present.

## Under the hood, from a user perspective

Prune registers custom filter choices with the same native section concepts used by Epic filters. Selecting a filter affects category visibility and optionally category order. You still use Unreal's normal property controls for the values themselves.

## Behavior to understand

- Epic filters remain Unreal-owned; Prune stores reversible overrides.
- Different Actor classes may have different available categories.
- With multiple Ctrl-selected filters, native combined visibility and category order prevail.
- Configuration and portable exports are separate from Actor or gameplay data.

## Integration limitations

Not every third-party Details panel uses the same native Actor Details filter row. Prune's documented workflow is the standard **Level Editor Actor Details** view rather than arbitrary custom property editor tabs.

---

For engine details and implementation boundaries, [read Technical](TECHNICAL.md).
