# Troubleshooting Prune — Design

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

<!-- Media target: IMAGE | prune-troubleshooting | docs/assets/media/prune-troubleshooting.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-troubleshooting">
  <img src="../../assets/media-placeholder.png" alt="Comparing a filtered Details panel against All" loading="lazy">
  <figcaption>Switching to All is a reliable first check when a property appears missing from an active Prune filter.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture the same Actor with a custom filter active in one view and All in another, showing that a hidden category becomes visible again. Include the active filter labels.</span></figcaption>
</figure>

## Prune controls do not appear

Check that the plugin is installed/enabled for a supported **UE 5.8.x Win64 editor**, select an Actor in the Level Editor, and use its normal Actor Details panel. Restart after enabling Prune if prompted. Prune targets the native Actor Details filter row, not every arbitrary Details-like tab.

## A category is missing from the editor

Make sure your Actor actually exposes that category under **All**. Categories depend on Actor/component class and active customizations. If you just introduced a new component or plugin category, rebuild/refresh the Details view before deciding the category is absent.

## A filter hides something unexpectedly

Switch to **All**, confirm the property still exists, then edit the active filter and inspect which category IDs are checked. A filtered-out property is not deleted. If multiple filters are Ctrl-selected, Unreal's combined selection behavior controls visibility.

## Edit Filter is disabled

Activate **one** editable filter. The control is disabled for All, multi-filter Ctrl selection, or an ambiguous/non-editable section.

## Ordering doesn't behave as expected

Distinguish **Details category order** inside an active filter from **filter-button order** managed per Actor class. A custom category order is deliberately not applied when multiple filters are active.

## Epic filter seems wrong

Find the corresponding **Epic Override** in Filter Manager and use **Reset to Epic Default** if you want the native definition back.

## Import or export fails

Use `.prunefilters.json`, confirm the file is valid and supports format version **1**, and that the destination has a valid selected Actor class for Class-scoped imports. Native Epic filters are not exported directly—duplicate one to make it portable.

## Reporting a bug

Include Prune/UE versions, Actor class, exact filter name and scope, whether it is Epic/custom/override, steps, screenshots and relevant Output Log lines. Report at [Prune Issues](https://github.com/mippi-the-dork/Prune/issues).

---

For engine details and implementation boundaries, [read Technical](TECHNICAL.md).
