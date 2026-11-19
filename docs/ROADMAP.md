# Prune Roadmap

**Current documented release:** Prune **v1.0.0** · Unreal Engine 5.8.x · Win64 · Editor-only.

## Release history

| Version | Status | Capability |
| --- | --- | --- |
| **v1.0.0 — Native Actor Details Filters** | Released | Custom filters, Epic overrides, Class/Global scope, category and button ordering, Filter Manager, import/export, descriptions and configurable appearance |
| **Future updates** | Exploration only | Potential refinements to authoring, workflows, compatibility, and filter sharing; specific features and version milestones have not been committed |

The repository does not provide a verified multi-release changelog for Prune. Unlike Focus, there is no confirmed earlier product history to invent here.

## What matters for future improvements?

The principles for evaluating additional Prune features are:

- **Stay native:** Keep the standard Unreal Actor Details layout and native left-click/Ctrl-click behavior.
- **Reduce browsing:** Focus on features that actually save time in complex Actor Details panels.
- **Preserve ownership:** Avoid destructive changes to Epic's definitions; retain reversible overrides.
- **Respect scope:** Keep Actor-class and Global behavior consistent and predictable.
- **Remain portable:** Make user-authored filters easy to share without coupling them to a particular Actor instance.
- **Preserve performance:** Category discovery and UI refresh should not add noticeable cost to large editor workflows.

These are criteria for future feature evaluation, **not an announced implementation roadmap or release guarantee**.

## Potential research areas, not committed features

Future user feedback may justify reviewing improved filter discovery, reusable configuration workflows, additional diagnostics for difficult Details customizations, and navigation conveniences for larger filter collections. None is claimed to ship in any particular version.

<!-- Media target: IMAGE | prune-roadmap | docs/assets/media/prune-roadmap.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-roadmap">
  <img src="assets/media-placeholder.png" alt="Prune released baseline and possible future directions" loading="lazy">
  <figcaption>The roadmap distinguishes Prune's available v1.0 filter-authoring capabilities from unscheduled improvements.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Create a clean diagram with v1.0.0 marked Released and the core filter features grouped beneath it, followed by a clearly separate Future / Under Evaluation area without an invented version number.</span></figcaption>
</figure>


For shipped behavior, see [Getting Started](01-getting-started/README.md), [Authoring Filters](02-authoring-filters/README.md), and [Managing Filters](03-managing-filters/README.md).
