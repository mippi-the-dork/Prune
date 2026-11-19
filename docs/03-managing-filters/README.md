# 03. Managing Filters

The Filter Manager collects custom presets, Epic native filters, Epic overrides and the filter bar's saved ordering in one window. Management actions preserve the distinction between **Unreal-owned** and **Prune-owned** definitions.

<!-- Media target: IMAGE | prune-section-manager | docs/assets/media/prune-section-manager.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-section-manager">
  <img src="../assets/media-placeholder.png" alt="Filter Manager with built-in, overridden and custom filters" loading="lazy">
  <figcaption>The manager distinguishes filter ownership, scope and category coverage while providing editing and sharing actions.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture the full Filter Manager showing Epic, Epic Override, Class and Global entries, an active filter, and the import/export and reset controls.</span></figcaption>
</figure>


## Choose the operation intentionally

| You want to... | Use |
| --- | --- |
| Modify the effective General/Physics/Rendering button | Edit a compatible Epic filter to create an override |
| Return that button to native behavior | Reset to Epic Default |
| Make an independently editable version | Duplicate into a custom Prune filter |
| Move a favorite button earlier in the row | Filter Manager drag-and-drop ordering |
| Restore ordinary filter-button order | Reset Button Order |
| Transfer custom filters between projects | Export/Import `.prunefilters.json` |

Prune does not permanently replace Epic's native definitions and does not export native Epic filters directly.

## Explore Managing Filters

| Topic | What you'll learn | Design | Technical |
| --- | --- | --- | --- |
| **Editing Epic Filters** | Use reversible overrides or independent copies | [Design](epic-filters/DESIGN.md) | [Technical](epic-filters/TECHNICAL.md) |
| **Filter Manager** | Inspect and manage all applicable filter definitions | [Design](manager/DESIGN.md) | [Technical](manager/TECHNICAL.md) |
| **Filter Buttons and Context Menus** | Reorder buttons and preserve native selection | [Design](filter-bar/DESIGN.md) | [Technical](filter-bar/TECHNICAL.md) |
| **Importing and Exporting Filters** | Share portable versioned JSON definitions | [Design](import-export/DESIGN.md) | [Technical](import-export/TECHNICAL.md) |

Each Design/Technical pair is maintained separately: user behavior in Design, exact engine ownership and constraints in Technical.
