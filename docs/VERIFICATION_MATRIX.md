# Prune Verification Matrix

**Target:** Prune v1.0.0 · UE 5.8.x · Win64 Editor.  
**Status:** These are test specifications, **not a passing test report**. Record exact UE build, Prune commit/release, project, tester, date, reproduction steps, logs and screenshots for each executed test.

## Native Actor Details workflows

| ID | Test | Expected behavior | Status |
| --- | --- | --- | --- |
| PRN-01 | Enable plugin and select an Actor | Prune New, Manager and Edit controls appear in native Actor Details | Not run |
| PRN-02 | Create Class filter with name/description | Button available on corresponding Actor family | Not run |
| PRN-03 | Create Global filter | Filter available across multiple compatible Actor classes | Not run |
| PRN-04 | Edit category checkbox list | Only selected Details categories appear; property values unchanged | Not run |
| PRN-05 | Search categories while authoring | Search narrows the list but does not toggle the stored checkboxes | Not run |
| PRN-06 | Drag category rows, Save, activate filter | Authored category order appears with native drag/drop feedback | Not run |
| PRN-07 | Add new plugin/Blueprint Details categories after saving a filter | Newly discovered categories aren't silently deleted | Not run |
| PRN-08 | Duplicate visible category labels | Grouped presentation maps to appropriate internal IDs | Not run |
| PRN-09 | Edit an Epic built-in filter | Prune override changes effective section, original Epic source retained | Not run |
| PRN-10 | Reset a modified Epic section | Resets to Epic Default and removes Prune override | Not run |
| PRN-11 | Duplicate Epic filter | Independent editable, renameable custom preset created | Not run |
| PRN-12 | Click All and Ctrl-select multiple filters | Native visibility semantics retained; Edit disabled for ambiguous active filters | Not run |
| PRN-13 | Use Manager search and edit/delete controls | Appropriate custom/native operations offered, invalid operations unavailable | Not run |
| PRN-14 | Reorder filter buttons and Reset Button Order | Per-class button order saved, reset restores native order | Not run |
| PRN-15 | Right-click custom/Epic filter buttons | Correct contextual actions for each ownership type | Not run |
| PRN-16 | Export one and multiple custom filters | Portable Prune format v1 JSON generated; Epic native not directly exported | Not run |
| PRN-17 | Import into another Actor class | Class-scoped presets remap, Global remains Global, IDs regenerated | Not run |
| PRN-18 | Import colliding names | No silent overwrite; collision-safe names generated | Not run |
| PRN-19 | Import invalid JSON, unsupported version, empty/partial entries | Error/skipped entries handled without corrupting existing filter state | Not run |
| PRN-20 | Restart editor with saved filters/overrides/order | Editor-per-project user state restored | Not run |
| PRN-21 | Change Project Settings for drag colors, tooltips and window size | Editor UI respects settings and windows remain resizable | Not run |
| PRN-22 | Disable/re-enable plugin or close during native filter activity | Hooks/native sections cleanly restored; no leaked stale Slate elements | Not run |

## Documentation website

| ID | Test | Expected behavior | Status |
| --- | --- | --- | --- |
| DOC-01 | Open Prune site from GitHub Pages root | Home and five section indexes load | Not run |
| DOC-02 | Browse all 17 Design/Technical pairs | Sidebar, breadcrumbs, switches, next/previous links work | Structural check only |
| DOC-03 | Search for content not in a page title | Lazy full-text index returns matching body excerpt | Source reviewed; browser not run |
| DOC-04 | Verify images and caption/instruction pairs | All placeholder figures render within article; no overflow | Source reviewed; browser not run |
| DOC-05 | Test mobile widths, long tables and sidebar scroll | No clipping or inaccessible navigation | Not run |
| DOC-06 | Navigate via keyboard and screen reader | Focus order, search shortcuts and controls remain accessible | Not run |
| DOC-07 | Verify real Prune hero and plugin icon in header | Correct branding and image rendering | Source checked; browser not run |

## Recording outcomes

Use **Passed**, **Failed**, **Blocked**, or **Not applicable** only after executing a test. A source review or JavaScript syntax check is not an observed UE/browser success. Attach test logs, exact versions, screenshots, and links to any issue.

## Release exit criteria

All core workflows, import/export, native override reset and per-user persistence should have recorded evidence. All documentation media and key browser layouts should be checked before claiming the manual is fully verified.
