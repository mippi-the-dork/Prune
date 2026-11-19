# 05. Support and Troubleshooting

Prune troubleshooting starts with the **Actor selection, the native Details filter row, and the active filter scope**. Most reported symptoms can be narrowed down by comparing the current view against Unreal's All filter.

<!-- Media target: IMAGE | prune-section-troubleshoot | docs/assets/media/prune-section-troubleshoot.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-section-troubleshoot">
  <img src="../assets/media-placeholder.png" alt="Comparing an active Prune filter to the All view" loading="lazy">
  <figcaption>All provides a quick way to verify that a category hidden by a filter still exists in Unreal's Details panel.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture two views of the same Actor, with a custom filter active on one and All active on the other. Highlight the categories present only in the All view.</span></figcaption>
</figure>


## Find the root of a problem

If controls are missing, confirm plugin activation, supported engine/platform, Actor selection and the normal Level Editor Details view. If a category is missing, switch to **All** before deciding that the property itself has disappeared.

If Edit is disabled, look for All or multiple active filters. If a native filter differs from expectations, inspect its override and Reset to Epic Default when appropriate. If import fails, verify the file is a valid Prune format version 1 export.

For unresolved cases, include the plugin/engine version, Actor class, filter type/scope, steps, and relevant Output Log lines at [Prune Issues](https://github.com/mippi-the-dork/Prune/issues).

## Explore Support and Troubleshooting

| Topic | What you'll learn | Design | Technical |
| --- | --- | --- | --- |
| **Troubleshooting Prune** | Resolve missing controls, categories and imports | [Design](troubleshooting/DESIGN.md) | [Technical](troubleshooting/TECHNICAL.md) |
| **FAQ, Updates and Support** | Compatibility, common questions and reporting | [Design](faq/DESIGN.md) | [Technical](faq/TECHNICAL.md) |

Each Design/Technical pair is maintained separately: user behavior in Design, exact engine ownership and constraints in Technical.
