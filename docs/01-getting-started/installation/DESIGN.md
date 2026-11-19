# Installation and Compatibility — Design

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

<!-- Media target: IMAGE | prune-install | docs/assets/media/prune-install.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-install">
  <img src="../../assets/media-placeholder.png" alt="Prune plugin installed under an Unreal project's Plugins directory" loading="lazy">
  <figcaption>A project-local installation keeps the plugin descriptor directly inside the Prune folder.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture Windows File Explorer at YourProject/Plugins/Prune, showing Prune.uplugin plus Source and Resources. Keep the address bar visible and avoid an extra nested Prune folder.</span></figcaption>
</figure>

## Supported target

Prune v1.0.0 targets **Unreal Engine 5.8.x on Win64**. It is an Editor plugin; no separate runtime setup is required.

## Install from Fab or a packaged release

Use a matching Fab installation or download a compatible ZIP from [Prune Releases](https://github.com/mippi-the-dork/Prune/releases). For a project-local installation, close Unreal Editor and extract the entire plugin folder under:

```text
YourProject/
  Plugins/
    Prune/
      Prune.uplugin
      Source/
      Resources/
      Config/
```

The descriptor must be at `Plugins/Prune/Prune.uplugin`, not in an extra nested folder. Reopen the editor, enable Prune under **Edit → Plugins** if necessary, and restart if prompted.

## Install from source

Clone the [Prune repository](https://github.com/mippi-the-dork/Prune) under `YourProject/Plugins/Prune`. Generate project files if your UE C++ project requires it, then compile the project **Editor** target.

## Confirm activation

Select an Actor in the Level Editor. The usual Details filter row should now include the additional Prune controls for **New**, **Manager**, and **Edit**.

## Updating or troubleshooting

Replace the *whole* old plugin directory while the editor is closed; avoid mixing binaries from different releases. If the controls do not appear, confirm the correct Unreal version, plugin activation, an Actor selection, and the normal Level Editor Actor Details view.

---

For the native Unreal implementation and limitations, [read Technical](TECHNICAL.md).
