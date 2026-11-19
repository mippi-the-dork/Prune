# 04. Settings and Implementation

Prune has two different configuration responsibilities: **presentation preferences** such as drag colors and window sizes, and **saved filter state** such as names, category selection, overrides and per-class button order. Neither is attached to individual Actors.

<!-- Media target: IMAGE | prune-section-settings | docs/assets/media/prune-section-settings.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-section-settings">
  <img src="../assets/media-placeholder.png" alt="Prune Project Settings and persisted filter state diagram" loading="lazy">
  <figcaption>Prune's display preferences and per-user filter definitions are separate from Actor property data.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Make a labeled two-column diagram showing Project Settings preferences and EditorPerProjectUserSettings filter data, with a separate Actor properties block explicitly unchanged by Prune.</span></figcaption>
</figure>


## Where the data lives

Appearance settings are exposed under **Project Settings → Plugins → Prune**. The user-authored filter definitions live in per-project **editor-user configuration**. Portable JSON exports are separate files intended for sharing and transfer.

The implementation extends the standard Actor Details panel through `OnExtendActorDetails` and native Property Sections. It reads category IDs after relevant editor customizations have run, so applicable plugin-added categories can participate.

**Saving a filter is not saving an Actor.** Prune neither stores a component on the Actor nor changes packaged gameplay properties.

## Explore Settings and Implementation

| Topic | What you'll learn | Design | Technical |
| --- | --- | --- | --- |
| **Project Settings and Appearance** | Customize drag feedback, tooltips and window sizes | [Design](preferences/DESIGN.md) | [Technical](preferences/TECHNICAL.md) |
| **Saving, Scope and Editor State** | Understand user config versus Actor data | [Design](persistence/DESIGN.md) | [Technical](persistence/TECHNICAL.md) |
| **Native Details Integration** | Understand Prune's editor module and Property Sections | [Design](architecture/DESIGN.md) | [Technical](architecture/TECHNICAL.md) |

Each Design/Technical pair is maintained separately: user behavior in Design, exact engine ownership and constraints in Technical.
