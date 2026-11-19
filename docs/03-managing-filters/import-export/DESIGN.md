# Importing and Exporting Filters — Design

**Prune v1.0.0 · Editor-only · UE 5.8.x**

<!-- Media target: GIF | prune-import-export | docs/assets/media/prune-import-export.gif -->
<figure class="doc-media" data-kind="gif" data-media-id="prune-import-export">
  <img src="../../../assets/media-placeholder.png" alt="Exporting and importing a Prune filter preset" loading="lazy">
  <figcaption>Portable JSON exports let the same filter layout be reused in another project or Actor class.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record exporting a Gameplay filter from Filter Manager, then selecting another Actor class and importing the saved .prunefilters.json file. Show the imported filter appearing with a safe copy name if a collision occurs.</span></figcaption>
</figure>

## Portable format

Prune exports custom filter definitions as JSON files normally named with the `.prunefilters.json` extension. These files can be imported into a different project or Actor class.

## Export multiple filters

Open **Filter Manager → Export**. Prune exports eligible custom filters available in the manager's current context, including their names, descriptions, scopes, hidden category identifiers and category order.

Epic-owned native filters are **not directly exported**. Duplicate one to create a Prune-owned filter if you want a portable version.

## Export one filter

Right-click an eligible custom filter button and choose **Export** when available.

## Import a saved file

1. Select a representative Actor for the desired destination class.
2. Open Filter Manager and choose **Import**.
3. Select a supported `.prunefilters.json` file.
4. Review the imported filters in the manager and the Actor Details bar.

Global filters remain Global. Imported Class filters are mapped to the Actor class currently managed. Imported filters receive fresh internal IDs so they do not overwrite existing filters; name collisions use safe Copy names.

## Validation and safety

An invalid file, unsupported format version or malformed JSON is rejected. Do not assume an imported filter has every category on a different Actor; its actual Details categories still depend on the destination class.

## Team sharing

Exports are the practical way to share a chosen filter setup. Prune's normal editor-user persistence is distinct from a committed preset file.

---

For native implementation details, [read Technical](TECHNICAL.md).
