# Importing and Exporting Filters — Technical

**Prune v1.0.0 · Editor-only · UE 5.8.x**

## Export format v1

`ExportManagedFiltersToFile` produces a JSON root object with `Format: "PruneFilters"`, `FormatVersion: 1`, `PluginVersion: "1.0.0"` and a `Filters` array. It exports only entries with `EPruneFilterKind::Custom`, not native Epic sections.

## Import validation

`ImportFiltersFromFile` uses `FJsonSerializer` to validate parsing, checks the exact `Format` marker and supports format version **1**. The import counts imported/renamed/skipped entries and reports errors through the native editor dialog.

## Identity and scope migration

Imported definitions receive new GUID-backed internal IDs, avoiding overwriting local presets. Global remains Global; Class-scoped entries are bound to the current destination Actor class.

## Category identifiers

Hidden category IDs and category order are serialized by stable native `FName` identifiers, rather than only user-visible display strings. Missing categories in the destination do not materialize properties.

## File dialogs

`IDesktopPlatform` handles native open/save file dialogs, with `.prunefilters.json` as the recommended extension. Writing and reading remain editor-side file operations.

---

For the practical workflow, [read Design](DESIGN.md).
