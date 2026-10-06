# Prune

**Take control of Unreal Engine's Actor Details panel with editable category filters.**

Prune extends the standard Level Editor Actor Details filter row so you can create your own category filters, edit Epic's existing filters, reorder what matters, and move useful filter setups between projects.

It works with Unreal's native Details panel instead of replacing it.

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.8.0--5.8.3-black?logo=unrealengine)  
![Platform](https://img.shields.io/badge/Platform-Windows%2064--bit-blue)  
![Type](https://img.shields.io/badge/Plugin-Editor%20Only-green)  
![Version](https://img.shields.io/badge/Version-1.0.0-blue)  
![License](https://img.shields.io/badge/License-MIT-green)

---

## What is Prune?

Unreal's Details panel can become difficult to scan once an Actor exposes a large number of categories.

The built-in Property Section filters help, but they are fixed around the categories Unreal or another plugin registered for them. Different disciplines, Actor types, and workflows often need different views of the same Details panel.

Prune makes those filters editable.

With Prune you can:

- Create custom Details filters.
- Choose exactly which categories a filter shows.
- Reorder categories inside a filter.
- Reorder filter buttons per Actor class.
- Override Epic filters without permanently replacing their original definitions.
- Duplicate Epic filters into independent Prune filters.
- Create Class-scoped or Global filters.
- Add descriptions and searchable organization.
- Import and export portable filter presets.
- Manage filters without replacing Unreal's native Details UI.

Prune is intended to make the Details panel easier to work in, not turn it into a separate property editor.

---

# Features

### Custom Details Filters

Create your own named filters directly from the Actor Details filter row.

Choose which discovered categories are visible, then save the filter for later use.

### Editable Epic Filters

Prune can edit existing Epic Property Section filters such as **General**, **Actor**, **LOD**, **Physics**, and **Rendering**.

Those edits are stored as Prune overrides. Epic's underlying filter definition remains available and can be restored with **Reset**.

### Class and Global Scope

Custom filters and Epic overrides can be scoped to:

- **Class**: applies to the current Actor class and follows Unreal's normal derived-class Property Section behavior.
- **Global**: available across Actor classes.

### Category Reordering

Drag categories inside the filter editor to control the order they appear while that filter is active.

Prune preserves categories it has not encountered yet and keeps Unreal's native ordering when no custom order is required.

### Filter Button Reordering

Open the **Filter Manager** and drag filters into the order you want for the current Actor class.

The **All** filter and Unreal-owned helper sections retain their native behavior.

### Filter Manager

The Filter Manager provides one place to search and manage the filters available to the current Actor class.

Each row shows useful state at a glance, including:

- Epic or Prune identity.
- Global or Class scope.
- Epic Override state.
- Active state.
- Visible category count.

Common actions such as Edit, Duplicate, Reset, and Delete are available directly from the manager where appropriate.

### Filter Button Context Menus

Right-click an editable filter button in Actor Details for context-sensitive actions.

Depending on the filter, actions can include:

- **Edit**
- **Duplicate**
- **Export**
- **Delete**
- **Reset**
- **Filter Manager**
- **Settings**

Normal left-click and Ctrl-click filter selection remain owned by Unreal's native filter buttons.

### Duplicate Filters

Duplicate custom Prune filters or Epic filters into new independent Prune filters.

Duplicate names are collision-safe and use names such as `Copy`, `Copy 2`, and so on when needed.

### Descriptions

Filters can store an optional description.

Descriptions appear in the Filter Manager and can also appear as filter-button tooltips.

### Portable Import and Export

Export custom Prune filters to `.prunefilters.json` files and import them into another project or Actor class.

Exports preserve:

- Filter name.
- Description.
- Global or Class scope.
- Hidden category IDs.
- Category order.

Epic filters are not exported directly because their definitions are engine-owned. Duplicate an Epic filter first when you want to turn it into a portable Prune preset.

Import validates the file format before changing local filters and safely reports malformed, empty, partial, or unsupported files.

### Project Settings

Prune adds **Project Settings > Plugins > Prune** for editor appearance and window defaults.

Available settings include:

- Reorder drag color.
- Dragged content color.
- Drop line color.
- Drop line thickness.
- Default filter editor width and height.
- Default Filter Manager width and height.
- Description tooltip visibility.

### Persistent Per-Project Editor State

Prune stores its editor-only data in `EditorPerProjectUserSettings.ini`.

Saved state includes custom filters, descriptions, scope, category visibility, category order, Epic overrides, and per-class filter-button ordering.

### Plugin Category Support

Prune discovers the finished Actor Details category map after customizations have run, allowing plugin-added categories to participate when they are present in the Details layout.

### Editor Only

Prune is an editor workflow plugin.

It does not add runtime systems to packaged games and does not require engine source modifications.

---

# Using Prune

Prune integrates into the standard Actor Details filter area in the Level Editor.

## Create a Filter

1. Select an Actor in the Level Editor.
2. Open its Details panel.
3. Click the Prune **+** control beside the native filter row.
4. Name the filter.
5. Choose **Class** or **Global** scope.
6. Show or hide the categories you want.
7. Drag categories to set their order if desired.
8. Save the filter.

The new filter appears beside Unreal's normal Property Section filters.

---

## Edit a Filter

Use the Prune edit control or right-click an editable filter button and choose **Edit**.

For a custom Prune filter, the saved filter is updated directly.

For an Epic filter, Prune creates or updates an override while retaining Epic's original definition underneath it.

---

## Reset an Epic Filter

If an Epic filter has a Prune override, use **Reset** from the Filter Manager or the filter's context menu.

Reset removes the Prune override and restores Epic's original filter behavior.

---

## Reorder Filter Buttons

1. Open the **Filter Manager**.
2. Drag filter rows into the desired order.
3. Click **Done** when finished.

Changes apply immediately and are remembered per Actor class.

Use **Reset Button Order** to return the class to Unreal's normal filter ordering.

---

## Import and Export Filters

Open the **Filter Manager** and use **Import** or **Export**.

Export writes the custom Prune filters available to the current Actor class into a `.prunefilters.json` file.

When importing:

- Global filters remain Global.
- Class-scoped filters are mapped to the Actor class currently being managed.
- Name collisions create a new collision-safe Copy name instead of overwriting an existing filter.
- Imported filters receive new internal IDs.

This makes filter presets easy to move between projects or share with teammates without coupling the file to a specific project configuration.

---

# Example Workflow

Imagine a gameplay Actor exposes categories for rendering, collision, movement, interaction, debug data, audio, networking, components, and project-specific systems.

A gameplay designer may only care about a subset of those categories during normal iteration.

With Prune you could create a **Gameplay** filter that shows only:

- Transform
- Movement
- Interaction
- Collision
- Gameplay

An environment artist could create a different filter for the same Actor family that focuses on rendering and materials.

The Actor stays unchanged. Prune only changes how its existing Details categories are presented in the editor.

---

# Installation

Prune can be installed through **Fab**, from a **GitHub Release**, or directly from the **GitHub source**.

For most users, Fab or a prepared GitHub Release is recommended.

## Fab / Epic Games Launcher

1. Add **Prune** to your library on Fab.
2. Open the **Epic Games Launcher**.
3. Locate Prune in your Fab / Vault library.
4. Install it to a supported Unreal Engine version.
5. Launch your project.
6. Open **Edit > Plugins**.
7. Search for **Prune**.
8. Enable the plugin if necessary.
9. Restart Unreal Editor if prompted.

## GitHub Release

Download the latest release from:

https://github.com/mippi-the-dork/Prune/releases

Close Unreal Editor, then extract the `Prune` folder into your project's `Plugins` directory:

```text
YourProject/
├── Plugins/
│   └── Prune/
│       ├── Resources/
│       ├── Source/
│       ├── README.md
│       └── Prune.uplugin
└── YourProject.uproject
```

Launch the project, enable Prune under **Edit > Plugins** if necessary, and restart when prompted.

## GitHub Source

Building Prune from source requires a working Unreal Engine C++ development environment.

```bash
cd YourProject/Plugins
git clone https://github.com/mippi-the-dork/Prune.git
```

Generate project files if necessary, build your project's Editor target, then launch Unreal Editor.

---

# Updating Prune

For manual release installations:

1. Close Unreal Editor.
2. Remove the existing `Plugins/Prune` folder.
3. Extract the new release into `Plugins`.
4. Reopen the project.

Replacing the complete plugin folder is recommended instead of copying individual release files over an older version.

For Git source installations, pull the latest changes and rebuild when source files have changed.

---

# Compatibility

| | |
|---|---|
| **Prune Version** | 1.0.0 |
| **Unreal Engine** | 5.8.0 - 5.8.3 |
| **Platform** | Windows 64-bit |
| **Plugin Type** | Editor Only |
| **Runtime Dependency** | None |
| **Packaged Game Impact** | None |
| **Engine Source Changes** | None |

Compatibility with additional Unreal Engine versions or platforms should not be assumed unless explicitly listed in a release.

---

# How Prune Works

Prune integrates with Unreal's existing Actor Details filter area rather than replacing the Details panel.

It uses Unreal's public Details interfaces and Property Section system to discover the categories available to the current Actor layout, register custom sections, apply saved visibility, and reorder categories when required.

Prune keeps Unreal's native filter widget in place and adds its own controls beside it. This allows Unreal to continue rebuilding and managing the native filter row normally.

Persistent Prune data is editor-only and stored separately from Actor gameplay data.

---

# What Prune Does Not Do

Prune is a **Details presentation and workflow tool**.

It does not:

- Remove properties or categories from an Actor class.
- Change gameplay behavior.
- Modify packaged builds.
- Replace Unreal's Details panel.
- Replace Unreal's Property Section system.
- Change Blueprint Class Defaults.
- Change Component Details panels.
- Force every Actor class to use the same filter layout.

---

# Current Limitations

- Prune currently targets **Level Editor instance Actor Details**.
- Component Details and Blueprint Class Defaults are not integrated in 1.0.0.
- An Epic override can only explicitly modify categories Prune has encountered while editing that override. Newly encountered categories retain their underlying Unreal behavior until changed.
- A custom filter containing no populated category for the current Actor may not appear because Unreal only displays Property Sections mapped to populated categories.

---

# Troubleshooting

## Prune Controls Do Not Appear

Check **Edit > Plugins**, search for **Prune**, and confirm the plugin is enabled.

Prune currently integrates with Level Editor Actor Details, so make sure you are inspecting an Actor instance rather than Blueprint Class Defaults or another Details surface.

## A Category Is Missing From the Filter Editor

Prune discovers categories present in the finished Details layout for the current Actor.

If a category is conditional, plugin-provided, or only appears for a particular component or state, make sure that category is actually present on the Actor being edited.

## A Filter Does Not Appear For an Actor

Unreal only displays Property Sections that map to populated categories.

A filter with no populated category for the current Actor may therefore not appear in the native filter row.

## Imported Filters Were Renamed

Prune never overwrites an existing filter with the same name during import.

Collisions receive names such as `Copy`, `Copy 2`, and so on.

---

# Reporting Bugs

If you encounter a problem, please open an issue:

https://github.com/mippi-the-dork/Prune/issues

Include:

- Prune version.
- Unreal Engine version.
- Windows version.
- Whether Prune came from Fab, a GitHub Release, or source.
- Actor class involved.
- Whether the filter was Epic, an Epic Override, or a custom Prune filter.
- Steps to reproduce the problem.
- Screenshots or video when relevant.
- Relevant Unreal Editor log output when available.

---

# Feature Requests

Feature requests are welcome through GitHub Issues.

Describe the workflow problem you are trying to solve, not only the implementation you would like to see. That makes it easier to decide whether the feature belongs in Prune and whether there may be a smaller solution.

---

# Contributions

Pull requests are welcome.

For significant changes, opening an Issue first is recommended so the direction can be discussed before substantial work is done.

Prune is intended to remain a focused Details-panel workflow utility, so additions should support that purpose without turning it into a replacement property editor.

---

# License

Prune is distributed under the **MIT License**.

---

# About

Prune is an Unreal Engine editor utility created by **Mippi the Dork**.

It was built around a simple idea:

> The Details panel should show the information you need for the job you're doing now.
