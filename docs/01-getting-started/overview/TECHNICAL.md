# What Is Prune? — Technical

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

<!-- Media target: IMAGE | prune-architecture-overview | docs/assets/media/prune-architecture-overview.png -->
<figure class="doc-media" data-kind="image" data-media-id="prune-architecture-overview">
  <img src="../../../assets/media-placeholder.png" alt="Diagram of Prune's native Details panel extension points" loading="lazy">
  <figcaption>Prune adds filter authoring and presentation controls around the native Details layout.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Create a labeled diagram showing the Unreal Property Editor, OnExtendActorDetails, FPruneModule, FPruneState and the native filter row. Show that Prune changes presentation rather than Actor data.</span></figcaption>
</figure>

## Module contract

`Prune.uplugin` declares an Editor-only `Prune` module, `PostEngineInit` loading and a Win64 platform target. `FPruneModule::StartupModule` initializes `FPruneState`, loads `DetailCustomizations`, registers a callback with `OnExtendActorDetails`, and requests `FPropertyEditorModule::NotifyCustomizationModuleChanged()`.

## Native extension rather than replacement

`FPruneDetailsCustomization::ExtendActorDetails` receives an `IDetailLayoutBuilder` and the selected Actors. Prune integrates with existing Property Sections and their native Slate filter row. It does **not** implement a separate Details editor.

## Ownership

`FPruneState` owns custom preset definitions, native overrides, scoped button ordering and per-user persistence. `FPruneLayoutContext` tracks each relevant Details layout and categories discovered after customizations.

## Behavioral boundary

Prune controls category visibility and ordering in the editor's presentation layer. Hiding a category never modifies the Actor property's stored value, editor transaction contents or packaged gameplay.

## Source references

[PruneModule.cpp](https://github.com/mippi-the-dork/Prune/blob/main/Source/Prune/Private/PruneModule.cpp) · [PruneState.h](https://github.com/mippi-the-dork/Prune/blob/main/Source/Prune/Private/PruneState.h).

---

For the step-by-step user workflow, [read Design](DESIGN.md).
