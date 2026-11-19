# Prune Documentation Audit

**Baseline:** Public Prune v1.0.0 source/README · Unreal Engine 5.8.x Win64.  
**Status:** Source-based documentation review. The documentation website and plugin have not been independently tested in a live Unreal Editor here.

## What was reviewed

- `Prune.uplugin` and native module lifecycle.
- `FPruneState`, `FPruneLayoutContext` and their saved filters/native overrides.
- `FPruneDetailsCustomization`, Filter Manager, New/Edit dialogs, native filter buttons and portable import/export.
- `UPruneSettings` and the README's installation and feature descriptions.

## Strengths

The current release has a relatively cohesive feature set: Prune builds on native Actor Details, supports Class/Global scope, integrates newly discovered categories, and preserves Epic's section definitions behind reversible overrides. Source ownership is concentrated in the module, state, customization and settings files. These map naturally to distinct Design and Technical guides.

## Remaining gaps and recommended improvements

| Priority | Gap | Action | Status |
| --- | --- | --- | --- |
| P1 | Authentic UI examples | Capture the prescribed screenshots/GIFs for filter editing, category ordering, Manager, scopes, import/export and settings | **Media slots placed; captures outstanding** |
| P1 | Live editor validation | Check each documented workflow in UE 5.8.x, including Epic overrides, class inheritance and Ctrl-multiple native filters | **Not run** |
| P1 | Import safety and portability | Validate format v1 rejection, partial/malformed JSON, scope remapping and name collisions on real files | **Not run** |
| P1 | Editor persistence | Verify per-project user config across restart, class-specific orders, and behavior when changing selected Actors | **Not run** |
| P1 | Website QA | Verify browser search, mobile sidebar, Markdown figures, captions and keyboard navigation | **Not run** |
| P2 | Source version pinning | Replace links to moving `main` with a release tag/commit when a stable tag is available | **Planned** |
| P2 | Accessibility | Test keyboard-only access, focus indicators, legibility of drag colors and small-window controls | **Not run** |
| P2 | Historical release provenance | Add verified changelog history as real releases are documented; do not invent past features | **Ongoing maintenance** |

## Page quality rules

Each **Design** page answers what the user can do, how to do it, what happens, when an option is unavailable, and how to reverse the action. Each **Technical** page identifies the real C++ owner and native integration and does not duplicate the user's step-by-step instructions.

Screenshots and GIFs use the supplied placeholder until real Prune editor captures exist. Each slot has a reader-facing caption followed by explicit capture instructions. Keep the figure types and names stable when replacing the assets; see the [Media Inventory](MEDIA_INVENTORY.md).

## Important accuracy boundaries

A filter is not a data transformation. Category visibility and order should never be described as modifying Actor property values. Epic overrides are reversible and distinct from custom presets. Global scope governs the availability of a filter button, not whether a specific Actor possesses every category. The editor-user configuration is distinct from portable JSON exports and Actor packages.

## Next quality pass

1. Run the [Verification Matrix](VERIFICATION_MATRIX.md) in a clean UE 5.8.x editor project.
2. Capture real editor screenshots/GIFs matching the inventory; avoid fabricated UI.
3. Test the website in desktop/mobile browsers and mark observed results honestly.
4. Correct any source-versus-editor discrepancies before labeling the manual fully verified.
