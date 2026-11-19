# Project Settings and Appearance — Technical

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

## UDeveloperSettings

`UPruneSettings` is a `UDeveloperSettings` class declared `Config = Prune, DefaultConfig`. It exposes filter editor drag/drop feedback colors and thickness, default editor dimensions, description tooltip visibility, and Filter Manager dimensions.

## Native Slate consumption

`ShowFilterEditor` reads the configured values to build the native Slate dialog and its drag visual feedback. Filter Manager construction likewise reads preferred default sizes. Resizable windows do not require altering persistent filter definitions.

## Independent persistence paths

`UPruneSettings` uses its plugin configuration, whereas `FPruneState` stores user's filter definitions and per-class button-order state through `GEditorPerProjectIni`. This distinction matters for teams that share Project Settings but maintain personal filter layouts.

## Source

[PruneSettings.h](https://github.com/mippi-the-dork/Prune/blob/main/Source/Prune/Public/PruneSettings.h) · [PruneDetailsCustomization.cpp](https://github.com/mippi-the-dork/Prune/blob/main/Source/Prune/Private/PruneDetailsCustomization.cpp).

---

For the corresponding user workflow, [read Design](DESIGN.md).
