# 02. Authoring Filters

Authoring a Prune filter means defining a **view of existing Details categories**, not creating or editing Actor properties. Each definition includes a name, optional description, scope, visibility choices and category order.

<!-- Media target: GIF | prune-section-author | docs/assets/media/prune-section-author.gif -->
<figure class="doc-media" data-kind="gif" data-media-id="prune-section-author">
  <img src="../assets/media-placeholder.png" alt="Creating a filter and choosing relevant Details categories" loading="lazy">
  <figcaption>A custom Prune filter makes frequently used categories easier to find without hiding their underlying property data permanently.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record clicking New Filter, entering a name, checking useful categories, dragging one category into a new position and saving the finished filter. Show the resulting filtered Details layout.</span></figcaption>
</figure>


## One filter, one task

A Gameplay filter might show Transform, Movement, AI, Collision and Gameplay where those categories exist. A Visuals filter could prioritize Rendering, Materials and Lighting. Both are views of the same Actor data.

Use category search to find relevant controls without changing checkbox state. Drag categories into a deliberate reading order, save the filter, and switch between it and **All** to compare. For reuse across many Actor classes, choose Global; otherwise Class can limit availability to a family.

**Multiple Ctrl-selected filters** use Unreal's native combined category visibility and order, not the order of an arbitrary single filter.

## Explore Authoring Filters

| Topic | What you'll learn | Design | Technical |
| --- | --- | --- | --- |
| **Creating Custom Filters** | Name, describe and build custom category views | [Design](create/DESIGN.md) | [Technical](create/TECHNICAL.md) |
| **Choosing and Searching Categories** | Control which Details categories appear | [Design](categories/DESIGN.md) | [Technical](categories/TECHNICAL.md) |
| **Reordering Details Categories** | Drag categories into workflow-specific order | [Design](ordering/DESIGN.md) | [Technical](ordering/TECHNICAL.md) |
| **Class and Global Filters** | Choose one Actor family or broad availability | [Design](scopes/DESIGN.md) | [Technical](scopes/TECHNICAL.md) |
| **Editing, Duplicating and Deleting** | Maintain independent custom presets | [Design](editing/DESIGN.md) | [Technical](editing/TECHNICAL.md) |

Each Design/Technical pair is maintained separately: user behavior in Design, exact engine ownership and constraints in Technical.
