# FAQ, Updates and Support — Technical

**Prune v1.0.0 · Unreal Engine 5.8.x · Win64 · Editor-only**

## Release baseline

The plugin descriptor declares `VersionName: "1.0.0"`, `Version: 26`, `EngineVersion: "5.8.0"`, an Editor module and Win64. The numeric internal descriptor `Version` field is not a public release count or new feature milestone.

## Upgrade boundaries

Custom filters and overrides are persisted through editor user configuration, not through the plugin's installed directory. Overwriting a plugin folder is not a substitute for resetting a specific user's filter state.

## Portability

Portable format version 1 is distinct from the plugin's public version 1.0.0. New import/export schemas should be versioned independently rather than silently changing the interpretation of existing presets.

## Support evidence

For a source-level bug, record UE and plugin versions, Editor/Slate stack or Output Log, Actor class hierarchy, native/custom filter identity, and whether the issue happens before or after Details refresh. Include the exact `.prunefilters.json` metadata when relevant, but remove personal project information before posting publicly.

## Test limit

No runtime/packaged behavior is shipped. Native Slate customization, editor preference persistence and schema import/export must be tested in the running UE Editor; repository source inspection does not establish passed browser/editor QA.

---

For the corresponding user workflow, [read Design](DESIGN.md).
