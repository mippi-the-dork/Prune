# FAQ, Updates and Support — Design

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

<!-- Media target: IMAGE | prune-update-location | docs/assets/media/prune-update-location.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-update-location">
  <img src="../../assets/media-placeholder.png" alt="Installed Prune plugin folder before replacing a release" loading="lazy">
  <figcaption>Replacing the complete plugin folder avoids mixing source files and binaries from different releases.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture the Windows project Plugins/Prune folder with Prune.uplugin visible and the matching release ZIP shown nearby. Avoid showing personal usernames or proprietary project content.</span></figcaption>
</figure>

## Frequently asked questions

**Does Prune alter my Actors?** No. It changes which native Details categories you see and their order.

**Can I change Epic filters?** Yes, through reversible Prune overrides. The original Epic definition remains underneath.

**Can I edit several selected filters at once?** No. Ctrl-combining filters remains native Unreal behavior; Prune's Edit control requires one unambiguous active filter.

**Do filters automatically transfer to teammates?** Not through Actor packages. Export suitable custom filters to `.prunefilters.json` and share the file deliberately.

**Why isn't a Global filter showing a category on my Actor?** Global controls the filter's scope, not which native categories the Actor possesses.

**Can I rename an Epic filter?** The original Epic name stays read-only. Duplicate it into a custom Prune filter for an independently named version.

**Does Prune run in packaged games?** No. It is an Editor-only plugin.

## Updating safely

Close Unreal, replace the full `Plugins/Prune` release folder, then reopen your project. If building from source, update your checkout and build the matching Editor target. Prune filter user settings are stored separately from the plugin folder.

## Compatibility and reporting

Current documented target: **v1.0.0 for Unreal Engine 5.8.x, Win64**. For an issue, capture the exact Actor class, Prune filter type, current category selection and reproduction steps.

[GitHub Releases](https://github.com/mippi-the-dork/Prune/releases) · [Issues](https://github.com/mippi-the-dork/Prune/issues) · [Source](https://github.com/mippi-the-dork/Prune).

---

For engine details and implementation boundaries, [read Technical](TECHNICAL.md).
