# 01. Getting Started

Prune extends the native **Actor Details Property Section filter row** with three compact actions. It is intended to reduce repetitive Details browsing while leaving all Actor values and native property editing behavior intact.

<!-- Media target: IMAGE | prune-section-start | docs/assets/media/prune-section-start.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-section-start">
  <img src="../assets/media-placeholder.png" alt="Prune filter controls within an Unreal Actor Details panel" loading="lazy">
  <figcaption>The standard Details filter row gains Prune's authoring controls without changing native property editing.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture the Details panel of a complex Actor with several existing Epic filters, two Prune filters, and the New, Manager and Edit controls in the same frame.</span></figcaption>
</figure>


## First experience

1. Install a matching UE 5.8.x Win64 Editor build, then enable Prune.
2. Select an Actor and locate the extra **New Filter**, **Filter Manager**, and **Edit Filter** controls in the ordinary Details filter row.
3. Create a named filter, check the categories you need, and choose whether it belongs to that Actor class or is Global.
4. Save and activate it. Use **All** to return to the full property list.

**Design** pages explain the user's controls and workflows. **Technical** pages cover native Editor hooks, data ownership and limitations.

## Explore Getting Started

| Topic | What you'll learn | Design | Technical |
| --- | --- | --- | --- |
| **What Is Prune?** | Purpose, capabilities and native Details integration | [Design](overview/DESIGN.md) | [Technical](overview/TECHNICAL.md) |
| **Installation and Compatibility** | UE 5.8.x Win64 setup and updating | [Design](installation/DESIGN.md) | [Technical](installation/TECHNICAL.md) |
| **Your First Filter** | Create, activate and inspect a simple view | [Design](first-use/DESIGN.md) | [Technical](first-use/TECHNICAL.md) |

Each Design/Technical pair is maintained separately: user behavior in Design, exact engine ownership and constraints in Technical.
